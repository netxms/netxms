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
** File: dci_data_query.cpp
**
**/

#include "nxcore.h"

#define SELECTION_COLUMNS (historicalDataType != HDT_RAW) ? tablePrefix : _T(""), (historicalDataType == HDT_RAW_AND_PROCESSED) ? _T("_value,raw_value") : ((historicalDataType == HDT_PROCESSED) || (historicalDataType == HDT_FULL_TABLE)) ?  _T("_value") : _T("raw_value")

/**
 * Prepare statement for reading data from idata/tdata table
 */
DB_STATEMENT NXCORE_EXPORTABLE PrepareDataSelect(DB_HANDLE hdb, uint32_t nodeId, int dciType, DCObjectStorageClass storageClass,
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
            nxlog_write(NXLOG_WARNING, _T("INTERNAL ERROR: unsupported database in PrepareDataSelect"));
            return nullptr;
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
            nxlog_write(NXLOG_WARNING, _T("INTERNAL ERROR: unsupported database in PrepareDataSelect"));
            return nullptr;
      }
   }
   return DBPrepare(hdb, query);
}

/**
 * Predicate that excludes idata rows whose value cannot be cast to a number.
 * A numeric DCI may still have empty-string rows (e.g. failed collections), and strict
 * dialects (PostgreSQL, MSSQL, MySQL strict mode, Oracle, DB2) error on those casts.
 * Mirrors the regex used by the TSDB continuous aggregates in sql/dbinit_tsdb.sql.
 */
static inline const TCHAR *GetIdataValueNumericFilter()
{
   switch(g_dbSyntax)
   {
      case DB_SYNTAX_PGSQL:
      case DB_SYNTAX_TSDB:
         return _T(" AND idata_value ~ '^[-+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?$'");
      case DB_SYNTAX_MYSQL:
         return _T(" AND idata_value REGEXP '^[-+]?[0-9]*\\\\.?[0-9]+([eE][-+]?[0-9]+)?$'");
      case DB_SYNTAX_ORACLE:
      case DB_SYNTAX_DB2:
         return _T(" AND REGEXP_LIKE(idata_value, '^[-+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?$')");
      case DB_SYNTAX_MSSQL:
         return _T(" AND TRY_CAST(idata_value AS float) IS NOT NULL");
      default:
         return _T(" AND idata_value<>''");
   }
}

/**
 * Prepare statement for reading aggregated data from idata table (numeric DCIs only).
 * Returns time-bucketed AVG/MIN/MAX values. Bucket size is formatted directly into the
 * query string (server-computed value, no injection risk).
 */
DB_STATEMENT NXCORE_EXPORTABLE PrepareAggregatedDataSelect(DB_HANDLE hdb, uint32_t nodeId, DCObjectStorageClass storageClass, int64_t bucketSizeMs, const TCHAR *condition)
{
   const TCHAR *numericFilter = GetIdataValueNumericFilter();
   TCHAR query[1024];
   if (g_flags & AF_SINGLE_TABLE_PERF_DATA)
   {
      switch(g_dbSyntax)
      {
         case DB_SYNTAX_TSDB:
            _sntprintf(query, 1024,
               _T("SELECT timestamptz_to_ms(time_bucket('") INT64_FMT _T(" milliseconds',idata_timestamp)),")
               _T("AVG(idata_value::double precision),MIN(idata_value::double precision),MAX(idata_value::double precision) ")
               _T("FROM idata_sc_%s WHERE item_id=?%s%s GROUP BY 1 ORDER BY 1 DESC"),
               bucketSizeMs, DCObject::getStorageClassName(storageClass), condition, numericFilter);
            break;
         case DB_SYNTAX_PGSQL:
         case DB_SYNTAX_SQLITE:
            _sntprintf(query, 1024,
               _T("SELECT (idata_timestamp/") INT64_FMT _T(")*") INT64_FMT _T(",")
               _T("AVG(CAST(idata_value AS double precision)),MIN(CAST(idata_value AS double precision)),MAX(CAST(idata_value AS double precision)) ")
               _T("FROM idata WHERE item_id=?%s%s GROUP BY 1 ORDER BY 1 DESC"),
               bucketSizeMs, bucketSizeMs, condition, numericFilter);
            break;
         case DB_SYNTAX_MYSQL:
            _sntprintf(query, 1024,
               _T("SELECT (idata_timestamp DIV ") INT64_FMT _T(")*") INT64_FMT _T(",")
               _T("AVG(CAST(idata_value AS DECIMAL(30,10))),MIN(CAST(idata_value AS DECIMAL(30,10))),MAX(CAST(idata_value AS DECIMAL(30,10))) ")
               _T("FROM idata WHERE item_id=?%s%s GROUP BY 1 ORDER BY 1 DESC"),
               bucketSizeMs, bucketSizeMs, condition, numericFilter);
            break;
         case DB_SYNTAX_MSSQL:
            _sntprintf(query, 1024,
               _T("SELECT (idata_timestamp/") INT64_FMT _T(")*") INT64_FMT _T(",")
               _T("AVG(CAST(idata_value AS float)),MIN(CAST(idata_value AS float)),MAX(CAST(idata_value AS float)) ")
               _T("FROM idata WHERE item_id=?%s%s GROUP BY (idata_timestamp/") INT64_FMT _T(")*") INT64_FMT _T(" ORDER BY 1 DESC"),
               bucketSizeMs, bucketSizeMs, condition, numericFilter, bucketSizeMs, bucketSizeMs);
            break;
         case DB_SYNTAX_ORACLE:
            _sntprintf(query, 1024,
               _T("SELECT TRUNC(idata_timestamp/") INT64_FMT _T(")*") INT64_FMT _T(",")
               _T("AVG(TO_NUMBER(idata_value)),MIN(TO_NUMBER(idata_value)),MAX(TO_NUMBER(idata_value)) ")
               _T("FROM idata WHERE item_id=?%s%s GROUP BY TRUNC(idata_timestamp/") INT64_FMT _T(")*") INT64_FMT _T(" ORDER BY 1 DESC"),
               bucketSizeMs, bucketSizeMs, condition, numericFilter, bucketSizeMs, bucketSizeMs);
            break;
         case DB_SYNTAX_DB2:
            _sntprintf(query, 1024,
               _T("SELECT (idata_timestamp/") INT64_FMT _T(")*") INT64_FMT _T(",")
               _T("AVG(CAST(idata_value AS DOUBLE)),MIN(CAST(idata_value AS DOUBLE)),MAX(CAST(idata_value AS DOUBLE)) ")
               _T("FROM idata WHERE item_id=?%s%s GROUP BY (idata_timestamp/") INT64_FMT _T(")*") INT64_FMT _T(" ORDER BY 1 DESC"),
               bucketSizeMs, bucketSizeMs, condition, numericFilter, bucketSizeMs, bucketSizeMs);
            break;
         default:
            nxlog_write(NXLOG_WARNING, _T("INTERNAL ERROR: unsupported database in PrepareAggregatedDataSelect"));
            return nullptr;
      }
   }
   else
   {
      switch(g_dbSyntax)
      {
         case DB_SYNTAX_PGSQL:
         case DB_SYNTAX_SQLITE:
         case DB_SYNTAX_TSDB:
            _sntprintf(query, 1024,
               _T("SELECT (idata_timestamp/") INT64_FMT _T(")*") INT64_FMT _T(",")
               _T("AVG(CAST(idata_value AS double precision)),MIN(CAST(idata_value AS double precision)),MAX(CAST(idata_value AS double precision)) ")
               _T("FROM idata_%u WHERE item_id=?%s%s GROUP BY 1 ORDER BY 1 DESC"),
               bucketSizeMs, bucketSizeMs, nodeId, condition, numericFilter);
            break;
         case DB_SYNTAX_MYSQL:
            _sntprintf(query, 1024,
               _T("SELECT (idata_timestamp DIV ") INT64_FMT _T(")*") INT64_FMT _T(",")
               _T("AVG(CAST(idata_value AS DECIMAL(30,10))),MIN(CAST(idata_value AS DECIMAL(30,10))),MAX(CAST(idata_value AS DECIMAL(30,10))) ")
               _T("FROM idata_%u WHERE item_id=?%s%s GROUP BY 1 ORDER BY 1 DESC"),
               bucketSizeMs, bucketSizeMs, nodeId, condition, numericFilter);
            break;
         case DB_SYNTAX_MSSQL:
            _sntprintf(query, 1024,
               _T("SELECT (idata_timestamp/") INT64_FMT _T(")*") INT64_FMT _T(",")
               _T("AVG(CAST(idata_value AS float)),MIN(CAST(idata_value AS float)),MAX(CAST(idata_value AS float)) ")
               _T("FROM idata_%u WHERE item_id=?%s%s GROUP BY (idata_timestamp/") INT64_FMT _T(")*") INT64_FMT _T(" ORDER BY 1 DESC"),
               bucketSizeMs, bucketSizeMs, nodeId, condition, numericFilter, bucketSizeMs, bucketSizeMs);
            break;
         case DB_SYNTAX_ORACLE:
            _sntprintf(query, 1024,
               _T("SELECT TRUNC(idata_timestamp/") INT64_FMT _T(")*") INT64_FMT _T(",")
               _T("AVG(TO_NUMBER(idata_value)),MIN(TO_NUMBER(idata_value)),MAX(TO_NUMBER(idata_value)) ")
               _T("FROM idata_%u WHERE item_id=?%s%s GROUP BY TRUNC(idata_timestamp/") INT64_FMT _T(")*") INT64_FMT _T(" ORDER BY 1 DESC"),
               bucketSizeMs, bucketSizeMs, nodeId, condition, numericFilter, bucketSizeMs, bucketSizeMs);
            break;
         case DB_SYNTAX_DB2:
            _sntprintf(query, 1024,
               _T("SELECT (idata_timestamp/") INT64_FMT _T(")*") INT64_FMT _T(",")
               _T("AVG(CAST(idata_value AS DOUBLE)),MIN(CAST(idata_value AS DOUBLE)),MAX(CAST(idata_value AS DOUBLE)) ")
               _T("FROM idata_%u WHERE item_id=?%s%s GROUP BY (idata_timestamp/") INT64_FMT _T(")*") INT64_FMT _T(" ORDER BY 1 DESC"),
               bucketSizeMs, bucketSizeMs, nodeId, condition, numericFilter, bucketSizeMs, bucketSizeMs);
            break;
         default:
            nxlog_write(NXLOG_WARNING, _T("INTERNAL ERROR: unsupported database in PrepareAggregatedDataSelect"));
            return nullptr;
      }
   }
   return DBPrepare(hdb, query);
}

/**
 * Build the SELECT column list for a tier read, given the requested aggregation function.
 * MINMAX returns two columns (min,max); single-function modes return one. sample_count is
 * always appended as the trailing column so the client can surface it in tooltips.
 */
static const wchar_t* GetTierColumnList(DciAggregationFunction function)
{
   switch (function)
   {
      case DCI_HAGG_MIN:
         return L"min_value,sample_count";
      case DCI_HAGG_MAX:
         return L"max_value,sample_count";
      case DCI_HAGG_MINMAX:
         return L"min_value,max_value,sample_count";
      case DCI_HAGG_AVG:
      default:
         return L"avg_value,sample_count";
   }
}

/**
 * Prepare statement for reading aggregated data from a tier table (idata_1h_<N> / idata_1d_<N>
 * on non-TSDB; idata_1h / idata_1d unified views on TSDB).
 *
 * `condition` carries the same bucket_start range predicates the caller produces for the raw
 * path, with `bucket_start` substituted for `idata_timestamp` (TSDB wraps via ms_to_timestamptz).
 *
 * Returns rows: (bucket_start_ms, value) for AVG/MIN/MAX or (bucket_start_ms, min, max) for MINMAX,
 * ordered by bucket_start DESC, capped at maxRows.
 */
DB_STATEMENT NXCORE_EXPORTABLE PrepareTieredDataSelect(DB_HANDLE hdb, uint32_t nodeId, DciTier tier,
         DciAggregationFunction function, uint32_t maxRows, const wchar_t *condition)
{
   if ((tier != DCI_TIER_HOURLY) && (tier != DCI_TIER_DAILY))
   {
      nxlog_write(NXLOG_WARNING, L"INTERNAL ERROR: PrepareTieredDataSelect called with non-tier value");
      return nullptr;
   }

   const wchar_t *suffix = (tier == DCI_TIER_HOURLY) ? L"1h" : L"1d";
   const wchar_t *columns = GetTierColumnList(function);
   wchar_t query[1024];

   if (g_dbSyntax == DB_SYNTAX_TSDB)
   {
      // TSDB unified views (idata_1h / idata_1d) expose bucket_start as timestamptz.
      _sntprintf(query, 1024,
         L"SELECT timestamptz_to_ms(bucket_start),%ls FROM idata_%ls WHERE item_id=?%ls "
         L"ORDER BY bucket_start DESC LIMIT %u",
         columns, suffix, condition, maxRows);
   }
   else
   {
      switch(g_dbSyntax)
      {
         case DB_SYNTAX_MSSQL:
            _sntprintf(query, 1024,
               L"SELECT TOP %u bucket_start,%ls FROM idata_%ls_%u WHERE item_id=?%ls "
               L"ORDER BY bucket_start DESC",
               maxRows, columns, suffix, nodeId, condition);
            break;
         case DB_SYNTAX_ORACLE:
            _sntprintf(query, 1024,
               L"SELECT * FROM (SELECT bucket_start,%ls FROM idata_%ls_%u WHERE item_id=?%ls "
               L"ORDER BY bucket_start DESC) WHERE ROWNUM<=%u",
               columns, suffix, nodeId, condition, maxRows);
            break;
         case DB_SYNTAX_MYSQL:
         case DB_SYNTAX_PGSQL:
         case DB_SYNTAX_SQLITE:
            _sntprintf(query, 1024,
               L"SELECT bucket_start,%ls FROM idata_%ls_%u WHERE item_id=?%ls "
               L"ORDER BY bucket_start DESC LIMIT %u",
               columns, suffix, nodeId, condition, maxRows);
            break;
         case DB_SYNTAX_DB2:
            _sntprintf(query, 1024,
               L"SELECT bucket_start,%ls FROM idata_%ls_%u WHERE item_id=?%ls "
               L"ORDER BY bucket_start DESC FETCH FIRST %u ROWS ONLY",
               columns, suffix, nodeId, condition, maxRows);
            break;
         default:
            nxlog_write(NXLOG_WARNING, L"INTERNAL ERROR: unsupported database in PrepareTieredDataSelect");
            return nullptr;
      }
   }
   return DBPrepare(hdb, query);
}

/**
 * Resolve DciTier from caller request to the actual tier the dispatcher will read.
 *
 * AUTO heuristic: pick the densest tier whose point count fits within autoSelectThreshold.
 * Explicit tiers are honored, with two safety downgrades:
 *   - String/table DCIs always read raw (aggregates are numeric only)
 *   - Non-TSDB tier read where the per-object aggregate table doesn't exist falls back to raw
 *
 * runtimeFlags is the data collection target's runtime flag bitmask (used to check
 * ODF_HAS_IDATA_1H_TABLE / ODF_HAS_IDATA_1D_TABLE on non-TSDB). On TSDB the unified views
 * always exist when aggregation is enabled, so the flags are not consulted.
 */
DciTier NXCORE_EXPORTABLE ResolveDciTier(DciTier requested, const DCObject& dci, int dciType, int64_t timeFrom, int64_t timeTo,
         uint32_t runtimeFlags, int autoSelectThreshold)
{
   // Aggregates are numeric single-value DCIs only
   if (dciType != DCO_TYPE_ITEM)
      return DCI_TIER_RAW;
   if (static_cast<const DCItem&>(dci).getTransformedDataType() == DCI_DT_STRING)
      return DCI_TIER_RAW;

   auto hasTierTable = [&](DciTier tier) -> bool
   {
      if (g_dbSyntax == DB_SYNTAX_TSDB)
         return true;   // Unified views always present when feature is initialized
      uint32_t flag = (tier == DCI_TIER_HOURLY) ? ODF_HAS_IDATA_1H_TABLE : ODF_HAS_IDATA_1D_TABLE;
      return (runtimeFlags & flag) != 0;
   };

   if (requested == DCI_TIER_RAW)
      return DCI_TIER_RAW;
   if (requested == DCI_TIER_HOURLY)
      return hasTierTable(DCI_TIER_HOURLY) ? DCI_TIER_HOURLY : DCI_TIER_RAW;
   if (requested == DCI_TIER_DAILY)
      return hasTierTable(DCI_TIER_DAILY) ? DCI_TIER_DAILY : DCI_TIER_RAW;

   // AUTO: choose the densest tier whose point count fits the threshold.
   // Time arithmetic on the wire is in milliseconds.
   if ((timeFrom <= 0) || (timeTo <= timeFrom))
      return DCI_TIER_RAW;

   int64_t spanMs = timeTo - timeFrom;
   int32_t pollingInterval = dci.getEffectivePollingInterval();
   if (pollingInterval <= 0)
      pollingInterval = 60;   // Defensive: guard against div-by-zero with sensible default

   int64_t pointsRaw = spanMs / (static_cast<int64_t>(pollingInterval) * 1000);
   if (pointsRaw <= autoSelectThreshold)
      return DCI_TIER_RAW;

   int64_t pointsHourly = spanMs / (3600LL * 1000);
   if ((pointsHourly <= autoSelectThreshold) && hasTierTable(DCI_TIER_HOURLY))
      return DCI_TIER_HOURLY;

   if (hasTierTable(DCI_TIER_DAILY))
      return DCI_TIER_DAILY;
   if (hasTierTable(DCI_TIER_HOURLY))
      return DCI_TIER_HOURLY;
   return DCI_TIER_RAW;
}
