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
// Static data
//

static CONDITION m_hCondSyncFinished;


//
// Process events coming from server
//

void ProcessEvent(CSCPMessage *pMsg)
{
   NXC_EVENT *pEvent;

   switch(pMsg->GetCode())
   {
      case CMD_EVENT_LIST_END:
         if (g_dwState == STATE_SYNC_EVENTS)
            ConditionSet(m_hCondSyncFinished);
         break;
      case CMD_EVENT:
         // Allocate new event structure and fill it with values from message
         pEvent = (NXC_EVENT *)MemAlloc(sizeof(NXC_EVENT));

         pEvent->dwEventId = pMsg->GetVariableLong("id");
         pEvent->dwSeverity = pMsg->GetVariableLong("severity");
         pEvent->dwSourceId = pMsg->GetVariableLong("source");
         pEvent->dwTimeStamp = pMsg->GetVariableLong("timestamp");
         pMsg->GetVariableStr("message", pEvent->szMessage, MAX_EVENT_MSG_LENGTH);

         // Call client's callback to handle new record
         // It's up to client to destroy allocated event structure
         CallEventHandler(NXC_EVENT_NEW_ELOG_RECORD, 0, pEvent);
         break;
      default:
         break;
   }
}


//
// Synchronize event log
//

void SyncEvents(void)
{
   CSCPMessage msg;

   ChangeState(STATE_SYNC_EVENTS);
   m_hCondSyncFinished = ConditionCreate();

   msg.SetCode(CMD_GET_EVENTS);
   msg.SetId(0);
   SendMsg(&msg);

   // Wait for object list end or for disconnection
   while(g_dwState != STATE_DISCONNECTED)
   {
      if (ConditionWait(m_hCondSyncFinished, 500))
         break;
   }

   ConditionDestroy(m_hCondSyncFinished);
   if (g_dwState != STATE_DISCONNECTED)
      ChangeState(STATE_IDLE);
}
