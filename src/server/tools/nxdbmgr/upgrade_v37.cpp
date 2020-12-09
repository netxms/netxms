/*
** nxdbmgr - NetXMS database manager
** Copyright (C) 2020 Raden Solutions
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
** File: upgrade_v37.cpp
**
**/

#include "nxdbmgr.h"
#include <nxevent.h>

/**
 * Upgrade from 37.0 to 37.1
 */
static bool H_UpgradeFromV0()
{
   CHK_EXEC(CreateEventTemplate(EVENT_CLUSTER_AUTOADD, _T("SYS_CLUSTER_AUTOADD"),
            SEVERITY_NORMAL, EF_LOG, _T("308fce5f-69ec-450a-a42b-d1e5178512a5"),
            _T("Node %2 automatically added to cluster %4"),
            _T("Generated when node added to cluster object by autoadd rule.\r\n")
            _T("Parameters:\r\n")
            _T("   1) Node ID\r\n")
            _T("   2) Node name\r\n")
            _T("   3) Cluster ID\r\n")
            _T("   4) Cluster name")
            ));

   CHK_EXEC(CreateEventTemplate(EVENT_CLUSTER_AUTOREMOVE, _T("SYS_CLUSTER_AUTOREMOVE"),
            SEVERITY_NORMAL, EF_LOG, _T("f2cdb47a-ae37-43d7-851d-f8f85e1e9f0c"),
            _T("Node %2 automatically removed from cluster %4"),
            _T("Generated when node removed from cluster object by autoadd rule.\r\n")
            _T("Parameters:\r\n")
            _T("   1) Node ID\r\n")
            _T("   2) Node name\r\n")
            _T("   3) Cluster ID\r\n")
            _T("   4) Cluster name")
            ));
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
   { 0,  37, 1,  H_UpgradeFromV0  },
   { 0,  0,  0,  nullptr          }
};

/**
 * Upgrade database to new version
 */
bool MajorSchemaUpgrade_V37()
{
   int32_t major, minor;
   if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
      return false;

   while((major == 37) && (minor < DB_SCHEMA_VERSION_V37_MINOR))
   {
      // Find upgrade procedure
      int i;
      for(i = 0; s_dbUpgradeMap[i].upgradeProc != nullptr; i++)
         if (s_dbUpgradeMap[i].version == minor)
            break;
      if (s_dbUpgradeMap[i].upgradeProc == nullptr)
      {
         _tprintf(_T("Unable to find upgrade procedure for version 36.%d\n"), minor);
         return false;
      }
      _tprintf(_T("Upgrading from version 37.%d to %d.%d\n"), minor, s_dbUpgradeMap[i].nextMajor, s_dbUpgradeMap[i].nextMinor);
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
