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
** File: event_forwarder.cpp
**
** OTLP event forwarder driver. Exports NetXMS events to an external OpenTelemetry
** collector as OTLP log records over HTTP/protobuf. Registered with the core event
** forwarding framework (see src/server/core/event_forwarder.cpp) under driver name "otlp".
**
** Events are converted and buffered by forward() and delivered asynchronously by an
** internal flush thread in batches (grouped by source node = OTLP resource), keeping the
** per-event call fast and amortizing HTTP round-trips.
**
**/

#include "otlp.h"
#include <efdrv.h>
#include <nxlibcurl.h>
#include <netxms-version.h>
#include "generated/logs_service.pb.h"

#define DEBUG_TAG_EXPORT  L"otlp.export"

#define DEFAULT_BATCH_SIZE        100
#define DEFAULT_FLUSH_INTERVAL    5      // seconds
#define DEFAULT_TIMEOUT           30     // seconds
#define DEFAULT_QUEUE_SIZE_LIMIT  10000
#define SEND_RETRY_COUNT          2

namespace logs_v1 = opentelemetry::proto::logs::v1;

/**
 * Map NetXMS event severity to OTLP severity number.
 * Inverse of the ingest-side band mapping (see logs.cpp:MapOtelSeverity).
 */
static int MapSeverityToOtel(uint32_t severity)
{
   switch(severity)
   {
      case SEVERITY_NORMAL:
         return logs_v1::SEVERITY_NUMBER_INFO;       // 9
      case SEVERITY_WARNING:
         return logs_v1::SEVERITY_NUMBER_WARN;       // 13
      case SEVERITY_MINOR:
         return logs_v1::SEVERITY_NUMBER_WARN4;      // 16
      case SEVERITY_MAJOR:
         return logs_v1::SEVERITY_NUMBER_ERROR;      // 17
      case SEVERITY_CRITICAL:
         return logs_v1::SEVERITY_NUMBER_FATAL;      // 21
      default:
         return logs_v1::SEVERITY_NUMBER_INFO;
   }
}

/**
 * Severity number to text (used as OTLP severity_text)
 */
static const char *SeverityText(uint32_t severity)
{
   static const char *texts[] = { "NORMAL", "WARNING", "MINOR", "MAJOR", "CRITICAL" };
   return (severity < sizeof(texts) / sizeof(texts[0])) ? texts[severity] : "UNKNOWN";
}

/**
 * Buffered export record - a converted event awaiting delivery
 */
struct OtlpExportRecord
{
   uint32_t nodeId;
   std::string serviceName;   // resource service.name
   std::string hostName;      // resource host.name
   std::string hostGuid;      // resource host.id (object GUID)
   std::string recipient;     // resource netxms.recipient (driver-specific, optional)
   uint64_t timeUnixNano;
   uint64_t observedTimeUnixNano;
   int severityNumber;
   std::string severityText;
   std::string body;
   std::vector<OtlpExportAttribute> attributes;
};

/**
 * OTLP event forwarder driver
 */
class OtlpEventForwarderDriver : public EventForwarderDriver
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
   std::vector<OtlpExportRecord> m_buffer;
   THREAD m_flushThread;
   bool m_stop;
   bool m_healthStatus;
   bool m_queueOverflow;
   uint32_t m_droppedCount;

   CURL *m_curl;

   void buildRecord(OtlpExportRecord& rec, const Event& event, const shared_ptr<NetObj>& source);
   bool setupCurlHandle();
   bool sendBatch(const std::vector<OtlpExportRecord>& batch);
   void flushThread();

public:
   OtlpEventForwarderDriver(json_t *configuration);
   virtual ~OtlpEventForwarderDriver();

   virtual bool forward(const Event& event, const TCHAR *recipient, const shared_ptr<NetObj>& source) override;
   virtual bool checkHealth() override { return m_healthStatus; }
};

/**
 * Constructor - parse configuration, build HTTP headers, start flush thread
 */
OtlpEventForwarderDriver::OtlpEventForwarderDriver(json_t *configuration) :
         m_bufferLock(MutexType::FAST), m_wakeup(false)
{
   char *endpoint = json_object_get_string_a(configuration, "endpoint", "");
   m_url = endpoint;
   MemFree(endpoint);

   m_verifyPeer = json_object_get_boolean(configuration, "verifyPeer", true);

   char *serviceName = json_object_get_string_a(configuration, "serviceName", "");
   m_serviceNameOverride = serviceName;
   MemFree(serviceName);

   m_batchSize = json_object_get_uint32(configuration, "batchSize", DEFAULT_BATCH_SIZE);
   if (m_batchSize == 0)
      m_batchSize = DEFAULT_BATCH_SIZE;
   m_flushInterval = json_object_get_uint32(configuration, "flushInterval", DEFAULT_FLUSH_INTERVAL);
   if (m_flushInterval == 0)
      m_flushInterval = DEFAULT_FLUSH_INTERVAL;
   m_timeout = json_object_get_int32(configuration, "timeout", DEFAULT_TIMEOUT);
   if (m_timeout <= 0)
      m_timeout = DEFAULT_TIMEOUT;
   m_queueSizeLimit = json_object_get_uint32(configuration, "queueSizeLimit", DEFAULT_QUEUE_SIZE_LIMIT);

   // Build static HTTP headers
   m_headers = curl_slist_append(nullptr, "Content-Type: application/x-protobuf");

   char *authToken = json_object_get_string_a(configuration, "authToken", "");
   if (authToken[0] != 0)
   {
      std::string header("Authorization: Bearer ");
      header.append(authToken);
      m_headers = curl_slist_append(m_headers, header.c_str());
   }
   MemFree(authToken);

   json_t *customHeaders = json_object_get_object_ex(configuration, "headers");
   if (customHeaders != nullptr)
   {
      const char *key;
      json_t *value;
      json_object_foreach(customHeaders, key, value)
      {
         if (json_is_string(value))
         {
            std::string header(key);
            header.append(": ");
            header.append(json_string_value(value));
            m_headers = curl_slist_append(m_headers, header.c_str());
         }
      }
      json_decref(customHeaders);
   }

   m_curl = nullptr;
   m_stop = false;
   m_healthStatus = true;
   m_queueOverflow = false;
   m_droppedCount = 0;
   m_flushThread = ThreadCreateEx(this, &OtlpEventForwarderDriver::flushThread);

   nxlog_debug_tag(DEBUG_TAG_EXPORT, 3, L"OTLP event forwarder created (endpoint=%hs, batchSize=%u, flushInterval=%us)",
      m_url.c_str(), m_batchSize, m_flushInterval);
}

/**
 * Destructor - stop flush thread (final flush happens there) and release resources
 */
OtlpEventForwarderDriver::~OtlpEventForwarderDriver()
{
   m_stop = true;
   m_wakeup.set();
   ThreadJoin(m_flushThread);

   if (m_curl != nullptr)
      curl_easy_cleanup(m_curl);
   curl_slist_free_all(m_headers);
}

/**
 * Convert an event (and its source object) into an export record
 */
void OtlpEventForwarderDriver::buildRecord(OtlpExportRecord& rec, const Event& event, const shared_ptr<NetObj>& source)
{
   std::string nodeName;
   if (source != nullptr)
   {
      rec.nodeId = source->getId();
      nodeName = WideToUtf8(source->getName());
      char guidText[64];
      rec.hostGuid = source->getGuid().toStringA(guidText);
   }
   else
   {
      rec.nodeId = 0;
      nodeName = "netxms-server";
   }
   rec.serviceName = !m_serviceNameOverride.empty() ? m_serviceNameOverride : nodeName;
   rec.hostName = nodeName;

   rec.timeUnixNano = static_cast<uint64_t>(event.getTimestamp()) * 1000000000ULL;
   rec.observedTimeUnixNano = static_cast<uint64_t>(GetCurrentTimeMs()) * 1000000ULL;
   rec.severityNumber = MapSeverityToOtel(event.getSeverity());
   rec.severityText = SeverityText(event.getSeverity());

   const wchar_t *message = event.getMessage();
   if ((message != nullptr) && (message[0] != 0))
      rec.body = WideToUtf8(message);

   rec.attributes.emplace_back("event.id", static_cast<int64_t>(event.getId()));
   rec.attributes.emplace_back("event.code", static_cast<int64_t>(event.getCode()));
   rec.attributes.emplace_back(std::string("event.name"), WideToUtf8(event.getName()));
   rec.attributes.emplace_back(std::string("event.origin"), std::string(EventOriginName(event.getOrigin())));
   if (event.getDciId() != 0)
      rec.attributes.emplace_back("dci.id", static_cast<int64_t>(event.getDciId()));

   StringBuffer tags = event.getTagsAsList();
   if (!tags.isEmpty())
      rec.attributes.emplace_back(std::string("event.tags"), WideToUtf8(tags.cstr()));

   // Named event parameters
   for(int i = 0; i < event.getParametersCount(); i++)
   {
      const wchar_t *name = event.getParameterName(i);
      std::string key("param.");
      if ((name != nullptr) && (name[0] != 0))
      {
         key.append(WideToUtf8(name));
      }
      else
      {
         char indexText[16];
         IntegerToString(i + 1, indexText);
         key.append(indexText);
      }
      rec.attributes.emplace_back(std::move(key), WideToUtf8(event.getParameter(i, L"")));
   }
}

/**
 * Forward (enqueue) a single event. Returns immediately - actual delivery is asynchronous.
 */
bool OtlpEventForwarderDriver::forward(const Event& event, const TCHAR *recipient, const shared_ptr<NetObj>& source)
{
   // Loop prevention: never re-export events that originated from OTLP ingestion
   if (event.getOrigin() == EventOrigin::OPENTELEMETRY)
      return true;

   OtlpExportRecord rec;
   buildRecord(rec, event, source);
   if ((recipient != nullptr) && (recipient[0] != 0))
      rec.recipient = WideToUtf8(recipient);

   bool reachedBatch = false;
   m_bufferLock.lock();
   if ((m_queueSizeLimit > 0) && (m_buffer.size() >= m_queueSizeLimit))
   {
      m_droppedCount++;
      if (!m_queueOverflow)
      {
         m_queueOverflow = true;
         nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG_EXPORT,
            L"OTLP event forwarder buffer is full (limit=%u), new events will be dropped", m_queueSizeLimit);
      }
      m_bufferLock.unlock();
      return true;
   }
   if (m_queueOverflow)
   {
      m_queueOverflow = false;
      nxlog_debug_tag(DEBUG_TAG_EXPORT, 4, L"OTLP event forwarder buffer is below limit");
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
bool OtlpEventForwarderDriver::setupCurlHandle()
{
   if (m_curl != nullptr)
      curl_easy_cleanup(m_curl);

   m_curl = curl_easy_init();
   if (m_curl == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_EXPORT, 4, L"Call to curl_easy_init() failed");
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
   curl_easy_setopt(m_curl, CURLOPT_USERAGENT, "NetXMS OTLP Forwarder/" NETXMS_VERSION_STRING_A);
   curl_easy_setopt(m_curl, CURLOPT_URL, m_url.c_str());
   curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, m_headers);

   EnableLibCURLUnexpectedEOFWorkaround(m_curl);
   return true;
}

/**
 * Serialize a batch into an OTLP ExportLogsServiceRequest and POST it. Records are grouped
 * by source node into separate resource entries.
 */
bool OtlpEventForwarderDriver::sendBatch(const std::vector<OtlpExportRecord>& batch)
{
   opentelemetry::proto::collector::logs::v1::ExportLogsServiceRequest request;

   // Group records into one resource per (source node, recipient) combination
   std::unordered_map<std::string, logs_v1::ScopeLogs*> scopeByResource;
   for(const OtlpExportRecord& rec : batch)
   {
      char nodeKey[16];
      std::string resourceKey(IntegerToString(rec.nodeId, nodeKey));
      resourceKey.append(1, '\x1f');
      resourceKey.append(rec.recipient);

      logs_v1::ScopeLogs *scopeLogs;
      auto it = scopeByResource.find(resourceKey);
      if (it == scopeByResource.end())
      {
         logs_v1::ResourceLogs *resourceLogs = request.add_resource_logs();
         auto resource = resourceLogs->mutable_resource();
         AddStringAttribute(resource, "service.name", rec.serviceName);
         AddStringAttribute(resource, "host.name", rec.hostName);
         if (!rec.hostGuid.empty())
            AddStringAttribute(resource, "host.id", rec.hostGuid);
         if (!rec.recipient.empty())
            AddStringAttribute(resource, "netxms.recipient", rec.recipient);
         scopeLogs = resourceLogs->add_scope_logs();
         scopeLogs->mutable_scope()->set_name("netxms");
         scopeByResource[resourceKey] = scopeLogs;
      }
      else
      {
         scopeLogs = it->second;
      }

      logs_v1::LogRecord *logRecord = scopeLogs->add_log_records();
      logRecord->set_time_unix_nano(rec.timeUnixNano);
      logRecord->set_observed_time_unix_nano(rec.observedTimeUnixNano);
      logRecord->set_severity_number(static_cast<logs_v1::SeverityNumber>(rec.severityNumber));
      logRecord->set_severity_text(rec.severityText);
      if (!rec.body.empty())
         logRecord->mutable_body()->set_string_value(rec.body);
      for(const OtlpExportAttribute& attr : rec.attributes)
      {
         auto kv = logRecord->add_attributes();
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
      nxlog_debug_tag(DEBUG_TAG_EXPORT, 4, L"Failed to serialize OTLP ExportLogsServiceRequest");
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

      nxlog_debug_tag(DEBUG_TAG_EXPORT, 7, L"Sending OTLP logs batch (%u records, %u bytes) to %hs",
         static_cast<uint32_t>(batch.size()), static_cast<uint32_t>(payload.size()), m_url.c_str());

      if (curl_easy_perform(m_curl) == CURLE_OK)
      {
         long responseCode;
         curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &responseCode);
         nxlog_debug_tag(DEBUG_TAG_EXPORT, 7, L"OTLP collector HTTP response %03ld, %d bytes data",
            responseCode, static_cast<int>(responseData.size()));
         if ((responseCode >= 200) && (responseCode < 300))
            return true;

         nxlog_debug_tag(DEBUG_TAG_EXPORT, 4, L"OTLP collector returned HTTP %ld", responseCode);
         curl_easy_cleanup(m_curl);
         m_curl = nullptr;
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_EXPORT, 5, L"Call to curl_easy_perform() failed (%hs)", errorText);
         curl_easy_cleanup(m_curl);
         m_curl = nullptr;
      }
   }

   return false;
}

/**
 * Flush worker thread - delivers buffered records in batches
 */
void OtlpEventForwarderDriver::flushThread()
{
   nxlog_debug_tag(DEBUG_TAG_EXPORT, 2, L"OTLP event forwarder flush thread started");
   while(!m_stop)
   {
      m_wakeup.wait(m_flushInterval * 1000);
      m_wakeup.reset();

      std::vector<OtlpExportRecord> batch;
      m_bufferLock.lock();
      if (!m_buffer.empty())
         batch.swap(m_buffer);
      m_bufferLock.unlock();

      if (batch.empty())
         continue;

      if (sendBatch(batch))
      {
         m_healthStatus = true;
         nxlog_debug_tag(DEBUG_TAG_EXPORT, 6, L"OTLP logs batch of %u records delivered", static_cast<uint32_t>(batch.size()));
      }
      else
      {
         m_healthStatus = false;
         m_bufferLock.lock();
         m_droppedCount += static_cast<uint32_t>(batch.size());
         uint32_t totalDropped = m_droppedCount;
         m_bufferLock.unlock();
         nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG_EXPORT,
            L"Failed to deliver OTLP logs batch (%u records dropped, total dropped=%u)",
            static_cast<uint32_t>(batch.size()), totalDropped);
      }
   }

   // Final flush on shutdown
   std::vector<OtlpExportRecord> batch;
   m_bufferLock.lock();
   batch.swap(m_buffer);
   m_bufferLock.unlock();
   if (!batch.empty())
      sendBatch(batch);

   nxlog_debug_tag(DEBUG_TAG_EXPORT, 2, L"OTLP event forwarder flush thread stopped");
}

/**
 * Driver factory
 */
static EventForwarderDriver *CreateOtlpEventForwarderDriver(json_t *configuration)
{
   char *endpoint = json_object_get_string_a(configuration, "endpoint", "");
   bool valid = (endpoint[0] != 0);
   MemFree(endpoint);
   if (!valid)
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_EXPORT, L"OTLP event forwarder driver requires \"endpoint\" in configuration");
      return nullptr;
   }
   return new OtlpEventForwarderDriver(configuration);
}

/**
 * Register OTLP event forwarder driver with the core framework
 */
void RegisterOtlpEventForwarder()
{
   RegisterEventForwarderDriver(L"otlp", CreateOtlpEventForwarderDriver);
}
