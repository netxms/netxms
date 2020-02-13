/*
** nxdbmgr - NetXMS database manager
** Copyright (C) 2004-2020 Raden Solutions
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
** File: upgrade_v32.cpp
**
**/

#include "nxdbmgr.h"
#include <nxevent.h>

/**
 * Upgrade from 32.7 to 32.8
 */
static bool H_UpgradeFromV7()
{
   static const TCHAR *batch =
            _T("ALTER TABLE websvc_definitions ADD description varchar(2000)\n")
            _T("ALTER TABLE nodes ADD vendor varchar(127)\n")
            _T("ALTER TABLE nodes ADD product_name varchar(127)\n")
            _T("ALTER TABLE nodes ADD product_version varchar(15)\n")
            _T("ALTER TABLE nodes ADD product_code varchar(31)\n")
            _T("ALTER TABLE nodes ADD serial_number varchar(31)\n")
            _T("ALTER TABLE nodes ADD cip_device_type integer\n")
            _T("ALTER TABLE nodes ADD cip_status integer\n")
            _T("ALTER TABLE nodes ADD cip_state integer\n")
            _T("UPDATE nodes SET cip_device_type=0,cip_status=0,cip_state=0\n")
            _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("nodes"), _T("cip_device_type")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("nodes"), _T("cip_status")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("nodes"), _T("cip_state")));
   CHK_EXEC(SetMinorSchemaVersion(8));
   return true;
}

/**
 * Upgrade from 32.6 to 32.7
 */
static bool H_UpgradeFromV6()
{
   CHK_EXEC(CreateTable(
         _T("CREATE TABLE websvc_definitions (")
         _T("id integer not null,")
         _T("guid varchar(36) not null,")
         _T("name varchar(63) not null,")
         _T("url varchar(4000) null,")
         _T("auth_type integer not null,")
         _T("login varchar(255) null,")
         _T("password varchar(255) null,")
         _T("cache_retention_time integer not null,")
         _T("request_timeout integer not null,")
         _T("PRIMARY KEY(id))")));
   CHK_EXEC(CreateTable(
         _T("CREATE TABLE websvc_headers (")
         _T("websvc_id integer not null,")
         _T("name varchar(63) not null,")
         _T("value varchar(2000) null,")
         _T("PRIMARY KEY(websvc_id))")));
   CHK_EXEC(SetMinorSchemaVersion(7));
   return true;
}

/**
 * Upgrade from 32.5 to 32.6
 */
static bool H_UpgradeFromV5()
{
   static const TCHAR *batch =
            _T("DELETE FROM config WHERE var_name='EnableCheckPointSNMP'\n")
            _T("ALTER TABLE items ADD snmp_version integer\n")
            _T("ALTER TABLE dc_tables ADD snmp_version integer\n")
            _T("UPDATE items SET snmp_version=127\n")
            _T("UPDATE dc_tables SET snmp_version=127\n")
            _T("UPDATE items SET source=2,snmp_version=0,snmp_port=260 WHERE source=3\n")
            _T("UPDATE dc_tables SET source=2,snmp_version=0,snmp_port=260 WHERE source=3\n")
            _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("items"), _T("snmp_version")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("dc_tables"), _T("snmp_version")));
   CHK_EXEC(SetMinorSchemaVersion(6));
   return true;
}

/**
 * Upgrade from 32.4 to 32.5
 */
static bool H_UpgradeFromV4()
{
   CHK_EXEC(DBDropColumn(g_dbHandle, _T("nodes"), _T("chassis_id")));
   CHK_EXEC(DBRenameColumn(g_dbHandle, _T("nodes"), _T("rack_id"), _T("physical_container_id")));
   CHK_EXEC(SQLQuery(_T("ALTER TABLE nodes ADD chassis_placement_config varchar(2000)\n")));
   CHK_EXEC(SetMinorSchemaVersion(5));
   return true;
}

/**
 * Upgrade from 32.3 to 32.4
 */
static bool H_UpgradeFromV3()
{
   static const TCHAR *batch =
            _T("ALTER TABLE object_properties ADD creation_time integer\n")
            _T("UPDATE object_properties SET creation_time=0\n")
            _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("object_properties"), _T("creation_time")));
   CHK_EXEC(SetMinorSchemaVersion(4));
   return true;
}

/**
 * Upgrade from 32.2 to 32.3 (also included in 31.10)
 */
static bool H_UpgradeFromV2()
{
   if (GetSchemaLevelForMajorVersion(31) < 10)
   {
      static const TCHAR *batch =
               _T("ALTER TABLE user_agent_notifications ADD on_startup char(1)\n")
               _T("UPDATE user_agent_notifications SET on_startup='0'\n")
               _T("<END>");
      CHK_EXEC(SQLBatch(batch));
      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("user_agent_notifications"), _T("on_startup")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(31, 10));
   }
   CHK_EXEC(SetMinorSchemaVersion(3));
   return true;
}

/**
 * Upgrade from 32.1 to 32.2
 */
static bool H_UpgradeFromV1()
{
   static const TCHAR *batch =
            _T("ALTER TABLE alarms ADD rca_script_name varchar(255)\n")
            _T("ALTER TABLE alarms ADD impact varchar(1000)\n")
            _T("ALTER TABLE event_policy ADD alarm_impact varchar(1000)\n")
            _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(SetMinorSchemaVersion(2));
   return true;
}

/**
 * Upgrade from 32.0 to 32.1
 */
static bool H_UpgradeFromV0()
{
   static const TCHAR *batch =
            _T("ALTER TABLE alarms ADD parent_alarm_id integer\n")
            _T("UPDATE alarms SET parent_alarm_id=0\n")
            _T("ALTER TABLE event_policy ADD rca_script_name varchar(255)\n")
            _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("alarms"), _T("parent_alarm_id")));

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
   bool (* upgradeProc)();
} s_dbUpgradeMap[] =
{
   { 7,  31, 8, H_UpgradeFromV7 },
   { 6,  31, 7, H_UpgradeFromV6 },
   { 5,  31, 6, H_UpgradeFromV5 },
   { 4,  31, 5, H_UpgradeFromV4 },
   { 3,  32, 4, H_UpgradeFromV3 },
   { 2,  32, 3, H_UpgradeFromV2 },
   { 1,  32, 2, H_UpgradeFromV1 },
   { 0,  32, 1, H_UpgradeFromV0 },
   { 0,  0,  0, NULL }
};

/**
 * Upgrade database to new version
 */
bool MajorSchemaUpgrade_V32()
{
   INT32 major, minor;
   if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
      return false;

   while((major == 32) && (minor < DB_SCHEMA_VERSION_V32_MINOR))
   {
      // Find upgrade procedure
      int i;
      for(i = 0; s_dbUpgradeMap[i].upgradeProc != NULL; i++)
         if (s_dbUpgradeMap[i].version == minor)
            break;
      if (s_dbUpgradeMap[i].upgradeProc == NULL)
      {
         _tprintf(_T("Unable to find upgrade procedure for version 32.%d\n"), minor);
         return false;
      }
      _tprintf(_T("Upgrading from version 32.%d to %d.%d\n"), minor, s_dbUpgradeMap[i].nextMajor, s_dbUpgradeMap[i].nextMinor);
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
