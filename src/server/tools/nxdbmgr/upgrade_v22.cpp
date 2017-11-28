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
** File: upgrade_v22.cpp
**
**/

#include "nxdbmgr.h"

/**
 * Upgrade from 22.4 to 22.5
 */
static bool H_UpgradeFromV4()
{
   static const TCHAR *batch =
            _T("ALTER TABLE items ADD instance_retention_time integer\n")
            _T("ALTER TABLE dc_tables ADD instance_retention_time integer\n")
            _T("UPDATE items SET instance_retention_time=-1\n")
            _T("UPDATE dc_tables SET instance_retention_time=-1\n")
            _T("INSERT INTO config (var_name,var_value,default_value,is_visible,need_server_restart,is_public,data_type,description) ")
            _T("VALUES ('InstanceRetentionTime','0','0',1,1,'Y','I','Default retention time (in days) for missing DCI instances')\n")
            _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   CHK_EXEC(DBSetNotNullConstraint(g_hCoreDB, _T("items"), _T("instance_retention_time")));
   CHK_EXEC(DBSetNotNullConstraint(g_hCoreDB, _T("dc_tables"), _T("instance_retention_time")));

   CHK_EXEC(SetMinorSchemaVersion(5));
   return true;
}

/**
 * Upgrade from 22.3 to 22.4
 */
static bool H_UpgradeFromV3()
{
   if (GetSchemaLevelForMajorVersion(21) < 5)
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

      CHK_EXEC(SetSchemaLevelForMajorVersion(21, 5));
   }
   CHK_EXEC(SetMinorSchemaVersion(4));
   return true;
}

/**
 * Upgrade from 22.2 to 22.3
 */
static bool H_UpgradeFromV2()
{
   CHK_EXEC(CreateTable(
         _T("CREATE TABLE dci_access (")
         _T("   dci_id integer not null,")
         _T("   user_id integer not null,")
         _T("   PRIMARY KEY(dci_id,user_id))")));
   CHK_EXEC(SetMinorSchemaVersion(3));
   return true;
}

/**
 * Upgrade from 22.1 to 22.2
 */
static bool H_UpgradeFromV1()
{
   CHK_EXEC(CreateConfigParam(_T("DBWriter.MaxRecordsPerTransaction"), _T("1000"), _T("Maximum number of records per one transaction for delayed database writes."), 'I', true, true, false, false));
   CHK_EXEC(SetMinorSchemaVersion(2));
   return true;
}

/**
 * Upgrade from 22.0 to 22.1
 */
static bool H_UpgradeFromV0()
{
   int count = ConfigReadInt(_T("NumberOfDataCollectors"), 250);
   TCHAR value[64];
   _sntprintf(value, 64,_T("%d"), std::max(250, count));
   CHK_EXEC(CreateConfigParam(_T("DataCollector.ThreadPool.BaseSize"), _T("10"), _T("Base size for data collector thread pool."), 'I', true, true, false, false));
   CHK_EXEC(CreateConfigParam(_T("DataCollector.ThreadPool.MaxSize"), value, _T("Maximum size for data collector thread pool."), 'I', true, true, false, false));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET default_value='250' WHERE var_name='DataCollector.ThreadPool.MaxSize'")));
   CHK_EXEC(SQLQuery(_T("DELETE FROM config WHERE var_name='NumberOfDataCollectors'")));
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
   { 4, 22, 5, H_UpgradeFromV4 },
   { 3, 22, 4, H_UpgradeFromV3 },
   { 2, 22, 3, H_UpgradeFromV2 },
   { 1, 22, 2, H_UpgradeFromV1 },
   { 0, 22, 1, H_UpgradeFromV0 },
   { 0, 0, 0, NULL }
};

/**
 * Upgrade database to new version
 */
bool MajorSchemaUpgrade_V22()
{
   INT32 major, minor;
   if (!DBGetSchemaVersion(g_hCoreDB, &major, &minor))
      return false;

   while((major == 22) && (minor < DB_SCHEMA_VERSION_V22_MINOR))
   {
      // Find upgrade procedure
      int i;
      for(i = 0; s_dbUpgradeMap[i].upgradeProc != NULL; i++)
         if (s_dbUpgradeMap[i].version == minor)
            break;
      if (s_dbUpgradeMap[i].upgradeProc == NULL)
      {
         _tprintf(_T("Unable to find upgrade procedure for version 22.%d\n"), minor);
         return false;
      }
      _tprintf(_T("Upgrading from version 22.%d to %d.%d\n"), minor, s_dbUpgradeMap[i].nextMajor, s_dbUpgradeMap[i].nextMinor);
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
