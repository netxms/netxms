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
 * Check whether an index exists on the given table. Only supports MySQL, PostgreSQL, MSSQL.
 */
static bool IDataIndexExists(const wchar_t *tableName, const wchar_t *indexName)
{
   wchar_t query[512];
   switch (g_dbSyntax)
   {
      case DB_SYNTAX_MYSQL:
         nx_swprintf(query, 512,
               L"SELECT 1 FROM information_schema.STATISTICS WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='%s' AND INDEX_NAME='%s'",
               tableName, indexName);
         break;
      case DB_SYNTAX_PGSQL:
         nx_swprintf(query, 512,
               L"SELECT 1 FROM pg_indexes WHERE schemaname=current_schema() AND tablename='%s' AND indexname='%s'",
               tableName, indexName);
         break;
      case DB_SYNTAX_MSSQL:
         nx_swprintf(query, 512,
               L"SELECT 1 FROM sys.indexes WHERE name='%s' AND object_id=OBJECT_ID('%s')",
               indexName, tableName);
         break;
      default:
         return false;
   }

   DB_RESULT hResult = SQLSelect(query);
   if (hResult == nullptr)
      return false;
   bool exists = DBGetNumRows(hResult) > 0;
   DBFreeResult(hResult);
   return exists;
}

/**
 * Upgrade from 62.4 to 62.5
 */
static bool H_UpgradeFromV4()
{
   // Per-DCI idata tables created after the 6.0 upgrade have only
   // PRIMARY KEY(item_id, idata_timestamp), which cannot serve queries
   // filtering/sorting by idata_timestamp alone. Restore the secondary
   // (idata_timestamp, item_id) index for MySQL, PostgreSQL (non-TSDB),
   // and MSSQL, and backfill it on existing idata_* tables.
   if ((g_dbSyntax == DB_SYNTAX_MYSQL) || (g_dbSyntax == DB_SYNTAX_PGSQL) || (g_dbSyntax == DB_SYNTAX_MSSQL))
   {
      CHK_EXEC(DBMgrMetaDataWriteStr(L"IDataIndexCreationCommand_0",
            L"CREATE INDEX idx_idata_%d_timestamp_id ON idata_%d(idata_timestamp,item_id)"));

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
         if (IDataIndexExists(tableName, indexName))
            continue;

         wchar_t query[256];
         nx_swprintf(query, 256, L"CREATE INDEX %s ON %s(idata_timestamp,item_id)", indexName, tableName);
         CHK_EXEC(SQLQuery(query));
      }
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
