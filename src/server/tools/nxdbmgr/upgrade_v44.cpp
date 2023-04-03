/*
** nxdbmgr - NetXMS database manager
** Copyright (C) 2022-2023 Raden Solutions
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
** File: upgrade_v44.cpp
**
**/

#include "nxdbmgr.h"
#include <nxevent.h>

/**
 * Upgrade from 44.4 to 44.5
 */
static bool H_UpgradeFromV4()
{
   CHK_EXEC(CreateEventTemplate(EVENT_ASSET_AUTO_UPDATE_FAILED, _T("SYS_ASSET_AUTO_UPDATE_FAILED"),
                                EVENT_SEVERITY_MAJOR, EF_LOG, _T("f4ae5e79-9d39-41d0-b74c-b6c591930d08"),
                                _T("Automatic update of asset management attribute \"%<name>\" with value \"%<newValue>\" failed (%<reason>)"),
                                _T("Generated when automatic update of asset management attribute fails.\r\n")
                                _T("Parameters:\r\n")
                                _T("   name - attribute's name\r\n")
                                _T("   displayName - attribute's display name\r\n")
                                _T("   dataType - attribute's data type\r\n")
                                _T("   currValue - current attribute's value\r\n")
                                _T("   newValue - new attribute's value\r\n")
                                _T("   reason - failure reason")
                                ));

   int ruleId = NextFreeEPPruleID();
   TCHAR query[1024];
   _sntprintf(query, 1024, _T("INSERT INTO event_policy (rule_id,rule_guid,flags,comments,alarm_message,alarm_severity,alarm_key,filter_script,alarm_timeout,alarm_timeout_event) ")
                           _T("VALUES (%d,'1499d2d3-2304-4bb1-823b-0c530cbb6224',7944,'Generate alarm when automatic update of asset management attribute fails','%%m',5,'ASSET_UPDATE_FAILED_%%i_%%<name>','',0,%d)"),
         ruleId, EVENT_ALARM_TIMEOUT);
   CHK_EXEC(SQLQuery(query));

   CHK_EXEC(SetMinorSchemaVersion(5));
   return true;
}

/**
 * Upgrade from 44.3 to 44.4
 */
static bool H_UpgradeFromV3()
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
      _T("   dciValue - Last collected DCI value\r\n")
      _T("   operation - Threshold''s operation code\r\n")
      _T("   function - Threshold''s function code\r\n")
      _T("   pollCount - Threshold''s required poll count\r\n")
      _T("   thresholdDefinition - Threshold''s textual definition'")
      _T(" WHERE event_code=17")));

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
      _T("   dciValue - Last collected DCI value\r\n")
      _T("   operation - Threshold''s operation code\r\n")
      _T("   function - Threshold''s function code\r\n")
      _T("   pollCount - Threshold''s required poll count\r\n")
      _T("   thresholdDefinition - Threshold''s textual definition'")
      _T( "WHERE event_code=18")));

   CHK_EXEC(SetMinorSchemaVersion(4));
   return true;
}

/**
 * Upgrade from 44.2 to 44.3
 */
static bool H_UpgradeFromV2()
{
   CHK_EXEC(DBRenameColumn(g_dbHandle, _T("object_properties"), _T("submap_id"), _T("drilldown_object_id")));
   CHK_EXEC(SetMinorSchemaVersion(3));
   return true;
}

/**
 * Upgrade from 44.1 to 44.2
 */
static bool H_UpgradeFromV1()
{
   CHK_EXEC(CreateConfigParam(_T("Server.Security.2FA.TrustedDeviceTTL"), _T("0"), _T("TTL in seconds for 2FA trusted device."), _T("seconds"), 'I', true, false, false, false));
   CHK_EXEC(SetMinorSchemaVersion(2));
   return true;
}

/**
 * Upgrade from 44.0 to 44.1
 */
static bool H_UpgradeFromV0()
{
   CHK_EXEC(CreateTable(
            _T("CREATE TABLE am_attributes (")
            _T("attr_name varchar(63) not null,")
            _T("display_name varchar(255) null,")
            _T("data_type integer not null,")
            _T("is_mandatory char(1) not null,")
            _T("is_unique char(1) not null,")
            _T("autofill_script $SQL:TEXT null,")
            _T("range_min integer not null,")
            _T("range_max integer not null,")
            _T("sys_type integer not null,")
            _T("PRIMARY KEY(attr_name))")));

   CHK_EXEC(CreateTable(
            _T("CREATE TABLE am_enum_values (")
            _T("attr_name varchar(63) not null,")
            _T("attr_value varchar(63) not null,")
            _T("display_name varchar(255) not null,")
            _T("PRIMARY KEY(attr_name,attr_value))")));

   CHK_EXEC(CreateTable(
            _T("CREATE TABLE am_object_data (")
            _T("object_id integer not null,")
            _T("attr_name varchar(63) not null,")
            _T("attr_value varchar(255) null,")
            _T("PRIMARY KEY(object_id,attr_name))")));

   // Update access rights for predefined "Admins" group
   DB_RESULT hResult = SQLSelect(_T("SELECT system_access FROM user_groups WHERE id=1073741825"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         uint64_t access = DBGetFieldUInt64(hResult, 0, 0);
         access |= SYSTEM_ACCESS_AM_ATTRIBUTE_MANAGE;
         TCHAR query[256];
         _sntprintf(query, 256, _T("UPDATE user_groups SET system_access=") UINT64_FMT _T(" WHERE id=1073741825"), access);
         CHK_EXEC(SQLQuery(query));
      }
      DBFreeResult(hResult);
   }
   else
   {
      if (!g_ignoreErrors)
         return false;
   }

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
   { 4,  44, 5,  H_UpgradeFromV4  },
   { 3,  44, 4,  H_UpgradeFromV3  },
   { 2,  44, 3,  H_UpgradeFromV2  },
   { 1,  44, 2,  H_UpgradeFromV1  },
   { 0,  44, 1,  H_UpgradeFromV0  },
   { 0,  0,  0,  nullptr }
};

/**
 * Upgrade database to new version
 */
bool MajorSchemaUpgrade_V44()
{
   int32_t major, minor;
   if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
      return false;

   while ((major == 44) && (minor < DB_SCHEMA_VERSION_V44_MINOR))
   {
      // Find upgrade procedure
      int i;
      for (i = 0; s_dbUpgradeMap[i].upgradeProc != nullptr; i++)
         if (s_dbUpgradeMap[i].version == minor)
            break;
      if (s_dbUpgradeMap[i].upgradeProc == nullptr)
      {
         _tprintf(_T("Unable to find upgrade procedure for version 44.%d\n"), minor);
         return false;
      }
      _tprintf(_T("Upgrading from version 44.%d to %d.%d\n"), minor, s_dbUpgradeMap[i].nextMajor, s_dbUpgradeMap[i].nextMinor);
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
