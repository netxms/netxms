/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2020 Victor Kirhenshtein
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
#include <uthash.h>

#define DEBUG_TAG _T("event.proc")

#if WITH_ZMQ
#include "zeromq.h"
#endif

/**
 * Number of processed events since start
 */
VolatileCounter64 g_totalEventsProcessed = 0;

/**
 * Static data
 */
static THREAD s_threadStormDetector = INVALID_THREAD_HANDLE;
static THREAD s_threadLogger = INVALID_THREAD_HANDLE;
static ObjectQueue<Event> s_loggerQueue(4096, Ownership::True);

/**
 * Handler for EnumerateSessions()
 */
static void BroadcastEvent(ClientSession *session, Event *event)
{
   if (session->isAuthenticated())
      session->onNewEvent(event);
}

/**
 * Event storm detector thread
 */
static void EventStormDetector()
{
   ThreadSetName("EvtStormDetect");

	if (!ConfigReadBoolean(_T("EventStorm.EnableDetection"), false))
	{
		// Event storm detection is off
	   nxlog_debug_tag(DEBUG_TAG, 1, _T("Event storm detector thread stopped because event storm detection is off"));
		return;
	}

	uint64_t eventsPerSecond = ConfigReadInt(_T("EventStorm.EventsPerSecond"), 100);
	int duration = ConfigReadInt(_T("EventStorm.Duration"), 15);

	int actualDuration = 0;
	uint64_t prevEvents = g_totalEventsProcessed;
	while(!(g_flags & AF_SHUTDOWN))
	{
		ThreadSleepMs(1000);
		uint64_t numEvents = g_totalEventsProcessed - prevEvents;
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
}

/**
 * Event logger
 */
static void EventLogger()
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
			DB_STATEMENT hStmt = DBPrepare(hdb,
			         (g_dbSyntax == DB_SYNTAX_TSDB) ?
                        _T("INSERT INTO event_log (event_id,event_code,event_timestamp,origin,")
                        _T("origin_timestamp,event_source,zone_uin,dci_id,event_severity,event_message,root_event_id,event_tags,raw_data) ")
                        _T("VALUES (?,?,to_timestamp(?),?,?,?,?,?,?,?,?,?,?)") :
                        _T("INSERT INTO event_log (event_id,event_code,event_timestamp,origin,")
                        _T("origin_timestamp,event_source,zone_uin,dci_id,event_severity,event_message,root_event_id,event_tags,raw_data) ")
                        _T("VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?)"), true);
			if (hStmt != nullptr)
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
				} while((pEvent != nullptr) && (pEvent != INVALID_POINTER_VALUE));
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
}

/**
 * Process event
 */
static void ProcessEvent(Event *event, int processorId)
{
   // Expand message text
   // We cannot expand message text in PostEvent because of
   // possible deadlock on g_rwlockIdIndex
   event->expandMessageText();

   // Attempt to correlate event to some of previous events
   CorrelateEvent(event);

   // Pass event to modules
   CALL_ALL_MODULES(pfEventHandler, (event));

   shared_ptr<NetObj> sourceObject = FindObjectById(event->getSourceId());
   if (sourceObject == nullptr)
   {
      sourceObject = FindObjectById(g_dwMgmtNode);
      if (sourceObject == nullptr)
         sourceObject = g_entireNetwork;
   }

   ScriptVMHandle vm = CreateServerScriptVM(_T("Hook::EventProcessor"), sourceObject);
   if (vm.isValid())
   {
      nxlog_debug_tag(DEBUG_TAG, 7, _T("Running event processor hook script"));
      vm->setGlobalVariable("$event", vm->createValue(new NXSL_Object(vm, &g_nxslEventClass, event, true)));
      if (!vm->run())
      {
         if (event->getCode() != EVENT_SCRIPT_ERROR) // To avoid infinite loop
         {
            PostSystemEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", _T("Hook::EventProcessor"), vm->getErrorText(), 0);
         }
         nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Event processor hook script execution error (%s)"), vm->getErrorText());
      }
   }

   // Send event to all connected clients
   EnumerateClientSessions(BroadcastEvent, event);

   // Write event information to debug
   if (nxlog_get_debug_level_tag(DEBUG_TAG) >= 5)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("EVENT %s [%d] at {%d} (ID:") UINT64_FMT _T(" F:0x%04X S:%d TAGS:\"%s\"%s) FROM %s: %s"),
                      event->getName(), event->getCode(), processorId, event->getId(), event->getFlags(), event->getSeverity(),
                      (const TCHAR *)event->getTagsAsList(),
                      (event->getRootId() == 0) ? _T("") : _T(" CORRELATED"),
                      sourceObject->getName(), event->getMessage());
   }

   // Pass event through event processing policy if it is not correlated
   if (event->getRootId() == 0)
   {
#ifdef WITH_ZMQ
      ZmqPublishEvent(event);
#endif

      g_pEventPolicy->processEvent(event);
      nxlog_debug_tag(DEBUG_TAG, 7, _T("Event ") UINT64_FMT _T(" with code %d passed event processing policy"), event->getId(), event->getCode());
   }

   // Write event to log if required, otherwise destroy it
   // Don't write SYS_DB_QUERY_FAILED to log to prevent
   // possible event recursion in case of severe DB failure
   // Logger will destroy event object after logging
   if ((event->getFlags() & EF_LOG) && (event->getCode() != EVENT_DB_QUERY_FAILED))
   {
      s_loggerQueue.put(event);
   }
   else
   {
      delete event;
      nxlog_debug_tag(DEBUG_TAG, 7, _T("Event object destroyed"));
   }

   InterlockedIncrement64(&g_totalEventsProcessed);
}

/**
 * Event processing thread for serial processing
 */
static void SerialEventProcessor()
{
   ThreadSetName("EventProcessor");

   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Parallel event processing disabled"));

   while(true)
   {
      Event *event = g_eventQueue.getOrBlock();
      if (event == INVALID_POINTER_VALUE)
         break;   // Shutdown indicator

      if (g_flags & AF_EVENT_STORM_DETECTED)
      {
         delete event;
         InterlockedIncrement64(&g_totalEventsProcessed);
         continue;
      }

      ProcessEvent(event, 0);
   }

   s_loggerQueue.put(INVALID_POINTER_VALUE);
   ThreadJoin(s_threadStormDetector);
   ThreadJoin(s_threadLogger);
   nxlog_debug_tag(DEBUG_TAG, 1, _T("Event processing thread stopped"));
}

/**
 * Event queue binding
 */
struct EventQueueBinding
{
   UT_hash_handle hh;
   size_t keyLength;
   char key[128];
   ObjectQueue<Event> *queue;
   VolatileCounter usage;
   int processingThread;
};

/**
 * Event processing thread
 */
struct EventProcessingThread
{
   ObjectQueue<Event> queue;
   THREAD thread;
   int64_t averageWaitTime;
   uint64_t processedEvents;
   uint32_t bindings;

   EventProcessingThread() : queue(4096, Ownership::True)
   {
      thread = INVALID_THREAD_HANDLE;
      averageWaitTime = 0;
      processedEvents = 0;
      bindings = 0;
   }

   void run(int id);

   uint32_t getAverageWaitTime() { return static_cast<uint32_t>(averageWaitTime / EMA_FP_1); }
};

/**
 * Event processing thread main loop
 */
void EventProcessingThread::run(int id)
{
   char tname[32];
   snprintf(tname, 32, "EP-%d", id);
   ThreadSetName(tname);

   while(true)
   {
      Event *event = queue.getOrBlock(1000);
      if (event == INVALID_POINTER_VALUE)
         break;   // Shutdown indicator

      if (event != nullptr)
      {
         UpdateExpMovingAverage(averageWaitTime, EMA_EXP_180, GetCurrentTimeMs() - event->getQueueTime());
         EventQueueBinding *binding = event->getQueueBinding();
         ProcessEvent(event, id);
         InterlockedDecrement(&binding->usage);
         processedEvents++;
      }
      else
      {
         // Idle loop
         UpdateExpMovingAverage(averageWaitTime, EMA_EXP_180, static_cast<int64_t>(0));
      }
   }
}

/**
 * Event processing threads
 */
static EventProcessingThread *s_processingThreads = nullptr;
static int s_processingThreadCount = 0;

/**
 * Event processing thread for parallel processing
 */
static void ParallelEventProcessor()
{
   ThreadSetName("EPMaster");

	int poolSize = ConfigReadInt(_T("Events.Processor.PoolSize"), 1);
   TCHAR queueSelector[256];
   ConfigReadStr(_T("Events.Processor.QueueSelector"), queueSelector, 256, _T("%z"));
   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Parallel event processing enabled (queue selector \"%s\")"), queueSelector);

   EventQueueBinding *queueBindings = nullptr;
   ObjectMemoryPool<EventQueueBinding> memoryPool(1024);

   s_processingThreads = new EventProcessingThread[poolSize];
   for(int i = 0; i < poolSize; i++)
      s_processingThreads[i].thread = ThreadCreateEx(&s_processingThreads[i], &EventProcessingThread::run, i + 1);
   s_processingThreadCount = poolSize;
   int *weights = static_cast<int*>(MemAllocLocal(sizeof(int) * poolSize));

   time_t lastCleanupTime = time(nullptr);

   while(true)
   {
      Event *event = g_eventQueue.getOrBlock();
      if (event == INVALID_POINTER_VALUE)
         break;   // Shutdown indicator

		if (g_flags & AF_EVENT_STORM_DETECTED)
		{
	      delete event;
	      InterlockedIncrement64(&g_totalEventsProcessed);
			continue;
		}

		time_t now = event->getTimestamp(); // Get current time from event, it should be (almost) current

      StringBuffer key = event->expandText(queueSelector, nullptr);
#ifdef UNICODE
      char keyBytes[128];
      size_t keyLen = wchar_to_utf8(key.cstr(), key.length(), keyBytes, 128);
#else
      char *keyBytes = key.getBuffer();
      size_t keyLen = key.length();
      if (keyLen > 128)
         keyLen = 128;
#endif

      EventQueueBinding *qb;
      HASH_FIND(hh, queueBindings, keyBytes, keyLen, qb);
      if (qb == nullptr)
      {
         // Select less loaded queue
         memset(weights, 0, sizeof(int) * poolSize);
         for(int i = 0; i < poolSize; i++)
         {
            uint32_t waitTime = s_processingThreads[i].getAverageWaitTime();
            if (waitTime == 0)
               weights[i]++;
            else
               weights[i] -= waitTime / 100 + 1;

            uint32_t size = s_processingThreads[i].queue.size();
            if (size == 0)
               weights[i] += 2;
            else
               weights[i] -= size / 10 + 1;

            uint32_t load = s_processingThreads[i].bindings;
            if (load == 0)
               weights[i] += 100;
            else
               weights[i] -= load / 10;
         }

         int selectedThread = 0;
         int bestWeight = INT_MIN;
         for(int i = 0; i < poolSize; i++)
            if (weights[i] > bestWeight)
            {
               bestWeight = weights[i];
               selectedThread = i;
            }

         qb = memoryPool.allocate();
         memset(qb, 0, sizeof(EventQueueBinding));
         qb->keyLength = keyLen;
         memcpy(qb->key, keyBytes, keyLen);
         qb->queue = &s_processingThreads[selectedThread].queue;
         qb->processingThread = selectedThread;
         qb->usage = 1;
         HASH_ADD_KEYPTR(hh, queueBindings, qb->key, keyLen, qb);
         s_processingThreads[selectedThread].bindings++;
      }
      else
      {
         InterlockedIncrement(&qb->usage);
      }
      event->setQueueTime(GetCurrentTimeMs());
      event->setQueueBinding(qb);
      qb->queue->put(event);

      // Remove outdated bindings
      if (lastCleanupTime < now - 60)
      {
         nxlog_debug_tag(DEBUG_TAG, 7, _T("Running event queues binding cleanup"));

         EventQueueBinding *curr, *tmp;
         HASH_ITER(hh, queueBindings, curr, tmp)
         {
            if (curr->usage == 0)
            {
               s_processingThreads[curr->processingThread].bindings--;
               if (s_processingThreads[curr->processingThread].bindings == 0)
               {
                  s_processingThreads[curr->processingThread].averageWaitTime = 0;
               }
               HASH_DEL(queueBindings, curr);
               memoryPool.free(curr);
            }
         }

         lastCleanupTime = now;
      }
   }

   for(int i = 0; i < poolSize; i++)
   {
      s_processingThreads[i].queue.put(INVALID_POINTER_VALUE);
      ThreadJoin(s_processingThreads[i].thread);
   }
   s_processingThreadCount = 0;
   delete[] s_processingThreads;
   HASH_CLEAR(hh, queueBindings);
   MemFreeLocal(weights);

	s_loggerQueue.put(INVALID_POINTER_VALUE);
	ThreadJoin(s_threadStormDetector);
	ThreadJoin(s_threadLogger);
   nxlog_debug_tag(DEBUG_TAG, 1, _T("Event processing thread stopped"));
}

/**
 * Start event processor
 */
THREAD StartEventProcessor()
{
   s_threadLogger = ThreadCreateEx(EventLogger);
   s_threadStormDetector = ThreadCreateEx(EventStormDetector);
   return (ConfigReadInt(_T("Events.Processor.PoolSize"), 1) > 1) ? ThreadCreateEx(ParallelEventProcessor) : ThreadCreateEx(SerialEventProcessor);
}

/**
 * Get size of event processor queue
 */
int64_t GetEventProcessorQueueSize()
{
   int64_t size = g_eventQueue.size();
   for(int i = 0; i < s_processingThreadCount; i++)
      size += s_processingThreads[i].queue.size();
   return size;
}

/**
 * Get stats for event processing threads. Returned array should be deleted by caller.
 */
StructArray<EventProcessingThreadStats> *GetEventProcessingThreadStats()
{
   auto stats = new StructArray<EventProcessingThreadStats>(s_processingThreadCount);
   for(int i = 0; i < s_processingThreadCount; i++)
   {
      EventProcessingThreadStats s;
      s.processedEvents = s_processingThreads[i].processedEvents;
      s.averageWaitTime = s_processingThreads[i].getAverageWaitTime();
      s.queueSize = s_processingThreads[i].queue.size();
      s.bindings = s_processingThreads[i].bindings;
      stats->add(&s);
   }
   return stats;
}

/**
 * Compare event with ID
 */
static bool CompareEvent(const uint64_t *id, const Event *event)
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
Event *FindEventInLoggerQueue(uint64_t eventId)
{
   return s_loggerQueue.find(&eventId, CompareEvent, CopyEvent);
}

/**
 * Get size of event log writer queue
 */
int64_t GetEventLogWriterQueueSize()
{
   return s_loggerQueue.size();
}
