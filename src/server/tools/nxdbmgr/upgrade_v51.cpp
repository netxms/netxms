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
 * Upgrade from 51.16 to 51.17
 */
static bool H_UpgradeFromV16()
{
   static const TCHAR *batch =
      _T("ALTER TABLE items ADD transformed_datatype integer\n")
      _T("UPDATE items SET transformed_datatype=6\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("items"), _T("transformed_datatype")));

   CHK_EXEC(SetMinorSchemaVersion(17));
   return true;
}

/**
 * Upgrade from 51.15 to 51.16
 */
static bool H_UpgradeFromV15()
{
   CHK_EXEC(SQLQuery(_T("DELETE FROM event_cfg WHERE event_code=30"))); // Delete old event with same code (SYS_SMS_FAILURE) if still there
   CHK_EXEC(CreateEventTemplate(EVENT_ALL_THRESHOLDS_REARMED, _T("SYS_ALL_THRESHOLDS_REARMED"),
         EVENT_SEVERITY_NORMAL, EF_LOG, _T("12ecc8fa-e39e-497b-ad9f-a7786fcbcf46"),
         _T("All thresholds rearmed for data collection item %<dciDescription> (Metric: %<dciName>)"),
         _T("Generated when all thresholds are rearmed for specific data collection item.\r\n")
         _T("Parameters are accessible via %<...> and can have \"m\" or \"multipliers\" and \"u\" or \"units\" format modifiers for value formatting (for example %<{m,u}currentValue>).\r\n\r\n")
         _T("Parameters:\r\n")
         _T("   1) dciName - Metric name\r\n")
         _T("   2) dciDescription - Data collection item description\r\n")
         _T("   3) dciId - Data collection item ID\r\n")
         _T("   4) instance - Instance\r\n")
         _T("   5) dciValue - Last collected DCI value")
      ));

   static const TCHAR *batch =
      _T("ALTER TABLE items ADD all_rearmed_event integer\n")
      _T("UPDATE items SET all_rearmed_event=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("items"), _T("all_rearmed_event")));

   CHK_EXEC(SetMinorSchemaVersion(16));
   return true;
}

/**
 * Upgrade from 51.14 to 51.15
 */
static bool H_UpgradeFromV14()
{
   CHK_EXEC(CreateConfigParam(_T("Client.DefaultLineChartPeriod"),
                              _T("60"),
                              _T("Default period (in minutes) to display collected data for when opening ad-hoc line chart."),
                              _T("minutes"), 'I', true, false, false, false));
   CHK_EXEC(SetMinorSchemaVersion(15));
   return true;
}

/**
 * Upgrade from 51.13 to 51.14
 */
static bool H_UpgradeFromV13()
{
   CHK_EXEC(CreateConfigParam(_T("Client.ObjectBrowser.ShowTargetsUnderTemplates"),
                              _T("0"),
                              _T("If enabled, template target objects (nodes, access points, etc.) will be shown under templates applied to those objects."),
                              nullptr, 'B', true, false, false, false));
   CHK_EXEC(SetMinorSchemaVersion(14));
   return true;
}

/**
 * Upgrade from 51.12 to 51.13
 */
static bool H_UpgradeFromV12()
{
   if (GetSchemaLevelForMajorVersion(50) < 46)
   {
      CHK_EXEC(CreateConfigParam(_T("Client.InactivityTimeout"),
                                 _T("0"),
                                 _T("User inactivity timeout in seconds. Client will disconnect if no user activity detected within given time interval. Value of 0 disables inactivity timeout."),
                                 _T("seconds"), 'I', true, false, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(50, 46));
   }
   CHK_EXEC(SetMinorSchemaVersion(13));
   return true;
}

/**
 * Upgrade from 51.11 to 51.12
 */
static bool H_UpgradeFromV11()
{
   CHK_EXEC(SQLQuery(_T("DROP TABLE licenses")));

   CHK_EXEC(CreateTable(
         _T("CREATE TABLE licenses (")
         _T("   id integer not null,")
         _T("   content varchar(2000) null,")
         _T("   PRIMARY KEY(id))")));

   CHK_EXEC(SetMinorSchemaVersion(12));
   return true;
}

/**
 * Upgrade from 51.10 to 51.11
 */
static bool H_UpgradeFromV10()
{
   static const TCHAR *batch =
      _T("ALTER TABLE policy_action_list ADD active char(1)\n")
      _T("UPDATE policy_action_list SET active='1'\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("policy_action_list"), _T("active")));

   CHK_EXEC(SetMinorSchemaVersion(11));
   return true;
}

/**
 * Upgrade from 51.9 to 51.10
 */
static bool H_UpgradeFromV9()
{
   if (GetSchemaLevelForMajorVersion(50) < 45)
   {
      DB_RESULT hResult = SQLSelect(_T("SELECT map_id,link_id,element_data FROM network_map_links WHERE element_data LIKE '%</style>   <style>%'"));
      if (hResult != nullptr)
      {
         int count = DBGetNumRows(hResult);
         DB_STATEMENT hStmt = DBPrepare(g_dbHandle, _T("UPDATE network_map_links SET element_data=? WHERE map_id=? AND link_id=?"), count > 1);
         if (hStmt != nullptr)
         {
            for (int i = 0; i < count; i++)
            {
               StringBuffer xml = DBGetFieldAsString(hResult, i, 2);
               ssize_t start = xml.find(_T("<style>"));
               ssize_t end = 0, tmp = start;
               while(tmp != -1)
               {
                  end = tmp;
                  tmp = xml.find(_T("</style>"), end + 8);
               }

               start += 7;
               end -= 1;
               xml.removeRange(start, end - start);

               DBBind(hStmt, 1, DB_SQLTYPE_TEXT, xml, DB_BIND_STATIC);
               DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, DBGetFieldULong(hResult, i, 0));
               DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, DBGetFieldULong(hResult, i, 1));
               if (!SQLExecute(hStmt) && !g_ignoreErrors)
               {
                  DBFreeResult(hResult);
                  DBFreeStatement(hStmt);
                  return false;
               }

            }
            DBFreeStatement(hStmt);
         }
         else if (!g_ignoreErrors)
         {
            DBFreeResult(hResult);
            return false;
         }

         DBFreeResult(hResult);
      }
      else
      {
         if (!g_ignoreErrors)
            return false;
      }
      CHK_EXEC(SetSchemaLevelForMajorVersion(50, 45));
   }
   CHK_EXEC(SetMinorSchemaVersion(10));
   return true;
}

/**
 * Upgrade from 51.8 to 51.9
 */
static bool H_UpgradeFromV8()
{
   CHK_EXEC(CreateConfigParam(_T("Client.ObjectOverview.ShowCommentsOnlyIfPresent"),
                              _T("1"),
                              _T("If enabled, comments section in object overview will only be shown when object comments are not empty."),
                              nullptr, 'B', true, false, false, false));
   CHK_EXEC(SetMinorSchemaVersion(9));
   return true;
}

/**
 * Upgrade from 51.7 to 51.8
 */
static bool H_UpgradeFromV7()
{
   CHK_EXEC(SQLQuery(_T("ALTER TABLE software_inventory ADD uninstall_key varchar(255)")));
   CHK_EXEC(SetMinorSchemaVersion(8));
   return true;
}

/**
 * Upgrade from 51.6 to 51.7
 */
static bool H_UpgradeFromV6()
{
   if (GetSchemaLevelForMajorVersion(50) < 44)
   {
      CHK_EXEC(CreateConfigParam(_T("AgentTunnels.BindByIPAddress"),
                                 _T("0"),
                                 _T("Enable/disable agent tunnel binding by IP address. If enabled and agent certificate is not provided, tunnel will be bound to node with IP address matching tunnel source IP address."),
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
   { 16, 51, 17, H_UpgradeFromV16 },
   { 15, 51, 16, H_UpgradeFromV15 },
   { 14, 51, 15, H_UpgradeFromV14 },
   { 13, 51, 14, H_UpgradeFromV13 },
   { 12, 51, 13, H_UpgradeFromV12 },
   { 11, 51, 12, H_UpgradeFromV11 },
   { 10, 51, 11, H_UpgradeFromV10 },
   { 9,  51, 10, H_UpgradeFromV9  },
   { 8,  51, 9,  H_UpgradeFromV8  },
   { 7,  51, 8,  H_UpgradeFromV7  },
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
