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
         static const TCHAR *batch =
            _T("UPDATE metadata SET var_value='ALTER TABLE idata_%d ADD PRIMARY KEY (item_id,idata_timestamp)' WHERE var_name='IDataIndexCreationCommand_0'\n")
            _T("UPDATE metadata SET var_value='ALTER TABLE tdata_%d ADD PRIMARY KEY (item_id,tdata_timestamp)' WHERE var_name='TDataIndexCreationCommand_0'\n")
            _T("<END>");
         CHK_EXEC(SQLBatch(batch));

         RegisterOnlineUpgrade(52, 21);
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

   while ((major == 53) && (minor < DB_SCHEMA_VERSION_V53_MINOR))
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
