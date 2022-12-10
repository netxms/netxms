/*
** nxdbmgr - NetXMS database manager
** Copyright (C) 2022 Raden Solutions
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
** File: upgrade_v43.cpp
**
**/

#include "nxdbmgr.h"

/**
 * Upgrade from 43.3 to 43.4
 */
static bool H_UpgradeFromV3()
{
   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
         _T("message='Threshold reached for data collection item \"%<dciDescription>\" (Parameter: %<dciName>; Threshold value: %<{multipliers, units}thresholdValue>; Actual value: %<{multipliers, units}currentValue>)'")
         _T(" WHERE message='Threshold reached for data collection item \"%2\" (Parameter: %1; Threshold value: %3; Actual value: %4)'")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
         _T("message='Threshold rearmed for data collection item %<dciDescription> (Parameter: %<dciName>)'")
         _T(" WHERE message='Threshold rearmed for data collection item %2 (Parameter: %1)'")));

   if (g_dbSyntax == DB_SYNTAX_ORACLE)
   {
      CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
         _T("description='Generated when threshold value reached for specific data collection item.\r\n")
         _T("Parameters are accessible via %<...> and can have \"m\" or \"multipliers\" and \"u\" or \"units\" format modifiers for value formatting (for example %<{m,u}currentValue>).\r\n\r\n")
         _T("Parameters:\r\n")
         _T("   dciName - Parameter name\r\n")
         _T("   dciDescription - Item description\r\n")
         _T("   thresholdValue - Threshold value\r\n")
         _T("   currentValue - Actual value which is compared to threshold value\r\n")
         _T("   dciId - Data collection item ID\r\n")
         _T("   instance - Instance\r\n")
         _T("   isRepeatedEvent - Repeat flag\r\n")
         _T("   dciValue - Last collected DCI value'")
         _T(" WHERE to_char(description)='Generated when threshold value reached for specific data collection item.\r\n")
         _T("Parameters:\r\n")
         _T("   1) Parameter name\r\n")
         _T("   2) Item description\r\n")
         _T("   3) Threshold value\r\n")
         _T("   4) Actual value\r\n")
         _T("   5) Data collection item ID\r\n")
         _T("   6) Instance\r\n")
         _T("   7) Repeat flag\r\n")
         _T("   8) Current DCI value'")));

      CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
            _T("description='Generated when threshold check is rearmed for specific data collection item.\r\n")
            _T("Parameters are accessible via %<...> and can have \"m\" or \"multipliers\" and \"u\" or \"units\" format modifiers for value formatting (for example %<{m,u}currentValue>)\r\n\r\n")
            _T("Parameters:\r\n")
            _T("   dciName - Parameter name\r\n")
            _T("   dciDescription - Item description\r\n")
            _T("   dciId - Data collection item ID\r\n")
            _T("   instance - Instance\r\n")
            _T("   thresholdValue - Threshold value\r\n")
            _T("   currentValue - Actual value which is compared to threshold value\r\n")
            _T("   dciValue -  Last collected DCI value'")
            _T(" WHERE to_char(description)='Generated when threshold check is rearmed for specific data collection item.\r\n")
            _T("Parameters:\r\n")
            _T("   1) Parameter name\r\n")
            _T("   2) Item description\r\n")
            _T("   3) Data collection item ID\r\n")
            _T("   4) Instance\r\n")
            _T("   5) Threshold value\r\n")
            _T("   6) Actual value\r\n")
            _T("   7) Current DCI value'")));
   }
   else
   {
      CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
            _T("description='Generated when threshold value reached for specific data collection item.\r\n")
            _T("Parameters are accessible via %<...> and can have \"m\" or \"multipliers\" and \"u\" or \"units\" format modifiers for value formatting (for example %<{m,u}currentValue>).\r\n\r\n")
            _T("Parameters:\r\n")
            _T("   dciName - Parameter name\r\n")
            _T("   dciDescription - Item description\r\n")
            _T("   thresholdValue - Threshold value\r\n")
            _T("   currentValue - Actual value which is compared to threshold value\r\n")
            _T("   dciId - Data collection item ID\r\n")
            _T("   instance - Instance\r\n")
            _T("   isRepeatedEvent - Repeat flag\r\n")
            _T("   dciValue - Last collected DCI value'")
            _T(" WHERE description='Generated when threshold value reached for specific data collection item.\r\n")
            _T("Parameters:\r\n")
            _T("   1) Parameter name\r\n")
            _T("   2) Item description\r\n")
            _T("   3) Threshold value\r\n")
            _T("   4) Actual value\r\n")
            _T("   5) Data collection item ID\r\n")
            _T("   6) Instance\r\n")
            _T("   7) Repeat flag\r\n")
            _T("   8) Current DCI value'")));

      CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
            _T("description='Generated when threshold check is rearmed for specific data collection item.\r\n")
            _T("Parameters are accessible via %<...> and can have \"m\" or \"multipliers\" and \"u\" or \"units\" format modifiers for value formatting (for example %<{m,u}currentValue>).\r\n\r\n")
            _T("Parameters:\r\n")
            _T("   dciName - Parameter name\r\n")
            _T("   dciDescription - Item description\r\n")
            _T("   dciId - Data collection item ID\r\n")
            _T("   instance - Instance\r\n")
            _T("   thresholdValue - Threshold value\r\n")
            _T("   currentValue - Actual value which is compared to threshold value\r\n")
            _T("   dciValue -  Last collected DCI value'")
            _T(" WHERE description='Generated when threshold check is rearmed for specific data collection item.\r\n")
            _T("Parameters:\r\n")
            _T("   1) Parameter name\r\n")
            _T("   2) Item description\r\n")
            _T("   3) Data collection item ID\r\n")
            _T("   4) Instance\r\n")
            _T("   5) Threshold value\r\n")
            _T("   6) Actual value\r\n")
            _T("   7) Current DCI value'")));
   }

   CHK_EXEC(SetMinorSchemaVersion(4));
   return true;
}

/**
 * Upgrade from 43.2 to 43.3
 */
static bool H_UpgradeFromV2()
{
   CHK_EXEC(SQLQuery(_T("ALTER TABLE policy_source_list ADD exclusion char(1)")));
   CHK_EXEC(SQLQuery(_T("UPDATE policy_source_list SET exclusion='0'")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("policy_source_list"), _T("exclusion")));
   CHK_EXEC(DBDropPrimaryKey(g_dbHandle, _T("policy_source_list")));
   CHK_EXEC(DBAddPrimaryKey(g_dbHandle, _T("policy_source_list"), _T("rule_id,object_id,exclusion")));
   CHK_EXEC(SetMinorSchemaVersion(3));
   return true;
}

/**
 * Change type of action column for policy_*_actions tables
 */
static bool ChangeActionColumnType(const TCHAR *table, const TCHAR *primaryKey)
{
   CHK_EXEC(DBDropPrimaryKey(g_dbHandle, table));
   CHK_EXEC(SQLQueryFormatted(_T("ALTER TABLE %s ADD action_tmp char(1)"), table));
   CHK_EXEC(SQLQueryFormatted(_T("UPDATE %s SET action_tmp='1' WHERE action=1"), table));
   CHK_EXEC(SQLQueryFormatted(_T("UPDATE %s SET action_tmp='2' WHERE action=2"), table));
   CHK_EXEC(DBDropColumn(g_dbHandle, table, _T("action")));
   CHK_EXEC(DBRenameColumn(g_dbHandle, table, _T("action_tmp"), _T("action")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, table, _T("action")));
   CHK_EXEC(DBAddPrimaryKey(g_dbHandle, table, primaryKey));
   return true;

}

/**
 * Upgrade from 43.1 to 43.2
 */
static bool H_UpgradeFromV1()
{
   CHK_EXEC(DBRenameTable(g_dbHandle, _T("policy_cutom_attribute_actions"), _T("policy_cattr_actions")));
   CHK_EXEC(ChangeActionColumnType(_T("policy_pstorage_actions"), _T("rule_id,ps_key,action")));
   CHK_EXEC(ChangeActionColumnType(_T("policy_cattr_actions"), _T("rule_id,attribute_name,action")));

   CHK_EXEC(SetMinorSchemaVersion(2));
   return true;
}

/**
 * Upgrade from 43.0 to 43.2
 */
static bool H_UpgradeFromV0()
{
   CHK_EXEC(CreateTable(
            _T("CREATE TABLE policy_cattr_actions (")
            _T("rule_id integer not null,")
            _T("attribute_name varchar(127) not null,")
            _T("action char(1) not null,")
            _T("value $SQL:TEXT null,")
            _T("PRIMARY KEY(rule_id,attribute_name,action))")));

   CHK_EXEC(SQLQuery(_T("ALTER TABLE event_policy ADD action_script $SQL:TEXT")));
   CHK_EXEC(DBRenameColumn(g_dbHandle, _T("event_policy"), _T("script"), _T("filter_script")));

   CHK_EXEC(ChangeActionColumnType(_T("policy_pstorage_actions"), _T("rule_id,ps_key,action")));

   CHK_EXEC(SetMinorSchemaVersion(2));
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
   { 3,  43, 4,  H_UpgradeFromV3  },
   { 2,  43, 3,  H_UpgradeFromV2  },
   { 1,  43, 2,  H_UpgradeFromV1  },
   { 0,  43, 2,  H_UpgradeFromV0  },
   { 0,  0,  0,  nullptr }
};

/**
 * Upgrade database to new version
 */
bool MajorSchemaUpgrade_V43()
{
   int32_t major, minor;
   if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
      return false;

   while ((major == 43) && (minor < DB_SCHEMA_VERSION_V43_MINOR))
   {
      // Find upgrade procedure
      int i;
      for (i = 0; s_dbUpgradeMap[i].upgradeProc != nullptr; i++)
         if (s_dbUpgradeMap[i].version == minor)
            break;
      if (s_dbUpgradeMap[i].upgradeProc == nullptr)
      {
         _tprintf(_T("Unable to find upgrade procedure for version 43.%d\n"), minor);
         return false;
      }
      _tprintf(_T("Upgrading from version 43.%d to %d.%d\n"), minor, s_dbUpgradeMap[i].nextMajor, s_dbUpgradeMap[i].nextMinor);
      DBBegin(g_dbHandle);
      if (s_dbUpgradeMap[i].upgradeProc())
      {
         DBCommit(g_dbHandle);
         if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
            return false;
      }
      else
      {
         _tprintf(_T("Rolling back last stage due to upgrade errors...\n"));
         DBRollback(g_dbHandle);
         return false;
      }
   }
   return true;
}
