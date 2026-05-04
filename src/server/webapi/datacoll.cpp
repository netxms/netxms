/*
** NetXMS - Network Management System
** Copyright (C) 2023-2026 Raden Solutions
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

/**
 * Parse `tier` query parameter — auto/raw/hourly/daily, case-insensitive. Numeric values
 * matching the DciTier enum (0..3) are also accepted to mirror the NXCP wire format.
 */
static DciTier ParseDciTierParameter(const char *value)
{
   if ((value == nullptr) || (*value == 0))
      return DCI_TIER_AUTO;
   if (!stricmp(value, "raw") || !strcmp(value, "1"))
      return DCI_TIER_RAW;
   if (!stricmp(value, "hourly") || !strcmp(value, "2"))
      return DCI_TIER_HOURLY;
   if (!stricmp(value, "daily") || !strcmp(value, "3"))
      return DCI_TIER_DAILY;
   return DCI_TIER_AUTO;
}

/**
 * Parse `function` query parameter — avg/min/max/minmax, case-insensitive. Numeric values
 * matching the DciAggregationFunction enum (0..3) are also accepted.
 */
static DciAggregationFunction ParseDciAggregationFunction(const char *value)
{
   if ((value == nullptr) || (*value == 0))
      return DCI_HAGG_AVG;
   if (!stricmp(value, "min") || !strcmp(value, "1"))
      return DCI_HAGG_MIN;
   if (!stricmp(value, "max") || !strcmp(value, "2"))
      return DCI_HAGG_MAX;
   if (!stricmp(value, "minmax") || !strcmp(value, "3"))
      return DCI_HAGG_MINMAX;
   return DCI_HAGG_AVG;
}

/**
 * Format DciTier as a stable string for JSON responses.
 */
static const char* DciTierName(DciTier tier)
{
   switch(tier)
   {
      case DCI_TIER_RAW:    return "raw";
      case DCI_TIER_HOURLY: return "hourly";
      case DCI_TIER_DAILY:  return "daily";
      default:              return "auto";
   }
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
   uint32_t maxDataPoints = context->getQueryParameterAsUInt32("maxDataPoints");
   Timestamp timeFrom = Timestamp::fromTime(context->getQueryParameterAsTime("timeFrom"));
   Timestamp timeTo = Timestamp::fromTime(context->getQueryParameterAsTime("timeTo"));
   DciTier requestedTier = ParseDciTierParameter(context->getQueryParameter("tier"));
   DciAggregationFunction aggFunction = ParseDciAggregationFunction(context->getQueryParameter("function"));

   if ((maxRows == 0) || (maxRows > MAX_DCI_DATA_RECORDS))
      maxRows = MAX_DCI_DATA_RECORDS;

   // Tier dispatch: identical heuristic to the NXCP path so behavior matches the desktop client.
   int autoSelectThreshold = ConfigReadInt(L"DataCollection.Aggregation.MaxAutoSelectPoints", 5000);
   DciTier resolvedTier = ResolveDciTier(requestedTier, *dci, DCO_TYPE_ITEM,
            timeFrom.isNull() ? 0 : timeFrom.asMilliseconds(),
            timeTo.isNull() ? 0 : timeTo.asMilliseconds(),
            static_cast<DataCollectionTarget&>(*object).getRuntimeFlags(), autoSelectThreshold);

   // On-the-fly bucketing only applies to the raw path. Tier reads pull pre-rolled buckets.
   bool isNumeric = (static_cast<DCItem&>(*dci).getTransformedDataType() != DCI_DT_STRING);
   bool useAggregation = (resolvedTier == DCI_TIER_RAW) && (maxDataPoints > 0) && isNumeric &&
      !timeFrom.isNull() && !timeTo.isNull() &&
      (timeTo.asMilliseconds() > timeFrom.asMilliseconds());

   json_t *response = json_object();
   json_object_set_new(response, "description", json_string_t(dci->getDescription()));
   json_object_set_new(response, "unitName", json_string_t(static_cast<DCItem&>(*dci).getUnitName()));
   json_object_set_new(response, "tierServed", json_string(DciTierName(resolvedTier)));

   json_t *values = json_array();
   json_object_set_new(response, "values", values);

   // If only last value requested, try to get it from cache first (raw path only)
   if ((resolvedTier == DCI_TIER_RAW) && !useAggregation && (maxRows == 1) && timeTo.isNull() && (historicalDataType == HDT_PROCESSED))
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

   // Tier reads filter on bucket_start (idata_1h_<N>/idata_1d_<N> or unified TSDB views) which is
   // stored as a millisecond bigint on non-TSDB and timestamptz on TSDB. The raw path keeps the
   // existing idata_timestamp predicates so on-the-fly bucketing and full-table reads still work.
   wchar_t condition[256] = L"";
   if (resolvedTier != DCI_TIER_RAW)
   {
      if (g_dbSyntax == DB_SYNTAX_TSDB)
      {
         if (!timeFrom.isNull())
            wcscpy(condition, L" AND bucket_start>=ms_to_timestamptz(?)");
         if (!timeTo.isNull())
            wcscat(condition, L" AND bucket_start<=ms_to_timestamptz(?)");
      }
      else
      {
         if (!timeFrom.isNull())
            wcscpy(condition, L" AND bucket_start>=?");
         if (!timeTo.isNull())
            wcscat(condition, L" AND bucket_start<=?");
      }
   }
   else if ((g_dbSyntax == DB_SYNTAX_TSDB) && (g_flags & AF_SINGLE_TABLE_PERF_DATA))
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

   DB_STATEMENT hStmt;
   int64_t bucketSizeMs = 0;
   if (resolvedTier != DCI_TIER_RAW)
   {
      hStmt = PrepareTieredDataSelect(hdb, objectId, resolvedTier, aggFunction, maxRows, condition);
   }
   else if (useAggregation)
   {
      bucketSizeMs = (timeTo.asMilliseconds() - timeFrom.asMilliseconds()) / maxDataPoints;
      if (bucketSizeMs < 1)
         bucketSizeMs = 1;
      hStmt = PrepareAggregatedDataSelect(hdb, objectId, dci->getStorageClass(), bucketSizeMs, condition);
   }
   else
   {
      hStmt = PrepareDataSelect(hdb, objectId, DCO_TYPE_ITEM, dci->getStorageClass(), maxRows, historicalDataType, condition);
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
         if (resolvedTier != DCI_TIER_RAW)
         {
            // Tier shape: bucket_start (ms), value(s), sample_count. Column index of sample_count
            // depends on whether MINMAX returned one or two value columns — see GetTierColumnList.
            json_object_set_new(response, "aggregated", json_true());
            const char *valueKey = "avg";
            if (aggFunction == DCI_HAGG_MIN)
               valueKey = "min";
            else if (aggFunction == DCI_HAGG_MAX)
               valueKey = "max";
            int sampleCountColumn = (aggFunction == DCI_HAGG_MINMAX) ? 3 : 2;
            while(DBFetch(hResult))
            {
               json_t *dataPoint = json_object();
               json_object_set_new(dataPoint, "timestamp", DBGetFieldTimestamp(hResult, 0).asJson());
               if (aggFunction == DCI_HAGG_MINMAX)
               {
                  json_object_set_new(dataPoint, "min", json_real(DBGetFieldDouble(hResult, 1)));
                  json_object_set_new(dataPoint, "max", json_real(DBGetFieldDouble(hResult, 2)));
               }
               else
               {
                  json_object_set_new(dataPoint, valueKey, json_real(DBGetFieldDouble(hResult, 1)));
               }
               json_object_set_new(dataPoint, "sampleCount", json_integer(DBGetFieldLong(hResult, sampleCountColumn)));
               json_array_append_new(values, dataPoint);
            }
         }
         else if (useAggregation)
         {
            json_object_set_new(response, "aggregated", json_true());
            json_object_set_new(response, "bucketSize", json_integer(bucketSizeMs));
            while(DBFetch(hResult))
            {
               json_t *dataPoint = json_object();
               json_object_set_new(dataPoint, "timestamp", DBGetFieldTimestamp(hResult, 0).asJson());
               json_object_set_new(dataPoint, "avg", json_real(DBGetFieldDouble(hResult, 1)));
               json_object_set_new(dataPoint, "min", json_real(DBGetFieldDouble(hResult, 2)));
               json_object_set_new(dataPoint, "max", json_real(DBGetFieldDouble(hResult, 3)));
               json_array_append_new(values, dataPoint);
            }
         }
         else
         {
            TCHAR textBuffer[MAX_DCI_STRING_VALUE];
            while(DBFetch(hResult))
            {
               json_t *dataPoint = json_object();
               json_object_set_new(dataPoint, "timestamp", DBGetFieldTimestamp(hResult, 0).asJson());
               json_object_set_new(dataPoint, "value", json_string_t(DBGetField(hResult, 1, textBuffer, MAX_DCI_STRING_VALUE)));
               json_array_append_new(values, dataPoint);
            }
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
      json_decref(response);
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

/**
 * Handler for /v1/objects/:object-id/data-collection/performance-view
 * Returns list of DCIs configured for performance view
 */
int H_PerformanceViewDCIs(Context *context)
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
   json_t *dcis = json_array();
   static_cast<DataCollectionTarget&>(*object).getPerfTabDCIList(dcis, context->getUserId());
   json_object_set_new(response, "dcis", dcis);

   context->setResponseData(response);
   json_decref(response);
   return 200;
}
