/*
** NetXMS - Network Management System
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
** File: metric_export.cpp
**
** OTLP metric export driver. Exports collected DCI values to an external OpenTelemetry
** collector as OTLP metrics over HTTP/protobuf. Registered with the core performance
** data storage framework (see src/server/core/pds.cpp) under driver name "OTLP".
**
** Values are converted and buffered by saveDCItemValue() and delivered asynchronously
** by an internal flush thread in batches (grouped by owner node = OTLP resource),
** keeping the per-value call fast and amortizing HTTP round-trips.
**
** Configuration (server configuration file, all keys optional except Endpoint):
**
**   [OpenTelemetry/MetricExport]
**   Endpoint = https://collector.example.com/v1/metrics
**   AuthToken = secret                 # sent as "Authorization: Bearer <token>"
**   Header = X-Custom: value           # additional HTTP header (can be repeated)
**   VerifyPeer = yes                   # TLS peer verification
**   ServiceName = netxms               # override resource service.name (default: node name)
**   BatchSize = 100
**   FlushInterval = 5                  # seconds
**   Timeout = 30                       # HTTP timeout in seconds
**   QueueSizeLimit = 10000             # 0 to disable limit
**
**/

#include "otlp.h"
#include <pdsdrv.h>
#include <nxlibcurl.h>
#include <netxms-version.h>
#include "generated/metrics_service.pb.h"

#define DEBUG_TAG_METRICS  L"otlp.metrics"

#define DEFAULT_BATCH_SIZE        100
#define DEFAULT_FLUSH_INTERVAL    5      // seconds
#define DEFAULT_TIMEOUT           30     // seconds
#define DEFAULT_QUEUE_SIZE_LIMIT  10000
#define SEND_RETRY_COUNT          2

namespace metrics_v1 = opentelemetry::proto::metrics::v1;

/**
 * OTLP metric shape derived from DCI delta calculation method and data type
 */
enum class OtlpMetricKind
{
   GAUGE = 0,
   SUM_DELTA = 1,
   SUM_CUMULATIVE = 2
};

/**
 * Buffered export record - a converted DCI value awaiting delivery
 */
struct MetricExportRecord
{
   uint32_t nodeId;
   std::string serviceName;   // resource service.name
   std::string hostName;      // resource host.name
   std::string hostGuid;      // resource host.id (object GUID)
   std::string name;
   std::string description;
   std::string unit;
   uint64_t timeUnixNano;
   uint64_t startTimeUnixNano;    // delta-sum interval start (0 = unset)
   OtlpMetricKind kind;
   bool monotonic;
   bool isInt;
   int64_t intValue;
   double doubleValue;
   std::vector<OtlpExportAttribute> attributes;
};

/**
 * OTLP metric export driver
 */
class OtlpMetricExportDriver : public PerfDataStorageDriver
{
private:
   std::string m_url;
   curl_slist *m_headers;
   bool m_verifyPeer;
   std::string m_serviceNameOverride;
   uint32_t m_batchSize;
   uint32_t m_flushInterval;       // seconds
   long m_timeout;                 // seconds
   uint32_t m_queueSizeLimit;

   Mutex m_bufferLock;
   Condition m_wakeup;
   std::vector<MetricExportRecord> m_buffer;
   THREAD m_flushThread;
   bool m_stop;
   bool m_queueOverflow;
   uint64_t m_droppedCount;

   CURL *m_curl;

   bool getAttributesFromObject(const NetObj& object, MetricExportRecord *record);
   bool setupCurlHandle();
   bool sendBatch(const std::vector<MetricExportRecord>& batch);
   void flushThread();

public:
   OtlpMetricExportDriver();
   virtual ~OtlpMetricExportDriver();

   virtual const wchar_t *getName() override;
   virtual bool init(Config *config) override;
   virtual void shutdown() override;
   virtual bool saveDCItemValue(DCItem *dci, Timestamp timestamp, Timestamp startTimestamp, const wchar_t *value) override;
   virtual DataCollectionError getInternalMetric(const wchar_t *metric, wchar_t *value) override;
};

/**
 * Constructor
 */
OtlpMetricExportDriver::OtlpMetricExportDriver() : m_bufferLock(MutexType::FAST), m_wakeup(false)
{
   m_headers = nullptr;
   m_verifyPeer = true;
   m_batchSize = DEFAULT_BATCH_SIZE;
   m_flushInterval = DEFAULT_FLUSH_INTERVAL;
   m_timeout = DEFAULT_TIMEOUT;
   m_queueSizeLimit = DEFAULT_QUEUE_SIZE_LIMIT;
   m_flushThread = INVALID_THREAD_HANDLE;
   m_stop = false;
   m_queueOverflow = false;
   m_droppedCount = 0;
   m_curl = nullptr;
}

/**
 * Destructor - shutdown() is called by the core before deletion
 */
OtlpMetricExportDriver::~OtlpMetricExportDriver()
{
   if (m_curl != nullptr)
      curl_easy_cleanup(m_curl);
   curl_slist_free_all(m_headers);
}

/**
 * Get driver name
 */
const wchar_t *OtlpMetricExportDriver::getName()
{
   return L"OTLP";
}

/**
 * Initialize driver - parse configuration, build HTTP headers, start flush thread
 */
bool OtlpMetricExportDriver::init(Config *config)
{
   const wchar_t *endpoint = config->getValue(L"/OpenTelemetry/MetricExport/Endpoint", L"");
   if (endpoint[0] == 0)
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_METRICS, L"OTLP metric export requires OpenTelemetry/MetricExport/Endpoint in server configuration");
      return false;
   }
   m_url = WideToUtf8(endpoint);

   if (!InitializeLibCURL())
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_METRICS, L"cURL initialization failed");
      return false;
   }

   m_verifyPeer = config->getValueAsBoolean(L"/OpenTelemetry/MetricExport/VerifyPeer", true);
   m_serviceNameOverride = WideToUtf8(config->getValue(L"/OpenTelemetry/MetricExport/ServiceName", L""));

   m_batchSize = config->getValueAsUInt(L"/OpenTelemetry/MetricExport/BatchSize", DEFAULT_BATCH_SIZE);
   if (m_batchSize == 0)
      m_batchSize = DEFAULT_BATCH_SIZE;
   m_flushInterval = config->getValueAsUInt(L"/OpenTelemetry/MetricExport/FlushInterval", DEFAULT_FLUSH_INTERVAL);
   if (m_flushInterval == 0)
      m_flushInterval = DEFAULT_FLUSH_INTERVAL;
   m_timeout = config->getValueAsInt(L"/OpenTelemetry/MetricExport/Timeout", DEFAULT_TIMEOUT);
   if (m_timeout <= 0)
      m_timeout = DEFAULT_TIMEOUT;
   m_queueSizeLimit = config->getValueAsUInt(L"/OpenTelemetry/MetricExport/QueueSizeLimit", DEFAULT_QUEUE_SIZE_LIMIT);

   // Build static HTTP headers
   m_headers = curl_slist_append(nullptr, "Content-Type: application/x-protobuf");

   const wchar_t *authToken = config->getValue(L"/OpenTelemetry/MetricExport/AuthToken", L"");
   if (authToken[0] != 0)
   {
      std::string header("Authorization: Bearer ");
      header.append(WideToUtf8(authToken));
      m_headers = curl_slist_append(m_headers, header.c_str());
   }

   ConfigEntry *customHeaders = config->getEntry(L"/OpenTelemetry/MetricExport/Header");
   if (customHeaders != nullptr)
   {
      for(int i = 0; i < customHeaders->getValueCount(); i++)
         m_headers = curl_slist_append(m_headers, WideToUtf8(customHeaders->getValue(i)).c_str());
   }

   m_flushThread = ThreadCreateEx(this, &OtlpMetricExportDriver::flushThread);

   nxlog_debug_tag(DEBUG_TAG_METRICS, 3, L"OTLP metric export driver initialized (endpoint=%hs, batchSize=%u, flushInterval=%us)",
      m_url.c_str(), m_batchSize, m_flushInterval);
   return true;
}

/**
 * Shutdown driver - stop flush thread (final flush happens there)
 */
void OtlpMetricExportDriver::shutdown()
{
   m_stop = true;
   m_wakeup.set();
   ThreadJoin(m_flushThread);
   nxlog_debug_tag(DEBUG_TAG_METRICS, 1, L"OTLP metric export driver shutdown completed");
}

/**
 * Collect tag:* custom attributes from object; returns true if export ignore flag is set
 */
bool OtlpMetricExportDriver::getAttributesFromObject(const NetObj& object, MetricExportRecord *record)
{
   StringMap *ca = object.getCustomAttributes();
   if (ca == nullptr)
      return false;

   bool ignoreMetric = false;
   StringList keys = ca->keys();
   for(int i = 0; i < keys.size(); i++)
   {
      const wchar_t *key = keys.get(i);
      if (!wcsicmp(key, L"pds:ignore") || !wcsicmp(key, L"otel:ignore"))
      {
         ignoreMetric = true;
         break;
      }

      if (!wcsnicmp(key, L"tag:", 4) || !wcsnicmp(key, L"tag_", 4))
      {
         const wchar_t *value = ca->get(key);
         if ((value != nullptr) && (value[0] != 0))
            record->attributes.emplace_back(WideToUtf8(&key[4]), WideToUtf8(value));
      }
   }

   delete ca;
   return ignoreMetric;
}

/**
 * Convert and enqueue a single DCI value. Returns immediately - actual delivery is asynchronous.
 */
bool OtlpMetricExportDriver::saveDCItemValue(DCItem *dci, Timestamp timestamp, Timestamp startTimestamp, const wchar_t *value)
{
   if (*value == 0)
      return true;

   MetricExportRecord rec;
   rec.startTimeUnixNano = 0;

   // Value - OTLP number data points are either int64 or double; string DCIs cannot be represented
   wchar_t *eptr;
   switch(dci->getTransformedDataType())
   {
      case DCI_DT_INT:
      case DCI_DT_INT64:
         errno = 0;
         rec.intValue = wcstoll(value, &eptr, 0);
         rec.isInt = true;
         break;
      case DCI_DT_UINT:
      case DCI_DT_UINT64:
      case DCI_DT_COUNTER32:
      case DCI_DT_COUNTER64:
         errno = 0;
         rec.intValue = static_cast<int64_t>(wcstoull(value, &eptr, 0));
         rec.isInt = true;
         break;
      case DCI_DT_FLOAT:
         errno = 0;
         rec.doubleValue = wcstod(value, &eptr);
         rec.isInt = false;
         break;
      default:    // string or null
         nxlog_debug_tag(DEBUG_TAG_METRICS, 7, L"Metric %s [%u] not sent: data type cannot be represented as OTLP number", dci->getName().cstr(), dci->getId());
         return true;
   }
   if ((*eptr != 0) || (errno == ERANGE))
   {
      nxlog_debug_tag(DEBUG_TAG_METRICS, 7, L"Metric %s [%u] not sent: value \"%s\" did not pass number validation", dci->getName().cstr(), dci->getId(), value);
      return true;
   }

   // Gauge vs Sum - mirror image of the ingest-side gauge/monotonic-sum split (see receiver.cpp).
   // Raw counters are cumulative monotonic sums, simple deltas are delta-temporality sums,
   // everything else (original plain values, average-per-second/minute rates) is a gauge.
   bool isCounterType = (dci->getTransformedDataType() == DCI_DT_COUNTER32) || (dci->getTransformedDataType() == DCI_DT_COUNTER64);
   switch(dci->getDeltaCalculationMethod())
   {
      case DCM_ORIGINAL_VALUE:
         rec.kind = isCounterType ? OtlpMetricKind::SUM_CUMULATIVE : OtlpMetricKind::GAUGE;
         rec.monotonic = isCounterType;
         break;
      case DCM_SIMPLE:
         rec.kind = OtlpMetricKind::SUM_DELTA;
         rec.monotonic = isCounterType;
         // Delta sums carry an interval: start = previous collection timestamp, end = this timestamp.
         // Left unset (0) for the first sample, when there is no previous value yet.
         if (!startTimestamp.isNull())
            rec.startTimeUnixNano = static_cast<uint64_t>(startTimestamp.asMilliseconds()) * 1000000ULL;
         break;
      default:    // average per second / per minute - already a rate
         rec.kind = OtlpMetricKind::GAUGE;
         rec.monotonic = false;
         break;
   }

   // Collect tag:* attributes and check export ignore flags on template, owner, and related object
   uint32_t templateId = dci->getTemplateId();
   if (templateId == dci->getOwnerId())
   {
      // Created by instance discovery, try to get parent template
      shared_ptr<DCObject> instanceDiscoveryDCI = dci->getOwner()->getDCObjectById(dci->getTemplateItemId(), 0);
      templateId = (instanceDiscoveryDCI != nullptr) ? instanceDiscoveryDCI->getTemplateId() : 0;
   }
   if (templateId != 0)
   {
      shared_ptr<NetObj> templateObject = FindObjectById(templateId, OBJECT_TEMPLATE);
      if ((templateObject != nullptr) && getAttributesFromObject(*templateObject, &rec))
      {
         nxlog_debug_tag(DEBUG_TAG_METRICS, 7, L"Metric %s [%u] not sent: ignore flag set on template object %s", dci->getName().cstr(), dci->getId(), templateObject->getName());
         return true;
      }
   }

   shared_ptr<DataCollectionOwner> owner = dci->getOwner();
   if (getAttributesFromObject(*owner, &rec))
   {
      nxlog_debug_tag(DEBUG_TAG_METRICS, 7, L"Metric %s [%u] not sent: ignore flag set on owner object", dci->getName().cstr(), dci->getId());
      return true;
   }

   shared_ptr<NetObj> relatedObject = FindObjectById(dci->getRelatedObject());
   if (relatedObject != nullptr)
   {
      if (getAttributesFromObject(*relatedObject, &rec))
      {
         nxlog_debug_tag(DEBUG_TAG_METRICS, 7, L"Metric %s [%u] not sent: ignore flag set on related object %s", dci->getName().cstr(), dci->getId(), relatedObject->getName());
         return true;
      }
      rec.attributes.emplace_back(std::string("netxms.related_object"), WideToUtf8(relatedObject->getName()));
   }

   // Metric name - for sources where the native name is an address rather than a meaningful
   // name (SNMP OID, Modbus register) use DCI description instead (same convention as ClickHouse driver)
   if ((dci->getDataSource() == DS_SNMP_AGENT) || (dci->getDataSource() == DS_MODBUS) ||
       ((dci->getDataSource() == DS_INTERNAL) && !wcsnicmp(dci->getName(), L"Dummy", 5)))
   {
      rec.name = WideToUtf8(dci->getDescription());
   }
   else
   {
      rec.name = WideToUtf8(dci->getName());
      std::string description = WideToUtf8(dci->getDescription());
      if (description != rec.name)
         rec.description = std::move(description);
   }

   rec.unit = WideToUtf8(dci->getUnitName());
   rec.timeUnixNano = static_cast<uint64_t>(timestamp.asMilliseconds()) * 1000000ULL;

   rec.nodeId = owner->getId();
   rec.hostName = WideToUtf8(owner->getName());
   char guidText[64];
   rec.hostGuid = owner->getGuid().toStringA(guidText);
   rec.serviceName = !m_serviceNameOverride.empty() ? m_serviceNameOverride : rec.hostName;

   rec.attributes.emplace_back("netxms.dci.id", static_cast<int64_t>(dci->getId()));
   const wchar_t *dataSource = dci->getDataProviderName();
   rec.attributes.emplace_back(std::string("netxms.data_source"), WideToUtf8(dataSource));
   SharedString instance = dci->getInstanceName();
   if (!instance.isEmpty())
      rec.attributes.emplace_back(std::string("netxms.instance"), WideToUtf8(instance.cstr()));

   bool reachedBatch = false;
   m_bufferLock.lock();
   if ((m_queueSizeLimit > 0) && (m_buffer.size() >= m_queueSizeLimit))
   {
      m_droppedCount++;
      if (!m_queueOverflow)
      {
         m_queueOverflow = true;
         nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG_METRICS,
            L"OTLP metric export buffer is full (limit=%u), new values will be dropped", m_queueSizeLimit);
      }
      m_bufferLock.unlock();
      return true;
   }
   if (m_queueOverflow)
   {
      m_queueOverflow = false;
      nxlog_debug_tag(DEBUG_TAG_METRICS, 4, L"OTLP metric export buffer is below limit");
   }
   m_buffer.push_back(std::move(rec));
   reachedBatch = (m_buffer.size() >= m_batchSize);
   m_bufferLock.unlock();

   if (reachedBatch)
      m_wakeup.set();
   return true;
}

/**
 * Initialize cURL handle
 */
bool OtlpMetricExportDriver::setupCurlHandle()
{
   if (m_curl != nullptr)
      curl_easy_cleanup(m_curl);

   m_curl = curl_easy_init();
   if (m_curl == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_METRICS, 4, L"Call to curl_easy_init() failed");
      return false;
   }

#if HAVE_DECL_CURLOPT_NOSIGNAL
   curl_easy_setopt(m_curl, CURLOPT_NOSIGNAL, 1L);
#endif
   curl_easy_setopt(m_curl, CURLOPT_POST, 1L);
   curl_easy_setopt(m_curl, CURLOPT_HEADER, 0L);
   curl_easy_setopt(m_curl, CURLOPT_TIMEOUT, m_timeout);
   curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, ByteStream::curlWriteFunction);
   curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, m_verifyPeer ? 1L : 0L);
   curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYHOST, m_verifyPeer ? 2L : 0L);
   curl_easy_setopt(m_curl, CURLOPT_USERAGENT, "NetXMS OTLP Exporter/" NETXMS_VERSION_STRING_A);
   curl_easy_setopt(m_curl, CURLOPT_URL, m_url.c_str());
   curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, m_headers);

   EnableLibCURLUnexpectedEOFWorkaround(m_curl);
   return true;
}

/**
 * Serialize a batch into an OTLP ExportMetricsServiceRequest and POST it. Records are grouped
 * by owner node into separate resource entries, and by metric identity within each resource.
 */
bool OtlpMetricExportDriver::sendBatch(const std::vector<MetricExportRecord>& batch)
{
   opentelemetry::proto::collector::metrics::v1::ExportMetricsServiceRequest request;

   std::unordered_map<uint32_t, metrics_v1::ScopeMetrics*> scopeByNode;
   std::unordered_map<std::string, metrics_v1::Metric*> metricByKey;
   for(const MetricExportRecord& rec : batch)
   {
      metrics_v1::ScopeMetrics *scopeMetrics;
      auto si = scopeByNode.find(rec.nodeId);
      if (si == scopeByNode.end())
      {
         metrics_v1::ResourceMetrics *resourceMetrics = request.add_resource_metrics();
         auto resource = resourceMetrics->mutable_resource();
         AddStringAttribute(resource, "service.name", rec.serviceName);
         AddStringAttribute(resource, "host.name", rec.hostName);
         AddStringAttribute(resource, "host.id", rec.hostGuid);
         scopeMetrics = resourceMetrics->add_scope_metrics();
         scopeMetrics->mutable_scope()->set_name("netxms");
         scopeByNode[rec.nodeId] = scopeMetrics;
      }
      else
      {
         scopeMetrics = si->second;
      }

      // Merge data points of the same metric within a resource into one metric entry
      char nodeKey[16];
      std::string metricKey(IntegerToString(rec.nodeId, nodeKey));
      metricKey.append(1, '\x1f');
      metricKey.append(rec.name);
      metricKey.append(1, '\x1f');
      metricKey.append(1, static_cast<char>('0' + static_cast<int>(rec.kind) * 2 + (rec.monotonic ? 1 : 0)));
      metricKey.append(rec.unit);

      metrics_v1::Metric *metric;
      auto mi = metricByKey.find(metricKey);
      if (mi == metricByKey.end())
      {
         metric = scopeMetrics->add_metrics();
         metric->set_name(rec.name);
         if (!rec.description.empty())
            metric->set_description(rec.description);
         if (!rec.unit.empty())
            metric->set_unit(rec.unit);
         if (rec.kind != OtlpMetricKind::GAUGE)
         {
            metrics_v1::Sum *sum = metric->mutable_sum();
            sum->set_is_monotonic(rec.monotonic);
            sum->set_aggregation_temporality((rec.kind == OtlpMetricKind::SUM_CUMULATIVE) ?
               metrics_v1::AGGREGATION_TEMPORALITY_CUMULATIVE : metrics_v1::AGGREGATION_TEMPORALITY_DELTA);
         }
         metricByKey[metricKey] = metric;
      }
      else
      {
         metric = mi->second;
      }

      metrics_v1::NumberDataPoint *dp = (rec.kind == OtlpMetricKind::GAUGE) ?
         metric->mutable_gauge()->add_data_points() : metric->mutable_sum()->add_data_points();
      dp->set_time_unix_nano(rec.timeUnixNano);
      if (rec.startTimeUnixNano != 0)
         dp->set_start_time_unix_nano(rec.startTimeUnixNano);
      if (rec.isInt)
         dp->set_as_int(rec.intValue);
      else
         dp->set_as_double(rec.doubleValue);
      for(const OtlpExportAttribute& attr : rec.attributes)
      {
         auto kv = dp->add_attributes();
         kv->set_key(attr.key);
         if (attr.isInt)
            kv->mutable_value()->set_int_value(attr.intValue);
         else
            kv->mutable_value()->set_string_value(attr.stringValue);
      }
   }

   std::string payload;
   if (!request.SerializeToString(&payload))
   {
      nxlog_debug_tag(DEBUG_TAG_METRICS, 4, L"Failed to serialize OTLP ExportMetricsServiceRequest");
      return false;
   }

   for(int attempt = 0; attempt <= SEND_RETRY_COUNT; attempt++)
   {
      if ((m_curl == nullptr) && !setupCurlHandle())
         return false;

      curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, payload.data());
      curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(payload.size()));

      ByteStream responseData(4096);
      curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &responseData);

      char errorText[CURL_ERROR_SIZE];
      curl_easy_setopt(m_curl, CURLOPT_ERRORBUFFER, errorText);

      nxlog_debug_tag(DEBUG_TAG_METRICS, 7, L"Sending OTLP metrics batch (%u data points, %u bytes) to %hs",
         static_cast<uint32_t>(batch.size()), static_cast<uint32_t>(payload.size()), m_url.c_str());

      if (curl_easy_perform(m_curl) == CURLE_OK)
      {
         long responseCode;
         curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &responseCode);
         nxlog_debug_tag(DEBUG_TAG_METRICS, 7, L"OTLP collector HTTP response %03ld, %d bytes data",
            responseCode, static_cast<int>(responseData.size()));
         if ((responseCode >= 200) && (responseCode < 300))
         {
            if (responseData.size() > 0)
            {
               opentelemetry::proto::collector::metrics::v1::ExportMetricsServiceResponse response;
               if (response.ParseFromArray(responseData.buffer(), static_cast<int>(responseData.size())) &&
                   response.has_partial_success() && (response.partial_success().rejected_data_points() > 0))
               {
                  nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG_METRICS, L"OTLP collector rejected %d data points (%hs)",
                     static_cast<int>(response.partial_success().rejected_data_points()),
                     response.partial_success().error_message().c_str());
               }
            }
            return true;
         }

         nxlog_debug_tag(DEBUG_TAG_METRICS, 4, L"OTLP collector returned HTTP %ld", responseCode);
         curl_easy_cleanup(m_curl);
         m_curl = nullptr;
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_METRICS, 5, L"Call to curl_easy_perform() failed (%hs)", errorText);
         curl_easy_cleanup(m_curl);
         m_curl = nullptr;
      }
   }

   return false;
}

/**
 * Flush worker thread - delivers buffered records in batches
 */
void OtlpMetricExportDriver::flushThread()
{
   nxlog_debug_tag(DEBUG_TAG_METRICS, 2, L"OTLP metric export flush thread started");
   while(!m_stop)
   {
      m_wakeup.wait(m_flushInterval * 1000);
      m_wakeup.reset();

      std::vector<MetricExportRecord> batch;
      m_bufferLock.lock();
      if (!m_buffer.empty())
         batch.swap(m_buffer);
      m_bufferLock.unlock();

      if (batch.empty())
         continue;

      if (sendBatch(batch))
      {
         nxlog_debug_tag(DEBUG_TAG_METRICS, 6, L"OTLP metrics batch of %u data points delivered", static_cast<uint32_t>(batch.size()));
      }
      else
      {
         m_bufferLock.lock();
         m_droppedCount += batch.size();
         uint64_t totalDropped = m_droppedCount;
         m_bufferLock.unlock();
         nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG_METRICS,
            L"Failed to deliver OTLP metrics batch (%u data points dropped, total dropped=" UINT64_FMT L")",
            static_cast<uint32_t>(batch.size()), totalDropped);
      }
   }

   // Final flush on shutdown
   std::vector<MetricExportRecord> batch;
   m_bufferLock.lock();
   batch.swap(m_buffer);
   m_bufferLock.unlock();
   if (!batch.empty())
      sendBatch(batch);

   nxlog_debug_tag(DEBUG_TAG_METRICS, 2, L"OTLP metric export flush thread stopped");
}

/**
 * Get driver's internal metrics
 */
DataCollectionError OtlpMetricExportDriver::getInternalMetric(const wchar_t *metric, wchar_t *value)
{
   DataCollectionError rc = DCE_SUCCESS;
   if (!wcsicmp(metric, L"queueSize"))
   {
      m_bufferLock.lock();
      ret_uint(value, static_cast<uint32_t>(m_buffer.size()));
      m_bufferLock.unlock();
   }
   else if (!wcsicmp(metric, L"messageDrops"))
   {
      m_bufferLock.lock();
      ret_uint64(value, m_droppedCount);
      m_bufferLock.unlock();
   }
   else
   {
      value[0] = 0;
      rc = DCE_NOT_SUPPORTED;
   }
   return rc;
}

/**
 * Register OTLP metric export driver with the core performance data storage framework
 * (no-op if export is not configured)
 */
void RegisterOtlpMetricExporter(Config *config)
{
   const wchar_t *endpoint = config->getValue(L"/OpenTelemetry/MetricExport/Endpoint", L"");
   if (endpoint[0] == 0)
   {
      nxlog_debug_tag(DEBUG_TAG_METRICS, 2, L"OTLP metric export is not configured");
      return;
   }
   RegisterPerfDataStorageDriver(new OtlpMetricExportDriver());
}
