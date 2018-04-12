/* 
** NetXMS - Network Management System
** NetXMS Foundation Library
** Copyright (C) 2003-2016 Victor Kirhenshtein
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

/**
 * Load average calculation
 */
#define FP_SHIFT  11              /* nr of bits of precision */
#define FP_1      (1 << FP_SHIFT) /* 1.0 as fixed-point */
#define EXP_1     1884            /* 1/exp(5sec/1min) as fixed-point */
#define EXP_5     2014            /* 1/exp(5sec/5min) */
#define EXP_15    2037            /* 1/exp(5sec/15min) */ 
#define CALC_LOAD(load, exp, n) do { load *= exp; load += n * (FP_1 - exp); load >>= FP_SHIFT; } while(0)

/**
 * Worker thread idle timeout in milliseconds
 */
#define THREAD_IDLE_TIMEOUT   300000

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
   bool inactivityStop;
   INT64 runTime;
};

/**
 * Thread pool
 */
struct ThreadPool
{
   int minThreads;
   int maxThreads;
   int stackSize;
   VolatileCounter activeRequests;
   MUTEX mutex;
   THREAD maintThread;
   CONDITION maintThreadWakeup;
   HashMap<UINT64, WorkerThreadInfo> *threads;
   Queue *queue;
   StringObjectMap<Queue> *serializationQueues;
   MUTEX serializationLock;
   ObjectArray<WorkRequest> *schedulerQueue;
   MUTEX schedulerLock;
   TCHAR *name;
   bool shutdownMode;
   INT32 loadAverage[3];
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
   ThreadJoin(((WorkerThreadInfo *)arg)->handle);
   delete (WorkerThreadInfo *)arg;
}

/**
 * Worker thread function
 */
static THREAD_RESULT THREAD_CALL WorkerThread(void *arg)
{
   ThreadPool *p = ((WorkerThreadInfo *)arg)->pool;
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
      WorkRequest *rq = (WorkRequest *)q->getOrBlock();
      
      if (rq->func == NULL) // stop indicator
      {
         if (rq->inactivityStop)
         {
            MutexLock(p->mutex);
            p->threads->remove(CAST_FROM_POINTER(arg, UINT64));
            MutexUnlock(p->mutex);

            nxlog_debug(3, _T("Stopping worker thread in thread pool %s due to inactivity"), p->name);

            free(rq);
            rq = (WorkRequest *)malloc(sizeof(WorkRequest));
            rq->func = JoinWorkerThread;
            rq->arg = arg;
            InterlockedIncrement(&p->activeRequests);
            p->queue->put(rq);
         }
         break;
      }
      
      rq->func(rq->arg);
      free(rq);
      InterlockedDecrement(&p->activeRequests);
   }
   return THREAD_OK;
}

/**
 * Thread pool maintenance thread
 */
static THREAD_RESULT THREAD_CALL MaintenanceThread(void *arg)
{
   ThreadPool *p = (ThreadPool *)arg;

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

         INT32 requestCount = (INT32)p->activeRequests << FP_SHIFT;
         CALC_LOAD(p->loadAverage[0], EXP_1, requestCount);
         CALC_LOAD(p->loadAverage[1], EXP_5, requestCount);
         CALC_LOAD(p->loadAverage[2], EXP_15, requestCount);

         count++;
         if (count == 12)  // do pool check once per minute
         {
            MutexLock(p->mutex);
            INT32 threadCount = p->threads->size();
            MutexUnlock(p->mutex);
            if ((threadCount > p->minThreads) && (p->loadAverage[1] < 1024 * threadCount)) // 5 minutes load average < 0.5 * thread count
            {
               WorkRequest *rq = (WorkRequest *)malloc(sizeof(WorkRequest));
               rq->func = NULL;
               rq->arg = NULL;
               rq->inactivityStop = true;
               p->queue->put(rq);
            }
            count = 0;
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
            p->queue->put(rq);
         }
      }
      MutexUnlock(p->schedulerLock);
   }
   nxlog_debug(3, _T("Maintenance thread for thread pool %s stopped"), p->name);
   return THREAD_OK;
}

/**
 * Create thread pool
 */
ThreadPool LIBNETXMS_EXPORTABLE *ThreadPoolCreate(const TCHAR *name, int minThreads, int maxThreads, int stackSize)
{
   ThreadPool *p = (ThreadPool *)malloc(sizeof(ThreadPool));
   p->minThreads = minThreads;
   p->maxThreads = maxThreads;
   p->stackSize = stackSize;
   p->activeRequests = 0;
   p->threads = new HashMap<UINT64, WorkerThreadInfo>();
   p->queue = new Queue(64, 64);
   p->mutex = MutexCreate();
   p->maintThreadWakeup = ConditionCreate(false);
   p->serializationQueues = new StringObjectMap<Queue>(true);
   p->serializationQueues->setIgnoreCase(false);
   p->serializationLock = MutexCreate();
   p->schedulerQueue = new ObjectArray<WorkRequest>(16, 16, false);
   p->schedulerLock = MutexCreate();
   p->name = (name != NULL) ? _tcsdup(name) : _tcsdup(_T("NONAME"));
   p->shutdownMode = false;
   p->loadAverage[0] = 0;
   p->loadAverage[1] = 0;
   p->loadAverage[2] = 0;

   for(int i = 0; i < p->minThreads; i++)
   {
      WorkerThreadInfo *wt = new WorkerThreadInfo;
      wt->pool = p;
      wt->handle = ThreadCreateEx(WorkerThread, stackSize, wt);
      p->threads->set(CAST_FROM_POINTER(wt, UINT64), wt);
   }

   p->maintThread = ThreadCreateEx(MaintenanceThread, 256 * 1024, p);

   s_registryLock.lock();
   s_registry.set(p->name, p);
   s_registryLock.unlock();

   nxlog_debug(1, _T("Thread pool %s initialized (min=%d, max=%d)"), p->name, p->minThreads, p->maxThreads);
   return p;
}

/**
 * Callback for joining all worker threads on pool destruction
 */
static EnumerationCallbackResult ThreadPoolDestroyCallback(const void *key, const void *object, void *arg)
{
   ThreadJoin(((WorkerThreadInfo *)object)->handle);
   return _CONTINUE;
}

/**
 * Destroy thread pool
 */
void LIBNETXMS_EXPORTABLE ThreadPoolDestroy(ThreadPool *p)
{
   nxlog_debug(3, _T("Stopping threads in thread pool %s"), p->name);

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
   rq.inactivityStop = false;
   for(int i = 0; i < p->threads->size(); i++)
      p->queue->put(&rq);
   p->threads->forEach(ThreadPoolDestroyCallback, NULL);

   nxlog_debug(1, _T("Thread pool %s destroyed"), p->name);
   p->threads->setOwner(true);
   delete p->threads;
   delete p->queue;
   delete p->serializationQueues;
   MutexDestroy(p->serializationLock);
   delete p->schedulerQueue;
   MutexDestroy(p->schedulerLock);
   MutexDestroy(p->mutex);
   free(p->name);
   free(p);
}

/**
 * Execute task as soon as possible
 */
void LIBNETXMS_EXPORTABLE ThreadPoolExecute(ThreadPool *p, ThreadPoolWorkerFunction f, void *arg)
{
   if (InterlockedIncrement(&p->activeRequests) > p->threads->size())
   {
      bool started = false;
      MutexLock(p->mutex);
      if (p->threads->size() < p->maxThreads)
      {
         WorkerThreadInfo *wt = new WorkerThreadInfo;
         wt->pool = p;
         wt->handle = ThreadCreateEx(WorkerThread, p->stackSize, wt);
         p->threads->set(CAST_FROM_POINTER(wt, UINT64), wt);
         started = true;
      }
      MutexUnlock(p->mutex);
      if (started)
         nxlog_debug(3, _T("New thread started in thread pool %s"), p->name);
   }

   WorkRequest *rq = (WorkRequest *)malloc(sizeof(WorkRequest));
   rq->func = f;
   rq->arg = arg;
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
   
   Queue *q = p->serializationQueues->get(key);
   if (q == NULL)
   {
      q = new Queue(8, 8);
      p->serializationQueues->set(key, q);

      RequestSerializationData *data = new RequestSerializationData;
      data->key = _tcsdup(key);
      data->pool = p;
      data->queue = q;
      ThreadPoolExecute(p, ProcessSerializedRequests, data);
   }

   WorkRequest *rq = (WorkRequest *)malloc(sizeof(WorkRequest));
   rq->func = f;
   rq->arg = arg;
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
   info->activeRequests = p->activeRequests;
   info->load = (info->curThreads > 0) ? info->activeRequests * 100 / info->curThreads : 0;
   info->usage = info->curThreads * 100 / info->maxThreads;
   info->loadAvg[0] = (double)p->loadAverage[0] / FP_1;
   info->loadAvg[1] = (double)p->loadAverage[1] / FP_1;
   info->loadAvg[2] = (double)p->loadAverage[2] / FP_1;
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
