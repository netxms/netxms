/*
** nxdbmgr - NetXMS database manager
** Copyright (C) 2022-2026 Raden Solutions
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
** File: upgrade_v61.cpp
**
**/

#include "nxdbmgr.h"
#include <nxevent.h>

/**
 * Upgrade from 61.36 to 61.37
 */
static bool H_UpgradeFromV36()
{
   if (GetSchemaLevelForMajorVersion(60) < 38)
   {
      // Earlier 6.0 upgrade procedures wrote Oracle idata table creation command with a typo in
      // primary key definition ("tdata_timestamp" instead of "idata_timestamp"), so idata tables
      // could not be created for data collection targets added after that upgrade (issue #3411).
      // DDL is issued directly instead of via CreateIDataTable() - on 6.2 and later that function
      // generates the statement in code (issue #3204), and historical upgrade paths must keep the
      // metadata-driven schema of their era.
      if (g_dbSyntax == DB_SYNTAX_ORACLE)
      {
         static const wchar_t *idataCreationCommand =
               L"CREATE TABLE idata_%d (item_id integer not null,idata_timestamp number(20) not null,idata_value varchar2(255 char) null,raw_value varchar2(255 char) null, PRIMARY KEY(item_id,idata_timestamp))";

         CHK_EXEC(DBMgrMetaDataWriteStr(L"IDataTableCreationCommand", idataCreationCommand));

         if (!DBMgrMetaDataReadInt32(L"SingeTablePerfData", 0))
         {
            IntegerArray<uint32_t> targets = GetDataCollectionTargets();
            for(int i = 0; i < targets.size(); i++)
            {
               uint32_t objectId = targets.get(i);
               wchar_t table[64];
               nx_swprintf(table, 64, L"idata_%u", objectId);

               // Only a definite "table found" answer may skip creation - a failed existence
               // check must not silently leave a missing table behind
               if (DBIsTableExist(g_dbHandle, table) == DBIsTableExist_Found)
                  continue;

               wchar_t query[256];
               nx_swprintf(query, 256, idataCreationCommand, objectId);
               WriteToTerminalEx(L"Creating missing table \x1b[1m%s\x1b[0m...\n", table);
               CHK_EXEC(SQLQuery(query));
            }
         }
      }

      CHK_EXEC(SetSchemaLevelForMajorVersion(60, 38));
   }
   CHK_EXEC(SetMinorSchemaVersion(37));
   return true;
}

/**
 * Upgrade from 61.35 to 61.36
 */
static bool H_UpgradeFromV35()
{
   // Per-DCI idata tables created after the 6.0 upgrade have only
   // PRIMARY KEY(item_id, idata_timestamp), which cannot serve queries
   // filtering/sorting by idata_timestamp alone. Restore the secondary
   // (idata_timestamp, item_id) index for MySQL, PostgreSQL (non-TSDB),
   // and MSSQL, and backfill it on existing idata_* tables.
   if ((g_dbSyntax == DB_SYNTAX_MYSQL) || (g_dbSyntax == DB_SYNTAX_PGSQL) || (g_dbSyntax == DB_SYNTAX_MSSQL))
   {
      CHK_EXEC(DBMgrMetaDataWriteStr(L"IDataIndexCreationCommand_0", L"CREATE INDEX idx_idata_%u_timestamp_id ON idata_%u(idata_timestamp,item_id)"));

      IntegerArray<uint32_t> targets = GetDataCollectionTargets();
      for(int i = 0; i < targets.size(); i++)
      {
         uint32_t id = targets.get(i);
         wchar_t tableName[64], indexName[64];
         nx_swprintf(tableName, 64, L"idata_%u", id);
         nx_swprintf(indexName, 64, L"idx_idata_%u_timestamp_id", id);

         if (DBIsTableExist(g_dbHandle, tableName) != DBIsTableExist_Found)
            continue;

         // Skip if the index already exists (e.g. on pre-6.0 PostgreSQL tables, which
         // already have this exact index). Attempting CREATE INDEX here would fail and
         // on PostgreSQL would poison the entire upgrade transaction.
         if (IsIndexExists(tableName, indexName))
            continue;

         wchar_t query[256];
         nx_swprintf(query, 256, L"CREATE INDEX %s ON %s(idata_timestamp,item_id)", indexName, tableName);
         WriteToTerminalEx(L"Indexing table \x1b[1m%s\x1b[0m...\n", tableName);
         CHK_EXEC(SQLQuery(query));
      }
   }
   CHK_EXEC(SetMinorSchemaVersion(36));
   return true;
}

/**
 * Upgrade from 61.34 to 61.35
 */
static bool H_UpgradeFromV34()
{
   CHK_EXEC(CreateTable(
      L"CREATE TABLE lp_absence_state ("
      L"  parser_type char(1) not null,"
      L"  rule_guid varchar(36) not null,"
      L"  object_id integer not null,"
      L"  last_match_time integer not null,"
      L"  last_alert_time integer not null,"
      L"  PRIMARY KEY(parser_type,rule_guid,object_id))"));
   CHK_EXEC(SetMinorSchemaVersion(35));
   return true;
}

/**
 * Upgrade from 61.33 to 61.34
 */
static bool H_UpgradeFromV33()
{
   CHK_EXEC(CreateTable(
      L"CREATE TABLE ai_saved_prompts ("
      L"  id integer not null,"
      L"  user_id integer not null,"
      L"  name varchar(255) not null,"
      L"  description varchar(2000) null,"
      L"  prompt_text $SQL:TEXT not null,"
      L"  PRIMARY KEY(id))"));
   CHK_EXEC(SQLQuery(L"CREATE INDEX idx_ai_saved_prompts_user ON ai_saved_prompts(user_id)"));
   CHK_EXEC(SetMinorSchemaVersion(34));
   return true;
}

/**
 * Upgrade from 61.32 to 61.33
 */
static bool H_UpgradeFromV32()
{
   CHK_EXEC(CreateEventTemplate(EVENT_ACTION_EXECUTION_FAILURE, _T("SYS_ACTION_EXECUTION_FAILURE"),
      EVENT_SEVERITY_WARNING, 1, _T("3337d969-4fe3-4b48-9f22-4bb83abed368"),
      _T("Execution of action \"%<actionName>\" (type: %<actionType>) failed"),
      _T("Generated when server action execution fails.\r\n")
      _T("Parameters:\r\n")
      _T("   1) actionName - Action name\r\n")
      _T("   2) actionType - Action type\r\n")
      _T("   3) actionData - Action data (command, script name, etc.)\r\n")
      _T("   4) ruleDescription - Event processing policy rule that triggered the action\r\n")
      _T("   5) ruleGuid - Event processing policy rule GUID\r\n")
      _T("   6) eventCode - Original event code that triggered the action\r\n")
      _T("   7) eventId - Original event ID that triggered the action")));
   CHK_EXEC(SetMinorSchemaVersion(33));
   return true;
}

/**
 * Upgrade from 61.31 to 61.32
 */
static bool H_UpgradeFromV31()
{
   CHK_EXEC(CreateTable(
      L"CREATE TABLE ai_disabled_items ("
      L"  item_type char(1) not null,"
      L"  item_name varchar(127) not null,"
      L"  PRIMARY KEY(item_type,item_name))"));

   // Add SYSTEM_ACCESS_MANAGE_AI_SKILLS to default "Admins" group
   DB_RESULT hResult = SQLSelect(L"SELECT system_access FROM user_groups WHERE id=1073741825");
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         uint64_t access = DBGetFieldUInt64(hResult, 0, 0);
         access |= SYSTEM_ACCESS_MANAGE_AI_SKILLS;
         wchar_t query[256];
         nx_swprintf(query, 256, L"UPDATE user_groups SET system_access=" INT64_FMT L" WHERE id=1073741825", access);
         CHK_EXEC(SQLQuery(query));
      }
      DBFreeResult(hResult);
   }
   else if (!g_ignoreErrors)
   {
      return false;
   }

   CHK_EXEC(SetMinorSchemaVersion(32));
   return true;
}

/**
 * Upgrade from 61.30 to 61.31
 */
static bool H_UpgradeFromV30()
{
   // Add SYSTEM_ACCESS_USE_AI_ASSISTANT to built-in "Everyone" group
   DB_RESULT hResult = SQLSelect(L"SELECT system_access FROM user_groups WHERE id=1073741824");
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         uint64_t access = DBGetFieldUInt64(hResult, 0, 0);
         access |= SYSTEM_ACCESS_USE_AI_ASSISTANT;
         wchar_t query[256];
         nx_swprintf(query, 256, L"UPDATE user_groups SET system_access=" INT64_FMT L" WHERE id=1073741824", access);
         CHK_EXEC(SQLQuery(query));
      }
      DBFreeResult(hResult);
   }
   else if (!g_ignoreErrors)
   {
      return false;
   }

   // Add SYSTEM_ACCESS_USE_AI_ASSISTANT to default "Admins" group
   hResult = SQLSelect(L"SELECT system_access FROM user_groups WHERE id=1073741825");
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         uint64_t access = DBGetFieldUInt64(hResult, 0, 0);
         access |= SYSTEM_ACCESS_USE_AI_ASSISTANT;
         wchar_t query[256];
         nx_swprintf(query, 256, L"UPDATE user_groups SET system_access=" INT64_FMT L" WHERE id=1073741825", access);
         CHK_EXEC(SQLQuery(query));
      }
      DBFreeResult(hResult);
   }
   else if (!g_ignoreErrors)
   {
      return false;
   }

   CHK_EXEC(SetMinorSchemaVersion(31));
   return true;
}

/**
 * Upgrade from 61.29 to 61.30
 */
static bool H_UpgradeFromV29()
{
   CHK_EXEC(SQLQuery(
      _T("INSERT INTO script_library (guid,script_id,script_name,script_code) ")
      _T("VALUES ('748237d9-01cd-49ee-b066-6ff115ae5a8b',29,'Hook::DiscoveryFilter','return true;\r\n')")));

   // Migrate existing filter script to hook
   TCHAR scriptName[MAX_CONFIG_VALUE_LENGTH];
   DBMgrConfigReadStr(_T("NetworkDiscovery.Filter.Script"), scriptName, MAX_CONFIG_VALUE_LENGTH, _T("none"));
   if (_tcsicmp(scriptName, _T("none")) && (scriptName[0] != 0))
   {
      DB_STATEMENT hStmt = DBPrepare(g_dbHandle,
         _T("SELECT script_code FROM script_library WHERE script_name = ?"));
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, scriptName, DB_BIND_STATIC);
         DB_RESULT hResult = DBSelectPrepared(hStmt);
         if (hResult != nullptr)
         {
            if (DBGetNumRows(hResult) > 0)
            {
               TCHAR *scriptCode = DBGetField(hResult, 0, 0, nullptr, 0);
               if (scriptCode != nullptr)
               {
                  DB_STATEMENT hUpdate = DBPrepare(g_dbHandle,
                     _T("UPDATE script_library SET script_code = ? WHERE script_name = 'Hook::DiscoveryFilter'"));
                  if (hUpdate != nullptr)
                  {
                     DBBind(hUpdate, 1, DB_SQLTYPE_TEXT, scriptCode, DB_BIND_DYNAMIC);
                     CHK_EXEC(SQLExecute(hUpdate));
                     DBFreeStatement(hUpdate);
                  }
                  else
                  {
                     MemFree(scriptCode);
                  }
               }
            }
            DBFreeResult(hResult);
         }
         DBFreeStatement(hStmt);
      }
   }

   // Clear DFF_EXECUTE_SCRIPT bit (0x0004) from filter flags
   uint32_t flags = DBMgrConfigReadUInt32(_T("NetworkDiscovery.Filter.Flags"), 0);
   flags &= ~0x0004u;
   TCHAR query[256];
   _sntprintf(query, 256, _T("UPDATE config SET var_value='%u' WHERE var_name='NetworkDiscovery.Filter.Flags'"), flags);
   CHK_EXEC(SQLQuery(query));

   // Remove obsolete config variable
   CHK_EXEC(SQLQuery(_T("DELETE FROM config WHERE var_name='NetworkDiscovery.Filter.Script'")));

   CHK_EXEC(SetMinorSchemaVersion(30));
   return true;
}

/**
 * Upgrade from 61.28 to 61.29
 */
static bool H_UpgradeFromV28()
{
   static const wchar_t *batch =
      L"ALTER TABLE server_action_execution_log ADD rule_guid varchar(36)\n"
      L"ALTER TABLE server_action_execution_log ADD rule_description varchar(255)\n"
      L"ALTER TABLE notification_log ADD rule_description varchar(255)\n"
      L"<END>";
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(SetMinorSchemaVersion(29));
   return true;
}

/**
 * Upgrade from 61.27 to 61.28
 */
static bool H_UpgradeFromV27()
{
   DB_STATEMENT hStmt = DBPrepare(g_dbHandle, _T("UPDATE event_cfg SET description=? WHERE event_code=?"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR,
         _T("Generated when new software package is found.\r\n")
         _T("Parameters:\r\n")
         _T("   1) name - Package name\r\n")
         _T("   2) version - Package version\r\n")
         _T("   3) user - User name (empty for system-wide packages)"), DB_BIND_STATIC);
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, EVENT_PACKAGE_INSTALLED);
      CHK_EXEC(SQLExecute(hStmt));

      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR,
         _T("Generated when software package version change is detected.\r\n")
         _T("Parameters:\r\n")
         _T("   1) name - Package name\r\n")
         _T("   2) version - New package version\r\n")
         _T("   3) previousVersion - Old package version\r\n")
         _T("   4) user - User name (empty for system-wide packages)"), DB_BIND_STATIC);
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, EVENT_PACKAGE_UPDATED);
      CHK_EXEC(SQLExecute(hStmt));

      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR,
         _T("Generated when software package removal is detected.\r\n")
         _T("Parameters:\r\n")
         _T("   1) name - Package name\r\n")
         _T("   2) version - Last known package version\r\n")
         _T("   3) user - User name (empty for system-wide packages)"), DB_BIND_STATIC);
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, EVENT_PACKAGE_REMOVED);
      CHK_EXEC(SQLExecute(hStmt));

      DBFreeStatement(hStmt);
   }
   else
   {
      if (!g_ignoreErrors)
         return false;
   }
   CHK_EXEC(SetMinorSchemaVersion(28));
   return true;
}

/**
 * Upgrade from 61.26 to 61.27
 */
static bool H_UpgradeFromV26()
{
   DB_STATEMENT hStmt = DBPrepare(g_dbHandle, _T("UPDATE event_cfg SET message=?,description=? WHERE event_code=?"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, _T("MAC address %<macAddress> moved from %<oldSwitchName>/%<oldPortName> to %<switchName>/%<newPortName>"), DB_BIND_STATIC);
      DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR,
         _T("Generated when a MAC address moves between ports on the same or different switches.\r\n")
         _T("Parameters:\r\n")
         _T("   1) macAddress - MAC address\r\n")
         _T("   2) switchName - New switch name\r\n")
         _T("   3) oldPortName - Previous port name\r\n")
         _T("   4) newPortName - New port name\r\n")
         _T("   5) ipAddress - IP address (if known)\r\n")
         _T("   6) vlanId - VLAN ID\r\n")
         _T("   7) oldSwitchName - Previous switch name\r\n")
         _T("   8) oldSwitchId - Previous switch object ID"), DB_BIND_STATIC);
      DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, EVENT_MAC_MOVED);
      CHK_EXEC(SQLExecute(hStmt));
      DBFreeStatement(hStmt);
   }
   else
   {
      if (!g_ignoreErrors)
         return false;
   }
   CHK_EXEC(SetMinorSchemaVersion(27));
   return true;
}

/**
 * Upgrade from 61.25 to 61.26
 */
static bool H_UpgradeFromV25()
{
   static const wchar_t *batch =
      L"UPDATE config SET data_type='C' WHERE var_name='NotificationChannels.RateLimit.ChannelRateUnit'\n"
      L"INSERT INTO config_values (var_name,var_value,var_description) VALUES ('NotificationChannels.RateLimit.ChannelRateUnit','second','Second')\n"
      L"INSERT INTO config_values (var_name,var_value,var_description) VALUES ('NotificationChannels.RateLimit.ChannelRateUnit','minute','Minute')\n"
      L"INSERT INTO config_values (var_name,var_value,var_description) VALUES ('NotificationChannels.RateLimit.ChannelRateUnit','hour','Hour')\n"
      L"UPDATE config SET data_type='C' WHERE var_name='NotificationChannels.RateLimit.RecipientRateUnit'\n"
      L"INSERT INTO config_values (var_name,var_value,var_description) VALUES ('NotificationChannels.RateLimit.RecipientRateUnit','second','Second')\n"
      L"INSERT INTO config_values (var_name,var_value,var_description) VALUES ('NotificationChannels.RateLimit.RecipientRateUnit','minute','Minute')\n"
      L"INSERT INTO config_values (var_name,var_value,var_description) VALUES ('NotificationChannels.RateLimit.RecipientRateUnit','hour','Hour')\n"
      L"<END>";
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(SetMinorSchemaVersion(26));
   return true;
}

/**
 * Upgrade from 61.24 to 61.25
 */
static bool H_UpgradeFromV24()
{
   CHK_EXEC(SQLQuery(L"ALTER TABLE nodes ADD eip_address varchar(48)"));
   CHK_EXEC(SetMinorSchemaVersion(25));
   return true;
}

/**
 * Upgrade from 61.23 to 61.24
 */
static bool H_UpgradeFromV23()
{
   CHK_EXEC(SQLQuery(L"ALTER TABLE templates ADD exclusion_group varchar(63)"));
   CHK_EXEC(SetMinorSchemaVersion(24));
   return true;
}

/**
 * Upgrade from 61.22 to 61.23
 */
static bool H_UpgradeFromV22()
{
   int ruleId = NextFreeEPPruleID();
   TCHAR query[2048];
   _sntprintf(query, 2048,
      _T("INSERT INTO event_policy (rule_id,rule_guid,flags,comments,alarm_message,alarm_severity,alarm_key,filter_script,alarm_timeout,alarm_timeout_event,incident_delay,incident_ai_depth) ")
      _T("VALUES (%d,'abced820-4a67-4a52-8f0c-59c787e34128',7944,'Terminate unexpected up alarm when interface with expected state DOWN is administratively disabled',")
      _T("'%%m',6,'IF_UNEXP_UP_%%i_%%5',")
      _T("'interface = GetInterfaceObject($node, $event->parameters[5]);\r\n\r\n")
      _T("// if interface not found or interface expected state is not DOWN, do not match\r\n")
      _T("// otherwise (if interface expected state is DOWN), match\r\n")
      _T("return (interface != null && interface->expectedState == 1);',0,%d,0,0)"),
      ruleId, EVENT_ALARM_TIMEOUT);
   CHK_EXEC(SQLQuery(query));

   _sntprintf(query, 2048, _T("INSERT INTO policy_event_list (rule_id,event_code) VALUES (%d,%d)"), ruleId, EVENT_INTERFACE_DISABLED);
   CHK_EXEC(SQLQuery(query));

   CHK_EXEC(SetMinorSchemaVersion(23));
   return true;
}

/**
 * Upgrade from 61.21 to 61.22
 */
static bool H_UpgradeFromV21()
{
   CHK_EXEC(CreateConfigParam(L"SNMP.MinVersion", L"0",
      L"Minimum allowed SNMP version for node communication. Can be overridden per node using SysConfig:SNMP.MinVersion custom attribute.",
      nullptr, 'C', true, false, false, false));

   static const wchar_t *batch =
      L"INSERT INTO config_values (var_name,var_value,var_description) VALUES ('SNMP.MinVersion','0','SNMPv1')\n"
      L"INSERT INTO config_values (var_name,var_value,var_description) VALUES ('SNMP.MinVersion','1','SNMPv2c')\n"
      L"INSERT INTO config_values (var_name,var_value,var_description) VALUES ('SNMP.MinVersion','3','SNMPv3')\n"
      L"<END>";
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(SetMinorSchemaVersion(22));
   return true;
}

/**
 * Upgrade from 61.20 to 61.21
 */
static bool H_UpgradeFromV20()
{
   static const wchar_t *batch =
      L"ALTER TABLE network_maps ADD link_color_source integer\n"
      L"UPDATE network_maps SET link_color_source=0\n"
      L"<END>";
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, L"network_maps", L"link_color_source"));
   CHK_EXEC(SetMinorSchemaVersion(21));
   return true;
}

/**
 * Upgrade from 61.19 to 61.20
 */
static bool H_UpgradeFromV19()
{
   static const wchar_t *batch =
      L"ALTER TABLE ap_common ADD last_modified integer\n"
      L"UPDATE ap_common SET last_modified=0\n"
      L"<END>";
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, L"ap_common", L"last_modified"));
   CHK_EXEC(SetMinorSchemaVersion(20));
   return true;
}

/**
 * Upgrade from 61.18 to 61.19
 */
static bool H_UpgradeFromV18()
{
   if (g_dbSyntax == DB_SYNTAX_TSDB)
   {
      CHK_EXEC(CreateTable(
         L"CREATE TABLE connection_history ("
         L"  record_id $SQL:INT64 not null,"
         L"  event_timestamp timestamptz not null,"
         L"  mac_address varchar(16) not null,"
         L"  ip_address varchar(48) null,"
         L"  node_id integer not null,"
         L"  switch_id integer not null,"
         L"  interface_id integer not null,"
         L"  vlan_id integer not null,"
         L"  event_type integer not null,"
         L"  old_switch_id integer not null,"
         L"  old_interface_id integer not null,"
         L"  PRIMARY KEY(record_id,event_timestamp))"));
      CHK_EXEC(SQLQuery(L"SELECT create_hypertable('connection_history', 'event_timestamp', chunk_time_interval => interval '86400 seconds')"));
   }
   else
   {
      CHK_EXEC(CreateTable(
         L"CREATE TABLE connection_history ("
         L"  record_id $SQL:INT64 not null,"
         L"  event_timestamp integer not null,"
         L"  mac_address varchar(16) not null,"
         L"  ip_address varchar(48) null,"
         L"  node_id integer not null,"
         L"  switch_id integer not null,"
         L"  interface_id integer not null,"
         L"  vlan_id integer not null,"
         L"  event_type integer not null,"
         L"  old_switch_id integer not null,"
         L"  old_interface_id integer not null,"
         L"  PRIMARY KEY(record_id))"));
   }

   CHK_EXEC(SQLQuery(L"CREATE INDEX idx_conn_history_mac ON connection_history(mac_address)"));
   CHK_EXEC(SQLQuery(L"CREATE INDEX idx_conn_history_timestamp ON connection_history(event_timestamp)"));
   CHK_EXEC(SQLQuery(L"CREATE INDEX idx_conn_history_switch ON connection_history(switch_id)"));
   CHK_EXEC(SQLQuery(L"CREATE INDEX idx_conn_history_node ON connection_history(node_id)"));

   CHK_EXEC(CreateConfigParam(L"ConnectionHistory.RetentionTime", L"90",
      L"Number of days to keep connection history records.",
      L"days", 'I', true, false, false, false));
   CHK_EXEC(CreateConfigParam(L"ConnectionHistory.EnableCollection", L"1",
      L"Enable or disable collection of MAC address connection history during topology polls.",
      nullptr, 'B', true, false, false, false));

   CHK_EXEC(CreateEventTemplate(EVENT_MAC_CONNECTED, _T("SYS_MAC_CONNECTED"),
      EVENT_SEVERITY_NORMAL, EF_LOG, _T("620b7117-608f-46ad-8172-d235ff4c10e8"),
      _T("MAC address %<macAddress> connected to port %<portName> on switch %<switchName>"),
      _T("Generated when a MAC address is detected on a switch port.\r\n")
      _T("Parameters:\r\n")
      _T("   1) macAddress - MAC address\r\n")
      _T("   2) switchName - Switch name\r\n")
      _T("   3) portName - Port name\r\n")
      _T("   4) ipAddress - IP address (if known)\r\n")
      _T("   5) vlanId - VLAN ID")));

   CHK_EXEC(CreateEventTemplate(EVENT_MAC_DISCONNECTED, _T("SYS_MAC_DISCONNECTED"),
      EVENT_SEVERITY_NORMAL, EF_LOG, _T("231ed7da-00ae-4616-bb16-bc367ef161b7"),
      _T("MAC address %<macAddress> disconnected from port %<portName> on switch %<switchName>"),
      _T("Generated when a MAC address disappears from a switch port.\r\n")
      _T("Parameters:\r\n")
      _T("   1) macAddress - MAC address\r\n")
      _T("   2) switchName - Switch name\r\n")
      _T("   3) portName - Port name\r\n")
      _T("   4) ipAddress - IP address (if known)\r\n")
      _T("   5) vlanId - VLAN ID")));

   CHK_EXEC(CreateEventTemplate(EVENT_MAC_MOVED, _T("SYS_MAC_MOVED"),
      EVENT_SEVERITY_WARNING, EF_LOG, _T("c44e7313-e083-450f-acd8-e0934e230b9f"),
      _T("MAC address %<macAddress> moved from %<oldPortName> to %<newPortName> on switch %<switchName>"),
      _T("Generated when a MAC address moves between ports on the same switch.\r\n")
      _T("Parameters:\r\n")
      _T("   1) macAddress - MAC address\r\n")
      _T("   2) switchName - Switch name\r\n")
      _T("   3) oldPortName - Previous port name\r\n")
      _T("   4) newPortName - New port name\r\n")
      _T("   5) ipAddress - IP address (if known)\r\n")
      _T("   6) vlanId - VLAN ID")));

   CHK_EXEC(SetMinorSchemaVersion(19));
   return true;
}

/**
 * Upgrade from 61.17 to 61.18
 */
static bool H_UpgradeFromV17()
{
   static const wchar_t *batch =
      L"ALTER TABLE nodes ADD trap_snmp_version integer\n"
      L"ALTER TABLE nodes ADD trap_community varchar(127)\n"
      L"ALTER TABLE nodes ADD trap_usm_auth_password varchar(127)\n"
      L"ALTER TABLE nodes ADD trap_usm_priv_password varchar(127)\n"
      L"ALTER TABLE nodes ADD trap_usm_methods integer\n"
      L"UPDATE nodes SET trap_snmp_version=0,trap_usm_methods=0\n"
      L"<END>";
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, L"nodes", L"trap_snmp_version"));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, L"nodes", L"trap_usm_methods"));
   CHK_EXEC(SetMinorSchemaVersion(18));
   return true;
}

/**
 * Upgrade from 61.16 to 61.17
 */
static bool H_UpgradeFromV16()
{
   CHK_EXEC(SQLQuery(L"ALTER TABLE input_fields ADD default_value varchar(255)"));
   CHK_EXEC(SetMinorSchemaVersion(17));
   return true;
}

/**
 * Upgrade from 61.15 to 61.16
 */
static bool H_UpgradeFromV15()
{
   CHK_EXEC(CreateEventTemplate(EVENT_NODE_DELETED, _T("SYS_NODE_DELETED"),
      EVENT_SEVERITY_NORMAL, 0, _T("b88fedba-faeb-4a06-b671-e7cf515c2e69"),
      _T("Node %<objectName> deleted"),
      _T("Generated when node object deleted from the database.\r\n")
      _T("Parameters:\r\n")
      _T("   1) objectId - Object ID\r\n")
      _T("   2) objectName - Object name\r\n")
      _T("   3) objectClassName - Object class name\r\n")
      _T("   4) primaryIpAddress - Primary IP address")));

   CHK_EXEC(CreateEventTemplate(EVENT_CLUSTER_DELETED, _T("SYS_CLUSTER_DELETED"),
      EVENT_SEVERITY_NORMAL, 0, _T("f926f25c-4cfc-4fdc-811b-add87ae33d2e"),
      _T("Cluster %<objectName> deleted"),
      _T("Generated when cluster object deleted from the database.\r\n")
      _T("Parameters:\r\n")
      _T("   1) objectId - Object ID\r\n")
      _T("   2) objectName - Object name\r\n")
      _T("   3) objectClassName - Object class name\r\n")
      _T("   4) primaryIpAddress - Primary IP address")));

   CHK_EXEC(CreateEventTemplate(EVENT_SENSOR_DELETED, _T("SYS_SENSOR_DELETED"),
      EVENT_SEVERITY_NORMAL, 0, _T("05eadb63-76d5-4cbc-82e5-a23af1ca5b7b"),
      _T("Sensor %<objectName> deleted"),
      _T("Generated when sensor object deleted from the database.\r\n")
      _T("Parameters:\r\n")
      _T("   1) objectId - Object ID\r\n")
      _T("   2) objectName - Object name\r\n")
      _T("   3) objectClassName - Object class name\r\n")
      _T("   4) primaryIpAddress - Primary IP address")));

   CHK_EXEC(CreateEventTemplate(EVENT_ACCESSPOINT_DELETED, _T("SYS_ACCESSPOINT_DELETED"),
      EVENT_SEVERITY_NORMAL, 0, _T("50cda310-4277-4168-8cb2-873e9dd93c01"),
      _T("Access point %<objectName> deleted"),
      _T("Generated when access point object deleted from the database.\r\n")
      _T("Parameters:\r\n")
      _T("   1) objectId - Object ID\r\n")
      _T("   2) objectName - Object name\r\n")
      _T("   3) objectClassName - Object class name\r\n")
      _T("   4) primaryIpAddress - Primary IP address")));

   CHK_EXEC(CreateEventTemplate(EVENT_OBJECT_DELETED, _T("SYS_OBJECT_DELETED"),
      EVENT_SEVERITY_NORMAL, 0, _T("0c9f0861-6546-48b5-9622-ea085261ba7a"),
      _T("Object %<objectName> deleted"),
      _T("Generated when object deleted from the database.\r\n")
      _T("Parameters:\r\n")
      _T("   1) objectId - Object ID\r\n")
      _T("   2) objectName - Object name\r\n")
      _T("   3) objectClassName - Object class name\r\n")
      _T("   4) primaryIpAddress - Primary IP address")));

   // Update existing SYS_SUBNET_DELETED event to use common parameters
   CHK_EXEC(SQLQuery(
      _T("UPDATE event_cfg SET ")
      _T("message='Subnet %<objectName> deleted',")
      _T("description='Generated when subnet object deleted from the database.\r\n")
      _T("Parameters:\r\n")
      _T("   1) objectId - Object ID\r\n")
      _T("   2) objectName - Object name\r\n")
      _T("   3) objectClassName - Object class name\r\n")
      _T("   4) primaryIpAddress - Primary IP address' ")
      _T("WHERE event_code=19")));

   // Add Hook::ObjectDelete script
   CHK_EXEC(SQLQuery(
      _T("INSERT INTO script_library (guid,script_id,script_name,script_code) ")
      _T("VALUES ('ab3547c6-ec79-4ff8-83f4-cb8708fae3dd',28,'Hook::ObjectDelete','')")));

   CHK_EXEC(SetMinorSchemaVersion(16));
   return true;
}

/**
 * Upgrade from 61.14 to 61.15
 */
static bool H_UpgradeFromV14()
{
   if (GetSchemaLevelForMajorVersion(60) < 37)
   {
      CHK_EXEC(DBResizeColumn(g_dbHandle, L"nodes", L"serial_number", 127, true));
      CHK_EXEC(SetSchemaLevelForMajorVersion(60, 37));
   }
   CHK_EXEC(SetMinorSchemaVersion(15));
   return true;
}

/**
 * Upgrade from 61.13 to 61.14
 */
static bool H_UpgradeFromV13()
{
   CHK_EXEC(CreateConfigParam(L"OTLP.MatchCacheTTL", L"300",
      L"TTL in seconds for OTLP resource-to-node match cache.",
      L"seconds", 'I', true, false, false, false));
   CHK_EXEC(CreateConfigParam(L"OTLP.LogUnmatchedResources", L"0",
      L"Log warning when OTLP resources cannot be matched to any node.",
      nullptr, 'B', true, false, false, false));

   CHK_EXEC(CreateTable(
      L"CREATE TABLE otlp_matching_rules ("
      L"  id integer not null,"
      L"  sequence_number integer not null,"
      L"  otel_attribute varchar(255) not null,"
      L"  regex_transform varchar(255) null,"
      L"  node_property integer not null,"
      L"  custom_attr_name varchar(127) null,"
      L"  PRIMARY KEY(id))"));

   // Seed default matching rules
   CHK_EXEC(SQLQuery(L"INSERT INTO otlp_matching_rules (id,sequence_number,otel_attribute,regex_transform,node_property,custom_attr_name) VALUES (1,1,'host.name',NULL,0,NULL)"));
   CHK_EXEC(SQLQuery(L"INSERT INTO otlp_matching_rules (id,sequence_number,otel_attribute,regex_transform,node_property,custom_attr_name) VALUES (2,2,'host.name',NULL,1,NULL)"));
   CHK_EXEC(SQLQuery(L"INSERT INTO otlp_matching_rules (id,sequence_number,otel_attribute,regex_transform,node_property,custom_attr_name) VALUES (3,3,'host.ip',NULL,2,NULL)"));

   CHK_EXEC(SetMinorSchemaVersion(14));
   return true;
}

/**
 * Upgrade from 61.12 to 61.13
 */
static bool H_UpgradeFromV12()
{
   CHK_EXEC(CreateTable(
      L"CREATE TABLE trusted_devices ("
      L"  id integer not null,"
      L"  user_id integer not null,"
      L"  token_hash char(64) not null,"
      L"  description varchar(255) null,"
      L"  creation_time integer not null,"
      L"  PRIMARY KEY(id))"));
   CHK_EXEC(SQLQuery(L"CREATE INDEX idx_trusted_devices_user_id ON trusted_devices(user_id)"));
   CHK_EXEC(SetMinorSchemaVersion(13));
   return true;
}

/**
 * Upgrade from 61.11 to 61.12
 */
static bool H_UpgradeFromV11()
{
   if (GetSchemaLevelForMajorVersion(60) < 36)
   {
      CHK_EXEC(CreateConfigParam(L"Syslog.ResolverCacheTTL", L"300",
         L"TTL in seconds for syslog hostname resolver cache. Caches DNS resolution results to avoid repeated lookups for the same hostname. Set to 0 to disable caching.",
         L"seconds", 'I', true, false, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(60, 36));
   }
   CHK_EXEC(SetMinorSchemaVersion(12));
   return true;
}

/**
 * Upgrade from 61.10 to 61.11
 */
static bool H_UpgradeFromV10()
{
   CHK_EXEC(SQLQuery(L"ALTER TABLE object_tools ADD remote_host varchar(255)"));
   CHK_EXEC(SetMinorSchemaVersion(11));
   return true;
}

/**
 * Upgrade from 61.9 to 61.10
 */
static bool H_UpgradeFromV9()
{
   CHK_EXEC(CreateConfigParam(L"NotificationChannels.RateLimit.ChannelBurst", L"0",
      L"Default channel-level burst size for rate limiting (0 = disabled).",
      L"messages", 'I', true, false, false, false));
   CHK_EXEC(CreateConfigParam(L"NotificationChannels.RateLimit.ChannelRate", L"0",
      L"Default channel-level sustained rate for rate limiting (0 = disabled).",
      L"messages", 'I', true, false, false, false));
   CHK_EXEC(CreateConfigParam(L"NotificationChannels.RateLimit.ChannelRateUnit", L"minute",
      L"Time unit for channel rate limit: second, minute, or hour.",
      nullptr, 'S', true, false, false, false));
   CHK_EXEC(CreateConfigParam(L"NotificationChannels.RateLimit.DigestInterval", L"300",
      L"Interval in seconds between digest notification deliveries.",
      L"seconds", 'I', true, false, false, false));
   CHK_EXEC(CreateConfigParam(L"NotificationChannels.RateLimit.DigestThreshold", L"50",
      L"Queue depth that triggers digest mode. When throttled messages accumulate beyond this count, they are batched into a digest instead of being delivered individually (0 = disabled).",
      L"messages", 'I', true, false, false, false));
   CHK_EXEC(CreateConfigParam(L"NotificationChannels.RateLimit.RecipientBurst", L"0",
      L"Default per-recipient burst size for rate limiting (0 = disabled).",
      L"messages", 'I', true, false, false, false));
   CHK_EXEC(CreateConfigParam(L"NotificationChannels.RateLimit.RecipientRate", L"0",
      L"Default per-recipient sustained rate for rate limiting (0 = disabled).",
      L"messages", 'I', true, false, false, false));
   CHK_EXEC(CreateConfigParam(L"NotificationChannels.RateLimit.RecipientRateUnit", L"minute",
      L"Time unit for recipient rate limit: second, minute, or hour.",
      nullptr, 'S', true, false, false, false));
   CHK_EXEC(SetMinorSchemaVersion(10));
   return true;
}

/**
 * Upgrade from 61.8 to 61.9
 */
static bool H_UpgradeFromV8()
{
   CHK_EXEC(CreateEventTemplate(EVENT_DEVICE_REPLACED, _T("SYS_DEVICE_REPLACED"),
      EVENT_SEVERITY_MAJOR, EF_LOG, _T("d5a3f1e6-7b8c-4d9a-be2f-3c4d5e6f7a8b"),
      _T("Device replaced (serial number: %<oldSerialNumber> -> %<newSerialNumber>)"),
      _T("Generated when device replacement is detected during configuration poll.\r\n")
      _T("Parameters:\r\n")
      _T("   1) oldSerialNumber - Serial number of the old device\r\n")
      _T("   2) newSerialNumber - Serial number of the new device\r\n")
      _T("   3) oldHardwareId - Hardware ID of the old device\r\n")
      _T("   4) newHardwareId - Hardware ID of the new device\r\n")
      _T("   5) oldSnmpObjectId - SNMP object ID of the old device\r\n")
      _T("   6) newSnmpObjectId - SNMP object ID of the new device")));
   CHK_EXEC(SetMinorSchemaVersion(9));
   return true;
}

/**
 * Upgrade from 61.7 to 61.8
 */
static bool H_UpgradeFromV7()
{
   if (GetSchemaLevelForMajorVersion(60) < 35)
   {
      CHK_EXEC(CreateConfigParam(L"Agent.UploadBandwidthLimit", L"0",
         L"Bandwidth limit for file uploads from server to agent (0 = unlimited).",
         L"KB/s", 'I', true, false, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(60, 35));
   }
   CHK_EXEC(SetMinorSchemaVersion(8));
   return true;
}

/**
 * Upgrade from 61.6 to 61.7
 */
static bool H_UpgradeFromV6()
{
   CHK_EXEC(SQLQuery(L"ALTER TABLE mapping_tables ADD guid varchar(36)"));

   DB_RESULT hResult = SQLSelect(L"SELECT id FROM mapping_tables");
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         uuid guid = uuid::generate();
         wchar_t query[256];
         _sntprintf(query, 256, L"UPDATE mapping_tables SET guid='%s' WHERE id=%d", guid.toString().cstr(), DBGetFieldInt32(hResult, i, 0));
         CHK_EXEC(SQLQuery(query));
      }
      DBFreeResult(hResult);
   }
   else if (!g_ignoreErrors)
   {
      return false;
   }

   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, L"mapping_tables", L"guid"));
   CHK_EXEC(SetMinorSchemaVersion(7));
   return true;
}

/**
 * Upgrade from 61.5 to 61.6
 */
static bool H_UpgradeFromV5()
{
   CHK_EXEC(DBResizeColumn(g_dbHandle, L"syslog", L"msg_tag", 48, true));
   static const wchar_t *batch =
      L"ALTER TABLE syslog ADD msg_procid varchar(128)\n"
      L"ALTER TABLE syslog ADD msg_msgid varchar(32)\n"
      L"ALTER TABLE syslog ADD msg_sd $SQL:TEXT\n"
      L"<END>";
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(SetMinorSchemaVersion(6));
   return true;
}

/**
 * Upgrade from 61.4 to 61.5
 */
static bool H_UpgradeFromV4()
{
   CHK_EXEC(CreateTable(
      L"CREATE TABLE cloud_domains ("
      L"  id integer not null,"
      L"  connector_name varchar(63) not null,"
      L"  credentials varchar(4000) null,"
      L"  discovery_filter varchar(4000) null,"
      L"  removal_policy smallint not null,"
      L"  grace_period integer not null,"
      L"  last_discovery_status smallint not null,"
      L"  last_discovery_time integer not null,"
      L"  last_discovery_msg varchar(4000) null,"
      L"  PRIMARY KEY(id))"));

   CHK_EXEC(CreateTable(
      L"CREATE TABLE resources ("
      L"  id integer not null,"
      L"  parent_id integer not null,"
      L"  cloud_resource_id varchar(1024) not null,"
      L"  resource_type varchar(256) not null,"
      L"  region varchar(128) null,"
      L"  state smallint not null,"
      L"  provider_state varchar(256) null,"
      L"  linked_node_id integer not null,"
      L"  account_id varchar(256) null,"
      L"  last_discovery_time integer not null,"
      L"  connector_data $SQL:TEXT null,"
      L"  PRIMARY KEY(id))"));

   CHK_EXEC(CreateTable(
      L"CREATE TABLE resource_tags ("
      L"  resource_id integer not null,"
      L"  tag_key varchar(256) not null,"
      L"  tag_value varchar(1024) null,"
      L"  PRIMARY KEY(resource_id,tag_key))"));

   CHK_EXEC(SetMinorSchemaVersion(5));
   return true;
}

/**
 * Upgrade from 61.3 to 61.4
 */
static bool H_UpgradeFromV3()
{
   static const wchar_t *batch =
      L"ALTER TABLE users ADD tfa_grace_logins integer\n"
      L"UPDATE users SET tfa_grace_logins=5\n"
      L"<END>";
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, L"users", L"tfa_grace_logins"));
   CHK_EXEC(CreateConfigParam(L"Server.Security.2FA.EnforceForAll", L"0", L"Enforce two-factor authentication for all users", nullptr, 'B', true, false, false, false));
   CHK_EXEC(CreateConfigParam(L"Server.Security.2FA.GraceLoginCount", L"5", L"Number of grace logins allowed for users who have not configured two-factor authentication when enforcement is active", nullptr, 'I', true, false, false, false));
   CHK_EXEC(SetMinorSchemaVersion(4));
   return true;
}

/**
 * Upgrade from 61.2 to 61.3
 */
static bool H_UpgradeFromV2()
{
   static const wchar_t *batch =
      L"ALTER TABLE thresholds ADD regenerate_on_value_change char(1)\n"
      L"UPDATE thresholds SET regenerate_on_value_change='0'\n"
      L"<END>";
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, L"thresholds", L"regenerate_on_value_change"));
   CHK_EXEC(SetMinorSchemaVersion(3));
   return true;
}

/**
 * Upgrade from 61.1 to 61.2
 */
static bool H_UpgradeFromV1()
{
   CHK_EXEC(CreateEventTemplate(EVENT_RUNNING_CONFIG_CHANGED, _T("SYS_RUNNING_CONFIG_CHANGED"),
      EVENT_SEVERITY_WARNING, EF_LOG, _T("b3a1e2c4-5d6f-4a8b-9c0e-1f2d3a4b5c6d"),
      _T("Device running configuration changed"),
      _T("Generated when device running configuration change is detected during backup.\r\n")
      _T("Parameters:\r\n")
      _T("   1) previousHash - SHA-256 hash of the previous configuration\r\n")
      _T("   2) newHash - SHA-256 hash of the new configuration")));

   CHK_EXEC(CreateEventTemplate(EVENT_STARTUP_CONFIG_CHANGED, _T("SYS_STARTUP_CONFIG_CHANGED"),
      EVENT_SEVERITY_WARNING, EF_LOG, _T("c4b2f3d5-6e7a-4b9c-ad1f-2e3d4a5b6c7e"),
      _T("Device startup configuration changed"),
      _T("Generated when device startup configuration change is detected during backup.\r\n")
      _T("Parameters:\r\n")
      _T("   1) previousHash - SHA-256 hash of the previous configuration\r\n")
      _T("   2) newHash - SHA-256 hash of the new configuration")));

   CHK_EXEC(SetMinorSchemaVersion(2));
   return true;
}

/**
 * Upgrade from 61.0 to 61.1
 */
static bool H_UpgradeFromV0()
{
   static const wchar_t *batch =
      L"ALTER TABLE thresholds ADD deactivation_sample_count integer\n"
      L"ALTER TABLE thresholds ADD clear_match_count integer\n"
      L"UPDATE thresholds SET deactivation_sample_count=1,clear_match_count=0\n"
      L"ALTER TABLE dct_thresholds ADD deactivation_sample_count integer\n"
      L"UPDATE dct_thresholds SET deactivation_sample_count=1\n"
      L"ALTER TABLE dct_threshold_instances ADD clear_match_count integer\n"
      L"UPDATE dct_threshold_instances SET clear_match_count=0\n"
      L"<END>";
   CHK_EXEC(SQLBatch(batch));

   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, L"thresholds", L"deactivation_sample_count"));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, L"thresholds", L"clear_match_count"));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, L"dct_thresholds", L"deactivation_sample_count"));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, L"dct_threshold_instances", L"clear_match_count"));

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
   { 36, 61, 37, H_UpgradeFromV36 },
   { 35, 61, 36, H_UpgradeFromV35 },
   { 34, 61, 35, H_UpgradeFromV34 },
   { 33, 61, 34, H_UpgradeFromV33 },
   { 32, 61, 33, H_UpgradeFromV32 },
   { 31, 61, 32, H_UpgradeFromV31 },
   { 30, 61, 31, H_UpgradeFromV30 },
   { 29, 61, 30, H_UpgradeFromV29 },
   { 28, 61, 29, H_UpgradeFromV28 },
   { 27, 61, 28, H_UpgradeFromV27 },
   { 26, 61, 27, H_UpgradeFromV26 },
   { 25, 61, 26, H_UpgradeFromV25 },
   { 24, 61, 25, H_UpgradeFromV24 },
   { 23, 61, 24, H_UpgradeFromV23 },
   { 22, 61, 23, H_UpgradeFromV22 },
   { 21, 61, 22, H_UpgradeFromV21 },
   { 20, 61, 21, H_UpgradeFromV20 },
   { 19, 61, 20, H_UpgradeFromV19 },
   { 18, 61, 19, H_UpgradeFromV18 },
   { 17, 61, 18, H_UpgradeFromV17 },
   { 16, 61, 17, H_UpgradeFromV16 },
   { 15, 61, 16, H_UpgradeFromV15 },
   { 14, 61, 15, H_UpgradeFromV14 },
   { 13, 61, 14, H_UpgradeFromV13 },
   { 12, 61, 13, H_UpgradeFromV12 },
   { 11, 61, 12, H_UpgradeFromV11 },
   { 10, 61, 11, H_UpgradeFromV10 },
   { 9,  61, 10, H_UpgradeFromV9  },
   { 8,  61,  9, H_UpgradeFromV8  },
   { 7,  61,  8, H_UpgradeFromV7  },
   { 6,  61,  7, H_UpgradeFromV6  },
   { 5,  61,  6, H_UpgradeFromV5  },
   { 4,  61,  5, H_UpgradeFromV4  },
   { 3,  61,  4, H_UpgradeFromV3  },
   { 2,  61,  3, H_UpgradeFromV2  },
   { 1,  61,  2, H_UpgradeFromV1  },
   { 0,  61,  1, H_UpgradeFromV0  },
   { 0,  0,   0, nullptr }
};

/**
 * Upgrade database to new version
 */
bool MajorSchemaUpgrade_V61()
{
   int32_t major, minor;
   if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
      return false;

   while ((major == 61) && (minor < DB_SCHEMA_VERSION_V61_MINOR))
   {
      // Find upgrade procedure
      int i;
      for (i = 0; s_dbUpgradeMap[i].upgradeProc != nullptr; i++)
         if (s_dbUpgradeMap[i].version == minor)
            break;
      if (s_dbUpgradeMap[i].upgradeProc == nullptr)
      {
         WriteToTerminalEx(L"Unable to find upgrade procedure for version 61.%d\n", minor);
         return false;
      }
      WriteToTerminalEx(L"Upgrading from version 61.%d to %d.%d\n", minor, s_dbUpgradeMap[i].nextMajor, s_dbUpgradeMap[i].nextMinor);
      DBBegin(g_dbHandle);
      if (s_dbUpgradeMap[i].upgradeProc())
      {
         DBCommit(g_dbHandle);
         if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
            return false;
      }
      else
      {
         WriteToTerminal(L"Rolling back last stage due to upgrade errors...\n");
         DBRollback(g_dbHandle);
         return false;
      }
   }
   return true;
}
