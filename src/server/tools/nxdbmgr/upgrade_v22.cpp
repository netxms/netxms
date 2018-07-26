/*
** nxdbmgr - NetXMS database manager
** Copyright (C) 2004-2018 Victor Kirhenshtein
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
#include <nxevent.h>

/**
 * Upgrade from 22.33 to 22.34
 */
static bool H_UpgradeFromV33()
{
   static const TCHAR *batch =
      _T("ALTER TABLE event_log ADD raw_data $SQL:TEXT\n")
      _T("ALTER TABLE scheduled_tasks ADD task_key varchar(255)\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   CHK_EXEC(SetMinorSchemaVersion(34));
   return true;
}

/**
 * Upgrade from 22.32 to 22.33
 */
static bool H_UpgradeFromV32()
{
   static const TCHAR *batch =
      _T("ALTER TABLE policy_action_list ADD timer_delay integer\n")
      _T("ALTER TABLE policy_action_list ADD timer_key varchar(255)\n")
      _T("UPDATE policy_action_list SET timer_delay=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_hCoreDB, _T("policy_action_list"), _T("timer_delay")));

   CHK_EXEC(CreateTable(
      _T("CREATE TABLE policy_timer_cancellation_list (")
      _T("   rule_id integer not null,")
      _T("   timer_key varchar(255) not null,")
      _T("   PRIMARY KEY(rule_id,timer_key))")));

   CHK_EXEC(SetMinorSchemaVersion(33));
   return true;
}

/**
 * Upgrade from 22.31 to 22.32
 */
static bool H_UpgradeFromV31()
{
   CHK_EXEC(CreateLibraryScript(17, _T("ee6dd107-982b-4ad1-980b-fc0cc7a03911"), _T("Hook::DiscoveryPoll"),
            _T("/* Available global variables:\r\n *  $node - current node, object of 'Node' type\r\n *\r\n * Expected return value:\r\n *  none - returned value is ignored\r\n */\r\n")));
   CHK_EXEC(CreateLibraryScript(18, _T("a02ea666-e1e9-4f98-a746-1c3ce19428e9"), _T("Hook::PostObjectCreate"),
            _T("/* Available global variables:\r\n *  $object - current object, one of 'NetObj' subclasses\r\n *  $node - current object if it is 'Node' class\r\n *\r\n * Expected return value:\r\n *  none - returned value is ignored\r\n */\r\n")));
   CHK_EXEC(SetMinorSchemaVersion(32));
   return true;
}

/**
 * Upgrade from 22.30 to 22.31
 */
static bool H_UpgradeFromV30()
{
   CHK_EXEC(CreateTable(
      _T("CREATE TABLE interface_vlan_list (")
      _T("   iface_id integer not null,")
      _T("   vlan_id integer not null,")
      _T("   PRIMARY KEY(iface_id,vlan_id))")));
   CHK_EXEC(SetMinorSchemaVersion(31));
   return true;
}

/**
 * Upgrade from 22.29 to 22.30
 */
static bool H_UpgradeFromV29()
{
   CHK_EXEC(SQLQuery(_T("ALTER TABLE nodes ADD hypervisor_type varchar(31)")));
   CHK_EXEC(SQLQuery(_T("ALTER TABLE nodes ADD hypervisor_info varchar(255)")));
   CHK_EXEC(SetMinorSchemaVersion(30));
   return true;
}

/**
 * Upgrade from 22.28 to 22.29
 */
static bool H_UpgradeFromV28()
{
   DB_STATEMENT hStmt = DBPrepare(g_hCoreDB, _T("UPDATE event_cfg SET description=? WHERE event_code=?"));
   if (hStmt != NULL)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_TEXT,
               _T("Generated when agent tunnel is open and bound.\r\n")
               _T("Parameters:\r\n")
               _T("   1) Tunnel ID (tunnelId)\r\n")
               _T("   2) Remote system IP address (ipAddress)\r\n")
               _T("   3) Remote system name (systemName)\r\n")
               _T("   4) Remote system FQDN (hostName)\r\n")
               _T("   5) Remote system platform (platformName)\r\n")
               _T("   6) Remote system information (systemInfo)\r\n")
               _T("   7) Agent version (agentVersion)\r\n")
               _T("   8) Agent ID (agentId)"),
               DB_BIND_STATIC);
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, EVENT_TUNNEL_OPEN);
      bool success = DBExecute(hStmt);
      if (!success && !g_bIgnoreErrors)
      {
         DBFreeStatement(hStmt);
         return false;
      }

      DBBind(hStmt, 1, DB_SQLTYPE_TEXT,
               _T("Generated when agent tunnel is closed.\r\n")
               _T("Parameters:\r\n")
               _T("   1) Tunnel ID (tunnelId)\r\n")
               _T("   2) Remote system IP address (ipAddress)\r\n")
               _T("   3) Remote system name (systemName)\r\n")
               _T("   4) Remote system FQDN (hostName)\r\n")
               _T("   5) Remote system platform (platformName)\r\n")
               _T("   6) Remote system information (systemInfo)\r\n")
               _T("   7) Agent version (agentVersion)\r\n")
               _T("   8) Agent ID (agentId)"),
               DB_BIND_STATIC);
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, EVENT_TUNNEL_CLOSED);
      success = DBExecute(hStmt);
      if (!success && !g_bIgnoreErrors)
      {
         DBFreeStatement(hStmt);
         return false;
      }

      DBBind(hStmt, 1, DB_SQLTYPE_TEXT,
               _T("Generated when unbound agent tunnel is not bound or closed for more than configured threshold.\r\n")
               _T("Parameters:\r\n")
               _T("   1) Tunnel ID (tunnelId)\r\n")
               _T("   2) Remote system IP address (ipAddress)\r\n")
               _T("   3) Remote system name (systemName)\r\n")
               _T("   4) Remote system FQDN (hostName)\r\n")
               _T("   5) Remote system platform (platformName)\r\n")
               _T("   6) Remote system information (systemInfo)\r\n")
               _T("   7) Agent version (agentVersion)\r\n")
               _T("   8) Agent ID (agentId)\r\n")
               _T("   9) Configured idle timeout (idleTimeout)"),
               DB_BIND_STATIC);
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, EVENT_UNBOUND_TUNNEL);
      success = DBExecute(hStmt);
      if (!success && !g_bIgnoreErrors)
      {
         DBFreeStatement(hStmt);
         return false;
      }

      DBFreeStatement(hStmt);
   }
   else
   {
      if (!g_bIgnoreErrors)
         return false;
   }

   CHK_EXEC(CreateEventTemplate(EVENT_TUNNEL_AGENT_ID_MISMATCH, _T("SYS_TUNNEL_AGENT_ID_MISMATCH"), SEVERITY_WARNING, EF_LOG, _T("30792e3d-f94a-4866-9616-457ba3ac276a"),
            _T("Agent ID %<nodeAgentId> on node do not match agent ID %<tunnelAgentId> on tunnel from %<systemName> (%<ipAddress>) at bind"),
            _T("Generated when agent ID mismatch detected during tunnel bind.\r\n")
            _T("Parameters:\r\n")
            _T("   1) Tunnel ID (tunnelId)\r\n")
            _T("   2) Remote system IP address (ipAddress)\r\n")
            _T("   3) Remote system name (systemName)\r\n")
            _T("   4) Remote system FQDN (hostName)\r\n")
            _T("   5) Remote system platform (platformName)\r\n")
            _T("   6) Remote system information (systemInfo)\r\n")
            _T("   7) Agent version (agentVersion)\r\n")
            _T("   8) Tunnel agent ID (tunnelAgentId)\r\n")
            _T("   9) Node agent ID (nodeAgentId)")
            ));

   CHK_EXEC(SetMinorSchemaVersion(29));
   return true;
}

/**
 * Upgrade from 22.27 to 22.28
 */
static bool H_UpgradeFromV27()
{
   CHK_EXEC(CreateEventTemplate(EVENT_TUNNEL_OPEN, _T("SYS_TUNNEL_OPEN"), SEVERITY_NORMAL, EF_LOG, _T("2569c729-1f8c-4a13-9e75-a1d0c1995bc2"),
            _T("Agent tunnel from %<systemName> (%<ipAddress>) is open"),
            _T("Generated when agent tunnel is open and bound.\r\n")
            _T("Parameters:\r\n")
            _T("   1) Tunnel ID (tunnelId)\r\n")
            _T("   2) Remote system IP address (ipAddress)\r\n")
            _T("   3) Remote system name (systemName)\r\n")
            _T("   4) Remote system FQDN (hostName)\r\n")
            _T("   5) Remote system platform (platformName)\r\n")
            _T("   6) Remote system information (systemInfo)\r\n")
            _T("   7) Agent version (agentVersion)")
            ));
   CHK_EXEC(CreateEventTemplate(EVENT_TUNNEL_CLOSED, _T("SYS_TUNNEL_CLOSED"), SEVERITY_WARNING, EF_LOG, _T("50a61266-710d-48d7-b620-9eaa0f85a94f"),
            _T("Agent tunnel from %<systemName> (%<ipAddress>) is closed"),
            _T("Generated when agent tunnel is closed.\r\n")
            _T("Parameters:\r\n")
            _T("   1) Tunnel ID (tunnelId)\r\n")
            _T("   2) Remote system IP address (ipAddress)\r\n")
            _T("   3) Remote system name (systemName)\r\n")
            _T("   4) Remote system FQDN (hostName)\r\n")
            _T("   5) Remote system platform (platformName)\r\n")
            _T("   6) Remote system information (systemInfo)\r\n")
            _T("   7) Agent version (agentVersion)")
            ));
   CHK_EXEC(SetMinorSchemaVersion(28));
   return true;
}

/**
 * Upgrade from 22.26 to 22.27
 */
static bool H_UpgradeFromV26()
{
   CHK_EXEC(SQLQuery(_T("UPDATE config SET var_name='Client.AlarmList.DisplayLimit' WHERE var_name='AlarmListDisplayLimit'")));
   CHK_EXEC(CreateConfigParam(_T("Client.ObjectBrowser.AutoApplyFilter"), _T("1"), _T("Enable or disable object browser's filter applying as user types (if disabled, user has to press ENTER to apply filter)."), 'B', true, false, false, false));
   CHK_EXEC(CreateConfigParam(_T("Client.ObjectBrowser.FilterDelay"), _T("300"), _T("Delay between typing in object browser''s filter and applying it to object tree."), 'I', true, false, false, false));
   CHK_EXEC(CreateConfigParam(_T("Client.ObjectBrowser.MinFilterStringLength"), _T("1"), _T("Minimal length of filter string in object browser required for automatic apply."), 'I', true, false, false, false));
   CHK_EXEC(SetMinorSchemaVersion(27));
   return true;
}

/**
 * Upgrade from 22.25 to 22.26
 */
static bool H_UpgradeFromV25()
{
   CHK_EXEC(CreateTable(
      _T("CREATE TABLE certificate_action_log (")
      _T("   record_id integer not null,")
      _T("   timestamp integer not null,")
      _T("   operation integer not null,")
      _T("   user_id integer not null,")
      _T("   node_id integer not null,")
      _T("   node_guid varchar(36) null,")
      _T("   cert_type integer not null,")
      _T("   subject varchar(255) null,")
      _T("   serial integer null,")
      _T("   PRIMARY KEY(record_id))")));

   CHK_EXEC(SQLQuery(_T("ALTER TABLE nodes ADD agent_id varchar(36)")));
   CHK_EXEC(SQLQuery(_T("ALTER TABLE nodes ADD agent_cert_subject varchar(500)")));

   CHK_EXEC(CreateEventTemplate(EVENT_AGENT_ID_CHANGED, _T("SYS_AGENT_ID_CHANGED"), SEVERITY_WARNING, EF_LOG,
            _T("741f0abc-1e69-46e4-adbc-bf1c4ed8549a"),
            _T("Agent ID changed from %1 to %2"),
            _T("Generated when agent ID change detected.\r\n")
            _T("Parameters:\r\n")
            _T("   1) Old agent ID\r\n")
            _T("   2) New agent ID")
            ));

   CHK_EXEC(SetMinorSchemaVersion(26));
   return true;
}

/**
 * Upgrade from 22.24 to 22.25
 */
static bool H_UpgradeFromV24()
{
   CHK_EXEC(SQLQuery(_T("ALTER TABLE actions ADD guid varchar(36)")));
   CHK_EXEC(GenerateGUID(_T("actions"), _T("action_id"), _T("guid"), NULL));
   DBSetNotNullConstraint(g_hCoreDB, _T("actions"), _T("guid"));
   CHK_EXEC(SetMinorSchemaVersion(25));
   return true;
}

/**
 * Upgrade from 22.23 to 22.24
 */
static bool H_UpgradeFromV23()
{
   CHK_EXEC(DBRenameColumn(g_hCoreDB, _T("dct_threshold_instances"), _T("row"), _T("row_number")));
   CHK_EXEC(SetMinorSchemaVersion(24));
   return true;
}

/**
 * Upgrade from 22.22 to 22.24
 */
static bool H_UpgradeFromV22()
{
   static const TCHAR *batch =
      _T("ALTER TABLE object_properties ADD state_before_maint integer\n")
      _T("ALTER TABLE dct_threshold_instances ADD row_number integer\n")
      _T("ALTER TABLE dct_threshold_instances ADD maint_copy char(1)\n")
      _T("ALTER TABLE thresholds ADD state_before_maint char(1)\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   CHK_EXEC(DBDropColumn(g_hCoreDB, _T("object_properties"), _T("maint_mode")));
   CHK_EXEC(SQLQuery(_T("UPDATE dct_threshold_instances SET maint_copy='0'")));
   CHK_EXEC(DBDropPrimaryKey(g_hCoreDB, _T("dct_threshold_instances")));
   CHK_EXEC(DBSetNotNullConstraint(g_hCoreDB, _T("dct_threshold_instances"), _T("maint_copy")));
   CHK_EXEC(DBAddPrimaryKey(g_hCoreDB, _T("dct_threshold_instances"), _T("threshold_id,instance,maint_copy")));

   CHK_EXEC(SetMinorSchemaVersion(24));
   return true;
}

/**
 * Upgrade from 22.21 to 22.22
 */
static bool H_UpgradeFromV21()
{
   CHK_EXEC(CreateConfigParam(_T("Alarms.IgnoreHelpdeskState"), _T("0"), _T("If set alarm helpdesk state will be ignored when resolving or terminating."), 'B', true, false, false, false));
   CHK_EXEC(SetMinorSchemaVersion(22));
   return true;
}

/**
 * Upgrade from 22.20 to 22.21
 */
static bool H_UpgradeFromV20()
{
   static const TCHAR *batch =
      _T("ALTER TABLE alarms ADD zone_uin integer\n")
      _T("ALTER TABLE event_log ADD zone_uin integer\n")
      _T("ALTER TABLE snmp_trap_log ADD zone_uin integer\n")
      _T("ALTER TABLE syslog ADD zone_uin integer\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   RegisterOnlineUpgrade(22, 21);

   CHK_EXEC(SetMinorSchemaVersion(21));
   return true;
}

/**
 * Upgrade from 22.19 to 22.20
 */
static bool H_UpgradeFromV19()
{
   CHK_EXEC(SQLQuery(_T("UPDATE config_values SET var_description='Don''t use aliases' WHERE var_name='UseInterfaceAliases' AND var_value='0'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET description='Resolve node name using DNS, SNMP system name, or host name if current node name is it''s IP address.' WHERE var_name='ResolveNodeNames'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET description='Number of hops from seed node to be added to topology map.' WHERE var_name='TopologyDiscoveryRadius'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET description='Network topology information expiration time. Server will use cached topology information if it is newer than given interval.' WHERE var_name='TopologyExpirationTime'")));
   CHK_EXEC(SetMinorSchemaVersion(20));
   return true;
}

/**
 * Upgrade from 22.18 to 22.19
 */
static bool H_UpgradeFromV18()
{
   CHK_EXEC(CreateConfigParam(_T("ThreadPool.Syncer.BaseSize"), _T("1"), _T("Base size for syncer thread pool."), 'I', true, true, false, false));
   CHK_EXEC(CreateConfigParam(_T("ThreadPool.Syncer.MaxSize"), _T("1"), _T("Maximum size for syncer thread pool (value of 1 will disable pool creation)."), 'I', true, true, false, false));
   CHK_EXEC(SetMinorSchemaVersion(19));
   return true;
}

/**
 * Upgrade from 22.17 to 22.18
 */
static bool H_UpgradeFromV17()
{
   CHK_EXEC(CreateConfigParam(_T("DBWriter.RawDataFlushInterval"), _T("30"), _T("Interval between writes of accumulated raw DCI data to database."), 'I', true, true, false, false));
   CHK_EXEC(SetMinorSchemaVersion(18));
   return true;
}

/**
 * Upgrade from 22.16 to 22.17
 */
static bool H_UpgradeFromV16()
{
   CHK_EXEC(CreateConfigParam(_T("DBWriter.DataQueues"), _T("1"), _T("Number of queues for DCI data writer."), 'I', true, true, false, false));
   CHK_EXEC(SetMinorSchemaVersion(17));
   return true;
}

/**
 * Upgrade from 22.15 to 22.16
 */
static bool H_UpgradeFromV15()
{
   CHK_EXEC(CreateConfigParam(_T("DataCollection.ScriptErrorReportInterval"), _T("86400"), _T("Minimal interval between reporting errors in data collection related script."), 'I', true, false, false, false));
   CHK_EXEC(SetMinorSchemaVersion(16));
   return true;
}

/**
 * Upgrade from 22.14 to 22.15
 */
static bool H_UpgradeFromV14()
{
   CHK_EXEC(CreateConfigParam(_T("NXSL.EnableFileIOFunctions"), _T("0"), _T("Enable/disable server-side NXSL functions for file I/O (such as OpenFile, DeleteFile, etc.)."), 'B', true, true, false, false));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET var_name='NXSL.EnableContainerFunctions',description='Enable/disable server-side NXSL functions for container management (such as CreateContainer, RemoveContainer, BindObject, UnbindObject).' WHERE var_name='EnableNXSLContainerFunctions'")));
   CHK_EXEC(SetMinorSchemaVersion(15));
   return true;
}

/**
 * Upgrade from 22.13 to 22.14
 */
static bool H_UpgradeFromV13()
{
   CHK_EXEC(CreateConfigParam(_T("DataCollection.OnDCIDelete.TerminateRelatedAlarms"), _T("1"), _T("Enable/disable automatic termination of related alarms when data collection item is deleted."), 'B', true, false, false, false));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='Inactivity time after which user account will be blocked (0 to disable blocking).' WHERE var_name='BlockInactiveUserAccounts'")));
   CHK_EXEC(SetMinorSchemaVersion(14));
   return true;
}

/**
 * Upgrade from 22.12 to 22.13
 */
static bool H_UpgradeFromV12()
{
   CHK_EXEC(CreateEventTemplate(EVENT_UNBOUND_TUNNEL, _T("SYS_UNBOUND_TUNNEL"), SEVERITY_NORMAL, EF_LOG, _T("7f781ec2-a8f5-4c02-ad7f-9e5b0a223b87"),
            _T("Unbound agent tunnel from %<systemName> (%<ipAddress>) is idle for more than %<idleTimeout> seconds"),
            _T("Generated when unbound agent tunnel is not bound or closed for more than configured threshold.\r\n")
            _T("Parameters:\r\n")
            _T("   1) Tunnel ID (tunnelId)\r\n")
            _T("   2) Remote system IP address (ipAddress)\r\n")
            _T("   3) Remote system name (systemName)\r\n")
            _T("   4) Remote system FQDN (hostName)\r\n")
            _T("   5) Remote system platform (platformName)\r\n")
            _T("   6) Remote system information (systemInfo)\r\n")
            _T("   7) Agent version (agentVersion)\r\n")
            _T("   8) Configured idle timeout (idleTimeout)")
            ));

   CHK_EXEC(CreateConfigParam(_T("AgentTunnels.NewNodesContainer"), _T(""), _T("Name of the container where nodes created automatically for unbound tunnels will be placed. If empty or missing, such nodes will be created in infrastructure services root."), 'S', true, false, false, false));
   CHK_EXEC(CreateConfigParam(_T("AgentTunnels.UnboundTunnelTimeout"), _T("3600"), _T("Unbound agent tunnels inactivity timeout. If tunnel is not bound or closed after timeout, action defined by AgentTunnels.UnboundTunnelTimeoutAction parameter will be taken."), 'I', true, false, false, false));
   CHK_EXEC(CreateConfigParam(_T("AgentTunnels.UnboundTunnelTimeoutAction"), _T("0"), _T("Action to be taken when unbound agent tunnel idle timeout expires."), 'C', true, false, false, false));

   static TCHAR batch[] =
      _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('AgentTunnels.UnboundTunnelTimeoutAction','0','Reset tunnel')\n")
      _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('AgentTunnels.UnboundTunnelTimeoutAction','1','Generate event')\n")
      _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('AgentTunnels.UnboundTunnelTimeoutAction','2','Bind tunnel to existing node')\n")
      _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('AgentTunnels.UnboundTunnelTimeoutAction','3','Bind tunnel to existing node or create new node')\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET description='Generated when new node object added to the database.\r\nParameters:\r\n   1) Node origin (0 = created manually, 1 = created by network discovery, 2 = created by tunnel auto bind)' WHERE event_code=1")));

   CHK_EXEC(SetMinorSchemaVersion(13));
   return true;
}

/**
 * Upgrade from 22.11 to 22.12
 */
static bool H_UpgradeFromV11()
{
   static const TCHAR *batch =
      _T("ALTER TABLE ap_common ADD flags integer\n")
      _T("UPDATE ap_common SET flags=0\n")
      _T("ALTER TABLE ap_common ADD deploy_filter $SQL:TEXT\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_hCoreDB, _T("ap_common"), _T("flags")));

   CHK_EXEC(CreateEventTemplate(EVENT_POLICY_AUTODEPLOY, _T("SYS_POLICY_AUTODEPLOY"), SEVERITY_NORMAL, EF_LOG, _T("f26d70b3-d48d-472c-b2ec-bfa7c20958ea"),
            _T("Agent policy %4 automatically deployed to node %2"),
            _T("Generated when agent policy deployed to node by autodeploy rule.\r\n")
            _T("Parameters:\r\n")
            _T("   1) Node ID\r\n")
            _T("   2) Node name\r\n")
            _T("   3) Policy ID\r\n")
            _T("   4) Policy name")
            ));

   CHK_EXEC(CreateEventTemplate(EVENT_POLICY_AUTOUNINSTALL, _T("SYS_POLICY_AUTOUNINSTALL"), SEVERITY_NORMAL, EF_LOG, _T("2fbac886-2cfa-489f-bef1-364a38fa7062"),
            _T("Agent policy %4 automatically uninstalled from node %2"),
            _T("Generated when agent policy uninstalled from node by autodeploy rule.\r\n")
            _T("Parameters:\r\n")
            _T("   1) Node ID\r\n")
            _T("   2) Node name\r\n")
            _T("   3) Policy ID\r\n")
            _T("   4) Policy name")
            ));

   CHK_EXEC(SetMinorSchemaVersion(12));
   return true;
}

/**
 * Upgrade from 22.10 to 22.11
 */
static bool H_UpgradeFromV10()
{
   CHK_EXEC(SQLQuery(_T("UPDATE config SET var_name='Housekeeper.StartTime' WHERE var_name='HousekeeperStartTime'")));
   CHK_EXEC(CreateConfigParam(_T("Housekeeper.Throttle.HighWatermark"), _T("250000"), _T("High watermark for housekeeper throttling"), 'I', true, false, false, false));
   CHK_EXEC(CreateConfigParam(_T("Housekeeper.Throttle.LowWatermark"), _T("50000"), _T("Low watermark for housekeeper throttling"), 'I', true, false, false, false));
   CHK_EXEC(SetMinorSchemaVersion(11));
   return true;
}

/**
 * Upgrade from 22.9 to 22.10
 */
static bool H_UpgradeFromV9()
{
   static TCHAR batch[] =
      _T("UPDATE config SET var_name='ThreadPool.DataCollector.BaseSize' WHERE var_name='DataCollector.ThreadPool.BaseSize'\n")
      _T("UPDATE config SET var_name='ThreadPool.DataCollector.MaxSize' WHERE var_name='DataCollector.ThreadPool.MaxSize'\n")
      _T("UPDATE config SET var_name='ThreadPool.Poller.BaseSize',description='Base size for poller thread pool' WHERE var_name='PollerThreadPoolBaseSize'\n")
      _T("UPDATE config SET var_name='ThreadPool.Poller.MaxSize',description='Maximum size for poller thread pool' WHERE var_name='PollerThreadPoolMaxSize'\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(CreateConfigParam(_T("ThreadPool.Agent.BaseSize"), _T("4"), _T("Base size for agent connector thread pool"), 'I', true, true, false, false));
   CHK_EXEC(CreateConfigParam(_T("ThreadPool.Agent.MaxSize"), _T("256"), _T("Maximum size for agent connector thread pool"), 'I', true, true, false, false));
   CHK_EXEC(CreateConfigParam(_T("ThreadPool.Main.BaseSize"), _T("8"), _T("Base size for main server thread pool"), 'I', true, true, false, false));
   CHK_EXEC(CreateConfigParam(_T("ThreadPool.Main.MaxSize"), _T("256"), _T("Maximum size for main server thread pool"), 'I', true, true, false, false));
   CHK_EXEC(CreateConfigParam(_T("ThreadPool.Scheduler.BaseSize"), _T("1"), _T("Base size for scheduler thread pool"), 'I', true, true, false, false));
   CHK_EXEC(CreateConfigParam(_T("ThreadPool.Scheduler.MaxSize"), _T("64"), _T("Maximum size for scheduler thread pool"), 'I', true, true, false, false));
   CHK_EXEC(SetMinorSchemaVersion(10));
   return true;
}

/**
 * Upgrade from 22.8 to 22.9
 */
static bool H_UpgradeFromV8()
{
   CHK_EXEC(DBResizeColumn(g_hCoreDB, _T("nodes"), _T("lldp_id"), 255, true));
   CHK_EXEC(SetMinorSchemaVersion(9));
   return true;
}

/**
 * Upgrade from 22.7 to 22.8
 */
static bool H_UpgradeFromV7()
{
   static TCHAR batch[] =
      _T("ALTER TABLE nodes ADD rack_image_rear varchar(36)\n")
      _T("ALTER TABLE chassis ADD rack_image_rear varchar(36)\n")
      _T("UPDATE nodes SET rack_image_rear='00000000-0000-0000-0000-000000000000'\n")
      _T("UPDATE chassis SET rack_image_rear='00000000-0000-0000-0000-000000000000'\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBRenameColumn(g_hCoreDB, _T("nodes"), _T("rack_image"), _T("rack_image_front")));
   CHK_EXEC(DBRenameColumn(g_hCoreDB, _T("chassis"), _T("rack_image"), _T("rack_image_front")));
   CHK_EXEC(SetMinorSchemaVersion(8));
   return true;
}

/**
 * Upgrade from 22.6 to 22.7
 */
static bool H_UpgradeFromV6()
{
   static TCHAR batch[] =
      _T("UPDATE config SET default_value='2' WHERE var_name='DefaultEncryptionPolicy'\n")
      _T("UPDATE config SET var_value='2' WHERE var_name='DefaultEncryptionPolicy' AND var_value!='3'\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(SetMinorSchemaVersion(7));
   return true;
}

/**
 * Upgrade from 22.5 to 22.6
 */
static bool H_UpgradeFromV5()
{
   static TCHAR batch[] =
      _T("ALTER TABLE racks ADD passive_elements $SQL:TEXT\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(SetMinorSchemaVersion(6));
   return true;
}

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
   { 33, 22, 34, H_UpgradeFromV33 },
   { 32, 22, 33, H_UpgradeFromV32 },
   { 31, 22, 32, H_UpgradeFromV31 },
   { 30, 22, 31, H_UpgradeFromV30 },
   { 29, 22, 30, H_UpgradeFromV29 },
   { 28, 22, 29, H_UpgradeFromV28 },
   { 27, 22, 28, H_UpgradeFromV27 },
   { 26, 22, 27, H_UpgradeFromV26 },
   { 25, 22, 26, H_UpgradeFromV25 },
   { 24, 22, 25, H_UpgradeFromV24 },
   { 23, 22, 24, H_UpgradeFromV23 },
   { 22, 22, 24, H_UpgradeFromV22 },
   { 21, 22, 22, H_UpgradeFromV21 },
   { 20, 22, 21, H_UpgradeFromV20 },
   { 19, 22, 20, H_UpgradeFromV19 },
   { 18, 22, 19, H_UpgradeFromV18 },
   { 17, 22, 18, H_UpgradeFromV17 },
   { 16, 22, 17, H_UpgradeFromV16 },
   { 15, 22, 16, H_UpgradeFromV15 },
   { 14, 22, 15, H_UpgradeFromV14 },
   { 13, 22, 14, H_UpgradeFromV13 },
   { 12, 22, 13, H_UpgradeFromV12 },
   { 11, 22, 12, H_UpgradeFromV11 },
   { 10, 22, 11, H_UpgradeFromV10 },
   { 9,  22, 10, H_UpgradeFromV9 },
   { 8,  22, 9,  H_UpgradeFromV8 },
   { 7,  22, 8,  H_UpgradeFromV7 },
   { 6,  22, 7,  H_UpgradeFromV6 },
   { 5,  22, 6,  H_UpgradeFromV5 },
   { 4,  22, 5,  H_UpgradeFromV4 },
   { 3,  22, 4,  H_UpgradeFromV3 },
   { 2,  22, 3,  H_UpgradeFromV2 },
   { 1,  22, 2,  H_UpgradeFromV1 },
   { 0,  22, 1,  H_UpgradeFromV0 },
   { 0,  0,  0,  NULL }
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
