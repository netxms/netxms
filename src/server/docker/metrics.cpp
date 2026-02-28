/*
** NetXMS Docker cloud connector module
** Copyright (C) 2026 Raden Solutions
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
** File: metrics.cpp
**/

#include "docker.h"

/**
 * Container metric definitions
 */
static void AddContainerMetricDefinitions(ObjectArray<MetricDefinition> *list)
{
   auto addMetric = [list](const WCHAR *name, const WCHAR *displayName, const WCHAR *unit,
      const WCHAR *description, uint32_t aggregations, int16_t defaultAgg, uint32_t minInterval)
   {
      MetricDefinition *m = new MetricDefinition();
      wcslcpy(m->name, name, MAX_PARAM_NAME);
      wcslcpy(m->displayName, displayName, 256);
      wcslcpy(m->unit, unit, 64);
      wcslcpy(m->description, description, 1024);
      m->supportedAggregations = aggregations;
      m->defaultAggregation = defaultAgg;
      m->minInterval = minInterval;
      list->add(m);
   };

   uint32_t avgMinMax = (1 << METRIC_AGGREGATION_AVERAGE) | (1 << METRIC_AGGREGATION_MIN) | (1 << METRIC_AGGREGATION_MAX);
   uint32_t avgOnly = (1 << METRIC_AGGREGATION_AVERAGE);

   addMetric(L"cpu_percent", L"CPU Usage", L"Percent",
      L"Container CPU usage as a percentage of total host CPU", avgMinMax, METRIC_AGGREGATION_AVERAGE, 30);

   addMetric(L"memory_usage", L"Memory Usage", L"Bytes",
      L"Current memory usage of the container", avgMinMax, METRIC_AGGREGATION_AVERAGE, 30);

   addMetric(L"memory_limit", L"Memory Limit", L"Bytes",
      L"Memory limit assigned to the container", avgOnly, METRIC_AGGREGATION_AVERAGE, 300);

   addMetric(L"memory_percent", L"Memory Usage Percentage", L"Percent",
      L"Container memory usage as a percentage of its limit", avgMinMax, METRIC_AGGREGATION_AVERAGE, 30);

   addMetric(L"network_rx_bytes", L"Network RX Bytes", L"Bytes",
      L"Total bytes received across all container network interfaces", avgMinMax, METRIC_AGGREGATION_AVERAGE, 30);

   addMetric(L"network_tx_bytes", L"Network TX Bytes", L"Bytes",
      L"Total bytes transmitted across all container network interfaces", avgMinMax, METRIC_AGGREGATION_AVERAGE, 30);

   addMetric(L"block_read_bytes", L"Block Read Bytes", L"Bytes",
      L"Total bytes read from block devices", avgMinMax, METRIC_AGGREGATION_AVERAGE, 30);

   addMetric(L"block_write_bytes", L"Block Write Bytes", L"Bytes",
      L"Total bytes written to block devices", avgMinMax, METRIC_AGGREGATION_AVERAGE, 30);

   addMetric(L"pids", L"Process Count", L"Count",
      L"Number of processes running in the container", avgMinMax, METRIC_AGGREGATION_AVERAGE, 30);

   addMetric(L"restart_count", L"Restart Count", L"Count",
      L"Number of times the container has been restarted", avgOnly, METRIC_AGGREGATION_AVERAGE, 60);
}

/**
 * Get metric definitions for a given resource type
 */
ObjectArray<MetricDefinition> *DockerGetMetricDefinitions(const TCHAR *resourceType, const TCHAR *resourceId,
   json_t *credentials)
{
   if (!wcscmp(resourceType, L"docker.container"))
   {
      ObjectArray<MetricDefinition> *list = new ObjectArray<MetricDefinition>(16, 16, Ownership::True);
      AddContainerMetricDefinitions(list);
      return list;
   }

   return nullptr;
}

/**
 * Calculate CPU percentage from Docker stats response
 */
static double CalculateCPUPercent(json_t *stats)
{
   json_t *cpuStats = json_object_get(stats, "cpu_stats");
   json_t *preCpuStats = json_object_get(stats, "precpu_stats");
   if (!json_is_object(cpuStats) || !json_is_object(preCpuStats))
      return 0.0;

   json_t *cpuUsage = json_object_get(cpuStats, "cpu_usage");
   json_t *preCpuUsage = json_object_get(preCpuStats, "cpu_usage");
   if (!json_is_object(cpuUsage) || !json_is_object(preCpuUsage))
      return 0.0;

   int64_t totalUsage = json_integer_value(json_object_get(cpuUsage, "total_usage"));
   int64_t preTotalUsage = json_integer_value(json_object_get(preCpuUsage, "total_usage"));
   int64_t systemUsage = json_integer_value(json_object_get(cpuStats, "system_cpu_usage"));
   int64_t preSystemUsage = json_integer_value(json_object_get(preCpuStats, "system_cpu_usage"));

   int64_t cpuDelta = totalUsage - preTotalUsage;
   int64_t systemDelta = systemUsage - preSystemUsage;

   if (systemDelta <= 0 || cpuDelta < 0)
      return 0.0;

   // Get number of CPUs
   int onlineCPUs = 0;
   json_t *onlineCPUsJson = json_object_get(cpuStats, "online_cpus");
   if (json_is_integer(onlineCPUsJson))
      onlineCPUs = static_cast<int>(json_integer_value(onlineCPUsJson));

   if (onlineCPUs <= 0)
      onlineCPUs = 1;

   return (static_cast<double>(cpuDelta) / static_cast<double>(systemDelta)) * onlineCPUs * 100.0;
}

/**
 * Sum network bytes across all interfaces
 */
static int64_t SumNetworkBytes(json_t *stats, const char *field)
{
   json_t *networks = json_object_get(stats, "networks");
   if (!json_is_object(networks))
      return 0;

   int64_t total = 0;
   const char *ifName;
   json_t *ifData;
   json_object_foreach(networks, ifName, ifData)
   {
      json_t *val = json_object_get(ifData, field);
      if (json_is_integer(val))
         total += json_integer_value(val);
   }
   return total;
}

/**
 * Sum block I/O bytes
 */
static int64_t SumBlockIOBytes(json_t *stats, const char *op)
{
   json_t *blkioStats = json_object_get(stats, "blkio_stats");
   if (!json_is_object(blkioStats))
      return 0;

   json_t *ioServiceBytes = json_object_get(blkioStats, "io_service_bytes_recursive");
   if (!json_is_array(ioServiceBytes))
      return 0;

   int64_t total = 0;
   size_t index;
   json_t *entry;
   json_array_foreach(ioServiceBytes, index, entry)
   {
      const char *entryOp = json_object_get_string_utf8(entry, "op", "");
      if (!stricmp(entryOp, op))
      {
         json_t *val = json_object_get(entry, "value");
         if (json_is_integer(val))
            total += json_integer_value(val);
      }
   }
   return total;
}

/**
 * Collect a metric value for a container
 */
DataCollectionError DockerCollect(const TCHAR *resourceId, const TCHAR *metric, int16_t aggregation, TCHAR *value, size_t bufLen, json_t *credentials)
{
   DockerClient client;
   if (!client.configure(credentials))
      return DCE_COMM_ERROR;

   // Convert resource ID to UTF-8 for API calls
   char containerId[1024];
   wchar_to_utf8(resourceId, -1, containerId, sizeof(containerId));

   // Convert metric name to UTF-8
   char metricName[MAX_PARAM_NAME];
   wchar_to_utf8(metric, -1, metricName, sizeof(metricName));

   // restart_count uses container inspect, not stats
   if (!strcmp(metricName, "restart_count"))
   {
      char path[1280];
      snprintf(path, sizeof(path), "/containers/%s/json", containerId);
      json_t *response = client.httpGet(path);
      if (response == nullptr)
         return DCE_COMM_ERROR;

      json_t *restartCount = json_object_get(response, "RestartCount");
      if (json_is_integer(restartCount))
      {
         ret_int64(value, json_integer_value(restartCount));
      }
      else
      {
         wcscpy(value, L"0");
      }
      json_decref(response);
      return DCE_SUCCESS;
   }

   // All other metrics use container stats
   char path[1280];
   snprintf(path, sizeof(path), "/containers/%s/stats?stream=false&one-shot=true", containerId);
   json_t *stats = client.httpGet(path);
   if (stats == nullptr)
      return DCE_COMM_ERROR;

   DataCollectionError result = DCE_SUCCESS;

   if (!strcmp(metricName, "cpu_percent"))
   {
      ret_double(value, CalculateCPUPercent(stats), 2);
   }
   else if (!strcmp(metricName, "memory_usage"))
   {
      json_t *memStats = json_object_get(stats, "memory_stats");
      ret_int64(value, json_object_get_int64(memStats, "usage"));
   }
   else if (!strcmp(metricName, "memory_limit"))
   {
      json_t *memStats = json_object_get(stats, "memory_stats");
      ret_int64(value, json_object_get_int64(memStats, "limit"));
   }
   else if (!strcmp(metricName, "memory_percent"))
   {
      json_t *memStats = json_object_get(stats, "memory_stats");
      if (json_is_object(memStats))
      {
         int64_t usage = json_integer_value(json_object_get(memStats, "usage"));
         int64_t limit = json_integer_value(json_object_get(memStats, "limit"));
         double percent = (limit > 0) ? (static_cast<double>(usage) / static_cast<double>(limit) * 100.0) : 0.0;
         ret_double(value, percent, 2);
      }
      else
      {
         wcscpy(value, L"0.00");
      }
   }
   else if (!strcmp(metricName, "network_rx_bytes"))
   {
      ret_int64(value, SumNetworkBytes(stats, "rx_bytes"));
   }
   else if (!strcmp(metricName, "network_tx_bytes"))
   {
      ret_int64(value, SumNetworkBytes(stats, "tx_bytes"));
   }
   else if (!strcmp(metricName, "block_read_bytes"))
   {
      ret_int64(value, SumBlockIOBytes(stats, "read"));
   }
   else if (!strcmp(metricName, "block_write_bytes"))
   {
      ret_int64(value, SumBlockIOBytes(stats, "write"));
   }
   else if (!strcmp(metricName, "pids"))
   {
      json_t *pidsStats = json_object_get(stats, "pids_stats");
      ret_int64(value, json_object_get_int64(pidsStats, "current"));
   }
   else
   {
      result = DCE_NOT_SUPPORTED;
   }

   json_decref(stats);
   return result;
}

/**
 * Collect table data (not supported)
 */
DataCollectionError DockerCollectTable(const TCHAR *resourceId, const TCHAR *metric, int16_t aggregation, shared_ptr<Table> *value, json_t *credentials)
{
   return DCE_NOT_SUPPORTED;
}

/**
 * Query current state of a resource
 */
int16_t DockerQueryState(const TCHAR *resourceId, char *providerState, size_t bufLen, json_t *credentials)
{
   DockerClient client;
   if (!client.configure(credentials))
      return RESOURCE_STATE_UNKNOWN;

   char id[1024];
   wchar_to_utf8(resourceId, -1, id, sizeof(id));

   // Try container inspect first
   char path[1280];
   snprintf(path, sizeof(path), "/containers/%s/json", id);
   json_t *response = client.httpGet(path);
   if (response != nullptr)
   {
      json_t *stateObj = json_object_get(response, "State");
      const char *status = nullptr;
      if (json_is_object(stateObj))
         status = json_object_get_string_utf8(stateObj, "Status", nullptr);

      int16_t state = DockerContainerStateToResourceState(status);
      if (providerState != nullptr)
         strlcpy(providerState, (status != nullptr) ? status : "", bufLen);
      json_decref(response);
      return state;
   }

   // Try network inspect
   snprintf(path, sizeof(path), "/networks/%s", id);
   response = client.httpGet(path);
   if (response != nullptr)
   {
      if (providerState != nullptr)
         strlcpy(providerState, "active", bufLen);
      json_decref(response);
      return RESOURCE_STATE_ACTIVE;
   }

   // Assume volume (volumes API uses name, not ID)
   snprintf(path, sizeof(path), "/volumes/%s", id);
   response = client.httpGet(path);
   if (response != nullptr)
   {
      if (providerState != nullptr)
         strlcpy(providerState, "active", bufLen);
      json_decref(response);
      return RESOURCE_STATE_ACTIVE;
   }

   return RESOURCE_STATE_UNKNOWN;
}
