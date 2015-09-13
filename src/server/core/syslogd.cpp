/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2014 Victor Kirhenshtein
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

/**
 * Max syslog message length
 */
#define MAX_SYSLOG_MSG_LEN    1024

/**
 * Queued syslog message structure
 */
struct QUEUED_SYSLOG_MESSAGE
{
   UINT32 dwSourceIP;
   int nBytes;
   char *psMsg;
};

/**
 * Queues
 */
Queue g_syslogProcessingQueue(1000, 100);
Queue g_syslogWriteQueue(1000, 100);

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
   memcpy(&timestamp, localtime(&t), sizeof(struct tm));

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
static BOOL ParseSyslogMessage(char *psMsg, int nMsgLen, NX_SYSLOG_RECORD *pRec)
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
      pRec->tmTimeStamp = time(NULL);
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
   nLen = min(nMsgLen - nPos, MAX_LOG_MSG_LENGTH);
   memcpy(pRec->szMessage, pCurr, nLen);

   return TRUE;
}

/**
 * Find node by host name
 */
static Node *FindNodeByHostname(const char *hostName)
{
   if (hostName[0] == 0)
      return NULL;

   Node *node = NULL;
   UINT32 ipAddr = ntohl(ResolveHostNameA(hostName));
	if ((ipAddr != INADDR_NONE) && (ipAddr != INADDR_ANY))
   {
      node = FindNodeByIP((g_flags & AF_TRAP_SOURCES_IN_ALL_ZONES) ? ALL_ZONES : 0, ipAddr);
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
 * dwSourceIP is an IP address from which we receive message
 */
static Node *BindMsgToNode(NX_SYSLOG_RECORD *pRec, UINT32 dwSourceIP)
{
   Node *node = NULL;

   if (s_nodeMatchingPolicy == SOURCE_IP_THEN_HOSTNAME)
   {
      node = FindNodeByIP((g_flags & AF_TRAP_SOURCES_IN_ALL_ZONES) ? ALL_ZONES : 0, dwSourceIP);
      if (node == NULL)
      {
         node = FindNodeByHostname(pRec->szHostName);
      }
   }
   else
   {
      node = FindNodeByHostname(pRec->szHostName);
      if (node == NULL)
      {
         node = FindNodeByIP((g_flags & AF_TRAP_SOURCES_IN_ALL_ZONES) ? ALL_ZONES : 0, dwSourceIP);
      }
   }

	if (node != NULL)
   {
      pRec->dwSourceObject = node->getId();
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
         IpToStrA(dwSourceIP, pRec->szHostName);
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
   DbgPrintf(1, _T("Syslog writer thread started"));
   while(true)
   {
      NX_SYSLOG_RECORD *r = (NX_SYSLOG_RECORD *)g_syslogWriteQueue.getOrBlock();
      if (r == INVALID_POINTER_VALUE)
         break;

      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

      DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO syslog (msg_id,msg_timestamp,facility,severity,source_object_id,hostname,msg_tag,msg_text) VALUES (?,?,?,?,?,?,?,?)"));
      if (hStmt == NULL)
      {
         free(r);
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
#ifdef UNICODE
         DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, WideStringFromMBString(r->szHostName), DB_BIND_DYNAMIC);
         DBBind(hStmt, 7, DB_SQLTYPE_VARCHAR, WideStringFromMBString(r->szTag), DB_BIND_DYNAMIC);
         DBBind(hStmt, 8, DB_SQLTYPE_VARCHAR, WideStringFromMBString(r->szMessage), DB_BIND_DYNAMIC);
#else
         DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, r->szHostName, DB_BIND_STATIC);
         DBBind(hStmt, 7, DB_SQLTYPE_VARCHAR, r->szTag, DB_BIND_STATIC);
         DBBind(hStmt, 8, DB_SQLTYPE_VARCHAR, r->szMessage, DB_BIND_STATIC);
#endif

         if (!DBExecute(hStmt))
         {
            free(r);
            break;
         }
         free(r);
         count++;
         if (count == 1000)
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
   DbgPrintf(1, _T("Syslog writer thread stopped"));
   return THREAD_OK;
}

/**
 * Process syslog message
 */
static void ProcessSyslogMessage(char *psMsg, int nMsgLen, UINT32 dwSourceIP)
{
   NX_SYSLOG_RECORD record;

	DbgPrintf(6, _T("ProcessSyslogMessage: Raw syslog message to process:\n%hs"), psMsg);
   if (ParseSyslogMessage(psMsg, nMsgLen, &record))
   {
      record.qwMsgId = s_msgId++;
      Node *node = BindMsgToNode(&record, dwSourceIP);

      g_syslogWriteQueue.put(nx_memdup(&record, sizeof(NX_SYSLOG_RECORD)));

      // Send message to all connected clients
      EnumerateClientSessions(BroadcastSyslogMessage, &record);

		TCHAR ipAddr[32];
		DbgPrintf(6, _T("Syslog message: ipAddr=%s objectId=%d tag=\"%hs\" msg=\"%hs\""),
		          IpToStr(dwSourceIP, ipAddr), record.dwSourceObject, record.szTag, record.szMessage);

		MutexLock(s_parserLock);
		if ((record.dwSourceObject != 0) && (s_parser != NULL) &&
          ((node->Status() != STATUS_UNMANAGED) || (g_flags & AF_TRAPS_FROM_UNMANAGED_NODES)))
		{
#ifdef UNICODE
			WCHAR wtag[MAX_SYSLOG_TAG_LEN];
			WCHAR wmsg[MAX_LOG_MSG_LENGTH];
			MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, record.szTag, -1, wtag, MAX_SYSLOG_TAG_LEN);
			MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, record.szMessage, -1, wmsg, MAX_LOG_MSG_LENGTH);
			s_parser->matchEvent(wtag, record.nFacility, 1 << record.nSeverity, wmsg, record.dwSourceObject);
#else
			s_parser->matchEvent(record.szTag, record.nFacility, 1 << record.nSeverity, record.szMessage, record.dwSourceObject);
#endif
		}
		MutexUnlock(s_parserLock);
   }
	else
	{
		DbgPrintf(6, _T("ProcessSyslogMessage: Cannot parse syslog message"));
	}
}

/**
 * Syslog processing thread
 */
static THREAD_RESULT THREAD_CALL SyslogProcessingThread(void *pArg)
{
   QUEUED_SYSLOG_MESSAGE *pMsg;

   while(1)
   {
      pMsg = (QUEUED_SYSLOG_MESSAGE *)g_syslogProcessingQueue.getOrBlock();
      if (pMsg == INVALID_POINTER_VALUE)
         break;

      ProcessSyslogMessage(pMsg->psMsg, pMsg->nBytes, pMsg->dwSourceIP);
      free(pMsg->psMsg);
      free(pMsg);
   }
   return THREAD_OK;
}

/**
 * Queue syslog message for processing
 */
static void QueueSyslogMessage(char *psMsg, int nMsgLen, UINT32 dwSourceIP)
{
   QUEUED_SYSLOG_MESSAGE *pMsg;

   pMsg = (QUEUED_SYSLOG_MESSAGE *)malloc(sizeof(QUEUED_SYSLOG_MESSAGE));
   pMsg->dwSourceIP = dwSourceIP;
   pMsg->nBytes = nMsgLen;
   pMsg->psMsg = (char *)nx_memdup(psMsg, nMsgLen + 1);
   g_syslogProcessingQueue.put(pMsg);
}

/**
 * Callback for syslog parser
 */
static void SyslogParserCallback(UINT32 eventCode, const TCHAR *eventName, const TCHAR *line, 
                                 const TCHAR *source, UINT32 facility, UINT32 severity,
                                 int paramCount, TCHAR **params, UINT32 objectId, void *userArg)
{
	char format[] = "ssssssssssssssssssssssssssssssss";
	TCHAR *plist[32];
	int i, count;

	count = min(paramCount, 32);
	format[count] = 0;
	for(i = 0; i < count; i++)
		plist[i] = params[i];
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
	EVENT_TEMPLATE *event;
	bool success = false;

	event = FindEventTemplateByName(name);
	if (event != NULL)
	{
		*code = event->dwCode;
		success = true;
	}
	return success;
}

/**
 * Create syslog parser from config
 */
static void CreateParserFromConfig()
{
	char *xml;

	MutexLock(s_parserLock);
	delete_and_null(s_parser);
#ifdef UNICODE
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
	xml = ConfigReadCLOB("SyslogParser", "<parser></parser>");
#endif
	if (xml != NULL)
	{
		TCHAR parseError[256];
		ObjectArray<LogParser> *parsers = LogParser::createFromXml(xml, -1, parseError, 256, EventNameResolver);
		if ((parsers != NULL) && (parsers->size() > 0))
		{
			s_parser = parsers->get(0);
			s_parser->setCallback(SyslogParserCallback);
			DbgPrintf(3, _T("syslogd: parser successfully created from config"));
		}
		else
		{
			nxlog_write(MSG_SYSLOG_PARSER_INIT_FAILED, EVENTLOG_ERROR_TYPE, "s", parseError);
		}
		free(xml);
		delete parsers;
	}
	MutexUnlock(s_parserLock);
}

/**
 * Syslog messages receiver thread
 */
THREAD_RESULT THREAD_CALL SyslogDaemon(void *pArg)
{
   SOCKET hSocket;
   struct sockaddr_in addr;
   int nBytes, nPort, nRet;
   socklen_t nAddrLen;
   char sMsg[MAX_SYSLOG_MSG_LEN + 1];
   DB_RESULT hResult;
   fd_set rdfs;
   struct timeval tv;

   s_nodeMatchingPolicy = (NodeMatchingPolicy)ConfigReadInt(_T("SyslogNodeMatchingPolicy"), SOURCE_IP_THEN_HOSTNAME);

   // Determine first available message id
   hResult = DBSelect(g_hCoreDB, _T("SELECT max(msg_id) FROM syslog"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         s_msgId = max(DBGetFieldUInt64(hResult, 0, 0) + 1, s_msgId);
      }
      DBFreeResult(hResult);
   }

   hSocket = socket(AF_INET, SOCK_DGRAM, 0);
   if (hSocket == INVALID_SOCKET)
   {
      nxlog_write(MSG_SOCKET_FAILED, EVENTLOG_ERROR_TYPE, "s", _T("SyslogDaemon"));
      return THREAD_OK;
   }

	SetSocketExclusiveAddrUse(hSocket);
	SetSocketReuseFlag(hSocket);

   // Get listen port number
   nPort = ConfigReadInt(_T("SyslogListenPort"), 514);
   if ((nPort < 1) || (nPort > 65535))
      nPort = 514;

   // Fill in local address structure
   memset(&addr, 0, sizeof(struct sockaddr_in));
   addr.sin_family = AF_INET;
   addr.sin_addr.s_addr = !_tcscmp(g_szListenAddress, _T("*")) ? 0 : ResolveHostName(g_szListenAddress);
   addr.sin_port = htons((WORD)nPort);

   // Bind socket
   if (bind(hSocket, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) != 0)
   {
      nxlog_write(MSG_BIND_ERROR, EVENTLOG_ERROR_TYPE, "dse", nPort, _T("SyslogDaemon"), WSAGetLastError());
      closesocket(hSocket);
      return THREAD_OK;
   }
	nxlog_write(MSG_LISTENING_FOR_SYSLOG, EVENTLOG_INFORMATION_TYPE, "ad", ntohl(addr.sin_addr.s_addr), nPort);

   SetLogParserTraceCallback(DbgPrintf2);
   InitLogParserLibrary();

	// Create message parser
	s_parserLock = MutexCreate();
	CreateParserFromConfig();

   // Start processing thread
   THREAD hProcessingThread = ThreadCreateEx(SyslogProcessingThread, 0, NULL);
   THREAD hWriterThread = ThreadCreateEx(SyslogWriterThread, 0, NULL);

   DbgPrintf(1, _T("Syslog Daemon started"));

   // Wait for packets
   while(!IsShutdownInProgress())
   {
      FD_ZERO(&rdfs);
      FD_SET(hSocket, &rdfs);
      tv.tv_sec = 1;
      tv.tv_usec = 0;
      nRet = select((int)hSocket + 1, &rdfs, NULL, NULL, &tv);
      if (nRet > 0)
      {
         nAddrLen = sizeof(struct sockaddr_in);
         nBytes = recvfrom(hSocket, sMsg, MAX_SYSLOG_MSG_LEN, 0, (struct sockaddr *)&addr, &nAddrLen);
         if (nBytes > 0)
         {
				sMsg[nBytes] = 0;
            QueueSyslogMessage(sMsg, nBytes, ntohl(addr.sin_addr.s_addr));
         }
         else
         {
            // Sleep on error
            ThreadSleepMs(100);
         }
      }
      else if (nRet == -1)
      {
         // Sleep on error
         ThreadSleepMs(100);
      }
   }

   // Stop processing thread
   g_syslogProcessingQueue.put(INVALID_POINTER_VALUE);
   ThreadJoin(hProcessingThread);

   // Stop writer thread - it must be done after processing thread already finished
   g_syslogWriteQueue.put(INVALID_POINTER_VALUE);
   ThreadJoin(hWriterThread);

	delete s_parser;
   CleanupLogParserLibrary();

   DbgPrintf(1, _T("Syslog Daemon stopped"));
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
		return;	// Syslog daemon not initialized
	CreateParserFromConfig();
}
