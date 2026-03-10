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
** File: receiver.cpp
**
**/

#include "otlp.h"
#include "generated/metrics_service.pb.h"
#include <uthash.h>

/**
 * Counter state for monotonic sum rate computation
 */
struct CounterState
{
   UT_hash_handle hh;
   double prevValue;
   uint64_t prevTimeNano;
   char key[1]; // variable-length key: "nodeId:metricName:dciId"
};

static CounterState *s_counterState = nullptr;
static Mutex s_counterStateLock;

/**
 * Extract resource attributes into a map
 */
static void ExtractResourceAttributes(const opentelemetry::proto::resource::v1::Resource& resource, std::map<std::string, std::string>& attributes)
{
   for (int i = 0; i < resource.attributes_size(); i++)
   {
      const auto& kv = resource.attributes(i);
      const auto& av = kv.value();
      char buffer[32];
      switch (av.value_case())
      {
         case opentelemetry::proto::common::v1::AnyValue::kStringValue:
            attributes.emplace(kv.key(), av.string_value());
            break;
         case opentelemetry::proto::common::v1::AnyValue::kIntValue:
            IntegerToString(av.int_value(), buffer);
            attributes.emplace(kv.key(), buffer);
            break;
         case opentelemetry::proto::common::v1::AnyValue::kDoubleValue:
            snprintf(buffer, 32, "%f", av.double_value());
            attributes.emplace(kv.key(), buffer);
            break;
         case opentelemetry::proto::common::v1::AnyValue::kBoolValue:
            attributes.emplace(kv.key(), av.bool_value() ? "true" : "false");
            break;
         default:
            attributes.emplace(kv.key(), "");
            break;
      }
   }
}

/**
 * Extract data point attributes into a map.
 * Works with any protobuf message type that has attributes() returning KeyValue repeated field.
 */
template<typename T>
static void ExtractDataPointAttributes(const T& dp, std::map<std::string, std::string>& attributes)
{
   for (int i = 0; i < dp.attributes_size(); i++)
   {
      const auto& kv = dp.attributes(i);
      const auto& av = kv.value();
      char buffer[32];
      switch (av.value_case())
      {
         case opentelemetry::proto::common::v1::AnyValue::kStringValue:
            attributes.emplace(kv.key(), av.string_value());
            break;
         case opentelemetry::proto::common::v1::AnyValue::kIntValue:
            IntegerToString(av.int_value(), buffer);
            attributes.emplace(kv.key(), buffer);
            break;
         case opentelemetry::proto::common::v1::AnyValue::kDoubleValue:
            snprintf(buffer, 32, "%f", av.double_value());
            attributes.emplace(kv.key(), buffer);
            break;
         case opentelemetry::proto::common::v1::AnyValue::kBoolValue:
            attributes.emplace(kv.key(), av.bool_value() ? "true" : "false");
            break;
         default:
            attributes.emplace(kv.key(), "");
            break;
      }
   }
}

/**
 * Get numeric value from a NumberDataPoint as double
 */
static double GetDataPointValue(const opentelemetry::proto::metrics::v1::NumberDataPoint& dp)
{
   switch (dp.value_case())
   {
      case opentelemetry::proto::metrics::v1::NumberDataPoint::kAsDouble:
         return dp.as_double();
      case opentelemetry::proto::metrics::v1::NumberDataPoint::kAsInt:
         return static_cast<double>(dp.as_int());
      default:
         return 0.0;
   }
}

/**
 * Find a matching DCI for an OTLP data point.
 * First tries exact name match with DS_OTLP source, then falls back to instance discovery.
 */
static shared_ptr<DCObject> FindOTLPDci(const shared_ptr<Node>& node, const char *metricName, const std::map<std::string, std::string>& attributes)
{
   wchar_t wname[256];
   utf8_to_wchar(metricName, -1, wname, 256);

   // Try exact name match first (non-instance DCIs with DS_OTLP)
   shared_ptr<DCObject> dco = node->getDCObjectByName(wname, 0);
   if (dco != nullptr && dco->getType() == DCO_TYPE_ITEM && dco->getDataSource() == DS_OTLP && dco->getStatus() == ITEM_STATUS_ACTIVE)
      return dco;

   // Try instance discovery
   return ResolveOTelInstance(node, metricName, attributes);
}

/**
 * Push a value to a node for a given metric, with pre-extracted attributes.
 */
static void PushValueToNode(const shared_ptr<Node>& node, const char *metricName, double value, uint64_t timeNano, const std::map<std::string, std::string>& attributes)
{
   shared_ptr<DCObject> dco = FindOTLPDci(node, metricName, attributes);
   if (dco != nullptr)
   {
      wchar_t valueStr[64];
      _sntprintf(valueStr, 64, _T("%f"), value);

      Timestamp timestamp = Timestamp::fromNanoseconds(timeNano);
      node->processNewDCValue(dco, timestamp, valueStr, shared_ptr<Table>(), true);
      nxlog_debug_tag(DEBUG_TAG_OTLP, 7, L"Pushed value %s for metric %hs to DCI %u on node %s [%u]",
         valueStr, metricName, dco->getId(), node->getName(), node->getId());
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG_OTLP, 7, L"No matching DCI for metric %hs on node %s [%u]",
         metricName, node->getName(), node->getId());
   }
}

/**
 * Build a counter state key for monotonic sum tracking.
 * Format: "nodeId:metricName:dciId". Returns key length (excluding null terminator).
 */
static int BuildCounterStateKey(uint32_t nodeId, const char *metricName, uint32_t dciId, char *buffer, size_t bufferSize)
{
   return snprintf(buffer, bufferSize, "%u:%s:%u", nodeId, metricName, dciId);
}

/**
 * Shutdown counter state tracking
 */
void ShutdownCounterState()
{
   s_counterStateLock.lock();
   CounterState *entry, *tmp;
   HASH_ITER(hh, s_counterState, entry, tmp)
   {
      HASH_DEL(s_counterState, entry);
      MemFree(entry);
   }
   s_counterStateLock.unlock();
}

/**
 * Process a single NumberDataPoint for a gauge or non-monotonic sum
 */
static void ProcessDirectValue(const shared_ptr<Node>& node, const char *metricName, const opentelemetry::proto::metrics::v1::NumberDataPoint& dp)
{
   std::map<std::string, std::string> attributes;
   ExtractDataPointAttributes(dp, attributes);

   if (nxlog_get_debug_level_tag(DEBUG_TAG_OTLP) >= 8)
   {
      nxlog_debug_tag(DEBUG_TAG_OTLP, 8, L"Data point attributes for metric %hs:", metricName);
      for (const auto& kv : attributes)
      {
         nxlog_debug_tag(DEBUG_TAG_OTLP, 8, L"   %hs = %hs", kv.first.c_str(), kv.second.c_str());
      }
   }

   PushValueToNode(node, metricName, GetDataPointValue(dp), dp.time_unix_nano(), attributes);
}

/**
 * Process a single NumberDataPoint for a monotonic sum (counter -> rate)
 */
static void ProcessMonotonicValue(const shared_ptr<Node>& node, const char *metricName, const opentelemetry::proto::metrics::v1::NumberDataPoint& dp)
{
   std::map<std::string, std::string> attributes;
   ExtractDataPointAttributes(dp, attributes);

   shared_ptr<DCObject> dco = FindOTLPDci(node, metricName, attributes);
   if (dco == nullptr)
      return;

   double currentValue = GetDataPointValue(dp);
   uint64_t currentTimeNano = dp.time_unix_nano();

   char stateKey[512];
   int keyLen = BuildCounterStateKey(node->getId(), metricName, dco->getId(), stateKey, sizeof(stateKey));

   s_counterStateLock.lock();
   CounterState *state = nullptr;
   HASH_FIND(hh, s_counterState, stateKey, keyLen, state);
   if (state != nullptr)
   {
      double timeDelta = static_cast<double>(currentTimeNano - state->prevTimeNano) / 1e9;
      if (timeDelta > 0)
      {
         double valueDelta = currentValue - state->prevValue;
         if (valueDelta < 0)
            valueDelta = currentValue;  // Counter reset
         double rate = valueDelta / timeDelta;

         wchar_t valueStr[64];
         _sntprintf(valueStr, 64, _T("%f"), rate);

         Timestamp timestamp = Timestamp::fromNanoseconds(currentTimeNano);
         node->processNewDCValue(dco, timestamp, valueStr, shared_ptr<Table>(), true);
         nxlog_debug_tag(DEBUG_TAG_OTLP, 7, L"Pushed rate %s for counter %hs to DCI %u on node %s [%u]",
            valueStr, metricName, dco->getId(), node->getName(), node->getId());
      }
      state->prevValue = currentValue;
      state->prevTimeNano = currentTimeNano;
   }
   else
   {
      // First data point - record state, no rate to compute yet
      state = static_cast<CounterState*>(MemAlloc(offsetof(CounterState, key) + keyLen + 1));
      state->prevValue = currentValue;
      state->prevTimeNano = currentTimeNano;
      memcpy(state->key, stateKey, keyLen + 1);
      HASH_ADD_KEYPTR(hh, s_counterState, state->key, keyLen, state);
      nxlog_debug_tag(DEBUG_TAG_OTLP, 7, L"Initialized counter state for %hs on node %s [%u]", metricName, node->getName(), node->getId());
   }
   s_counterStateLock.unlock();
}

/**
 * Format a double as a clean string (no trailing zeros).
 * E.g., 5.0 -> "5", 0.5 -> "0.5", 100.0 -> "100"
 */
static void FormatBucketBound(double value, char *buffer, size_t bufferSize)
{
   snprintf(buffer, bufferSize, "%g", value);
}

/**
 * Process a single HistogramDataPoint by decomposing it into individual values
 * with synthetic otel.bucket attribute
 */
static void ProcessHistogramDataPoint(const shared_ptr<Node>& node, const char *metricName, const opentelemetry::proto::metrics::v1::HistogramDataPoint& dp)
{
   std::map<std::string, std::string> baseAttributes;
   ExtractDataPointAttributes(dp, baseAttributes);

   uint64_t timeNano = dp.time_unix_nano();

   if (nxlog_get_debug_level_tag(DEBUG_TAG_OTLP) >= 8)
   {
      nxlog_debug_tag(DEBUG_TAG_OTLP, 8, L"Histogram data point attributes for metric %hs:", metricName);
      for (const auto& kv : baseAttributes)
      {
         nxlog_debug_tag(DEBUG_TAG_OTLP, 8, L"   %hs = %hs", kv.first.c_str(), kv.second.c_str());
      }
      nxlog_debug_tag(DEBUG_TAG_OTLP, 8, L"Histogram %hs: %d bounds, %d buckets, count=%llu",
         metricName, dp.explicit_bounds_size(), dp.bucket_counts_size(), dp.count());
   }

   // Push bucket counts
   for (int i = 0; i < dp.bucket_counts_size(); i++)
   {
      std::map<std::string, std::string> attributes(baseAttributes);
      char boundStr[64];
      if (i < dp.explicit_bounds_size())
      {
         char numBuf[32];
         FormatBucketBound(dp.explicit_bounds(i), numBuf, sizeof(numBuf));
         snprintf(boundStr, sizeof(boundStr), "le%s", numBuf);
      }
      else
      {
         strcpy(boundStr, "inf");
      }
      attributes["otel.bucket"] = boundStr;
      PushValueToNode(node, metricName, static_cast<double>(dp.bucket_counts(i)), timeNano, attributes);
   }

   // Push aggregate statistics
   {
      std::map<std::string, std::string> attributes(baseAttributes);
      attributes["otel.bucket"] = "count";
      PushValueToNode(node, metricName, static_cast<double>(dp.count()), timeNano, attributes);
   }

   if (dp.has_sum())
   {
      std::map<std::string, std::string> attributes(baseAttributes);
      attributes["otel.bucket"] = "sum";
      PushValueToNode(node, metricName, dp.sum(), timeNano, attributes);
   }

   if (dp.has_min())
   {
      std::map<std::string, std::string> attributes(baseAttributes);
      attributes["otel.bucket"] = "min";
      PushValueToNode(node, metricName, dp.min(), timeNano, attributes);
   }

   if (dp.has_max())
   {
      std::map<std::string, std::string> attributes(baseAttributes);
      attributes["otel.bucket"] = "max";
      PushValueToNode(node, metricName, dp.max(), timeNano, attributes);
   }
}

/**
 * Process metrics for a matched node
 */
static void ProcessMetricsForNode(const shared_ptr<Node>& node, const opentelemetry::proto::metrics::v1::ScopeMetrics& scopeMetrics)
{
   for (int m = 0; m < scopeMetrics.metrics_size(); m++)
   {
      const auto& metric = scopeMetrics.metrics(m);
      const char *metricName = metric.name().c_str();

      switch (metric.data_case())
      {
         case opentelemetry::proto::metrics::v1::Metric::kGauge:
            for (int d = 0; d < metric.gauge().data_points_size(); d++)
            {
               ProcessDirectValue(node, metricName, metric.gauge().data_points(d));
            }
            break;

         case opentelemetry::proto::metrics::v1::Metric::kSum:
            if (metric.sum().is_monotonic())
            {
               for (int d = 0; d < metric.sum().data_points_size(); d++)
               {
                  ProcessMonotonicValue(node, metricName, metric.sum().data_points(d));
               }
            }
            else
            {
               for (int d = 0; d < metric.sum().data_points_size(); d++)
               {
                  ProcessDirectValue(node, metricName, metric.sum().data_points(d));
               }
            }
            break;

         case opentelemetry::proto::metrics::v1::Metric::kHistogram:
            for (int d = 0; d < metric.histogram().data_points_size(); d++)
            {
               ProcessHistogramDataPoint(node, metricName, metric.histogram().data_points(d));
            }
            break;

         default:
            nxlog_debug_tag(DEBUG_TAG_OTLP, 6, L"Unsupported metric type %d for %hs (skipped)", metric.data_case(), metricName);
            break;
      }
   }
}

/**
 * Handler for OTLP metrics endpoint (POST /otlp-backend/metrics)
 */
int H_OtlpMetrics(Context *context)
{
   // Parse protobuf request
   opentelemetry::proto::collector::metrics::v1::ExportMetricsServiceRequest request;
   if (!context->hasRequestData() ||
       !request.ParseFromArray(context->getRequestData(), static_cast<int>(context->getRequestDataSize())))
   {
      nxlog_debug_tag(DEBUG_TAG_OTLP, 4, L"Failed to parse OTLP ExportMetricsServiceRequest");
      context->setErrorResponse("Invalid protobuf payload");
      return 400;
   }

   nxlog_debug_tag(DEBUG_TAG_OTLP, 6, L"Received OTLP metrics batch with %d resource metrics", request.resource_metrics_size());

   int matchedCount = 0;
   int unmatchedCount = 0;

   for (int r = 0; r < request.resource_metrics_size(); r++)
   {
      const auto& resourceMetrics = request.resource_metrics(r);

      // Extract resource attributes
      std::map<std::string, std::string> attributes;
      if (resourceMetrics.has_resource())
      {
         ExtractResourceAttributes(resourceMetrics.resource(), attributes);
      }

      // Match to a node
      shared_ptr<Node> node = MatchResourceToNode(attributes);
      if (node == nullptr)
      {
         unmatchedCount++;
         continue;
      }
      matchedCount++;

      // Process all scope metrics for this resource
      for (int s = 0; s < resourceMetrics.scope_metrics_size(); s++)
      {
         ProcessMetricsForNode(node, resourceMetrics.scope_metrics(s));
      }
   }

   nxlog_debug_tag(DEBUG_TAG_OTLP, 5, L"OTLP batch processed: %d matched, %d unmatched", matchedCount, unmatchedCount);

   // Return OTLP response (protobuf)
   opentelemetry::proto::collector::metrics::v1::ExportMetricsServiceResponse response;
   std::string serialized;
   response.SerializeToString(&serialized);
   context->setResponseData(serialized.data(), serialized.size(), "application/x-protobuf");
   return 200;
}
