/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Victor Kirhenshtein
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
** File: dcagg.cpp
**
** DCI data aggregation (issue #419). Non-TSDB rollup is implemented in this
** file directly; on TSDB the rollup is performed by per-storage-class
** continuous aggregates whose refresh and retention policies are managed by
** ReconcileTSDBAggregation() at server startup.
**
**/

#include "nxcore.h"

/**
 * Housekeeper back-pressure helper defined in hk.cpp.
 */
bool ThrottleHousekeeper();

#define DEBUG_TAG L"dc.aggregation"

#define ONE_HOUR_MS   _LL(3600000)
#define ONE_DAY_MS    _LL(86400000)

/**
 * Column types for per-object aggregate tables, by database dialect - 64 bit integer
 */
static inline const wchar_t *GetAggregateBigIntType()
{
   return (g_dbSyntax == DB_SYNTAX_ORACLE) ? L"number(20)" : L"bigint";
}

/**
 * Column types for per-object aggregate tables, by database dialect - double
 */
static inline const wchar_t *GetAggregateDoubleType()
{
   switch(g_dbSyntax)
   {
      case DB_SYNTAX_MSSQL:
         return L"float";
      case DB_SYNTAX_ORACLE:
         return L"binary_double";
      case DB_SYNTAX_SQLITE:
         return L"real";
      default:
         return L"double precision";
   }
}

/**
 * SQL expression for casting raw idata_value (varchar) to double precision.
 */
static inline const wchar_t *GetIdataValueCastExpr()
{
   switch(g_dbSyntax)
   {
      case DB_SYNTAX_MYSQL:
         return L"CAST(idata_value AS DECIMAL(30,10))";
      case DB_SYNTAX_MSSQL:
         return L"CAST(idata_value AS float)";
      case DB_SYNTAX_ORACLE:
         return L"TO_NUMBER(idata_value)";
      case DB_SYNTAX_DB2:
         return L"CAST(idata_value AS DOUBLE)";
      default:
         return L"CAST(idata_value AS double precision)";
   }
}

/**
 * Predicate that excludes idata rows whose value cannot be cast to a number.
 * A numeric DCI may still have empty-string rows (e.g. failed collections), and strict
 * dialects (PostgreSQL, MSSQL, MySQL strict mode, Oracle, DB2) error on those casts.
 * Mirrors the regex used by the TSDB continuous aggregates in sql/dbinit_tsdb.sql.
 */
static inline const wchar_t *GetIdataValueNumericFilter()
{
   switch(g_dbSyntax)
   {
      case DB_SYNTAX_PGSQL:
      case DB_SYNTAX_TSDB:
         return L" AND idata_value ~ '^[-+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?$'";
      case DB_SYNTAX_MYSQL:
         return L" AND idata_value REGEXP '^[-+]?[0-9]*\\\\.?[0-9]+([eE][-+]?[0-9]+)?$'";
      case DB_SYNTAX_ORACLE:
      case DB_SYNTAX_DB2:
         return L" AND REGEXP_LIKE(idata_value, '^[-+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?$')";
      case DB_SYNTAX_MSSQL:
         return L" AND TRY_CAST(idata_value AS float) IS NOT NULL";
      default:
         return L" AND idata_value<>''";
   }
}

/**
 * Ensure the per-object hourly or daily aggregate table exists for this object. Uses
 * the ODF_HAS_IDATA_1H_TABLE / ODF_HAS_IDATA_1D_TABLE runtime flag as a fast path; on
 * first call for a given object the CREATE TABLE runs and the flag is set so the table
 * is recognised on reload and dropped on object deletion.
 *
 * Non-TSDB only - TSDB aggregation uses global continuous aggregates.
 */
bool DataCollectionTarget::ensureAggregateTable(DB_HANDLE hdb, bool hourly)
{
   uint32_t runtimeFlag = hourly ? ODF_HAS_IDATA_1H_TABLE : ODF_HAS_IDATA_1D_TABLE;
   if (m_runtimeFlags & runtimeFlag)
      return true;

   const wchar_t *suffix = hourly ? L"1h" : L"1d";
   const wchar_t *int64Type = GetAggregateBigIntType();
   const wchar_t *doubleType = GetAggregateDoubleType();

   wchar_t query[512];
   nx_swprintf(query, 512,
      L"CREATE TABLE idata_%s_%u (item_id integer not null,bucket_start %s not null,"
      L"min_value %s null,max_value %s null,avg_value %s null,sample_count integer not null,"
      L"PRIMARY KEY(item_id,bucket_start))",
      suffix, m_id, int64Type, doubleType, doubleType, doubleType);
   if (!DBQuery(hdb, query))
   {
      nxlog_debug_tag(DEBUG_TAG, 3, L"Failed to create aggregate table idata_%s_%u", suffix, m_id);
      return false;
   }

   lockProperties();
   m_runtimeFlags |= runtimeFlag;
   unlockProperties();
   nxlog_debug_tag(DEBUG_TAG, 4, L"Created aggregate table idata_%s_%u", suffix, m_id);
   return true;
}

/**
 * Build dialect-specific UPSERT statement for aggregate table idata_<suffix>_<objectId>.
 * Bound parameters: 1=item_id, 2=bucket_start, 3=min, 4=max, 5=avg, 6=sample_count.
 */
static void BuildAggregateUpsert(wchar_t *query, size_t size, const wchar_t *suffix, uint32_t objectId)
{
   switch(g_dbSyntax)
   {
      case DB_SYNTAX_MYSQL:
         nx_swprintf(query, size,
            L"INSERT INTO idata_%s_%u (item_id,bucket_start,min_value,max_value,avg_value,sample_count) "
            L"VALUES (?,?,?,?,?,?) "
            L"ON DUPLICATE KEY UPDATE min_value=VALUES(min_value),max_value=VALUES(max_value),"
            L"avg_value=VALUES(avg_value),sample_count=VALUES(sample_count)",
            suffix, objectId);
         break;
      case DB_SYNTAX_MSSQL:
         nx_swprintf(query, size,
            L"MERGE INTO idata_%s_%u WITH (HOLDLOCK) AS t "
            L"USING (VALUES (?,?,?,?,?,?)) AS s(item_id,bucket_start,min_value,max_value,avg_value,sample_count) "
            L"ON t.item_id=s.item_id AND t.bucket_start=s.bucket_start "
            L"WHEN MATCHED THEN UPDATE SET min_value=s.min_value,max_value=s.max_value,"
            L"avg_value=s.avg_value,sample_count=s.sample_count "
            L"WHEN NOT MATCHED THEN INSERT (item_id,bucket_start,min_value,max_value,avg_value,sample_count) "
            L"VALUES (s.item_id,s.bucket_start,s.min_value,s.max_value,s.avg_value,s.sample_count);",
            suffix, objectId);
         break;
      case DB_SYNTAX_ORACLE:
         nx_swprintf(query, size,
            L"MERGE INTO idata_%s_%u t "
            L"USING (SELECT ? AS item_id,? AS bucket_start,? AS min_value,? AS max_value,"
            L"? AS avg_value,? AS sample_count FROM dual) s "
            L"ON (t.item_id=s.item_id AND t.bucket_start=s.bucket_start) "
            L"WHEN MATCHED THEN UPDATE SET t.min_value=s.min_value,t.max_value=s.max_value,"
            L"t.avg_value=s.avg_value,t.sample_count=s.sample_count "
            L"WHEN NOT MATCHED THEN INSERT (item_id,bucket_start,min_value,max_value,avg_value,sample_count) "
            L"VALUES (s.item_id,s.bucket_start,s.min_value,s.max_value,s.avg_value,s.sample_count)",
            suffix, objectId);
         break;
      case DB_SYNTAX_SQLITE:
         // SQLite 3.24+ supports ON CONFLICT; INSERT OR REPLACE covers older versions too.
         nx_swprintf(query, size,
            L"INSERT OR REPLACE INTO idata_%s_%u (item_id,bucket_start,min_value,max_value,avg_value,sample_count) "
            L"VALUES (?,?,?,?,?,?)",
            suffix, objectId);
         break;
      case DB_SYNTAX_PGSQL:
      case DB_SYNTAX_TSDB:
      default:
         nx_swprintf(query, size,
            L"INSERT INTO idata_%s_%u (item_id,bucket_start,min_value,max_value,avg_value,sample_count) "
            L"VALUES (?,?,?,?,?,?) "
            L"ON CONFLICT (item_id,bucket_start) DO UPDATE SET "
            L"min_value=excluded.min_value,max_value=excluded.max_value,"
            L"avg_value=excluded.avg_value,sample_count=excluded.sample_count",
            suffix, objectId);
         break;
   }
}

/**
 * Bind and execute one UPSERT row.
 */
static bool ExecuteAggregateUpsert(DB_STATEMENT hStmt, uint32_t dciId, int64_t bucketStart,
         double minValue, double maxValue, double avgValue, int32_t sampleCount)
{
   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, dciId);
   DBBind(hStmt, 2, DB_SQLTYPE_BIGINT, bucketStart);
   DBBind(hStmt, 3, DB_SQLTYPE_DOUBLE, minValue);
   DBBind(hStmt, 4, DB_SQLTYPE_DOUBLE, maxValue);
   DBBind(hStmt, 5, DB_SQLTYPE_DOUBLE, avgValue);
   DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, sampleCount);
   return DBExecute(hStmt);
}

/**
 * Floor a millisecond timestamp to the start of the bucket that contains it.
 */
static inline int64_t FloorToBucket(int64_t timestampMs, int64_t bucketSizeMs)
{
   return (timestampMs / bucketSizeMs) * bucketSizeMs;
}

/**
 * Build SELECT statement for reading raw idata_<N> values grouped into buckets of
 * bucketSizeMs. Returns rows (bucket_start, min, max, avg, count) for buckets with
 * at least one sample, ordered ascending.
 */
static void BuildRawBucketSelect(wchar_t *query, size_t size, uint32_t objectId, int64_t bucketSizeMs)
{
   const wchar_t *castExpr = GetIdataValueCastExpr();
   const wchar_t *numericFilter = GetIdataValueNumericFilter();

   switch(g_dbSyntax)
   {
      case DB_SYNTAX_MYSQL:
         nx_swprintf(query, size,
            L"SELECT (idata_timestamp DIV " INT64_FMT L")*" INT64_FMT L","
            L"MIN(%s),MAX(%s),AVG(%s),COUNT(*) "
            L"FROM idata_%u WHERE item_id=? AND idata_timestamp>=? AND idata_timestamp<?%s "
            L"GROUP BY 1 ORDER BY 1",
            bucketSizeMs, bucketSizeMs, castExpr, castExpr, castExpr, objectId, numericFilter);
         break;
      case DB_SYNTAX_ORACLE:
         nx_swprintf(query, size,
            L"SELECT TRUNC(idata_timestamp/" INT64_FMT L")*" INT64_FMT L","
            L"MIN(%s),MAX(%s),AVG(%s),COUNT(*) "
            L"FROM idata_%u WHERE item_id=? AND idata_timestamp>=? AND idata_timestamp<?%s "
            L"GROUP BY TRUNC(idata_timestamp/" INT64_FMT L")*" INT64_FMT L" ORDER BY 1",
            bucketSizeMs, bucketSizeMs, castExpr, castExpr, castExpr, objectId, numericFilter, bucketSizeMs, bucketSizeMs);
         break;
      case DB_SYNTAX_MSSQL:
         nx_swprintf(query, size,
            L"SELECT (idata_timestamp/" INT64_FMT L")*" INT64_FMT L","
            L"MIN(%s),MAX(%s),AVG(%s),COUNT(*) "
            L"FROM idata_%u WHERE item_id=? AND idata_timestamp>=? AND idata_timestamp<?%s "
            L"GROUP BY (idata_timestamp/" INT64_FMT L")*" INT64_FMT L" ORDER BY 1",
            bucketSizeMs, bucketSizeMs, castExpr, castExpr, castExpr, objectId, numericFilter, bucketSizeMs, bucketSizeMs);
         break;
      default:
         nx_swprintf(query, size,
            L"SELECT (idata_timestamp/" INT64_FMT L")*" INT64_FMT L","
            L"MIN(%s),MAX(%s),AVG(%s),COUNT(*) "
            L"FROM idata_%u WHERE item_id=? AND idata_timestamp>=? AND idata_timestamp<?%s "
            L"GROUP BY 1 ORDER BY 1",
            bucketSizeMs, bucketSizeMs, castExpr, castExpr, castExpr, objectId, numericFilter);
         break;
   }
}

/**
 * Build SELECT for reading hourly aggregate rows grouped into day buckets.
 * Weighted average: SUM(avg_value * sample_count) / SUM(sample_count).
 */
static void BuildHourlyToDailySelect(wchar_t *query, size_t size, uint32_t objectId)
{
   switch(g_dbSyntax)
   {
      case DB_SYNTAX_MYSQL:
         nx_swprintf(query, size,
            L"SELECT (bucket_start DIV " INT64_FMT L")*" INT64_FMT L","
            L"MIN(min_value),MAX(max_value),"
            L"SUM(avg_value*sample_count)/SUM(sample_count),SUM(sample_count) "
            L"FROM idata_1h_%u WHERE item_id=? AND bucket_start>=? AND bucket_start<? "
            L"GROUP BY 1 ORDER BY 1",
            ONE_DAY_MS, ONE_DAY_MS, objectId);
         break;
      case DB_SYNTAX_ORACLE:
         nx_swprintf(query, size,
            L"SELECT TRUNC(bucket_start/" INT64_FMT L")*" INT64_FMT L","
            L"MIN(min_value),MAX(max_value),"
            L"SUM(avg_value*sample_count)/SUM(sample_count),SUM(sample_count) "
            L"FROM idata_1h_%u WHERE item_id=? AND bucket_start>=? AND bucket_start<? "
            L"GROUP BY TRUNC(bucket_start/" INT64_FMT L")*" INT64_FMT L" ORDER BY 1",
            ONE_DAY_MS, ONE_DAY_MS, objectId, ONE_DAY_MS, ONE_DAY_MS);
         break;
      case DB_SYNTAX_MSSQL:
         nx_swprintf(query, size,
            L"SELECT (bucket_start/" INT64_FMT L")*" INT64_FMT L","
            L"MIN(min_value),MAX(max_value),"
            L"SUM(avg_value*sample_count)/SUM(sample_count),SUM(sample_count) "
            L"FROM idata_1h_%u WHERE item_id=? AND bucket_start>=? AND bucket_start<? "
            L"GROUP BY (bucket_start/" INT64_FMT L")*" INT64_FMT L" ORDER BY 1",
            ONE_DAY_MS, ONE_DAY_MS, objectId, ONE_DAY_MS, ONE_DAY_MS);
         break;
      default:
         nx_swprintf(query, size,
            L"SELECT (bucket_start/" INT64_FMT L")*" INT64_FMT L","
            L"MIN(min_value),MAX(max_value),"
            L"SUM(avg_value*sample_count)/SUM(sample_count),SUM(sample_count) "
            L"FROM idata_1h_%u WHERE item_id=? AND bucket_start>=? AND bucket_start<? "
            L"GROUP BY 1 ORDER BY 1",
            ONE_DAY_MS, ONE_DAY_MS, objectId);
         break;
   }
}

/**
 * Roll up raw idata_<N> values for a single eligible DCI into hourly aggregate buckets.
 * Returns the highest bucket-end processed (exclusive), or 0 if nothing processed.
 *
 * Start is taken from the DCI's in-memory watermark. 0/null means "fresh DCI" and starts
 * from the most recent closed bucket. End is aligned to bucket boundary and capped at
 * now - HourlyCloseWindow so partial buckets stay open.
 */
static int64_t RollupHourlyForDCI(DB_HANDLE hdb, DataCollectionTarget *target, DCItem *dci, int64_t nowMs, int64_t hourlyCloseWindowMs)
{
   int64_t watermark = dci->getAggregationWatermark();
   int64_t cutoff = FloorToBucket(nowMs - hourlyCloseWindowMs, ONE_HOUR_MS);
   int64_t rangeStart;
   if (watermark > 0)
   {
      rangeStart = FloorToBucket(watermark, ONE_HOUR_MS);
   }
   else
   {
      rangeStart = cutoff - ONE_HOUR_MS;   // Fresh DCI - start from the most recent closed bucket
      if (rangeStart < 0)
         rangeStart = 0;
   }

   if ((rangeStart >= cutoff) || !target->ensureAggregateTable(hdb, true))
      return 0;

   wchar_t selectSql[1024];
   BuildRawBucketSelect(selectSql, 1024, target->getId(), ONE_HOUR_MS);

   DB_STATEMENT hSelect = DBPrepare(hdb, selectSql);
   if (hSelect == nullptr)
      return 0;

   DBBind(hSelect, 1, DB_SQLTYPE_INTEGER, dci->getId());
   DBBind(hSelect, 2, DB_SQLTYPE_BIGINT, rangeStart);
   DBBind(hSelect, 3, DB_SQLTYPE_BIGINT, cutoff);

   DB_RESULT hResult = DBSelectPrepared(hSelect);
   DBFreeStatement(hSelect);
   if (hResult == nullptr)
      return 0;

   int rows = DBGetNumRows(hResult);
   if (rows == 0)
   {
      DBFreeResult(hResult);
      return cutoff;   // No raw data in range, but caught up
   }

   wchar_t upsertSql[1024];
   BuildAggregateUpsert(upsertSql, 1024, L"1h", target->getId());
   DB_STATEMENT hUpsert = DBPrepare(hdb, upsertSql);
   if (hUpsert == nullptr)
   {
      DBFreeResult(hResult);
      return 0;
   }

   int upsertCount = 0;
   for(int i = 0; i < rows; i++)
   {
      int64_t bucketStart = DBGetFieldInt64(hResult, i, 0);
      int32_t sampleCount = DBGetFieldLong(hResult, i, 4);
      if (sampleCount <= 0)
         continue;   // Skip empty buckets (leaves chart gap)

      double minValue = DBGetFieldDouble(hResult, i, 1);
      double maxValue = DBGetFieldDouble(hResult, i, 2);
      double avgValue = DBGetFieldDouble(hResult, i, 3);

      if (!ExecuteAggregateUpsert(hUpsert, dci->getId(), bucketStart, minValue, maxValue, avgValue, sampleCount))
         nxlog_debug_tag(DEBUG_TAG, 4, L"Hourly UPSERT failed for DCI [%u] bucket " INT64_FMT, dci->getId(), bucketStart);
      else
         upsertCount++;
   }

   DBFreeStatement(hUpsert);
   DBFreeResult(hResult);

   nxlog_debug_tag(DEBUG_TAG, 7,
      L"Hourly rollup for DCI [%u] on %s [%u]: " INT64_FMT L" -> " INT64_FMT L" (%d buckets)",
      dci->getId(), target->getName(), target->getId(), rangeStart, cutoff, upsertCount);

   return cutoff;
}

/**
 * Roll up hourly aggregate rows for a single eligible DCI into daily aggregate buckets.
 *
 * Reads from idata_1h_<N> so hourly corrections (via watermark push-back and re-rollup)
 * flow naturally into daily. Re-processes a sliding safety window to cover any late
 * hourly backfills that post-date the most recent daily row.
 */
static void RollupDailyForDCI(DB_HANDLE hdb, DataCollectionTarget *target, DCItem *dci,
         int64_t nowMs, int64_t dailyCloseWindowMs)
{
   if (!(target->getRuntimeFlags() & ODF_HAS_IDATA_1H_TABLE))
      return;   // No hourly aggregates to roll up from

   int64_t cutoff = FloorToBucket(nowMs - dailyCloseWindowMs, ONE_DAY_MS);
   if (cutoff <= 0)
      return;

   const int64_t safetyWindowMs = 7 * ONE_DAY_MS;
   int64_t rangeStart = 0;

   wchar_t query[256];
   DB_RESULT hResult;

   // Daily table may not exist yet on first run - only query it if the flag says it does.
   if (target->getRuntimeFlags() & ODF_HAS_IDATA_1D_TABLE)
   {
      nx_swprintf(query, 256, L"SELECT MAX(bucket_start) FROM idata_1d_%u WHERE item_id=%u", target->getId(), dci->getId());
      hResult = DBSelect(hdb, query);
      if (hResult != nullptr)
      {
         if (DBGetNumRows(hResult) > 0)
         {
            int64_t maxDay = DBGetFieldInt64(hResult, 0, 0);
            if (maxDay > 0)
               rangeStart = maxDay - safetyWindowMs;
         }
         DBFreeResult(hResult);
      }
   }

   if (rangeStart <= 0)
   {
      nx_swprintf(query, 256, L"SELECT MIN(bucket_start) FROM idata_1h_%u WHERE item_id=%u", target->getId(), dci->getId());
      hResult = DBSelect(hdb, query);
      if (hResult != nullptr)
      {
         if (DBGetNumRows(hResult) > 0)
            rangeStart = FloorToBucket(DBGetFieldInt64(hResult, 0, 0), ONE_DAY_MS);
         DBFreeResult(hResult);
      }
   }

   if (rangeStart < 0)
      rangeStart = 0;
   if (rangeStart >= cutoff)
      return;

   if (!target->ensureAggregateTable(hdb, false))
      return;

   wchar_t selectSql[1024];
   BuildHourlyToDailySelect(selectSql, 1024, target->getId());

   DB_STATEMENT hSelect = DBPrepare(hdb, selectSql);
   if (hSelect == nullptr)
      return;

   DBBind(hSelect, 1, DB_SQLTYPE_INTEGER, dci->getId());
   DBBind(hSelect, 2, DB_SQLTYPE_BIGINT, rangeStart);
   DBBind(hSelect, 3, DB_SQLTYPE_BIGINT, cutoff);

   hResult = DBSelectPrepared(hSelect);
   DBFreeStatement(hSelect);
   if (hResult == nullptr)
      return;

   int rows = DBGetNumRows(hResult);
   if (rows == 0)
   {
      DBFreeResult(hResult);
      return;
   }

   wchar_t upsertSql[1024];
   BuildAggregateUpsert(upsertSql, 1024, L"1d", target->getId());
   DB_STATEMENT hUpsert = DBPrepare(hdb, upsertSql);
   if (hUpsert == nullptr)
   {
      DBFreeResult(hResult);
      return;
   }

   int upsertCount = 0;
   for(int i = 0; i < rows; i++)
   {
      int64_t bucketStart = DBGetFieldInt64(hResult, i, 0);
      int32_t sampleCount = DBGetFieldLong(hResult, i, 4);
      if (sampleCount <= 0)
         continue;

      double minValue = DBGetFieldDouble(hResult, i, 1);
      double maxValue = DBGetFieldDouble(hResult, i, 2);
      double avgValue = DBGetFieldDouble(hResult, i, 3);

      if (!ExecuteAggregateUpsert(hUpsert, dci->getId(), bucketStart, minValue, maxValue, avgValue, sampleCount))
         nxlog_debug_tag(DEBUG_TAG, 4, L"Daily UPSERT failed for DCI [%u] bucket " INT64_FMT, dci->getId(), bucketStart);
      else
         upsertCount++;
   }

   DBFreeStatement(hUpsert);
   DBFreeResult(hResult);

   nxlog_debug_tag(DEBUG_TAG, 7,
      L"Daily rollup for DCI [%u] on %s [%u]: " INT64_FMT L" -> " INT64_FMT L" (%d buckets)",
      dci->getId(), target->getName(), target->getId(), rangeStart, cutoff, upsertCount);
}

/**
 * Collect every DataCollectionTarget in the system into a SharedObjectArray.
 */
static void CollectAllDCTargets(SharedObjectArray<NetObj> *objects)
{
   g_idxAccessPointById.getObjects(objects);
   g_idxChassisById.getObjects(objects);
   g_idxClusterById.getObjects(objects);
   g_idxCollectorById.getObjects(objects);
   g_idxMobileDeviceById.getObjects(objects);
   g_idxNodeById.getObjects(objects);
   g_idxSensorById.getObjects(objects);
}

/**
 * Iterate every DCT in the system, filter for DCIs eligible for the given tier, and
 * invoke the per-DCI handler. Returns the number of DCIs processed.
 */
static int ForEachAggregationEligibleDCI(DB_HANDLE hdb, int minPollingIntervalSeconds,
         bool globalEnabled, std::function<void(DB_HANDLE, DataCollectionTarget*, DCItem*)> handler)
{
   SharedObjectArray<NetObj> objects(1024, 1024);
   CollectAllDCTargets(&objects);

   int processed = 0;
   for(int i = 0; i < objects.size(); i++)
   {
      if (IsShutdownInProgress())
         break;

      DataCollectionTarget *target = static_cast<DataCollectionTarget*>(objects.get(i));
      SharedObjectArray<DCObject> dcis = target->getDCObjectsByFilter(
         [globalEnabled, minPollingIntervalSeconds] (DCObject *dco) -> bool
         {
            if (dco->getType() != DCO_TYPE_ITEM)
               return false;
            DCItem *dci = static_cast<DCItem*>(dco);
            if (!dci->isAggregationActive(globalEnabled))
               return false;
            return dci->getEffectivePollingInterval() <= minPollingIntervalSeconds;
         });

      for(int j = 0; (j < dcis.size()) && !IsShutdownInProgress(); j++)
      {
         handler(hdb, target, static_cast<DCItem*>(dcis.get(j)));
         processed++;
      }
   }
   return processed;
}

/**
 * Advance a DCI's watermark to newWatermark, using an in-memory compare-and-swap against
 * the captured start watermark. If the hot-path pushed the watermark back during rollup,
 * we leave it alone and the next rollup run picks up the pushed-back value.
 */
static void AdvanceAggregationWatermark(DCItem *dci, int64_t expectedStart, int64_t newWatermark, DB_STATEMENT hStmt)
{
   int64_t currentInMemory = 0;
   if (!dci->tryAdvanceAggregationWatermark(expectedStart, newWatermark, &currentInMemory))
   {
      nxlog_debug_tag(DEBUG_TAG, 7,
         L"Watermark not advanced for DCI [%u]: start=" INT64_FMT L", new=" INT64_FMT L", current=" INT64_FMT,
         dci->getId(), expectedStart, newWatermark, currentInMemory);
      return;
   }

   DBBind(hStmt, 1, DB_SQLTYPE_BIGINT, newWatermark);
   DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, dci->getId());
   DBExecute(hStmt);
}

/**
 * Scheduled task handler - hourly DCI data aggregation rollup.
 *
 * Runs at :05 past each hour. For every eligible DCI:
 *   - read raw idata_<N> rows within [watermark, now - HourlyCloseWindow)
 *   - group into 1-hour buckets, skip empty buckets
 *   - UPSERT non-empty buckets into idata_1h_<N>
 *   - advance per-DCI aggregation_watermark
 *
 * No-op on TSDB (continuous aggregates handle this natively).
 */
void HourlyDataAggregationRollup(const shared_ptr<ScheduledTaskParameters>& parameters)
{
   if (!ConfigReadBoolean(L"DataCollection.Aggregation.Enabled", false))
   {
      nxlog_debug_tag(DEBUG_TAG, 7, L"Hourly rollup skipped - master switch disabled");
      return;
   }
   if (g_dbSyntax == DB_SYNTAX_TSDB)
   {
      nxlog_debug_tag(DEBUG_TAG, 7, L"Hourly rollup skipped - TSDB uses continuous aggregates");
      return;
   }
   if (g_flags & AF_SINGLE_TABLE_PERF_DATA)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"Hourly rollup skipped - single-table performance data storage is not supported by the aggregation engine");
      return;
   }

   nxlog_debug_tag(DEBUG_TAG, 4, L"Hourly rollup started");
   int64_t startTime = GetMonotonicClockTime();

   int64_t nowMs = GetCurrentTimeMs();
   int64_t closeWindowMs = static_cast<int64_t>(ConfigReadInt(L"DataCollection.Aggregation.HourlyCloseWindow", 300)) * 1000;

   // Hourly tier requires collection interval <= bucket/2 = 30 minutes.
   const int minPollingIntervalSeconds = 1800;

   int processed = 0;
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, L"UPDATE items SET aggregation_watermark=? WHERE item_id=?", true);
   if (hStmt != nullptr)
   {
      processed = ForEachAggregationEligibleDCI(hdb, minPollingIntervalSeconds, true,
         [nowMs, closeWindowMs, hStmt] (DB_HANDLE hdb, DataCollectionTarget *target, DCItem *dci) -> void
         {
            int64_t startWatermark = dci->getAggregationWatermark();
            int64_t processedThrough = RollupHourlyForDCI(hdb, target, dci, nowMs, closeWindowMs);
            if (processedThrough > 0)
               AdvanceAggregationWatermark(dci, startWatermark, processedThrough, hStmt);
         });
      DBFreeStatement(hStmt);
   }
   DBConnectionPoolReleaseConnection(hdb);

   uint32_t elapsed = static_cast<uint32_t>(GetMonotonicClockTime() - startTime);
   nxlog_debug_tag(DEBUG_TAG, 4, L"Hourly rollup completed - %d DCIs processed in %u ms", processed, elapsed);
}

/**
 * Scheduled task handler - daily DCI data aggregation rollup.
 *
 * Runs at 00:30 daily. For every eligible DCI reads from idata_1h_<N> and UPSERTs day
 * buckets into idata_1d_<N>. A sliding 7-day safety window handles late hourly backfills.
 * No-op on TSDB.
 */
void DailyDataAggregationRollup(const shared_ptr<ScheduledTaskParameters>& parameters)
{
   if (!ConfigReadBoolean(L"DataCollection.Aggregation.Enabled", false))
   {
      nxlog_debug_tag(DEBUG_TAG, 7, L"Daily rollup skipped - master switch disabled");
      return;
   }
   if (g_dbSyntax == DB_SYNTAX_TSDB)
   {
      nxlog_debug_tag(DEBUG_TAG, 7, L"Daily rollup skipped - TSDB uses continuous aggregates");
      return;
   }
   if (g_flags & AF_SINGLE_TABLE_PERF_DATA)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"Daily rollup skipped - single-table performance data storage is not supported by the aggregation engine");
      return;
   }

   nxlog_debug_tag(DEBUG_TAG, 4, L"Daily rollup started");
   int64_t startTime = GetMonotonicClockTime();

   int64_t nowMs = GetCurrentTimeMs();
   int64_t closeWindowMs = static_cast<int64_t>(ConfigReadInt(L"DataCollection.Aggregation.DailyCloseWindow", 1800)) * 1000;

   // Daily tier requires collection interval <= bucket/2 = 12 hours.
   const int minPollingIntervalSeconds = 43200;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   int processed = ForEachAggregationEligibleDCI(hdb, minPollingIntervalSeconds, true,
      [nowMs, closeWindowMs] (DB_HANDLE hdb, DataCollectionTarget *target, DCItem *dci) -> void
      {
         RollupDailyForDCI(hdb, target, dci, nowMs, closeWindowMs);
      });
   DBConnectionPoolReleaseConnection(hdb);

   uint32_t elapsed = static_cast<uint32_t>(GetMonotonicClockTime() - startTime);
   nxlog_debug_tag(DEBUG_TAG, 4, L"Daily rollup completed - %d DCIs processed in %u ms", processed, elapsed);
}

/**
 * Delete rows older than the effective retention time from a single object's aggregate
 * table. Honors per-DCI retention overrides by grouping DCIs with the same retention
 * into partitioned DELETE statements.
 */
static void PruneAggregateTable(DB_HANDLE hdb, DataCollectionTarget *target, bool hourly,
         int32_t defaultRetentionDays, int64_t nowMs)
{
   uint32_t runtimeFlag = hourly ? ODF_HAS_IDATA_1H_TABLE : ODF_HAS_IDATA_1D_TABLE;
   if (!(target->getRuntimeFlags() & runtimeFlag))
      return;

   HashMap<int, StringBuffer> groups(Ownership::True);
   SharedObjectArray<DCObject> items = target->getDCObjectsByFilter(
      [] (DCObject *dco) -> bool { return dco->getType() == DCO_TYPE_ITEM; });

   for(int i = 0; i < items.size(); i++)
   {
      DCItem *dci = static_cast<DCItem*>(items.get(i));
      int32_t override = hourly ? dci->getHourlyRetention() : dci->getDailyRetention();
      int32_t retentionDays = (override > 0) ? override : defaultRetentionDays;

      StringBuffer *group = groups.get(retentionDays);
      if (group == nullptr)
      {
         group = new StringBuffer();
         group->append(dci->getId());
         groups.set(retentionDays, group);
      }
      else
      {
         group->append(L',');
         group->append(dci->getId());
      }
   }

   if (groups.size() == 0)
      return;

   const wchar_t *suffix = hourly ? L"1h" : L"1d";
   uint32_t targetId = target->getId();
   groups.forEach(
      [hdb, suffix, targetId, nowMs] (const int retentionDays, StringBuffer *idList) -> EnumerationCallbackResult
      {
         if (retentionDays <= 0)
            return _CONTINUE;   // 0 or negative means "keep forever"

         int64_t cutoff = nowMs - static_cast<int64_t>(retentionDays) * ONE_DAY_MS;
         wchar_t query[512];
         nx_swprintf(query, 512,
            L"DELETE FROM idata_%s_%u WHERE bucket_start<" INT64_FMT L" AND item_id IN (%s)",
            suffix, targetId, cutoff, idList->cstr());
         DBQuery(hdb, query);
         return _CONTINUE;
      });
}

/**
 * Housekeeper task - prune expired rows from per-object aggregate tables.
 * Non-TSDB only; TSDB uses chunk-drop retention policies.
 */
void CleanDCIAggregates(DB_HANDLE hdb)
{
   if (!ConfigReadBoolean(L"DataCollection.Aggregation.Enabled", false))
      return;
   if (g_dbSyntax == DB_SYNTAX_TSDB)
      return;
   if (g_flags & AF_SINGLE_TABLE_PERF_DATA)
      return;

   int32_t hourlyRetention = ConfigReadInt(L"DataCollection.Aggregation.DefaultHourlyRetentionTime", 365);
   int32_t dailyRetention = ConfigReadInt(L"DataCollection.Aggregation.DefaultDailyRetentionTime", 1825);
   int64_t nowMs = GetCurrentTimeMs();

   SharedObjectArray<NetObj> objects(1024, 1024);
   CollectAllDCTargets(&objects);

   for(int i = 0; (i < objects.size()) && !IsShutdownInProgress(); i++)
   {
      DataCollectionTarget *target = static_cast<DataCollectionTarget*>(objects.get(i));
      PruneAggregateTable(hdb, target, true, hourlyRetention, nowMs);
      PruneAggregateTable(hdb, target, false, dailyRetention, nowMs);
      ThrottleHousekeeper();
   }
}

/**
 * Storage classes used for per-class continuous aggregates on TSDB. Order and
 * names must match sql/schema.in and DCObject::getStorageClassName.
 */
static constexpr DCObjectStorageClass s_tsdbStorageClasses[] = {
   DCObjectStorageClass::DEFAULT,
   DCObjectStorageClass::BELOW_7,
   DCObjectStorageClass::BELOW_30,
   DCObjectStorageClass::BELOW_90,
   DCObjectStorageClass::BELOW_180,
   DCObjectStorageClass::OTHER
};

/**
 * Detach refresh policies from all TSDB continuous aggregates. Retention
 * policies are intentionally left in place so chunks continue to age out
 * even when the master switch is off.
 */
static void DetachTSDBRefreshPolicies(DB_HANDLE hdb)
{
   wchar_t query[256];
   for(size_t i = 0; i < sizeof(s_tsdbStorageClasses) / sizeof(s_tsdbStorageClasses[0]); i++)
   {
      const wchar_t *cls = DCObject::getStorageClassName(s_tsdbStorageClasses[i]);
      nx_swprintf(query, 256, L"SELECT remove_continuous_aggregate_policy('idata_1h_sc_%ls', if_exists => true)", cls);
      DBQuery(hdb, query);
      nx_swprintf(query, 256, L"SELECT remove_continuous_aggregate_policy('idata_1d_sc_%ls', if_exists => true)", cls);
      DBQuery(hdb, query);
   }
}

/**
 * Attach refresh and retention policies to all TSDB continuous aggregates,
 * picking up current configuration values. Always remove-then-add so changes
 * to the relevant config variables land on the next server restart.
 */
static void AttachTSDBPolicies(DB_HANDLE hdb)
{
   int refreshStartOffset = ConfigReadInt(L"DataCollection.Aggregation.TSDB.RefreshStartOffset", 30);
   int refreshSchedule = ConfigReadInt(L"DataCollection.Aggregation.TSDB.RefreshScheduleInterval", 600);
   int hourlyRetention = ConfigReadInt(L"DataCollection.Aggregation.DefaultHourlyRetentionTime", 365);
   int dailyRetention = ConfigReadInt(L"DataCollection.Aggregation.DefaultDailyRetentionTime", 1825);

   wchar_t query[512];
   for(size_t i = 0; i < sizeof(s_tsdbStorageClasses) / sizeof(s_tsdbStorageClasses[0]); i++)
   {
      const wchar_t *cls = DCObject::getStorageClassName(s_tsdbStorageClasses[i]);

      nx_swprintf(query, 512, L"SELECT remove_continuous_aggregate_policy('idata_1h_sc_%ls', if_exists => true)", cls);
      DBQuery(hdb, query);
      nx_swprintf(query, 512,
         L"SELECT add_continuous_aggregate_policy('idata_1h_sc_%ls', "
         L"start_offset => INTERVAL '%d days', "
         L"end_offset => INTERVAL '1 hour', "
         L"schedule_interval => INTERVAL '%d seconds')",
         cls, refreshStartOffset, refreshSchedule);
      DBQuery(hdb, query);

      nx_swprintf(query, 512, L"SELECT remove_continuous_aggregate_policy('idata_1d_sc_%ls', if_exists => true)", cls);
      DBQuery(hdb, query);
      nx_swprintf(query, 512,
         L"SELECT add_continuous_aggregate_policy('idata_1d_sc_%ls', "
         L"start_offset => INTERVAL '%d days', "
         L"end_offset => INTERVAL '1 day', "
         L"schedule_interval => INTERVAL '%d seconds')",
         cls, refreshStartOffset, refreshSchedule);
      DBQuery(hdb, query);

      nx_swprintf(query, 512, L"SELECT remove_retention_policy('idata_1h_sc_%ls', if_exists => true)", cls);
      DBQuery(hdb, query);
      nx_swprintf(query, 512,
         L"SELECT add_retention_policy('idata_1h_sc_%ls', drop_after => INTERVAL '%d days')",
         cls, hourlyRetention);
      DBQuery(hdb, query);

      nx_swprintf(query, 512, L"SELECT remove_retention_policy('idata_1d_sc_%ls', if_exists => true)", cls);
      DBQuery(hdb, query);
      nx_swprintf(query, 512,
         L"SELECT add_retention_policy('idata_1d_sc_%ls', drop_after => INTERVAL '%d days')",
         cls, dailyRetention);
      DBQuery(hdb, query);
   }
}

/**
 * One-shot backfill that materializes all existing raw history into the
 * continuous aggregates. Runs in a worker thread because on large installs
 * it can take hours; the user-visible side effect is elevated DB load.
 */
static void BackfillTSDBAggregates()
{
   nxlog_debug_tag(DEBUG_TAG, 1, L"Backfilling TSDB continuous aggregates");
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   wchar_t query[256];
   for(size_t i = 0; i < sizeof(s_tsdbStorageClasses) / sizeof(s_tsdbStorageClasses[0]); i++)
   {
      const wchar_t *cls = DCObject::getStorageClassName(s_tsdbStorageClasses[i]);
      nx_swprintf(query, 256, L"CALL refresh_continuous_aggregate('idata_1h_sc_%ls', NULL, now())", cls);
      DBQuery(hdb, query);
   }
   for(size_t i = 0; i < sizeof(s_tsdbStorageClasses) / sizeof(s_tsdbStorageClasses[0]); i++)
   {
      const wchar_t *cls = DCObject::getStorageClassName(s_tsdbStorageClasses[i]);
      nx_swprintf(query, 256, L"CALL refresh_continuous_aggregate('idata_1d_sc_%ls', NULL, now())", cls);
      DBQuery(hdb, query);
   }
   DBConnectionPoolReleaseConnection(hdb);
   nxlog_debug_tag(DEBUG_TAG, 1, L"TSDB continuous aggregate backfill complete");
}

/**
 * Reconcile TSDB-side aggregation state with the master switch. Called once
 * during server startup. Aggregation.Enabled is need_server_restart=1 so we
 * don't track its value at runtime - flipping the switch and restarting the
 * server is the supported way to enable or disable aggregation.
 *
 * Detects the enable transition by checking whether a refresh policy is
 * already attached on idata_1h_sc_default; if not and BackfillOnEnable is
 * true, queues a one-shot backfill in a worker thread.
 */
void ReconcileTSDBAggregation()
{
   if (g_dbSyntax != DB_SYNTAX_TSDB)
      return;

   bool enabled = ConfigReadBoolean(L"DataCollection.Aggregation.Enabled", false);

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   bool policyAttached = false;
   DB_RESULT hResult = DBSelect(hdb,
      L"SELECT count(*) FROM timescaledb_information.jobs "
      L"WHERE proc_name='policy_refresh_continuous_aggregate' "
      L"AND hypertable_name='idata_1h_sc_default'");
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         policyAttached = (DBGetFieldLong(hResult, 0, 0) > 0);
      DBFreeResult(hResult);
   }

   if (enabled)
   {
      AttachTSDBPolicies(hdb);
      bool firstEnable = !policyAttached;
      if (firstEnable && ConfigReadBoolean(L"DataCollection.Aggregation.BackfillOnEnable", true))
         ThreadPoolExecute(g_mainThreadPool, BackfillTSDBAggregates);
      nxlog_debug_tag(DEBUG_TAG, 1, L"TSDB aggregation enabled%s", firstEnable ? L" (initial activation)" : L"");
   }
   else if (policyAttached)
   {
      DetachTSDBRefreshPolicies(hdb);
      nxlog_debug_tag(DEBUG_TAG, 1, L"TSDB aggregation refresh policies detached");
   }

   DBConnectionPoolReleaseConnection(hdb);
}
