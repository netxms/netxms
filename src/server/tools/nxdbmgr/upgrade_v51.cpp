/*
** nxdbmgr - NetXMS database manager
** Copyright (C) 2022-2024 Raden Solutions
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
** File: upgrade_v51.cpp
**
**/

#include "nxdbmgr.h"
#include <nxevent.h>

/**
 * Upgrade from 51.6 to 51.7
 */
static bool H_UpgradeFromV6()
{
   if (GetSchemaLevelForMajorVersion(50) < 44)
   {
      CHK_EXEC(CreateConfigParam(_T("AgentTunnels.BindByIPAddress"),
                                 _T("0"),
                                 _T("nable/disable agent tunnel binding by IP address. If enabled and agent certificate is not provided, tunnel will be bound to node with IP address matching tunnel source IP address."),
                                 nullptr, 'B', true, false, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(50, 44));
   }
   CHK_EXEC(SetMinorSchemaVersion(7));
   return true;
}

/**
 * Upgrade from 51.5 to 51.6
 */
static bool H_UpgradeFromV5()
{
   CHK_EXEC(DBRenameColumn(g_dbHandle, _T("nodes"), _T("capabilities"), _T("capabilities_32bit")));
   CHK_EXEC(SQLQuery(_T("ALTER TABLE nodes ADD capabilities $SQL:INT64")));
   CHK_EXEC(SQLQuery(_T("UPDATE nodes SET capabilities=capabilities_32bit")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("nodes"), _T("capabilities")));
   CHK_EXEC(DBDropColumn(g_dbHandle, _T("nodes"), _T("capabilities_32bit")));

   CHK_EXEC(CreateTable(
         _T("CREATE TABLE vnc_credentials (")
         _T("   zone_uin integer not null,")
         _T("   id integer not null,")
         _T("   password varchar(63) null,")
         _T("   PRIMARY KEY(zone_uin,id))")));

   static const TCHAR *batch =
      _T("ALTER TABLE nodes ADD vnc_password varchar(63)\n")
      _T("ALTER TABLE nodes ADD vnc_port integer\n")
      _T("ALTER TABLE nodes ADD vnc_proxy integer\n")
      _T("UPDATE nodes SET vnc_port=0,vnc_proxy=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("nodes"), _T("vnc_port")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("nodes"), _T("vnc_proxy")));

   CHK_EXEC(SetMinorSchemaVersion(6));
   return true;
}

/**
 * Upgrade from 51.4 to 51.5
 */
static bool H_UpgradeFromV4()
{
   CHK_EXEC(CreateConfigParam(_T("Objects.Interfaces.ClearPeerOnUnmanage"),
                              _T("0"),
                              _T("If set to true, interface peer will be cleared when interface is unmanaged."),
                              nullptr, 'B', true, false, false, false));
   CHK_EXEC(SetMinorSchemaVersion(5));
   return true;
}

/**
 * Upgrade from 51.3 to 51.4
 */
static bool H_UpgradeFromV3()
{
   static const TCHAR *batch =
      _T("ALTER TABLE access_points ADD down_since integer\n")
      _T("UPDATE access_points SET down_since=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("access_points"), _T("down_since")));
   CHK_EXEC(SetMinorSchemaVersion(4));
   return true;
}

/**
 * Upgrade from 51.2 to 51.3
 */
static bool H_UpgradeFromV2()
{
   CHK_EXEC(CreateConfigParam(_T("Topology.AdHocRequest.IncludePhysicalLinks"), _T("0"), _T("If set to true, physical links will be added to ad-hoc L2 maps."), nullptr, 'B', true, false, true, false));
   CHK_EXEC(SetMinorSchemaVersion(3));
   return true;
}

/**
 * Upgrade from 51.1 to 51.2
 */
static bool H_UpgradeFromV1()
{
   CHK_EXEC(SQLQuery( _T("UPDATE config SET need_server_restart=0 WHERE var_name='AgentPolicy.MaxFileSize'")));
   CHK_EXEC(SetMinorSchemaVersion(2));
   return true;
}

/**
 * Upgrade from 51.0 to 51.1
 */
static bool H_UpgradeFromV0()
{
   CHK_EXEC(CreateEventTemplate(EVENT_CIRCUIT_AUTOBIND, _T("SYS_CIRCUIT_AUTOBIND"),
         EVENT_SEVERITY_NORMAL, EF_LOG, _T("53372caa-f00b-40d9-961b-ca41109c91c7"),
         _T("Interface %2 on node %6 automatically bound to circuit %4"),
         _T("Generated when interface bound to circuit by autobind rule.\r\n")
         _T("Parameters:\r\n")
         _T("   1) interfaceId - Interface ID\r\n")
         _T("   2) interfaceName - Interface name\r\n")
         _T("   3) circuitId - Circuit ID\r\n")
         _T("   4) circuitName - Circuit name\r\n")
         _T("   5) nodeId - Interface owning node ID\r\n")
         _T("   6) nodeName - Interface owning node name")
      ));

   CHK_EXEC(CreateEventTemplate(EVENT_CIRCUIT_AUTOUNBIND, _T("SYS_CIRCUIT_AUTOUNBIND"),
         EVENT_SEVERITY_NORMAL, EF_LOG, _T("bc04d7c0-f2f6-4558-9842-6f751c3657b1"),
         _T("Interface %2 on node %6 automatically unbound from circuit %4"),
         _T("Generated when interface unbound from circuit by autobind rule.\r\n")
         _T("Parameters:\r\n")
         _T("   1) interfaceId - Interface ID\r\n")
         _T("   2) interfaceName - Interface name\r\n")
         _T("   3) circuitId - Circuit ID\r\n")
         _T("   4) circuitName - Circuit name\r\n")
         _T("   5) nodeId - Interface owning node ID\r\n")
         _T("   6) nodeName - Interface owning node name")
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
} s_dbUpgradeMap[] = {
   { 6,  51, 7,  H_UpgradeFromV6  },
   { 5,  51, 6,  H_UpgradeFromV5  },
   { 4,  51, 5,  H_UpgradeFromV4  },
   { 3,  51, 4,  H_UpgradeFromV3  },
   { 2,  51, 3,  H_UpgradeFromV2  },
   { 1,  51, 2,  H_UpgradeFromV1  },
   { 0,  51, 1,  H_UpgradeFromV0  },
   { 0,  0,  0,  nullptr }
};

/**
 * Upgrade database to new version
 */
bool MajorSchemaUpgrade_V51()
{
   int32_t major, minor;
   if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
      return false;

   while ((major == 51) && (minor < DB_SCHEMA_VERSION_V51_MINOR))
   {
      // Find upgrade procedure
      int i;
      for (i = 0; s_dbUpgradeMap[i].upgradeProc != nullptr; i++)
         if (s_dbUpgradeMap[i].version == minor)
            break;
      if (s_dbUpgradeMap[i].upgradeProc == nullptr)
      {
         _tprintf(_T("Unable to find upgrade procedure for version 51.%d\n"), minor);
         return false;
      }
      _tprintf(_T("Upgrading from version 51.%d to %d.%d\n"), minor, s_dbUpgradeMap[i].nextMajor, s_dbUpgradeMap[i].nextMinor);
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
