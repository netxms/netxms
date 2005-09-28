/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004, 2005 Victor Kirhenshtein
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
** $module: syslogd.cpp
**
**/

#include "nxcore.h"
#include <nxlog.h>


//
// Constants
//

#define MAX_SYSLOG_MSG_LEN    1024


//
// Static data
//

static QWORD m_qwMsgId = 1;


//
// Parse timestamp field
//

static BOOL ParseTimeStamp(char **ppStart, int nMsgSize, int *pnPos, DWORD *pdwTime)
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
   if (*pCurr != ' ')
      return FALSE;  // Space should follow timestamp
   pCurr++;

   // Convert to system time
   *pdwTime = mktime(&timestamp);
   if (*pdwTime == 0xFFFFFFFF)
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
   if (ParseTimeStamp(&pCurr, nMsgLen, &nPos, &pRec->dwTimeStamp))
   {
      // Hostname
      for(i = 0; isalnum(*pCurr) && (i < MAX_SYSLOG_HOSTNAME_LEN) && (nPos < nMsgLen); i++, nPos++, pCurr++)
         pRec->szHostName[i] = *pCurr;
      if ((nPos >= nMsgLen) || (*pCurr != ' '))
         return FALSE;  // space should follow host name
      pCurr++;
      nPos++;
   }
   else
   {
      pRec->dwTimeStamp = time(NULL);
   }

   // Parse MSG part
   for(i = 0; isalnum(*pCurr) && (i < MAX_SYSLOG_TAG_LEN) && (nPos < nMsgLen); i++, nPos++, pCurr++)
      pRec->szTag[i] = *pCurr;
   if ((i == MAX_SYSLOG_TAG_LEN) || (nPos >= nMsgLen))
   {
      // Too long tag, assuming that it's a part of message
      pCurr -= i;
      nPos -= i;
      pRec->szTag[0] = 0;
   }
   else
   {
      if (*pCurr == ' ')
      {
         pCurr++;
         nPos++;
      }
   }
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
         pRec->dwSourceObject = pNode->Id();
   }
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
      BindMsgToNode(&record, dwSourceIP);
      pszEscMsg = EncodeSQLString(record.szMessage);
      _sntprintf(szQuery, 4096, 
                 _T("INSERT INTO syslog (msg_id,msg_timestamp,facility,severity,")
                 _T("source_object_id,hostname,msg_tag,msg_text) VALUES ")
                 _T("(" UINT64_FMT ",%ld,%d,%d,%ld,'%s','%s','%s')"),
                 m_qwMsgId++, record.dwTimeStamp, record.nFacility,
                 record.nSeverity, record.dwSourceObject,
                 record.szHostName, record.szTag, pszEscMsg);
      free(pszEscMsg);
      DBQuery(g_hCoreDB, szQuery);
   }
}


//
// Syslog messages receiver thread
//

THREAD_RESULT THREAD_CALL SyslogDaemon(void *pArg)
{
   SOCKET hSocket;
   struct sockaddr_in addr;
   int nBytes, nPort;
   socklen_t nAddrLen;
   char sMsg[MAX_SYSLOG_MSG_LEN];
   DB_RESULT hResult;

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
   addr.sin_addr.s_addr = htonl(INADDR_ANY);
   addr.sin_port = htons((WORD)nPort);

   // Bind socket
   if (bind(hSocket, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) != 0)
   {
      WriteLog(MSG_BIND_ERROR, EVENTLOG_ERROR_TYPE, "dse", nPort, "SyslogDaemon", WSAGetLastError());
      closesocket(hSocket);
      return THREAD_OK;
   }

   DbgPrintf(AF_DEBUG_MISC, _T("Syslog Daemon started"));

   // Wait for packets
   while(!ShutdownInProgress())
   {
      nAddrLen = sizeof(struct sockaddr_in);
      nBytes = recvfrom(hSocket, sMsg, MAX_SYSLOG_MSG_LEN, 0,
                        (struct sockaddr *)&addr, &nAddrLen);
      if (nBytes > 0)
      {
         ProcessSyslogMessage(sMsg, nBytes, ntohl(addr.sin_addr.s_addr));
      }
      else
      {
         // Sleep on error
         ThreadSleepMs(100);
      }
   }

   DbgPrintf(AF_DEBUG_SNMP, _T("SNMP Trap Receiver terminated"));
   return THREAD_OK;
}
