/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004 Victor Kirhenshtein
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
** $module: snmptrap.cpp
**
**/

#include "nms_core.h"


//
// Constants
//

#define MAX_PACKET_LENGTH     65536


//
// Process trap
//

static void ProcessTrap(SNMP_PDU *pdu, struct sockaddr_in *pOrigin)
{
   DWORD i, dwOriginAddr, dwBufPos, dwBufSize;
   TCHAR *pszTrapArgs, szBuffer[512];
   SNMP_Variable *pVar;
   Node *pNode;

   dwOriginAddr = ntohl(pOrigin->sin_addr.s_addr);
   DbgPrintf(AF_DEBUG_SNMP, "Received SNMP trap %s from %s\n", 
             pdu->GetTrapId()->GetValueAsText(), IpToStr(dwOriginAddr, szBuffer));

   // Match IP address to object
   pNode = FindNodeByIP(dwOriginAddr);
   if (pNode != NULL)
   {
      // Build trap's parameters string
      dwBufSize = pdu->GetNumVariables() * 4096;
      pszTrapArgs = (TCHAR *)malloc(sizeof(TCHAR) * dwBufSize);
      for(i = 0, dwBufPos = 0; i < pdu->GetNumVariables(); i++)
      {
         pVar = pdu->GetVariable(i);
         dwBufPos += _sntprintf(&pszTrapArgs[dwBufPos], dwBufSize - dwBufPos, _T("%s%s == '%s'"),
                                (i == 0) ? _T("") : _T("; "),
                                pVar->GetName()->GetValueAsText(), 
                                pVar->GetValueAsString(szBuffer, 512));
      }

      // Generate default event for unmatched traps
      PostEvent(EVENT_SNMP_UNMATCHED_TRAP, pNode->Id(), "ss", 
                pdu->GetTrapId()->GetValueAsText(), pszTrapArgs);
      free(pszTrapArgs);
   }
}


//
// SNMP trap receiver thread
//

THREAD_RESULT THREAD_CALL SNMPTrapReceiver(void *pArg)
{
   SOCKET hSocket;
   struct sockaddr_in addr;
   int iAddrLen, iBytes;
   SNMP_Transport *pTransport;
   SNMP_PDU *pdu;

   hSocket = socket(AF_INET, SOCK_DGRAM, 0);
   if (hSocket == -1)
   {
      WriteLog(MSG_SOCKET_FAILED, EVENTLOG_ERROR_TYPE, "s", "SNMPTrapReceiver");
      return THREAD_OK;
   }

   // Fill in local address structure
   memset(&addr, 0, sizeof(struct sockaddr_in));
   addr.sin_family = AF_INET;
   addr.sin_addr.s_addr = htonl(INADDR_ANY);
   addr.sin_port = htons(162);

   // Bind socket
   if (bind(hSocket, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) != 0)
   {
      WriteLog(MSG_BIND_ERROR, EVENTLOG_ERROR_TYPE, "dse", 162, "SNMPTrapReceiver", WSAGetLastError());
      closesocket(hSocket);
      return THREAD_OK;
   }

   pTransport = new SNMP_Transport(hSocket);
   DbgPrintf(AF_DEBUG_SNMP, _T("SNMP Trap Receiver started"));

   // Wait for packets
   while(!ShutdownInProgress())
   {
      iAddrLen = sizeof(struct sockaddr_in);
      iBytes = pTransport->Read(&pdu, 2000, (struct sockaddr *)&addr, &iAddrLen);
      if ((iBytes > 0) && (pdu != NULL))
      {
         if (pdu->GetCommand() == SNMP_TRAP)
            ProcessTrap(pdu, &addr);
         delete pdu;
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
