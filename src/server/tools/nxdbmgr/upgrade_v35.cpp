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
** File: upgrade_v35.cpp
**
**/

#include "nxdbmgr.h"
#include <nxevent.h>

/**
 * Upgrade from 35.17 to 36.0
 */
static bool H_UpgradeFromV17()
{
   CHK_EXEC(SetMajorSchemaVersion(36, 0));
   return true;
}

/**
 * Upgrade form 35.16 to 35.17
 */
static bool H_UpgradeFromV16()
{
   CHK_EXEC(SQLQuery(_T("UPDATE config SET var_name='SNMP.Traps.LogAll',need_server_restart=0,description='Log all SNMP traps (even those received from addresses not belonging to any known node).' WHERE var_name='LogAllSNMPTraps'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET var_name='SNMP.Traps.AllowVarbindsConversion',need_server_restart=0 WHERE var_name='AllowTrapVarbindsConversion'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET need_server_restart=0 WHERE var_name='SNMP.Traps.ProcessUnmanagedNodes'")));
   CHK_EXEC(SetMinorSchemaVersion(17));
   return true;
}

/**
 * Upgrade form 35.15 to 35.16
 */
static bool H_UpgradeFromV15()
{
   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET event_name='SNMP_TRAP_FLOOD_ENDED' WHERE event_code=110")));
   CHK_EXEC(SetMinorSchemaVersion(16));
   return true;
}

/**
 * Upgrade form 35.14 to 35.15
 */
static bool H_UpgradeFromV14()
{
   CHK_EXEC(SQLQuery(_T("ALTER TABLE object_properties ADD alias varchar(255)")));
   CHK_EXEC(SQLQuery(_T("UPDATE object_properties SET alias=(SELECT alias FROM interfaces WHERE interfaces.id=object_properties.object_id)")));
   CHK_EXEC(DBDropColumn(g_dbHandle, _T("interfaces"), _T("alias")));
   CHK_EXEC(SetMinorSchemaVersion(15));
   return true;
}

/**
 * Upgrade form 35.13 to 35.14
 */
static bool H_UpgradeFromV13()
{
   CHK_EXEC(SQLQuery(_T("ALTER TABLE policy_action_list ADD blocking_timer_key varchar(127)")));
   CHK_EXEC(SetMinorSchemaVersion(14));
   return true;
}

/**
 * Upgrade form 35.12 to 35.13
 */
static bool H_UpgradeFromV12()
{
   CHK_EXEC(SQLQuery(_T("DELETE FROM config WHERE var_name='AllowDirectNotifications'")));
   CHK_EXEC(SetMinorSchemaVersion(13));
   return true;
}

/**
 * Upgrade form 35.11 to 35.12
 */
static bool H_UpgradeFromV11()
{
   CHK_EXEC(SQLQuery(_T("ALTER TABLE nodes ADD cip_vendor_code integer")));
   CHK_EXEC(SQLQuery(_T("UPDATE nodes SET cip_vendor_code=0")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("nodes"), _T("cip_vendor_code")));
   CHK_EXEC(SetMinorSchemaVersion(12));
   return true;
}

/**
 * Upgrade from 35.10 to 35.11
 */
static bool H_UpgradeFromV10()
{
   CHK_EXEC(CreateConfigParam(_T("Events.Processor.PoolSize"), _T("1"), _T("Number of threads for parallel event processing."), _T("threads"), 'I', true, true, false, false));
   CHK_EXEC(CreateConfigParam(_T("Events.Processor.QueueSelector"), _T("%z"), _T("Queue selector for parallel event processing."), nullptr, 'S', true, true, false, false));
   CHK_EXEC(SetMinorSchemaVersion(11));
   return true;
}

/**
 * Update group ID in given table
 */
bool UpdateGroupId(const TCHAR *table, const TCHAR *column)
{
   TCHAR query[256];
   _sntprintf(query, 256, _T("SELECT DISTINCT %s FROM %s"), column, table);
   DB_RESULT hResult = SQLSelect(query);
   if (hResult == nullptr)
      return false;

   bool success = true;
   int count = DBGetNumRows(hResult);
   for(int i = 0; (i < count) && success; i++)
   {
      uint32_t id = DBGetFieldULong(hResult, i, 0);
      if ((id & 0x80000000) == 0)
         continue;

      _sntprintf(query, 256, _T("UPDATE %s SET %s=%u WHERE %s=%d"), table, column, (id & 0x7FFFFFFF) | 0x40000000, column, id);
      success = SQLQuery(query);
   }
   DBFreeResult(hResult);
   return success;
}

/**
 * Upgrade from 35.9 to 35.10
 */
static bool H_UpgradeFromV9()
{
   CHK_EXEC(UpdateGroupId(_T("user_groups"), _T("id")));
   CHK_EXEC(UpdateGroupId(_T("user_group_members"), _T("group_id")));
   CHK_EXEC(UpdateGroupId(_T("userdb_custom_attributes"), _T("object_id")));
   CHK_EXEC(UpdateGroupId(_T("acl"), _T("user_id")));
   CHK_EXEC(UpdateGroupId(_T("dci_access"), _T("user_id")));
   CHK_EXEC(UpdateGroupId(_T("alarm_category_acl"), _T("user_id")));
   CHK_EXEC(UpdateGroupId(_T("object_tools_acl"), _T("user_id")));
   CHK_EXEC(UpdateGroupId(_T("graph_acl"), _T("user_id")));
   CHK_EXEC(UpdateGroupId(_T("object_access_snapshot"), _T("user_id")));
   CHK_EXEC(UpdateGroupId(_T("responsible_users"), _T("user_id")));
   CHK_EXEC(SetSchemaLevelForMajorVersion(34, 17));
   CHK_EXEC(SetMinorSchemaVersion(10));
   return true;
}

/**
 * Upgrade from 35.8 to 35.9
 */
static bool H_UpgradeFromV8()
{
   CHK_EXEC(CreateConfigParam(_T("SNMP.Traps.RateLimit.Threshold"), _T("0"), _T("Threshold for number of SNMP traps per second that defines SNMP trap flood condition. Detection is disabled if 0 is set."), _T("seconds"), 'I', true, false, false, false));
   CHK_EXEC(CreateConfigParam(_T("SNMP.Traps.RateLimit.Duration"), _T("15"), _T("Time period for SNMP traps per second to be above threshold that defines SNMP trap flood condition."), _T("seconds"), 'I', true, false, false, true));

   CHK_EXEC(CreateEventTemplate(EVENT_SNMP_TRAP_FLOOD_DETECTED, _T("SNMP_TRAP_FLOOD_DETECTED"),
            SEVERITY_MAJOR, EF_LOG, _T("6b2bb689-23b7-4e7c-9128-5102f658e450"),
            _T("SNMP trap flood detected (Traps per second: %1)"),
            _T("Generated when system detects an SNMP trap flood.\r\n")
            _T("Parameters:\r\n")
            _T("   1) SNMP traps per second\r\n")
            _T("   2) Duration\r\n")
            _T("   3) Threshold")
            ));
   CHK_EXEC(CreateEventTemplate(EVENT_SNMP_TRAP_FLOOD_ENDED, _T("SNMP_TRAP_FLOOD_ENDED"),
            SEVERITY_NORMAL, EF_LOG, _T("f2c41199-9338-4c9a-9528-d65835c6c271"),
            _T("SNMP trap flood ended"),
            _T("Generated after SNMP trap flood state is cleared.\r\n")
            _T("Parameters:\r\n")
            _T("   1) SNMP traps per second\r\n")
            _T("   2) Duration\r\n")
            _T("   3) Threshold")
            ));

   CHK_EXEC(SetMinorSchemaVersion(9));
   return true;
}

/**
 * Upgrade from 35.7 to 35.8
 */
static bool H_UpgradeFromV7()
{
   static const TCHAR *batch =
         _T("UPDATE config SET description='A bitmask for encryption algorithms allowed in the server(sum the values to allow multiple algorithms at once): 1 = AES256, 2 = Blowfish-256, 4 = IDEA, 8 = 3DES, 16 = AES128, 32 = Blowfish-128.' WHERE var_name='AllowedCiphers'\n")
         _T("UPDATE config SET description='Comma-separated list of hosts to be used as beacons for checking NetXMS server network connectivity. Either DNS names or IP addresses can be used. This list is pinged by NetXMS server and if none of the hosts have responded, server considers that connection with network is lost and generates specific event.' WHERE var_name='BeaconHosts'\n")
         _T("UPDATE config SET description='The LdapConnectionString configuration parameter may be a comma- or whitespace-separated list of URIs containing only the schema, the host, and the port fields. Format: schema://host:port.' WHERE var_name='LDAP.ConnectionString'\n")
         _T("UPDATE config SET units='minutes',description='The synchronization interval (in minutes) between the NetXMS server and the LDAP server. If the parameter is set to 0, no synchronization will take place.' WHERE var_name='LDAP.SyncInterval'\n")
         _T("UPDATE config SET var_name='Client.MinViewRefreshInterval',default_value='300',units='milliseconds',description='Minimal interval between view refresh in milliseconds (hint for client).' WHERE var_name='MinViewRefreshInterval'\n")
         _T("UPDATE config SET var_name='Beacon.Hosts' WHERE var_name='BeaconHosts'\n")
         _T("UPDATE config SET var_name='Beacon.PollingInterval' WHERE var_name='BeaconPollingInterval'\n")
         _T("UPDATE config SET var_name='Beacon.Timeout' WHERE var_name='BeaconTimeout'\n")
         _T("UPDATE config SET var_name='SNMP.Traps.Enable' WHERE var_name='EnableSNMPTraps'\n")
         _T("UPDATE config SET var_name='SNMP.Traps.ListenerPort' WHERE var_name='SNMPTrapPort'\n")
         _T("UPDATE config_values SET var_name='SNMP.Traps.ListenerPort' WHERE var_name='SNMPTrapPort'\n")
         _T("UPDATE config SET var_name='SNMP.Traps.ProcessUnmanagedNodes',default_value='0',data_type='B',description='Enable/disable processing of SNMP traps received from unmanaged nodes.' WHERE var_name='ProcessTrapsFromUnmanagedNodes'\n")
         _T("DELETE FROM config WHERE var_name='SNMPPorts'\n")
         _T("DELETE FROM config_values WHERE var_name='SNMPPorts'\n")
         _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   CHK_EXEC(CreateConfigParam(_T("SNMP.Traps.ProcessUnmanagedNodes"), _T("0"), _T("Enable/disable processing of SNMP traps received from unmanaged nodes."), nullptr, 'B', true, true, false, false));

   CHK_EXEC(SetMinorSchemaVersion(8));
   return true;
}

/**
 * Upgrade from 35.6 to 35.7
 */
static bool H_UpgradeFromV6()
{
   CHK_EXEC(SQLQuery(
         _T("INSERT INTO script_library (guid,script_id,script_name,script_code) ")
         _T("VALUES ('9c2dba59-493b-4645-9159-2ad7a28ea611',23,'Hook::OpenUnboundTunnel','")
         _T("/* Available global variables:\r\n")
         _T(" *  $tunnel - incoming tunnel information (object of ''Tunnel'' class)\r\n")
         _T(" *\r\n")
         _T(" * Expected return value:\r\n")
         _T(" *  none - returned value is ignored\r\n */\r\n')")));
   CHK_EXEC(SQLQuery(
         _T("INSERT INTO script_library (guid,script_id,script_name,script_code) ")
         _T("VALUES ('64c90b92-27e9-4a96-98ea-d0e152d71262',24,'Hook::OpenBoundTunnel','")
         _T("/* Available global variables:\r\n")
         _T(" *  $node - node this tunnel was bound to (object of ''Node'' class)\r\n")
         _T(" *  $tunnel - incoming tunnel information (object of ''Tunnel'' class)\r\n")
         _T(" *\r\n")
         _T(" * Expected return value:\r\n")
         _T(" *  none - returned value is ignored\r\n */\r\n')")));

   CHK_EXEC(SetMinorSchemaVersion(7));
   return true;
}

/**
 * Upgrade from 35.5 to 35.6
 */
static bool H_UpgradeFromV5()
{
   CHK_EXEC(CreateConfigParam(_T("RoamingServer"), _T("0"), _T("Enable/disable roaming mode for server (when server can be disconnected from one network and connected to another or IP address of the server can change)."), nullptr, 'B', true, false, false, false));
   CHK_EXEC(SetMinorSchemaVersion(6));
   return true;
}

/**
 * Upgrade from 35.4 to 35.5
 */
static bool H_UpgradeFromV4()
{
   if (GetSchemaLevelForMajorVersion(34) < 16)
   {
      CHK_EXEC(CreateConfigParam(_T("FirstFreeDCIId"), _T("1"), false, true, false));

      CHK_EXEC(CreateTable(
            _T("CREATE TABLE dci_delete_list (")
            _T("node_id integer not null,")
            _T("dci_id integer not null,")
            _T("type char(1) not null,")
            _T("PRIMARY KEY (node_id,dci_id))")));

      CHK_EXEC(SetSchemaLevelForMajorVersion(34, 16));
   }

   CHK_EXEC(SetMinorSchemaVersion(5));
   return true;
}

/**
 * Upgrade from 35.3 to 35.4
 */
static bool H_UpgradeFromV3()
{
   if (GetSchemaLevelForMajorVersion(34) < 15)
   {
      CHK_EXEC(CreateConfigParam(_T("DBWriter.HouseKeeperInterlock"), _T("0"), _T("Controls if server should block background write of collected performance data while housekeeper deletes expired records."), nullptr, 'C', true, false, false, false));

      static const TCHAR *batch =
            _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('DBWriter.HouseKeeperInterlock','0','Auto')\n")
            _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('DBWriter.HouseKeeperInterlock','1','Off')\n")
            _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('DBWriter.HouseKeeperInterlock','2','On')\n")
            _T("UPDATE config_values SET var_name='LDAP.UserDeleteAction' WHERE var_name='LdapUserDeleteAction'\n")
            _T("<END>");
      CHK_EXEC(SQLBatch(batch));

      CHK_EXEC(SetSchemaLevelForMajorVersion(34, 15));
   }
   CHK_EXEC(SetMinorSchemaVersion(4));
   return true;
}

/**
 * Upgrade from 35.2 to 35.3
 */
static bool H_UpgradeFromV2()
{
   if (GetSchemaLevelForMajorVersion(34) < 13)
   {
      CHK_EXEC(SQLQuery(_T("INSERT INTO metadata (var_name,var_value) VALUES ('PruneCustomAttributes', '1')")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(34, 13));
   }
   CHK_EXEC(SetMinorSchemaVersion(3));
   return true;
}

/**
 * Upgrade from 35.1 to 35.2
 */
static bool H_UpgradeFromV1()
{
   if (g_dbSyntax == DB_SYNTAX_TSDB)
   {
      CHK_EXEC(SQLQuery(_T("ALTER TABLE alarms RENAME TO old_alarms")));
      CHK_EXEC(CreateTable(
            _T("CREATE TABLE alarms (")
            _T("   alarm_id integer not null,")
            _T("   parent_alarm_id integer not null,")
            _T("   alarm_state integer not null,")
            _T("   hd_state integer not null,")
            _T("   hd_ref varchar(63) null,")
            _T("   creation_time integer not null,")
            _T("   last_change_time integer not null,")
            _T("   rule_guid varchar(36) null,")
            _T("   source_object_id integer not null,")
            _T("   zone_uin integer not null,")
            _T("   source_event_code integer not null,")
            _T("   source_event_id $SQL:INT64 not null,")
            _T("   dci_id integer not null,")
            _T("   message varchar(2000) null,")
            _T("   original_severity integer not null,")
            _T("   current_severity integer not null,")
            _T("   repeat_count integer not null,")
            _T("   alarm_key varchar(255) null,")
            _T("   ack_by integer not null,")
            _T("   resolved_by integer not null,")
            _T("   term_by integer not null,")
            _T("   timeout integer not null,")
            _T("   timeout_event integer not null,")
            _T("   ack_timeout integer not null,")
            _T("   alarm_category_ids varchar(255) null,")
            _T("   event_tags varchar(2000) null,")
            _T("   rca_script_name varchar(255) null,")
            _T("   impact varchar(1000) null,")
            _T("   PRIMARY KEY(alarm_id))")));
      CHK_EXEC(SQLQuery(_T("CREATE INDEX idx_alarms_source_object_id ON alarms(source_object_id)")));
      CHK_EXEC(SQLQuery(_T("CREATE INDEX idx_alarms_last_change_time ON alarms(last_change_time)")));
      CHK_EXEC(SQLQuery(_T("INSERT INTO alarms (alarm_id,parent_alarm_id,alarm_state,hd_state,hd_ref,creation_time,last_change_time,rule_guid,source_object_id,zone_uin,source_event_code,source_event_id,dci_id,message,original_severity,current_severity,repeat_count,alarm_key,ack_by,resolved_by,term_by,timeout,timeout_event,ack_timeout,alarm_category_ids,event_tags,rca_script_name,impact) SELECT alarm_id,parent_alarm_id,alarm_state,hd_state,hd_ref,creation_time,last_change_time,rule_guid,source_object_id,zone_uin,source_event_code,source_event_id,dci_id,message,original_severity,current_severity,repeat_count,alarm_key,ack_by,resolved_by,term_by,timeout,timeout_event,ack_timeout,alarm_category_ids,event_tags,rca_script_name,impact FROM old_alarms")));
      CHK_EXEC(SQLQuery(_T("DROP TABLE old_alarms CASCADE")));

      CHK_EXEC(SQLQuery(_T("ALTER TABLE event_log RENAME TO event_log_v35_2")));
      CHK_EXEC(SQLQuery(_T("DROP INDEX IF EXISTS idx_event_log_event_timestamp")));
      CHK_EXEC(SQLQuery(_T("DROP INDEX IF EXISTS idx_event_log_source")));
      CHK_EXEC(SQLQuery(_T("DROP INDEX IF EXISTS idx_event_log_root_id")));
      CHK_EXEC(CreateTable(
            _T("CREATE TABLE event_log (")
            _T("   event_id $SQL:INT64 not null,")
            _T("   event_code integer not null,")
            _T("   event_timestamp timestamptz not null,")
            _T("   origin integer not null,")
            _T("   origin_timestamp integer not null,")
            _T("   event_source integer not null,")
            _T("   zone_uin integer not null,")
            _T("   dci_id integer not null,")
            _T("   event_severity integer not null,")
            _T("   event_message varchar(2000) null,")
            _T("   event_tags varchar(2000) null,")
            _T("   root_event_id $SQL:INT64 not null,")
            _T("   raw_data $SQL:TEXT null,")
            _T("   PRIMARY KEY(event_id,event_timestamp))")));
      CHK_EXEC(SQLQuery(_T("CREATE INDEX idx_event_log_event_timestamp ON event_log(event_timestamp)")));
      CHK_EXEC(SQLQuery(_T("CREATE INDEX idx_event_log_source ON event_log(event_source)")));
      CHK_EXEC(SQLQuery(_T("CREATE INDEX idx_event_log_root_id ON event_log(root_event_id) WHERE root_event_id > 0")));
      CHK_EXEC(SQLQuery(_T("SELECT create_hypertable('event_log', 'event_timestamp', chunk_time_interval => interval '86400 seconds')")));

      CHK_EXEC(SQLQuery(_T("ALTER TABLE syslog RENAME TO syslog_v35_2")));
      CHK_EXEC(SQLQuery(_T("DROP INDEX IF EXISTS idx_syslog_msg_timestamp")));
      CHK_EXEC(SQLQuery(_T("DROP INDEX IF EXISTS idx_syslog_source")));
      CHK_EXEC(CreateTable(
            _T("CREATE TABLE syslog (")
            _T("   msg_id $SQL:INT64 not null,")
            _T("   msg_timestamp timestamptz not null,")
            _T("   facility integer not null,")
            _T("   severity integer not null,")
            _T("   source_object_id integer not null,")
            _T("   zone_uin integer not null,")
            _T("   hostname varchar(127) null,")
            _T("   msg_tag varchar(32) null,")
            _T("   msg_text $SQL:TEXT null,")
            _T("   PRIMARY KEY(msg_id,msg_timestamp))")));
      CHK_EXEC(SQLQuery(_T("CREATE INDEX idx_syslog_msg_timestamp ON syslog(msg_timestamp)")));
      CHK_EXEC(SQLQuery(_T("CREATE INDEX idx_syslog_source ON syslog(source_object_id)")));
      CHK_EXEC(SQLQuery(_T("SELECT create_hypertable('syslog', 'msg_timestamp', chunk_time_interval => interval '86400 seconds')")));

      CHK_EXEC(SQLQuery(_T("ALTER TABLE snmp_trap_log RENAME TO snmp_trap_log_v35_2")));
      CHK_EXEC(SQLQuery(_T("DROP INDEX IF EXISTS idx_snmp_trap_log_tt")));
      CHK_EXEC(SQLQuery(_T("DROP INDEX IF EXISTS idx_snmp_trap_log_oid")));
      CHK_EXEC(CreateTable(
            _T("CREATE TABLE snmp_trap_log (")
            _T("   trap_id $SQL:INT64 not null,")
            _T("   trap_timestamp timestamptz not null,")
            _T("   ip_addr varchar(48) not null,")
            _T("   object_id integer not null,")
            _T("   zone_uin integer not null,")
            _T("   trap_oid varchar(255) not null,")
            _T("   trap_varlist $SQL:TEXT null,")
            _T("   PRIMARY KEY(trap_id,trap_timestamp))")));
      CHK_EXEC(SQLQuery(_T("CREATE INDEX idx_snmp_trap_log_tt ON snmp_trap_log(trap_timestamp)")));
      CHK_EXEC(SQLQuery(_T("CREATE INDEX idx_snmp_trap_log_oid ON snmp_trap_log(object_id)")));
      CHK_EXEC(SQLQuery(_T("SELECT create_hypertable('snmp_trap_log', 'trap_timestamp', chunk_time_interval => interval '86400 seconds')")));

      RegisterOnlineUpgrade(35, 2);
   }
   else
   {
      CHK_EXEC(SQLQuery(_T("CREATE INDEX idx_alarms_source_object_id ON alarms(source_object_id)")));
      CHK_EXEC(SQLQuery(_T("CREATE INDEX idx_alarms_last_change_time ON alarms(last_change_time)")));
   }
   CHK_EXEC(SetMinorSchemaVersion(2));
   return true;
}

/**
 * Upgrade from 35.0 to 35.1
 */
static bool H_UpgradeFromV0()
{
   CHK_EXEC(SQLQuery(_T("UPDATE config SET need_server_restart=0 WHERE var_name='CheckTrustedNodes'")));
   CHK_EXEC(SQLQuery(_T("UPDATE config SET need_server_restart=0 WHERE var_name='NXSL.EnableContainerFunctions'")));
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
   { 17, 36, 0,  H_UpgradeFromV17 },
   { 16, 35, 17, H_UpgradeFromV16 },
   { 15, 35, 16, H_UpgradeFromV15 },
   { 14, 35, 15, H_UpgradeFromV14 },
   { 13, 35, 14, H_UpgradeFromV13 },
   { 12, 35, 13, H_UpgradeFromV12 },
   { 11, 35, 12, H_UpgradeFromV11 },
   { 10, 35, 11, H_UpgradeFromV10 },
   { 9,  35, 10, H_UpgradeFromV9  },
   { 8,  35, 9,  H_UpgradeFromV8  },
   { 7,  35, 8,  H_UpgradeFromV7  },
   { 6,  35, 7,  H_UpgradeFromV6  },
   { 5,  35, 6,  H_UpgradeFromV5  },
   { 4,  35, 5,  H_UpgradeFromV4  },
   { 3,  35, 4,  H_UpgradeFromV3  },
   { 2,  35, 3,  H_UpgradeFromV2  },
   { 1,  35, 2,  H_UpgradeFromV1  },
   { 0,  35, 1,  H_UpgradeFromV0  },
   { 0,  0,  0,  nullptr          }
};

/**
 * Upgrade database to new version
 */
bool MajorSchemaUpgrade_V35()
{
   INT32 major, minor;
   if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
      return false;

   while(major == 35)
   {
      // Find upgrade procedure
      int i;
      for(i = 0; s_dbUpgradeMap[i].upgradeProc != nullptr; i++)
         if (s_dbUpgradeMap[i].version == minor)
            break;
      if (s_dbUpgradeMap[i].upgradeProc == nullptr)
      {
         _tprintf(_T("Unable to find upgrade procedure for version 35.%d\n"), minor);
         return false;
      }
      _tprintf(_T("Upgrading from version 35.%d to %d.%d\n"), minor, s_dbUpgradeMap[i].nextMajor, s_dbUpgradeMap[i].nextMinor);
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
