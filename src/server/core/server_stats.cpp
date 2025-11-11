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
** File: selfmon.cpp
**
**/

#include "nxcore.h"
#include <nxcore_discovery.h>
#include <gauge_helpers.h>

#define DEBUG_TAG _T("statcoll")

struct SnmpTrap;

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
extern ObjectQueue<SnmpTrap> g_snmpTrapProcessorQueue;
extern ObjectQueue<SnmpTrap> g_snmpTrapWriterQueue;
extern ObjectQueue<SyslogMessage> g_syslogProcessingQueue;
extern ObjectQueue<SyslogMessage> g_syslogWriteQueue;
extern ObjectQueue<WindowsEvent> g_windowsEventProcessingQueue;
extern ObjectQueue<WindowsEvent> g_windowsEventWriterQueue;
extern ThreadPool *g_dataCollectorThreadPool;
extern ThreadPool *g_pollerThreadPool;
extern ThreadPool *g_schedulerThreadPool;

int64_t GetEventLogWriterQueueSize();
int64_t GetEventProcessorQueueSize();

/**
 * Internal queue statistic
 */
static StringObjectMap<Gauge64> s_queues(Ownership::True);
static Mutex s_queuesLock;

/**
 * Add queue to collector (using Queue object)
 */
inline void AddQueueToCollector(const TCHAR *name, Queue *queue)
{
   if (queue != nullptr)
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
   if (pool != nullptr)
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
inline void AddQueueToCollector(const TCHAR *name, int64_t (*function)())
{
   Gauge64 *gauge = new GaugeFunction(name, function, QUEUE_STATS_COLLECTION_INTERVAL, QUEUE_STATS_AVERAGE_PERIOD);
   s_queues.set(name, gauge);
}

/**
 * Get total size of all DB writer queues
 */
static int64_t GetTotalDBWriterQueueSize()
{
   return GetIDataWriterQueueSize() + GetRawDataWriterQueueSize() + g_dbWriterQueue.size();
}

/**
 * Statistics collection thread
 */
void ServerStatCollector()
{
   ThreadSetName("StatCollector");

   s_queuesLock.lock();
   AddQueueToCollector(_T("DataCollector"), g_dataCollectorThreadPool);
   AddQueueToCollector(_T("DBWriter.IData"), GetIDataWriterQueueSize);
   AddQueueToCollector(_T("DBWriter.Other"), &g_dbWriterQueue);
   AddQueueToCollector(_T("DBWriter.RawData"), GetRawDataWriterQueueSize);
   AddQueueToCollector(_T("DBWriter.Total"), GetTotalDBWriterQueueSize);
   AddQueueToCollector(_T("EventLogWriter"), GetEventLogWriterQueueSize);
   AddQueueToCollector(_T("EventProcessor"), GetEventProcessorQueueSize);
   AddQueueToCollector(_T("NodeDiscoveryPoller"), GetDiscoveryPollerQueueSize);
   AddQueueToCollector(_T("Poller"), g_pollerThreadPool);
   AddQueueToCollector(_T("Scheduler"), g_schedulerThreadPool);
   AddQueueToCollector(_T("SNMPTrapProcessor"), &g_snmpTrapProcessorQueue);
   AddQueueToCollector(_T("SNMPTrapWriter"), &g_snmpTrapWriterQueue);
   AddQueueToCollector(_T("SyslogProcessor"), &g_syslogProcessingQueue);
   AddQueueToCollector(_T("SyslogWriter"), &g_syslogWriteQueue);
   AddQueueToCollector(_T("TemplateUpdater"), &g_templateUpdateQueue);
   AddQueueToCollector(_T("WindowsEventProcessor"), &g_windowsEventProcessingQueue);
   AddQueueToCollector(_T("WindowsEventWriter"), &g_windowsEventWriterQueue);
   s_queuesLock.unlock();

   int counter = 0;
   nxlog_debug_tag(DEBUG_TAG, 1, _T("Server statistic collector thread started"));
   while(!SleepAndCheckForShutdown(QUEUE_STATS_COLLECTION_INTERVAL))
   {
      s_queuesLock.lock();
      s_queues.forEach(
         [] (const TCHAR *key, const Gauge64 *gauge) -> EnumerationCallbackResult
         {
            const_cast<Gauge64*>(gauge)->update();
            return _CONTINUE;
         });
      counter++;
      if ((counter % (60 / QUEUE_STATS_COLLECTION_INTERVAL)) == 0)
      {
         counter = 0;
         s_queues.forEach(
            [] (const TCHAR *key, const Gauge64 *gauge) -> EnumerationCallbackResult
            {
               nxlog_debug_tag(_T("statcoll.output"), 8, _T("Queue name: %s; Cur: %d; Min: %d; Max: %d; Avg: %f"),
                  key, gauge->getCurrent(), gauge->getMin(), gauge->getMax(), gauge->getAverage());
               return _CONTINUE;
            });
      }
      s_queuesLock.unlock();
   }
   nxlog_debug_tag(DEBUG_TAG, 1, _T("Server statistic collector thread stopped"));
}

/**
 * Get queue statistic for internal parameter
 */
DataCollectionError GetQueueStatistics(const wchar_t *parameter, StatisticType type, wchar_t *value)
{
   wchar_t name[MAX_GAUGE_NAME_LEN];
   if (!AgentGetParameterArg(parameter, 1, name, MAX_GAUGE_NAME_LEN))
      return DCE_NOT_SUPPORTED;

   s_queuesLock.lock();

   Gauge64 *stat = s_queues.get(name);
   if (stat == nullptr)
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
static EnumerationCallbackResult GetCacheMemoryUsage(NetObj *object, uint64_t *size)
{
   if (object->isDataCollectionTarget())
      *size += static_cast<DataCollectionTarget*>(object)->getCacheMemoryUsage();
   return _CONTINUE;
}

/**
 * Get amount of memory used by DCI cache
 */
uint64_t GetDCICacheMemoryUsage()
{
   uint64_t dciCache = 0;
   g_idxObjectById.forEach(GetCacheMemoryUsage, &dciCache);
   return dciCache;
}

/**
 * NXSL function GetServerQueueNames
 */
int F_GetServerQueueNames(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   s_queuesLock.lock();
   *result = vm->createValue(new NXSL_Array(vm, s_queues.keys()));
   s_queuesLock.unlock();
   return NXSL_ERR_SUCCESS;
}

/**
 * Get stats of all queues as a table
 */
void GetAllQueueStatistics(Table *table)
{
   table->addColumn(L"NAME", DCI_DT_STRING, L"Queue name", true);
   table->addColumn(L"CURRENT", DCI_DT_UINT64, L"Current size");
   table->addColumn(L"MIN", DCI_DT_UINT64, L"Minimum size");
   table->addColumn(L"MAX", DCI_DT_UINT64, L"Maximum size");
   table->addColumn(L"AVERAGE", DCI_DT_FLOAT, L"Average size");

   s_queuesLock.lock();
   s_queues.forEach(
      [table] (const TCHAR *key, const Gauge64 *gauge) -> EnumerationCallbackResult
      {
         table->addRow();
         table->set(0, key);
         table->set(1, static_cast<uint64_t>(gauge->getCurrent()));
         table->set(2, static_cast<uint64_t>(gauge->getMin()));
         table->set(3, static_cast<uint64_t>(gauge->getMax()));
         table->set(4, gauge->getAverage());
         return _CONTINUE;
      });
   s_queuesLock.unlock();
}
