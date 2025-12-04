/*
** NetXMS - Network Management System
** Copyright (C) 2023-2025 Raden Solutions
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
**/

#include "webapi.h"

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
            _sntprintf(query, 512, _T("SELECT date_part('epoch',%s_timestamp)::int,%s%s FROM %s_sc_%s WHERE item_id=?%s ORDER BY %s_timestamp DESC LIMIT %u"),
                     tablePrefix, SELECTION_COLUMNS,
                     tablePrefix, DCObject::getStorageClassName(storageClass), condition, tablePrefix, maxRows);
            break;
         case DB_SYNTAX_DB2:
            _sntprintf(query, 512, _T("SELECT %s_timestamp,%s%s FROM %s WHERE item_id=?%s ORDER BY %s_timestamp DESC FETCH FIRST %u ROWS ONLY"),
                     tablePrefix, SELECTION_COLUMNS,
                     tablePrefix, condition, tablePrefix, maxRows);
            break;
         default:
            nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG_WEBAPI, _T("INTERNAL ERROR: unsupported database in PrepareDataSelect"));
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
            nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG_WEBAPI, _T("INTERNAL ERROR: unsupported database in PrepareDataSelect"));
            return nullptr;   // Unsupported database
      }
   }
   return DBPrepare(hdb, query);
}

/**
 * Handler for /v1/objects/:object-id/data-collection/:dci-id/history
 */
int H_DataCollectionHistory(Context *context)
{
   uint32_t objectId = context->getPlaceholderValueAsUInt32(_T("object-id"));
   if (objectId == 0)
      return 400;

   shared_ptr<NetObj> object = FindObjectById(objectId);
   if (object == nullptr)
      return 404;

   if (!object->checkAccessRights(context->getUserId(), OBJECT_ACCESS_READ))
      return 403;

   if (!object->isDataCollectionTarget())
   {
      context->setErrorResponse("Object is not data collection target");
      return 400;
   }

   uint32_t dciId = context->getPlaceholderValueAsUInt32(_T("dci-id"));
   if (dciId == 0)
      return 400;

   shared_ptr<DCObject> dci = static_cast<DataCollectionTarget&>(*object).getDCObjectById(dciId, context->getUserId());
   if (dci == nullptr)
   {
      context->setErrorResponse("Invalid DCI ID");
      return 404;
   }

   if (dci->getType() != DCO_TYPE_ITEM)
   {
      context->setErrorResponse("This endpoint can be used only for single-value DCIs");
      return 400;
   }

   HistoricalDataType historicalDataType = static_cast<HistoricalDataType>(context->getQueryParameterAsInt32("historicalDataType", HDT_PROCESSED));
   uint32_t maxRows = context->getQueryParameterAsUInt32("maxRows");
   Timestamp timeFrom = Timestamp::fromTime(context->getQueryParameterAsTime("timeFrom"));
   Timestamp timeTo = Timestamp::fromTime(context->getQueryParameterAsTime("timeTo"));

   if ((maxRows == 0) || (maxRows > MAX_DCI_DATA_RECORDS))
      maxRows = MAX_DCI_DATA_RECORDS;

   json_t *response = json_object();
   json_object_set_new(response, "description", json_string_t(dci->getDescription()));
   json_object_set_new(response, "unitName", json_string_t(static_cast<DCItem&>(*dci).getUnitName()));

   json_t *values = json_array();
   json_object_set_new(response, "values", values);

   // If only last value requested, try to get it from cache first
   if ((maxRows == 1) && timeTo.isNull() && (historicalDataType == HDT_PROCESSED))
   {
      ItemValue *v = static_cast<DCItem&>(*dci).getInternalLastValue();
      if (v != nullptr)
      {
         json_t *dataPoint = json_object();
         json_object_set_new(dataPoint, "timestamp", v->getTimeStamp().asJson());
         json_object_set_new(dataPoint, "value", json_string_t(v->getString()));
         json_array_append_new(values, dataPoint);
         delete v;

         context->setResponseData(response);
         return 200;
      }
   }

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

   bool success = false;
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = PrepareDataSelect(hdb, objectId, DCO_TYPE_ITEM, dci->getStorageClass(), maxRows, historicalDataType, condition);
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
         TCHAR textBuffer[MAX_DCI_STRING_VALUE];
         while(DBFetch(hResult))
         {
            json_t *dataPoint = json_object();
            json_object_set_new(dataPoint, "timestamp", DBGetFieldTimestamp(hResult, 0).asJson());
            json_object_set_new(dataPoint, "value", json_string_t(DBGetField(hResult, 1, textBuffer, MAX_DCI_STRING_VALUE)));
            json_array_append_new(values, dataPoint);
         }
         DBFreeResult(hResult);
         success = true;
      }
      DBFreeStatement(hStmt);
   }
   DBConnectionPoolReleaseConnection(hdb);

   if (!success)
   {
      context->setErrorResponse("Database failure");
      return 500;
   }

   context->setResponseData(response);
   json_decref(response);
   return 200;
}

/**
 * Handler for /v1/objects/:object-id/data-collection/current-values
 * Returns current values for all DCIs of given object
 */
int H_DataCollectionCurrentValues(Context *context)
{
   uint32_t objectId = context->getPlaceholderValueAsUInt32(_T("object-id"));
   if (objectId == 0)
      return 400;

   shared_ptr<NetObj> object = FindObjectById(objectId);
   if (object == nullptr)
      return 404;

   if (!object->checkAccessRights(context->getUserId(), OBJECT_ACCESS_READ))
      return 403;

   if (!object->isDataCollectionTarget())
   {
      context->setErrorResponse("Object is not data collection target");
      return 400;
   }

   json_t *response = json_object();
   json_object_set_new(response, "objectId", json_integer(objectId));
   json_object_set_new(response, "objectName", json_string_t(object->getName()));

   json_t *values = json_array();
   static_cast<DataCollectionTarget&>(*object).getDataCollectionSummary(values, false, false, false, context->getUserId());
   json_object_set_new(response, "values", values);

   context->setResponseData(response);
   json_decref(response);
   return 200;
}
