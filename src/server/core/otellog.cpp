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
** File: otellog.cpp
**
**/

#include "nxcore.h"
#include <nxlpapi.h>

#define DEBUG_TAG L"otel.log"

/**
 * Processing queues
 */
static ObjectQueue<OtelLogRecord> s_processingQueue;
static ObjectQueue<OtelLogRecord> s_writerQueue;

/**
 * Total number of received OpenTelemetry log records
 */
VolatileCounter64 g_otelLogsReceived = 0;

/**
 * Static data
 */
static uint64_t s_recordId = 1;  // Next available record ID
static LogParser *s_parser = nullptr;
static Mutex s_parserLock;
static THREAD s_writerThread = INVALID_THREAD_HANDLE;
static THREAD s_processingThread = INVALID_THREAD_HANDLE;
static int s_absenceSaveCounter = 0;
static bool s_running = false;
static bool s_enableStorage = true;

/**
 * Put new OpenTelemetry log record to the queue
 */
void NXCORE_EXPORTABLE QueueOtelLog(OtelLogRecord *record)
{
   record->timestamp = GetCurrentTimeMs();
   s_processingQueue.put(record);
}

/**
 * Get next OpenTelemetry log record id
 */
uint64_t GetNextOtelLogId()
{
   return s_recordId;
}

/**
 * Serialize record attributes to a compact JSON string (UTF-8).
 * Returned buffer must be released with MemFree().
 */
static char *SerializeAttributes(const OtelLogRecord *record)
{
   if (record->attributes.size() == 0)
      return nullptr;
   json_t *json = record->attributes.toJson();
   char *result = json_dumps(json, JSON_COMPACT);
   json_decref(json);
   return result;
}

/**
 * OpenTelemetry log writer thread
 */
static void OtelLogWriterThread()
{
   ThreadSetName("OtelLogWriter");
   int maxRecords = ConfigReadInt(L"DBWriter.MaxRecordsPerTransaction", 1000);
   nxlog_debug_tag(DEBUG_TAG, 2, L"OpenTelemetry log writer started");

   while(true)
   {
      OtelLogRecord *record = s_writerQueue.getOrBlock();
      if (record == INVALID_POINTER_VALUE)
         break;

      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
      DB_STATEMENT hStmt = DBPrepare(hdb,
            (g_dbSyntax == DB_SYNTAX_TSDB) ?
                  L"INSERT INTO otel_log (id,log_timestamp,origin_timestamp,observed_timestamp,node_id,zone_uin,service_name,scope_name,severity_number,severity_text,trace_id,span_id,flags,dropped_attributes_count,body,attributes) VALUES (?,ms_to_timestamptz(?),?,?,?,?,?,?,?,?,?,?,?,?,?,?)" :
                  L"INSERT INTO otel_log (id,log_timestamp,origin_timestamp,observed_timestamp,node_id,zone_uin,service_name,scope_name,severity_number,severity_text,trace_id,span_id,flags,dropped_attributes_count,body,attributes) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)", true);
      if (hStmt == nullptr)
      {
         delete record;
         DBConnectionPoolReleaseConnection(hdb);
         continue;
      }

      int count = 0;
      DBBegin(hdb);
      while(true)
      {
         char *attributes = SerializeAttributes(record);

         DBBind(hStmt, 1, DB_SQLTYPE_BIGINT, s_recordId++);
         DBBind(hStmt, 2, DB_SQLTYPE_BIGINT, record->timestamp);
         DBBind(hStmt, 3, DB_SQLTYPE_BIGINT, record->originTimestamp);
         DBBind(hStmt, 4, DB_SQLTYPE_BIGINT, record->observedTimestamp);
         DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, record->nodeId);
         DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, record->zoneUIN);
         DBBind(hStmt, 7, DB_SQLTYPE_VARCHAR, record->serviceName, DB_BIND_STATIC, 127);
         DBBind(hStmt, 8, DB_SQLTYPE_VARCHAR, record->scopeName, DB_BIND_STATIC, 127);
         DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, record->severityNumber);
         DBBind(hStmt, 10, DB_SQLTYPE_VARCHAR, record->severityText, DB_BIND_STATIC, 31);
         DBBind(hStmt, 11, DB_SQLTYPE_VARCHAR, record->traceId, DB_BIND_STATIC, 32);
         DBBind(hStmt, 12, DB_SQLTYPE_VARCHAR, record->spanId, DB_BIND_STATIC, 16);
         DBBind(hStmt, 13, DB_SQLTYPE_INTEGER, record->flags);
         DBBind(hStmt, 14, DB_SQLTYPE_INTEGER, record->droppedAttributesCount);
         DBBind(hStmt, 15, DB_SQLTYPE_TEXT, record->body, DB_BIND_STATIC);
         DBBind(hStmt, 16, DB_SQLTYPE_TEXT, DB_CTYPE_UTF8_STRING, attributes, DB_BIND_STATIC);
         bool success = DBExecute(hStmt);
         MemFree(attributes);
         delete record;
         if (!success)
            break;

         count++;
         if (count == maxRecords)
            break;

         record = s_writerQueue.getOrBlock(500);
         if ((record == nullptr) || (record == INVALID_POINTER_VALUE))
            break;
      }
      DBCommit(hdb);
      DBFreeStatement(hStmt);
      DBConnectionPoolReleaseConnection(hdb);
      if (record == INVALID_POINTER_VALUE)
         break;
   }

   nxlog_debug_tag(DEBUG_TAG, 2, L"OpenTelemetry log writer stopped");
}

/**
 * OpenTelemetry log writer thread - PostgreSQL version
 */
static void OtelLogWriterThread_PGSQL()
{
   ThreadSetName("OtelLogWriter");
   nxlog_debug_tag(DEBUG_TAG, 2, L"OpenTelemetry log writer started");

   int maxRecordsPerTxn = ConfigReadInt(L"DBWriter.MaxRecordsPerTransaction", 1000);
   int maxRecordsPerStmt = ConfigReadInt(L"DBWriter.MaxRecordsPerStatement", 100);
   if (maxRecordsPerTxn < maxRecordsPerStmt)
      maxRecordsPerTxn = maxRecordsPerStmt;
   else if (maxRecordsPerTxn % maxRecordsPerStmt != 0)
      maxRecordsPerTxn = (maxRecordsPerTxn / maxRecordsPerStmt + 1) * maxRecordsPerStmt;
   bool convertTimestamp = (g_dbSyntax == DB_SYNTAX_TSDB);

   StringBuffer query;
   query.setAllocationStep(65536);
   const wchar_t *queryBase = L"INSERT INTO otel_log (id,log_timestamp,origin_timestamp,observed_timestamp,node_id,zone_uin,service_name,scope_name,severity_number,severity_text,trace_id,span_id,flags,dropped_attributes_count,body,attributes) VALUES ";

   while(true)
   {
      OtelLogRecord *record = s_writerQueue.getOrBlock();
      if (record == INVALID_POINTER_VALUE)
         break;

      query = queryBase;
      int countTxn = 0, countStmt = 0;

      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
      if (!DBBegin(hdb))
      {
         delete record;
         DBConnectionPoolReleaseConnection(hdb);
         continue;
      }

      while(true)
      {
         query.append(L'(');
         query.append(s_recordId++);
         if (convertTimestamp)
         {
            query.append(L",ms_to_timestamptz(");
            query.append(record->timestamp);
            query.append(L"),");
         }
         else
         {
            query.append(L',');
            query.append(record->timestamp);
            query.append(L',');
         }
         query.append(record->originTimestamp);
         query.append(L',');
         query.append(record->observedTimestamp);
         query.append(L',');
         query.append(record->nodeId);
         query.append(L',');
         query.append(record->zoneUIN);
         query.append(L',');
         query.append(DBPrepareString(hdb, record->serviceName, 127));
         query.append(L',');
         query.append(DBPrepareString(hdb, record->scopeName, 127));
         query.append(L',');
         query.append(record->severityNumber);
         query.append(L',');
         query.append(DBPrepareString(hdb, record->severityText, 31));
         query.append(L',');
         query.append(DBPrepareString(hdb, record->traceId, 32));
         query.append(L',');
         query.append(DBPrepareString(hdb, record->spanId, 16));
         query.append(L',');
         query.append(record->flags);
         query.append(L',');
         query.append(record->droppedAttributesCount);
         query.append(L',');
         query.append(DBPrepareString(hdb, record->body));
         query.append(L',');
         char *attributes = SerializeAttributes(record);
         query.append(DBPrepareStringUTF8(hdb, attributes));
         MemFree(attributes);
         query.append(L"),");
         delete record;

         countTxn++;
         countStmt++;

         if (countStmt >= maxRecordsPerStmt)
         {
            countStmt = 0;
            query.shrink();   // Remove trailing , from last block
            query.append(L" ON CONFLICT DO NOTHING");
            if (!DBQuery(hdb, query))
               break;
            query = queryBase;
         }

         if (countTxn >= maxRecordsPerTxn)
            break;

         record = s_writerQueue.getOrBlock(500);
         if ((record == nullptr) || (record == INVALID_POINTER_VALUE))
            break;
      }
      if (countStmt > 0)
      {
         query.shrink();   // Remove trailing , from last block
         query.append(L" ON CONFLICT DO NOTHING");
         DBQuery(hdb, query);
      }
      DBCommit(hdb);
      DBConnectionPoolReleaseConnection(hdb);
      if (record == INVALID_POINTER_VALUE)
         break;
   }

   nxlog_debug_tag(DEBUG_TAG, 2, L"OpenTelemetry log writer stopped");
}

/**
 * Callback for OpenTelemetry log parser
 */
static void OtelLogParserCallback(const LogParserCallbackData& data)
{
   nxlog_debug_tag(DEBUG_TAG, 7, L"OpenTelemetry log record matched, capture group count = %d, repeat count = %d", data.captureGroups->size(), data.repeatCount);

   shared_ptr<Node> node = static_pointer_cast<Node>(FindObjectById(data.objectId, OBJECT_NODE));
   if ((node != nullptr) && ((node->getStatus() != STATUS_UNMANAGED) || (g_flags & AF_TRAPS_FROM_UNMANAGED_NODES)))
   {
      StringMap pmap;
      data.captureGroups->addAllToMap(&pmap);

      EventBuilder builder(data.eventCode, data.objectId);
      builder
         .origin(EventOrigin::OPENTELEMETRY)
         .originTimestamp(data.logRecordTimestamp)
         .tag(data.eventTag)
         .params(pmap);

      if (data.eventTag != nullptr)
         builder.param(L"eventTag", data.eventTag);

      if (data.source != nullptr)
         builder.param(L"otelService", data.source);
      if (data.logName != nullptr)
         builder.param(L"otelScope", data.logName);
      builder.param(L"otelSeverityNumber", data.windowsEventId);
      builder.param(L"otelSeverity", data.severity);
      builder.param(L"repeatCount", data.repeatCount);

      if (data.namedVariables != nullptr)
      {
         for (const auto *p : *data.namedVariables)
            builder.param(p->key, p->value);
      }

      builder.post();
   }
}

/**
 * Save absence detection state to database.
 * Must be called with s_parserLock held.
 */
static void SaveOtelLogAbsenceState()
{
   if (s_parser == nullptr)
      return;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   if (hdb == nullptr)
      return;

   DBBegin(hdb);
   DBQuery(hdb, L"DELETE FROM lp_absence_state WHERE parser_type='O'");

   DB_STATEMENT hStmt = DBPrepare(hdb, L"INSERT INTO lp_absence_state (parser_type,rule_guid,object_id,last_match_time,last_alert_time) VALUES ('O',?,?,?,?)", true);
   if (hStmt != nullptr)
   {
      s_parser->forEachAbsenceState(
         [hStmt] (const uuid& ruleGuid, uint32_t objectId, const AbsenceState *state)
         {
            wchar_t guidStr[64];
            DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, ruleGuid.toString(guidStr), DB_BIND_STATIC);
            DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, objectId);
            DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(state->lastMatchTime));
            DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(state->lastAlertTime));
            DBExecute(hStmt);
         });
      DBFreeStatement(hStmt);
   }

   DBCommit(hdb);
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Load absence detection state from database into parser.
 * Must be called with s_parserLock held and s_parser not null.
 */
static void LoadOtelLogAbsenceState()
{
   if (s_parser == nullptr)
      return;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   if (hdb == nullptr)
      return;

   DB_RESULT hResult = DBSelect(hdb, L"SELECT rule_guid,object_id,last_match_time,last_alert_time FROM lp_absence_state WHERE parser_type='O'");
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for (int i = 0; i < count; i++)
      {
         wchar_t guidStr[64];
         DBGetField(hResult, i, 0, guidStr, 64);
         uuid ruleGuid = uuid::parse(guidStr);
         uint32_t objectId = DBGetFieldULong(hResult, i, 1);
         time_t lastMatchTime = static_cast<time_t>(DBGetFieldULong(hResult, i, 2));
         time_t lastAlertTime = static_cast<time_t>(DBGetFieldULong(hResult, i, 3));
         s_parser->loadAbsenceState(ruleGuid, objectId, lastMatchTime, lastAlertTime);
      }
      DBFreeResult(hResult);
      nxlog_debug_tag(DEBUG_TAG, 3, L"Loaded %d OpenTelemetry log absence state entries from database", count);
   }

   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * OpenTelemetry log absence check task - runs periodically via thread pool
 */
static void OtelLogAbsenceCheckTask()
{
   if (!s_running)
      return;

   s_parserLock.lock();
   if (s_parser != nullptr)
   {
      s_parser->checkAbsenceRules(time(nullptr));
      s_absenceSaveCounter++;
      if (s_absenceSaveCounter >= 5)
      {
         SaveOtelLogAbsenceState();
         s_absenceSaveCounter = 0;
      }
   }
   s_parserLock.unlock();

   if (s_running)
      ThreadPoolScheduleRelative(g_mainThreadPool, 60000, OtelLogAbsenceCheckTask);
}

/**
 * Initialize parser on start or on config change
 */
void InitializeOtelLogParser()
{
   s_parserLock.lock();
   LogParser *prev = s_parser;
   s_parser = nullptr;
   char *xml;
   wchar_t *wxml = ConfigReadCLOB(L"OpenTelemetryLogParser", L"<parser></parser>");
   if (wxml != nullptr)
   {
      xml = UTF8StringFromWideString(wxml);
      MemFree(wxml);
   }
   else
   {
      xml = nullptr;
   }
   if (xml != nullptr)
   {
      wchar_t parseError[256];
      ObjectArray<LogParser> *parsers = LogParser::createFromXml(xml, -1, parseError, 256, EventNameResolver);
      if ((parsers != nullptr) && (parsers->size() > 0))
      {
         s_parser = parsers->get(0);
         s_parser->setCallback(OtelLogParserCallback);
         if (prev != nullptr)
            s_parser->restoreCounters(prev);
         else
            LoadOtelLogAbsenceState();
         nxlog_debug_tag(DEBUG_TAG, 3, L"OpenTelemetry log parser successfully created from config");
      }
      else
      {
         nxlog_write(NXLOG_ERROR, L"Cannot initialize OpenTelemetry log parser (%s)", parseError);
      }
      MemFree(xml);
      delete parsers;
   }
   s_parserLock.unlock();
   delete prev;
}

/**
 * OpenTelemetry log processing thread
 */
static void OtelLogProcessingThread()
{
   ThreadSetName("OtelLogProc");

   while(true)
   {
      OtelLogRecord *record = s_processingQueue.getOrBlock();
      if (record == INVALID_POINTER_VALUE)
         break;

      InterlockedIncrement64(&g_otelLogsReceived);

      bool writeToDatabase = true;

      s_parserLock.lock();
      if (s_parser != nullptr)
      {
         s_parser->matchEvent(record->serviceName, static_cast<uint32_t>(record->severityNumber), record->mappedSeverity,
            record->body, nullptr, 0, record->nodeId, static_cast<time_t>(record->originTimestamp / 1000), record->scopeName,
            &writeToDatabase, &record->attributes);
      }
      s_parserLock.unlock();

      if (writeToDatabase && s_enableStorage)
         s_writerQueue.put(record);
      else
         delete record;
   }
}

/**
 * Collects information about events used by OpenTelemetry log parser
 */
void GetOtelLogEventReferences(uint32_t eventCode, ObjectArray<EventReference>* eventReferences)
{
   s_parserLock.lock();
   if (s_parser != nullptr && s_parser->isUsingEvent(eventCode))
   {
      eventReferences->add(new EventReference(EventReferenceType::OTEL_LOG));
   }
   s_parserLock.unlock();
}

/**
 * Handler for OpenTelemetry log related configuration changes
 */
void OnOtelLogsConfigurationChange(const wchar_t *name, const wchar_t *value)
{
   if (!wcscmp(name, L"OTLP.Logs.EnableStorage"))
   {
      s_enableStorage = wcstol(value, nullptr, 0) ? true : false;
      nxlog_debug_tag(DEBUG_TAG, 4, L"Local OpenTelemetry log storage is %s", s_enableStorage ? L"enabled" : L"disabled");
   }
}

/**
 * Start OpenTelemetry log writer and processing threads
 */
void StartOtelLogProcessing()
{
   s_enableStorage = ConfigReadBoolean(L"OTLP.Logs.EnableStorage", false);

   // Determine first available record id
   uint64_t id = ConfigReadUInt64(L"FirstFreeOtelLogId", s_recordId);
   if (id > s_recordId)
      s_recordId = id;
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, L"SELECT max(id) FROM otel_log");
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         s_recordId = std::max(DBGetFieldUInt64(hResult, 0, 0) + 1, s_recordId);
      }
      DBFreeResult(hResult);
   }
   DBConnectionPoolReleaseConnection(hdb);

   InitLogParserLibrary();

   // Create message parser
   InitializeOtelLogParser();

   s_writerThread = ThreadCreateEx((g_dbSyntax == DB_SYNTAX_PGSQL) || (g_dbSyntax == DB_SYNTAX_TSDB) ? OtelLogWriterThread_PGSQL : OtelLogWriterThread);
   s_processingThread = ThreadCreateEx(OtelLogProcessingThread);
   s_running = true;
   ThreadPoolScheduleRelative(g_mainThreadPool, 60000, OtelLogAbsenceCheckTask);
}

/**
 * Stop OpenTelemetry log writer and processing threads
 */
void StopOtelLogProcessing()
{
   s_running = false;

   s_processingQueue.put(INVALID_POINTER_VALUE);
   nxlog_debug_tag(DEBUG_TAG, 3, L"Waiting for OpenTelemetry log processing thread to stop");
   ThreadJoin(s_processingThread);

   s_writerQueue.put(INVALID_POINTER_VALUE);
   nxlog_debug_tag(DEBUG_TAG, 3, L"Waiting for OpenTelemetry log writer to stop");
   ThreadJoin(s_writerThread);

   // Save absence state before shutting down
   s_parserLock.lock();
   SaveOtelLogAbsenceState();
   s_parserLock.unlock();

   CleanupLogParserLibrary();
}
