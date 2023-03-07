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
