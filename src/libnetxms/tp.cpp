/* 
** NetXMS - Network Management System
** NetXMS Foundation Library
** Copyright (C) 2003-2018 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: tp.cpp
**
**/

#include "libnetxms.h"
#include <nxqueue.h>

#define DEBUG_TAG _T("threads.pool")

/**
 * Exponential moving average calculation
 */
#define FP_SHIFT  11              /* nr of bits of precision */
#define FP_1      (1 << FP_SHIFT) /* 1.0 as fixed-point */
#define EXP_1     1884            /* 1/exp(5sec/1min) as fixed-point */
#define EXP_5     2014            /* 1/exp(5sec/5min) */
#define EXP_15    2037            /* 1/exp(5sec/15min) */ 
#define CALC_EMA(load, exp, n) do { load *= exp; load += n * (FP_1 - exp); load >>= FP_SHIFT; } while(0)

/**
 * Wait time watermarks (milliseconds)
 */
static UINT32 s_waitTimeHighWatermark = 200;
static UINT32 s_waitTimeLowWatermark = 100;

/**
 * Thread pool maintenance thread responsiveness
 */
static int s_maintThreadResponsiveness = 12;

/**
 * Worker thread idle timeout (milliseconds)
 */
#define MIN_WORKER_IDLE_TIMEOUT  10000
#define MAX_WORKER_IDLE_TIMEOUT  600000

/**
 * Worker thread data
 */
struct WorkerThreadInfo
{
   ThreadPool *pool;
   THREAD handle;
};

/**
 * Thread work request
 */
struct WorkRequest
{
   ThreadPoolWorkerFunction func;
   void *arg;
   INT64 queueTime;
   INT64 runTime;
};

/**
 * Request queue for serialized execution
 */
class SerializationQueue: public Queue
{
private:
   UINT32 m_maxWaitTime;

public:
   SerializationQueue() : Queue() { m_maxWaitTime = 0; }
   SerializationQueue(UINT32 initialSize, UINT32 bufferIncrement) : Queue(initialSize, bufferIncrement) { m_maxWaitTime = 0; }

   UINT32 getMaxWaitTime() { return m_maxWaitTime; }
   void updateMaxWaitTime(UINT32 waitTime) { m_maxWaitTime = std::max(waitTime, m_maxWaitTime); }
};

/**
 * Thread pool
 */
struct ThreadPool
{
   int minThreads;
   int maxThreads;
   int stackSize;
   UINT32 workerIdleTimeout;
   VolatileCounter activeRequests;
   MUTEX mutex;
   THREAD maintThread;
   CONDITION maintThreadWakeup;
   HashMap<UINT64, WorkerThreadInfo> *threads;
   Queue *queue;
   StringObjectMap<SerializationQueue> *serializationQueues;
   MUTEX serializationLock;
   ObjectArray<WorkRequest> *schedulerQueue;
   MUTEX schedulerLock;
   TCHAR *name;
   bool shutdownMode;
   INT64 loadAverage[3];
   INT64 averageWaitTime;
   UINT64 threadStartCount;
   UINT64 threadStopCount;
   VolatileCounter64 taskExecutionCount;
};

/**
 * Thread pool registry
 */
static StringObjectMap<ThreadPool> s_registry(false);
static Mutex s_registryLock;

/**
 * Worker function to join stopped thread
 */
static void JoinWorkerThread(void *arg)
{
   ThreadJoin(static_cast<WorkerThreadInfo*>(arg)->handle);
   delete static_cast<WorkerThreadInfo*>(arg);
}

/**
 * Worker thread function
 */
static THREAD_RESULT THREAD_CALL WorkerThread(void *arg)
{
   ThreadPool *p = static_cast<WorkerThreadInfo*>(arg)->pool;
   Queue *q = p->queue;

   char threadName[16];
   threadName[0] = '$';
#ifdef UNICODE
   WideCharToMultiByte(CP_ACP, WC_DEFAULTCHAR | WC_COMPOSITECHECK, p->name, -1, &threadName[1], 11, NULL, NULL);
#else
   strlcpy(&threadName[1], p->name, 11);
#endif
   strlcat(threadName, "/WRK", 16);
   ThreadSetName(threadName);

   while(true)
   {
      WorkRequest *rq = static_cast<WorkRequest*>(q->getOrBlock(p->workerIdleTimeout));

      if (rq == NULL)
      {
         MutexLock(p->mutex);
         if ((p->threads->size() <= p->minThreads) || (p->averageWaitTime / FP_1 > s_waitTimeLowWatermark))
         {
            MutexUnlock(p->mutex);
            continue;
         }
         p->threads->remove(CAST_FROM_POINTER(arg, UINT64));
         p->threadStopCount++;
         MutexUnlock(p->mutex);

         nxlog_debug_tag(DEBUG_TAG, 5, _T("Stopping worker thread in thread pool %s due to inactivity"), p->name);

         free(rq);
         rq = MemAllocStruct<WorkRequest>();
         rq->func = JoinWorkerThread;
         rq->arg = arg;
         rq->queueTime = GetCurrentTimeMs();
         InterlockedIncrement(&p->activeRequests);
         p->queue->put(rq);
         break;
      }
      
      if (rq->func == NULL) // stop indicator
         break;
      
      INT64 waitTime = (GetCurrentTimeMs() - rq->queueTime) << FP_SHIFT;
      MutexLock(p->mutex);
      CALC_EMA(p->averageWaitTime, EXP_15, waitTime);
      MutexUnlock(p->mutex);

      rq->func(rq->arg);
      MemFree(rq);
      InterlockedDecrement(&p->activeRequests);
   }
   return THREAD_OK;
}

/**
 * Thread pool maintenance thread
 */
static THREAD_RESULT THREAD_CALL MaintenanceThread(void *arg)
{
   ThreadPool *p = static_cast<ThreadPool*>(arg);

   char threadName[16];
   threadName[0] = '$';
#ifdef UNICODE
   WideCharToMultiByte(CP_ACP, WC_DEFAULTCHAR | WC_COMPOSITECHECK, p->name, -1, &threadName[1], 11, NULL, NULL);
#else
   strlcpy(&threadName[1], p->name, 11);
#endif
   strlcat(threadName, "/MNT", 16);
   ThreadSetName(threadName);

   int count = 0;
   UINT32 sleepTime = 5000;
   UINT32 cycleTime = 0;
   while(!p->shutdownMode)
   {
      INT64 startTime = GetCurrentTimeMs();
      ConditionWait(p->maintThreadWakeup, sleepTime);
      cycleTime += static_cast<UINT32>(GetCurrentTimeMs() - startTime);

      // Update load data every 5 seconds
      if (cycleTime >= 5000)
      {
         cycleTime = 0;

         INT64 requestCount = static_cast<INT64>(p->activeRequests) << FP_SHIFT;
         CALC_EMA(p->loadAverage[0], EXP_1, requestCount);
         CALC_EMA(p->loadAverage[1], EXP_5, requestCount);
         CALC_EMA(p->loadAverage[2], EXP_15, requestCount);

         count++;
         if (count == s_maintThreadResponsiveness)
         {
            TCHAR debugMessage[1024] = _T("");
            int started = 0;
            bool failure = false;

            MutexLock(p->mutex);
            int threadCount = p->threads->size();
            INT64 averageWaitTime = p->averageWaitTime / FP_1;
            if ((averageWaitTime > s_waitTimeHighWatermark) && (threadCount < p->maxThreads))
            {
               int delta = std::min(p->maxThreads - threadCount, std::max((static_cast<int>(p->activeRequests) - threadCount) / 2, 1));
               for(int i = 0; i < delta; i++)
               {
                  WorkerThreadInfo *wt = new WorkerThreadInfo;
                  wt->pool = p;
                  wt->handle = ThreadCreateEx(WorkerThread, p->stackSize, wt);
                  if (wt->handle != INVALID_THREAD_HANDLE)
                  {
                     p->threads->set(CAST_FROM_POINTER(wt, UINT64), wt);
                     p->threadStartCount++;
                     started++;
                  }
                  else
                  {
                     delete wt;
                     failure = true;
                     break;
                  }
               }
               if (p->workerIdleTimeout < MAX_WORKER_IDLE_TIMEOUT)
               {
                  p->workerIdleTimeout *= 2;
                  _sntprintf(debugMessage, 1024, _T("Worker idle timeout increased to %d milliseconds for thread pool %s"), p->workerIdleTimeout, p->name);
               }
            }
            else if ((averageWaitTime < s_waitTimeLowWatermark) && (threadCount > p->minThreads) && (p->workerIdleTimeout > MIN_WORKER_IDLE_TIMEOUT))
            {
               p->workerIdleTimeout /= 2;
               _sntprintf(debugMessage, 1024, _T("Worker idle timeout decreased to %d milliseconds for thread pool %s"), p->workerIdleTimeout, p->name);
            }
            MutexUnlock(p->mutex);
            count = 0;

            if (started > 1)
               nxlog_debug_tag(DEBUG_TAG, 3, _T("%d new threads started in thread pool %s"), started, p->name);
            else if (started > 0)
               nxlog_debug_tag(DEBUG_TAG, 3, _T("New thread started in thread pool %s"), p->name);
            if (failure)
               nxlog_debug_tag(DEBUG_TAG, 1, _T("Cannot create worker thread in pool %s"), p->name);
            if (debugMessage[0] != 0)
               nxlog_debug_tag(DEBUG_TAG, 4, _T("%s"), debugMessage);
         }
      }
      sleepTime = 5000 - cycleTime;

      // Check scheduler queue
      MutexLock(p->schedulerLock);
      if (p->schedulerQueue->size() > 0)
      {
         INT64 now = GetCurrentTimeMs();
         WorkRequest *rq;
         while(p->schedulerQueue->size() > 0)
         {
            rq = p->schedulerQueue->get(0);
            if (rq->runTime > now)
            {
               UINT32 delay = static_cast<UINT32>(rq->runTime - now);
               if (delay < sleepTime)
                  sleepTime = delay;
               break;
            }
            p->schedulerQueue->remove(0);
            InterlockedIncrement(&p->activeRequests);
            rq->queueTime = now;
            p->queue->put(rq);
         }
      }
      MutexUnlock(p->schedulerLock);
   }
   nxlog_debug_tag(DEBUG_TAG, 3, _T("Maintenance thread for thread pool %s stopped"), p->name);
   return THREAD_OK;
}

/**
 * Create thread pool
 */
ThreadPool LIBNETXMS_EXPORTABLE *ThreadPoolCreate(const TCHAR *name, int minThreads, int maxThreads, int stackSize)
{
   ThreadPool *p = MemAllocStruct<ThreadPool>();
   p->minThreads = minThreads;
   p->maxThreads = maxThreads;
   p->stackSize = stackSize;
   p->workerIdleTimeout = MIN_WORKER_IDLE_TIMEOUT;
   p->activeRequests = 0;
   p->threads = new HashMap<UINT64, WorkerThreadInfo>();
   p->queue = new Queue(64, 64);
   p->mutex = MutexCreate();
   p->maintThreadWakeup = ConditionCreate(false);
   p->serializationQueues = new StringObjectMap<SerializationQueue>(true);
   p->serializationQueues->setIgnoreCase(false);
   p->serializationLock = MutexCreate();
   p->schedulerQueue = new ObjectArray<WorkRequest>(16, 16, false);
   p->schedulerLock = MutexCreate();
   p->name = (name != NULL) ? _tcsdup(name) : _tcsdup(_T("NONAME"));
   p->shutdownMode = false;

   p->maintThread = ThreadCreateEx(MaintenanceThread, 256 * 1024, p);

   MutexLock(p->mutex);
   for(int i = 0; i < p->minThreads; i++)
   {
      WorkerThreadInfo *wt = new WorkerThreadInfo;
      wt->pool = p;
      wt->handle = ThreadCreateEx(WorkerThread, stackSize, wt);
      if (wt->handle != INVALID_THREAD_HANDLE)
      {
         p->threads->set(CAST_FROM_POINTER(wt, UINT64), wt);
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 1, _T("Cannot create worker thread in pool %s"), p->name);
         delete wt;
      }
   }
   MutexUnlock(p->mutex);

   s_registryLock.lock();
   s_registry.set(p->name, p);
   s_registryLock.unlock();

   nxlog_debug_tag(DEBUG_TAG, 1, _T("Thread pool %s initialized (min=%d, max=%d)"), p->name, p->minThreads, p->maxThreads);
   return p;
}

/**
 * Callback for joining all worker threads on pool destruction
 */
static EnumerationCallbackResult ThreadPoolDestroyCallback(const void *key, const void *object, void *arg)
{
   ThreadJoin(static_cast<const WorkerThreadInfo*>(object)->handle);
   return _CONTINUE;
}

/**
 * Destroy thread pool
 */
void LIBNETXMS_EXPORTABLE ThreadPoolDestroy(ThreadPool *p)
{
   nxlog_debug_tag(DEBUG_TAG, 3, _T("Stopping threads in thread pool %s"), p->name);

   s_registryLock.lock();
   s_registry.remove(p->name);
   s_registryLock.unlock();

   MutexLock(p->mutex);
   p->shutdownMode = true;
   MutexUnlock(p->mutex);

   ConditionSet(p->maintThreadWakeup);
   ThreadJoin(p->maintThread);
   ConditionDestroy(p->maintThreadWakeup);

   WorkRequest rq;
   rq.func = NULL;
   rq.queueTime = GetCurrentTimeMs();
   for(int i = 0; i < p->threads->size(); i++)
      p->queue->put(&rq);
   p->threads->forEach(ThreadPoolDestroyCallback, NULL);

   nxlog_debug_tag(DEBUG_TAG, 1, _T("Thread pool %s destroyed"), p->name);
   p->threads->setOwner(true);
   delete p->threads;
   delete p->queue;
   delete p->serializationQueues;
   MutexDestroy(p->serializationLock);
   delete p->schedulerQueue;
   MutexDestroy(p->schedulerLock);
   MutexDestroy(p->mutex);
   MemFree(p->name);
   MemFree(p);
}

/**
 * Execute task as soon as possible
 */
void LIBNETXMS_EXPORTABLE ThreadPoolExecute(ThreadPool *p, ThreadPoolWorkerFunction f, void *arg)
{
   InterlockedIncrement(&p->activeRequests);
   InterlockedIncrement64(&p->taskExecutionCount);
   WorkRequest *rq = MemAllocStruct<WorkRequest>();
   rq->func = f;
   rq->arg = arg;
   rq->queueTime = GetCurrentTimeMs();
   p->queue->put(rq);
}

/**
 * Request serialization data
 */
struct RequestSerializationData
{
   TCHAR *key;
   ThreadPool *pool;
   Queue *queue;
};

/**
 * Worker function to process serialized requests
 */
static void ProcessSerializedRequests(void *arg)
{
   RequestSerializationData *data = (RequestSerializationData *)arg;
   while(true)
   {
      MutexLock(data->pool->serializationLock);
      WorkRequest *rq = (WorkRequest *)data->queue->get();
      if (rq == NULL)
      {
         data->pool->serializationQueues->remove(data->key);
         MutexUnlock(data->pool->serializationLock);
         break;
      }
      SerializationQueue *q = data->pool->serializationQueues->get(data->key);
      q->updateMaxWaitTime(GetCurrentTimeMs() - rq->queueTime);
      MutexUnlock(data->pool->serializationLock);

      rq->func(rq->arg);
      free(rq);
   }
   free(data->key);
   delete data;
}

/**
 * Execute task serialized (not before previous task with same key ends)
 */
void LIBNETXMS_EXPORTABLE ThreadPoolExecuteSerialized(ThreadPool *p, const TCHAR *key, ThreadPoolWorkerFunction f, void *arg)
{
   MutexLock(p->serializationLock);
   
   SerializationQueue *q = p->serializationQueues->get(key);
   if (q == NULL)
   {
      q = new SerializationQueue(8, 8);
      p->serializationQueues->set(key, q);

      RequestSerializationData *data = new RequestSerializationData;
      data->key = _tcsdup(key);
      data->pool = p;
      data->queue = q;
      ThreadPoolExecute(p, ProcessSerializedRequests, data);
   }

   WorkRequest *rq = MemAllocStruct<WorkRequest>();
   rq->func = f;
   rq->arg = arg;
   rq->queueTime = GetCurrentTimeMs();
   q->put(rq);

   MutexUnlock(p->serializationLock);
}

/**
 * Scheduled requests comparator (used for task sorting)
 */
static int ScheduledRequestsComparator(const WorkRequest **e1, const WorkRequest **e2)
{
   time_t t1 = (*e1)->runTime;
   time_t t2 = (*e2)->runTime;
   return (t1 < t2) ? -1 : ((t1 > t2) ? 1 : 0);
}

/**
 * Schedule task for execution using absolute time (in milliseconds)
 */
void LIBNETXMS_EXPORTABLE ThreadPoolScheduleAbsoluteMs(ThreadPool *p, INT64 runTime, ThreadPoolWorkerFunction f, void *arg)
{
   WorkRequest *rq = (WorkRequest *)malloc(sizeof(WorkRequest));
   rq->func = f;
   rq->arg = arg;
   rq->runTime = runTime;
   rq->queueTime = GetCurrentTimeMs();

   MutexLock(p->schedulerLock);
   p->schedulerQueue->add(rq);
   p->schedulerQueue->sort(ScheduledRequestsComparator);
   MutexUnlock(p->schedulerLock);
   ConditionSet(p->maintThreadWakeup);
}

/**
 * Schedule task for execution using absolute time
 */
void LIBNETXMS_EXPORTABLE ThreadPoolScheduleAbsolute(ThreadPool *p, time_t runTime, ThreadPoolWorkerFunction f, void *arg)
{
   ThreadPoolScheduleAbsoluteMs(p, static_cast<INT64>(runTime) * 1000, f, arg);
}

/**
 * Schedule task for execution using relative time (delay in milliseconds)
 */
void LIBNETXMS_EXPORTABLE ThreadPoolScheduleRelative(ThreadPool *p, UINT32 delay, ThreadPoolWorkerFunction f, void *arg)
{
   if (delay > 0)
      ThreadPoolScheduleAbsoluteMs(p, GetCurrentTimeMs() + delay, f, arg);
   else
      ThreadPoolExecute(p, f, arg);
}

/**
 * Get pool information
 */
void LIBNETXMS_EXPORTABLE ThreadPoolGetInfo(ThreadPool *p, ThreadPoolInfo *info)
{
   MutexLock(p->mutex);
   info->name = p->name;
   info->minThreads = p->minThreads;
   info->maxThreads = p->maxThreads;
   info->curThreads = p->threads->size();
   info->threadStarts = p->threadStartCount;
   info->threadStops = p->threadStopCount;
   info->activeRequests = p->activeRequests;
   info->totalRequests = p->taskExecutionCount;
   info->load = (info->curThreads > 0) ? info->activeRequests * 100 / info->curThreads : 0;
   info->usage = info->curThreads * 100 / info->maxThreads;
   info->loadAvg[0] = static_cast<double>(p->loadAverage[0]) / FP_1;
   info->loadAvg[1] = static_cast<double>(p->loadAverage[1]) / FP_1;
   info->loadAvg[2] = static_cast<double>(p->loadAverage[2]) / FP_1;
   info->averageWaitTime = static_cast<UINT32>(p->averageWaitTime / FP_1);
   MutexUnlock(p->mutex);
   MutexLock(p->schedulerLock);
   info->scheduledRequests = p->schedulerQueue->size();
   MutexUnlock(p->schedulerLock);
}

/**
 * Get pool information by name
 */
bool LIBNETXMS_EXPORTABLE ThreadPoolGetInfo(const TCHAR *name, ThreadPoolInfo *info)
{
   s_registryLock.lock();
   ThreadPool *p = s_registry.get(name);
   if (p != NULL)
      ThreadPoolGetInfo(p, info);
   s_registryLock.unlock();
   return p != NULL;
}

/**
 * Get all thread pool names
 */
StringList LIBNETXMS_EXPORTABLE *ThreadPoolGetAllPools()
{
   s_registryLock.lock();
   StringList *list = s_registry.keys();
   s_registryLock.unlock();
   return list;
}

/**
 * Get number of queued jobs on the pool by key
 */
int LIBNETXMS_EXPORTABLE ThreadPoolGetSerializedRequestCount(ThreadPool *p, const TCHAR *key)
{
   MutexLock(p->serializationLock);
   SerializationQueue *q = p->serializationQueues->get(key);
   int count = (q != NULL) ? q->size() : 0;
   MutexUnlock(p->serializationLock);
   return count;
}

/**
 * Get number of queued jobs on the pool by key
 */
UINT32 LIBNETXMS_EXPORTABLE ThreadPoolGetSerializedRequestMaxWaitTime(ThreadPool *p, const TCHAR *key)
{
   MutexLock(p->serializationLock);
   SerializationQueue *q = p->serializationQueues->get(key);
   UINT32 waitTime = (q != NULL) ? q->getMaxWaitTime() : 0;
   MutexUnlock(p->serializationLock);
   return waitTime;
}

/**
 * Set thread pool resize parameters - responsiveness and wait time high/low watermarks
 */
void LIBNETXMS_EXPORTABLE ThreadPoolSetResizeParameters(int responsiveness, UINT32 waitTimeHWM, UINT32 waitTimeLWM)
{
   if ((responsiveness > 0) && (responsiveness <= 24))
      s_maintThreadResponsiveness = responsiveness;
   if (waitTimeHWM > 0)
      s_waitTimeHighWatermark = waitTimeHWM;
   if (waitTimeLWM > 0)
      s_waitTimeLowWatermark = waitTimeLWM;
}
