/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Raden Solutions
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
** File: datacoll.cpp
**
**/

#include "aitools.h"
#include <nxevent.h>

/**
 * Helper: Parse delta calculation string to constant
 */
static int ParseDeltaCalculation(const char *str)
{
   if (str == nullptr || !stricmp(str, "original"))
      return DCM_ORIGINAL_VALUE;
   if (!stricmp(str, "delta"))
      return DCM_SIMPLE;
   if (!stricmp(str, "averagePerSecond"))
      return DCM_AVERAGE_PER_SECOND;
   if (!stricmp(str, "averagePerMinute"))
      return DCM_AVERAGE_PER_MINUTE;
   return -1;
}

/**
 * Helper: Parse threshold function string to constant
 */
static int ParseThresholdFunction(const char *str)
{
   if (str == nullptr || !stricmp(str, "last"))
      return F_LAST;
   if (!stricmp(str, "average"))
      return F_AVERAGE;
   if (!stricmp(str, "meanDeviation"))
      return F_MEAN_DEVIATION;
   if (!stricmp(str, "diff"))
      return F_DIFF;
   if (!stricmp(str, "error"))
      return F_ERROR;
   if (!stricmp(str, "sum"))
      return F_SUM;
   if (!stricmp(str, "script"))
      return F_SCRIPT;
   if (!stricmp(str, "absDeviation"))
      return F_ABS_DEVIATION;
   if (!stricmp(str, "anomaly"))
      return F_ANOMALY;
   return -1;
}

/**
 * Helper: Parse threshold operation string to constant
 */
static int ParseThresholdOperation(const char *str)
{
   if (str == nullptr || !stricmp(str, "greaterOrEqual"))
      return OP_GT_EQ;
   if (!stricmp(str, "less"))
      return OP_LE;
   if (!stricmp(str, "lessOrEqual"))
      return OP_LE_EQ;
   if (!stricmp(str, "equal"))
      return OP_EQ;
   if (!stricmp(str, "greater"))
      return OP_GT;
   if (!stricmp(str, "notEqual"))
      return OP_NE;
   if (!stricmp(str, "like"))
      return OP_LIKE;
   if (!stricmp(str, "notLike"))
      return OP_NOTLIKE;
   if (!stricmp(str, "ilike"))
      return OP_ILIKE;
   if (!stricmp(str, "inotLike"))
      return OP_INOTLIKE;
   return -1;
}

/**
 * Helper: Find DCI by name or numeric ID
 */
static shared_ptr<DCObject> FindDCIByNameOrId(DataCollectionTarget *target, const char *nameOrId, uint32_t userId)
{
   // Try as numeric ID first
   char *endptr;
   uint32_t id = strtoul(nameOrId, &endptr, 10);
   if (*endptr == 0)
      return target->getDCObjectById(id, userId);

   // Try as name
   String name(nameOrId, "utf-8");
   return target->getDCObjectByName(name, userId);
}

/**
 * Helper: Resolve event code from name or numeric string
 */
static uint32_t ResolveEventCode(const char *eventNameOrCode, uint32_t defaultCode)
{
   if (eventNameOrCode == nullptr || eventNameOrCode[0] == 0)
      return defaultCode;

   // Try as numeric first
   char *endptr;
   uint32_t code = strtoul(eventNameOrCode, &endptr, 10);
   if (*endptr == 0)
      return code;

   // Try as event name
   String name(eventNameOrCode, "utf-8");
   return EventCodeFromName(name, defaultCode);
}

/**
 * Get data collection items and their current values for given object
 */
std::string F_GetMetrics(json_t *arguments, uint32_t userId)
{
   const char *objectName = json_object_get_string_utf8(arguments, "object", nullptr);
   if ((objectName == nullptr) || (objectName[0] == 0))
      return std::string("Object name or ID must be provided");

   shared_ptr<NetObj> object = FindObjectByNameOrId(objectName);
   if ((object == nullptr) || !object->checkAccessRights(userId, OBJECT_ACCESS_READ))
   {
      char buffer[256];
      snprintf(buffer, 256, "Object with name or ID \"%s\" is not known", objectName);
      return std::string(buffer);
   }

   if (!object->isDataCollectionTarget())
   {
      char buffer[256];
      snprintf(buffer, 256, "Object with name or ID \"%s\" is not a data collection target", objectName);
      return std::string(buffer);
   }

   wchar_t nameFilter[256];
   utf8_to_wchar(json_object_get_string_utf8(arguments, "filter", ""), -1, nameFilter, 256);

   json_t *output = json_array();
   static_cast<DataCollectionTarget&>(*object).getDataCollectionSummary(output, false, false, true, userId);
   return JsonToString(output);
}

#define SELECTION_COLUMNS (historicalDataType != HDT_RAW) ? tablePrefix : _T(""), (historicalDataType == HDT_RAW_AND_PROCESSED) ? _T("_value,raw_value") : ((historicalDataType == HDT_PROCESSED) || (historicalDataType == HDT_FULL_TABLE)) ?  _T("_value") : _T("raw_value")

/**
 * Prepare statement for reading data from idata/tdata table
 */
static DB_STATEMENT PrepareDataSelect(DB_HANDLE hdb, uint32_t nodeId, int dciType, DCObjectStorageClass storageClass,
         uint32_t maxRows, HistoricalDataType historicalDataType, const TCHAR *condition)
{
   const TCHAR *tablePrefix = (dciType == DCO_TYPE_ITEM) ? _T("idata") : _T("tdata");
   TCHAR query[512];
   if (g_flags & AF_SINGLE_TABLE_PERF_DATA)
   {
      switch(g_dbSyntax)
      {
         case DB_SYNTAX_MSSQL:
            _sntprintf(query, 512, _T("SELECT TOP %u %s_timestamp,%s%s FROM %s WHERE item_id=?%s ORDER BY %s_timestamp DESC"),
                     maxRows, tablePrefix, SELECTION_COLUMNS,
                     tablePrefix, condition, tablePrefix);
            break;
         case DB_SYNTAX_ORACLE:
            _sntprintf(query, 512, _T("SELECT * FROM (SELECT %s_timestamp,%s%s FROM %s WHERE item_id=?%s ORDER BY %s_timestamp DESC) WHERE ROWNUM<=%u"),
                     tablePrefix, SELECTION_COLUMNS,
                     tablePrefix, condition, tablePrefix, maxRows);
            break;
         case DB_SYNTAX_MYSQL:
         case DB_SYNTAX_PGSQL:
         case DB_SYNTAX_SQLITE:
            _sntprintf(query, 512, _T("SELECT %s_timestamp,%s%s FROM %s WHERE item_id=?%s ORDER BY %s_timestamp DESC LIMIT %u"),
                     tablePrefix, SELECTION_COLUMNS,
                     tablePrefix, condition, tablePrefix, maxRows);
            break;
         case DB_SYNTAX_TSDB:
            _sntprintf(query, 512, _T("SELECT timestamptz_to_ms(%s_timestamp),%s%s FROM %s_sc_%s WHERE item_id=?%s ORDER BY %s_timestamp DESC LIMIT %u"),
                     tablePrefix, SELECTION_COLUMNS,
                     tablePrefix, DCObject::getStorageClassName(storageClass), condition, tablePrefix, maxRows);
            break;
         case DB_SYNTAX_DB2:
            _sntprintf(query, 512, _T("SELECT %s_timestamp,%s%s FROM %s WHERE item_id=?%s ORDER BY %s_timestamp DESC FETCH FIRST %u ROWS ONLY"),
                     tablePrefix, SELECTION_COLUMNS,
                     tablePrefix, condition, tablePrefix, maxRows);
            break;
         default:
            nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("INTERNAL ERROR: unsupported database in PrepareDataSelect"));
            return nullptr;   // Unsupported database
      }
   }
   else
   {
      switch(g_dbSyntax)
      {
         case DB_SYNTAX_MSSQL:
            _sntprintf(query, 512, _T("SELECT TOP %u %s_timestamp,%s%s FROM %s_%u WHERE item_id=?%s ORDER BY %s_timestamp DESC"),
                     maxRows, tablePrefix, SELECTION_COLUMNS,
                     tablePrefix, nodeId, condition, tablePrefix);
            break;
         case DB_SYNTAX_ORACLE:
            _sntprintf(query, 512, _T("SELECT * FROM (SELECT %s_timestamp,%s%s FROM %s_%u WHERE item_id=?%s ORDER BY %s_timestamp DESC) WHERE ROWNUM<=%u"),
                     tablePrefix, SELECTION_COLUMNS,
                     tablePrefix, nodeId, condition, tablePrefix, maxRows);
            break;
         case DB_SYNTAX_MYSQL:
         case DB_SYNTAX_PGSQL:
         case DB_SYNTAX_SQLITE:
         case DB_SYNTAX_TSDB:
            _sntprintf(query, 512, _T("SELECT %s_timestamp,%s%s FROM %s_%u WHERE item_id=?%s ORDER BY %s_timestamp DESC LIMIT %u"),
                     tablePrefix, SELECTION_COLUMNS,
                     tablePrefix, nodeId, condition, tablePrefix, maxRows);
            break;
         case DB_SYNTAX_DB2:
            _sntprintf(query, 512, _T("SELECT %s_timestamp,%s%s FROM %s_%u WHERE item_id=?%s ORDER BY %s_timestamp DESC FETCH FIRST %u ROWS ONLY"),
                     tablePrefix, SELECTION_COLUMNS,
                     tablePrefix, nodeId, condition, tablePrefix, maxRows);
            break;
         default:
            nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("INTERNAL ERROR: unsupported database in PrepareDataSelect"));
            return nullptr;   // Unsupported database
      }
   }
   return DBPrepare(hdb, query);
}

/**
 * Get historical data for data collection item
 */
std::string F_GetHistoricalData(json_t *arguments, uint32_t userId)
{
   const char *objectName = json_object_get_string_utf8(arguments, "object", nullptr);
   if ((objectName == nullptr) || (objectName[0] == 0))
      return std::string("Object name or ID must be provided");

   String metricName(json_object_get_string_utf8(arguments, "metric", nullptr), "utf-8");
   if (metricName.isEmpty())
      return std::string("Metric name must be provided");

   shared_ptr<NetObj> object = FindObjectByNameOrId(objectName);
   if ((object == nullptr) || !object->checkAccessRights(userId, OBJECT_ACCESS_READ))
   {
      char buffer[256];
      snprintf(buffer, 256, "Object with name or ID \"%s\" is not known", objectName);
      return std::string(buffer);
   }

   if (!object->isDataCollectionTarget())
   {
      char buffer[256];
      snprintf(buffer, 256, "Object with name or ID \"%s\" is not a data collection target", objectName);
      return std::string(buffer);
   }

   DataCollectionTarget *target = static_cast<DataCollectionTarget*>(object.get());
   
   // Find the DCI by name
   shared_ptr<DCObject> dci = target->getDCObjectByName(metricName, userId);
   if (dci == nullptr)
   {
      char buffer[256];
      snprintf(buffer, 256, "Metric \"%ls\" not found on object \"%s\"", metricName.cstr(), objectName);
      return std::string(buffer);
   }

   if (dci->getType() != DCO_TYPE_ITEM)
   {
      char buffer[256];
      snprintf(buffer, 256, "Metric \"%ls\" is not a data collection item", metricName.cstr());
      return std::string(buffer);
   }

   // Parse time range
   const char *timeFromStr = json_object_get_string_utf8(arguments, "timeFrom", "-1440"); // Default to last 24 hours
   const char *timeToStr = json_object_get_string_utf8(arguments, "timeTo", nullptr);

   Timestamp timeTo;
   if (timeToStr != nullptr)
   {
      timeTo = Timestamp::fromTime(ParseTimestamp(timeToStr));
      if (timeTo.isNull())
      {
         char buffer[256];
         snprintf(buffer, 256, "Invalid time format: \"%s\"", timeToStr);
         return std::string(buffer);
      }
   }
   else
   {
      timeTo = Timestamp::now();
   }

   Timestamp timeFrom = Timestamp::fromTime(ParseTimestamp(timeFromStr));
   if (timeFrom.isNull())
   {
      char buffer[256];
      snprintf(buffer, 256, "Invalid time format: \"%s\"", timeFromStr);
      return std::string(buffer);
   }

   // Get historical data
   wchar_t condition[256] = L"";
   if ((g_dbSyntax == DB_SYNTAX_TSDB) && (g_flags & AF_SINGLE_TABLE_PERF_DATA))
   {
      if (!timeFrom.isNull())
         wcscpy(condition, L" AND idata_timestamp>=ms_to_timestamptz(?)");
      if (!timeTo.isNull())
         wcscat(condition, L" AND idata_timestamp<=ms_to_timestamptz(?)");
   }
   else
   {
      if (!timeFrom.isNull())
         wcscpy(condition, L" AND idata_timestamp>=?");
      if (!timeTo.isNull())
         wcscat(condition, L" AND idata_timestamp<=?");
   }

   json_t *output = nullptr;
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = PrepareDataSelect(hdb, object->getId(), DCO_TYPE_ITEM, dci->getStorageClass(), MAX_DCI_DATA_RECORDS, HDT_PROCESSED, condition);
   if (hStmt != nullptr)
   {
      int pos = 1;
      DBBind(hStmt, pos++, DB_SQLTYPE_INTEGER, dci->getId());
      if (!timeFrom.isNull())
         DBBind(hStmt, pos++, DB_SQLTYPE_BIGINT, timeFrom);
      if (!timeTo.isNull())
         DBBind(hStmt, pos++, DB_SQLTYPE_BIGINT, timeTo);

      DB_UNBUFFERED_RESULT hResult = DBSelectPreparedUnbuffered(hStmt);
      if (hResult != nullptr)
      {
         // Convert to JSON
         output = json_object();
         json_object_set_new(output, "objectId", json_integer(object->getId()));
         json_object_set_new(output, "objectName", json_string_t(object->getName()));
         json_object_set_new(output, "metricId", json_integer(dci->getId()));
         json_object_set_new(output, "metricName", json_string_t(dci->getName().cstr()));
         json_object_set_new(output, "metricDescription", json_string_t(dci->getDescription().cstr()));
         json_object_set_new(output, "timeFrom", timeFrom.asJson());
         json_object_set_new(output, "timeTo", timeTo.asJson());

         json_t *dataPoints = json_array();
         wchar_t textBuffer[MAX_DCI_STRING_VALUE];
         while(DBFetch(hResult))
         {
            json_t *dataPoint = json_object();
            json_object_set_new(dataPoint, "timestamp", DBGetFieldTimestamp(hResult, 0).asJson());
            json_object_set_new(dataPoint, "value", json_string_t(DBGetField(hResult, 1, textBuffer, MAX_DCI_STRING_VALUE)));
            json_array_append_new(dataPoints, dataPoint);
         }
         DBFreeResult(hResult);
         json_object_set_new(output, "dataPoints", dataPoints);
      }
      DBFreeStatement(hStmt);
   }
   DBConnectionPoolReleaseConnection(hdb);

   if (output == nullptr)
      return std::string("Failed to retrieve historical data");

   return JsonToString(output);
}

/**
 * Create metric
 */
std::string F_CreateMetric(json_t *arguments, uint32_t userId)
{
   const char *objectName = json_object_get_string_utf8(arguments, "object", nullptr);
   if ((objectName == nullptr) || (objectName[0] == 0))
      return std::string("Object name or ID must be provided");

   String metricName(json_object_get_string_utf8(arguments, "metric", nullptr), "utf-8");
   if (metricName.isEmpty())
      return std::string("Metric name must be provided");

   String metricDescription(json_object_get_string_utf8(arguments, "description", json_object_get_string_utf8(arguments, "metric", nullptr)), "utf-8");

   const char *originStr = json_object_get_string_utf8(arguments, "origin", "agent");
   int origin = DS_NATIVE_AGENT;
   if (!stricmp(originStr, "agent"))
      origin = DS_NATIVE_AGENT;
   else if (!stricmp(originStr, "snmp"))
      origin = DS_SNMP_AGENT;
   else if (!stricmp(originStr, "script"))
      origin = DS_SCRIPT;
   else
      return std::string("Invalid origin specified");

   const char *dataTypeStr = json_object_get_string_utf8(arguments, "dataType", "string");
   int dataType = DCI_DT_STRING;
   if (!stricmp(dataTypeStr, "int") || !stricmp(dataTypeStr, "integer"))
      dataType = DCI_DT_INT;
   else if (!stricmp(dataTypeStr, "unsigned-int") || !stricmp(dataTypeStr, "unsigned-integer"))
      dataType = DCI_DT_UINT;
   else if (!stricmp(dataTypeStr, "int64") || !stricmp(dataTypeStr, "integer64"))
      dataType = DCI_DT_INT64;
   else if (!stricmp(dataTypeStr, "uint64") || !stricmp(dataTypeStr, "unsigned-int64") || !stricmp(dataTypeStr, "unsigned-integer64"))
      dataType = DCI_DT_UINT64;
   else if (!stricmp(dataTypeStr, "counter32"))
      dataType = DCI_DT_COUNTER32;
   else if (!stricmp(dataTypeStr, "counter64"))
      dataType = DCI_DT_COUNTER64;
   else if (!stricmp(dataTypeStr, "float"))
      dataType = DCI_DT_FLOAT;
   else if (!stricmp(dataTypeStr, "string"))
      dataType = DCI_DT_STRING;
   else
      return std::string("Invalid data type specified");

   // Parse new optional parameters
   json_t *pollingIntervalJson = json_object_get(arguments, "pollingInterval");
   json_t *retentionTimeJson = json_object_get(arguments, "retentionTime");

   BYTE pollingScheduleType = DC_POLLING_SCHEDULE_DEFAULT;
   TCHAR pollingInterval[64] = _T("");
   if (pollingIntervalJson != nullptr)
   {
      pollingScheduleType = DC_POLLING_SCHEDULE_CUSTOM;
      if (json_is_integer(pollingIntervalJson))
         _sntprintf(pollingInterval, 64, _T("%d"), static_cast<int>(json_integer_value(pollingIntervalJson)));
      else if (json_is_string(pollingIntervalJson))
      {
         String interval(json_string_value(pollingIntervalJson), "utf-8");
         _tcslcpy(pollingInterval, interval, 64);
      }
   }

   BYTE retentionType = DC_RETENTION_DEFAULT;
   TCHAR retentionTime[64] = _T("");
   if (retentionTimeJson != nullptr)
   {
      retentionType = DC_RETENTION_CUSTOM;
      if (json_is_integer(retentionTimeJson))
         _sntprintf(retentionTime, 64, _T("%d"), static_cast<int>(json_integer_value(retentionTimeJson)));
      else if (json_is_string(retentionTimeJson))
      {
         String retention(json_string_value(retentionTimeJson), "utf-8");
         _tcslcpy(retentionTime, retention, 64);
      }
   }

   int deltaCalculation = ParseDeltaCalculation(json_object_get_string_utf8(arguments, "deltaCalculation", nullptr));
   if (deltaCalculation < 0)
      return std::string("Invalid delta calculation specified");

   int sampleCount = json_object_get_int32(arguments, "sampleCount", 1);
   int multiplier = json_object_get_int32(arguments, "multiplier", 0);
   String unitName(json_object_get_string_utf8(arguments, "unitName", nullptr), "utf-8");
   String transformationScript = json_object_get_string(arguments, "transformationScript", L"");

   const char *statusStr = json_object_get_string_utf8(arguments, "status", "active");
   int status = ITEM_STATUS_ACTIVE;
   if (!stricmp(statusStr, "active"))
      status = ITEM_STATUS_ACTIVE;
   else if (!stricmp(statusStr, "disabled"))
      status = ITEM_STATUS_DISABLED;
   else
      return std::string("Invalid status specified");

   const char *anomalyStr = json_object_get_string_utf8(arguments, "anomalyDetection", "none");
   uint32_t anomalyFlags = 0;
   if (!stricmp(anomalyStr, "iforest"))
      anomalyFlags = DCF_DETECT_ANOMALIES_IFOREST;
   else if (!stricmp(anomalyStr, "ai"))
      anomalyFlags = DCF_DETECT_ANOMALIES_AI;
   else if (!stricmp(anomalyStr, "both"))
      anomalyFlags = DCF_DETECT_ANOMALIES_IFOREST | DCF_DETECT_ANOMALIES_AI;
   else if (stricmp(anomalyStr, "none") != 0)
      return std::string("Invalid anomaly detection mode specified");

   shared_ptr<NetObj> object = FindObjectByNameOrId(objectName);
   if ((object == nullptr) || !object->checkAccessRights(userId, OBJECT_ACCESS_READ))
   {
      char buffer[256];
      snprintf(buffer, 256, "Object with name or ID \"%s\" is not known", objectName);
      return std::string(buffer);
   }

   if (!object->isDataCollectionTarget())
   {
      char buffer[256];
      snprintf(buffer, 256, "Object with name or ID \"%s\" is not a data collection target", objectName);
      return std::string(buffer);
   }

   if (!object->checkAccessRights(userId, OBJECT_ACCESS_MODIFY))
   {
      char buffer[256];
      snprintf(buffer, 256, "Insufficient rights to modify data collection settings on object \"%s\"", objectName);
      return std::string(buffer);
   }

   DCItem *dci = new DCItem(CreateUniqueId(IDG_ITEM), metricName, origin, dataType,
         pollingScheduleType, (pollingInterval[0] != 0) ? pollingInterval : nullptr,
         retentionType, (retentionTime[0] != 0) ? retentionTime : nullptr,
         static_pointer_cast<DataCollectionOwner>(object), metricDescription);

   // Set additional properties
   dci->setStatus(status, false);
   if (deltaCalculation != DCM_ORIGINAL_VALUE)
      dci->setDeltaCalculation(static_cast<BYTE>(deltaCalculation));
   if (sampleCount != 1)
      dci->setSampleCount(sampleCount);
   if (multiplier != 0)
      dci->setMultiplier(multiplier);
   if (!unitName.isEmpty())
      dci->setUnitName(unitName);
   if (!transformationScript.isEmpty())
      dci->setTransformationScript(transformationScript);
   if (anomalyFlags != 0)
      dci->setFlag(anomalyFlags);

   if (!static_cast<DataCollectionOwner&>(*object).addDCObject(dci, false, true))
   {
      delete dci;
      return std::string("Failed to create data collection item");
   }

   char buffer[32];
   return std::string("Metric created successfully with ID ") + IntegerToString(dci->getId(), buffer);
}

/**
 * Edit metric
 */
std::string F_EditMetric(json_t *arguments, uint32_t userId)
{
   const char *objectName = json_object_get_string_utf8(arguments, "object", nullptr);
   if ((objectName == nullptr) || (objectName[0] == 0))
      return std::string("Object name or ID must be provided");

   const char *metricNameOrId = json_object_get_string_utf8(arguments, "metric", nullptr);
   if ((metricNameOrId == nullptr) || (metricNameOrId[0] == 0))
      return std::string("Metric name or ID must be provided");

   shared_ptr<NetObj> object = FindObjectByNameOrId(objectName);
   if ((object == nullptr) || !object->checkAccessRights(userId, OBJECT_ACCESS_READ))
   {
      char buffer[256];
      snprintf(buffer, 256, "Object with name or ID \"%s\" is not known", objectName);
      return std::string(buffer);
   }

   if (!object->isDataCollectionTarget())
   {
      char buffer[256];
      snprintf(buffer, 256, "Object with name or ID \"%s\" is not a data collection target", objectName);
      return std::string(buffer);
   }

   if (!object->checkAccessRights(userId, OBJECT_ACCESS_MODIFY))
   {
      char buffer[256];
      snprintf(buffer, 256, "Insufficient rights to modify data collection settings on object \"%s\"", objectName);
      return std::string(buffer);
   }

   DataCollectionTarget *target = static_cast<DataCollectionTarget*>(object.get());
   shared_ptr<DCObject> dco = FindDCIByNameOrId(target, metricNameOrId, userId);
   if (dco == nullptr)
   {
      char buffer[256];
      snprintf(buffer, 256, "Metric \"%s\" not found on object \"%s\"", metricNameOrId, objectName);
      return std::string(buffer);
   }

   if (dco->getType() != DCO_TYPE_ITEM)
   {
      char buffer[256];
      snprintf(buffer, 256, "Object \"%s\" is not a data collection item", metricNameOrId);
      return std::string(buffer);
   }

   DCItem *dci = static_cast<DCItem*>(dco.get());

   // Apply updates for provided fields
   json_t *pollingIntervalJson = json_object_get(arguments, "pollingInterval");
   if (pollingIntervalJson != nullptr)
   {
      wchar_t interval[64];
      if (json_is_integer(pollingIntervalJson))
      {
         IntegerToString(static_cast<int32_t>(json_integer_value(pollingIntervalJson)), interval);
      }
      else if (json_is_string(pollingIntervalJson))
      {
         String s(json_string_value(pollingIntervalJson), "utf-8");
         wcslcpy(interval, s, 64);
      }
      else
      {
         return std::string("Invalid polling interval value");
      }
      dci->setPollingIntervalType(DC_POLLING_SCHEDULE_CUSTOM);
      dci->setPollingInterval(interval);
   }

   json_t *retentionTimeJson = json_object_get(arguments, "retentionTime");
   if (retentionTimeJson != nullptr)
   {
      wchar_t retention[64];
      if (json_is_integer(retentionTimeJson))
      {
         IntegerToString(static_cast<int32_t>(json_integer_value(retentionTimeJson)), retention);
      }
      else if (json_is_string(retentionTimeJson))
      {
         String s(json_string_value(retentionTimeJson), "utf-8");
         wcslcpy(retention, s, 64);
      }
      else
      {
         return std::string("Invalid retention time value");
      }
      dci->setRetentionType(DC_RETENTION_CUSTOM);
      dci->setRetention(retention);
   }

   const char *statusStr = json_object_get_string_utf8(arguments, "status", nullptr);
   if (statusStr != nullptr)
   {
      int status;
      if (!stricmp(statusStr, "active"))
         status = ITEM_STATUS_ACTIVE;
      else if (!stricmp(statusStr, "disabled"))
         status = ITEM_STATUS_DISABLED;
      else
         return std::string("Invalid status specified");
      dci->setStatus(status, true, true);
   }

   const char *unitNameStr = json_object_get_string_utf8(arguments, "unitName", nullptr);
   if (unitNameStr != nullptr)
   {
      String unitName(unitNameStr, "utf-8");
      dci->setUnitName(unitName);
   }

   dci->getOwner()->markAsModified(MODIFY_DATA_COLLECTION);

   char buffer[128];
   snprintf(buffer, 128, "Metric with ID %u updated successfully", dci->getId());
   return std::string(buffer);
}

/**
 * Delete metric
 */
std::string F_DeleteMetric(json_t *arguments, uint32_t userId)
{
   const char *objectName = json_object_get_string_utf8(arguments, "object", nullptr);
   if ((objectName == nullptr) || (objectName[0] == 0))
      return std::string("Object name or ID must be provided");

   const char *metricNameOrId = json_object_get_string_utf8(arguments, "metric", nullptr);
   if ((metricNameOrId == nullptr) || (metricNameOrId[0] == 0))
      return std::string("Metric name or ID must be provided");

   shared_ptr<NetObj> object = FindObjectByNameOrId(objectName);
   if ((object == nullptr) || !object->checkAccessRights(userId, OBJECT_ACCESS_READ))
   {
      char buffer[256];
      snprintf(buffer, 256, "Object with name or ID \"%s\" is not known", objectName);
      return std::string(buffer);
   }

   if (!object->isDataCollectionTarget())
   {
      char buffer[256];
      snprintf(buffer, 256, "Object with name or ID \"%s\" is not a data collection target", objectName);
      return std::string(buffer);
   }

   if (!object->checkAccessRights(userId, OBJECT_ACCESS_MODIFY))
   {
      char buffer[256];
      snprintf(buffer, 256, "Insufficient rights to modify data collection settings on object \"%s\"", objectName);
      return std::string(buffer);
   }

   DataCollectionTarget *target = static_cast<DataCollectionTarget*>(object.get());
   shared_ptr<DCObject> dco = FindDCIByNameOrId(target, metricNameOrId, userId);
   if (dco == nullptr)
   {
      char buffer[256];
      snprintf(buffer, 256, "Metric \"%s\" not found on object \"%s\"", metricNameOrId, objectName);
      return std::string(buffer);
   }

   uint32_t dciId = dco->getId();
   uint32_t rcc = 0;
   if (!static_cast<DataCollectionOwner&>(*object).deleteDCObject(dciId, true, userId, &rcc, nullptr))
   {
      char buffer[256];
      snprintf(buffer, 256, "Failed to delete metric with ID %u (error code %u)", dciId, rcc);
      return std::string(buffer);
   }

   char buffer[128];
   snprintf(buffer, 128, "Metric with ID %u deleted successfully", dciId);
   return std::string(buffer);
}

/**
 * Get thresholds for a metric
 */
std::string F_GetThresholds(json_t *arguments, uint32_t userId)
{
   const char *objectName = json_object_get_string_utf8(arguments, "object", nullptr);
   if ((objectName == nullptr) || (objectName[0] == 0))
      return std::string("Object name or ID must be provided");

   const char *metricNameOrId = json_object_get_string_utf8(arguments, "metric", nullptr);
   if ((metricNameOrId == nullptr) || (metricNameOrId[0] == 0))
      return std::string("Metric name or ID must be provided");

   shared_ptr<NetObj> object = FindObjectByNameOrId(objectName);
   if ((object == nullptr) || !object->checkAccessRights(userId, OBJECT_ACCESS_READ))
   {
      char buffer[256];
      snprintf(buffer, 256, "Object with name or ID \"%s\" is not known", objectName);
      return std::string(buffer);
   }

   if (!object->isDataCollectionTarget())
   {
      char buffer[256];
      snprintf(buffer, 256, "Object with name or ID \"%s\" is not a data collection target", objectName);
      return std::string(buffer);
   }

   DataCollectionTarget *target = static_cast<DataCollectionTarget*>(object.get());
   shared_ptr<DCObject> dco = FindDCIByNameOrId(target, metricNameOrId, userId);
   if (dco == nullptr)
   {
      char buffer[256];
      snprintf(buffer, 256, "Metric \"%s\" not found on object \"%s\"", metricNameOrId, objectName);
      return std::string(buffer);
   }

   if (dco->getType() != DCO_TYPE_ITEM)
   {
      char buffer[256];
      snprintf(buffer, 256, "Object \"%s\" is not a data collection item", metricNameOrId);
      return std::string(buffer);
   }

   DCItem *dci = static_cast<DCItem*>(dco.get());
   json_t *output = json_array();

   int count = dci->getThresholdCount();
   for (int i = 0; i < count; i++)
   {
      Threshold *t = dci->getThreshold(i);
      if (t != nullptr)
         json_array_append_new(output, t->toJson());
   }

   return JsonToString(output);
}

/**
 * Add threshold to a metric
 */
std::string F_AddThreshold(json_t *arguments, uint32_t userId)
{
   const char *objectName = json_object_get_string_utf8(arguments, "object", nullptr);
   if ((objectName == nullptr) || (objectName[0] == 0))
      return std::string("Object name or ID must be provided");

   const char *metricNameOrId = json_object_get_string_utf8(arguments, "metric", nullptr);
   if ((metricNameOrId == nullptr) || (metricNameOrId[0] == 0))
      return std::string("Metric name or ID must be provided");

   const char *valueStr = json_object_get_string_utf8(arguments, "value", nullptr);
   if ((valueStr == nullptr) || (valueStr[0] == 0))
      return std::string("Threshold value must be provided");

   shared_ptr<NetObj> object = FindObjectByNameOrId(objectName);
   if ((object == nullptr) || !object->checkAccessRights(userId, OBJECT_ACCESS_READ))
   {
      char buffer[256];
      snprintf(buffer, 256, "Object with name or ID \"%s\" is not known", objectName);
      return std::string(buffer);
   }

   if (!object->isDataCollectionTarget())
   {
      char buffer[256];
      snprintf(buffer, 256, "Object with name or ID \"%s\" is not a data collection target", objectName);
      return std::string(buffer);
   }

   if (!object->checkAccessRights(userId, OBJECT_ACCESS_MODIFY))
   {
      char buffer[256];
      snprintf(buffer, 256, "Insufficient rights to modify data collection settings on object \"%s\"", objectName);
      return std::string(buffer);
   }

   DataCollectionTarget *target = static_cast<DataCollectionTarget*>(object.get());
   shared_ptr<DCObject> dco = FindDCIByNameOrId(target, metricNameOrId, userId);
   if (dco == nullptr)
   {
      char buffer[256];
      snprintf(buffer, 256, "Metric \"%s\" not found on object \"%s\"", metricNameOrId, objectName);
      return std::string(buffer);
   }

   if (dco->getType() != DCO_TYPE_ITEM)
   {
      char buffer[256];
      snprintf(buffer, 256, "Object \"%s\" is not a data collection item", metricNameOrId);
      return std::string(buffer);
   }

   DCItem *dci = static_cast<DCItem*>(dco.get());

   // Parse threshold parameters
   int function = ParseThresholdFunction(json_object_get_string_utf8(arguments, "function", "last"));
   if (function < 0)
      return std::string("Invalid threshold function specified");

   int operation = ParseThresholdOperation(json_object_get_string_utf8(arguments, "operation", "greaterOrEqual"));
   if (operation < 0)
      return std::string("Invalid threshold operation specified");

   int sampleCount = json_object_get_int32(arguments, "sampleCount", 1);
   int repeatInterval = json_object_get_int32(arguments, "repeatInterval", -1);

   const char *activationEvent = json_object_get_string_utf8(arguments, "activationEvent", "SYS_THRESHOLD_REACHED");
   const char *deactivationEvent = json_object_get_string_utf8(arguments, "deactivationEvent", "SYS_THRESHOLD_REARMED");

   const char *script = json_object_get_string_utf8(arguments, "script", nullptr);

   // Create threshold using JSON constructor
   json_t *thresholdJson = json_object();
   json_object_set_new(thresholdJson, "function", json_integer(function));
   json_object_set_new(thresholdJson, "condition", json_integer(operation));
   json_object_set_new(thresholdJson, "value", json_string(valueStr));
   json_object_set_new(thresholdJson, "sampleCount", json_integer(sampleCount));
   json_object_set_new(thresholdJson, "repeatInterval", json_integer(repeatInterval));
   if (script != nullptr)
      json_object_set_new(thresholdJson, "script", json_string(script));

   // Set events - the Threshold constructor will resolve names to codes
   json_object_set_new(thresholdJson, "activationEvent", json_string(activationEvent));
   json_object_set_new(thresholdJson, "deactivationEvent", json_string(deactivationEvent));

   Threshold *threshold = new Threshold(thresholdJson, dci);
   json_decref(thresholdJson);

   dci->addThreshold(threshold);
   dci->updateCacheSize();

   dci->getOwner()->markAsModified(MODIFY_DATA_COLLECTION);

   char buffer[128];
   snprintf(buffer, 128, "Threshold with ID %u added to metric %u", threshold->getId(), dci->getId());
   return std::string(buffer);
}

/**
 * Delete threshold from a metric
 */
std::string F_DeleteThreshold(json_t *arguments, uint32_t userId)
{
   const char *objectName = json_object_get_string_utf8(arguments, "object", nullptr);
   if ((objectName == nullptr) || (objectName[0] == 0))
      return std::string("Object name or ID must be provided");

   const char *metricNameOrId = json_object_get_string_utf8(arguments, "metric", nullptr);
   if ((metricNameOrId == nullptr) || (metricNameOrId[0] == 0))
      return std::string("Metric name or ID must be provided");

   uint32_t thresholdId = json_object_get_uint32(arguments, "thresholdId", 0);
   if (thresholdId == 0)
      return std::string("Threshold ID must be provided");

   shared_ptr<NetObj> object = FindObjectByNameOrId(objectName);
   if ((object == nullptr) || !object->checkAccessRights(userId, OBJECT_ACCESS_READ))
   {
      char buffer[256];
      snprintf(buffer, 256, "Object with name or ID \"%s\" is not known", objectName);
      return std::string(buffer);
   }

   if (!object->isDataCollectionTarget())
   {
      char buffer[256];
      snprintf(buffer, 256, "Object with name or ID \"%s\" is not a data collection target", objectName);
      return std::string(buffer);
   }

   if (!object->checkAccessRights(userId, OBJECT_ACCESS_MODIFY))
   {
      char buffer[256];
      snprintf(buffer, 256, "Insufficient rights to modify data collection settings on object \"%s\"", objectName);
      return std::string(buffer);
   }

   DataCollectionTarget *target = static_cast<DataCollectionTarget*>(object.get());
   shared_ptr<DCObject> dco = FindDCIByNameOrId(target, metricNameOrId, userId);
   if (dco == nullptr)
   {
      char buffer[256];
      snprintf(buffer, 256, "Metric \"%s\" not found on object \"%s\"", metricNameOrId, objectName);
      return std::string(buffer);
   }

   if (dco->getType() != DCO_TYPE_ITEM)
   {
      char buffer[256];
      snprintf(buffer, 256, "Object \"%s\" is not a data collection item", metricNameOrId);
      return std::string(buffer);
   }

   DCItem *dci = static_cast<DCItem*>(dco.get());
   if (!dci->deleteThresholdById(thresholdId))
   {
      char buffer[256];
      snprintf(buffer, 256, "Threshold with ID %u not found on metric \"%s\"", thresholdId, metricNameOrId);
      return std::string(buffer);
   }

   dci->updateCacheSize();
   dci->getOwner()->markAsModified(MODIFY_DATA_COLLECTION);

   char buffer[128];
   snprintf(buffer, 128, "Threshold with ID %u deleted from metric %u", thresholdId, dci->getId());
   return std::string(buffer);
}
