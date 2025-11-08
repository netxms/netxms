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
         L"   PRIMARY KEY(record_id))"));
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
