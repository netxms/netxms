/*
** nxdbmgr - NetXMS database manager
** Copyright (C) 2004-2024 Victor Kirhenshtein
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
** File: convert.cpp
**
**/

#include "nxdbmgr.h"

/**
 * Migrate single data table to TSDB
 */
bool MigrateDataToSingleTable_TSDB(DB_HANDLE hdbSource, uint32_t nodeId, bool tdata);

/**
 * Format query into static buffer
 */
static TCHAR *FormatQuery(const TCHAR *format, ...)
{
   static TCHAR query[4096];
   va_list args;
   va_start(args, format);
   _vsntprintf(query, 4096, format, args);
   va_end(args);
   return query;
}

/**
 * Convert timestamp column from integer to timestamptz
 */
static bool ConvertTimestampColumn(const TCHAR *table, const TCHAR *tscolumn)
{
   CHK_EXEC_NO_SP(DBBegin(g_dbHandle));
   CHK_EXEC_RB(SQLQuery(FormatQuery(_T("ALTER TABLE %s RENAME COLUMN %s TO conv_%s"), table, tscolumn, tscolumn)));
   CHK_EXEC_RB(SQLQuery(FormatQuery(_T("ALTER TABLE %s ADD %s timestamptz"), table, tscolumn)));
   CHK_EXEC_RB(SQLQuery(FormatQuery(_T("UPDATE %s SET %s = to_timestamp(conv_%s)"), table, tscolumn, tscolumn)));
   CHK_EXEC_RB(SQLQuery(FormatQuery(_T("ALTER TABLE %s DROP COLUMN conv_%s"), table, tscolumn)));
   CHK_EXEC_RB(SQLQuery(FormatQuery(_T("ALTER TABLE %s ALTER COLUMN %s SET NOT NULL"), table, tscolumn)));
   CHK_EXEC_NO_SP(DBCommit(g_dbHandle));
   return true;
}

/**
 * Convert table to hypertable
 */
static bool ConvertTable(const TCHAR *table, const TCHAR *tscolumn, const TCHAR *newPK, const TCHAR *extraTimeColumn = nullptr)
{
   WriteToTerminalEx(_T("Converting table \x1b[1m%s\x1b[0m\n"), table);
   CHK_EXEC_NO_SP(DBDropPrimaryKey(g_dbHandle, table));
   CHK_EXEC_NO_SP(ConvertTimestampColumn(table, tscolumn));
   if (extraTimeColumn != nullptr)
      CHK_EXEC_NO_SP(ConvertTimestampColumn(table, extraTimeColumn));
   CHK_EXEC_NO_SP(SQLQuery(FormatQuery(_T("ALTER TABLE %s ADD PRIMARY KEY (%s)"), table, newPK)));
   CHK_EXEC_NO_SP(SQLQuery(StringBuffer(_T("SELECT create_hypertable('")).append(table).append(_T("', '")).append(tscolumn).append(_T("', chunk_time_interval => interval '86400 seconds', migrate_data => true)"))));
   return true;
}

/**
 * Create idata_nn table
 */
static bool CreateIDataTable(const TCHAR *storageClass)
{
   CHK_EXEC_NO_SP(SQLQuery(FormatQuery(
      _T("CREATE TABLE idata_sc_%s (item_id integer not null, idata_timestamp timestamptz not null, idata_value varchar(255) null, raw_value varchar(255) null, PRIMARY KEY(item_id,idata_timestamp))"),
      storageClass)));
   CHK_EXEC_NO_SP(SQLQuery(FormatQuery(_T("SELECT create_hypertable('idata_sc_%s', 'idata_timestamp', chunk_time_interval => interval '86400 seconds')"), storageClass)));
   return true;
}

/**
 * Create tdata_nn table
 */
static bool CreateTDataTable(const TCHAR *storageClass)
{
   CHK_EXEC_NO_SP(SQLQuery(FormatQuery(
      _T("CREATE TABLE tdata_sc_%s (item_id integer not null, tdata_timestamp timestamptz not null, tdata_value text null, PRIMARY KEY(item_id,tdata_timestamp))"),
      storageClass)));
   CHK_EXEC_NO_SP(SQLQuery(FormatQuery(_T("SELECT create_hypertable('tdata_sc_%s', 'tdata_timestamp', chunk_time_interval => interval '86400 seconds')"), storageClass)));
   return true;
}

/**
 * Convert data tables
 */
static bool ConvertDataTables(DB_HANDLE hdb)
{
   IntegerArray<uint32_t> *targets = GetDataCollectionTargets();
   if (targets == nullptr)
      return false;

   // Copy data from idata_xx and tdata_xx tables for each data collection target
   int count = targets->size();
   int i;
   for(i = 0; i < count; i++)
   {
      uint32_t id = targets->get(i);

      if (DBIsTableExist(g_dbHandle, StringBuffer(_T("idata_")).append(id)) == DBIsTableExist_Found)
      {
         if (!MigrateDataToSingleTable_TSDB(hdb, id, false) && !g_ignoreErrors)
            break;
         CHK_EXEC_NO_SP(SQLQuery(FormatQuery(_T("DROP TABLE idata_%u"), id)));
      }
      else
      {
         WriteToTerminalEx(_T("Skipping table \x1b[1midata_%u\x1b[0m\n"), id);
      }

      if (DBIsTableExist(g_dbHandle, StringBuffer(_T("tdata_")).append(id)) == DBIsTableExist_Found)
      {
         if (!MigrateDataToSingleTable_TSDB(hdb, id, true) && !g_ignoreErrors)
            break;
         CHK_EXEC_NO_SP(SQLQuery(FormatQuery(_T("DROP TABLE tdata_%u"), id)));
      }
      else
      {
         WriteToTerminalEx(_T("Skipping table \x1b[1mtdata_%u\x1b[0m\n"), id);
      }
   }

   delete targets;
   return i == count;
}

/**
 * Convert database to TimescaleDB
 */
bool ConvertDatabase()
{
   if (!ValidateDatabase())
      return false;

   if (g_dbSyntax != DB_SYNTAX_PGSQL)
   {
      _tprintf(_T("Only PostgreSQL database can be converted to TimescaleDB database\n"));
      return false;
   }

   WriteToTerminal(_T("\n\n\x1b[1mWARNING!!!\x1b[0m\n"));
   if (!GetYesNo(_T("This operation will convert current database schema for use with TimescaleDB extension. This operation is irreversible.\nAre you sure?")))
      return false;

   CHK_EXEC_NO_SP(SQLQuery(_T("CREATE EXTENSION IF NOT EXISTS timescaledb CASCADE")));

   CHK_EXEC_NO_SP(ConvertTable(_T("asset_change_log"), _T("operation_timestamp"), _T("record_id,operation_timestamp")));
   CHK_EXEC_NO_SP(ConvertTable(_T("certificate_action_log"), _T("operation_timestamp"), _T("record_id,operation_timestamp")));
   CHK_EXEC_NO_SP(ConvertTable(_T("event_log"), _T("event_timestamp"), _T("event_id,event_timestamp")));
   CHK_EXEC_NO_SP(ConvertTable(_T("maintenance_journal"), _T("creation_time"), _T("record_id,creation_time"), _T("modification_time")));
   CHK_EXEC_NO_SP(ConvertTable(_T("notification_log"), _T("notification_timestamp"), _T("id,notification_timestamp")));
   CHK_EXEC_NO_SP(ConvertTable(_T("package_deployment_log"), _T("execution_time"), _T("job_id,execution_time")));
   CHK_EXEC_NO_SP(ConvertTable(_T("server_action_execution_log"), _T("action_timestamp"), _T("id,action_timestamp")));
   CHK_EXEC_NO_SP(ConvertTable(_T("snmp_trap_log"), _T("trap_timestamp"), _T("trap_id,trap_timestamp")));
   CHK_EXEC_NO_SP(ConvertTable(_T("syslog"), _T("msg_timestamp"), _T("msg_id,msg_timestamp")));
   CHK_EXEC_NO_SP(ConvertTable(_T("win_event_log"), _T("event_timestamp"), _T("id,event_timestamp")));

   CHK_EXEC_NO_SP(CreateIDataTable(_T("default")));
   CHK_EXEC_NO_SP(CreateIDataTable(_T("7")));
   CHK_EXEC_NO_SP(CreateIDataTable(_T("30")));
   CHK_EXEC_NO_SP(CreateIDataTable(_T("90")));
   CHK_EXEC_NO_SP(CreateIDataTable(_T("180")));
   CHK_EXEC_NO_SP(CreateIDataTable(_T("other")));

   CHK_EXEC_NO_SP(CreateTDataTable(_T("default")));
   CHK_EXEC_NO_SP(CreateTDataTable(_T("7")));
   CHK_EXEC_NO_SP(CreateTDataTable(_T("30")));
   CHK_EXEC_NO_SP(CreateTDataTable(_T("90")));
   CHK_EXEC_NO_SP(CreateTDataTable(_T("180")));
   CHK_EXEC_NO_SP(CreateTDataTable(_T("other")));

   if (!g_skipDataMigration)
   {
      DB_HANDLE hdb = ConnectToDatabase();
      if (hdb != nullptr)
      {
         CHK_EXEC_NO_SP_WITH_HOOK(ConvertDataTables(hdb), DBDisconnect(hdb));
         DBDisconnect(hdb);
      }
      else if (!g_ignoreErrors)
      {
         return false;
      }
   }
   else
   {
      WriteToTerminal(_T("Skipping data tables migration. Data tables can be migrated while server is running by executing command \x1b[1mnxdbmgr background-convert\x1b[0m\n"));
   }

   CHK_EXEC_NO_SP(SQLQuery(_T("DROP TABLE idata")));
   CHK_EXEC_NO_SP(SQLQuery(
      _T("CREATE VIEW idata AS")
      _T("   SELECT * FROM idata_sc_default")
      _T("   UNION ALL")
      _T("   SELECT * FROM idata_sc_7")
      _T("   UNION ALL")
      _T("   SELECT * FROM idata_sc_30")
      _T("   UNION ALL")
      _T("   SELECT * FROM idata_sc_90")
      _T("   UNION ALL")
      _T("   SELECT * FROM idata_sc_180")
      _T("   UNION ALL")
      _T("   SELECT * FROM idata_sc_other")));

   CHK_EXEC_NO_SP(SQLQuery(_T("DROP TABLE tdata")));
   CHK_EXEC_NO_SP(SQLQuery(
      _T("CREATE VIEW tdata AS")
      _T("   SELECT * FROM tdata_sc_default")
      _T("   UNION ALL")
      _T("   SELECT * FROM tdata_sc_7")
      _T("   UNION ALL")
      _T("   SELECT * FROM tdata_sc_30")
      _T("   UNION ALL")
      _T("   SELECT * FROM tdata_sc_90")
      _T("   UNION ALL")
      _T("   SELECT * FROM tdata_sc_180")
      _T("   UNION ALL")
      _T("   SELECT * FROM tdata_sc_other")));

   CHK_EXEC_NO_SP(SQLQuery(_T("UPDATE metadata SET var_value='TSDB' WHERE var_name='Syntax'")));
   CHK_EXEC_NO_SP(SQLQuery(_T("UPDATE metadata SET var_value='1' WHERE var_name='SingeTablePerfData'")));

   WriteToTerminal(_T("Database conversion is \x1b[1;32mSUCCESSFUL\x1b[0m\n"));
   return true;
}

/**
 * Convert only data tables to TimescaleDB
 */
bool ConvertDataTables()
{
   if (!ValidateDatabase(true))
      return false;

   if (g_dbSyntax != DB_SYNTAX_TSDB)
   {
      _tprintf(_T("Only TimescaleDB database can be processed\n"));
      return false;
   }

   DB_HANDLE hdb = ConnectToDatabase();
   if (hdb == nullptr)
      return false;

   bool success = ConvertDataTables(hdb);
   DBDisconnect(hdb);

   if (success)
      WriteToTerminal(_T("Data tables conversion is \x1b[1;32mSUCCESSFUL\x1b[0m\n"));
   return success;
}
