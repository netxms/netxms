/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2011 Victor Kirhenshtein
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
   if (pSession->isAuthenticated())
      pSession->onNewEvent((Event *)pArg);
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
	   DbgPrintf(1, _T("Event storm detector thread stopped because event storm detection is off"));
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
				DbgPrintf(2, _T("Event storm detected: threshold=") INT64_FMT _T(" eventsPerSecond=") INT64_FMT, eventsPerSecond, numEvents);
				PostEvent(EVENT_EVENT_STORM_DETECTED, g_dwMgmtNode, "DdD", numEvents, duration, eventsPerSecond);
			}
		}
		else if ((numEvents < eventsPerSecond) && (g_dwFlags & AF_EVENT_STORM_DETECTED))
		{
			actualDuration = 0;
			g_dwFlags &= ~AF_EVENT_STORM_DETECTED;
		   DbgPrintf(2, _T("Event storm condition cleared"));
			PostEvent(EVENT_EVENT_STORM_ENDED, g_dwMgmtNode, "DdD", numEvents, duration, eventsPerSecond);
		}
	}
   DbgPrintf(1, _T("Event storm detector thread stopped"));
	return THREAD_OK;
}


//
// Event processing thread
//

THREAD_RESULT THREAD_CALL EventProcessor(void *arg)
{
   Event *pEvent;
	DWORD i;

	m_threadStormDetector = ThreadCreateEx(EventStormDetector, 0, NULL);
   while(!IsShutdownInProgress())
   {
      pEvent = (Event *)g_pEventQueue->GetOrBlock();
      if (pEvent == INVALID_POINTER_VALUE)
         break;   // Shutdown indicator

		if (g_dwFlags & AF_EVENT_STORM_DETECTED)
		{
	      delete pEvent;
	      g_totalEventsProcessed++;
			continue;
		}

      // Expand message text
      // We cannot expand message text in PostEvent because of
      // possible deadlock on g_rwlockIdIndex
      pEvent->expandMessageText();

      // Attempt to correlate event to some of previous events
      CorrelateEvent(pEvent);

		// Pass event to modules
      for(i = 0; i < g_dwNumModules; i++)
		{
			if (g_pModuleList[i].pfEventHandler != NULL)
				g_pModuleList[i].pfEventHandler(pEvent);
		}

      // Write event to log if required
		// Don't write SYS_DB_QUERY_FAILED to log to prevent
		// possible event recursion in case of severe DB failure
		if ((pEvent->getFlags() & EF_LOG) && (pEvent->getCode() != EVENT_DB_QUERY_FAILED))
      {
         TCHAR szQuery[8192];

         _sntprintf(szQuery, 8192, _T("INSERT INTO event_log (event_id,event_code,event_timestamp,")
                                   _T("event_source,event_severity,event_message,root_event_id,user_tag) ")
                                   _T("VALUES (") INT64_FMT _T(",%d,") TIME_T_FMT _T(",%d,%d,%s,") INT64_FMT _T(",%s)"), 
                  pEvent->getId(), pEvent->getCode(), pEvent->getTimeStamp(),
                  pEvent->getSourceId(), pEvent->getSeverity(),
						(const TCHAR *)DBPrepareString(g_hCoreDB, pEvent->getMessage(), EVENTLOG_MAX_MESSAGE_SIZE),
						pEvent->getRootId(), (const TCHAR *)DBPrepareString(g_hCoreDB, pEvent->getUserTag(), EVENTLOG_MAX_USERTAG_SIZE));
         QueueSQLRequest(szQuery);
      }

      // Send event to all connected clients
      EnumerateClientSessions(BroadcastEvent, pEvent);

      // Write event information to debug
      if (g_nDebugLevel >= 5)
      {
         NetObj *pObject = FindObjectById(pEvent->getSourceId());
         if (pObject == NULL)
            pObject = g_pEntireNet;
			DbgPrintf(5, _T("EVENT %d (ID:") UINT64_FMT _T(" F:0x%04X S:%d%s) FROM %s: %s"), pEvent->getCode(), 
                   pEvent->getId(), pEvent->getFlags(), pEvent->getSeverity(),
                   (pEvent->getRootId() == 0) ? _T("") : _T(" CORRELATED"),
                   pObject->Name(), pEvent->getMessage());
      }

      // Pass event through event processing policy if it is not correlated
      if (pEvent->getRootId() == 0)
		{
         g_pEventPolicy->ProcessEvent(pEvent);
			DbgPrintf(7, _T("Event ") UINT64_FMT _T(" with code %d passed event processing policy"), pEvent->getId(), pEvent->getCode());
		}

      // Destroy event
      delete pEvent;
		DbgPrintf(7, _T("Event object destroyed"));
      
      g_totalEventsProcessed++;
   }

	ThreadJoin(m_threadStormDetector);
   DbgPrintf(1, _T("Event processing thread stopped"));
   return THREAD_OK;
}
