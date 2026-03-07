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
 * Prepare statement for reading aggregated data from idata table (numeric DCIs only).
 * Returns time-bucketed AVG/MIN/MAX values. Bucket size is formatted directly into the
 * query string (server-computed value, no injection risk).
 */
DB_STATEMENT NXCORE_EXPORTABLE PrepareAggregatedDataSelect(DB_HANDLE hdb, uint32_t nodeId, DCObjectStorageClass storageClass, int64_t bucketSizeMs, const TCHAR *condition)
{
   TCHAR query[1024];
   if (g_flags & AF_SINGLE_TABLE_PERF_DATA)
   {
      switch(g_dbSyntax)
      {
         case DB_SYNTAX_TSDB:
            _sntprintf(query, 1024,
               _T("SELECT timestamptz_to_ms(time_bucket('") INT64_FMT _T(" milliseconds',idata_timestamp)),")
               _T("AVG(idata_value::double precision),MIN(idata_value::double precision),MAX(idata_value::double precision) ")
               _T("FROM idata_sc_%s WHERE item_id=?%s GROUP BY 1 ORDER BY 1 DESC"),
               bucketSizeMs, DCObject::getStorageClassName(storageClass), condition);
            break;
         case DB_SYNTAX_PGSQL:
         case DB_SYNTAX_SQLITE:
            _sntprintf(query, 1024,
               _T("SELECT (idata_timestamp/") INT64_FMT _T(")*") INT64_FMT _T(",")
               _T("AVG(CAST(idata_value AS double precision)),MIN(CAST(idata_value AS double precision)),MAX(CAST(idata_value AS double precision)) ")
               _T("FROM idata WHERE item_id=?%s GROUP BY 1 ORDER BY 1 DESC"),
               bucketSizeMs, bucketSizeMs, condition);
            break;
         case DB_SYNTAX_MYSQL:
            _sntprintf(query, 1024,
               _T("SELECT (idata_timestamp DIV ") INT64_FMT _T(")*") INT64_FMT _T(",")
               _T("AVG(CAST(idata_value AS DECIMAL(30,10))),MIN(CAST(idata_value AS DECIMAL(30,10))),MAX(CAST(idata_value AS DECIMAL(30,10))) ")
               _T("FROM idata WHERE item_id=?%s GROUP BY 1 ORDER BY 1 DESC"),
               bucketSizeMs, bucketSizeMs, condition);
            break;
         case DB_SYNTAX_MSSQL:
            _sntprintf(query, 1024,
               _T("SELECT (idata_timestamp/") INT64_FMT _T(")*") INT64_FMT _T(",")
               _T("AVG(CAST(idata_value AS float)),MIN(CAST(idata_value AS float)),MAX(CAST(idata_value AS float)) ")
               _T("FROM idata WHERE item_id=?%s GROUP BY (idata_timestamp/") INT64_FMT _T(")*") INT64_FMT _T(" ORDER BY 1 DESC"),
               bucketSizeMs, bucketSizeMs, condition, bucketSizeMs, bucketSizeMs);
            break;
         case DB_SYNTAX_ORACLE:
            _sntprintf(query, 1024,
               _T("SELECT TRUNC(idata_timestamp/") INT64_FMT _T(")*") INT64_FMT _T(",")
               _T("AVG(TO_NUMBER(idata_value)),MIN(TO_NUMBER(idata_value)),MAX(TO_NUMBER(idata_value)) ")
               _T("FROM idata WHERE item_id=?%s GROUP BY TRUNC(idata_timestamp/") INT64_FMT _T(")*") INT64_FMT _T(" ORDER BY 1 DESC"),
               bucketSizeMs, bucketSizeMs, condition, bucketSizeMs, bucketSizeMs);
            break;
         case DB_SYNTAX_DB2:
            _sntprintf(query, 1024,
               _T("SELECT (idata_timestamp/") INT64_FMT _T(")*") INT64_FMT _T(",")
               _T("AVG(CAST(idata_value AS DOUBLE)),MIN(CAST(idata_value AS DOUBLE)),MAX(CAST(idata_value AS DOUBLE)) ")
               _T("FROM idata WHERE item_id=?%s GROUP BY (idata_timestamp/") INT64_FMT _T(")*") INT64_FMT _T(" ORDER BY 1 DESC"),
               bucketSizeMs, bucketSizeMs, condition, bucketSizeMs, bucketSizeMs);
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
               _T("FROM idata_%u WHERE item_id=?%s GROUP BY 1 ORDER BY 1 DESC"),
               bucketSizeMs, bucketSizeMs, nodeId, condition);
            break;
         case DB_SYNTAX_MYSQL:
            _sntprintf(query, 1024,
               _T("SELECT (idata_timestamp DIV ") INT64_FMT _T(")*") INT64_FMT _T(",")
               _T("AVG(CAST(idata_value AS DECIMAL(30,10))),MIN(CAST(idata_value AS DECIMAL(30,10))),MAX(CAST(idata_value AS DECIMAL(30,10))) ")
               _T("FROM idata_%u WHERE item_id=?%s GROUP BY 1 ORDER BY 1 DESC"),
               bucketSizeMs, bucketSizeMs, nodeId, condition);
            break;
         case DB_SYNTAX_MSSQL:
            _sntprintf(query, 1024,
               _T("SELECT (idata_timestamp/") INT64_FMT _T(")*") INT64_FMT _T(",")
               _T("AVG(CAST(idata_value AS float)),MIN(CAST(idata_value AS float)),MAX(CAST(idata_value AS float)) ")
               _T("FROM idata_%u WHERE item_id=?%s GROUP BY (idata_timestamp/") INT64_FMT _T(")*") INT64_FMT _T(" ORDER BY 1 DESC"),
               bucketSizeMs, bucketSizeMs, nodeId, condition, bucketSizeMs, bucketSizeMs);
            break;
         case DB_SYNTAX_ORACLE:
            _sntprintf(query, 1024,
               _T("SELECT TRUNC(idata_timestamp/") INT64_FMT _T(")*") INT64_FMT _T(",")
               _T("AVG(TO_NUMBER(idata_value)),MIN(TO_NUMBER(idata_value)),MAX(TO_NUMBER(idata_value)) ")
               _T("FROM idata_%u WHERE item_id=?%s GROUP BY TRUNC(idata_timestamp/") INT64_FMT _T(")*") INT64_FMT _T(" ORDER BY 1 DESC"),
               bucketSizeMs, bucketSizeMs, nodeId, condition, bucketSizeMs, bucketSizeMs);
            break;
         case DB_SYNTAX_DB2:
            _sntprintf(query, 1024,
               _T("SELECT (idata_timestamp/") INT64_FMT _T(")*") INT64_FMT _T(",")
               _T("AVG(CAST(idata_value AS DOUBLE)),MIN(CAST(idata_value AS DOUBLE)),MAX(CAST(idata_value AS DOUBLE)) ")
               _T("FROM idata_%u WHERE item_id=?%s GROUP BY (idata_timestamp/") INT64_FMT _T(")*") INT64_FMT _T(" ORDER BY 1 DESC"),
               bucketSizeMs, bucketSizeMs, nodeId, condition, bucketSizeMs, bucketSizeMs);
            break;
         default:
            nxlog_write(NXLOG_WARNING, _T("INTERNAL ERROR: unsupported database in PrepareAggregatedDataSelect"));
            return nullptr;
      }
   }
   return DBPrepare(hdb, query);
}
