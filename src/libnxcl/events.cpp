/* 
** NetXMS - Network Management System
** Client Library
** Copyright (C) 2004 Victor Kirhenshtein
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
** $module: events.cpp
**
**/

#include "libnxcl.h"


//
// Process events coming from server
//

void ProcessEvent(CSCPMessage *pMsg, CSCP_MESSAGE *pRawMsg)
{
   NXC_EVENT *pEvent;
   WORD wCode;

   wCode = (pMsg != NULL) ? pMsg->GetCode() : pRawMsg->wCode;

   switch(wCode)
   {
      case CMD_EVENT_LIST_END:
         CompleteSync(RCC_SUCCESS);
         break;
      case CMD_EVENT:
         if (pRawMsg != NULL)    // We should receive events as raw data
         {
            // Allocate new event structure and fill it with values from message
            pEvent = (NXC_EVENT *)malloc(sizeof(NXC_EVENT));
            memcpy(pEvent, pRawMsg->df, sizeof(NXC_EVENT));
            pEvent->dwEventId = ntohl(pEvent->dwEventId);
            pEvent->dwSeverity = ntohl(pEvent->dwSeverity);
            pEvent->dwSourceId = ntohl(pEvent->dwSourceId);
            pEvent->dwTimeStamp = ntohl(pEvent->dwTimeStamp);

            // Call client's callback to handle new record
            // It's up to client to destroy allocated event structure
            CallEventHandler(NXC_EVENT_NEW_ELOG_RECORD, 0, pEvent);
         }
         break;
      default:
         break;
   }
}


//
// Synchronize event log
// This function is NOT REENTRANT
//

DWORD LIBNXCL_EXPORTABLE NXCSyncEvents(void)
{
   CSCPMessage msg;
   DWORD dwRetCode, dwRqId;

   dwRqId = g_dwMsgId++;
   PrepareForSync();

   msg.SetCode(CMD_GET_EVENTS);
   msg.SetId(dwRqId);
   SendMsg(&msg);

   dwRetCode = WaitForRCC(dwRqId);
   if (dwRetCode == RCC_SUCCESS)
      dwRetCode = WaitForSync(INFINITE);

   return dwRetCode;
}
