/*
** NetXMS - Network Management System
** Copyright (C) 2003-2018 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: syslogd.cpp
**
**/

#include "nxcore.h"
#include <nxlog.h>
#include <nxlpapi.h>

#define DEBUG_TAG _T("syslog")

/**
 * Max syslog message length
 */
#define MAX_SYSLOG_MSG_LEN    1024

/**
 * Queued syslog message structure
 */
class QueuedSyslogMessage
{
public:
   InetAddress sourceAddr;
   time_t timestamp;
   UINT32 zoneUIN;
   UINT32 nodeId;
   char *message;
   int messageLength;

   QueuedSyslogMessage(const InetAddress& addr, const char *msg, int msgLen) : sourceAddr(addr)
   {
      message = (char *)nx_memdup(msg, msgLen + 1);
      messageLength = msgLen;
      timestamp = time(NULL);
      zoneUIN = 0;
      nodeId = 0;
   }

   QueuedSyslogMessage(const InetAddress& addr, time_t t, UINT32 zuin, UINT32 nid, const char *msg, int msgLen) : sourceAddr(addr)
   {
      message = (char *)nx_memdup(msg, msgLen + 1);
      messageLength = msgLen;
      timestamp = t;
      zoneUIN = zuin;
      nodeId = nid;
   }

   ~QueuedSyslogMessage()
   {
      free(message);
   }
};

/**
 * Queues
 */
Queue g_syslogProcessingQueue(1000, 100);
Queue g_syslogWriteQueue(1000, 100);

/**
 * Total number of received syslog messages
 */
VolatileCounter64 g_syslogMessagesReceived = 0;

/**
 * Node matching policy
 */
enum NodeMatchingPolicy
{
   SOURCE_IP_THEN_HOSTNAME = 0,
   HOSTNAME_THEN_SOURCE_IP = 1
};

/**
 * Static data
 */
static UINT64 s_msgId = 1;
static LogParser *s_parser = NULL;
static MUTEX s_parserLock = INVALID_MUTEX_HANDLE;
static NodeMatchingPolicy s_nodeMatchingPolicy = SOURCE_IP_THEN_HOSTNAME;
static THREAD s_receiverThread = INVALID_THREAD_HANDLE;
static THREAD s_processingThread = INVALID_THREAD_HANDLE;
static THREAD s_writerThread = INVALID_THREAD_HANDLE;
static bool s_running = true;
static bool s_alwaysUseServerTime = false;

/**
 * Parse timestamp field
 */
static BOOL ParseTimeStamp(char **ppStart, int nMsgSize, int *pnPos, time_t *ptmTime)
{
   static char psMonth[12][5] = { "Jan ", "Feb ", "Mar ", "Apr ",
                                  "May ", "Jun ", "Jul ", "Aug ",
                                  "Sep ", "Oct ", "Nov ", "Dec " };
   struct tm timestamp;
   time_t t;
   char szBuffer[16], *pCurr = *ppStart;
   int i;

   if (nMsgSize - *pnPos < 16)
      return FALSE;  // Timestamp cannot be shorter than 16 bytes

   // Prepare local time structure
   t = time(NULL);
#if HAVE_LOCALTIME_R
   localtime_r(&t, &timestamp);
#else
   memcpy(&timestamp, localtime(&t), sizeof(struct tm));
#endif

   // Month
   for(i = 0; i < 12; i++)
      if (!memcmp(pCurr, psMonth[i], 4))
      {
         timestamp.tm_mon = i;
         break;
      }
   if (i == 12)
      return FALSE;
   pCurr += 4;

   // Day of week
   if (isdigit(*pCurr))
   {
      timestamp.tm_mday = *pCurr - '0';
   }
   else
   {
      if (*pCurr != ' ')
         return FALSE;  // Invalid day of month
      timestamp.tm_mday = 0;
   }
   pCurr++;
   if (isdigit(*pCurr))
   {
      timestamp.tm_mday = timestamp.tm_mday * 10 + (*pCurr - '0');
   }
   else
   {
      return FALSE;  // Invalid day of month
   }
   pCurr++;
   if (*pCurr != ' ')
      return FALSE;
   pCurr++;

   // HH:MM:SS
   memcpy(szBuffer, pCurr, 8);
   szBuffer[8] = 0;
   if (sscanf(szBuffer, "%02d:%02d:%02d", &timestamp.tm_hour,
              &timestamp.tm_min, &timestamp.tm_sec) != 3)
      return FALSE;  // Invalid time format
   pCurr += 8;

	// Check for Cisco variant - HH:MM:SS.nnn
	if (*pCurr == '.')
	{
		pCurr++;
		if (isdigit(*pCurr))
			pCurr++;
		if (isdigit(*pCurr))
			pCurr++;
		if (isdigit(*pCurr))
			pCurr++;
	}

   if (*pCurr != ' ')
      return FALSE;  // Space should follow timestamp
   pCurr++;

   // Convert to system time
   *ptmTime = mktime(&timestamp);
   if (*ptmTime == ((time_t)-1))
      return FALSE;

   // Adjust current position
   *pnPos += (int)(pCurr - *ppStart);
   *ppStart = pCurr;
   return TRUE;
}

/**
 * Parse syslog message
 */
static BOOL ParseSyslogMessage(char *psMsg, int nMsgLen, time_t receiverTime, NX_SYSLOG_RECORD *pRec)
{
   int i, nLen, nPos = 0;
   char *pCurr = psMsg;

   memset(pRec, 0, sizeof(NX_SYSLOG_RECORD));

   // Parse PRI part
   if (*psMsg == '<')
   {
      int nPri = 0, nCount = 0;

      for(pCurr++, nPos++; isdigit(*pCurr) && (nPos < nMsgLen); pCurr++, nPos++, nCount++)
         nPri = nPri * 10 + (*pCurr - '0');
      if (nPos >= nMsgLen)
         return FALSE;  // Unexpected end of message

      if ((*pCurr == '>') && (nCount > 0) && (nCount <4))
      {
         pRec->nFacility = nPri / 8;
         pRec->nSeverity = nPri % 8;
         pCurr++;
         nPos++;
      }
      else
      {
         return FALSE;  // Invalid message
      }
   }
   else
   {
      // Set default PRI of 13
      pRec->nFacility = 1;
      pRec->nSeverity = SYSLOG_SEVERITY_NOTICE;
   }

   // Parse HEADER part
   if (ParseTimeStamp(&pCurr, nMsgLen, &nPos, &pRec->tmTimeStamp))
   {
      // Use server time if configured
      // We still had to parse timestamp to get correct start position for MSG part
      if (s_alwaysUseServerTime)
      {
         pRec->tmTimeStamp = receiverTime;
      }

      // Hostname
      for(i = 0; (*pCurr >= 33) && (*pCurr <= 126) && (i < MAX_SYSLOG_HOSTNAME_LEN - 1) && (nPos < nMsgLen); i++, nPos++, pCurr++)
         pRec->szHostName[i] = *pCurr;
      if ((nPos >= nMsgLen) || (*pCurr != ' '))
      {
         // Not a valid hostname, assuming to be a part of message
         pCurr -= i;
         nPos -= i;
         pRec->szHostName[0] = 0;
      }
      else
      {
         pCurr++;
         nPos++;
      }
   }
   else
   {
      pRec->tmTimeStamp = receiverTime;
   }

   // Parse MSG part
   for(i = 0; isalnum(*pCurr) && (i < MAX_SYSLOG_TAG_LEN) && (nPos < nMsgLen); i++, nPos++, pCurr++)
      pRec->szTag[i] = *pCurr;
   if ((i == MAX_SYSLOG_TAG_LEN) || (nPos >= nMsgLen))
   {
      // Too long tag, assuming that it's a part of message
      pRec->szTag[0] = 0;
   }
   pCurr -= i;
   nPos -= i;
   nLen = std::min(nMsgLen - nPos, MAX_LOG_MSG_LENGTH);
   memcpy(pRec->szMessage, pCurr, nLen);

   return TRUE;
}

/**
 * Find node by host name
 */
static Node *FindNodeByHostname(const char *hostName, UINT32 zoneUIN)
{
   if (hostName[0] == 0)
      return NULL;

   Node *node = NULL;
   InetAddress ipAddr = InetAddress::resolveHostName(hostName);
	if (ipAddr.isValidUnicast())
   {
      node = FindNodeByIP(zoneUIN, (g_flags & AF_TRAP_SOURCES_IN_ALL_ZONES) != 0, ipAddr);
   }

   if (node == NULL)
	{
#ifdef UNICODE
		WCHAR wname[MAX_OBJECT_NAME];
		MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, hostName, -1, wname, MAX_OBJECT_NAME);
		wname[MAX_OBJECT_NAME - 1] = 0;
		node = (Node *)FindObjectByName(wname, OBJECT_NODE);
#else
		node = (Node *)FindObjectByName(hostName, OBJECT_NODE);
#endif
   }
   return node;
}

/**
 * Bind syslog message to NetXMS node object
 * sourceAddr is an IP address from which we receive message
 */
static Node *BindMsgToNode(NX_SYSLOG_RECORD *pRec, const InetAddress& sourceAddr, UINT32 zoneUIN, UINT32 nodeId)
{
   nxlog_debug_tag(DEBUG_TAG, 6, _T("BindMsgToNode: addr=%s zoneUIN=%d"), (const TCHAR *)sourceAddr.toString(), zoneUIN);

   Node *node = NULL;
   if (nodeId != 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("BindMsgToNode: node ID explicitly set to %d"), nodeId);
      node = (Node *)FindObjectById(nodeId, OBJECT_NODE);
   }
   else if (sourceAddr.isLoopback() && (zoneUIN == 0))
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("BindMsgToNode: source is loopback in default zone, binding to management node (ID %d)"), g_dwMgmtNode);
      node = (Node *)FindObjectById(g_dwMgmtNode, OBJECT_NODE);
   }
   else if (s_nodeMatchingPolicy == SOURCE_IP_THEN_HOSTNAME)
   {
      node = FindNodeByIP(zoneUIN, (g_flags & AF_TRAP_SOURCES_IN_ALL_ZONES) != 0, sourceAddr);
      if (node == NULL)
      {
         node = FindNodeByHostname(pRec->szHostName, zoneUIN);
      }
   }
   else
   {
      node = FindNodeByHostname(pRec->szHostName, zoneUIN);
      if (node == NULL)
      {
         node = FindNodeByIP(zoneUIN, (g_flags & AF_TRAP_SOURCES_IN_ALL_ZONES) != 0, sourceAddr);
      }
   }

	if (node != NULL)
   {
	   node->incSyslogMessageCount();
      pRec->dwSourceObject = node->getId();
      pRec->zoneUIN = zoneUIN;
      if (pRec->szHostName[0] == 0)
		{
#ifdef UNICODE
			WideCharToMultiByte(CP_ACP, WC_DEFAULTCHAR | WC_COMPOSITECHECK, node->getName(), -1, pRec->szHostName, MAX_SYSLOG_HOSTNAME_LEN, NULL, NULL);
			pRec->szHostName[MAX_SYSLOG_HOSTNAME_LEN - 1] = 0;
#else
         nx_strncpy(pRec->szHostName, node->getName(), MAX_SYSLOG_HOSTNAME_LEN);
#endif
		}
   }
   else
   {
      if (pRec->szHostName[0] == 0)
      {
         sourceAddr.toStringA(pRec->szHostName);
      }
   }

   return node;
}

/**
 * Handler for EnumerateSessions()
 */
static void BroadcastSyslogMessage(ClientSession *pSession, void *pArg)
{
   if (pSession->isAuthenticated())
      pSession->onSyslogMessage((NX_SYSLOG_RECORD *)pArg);
}

/**
 * Syslog writer thread
 */
static THREAD_RESULT THREAD_CALL SyslogWriterThread(void *arg)
{
   ThreadSetName("SyslogWriter");
   nxlog_debug_tag(DEBUG_TAG, 1, _T("Syslog writer thread started"));
   int maxRecords = ConfigReadInt(_T("DBWriter.MaxRecordsPerTransaction"), 1000);
   while(true)
   {
      NX_SYSLOG_RECORD *r = (NX_SYSLOG_RECORD *)g_syslogWriteQueue.getOrBlock();
      if (r == INVALID_POINTER_VALUE)
         break;

      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

      DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO syslog (msg_id,msg_timestamp,facility,severity,source_object_id,zone_uin,hostname,msg_tag,msg_text) VALUES (?,?,?,?,?,?,?,?,?)"), true);
      if (hStmt == NULL)
      {
         free(r);
         DBConnectionPoolReleaseConnection(hdb);
         continue;
      }

      int count = 0;
      DBBegin(hdb);
      while(true)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_BIGINT, r->qwMsgId);
         DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, (INT32)r->tmTimeStamp);
         DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, r->nFacility);
         DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, r->nSeverity);
         DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, r->dwSourceObject);
         DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, r->zoneUIN);
#ifdef UNICODE
         DBBind(hStmt, 7, DB_SQLTYPE_VARCHAR, WideStringFromMBString(r->szHostName), DB_BIND_DYNAMIC);
         DBBind(hStmt, 8, DB_SQLTYPE_VARCHAR, WideStringFromMBString(r->szTag), DB_BIND_DYNAMIC);
         DBBind(hStmt, 9, DB_SQLTYPE_VARCHAR, WideStringFromMBString(r->szMessage), DB_BIND_DYNAMIC);
#else
         DBBind(hStmt, 7, DB_SQLTYPE_VARCHAR, r->szHostName, DB_BIND_STATIC);
         DBBind(hStmt, 8, DB_SQLTYPE_VARCHAR, r->szTag, DB_BIND_STATIC);
         DBBind(hStmt, 9, DB_SQLTYPE_VARCHAR, r->szMessage, DB_BIND_STATIC);
#endif

         if (!DBExecute(hStmt))
         {
            free(r);
            break;
         }
         free(r);
         count++;
         if (count == maxRecords)
            break;
         r = (NX_SYSLOG_RECORD *)g_syslogWriteQueue.get();
         if ((r == NULL) || (r == INVALID_POINTER_VALUE))
            break;
      }
      DBCommit(hdb);
      DBFreeStatement(hStmt);
      DBConnectionPoolReleaseConnection(hdb);
      if (r == INVALID_POINTER_VALUE)
         break;
   }
   nxlog_debug_tag(DEBUG_TAG, 1, _T("Syslog writer thread stopped"));
   return THREAD_OK;
}

/**
 * Process syslog message
 */
static void ProcessSyslogMessage(QueuedSyslogMessage *msg)
{
   NX_SYSLOG_RECORD record;

	nxlog_debug_tag(DEBUG_TAG, 6, _T("ProcessSyslogMessage: Raw syslog message to process:\n%hs"), msg->message);
   if (ParseSyslogMessage(msg->message, msg->messageLength, msg->timestamp, &record))
   {
      InterlockedIncrement64(&g_syslogMessagesReceived);

      record.qwMsgId = s_msgId++;
      Node *node = BindMsgToNode(&record, msg->sourceAddr, msg->zoneUIN, msg->nodeId);

      g_syslogWriteQueue.put(nx_memdup(&record, sizeof(NX_SYSLOG_RECORD)));

      // Send message to all connected clients
      EnumerateClientSessions(BroadcastSyslogMessage, &record);

		TCHAR ipAddr[64];
		nxlog_debug_tag(DEBUG_TAG, 6, _T("Syslog message: ipAddr=%s zone=%d objectId=%d tag=\"%hs\" msg=\"%hs\""),
		            msg->sourceAddr.toString(ipAddr), msg->zoneUIN, record.dwSourceObject, record.szTag, record.szMessage);

		MutexLock(s_parserLock);
		if ((record.dwSourceObject != 0) && (s_parser != NULL) &&
          ((node->getStatus() != STATUS_UNMANAGED) || (g_flags & AF_TRAPS_FROM_UNMANAGED_NODES)))
		{
#ifdef UNICODE
			WCHAR wtag[MAX_SYSLOG_TAG_LEN];
			WCHAR wmsg[MAX_LOG_MSG_LENGTH];
			MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, record.szTag, -1, wtag, MAX_SYSLOG_TAG_LEN);
			MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, record.szMessage, -1, wmsg, MAX_LOG_MSG_LENGTH);
			s_parser->matchEvent(wtag, record.nFacility, 1 << record.nSeverity, wmsg, NULL, 0, record.dwSourceObject);
#else
			s_parser->matchEvent(record.szTag, record.nFacility, 1 << record.nSeverity, record.szMessage, NULL, 0, record.dwSourceObject);
#endif
		}
		MutexUnlock(s_parserLock);

	   if ((record.dwSourceObject == 0) && (g_flags & AF_SYSLOG_DISCOVERY))  // unknown node, discovery enabled
	   {
	      nxlog_debug_tag(DEBUG_TAG, 4, _T("ProcessSyslogMessage: source not matched to node, adding new IP address %s for discovery"), msg->sourceAddr.toString(ipAddr));
	      CheckPotentialNode(msg->sourceAddr, msg->zoneUIN);
	   }
   }
	else
	{
		nxlog_debug_tag(DEBUG_TAG, 6, _T("ProcessSyslogMessage: Cannot parse syslog message"));
	}
}

/**
 * Syslog processing thread
 */
static THREAD_RESULT THREAD_CALL SyslogProcessingThread(void *pArg)
{
   ThreadSetName("SyslogProcessor");
   while(true)
   {
      QueuedSyslogMessage *msg = (QueuedSyslogMessage *)g_syslogProcessingQueue.getOrBlock();
      if (msg == INVALID_POINTER_VALUE)
         break;

      ProcessSyslogMessage(msg);
      delete msg;
   }
   return THREAD_OK;
}

/**
 * Queue syslog message for processing
 */
static void QueueSyslogMessage(char *msg, int msgLen, const InetAddress& sourceAddr)
{
   g_syslogProcessingQueue.put(new QueuedSyslogMessage(sourceAddr, msg, msgLen));
}

/**
 * Queue proxied syslog message for processing
 */
void QueueProxiedSyslogMessage(const InetAddress &addr, UINT32 zoneUIN, UINT32 nodeId, time_t timestamp, const char *msg, int msgLen)
{
   g_syslogProcessingQueue.put(new QueuedSyslogMessage(addr, timestamp, zoneUIN, nodeId, msg, msgLen));
}

/**
 * Callback for syslog parser
 */
static void SyslogParserCallback(UINT32 eventCode, const TCHAR *eventName, const TCHAR *line, const TCHAR *source, UINT32 facility, UINT32 severity,
                                 StringList *captureGroups, StringList *variables, UINT64 recordId, UINT32 objectId, int repeatCount, void *userArg,
                                 const TCHAR *agentAction, const StringList *agentActionArgs)
{
	char format[] = "sssssssssssssssssssssssssssssssss";
	const TCHAR *plist[33];

	nxlog_debug_tag(DEBUG_TAG, 7, _T("Syslog message matched, capture group count = %d, repeat count = %d"), captureGroups->size(), repeatCount);

	int count = std::min(captureGroups->size(), 32);
	format[count + 1] = 0;
	for(int i = 0; i < count; i++)
		plist[i] = captureGroups->get(i);

	TCHAR repeatCountText[16];
   _sntprintf(repeatCountText, 16, _T("%d"), repeatCount);
   plist[count] = repeatCountText;

   PostEvent(eventCode, objectId, format,
	          plist[0], plist[1], plist[2], plist[3],
	          plist[4], plist[5], plist[6], plist[7],
	          plist[8], plist[9], plist[10], plist[11],
	          plist[12], plist[13], plist[14], plist[15],
	          plist[16], plist[17], plist[18], plist[19],
	          plist[20], plist[21], plist[22], plist[23],
	          plist[24], plist[25], plist[26], plist[27],
	          plist[28], plist[29], plist[30], plist[31]);
}

/**
 * Event name resolver
 */
static bool EventNameResolver(const TCHAR *name, UINT32 *code)
{
	bool success = false;
	EventTemplate *event = FindEventTemplateByName(name);
	if (event != NULL)
	{
		*code = event->getCode();
		event->decRefCount();
		success = true;
	}
	return success;
}

/**
 * Create syslog parser from config
 */
static void CreateParserFromConfig()
{
	MutexLock(s_parserLock);
	LogParser *prev = s_parser;
	s_parser = NULL;
#ifdef UNICODE
   char *xml;
	WCHAR *wxml = ConfigReadCLOB(_T("SyslogParser"), _T("<parser></parser>"));
	if (wxml != NULL)
	{
		xml = UTF8StringFromWideString(wxml);
		free(wxml);
	}
	else
	{
		xml = NULL;
	}
#else
	char *xml = ConfigReadCLOB("SyslogParser", "<parser></parser>");
#endif
	if (xml != NULL)
	{
		TCHAR parseError[256];
		ObjectArray<LogParser> *parsers = LogParser::createFromXml(xml, -1, parseError, 256, EventNameResolver);
		if ((parsers != NULL) && (parsers->size() > 0))
		{
			s_parser = parsers->get(0);
			s_parser->setCallback(SyslogParserCallback);
			if (prev != NULL)
			   s_parser->restoreCounters(prev);
			nxlog_debug_tag(DEBUG_TAG, 3, _T("Syslog parser successfully created from config"));
		}
		else
		{
			nxlog_write(MSG_SYSLOG_PARSER_INIT_FAILED, EVENTLOG_ERROR_TYPE, "s", parseError);
		}
		free(xml);
		delete parsers;
	}
	MutexUnlock(s_parserLock);
	delete prev;
}

/**
 * Syslog messages receiver thread
 */
static THREAD_RESULT THREAD_CALL SyslogReceiver(void *pArg)
{
   ThreadSetName("SyslogReceiver");

   SOCKET hSocket = socket(AF_INET, SOCK_DGRAM, 0);
#ifdef WITH_IPV6
   SOCKET hSocket6 = socket(AF_INET6, SOCK_DGRAM, 0);
#endif

#ifdef WITH_IPV6
   if ((hSocket == INVALID_SOCKET) && (hSocket6 == INVALID_SOCKET))
#else
   if (hSocket == INVALID_SOCKET)
#endif
   {
      nxlog_write(MSG_SOCKET_FAILED, EVENTLOG_ERROR_TYPE, "s", _T("SyslogReceiver"));
      return THREAD_OK;
   }

	SetSocketExclusiveAddrUse(hSocket);
	SetSocketReuseFlag(hSocket);
#ifndef _WIN32
   fcntl(hSocket, F_SETFD, fcntl(hSocket, F_GETFD) | FD_CLOEXEC);
#endif

#ifdef WITH_IPV6
   SetSocketExclusiveAddrUse(hSocket6);
   SetSocketReuseFlag(hSocket6);
#ifndef _WIN32
   fcntl(hSocket6, F_SETFD, fcntl(hSocket6, F_GETFD) | FD_CLOEXEC);
#endif
#ifdef IPV6_V6ONLY
   int on = 1;
   setsockopt(hSocket6, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&on, sizeof(int));
#endif
#endif

   // Get listen port number
   int port = ConfigReadInt(_T("SyslogListenPort"), 514);
   if ((port < 1) || (port > 65535))
   {
      nxlog_debug_tag(DEBUG_TAG, 2, _T("Invalid syslog listen port number %d, using default"), port);
      port = 514;
   }

   // Fill in local address structure
   struct sockaddr_in servAddr;
   memset(&servAddr, 0, sizeof(struct sockaddr_in));
   servAddr.sin_family = AF_INET;

#ifdef WITH_IPV6
   struct sockaddr_in6 servAddr6;
   memset(&servAddr6, 0, sizeof(struct sockaddr_in6));
   servAddr6.sin6_family = AF_INET6;
#endif

   if (!_tcscmp(g_szListenAddress, _T("*")))
   {
      servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
#ifdef WITH_IPV6
      memset(servAddr6.sin6_addr.s6_addr, 0, 16);
#endif
   }
   else
   {
      InetAddress bindAddress = InetAddress::resolveHostName(g_szListenAddress, AF_INET);
      if (bindAddress.isValid() && (bindAddress.getFamily() == AF_INET))
      {
         servAddr.sin_addr.s_addr = htonl(bindAddress.getAddressV4());
      }
      else
      {
         servAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
      }
#ifdef WITH_IPV6
      bindAddress = InetAddress::resolveHostName(g_szListenAddress, AF_INET6);
      if (bindAddress.isValid() && (bindAddress.getFamily() == AF_INET6))
      {
         memcpy(servAddr6.sin6_addr.s6_addr, bindAddress.getAddressV6(), 16);
      }
      else
      {
         memset(servAddr6.sin6_addr.s6_addr, 0, 15);
         servAddr6.sin6_addr.s6_addr[15] = 1;
      }
#endif
   }
   servAddr.sin_port = htons((UINT16)port);
#ifdef WITH_IPV6
   servAddr6.sin6_port = htons((UINT16)port);
#endif

   // Bind socket
   TCHAR buffer[64];
   int bindFailures = 0;
   nxlog_debug_tag(DEBUG_TAG, 5, _T("Trying to bind on UDP %s:%d"), SockaddrToStr((struct sockaddr *)&servAddr, buffer), ntohs(servAddr.sin_port));
   if (bind(hSocket, (struct sockaddr *)&servAddr, sizeof(struct sockaddr_in)) != 0)
   {
      nxlog_write(MSG_BIND_ERROR, EVENTLOG_ERROR_TYPE, "dse", port, _T("SyslogReceiver"), WSAGetLastError());
      bindFailures++;
      closesocket(hSocket);
      hSocket = INVALID_SOCKET;
   }

#ifdef WITH_IPV6
   nxlog_debug_tag(DEBUG_TAG, 5, _T("Trying to bind on UDP [%s]:%d"), SockaddrToStr((struct sockaddr *)&servAddr6, buffer), ntohs(servAddr6.sin6_port));
   if (bind(hSocket6, (struct sockaddr *)&servAddr6, sizeof(struct sockaddr_in6)) != 0)
   {
      nxlog_write(MSG_BIND_ERROR, EVENTLOG_ERROR_TYPE, "dse", port, _T("SyslogReceiver"), WSAGetLastError());
      bindFailures++;
      closesocket(hSocket6);
      hSocket6 = INVALID_SOCKET;
   }
#else
   bindFailures++;
#endif

   // Abort if cannot bind to at least one socket
   if (bindFailures == 2)
   {
      nxlog_debug_tag(DEBUG_TAG, 1, _T("Syslog receiver aborted - cannot bind at least one socket"));
      return THREAD_OK;
   }

   if (hSocket != INVALID_SOCKET)
      nxlog_write(MSG_LISTENING_FOR_SYSLOG, EVENTLOG_INFORMATION_TYPE, "ad", ntohl(servAddr.sin_addr.s_addr), port);
#ifdef WITH_IPV6
   if (hSocket6 != INVALID_SOCKET)
      nxlog_write(MSG_LISTENING_FOR_SYSLOG, EVENTLOG_INFORMATION_TYPE, "Hd", servAddr6.sin6_addr.s6_addr, port);
#endif

   SocketPoller sp;

   nxlog_debug_tag(DEBUG_TAG, 1, _T("Syslog receiver thread started"));

   // Wait for packets
   while(s_running)
   {
      sp.reset();
      if (hSocket != INVALID_SOCKET)
         sp.add(hSocket);
#ifdef WITH_IPV6
      if (hSocket6 != INVALID_SOCKET)
         sp.add(hSocket6);
#endif

      int rc = sp.poll(1000);
      if (rc > 0)
      {
         char syslogMessage[MAX_SYSLOG_MSG_LEN + 1];
         SockAddrBuffer addr;
         socklen_t addrLen = sizeof(SockAddrBuffer);
#ifdef WITH_IPV6
         SOCKET s = sp.isSet(hSocket) ? hSocket : hSocket6;
#else
         SOCKET s = hSocket;
#endif
         int bytes = recvfrom(s, syslogMessage, MAX_SYSLOG_MSG_LEN, 0, (struct sockaddr *)&addr, &addrLen);
         if (bytes > 0)
         {
            syslogMessage[bytes] = 0;
            QueueSyslogMessage(syslogMessage, bytes, InetAddress::createFromSockaddr((struct sockaddr *)&addr));
         }
         else
         {
            // Sleep on error
            ThreadSleepMs(100);
         }
      }
      else if (rc == -1)
      {
         // Sleep on error
         ThreadSleepMs(100);
      }
   }

   if (hSocket != INVALID_SOCKET)
      closesocket(hSocket);
#ifdef WITH_IPV6
   if (hSocket6 != INVALID_SOCKET)
      closesocket(hSocket6);
#endif

   nxlog_debug_tag(DEBUG_TAG, 1, _T("Syslog receiver thread stopped"));
   return THREAD_OK;
}

/**
 * Create NXCP message from NX_SYSLOG_RECORD structure
 */
void CreateMessageFromSyslogMsg(NXCPMessage *pMsg, NX_SYSLOG_RECORD *pRec)
{
   UINT32 dwId = VID_SYSLOG_MSG_BASE;

   pMsg->setField(VID_NUM_RECORDS, (UINT32)1);
   pMsg->setField(dwId++, pRec->qwMsgId);
   pMsg->setField(dwId++, (UINT32)pRec->tmTimeStamp);
   pMsg->setField(dwId++, (WORD)pRec->nFacility);
   pMsg->setField(dwId++, (WORD)pRec->nSeverity);
   pMsg->setField(dwId++, pRec->dwSourceObject);
   pMsg->setFieldFromMBString(dwId++, pRec->szHostName);
   pMsg->setFieldFromMBString(dwId++, pRec->szTag);
   pMsg->setFieldFromMBString(dwId++, pRec->szMessage);
}

/**
 * Reinitialize parser on configuration change
 */
void ReinitializeSyslogParser()
{
   if (s_parserLock == INVALID_MUTEX_HANDLE)
      return;  // Syslog daemon not initialized
   CreateParserFromConfig();
}

/**
 * Handler for syslog related configuration changes
 */
void OnSyslogConfigurationChange(const TCHAR *name, const TCHAR *value)
{
   if (!_tcscmp(name, _T("SyslogIgnoreMessageTimestamp")))
   {
      s_alwaysUseServerTime = _tcstol(value, NULL, 0) ? true : false;
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Ignore message timestamp option set to %s"), s_alwaysUseServerTime ? _T("ON") : _T("OFF"));
   }
}

/**
 * Get syslog rule check count in NXSL
 */
int F_GetSyslogRuleCheckCount(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if ((argc != 1) && (argc != 2))
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   if ((argc == 2) && !argv[1]->isInteger() && !argv[1]->isObject(_T("NetObj")))
      return NXSL_ERR_NOT_INTEGER;

   UINT32 objectId = 0;
   if (argc == 2)
   {
      if (argv[1]->isInteger())
      {
         objectId = argv[1]->getValueAsUInt32();
      }
      else
      {
         ((NetObj *)argv[1]->getValueAsObject()->getData())->getId();
      }
   }

   if (s_parserLock == INVALID_MUTEX_HANDLE)
   {
      // Syslog daemon not initialized
      *result = vm->createValue(-1);
      return 0;
   }

   MutexLock(s_parserLock);
   *result = vm->createValue(s_parser->getRuleCheckCount(argv[0]->getValueAsCString(), objectId));
   MutexUnlock(s_parserLock);
   return 0;
}

/**
 * Get syslog rule match count in NXSL
 */
int F_GetSyslogRuleMatchCount(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if ((argc != 1) && (argc != 2))
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if ((argc == 2) && !argv[1]->isInteger() && !argv[1]->isObject(_T("NetObj")))
      return NXSL_ERR_NOT_INTEGER;

   UINT32 objectId = 0;
   if (argc == 2)
   {
      if (argv[1]->isInteger())
      {
         objectId = argv[1]->getValueAsUInt32();
      }
      else
      {
         ((NetObj *)argv[1]->getValueAsObject()->getData())->getId();
      }
   }

   if (s_parserLock == INVALID_MUTEX_HANDLE)
   {
      // Syslog daemon not initialized
      *result = vm->createValue(-1);
      return 0;
   }

   MutexLock(s_parserLock);
   *result = vm->createValue(s_parser->getRuleMatchCount(argv[0]->getValueAsCString(), objectId));
   MutexUnlock(s_parserLock);
   return 0;
}

/**
 * Start built-in syslog server
 */
void StartSyslogServer()
{
   s_nodeMatchingPolicy = (NodeMatchingPolicy)ConfigReadInt(_T("SyslogNodeMatchingPolicy"), SOURCE_IP_THEN_HOSTNAME);
   s_alwaysUseServerTime = ConfigReadBoolean(_T("SyslogIgnoreMessageTimestamp"), false);

   // Determine first available message id
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, _T("SELECT max(msg_id) FROM syslog"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         s_msgId = std::max(DBGetFieldUInt64(hResult, 0, 0) + 1, s_msgId);
      }
      DBFreeResult(hResult);
   }
   DBConnectionPoolReleaseConnection(hdb);

   InitLogParserLibrary();

   // Create message parser
   s_parserLock = MutexCreate();
   CreateParserFromConfig();

   // Start processing thread
   s_processingThread = ThreadCreateEx(SyslogProcessingThread, 0, NULL);
   s_writerThread = ThreadCreateEx(SyslogWriterThread, 0, NULL);

   if (ConfigReadBoolean(_T("EnableSyslogReceiver"), false))
      s_receiverThread = ThreadCreateEx(SyslogReceiver, 0, NULL);
}

/**
 * Stop built-in syslog server
 */
void StopSyslogServer()
{
   s_running = false;
   ThreadJoin(s_receiverThread);

   // Stop processing thread
   g_syslogProcessingQueue.put(INVALID_POINTER_VALUE);
   ThreadJoin(s_processingThread);

   // Stop writer thread - it must be done after processing thread already finished
   g_syslogWriteQueue.put(INVALID_POINTER_VALUE);
   ThreadJoin(s_writerThread);

   delete s_parser;
   CleanupLogParserLibrary();
}
