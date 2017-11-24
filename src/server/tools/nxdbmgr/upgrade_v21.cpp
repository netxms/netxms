/*
** nxdbmgr - NetXMS database manager
** Copyright (C) 2004-2017 Victor Kirhenshtein
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
** File: upgrade_v21.cpp
**
**/

#include "nxdbmgr.h"

/**
 * Upgrade from 21.5 to 22.0
 */
static bool H_UpgradeFromV5()
{
   CHK_EXEC(SetMajorSchemaVersion(22, 0));
   return true;
}

/**
 * Upgrade from 21.4 to 21.5
 */
static bool H_UpgradeFromV4()
{
   static const TCHAR *batch =
            _T("ALTER TABLE nodes ADD rack_orientation integer\n")
            _T("ALTER TABLE chassis ADD rack_orientation integer\n")
            _T("UPDATE nodes SET rack_orientation=0\n")
            _T("UPDATE chassis SET rack_orientation=0\n")
            _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   CHK_EXEC(DBSetNotNullConstraint(g_hCoreDB, _T("nodes"), _T("rack_orientation")));
   CHK_EXEC(DBSetNotNullConstraint(g_hCoreDB, _T("chassis"), _T("rack_orientation")));

   CHK_EXEC(SetMinorSchemaVersion(5));
   return true;
}

/**
 * Upgrade from 21.3 to 21.4
 */
static bool H_UpgradeFromV3()
{
   DB_RESULT hResult = DBSelect(g_hCoreDB, _T("SELECT access_rights,object_id FROM acl WHERE user_id=-2147483647")); // Get group Admins object acl
   if (hResult != NULL)
   {
      DB_STATEMENT hStmt = DBPrepare(g_hCoreDB, _T("UPDATE acl SET access_rights=? WHERE user_id=-2147483647 AND object_id=? "));
      if (hStmt != NULL)
      {
         int nRows = DBGetNumRows(hResult);
         UINT32 rights;
         for(int i = 0; i < nRows; i++)
         {
            rights = DBGetFieldULong(hResult, i, 0);
            if (rights & OBJECT_ACCESS_READ)
            {
               rights |= (OBJECT_ACCESS_READ_AGENT | OBJECT_ACCESS_READ_SNMP | OBJECT_ACCESS_SCREENSHOT);
               DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, rights);
               DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, DBGetFieldULong(hResult, i, 1));

               if (!SQLExecute(hStmt))
               {
                  if (!g_bIgnoreErrors)
                  {
                     DBFreeStatement(hStmt);
                     DBFreeResult(hResult);
                     return FALSE;
                  }
               }
            }
         }

         DBFreeStatement(hStmt);
      }
      else if (!g_bIgnoreErrors)
         return FALSE;
      DBFreeResult(hResult);
   }
   else if (!g_bIgnoreErrors)
      return FALSE;

   CHK_EXEC(SetMinorSchemaVersion(4));
   return true;
}

/**
 * Upgrade from 21.2 to 21.3
 */
static bool H_UpgradeFromV2()
{
   static const TCHAR *batch =
            _T("UPDATE nodes SET fail_time_snmp=0 WHERE fail_time_snmp IS NULL\n")
            _T("UPDATE nodes SET fail_time_agent=0 WHERE fail_time_agent IS NULL\n")
            _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   DBSetNotNullConstraint(g_hCoreDB, _T("nodes"), _T("fail_time_snmp"));
   DBSetNotNullConstraint(g_hCoreDB, _T("nodes"), _T("fail_time_agent"));

   CHK_EXEC(SetMinorSchemaVersion(3));
   return true;
}

/**
 * Upgrade from 21.1 to 21.2
 */
static bool H_UpgradeFromV1()
{
   static const TCHAR *batch =
            _T("ALTER TABLE nodes ADD fail_time_snmp integer\n")
            _T("ALTER TABLE nodes ADD fail_time_agent integer\n")
            _T("<END>");
   CHK_EXEC(SQLBatch(batch));
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
   bool (* upgradeProc)();
} s_dbUpgradeMap[] =
{
   { 5, 22, 0, H_UpgradeFromV5 },
   { 4, 21, 5, H_UpgradeFromV4 },
   { 3, 21, 4, H_UpgradeFromV3 },
   { 2, 21, 3, H_UpgradeFromV2 },
   { 1, 21, 2, H_UpgradeFromV1 },
   { 0, 0, 0, NULL }
};

/**
 * Upgrade database to new version
 */
bool MajorSchemaUpgrade_V21()
{
   INT32 major, minor;
   if (!DBGetSchemaVersion(g_hCoreDB, &major, &minor))
      return false;

   while(major == 21)
   {
      // Find upgrade procedure
      int i;
      for(i = 0; s_dbUpgradeMap[i].upgradeProc != NULL; i++)
         if (s_dbUpgradeMap[i].version == minor)
            break;
      if (s_dbUpgradeMap[i].upgradeProc == NULL)
      {
         _tprintf(_T("Unable to find upgrade procedure for version 21.%d\n"), minor);
         return false;
      }
      _tprintf(_T("Upgrading from version 21.%d to %d.%d\n"), minor, s_dbUpgradeMap[i].nextMajor, s_dbUpgradeMap[i].nextMinor);
      DBBegin(g_hCoreDB);
      if (s_dbUpgradeMap[i].upgradeProc())
      {
         DBCommit(g_hCoreDB);
         if (!DBGetSchemaVersion(g_hCoreDB, &major, &minor))
            return false;
      }
      else
      {
         _tprintf(_T("Rolling back last stage due to upgrade errors...\n"));
         DBRollback(g_hCoreDB);
         return false;
      }
   }
   return true;
}
