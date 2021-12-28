/*
** nxdbmgr - NetXMS database manager
** Copyright (C) 2004-2021 Victor Kirhenshtein
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
** File: upgrade_v31.cpp
**
**/

#include "nxdbmgr.h"
#include <nxevent.h>

/**
 * Upgrade from 31.10 to 32.0
 */
static bool H_UpgradeFromV10()
{
   CHK_EXEC(SetMajorSchemaVersion(32, 0));
   return true;
}

/**
 * Upgrade from 31.9 to 31.10
 */
static bool H_UpgradeFromV9()
{
   static const TCHAR *batch =
            _T("ALTER TABLE user_agent_notifications ADD on_startup char(1)\n")
            _T("UPDATE user_agent_notifications SET on_startup='0'\n")
            _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("user_agent_notifications"), _T("on_startup")));

   CHK_EXEC(SetMinorSchemaVersion(10));
   return true;
}

/**
 * Upgrade from 31.8 to 31.9
 */
static bool H_UpgradeFromV8()
{
   CHK_EXEC(DBRemoveNotNullConstraint(g_dbHandle, _T("rack_passive_elements"), _T("name")));
   CHK_EXEC(DBRemoveNotNullConstraint(g_dbHandle, _T("physical_links"), _T("description")));

   CHK_EXEC(SetMinorSchemaVersion(9));
   return true;
}

/**
 * Upgrade from 31.7 to 31.8
 */
static bool H_UpgradeFromV7()
{
   static const TCHAR *batch =
            _T("ALTER TABLE object_custom_attributes ADD flags integer\n")
            _T("UPDATE object_custom_attributes SET flags=0\n")
            _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("object_custom_attributes"), _T("flags")));

   CHK_EXEC(SetMinorSchemaVersion(8));
   return true;
}

/**
 * Upgrade from 31.6 to 31.7
 */
static bool H_UpgradeFromV6()
{
   CHK_EXEC(SQLQuery(
         _T("INSERT INTO script_library (guid,script_id,script_name,script_code) ")
         _T("VALUES ('5648916c-ad47-45a5-8960-443a98dace46',21,'Hook::EventProcessor','")
         _T("/* Available global variables:\r\n")
         _T(" *  $object - event source object, one of ''NetObj'' subclasses\r\n")
         _T(" *  $node - event source object if it is ''Node'' class\n\n")
         _T(" *  $event - event being processed (object of ''Event'' class)\r\n")
         _T(" *\r\n")
         _T(" * Expected return value:\r\n")
         _T(" *  none - returned value is ignored\r\n */\r\n')")));

   CHK_EXEC(SetMinorSchemaVersion(7));
   return true;
}

/**
 * Upgrade from 31.5 to 31.6
 */
static bool H_UpgradeFromV5()
{
   static const TCHAR *batch =
            _T("UPDATE config SET var_name='Objects.Interfaces.DefaultExpectedState' WHERE var_name='DefaultInterfaceExpectedState'\n")
            _T("UPDATE config SET var_name='Objects.Interfaces.UseAliases' WHERE var_name='UseInterfaceAliases'\n")
            _T("UPDATE config SET var_name='Objects.Interfaces.UseIfXTable' WHERE var_name='UseIfXTable'\n")
            _T("UPDATE config SET var_name='Objects.Nodes.ResolveDNSToIPOnStatusPoll' WHERE var_name='ResolveDNSToIPOnStatusPoll'\n")
            _T("UPDATE config SET var_name='Objects.Nodes.ResolveNames' WHERE var_name='ResolveNodeNames'\n")
            _T("UPDATE config SET var_name='Objects.Nodes.SyncNamesWithDNS' WHERE var_name='SyncNodeNamesWithDNS'\n")
            _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   CHK_EXEC(CreateConfigParam(_T("Objects.Interfaces.NamePattern"), _T(""), _T("Custom name pattern for interface objects."), NULL, 'S', true, false, false, false));

   DB_RESULT hResult = SQLSelect(_T("SELECT var_name FROM config WHERE var_name LIKE 'Ldap%'"));
   if (hResult != NULL)
   {
      TCHAR name[128], query[512];
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         DBGetField(hResult, i, 0, name, 128);
         _sntprintf(query, 512, _T("UPDATE config SET var_name='LDAP.%s' WHERE var_name='%s'"), &name[4], name);
         CHK_EXEC(SQLQuery(query));
      }
      DBFreeResult(hResult);
   }
   else if (!g_ignoreErrors)
   {
      return false;
   }

   CHK_EXEC(SetMinorSchemaVersion(6));
   return true;
}

/**
 * Convert flags in items and dc_tables
 */
static bool ConvertDCObjectFlags(const TCHAR *table)
{
   TCHAR query[256] = _T("SELECT item_id,flags,retention_time,polling_interval FROM ");
   _tcslcat(query, table, 256);
   DB_RESULT hResult = SQLSelect(query);
   if (hResult == NULL)
      return false;

   int count = DBGetNumRows(hResult);
   for (int i = 0; i < count; i++)
   {
      UINT32 id = DBGetFieldULong(hResult, i, 0);
      UINT32 flags = DBGetFieldULong(hResult, i, 1);
      int retentionTime = DBGetFieldLong(hResult, i, 2);
      int pollingInterval = DBGetFieldLong(hResult, i, 3);
      if (((flags & 0x201) != 0) || (pollingInterval > 0) || (retentionTime > 0))
      {
         _sntprintf(query, 256, _T("UPDATE %s SET polling_schedule_type='%c',retention_type='%c',flags=%d WHERE item_id=%u"),
            table, (flags & 1) ? _T('2') : ((pollingInterval <= 0) ? _T('0') : _T('1')),
            (flags & 0x200) ? _T('2') : (retentionTime <= 0) ? _T('0') : _T('1'),
            flags & ~0x201, id);
         CHK_EXEC_NO_SP(SQLQuery(query));
      }
   }

   DBFreeResult(hResult);
   return true;
}

/**
 * Upgrade from 31.4 to 31.5
 */
static bool H_UpgradeFromV4()
{
   static const TCHAR *batch =
            _T("ALTER TABLE items ADD polling_schedule_type char(1)\n")
            _T("ALTER TABLE items ADD retention_type char(1)\n")
            _T("UPDATE items SET polling_schedule_type='0',retention_type='0'\n")
            _T("ALTER TABLE dc_tables ADD polling_schedule_type char(1)\n")
            _T("ALTER TABLE dc_tables ADD retention_type char(1)\n")
            _T("UPDATE dc_tables SET polling_schedule_type='0',retention_type='0'\n")
            _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("items"), _T("polling_schedule_type")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("items"), _T("retention_type")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("dc_tables"), _T("polling_schedule_type")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("dc_tables"), _T("retention_type")));

   CHK_EXEC(ConvertDCObjectFlags(_T("items")));
   CHK_EXEC(ConvertDCObjectFlags(_T("dc_tables")));

   CHK_EXEC(SetMinorSchemaVersion(5));
   return true;
}

/**
 * Upgrade from 31.3 to 31.4
 */
static bool H_UpgradeFromV3()
{
   static const TCHAR *batch =
            _T("ALTER TABLE items ADD polling_interval_src varchar(255)\n")
            _T("ALTER TABLE items ADD retention_time_src varchar(255)\n")
            _T("UPDATE items SET polling_interval_src=polling_interval,retention_time_src=retention_time\n")
            _T("ALTER TABLE dc_tables ADD polling_interval_src varchar(255)\n")
            _T("ALTER TABLE dc_tables ADD retention_time_src varchar(255)\n")
            _T("UPDATE dc_tables SET polling_interval_src=polling_interval,retention_time_src=retention_time\n")
            _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(SetMinorSchemaVersion(4));
   return true;
}

/**
 * Upgrade from 31.2 to 31.3
 */
static bool H_UpgradeFromV2()
{
   CHK_EXEC(CreateEventTemplate(EVENT_SNMP_DISCARD_TRAP, _T("SNMP_DISCARD_TRAP"),
            SEVERITY_NORMAL, 0, _T("48f1217b-2eeb-4cde-88ab-da3de662bb2d"),
            _T("SNMP trap received: %1"),
            _T("Default event for discarded SNMP traps.\r\n")
            _T("Parameters:\r\n")
            _T("   1) SNMP trap OID")
            ));
   CHK_EXEC(SetMinorSchemaVersion(3));
   return true;
}

/**
 * Upgrade from 31.1 to 31.2
 */
static bool H_UpgradeFromV1()
{
   CHK_EXEC(CreateTable(
      _T("CREATE TABLE nc_persistent_storage (")
      _T("  channel_name varchar(63) not null,")
      _T("  entry_name varchar(127) not null,")
      _T("  entry_value varchar(2000) null,")
      _T("PRIMARY KEY(channel_name, entry_name))")));

   CHK_EXEC(SetMinorSchemaVersion(2));
   return true;
}

/**
 * Upgrade from 31.0 to 31.1
 */
static bool H_UpgradeFromV0()
{
   CHK_EXEC(CreateTable(
      _T("CREATE TABLE rack_passive_elements (")
      _T("  id integer not null,")
      _T("  rack_id integer not null,")
      _T("  name varchar(255) not null,")
      _T("  type integer not null,")
      _T("  position integer not null,")
      _T("  orientation integer not null,")
      _T("  port_count integer not null,")
      _T("PRIMARY KEY(id))")));

   CHK_EXEC(CreateTable(
      _T("CREATE TABLE physical_links (")
      _T("  id integer not null,")
      _T("  description varchar(255) not null,")
      _T("  left_object_id integer not null,")
      _T("  left_patch_pannel_id integer not null,")
      _T("  left_port_number integer not null,")
      _T("  left_front char(1) not null,")
      _T("  right_object_id integer not null,")
      _T("  right_patch_pannel_id integer not null,")
      _T("  right_port_number integer not null,")
      _T("  right_front char(1) not null,")
      _T("PRIMARY KEY(id))")));

   int id = 1;
   DB_RESULT hResult = SQLSelect(_T("SELECT id,passive_elements FROM racks"));
   if (hResult != NULL)
   {
      DB_STATEMENT hStmt = DBPrepare(g_dbHandle,
                    _T("INSERT INTO rack_passive_elements (id,rack_id,name,type,position,orientation,port_count)")
                    _T(" VALUES (?,?,?,?,?,?,0)"));
      if (hStmt != NULL)
      {
         int nRows = DBGetNumRows(hResult);
         for(int i = 0; i < nRows; i++)
         {
            UINT32 rackId = DBGetFieldULong(hResult, i, 0);
            char *xml = DBGetFieldUTF8(hResult, i, 1, NULL, 0);
            if (xml != NULL)
            {
               Config cfg;
               if (cfg.loadXmlConfigFromMemory(xml, strlen(xml), NULL, "passiveElementGroup", false))
               {
                  unique_ptr<ObjectArray<ConfigEntry>> list = cfg.getSubEntries(_T("/elements"), NULL);
                  if (list != NULL)
                  {
                     for(int i = 0; i < list->size(); i++)
                     {
                        ConfigEntry *e = list->get(i);
                        const TCHAR *tmp = e->getSubEntryValue(_T("type"), 0, _T("FILLER_PANEL"));
                        int type = 0;
                        if (!_tcscmp(tmp, _T("FILLER_PANEL")))
                           type = 1;
                        else if (!_tcscmp(tmp, _T("ORGANISER")))
                           type = 2;

                        tmp = e->getSubEntryValue(_T("orientation"), 0, _T("FRONT"));
                        int orientation = 0;
                        if (!_tcscmp(tmp, _T("FRONT")))
                           orientation = 1;
                        else if (!_tcscmp(tmp, _T("REAR")))
                           orientation = 2;

                        DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, id++);
                        DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, rackId);
                        DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, e->getSubEntryValue(_T("name")), DB_BIND_STATIC);
                        DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, type);
                        DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, e->getSubEntryValueAsInt(_T("position")));
                        DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, orientation);
                        if (!SQLExecute(hStmt) && !g_ignoreErrors)
                        {
                           MemFree(xml);
                           DBFreeStatement(hStmt);
                           DBFreeResult(hResult);
                           return false;
                        }
                     }
                  }
               }
            }
            MemFree(xml);
         }
         DBFreeStatement(hStmt);
      }
      else
      {
         if (!g_ignoreErrors)
         {
            DBFreeResult(hResult);
            return false;
         }
      }
      DBFreeResult(hResult);
   }
   else
   {
      if (!g_ignoreErrors)
      {
         return false;
      }
   }

   CHK_EXEC(DBDropColumn(g_dbHandle, _T("racks"), _T("passive_elements")));

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
   { 10, 32, 0,  H_UpgradeFromV10 },
   { 9,  31, 10, H_UpgradeFromV9 },
   { 8,  31, 9,  H_UpgradeFromV8 },
   { 7,  31, 8,  H_UpgradeFromV7 },
   { 6,  31, 7,  H_UpgradeFromV6 },
   { 5,  31, 6,  H_UpgradeFromV5 },
   { 4,  31, 5,  H_UpgradeFromV4 },
   { 3,  31, 4,  H_UpgradeFromV3 },
   { 2,  31, 3,  H_UpgradeFromV2 },
   { 1,  31, 2,  H_UpgradeFromV1 },
   { 0,  31, 1,  H_UpgradeFromV0 },
   { 0,  0,  0,  NULL }
};

/**
 * Upgrade database to new version
 */
bool MajorSchemaUpgrade_V31()
{
   INT32 major, minor;
   if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
      return false;

   while(major == 31)
   {
      // Find upgrade procedure
      int i;
      for(i = 0; s_dbUpgradeMap[i].upgradeProc != NULL; i++)
         if (s_dbUpgradeMap[i].version == minor)
            break;
      if (s_dbUpgradeMap[i].upgradeProc == NULL)
      {
         _tprintf(_T("Unable to find upgrade procedure for version 31.%d\n"), minor);
         return false;
      }
      _tprintf(_T("Upgrading from version 31.%d to %d.%d\n"), minor, s_dbUpgradeMap[i].nextMajor, s_dbUpgradeMap[i].nextMinor);
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
