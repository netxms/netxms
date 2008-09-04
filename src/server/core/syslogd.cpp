/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 Victor Kirhenshtein
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


//
// Constants
//

#define MAX_SYSLOG_MSG_LEN    1024


//
// Queued syslog message structure
//

struct QUEUED_SYSLOG_MESSAGE
{
   DWORD dwSourceIP;
   int nBytes;
   char *psMsg;
};


//
// Static data
//

static QWORD m_qwMsgId = 1;
static Queue *m_pSyslogQueue = NULL;
static LogParser *m_parser = NULL;


//
// Parse timestamp field
//

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


//
// Parse syslog message
//

static BOOL ParseSyslogMessage(char *psMsg, int nMsgLen, NX_LOG_RECORD *pRec)
{
   int i, nLen, nPos = 0;
   char *pCurr = psMsg;

   memset(pRec, 0, sizeof(NX_LOG_RECORD));

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
      for(i = 0; isalnum(*pCurr) && (i < MAX_SYSLOG_HOSTNAME_LEN) && (nPos < nMsgLen); i++, nPos++, pCurr++)
         pRec->szHostName[i] = *pCurr;
      if ((nPos >= nMsgLen) || (*pCurr != ' '))
      {
         // Not a valid hostname, assuming to be a part of message
         pCurr -= i;
         nPos -= i;
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


//
// Bind syslog message to NetXMS node object
// dwSourceIP is an IP address from which we receive message
//

static void BindMsgToNode(NX_LOG_RECORD *pRec, DWORD dwSourceIP)
{
   Node *pNode;
   DWORD dwIpAddr;

   // Determine IP address of a source
   if (pRec->szHostName[0] == 0)
   {
      // Hostname was not defined in the message
      dwIpAddr = dwSourceIP;
   }
   else
   {
      dwIpAddr = ResolveHostName(pRec->szHostName);
   }

   // Match source IP to NetXMS object
   if (dwIpAddr != INADDR_NONE)
   {
      pNode = FindNodeByIP(dwIpAddr);
      if (pNode != NULL)
      {
         pRec->dwSourceObject = pNode->Id();
         if (pRec->szHostName[0] == 0)
            nx_strncpy(pRec->szHostName, pNode->Name(), MAX_SYSLOG_HOSTNAME_LEN);
      }
      else
      {
         if (pRec->szHostName[0] == 0)
            IpToStr(dwSourceIP, pRec->szHostName);
      }
   }
}


//
// Handler for EnumerateSessions()
//

static void BroadcastSyslogMessage(ClientSession *pSession, void *pArg)
{
   if (pSession->IsAuthenticated())
      pSession->OnSyslogMessage((NX_LOG_RECORD *)pArg);
}


//
// Process syslog message
//

static void ProcessSyslogMessage(char *psMsg, int nMsgLen, DWORD dwSourceIP)
{
   NX_LOG_RECORD record;
   TCHAR *pszEscMsg, szQuery[4096];

   if (ParseSyslogMessage(psMsg, nMsgLen, &record))
   {
      record.qwMsgId = m_qwMsgId++;
      BindMsgToNode(&record, dwSourceIP);
      pszEscMsg = EncodeSQLString(record.szMessage);
      _sntprintf(szQuery, 4096, 
                 _T("INSERT INTO syslog (msg_id,msg_timestamp,facility,severity,")
                 _T("source_object_id,hostname,msg_tag,msg_text) VALUES ")
                 _T("(" UINT64_FMT "," TIME_T_FMT ",%d,%d,%d,'%s','%s','%s')"),
                 record.qwMsgId, record.tmTimeStamp, record.nFacility,
                 record.nSeverity, record.dwSourceObject,
                 record.szHostName, record.szTag, pszEscMsg);
      free(pszEscMsg);
      DBQuery(g_hCoreDB, szQuery);

      // Send message to all connected clients
      EnumerateClientSessions(BroadcastSyslogMessage, &record);

		if ((record.dwSourceObject != 0) && (m_parser != NULL))
		{
			m_parser->MatchLine(record.szMessage);
		}
   }
}


//
// Syslog processing thread
//

static THREAD_RESULT THREAD_CALL SyslogProcessingThread(void *pArg)
{
   QUEUED_SYSLOG_MESSAGE *pMsg;

   while(1)
   {
      pMsg = (QUEUED_SYSLOG_MESSAGE *)m_pSyslogQueue->GetOrBlock();
      if (pMsg == INVALID_POINTER_VALUE)
         break;

      ProcessSyslogMessage(pMsg->psMsg, pMsg->nBytes, pMsg->dwSourceIP);
      free(pMsg->psMsg);
      free(pMsg);
   }
   return THREAD_OK;
}


//
// Queue syslog message for processing
//

static void QueueSyslogMessage(char *psMsg, int nMsgLen, DWORD dwSourceIP)
{
   QUEUED_SYSLOG_MESSAGE *pMsg;

   pMsg = (QUEUED_SYSLOG_MESSAGE *)malloc(sizeof(QUEUED_SYSLOG_MESSAGE));
   pMsg->dwSourceIP = dwSourceIP;
   pMsg->nBytes = nMsgLen;
   pMsg->psMsg = (char *)nx_memdup(psMsg, nMsgLen);
   m_pSyslogQueue->Put(pMsg);
}


//
// Syslog messages receiver thread
//

THREAD_RESULT THREAD_CALL SyslogDaemon(void *pArg)
{
   SOCKET hSocket;
   struct sockaddr_in addr;
   int nBytes, nPort, nRet;
   socklen_t nAddrLen;
   char sMsg[MAX_SYSLOG_MSG_LEN];
   DB_RESULT hResult;
   THREAD hProcessingThread;
   fd_set rdfs;
   struct timeval tv;

   // Determine first available message id
   hResult = DBSelect(g_hCoreDB, _T("SELECT max(msg_id) FROM syslog"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         m_qwMsgId = max(DBGetFieldUInt64(hResult, 0, 0) + 1, m_qwMsgId);
      }
      DBFreeResult(hResult);
   }

   hSocket = socket(AF_INET, SOCK_DGRAM, 0);
   if (hSocket == -1)
   {
      WriteLog(MSG_SOCKET_FAILED, EVENTLOG_ERROR_TYPE, "s", "SyslogDaemon");
      return THREAD_OK;
   }

	SetSocketReuseFlag(hSocket);

   // Get listen port number
   nPort = ConfigReadInt(_T("SyslogListenPort"), 514);
   if ((nPort < 1) || (nPort > 65535))
      nPort = 514;

   // Fill in local address structure
   memset(&addr, 0, sizeof(struct sockaddr_in));
   addr.sin_family = AF_INET;
   addr.sin_addr.s_addr = ResolveHostName(g_szListenAddress);
   addr.sin_port = htons((WORD)nPort);

   // Bind socket
   if (bind(hSocket, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) != 0)
   {
      WriteLog(MSG_BIND_ERROR, EVENTLOG_ERROR_TYPE, "dse", nPort, "SyslogDaemon", WSAGetLastError());
      closesocket(hSocket);
      return THREAD_OK;
   }
	WriteLog(MSG_LISTENING_FOR_SYSLOG, EVENTLOG_INFORMATION_TYPE, "ad", ntohl(addr.sin_addr.s_addr), nPort);

   // Start processing thread
   m_pSyslogQueue = new Queue(1000, 100);
   hProcessingThread = ThreadCreateEx(SyslogProcessingThread, 0, NULL);

   DbgPrintf(1, _T("Syslog Daemon started"));

   // Wait for packets
   while(!ShutdownInProgress())
   {
      FD_ZERO(&rdfs);
      FD_SET(hSocket, &rdfs);
      tv.tv_sec = 1;
      tv.tv_usec = 0;
      nRet = select((int)hSocket + 1, &rdfs, NULL, NULL, &tv);
      if (nRet > 0)
      {
         nAddrLen = sizeof(struct sockaddr_in);
         nBytes = recvfrom(hSocket, sMsg, MAX_SYSLOG_MSG_LEN, 0,
                           (struct sockaddr *)&addr, &nAddrLen);
         if (nBytes > 0)
         {
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
   m_pSyslogQueue->Put(INVALID_POINTER_VALUE);
   ThreadJoin(hProcessingThread);
   delete m_pSyslogQueue;

   DbgPrintf(1, _T("Syslog Daemon stopped"));
   return THREAD_OK;
}


//
// Create NXCP message from NX_LOG_RECORd structure
//

void CreateMessageFromSyslogMsg(CSCPMessage *pMsg, NX_LOG_RECORD *pRec)
{
   DWORD dwId = VID_SYSLOG_MSG_BASE;

   pMsg->SetVariable(VID_NUM_RECORDS, (DWORD)1);
   pMsg->SetVariable(dwId++, pRec->qwMsgId);
   pMsg->SetVariable(dwId++, (DWORD)pRec->tmTimeStamp);
   pMsg->SetVariable(dwId++, (WORD)pRec->nFacility);
   pMsg->SetVariable(dwId++, (WORD)pRec->nSeverity);
   pMsg->SetVariable(dwId++, pRec->dwSourceObject);
   pMsg->SetVariable(dwId++, pRec->szHostName);
   pMsg->SetVariable(dwId++, pRec->szTag);
   pMsg->SetVariable(dwId++, pRec->szMessage);
}
