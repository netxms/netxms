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
** File: upgrade_v37.cpp
**
**/

#include "nxdbmgr.h"
#include <nxevent.h>

/**
 * Upgrade from 37.6 to 38.0
 */
static bool H_UpgradeFromV6()
{
   CHK_EXEC(SetMajorSchemaVersion(38, 0));
   return true;
}

/**
 * Upgrade from 37.5 to 37.6
 */
static bool H_UpgradeFromV5()
{
   CHK_EXEC(SQLQuery(_T("ALTER TABLE users ADD email varchar(127)")));
   CHK_EXEC(SQLQuery(_T("ALTER TABLE users ADD phone_number varchar(63)")));
   CHK_EXEC(SetMinorSchemaVersion(6));
   return true;
}

/**
 * Upgrade from 37.4 to 37.5
 */
static bool H_UpgradeFromV4()
{
   static const TCHAR *batch =
         _T("ALTER TABLE nodes ADD ssh_port integer\n")
         _T("UPDATE nodes SET ssh_port=22\n")
         _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("nodes"), _T("ssh_port")));
   CHK_EXEC(SetMinorSchemaVersion(5));
   return true;
}

/**
 * Upgrade from 37.3 to 37.4
 */
static bool H_UpgradeFromV3()
{
   CHK_EXEC(SQLQuery(_T("UPDATE config SET var_name='Objects.Clusters.ContainerAutoBind' WHERE var_name='ClusterContainerAutoBind'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET var_name='Objects.Clusters.TemplateAutoApply' WHERE var_name='ClusterTemplateAutoApply'")));
   CHK_EXEC(CreateConfigParam(_T("Objects.AccessPoints.ContainerAutoBind"), _T("0"), _T("Enable/disable container auto binding for access points."), nullptr, 'B', true, false, false, false));
   CHK_EXEC(CreateConfigParam(_T("Objects.AccessPoints.TemplateAutoApply"), _T("0"), _T("Enable/disable template auto apply for access points."), nullptr, 'B', true, false, false, false));
   CHK_EXEC(CreateConfigParam(_T("Objects.MobileDevices.ContainerAutoBind"), _T("0"), _T("Enable/disable container auto binding for mobile devices."), nullptr, 'B', true, false, false, false));
   CHK_EXEC(CreateConfigParam(_T("Objects.MobileDevices.TemplateAutoApply"), _T("0"), _T("Enable/disable template auto apply for mobile devices."), nullptr, 'B', true, false, false, false));
   CHK_EXEC(CreateConfigParam(_T("Objects.Sensors.ContainerAutoBind"), _T("0"), _T("Enable/disable container auto binding for sensors."), nullptr, 'B', true, false, false, false));
   CHK_EXEC(CreateConfigParam(_T("Objects.Sensors.TemplateAutoApply"), _T("0"), _T("Enable/disable template auto apply for sensors."), nullptr, 'B', true, false, false, false));
   CHK_EXEC(SetMinorSchemaVersion(4));
   return true;
}

/**
 * Upgrade from 37.2 to 37.3
 */
static bool H_UpgradeFromV2()
{
   CHK_EXEC(CreateConfigParam(_T("Objects.Interfaces.Enable8021xStatusPoll"), _T("1"), _T("Globally enable or disable checking of 802.1x port state during status poll."), nullptr, 'B', true, false, false, false));
   CHK_EXEC(SetMinorSchemaVersion(3));
   return true;
}

/**
 * Upgrade from 37.1 to 37.2
 */
static bool H_UpgradeFromV1()
{
   CHK_EXEC(SQLQuery(_T("UPDATE config SET var_name='DataCollection.InstanceRetentionTime',default_value='7',need_server_restart=0 WHERE var_name='InstanceRetentionTime'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET var_value='7' WHERE var_name='DataCollection.InstanceRetentionTime' AND var_value='0'")));
   CHK_EXEC(SetMinorSchemaVersion(2));
   return true;
}

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
   { 6,  38, 0,  H_UpgradeFromV6  },
   { 5,  37, 6,  H_UpgradeFromV5  },
   { 4,  37, 5,  H_UpgradeFromV4  },
   { 3,  37, 4,  H_UpgradeFromV3  },
   { 2,  37, 3,  H_UpgradeFromV2  },
   { 1,  37, 2,  H_UpgradeFromV1  },
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

   while(major == 37)
   {
      // Find upgrade procedure
      int i;
      for(i = 0; s_dbUpgradeMap[i].upgradeProc != nullptr; i++)
         if (s_dbUpgradeMap[i].version == minor)
            break;
      if (s_dbUpgradeMap[i].upgradeProc == nullptr)
      {
         _tprintf(_T("Unable to find upgrade procedure for version 37.%d\n"), minor);
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
