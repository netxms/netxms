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
 * Helper: Parse origin string to constant
 */
static int ParseOrigin(const char *str)
{
   if (str == nullptr || !stricmp(str, "agent"))
      return DS_NATIVE_AGENT;
   if (!stricmp(str, "snmp"))
      return DS_SNMP_AGENT;
   if (!stricmp(str, "script"))
      return DS_SCRIPT;
   if (!stricmp(str, "ssh"))
      return DS_SSH;
   if (!stricmp(str, "push"))
      return DS_PUSH_AGENT;
   if (!stricmp(str, "webService"))
      return DS_WEB_SERVICE;
   if (!stricmp(str, "deviceDriver"))
      return DS_DEVICE_DRIVER;
   if (!stricmp(str, "mqtt"))
      return DS_MQTT;
   if (!stricmp(str, "modbus"))
      return DS_MODBUS;
   if (!stricmp(str, "internal"))
      return DS_INTERNAL;
   return -1;
}

/**
 * Helper: Origin constant to string
 */
static const char *OriginToString(int origin)
{
   switch(origin)
   {
      case DS_INTERNAL: return "internal";
      case DS_NATIVE_AGENT: return "agent";
      case DS_SNMP_AGENT: return "snmp";
      case DS_WEB_SERVICE: return "webService";
      case DS_PUSH_AGENT: return "push";
      case DS_WINPERF: return "windowsPerformance";
      case DS_SMCLP: return "smclp";
      case DS_SCRIPT: return "script";
      case DS_SSH: return "ssh";
      case DS_MQTT: return "mqtt";
      case DS_DEVICE_DRIVER: return "deviceDriver";
      case DS_MODBUS: return "modbus";
      case DS_ETHERNET_IP: return "ethernetIp";
      case DS_CLOUD_CONNECTOR: return "cloudConnector";
      case DS_OTLP: return "otlp";
      default: return "unknown";
   }
}

/**
 * Helper: Data type constant to string
 */
static const char *DataTypeToString(int dataType)
{
   switch(dataType)
   {
      case DCI_DT_INT: return "int";
      case DCI_DT_UINT: return "unsigned-int";
      case DCI_DT_INT64: return "int64";
      case DCI_DT_UINT64: return "unsigned-int64";
      case DCI_DT_FLOAT: return "float";
      case DCI_DT_STRING: return "string";
      case DCI_DT_COUNTER32: return "counter32";
      case DCI_DT_COUNTER64: return "counter64";
      default: return "unknown";
   }
}

/**
 * Helper: Parse data type string to constant
 */
static int ParseDataType(const char *str)
{
   if (str == nullptr || !stricmp(str, "string"))
      return DCI_DT_STRING;
   if (!stricmp(str, "int") || !stricmp(str, "integer"))
      return DCI_DT_INT;
   if (!stricmp(str, "unsigned-int") || !stricmp(str, "unsigned-integer"))
      return DCI_DT_UINT;
   if (!stricmp(str, "int64") || !stricmp(str, "integer64"))
      return DCI_DT_INT64;
   if (!stricmp(str, "uint64") || !stricmp(str, "unsigned-int64") || !stricmp(str, "unsigned-integer64"))
      return DCI_DT_UINT64;
   if (!stricmp(str, "counter32"))
      return DCI_DT_COUNTER32;
   if (!stricmp(str, "counter64"))
      return DCI_DT_COUNTER64;
   if (!stricmp(str, "float"))
      return DCI_DT_FLOAT;
   return -1;
}

/**
 * Helper: Delta calculation constant to string
 */
static const char *DeltaCalculationToString(int delta)
{
   switch(delta)
   {
      case DCM_ORIGINAL_VALUE: return "original";
      case DCM_SIMPLE: return "delta";
      case DCM_AVERAGE_PER_SECOND: return "averagePerSecond";
      case DCM_AVERAGE_PER_MINUTE: return "averagePerMinute";
      default: return "unknown";
   }
}

/**
 * Helper: Set or clear a flag on a DCObject based on a boolean argument
 */
static void ApplyFlagFromArguments(DCObject *dco, json_t *arguments, const char *argName, uint32_t flag)
{
   json_t *value = json_object_get(arguments, argName);
   if (value != nullptr)
   {
      if (json_is_true(value))
         dco->setFlag(flag);
      else
         dco->clearFlag(flag);
   }
}

/**
 * Get detailed configuration of a single metric
 */
std::string F_GetMetricDetails(json_t *arguments, uint32_t userId)
{
   const char *metricNameOrId = json_object_get_string_utf8(arguments, "metric", nullptr);
   if ((metricNameOrId == nullptr) || (metricNameOrId[0] == 0))
      return std::string("Metric name or ID must be provided");

   shared_ptr<NetObj> object = FindObjectByNameOrId(arguments, "object");
   if (object == nullptr)
      return std::string("Object not found");
   if (!object->checkAccessRights(userId, OBJECT_ACCESS_READ))
      return std::string("Access denied");

   if (!object->isDataCollectionTarget())
      return std::string("Object is not a data collection target");

   DataCollectionTarget *target = static_cast<DataCollectionTarget*>(object.get());
   shared_ptr<DCObject> dco = FindDCIByNameOrId(target, metricNameOrId, userId);
   if (dco == nullptr)
      return std::string("Metric not found");

   json_t *output = json_object();
   json_object_set_new(output, "id", json_integer(dco->getId()));
   json_object_set_new(output, "name", json_string_t(dco->getName().cstr()));
   json_object_set_new(output, "description", json_string_t(dco->getDescription().cstr()));
   json_object_set_new(output, "type", json_string(dco->getType() == DCO_TYPE_ITEM ? "item" : "table"));
   json_object_set_new(output, "origin", json_string(OriginToString(dco->getDataSource())));
   json_object_set_new(output, "status", json_string(dco->getStatus() == ITEM_STATUS_ACTIVE ? "active" : (dco->getStatus() == ITEM_STATUS_DISABLED ? "disabled" : "notSupported")));

   // Polling settings
   if (dco->getPollingScheduleType() == DC_POLLING_SCHEDULE_DEFAULT)
      json_object_set_new(output, "pollingInterval", json_string("default"));
   else
      json_object_set_new(output, "pollingInterval", json_integer(dco->getEffectivePollingInterval()));

   // Retention settings
   if (dco->getEffectiveRetentionTime() == 0 && !dco->isDataStorageEnabled())
      json_object_set_new(output, "retentionTime", json_string("none"));
   else
      json_object_set_new(output, "retentionTime", json_integer(dco->getEffectiveRetentionTime()));

   // Comments, tags
   SharedString comments = dco->getComments();
   if (!comments.isEmpty())
      json_object_set_new(output, "comments", json_string_t(comments.cstr()));

   SharedString userTag = dco->getUserTag();
   if (!userTag.isEmpty())
      json_object_set_new(output, "userTag", json_string_t(userTag.cstr()));

   SharedString systemTag = dco->getSystemTag();
   if (!systemTag.isEmpty())
      json_object_set_new(output, "systemTag", json_string_t(systemTag.cstr()));

   // Proxy/source node
   if (dco->getSourceNode() != 0)
      json_object_set_new(output, "sourceNode", json_integer(dco->getSourceNode()));

   // SNMP settings
   if (dco->getSnmpPort() != 0)
      json_object_set_new(output, "snmpPort", json_integer(dco->getSnmpPort()));

   // Transformation script
   SharedString transformScript = dco->getTransformationScriptSource();
   if (!transformScript.isEmpty())
      json_object_set_new(output, "transformationScript", json_string_t(transformScript.cstr()));

   // Template info
   if (dco->getTemplateId() != 0)
   {
      json_object_set_new(output, "templateId", json_integer(dco->getTemplateId()));
      json_object_set_new(output, "templateItemId", json_integer(dco->getTemplateItemId()));
   }

   // Related object
   if (dco->getRelatedObject() != 0)
      json_object_set_new(output, "relatedObject", json_integer(dco->getRelatedObject()));

   // Instance discovery
   if (dco->getInstanceDiscoveryMethod() != IDM_NONE)
   {
      json_object_set_new(output, "instanceDiscoveryMethod", json_integer(dco->getInstanceDiscoveryMethod()));
      SharedString instData = dco->getInstanceDiscoveryData();
      if (!instData.isEmpty())
         json_object_set_new(output, "instanceDiscoveryData", json_string_t(instData.cstr()));
   }

   // Flags
   json_t *flags = json_object();
   json_object_set_new(flags, "showOnObjectTooltip", json_boolean(dco->isShowOnObjectTooltip()));
   json_object_set_new(flags, "showInObjectOverview", json_boolean(dco->isShowInObjectOverview()));
   json_object_set_new(flags, "aggregateOnCluster", json_boolean(dco->isAggregateOnCluster()));
   json_object_set_new(flags, "calculateNodeStatus", json_boolean(dco->isStatusDCO()));
   json_object_set_new(flags, "aggregateWithErrors", json_boolean(dco->isAggregateWithErrors()));
   json_object_set_new(flags, "storeChangesOnly", json_boolean(dco->isStoreChangesOnly()));
   json_object_set_new(flags, "unsupportedAsError", json_boolean(dco->isUnsupportedAsError()));
   json_object_set_new(flags, "hideOnLastValuesPage", json_boolean((dco->getFlags() & DCF_HIDE_ON_LAST_VALUES_PAGE) != 0));
   json_object_set_new(output, "flags", flags);

   // Last poll/value timestamps
   if (!dco->getLastPollTime().isNull())
      json_object_set_new(output, "lastPollTime", dco->getLastPollTime().asJson());
   if (!dco->getLastValueTimestamp().isNull())
      json_object_set_new(output, "lastValueTimestamp", dco->getLastValueTimestamp().asJson());
   json_object_set_new(output, "errorCount", json_integer(dco->getErrorCount()));

   // DCItem-specific fields
   if (dco->getType() == DCO_TYPE_ITEM)
   {
      DCItem *dci = static_cast<DCItem*>(dco.get());
      json_object_set_new(output, "dataType", json_string(DataTypeToString(dci->getDataType())));
      json_object_set_new(output, "deltaCalculation", json_string(DeltaCalculationToString(dci->getDeltaCalculationMethod())));
      json_object_set_new(output, "sampleCount", json_integer(dci->getSampleCount()));

      if (dci->getMultiplier() != 0)
         json_object_set_new(output, "multiplier", json_integer(dci->getMultiplier()));

      SharedString unitName = dci->getUnitName();
      if (!unitName.isEmpty())
         json_object_set_new(output, "unitName", json_string_t(unitName.cstr()));

      if (dci->getSampleSaveInterval() > 1)
         json_object_set_new(output, "sampleSaveInterval", json_integer(dci->getSampleSaveInterval()));

      // Anomaly detection
      bool iforest = (dci->getFlags() & DCF_DETECT_ANOMALIES_IFOREST) != 0;
      bool ai = (dci->getFlags() & DCF_DETECT_ANOMALIES_AI) != 0;
      if (iforest && ai)
         json_object_set_new(output, "anomalyDetection", json_string("both"));
      else if (iforest)
         json_object_set_new(output, "anomalyDetection", json_string("iforest"));
      else if (ai)
         json_object_set_new(output, "anomalyDetection", json_string("ai"));
      else
         json_object_set_new(output, "anomalyDetection", json_string("none"));

      // Thresholds summary
      int thresholdCount = dci->getThresholdCount();
      json_object_set_new(output, "thresholdCount", json_integer(thresholdCount));
   }

   return JsonToString(output);
}

/**
 * Get data collection items and their current values for given object
 */
std::string F_GetMetrics(json_t *arguments, uint32_t userId)
{
   shared_ptr<NetObj> object = FindObjectByNameOrId(arguments, "object");
   if (object == nullptr)
      return std::string("Object not found");
   if (!object->checkAccessRights(userId, OBJECT_ACCESS_READ))
      return std::string("Access denied");

   if (!object->isDataCollectionTarget())
      return std::string("Object is not a data collection target");

   wchar_t nameFilter[256];
   utf8_to_wchar(json_object_get_string_utf8(arguments, "filter", ""), -1, nameFilter, 256);

   json_t *output = json_array();
   static_cast<DataCollectionTarget&>(*object).getDataCollectionSummary(output, false, false, true, userId);
   return JsonToString(output);
}

/**
 * Get historical data for data collection item
 */
std::string F_GetHistoricalData(json_t *arguments, uint32_t userId)
{
   String metricName(json_object_get_string_utf8(arguments, "metric", nullptr), "utf-8");
   if (metricName.isEmpty())
      return std::string("Metric name must be provided");

   shared_ptr<NetObj> object = FindObjectByNameOrId(arguments, "object");
   if (object == nullptr)
      return std::string("Object not found");
   if (!object->checkAccessRights(userId, OBJECT_ACCESS_READ))
      return std::string("Access denied");

   if (!object->isDataCollectionTarget())
      return std::string("Object is not a data collection target");

   DataCollectionTarget *target = static_cast<DataCollectionTarget*>(object.get());

   // Find the DCI by name
   shared_ptr<DCObject> dci = target->getDCObjectByName(metricName, userId);
   if (dci == nullptr)
      return std::string("Metric not found");

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

   // Parse maxDataPoints (default 500 for AI requests)
   int maxDataPoints = json_object_get_int32(arguments, "maxDataPoints", 500);

   // Determine if aggregation is applicable (numeric DCIs with valid time range)
   bool isNumeric = (static_cast<DCItem&>(*dci).getTransformedDataType() != DCI_DT_STRING);
   bool useAggregation = (maxDataPoints > 0) && isNumeric && !timeFrom.isNull() && !timeTo.isNull() &&
      (timeTo.asMilliseconds() > timeFrom.asMilliseconds());

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

   DB_STATEMENT hStmt;
   int64_t bucketSizeMs = 0;
   if (useAggregation)
   {
      bucketSizeMs = (timeTo.asMilliseconds() - timeFrom.asMilliseconds()) / maxDataPoints;
      if (bucketSizeMs < 1)
         bucketSizeMs = 1;
      hStmt = PrepareAggregatedDataSelect(hdb, object->getId(), dci->getStorageClass(), bucketSizeMs, condition);
   }
   else
   {
      hStmt = PrepareDataSelect(hdb, object->getId(), DCO_TYPE_ITEM, dci->getStorageClass(), MAX_DCI_DATA_RECORDS, HDT_PROCESSED, condition);
   }

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
         output = json_object();
         json_object_set_new(output, "objectId", json_integer(object->getId()));
         json_object_set_new(output, "objectName", json_string_t(object->getName()));
         json_object_set_new(output, "metricId", json_integer(dci->getId()));
         json_object_set_new(output, "metricName", json_string_t(dci->getName().cstr()));
         json_object_set_new(output, "metricDescription", json_string_t(dci->getDescription().cstr()));
         json_object_set_new(output, "timeFrom", timeFrom.asJson());
         json_object_set_new(output, "timeTo", timeTo.asJson());

         if (useAggregation)
         {
            json_object_set_new(output, "aggregated", json_true());
            json_object_set_new(output, "bucketSize", json_integer(bucketSizeMs));
            json_t *dataPoints = json_array();
            while(DBFetch(hResult))
            {
               json_t *dataPoint = json_object();
               json_object_set_new(dataPoint, "timestamp", DBGetFieldTimestamp(hResult, 0).asJson());
               json_object_set_new(dataPoint, "avg", json_real(DBGetFieldDouble(hResult, 1)));
               json_object_set_new(dataPoint, "min", json_real(DBGetFieldDouble(hResult, 2)));
               json_object_set_new(dataPoint, "max", json_real(DBGetFieldDouble(hResult, 3)));
               json_array_append_new(dataPoints, dataPoint);
            }
            json_object_set_new(output, "dataPoints", dataPoints);
         }
         else
         {
            json_t *dataPoints = json_array();
            wchar_t textBuffer[MAX_DCI_STRING_VALUE];
            while(DBFetch(hResult))
            {
               json_t *dataPoint = json_object();
               json_object_set_new(dataPoint, "timestamp", DBGetFieldTimestamp(hResult, 0).asJson());
               json_object_set_new(dataPoint, "value", json_string_t(DBGetField(hResult, 1, textBuffer, MAX_DCI_STRING_VALUE)));
               json_array_append_new(dataPoints, dataPoint);
            }
            json_object_set_new(output, "dataPoints", dataPoints);
         }
         DBFreeResult(hResult);
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
   String metricName(json_object_get_string_utf8(arguments, "metric", nullptr), "utf-8");
   if (metricName.isEmpty())
      return std::string("Metric name must be provided");

   String metricDescription(json_object_get_string_utf8(arguments, "description", json_object_get_string_utf8(arguments, "metric", nullptr)), "utf-8");

   int origin = ParseOrigin(json_object_get_string_utf8(arguments, "origin", "agent"));
   if (origin < 0)
      return std::string("Invalid origin specified. Supported: agent, snmp, script, ssh, push, webService, deviceDriver, mqtt, modbus, internal");

   int dataType = ParseDataType(json_object_get_string_utf8(arguments, "dataType", "string"));
   if (dataType < 0)
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

   shared_ptr<NetObj> object = FindObjectByNameOrId(arguments, "object");
   if (object == nullptr)
      return std::string("Object not found");
   if (!object->checkAccessRights(userId, OBJECT_ACCESS_READ))
      return std::string("Access denied");

   if (!object->isDataCollectionTarget())
      return std::string("Object is not a data collection target");

   if (!object->checkAccessRights(userId, OBJECT_ACCESS_MODIFY))
      return std::string("Access denied");

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

   // Comments and tags
   const char *commentsStr = json_object_get_string_utf8(arguments, "comments", nullptr);
   if (commentsStr != nullptr)
      dci->setComments(String(commentsStr, "utf-8"));

   const char *userTagStr = json_object_get_string_utf8(arguments, "userTag", nullptr);
   if (userTagStr != nullptr)
      dci->setUserTag(String(userTagStr, "utf-8"));

   // Source node (proxy)
   const char *sourceNodeStr = json_object_get_string_utf8(arguments, "sourceNode", nullptr);
   if (sourceNodeStr != nullptr)
   {
      shared_ptr<NetObj> sourceNode = FindObjectByNameOrId(arguments, "sourceNode");
      if (sourceNode == nullptr)
         { delete dci; return std::string("Source node not found"); }
      dci->setSourceNode(sourceNode->getId());
   }

   // SNMP settings
   int snmpPort = json_object_get_int32(arguments, "snmpPort", 0);
   if (snmpPort > 0)
      dci->setSnmpPort(static_cast<uint16_t>(snmpPort));

   // Boolean flags
   ApplyFlagFromArguments(dci, arguments, "showOnObjectTooltip", DCF_SHOW_ON_OBJECT_TOOLTIP);
   ApplyFlagFromArguments(dci, arguments, "showInObjectOverview", DCF_SHOW_IN_OBJECT_OVERVIEW);
   ApplyFlagFromArguments(dci, arguments, "calculateNodeStatus", DCF_CALCULATE_NODE_STATUS);
   ApplyFlagFromArguments(dci, arguments, "storeChangesOnly", DCF_STORE_CHANGES_ONLY);
   ApplyFlagFromArguments(dci, arguments, "hideOnLastValuesPage", DCF_HIDE_ON_LAST_VALUES_PAGE);

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
   const char *metricNameOrId = json_object_get_string_utf8(arguments, "metric", nullptr);
   if ((metricNameOrId == nullptr) || (metricNameOrId[0] == 0))
      return std::string("Metric name or ID must be provided");

   shared_ptr<NetObj> object = FindObjectByNameOrId(arguments, "object");
   if (object == nullptr)
      return std::string("Object not found");
   if (!object->checkAccessRights(userId, OBJECT_ACCESS_READ))
      return std::string("Access denied");

   if (!object->isDataCollectionTarget())
      return std::string("Object is not a data collection target");

   if (!object->checkAccessRights(userId, OBJECT_ACCESS_MODIFY))
      return std::string("Access denied");

   DataCollectionTarget *target = static_cast<DataCollectionTarget*>(object.get());
   shared_ptr<DCObject> dco = FindDCIByNameOrId(target, metricNameOrId, userId);
   if (dco == nullptr)
      return std::string("Metric not found");

   if (dco->getType() != DCO_TYPE_ITEM)
      return std::string("Object is not a data collection item");

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
      dci->setUnitName(SharedString(String(unitNameStr, "utf-8")));

   // Name (rename)
   const char *newName = json_object_get_string_utf8(arguments, "name", nullptr);
   if (newName != nullptr)
      dci->setName(String(newName, "utf-8"));

   // Description
   const char *descStr = json_object_get_string_utf8(arguments, "description", nullptr);
   if (descStr != nullptr)
      dci->setDescription(String(descStr, "utf-8"));

   // Origin (data source)
   const char *originStr = json_object_get_string_utf8(arguments, "origin", nullptr);
   if (originStr != nullptr)
   {
      int origin = ParseOrigin(originStr);
      if (origin < 0)
         return std::string("Invalid origin specified");
      dci->setSource(static_cast<BYTE>(origin));
   }

   // Data type
   const char *dataTypeStr = json_object_get_string_utf8(arguments, "dataType", nullptr);
   if (dataTypeStr != nullptr)
   {
      int dataType = ParseDataType(dataTypeStr);
      if (dataType < 0)
         return std::string("Invalid data type specified");
      dci->setDataType(static_cast<BYTE>(dataType));
   }

   // Delta calculation
   const char *deltaStr = json_object_get_string_utf8(arguments, "deltaCalculation", nullptr);
   if (deltaStr != nullptr)
   {
      int delta = ParseDeltaCalculation(deltaStr);
      if (delta < 0)
         return std::string("Invalid delta calculation specified");
      dci->setDeltaCalculation(static_cast<BYTE>(delta));
   }

   // Sample count
   json_t *sampleCountJson = json_object_get(arguments, "sampleCount");
   if (sampleCountJson != nullptr && json_is_integer(sampleCountJson))
      dci->setSampleCount(static_cast<int>(json_integer_value(sampleCountJson)));

   // Multiplier
   json_t *multiplierJson = json_object_get(arguments, "multiplier");
   if (multiplierJson != nullptr && json_is_integer(multiplierJson))
      dci->setMultiplier(static_cast<int>(json_integer_value(multiplierJson)));

   // Transformation script
   const char *scriptStr = json_object_get_string_utf8(arguments, "transformationScript", nullptr);
   if (scriptStr != nullptr)
      dci->setTransformationScript(String(scriptStr, "utf-8"));

   // Comments
   const char *commentsStr = json_object_get_string_utf8(arguments, "comments", nullptr);
   if (commentsStr != nullptr)
      dci->setComments(String(commentsStr, "utf-8"));

   // User tag
   const char *userTagStr = json_object_get_string_utf8(arguments, "userTag", nullptr);
   if (userTagStr != nullptr)
      dci->setUserTag(String(userTagStr, "utf-8"));

   // System tag
   const char *systemTagStr = json_object_get_string_utf8(arguments, "systemTag", nullptr);
   if (systemTagStr != nullptr)
      dci->setSystemTag(String(systemTagStr, "utf-8"));

   // Source node (proxy)
   const char *sourceNodeStr = json_object_get_string_utf8(arguments, "sourceNode", nullptr);
   if (sourceNodeStr != nullptr)
   {
      if (sourceNodeStr[0] == 0 || !stricmp(sourceNodeStr, "none"))
      {
         dci->setSourceNode(0);
      }
      else
      {
         shared_ptr<NetObj> sourceNode = FindObjectByNameOrId(arguments, "sourceNode");
         if (sourceNode == nullptr)
            return std::string("Source node not found");
         dci->setSourceNode(sourceNode->getId());
      }
   }

   // SNMP port
   json_t *snmpPortJson = json_object_get(arguments, "snmpPort");
   if (snmpPortJson != nullptr)
      dci->setSnmpPort(static_cast<uint16_t>(json_integer_value(snmpPortJson)));

   // Anomaly detection
   const char *anomalyStr = json_object_get_string_utf8(arguments, "anomalyDetection", nullptr);
   if (anomalyStr != nullptr)
   {
      dci->clearFlag(DCF_DETECT_ANOMALIES_IFOREST | DCF_DETECT_ANOMALIES_AI);
      if (!stricmp(anomalyStr, "iforest"))
         dci->setFlag(DCF_DETECT_ANOMALIES_IFOREST);
      else if (!stricmp(anomalyStr, "ai"))
         dci->setFlag(DCF_DETECT_ANOMALIES_AI);
      else if (!stricmp(anomalyStr, "both"))
         dci->setFlag(DCF_DETECT_ANOMALIES_IFOREST | DCF_DETECT_ANOMALIES_AI);
      else if (stricmp(anomalyStr, "none") != 0)
         return std::string("Invalid anomaly detection mode specified");
   }

   // Boolean flags
   ApplyFlagFromArguments(dci, arguments, "showOnObjectTooltip", DCF_SHOW_ON_OBJECT_TOOLTIP);
   ApplyFlagFromArguments(dci, arguments, "showInObjectOverview", DCF_SHOW_IN_OBJECT_OVERVIEW);
   ApplyFlagFromArguments(dci, arguments, "calculateNodeStatus", DCF_CALCULATE_NODE_STATUS);
   ApplyFlagFromArguments(dci, arguments, "storeChangesOnly", DCF_STORE_CHANGES_ONLY);
   ApplyFlagFromArguments(dci, arguments, "hideOnLastValuesPage", DCF_HIDE_ON_LAST_VALUES_PAGE);
   ApplyFlagFromArguments(dci, arguments, "aggregateOnCluster", DCF_AGGREGATE_ON_CLUSTER);

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
   const char *metricNameOrId = json_object_get_string_utf8(arguments, "metric", nullptr);
   if ((metricNameOrId == nullptr) || (metricNameOrId[0] == 0))
      return std::string("Metric name or ID must be provided");

   shared_ptr<NetObj> object = FindObjectByNameOrId(arguments, "object");
   if (object == nullptr)
      return std::string("Object not found");
   if (!object->checkAccessRights(userId, OBJECT_ACCESS_READ))
      return std::string("Access denied");

   if (!object->isDataCollectionTarget())
      return std::string("Object is not a data collection target");

   if (!object->checkAccessRights(userId, OBJECT_ACCESS_MODIFY))
      return std::string("Access denied");

   DataCollectionTarget *target = static_cast<DataCollectionTarget*>(object.get());
   shared_ptr<DCObject> dco = FindDCIByNameOrId(target, metricNameOrId, userId);
   if (dco == nullptr)
      return std::string("Metric not found");

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
   const char *metricNameOrId = json_object_get_string_utf8(arguments, "metric", nullptr);
   if ((metricNameOrId == nullptr) || (metricNameOrId[0] == 0))
      return std::string("Metric name or ID must be provided");

   shared_ptr<NetObj> object = FindObjectByNameOrId(arguments, "object");
   if (object == nullptr)
      return std::string("Object not found");
   if (!object->checkAccessRights(userId, OBJECT_ACCESS_READ))
      return std::string("Access denied");

   if (!object->isDataCollectionTarget())
      return std::string("Object is not a data collection target");

   DataCollectionTarget *target = static_cast<DataCollectionTarget*>(object.get());
   shared_ptr<DCObject> dco = FindDCIByNameOrId(target, metricNameOrId, userId);
   if (dco == nullptr)
      return std::string("Metric not found");

   if (dco->getType() != DCO_TYPE_ITEM)
      return std::string("Object is not a data collection item");

   DCItem *dci = static_cast<DCItem*>(dco.get());
   json_t *output = json_array();

   int count = dci->getThresholdCount();
   for (int i = 0; i < count; i++)
   {
      Threshold *t = dci->getThreshold(i);
      if (t != nullptr)
      {
         json_t *json = t->toJson();
         wchar_t name[256];
         EventNameFromCode(t->getEventCode(), name);
         json_object_set_new(json, "eventName", json_string_t(name));
         EventNameFromCode(t->getRearmEventCode(), name);
         json_object_set_new(json, "rearmEventName", json_string_t(name));
         json_array_append_new(output, json);
      }
   }

   return JsonToString(output);
}

/**
 * Add threshold to a metric
 */
std::string F_AddThreshold(json_t *arguments, uint32_t userId)
{
   const char *metricNameOrId = json_object_get_string_utf8(arguments, "metric", nullptr);
   if ((metricNameOrId == nullptr) || (metricNameOrId[0] == 0))
      return std::string("Metric name or ID must be provided");

   const char *valueStr = json_object_get_string_utf8(arguments, "value", nullptr);
   if ((valueStr == nullptr) || (valueStr[0] == 0))
      return std::string("Threshold value must be provided");

   shared_ptr<NetObj> object = FindObjectByNameOrId(arguments, "object");
   if (object == nullptr)
      return std::string("Object not found");
   if (!object->checkAccessRights(userId, OBJECT_ACCESS_READ))
      return std::string("Access denied");

   if (!object->isDataCollectionTarget())
      return std::string("Object is not a data collection target");

   if (!object->checkAccessRights(userId, OBJECT_ACCESS_MODIFY))
      return std::string("Access denied");

   DataCollectionTarget *target = static_cast<DataCollectionTarget*>(object.get());
   shared_ptr<DCObject> dco = FindDCIByNameOrId(target, metricNameOrId, userId);
   if (dco == nullptr)
      return std::string("Metric not found");

   if (dco->getType() != DCO_TYPE_ITEM)
      return std::string("Object is not a data collection item");

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
   const char *metricNameOrId = json_object_get_string_utf8(arguments, "metric", nullptr);
   if ((metricNameOrId == nullptr) || (metricNameOrId[0] == 0))
      return std::string("Metric name or ID must be provided");

   uint32_t thresholdId = json_object_get_uint32(arguments, "thresholdId", 0);
   if (thresholdId == 0)
      return std::string("Threshold ID must be provided");

   shared_ptr<NetObj> object = FindObjectByNameOrId(arguments, "object");
   if (object == nullptr)
      return std::string("Object not found");
   if (!object->checkAccessRights(userId, OBJECT_ACCESS_READ))
      return std::string("Access denied");

   if (!object->isDataCollectionTarget())
      return std::string("Object is not a data collection target");

   if (!object->checkAccessRights(userId, OBJECT_ACCESS_MODIFY))
      return std::string("Access denied");

   DataCollectionTarget *target = static_cast<DataCollectionTarget*>(object.get());
   shared_ptr<DCObject> dco = FindDCIByNameOrId(target, metricNameOrId, userId);
   if (dco == nullptr)
      return std::string("Metric not found");

   if (dco->getType() != DCO_TYPE_ITEM)
      return std::string("Object is not a data collection item");

   DCItem *dci = static_cast<DCItem*>(dco.get());
   if (!dci->deleteThresholdById(thresholdId))
      return std::string("Threshold not found");

   dci->updateCacheSize();
   dci->getOwner()->markAsModified(MODIFY_DATA_COLLECTION);

   char buffer[128];
   snprintf(buffer, 128, "Threshold with ID %u deleted from metric %u", thresholdId, dci->getId());
   return std::string(buffer);
}
