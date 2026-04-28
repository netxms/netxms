/*
** nxdbmgr - NetXMS database manager
** Copyright (C) 2022-2026 Raden Solutions
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
** File: upgrade_v62.cpp
**
**/

#include "nxdbmgr.h"
#include <nxevent.h>
#include <nxtools.h>

/**
 * Create per-storage-class hourly and daily continuous aggregates plus the
 * matching union views (TSDB only). Mirrors the DDL in sql/schema.in.
 */
static bool CreateAggregateCAGGsForStorageClass(const wchar_t *cls)
{
   wchar_t query[1024];

   nx_swprintf(query, 1024,
      L"CREATE MATERIALIZED VIEW idata_1h_sc_%ls WITH (timescaledb.continuous) AS "
      L"SELECT item_id, time_bucket(interval '1 hour', idata_timestamp) AS bucket_start, "
      L"min(idata_value::double precision) AS min_value, "
      L"max(idata_value::double precision) AS max_value, "
      L"avg(idata_value::double precision) AS avg_value, count(*) AS sample_count "
      L"FROM idata_sc_%ls "
      L"WHERE idata_value ~ '^[-+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?$' "
      L"GROUP BY item_id, time_bucket(interval '1 hour', idata_timestamp) "
      L"WITH NO DATA", cls, cls);
   if (!SQLQuery(query))
      return false;

   nx_swprintf(query, 1024,
      L"CREATE MATERIALIZED VIEW idata_1d_sc_%ls WITH (timescaledb.continuous) AS "
      L"SELECT item_id, time_bucket(interval '1 day', bucket_start) AS bucket_start, "
      L"min(min_value) AS min_value, max(max_value) AS max_value, "
      L"sum(avg_value * sample_count) / sum(sample_count) AS avg_value, "
      L"sum(sample_count) AS sample_count "
      L"FROM idata_1h_sc_%ls "
      L"GROUP BY item_id, time_bucket(interval '1 day', bucket_start) "
      L"WITH NO DATA", cls, cls);
   if (!SQLQuery(query))
      return false;
   return true;
}

/**
 * Upgrade from 62.8 to 62.9
 */
static bool H_UpgradeFromV8()
{
   if (g_dbSyntax == DB_SYNTAX_TSDB)
   {
      // Hierarchical CAGGs (daily-over-hourly) require TimescaleDB >= 2.10.
      // Fail the upgrade on older versions so the admin upgrades TimescaleDB
      // and re-runs nxdbmgr; never bump the schema version with the CAGGs missing.
      DB_RESULT hResult = SQLSelect(L"SELECT extversion FROM pg_extension WHERE extname='timescaledb'");
      if (hResult == nullptr)
         return false;

      bool versionOk = false;
      char version[64] = "";
      if (DBGetNumRows(hResult) > 0)
      {
         DBGetFieldA(hResult, 0, 0, version, 64);
         int major, minor;
         if (sscanf(version, "%d.%d.", &major, &minor) == 2)
            versionOk = (major > 2) || ((major == 2) && (minor >= 10));
      }
      DBFreeResult(hResult);

      if (!versionOk)
      {
         WriteToTerminalEx(L"\x1b[31mTimescaleDB %hs detected; DCI data aggregation requires TimescaleDB 2.10 or later. Upgrade TimescaleDB and re-run nxdbmgr.\x1b[0m\n",
            (version[0] != 0) ? version : "(unknown)");
         return false;
      }

      static const wchar_t *storageClasses[] = { L"default", L"7", L"30", L"90", L"180", L"other" };
      for(size_t i = 0; i < sizeof(storageClasses) / sizeof(storageClasses[0]); i++)
      {
         if (!CreateAggregateCAGGsForStorageClass(storageClasses[i]))
            return false;
      }

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

   CHK_EXEC(SetMinorSchemaVersion(9));
   return true;
}

/**
 * Upgrade from 62.7 to 62.8
 */
static bool H_UpgradeFromV7()
{
   CHK_EXEC(CreateConfigParam(L"Server.Security.PasswordHash.MemoryCostKB",
            L"65536",
            L"Argon2id memory cost in KiB used when hashing user passwords. Higher values strengthen resistance to GPU/ASIC cracking at the cost of login latency and server memory.",
            L"KB", 'I', true, false, false, false));
   CHK_EXEC(CreateConfigParam(L"Server.Security.PasswordHash.TimeCost",
            L"3",
            L"Argon2id iteration count used when hashing user passwords.",
            nullptr, 'I', true, false, false, false));
   CHK_EXEC(CreateConfigParam(L"Server.Security.PasswordHash.Parallelism",
            L"1",
            L"Argon2id parallelism (lanes) used when hashing user passwords.",
            nullptr, 'I', true, false, false, false));

   // Users still on legacy unsalted SHA-1 hashes cannot be migrated to Argon2id at-rest
   // (the plaintext is needed). Force a password change on next login so dormant accounts
   // do not remain on unsalted SHA-1 indefinitely.
   if ((g_dbSyntax == DB_SYNTAX_DB2) || (g_dbSyntax == DB_SYNTAX_INFORMIX) || (g_dbSyntax == DB_SYNTAX_ORACLE))
   {
      CHK_EXEC(SQLQuery(L"UPDATE users SET flags=flags+8 WHERE BITAND(flags,8)=0 AND password NOT LIKE '$%' AND password<>''"));
   }
   else if (g_dbSyntax == DB_SYNTAX_MSSQL)
   {
      CHK_EXEC(SQLQuery(L"UPDATE users SET flags=flags+8 WHERE (CAST(flags AS bigint) & CAST(8 AS bigint))=0 AND password NOT LIKE '$%' AND password<>''"));
   }
   else
   {
      CHK_EXEC(SQLQuery(L"UPDATE users SET flags=flags+8 WHERE (flags & 8)=0 AND password NOT LIKE '$%' AND password<>''"));
   }

   CHK_EXEC(SetMinorSchemaVersion(8));
   return true;
}

/**
 * Upgrade from 62.6 to 62.7
 */
static bool H_UpgradeFromV6()
{
   // Per-object idata/tdata table and index creation statements are now
   // emitted from code (see src/server/include/dci_table_creation.h),
   // not read from metadata rows. Drop the now-unused template rows.
   static const wchar_t *batch =
      L"DELETE FROM metadata WHERE var_name='IDataTableCreationCommand'\n"
      L"DELETE FROM metadata WHERE var_name LIKE 'IDataIndexCreationCommand_%'\n"
      L"DELETE FROM metadata WHERE var_name LIKE 'TDataTableCreationCommand_%'\n"
      L"DELETE FROM metadata WHERE var_name LIKE 'TDataIndexCreationCommand_%'\n"
      L"<END>";
   CHK_EXEC(SQLBatch(batch));

   CHK_EXEC(SetMinorSchemaVersion(7));
   return true;
}

/**
 * Upgrade from 62.5 to 62.6
 */
static bool H_UpgradeFromV5()
{
   static const wchar_t *batch =
      L"ALTER TABLE items ADD aggregation_mode char(1)\n"
      L"ALTER TABLE items ADD hourly_retention integer\n"
      L"ALTER TABLE items ADD daily_retention integer\n"
      L"ALTER TABLE items ADD aggregation_watermark $SQL:INT64\n"
      L"UPDATE items SET aggregation_mode='0'\n"
      L"<END>";
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, L"items", L"aggregation_mode"));

   CHK_EXEC(CreateConfigParam(L"DataCollection.Aggregation.Enabled",
            L"0",
            L"Master switch for DCI data aggregation. When enabled, the server rolls up raw DCI values into hourly and daily aggregates for eligible items.",
            nullptr, 'B', true, true, false, false));
   CHK_EXEC(CreateConfigParam(L"DataCollection.Aggregation.DefaultHourlyRetentionTime",
            L"365",
            L"Default retention time for hourly DCI aggregates. Individual DCIs can override this setting.",
            L"days", 'I', true, false, false, false));
   CHK_EXEC(CreateConfigParam(L"DataCollection.Aggregation.DefaultDailyRetentionTime",
            L"1825",
            L"Default retention time for daily DCI aggregates. Individual DCIs can override this setting.",
            L"days", 'I', true, false, false, false));
   CHK_EXEC(CreateConfigParam(L"DataCollection.Aggregation.HourlyCloseWindow",
            L"300",
            L"Lag in seconds before a closed hour is rolled up into the hourly aggregate, giving late samples time to arrive.",
            L"seconds", 'I', true, false, false, false));
   CHK_EXEC(CreateConfigParam(L"DataCollection.Aggregation.DailyCloseWindow",
            L"1800",
            L"Lag in seconds before a closed day is rolled up into the daily aggregate, giving late samples time to arrive.",
            L"seconds", 'I', true, false, false, false));
   CHK_EXEC(CreateConfigParam(L"DataCollection.Aggregation.BackfillOnEnable",
            L"1",
            L"When enabling aggregation, initialize per-DCI watermarks to the earliest retained raw timestamp so existing history is backfilled on the next rollup pass.",
            nullptr, 'B', true, false, false, false));
   CHK_EXEC(CreateConfigParam(L"DataCollection.Aggregation.MaxAutoSelectPoints",
            L"5000",
            L"Upper bound on the number of points returned by auto-selected aggregate tier when serving DCI history queries.",
            nullptr, 'I', true, false, false, false));
   CHK_EXEC(CreateConfigParam(L"DataCollection.Aggregation.TSDB.RefreshStartOffset",
            L"30",
            L"TimescaleDB continuous aggregate refresh lookback window. Caps the outage length that can be recovered via late-arriving data on TSDB backends.",
            L"days", 'I', true, false, false, false));
   CHK_EXEC(CreateConfigParam(L"DataCollection.Aggregation.TSDB.RefreshScheduleInterval",
            L"600",
            L"TimescaleDB continuous aggregate refresh cadence.",
            L"seconds", 'I', true, false, false, false));

   CHK_EXEC(SetMinorSchemaVersion(6));
   return true;
}

/**
 * Upgrade from 62.4 to 62.5
 */
static bool H_UpgradeFromV4()
{
   if (GetSchemaLevelForMajorVersion(61) < 36)
   {
      // Per-DCI idata tables created after the 6.0 upgrade have only
      // PRIMARY KEY(item_id, idata_timestamp), which cannot serve queries
      // filtering/sorting by idata_timestamp alone. Restore the secondary
      // (idata_timestamp, item_id) index for MySQL, PostgreSQL (non-TSDB),
      // and MSSQL, and backfill it on existing idata_* tables.
      if ((g_dbSyntax == DB_SYNTAX_MYSQL) || (g_dbSyntax == DB_SYNTAX_PGSQL) || (g_dbSyntax == DB_SYNTAX_MSSQL))
      {
         CHK_EXEC(DBMgrMetaDataWriteStr(L"IDataIndexCreationCommand_0", L"CREATE INDEX idx_idata_%u_timestamp_id ON idata_%u(idata_timestamp,item_id)"));

         IntegerArray<uint32_t> targets = GetDataCollectionTargets();
         for(int i = 0; i < targets.size(); i++)
         {
            uint32_t id = targets.get(i);
            wchar_t tableName[64], indexName[64];
            nx_swprintf(tableName, 64, L"idata_%u", id);
            nx_swprintf(indexName, 64, L"idx_idata_%u_timestamp_id", id);

            if (DBIsTableExist(g_dbHandle, tableName) != DBIsTableExist_Found)
               continue;

            // Skip if the index already exists (e.g. on pre-6.0 PostgreSQL tables, which
            // already have this exact index). Attempting CREATE INDEX here would fail and
            // on PostgreSQL would poison the entire upgrade transaction.
            if (IsIndexExists(tableName, indexName))
               continue;

            wchar_t query[256];
            nx_swprintf(query, 256, L"CREATE INDEX %s ON %s(idata_timestamp,item_id)", indexName, tableName);
            WriteToTerminalEx(L"Indexing table \x1b[1m%s\x1b[0m...\n", tableName);
            CHK_EXEC(SQLQuery(query));
         }
      }
      CHK_EXEC(SetSchemaLevelForMajorVersion(61, 36));
   }
   CHK_EXEC(SetMinorSchemaVersion(5));
   return true;
}

/**
 * Upgrade from 62.3 to 62.4
 */
static bool H_UpgradeFromV3()
{
   CHK_EXEC(CreateConfigParam(L"DebugConsole.AllowedScriptLocations",
            L"@library",
            L"Ordered, comma-separated list of locations searched by the debug console \"exec\" command. Use @library for the server script library or an absolute directory path (non-recursive). Empty value disables \"exec\".",
            nullptr, 'S', true, false, false, false));
   CHK_EXEC(SetMinorSchemaVersion(4));
   return true;
}

/**
 * Upgrade from 62.2 to 62.3
 */
static bool H_UpgradeFromV2()
{
   // Add SYSTEM_ACCESS_VIEW_NOTIFICATION_LOG (bit 55 = 0x80000000000000 = 36028797018963968)
   // and SYSTEM_ACCESS_VIEW_ACTION_LOG (bit 56 = 0x100000000000000 = 72057594037927936) to Admins group
   // (combined mask = 0x180000000000000 = 108086391056891904)
   if ((g_dbSyntax == DB_SYNTAX_DB2) || (g_dbSyntax == DB_SYNTAX_INFORMIX) || (g_dbSyntax == DB_SYNTAX_ORACLE))
   {
      CHK_EXEC(SQLQuery(L"UPDATE user_groups SET system_access=system_access+108086391056891904 WHERE id=1073741825 AND BITAND(system_access, 108086391056891904)=0"));
   }
   else if (g_dbSyntax == DB_SYNTAX_MSSQL)
   {
      CHK_EXEC(SQLQuery(L"UPDATE user_groups SET system_access=system_access+108086391056891904 WHERE id=1073741825 AND (CAST(system_access AS bigint) & CAST(108086391056891904 AS bigint))=0"));
   }
   else
   {
      CHK_EXEC(SQLQuery(L"UPDATE user_groups SET system_access=system_access+108086391056891904 WHERE id=1073741825 AND (system_access & 108086391056891904)=0"));
   }

   CHK_EXEC(SetMinorSchemaVersion(3));
   return true;
}

/**
 * Upgrade from 62.1 to 62.2
 */
static bool H_UpgradeFromV1()
{
   static const wchar_t *batch =
      L"ALTER TABLE object_tools ADD applicable_classes integer\n"
      L"UPDATE object_tools SET applicable_classes=1\n"
      L"<END>";
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, L"object_tools", L"applicable_classes"));
   CHK_EXEC(SetMinorSchemaVersion(2));
   return true;
}

/**
 * Upgrade from 62.0 to 62.1
 */
static bool H_UpgradeFromV0()
{
   CHK_EXEC(CreateConfigParam(L"Scripts.RestrictWriteAccess",
            L"1",
            L"Restrict write access for transformation, filter, predicate, and analysis scripts (DCI transformations, autobind filters, thresholds, conditions, RCA, etc.). When enabled, such scripts cannot modify objects.",
            nullptr, 'B', true, false, false, false));
   CHK_EXEC(SetMinorSchemaVersion(1));
   return true;
}

/**
 * Upgrade map
 */
static struct
{
   int version;
   int nextMajor;
   int nextMinor;
   bool (*upgradeProc)();
} s_dbUpgradeMap[] = {
   { 8,  62, 9,  H_UpgradeFromV8  },
   { 7,  62, 8,  H_UpgradeFromV7  },
   { 6,  62, 7,  H_UpgradeFromV6  },
   { 5,  62, 6,  H_UpgradeFromV5  },
   { 4,  62, 5,  H_UpgradeFromV4  },
   { 3,  62, 4,  H_UpgradeFromV3  },
   { 2,  62, 3,  H_UpgradeFromV2  },
   { 1,  62, 2,  H_UpgradeFromV1  },
   { 0,  62, 1,  H_UpgradeFromV0  },
   { 0,  0,  0,  nullptr }
};

/**
 * Upgrade database to new version
 */
bool MajorSchemaUpgrade_V62()
{
   int32_t major, minor;
   if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
      return false;

   while ((major == 62) && (minor < DB_SCHEMA_VERSION_V62_MINOR))
   {
      // Find upgrade procedure
      int i;
      for (i = 0; s_dbUpgradeMap[i].upgradeProc != nullptr; i++)
         if (s_dbUpgradeMap[i].version == minor)
            break;
      if (s_dbUpgradeMap[i].upgradeProc == nullptr)
      {
         WriteToTerminalEx(L"Unable to find upgrade procedure for version 62.%d\n", minor);
         return false;
      }
      WriteToTerminalEx(L"Upgrading from version 62.%d to %d.%d\n", minor, s_dbUpgradeMap[i].nextMajor, s_dbUpgradeMap[i].nextMinor);
      DBBegin(g_dbHandle);
      if (s_dbUpgradeMap[i].upgradeProc())
      {
         DBCommit(g_dbHandle);
         if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
            return false;
      }
      else
      {
         WriteToTerminal(L"Rolling back last stage due to upgrade errors...\n");
         DBRollback(g_dbHandle);
         return false;
      }
   }
   return true;
}
