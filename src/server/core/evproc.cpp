/* 
** Project X - Network Management System
** Copyright (C) 2003 Victor Kirhenshtein
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
** $module: evproc.cpp
**
**/

#include "nms_core.h"


//
// Handler for EnumerateSessions()
//

static void BroadcastEvent(ClientSession *pSession, void *pArg)
{
   if (pSession->GetState() == STATE_AUTHENTICATED)
      pSession->OnNewEvent((Event *)pArg);
}


//
// Event processing thread
//

void EventProcessor(void *arg)
{
   Event *pEvent;

   while(!ShutdownInProgress())
   {
      pEvent = (Event *)g_pEventQueue->GetOrBlock();

      // Write event to log if required
      if (pEvent->Flags() & EF_LOG)
      {
         char szQuery[1024];

         sprintf(szQuery, "INSERT INTO eventlog (event_id,timestamp,source,severity,message) "
                          "VALUES (%d,%d,%d,%d,'%s')", pEvent->Id(), pEvent->TimeStamp(),
                 pEvent->SourceId(), pEvent->Severity(), pEvent->Message());
         DBQuery(g_hCoreDB, szQuery);
      }

      // Send event to all connected clients
      EnumerateClientSessions(BroadcastEvent, pEvent);

      // Write event information to screen if event debugging is on
      if (IsStandalone() && (g_dwFlags & AF_DEBUG_EVENTS))
      {
         NetObj *pObject = FindObjectById(pEvent->SourceId());
         if (pObject == NULL)
            pObject = g_pEntireNet;
         printf("EVENT %d (F:0x%04X S:%d) FROM %s: %s\n", pEvent->Id(), 
                pEvent->Flags(), pEvent->Severity(), pObject->Name(), pEvent->Message());
      }

      // Destroy event
      delete pEvent;
   }
}
