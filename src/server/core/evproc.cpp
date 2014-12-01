/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2013 Victor Kirhenshtein
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

/**
 * Number of processed events since start
 */
INT64 g_totalEventsProcessed = 0;

/**
 * Static data
 */
static THREAD s_threadStormDetector = INVALID_THREAD_HANDLE;
static THREAD s_threadLogger = INVALID_THREAD_HANDLE;
static Queue *s_loggerQueue = NULL;

/**
 * Handler for EnumerateSessions()
 */
static void BroadcastEvent(ClientSession *pSession, void *pArg)
{
   if (pSession->isAuthenticated())
      pSession->onNewEvent((Event *)pArg);
}

/**
 * Event storm detector thread
 */
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
	while(!(g_flags & AF_SHUTDOWN))
	{
		ThreadSleepMs(1000);
		numEvents = g_totalEventsProcessed - prevEvents;
		prevEvents = g_totalEventsProcessed;
		if ((numEvents >= eventsPerSecond) && (!(g_flags & AF_EVENT_STORM_DETECTED)))
		{
			actualDuration++;
			if (actualDuration >= duration)
			{
				g_flags |= AF_EVENT_STORM_DETECTED;
				DbgPrintf(2, _T("Event storm detected: threshold=") INT64_FMT _T(" eventsPerSecond=") INT64_FMT, eventsPerSecond, numEvents);
				PostEvent(EVENT_EVENT_STORM_DETECTED, g_dwMgmtNode, "DdD", numEvents, duration, eventsPerSecond);
			}
		}
		else if ((numEvents < eventsPerSecond) && (g_flags & AF_EVENT_STORM_DETECTED))
		{
			actualDuration = 0;
			g_flags &= ~AF_EVENT_STORM_DETECTED;
		   DbgPrintf(2, _T("Event storm condition cleared"));
			PostEvent(EVENT_EVENT_STORM_ENDED, g_dwMgmtNode, "DdD", numEvents, duration, eventsPerSecond);
		}
	}
   DbgPrintf(1, _T("Event storm detector thread stopped"));
	return THREAD_OK;
}

/**
 * Event logger
 */
static THREAD_RESULT THREAD_CALL EventLogger(void *arg)
{
   while(!IsShutdownInProgress())
   {
      Event *pEvent = (Event *)s_loggerQueue->GetOrBlock();
      if (pEvent == INVALID_POINTER_VALUE)
         break;   // Shutdown indicator

		DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
		int syntaxId = DBGetSyntax(hdb);
		if (syntaxId == DB_SYNTAX_SQLITE)
		{ 
			TCHAR szQuery[4096];
			_sntprintf(szQuery, 4096, _T("INSERT INTO event_log (event_id,event_code,event_timestamp,event_source,")
											  _T("event_severity,event_message,root_event_id,user_tag) VALUES (") UINT64_FMT 
											  _T(",%d,%d,%d,%d,%s,") UINT64_FMT _T(",%s)"), pEvent->getId(), pEvent->getCode(), 
											  (UINT32)pEvent->getTimeStamp(), pEvent->getSourceId(), pEvent->getSeverity(), 
											  (const TCHAR *)DBPrepareString(hdb, pEvent->getMessage(), MAX_EVENT_MSG_LENGTH - 1),
											  pEvent->getRootId(), (const TCHAR *)DBPrepareString(hdb, pEvent->getUserTag(), 63));
			DBQuery(hdb, szQuery);
			DbgPrintf(8, _T("EventLogger: DBQuery: id=%d,code=%d"), (int)pEvent->getId(), (int)pEvent->getCode());
			delete pEvent;
		}
		else
		{
			DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO event_log (event_id,event_code,event_timestamp,")
				_T("event_source,event_severity,event_message,root_event_id,user_tag) ")
				_T("VALUES (?,?,?,?,?,?,?,?)"));
			if (hStmt != NULL)
			{
				do
				{
					DBBind(hStmt, 1, DB_SQLTYPE_BIGINT, pEvent->getId());
					DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, pEvent->getCode());
					DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, (UINT32)pEvent->getTimeStamp());
					DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, pEvent->getSourceId());
					DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, pEvent->getSeverity());
					if (_tcslen(pEvent->getMessage()) < MAX_EVENT_MSG_LENGTH)
					{
						DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, pEvent->getMessage(), DB_BIND_STATIC);
					}
					else
					{
						TCHAR *temp = _tcsdup(pEvent->getMessage());
						temp[MAX_EVENT_MSG_LENGTH - 1] = 0;
						DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, temp, DB_BIND_DYNAMIC);
					}
					DBBind(hStmt, 7, DB_SQLTYPE_BIGINT, pEvent->getRootId());
					if (_tcslen(pEvent->getMessage()) <= 63)
					{
						DBBind(hStmt, 8, DB_SQLTYPE_VARCHAR, pEvent->getUserTag(), DB_BIND_STATIC);
					}
					else
					{
						TCHAR *temp = _tcsdup(pEvent->getMessage());
						temp[63] = 0;
						DBBind(hStmt, 8, DB_SQLTYPE_VARCHAR, temp, DB_BIND_DYNAMIC);
					}
					DBExecute(hStmt);
					DbgPrintf(8, _T("EventLogger: DBExecute: id=%d,code=%d"), (int)pEvent->getId(), (int)pEvent->getCode());
					delete pEvent;
					pEvent = (Event *)s_loggerQueue->Get();
				} while((pEvent != NULL) && (pEvent != INVALID_POINTER_VALUE));
				DBFreeStatement(hStmt);
			}
			else
			{
				delete pEvent;
			}
		}

		DBConnectionPoolReleaseConnection(hdb);

		if (pEvent == INVALID_POINTER_VALUE)
         break;   // Shutdown indicator (need second check if got it in inner loop)
	}
	return THREAD_OK;
}

/**
 * Event processing thread
 */
THREAD_RESULT THREAD_CALL EventProcessor(void *arg)
{
	s_loggerQueue = new Queue;
	s_threadLogger = ThreadCreateEx(EventLogger, 0, NULL);
	s_threadStormDetector = ThreadCreateEx(EventStormDetector, 0, NULL);
   while(!IsShutdownInProgress())
   {
      Event *pEvent = (Event *)g_pEventQueue->GetOrBlock();
      if (pEvent == INVALID_POINTER_VALUE)
         break;   // Shutdown indicator

		if (g_flags & AF_EVENT_STORM_DETECTED)
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
      CALL_ALL_MODULES(pfEventHandler, (pEvent));

      // Send event to all connected clients
      EnumerateClientSessions(BroadcastEvent, pEvent);

      // Write event information to debug
      if (g_debugLevel >= 5)
      {
         NetObj *pObject = FindObjectById(pEvent->getSourceId());
         if (pObject == NULL)
            pObject = g_pEntireNet;
			DbgPrintf(5, _T("EVENT %d (ID:") UINT64_FMT _T(" F:0x%04X S:%d TAG:\"%s\"%s) FROM %s: %s"), pEvent->getCode(), 
                   pEvent->getId(), pEvent->getFlags(), pEvent->getSeverity(),
						 CHECK_NULL_EX(pEvent->getUserTag()),
                   (pEvent->getRootId() == 0) ? _T("") : _T(" CORRELATED"),
                   pObject->getName(), pEvent->getMessage());
      }

      // Pass event through event processing policy if it is not correlated
      if (pEvent->getRootId() == 0)
		{
         g_pEventPolicy->processEvent(pEvent);
			DbgPrintf(7, _T("Event ") UINT64_FMT _T(" with code %d passed event processing policy"), pEvent->getId(), pEvent->getCode());
		}

      // Write event to log if required, otherwise destroy it
		// Don't write SYS_DB_QUERY_FAILED to log to prevent
		// possible event recursion in case of severe DB failure
		// Logger will destroy event object after logging
		if ((pEvent->getFlags() & EF_LOG) && (pEvent->getCode() != EVENT_DB_QUERY_FAILED))
		{
			s_loggerQueue->Put(pEvent);
		}
		else
      {
			delete pEvent;
			DbgPrintf(7, _T("Event object destroyed"));
		}
      
      g_totalEventsProcessed++;
   }

	s_loggerQueue->Put(INVALID_POINTER_VALUE);
	ThreadJoin(s_threadStormDetector);
	ThreadJoin(s_threadLogger);
	delete s_loggerQueue;
   DbgPrintf(1, _T("Event processing thread stopped"));
   return THREAD_OK;
}
