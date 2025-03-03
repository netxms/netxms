/* 
** NetXMS - Network Management System
** NetXMS Foundation Library
** Copyright (C) 2003-2025 Victor Kirhenshtein
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
#include <welford.h>
#include <queue>

#define DEBUG_TAG _T("threads.pool")

/**
 * Wait time watermarks (milliseconds)
 */
static uint32_t s_waitTimeHighWatermark = 100;
static uint32_t s_waitTimeLowWatermark = 50;

/**
 * Thread pool maintenance thread responsiveness
 */
static int s_maintThreadResponsiveness = 12;

/**
 * Indicator for stop with deregistration
 */
static char s_stopAndUnregister[] = "UNREGISTER";

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
   int64_t queueTime;
   int64_t runTime;
};

/**
 * Request queue for serialized execution
 */
class SerializationQueue: public SQueue<WorkRequest>
{
private:
   uint32_t m_maxWaitTime;

public:
   SerializationQueue() : SQueue<WorkRequest>(128) { m_maxWaitTime = 0; }

   uint32_t getMaxWaitTime() { return m_maxWaitTime; }
   void updateMaxWaitTime(uint32_t waitTime) { m_maxWaitTime = std::max(waitTime, m_maxWaitTime); }
};

/**
 * Scheduled requests comparator (used for task sorting)
 */
struct ScheduledRequestsComparator
{
#if __cplusplus >= 201402L
   constexpr bool operator() (const WorkRequest& lhs, const WorkRequest& rhs) const
#else
   bool operator() (const WorkRequest& lhs, const WorkRequest& rhs) const
#endif
   {
      return lhs.runTime > rhs.runTime;
   }
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
   Mutex mutex;
   THREAD maintThread;
   Condition maintThreadWakeup;
   HashMap<uint64_t, WorkerThreadInfo> threads;
   SQueue<WorkRequest> queue;
   StringObjectMap<SerializationQueue> serializationQueues;
   Mutex serializationLock;
   std::priority_queue<WorkRequest, std::vector<WorkRequest>, ScheduledRequestsComparator> schedulerQueue;
   Mutex schedulerLock;
   TCHAR *name;
   bool shutdownMode;
   int64_t loadAverage[3];
   int64_t waitTimeEMA;
   WelfordVariance waitTimeVariance;
   int64_t queueSizeEMA;
   WelfordVariance queueSizeVariance;
   uint64_t threadStartCount;
   uint64_t threadStopCount;
   VolatileCounter64 taskExecutionCount;

   ThreadPool(const TCHAR *name, int minThreads, int maxThreads, int stackSize) :
         mutex(MutexType::FAST), maintThreadWakeup(false), queue(512), serializationQueues(Ownership::True),
         serializationLock(MutexType::FAST), schedulerLock(MutexType::FAST)
   {
      this->name = (name != nullptr) ? MemCopyString(name) : MemCopyString(_T("NONAME"));
      this->minThreads = std::max(minThreads, 1);
      this->maxThreads = std::max(maxThreads, this->minThreads);
      this->stackSize = stackSize;
      activeRequests = 0;
      maintThread = INVALID_THREAD_HANDLE;
      serializationQueues.setIgnoreCase(false);
      shutdownMode = false;
      memset(loadAverage, 0, sizeof(loadAverage));
      waitTimeEMA = 0;
      queueSizeEMA = 0;
      threadStartCount = 0;
      threadStopCount = 0;
      taskExecutionCount = 0;
   }

   ~ThreadPool()
   {
      threads.setOwner(Ownership::True);
      MemFree(name);
   }
};

/**
 * Thread pool registry
 */
static StringObjectMap<ThreadPool> s_registry(Ownership::False);
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
static void WorkerThread(WorkerThreadInfo *threadInfo)
{
   ThreadPool *p = threadInfo->pool;

   char threadName[16];
   threadName[0] = '$';
#ifdef UNICODE
   wchar_to_ASCII(p->name, -1, &threadName[1], 11);
   threadName[11] = 0;
#else
   strlcpy(&threadName[1], p->name, 11);
#endif
   strlcat(threadName, "/WRK", 16);
   ThreadSetName(threadName);

   while(true)
   {
      WorkRequest rq;
      p->queue.getOrBlock(&rq, INFINITE);
      if (rq.func == nullptr) // stop indicator
      {
         if (rq.arg == s_stopAndUnregister)
         {
            p->mutex.lock();
            p->threads.remove(CAST_FROM_POINTER(threadInfo, uint64_t));
            p->threadStopCount++;
            p->mutex.unlock();

            rq.func = JoinWorkerThread;
            rq.arg = threadInfo;
            rq.queueTime = GetCurrentTimeMs();
            InterlockedIncrement(&p->activeRequests);
            p->queue.put(rq);
         }
         break;
      }

      int64_t waitTime = GetCurrentTimeMs() - rq.queueTime;
      p->mutex.lock();
      UpdateExpMovingAverage(p->waitTimeEMA, EMA_EXP(1, 1000), waitTime); // Use last 1000 executions
      p->waitTimeVariance.update(waitTime);
      p->mutex.unlock();

      rq.func(rq.arg);
      InterlockedDecrement(&p->activeRequests);
   }

   nxlog_debug_tag(DEBUG_TAG, 8, _T("Worker thread in thread pool %s stopped"), p->name);
}

/**
 * Thread pool maintenance thread
 */
static void MaintenanceThread(ThreadPool *p)
{
   char threadName[16];
   threadName[0] = '$';
#ifdef UNICODE
   wchar_to_ASCII(p->name, -1, &threadName[1], 11);
   threadName[11] = 0;
#else
   strlcpy(&threadName[1], p->name, 11);
#endif
   strlcat(threadName, "/MNT", 16);
   ThreadSetName(threadName);

   int count = 0;
   uint32_t sleepTime = 5000;
   uint32_t cycleTime = 0;
   while(!p->shutdownMode)
   {
      int64_t startTime = GetCurrentTimeMs();
      p->maintThreadWakeup.wait(sleepTime);
      cycleTime += static_cast<uint32_t>(GetCurrentTimeMs() - startTime);

      // Update load data every 5 seconds
      if (cycleTime >= 5000)
      {
         cycleTime = 0;

         int64_t requestCount = static_cast<int64_t>(p->activeRequests);
         UpdateExpMovingAverage(p->loadAverage[0], EMA_EXP_12, requestCount);
         UpdateExpMovingAverage(p->loadAverage[1], EMA_EXP_60, requestCount);
         UpdateExpMovingAverage(p->loadAverage[2], EMA_EXP_180, requestCount);

         int64_t queueSize = static_cast<int64_t>(p->queue.size());
         UpdateExpMovingAverage(p->queueSizeEMA, EMA_EXP_180, queueSize);
         p->queueSizeVariance.update(queueSize);

         count++;
         if (count == s_maintThreadResponsiveness)
         {
            int started = 0;
            int stopped = 0;
            bool failure = false;

            p->mutex.lock();
            int threadCount = p->threads.size();
            uint32_t waitTimeEMA = static_cast<uint32_t>(p->waitTimeEMA / EMA_FP_1);
            uint32_t waitTimeSMA = static_cast<uint32_t>(p->waitTimeVariance.mean());
            int queueSizeEMA = static_cast<int>(p->queueSizeEMA / EMA_FP_1);
            int queueSizeSMA = static_cast<int>(p->queueSizeVariance.mean());
            if (((waitTimeEMA > s_waitTimeHighWatermark) && (waitTimeSMA > s_waitTimeHighWatermark) && (threadCount < p->maxThreads)) ||
                ((threadCount == 0) && (p->activeRequests > 0)))
            {
               int delta = std::min(p->maxThreads - threadCount, std::max(std::min(queueSizeSMA, queueSizeEMA) / 2, 1));
               for(int i = 0; i < delta; i++)
               {
                  WorkerThreadInfo *wt = new WorkerThreadInfo;
                  wt->pool = p;
                  wt->handle = ThreadCreateEx(WorkerThread, wt, p->stackSize);
                  if (wt->handle != INVALID_THREAD_HANDLE)
                  {
                     p->threads.set(CAST_FROM_POINTER(wt, uint64_t), wt);
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
            }
            else if ((waitTimeEMA < s_waitTimeLowWatermark) && (waitTimeSMA < s_waitTimeLowWatermark) && (threadCount > p->minThreads))
            {
               int loadAverage15 = static_cast<int>(GetExpMovingAverageValue(p->loadAverage[2]));
               if (loadAverage15 < threadCount / 2)
               {
                  stopped = threadCount - 2 * loadAverage15;
                  if (stopped > threadCount - p->minThreads)
                     stopped = threadCount - p->minThreads;
               }
               for(int i = 0; i < stopped; i++)
               {
                  WorkRequest rq;
                  rq.func = nullptr;
                  rq.arg = s_stopAndUnregister;
                  rq.queueTime = GetCurrentTimeMs();
                  p->queue.put(rq);
               }
            }
            p->waitTimeVariance.reset();
            p->queueSizeVariance.reset();
            p->mutex.unlock();
            count = 0;

            if (started > 1)
               nxlog_debug_tag(DEBUG_TAG, 3, _T("%d new threads started in thread pool %s (wait time EMA/SMA = %u/%u, queue size EMA/SMA = %d/%d)"),
                  started, p->name, waitTimeEMA, waitTimeSMA, queueSizeEMA, queueSizeSMA);
            else if (started > 0)
               nxlog_debug_tag(DEBUG_TAG, 3, _T("New thread started in thread pool %s (wait time EMA/SMA = %u/%u, queue size EMA/SMA = %d/%d)"),
                  p->name, waitTimeEMA, waitTimeSMA, queueSizeEMA, queueSizeSMA);

            if (failure)
               nxlog_debug_tag(DEBUG_TAG, 1, _T("Cannot create worker thread in pool %s"), p->name);

            if (stopped > 1)
               nxlog_debug_tag(DEBUG_TAG, 3, _T("Requested stop for %d threads in thread pool %s (wait time EMA/SMA = %u/%u, queue size SMA = %d)"),
                  stopped, p->name, waitTimeEMA, waitTimeSMA, queueSizeSMA);
            else if (stopped > 0)
               nxlog_debug_tag(DEBUG_TAG, 3, _T("Requested thread stop in thread pool %s (wait time EMA/SMA = %u/%u, queue size SMA = %d)"),
                  p->name, waitTimeEMA, waitTimeSMA, queueSizeSMA);
         }
      }
      sleepTime = 5000 - cycleTime;

      // Check scheduler queue
      p->schedulerLock.lock();
      if (p->schedulerQueue.size() > 0)
      {
         int64_t now = GetCurrentTimeMs();
         while(p->schedulerQueue.size() > 0)
         {
            WorkRequest rq = p->schedulerQueue.top();
            if (rq.runTime > now)
            {
               uint32_t delay = static_cast<uint32_t>(rq.runTime - now);
               if (delay < sleepTime)
                  sleepTime = delay;
               break;
            }
            InterlockedIncrement(&p->activeRequests);
            InterlockedIncrement64(&p->taskExecutionCount);
            rq.queueTime = now;
            p->queue.put(rq);
            p->schedulerQueue.pop();
         }
      }
      p->schedulerLock.unlock();
   }
   nxlog_debug_tag(DEBUG_TAG, 3, _T("Maintenance thread for thread pool %s stopped"), p->name);
}

/**
 * Create thread pool
 */
ThreadPool LIBNETXMS_EXPORTABLE *ThreadPoolCreate(const TCHAR *name, int minThreads, int maxThreads, int stackSize)
{
   auto p = new ThreadPool(name, minThreads, maxThreads, stackSize);
   p->maintThread = ThreadCreateEx(MaintenanceThread, p, 256 * 1024);

   p->mutex.lock();
   for(int i = 0; i < p->minThreads; i++)
   {
      WorkerThreadInfo *wt = new WorkerThreadInfo;
      wt->pool = p;
      wt->handle = ThreadCreateEx(WorkerThread, wt, stackSize);
      if (wt->handle != INVALID_THREAD_HANDLE)
      {
         p->threads.set(CAST_FROM_POINTER(wt, uint64_t), wt);
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 1, _T("Cannot create worker thread in pool %s"), p->name);
         delete wt;
      }
   }
   p->mutex.unlock();

   s_registryLock.lock();
   s_registry.set(p->name, p);
   s_registryLock.unlock();

   nxlog_debug_tag(DEBUG_TAG, 1, _T("Thread pool %s initialized (min=%d, max=%d)"), p->name, p->minThreads, p->maxThreads);
   return p;
}

/**
 * Destroy thread pool
 */
void LIBNETXMS_EXPORTABLE ThreadPoolDestroy(ThreadPool *p)
{
   if (p == nullptr)
   {
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Internal error: ThreadPoolDestroy called with null pointer"));
      return;
   }

   nxlog_debug_tag(DEBUG_TAG, 3, _T("Stopping threads in thread pool %s"), p->name);

   s_registryLock.lock();
   s_registry.remove(p->name);
   s_registryLock.unlock();

   p->shutdownMode = true;

   p->maintThreadWakeup.set();
   ThreadJoin(p->maintThread);

   WorkRequest rq;
   rq.func = nullptr;
   rq.arg = nullptr;
   rq.queueTime = GetCurrentTimeMs();
   p->mutex.lock();
   int count = p->threads.size();
   for(int i = 0; i < count; i++)
      p->queue.put(rq);
   p->mutex.unlock();

   p->threads.forEach(
      [] (const uint64_t& key, WorkerThreadInfo *object) -> EnumerationCallbackResult
      {
         ThreadJoin(object->handle);
         return _CONTINUE;
      });

   nxlog_debug_tag(DEBUG_TAG, 1, _T("Thread pool %s destroyed"), p->name);
   delete p;
}

/**
 * Execute task as soon as possible
 */
void LIBNETXMS_EXPORTABLE ThreadPoolExecute(ThreadPool *p, ThreadPoolWorkerFunction f, void *arg)
{
   if (p->shutdownMode)
      return;

   InterlockedIncrement(&p->activeRequests);
   InterlockedIncrement64(&p->taskExecutionCount);
   WorkRequest rq;
   rq.func = f;
   rq.arg = arg;
   rq.queueTime = GetCurrentTimeMs();
   p->queue.put(rq);
}

/**
 * Request serialization data
 */
struct RequestSerializationData
{
   ThreadPool *pool;
   SerializationQueue *queue;
   TCHAR key[1];  // Actual length is determined at runtime
};

/**
 * Worker function to process serialized requests
 */
static void ProcessSerializedRequests(RequestSerializationData *data)
{
   while(true)
   {
      WorkRequest rq;
      if (!data->queue->get(&rq))
      {
         // Because this thread normally does not hold serialization lock,
         // new serialized task may have been placed into queue between
         // get and lock calls. To avoid loosing it re-check queue again
         // with serialization lock being held.
         data->pool->serializationLock.lock();
         if (!data->queue->get(&rq))
         {
            data->pool->serializationQueues.remove(data->key);
            data->pool->serializationLock.unlock();
            break;
         }
         data->pool->serializationLock.unlock();
      }
      data->queue->updateMaxWaitTime(static_cast<uint32_t>(GetCurrentTimeMs() - rq.queueTime));

      rq.func(rq.arg);
   }
   MemFree(data);
}

/**
 * Execute task serialized (not before previous task with same key ends)
 */
void LIBNETXMS_EXPORTABLE ThreadPoolExecuteSerialized(ThreadPool *p, const TCHAR *key, ThreadPoolWorkerFunction f, void *arg)
{
   if (p->shutdownMode)
      return;

   WorkRequest rq;
   rq.func = f;
   rq.arg = arg;
   rq.queueTime = GetCurrentTimeMs();

   p->serializationLock.lock();
   SerializationQueue *q = p->serializationQueues.get(key);
   if (q == nullptr)
   {
      q = new SerializationQueue();
      p->serializationQueues.set(key, q);
      q->put(rq);

      size_t keyLen = _tcslen(key);
      auto data = static_cast<RequestSerializationData*>(MemAlloc(keyLen * sizeof(TCHAR) + sizeof(RequestSerializationData)));
      data->pool = p;
      data->queue = q;
      memcpy(data->key, key, (keyLen + 1) * sizeof(TCHAR));
      ThreadPoolExecute(p, ProcessSerializedRequests, data);
   }
   else
   {
      q->put(rq);
      InterlockedIncrement64(&p->taskExecutionCount);
   }
   p->serializationLock.unlock();
}

/**
 * Schedule task for execution using absolute time (in milliseconds)
 */
void LIBNETXMS_EXPORTABLE ThreadPoolScheduleAbsoluteMs(ThreadPool *p, int64_t runTime, ThreadPoolWorkerFunction f, void *arg)
{
   if (p->shutdownMode)
      return;

   WorkRequest rq;
   rq.func = f;
   rq.arg = arg;
   rq.runTime = runTime;
   rq.queueTime = GetCurrentTimeMs();

   p->schedulerLock.lock();
   p->schedulerQueue.push(rq);
   p->schedulerLock.unlock();
   p->maintThreadWakeup.set();
}

/**
 * Schedule task for execution using absolute time
 */
void LIBNETXMS_EXPORTABLE ThreadPoolScheduleAbsolute(ThreadPool *p, time_t runTime, ThreadPoolWorkerFunction f, void *arg)
{
   ThreadPoolScheduleAbsoluteMs(p, static_cast<int64_t>(runTime) * 1000, f, arg);
}

/**
 * Schedule task for execution using relative time (delay in milliseconds)
 */
void LIBNETXMS_EXPORTABLE ThreadPoolScheduleRelative(ThreadPool *p, uint32_t delay, ThreadPoolWorkerFunction f, void *arg)
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
   p->mutex.lock();
   info->name = p->name;
   info->minThreads = p->minThreads;
   info->maxThreads = p->maxThreads;
   info->curThreads = p->threads.size();
   info->threadStarts = p->threadStartCount;
   info->threadStops = p->threadStopCount;
   info->activeRequests = p->activeRequests;
   info->totalRequests = p->taskExecutionCount;
   info->load = (info->curThreads > 0) ? info->activeRequests * 100 / info->curThreads : 0;
   info->usage = info->curThreads * 100 / info->maxThreads;
   info->loadAvg[0] = GetExpMovingAverageValue(p->loadAverage[0]);
   info->loadAvg[1] = GetExpMovingAverageValue(p->loadAverage[1]);
   info->loadAvg[2] = GetExpMovingAverageValue(p->loadAverage[2]);
   info->waitTimeEMA = static_cast<uint32_t>(p->waitTimeEMA / EMA_FP_1);
   info->waitTimeSMA = static_cast<uint32_t>(p->waitTimeVariance.mean());
   info->waitTimeSD = static_cast<uint32_t>(p->waitTimeVariance.sd());
   info->queueSizeEMA = static_cast<uint32_t>(p->queueSizeEMA / EMA_FP_1);
   info->queueSizeSMA = static_cast<uint32_t>(p->queueSizeVariance.mean());
   info->queueSizeSD = static_cast<uint32_t>(p->queueSizeVariance.sd());
   p->mutex.unlock();

   p->schedulerLock.lock();
   info->scheduledRequests = static_cast<int32_t>(p->schedulerQueue.size());
   p->schedulerLock.unlock();

   info->serializedRequests = 0;
   p->serializationLock.lock();
   auto it = p->serializationQueues.begin();
   while(it.hasNext())
      info->serializedRequests += static_cast<int>(it.next()->value->size());
   p->serializationLock.unlock();
}

/**
 * Get pool information by name
 */
bool LIBNETXMS_EXPORTABLE ThreadPoolGetInfo(const TCHAR *name, ThreadPoolInfo *info)
{
   s_registryLock.lock();
   ThreadPool *p = s_registry.get(name);
   if (p != nullptr)
      ThreadPoolGetInfo(p, info);
   s_registryLock.unlock();
   return p != nullptr;
}

/**
 * Get registered thread pool by name
 */
ThreadPool LIBNETXMS_EXPORTABLE *ThreadPoolGetByName(const TCHAR *name)
{
   s_registryLock.lock();
   ThreadPool *p = s_registry.get(name);
   s_registryLock.unlock();
   return p;
}

/**
 * Get all thread pool names
 */
StringList LIBNETXMS_EXPORTABLE ThreadPoolGetAllPools()
{
   s_registryLock.lock();
   StringList list = s_registry.keys();
   s_registryLock.unlock();
   return list;
}

/**
 * Get number of queued jobs on the pool by key
 */
int LIBNETXMS_EXPORTABLE ThreadPoolGetSerializedRequestCount(ThreadPool *p, const TCHAR *key)
{
   p->serializationLock.lock();
   SerializationQueue *q = p->serializationQueues.get(key);
   int count = (q != nullptr) ? static_cast<int>(q->size()) : 0;
   p->serializationLock.unlock();
   return count;
}

/**
 * Get number of queued jobs on the pool by key
 */
uint32_t LIBNETXMS_EXPORTABLE ThreadPoolGetSerializedRequestMaxWaitTime(ThreadPool *p, const TCHAR *key)
{
   p->serializationLock.lock();
   SerializationQueue *q = p->serializationQueues.get(key);
   uint32_t waitTime = (q != nullptr) ? q->getMaxWaitTime() : 0;
   p->serializationLock.unlock();
   return waitTime;
}

/**
 * Set thread pool resize parameters - responsiveness and wait time high/low watermarks
 */
void LIBNETXMS_EXPORTABLE ThreadPoolSetResizeParameters(int responsiveness, uint32_t waitTimeHWM, uint32_t waitTimeLWM)
{
   if ((responsiveness > 0) && (responsiveness <= 24))
      s_maintThreadResponsiveness = responsiveness;
   if (waitTimeHWM > 0)
      s_waitTimeHighWatermark = waitTimeHWM;
   if (waitTimeLWM > 0)
      s_waitTimeLowWatermark = waitTimeLWM;
}
