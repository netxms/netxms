/*
** nxdbmgr - NetXMS database manager
** Copyright (C) 2004-2019 Victor Kirhenshtein
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
   DB_RESULT hResult = DBSelect(g_dbHandle, _T("SELECT id,passive_elements FROM racks"));
   if (hResult != NULL)
   {
      int nRows = DBGetNumRows(hResult);
      for(int i = 0; i < nRows; i++)
      {
         UINT32 rackId = DBGetFieldULong(hResult, i, 0);
         char *xml = DBGetFieldA(hResult, i, 1, NULL, 0);
         Config cfg;
         bool success = cfg.loadXmlConfigFromMemory(xml, strlen(xml), NULL, "passiveElementGroup", false);
         ObjectArray<ConfigEntry> *list = cfg.getSubEntries(_T("/elements"), NULL);
         DB_STATEMENT hStmt = DBPrepare(g_dbHandle,
                       _T("INSERT INTO rack_passive_elements (id,rack_id,name,type,position,orientation,port_count)")
                       _T(" VALUES (?,?,?,?,?,?,0)"));

         if (list != NULL)
         {
            for(int i = 0; i < list->size(); i++)
            {
               ConfigEntry *e = list->get(i);
               const TCHAR *tmp = e->getSubEntryValue(_T("type"));
               int type = 0;
               if(!_tcscmp(tmp, _T("FILLER_PANEL")))
                  type = 1;
               else if(!_tcscmp(tmp, _T("ORGANISER")))
                  type = 2;

               tmp = e->getSubEntryValue(_T("orientation"));
               int orientation = 0;
               if(!_tcscmp(tmp, _T("FRONT")))
                  orientation = 1;
               else if(!_tcscmp(tmp, _T("REAR")))
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
                  delete list;
                  return false;
               }
            }
            delete list;
         }
         MemFree(xml);
         DBFreeStatement(hStmt);
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
   { 1,  31, 2, H_UpgradeFromV1 },
   { 0,  31, 1, H_UpgradeFromV0 },
   { 0,  0,  0, NULL }
};

/**
 * Upgrade database to new version
 */
bool MajorSchemaUpgrade_V31()
{
   INT32 major, minor;
   if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
      return false;

   while((major == 31) && (minor < DB_SCHEMA_VERSION_V31_MINOR))
   {
      // Find upgrade procedure
      int i;
      for(i = 0; s_dbUpgradeMap[i].upgradeProc != NULL; i++)
         if (s_dbUpgradeMap[i].version == minor)
            break;
      if (s_dbUpgradeMap[i].upgradeProc == NULL)
      {
         _tprintf(_T("Unable to find upgrade procedure for version 30.%d\n"), minor);
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
