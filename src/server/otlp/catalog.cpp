/*
** NetXMS - Network Management System
** Copyright (C) 2024-2026 Raden Solutions
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
** File: catalog.cpp
**
** In-memory per-node catalog of OTLP metrics observed on the ingest stream.
** Stores metric name, type, and the set of attribute key names (never values),
** each with a "last seen" timestamp. Used to populate the metric selector in
** the management console. Entries age out via a sliding-window TTL.
**
**/

#include "otlp.h"

/**
 * Housekeeper sweep interval (milliseconds)
 */
#define METRIC_CATALOG_SWEEP_INTERVAL  300000

/**
 * Single metric entry: type, last-seen time, and observed attribute key names
 * (each mapped to its own last-seen time for key-level eviction).
 */
struct MetricCatalogEntry
{
   OTLPMetricType type;
   time_t lastSeen;
   std::map<std::string, time_t> attributeKeys;
};

/**
 * Catalog: node ID -> (metric name -> entry). Guarded by a single mutex - the
 * ingest hot path only does map lookups and timestamp writes under it, and the
 * housekeeper sweep is coarse (every few minutes).
 */
static std::map<uint32_t, std::map<std::string, MetricCatalogEntry>> s_catalog;
static Mutex s_catalogLock;
static bool s_housekeeperRunning = false;

/**
 * Map metric type to its lowercase wire name (also used as JSON value)
 */
static const char *MetricTypeName(OTLPMetricType type)
{
   switch (type)
   {
      case OTLPMetricType::GAUGE:
         return "gauge";
      case OTLPMetricType::SUM:
         return "sum";
      case OTLPMetricType::HISTOGRAM:
         return "histogram";
      default:
         return "unknown";
   }
}

/**
 * Record an observation of a metric for a node
 */
void RecordMetricObservation(uint32_t nodeId, const char *metricName, OTLPMetricType type, const std::map<std::string, std::string>& attributes)
{
   time_t now = time(nullptr);

   s_catalogLock.lock();
   MetricCatalogEntry& entry = s_catalog[nodeId][metricName];
   entry.type = type;
   entry.lastSeen = now;
   for (const auto& kv : attributes)
      entry.attributeKeys[kv.first] = now;
   s_catalogLock.unlock();
}

/**
 * Retention housekeeper: drop attribute keys and metrics not seen within the
 * configured window, then drop emptied node catalogs. Reschedules itself.
 */
static void MetricCatalogHousekeeper()
{
   if (!s_housekeeperRunning)
      return;

   uint32_t retention = ConfigReadULong(L"OTLP.MetricCatalogRetention", 86400);
   time_t cutoff = time(nullptr) - static_cast<time_t>(retention);
   int droppedMetrics = 0;

   s_catalogLock.lock();
   for (auto nodeIt = s_catalog.begin(); nodeIt != s_catalog.end(); )
   {
      auto& metrics = nodeIt->second;
      for (auto metricIt = metrics.begin(); metricIt != metrics.end(); )
      {
         MetricCatalogEntry& entry = metricIt->second;

         // Evict stale attribute keys
         for (auto keyIt = entry.attributeKeys.begin(); keyIt != entry.attributeKeys.end(); )
         {
            if (keyIt->second < cutoff)
               keyIt = entry.attributeKeys.erase(keyIt);
            else
               ++keyIt;
         }

         // Evict the metric itself if not seen within the window
         if (entry.lastSeen < cutoff)
         {
            metricIt = metrics.erase(metricIt);
            droppedMetrics++;
         }
         else
         {
            ++metricIt;
         }
      }

      if (metrics.empty())
         nodeIt = s_catalog.erase(nodeIt);
      else
         ++nodeIt;
   }
   s_catalogLock.unlock();

   if (droppedMetrics > 0)
      nxlog_debug_tag(DEBUG_TAG_OTLP, 6, L"Metric catalog housekeeper evicted %d stale metric(s)", droppedMetrics);

   if (s_housekeeperRunning)
      ThreadPoolScheduleRelative(g_mainThreadPool, METRIC_CATALOG_SWEEP_INTERVAL, MetricCatalogHousekeeper);
}

/**
 * Initialize per-node metric catalog
 */
void InitMetricCatalog()
{
   s_housekeeperRunning = true;
   ThreadPoolScheduleRelative(g_mainThreadPool, METRIC_CATALOG_SWEEP_INTERVAL, MetricCatalogHousekeeper);
   nxlog_debug_tag(DEBUG_TAG_OTLP, 1, L"OTLP metric catalog initialized");
}

/**
 * Shutdown per-node metric catalog
 */
void ShutdownMetricCatalog()
{
   s_housekeeperRunning = false;
   s_catalogLock.lock();
   s_catalog.clear();
   s_catalogLock.unlock();
   nxlog_debug_tag(DEBUG_TAG_OTLP, 1, L"OTLP metric catalog shut down");
}

/**
 * Fill an NXCP message with the metric catalog for a node.
 * Record layout (variable stride): base+0 = metric name, base+1 = type,
 * base+2 = attribute key count, base+3.. = attribute key names. The next
 * record starts at base + 3 + keyCount. VID_NUM_ELEMENTS holds the metric count.
 */
static void FillMessageWithMetrics(uint32_t nodeId, NXCPMessage *msg)
{
   uint32_t fieldId = VID_OTLP_METRIC_LIST_BASE;
   uint32_t count = 0;

   s_catalogLock.lock();
   auto nodeIt = s_catalog.find(nodeId);
   if (nodeIt != s_catalog.end())
   {
      for (const auto& metric : nodeIt->second)
      {
         msg->setFieldFromUtf8String(fieldId++, metric.first.c_str());
         msg->setField(fieldId++, static_cast<uint16_t>(metric.second.type));
         msg->setField(fieldId++, static_cast<uint32_t>(metric.second.attributeKeys.size()));
         for (const auto& key : metric.second.attributeKeys)
            msg->setFieldFromUtf8String(fieldId++, key.first.c_str());
         count++;
      }
   }
   s_catalogLock.unlock();

   msg->setField(VID_NUM_ELEMENTS, count);
}

/**
 * Build the metric catalog for a node as a JSON array of
 * { "name": ..., "type": ..., "attributeKeys": [...] }.
 */
static json_t *MetricsToJson(uint32_t nodeId)
{
   json_t *output = json_array();

   s_catalogLock.lock();
   auto nodeIt = s_catalog.find(nodeId);
   if (nodeIt != s_catalog.end())
   {
      for (const auto& metric : nodeIt->second)
      {
         json_t *entry = json_object();
         json_object_set_new(entry, "name", json_string(metric.first.c_str()));
         json_object_set_new(entry, "type", json_string(MetricTypeName(metric.second.type)));

         json_t *keys = json_array();
         for (const auto& key : metric.second.attributeKeys)
            json_array_append_new(keys, json_string(key.first.c_str()));
         json_object_set_new(entry, "attributeKeys", keys);

         json_array_append_new(output, entry);
      }
   }
   s_catalogLock.unlock();

   return output;
}

/**
 * Client session command handler for CMD_GET_OTLP_METRICS
 */
int H_OtlpClientCommand(uint32_t command, NXCPMessage *request, ClientSession *session)
{
   if (command != CMD_GET_OTLP_METRICS)
      return NXMOD_COMMAND_IGNORED;

   NXCPMessage response(CMD_REQUEST_COMPLETED, request->getId());

   uint32_t objectId = request->getFieldAsUInt32(VID_OBJECT_ID);
   shared_ptr<NetObj> object = FindObjectById(objectId, OBJECT_NODE);
   if (object == nullptr)
   {
      response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }
   else if (!object->checkAccessRights(session->getUserId(), OBJECT_ACCESS_READ))
   {
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
   }
   else
   {
      FillMessageWithMetrics(objectId, &response);
      response.setField(VID_RCC, RCC_SUCCESS);
   }

   session->sendMessage(response);
   return NXMOD_COMMAND_PROCESSED;
}

/**
 * WebAPI handler for GET /v1/objects/{id}/otlp-metrics
 */
int H_ObjectOtlpMetrics(Context *context)
{
   uint32_t objectId = context->getPlaceholderValueAsUInt32(L"object-id");
   if (objectId == 0)
      return 400;

   shared_ptr<NetObj> object = FindObjectById(objectId, OBJECT_NODE);
   if ((object == nullptr) || object->isDeleted())
      return 404;

   if (!object->checkAccessRights(context->getUserId(), OBJECT_ACCESS_READ))
      return 403;

   json_t *output = MetricsToJson(objectId);
   context->setResponseData(output);
   json_decref(output);
   return 200;
}
