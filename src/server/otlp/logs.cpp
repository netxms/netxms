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
** File: logs.cpp
**
**/

#include "otlp.h"
#include "generated/logs_service.pb.h"
#include "otlp_attributes.h"

/**
 * Map OTLP severity number (1-24) to NetXMS severity used as log parser level.
 * Bands: TRACE/DEBUG/INFO -> NORMAL, WARN -> WARNING, ERROR -> MAJOR, FATAL -> CRITICAL.
 */
static uint32_t MapOtelSeverity(int severityNumber)
{
   if (severityNumber >= 21)
      return SEVERITY_CRITICAL;
   if (severityNumber >= 17)
      return SEVERITY_MAJOR;
   if (severityNumber >= 13)
      return SEVERITY_WARNING;
   return SEVERITY_NORMAL;
}

/**
 * Convert raw identifier bytes (trace ID / span ID) to a lowercase hex wide string.
 */
static void BytesToHex(const std::string& bytes, wchar_t *out, size_t outChars)
{
   static const wchar_t hexChars[] = L"0123456789abcdef";
   size_t n = bytes.size();
   if (n * 2 >= outChars)
      n = (outChars - 1) / 2;
   for (size_t i = 0; i < n; i++)
   {
      uint8_t b = static_cast<uint8_t>(bytes[i]);
      out[i * 2] = hexChars[(b >> 4) & 0x0F];
      out[i * 2 + 1] = hexChars[b & 0x0F];
   }
   out[n * 2] = 0;
}

/**
 * Build a normalized log record from an OTLP LogRecord and hand it off to core.
 */
static void ProcessLogRecord(const shared_ptr<Node>& node, const std::map<std::string, std::string>& resourceAttributes,
   const char *serviceName, const char *scopeName, const opentelemetry::proto::logs::v1::LogRecord& logRecord)
{
   OtelLogRecord *record = new OtelLogRecord();
   record->nodeId = node->getId();
   record->zoneUIN = node->getZoneUIN();

   int64_t originNano = static_cast<int64_t>(logRecord.time_unix_nano());
   int64_t observedNano = static_cast<int64_t>(logRecord.observed_time_unix_nano());
   record->observedTimestamp = observedNano / 1000000;
   // Fall back to observed time when the record carries no event time
   record->originTimestamp = (originNano != 0) ? (originNano / 1000000) : record->observedTimestamp;

   record->severityNumber = static_cast<int>(logRecord.severity_number());
   record->mappedSeverity = MapOtelSeverity(record->severityNumber);
   utf8_to_wchar(logRecord.severity_text().c_str(), -1, record->severityText, 32);
   record->severityText[31] = 0;
   if (serviceName != nullptr)
   {
      utf8_to_wchar(serviceName, -1, record->serviceName, 128);
      record->serviceName[127] = 0;
   }
   if (scopeName != nullptr)
   {
      utf8_to_wchar(scopeName, -1, record->scopeName, 128);
      record->scopeName[127] = 0;
   }

   BytesToHex(logRecord.trace_id(), record->traceId, 33);
   BytesToHex(logRecord.span_id(), record->spanId, 17);

   record->flags = logRecord.flags();
   record->droppedAttributesCount = logRecord.dropped_attributes_count();

   if (logRecord.has_body())
   {
      std::string body;
      AnyValueToString(logRecord.body(), body);
      if (!body.empty())
         record->body = WideStringFromUTF8String(body.c_str());
   }

   // Merge resource attributes and record attributes into the stored attribute set
   std::map<std::string, std::string> attributes(resourceAttributes);
   ExtractDataPointAttributes(logRecord, attributes);
   for (const auto& kv : attributes)
   {
      wchar_t *key = WideStringFromUTF8String(kv.first.c_str());
      wchar_t *value = WideStringFromUTF8String(kv.second.c_str());
      record->attributes.set(key, value);
      MemFree(key);
      MemFree(value);
   }

   QueueOtelLog(record);
}

/**
 * Handler for OTLP logs endpoint (POST /otlp-backend/v1/logs)
 */
int H_OtlpLogs(Context *context)
{
   opentelemetry::proto::collector::logs::v1::ExportLogsServiceRequest request;
   if (!context->hasRequestData() ||
       !request.ParseFromArray(context->getRequestData(), static_cast<int>(context->getRequestDataSize())))
   {
      nxlog_debug_tag(DEBUG_TAG_OTLP, 4, L"Failed to parse OTLP ExportLogsServiceRequest");
      context->setErrorResponse("Invalid protobuf payload");
      return 400;
   }

   nxlog_debug_tag(DEBUG_TAG_OTLP, 6, L"Received OTLP logs batch with %d resource logs", request.resource_logs_size());

   int matchedCount = 0;
   int64_t rejectedRecords = 0;

   for (int r = 0; r < request.resource_logs_size(); r++)
   {
      const auto& resourceLogs = request.resource_logs(r);

      std::map<std::string, std::string> attributes;
      if (resourceLogs.has_resource())
         ExtractResourceAttributes(resourceLogs.resource(), attributes);

      shared_ptr<Node> node = MatchResourceToNode(attributes);
      if (node == nullptr)
      {
         for (int s = 0; s < resourceLogs.scope_logs_size(); s++)
            rejectedRecords += resourceLogs.scope_logs(s).log_records_size();
         continue;
      }
      matchedCount++;

      auto it = attributes.find("service.name");
      const char *serviceName = (it != attributes.end()) ? it->second.c_str() : nullptr;

      for (int s = 0; s < resourceLogs.scope_logs_size(); s++)
      {
         const auto& scopeLogs = resourceLogs.scope_logs(s);
         const char *scopeName = scopeLogs.has_scope() ? scopeLogs.scope().name().c_str() : nullptr;
         for (int l = 0; l < scopeLogs.log_records_size(); l++)
         {
            ProcessLogRecord(node, attributes, serviceName, scopeName, scopeLogs.log_records(l));
         }
      }
   }

   nxlog_debug_tag(DEBUG_TAG_OTLP, 5, L"OTLP logs batch processed: %d resources matched, " INT64_FMT " records rejected", matchedCount, rejectedRecords);

   opentelemetry::proto::collector::logs::v1::ExportLogsServiceResponse response;
   if (rejectedRecords > 0)
      response.mutable_partial_success()->set_rejected_log_records(rejectedRecords);
   std::string serialized;
   response.SerializeToString(&serialized);
   context->setResponseData(serialized.data(), serialized.size(), "application/x-protobuf");
   return 200;
}
