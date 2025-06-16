/*
** nxdbmgr - NetXMS database manager
** Copyright (C) 2022-2025 Raden Solutions
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
 * Upgrade from 51.27 to 52.0
 */
static bool H_UpgradeFromV27()
{
   CHK_EXEC(SetMajorSchemaVersion(52, 0));
   return true;
}

/**
 * Upgrade from 51.26 to 51.27
 */
static bool H_UpgradeFromV26()
{
   if (GetSchemaLevelForMajorVersion(50) < 49)
   {
      CHK_EXEC(CreateEventTemplate(EVENT_RESPONSIBLE_USER_ADDED, _T("SYS_RESPONSIBLE_USER_ADDED"),
            EVENT_SEVERITY_NORMAL, 0, _T("27e38dfb-1027-454a-8cd9-fbc49dcf0a9c"),
            _T("New responsible user %<userName> (ID: %<userId>, Tag: \"%<tag>\") added to object %<objectName> (ID: %<objectId>) by %<operator>"),
            _T("Generated when new responsible user added to the object.\r\n")
            _T("Parameters:\r\n")
            _T("   1) userId - User ID\r\n")
            _T("   2) userName - User name\r\n")
            _T("   3) tag - User tag\r\n")
            _T("   4) objectId - Object ID\r\n")
            _T("   5) objectName - Object name\r\n")
            _T("   6) operator - Operator (user who made change to the object)")
         ));

      CHK_EXEC(CreateEventTemplate(EVENT_RESPONSIBLE_USER_REMOVED, _T("SYS_RESPONSIBLE_USER_REMOVED"),
            EVENT_SEVERITY_NORMAL, 0, _T("c17409f9-1213-4c48-8249-62caa79a01c5"),
            _T("Responsible user %<userName> (ID: %<userId>, Tag: \"%<tag>\") removed from object %<objectName> (ID: %<objectId>) by %<operator>"),
            _T("Generated when new responsible user added to the object.\r\n")
            _T("Parameters:\r\n")
            _T("   1) userId - User ID\r\n")
            _T("   2) userName - User name\r\n")
            _T("   3) tag - User tag\r\n")
            _T("   4) objectId - Object ID\r\n")
            _T("   5) objectName - Object name\r\n")
            _T("   6) operator - Operator (user who made change to the object)")
         ));

      CHK_EXEC(CreateEventTemplate(EVENT_RESPONSIBLE_USER_MODIFIED, _T("SYS_RESPONSIBLE_USER_MODIFIED"),
            EVENT_SEVERITY_NORMAL, 0, _T("84baa8d0-f70c-4d92-b243-7fe5e7df0fed"),
            _T("Responsible user %<userName> (ID: %<userId>) tag on object %<objectName> (ID: %<objectId>) changed from \"%<prevTag>\" to \"%<tag>\" by %<operator>"),
            _T("Generated when the responsible user's record for the object is modified.\r\n")
            _T("Parameters:\r\n")
            _T("   1) userId - User ID\r\n")
            _T("   2) userName - User name\r\n")
            _T("   3) tag - User tag\r\n")
            _T("   4) prevTag - Old user tag\r\n")
            _T("   5) objectId - Object ID\r\n")
            _T("   6) objectName - Object name\r\n")
            _T("   7) operator - Operator (user who made change to the object)")
         ));

      CHK_EXEC(SetSchemaLevelForMajorVersion(50, 49));
   }
   CHK_EXEC(SetMinorSchemaVersion(27));
   return true;
}

/**
 * Upgrade from 51.25 to 51.26
 */
static bool H_UpgradeFromV25()
{
   CHK_EXEC(DBDropPrimaryKey(g_dbHandle, L"scheduled_tasks"));
   CHK_EXEC(DBRenameColumn(g_dbHandle, L"scheduled_tasks", L"id", L"old_id"));

   static const wchar_t *batch =
      L"ALTER TABLE scheduled_tasks ADD id $SQL:INT64\n"
      L"UPDATE scheduled_tasks SET id=old_id\n"
      L"<END>";
   CHK_EXEC(SQLBatch(batch));

   CHK_EXEC(DBDropColumn(g_dbHandle, L"scheduled_tasks", L"old_id"));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, L"scheduled_tasks", L"id"));
   CHK_EXEC(DBAddPrimaryKey(g_dbHandle, L"scheduled_tasks", L"id"));

   CHK_EXEC(SetMinorSchemaVersion(26));
   return true;
}

/**
 * Upgrade from 51.24 to 51.25
 */
static bool H_UpgradeFromV24()
{
   CHK_EXEC(SQLQuery(L"ALTER TABLE licenses ADD guid varchar(36)"));

   DB_RESULT hResult = SQLSelect(L"SELECT id FROM licenses");
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         int32_t id = htonl(DBGetFieldInt32(hResult, i, 0));
         uint8_t guid[UUID_LENGTH];
         memset(guid, 0, sizeof(uuid_t) - sizeof(int32_t));
         memcpy(&guid[UUID_LENGTH - sizeof(int32_t)], &id, sizeof(int32_t));

         wchar_t query[128];
         _sntprintf(query, 128, L"UPDATE licenses SET guid='%s' WHERE id=%d", uuid(guid).toString().cstr(), ntohl(id));
         CHK_EXEC(SQLQuery(query));
      }
      DBFreeResult(hResult);
   }
   else if (!g_ignoreErrors)
   {
      return false;
   }

   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, L"licenses", L"guid"));
   CHK_EXEC(SetMinorSchemaVersion(25));
   return true;
}

/**
 * Upgrade from 51.23 to 51.24
 */
static bool H_UpgradeFromV23()
{
   CHK_EXEC(CreateConfigParam(_T("Objects.Nodes.ConfigurationPoll.AlwaysCheckSNMP"),
         _T("1"),
         _T("Always check possible SNMP credentials during configuration poll, even if node is marked as unreachable via SNMP."),
         nullptr, 'B', true, false, false, false));
   CHK_EXEC(SetMinorSchemaVersion(24));
   return true;
}

/**
 * Upgrade from 51.22 to 51.23
 */
static bool H_UpgradeFromV22()
{
   if (GetSchemaLevelForMajorVersion(50) < 48)
   {
      CHK_EXEC(CreateConfigParam(_T("Client.MinVersion"),
            _T(""),
            _T("Minimal client version allowed for connection to this server"),
            nullptr, 'S', true, false, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(50, 48));
   }
   CHK_EXEC(SetMinorSchemaVersion(23));
   return true;
}

/**
 * Upgrade from 51.21 to 51.22
 */
static bool H_UpgradeFromV21()
{
   if (GetSchemaLevelForMajorVersion(50) < 47)
   {
      CHK_EXEC(CreateConfigParam(_T("ReportingServer.JDBC.Properties"),
                                 _T(""),
                                 _T("Additional properties for JDBC connector on reporting server."),
                                 nullptr, 'S', true, false, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(50, 47));
   }
   CHK_EXEC(SetMinorSchemaVersion(22));
   return true;
}

/**
 * Upgrade from 51.20 to 51.21
 */
static bool H_UpgradeFromV20()
{
   TCHAR definition[256];
   if (!DBGetColumnDataType(g_dbHandle, _T("access_points"), _T("down_since"), definition, 256))
   {
      static const TCHAR *batch =
         _T("ALTER TABLE access_points ADD down_since integer\n")
         _T("UPDATE access_points SET down_since=0\n")
         _T("<END>");
      CHK_EXEC(SQLBatch(batch));
      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("access_points"), _T("down_since")));
   }
   CHK_EXEC(SetMinorSchemaVersion(21));
   return true;
}

/**
 * Upgrade from 51.19 to 51.20
 */
static bool H_UpgradeFromV19()
{
   int ruleId = NextFreeEPPruleID();

   int count = 0;
   DB_RESULT hResult = SQLSelect(_T("SELECT rule_id FROM event_policy WHERE rule_guid='b76f142a-6932-499e-aa6e-ac16cf8effb1'"));
   if (hResult != nullptr)
   {
      count = DBGetNumRows(hResult);
      DBFreeResult(hResult);
   }
   else
   {
      if (!g_ignoreErrors)
         return false;
   }

   TCHAR query[1024];
   if (count == 0)
   {
      _sntprintf(query, 1024, _T("INSERT INTO event_policy (rule_id,rule_guid,flags,comments,alarm_message,alarm_severity,alarm_key,filter_script,alarm_timeout,alarm_timeout_event) ")
                              _T("VALUES (%d,'b76f142a-6932-499e-aa6e-ac16cf8effb1',7976,'Terminate alarms for hanged or unexpectedly stopped system threads that could have been created prior to server restart','%%m',6,'SYS_THREAD_HANG_.*','',0,%d)"),
            ruleId, EVENT_ALARM_TIMEOUT);
      CHK_EXEC(SQLQuery(query));

      _sntprintf(query, 1024, _T("INSERT INTO policy_event_list (rule_id,event_code) VALUES (%d,%d)"), ruleId, EVENT_SERVER_STARTED);
      CHK_EXEC(SQLQuery(query));

      ruleId++;
   }

   _sntprintf(query, 1024, _T("INSERT INTO event_policy (rule_id,rule_guid,flags,comments,alarm_message,alarm_severity,alarm_key,filter_script,alarm_timeout,alarm_timeout_event,downtime_tag) ")
                           _T("VALUES (%d,'202d5b85-48fc-4b6e-9da9-53a6eb9a341d',89856,'Start downtime','%%m',5,'','',0,%d,'NODE_DOWN')"),
         ruleId, EVENT_ALARM_TIMEOUT);
   CHK_EXEC(SQLQuery(query));

   _sntprintf(query, 1024, _T("INSERT INTO policy_event_list (rule_id,event_code) VALUES (%d,%d)"), ruleId, EVENT_NODE_DOWN);
   CHK_EXEC(SQLQuery(query));
   _sntprintf(query, 1024, _T("INSERT INTO policy_event_list (rule_id,event_code) VALUES (%d,%d)"), ruleId, EVENT_NODE_UNREACHABLE);
   CHK_EXEC(SQLQuery(query));

   ruleId++;

   _sntprintf(query, 1024, _T("INSERT INTO event_policy (rule_id,rule_guid,flags,comments,alarm_message,alarm_severity,alarm_key,filter_script,alarm_timeout,alarm_timeout_event,downtime_tag) ")
                           _T("VALUES (%d,'256c2abd-3e14-4e02-9606-7c78c77e1a78',139008,'End downtime','%%m',5,'','',0,%d,'NODE_DOWN')"),
         ruleId, EVENT_ALARM_TIMEOUT);
   CHK_EXEC(SQLQuery(query));

   _sntprintf(query, 1024, _T("INSERT INTO policy_event_list (rule_id,event_code) VALUES (%d,%d)"), ruleId, EVENT_NODE_UP);
   CHK_EXEC(SQLQuery(query));

   CHK_EXEC(SetMinorSchemaVersion(20));
   return true;
}

/**
 * Upgrade from 51.18 to 51.19
 */
static bool H_UpgradeFromV18()
{
   static const TCHAR *batch =
      _T("ALTER TABLE nodes ADD path_check_reason integer\n")
      _T("ALTER TABLE nodes ADD path_check_node_id integer\n")
      _T("ALTER TABLE nodes ADD path_check_iface_id integer\n")
      _T("UPDATE nodes SET path_check_reason=0,path_check_node_id=0,path_check_iface_id=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("nodes"), _T("path_check_reason")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("nodes"), _T("path_check_node_id")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("nodes"), _T("path_check_iface_id")));
   CHK_EXEC(SetMinorSchemaVersion(19));
   return true;
}

/**
 * Upgrade from 51.17 to 51.18
 */
static bool H_UpgradeFromV17()
{
   CHK_EXEC(DBResizeColumn(g_dbHandle, _T("access_points"), _T("mac_address"), 16, true));
   CHK_EXEC(DBResizeColumn(g_dbHandle, _T("interfaces"), _T("mac_addr"), 16, true));
   CHK_EXEC(SetMinorSchemaVersion(18));
   return true;
}

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
   { 27, 52, 0,  H_UpgradeFromV27 },
   { 26, 52, 27, H_UpgradeFromV26 },
   { 25, 52, 26, H_UpgradeFromV25 },
   { 24, 52, 25, H_UpgradeFromV24 },
   { 23, 52, 24, H_UpgradeFromV23 },
   { 22, 52, 23, H_UpgradeFromV22 },
   { 21, 51, 22, H_UpgradeFromV21 },
   { 20, 51, 21, H_UpgradeFromV20 },
   { 19, 51, 20, H_UpgradeFromV19 },
   { 18, 51, 19, H_UpgradeFromV18 },
   { 17, 51, 18, H_UpgradeFromV17 },
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

   while (major == 51)
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
