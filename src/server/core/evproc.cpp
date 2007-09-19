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
** File: evproc.cpp
**
**/

#include "nxcore.h"


//
// Handler for EnumerateSessions()
//

static void BroadcastEvent(ClientSession *pSession, void *pArg)
{
   if (pSession->IsAuthenticated())
      pSession->OnNewEvent((Event *)pArg);
}


//
// Event processing thread
//

THREAD_RESULT THREAD_CALL EventProcessor(void *arg)
{
   Event *pEvent;

   while(!ShutdownInProgress())
   {
      pEvent = (Event *)g_pEventQueue->GetOrBlock();
      if (pEvent == INVALID_POINTER_VALUE)
         break;   // Shutdown indicator

      // Expand message text
      // We cannot expand message text in PostEvent because of
      // possible deadlock on g_rwlockIdIndex
      pEvent->ExpandMessageText();

      // Attempt to correlate event to some of previous events
      CorrelateEvent(pEvent);

      // Write event to log if required
      if (pEvent->Flags() & EF_LOG)
      {
         char *pszMsg, *pszTag, szQuery[2048];

         pszMsg = EncodeSQLString(pEvent->Message());
         pszTag = EncodeSQLString(pEvent->UserTag());
         snprintf(szQuery, 2048, "INSERT INTO event_log (event_id,event_code,event_timestamp,"
                                 "event_source,event_severity,event_message,root_event_id,user_tag) "
#ifdef _WIN32
                                 "VALUES (%I64d,%d,%d,%d,%d,'%s',%I64d,'%s')", 
#else
                                 "VALUES (%lld,%d,%d,%d,%d,'%s',%lld,'%s')", 
#endif
                  pEvent->Id(), pEvent->Code(), pEvent->TimeStamp(),
                  pEvent->SourceId(), pEvent->Severity(), pszMsg,
                  pEvent->GetRootId(), pszTag);
         free(pszMsg);
			free(pszTag);
         QueueSQLRequest(szQuery);
      }

      // Send event to all connected clients
      EnumerateClientSessions(BroadcastEvent, pEvent);

      // Write event information to screen if event debugging is on
      if (IsStandalone() && (g_dwFlags & AF_DEBUG_EVENTS))
      {
         NetObj *pObject = FindObjectById(pEvent->SourceId());
         if (pObject == NULL)
            pObject = g_pEntireNet;
         printf("EVENT %d (F:0x%04X S:%d%s) FROM %s: %s\n", pEvent->Code(), 
                pEvent->Flags(), pEvent->Severity(),
                (pEvent->GetRootId() == 0) ? "" : " CORRELATED",
                pObject->Name(), pEvent->Message());
      }

      // Pass event through event processing policy
      if (pEvent->GetRootId() == 0)
         g_pEventPolicy->ProcessEvent(pEvent);

      // Destroy event
      delete pEvent;
   }

   DbgPrintf(AF_DEBUG_EVENTS, "Event processing thread #%d stopped", arg);
   return THREAD_OK;
}
