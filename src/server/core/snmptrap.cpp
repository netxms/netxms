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
#include <nxsnmp.h>


//
// Constants
//

#define MAX_PACKET_LENGTH     65536


//
// SNMP trap receiver thread
//

THREAD_RESULT THREAD_CALL SNMPTrapReceiver(void *pArg)
{
   SOCKET hSocket;
   struct sockaddr_in addr;
   int iAddrLen, iBytes;
   BYTE *packet;
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

   packet = (BYTE *)malloc(MAX_PACKET_LENGTH);

   // Wait for packets
   while(!ShutdownInProgress())
   {
      iAddrLen = sizeof(struct sockaddr_in);
      iBytes = recvfrom(hSocket, (char *)packet, MAX_PACKET_LENGTH, 0, (struct sockaddr *)&addr, &iAddrLen);
      if (iBytes > 0)
      {
         printf("SNMP: %d bytes packet received\n", iBytes);
         pdu = new SNMP_PDU;
         if (pdu->Parse(packet, iBytes))
         {
            printf("SNMP: version=%d community='%s'\nOID: %s\n",
               pdu->GetVersion(),pdu->GetCommunity(),
               pdu->GetTrapId() ? pdu->GetTrapId()->GetValueAsText() : "null");
            printf("     trap type = %d; spec type = %d\n",pdu->GetTrapType(),pdu->GetSpecificTrapType());
            for(DWORD i = 0; i < pdu->GetNumVariables(); i++)
            {
               SNMP_Variable *var;

               var = pdu->GetVariable(i);
               printf("%d: %s %02X\n", i, var->GetName()->GetValueAsText(), var->GetType());
            }
            delete pdu;
         }
         else
         {
            printf("SNMP: error parsing PDU\n");
         }
      }
      else
      {
         // Sleep on error
         ThreadSleepMs(100);
      }
   }

   free(packet);
   return THREAD_OK;
}
