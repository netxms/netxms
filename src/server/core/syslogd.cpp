/*
** NetXMS - Network Management System
** Copyright (C) 2003-2024 Victor Kirhenshtein
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
#include <nxcore_syslog.h>
#include <nxcore_discovery.h>
#include <nxlpapi.h>

#define DEBUG_TAG _T("syslog")

/**
 * Max syslog message length
 */
#define MAX_SYSLOG_MSG_LEN    1024

/**
 * Queues
 */
ObjectQueue<SyslogMessage> g_syslogProcessingQueue(1024, Ownership::False);
ObjectQueue<SyslogMessage> g_syslogWriteQueue(1024, Ownership::False);

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
static uint64_t s_msgId = 1;  // Next available message ID
static LogParser *s_parser = nullptr;
static Mutex s_parserLock(MutexType::FAST);
static NodeMatchingPolicy s_nodeMatchingPolicy = SOURCE_IP_THEN_HOSTNAME;
static THREAD s_receiverThread = INVALID_THREAD_HANDLE;
static THREAD s_processingThread = INVALID_THREAD_HANDLE;
static THREAD s_writerThread = INVALID_THREAD_HANDLE;
static bool s_running = true;
static bool s_alwaysUseServerTime = false;
static bool s_enableStorage = true;
static bool s_allowUnknownSources = false;
static bool s_parseUnknownSources = false;
static char s_syslogCodepage[16] = "";

/**
 * Parse timestamp field
 */
static bool ParseTimeStamp(char **start, size_t msgSize, size_t *currPos, time_t *output)
{
   static char psMonth[12][5] = { "Jan ", "Feb ", "Mar ", "Apr ",
                                  "May ", "Jun ", "Jul ", "Aug ",
                                  "Sep ", "Oct ", "Nov ", "Dec " };
   struct tm timestamp;
   time_t t;
   char szBuffer[16], *curr = *start;
   int i;

   if (msgSize - *currPos < 16)
      return false;  // Timestamp cannot be shorter than 16 bytes

   // Prepare local time structure
   t = time(nullptr);
#if HAVE_LOCALTIME_R
   localtime_r(&t, &timestamp);
#else
   memcpy(&timestamp, localtime(&t), sizeof(struct tm));
#endif

   // Month
   for(i = 0; i < 12; i++)
      if (!memcmp(curr, psMonth[i], 4))
      {
         timestamp.tm_mon = i;
         break;
      }
   if (i == 12)
      return false;
   curr += 4;

   // Day of week
   if (isdigit(*curr))
   {
      timestamp.tm_mday = *curr - '0';
   }
   else
   {
      if (*curr != ' ')
         return false;  // Invalid day of month
      timestamp.tm_mday = 0;
   }
   curr++;
   if (isdigit(*curr))
   {
      timestamp.tm_mday = timestamp.tm_mday * 10 + (*curr - '0');
   }
   else
   {
      return false;  // Invalid day of month
   }
   curr++;
   if (*curr != ' ')
      return false;
   curr++;

   // HH:MM:SS
   memcpy(szBuffer, curr, 8);
   szBuffer[8] = 0;
   if (sscanf(szBuffer, "%02d:%02d:%02d", &timestamp.tm_hour,
              &timestamp.tm_min, &timestamp.tm_sec) != 3)
      return false;  // Invalid time format
   curr += 8;

	// Check for Cisco variant - HH:MM:SS.nnn
	if (*curr == '.')
	{
		curr++;
		if (isdigit(*curr))
			curr++;
		if (isdigit(*curr))
			curr++;
		if (isdigit(*curr))
			curr++;
	}

   if (*curr != ' ')
      return false;  // Space should follow timestamp
   curr++;

   // Convert to system time
   *output = mktime(&timestamp);
   if (*output == ((time_t)-1))
      return false;

   // Adjust current position
   *currPos += static_cast<size_t>(curr - *start);
   *start = curr;
   return true;
}

/**
 * Parse syslog message
 */
bool SyslogMessage::parse()
{
   size_t currPos = 0;
   char *currPtr = m_rawData;
   // Parse PRI part
   if (*m_rawData == '<')
   {
      int nPri = 0, nCount = 0;
      for(currPtr++, currPos++; isdigit(*currPtr) && (currPos < m_rawDataLen); currPtr++, currPos++, nCount++)
         nPri = nPri * 10 + (*currPtr - '0');
      if (currPos >= m_rawDataLen)
         return false;  // Unexpected end of message

      if ((*currPtr == '>') && (nCount > 0) && (nCount <4))
      {
         m_facility = nPri / 8;
         m_severity = nPri % 8;
         currPtr++;
         currPos++;
      }
      else
      {
         return false;  // Invalid message
      }
   }

   // Parse HEADER part
   time_t msgTimestamp;
   if (ParseTimeStamp(&currPtr, m_rawDataLen, &currPos, &msgTimestamp))
   {
      // Use server time if configured
      // We still had to parse timestamp to get correct start position for MSG part
      if (!s_alwaysUseServerTime)
      {
         m_timestamp = msgTimestamp;
      }

      // Hostname
      int i;
      for(i = 0; (*currPtr >= 33) && (*currPtr <= 126) && (i < MAX_SYSLOG_HOSTNAME_LEN - 1) && (currPos < m_rawDataLen); i++, currPos++, currPtr++)
         m_hostName[i] = *currPtr;
      if ((currPos >= m_rawDataLen) || (*currPtr != ' '))
      {
         // Not a valid hostname, assuming to be a part of message
         currPtr -= i;
         currPos -= i;
         m_hostName[0] = 0;
      }
      else
      {
         m_hostName[i] = 0;
         currPtr++;
         currPos++;
      }
   }

   // Parse MSG part
   int i;
   for(i = 0; isalnum(*currPtr) && (i < MAX_SYSLOG_TAG_LEN) && (currPos < m_rawDataLen); i++, currPos++, currPtr++)
      m_tag[i] = *currPtr;
   if ((i == MAX_SYSLOG_TAG_LEN) || (currPos >= m_rawDataLen))
   {
      // Too long tag, assuming that it's a part of message
      m_tag[0] = 0;
   }
   else
   {
      m_tag[i] = 0;
   }
   currPtr -= i;
   currPos -= i;

   m_rawMessage = currPtr;
   return true;
}

/**
 * Fill NXCP message with syslog message data
 */
void SyslogMessage::fillNXCPMessage(NXCPMessage *msg) const
{
   uint32_t fieldId = VID_SYSLOG_MSG_BASE;
   msg->setField(VID_NUM_RECORDS, static_cast<uint32_t>(1));
   msg->setField(fieldId++, m_id);
   msg->setFieldFromTime(fieldId++, m_timestamp);
   msg->setField(fieldId++, m_facility);
   msg->setField(fieldId++, m_severity);
   msg->setField(fieldId++, m_nodeId);
   msg->setFieldFromMBString(fieldId++, m_hostName);
   msg->setFieldFromMBString(fieldId++, m_tag);
   msg->setField(fieldId++, m_message);
}

/**
 * Find node by host name
 */
static shared_ptr<Node> FindNodeByHostname(const char *hostName, int32_t zoneUIN)
{
   if (hostName[0] == 0)
      return shared_ptr<Node>();

   shared_ptr<Node> node;
   InetAddress ipAddr = InetAddress::resolveHostName(hostName);
   if (ipAddr.isValidUnicast())
   {
      node = FindNodeByIP(zoneUIN, (g_flags & AF_TRAP_SOURCES_IN_ALL_ZONES) != 0, ipAddr);
   }

   if (node == nullptr)
	{
      wchar_t wname[MAX_OBJECT_NAME];
		mb_to_wchar(hostName, -1, wname, MAX_OBJECT_NAME);
		wname[MAX_OBJECT_NAME - 1] = 0;
		node = static_pointer_cast<Node>(FindObjectByName(wname, OBJECT_NODE));
   }
   return node;
}

/**
 * Bind syslog message to NetXMS node object
 * sourceAddr is an IP address from which we receive message
 */
bool SyslogMessage::bindToNode()
{
   nxlog_debug_tag(DEBUG_TAG, 6, _T("SyslogRecord::bindToNode(): addr=%s zoneUIN=%d"), m_sourceAddress.toString().cstr(), m_zoneUIN);

   if (m_nodeId != 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("SyslogRecord::bindToNode(): node ID explicitly set to %u"), m_nodeId);
      m_node = static_pointer_cast<Node>(FindObjectById(m_nodeId, OBJECT_NODE));
   }
   else if (m_sourceAddress.isLoopback() && (m_zoneUIN == 0))
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("SyslogRecord::bindToNode(): source is loopback in default zone, binding to management node (ID %u)"), g_dwMgmtNode);
      m_node = static_pointer_cast<Node>(FindObjectById(g_dwMgmtNode, OBJECT_NODE));
   }
   else if (s_nodeMatchingPolicy == SOURCE_IP_THEN_HOSTNAME)
   {
      m_node = FindNodeByIP(m_zoneUIN, (g_flags & AF_TRAP_SOURCES_IN_ALL_ZONES) != 0, m_sourceAddress);
      if (m_node == nullptr)
      {
         m_node = FindNodeByHostname(m_hostName, m_zoneUIN);
      }
   }
   else
   {
      m_node = FindNodeByHostname(m_hostName, m_zoneUIN);
      if (m_node == nullptr)
      {
         m_node = FindNodeByIP(m_zoneUIN, (g_flags & AF_TRAP_SOURCES_IN_ALL_ZONES) != 0, m_sourceAddress);
      }
   }

	if (m_node != nullptr)
   {
	   m_node->incSyslogMessageCount();
	   m_nodeId = m_node->getId();
      m_zoneUIN = m_node->getZoneUIN();
      if (m_hostName[0] == 0)
		{
			wchar_to_mb(m_node->getName(), -1, m_hostName, MAX_SYSLOG_HOSTNAME_LEN);
			m_hostName[MAX_SYSLOG_HOSTNAME_LEN - 1] = 0;
      }
   }
   else
   {
      if (m_hostName[0] == 0)
         m_sourceAddress.toStringA(m_hostName);
      m_nodeId = 0;
   }

   return m_node != nullptr;
}

/**
 * Handler for EnumerateSessions()
 */
static void BroadcastSyslogMessage(ClientSession *session, SyslogMessage *msg)
{
   if (session->isAuthenticated())
      session->onSyslogMessage(msg);
}

/**
 * Syslog writer thread
 */
static void SyslogWriterThread()
{
   ThreadSetName("SyslogWriter");
   nxlog_debug_tag(DEBUG_TAG, 1, _T("Syslog writer thread started"));
   int maxRecords = ConfigReadInt(_T("DBWriter.MaxRecordsPerTransaction"), 1000);
   while(true)
   {
      SyslogMessage *msg = g_syslogWriteQueue.getOrBlock();
      if (msg == INVALID_POINTER_VALUE)
         break;

      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

      DB_STATEMENT hStmt = DBPrepare(hdb,
               (g_dbSyntax == DB_SYNTAX_TSDB) ?
                        _T("INSERT INTO syslog (msg_id,msg_timestamp,facility,severity,source_object_id,zone_uin,hostname,msg_tag,msg_text) VALUES (?,to_timestamp(?),?,?,?,?,?,?,?)") :
                        _T("INSERT INTO syslog (msg_id,msg_timestamp,facility,severity,source_object_id,zone_uin,hostname,msg_tag,msg_text) VALUES (?,?,?,?,?,?,?,?,?)"), true);
      if (hStmt == nullptr)
      {
         delete msg;
         DBConnectionPoolReleaseConnection(hdb);
         continue;
      }

      int count = 0;
      DBBegin(hdb);
      while(true)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_BIGINT, msg->getId());
         DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(msg->getTimestamp()));
         DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, msg->getFacility());
         DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, msg->getSeverity());
         DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, msg->getNodeId());
         DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, msg->getZoneUIN());
         DBBind(hStmt, 7, DB_SQLTYPE_VARCHAR, WideStringFromMBString(msg->getHostName()), DB_BIND_DYNAMIC);
         DBBind(hStmt, 8, DB_SQLTYPE_VARCHAR, WideStringFromMBString(msg->getTag()), DB_BIND_DYNAMIC);
         DBBind(hStmt, 9, DB_SQLTYPE_VARCHAR, msg->getMessage(), DB_BIND_STATIC);

         if (!DBExecute(hStmt))
         {
            delete msg;
            break;
         }
         delete msg;
         count++;
         if (count == maxRecords)
            break;
         msg = g_syslogWriteQueue.get();
         if ((msg == nullptr) || (msg == INVALID_POINTER_VALUE))
            break;
      }
      DBCommit(hdb);
      DBFreeStatement(hStmt);
      DBConnectionPoolReleaseConnection(hdb);
      if (msg == INVALID_POINTER_VALUE)
         break;
   }
   nxlog_debug_tag(DEBUG_TAG, 1, _T("Syslog writer thread stopped"));
}

/**
 * Process syslog message
 */
static void ProcessSyslogMessage(SyslogMessage *msg)
{
	nxlog_debug_tag(DEBUG_TAG, 6, _T("ProcessSyslogMessage: Raw syslog message to process:\n%hs"), msg->getRawData());
   if (msg->parse())
   {
      InterlockedIncrement64(&g_syslogMessagesReceived);

      if (!msg->bindToNode() && !s_allowUnknownSources)
      {
         nxlog_debug_tag(DEBUG_TAG, 6, _T("ProcessSyslogMessage: message from unknown source ignored"));
         delete msg;
         return;
      }

      msg->setId(s_msgId++);
      const char *codepage = (s_syslogCodepage[0] != 0) ? s_syslogCodepage : nullptr;
      if (msg->getNodeId() != 0)
      {
         const char *nodecp = msg->getNode()->getSyslogCodepage();
         if (nodecp[0] != 0)
            codepage = nodecp;
      }
      msg->convertRawMessage(codepage);

		TCHAR ipAddr[64];
		nxlog_debug_tag(DEBUG_TAG, 6, _T("Syslog message: ipAddr=%s zone=%d objectId=%u tag=\"%hs\" msg=\"%s\""),
		            msg->getSourceAddress().toString(ipAddr), msg->getZoneUIN(), msg->getNodeId(), msg->getTag(), msg->getMessage());

		bool writeToDatabase = true;
		s_parserLock.lock();
		if (((msg->getNodeId() != 0) || s_parseUnknownSources) && (s_parser != nullptr))
		{
		   wchar_t wtag[MAX_SYSLOG_TAG_LEN];
			mbcp_to_wchar(msg->getTag(), -1, wtag, MAX_SYSLOG_TAG_LEN, codepage);
			s_parser->matchEvent(wtag, msg->getFacility(), 1 << msg->getSeverity(), msg->getMessage(), nullptr, 0, msg->getNodeId(), 0, ipAddr, &writeToDatabase);
		}
		s_parserLock.unlock();

      // Send message to all connected clients
      EnumerateClientSessions(BroadcastSyslogMessage, msg);

	   if ((msg->getNodeId() == 0) && (g_flags & AF_SYSLOG_DISCOVERY))  // unknown node, discovery enabled
	   {
	      nxlog_debug_tag(DEBUG_TAG, 4, _T("ProcessSyslogMessage: source not matched to node, adding new IP address %s for discovery"),
	               msg->getSourceAddress().toString(ipAddr));
	      CheckPotentialNode(msg->getSourceAddress(), msg->getZoneUIN(), DA_SRC_SYSLOG, 0);
	   }

	   if (writeToDatabase && s_enableStorage)
         g_syslogWriteQueue.put(msg);
	   else
	      delete msg;
   }
	else
	{
		nxlog_debug_tag(DEBUG_TAG, 6, _T("ProcessSyslogMessage: Cannot parse syslog message"));
		delete msg;
	}
}

/**
 * Syslog processing thread
 */
static void SyslogProcessingThread()
{
   ThreadSetName("SyslogProcessor");
   while(true)
   {
      SyslogMessage *msg = g_syslogProcessingQueue.getOrBlock();
      if (msg == INVALID_POINTER_VALUE)
         break;

      ProcessSyslogMessage(msg);
   }
}

/**
 * Queue syslog message for processing
 */
static void QueueSyslogMessage(char *msg, int msgLen, const InetAddress& sourceAddr)
{
   g_syslogProcessingQueue.put(new SyslogMessage(sourceAddr, msg, msgLen));
}

/**
 * Queue proxied syslog message for processing
 */
void QueueProxiedSyslogMessage(const InetAddress &addr, int32_t zoneUIN, uint32_t nodeId, time_t timestamp, const char *msg, int msgLen)
{
   g_syslogProcessingQueue.put(new SyslogMessage(addr, timestamp, zoneUIN, nodeId, msg, msgLen));
}

/**
 * Callback for syslog parser
 */
static void SyslogParserCallback(const LogParserCallbackData& data)
{
	nxlog_debug_tag(DEBUG_TAG, 7, _T("Syslog message matched, capture group count = %d, repeat count = %d"), data.captureGroups->size(), data.repeatCount);

   shared_ptr<Node> node = static_pointer_cast<Node>(FindObjectById(data.objectId, OBJECT_NODE));
   if (((node != nullptr) && ((node->getStatus() != STATUS_UNMANAGED) || (g_flags & AF_TRAPS_FROM_UNMANAGED_NODES))) ||
       ((node == nullptr) && s_parseUnknownSources))
   {
      StringMap pmap;
      data.captureGroups->addAllToMap(&pmap);
      EventBuilder(data.eventCode, (node != nullptr) ? node->getId() : g_dwMgmtNode)
         .origin(EventOrigin::SYSLOG)
         .originTimestamp(data.logRecordTimestamp)
         .tag(data.eventTag)
         .params(pmap)
         .param(_T("repeatCount"), data.repeatCount)
         .param(_T("sourceAddress"), data.logName)
         .param(_T("severity"), data.severity)
         .param(_T("facility"), data.facility)
         .param(_T("tag"), data.source)
         .param(_T("message"), data.originalText)
         .post();
   }
}

/**
 * Create syslog parser from config
 */
static void CreateParserFromConfig()
{
	LockGuard lockGuard(s_parserLock);
	LogParser *prev = s_parser;
	s_parser = nullptr;
   char *xml;
   wchar_t *wxml = ConfigReadCLOB(_T("SyslogParser"), _T("<parser></parser>"));
	if (wxml != nullptr)
	{
		xml = UTF8StringFromWideString(wxml);
		MemFree(wxml);
	}
	else
	{
		xml = nullptr;
	}
	if (xml != nullptr)
	{
	   wchar_t parseError[256];
		ObjectArray<LogParser> *parsers = LogParser::createFromXml(xml, -1, parseError, 256, EventNameResolver);
		if ((parsers != nullptr) && (parsers->size() > 0))
		{
			s_parser = parsers->get(0);
			s_parser->setCallback(SyslogParserCallback);
			if (prev != nullptr)
			   s_parser->restoreCounters(prev);
			nxlog_debug_tag(DEBUG_TAG, 3, L"Syslog parser successfully created from config");
		}
		else
		{
			nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, L"Cannot initialize syslog parser (%s)", parseError);
		}
		MemFree(xml);
		delete parsers;
	}
	delete prev;
}

/**
 * Syslog messages receiver thread
 */
static void SyslogReceiver()
{
   ThreadSetName("SyslogReceiver");

   SOCKET hSocket = CreateSocket(AF_INET, SOCK_DGRAM, 0);
#ifdef WITH_IPV6
   SOCKET hSocket6 = CreateSocket(AF_INET6, SOCK_DGRAM, 0);
#endif

#ifdef WITH_IPV6
   if ((hSocket == INVALID_SOCKET) && (hSocket6 == INVALID_SOCKET))
#else
   if (hSocket == INVALID_SOCKET)
#endif
   {
      TCHAR buffer[1024];
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, L"Unable to create socket for syslog receiver (%s)", GetLastSocketErrorText(buffer, 1024));
      return;
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
   int port = ConfigReadInt(_T("Syslog.ListenPort"), 514);
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
      TCHAR buffer[1024];
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Unable to bind IPv4 socket for syslog receiver (%s)"), GetLastSocketErrorText(buffer, 1024));
      bindFailures++;
      closesocket(hSocket);
      hSocket = INVALID_SOCKET;
   }

#ifdef WITH_IPV6
   nxlog_debug_tag(DEBUG_TAG, 5, _T("Trying to bind on UDP [%s]:%d"), SockaddrToStr((struct sockaddr *)&servAddr6, buffer), ntohs(servAddr6.sin6_port));
   if (bind(hSocket6, (struct sockaddr *)&servAddr6, sizeof(struct sockaddr_in6)) != 0)
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Unable to bind IPv6 socket for syslog receiver (%s)"), GetLastSocketErrorText(buffer, 1024));
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
      return;
   }

   if (hSocket != INVALID_SOCKET)
   {
      TCHAR ipAddrText[64];
      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Listening for syslog messages on UDP socket %s:%u"), InetAddress(ntohl(servAddr.sin_addr.s_addr)).toString(ipAddrText), port);
   }
#ifdef WITH_IPV6
   if (hSocket6 != INVALID_SOCKET)
   {
      TCHAR ipAddrText[64];
      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Listening for syslog messages on UDP socket %s:%u"), InetAddress(servAddr6.sin6_addr.s6_addr).toString(ipAddrText), port);
   }
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
}

/**
 * Reinitialize parser on configuration change
 */
void ReinitializeSyslogParser()
{
   CreateParserFromConfig();
}

/**
 * Handler for syslog related configuration changes
 */
void OnSyslogConfigurationChange(const TCHAR *name, const TCHAR *value)
{
   if (!_tcscmp(name, _T("Syslog.AllowUnknownSources")))
   {
      s_allowUnknownSources = _tcstol(value, nullptr, 0) ? true : false;
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Unknown syslog sources are %s"), s_allowUnknownSources ? _T("allowed") : _T("not allowed"));
   }
   else if (!_tcscmp(name, _T("Syslog.Codepage")))
   {
      tchar_to_utf8(value, -1, s_syslogCodepage, sizeof(s_syslogCodepage));
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Server syslog default codepage is set as: %s"), value);
   }
   else if (!_tcscmp(name, _T("Syslog.EnableStorage")))
   {
      s_enableStorage = _tcstol(value, nullptr, 0) ? true : false;
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Local syslog storage is %s"), s_enableStorage ? _T("enabled") : _T("disabled"));
   }
   else if (!_tcscmp(name, _T("Syslog.IgnoreMessageTimestamp")))
   {
      s_alwaysUseServerTime = _tcstol(value, nullptr, 0) ? true : false;
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Ignore message timestamp option set to %s"), s_alwaysUseServerTime ? _T("ON") : _T("OFF"));
   }
   else if (!_tcscmp(name, _T("Syslog.ParseUnknownSourceMessages")))
   {
      s_parseUnknownSources = _tcstol(value, nullptr, 0) ? true : false;
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Parsing of messages from unknown syslog sources is %s"), s_parseUnknownSources ? _T("allowed") : _T("not allowed"));
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

   if ((argc == 2) && !argv[1]->isInteger() && !argv[1]->isObject(g_nxslNetObjClass.getName()))
      return NXSL_ERR_NOT_INTEGER;

   uint32_t objectId = 0;
   if (argc == 2)
   {
      if (argv[1]->isInteger())
      {
         objectId = argv[1]->getValueAsUInt32();
      }
      else
      {
         objectId = (*static_cast<shared_ptr<NetObj>*>(argv[1]->getValueAsObject()->getData()))->getId();
      }
   }

   s_parserLock.lock();
   *result = vm->createValue((s_parser != nullptr) ? s_parser->getRuleCheckCount(argv[0]->getValueAsCString(), objectId) : -1);
   s_parserLock.unlock();
   return 0;
}

/**
 * Get syslog rule match count in NXSL
 */
int F_GetSyslogRuleMatchCount(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if ((argc != 1) && (argc != 2))
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if ((argc == 2) && !argv[1]->isInteger() && !argv[1]->isObject(g_nxslNetObjClass.getName()))
      return NXSL_ERR_NOT_INTEGER;

   uint32_t objectId = 0;
   if (argc == 2)
   {
      if (argv[1]->isInteger())
      {
         objectId = argv[1]->getValueAsUInt32();
      }
      else
      {
         objectId = (*static_cast<shared_ptr<NetObj>*>(argv[1]->getValueAsObject()->getData()))->getId();
      }
   }

   s_parserLock.lock();
   *result = vm->createValue((s_parser != nullptr) ? s_parser->getRuleMatchCount(argv[0]->getValueAsCString(), objectId) : -1);
   s_parserLock.unlock();
   return 0;
}

/**
 * Get next syslog id
 */
uint64_t GetNextSyslogId()
{
   return s_msgId;
}

/**
 * Start built-in syslog server
 */
void StartSyslogServer()
{
   ConfigReadStrUTF8(_T("Syslog.Codepage"), s_syslogCodepage, 16, "");
   s_allowUnknownSources = ConfigReadBoolean(_T("Syslog.AllowUnknownSources"), false);
   s_parseUnknownSources = ConfigReadBoolean(_T("Syslog.ParseUnknownSourceMessages"), false);
   s_alwaysUseServerTime = ConfigReadBoolean(_T("Syslog.IgnoreMessageTimestamp"), false);
   s_enableStorage = ConfigReadBoolean(_T("Syslog.EnableStorage"), false);
   s_nodeMatchingPolicy = static_cast<NodeMatchingPolicy>(ConfigReadInt(_T("Syslog.NodeMatchingPolicy"), SOURCE_IP_THEN_HOSTNAME));

   // Determine first available message id
   uint64_t id = ConfigReadUInt64(_T("FirstFreeSyslogId"), s_msgId);
   if (id > s_msgId)
      s_msgId = id;
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, _T("SELECT max(msg_id) FROM syslog"));
   if (hResult != nullptr)
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
   CreateParserFromConfig();

   // Start processing thread
   s_processingThread = ThreadCreateEx(SyslogProcessingThread);
   s_writerThread = ThreadCreateEx(SyslogWriterThread);

   if (ConfigReadBoolean(_T("Syslog.EnableListener"), false))
      s_receiverThread = ThreadCreateEx(SyslogReceiver);
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

/**
 * Collects information about all syslog parser rules that are using specified event
 */
void GetSyslogEventReferences(uint32_t eventCode, ObjectArray<EventReference>* eventReferences)
{
   s_parserLock.lock();
   if ((s_parser != nullptr) && s_parser->isUsingEvent(eventCode))
   {
      eventReferences->add(new EventReference(EventReferenceType::SYSLOG));
   }
   s_parserLock.unlock();
}
