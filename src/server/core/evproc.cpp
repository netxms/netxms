/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2019 Victor Kirhenshtein
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

#define DEBUG_TAG _T("event.proc")

#if WITH_ZMQ
#include "zeromq.h"
#endif

/**
 * Number of processed events since start
 */
INT64 g_totalEventsProcessed = 0;

/**
 * Static data
 */
static THREAD s_threadStormDetector = INVALID_THREAD_HANDLE;
static THREAD s_threadLogger = INVALID_THREAD_HANDLE;
static ObjectQueue<Event> s_loggerQueue;

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
   ThreadSetName("EvtStormDetect");

	INT64 numEvents, prevEvents, eventsPerSecond;
	int duration, actualDuration = 0;
	
	if (!ConfigReadBoolean(_T("EnableEventStormDetection"), false))
	{
		// Event storm detection is off
	   nxlog_debug_tag(DEBUG_TAG, 1, _T("Event storm detector thread stopped because event storm detection is off"));
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
				nxlog_debug_tag(DEBUG_TAG, 2, _T("Event storm detected: threshold=") INT64_FMT _T(" eventsPerSecond=") INT64_FMT, eventsPerSecond, numEvents);
				PostSystemEvent(EVENT_EVENT_STORM_DETECTED, g_dwMgmtNode, "DdD", numEvents, duration, eventsPerSecond);
			}
		}
		else if ((numEvents < eventsPerSecond) && (g_flags & AF_EVENT_STORM_DETECTED))
		{
			actualDuration = 0;
			g_flags &= ~AF_EVENT_STORM_DETECTED;
		   nxlog_debug_tag(DEBUG_TAG, 2, _T("Event storm condition cleared"));
		   PostSystemEvent(EVENT_EVENT_STORM_ENDED, g_dwMgmtNode, "DdD", numEvents, duration, eventsPerSecond);
		}
	}
   nxlog_debug_tag(DEBUG_TAG, 1, _T("Event storm detector thread stopped"));
	return THREAD_OK;
}

/**
 * Event logger
 */
static THREAD_RESULT THREAD_CALL EventLogger(void *arg)
{
   ThreadSetName("EventLogger");

   while(true)
   {
      Event *pEvent = s_loggerQueue.getOrBlock();
      if (pEvent == INVALID_POINTER_VALUE)
         break;   // Shutdown indicator

		DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
		int syntaxId = DBGetSyntax(hdb);
		if (syntaxId == DB_SYNTAX_SQLITE)
		{ 
			StringBuffer query = _T("INSERT INTO event_log (event_id,event_code,event_timestamp,event_source,zone_uin,origin,")
			               _T("origin_timestamp,dci_id,event_severity,event_message,root_event_id,event_tags,raw_data) VALUES (");
			query.append(pEvent->getId());
         query.append(_T(','));
			query.append(pEvent->getCode());
         query.append(_T(','));
         query.append((UINT32)pEvent->getTimestamp());
         query.append(_T(','));
         query.append(pEvent->getSourceId());
         query.append(_T(','));
         query.append(pEvent->getZoneUIN());
         query.append(_T(','));
         query.append(static_cast<INT32>(pEvent->getOrigin()));
         query.append(_T(','));
         query.append(static_cast<UINT32>(pEvent->getOriginTimestamp()));
         query.append(_T(','));
         query.append(pEvent->getDciId());
         query.append(_T(','));
         query.append(pEvent->getSeverity());
         query.append(_T(','));
         query.append(DBPrepareString(hdb, pEvent->getMessage(), MAX_EVENT_MSG_LENGTH));
         query.append(_T(','));
         query.append(pEvent->getRootId());
         query.append(_T(','));
         query.append(DBPrepareString(hdb, pEvent->getTagsAsList(), 2000));
         query.append(_T(','));
         json_t *json = pEvent->toJson();
         char *jsonText = json_dumps(json, JSON_INDENT(3) | JSON_EMBED);
         query.append(DBPrepareStringUTF8(hdb, jsonText));
         MemFree(jsonText);
         json_decref(json);
         query.append(_T(')'));

			DBQuery(hdb, query);
			nxlog_debug_tag(DEBUG_TAG, 8, _T("EventLogger: DBQuery: id=%d,code=%d"), (int)pEvent->getId(), (int)pEvent->getCode());
			delete pEvent;
		}
		else
		{
			DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO event_log (event_id,event_code,event_timestamp,origin,")
				_T("origin_timestamp,event_source,zone_uin,dci_id,event_severity,event_message,root_event_id,event_tags,raw_data) ")
				_T("VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?)"), true);
			if (hStmt != NULL)
			{
				do
				{
					DBBind(hStmt, 1, DB_SQLTYPE_BIGINT, pEvent->getId());
					DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, pEvent->getCode());
					DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, static_cast<UINT32>(pEvent->getTimestamp()));
               DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, static_cast<INT32>(pEvent->getOrigin()));
               DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, static_cast<UINT32>(pEvent->getOriginTimestamp()));
					DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, pEvent->getSourceId());
               DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, pEvent->getZoneUIN());
               DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, pEvent->getDciId());
					DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, pEvent->getSeverity());
               DBBind(hStmt, 10, DB_SQLTYPE_VARCHAR, pEvent->getMessage(), DB_BIND_STATIC, MAX_EVENT_MSG_LENGTH);
					DBBind(hStmt, 11, DB_SQLTYPE_BIGINT, pEvent->getRootId());
               DBBind(hStmt, 12, DB_SQLTYPE_VARCHAR, pEvent->getTagsAsList(), DB_BIND_TRANSIENT, 2000);
               DBBind(hStmt, 13, DB_SQLTYPE_TEXT, pEvent->toJson(), DB_BIND_DYNAMIC);
					DBExecute(hStmt);
					nxlog_debug_tag(DEBUG_TAG, 8, _T("EventLogger: DBExecute: id=%d,code=%d"), (int)pEvent->getId(), (int)pEvent->getCode());
					delete pEvent;
					pEvent = s_loggerQueue.getOrBlock(500);
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
   ThreadSetName("EventProcessor");

	s_threadLogger = ThreadCreateEx(EventLogger, 0, NULL);
	s_threadStormDetector = ThreadCreateEx(EventStormDetector, 0, NULL);
   while(true)
   {
      Event *pEvent = g_eventQueue.getOrBlock();
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

      NetObj *sourceObject = FindObjectById(pEvent->getSourceId());
      if (sourceObject == NULL)
      {
         sourceObject = FindObjectById(g_dwMgmtNode);
         if (sourceObject == NULL)
            sourceObject = g_pEntireNet;
      }

      ScriptVMHandle vm = CreateServerScriptVM(_T("Hook::EventProcessor"), sourceObject);
      if (vm.isValid())
      {
         nxlog_debug_tag(DEBUG_TAG, 7, _T("Running event processor hook script"));
         vm->setGlobalVariable("$event", vm->createValue(new NXSL_Object(vm, &g_nxslEventClass, pEvent, true)));
         if (!vm->run())
         {
            if (pEvent->getCode() != EVENT_SCRIPT_ERROR) // To avoid infinite loop
            {
               PostSystemEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", _T("Hook::EventProcessor"), vm->getErrorText(), 0);
            }
            nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Event processor hook script execution error (%s)"), vm->getErrorText());
         }
      }

      // Send event to all connected clients
      EnumerateClientSessions(BroadcastEvent, pEvent);

      // Write event information to debug
      if (nxlog_get_debug_level_tag(DEBUG_TAG) >= 5)
      {
			nxlog_debug_tag(DEBUG_TAG, 5, _T("EVENT %s [%d] (ID:") UINT64_FMT _T(" F:0x%04X S:%d TAGS:\"%s\"%s) FROM %s: %s"),
                         pEvent->getName(), pEvent->getCode(), pEvent->getId(), pEvent->getFlags(), pEvent->getSeverity(),
						       (const TCHAR *)pEvent->getTagsAsList(),
                         (pEvent->getRootId() == 0) ? _T("") : _T(" CORRELATED"),
                         sourceObject->getName(), pEvent->getMessage());
      }

      // Pass event through event processing policy if it is not correlated
      if (pEvent->getRootId() == 0)
		{
#ifdef WITH_ZMQ
         ZmqPublishEvent(pEvent);
#endif

         g_pEventPolicy->processEvent(pEvent);
         nxlog_debug_tag(DEBUG_TAG, 7, _T("Event ") UINT64_FMT _T(" with code %d passed event processing policy"), pEvent->getId(), pEvent->getCode());
		}

      // Write event to log if required, otherwise destroy it
		// Don't write SYS_DB_QUERY_FAILED to log to prevent
		// possible event recursion in case of severe DB failure
		// Logger will destroy event object after logging
		if ((pEvent->getFlags() & EF_LOG) && (pEvent->getCode() != EVENT_DB_QUERY_FAILED))
		{
			s_loggerQueue.put(pEvent);
		}
		else
      {
			delete pEvent;
			nxlog_debug_tag(DEBUG_TAG, 7, _T("Event object destroyed"));
		}
      
      g_totalEventsProcessed++;
   }

	s_loggerQueue.put(INVALID_POINTER_VALUE);
	ThreadJoin(s_threadStormDetector);
	ThreadJoin(s_threadLogger);
   nxlog_debug_tag(DEBUG_TAG, 1, _T("Event processing thread stopped"));
   return THREAD_OK;
}

/**
 * Compare event with ID
 */
static bool CompareEvent(const UINT64 *id, const Event *event)
{
   return event->getId() == *id;
}

/**
 * Copy event (transformation for Queue::find)
 */
static Event *CopyEvent(Event *event)
{
   return new Event(event);
}

/**
 * Find event in logger's queue. If found, copy of queued event is returned.
 * Returned event object should be destroyed by caller.
 */
Event *FindEventInLoggerQueue(UINT64 eventId)
{
   return s_loggerQueue.find(&eventId, CompareEvent, CopyEvent);
}

/**
 * Get size of event log writer queue
 */
INT64 GetEventLogWriterQueueSize()
{
   return s_loggerQueue.size();
}
