/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Victor Kirhenshtein
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
#include <agent_tunnel.h>
#include <ncdrv.h>

#ifndef _WIN32
#include <sys/resource.h>
#endif

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
   nxlog_debug_tag(DEBUG_TAG, 1, L"Server statistic collector thread started");
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
   nxlog_debug_tag(DEBUG_TAG, 1, L"Server statistic collector thread stopped");
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

/**
 * Performance counters
 */
extern VolatileCounter64 g_snmpTrapsReceived;
extern VolatileCounter64 g_syslogMessagesReceived;
extern VolatileCounter64 g_windowsEventsReceived;
extern uint32_t g_averageDCIQueuingTime;

/**
 * AI usage counters
 */
extern VolatileCounter64 g_aiTotalRequests;
extern VolatileCounter64 g_aiTotalInputTokens;
extern VolatileCounter64 g_aiTotalOutputTokens;
extern VolatileCounter64 g_aiFailedRequests;
bool GetAIProviderStats(const TCHAR *providerName, int64_t *totalRequests, int64_t *totalInputTokens, int64_t *totalOutputTokens, int64_t *failedRequests);

/**
 * Get syncer run time statistic
 */
int64_t GetSyncerRunTime(StatisticType statType);

/**
 * Get internal metric from performance data storage driver
 */
DataCollectionError GetPerfDataStorageDriverMetric(const wchar_t *driver, const wchar_t *metric, wchar_t *value);

/**
 * Get status of notification channel
 */
bool GetNotificationChannelStatus(const TCHAR *name, NotificationChannelStatus *status);

/**
 * Get active discovery state
 */
bool IsActiveDiscoveryRunning();
String GetCurrentActiveDiscoveryRange();

/**
 * Get statistic for specific event processor
 */
static DataCollectionError GetEventProcessorStatistic(const TCHAR *param, int type, TCHAR *value)
{
   TCHAR pidText[64];
   if (!AgentGetParameterArg(param, 1, pidText, 64))
      return DCE_NOT_SUPPORTED;
   int pid = _tcstol(pidText, nullptr, 0);
   if (pid < 1)
      return DCE_NOT_SUPPORTED;

   StructArray<EventProcessingThreadStats> *stats = GetEventProcessingThreadStats();
   if (pid > stats->size())
   {
      delete stats;
      return DCE_NOT_SUPPORTED;
   }

   auto s = stats->get(pid - 1);
   switch(type)
   {
      case 'B':
         ret_uint(value, s->bindings);
         break;
      case 'P':
         ret_uint64(value, s->processedEvents);
         break;
      case 'Q':
         ret_uint(value, s->queueSize);
         break;
      case 'W':
         ret_uint(value, s->averageWaitTime);
         break;
   }

   delete stats;
   return DCE_SUCCESS;
}

/**
 * Get notification channel status and execute provided callback
 */
static DataCollectionError GetNotificationChannelStatus(const TCHAR *metric, std::function<void (const NotificationChannelStatus&)> callback)
{
   wchar_t channel[64];
   AgentGetParameterArg(metric, 1, channel, 64);

   NotificationChannelStatus status;
   if (!GetNotificationChannelStatus(channel, &status))
      return DCE_NO_SUCH_INSTANCE;

   callback(status);
   return DCE_SUCCESS;
}

/**
 * Get value for server's internal metric (local management node)
 */
DataCollectionError GetLocalManagementServerMetric(const wchar_t *name, wchar_t *buffer, size_t size)
{
   DataCollectionError rc = DCE_SUCCESS;

   if (!wcsicmp(name, L"Server.ActiveAlarms"))
   {
      ret_uint(buffer, GetAlarmCount());
   }
   else if (!wcsicmp(name, L"Server.ActiveNetworkDiscovery.CurrentRange"))
   {
      ret_string(buffer, GetCurrentActiveDiscoveryRange());
   }
   else if (!wcsicmp(name, L"Server.ActiveNetworkDiscovery.IsRunning"))
   {
      ret_boolean(buffer, IsActiveDiscoveryRunning());
   }
   else if (!wcsicmp(name, L"Server.AI.FailedRequests"))
   {
      ret_uint64(buffer, g_aiFailedRequests);
   }
   else if (MatchString(L"Server.AI.FailedRequests(*)", name, false))
   {
      wchar_t providerName[256];
      AgentGetMetricArgW(name, 1, providerName, 256);
      int64_t totalRequests, totalInputTokens, totalOutputTokens, failedRequests;
      if (GetAIProviderStats(providerName, &totalRequests, &totalInputTokens, &totalOutputTokens, &failedRequests))
         ret_int64(buffer, failedRequests);
      else
         rc = DCE_NOT_SUPPORTED;
   }
   else if (!wcsicmp(name, L"Server.AI.TokensIn"))
   {
      ret_uint64(buffer, g_aiTotalInputTokens);
   }
   else if (MatchString(L"Server.AI.TokensIn(*)", name, false))
   {
      wchar_t providerName[256];
      AgentGetMetricArgW(name, 1, providerName, 256);
      int64_t totalRequests, totalInputTokens, totalOutputTokens, failedRequests;
      if (GetAIProviderStats(providerName, &totalRequests, &totalInputTokens, &totalOutputTokens, &failedRequests))
         ret_int64(buffer, totalInputTokens);
      else
         rc = DCE_NOT_SUPPORTED;
   }
   else if (!wcsicmp(name, L"Server.AI.TokensOut"))
   {
      ret_uint64(buffer, g_aiTotalOutputTokens);
   }
   else if (MatchString(L"Server.AI.TokensOut(*)", name, false))
   {
      wchar_t providerName[256];
      AgentGetMetricArgW(name, 1, providerName, 256);
      int64_t totalRequests, totalInputTokens, totalOutputTokens, failedRequests;
      if (GetAIProviderStats(providerName, &totalRequests, &totalInputTokens, &totalOutputTokens, &failedRequests))
         ret_int64(buffer, totalOutputTokens);
      else
         rc = DCE_NOT_SUPPORTED;
   }
   else if (!wcsicmp(name, L"Server.AI.TotalRequests"))
   {
      ret_uint64(buffer, g_aiTotalRequests);
   }
   else if (MatchString(L"Server.AI.TotalRequests(*)", name, false))
   {
      wchar_t providerName[256];
      AgentGetMetricArgW(name, 1, providerName, 256);
      int64_t totalRequests, totalInputTokens, totalOutputTokens, failedRequests;
      if (GetAIProviderStats(providerName, &totalRequests, &totalInputTokens, &totalOutputTokens, &failedRequests))
         ret_int64(buffer, totalRequests);
      else
         rc = DCE_NOT_SUPPORTED;
   }
   else if (!wcsicmp(name, L"Server.AgentTunnels.Bound.AgentProxy"))
   {
      ret_int(buffer, GetTunnelCount(TunnelCapabilityFilter::AGENT_PROXY, true));
   }
   else if (!wcsicmp(name, L"Server.AgentTunnels.Bound.SnmpProxy"))
   {
      ret_int(buffer, GetTunnelCount(TunnelCapabilityFilter::SNMP_PROXY, true));
   }
   else if (!wcsicmp(name, L"Server.AgentTunnels.Bound.SnmpTrapProxy"))
   {
      ret_int(buffer, GetTunnelCount(TunnelCapabilityFilter::SNMP_TRAP_PROXY, true));
   }
   else if (!wcsicmp(name, L"Server.AgentTunnels.Bound.SyslogProxy"))
   {
      ret_int(buffer, GetTunnelCount(TunnelCapabilityFilter::SYSLOG_PROXY, true));
   }
   else if (!wcsicmp(name, L"Server.AgentTunnels.Bound.Total"))
   {
      ret_int(buffer, GetTunnelCount(TunnelCapabilityFilter::ANY, true));
   }
   else if (!wcsicmp(name, L"Server.AgentTunnels.Bound.UserAgent"))
   {
      ret_int(buffer, GetTunnelCount(TunnelCapabilityFilter::USER_AGENT, true));
   }
   else if (!wcsicmp(name, L"Server.AgentTunnels.Unbound.Total"))
   {
      ret_int(buffer, GetTunnelCount(TunnelCapabilityFilter::ANY, false));
   }
   else if (!wcsicmp(name, L"Server.AverageDCIQueuingTime"))
   {
      swprintf(buffer, size, L"%u", g_averageDCIQueuingTime);
   }
   else if (!wcsicmp(name, L"Server.Certificate.ExpirationDate"))
   {
      ret_string(buffer, GetServerCertificateExpirationDate());
   }
   else if (!wcsicmp(name, L"Server.Certificate.ExpirationTime"))
   {
      ret_int64(buffer, GetServerCertificateExpirationTime());
   }
   else if (!wcsicmp(name, L"Server.Certificate.ExpiresIn"))
   {
      ret_int(buffer, GetServerCertificateDaysUntilExpiration());
   }
   else if (!wcsicmp(name, L"Server.ClientSessions.Authenticated"))
   {
      swprintf(buffer, size, L"%d", GetSessionCount(true, false, -1, nullptr));
   }
   else if (MatchString(L"Server.ClientSessions.Authenticated(*)", name, false))
   {
      wchar_t loginName[256];
      AgentGetMetricArgW(name, 1, loginName, 256);
      IntegerToString(GetSessionCount(true, false, -1, loginName), buffer);
   }
   else if (!wcsicmp(name, L"Server.ClientSessions.Desktop"))
   {
      IntegerToString(GetSessionCount(true, true, CLIENT_TYPE_DESKTOP, nullptr), buffer);
   }
   else if (MatchString(L"Server.ClientSessions.Desktop(*)", name, false))
   {
      wchar_t loginName[256];
      AgentGetMetricArgW(name, 1, loginName, 256);
      IntegerToString(GetSessionCount(true, false, CLIENT_TYPE_DESKTOP, loginName), buffer);
   }
   else if (!wcsicmp(name, L"Server.ClientSessions.Mobile"))
   {
      IntegerToString(GetSessionCount(true, true, CLIENT_TYPE_MOBILE, nullptr), buffer);
   }
   else if (MatchString(L"Server.ClientSessions.Mobile(*)", name, false))
   {
      wchar_t loginName[256];
      AgentGetMetricArgW(name, 1, loginName, 256);
      IntegerToString(GetSessionCount(true, false, CLIENT_TYPE_MOBILE, loginName), buffer);
   }
   else if (!wcsicmp(name, L"Server.ClientSessions.Total"))
   {
      IntegerToString(GetSessionCount(true, true, -1, nullptr), buffer);
   }
   else if (!wcsicmp(name, L"Server.ClientSessions.Web"))
   {
      IntegerToString(GetSessionCount(true, true, CLIENT_TYPE_WEB, nullptr), buffer);
   }
   else if (MatchString(L"Server.ClientSessions.Web(*)", name, false))
   {
      wchar_t loginName[256];
      AgentGetMetricArgW(name, 1, loginName, 256);
      IntegerToString(GetSessionCount(true, false, CLIENT_TYPE_WEB, loginName), buffer);
   }
   else if (!wcsicmp(name, L"Server.DataCollectionItems"))
   {
      int dciCount = 0;
      g_idxObjectById.forEach([&dciCount](NetObj *object) -> EnumerationCallbackResult
         {
            if (object->isDataCollectionTarget())
               dciCount += static_cast<DataCollectionTarget*>(object)->getItemCount();
            return _CONTINUE;
         });
      ret_int(buffer, dciCount);
   }
   else if (!wcsicmp(name, L"Server.DB.Queries.Failed"))
   {
      LIBNXDB_PERF_COUNTERS counters;
      DBGetPerfCounters(&counters);
      IntegerToString(counters.failedQueries, buffer);
   }
   else if (!wcsicmp(name, L"Server.DB.Queries.LongRunning"))
   {
      LIBNXDB_PERF_COUNTERS counters;
      DBGetPerfCounters(&counters);
      IntegerToString(counters.longRunningQueries, buffer);
   }
   else if (!wcsicmp(name, L"Server.DB.Queries.NonSelect"))
   {
      LIBNXDB_PERF_COUNTERS counters;
      DBGetPerfCounters(&counters);
      IntegerToString(counters.nonSelectQueries, buffer);
   }
   else if (!wcsicmp(name, L"Server.DB.Queries.Select"))
   {
      LIBNXDB_PERF_COUNTERS counters;
      DBGetPerfCounters(&counters);
      IntegerToString(counters.selectQueries, buffer);
   }
   else if (!wcsicmp(name, L"Server.DB.Queries.Total"))
   {
      LIBNXDB_PERF_COUNTERS counters;
      DBGetPerfCounters(&counters);
      IntegerToString(counters.totalQueries, buffer);
   }
   else if (!wcsicmp(name, L"Server.DBWriter.Requests.IData"))
   {
      IntegerToString(g_idataWriteRequests, buffer);
   }
   else if (!wcsicmp(name, L"Server.DBWriter.Requests.Other"))
   {
      IntegerToString(g_otherWriteRequests, buffer);
   }
   else if (!wcsicmp(name, L"Server.DBWriter.Requests.RawData"))
   {
      IntegerToString(g_rawDataWriteRequests, buffer);
   }
   else if (MatchString(L"Server.EventProcessor.AverageWaitTime(*)", name, false))
   {
      rc = GetEventProcessorStatistic(name, 'W', buffer);
   }
   else if (MatchString(L"Server.EventProcessor.Bindings(*)", name, false))
   {
      rc = GetEventProcessorStatistic(name, 'B', buffer);
   }
   else if (MatchString(L"Server.EventProcessor.ProcessedEvents(*)", name, false))
   {
      rc = GetEventProcessorStatistic(name, 'P', buffer);
   }
   else if (MatchString(L"Server.EventProcessor.QueueSize(*)", name, false))
   {
      rc = GetEventProcessorStatistic(name, 'Q', buffer);
   }
   else if (!wcsicmp(name, L"Server.FileHandleLimit"))
   {
#ifdef _WIN32
      rc = DCE_NOT_SUPPORTED;
#else
      struct rlimit rl;
      if (getrlimit(RLIMIT_NOFILE, &rl) == 0)
         IntegerToString(static_cast<uint64_t>(rl.rlim_cur), buffer);
      else
         rc = DCE_COLLECTION_ERROR;
#endif
   }
   else if (!wcsicmp(name, L"Server.Heap.Active"))
   {
      int64_t bytes = GetActiveHeapMemory();
      if (bytes != -1)
         IntegerToString(bytes, buffer);
      else
         rc = DCE_NOT_SUPPORTED;
   }
   else if (!wcsicmp(name, L"Server.Heap.Allocated"))
   {
      int64_t bytes = GetAllocatedHeapMemory();
      if (bytes != -1)
         IntegerToString(bytes, buffer);
      else
         rc = DCE_NOT_SUPPORTED;
   }
   else if (!wcsicmp(name, L"Server.Heap.Mapped"))
   {
      int64_t bytes = GetMappedHeapMemory();
      if (bytes != -1)
         IntegerToString(bytes, buffer);
      else
         rc = DCE_NOT_SUPPORTED;
   }
   else if (!wcsicmp(name, L"Server.MemoryUsage.Alarms"))
   {
      ret_uint64(buffer, GetAlarmMemoryUsage());
   }
   else if (!wcsicmp(name, L"Server.MemoryUsage.DataCollectionCache"))
   {
      ret_uint64(buffer, GetDCICacheMemoryUsage());
   }
   else if (!wcsicmp(name, L"Server.MemoryUsage.RawDataWriter"))
   {
      ret_uint64(buffer, GetRawDataWriterMemoryUsage());
   }
   else if (MatchString(L"Server.NotificationChannel.HealthCheckStatus(*)", name, false))
   {
      rc = GetNotificationChannelStatus(name, [buffer](const NotificationChannelStatus& status) { ret_boolean(buffer, status.healthCheckStatus); });
   }
   else if (MatchString(L"Server.NotificationChannel.LastMessageTimestamp(*)", name, false))
   {
      rc = GetNotificationChannelStatus(name, [buffer](const NotificationChannelStatus& status) { ret_uint64(buffer, status.lastMessageTime); });
   }
   else if (MatchString(L"Server.NotificationChannel.MessageCount(*)", name, false))
   {
      rc = GetNotificationChannelStatus(name, [buffer](const NotificationChannelStatus& status) { ret_uint(buffer, status.messageCount); });
   }
   else if (MatchString(L"Server.NotificationChannel.QueueSize(*)", name, false))
   {
      rc = GetNotificationChannelStatus(name, [buffer](const NotificationChannelStatus& status) { ret_uint(buffer, status.queueSize); });
   }
   else if (MatchString(L"Server.NotificationChannel.SendFailureCount(*)", name, false))
   {
      rc = GetNotificationChannelStatus(name, [buffer](const NotificationChannelStatus& status) { ret_uint(buffer, status.failedSendCount); });
   }
   else if (MatchString(L"Server.NotificationChannel.SendStatus(*)", name, false))
   {
      rc = GetNotificationChannelStatus(name, [buffer](const NotificationChannelStatus& status) { ret_boolean(buffer, status.sendStatus); });
   }
   else if (!wcsicmp(name, L"Server.ObjectCount.AccessPoints"))
   {
      ret_uint(buffer, static_cast<uint32_t>(g_idxAccessPointById.size()));
   }
   else if (!wcsicmp(name, L"Server.ObjectCount.Clusters"))
   {
      ret_uint(buffer, static_cast<uint32_t>(g_idxClusterById.size()));
   }
   else if (!wcsicmp(name, L"Server.ObjectCount.Interfaces"))
   {
      uint32_t count = 0;
      g_idxObjectById.forEach(
         [&count] (NetObj *object) -> EnumerationCallbackResult
         {
            if (object->getObjectClass() == OBJECT_INTERFACE)
               count++;
            return _CONTINUE;
         });
      ret_uint(buffer, count);
   }
   else if (!wcsicmp(name, L"Server.ObjectCount.Nodes"))
   {
      ret_uint(buffer, static_cast<uint32_t>(g_idxNodeById.size()));
   }
   else if (!wcsicmp(name, L"Server.ObjectCount.Sensors"))
   {
      ret_uint(buffer, static_cast<uint32_t>(g_idxSensorById.size()));
   }
   else if (!wcsicmp(name, L"Server.ObjectCount.Total"))
   {
      ret_uint(buffer, static_cast<uint32_t>(g_idxObjectById.size()));
   }
   else if (MatchString(L"Server.PDS.DriverStat(*)", name, false))
   {
      wchar_t driver[64], metric[64];
      AgentGetMetricArgW(name, 1, driver, 64);
      AgentGetMetricArgW(name, 2, metric, 64);
      rc = GetPerfDataStorageDriverMetric(driver, metric, buffer);
   }
   else if (!wcsicmp(name, L"Server.Pollers.Autobind"))
   {
      ret_int(buffer, GetPollerCount(PollerType::AUTOBIND));
   }
   else if (!wcsicmp(name, L"Server.Pollers.Configuration"))
   {
      ret_int(buffer, GetPollerCount(PollerType::CONFIGURATION));
   }
   else if (!wcsicmp(name, L"Server.Pollers.Discovery"))
   {
      ret_int(buffer, GetPollerCount(PollerType::DISCOVERY));
   }
   else if (!wcsicmp(name, L"Server.Pollers.ICMP"))
   {
      ret_int(buffer, GetPollerCount(PollerType::ICMP));
   }
   else if (!wcsicmp(name, L"Server.Pollers.InstanceDiscovery"))
   {
      ret_int(buffer, GetPollerCount(PollerType::INSTANCE_DISCOVERY));
   }
   else if (!wcsicmp(name, L"Server.Pollers.MapUpdate"))
   {
      ret_int(buffer, GetPollerCount(PollerType::MAP_UPDATE));
   }
   else if (!wcsicmp(name, L"Server.Pollers.RoutingTable"))
   {
      ret_int(buffer, GetPollerCount(PollerType::ROUTING_TABLE));
   }
   else if (!wcsicmp(name, L"Server.Pollers.Status"))
   {
      ret_int(buffer, GetPollerCount(PollerType::STATUS));
   }
   else if (!wcsicmp(name, L"Server.Pollers.Topology"))
   {
      ret_int(buffer, GetPollerCount(PollerType::TOPOLOGY));
   }
   else if (!wcsicmp(name, L"Server.Pollers.Total"))
   {
      ret_int(buffer, GetTotalPollerCount());
   }
   else if (MatchString(L"Server.QueueSize.Average(*)", name, false))
   {
      rc = GetQueueStatistics(name, StatisticType::AVERAGE, buffer);
   }
   else if (MatchString(L"Server.QueueSize.Current(*)", name, false))
   {
      rc = GetQueueStatistics(name, StatisticType::CURRENT, buffer);
   }
   else if (MatchString(L"Server.QueueSize.Max(*)", name, false))
   {
      rc = GetQueueStatistics(name, StatisticType::MAX, buffer);
   }
   else if (MatchString(L"Server.QueueSize.Min(*)", name, false))
   {
      rc = GetQueueStatistics(name, StatisticType::MIN, buffer);
   }
   else if (!wcsicmp(name, L"Server.ReceivedSNMPTraps"))
   {
      ret_uint64(buffer, g_snmpTrapsReceived);
   }
   else if (!wcsicmp(name, L"Server.ReceivedSyslogMessages"))
   {
      ret_uint64(buffer, g_syslogMessagesReceived);
   }
   else if (!wcsicmp(name, L"Server.ReceivedWindowsEvents"))
   {
      ret_uint64(buffer, g_windowsEventsReceived);
   }
   else if (!wcsicmp(L"Server.SyncerRunTime.Average", name))
   {
      ret_int64(buffer, GetSyncerRunTime(StatisticType::AVERAGE));
   }
   else if (!wcsicmp(L"Server.SyncerRunTime.Last", name))
   {
      ret_int64(buffer, GetSyncerRunTime(StatisticType::CURRENT));
   }
   else if (!wcsicmp(L"Server.SyncerRunTime.Max", name))
   {
      ret_int64(buffer, GetSyncerRunTime(StatisticType::MAX));
   }
   else if (!wcsicmp(L"Server.SyncerRunTime.Min", name))
   {
      ret_int64(buffer, GetSyncerRunTime(StatisticType::MIN));
   }
   else if (MatchString(L"Server.ThreadPool.ActiveRequests(*)", name, false))
   {
      rc = GetThreadPoolStat(THREAD_POOL_ACTIVE_REQUESTS, name, buffer);
   }
   else if (MatchString(L"Server.ThreadPool.AverageWaitTime(*)", name, false))
   {
      rc = GetThreadPoolStat(THREAD_POOL_AVERAGE_WAIT_TIME, name, buffer);
   }
   else if (MatchString(L"Server.ThreadPool.CurrSize(*)", name, false))
   {
      rc = GetThreadPoolStat(THREAD_POOL_CURR_SIZE, name, buffer);
   }
   else if (MatchString(L"Server.ThreadPool.Load(*)", name, false))
   {
      rc = GetThreadPoolStat(THREAD_POOL_LOAD, name, buffer);
   }
   else if (MatchString(L"Server.ThreadPool.LoadAverage(*)", name, false))
   {
      rc = GetThreadPoolStat(THREAD_POOL_LOADAVG_1, name, buffer);
   }
   else if (MatchString(L"Server.ThreadPool.LoadAverage5(*)", name, false))
   {
      rc = GetThreadPoolStat(THREAD_POOL_LOADAVG_5, name, buffer);
   }
   else if (MatchString(L"Server.ThreadPool.LoadAverage15(*)", name, false))
   {
      rc = GetThreadPoolStat(THREAD_POOL_LOADAVG_15, name, buffer);
   }
   else if (MatchString(L"Server.ThreadPool.MaxSize(*)", name, false))
   {
      rc = GetThreadPoolStat(THREAD_POOL_MAX_SIZE, name, buffer);
   }
   else if (MatchString(L"Server.ThreadPool.MinSize(*)", name, false))
   {
      rc = GetThreadPoolStat(THREAD_POOL_MIN_SIZE, name, buffer);
   }
   else if (MatchString(L"Server.ThreadPool.ScheduledRequests(*)", name, false))
   {
      rc = GetThreadPoolStat(THREAD_POOL_SCHEDULED_REQUESTS, name, buffer);
   }
   else if (MatchString(L"Server.ThreadPool.Usage(*)", name, false))
   {
      rc = GetThreadPoolStat(THREAD_POOL_USAGE, name, buffer);
   }
   else if (!wcsicmp(name, L"Server.TotalEventsProcessed"))
   {
      ret_uint64(buffer, g_totalEventsProcessed);
   }
   else if (!wcsicmp(name, L"Server.Uptime"))
   {
      ret_int64(buffer, static_cast<int64_t>(time(nullptr) - g_serverStartTime));
   }
   else
   {
      rc = DCE_NOT_SUPPORTED;
   }

   return rc;
}
