/*
** nxdbmgr - NetXMS database manager
** Copyright (C) 2020-2021 Raden Solutions
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
** File: upgrade_v39.cpp
**
**/

#include "nxdbmgr.h"

/**
 * Upgrade from 39.1 to 39.2
 */
static bool H_UpgradeFromV1()
{
   if (DBIsTableExist(g_dbHandle, _T("zmq_subscription")) == DBIsTableExist_Found)
      CHK_EXEC(SQLQuery(_T("DROP TABLE zmq_subscription")));
   CHK_EXEC(SetMinorSchemaVersion(2));
   return true;
}

/**
 * Upgrade from 39.0 to 39.1
 */
static bool H_UpgradeFromV0()
{
   CHK_EXEC(CreateTable(
         _T("CREATE TABLE object_queries (")
         _T("   id integer not null,")
         _T("   name varchar(63) not null,")
         _T("   description varchar(255) null,")
         _T("   script $SQL:TEXT null,")
         _T("PRIMARY KEY(id))")));

   CHK_EXEC(CreateTable(
         _T("CREATE TABLE object_queries_input_fields (")
         _T("   query_id integer not null,")
         _T("   name varchar(31) not null,")
         _T("   input_type char(1) not null,")
         _T("   display_name varchar(127) null,")
         _T("   sequence_num integer not null,")
         _T("   PRIMARY KEY(query_id,name))")));

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
} s_dbUpgradeMap[] =
{
   { 1,  32, 2,  H_UpgradeFromV1  },
   { 0,  39, 1,  H_UpgradeFromV0  },
   { 0,  0,  0,  nullptr          }
};

/**
 * Upgrade database to new version
 */
bool MajorSchemaUpgrade_V39()
{
   int32_t major, minor;
   if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
      return false;

   while((major == 39) && (minor < DB_SCHEMA_VERSION_V39_MINOR))
   {
      // Find upgrade procedure
      int i;
      for(i = 0; s_dbUpgradeMap[i].upgradeProc != nullptr; i++)
         if (s_dbUpgradeMap[i].version == minor)
            break;
      if (s_dbUpgradeMap[i].upgradeProc == nullptr)
      {
         _tprintf(_T("Unable to find upgrade procedure for version 39.%d\n"), minor);
         return false;
      }
      _tprintf(_T("Upgrading from version 39.%d to %d.%d\n"), minor, s_dbUpgradeMap[i].nextMajor, s_dbUpgradeMap[i].nextMinor);
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
