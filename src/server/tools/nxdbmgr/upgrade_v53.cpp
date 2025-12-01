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
** File: upgrade_v53.cpp
**
**/

#include "nxdbmgr.h"

/**
 * Upgrade from 53.11 to 60.0
 */
static bool H_UpgradeFromV11()
{
   CHK_EXEC(SetMajorSchemaVersion(60, 0));
   return true;
}

/**
 * Upgrade from 53.10 to 53.11
 */
static bool H_UpgradeFromV10()
{
   CreateTable(_T("CREATE TABLE dashboard_templates (")
               _T("id INTEGER NOT NULL,")
               _T("num_columns INTEGER NOT NULL,")
               _T("name_template VARCHAR(255),")
               _T("PRIMARY KEY(id))"));
   CreateTable(_T("CREATE TABLE dashboard_template_instances (")
               _T("dashboard_template_id INTEGER NOT NULL,")
               _T("instance_object_id INTEGER NOT NULL,")
               _T("instance_dashboard_id INTEGER NOT NULL,")
               _T("PRIMARY KEY(dashboard_template_id,instance_object_id))"));

   static const TCHAR *batch =
      _T("ALTER TABLE dashboards ADD forced_context_object_id integer\n")
      _T("UPDATE dashboards SET forced_context_object_id=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("dashboards"), _T("forced_context_object_id")));

   CHK_EXEC(SetMinorSchemaVersion(11));
   return true;
}

/**
 * Upgrade from 53.9 to 53.10
 */
static bool H_UpgradeFromV9()
{
   if (GetSchemaLevelForMajorVersion(52) < 23)
   {
      CHK_EXEC(CreateConfigParam(L"Client.EnableWelcomePage",
               L"1",
               L"Enable or disable welcome page in client application (shown when server is upgraded to new version and contains release notes).",
               nullptr, 'B', true, false, false, false));

      CHK_EXEC(SetSchemaLevelForMajorVersion(52, 23));
   }

   CHK_EXEC(SetMinorSchemaVersion(10));
   return true;
}

/**
 * Upgrade from 53.8 to 53.9
 */
static bool H_UpgradeFromV8()
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
      _T("   14) instanceName - Instance name same as instance'")
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
      _T("   13) instanceName - Instance name same as instance'")
      _T( "WHERE event_code=18")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='All thresholds rearmed for data collection item %<dciDescription> (Metric: %<dciName>)")
      _T("Generated when all thresholds are rearmed for specific data collection item.\r\n")
      _T("Parameters are accessible via %<...> and can have \"m\" or \"multipliers\" and \"u\" or \"units\" format modifiers for value formatting (for example %<{m,u}currentValue>).\r\n\r\n")
      _T("Parameters:\r\n")
      _T("   1) dciName - Metric name\r\n")
      _T("   2) dciDescription - Data collection item description\r\n")
      _T("   3) dciId - Data collection item ID\r\n")
      _T("   4) instance - Instance\r\n")
      _T("   5) dciValue - Last collected DCI value\r\n")
      _T("   6) instanceValue - Instance value\r\n")
      _T("   7) instanceName - Instance name same as instance'")
      _T(" WHERE event_code=30")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when table threshold is activated.\r\n")
      _T("Parameters:\r\n")
      _T("   1) dciName - Table DCI name\r\n")
      _T("   2) dciDescription - Table DCI description\r\n")
      _T("   3) dciId - Table DCI ID\r\n")
      _T("   4) row - Table row\r\n")
      _T("   5) instance - Instance'")
      _T(" WHERE event_code=69")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when table threshold is deactivated.\r\n")
      _T("Parameters:\r\n")
      _T("   1) dciName - Table DCI name\r\n")
      _T("   2) dciDescription - Table DCI description\r\n")
      _T("   3) dciId - Table DCI ID\r\n")
      _T("   4) row - Table row\r\n")
      _T("   5) instance - Instance'")
      _T(" WHERE event_code=70")));

   CHK_EXEC(SetMinorSchemaVersion(9));
   return true;
}

/**
 * Upgrade from 53.7 to 53.8
 */
static bool H_UpgradeFromV7()
{
   if (GetSchemaLevelForMajorVersion(52) < 22)
   {
      static const TCHAR *batch =
         _T("UPDATE config SET var_name='Topology.RoutingTable.UpdateInterval' WHERE var_name='Topology.RoutingTableUpdateInterval'\n")
         _T("UPDATE object_custom_attributes SET attr_name='SysConfig:Topology.RoutingTable.UpdateInterval' WHERE attr_name='SysConfig:Topology.RoutingTableUpdateInterval'\n")
         _T("<END>");
      CHK_EXEC(SQLBatch(batch));

      CHK_EXEC(CreateConfigParam(L"Topology.RoutingTable.MaxSize",
               L"4000",
               L"Maximum retrievable routing table size. Larger routing tables will not be retrieved from devices.",
               L"records", 'I', true, false, false, false));

      CHK_EXEC(SetSchemaLevelForMajorVersion(52, 22));
   }

   CHK_EXEC(SetMinorSchemaVersion(8));
   return true;
}

/**
 * Upgrade from 53.6 to 53.7
 */
static bool H_UpgradeFromV6()
{
   if (GetSchemaLevelForMajorVersion(52) < 21)
   {
      CHK_EXEC(SQLQuery(_T("ALTER TABLE policy_action_list ADD record_id integer")));

      switch(g_dbSyntax)
      {
         case DB_SYNTAX_MYSQL:
            CHK_EXEC(SQLQuery(_T("SET @rownum := 0")));
            CHK_EXEC(SQLQuery(_T("UPDATE policy_action_list SET record_id = (@rownum := @rownum + 1)")));
            break;
         case DB_SYNTAX_TSDB:
         case DB_SYNTAX_PGSQL:
            CHK_EXEC(SQLQuery(_T("UPDATE policy_action_list SET record_id = sub.rn FROM (SELECT ctid, ROW_NUMBER() OVER (ORDER BY rule_id, action_id) AS rn FROM policy_action_list) sub WHERE policy_action_list.ctid = sub.ctid")));
            break;
         case DB_SYNTAX_SQLITE:
            CHK_EXEC(SQLQuery(_T("UPDATE policy_action_list SET record_id = (SELECT COUNT(*) FROM policy_action_list AS t2 WHERE t2.rowid <= policy_action_list.rowid)")));
            break;
         case DB_SYNTAX_ORACLE:
            CHK_EXEC(SQLQuery(_T("MERGE INTO policy_action_list t USING (SELECT rowid AS rid, ROW_NUMBER() OVER (ORDER BY rowid) AS rn FROM policy_action_list) s ON (t.rowid = s.rid) WHEN MATCHED THEN UPDATE SET t.record_id = s.rn")));
            break;
         case DB_SYNTAX_MSSQL:
            CHK_EXEC(SQLQuery(_T("WITH numbered AS (SELECT ROW_NUMBER() OVER (ORDER BY rule_id, action_id) AS rn, * FROM policy_action_list) UPDATE numbered SET record_id = rn")));
            break;
         default:
            WriteToTerminal(L"Unsupported DB syntax for row numbering in policy_action_list\n");
            return false;
      }
      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("policy_action_list"), _T("record_id")));
      DBAddPrimaryKey(g_dbHandle, _T("policy_action_list"), _T("record_id,rule_id"));

      if (g_dbSyntax == DB_SYNTAX_PGSQL)
      {
         // Previous upgrade code was removed; PostgreSQL.SkipPrimaryKeyUpdate is an indicator that no changes were made to data tables
         DBMgrMetaDataWriteInt32(L"PostgreSQL.SkipPrimaryKeyUpdate", 1);
      }

      CHK_EXEC(SetSchemaLevelForMajorVersion(52, 21));
   }

   CHK_EXEC(SetMinorSchemaVersion(7));
   return true;
}

/**
 * Upgrade from 53.5 to 53.6
 */
static bool H_UpgradeFromV5()
{
   static const TCHAR *batch =
      _T("ALTER TABLE items ADD thresholds_disable_end_time integer\n")
      _T("UPDATE items SET thresholds_disable_end_time=0\n")
      _T("ALTER TABLE dc_tables ADD thresholds_disable_end_time integer\n")
      _T("UPDATE dc_tables SET thresholds_disable_end_time=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("items"), _T("thresholds_disable_end_time")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("dc_tables"), _T("thresholds_disable_end_time")));
   CHK_EXEC(SetMinorSchemaVersion(6));
   return true;
}

/**
 * Upgrade from 53.4 to 53.5
 */
static bool H_UpgradeFromV4()
{
   CHK_EXEC(SQLQuery(_T("DELETE FROM config WHERE var_name='Objects.Security.ReadAccessViaMap'")));
   CHK_EXEC(SetMinorSchemaVersion(5));
   return true;
}

/**
 * Upgrade from 53.3 to 53.4
 */
static bool H_UpgradeFromV3()
{
   CHK_EXEC(SQLQuery(L"ALTER TABLE interfaces ADD state_before_maintenance varchar(2000)"));
   CHK_EXEC(SetMinorSchemaVersion(4));
   return true;
}

/**
 * Upgrade from 53.2 to 53.3
 */
static bool H_UpgradeFromV2()
{
   CHK_EXEC(DBDropColumn(g_dbHandle, _T("object_properties"), _T("is_system")));
   CHK_EXEC(SetMinorSchemaVersion(3));
   return true;
}

/**
 * Upgrade from 53.1 to 53.2
 */
static bool H_UpgradeFromV1()
{
   if (GetSchemaLevelForMajorVersion(52) < 20)
   {
      CHK_EXEC(CreateConfigParam(L"DataCollection.Scheduler.RequireConnectivity",
               L"0",
               L"Skip data collection scheduling if communication channel is unavailable.",
               nullptr, 'B', true, false, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(52, 20));
   }
   CHK_EXEC(SetMinorSchemaVersion(2));
   return true;
}

/**
 * Upgrade from 53.0 to 53.1
 */
static bool H_UpgradeFromV0()
{
   static const TCHAR *batch =
      _T("ALTER TABLE interfaces ADD peer_last_updated integer\n")
      _T("UPDATE interfaces SET peer_last_updated=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("interfaces"), _T("peer_last_updated")));

   CHK_EXEC(CreateConfigParam(L"Objects.Interfaces.PeerRetentionTime",
            L"30",
            L"Retention time for peer information which is no longer confirmed by polls.",
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
   { 11, 60, 0,  H_UpgradeFromV11 },
   { 10, 53, 11, H_UpgradeFromV10 },
   { 9,  53, 10, H_UpgradeFromV9  },
   { 8,  53, 9,  H_UpgradeFromV8  },
   { 7,  53, 8,  H_UpgradeFromV7  },
   { 6,  53, 7,  H_UpgradeFromV6  },
   { 5,  53, 6,  H_UpgradeFromV5  },
   { 4,  53, 5,  H_UpgradeFromV4  },
   { 3,  53, 4,  H_UpgradeFromV3  },
   { 2,  53, 3,  H_UpgradeFromV2  },
   { 1,  53, 2,  H_UpgradeFromV1  },
   { 0,  53, 1,  H_UpgradeFromV0  },
   { 0,  0,  0,  nullptr }
};

/**
 * Upgrade database to new version
 */
bool MajorSchemaUpgrade_V53()
{
   int32_t major, minor;
   if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
      return false;

   while (major == 53)
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
      WriteToTerminalEx(L"Upgrading from version 53.%d to %d.%d\n", minor, s_dbUpgradeMap[i].nextMajor, s_dbUpgradeMap[i].nextMinor);
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
