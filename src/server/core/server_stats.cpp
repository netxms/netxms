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
** File: selfmon.cpp
**
**/

#include "nxcore.h"
#include <gauge_helpers.h>

#define DEBUG_TAG _T("statcoll")

/**
 * Collection interval
 */
#define QUEUE_STATS_COLLECTION_INTERVAL   5

/**
 * Period for calculating moving average
 */
#define QUEUE_STATS_AVERAGE_PERIOD        900

/**
 * Externals
 */
extern Queue g_syslogProcessingQueue;
extern Queue g_syslogWriteQueue;
extern ThreadPool *g_dataCollectorThreadPool;
extern ThreadPool *g_pollerThreadPool;
extern ThreadPool *g_schedulerThreadPool;

INT64 GetEventLogWriterQueueSize();

/**
 * Internal queue statistic
 */
static StringObjectMap<Gauge64> s_queues(true);
static Mutex s_queuesLock;

/**
 * Add queue to collector (using Queue object)
 */
inline void AddQueueToCollector(const TCHAR *name, Queue *queue)
{
   if (queue != NULL)
   {
      Gauge64 *gauge = new GaugeQueueLength(name, queue, QUEUE_STATS_COLLECTION_INTERVAL, QUEUE_STATS_AVERAGE_PERIOD);
      s_queues.set(name, gauge);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 2, _T("Queue %s does not exist"));
   }
}

/**
 * Add queue to collector (using thread pool object)
 */
inline void AddQueueToCollector(const TCHAR *name, ThreadPool *pool)
{
   if (pool != NULL)
   {
      Gauge64 *gauge = new GaugeThreadPoolRequests(name, pool, QUEUE_STATS_COLLECTION_INTERVAL, QUEUE_STATS_AVERAGE_PERIOD);
      s_queues.set(name, gauge);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 2, _T("Thread pool %s does not exist"));
   }
}

/**
 * Add queue to collector (using callback function)
 */
inline void AddQueueToCollector(const TCHAR *name, INT64 (*function)())
{
   Gauge64 *gauge = new GaugeFunction(name, function, QUEUE_STATS_COLLECTION_INTERVAL, QUEUE_STATS_AVERAGE_PERIOD);
   s_queues.set(name, gauge);
}

/**
 * Get total size of all DB writer queues
 */
static INT64 GetTotalDBWriterQueueSize()
{
   return GetIDataWriterQueueSize() + GetRawDataWriterQueueSize() + g_dbWriterQueue->size();
}

/**
 * Callback for updating queue length gauges
 */
static EnumerationCallbackResult UpdateGauge(const TCHAR *key, const void *value, void *context)
{
   const_cast<Gauge64*>(static_cast<const Gauge64*>(value))->update();
   return _CONTINUE;
}

/**
 * Statistics collection thread
 */
THREAD_RESULT THREAD_CALL ServerStatCollector(void *arg)
{
   ThreadSetName("StatCollector");

   s_queuesLock.lock();
   AddQueueToCollector(_T("DataCollector"), g_dataCollectorThreadPool);
   AddQueueToCollector(_T("DBWriter.IData"), GetIDataWriterQueueSize);
   AddQueueToCollector(_T("DBWriter.Other"), g_dbWriterQueue);
   AddQueueToCollector(_T("DBWriter.RawData"), GetRawDataWriterQueueSize);
   AddQueueToCollector(_T("DBWriter.Total"), GetTotalDBWriterQueueSize);
   AddQueueToCollector(_T("EventLogWriter"), GetEventLogWriterQueueSize);
   AddQueueToCollector(_T("EventProcessor"), &g_eventQueue);
   AddQueueToCollector(_T("NodeDiscoveryPoller"), GetDiscoveryPollerQueueSize);
   AddQueueToCollector(_T("Poller"), g_pollerThreadPool);
   AddQueueToCollector(_T("Scheduler"), g_schedulerThreadPool);
   AddQueueToCollector(_T("SyslogProcessor"), &g_syslogProcessingQueue);
   AddQueueToCollector(_T("SyslogWriter"), &g_syslogWriteQueue);
   AddQueueToCollector(_T("TemplateUpdater"), &g_templateUpdateQueue);
   s_queuesLock.unlock();

   nxlog_debug_tag(DEBUG_TAG, 1, _T("Server statistic collector thread started"));
   while(!SleepAndCheckForShutdown(QUEUE_STATS_COLLECTION_INTERVAL))
   {
      s_queuesLock.lock();
      s_queues.forEach(UpdateGauge, NULL);
      s_queuesLock.unlock();
   }
   nxlog_debug_tag(DEBUG_TAG, 1, _T("Server statistic collector thread stopped"));
   return THREAD_OK;
}

/**
 * Get queue statistic for internal parameter
 */
DataCollectionError GetQueueStatistic(const TCHAR *parameter, StatisticType type, TCHAR *value)
{
   TCHAR name[MAX_GAUGE_NAME_LEN];
   if (!AgentGetParameterArg(parameter, 1, name, MAX_GAUGE_NAME_LEN))
      return DCE_NOT_SUPPORTED;

   s_queuesLock.lock();

   Gauge64 *stat = s_queues.get(name);
   if (stat == NULL)
   {
      s_queuesLock.unlock();
      return DCE_NO_SUCH_INSTANCE;
   }

   if (stat->isNull())
   {
      s_queuesLock.unlock();
      return DCE_COLLECTION_ERROR;
   }

   switch(type)
   {
      case StatisticType::AVERAGE:
         ret_double(value, stat->getAverage());
         break;
      case StatisticType::CURRENT:
         ret_int64(value, stat->getCurrent());
         break;
      case StatisticType::MAX:
         ret_int64(value, stat->getMax());
         break;
      case StatisticType::MIN:
         ret_int64(value, stat->getMin());
         break;
   }
   s_queuesLock.unlock();
   return DCE_SUCCESS;
}

/**
 * DCI cache memory usage calculation callback
 */
static void GetCacheMemoryUsage(NetObj *object, UINT64 *size)
{
   if (object->isDataCollectionTarget())
      *size += static_cast<DataCollectionTarget*>(object)->getCacheMemoryUsage();
}

/**
 * Get amount of memory used by DCI cache
 */
UINT64 GetDCICacheMemoryUsage()
{
   UINT64 dciCache = 0;
   g_idxObjectById.forEach(GetCacheMemoryUsage, &dciCache);
   return dciCache;
}
