/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004, 2005, 2006, 2007 Victor Kirhenshtein
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
// Global variables
//

INT64 g_totalEventsProcessed = 0;


//
// Static data
//

static THREAD m_threadStormDetector = INVALID_THREAD_HANDLE;


//
// Handler for EnumerateSessions()
//

static void BroadcastEvent(ClientSession *pSession, void *pArg)
{
   if (pSession->IsAuthenticated())
      pSession->OnNewEvent((Event *)pArg);
}


//
// Event storm detector thread
//

static THREAD_RESULT THREAD_CALL EventStormDetector(void *arg)
{
	INT64 numEvents, prevEvents, eventsPerSecond;
	int duration, actualDuration = 0;
	
	if (!ConfigReadInt(_T("EnableEventStormDetection"), 0))
	{
		// Event storm detection is off
	   DbgPrintf(1, "Event storm detector thread stopped because event storm detection is off");
		return THREAD_OK;
	}

	eventsPerSecond = ConfigReadInt(_T("EventStormEventsPerSecond"), 100);
	duration = ConfigReadInt(_T("EventStormDuraction"), 15);

	prevEvents = g_totalEventsProcessed;	
	while(!(g_dwFlags & AF_SHUTDOWN))
	{
		ThreadSleepMs(1000);
		numEvents = g_totalEventsProcessed - prevEvents;
		prevEvents = g_totalEventsProcessed;
		if ((numEvents >= eventsPerSecond) && (!(g_dwFlags & AF_EVENT_STORM_DETECTED)))
		{
			actualDuration++;
			if (actualDuration >= duration)
			{
				g_dwFlags |= AF_EVENT_STORM_DETECTED;
				DbgPrintf(2, "Event storm detected: threshold=" INT64_FMT " eventsPerSecond=" INT64_FMT, eventsPerSecond, numEvents);
				PostEvent(EVENT_EVENT_STORM_DETECTED, g_dwMgmtNode, "DdD", numEvents, duration, eventsPerSecond);
			}
		}
		else if ((numEvents < eventsPerSecond) && (g_dwFlags & AF_EVENT_STORM_DETECTED))
		{
			actualDuration = 0;
			g_dwFlags &= ~AF_EVENT_STORM_DETECTED;
		   DbgPrintf(2, "Event storm condition cleared");
			PostEvent(EVENT_EVENT_STORM_ENDED, g_dwMgmtNode, "DdD", numEvents, duration, eventsPerSecond);
		}
	}
   DbgPrintf(1, "Event storm detector thread stopped");
	return THREAD_OK;
}


//
// Event processing thread
//

THREAD_RESULT THREAD_CALL EventProcessor(void *arg)
{
   Event *pEvent;

	m_threadStormDetector = ThreadCreateEx(EventStormDetector, 0, NULL);
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
      if (IsStandalone() && (g_nDebugLevel >= 5))
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
      
      g_totalEventsProcessed++;
   }

	ThreadJoin(m_threadStormDetector);
   DbgPrintf(1, "Event processing thread stopped");
   return THREAD_OK;
}
