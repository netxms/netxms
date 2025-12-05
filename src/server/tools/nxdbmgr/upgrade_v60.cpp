/*
** nxdbmgr - NetXMS database manager
** Copyright (C) 2022-2025 Raden Solutions
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
** File: upgrade_v60.cpp
**
**/

#include "nxdbmgr.h"
#include <nxevent.h>

void DeleteDuplicateDataRecords(const wchar_t *tableType, uint32_t objectId);

/**
 * Add primary key for data tables for objects selected by given query
 */
static bool ConvertDataTables(const wchar_t *query, bool dataTablesWithPK)
{
   DB_RESULT hResult = SQLSelect(query);
   if (hResult == nullptr)
      return false;

   bool success = true;
   int count = DBGetNumRows(hResult);
   for(int i = 0; i < count; i++)
   {
      uint32_t id = DBGetFieldULong(hResult, i, 0);

      if (IsDataTableExist(L"idata_%u", id))
      {
         WriteToTerminalEx(L"Converting table \x1b[1midata_%u\x1b[0m\n", id);

         wchar_t table[64];
         nx_swprintf(table, 64, L"idata_%u", id);

         if (dataTablesWithPK)
         {
            CHK_EXEC_NO_SP(DBDropPrimaryKey(g_dbHandle, table));
         }
         else
         {
            DeleteDuplicateDataRecords(L"idata", id);

            wchar_t index[64];
            nx_swprintf(index, 64, L"idx_idata_%u_id_timestamp", id);
            DBDropIndex(g_dbHandle, table, index);
         }

         CHK_EXEC_NO_SP(ConvertColumnToInt64(table, L"idata_timestamp"));
         CHK_EXEC_NO_SP(SQLQueryFormatted(L"UPDATE %s SET idata_timestamp = idata_timestamp * 1000", table));
         DBAddPrimaryKey(g_dbHandle,table, _T("item_id,idata_timestamp"));
      }

      if (IsDataTableExist(L"tdata_%u", id))
      {
         WriteToTerminalEx(L"Converting table \x1b[1mtdata_%u\x1b[0m\n", id);

         wchar_t table[64];
         nx_swprintf(table, 64, L"tdata_%u", id);

         if (dataTablesWithPK)
         {
            CHK_EXEC_NO_SP(DBDropPrimaryKey(g_dbHandle, table));
         }
         else
         {
            DeleteDuplicateDataRecords(L"tdata", id);

            wchar_t index[64];
            nx_swprintf(index, 64, L"idx_tdata_%u", id);
            DBDropIndex(g_dbHandle, table, index);
         }

         CHK_EXEC_NO_SP(ConvertColumnToInt64(table, L"tdata_timestamp"));
         CHK_EXEC_NO_SP(SQLQueryFormatted(L"UPDATE %s SET tdata_timestamp = tdata_timestamp * 1000", table));
         DBAddPrimaryKey(g_dbHandle,table, L"item_id,tdata_timestamp");
      }
   }
   DBFreeResult(hResult);
   return success;
}

/**
 * Add primary key to all data tables for given object class
 */
static bool ConvertDataTablesForClass(const wchar_t *className, bool dataTablesWithPK)
{
   wchar_t query[1024];
   nx_swprintf(query, 256, L"SELECT id FROM %s", className);
   return ConvertDataTables(query, dataTablesWithPK);
}

/**
 * Upgrade from 60.7 to 60.8
 */
static bool H_UpgradeFromV7()
{
   if ((g_dbSyntax == DB_SYNTAX_PGSQL) || (g_dbSyntax == DB_SYNTAX_TSDB))
   {
      CHK_EXEC(SQLQuery(
         L"CREATE OR REPLACE FUNCTION ms_to_timestamptz(ms_timestamp BIGINT) "
         L"RETURNS TIMESTAMPTZ AS $$ "
         L"    SELECT timestamptz 'epoch' + ms_timestamp * interval '1 millisecond'; "
         L"$$ LANGUAGE sql IMMUTABLE;"));
      CHK_EXEC(SQLQuery(
         L"CREATE OR REPLACE FUNCTION timestamptz_to_ms(t timestamptz) "
         L"RETURNS BIGINT AS $$ "
         L"    SELECT (EXTRACT(epoch FROM t) * 1000)::bigint; "
         L"$$ LANGUAGE sql IMMUTABLE;"));
   }

   // Convert raw_dci_values timestamp columns to INT64 milliseconds
   CHK_EXEC(ConvertColumnToInt64(L"raw_dci_values", L"last_poll_time"));
   CHK_EXEC(ConvertColumnToInt64(L"raw_dci_values", L"cache_timestamp"));
   CHK_EXEC(SQLQuery(L"UPDATE raw_dci_values SET last_poll_time = last_poll_time * 1000, cache_timestamp = CASE WHEN cache_timestamp > 1 THEN cache_timestamp * 1000 ELSE cache_timestamp END"));

   // Check if idata_/tdata_ tables already use PKs
   bool dataTablesWithPK = false;
   if (g_dbSyntax == DB_SYNTAX_PGSQL)
   {
      if (IsOnlineUpgradePending())
      {
         WriteToTerminal(L"Pending online upgrades must be completed before this step\n");
         return false;
      }

      int schemaV52 = GetSchemaLevelForMajorVersion(52);
      if (((schemaV52 >= 21) && (schemaV52 < 24)) || (DBMgrMetaDataReadInt32(L"PostgreSQL.DataTablesHavePK", 0) == 1))
      {
         // idata/tdata tables already converted to PKs
         dataTablesWithPK = true;
      }
   }

   // Convert idata/tdata timestamp columns to INT64 milliseconds
   if (g_dbSyntax != DB_SYNTAX_TSDB)
   {
      CHK_EXEC(ConvertColumnToInt64(L"idata", L"idata_timestamp"));
      CHK_EXEC(ConvertColumnToInt64(L"tdata", L"tdata_timestamp"));
      CHK_EXEC(SQLBatch(
         _T("UPDATE idata SET idata_timestamp = idata_timestamp * 1000\n")
         _T("UPDATE tdata SET tdata_timestamp = tdata_timestamp * 1000\n")
         _T("<END>")));
   }

   // Update metadata for idata_nnn/tdata_nnn table creation
   wchar_t query[256];
   nx_swprintf(query, 256, L"CREATE TABLE idata_%%d (item_id integer not null,idata_timestamp %s not null,idata_value varchar(255) null,raw_value varchar(255) null)", GetSQLTypeName(SQL_TYPE_INT64));
   CHK_EXEC(DBMgrMetaDataWriteStr(L"IDataTableCreationCommand", query));
   nx_swprintf(query, 256, L"CREATE TABLE tdata_%%d (item_id integer not null,tdata_timestamp %s not null,tdata_value %s null)",
         GetSQLTypeName(SQL_TYPE_INT64), GetSQLTypeName(SQL_TYPE_TEXT));
   CHK_EXEC(DBMgrMetaDataWriteStr(L"TDataTableCreationCommand_0", query));

   // Convert all idata_nnn/tdata_nnn tables to INT64 milliseconds
   CHK_EXEC_NO_SP(ConvertDataTablesForClass(L"nodes", dataTablesWithPK));
   CHK_EXEC_NO_SP(ConvertDataTablesForClass(L"clusters", dataTablesWithPK));
   CHK_EXEC_NO_SP(ConvertDataTablesForClass(L"mobile_devices", dataTablesWithPK));
   CHK_EXEC_NO_SP(ConvertDataTablesForClass(L"access_points", dataTablesWithPK));
   CHK_EXEC_NO_SP(ConvertDataTablesForClass(L"chassis", dataTablesWithPK));
   CHK_EXEC_NO_SP(ConvertDataTablesForClass(L"sensors", dataTablesWithPK));
   CHK_EXEC_NO_SP(ConvertDataTables(L"SELECT id FROM object_containers WHERE object_class=29 OR object_class=30", dataTablesWithPK));

   CHK_EXEC(SetMinorSchemaVersion(8));
   return true;
}

/**
 * Upgrade from 60.6 to 60.7
 */
static bool H_UpgradeFromV6()
{
   CHK_EXEC(CreateEventTemplate(EVENT_INTERFACE_UNMANAGED, _T("SYS_IF_UNMANAGED"),
         EVENT_SEVERITY_NORMAL, EF_LOG, _T("dd449241-1886-41e3-a279-9a084ff64cec"),
         _T("Interface \"%<interfaceName>\" changed state to UNMANAGED (IP Addr: %<interfaceIpAddress>/%<interfaceNetMask>, IfIndex: %<interfaceIndex>)"),
         _T("Generated when interface status changed to unmanaged.\r\n")
         _T("Parameters:\r\n")
         _T("   1) interfaceObjectId - Interface object ID\r\n")
         _T("   2) interfaceName - Interface name\r\n")
         _T("   3) interfaceIpAddress - Interface IP address\r\n")
         _T("   4) interfaceNetMask - Interface netmask\r\n")
         _T("   5) interfaceIndex - Interface index")
      ));

   CHK_EXEC(SetMinorSchemaVersion(7));
   return true;
}

/**
 * Upgrade from 60.5 to 60.6
 */
static bool H_UpgradeFromV5()
{
   static const TCHAR *batch =
      _T("ALTER TABLE nodes ADD last_events varchar(255)\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   CHK_EXEC(SetMinorSchemaVersion(6));
   return true;
}

/**
 * Upgrade from 60.4 to 60.5
 */
static bool H_UpgradeFromV4()
{
   static const TCHAR *batch =
      _T("ALTER TABLE interfaces ADD max_speed $SQL:INT64\n")
      _T("UPDATE interfaces SET max_speed=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("interfaces"), _T("max_speed")));

   CHK_EXEC(SQLQuery(L"UPDATE event_cfg SET "
      L"description='Generated when system detects interface speed change.\r\n"
      L"Parameters:\r\n"
      L"   1) ifIndex - Interface index\r\n"
      L"   2) ifName - Interface name\r\n"
      L"   3) oldSpeed - Old speed in bps\r\n"
      L"   4) oldSpeedText - Old speed in bps with optional multiplier (kbps, Mbps, etc.)\r\n"
      L"   5) newSpeed - New speed in bps\r\n"
      L"   6) newSpeedText - New speed in bps with optional multiplier (kbps, Mbps, etc.)\r\n"
      L"   7) maxSpeed - Maximum speed in bps\r\n"
      L"   8) maxSpeedText - Maximum speed in bps with optional multiplier (kbps, Mbps, etc.)'"
      L" WHERE event_code=141"));   // SYS_IF_SPEED_CHANGED

   CHK_EXEC(CreateEventTemplate(EVENT_IF_SPEED_BELOW_MAXIMUM, _T("SYS_IF_SPEED_BELOW_MAXIMUM"),
         EVENT_SEVERITY_WARNING, EF_LOG, _T("c020c587-ab9f-4834-ba3c-91583c2871b1"),
         _T("Interface %<ifName> speed %<speedText> is below maximum %<maxSpeedText>"),
         _T("Generated when system detects that interface speed is below maximum.\r\n")
         _T("Parameters:\r\n")
         _T("   1) ifIndex - Interface index\r\n")
         _T("   2) ifName - Interface name\r\n")
         _T("   3) speed - Current speed in bps\r\n")
         _T("   4) speedText - Current speed in bps with optional multiplier (kbps, Mbps, etc.)\r\n")
         _T("   5) maxSpeed - Maximum speed in bps\r\n")
         _T("   6) maxSpeedText - Maximum speed in bps with optional multiplier (kbps, Mbps, etc.)")
      ));

   CHK_EXEC(SetMinorSchemaVersion(5));
   return true;
}

/**
 * Upgrade from 60.3 to 60.4
 */
static bool H_UpgradeFromV3()
{
   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when threshold value reached for specific data collection item.\r\n")
      _T("Parameters are accessible via %<...> and can have \"m\" or \"multipliers\" and \"u\" or \"units\" format modifiers for value formatting (for example %<{m,u}currentValue>).\r\n\r\n")
      _T("Parameters:\r\n")
      _T("   1) dciName - Parameter name\r\n")
      _T("   2) dciDescription - Item description\r\n")
      _T("   3) thresholdValue - Threshold value\r\n")
      _T("   4) currentValue - Actual value which is compared to threshold value\r\n")
      _T("   5) dciId - Data collection item ID\r\n")
      _T("   6) instance - Instance\r\n")
      _T("   7) isRepeatedEvent - Repeat flag\r\n")
      _T("   8) dciValue - Last collected DCI value\r\n")
      _T("   9) operation - Threshold''s operation code\r\n")
      _T("   10) function - Threshold''s function code\r\n")
      _T("   11) pollCount - Threshold''s required poll count\r\n")
      _T("   12) thresholdDefinition - Threshold''s textual definition\r\n")
      _T("   13) instanceValue - Instance value\r\n")
      _T("   14) instanceName - Instance name same as instance\r\n")
      _T("   15) thresholdId - Threshold''s ID'")
      _T(" WHERE event_code=17")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when threshold check is rearmed for specific data collection item.\r\n")
      _T("Parameters are accessible via %<...> and can have \"m\" or \"multipliers\" and \"u\" or \"units\" format modifiers for value formatting (for example %<{m,u}currentValue>)\r\n\r\n")
      _T("Parameters:\r\n")
      _T("   1) dciName - Parameter name\r\n")
      _T("   2) dciDescription - Item description\r\n")
      _T("   3) dciId - Data collection item ID\r\n")
      _T("   4) instance - Instance\r\n")
      _T("   5) thresholdValue - Threshold value\r\n")
      _T("   6) currentValue - Actual value which is compared to threshold value\r\n")
      _T("   7) dciValue - Last collected DCI value\r\n")
      _T("   8) operation - Threshold''s operation code\r\n")
      _T("   9) function - Threshold''s function code\r\n")
      _T("   10) pollCount - Threshold''s required poll count\r\n")
      _T("   11) thresholdDefinition - Threshold''s textual definition\r\n")
      _T("   12) instanceValue - Instance value\r\n")
      _T("   13) instanceName - Instance name same as instance\r\n")
      _T("   14) thresholdId - Threshold''s ID'")
      _T( "WHERE event_code=18")));

   CHK_EXEC(SetMinorSchemaVersion(4));
   return true;
}

/**
 * Upgrade from 60.2 to 60.3
 */
static bool H_UpgradeFromV2()
{
   CHK_EXEC(CreateTable(
      L"CREATE TABLE ai_tasks ("
      L"   id integer not null,"
      L"   user_id integer not null,"
      L"   description varchar(255) null,"
      L"   prompt $SQL:TEXT null,"
      L"   memento $SQL:TEXT null,"
      L"   last_execution_time integer not null,"
      L"   next_execution_time integer not null,"
      L"   iteration integer not null,"
      L"   PRIMARY KEY(id))"));

   if (g_dbSyntax == DB_SYNTAX_TSDB)
   {
      CHK_EXEC(CreateTable(
         L"CREATE TABLE ai_task_execution_log ("
         L"   record_id $SQL:INT64 not null,"
         L"   execution_timestamp timestamptz not null,"
         L"   task_id integer not null,"
         L"   task_description varchar(255) null,"
         L"   user_id integer not null,"
         L"   status char(1) not null,"
         L"   iteration integer not null,"
         L"   explanation $SQL:TEXT null,"
         L"   PRIMARY KEY(record_id,execution_timestamp))"));
      CHK_EXEC(SQLQuery(L"SELECT create_hypertable('ai_task_execution_log', 'execution_timestamp', chunk_time_interval => interval '86400 seconds')"));
   }
   else
   {
      CHK_EXEC(CreateTable(
         L"CREATE TABLE ai_task_execution_log ("
         L"   record_id $SQL:INT64 not null,"
         L"   execution_timestamp integer not null,"
         L"   task_id integer not null,"
         L"   task_description varchar(255) null,"
         L"   user_id integer not null,"
         L"   status char(1) not null,"
         L"   iteration integer not null,"
         L"   explanation $SQL:TEXT null,"
         L"   PRIMARY KEY(record_id))"));
   }

   CHK_EXEC(SQLQuery(L"CREATE INDEX idx_ai_task_exec_log_timestamp ON ai_task_execution_log(execution_timestamp)"));
   CHK_EXEC(SQLQuery(L"CREATE INDEX idx_ai_task_exec_log_asset_id ON ai_task_execution_log(task_id)"));

   CHK_EXEC(SetMinorSchemaVersion(3));
   return true;
}

/**
 * Upgrade from 60.1 to 60.2
 */
static bool H_UpgradeFromV1()
{
   CHK_EXEC(CreateConfigParam(L"ThreadPool.AITasks.BaseSize",
            L"4",
            L"Base size for AI tasks thread pool.",
            nullptr, 'I', true, true, false, false));
   CHK_EXEC(CreateConfigParam(L"ThreadPool.AITasks.MaxSize",
            L"4",
            L"Maximum size for AI tasks thread pool.",
            nullptr, 'I', true, true, false, false));

   CHK_EXEC(SQLQuery(_T("ALTER TABLE event_policy ADD ai_instructions $SQL:TEXT")));

   CHK_EXEC(SetMinorSchemaVersion(2));
   return true;
}

/**
 * Upgrade from 60.0 to 60.1
 */
static bool H_UpgradeFromV0()
{
   CHK_EXEC(CreateConfigParam(L"ReportingServer.ResultsRetentionTime",
            L"90",
            L"The retention time for report execution results.",
            L"days", 'I', true, false, false, false));

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
   { 7,  60, 8,  H_UpgradeFromV7  },
   { 6,  60, 7,  H_UpgradeFromV6  },
   { 5,  60, 6,  H_UpgradeFromV5  },
   { 4,  60, 5,  H_UpgradeFromV4  },
   { 3,  60, 4,  H_UpgradeFromV3  },
   { 2,  60, 3,  H_UpgradeFromV2  },
   { 1,  60, 2,  H_UpgradeFromV1  },
   { 0,  60, 1,  H_UpgradeFromV0  },
   { 0,  0,  0,  nullptr }
};

/**
 * Upgrade database to new version
 */
bool MajorSchemaUpgrade_V60()
{
   int32_t major, minor;
   if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
      return false;

   while ((major == 60) && (minor < DB_SCHEMA_VERSION_V60_MINOR))
   {
      // Find upgrade procedure
      int i;
      for (i = 0; s_dbUpgradeMap[i].upgradeProc != nullptr; i++)
         if (s_dbUpgradeMap[i].version == minor)
            break;
      if (s_dbUpgradeMap[i].upgradeProc == nullptr)
      {
         WriteToTerminalEx(L"Unable to find upgrade procedure for version 53.%d\n", minor);
         return false;
      }
      WriteToTerminalEx(L"Upgrading from version 60.%d to %d.%d\n", minor, s_dbUpgradeMap[i].nextMajor, s_dbUpgradeMap[i].nextMinor);
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
