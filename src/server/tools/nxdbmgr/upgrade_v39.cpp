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
 * Upgrade from 39.4 to 39.5
 */
static bool H_UpgradeFromV4()
{
   CHK_EXEC(CreateConfigParam(_T("Geolocation.History.RetentionTime"),
         _T("90"),
         _T("Retention time in days for objet's geolocation history. All records older than specified will be deleted by housekeeping process."),
         _T("days"),
         'I',
         true,
         false,
         false,
         false));

   CHK_EXEC(SetMinorSchemaVersion(5));
   return true;
}

/**
 * Upgrade from 39.3 to 39.4
 */
static bool H_UpgradeFromV3()
{
   CHK_EXEC(SQLQuery(_T("ALTER TABLE input_fields ADD flags integer")));

   DB_RESULT hResult = SQLSelect(_T("SELECT category,owner_id,name,config FROM input_fields"));
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         TCHAR category[2], name[MAX_OBJECT_NAME];
         DBGetField(hResult, i, 0, category, 2);
         uint32_t ownerId = DBGetFieldLong(hResult, i, 1);
         DBGetField(hResult, i, 2, name, MAX_OBJECT_NAME);
         TCHAR *config = DBGetField(hResult, i, 3, nullptr, 0);

         TCHAR query[1024];
         _sntprintf(query, 1024, _T("UPDATE input_fields SET flags=%d WHERE category='%s' AND owner_id=%u AND name=%s"),
                  (_tcsstr(config, _T("<validatePassword>true</validatePassword>")) != nullptr) ? 1 : 0, category, ownerId, DBPrepareString(g_dbHandle, name).cstr());
         MemFree(config);

         if (!SQLQuery(query) && !g_ignoreErrors)
         {
            DBFreeResult(hResult);
            return false;
         }
      }
      DBFreeResult(hResult);
   }
   else if (!g_ignoreErrors)
   {
      return false;
   }

   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("input_fields"), _T("flags")));
   CHK_EXEC(DBDropColumn(g_dbHandle, _T("input_fields"), _T("config")));

   CHK_EXEC(SetMinorSchemaVersion(4));
   return true;
}

/**
 * Upgrade from 39.2 to 39.3
 */
static bool H_UpgradeFromV2()
{
   CHK_EXEC(DBDropPrimaryKey(g_dbHandle, _T("object_tools_input_fields")));
   CHK_EXEC(DBRenameTable(g_dbHandle, _T("object_tools_input_fields"), _T("input_fields")));
   CHK_EXEC(DBRenameColumn(g_dbHandle, _T("input_fields"), _T("tool_id"), _T("owner_id")));
   CHK_EXEC(SQLQuery(_T("ALTER TABLE input_fields ADD category char(1)")));
   CHK_EXEC(SQLQuery(_T("UPDATE input_fields SET category='T'")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("input_fields"), _T("category")));
   CHK_EXEC(DBAddPrimaryKey(g_dbHandle, _T("input_fields"), _T("category,owner_id,name")));
   CHK_EXEC(SetMinorSchemaVersion(3));
   return true;
}

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
         _T("   guid varchar(36) not null,")
         _T("   name varchar(63) not null,")
         _T("   description varchar(255) null,")
         _T("   script $SQL:TEXT null,")
         _T("PRIMARY KEY(id))")));

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
   { 4,  39, 5,  H_UpgradeFromV4  },
   { 3,  39, 4,  H_UpgradeFromV3  },
   { 2,  39, 3,  H_UpgradeFromV2  },
   { 1,  39, 2,  H_UpgradeFromV1  },
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
