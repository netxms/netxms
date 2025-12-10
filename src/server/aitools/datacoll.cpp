/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Raden Solutions
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
         DC_POLLING_SCHEDULE_DEFAULT, nullptr, DC_RETENTION_DEFAULT, nullptr,
         static_pointer_cast<DataCollectionOwner>(object), metricDescription);
   if (!static_cast<DataCollectionOwner&>(*object).addDCObject(dci, false, false))
   {
      delete dci;
      return std::string("Failed to create data collection item");
   }

   char buffer[32];
   return std::string("Metric created successfully with ID ") + IntegerToString(dci->getId(), buffer);
}
