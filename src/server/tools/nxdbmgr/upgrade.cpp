/*
** nxdbmgr - NetXMS database manager
** Copyright (C) 2004-2026 Victor Kirhenshtein
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
** File: upgrade.cpp
**
**/

#include "nxdbmgr.h"
#include <nxevent.h>

/**
 * Storage classes that have TimescaleDB continuous aggregates
 */
static const wchar_t *s_aggregateStorageClasses[] = { L"default", L"7", L"30", L"90", L"180", L"other" };

/**
 * Columns of continuous aggregate materialization hypertable
 */
#define AGGREGATE_COLUMNS L"item_id,bucket_start,min_value,max_value,avg_value,sample_count"

/**
 * Get qualified name of materialization hypertable for given continuous aggregate. Name is set to empty string
 * if aggregate does not exist. Optionally reports if aggregate definition has numeric value filter with dot
 * given as bracket expression (aggregate that does not exist is reported as having correct filter).
 */
static bool GetMaterializationHypertable(const wchar_t *aggregate, wchar_t *name, size_t size, bool *correctFilter = nullptr)
{
   wchar_t query[512];
   nx_swprintf(query, 512,
      L"SELECT quote_ident(materialization_hypertable_schema) || '.' || quote_ident(materialization_hypertable_name),"
      L"position('[.]' in view_definition) FROM timescaledb_information.continuous_aggregates "
      L"WHERE view_schema=current_schema() AND view_name='%ls'", aggregate);
   name[0] = 0;
   if (correctFilter != nullptr)
      *correctFilter = true;

   DB_RESULT hResult = SQLSelect(query);
   if (hResult == nullptr)
      return false;

   if (DBGetNumRows(hResult) > 0)
   {
      DBGetField(hResult, 0, 0, name, size);
      if (correctFilter != nullptr)
         *correctFilter = (DBGetFieldLong(hResult, 0, 1) > 0);
   }
   DBFreeResult(hResult);
   return true;
}

/**
 * Save materialized data of continuous aggregates that have incorrect numeric value filter into ordinary tables
 * (idata_1h_sc_<class>_saved and idata_1d_sc_<class>_saved), then drop those aggregates and create them again
 * with correct filter. Should be called within transaction.
 */
static bool RecreateAggregatesWithIncorrectFilter()
{
   bool viewsDropped = false;
   wchar_t query[1024];
   for(size_t i = 0; i < sizeof(s_aggregateStorageClasses) / sizeof(s_aggregateStorageClasses[0]); i++)
   {
      const wchar_t *cls = s_aggregateStorageClasses[i];

      wchar_t aggregate[64], hourlyData[256], dailyData[256];
      bool correctFilter;
      nx_swprintf(aggregate, 64, L"idata_1h_sc_%ls", cls);
      CHK_EXEC(GetMaterializationHypertable(aggregate, hourlyData, 256, &correctFilter));
      if (correctFilter)
         continue;
      nx_swprintf(aggregate, 64, L"idata_1d_sc_%ls", cls);
      CHK_EXEC(GetMaterializationHypertable(aggregate, dailyData, 256));

      WriteToTerminalEx(L"Rebuilding continuous aggregates for storage class \x1b[1m%ls\x1b[0m\n", cls);

      if (!viewsDropped)
      {
         CHK_EXEC(SQLQuery(L"DROP VIEW IF EXISTS idata_1h"));
         CHK_EXEC(SQLQuery(L"DROP VIEW IF EXISTS idata_1d"));
         viewsDropped = true;
      }

      nx_swprintf(query, 1024, L"CREATE TABLE idata_1h_sc_%ls_saved AS SELECT " AGGREGATE_COLUMNS L" FROM %ls", cls, hourlyData);
      CHK_EXEC(SQLQuery(query));
      if (dailyData[0] != 0)
      {
         nx_swprintf(query, 1024, L"CREATE TABLE idata_1d_sc_%ls_saved AS SELECT " AGGREGATE_COLUMNS L" FROM %ls", cls, dailyData);
         CHK_EXEC(SQLQuery(query));
      }

      // Daily aggregate is built on top of hourly one and is dropped by CASCADE
      nx_swprintf(query, 1024, L"DROP MATERIALIZED VIEW idata_1h_sc_%ls CASCADE", cls);
      CHK_EXEC(SQLQuery(query));

      nx_swprintf(query, 1024,
         L"CREATE MATERIALIZED VIEW idata_1h_sc_%ls WITH (timescaledb.continuous) AS "
         L"SELECT item_id, time_bucket(interval '1 hour', idata_timestamp) AS bucket_start, "
         L"min(idata_value::double precision) AS min_value, "
         L"max(idata_value::double precision) AS max_value, "
         L"avg(idata_value::double precision) AS avg_value, count(*) AS sample_count "
         L"FROM idata_sc_%ls "
         L"WHERE idata_value ~ '^[-+]?[0-9]*[.]?[0-9]+([eE][-+]?[0-9]+)?$' "
         L"GROUP BY item_id, time_bucket(interval '1 hour', idata_timestamp) "
         L"WITH NO DATA", cls, cls);
      CHK_EXEC(SQLQuery(query));

      nx_swprintf(query, 1024,
         L"CREATE MATERIALIZED VIEW idata_1d_sc_%ls WITH (timescaledb.continuous) AS "
         L"SELECT item_id, time_bucket(interval '1 day', bucket_start) AS bucket_start, "
         L"min(min_value) AS min_value, max(max_value) AS max_value, "
         L"sum(avg_value * sample_count) / sum(sample_count) AS avg_value, "
         L"sum(sample_count) AS sample_count "
         L"FROM idata_1h_sc_%ls "
         L"GROUP BY item_id, time_bucket(interval '1 day', bucket_start) "
         L"WITH NO DATA", cls, cls);
      CHK_EXEC(SQLQuery(query));
   }

   if (viewsDropped)
   {
      CHK_EXEC(SQLQuery(
         L"CREATE VIEW idata_1h AS "
         L"SELECT * FROM idata_1h_sc_default UNION ALL "
         L"SELECT * FROM idata_1h_sc_7 UNION ALL "
         L"SELECT * FROM idata_1h_sc_30 UNION ALL "
         L"SELECT * FROM idata_1h_sc_90 UNION ALL "
         L"SELECT * FROM idata_1h_sc_180 UNION ALL "
         L"SELECT * FROM idata_1h_sc_other"));
      CHK_EXEC(SQLQuery(
         L"CREATE VIEW idata_1d AS "
         L"SELECT * FROM idata_1d_sc_default UNION ALL "
         L"SELECT * FROM idata_1d_sc_7 UNION ALL "
         L"SELECT * FROM idata_1d_sc_30 UNION ALL "
         L"SELECT * FROM idata_1d_sc_90 UNION ALL "
         L"SELECT * FROM idata_1d_sc_180 UNION ALL "
         L"SELECT * FROM idata_1d_sc_other"));
   }
   return true;
}

/**
 * Move saved materialized data back into continuous aggregates of given storage class. Should be called within
 * transaction. Hourly rows are restored up to and including given boundary bucket, replacing rows that were
 * materialized from raw data; daily rows are restored up to (not including) day of boundary bucket.
 */
static bool MoveSavedAggregateData(const wchar_t *cls, int64_t boundary)
{
   wchar_t aggregate[64], hourlyData[256], dailyData[256];
   nx_swprintf(aggregate, 64, L"idata_1h_sc_%ls", cls);
   CHK_EXEC(GetMaterializationHypertable(aggregate, hourlyData, 256));
   nx_swprintf(aggregate, 64, L"idata_1d_sc_%ls", cls);
   CHK_EXEC(GetMaterializationHypertable(aggregate, dailyData, 256));
   if ((hourlyData[0] == 0) || (dailyData[0] == 0))
   {
      WriteToTerminalEx(L"Continuous aggregates for storage class %ls are missing\n", cls);
      return false;
   }

   wchar_t query[1024];
   nx_swprintf(query, 1024, L"DELETE FROM %ls m USING idata_1h_sc_%ls_saved s WHERE m.item_id=s.item_id AND m.bucket_start=s.bucket_start", hourlyData, cls);
   CHK_EXEC(SQLQuery(query));

   nx_swprintf(query, 1024,
      L"INSERT INTO %ls (" AGGREGATE_COLUMNS L") SELECT " AGGREGATE_COLUMNS L" FROM idata_1h_sc_%ls_saved "
      L"WHERE bucket_start <= to_timestamp(" INT64_FMT L")", hourlyData, cls, boundary);
   CHK_EXEC(SQLQuery(query));

   nx_swprintf(aggregate, 64, L"idata_1d_sc_%ls_saved", cls);
   int rc = DBIsTableExist(g_dbHandle, aggregate);
   if (rc == DBIsTableExist_Failure)
      return false;
   if (rc == DBIsTableExist_Found)
   {
      nx_swprintf(query, 1024,
         L"INSERT INTO %ls (" AGGREGATE_COLUMNS L") SELECT " AGGREGATE_COLUMNS L" FROM idata_1d_sc_%ls_saved "
         L"WHERE bucket_start < time_bucket(interval '1 day', to_timestamp(" INT64_FMT L"))", dailyData, cls, boundary);
      CHK_EXEC(SQLQuery(query));

      nx_swprintf(query, 1024, L"DROP TABLE idata_1d_sc_%ls_saved", cls);
      CHK_EXEC(SQLQuery(query));
   }

   nx_swprintf(query, 1024, L"DROP TABLE idata_1h_sc_%ls_saved", cls);
   CHK_EXEC(SQLQuery(query));
   return true;
}

/**
 * Restore saved materialized data of continuous aggregates for given storage class (does nothing if there is
 * no saved data). Should be called outside of transaction.
 *
 * Just created continuous aggregate is considered invalid in whole time range, so any refresh replaces
 * materialized data within refresh window with data calculated from raw values - which would delete restored
 * rows for the time range where raw values are already dropped by retention. To avoid that, range up to oldest
 * raw value is refreshed while aggregates are still empty, and saved rows are inserted after that.
 */
static bool RestoreAggregateData(const wchar_t *cls)
{
   wchar_t query[1024];
   nx_swprintf(query, 1024, L"idata_1h_sc_%ls_saved", cls);
   int rc = DBIsTableExist(g_dbHandle, query);
   if (rc == DBIsTableExist_Failure)
      return false;
   if (rc != DBIsTableExist_Found)
      return true;

   // Start of hourly bucket with oldest raw value. That bucket is likely to be only partially covered by raw data.
   nx_swprintf(query, 1024, L"SELECT CAST(extract(epoch FROM time_bucket(interval '1 hour', coalesce(min(idata_timestamp), now()))) AS bigint) FROM idata_sc_%ls", cls);
   DB_RESULT hResult = SQLSelect(query);
   if (hResult == nullptr)
      return false;
   int64_t boundary = DBGetFieldInt64(hResult, 0, 0);
   DBFreeResult(hResult);

   nx_swprintf(query, 1024, L"CALL refresh_continuous_aggregate('idata_1h_sc_%ls', NULL, to_timestamp(" INT64_FMT L"))", cls, boundary + 3600);
   CHK_EXEC_NO_SP(SQLQuery(query));
   nx_swprintf(query, 1024, L"CALL refresh_continuous_aggregate('idata_1d_sc_%ls', NULL, time_bucket(interval '1 day', to_timestamp(" INT64_FMT L")))", cls, boundary);
   CHK_EXEC_NO_SP(SQLQuery(query));

   CHK_EXEC_NO_SP(DBBegin(g_dbHandle));
   if (!MoveSavedAggregateData(cls, boundary))
   {
      DBRollback(g_dbHandle);
      return false;
   }
   return DBCommit(g_dbHandle);
}

/**
 * Rebuild TimescaleDB continuous aggregates which were created with numeric value filter that accepts any
 * character in place of decimal dot, preserving already materialized data. Must be called outside of
 * transaction because continuous aggregate refresh cannot be executed within transaction block. Can be called
 * again after failure: aggregates are saved and created anew in single transaction, and presence of saved data
 * tables indicates that data still has to be restored.
 */
bool RebuildAggregatesWithIncorrectNumericFilter()
{
   CHK_EXEC_NO_SP(DBBegin(g_dbHandle));
   if (!RecreateAggregatesWithIncorrectFilter())
   {
      DBRollback(g_dbHandle);
      return false;
   }
   CHK_EXEC_NO_SP(DBCommit(g_dbHandle));

   for(size_t i = 0; i < sizeof(s_aggregateStorageClasses) / sizeof(s_aggregateStorageClasses[0]); i++)
   {
      if (!RestoreAggregateData(s_aggregateStorageClasses[i]))
         return false;
   }
   return true;
}

/**
 * Externals
 */
bool MajorSchemaUpgrade_V21();
bool MajorSchemaUpgrade_V22();
bool MajorSchemaUpgrade_V30();
bool MajorSchemaUpgrade_V31();
bool MajorSchemaUpgrade_V32();
bool MajorSchemaUpgrade_V33();
bool MajorSchemaUpgrade_V34();
bool MajorSchemaUpgrade_V35();
bool MajorSchemaUpgrade_V36();
bool MajorSchemaUpgrade_V37();
bool MajorSchemaUpgrade_V38();
bool MajorSchemaUpgrade_V39();
bool MajorSchemaUpgrade_V40();
bool MajorSchemaUpgrade_V41();
bool MajorSchemaUpgrade_V42();
bool MajorSchemaUpgrade_V43();
bool MajorSchemaUpgrade_V44();
bool MajorSchemaUpgrade_V45();
bool MajorSchemaUpgrade_V46();
bool MajorSchemaUpgrade_V50();
bool MajorSchemaUpgrade_V51();
bool MajorSchemaUpgrade_V52();
bool MajorSchemaUpgrade_V53();
bool MajorSchemaUpgrade_V60();
bool MajorSchemaUpgrade_V61();
bool MajorSchemaUpgrade_V62();
bool MajorSchemaUpgrade_V70();
bool MajorSchemaUpgrade_V71();

/**
 * Move to next major version of DB schema
 */
bool SetMajorSchemaVersion(int32_t nextMajor, int32_t nextMinor)
{
   int32_t currMajor, currMinor;
   if (!DBGetSchemaVersion(g_dbHandle, &currMajor, &currMinor))
      return false;

   wchar_t var[64], query[256];
   _sntprintf(var, 64, _T("SchemaVersionLevel.%d"), currMajor);
   if (IsDatabaseRecordExist(g_dbHandle, L"metadata", L"var_name", var))
      _sntprintf(query, 256, _T("UPDATE metadata SET var_value='%d' WHERE var_name='%s'"), currMinor, var);
   else
      _sntprintf(query, 256, _T("INSERT INTO metadata (var_name,var_value) VALUES ('%s','%d')"), var, currMinor);
   if (!SQLQuery(query))
      return false;

   _sntprintf(query, 256, _T("UPDATE metadata SET var_value='%d' WHERE var_name='SchemaVersionMajor'"), nextMajor);
   if (!SQLQuery(query))
      return false;

   _sntprintf(query, 256, _T("UPDATE metadata SET var_value='%d' WHERE var_name='SchemaVersionMinor'"), nextMinor);
   if (!SQLQuery(query))
      return false;

   return true;
}

/**
 * Move to next minor version of DB schema
 */
bool SetMinorSchemaVersion(int32_t nextMinor)
{
   return SQLQueryFormatted(L"UPDATE metadata SET var_value='%d' WHERE var_name='SchemaVersionMinor'", nextMinor);
}

/**
 * Get last used schema level for given major version
 */
int32_t GetSchemaLevelForMajorVersion(int32_t major)
{
   wchar_t var[64];
   _sntprintf(var, 64, _T("SchemaVersionLevel.%d"), major);
   int32_t minor = DBMgrMetaDataReadInt32(var, -1);
   if (minor != -1)
      return minor;

   // Missing minor version for requested major could mean two different scenarios:
   // 1. Initial installation was on newer version
   // 2. We didn't upgrade up to that major version yet
   // Attempt to deduce most recent major version by scanning all SchemaVersionLevel.* variables
   DB_RESULT hResult = SQLSelect(_T("SELECT var_name FROM metadata WHERE var_name LIKE 'SchemaVersionLevel.%'"));
   if (hResult == nullptr)
      return -1;  // Unexpected SQL error, assume that changes for requested major version was not made

   int32_t lastMajor = 0;
   int count = DBGetNumRows(hResult);
   for(int i = 0; i < count; i++)
   {
      TCHAR name[256];
      DBGetField(hResult, i, 0, name, 256);
      int32_t version = _tcstol(&name[19], nullptr, 10);
      if (version > lastMajor)
         lastMajor = version;
   }
   DBFreeResult(hResult);

   // Presence of SchemaVersionLevel for major version higher than requested indicates that
   // all changes for requested version was already included at database initialization
   return (lastMajor > major) ? INT_MAX : -1;
}

/**
 * Set last used schema level for given major version
 */
bool SetSchemaLevelForMajorVersion(int32_t major, int32_t level)
{
   wchar_t var[64], query[256];
   _sntprintf(var, 64, _T("SchemaVersionLevel.%d"), major);
   if (IsDatabaseRecordExist(g_dbHandle, _T("metadata"), _T("var_name"), var))
      _sntprintf(query, 256, _T("UPDATE metadata SET var_value='%d' WHERE var_name='%s'"), level, var);
   else
      _sntprintf(query, 256, _T("INSERT INTO metadata (var_name,var_value) VALUES ('%s','%d')"), var, level);
   return SQLQuery(query);
}

/**
 * Upgrade map
 */
static struct
{
   int majorVersion;
   bool (* upgradeProc)();
} s_dbUpgradeMap[] =
{
   { 21, MajorSchemaUpgrade_V21 },
   { 22, MajorSchemaUpgrade_V22 },
   { 30, MajorSchemaUpgrade_V30 },
   { 31, MajorSchemaUpgrade_V31 },
   { 32, MajorSchemaUpgrade_V32 },
   { 33, MajorSchemaUpgrade_V33 },
   { 34, MajorSchemaUpgrade_V34 },
   { 35, MajorSchemaUpgrade_V35 },
   { 36, MajorSchemaUpgrade_V36 },
   { 37, MajorSchemaUpgrade_V37 },
   { 38, MajorSchemaUpgrade_V38 },
   { 39, MajorSchemaUpgrade_V39 },
   { 40, MajorSchemaUpgrade_V40 },
   { 41, MajorSchemaUpgrade_V41 },
   { 42, MajorSchemaUpgrade_V42 },
   { 43, MajorSchemaUpgrade_V43 },
   { 44, MajorSchemaUpgrade_V44 },
   { 45, MajorSchemaUpgrade_V45 },
   { 46, MajorSchemaUpgrade_V46 },
   { 50, MajorSchemaUpgrade_V50 },
   { 51, MajorSchemaUpgrade_V51 },
   { 52, MajorSchemaUpgrade_V52 },
   { 53, MajorSchemaUpgrade_V53 },
   { 60, MajorSchemaUpgrade_V60 },
   { 61, MajorSchemaUpgrade_V61 },
   { 62, MajorSchemaUpgrade_V62 },
   { 70, MajorSchemaUpgrade_V70 },
   { 71, MajorSchemaUpgrade_V71 },
   { 0, nullptr }
};

/**
 * Upgrade database to new version
 */
bool UpgradeDatabase()
{
   WriteToTerminal(L"Upgrading database...\n");

   // Stale TimescaleDB extension registration breaks DDL statements with error that does not indicate real cause
   if (!ValidateTimescaleDBExtension() && !g_ignoreErrors)
   {
      WriteToTerminal(L"Database upgrade aborted\n");
      return false;
   }

   // Get database format version
   int32_t major, minor;
	if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
	{
	   WriteToTerminal(L"Unable to determine database schema version\n");
	   return false;
	}

   if (major == 0)
   {
      WriteToTerminal(
         L"Your database has pre-2.1 schema (major version 0), which is no longer supported by this tool.\n"
         L"Install NetXMS 6.1 first, run `nxdbmgr upgrade` there to bring the schema to a supported version,\n"
         L"then upgrade to the current NetXMS version.\n");
      return false;
   }

   if ((major == DB_SCHEMA_VERSION_MAJOR) && (minor == DB_SCHEMA_VERSION_MINOR))
   {
      WriteToTerminal(L"Core database schema is up to date\n");
      bool modSuccess = UpgradeModuleSchemas();
      WriteToTerminalEx(L"Database upgrade %s\n", modSuccess ? L"succeeded" : L"failed");
      return modSuccess;
   }

   if ((major > DB_SCHEMA_VERSION_MAJOR) || ((major == DB_SCHEMA_VERSION_MAJOR) && (minor > DB_SCHEMA_VERSION_MINOR)))
   {
       _tprintf(_T("Your database has core schema version %d.%d, this tool is compiled for version %d.%d.\n")
                   _T("You need to upgrade your server before using this database.\n"),
                major, minor, DB_SCHEMA_VERSION_MAJOR, DB_SCHEMA_VERSION_MINOR);
       return false;
   }

   // Check if database is locked
   bool locked = false;
   DB_RESULT hResult = DBSelect(g_dbHandle, L"SELECT var_value FROM config WHERE var_name='DBLockStatus'");
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         TCHAR buffer[MAX_DB_STRING];
         locked = _tcscmp(DBGetField(hResult, 0, 0, buffer, MAX_DB_STRING), _T("UNLOCKED"));
      }
      DBFreeResult(hResult);
   }
   if (locked)
   {
      WriteToTerminal(L"Database is locked\n");
      return false;
   }

   while(major < DB_SCHEMA_VERSION_MAJOR)
   {
      // Find upgrade procedure
      int i;
      for(i = 0; s_dbUpgradeMap[i].upgradeProc != nullptr; i++)
         if (s_dbUpgradeMap[i].majorVersion == major)
            break;
      if (s_dbUpgradeMap[i].upgradeProc == nullptr)
      {
         WriteToTerminalEx(L"Unable to find upgrade procedure for major version %d\n", major);
         break;
      }
      if (!s_dbUpgradeMap[i].upgradeProc())
         break;
      if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
      {
         WriteToTerminal(L"Internal error\n");
         break;
      }
   }

   if ((major == DB_SCHEMA_VERSION_MAJOR) && (minor < DB_SCHEMA_VERSION_MINOR))
   {
      // Find upgrade procedure
      int i;
      for(i = 0; s_dbUpgradeMap[i].upgradeProc != nullptr; i++)
         if (s_dbUpgradeMap[i].majorVersion == major)
            break;
      if (s_dbUpgradeMap[i].upgradeProc != nullptr)
      {
         s_dbUpgradeMap[i].upgradeProc();
         if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
            _tprintf(_T("Internal error\n"));
      }
      else
      {
         _tprintf(_T("Unable to find upgrade procedure for major version %d\n"), major);
      }
   }

   bool success = (major == DB_SCHEMA_VERSION_MAJOR) && (minor == DB_SCHEMA_VERSION_MINOR) && UpgradeModuleSchemas();
   WriteToTerminalEx(L"Database upgrade %s\n", success ? L"succeeded" : L"failed");
   return success;
}
