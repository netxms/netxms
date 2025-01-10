/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Victor Kirhenshtein
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

#define DEBUG_TAG L"event.proc"

#define MAX_DB_QUERY_FAILED_EVENTS     30

/**
 * Reset script error counter
 */
void ResetScriptErrorEventCounter();

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
 * Event storm detector thread
 */
static void EventStormDetector()
{
   ThreadSetName("EvtStormDetect");

	if (!ConfigReadBoolean(L"EventStorm.EnableDetection", false))
	{
		// Event storm detection is off
	   nxlog_debug_tag(DEBUG_TAG, 1, _T("Event storm detector thread stopped because event storm detection is off"));
		return;
	}

	uint64_t eventsPerSecond = ConfigReadInt(L"EventStorm.EventsPerSecond", 100);
	int duration = ConfigReadInt(L"EventStorm.Duration", 15);

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
			   InterlockedOr64(&g_flags, AF_EVENT_STORM_DETECTED);
				nxlog_debug_tag(DEBUG_TAG, 2, L"Event storm detected: threshold=" INT64_FMT L" eventsPerSecond=" INT64_FMT, eventsPerSecond, numEvents);
            EventBuilder(EVENT_EVENT_STORM_DETECTED, g_dwMgmtNode)
               .param(L"eps", numEvents)
               .param(L"duration", duration)
               .param(L"threshold", eventsPerSecond)
               .post();
			}
		}
		else if ((numEvents < eventsPerSecond) && (g_flags & AF_EVENT_STORM_DETECTED))
		{
			actualDuration = 0;
			InterlockedAnd64(&g_flags, ~AF_EVENT_STORM_DETECTED);
		   nxlog_debug_tag(DEBUG_TAG, 2, L"Event storm condition cleared");
         EventBuilder(EVENT_EVENT_STORM_ENDED, g_dwMgmtNode)
            .param(L"eps", numEvents)
            .param(L"duration", duration)
            .param(L"threshold", eventsPerSecond)
            .post();
		}
	}
   nxlog_debug_tag(DEBUG_TAG, 1, L"Event storm detector thread stopped");
}

/**
 * Timestamps of latest SYS_DB_QUERY_FAILED events
 */
static time_t s_dbQueryFailedTimestamps[MAX_DB_QUERY_FAILED_EVENTS];
static int s_dbQueryFailedTimestampPos = 0;

/**
 * Check that event can be written to database
 */
static bool IsEventWriteAllowed(Event *event)
{
   // Don't write SYS_DB_QUERY_FAILED to log if happening too often to prevent
   // possible event recursion in case of severe DB failure
   if  (event->getCode() == EVENT_DB_QUERY_FAILED)
   {
      time_t now = time(nullptr);
      bool allow = false;
      for(int i = 0; i < MAX_DB_QUERY_FAILED_EVENTS; i++)
      {
         if (s_dbQueryFailedTimestamps[i] < now - 60)
         {
            allow = true;
            break;
         }
      }
      s_dbQueryFailedTimestamps[s_dbQueryFailedTimestampPos++] = event->getTimestamp();
      if (s_dbQueryFailedTimestampPos == MAX_DB_QUERY_FAILED_EVENTS)
         s_dbQueryFailedTimestampPos = 0;
      if (!allow)
         nxlog_debug_tag(DEBUG_TAG, 5, L"EventLogger: event %s with ID " UINT64_FMT L" dropped by rate limiter", event->getName(), event->getId());
      return allow;
   }
   return true;
}

/**
 * Write event
 */
static inline void WriteEvent(DB_STATEMENT hStmt, Event *event)
{
   DBBind(hStmt, 1, DB_SQLTYPE_BIGINT, event->getId());
   DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, event->getCode());
   DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(event->getTimestamp()));
   DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, static_cast<int32_t>(event->getOrigin()));
   DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(event->getOriginTimestamp()));
   DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, event->getSourceId());
   DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, event->getZoneUIN());
   DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, event->getDciId());
   DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, event->getSeverity());
   DBBind(hStmt, 10, DB_SQLTYPE_VARCHAR, event->getMessage(), DB_BIND_STATIC, MAX_EVENT_MSG_LENGTH);
   DBBind(hStmt, 11, DB_SQLTYPE_BIGINT, event->getRootId());
   DBBind(hStmt, 12, DB_SQLTYPE_VARCHAR, event->getTagsAsList(), DB_BIND_TRANSIENT, 2000);
   DBBind(hStmt, 13, DB_SQLTYPE_TEXT, event->toJson(), DB_BIND_DYNAMIC);
   DBExecute(hStmt);
   nxlog_debug_tag(DEBUG_TAG, 8, _T("EventLogger: DBExecute: id=%d,code=%d"), (int)event->getId(), (int)event->getCode());
}

/**
 * Event logger
 */
static void EventLogger()
{
   ThreadSetName("EventLogger");

   while(true)
   {
      Event *event = s_loggerQueue.getOrBlock();
      if (event == INVALID_POINTER_VALUE)
         break;   // Shutdown indicator

      if (!IsEventWriteAllowed(event))
      {
         delete event;
         continue;
      }

		DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
		int syntaxId = DBGetSyntax(hdb);
		if (syntaxId == DB_SYNTAX_SQLITE)
		{ 
         StringBuffer query(L"INSERT INTO event_log (event_id,event_code,event_timestamp,event_source,zone_uin,origin,"
                            L"origin_timestamp,dci_id,event_severity,event_message,root_event_id,event_tags,raw_data) VALUES (");
         query.append(event->getId());
         query.append(L',');
         query.append(event->getCode());
         query.append(L',');
         query.append(static_cast<int64_t>(event->getTimestamp()));
         query.append(L',');
         query.append(event->getSourceId());
         query.append(L',');
         query.append(event->getZoneUIN());
         query.append(L',');
         query.append(static_cast<int32_t>(event->getOrigin()));
         query.append(L',');
         query.append(static_cast<int64_t>(event->getOriginTimestamp()));
         query.append(L',');
         query.append(event->getDciId());
         query.append(L',');
         query.append(event->getSeverity());
         query.append(L',');
         query.append(DBPrepareString(hdb, event->getMessage(), MAX_EVENT_MSG_LENGTH));
         query.append(L',');
         query.append(event->getRootId());
         query.append(L',');
         query.append(DBPrepareString(hdb, event->getTagsAsList(), 2000));
         query.append(L',');
         json_t *json = event->toJson();
         char *jsonText = json_dumps(json, JSON_COMPACT);
         query.append(DBPrepareStringUTF8(hdb, jsonText));
         MemFree(jsonText);
         json_decref(json);
         query.append(L')');

         DBQuery(hdb, query);
         nxlog_debug_tag(DEBUG_TAG, 8, L"EventLogger: DBQuery: id=" UINT64_FMT L",code=%u", event->getId(), event->getCode());
			delete event;
		}
		else
		{
			DB_STATEMENT hStmt = DBPrepare(hdb,
			         (g_dbSyntax == DB_SYNTAX_TSDB) ?
                        L"INSERT INTO event_log (event_id,event_code,event_timestamp,origin,"
                        L"origin_timestamp,event_source,zone_uin,dci_id,event_severity,event_message,root_event_id,event_tags,raw_data) "
                        L"VALUES (?,?,to_timestamp(?),?,?,?,?,?,?,?,?,?,?)" :
                        L"INSERT INTO event_log (event_id,event_code,event_timestamp,origin,"
                        L"origin_timestamp,event_source,zone_uin,dci_id,event_severity,event_message,root_event_id,event_tags,raw_data) "
                        L"VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?)", true);
			if (hStmt != nullptr)
			{
			   WriteEvent(hStmt, event);
			   delete event;
            event = s_loggerQueue.getOrBlock(500);
			   while((event != nullptr) && (event != INVALID_POINTER_VALUE))
				{
				   if (IsEventWriteAllowed(event))
				   {
		            WriteEvent(hStmt, event);
				   }
					delete event;
					event = s_loggerQueue.getOrBlock(500);
				}
				DBFreeStatement(hStmt);
			}
			else
			{
				delete event;
			}
		}

		DBConnectionPoolReleaseConnection(hdb);
		if (event == INVALID_POINTER_VALUE)
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

   // Execute pre-processing callback if set
   event->executeCallback();

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
      vm->setGlobalVariable("$event", vm->createValue(vm->createObject(&g_nxslEventClass, event, true)));
      if (!vm->run())
      {
         if (event->getCode() != EVENT_SCRIPT_ERROR) // To avoid infinite loop
         {
            ReportScriptError(SCRIPT_CONTEXT_EVENT_PROC, FindObjectById(event->getSourceId()).get(), 0, vm->getErrorText(), _T("Hook::EventProcessor"));
         }
      }
      vm.destroy();
   }

   // Send event to all connected clients
   if (!(event->getFlags() & EF_DO_NOT_MONITOR))
   {
      EnumerateClientSessions([](ClientSession *session, void *event) {
         if (session->isAuthenticated())
            session->onNewEvent(static_cast<Event*>(event));
      }, event);
   }

   // Write event information to debug
   if (nxlog_get_debug_level_tag(DEBUG_TAG) >= 5)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("EVENT %s [%u] at {%d} (ID:") UINT64_FMT _T(" F:0x%04X S:%d TAGS:\"%s\"%s) FROM %s: %s"),
                      event->getName(), event->getCode(), processorId, event->getId(), event->getFlags(), event->getSeverity(),
                      (const TCHAR *)event->getTagsAsList(),
                      (event->getRootId() == 0) ? _T("") : _T(" CORRELATED"),
                      sourceObject->getName(), event->getMessage());
   }

   // Pass event through event processing policy
   g_pEventPolicy->processEvent(event);
   nxlog_debug_tag(DEBUG_TAG, 7, _T("Event ") UINT64_FMT _T(" with code %u passed event processing policy"), event->getId(), event->getCode());

   // Write event to log if required, otherwise destroy it
   // Logger will destroy event object after logging
   if (event->getFlags() & EF_LOG)
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

      if ((g_flags & AF_EVENT_STORM_DETECTED) && (event->getCode() != EVENT_EVENT_STORM_DETECTED))
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
   uint64_t processedEvents;
   int64_t averageWaitTime;
   uint64_t maxWaitTime;
   uint32_t bindings;

   EventProcessingThread() : queue(4096, Ownership::True)
   {
      thread = INVALID_THREAD_HANDLE;
      processedEvents = 0;
      averageWaitTime = 0;
      maxWaitTime = 0;
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
      Event *event = queue.getOrBlock();
      if (event == INVALID_POINTER_VALUE)
         break;   // Shutdown indicator

      int64_t waitTime = GetCurrentTimeMs() - event->getQueueTime();
      UpdateExpMovingAverage(averageWaitTime, EMA_EXP_180, waitTime);
      if (static_cast<uint32_t>(waitTime) > maxWaitTime)
         maxWaitTime = static_cast<uint32_t>(waitTime);
      EventQueueBinding *binding = event->getQueueBinding();
      ProcessEvent(event, id);
      InterlockedDecrement(&binding->usage);
      processedEvents++;
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

   TCHAR queueSelector[256];
   ConfigReadStr(_T("Events.Processor.QueueSelector"), queueSelector, 256, _T("%z"));
   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Parallel event processing enabled (queue selector \"%s\")"), queueSelector);

	int poolSize = ConfigReadInt(_T("Events.Processor.PoolSize"), 1);
	if (poolSize > 128)
	{
	   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Configured thread pool size for parallel event processing is to big (configured value %d, adjusted to 128)"), poolSize);
	   poolSize = 128;
	}

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
      Event *event = g_eventQueue.getOrBlock(10000);
      if (event == INVALID_POINTER_VALUE)
         break;   // Shutdown indicator

      time_t now;
      if (event != nullptr)
      {
         if ((g_flags & AF_EVENT_STORM_DETECTED) && (event->getCode() != EVENT_EVENT_STORM_DETECTED))
         {
            delete event;
            InterlockedIncrement64(&g_totalEventsProcessed);
            continue;
         }

         now = event->getTimestamp(); // Get current time from event, it should be (almost) current

         StringBuffer key = event->expandText(queueSelector, nullptr);
         char keyBytes[128];
         size_t keyLen = wchar_to_utf8(key.cstr(), key.length(), keyBytes, 128);

         EventQueueBinding *qb;
         HASH_FIND(hh, queueBindings, keyBytes, keyLen, qb);
         if ((qb == nullptr) || ((qb->usage == 0) && (qb->queue->size() > 0)))
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

               uint32_t size = static_cast<uint32_t>(s_processingThreads[i].queue.size());
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

            if (qb == nullptr)
            {
               qb = memoryPool.allocate();
               memset(qb, 0, sizeof(EventQueueBinding));
               qb->keyLength = keyLen;
               memcpy(qb->key, keyBytes, keyLen);
               HASH_ADD_KEYPTR(hh, queueBindings, qb->key, keyLen, qb);
            }
            else
            {
               s_processingThreads[qb->processingThread].bindings--;
               if (s_processingThreads[qb->processingThread].bindings == 0)
               {
                  s_processingThreads[qb->processingThread].averageWaitTime = 0;
               }
            }
            qb->queue = &s_processingThreads[selectedThread].queue;
            qb->processingThread = selectedThread;
            qb->usage = 1;
            s_processingThreads[selectedThread].bindings++;
         }
         else
         {
            InterlockedIncrement(&qb->usage);
         }
         event->setQueueTime(GetCurrentTimeMs());
         event->setQueueBinding(qb);
         qb->queue->put(event);
      }
      else
      {
         now = time(nullptr);
      }

      // Remove outdated bindings
      if ((event == nullptr) || (lastCleanupTime < now - 30))
      {
         nxlog_debug_tag(DEBUG_TAG, 8, _T("Running event queues binding cleanup"));

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
   nxlog_debug_tag(DEBUG_TAG, 1, L"Event processing thread stopped");
}

/**
 * Start event processor
 */
THREAD StartEventProcessor()
{
   memset(s_dbQueryFailedTimestamps, 0, sizeof(s_dbQueryFailedTimestamps));
   s_threadLogger = ThreadCreateEx(EventLogger);
   s_threadStormDetector = ThreadCreateEx(EventStormDetector);
   ThreadPoolScheduleRelative(g_mainThreadPool, 600000, ResetScriptErrorEventCounter);
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
      s.maxWaitTime = static_cast<uint32_t>(s_processingThreads[i].maxWaitTime);
      s.queueSize = static_cast<uint32_t>(s_processingThreads[i].queue.size());
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
   return new Event(*event);
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
