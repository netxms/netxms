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
** File: upgrade_v44.cpp
**
**/

#include "nxdbmgr.h"
#include <nxevent.h>

/**
 * Upgrade from 44.29 to 45.0
 */
static bool H_UpgradeFromV29()
{
   CHK_EXEC(SetMajorSchemaVersion(45, 0));
   return true;
}

/**
 * Upgrade from 44.28 to 44.29
 */
static bool H_UpgradeFromV28()
{
   CHK_EXEC(DBDropPrimaryKey(g_dbHandle, _T("policy_action_list")));
   CHK_EXEC(SetMinorSchemaVersion(29));
   return true;
}

/**
 * Upgrade from 44.26 to 44.27
 */
static bool H_UpgradeFromV27()
{
   CHK_EXEC(CreateConfigParam(_T("Syslog.ParseUnknownSourceMessages"),
         _T("0"),
         _T("Enable or disable parsing of syslog messages received from unknown sources."),
         nullptr, 'B', true, false, false, false));
   CHK_EXEC(SetMinorSchemaVersion(28));
   return true;
}

/**
 * Upgrade from 44.26 to 44.27
 */
static bool H_UpgradeFromV26()
{
   static const TCHAR *batch =
      _T("ALTER TABLE ssh_keys ADD tmp_public_key $SQL:TEXT\n")
      _T("ALTER TABLE ssh_keys ADD tmp_private_key $SQL:TEXT\n")
      _T("UPDATE ssh_keys SET tmp_public_key=public_key, tmp_private_key=private_key\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   CHK_EXEC(DBDropColumn(g_dbHandle, _T("ssh_keys"), _T("public_key")));
   CHK_EXEC(DBDropColumn(g_dbHandle, _T("ssh_keys"), _T("private_key")));

   CHK_EXEC(DBRenameColumn(g_dbHandle, _T("ssh_keys"), _T("tmp_public_key"), _T("public_key")));
   CHK_EXEC(DBRenameColumn(g_dbHandle, _T("ssh_keys"), _T("tmp_private_key"), _T("private_key")));

   CHK_EXEC(SetMinorSchemaVersion(27));
   return true;
}

/**
 * Upgrade from 44.25 to 44.26
 */
static bool H_UpgradeFromV25()
{
   CHK_EXEC(CreateEventTemplate(EVENT_IF_SPEED_CHANGED, _T("SYS_IF_SPEED_CHANGED"),
         EVENT_SEVERITY_NORMAL, EF_LOG, _T("3967eab5-71c0-4ba7-aeb9-eb70bcf5caa9"),
         _T("Interface %<ifName> speed changed from %<oldSpeedText> to %<newSpeedText>"),
         _T("Generated when system detects interface speed change.\r\n")
         _T("Parameters:\r\n")
         _T("   1) ifIndex - Interface index\r\n")
         _T("   2) ifName - Interface name\r\n")
         _T("   3) oldSpeed - Old speed in bps\r\n")
         _T("   4) oldSpeedText - Old speed in bps with optional multiplier (kbps, Mbps, etc.)\r\n")
         _T("   5) newSpeed - New speed in bps\r\n")
         _T("   6) newSpeedText - New speed in bps with optional multiplier (kbps, Mbps, etc.)")
      ));
   CHK_EXEC(SetMinorSchemaVersion(26));
   return true;
}

/**
 * Upgrade from 44.24 to 44.25
 */
static bool H_UpgradeFromV24()
{
   CHK_EXEC(CreateConfigParam(_T("Objects.Maintenance.PredefinedPeriods"),
         _T("1h,8h,1d"),
         _T("Predefined object maintenance periods. Use m for minutes, h for hours and d for days."),
         _T(""),
         'S', true, false, false, false));
   CHK_EXEC(SetMinorSchemaVersion(25));
   return true;
}

/**
 * Upgrade from 44.23 to 44.24
 */
static bool H_UpgradeFromV23()
{
   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when threshold value reached for specific data collection item.\r\n")
      _T("Parameters are accessible via %<...> and can have \"m\" or \"multipliers\" and \"u\" or \"units\" format modifiers for value formatting (for example %<{m,u}currentValue>).\r\n\r\n")
      _T("Parameters:\r\n")
      _T("   1) dciName - Parameter name\r\n")
      _T("   2) dciDescription - Item description\r\n")
      _T("   3) thresholdValue - Threshold value\r\n")
      _T("   4) currentValue - Actual value which is compared to threshold value\r\n")
      _T("   5) dciId - Data collection item ID\r\n")
      _T("   6) instance - Instance\r\n")
      _T("   7) isRepeatedEvent - Repeat flag\r\n")
      _T("   8) dciValue - Last collected DCI value\r\n")
      _T("   9) operation - Threshold''s operation code\r\n")
      _T("   10) function - Threshold''s function code\r\n")
      _T("   11) pollCount - Threshold''s required poll count\r\n")
      _T("   12) thresholdDefinition - Threshold''s textual definition'")
      _T(" WHERE event_code=17")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when threshold check is rearmed for specific data collection item.\r\n")
      _T("Parameters are accessible via %<...> and can have \"m\" or \"multipliers\" and \"u\" or \"units\" format modifiers for value formatting (for example %<{m,u}currentValue>)\r\n\r\n")
      _T("Parameters:\r\n")
      _T("   1) dciName - Parameter name\r\n")
      _T("   2) dciDescription - Item description\r\n")
      _T("   3) dciId - Data collection item ID\r\n")
      _T("   4) instance - Instance\r\n")
      _T("   5) thresholdValue - Threshold value\r\n")
      _T("   6) currentValue - Actual value which is compared to threshold value\r\n")
      _T("   7) dciValue - Last collected DCI value\r\n")
      _T("   8) operation - Threshold''s operation code\r\n")
      _T("   9) function - Threshold''s function code\r\n")
      _T("   10) pollCount - Threshold''s required poll count\r\n")
      _T("   11) thresholdDefinition - Threshold''s textual definition'")
      _T( "WHERE event_code=18")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when automatic update of asset management attribute fails.\r\n")
      _T("Parameters:\r\n")
      _T("   1) name - attribute''s name\r\n")
      _T("   2) displayName - attribute''s display name\r\n")
      _T("   3) dataType - attribute''s data type\r\n")
      _T("   4) currValue - current attribute''s value\r\n")
      _T("   5) newValue - new attribute''s value\r\n")
      _T("   6) reason - failure reason'")
      _T(" WHERE event_code=133")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when asset is linked with node.\r\n")
      _T("Parameters:\r\n")
      _T("   1) assetId - asset ID\r\n")
      _T("   2) assetName - asset name'")
      _T(" WHERE event_code=134")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when asset is unlinked from node.\r\n")
      _T("Parameters:\r\n")
      _T("   1) assetId - asset ID\r\n")
      _T("   2) assetName - asset name'")
      _T(" WHERE event_code=135")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("message='Automatic linking of asset %<assetName> failed because it is already linked with node %<currentNodeName>',")
      _T("description='Generated when asset cannot be automatically linked with a node because of conflict.\r\n")
      _T("Parameters:\r\n")
      _T("   1) assetId - asset ID found by automatic linking process\r\n")
      _T("   2) assetName - asset name found by automatic linking process\r\n")
      _T("   3) currentNodeId - ID of currently linked node\r\n")
      _T("   4) currentNodeName - name of currently linked node'")
      _T(" WHERE event_code=136")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated on any system configuration error.\r\n")
      _T("Parameters:\r\n")
      _T("   1) subsystem - The subsystem which has the error\r\n")
      _T("   2) tag - Related tag for the error\r\n")
      _T("   3) descriptipon - Description of the error'")
      _T(" WHERE event_code=137")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when system detects interface Spanning Tree state change.\r\n")
      _T("Parameters:\r\n")
      _T("   1) ifIndex - Interface index\r\n")
      _T("   2) ifName - Interface name\r\n")
      _T("   3) oldState - Old state\r\n")
      _T("   4) oldStateText - Old state as text\r\n")
      _T("   5) newState - New state\r\n")
      _T("   6) newStateText - New state as text'")
      _T(" WHERE event_code=140")));

   CHK_EXEC(SetMinorSchemaVersion(24));
   return true;
}

/**
 * Upgrade from 44.22 to 44.23
 */
static bool H_UpgradeFromV22()
{
   CHK_EXEC(SQLQuery(_T("ALTER TABLE interfaces ADD if_name varchar(255)")));
   CHK_EXEC(SQLQuery(_T("UPDATE interfaces SET if_name=(SELECT name FROM object_properties WHERE object_properties.object_id=interfaces.id)")));
   CHK_EXEC(SetMinorSchemaVersion(23));
   return true;
}

/**
 * Upgrade from 44.21 to 44.22
 */
static bool H_UpgradeFromV21()
{
   CHK_EXEC(SQLQuery(_T("ALTER TABLE nodes ADD snmp_context_engine_id varchar(255)")));
   CHK_EXEC(SetMinorSchemaVersion(22));
   return true;
}

/**
 * Upgrade from 44.20 to 44.21
 */
static bool H_UpgradeFromV20()
{
   CHK_EXEC(SQLQuery(_T("ALTER TABLE thresholds ADD last_event_message varchar(2000)")));
   CHK_EXEC(SetMinorSchemaVersion(21));
   return true;
}

/**
 * Upgrade from 44.19 to 44.20
 */
static bool H_UpgradeFromV19()
{
   static const TCHAR *batch =
      _T("ALTER TABLE interfaces ADD stp_port_state integer\n")
      _T("UPDATE interfaces SET stp_port_state=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("interfaces"), _T("stp_port_state")));

   CHK_EXEC(CreateEventTemplate(EVENT_IF_STP_STATE_CHANGED, _T("SYS_IF_STP_STATE_CHANGED"),
         EVENT_SEVERITY_NORMAL, EF_LOG, _T("977a5634-d0ec-44a0-a7a5-6e193ca57c38"),
         _T("Interface %<ifName> STP state changed from %<oldStateText> to %<newStateText>"),
         _T("Generated when system detects interface Spanning Tree state change.\r\n")
         _T("Parameters:\r\n")
         _T("   1) ifIndex - Interface index\r\n")
         _T("   2) ifName - Interface name\r\n")
         _T("   3) oldState - Old state\r\n")
         _T("   4) oldStateText - Old state as text\r\n")
         _T("   5) newState - New state\r\n")
         _T("   6) newStateText - New state as text")
      ));
   CHK_EXEC(SetMinorSchemaVersion(20));
   return true;
}

/**
 * Upgrade from 44.18 to 44.19
 */
static bool H_UpgradeFromV18()
{
   CHK_EXEC(CreateEventTemplate(EVENT_MODBUS_UNREACHABLE, _T("SYS_MODBUS_UNREACHABLE"),
         EVENT_SEVERITY_MAJOR, EF_LOG, _T("f8dc0d5f-0e46-4bbf-a91d-bb3c0412db2e"),
         _T("Modbus connectivity failed"),
         _T("Generated when node is not responding to Modbus requests.\r\n")
         _T("Parameters:\r\n")
         _T("   No event-specific parameters")
      ));
   CHK_EXEC(CreateEventTemplate(EVENT_MODBUS_OK, _T("SYS_MODBUS_OK"),
         EVENT_SEVERITY_NORMAL, EF_LOG, _T("47584648-fc4e-4757-b4e7-f6f16b146bac"),
         _T("Modbus connectivity restored"),
         _T("Generated when Modbus connectivity with the node is restored.\r\n")
         _T("Parameters:\r\n")
         _T("   No event-specific parameters")
      ));
   CHK_EXEC(SetMinorSchemaVersion(19));
   return true;
}

/**
 * Upgrade from 44.17 to 44.18
 */
static bool H_UpgradeFromV17()
{
   CHK_EXEC(SQLQuery(_T("DELETE FROM config WHERE var_name='InternalCA'")));
   CHK_EXEC(SetMinorSchemaVersion(18));
   return true;
}

/**
 * Upgrade from 44.16 to 44.17
 */
static bool H_UpgradeFromV16()
{
   static const TCHAR *batch =
      _T("ALTER TABLE nodes ADD modbus_proxy integer\n")
      _T("ALTER TABLE nodes ADD modbus_tcp_port integer\n")
      _T("ALTER TABLE nodes ADD modbus_unit_id integer\n")
      _T("UPDATE nodes SET modbus_proxy=0,modbus_tcp_port=502,modbus_unit_id=255\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("nodes"), _T("modbus_proxy")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("nodes"), _T("modbus_tcp_port")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("nodes"), _T("modbus_unit_id")));
   CHK_EXEC(SetMinorSchemaVersion(17));
   return true;
}

/**
 * Upgrade from 44.15 to 44.16
 */
static bool H_UpgradeFromV15()
{
   static const TCHAR *batch =
      _T("ALTER TABLE assets ADD last_update_timestamp integer\n")
      _T("ALTER TABLE assets ADD last_update_uid integer\n")
      _T("UPDATE assets SET last_update_timestamp=0,last_update_uid=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("assets"), _T("last_update_timestamp")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("assets"), _T("last_update_uid")));
   CHK_EXEC(SetMinorSchemaVersion(16));
   return true;
}

/**
 * Upgrade from 44.14 to 44.15
 */
static bool H_UpgradeFromV14()
{
   static const TCHAR *batch =
      _T("ALTER TABLE am_attributes ADD is_hidden char(1)\n")
      _T("UPDATE am_attributes SET is_hidden='0'\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("am_attributes"), _T("is_hidden")));
   CHK_EXEC(SetMinorSchemaVersion(15));
   return true;
}

/**
 * Upgrade from 44.13 to 44.14
 */
static bool H_UpgradeFromV13()
{
   CHK_EXEC(CreateEventTemplate(EVENT_CONFIGURATION_ERROR, _T("SYS_CONFIGURATION_ERROR"),
         EVENT_SEVERITY_MINOR, EF_LOG, _T("762c581c-e9bf-11ed-a05b-0242ac120003"),
         _T("System configuration error (%<description>)"),
         _T("Generated on any system configuration error.\r\n")
         _T("Parameters:\r\n")
         _T("   1) subsystem - The subsystem which has the error\r\n")
         _T("   2) tag - Related tag for the error\r\n")
         _T("   3) descriptipon - Description of the error")
      ));
   CHK_EXEC(SetMinorSchemaVersion(14));
   return true;
}

/**
 * Upgrade from 44.12 to 44.13
 */
static bool H_UpgradeFromV12()
{
   CHK_EXEC(CreateConfigParam(_T("Objects.Assets.AllowDeleteIfLinked"),
         _T("0"),
         _T("Enable/disable deletion of linked assets."),
         nullptr, 'B', true, false, false, false));
   CHK_EXEC(SetMinorSchemaVersion(13));
   return true;
}

/**
 * Upgrade from 44.11 to 44.12
 */
static bool H_UpgradeFromV11()
{
   CHK_EXEC(SQLQuery( _T("UPDATE business_service_checks SET prototype_service_id=0,prototype_check_id=0 WHERE type=1 AND prototype_service_id=service_id")));
   CHK_EXEC(SetMinorSchemaVersion(12));
   return true;
}

/**
 * Upgrade from 44.10 to 44.11
 */
static bool H_UpgradeFromV10()
{
   CHK_EXEC(CreateEventTemplate(EVENT_ASSET_LINK, _T("SYS_ASSET_LINK"),
                                EVENT_SEVERITY_NORMAL, EF_LOG, _T("8dae7b06-b854-4d88-9eb7-721b6110c200"),
                                _T("Asset %<assetName> linked"),
                                _T("Generated when asset is linked with node.\r\n")
                                _T("Parameters:\r\n")
                                _T("   1) assetId - asset ID\r\n")
                                _T("   2) assetName - asset name")
                                ));

   CHK_EXEC(CreateEventTemplate(EVENT_ASSET_UNLINK, _T("SYS_ASSET_UNLINK"),
                                EVENT_SEVERITY_NORMAL, EF_LOG, _T("f149433b-ea2f-4bfd-bf23-35e7778b1b55"),
                                _T("Asset %<assetName> unlinked"),
                                _T("Generated when asset is unlinked from node.\r\n")
                                _T("Parameters:\r\n")
                                _T("   1) assetId - asset ID\r\n")
                                _T("   2) assetName - asset name")
                                ));

   CHK_EXEC(CreateEventTemplate(EVENT_ASSET_LINK_CONFLICT, _T("SYS_ASSET_LINK_CONFLICT"),
                                EVENT_SEVERITY_MINOR, EF_LOG, _T("2bfd6557-1b88-4cf0-801b-78cffb2afc3c"),
                                _T("Automatic linking of asset %<assetName> failed because it is already linked with node %<currentNodeName>"),
                                _T("Generated when asset cannot be automatically linked with a node because of conflict.\r\n")
                                _T("Parameters:\r\n")
                                _T("   1) assetId - asset ID found by automatic linking process\r\n")
                                _T("   2) assetName - asset name found by automatic linking process\r\n")
                                _T("   3) currentNodeId - ID of currently linked node\r\n")
                                _T("   4) currentNodeName - name of currently linked node")
                                ));

   int ruleId = NextFreeEPPruleID();
   TCHAR query[1024];
   _sntprintf(query, 1024, _T("INSERT INTO event_policy (rule_id,rule_guid,flags,comments,alarm_message,alarm_severity,alarm_key,filter_script,alarm_timeout,alarm_timeout_event) ")
                           _T("VALUES (%d,'acbf02d5-3ff1-4235-a8a8-f85755b9a06b',7944,'Generate alarm when asset linking conflict occurs','%%m',5,'ASSET_LINK_CONFLICT_%%i_%%<assetId>','',0,%d)"),
         ruleId, EVENT_ALARM_TIMEOUT);
   CHK_EXEC(SQLQuery(query));

   _sntprintf(query, 1024, _T("INSERT INTO policy_event_list (rule_id,event_code) VALUES (%d,%d)"), ruleId, EVENT_ASSET_LINK_CONFLICT);
   CHK_EXEC(SQLQuery(query));

   CHK_EXEC(SetMinorSchemaVersion(11));
   return true;
}

/**
 * Upgrade from 44.9 to 44.10
 */
static bool H_UpgradeFromV9()
{
   DB_RESULT result = SQLSelect(_T("SELECT id FROM clusters"));
   if (result != nullptr)
   {
      int count = DBGetNumRows(result);
      for (int i = 0; i < count; i++)
      {
         uint32_t clusterId = DBGetFieldULong(result, i, 0);

         TCHAR query[1024];
         _sntprintf(query, 1024,
               _T("UPDATE clusters SET zone_guid="
                     "coalesce((SELECT zone_guid FROM nodes WHERE id="
                     "(SELECT MIN(node_id) FROM cluster_members WHERE cluster_id=%d)), 0) WHERE id=%d"),
               clusterId, clusterId);
         if (!SQLQuery(query) && !g_ignoreErrors)
            return false;
      }
      DBFreeResult(result);
   }

   CHK_EXEC(SetMinorSchemaVersion(10));
   return true;
}

/**
 * Upgrade form 44.8 to 44.9
 */
static bool H_UpgradeFromV8()
{
   if (GetSchemaLevelForMajorVersion(43) < 9)
   {
      if (g_dbSyntax == DB_SYNTAX_TSDB)
      {
         CHK_EXEC(SQLQuery(_T("ALTER TABLE maintenance_journal RENAME TO maintenance_journal_v43_9")));
         CHK_EXEC(CreateTable(
               _T("CREATE TABLE maintenance_journal (")
               _T("   record_id integer not null,")
               _T("   object_id integer not null,")
               _T("   author integer not null,")
               _T("   last_edited_by integer not null,")
               _T("   description $SQL:TEXT null,")
               _T("   creation_time timestamptz not null,")
               _T("   modification_time timestamptz not null,")
               _T("   PRIMARY KEY(record_id,creation_time))")));
         CHK_EXEC(SQLQuery(_T("CREATE INDEX idx_maintjrn_creation_time ON maintenance_journal(creation_time)")));
         CHK_EXEC(SQLQuery(_T("CREATE INDEX idx_maintjrn_object_id ON maintenance_journal(object_id)")));
         CHK_EXEC(SQLQuery(_T("SELECT create_hypertable('maintenance_journal', 'creation_time', chunk_time_interval => interval '86400 seconds')")));

         CHK_EXEC(SQLQuery(_T("ALTER TABLE certificate_action_log RENAME TO certificate_action_log_v43_9")));
         CHK_EXEC(CreateTable(
               _T("CREATE TABLE certificate_action_log (")
               _T("   record_id integer not null,")
               _T("   operation_timestamp timestamptz not null,")
               _T("   operation integer not null,")
               _T("   user_id integer not null,")
               _T("   node_id integer not null,")
               _T("   node_guid varchar(36) null,")
               _T("   cert_type integer not null,")
               _T("   subject varchar(36) null,")
               _T("   serial integer null,")
               _T("   PRIMARY KEY(record_id,operation_timestamp))")));
         CHK_EXEC(SQLQuery(_T("CREATE INDEX idx_cert_action_log_timestamp ON certificate_action_log(operation_timestamp)")));
         CHK_EXEC(SQLQuery(_T("SELECT create_hypertable('certificate_action_log', 'operation_timestamp', chunk_time_interval => interval '86400 seconds')")));

         RegisterOnlineUpgrade(43, 9);
      }
      else
      {
         CHK_EXEC(SQLQuery(_T("CREATE INDEX idx_maintjrn_creation_time ON maintenance_journal(creation_time)")));
         CHK_EXEC(SQLQuery(_T("CREATE INDEX idx_maintjrn_object_id ON maintenance_journal(object_id)")));

         CHK_EXEC(DBRenameColumn(g_dbHandle, _T("certificate_action_log"), _T("timestamp"), _T("operation_timestamp")));
         CHK_EXEC(SQLQuery(_T("CREATE INDEX idx_cert_action_log_timestamp ON certificate_action_log(operation_timestamp)")));
      }

      CHK_EXEC(CreateConfigParam(_T("CertificateActionLog.RetentionTime"),
            _T("370"),
            _T("Retention time in days for certificate action log. All records older than specified will be deleted by housekeeping process."),
            _T("days"),
            'I', true, false, false, false));

      CHK_EXEC(SetSchemaLevelForMajorVersion(43, 9));
   }
   CHK_EXEC(SetMinorSchemaVersion(9));
   return true;
}

/**
 * Upgrade form 44.7 to 44.8
 */
static bool H_UpgradeFromV7()
{
   if (g_dbSyntax == DB_SYNTAX_TSDB)
   {
      CHK_EXEC(CreateTable(
         _T("CREATE TABLE asset_change_log (")
         _T("   record_id $SQL:INT64 not null,")
         _T("   operation_timestamp timestamptz not null,")
         _T("   asset_id integer not null,")
         _T("   attribute_name varchar(63) null,")
         _T("   operation integer not null,")
         _T("   old_value varchar(2000) null,")
         _T("   new_value varchar(2000) null,")
         _T("   user_id integer not null,")
         _T("   linked_object_id integer not null,")
         _T("PRIMARY KEY(record_id,operation_timestamp))")));
      CHK_EXEC(SQLQuery(_T("SELECT create_hypertable('asset_change_log', 'operation_timestamp', chunk_time_interval => interval '86400 seconds')")));
   }
   else
   {
      CHK_EXEC(CreateTable(
         _T("CREATE TABLE asset_change_log (")
         _T("   record_id $SQL:INT64 not null,")
         _T("   operation_timestamp integer not null,")
         _T("   asset_id integer not null,")
         _T("   attribute_name varchar(63) null,")
         _T("   operation integer not null,")
         _T("   old_value varchar(2000) null,")
         _T("   new_value varchar(2000) null,")
         _T("   user_id integer not null,")
         _T("   linked_object_id integer not null,")
         _T("PRIMARY KEY(record_id))")));
   }

   CHK_EXEC(SQLQuery(_T("CREATE INDEX idx_srv_asset_log_timestamp ON asset_change_log(operation_timestamp)")));
   CHK_EXEC(SQLQuery(_T("CREATE INDEX idx_srv_asset_log_asset_id ON asset_change_log(asset_id)")));

   CHK_EXEC(CreateConfigParam(_T("AssetChangeLog.RetentionTime"),
         _T("90"),
         _T("Retention time in days for the records in asset change log. All records older than specified will be deleted by housekeeping process."),
         _T("days"),
         'I', true, false, false, false));

   // Update access rights for predefined "Admins" group
   DB_RESULT hResult = SQLSelect(_T("SELECT system_access FROM user_groups WHERE id=1073741825"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         uint64_t access = DBGetFieldUInt64(hResult, 0, 0);
         access |= SYSTEM_ACCESS_VIEW_ASSET_CHANGE_LOG;
         TCHAR query[256];
         _sntprintf(query, 256, _T("UPDATE user_groups SET system_access=") UINT64_FMT _T(" WHERE id=1073741825"), access);
         CHK_EXEC(SQLQuery(query));
      }
      DBFreeResult(hResult);
   }
   else
   {
      if (!g_ignoreErrors)
         return false;
   }

   CHK_EXEC(SetMinorSchemaVersion(8));
   return true;
}

/**
 * Upgrade from 44.6 to 44.7
 */
static bool H_UpgradeFromV6()
{
   static const TCHAR *batch =
      _T("INSERT INTO acl (object_id,user_id,access_rights) VALUES (5,1073741825,524287)\n")
      _T("ALTER TABLE object_properties ADD asset_id integer\n")
      _T("UPDATE object_properties SET asset_id=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("object_properties"), _T("asset_id")));

   CHK_EXEC(DBRenameColumn(g_dbHandle, _T("am_enum_values"), _T("attr_value"), _T("value")));

   CHK_EXEC(CreateTable(
      _T("CREATE TABLE assets (")
      _T("   id integer not null,")
      _T("   linked_object_id integer not null,")
      _T("   PRIMARY KEY (id))")));

   CHK_EXEC(CreateTable(
      _T("CREATE TABLE asset_properties (")
      _T("   asset_id integer not null,")
      _T("   attr_name varchar(63) not null,")
      _T("   value varchar(2000) null,")
      _T("PRIMARY KEY(asset_id,attr_name))")));

   if (DBIsTableExist(g_dbHandle, _T("am_object_data")) == DBIsTableExist_Found)
   {
      CHK_EXEC(SQLQuery(_T("DROP TABLE am_object_data")));
   }

   CHK_EXEC(SetMinorSchemaVersion(7));
   return true;
}

/**
 * Upgrade from 44.5 to 44.6
 */
static bool H_UpgradeFromV5()
{
   CHK_EXEC(CreateConfigParam(_T("Objects.Nodes.Resolver.AddressFamilyHint"), _T("0"), _T("Address family hint for node DNS name resolver."), nullptr, 'C', true, false, false, false));

   static const TCHAR *batch =
      _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('Objects.Nodes.Resolver.AddressFamilyHint','0','None')\n")
      _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('Objects.Nodes.Resolver.AddressFamilyHint','1','IPv4')\n")
      _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('Objects.Nodes.Resolver.AddressFamilyHint','2','IPv6')\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   CHK_EXEC(SetMinorSchemaVersion(6));
   return true;
}

/**
 * Upgrade from 44.4 to 44.5
 */
static bool H_UpgradeFromV4()
{
   CHK_EXEC(CreateEventTemplate(EVENT_ASSET_AUTO_UPDATE_FAILED, _T("SYS_ASSET_AUTO_UPDATE_FAILED"),
                                EVENT_SEVERITY_MAJOR, EF_LOG, _T("f4ae5e79-9d39-41d0-b74c-b6c591930d08"),
                                _T("Automatic update of asset management attribute \"%<name>\" with value \"%<newValue>\" failed (%<reason>)"),
                                _T("Generated when automatic update of asset management attribute fails.\r\n")
                                _T("Parameters:\r\n")
                                _T("   1) name - attribute's name\r\n")
                                _T("   2) displayName - attribute's display name\r\n")
                                _T("   3) dataType - attribute's data type\r\n")
                                _T("   4) currValue - current attribute's value\r\n")
                                _T("   5) newValue - new attribute's value\r\n")
                                _T("   6) reason - failure reason")
                                ));

   int ruleId = NextFreeEPPruleID();
   TCHAR query[1024];
   _sntprintf(query, 1024, _T("INSERT INTO event_policy (rule_id,rule_guid,flags,comments,alarm_message,alarm_severity,alarm_key,filter_script,alarm_timeout,alarm_timeout_event) ")
                           _T("VALUES (%d,'1499d2d3-2304-4bb1-823b-0c530cbb6224',7944,'Generate alarm when automatic update of asset management attribute fails','%%m',5,'ASSET_UPDATE_FAILED_%%i_%%<name>','',0,%d)"),
         ruleId, EVENT_ALARM_TIMEOUT);
   CHK_EXEC(SQLQuery(query));

   _sntprintf(query, 1024, _T("INSERT INTO policy_event_list (rule_id,event_code) VALUES (%d,%d)"), ruleId, EVENT_ASSET_AUTO_UPDATE_FAILED);
   CHK_EXEC(SQLQuery(query));

   CHK_EXEC(SetMinorSchemaVersion(5));
   return true;
}

/**
 * Upgrade from 44.3 to 44.4
 */
static bool H_UpgradeFromV3()
{
   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when threshold value reached for specific data collection item.\r\n")
      _T("Parameters are accessible via %<...> and can have \"m\" or \"multipliers\" and \"u\" or \"units\" format modifiers for value formatting (for example %<{m,u}currentValue>).\r\n\r\n")
      _T("Parameters:\r\n")
      _T("   1) dciName - Parameter name\r\n")
      _T("   2) dciDescription - Item description\r\n")
      _T("   3) thresholdValue - Threshold value\r\n")
      _T("   4) currentValue - Actual value which is compared to threshold value\r\n")
      _T("   5) dciId - Data collection item ID\r\n")
      _T("   6) instance - Instance\r\n")
      _T("   7) isRepeatedEvent - Repeat flag\r\n")
      _T("   8) dciValue - Last collected DCI value\r\n")
      _T("   9) operation - Threshold''s operation code\r\n")
      _T("   10) function - Threshold''s function code\r\n")
      _T("   11) pollCount - Threshold''s required poll count\r\n")
      _T("   12) thresholdDefinition - Threshold''s textual definition'")
      _T(" WHERE event_code=17")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when threshold check is rearmed for specific data collection item.\r\n")
      _T("Parameters are accessible via %<...> and can have \"m\" or \"multipliers\" and \"u\" or \"units\" format modifiers for value formatting (for example %<{m,u}currentValue>)\r\n\r\n")
      _T("Parameters:\r\n")
      _T("   1) dciName - Parameter name\r\n")
      _T("   2) dciDescription - Item description\r\n")
      _T("   3) dciId - Data collection item ID\r\n")
      _T("   4) instance - Instance\r\n")
      _T("   5) thresholdValue - Threshold value\r\n")
      _T("   6) currentValue - Actual value which is compared to threshold value\r\n")
      _T("   7) dciValue - Last collected DCI value\r\n")
      _T("   8) operation - Threshold''s operation code\r\n")
      _T("   9) function - Threshold''s function code\r\n")
      _T("   10) pollCount - Threshold''s required poll count\r\n")
      _T("   11) thresholdDefinition - Threshold''s textual definition'")
      _T( "WHERE event_code=18")));

   CHK_EXEC(SetMinorSchemaVersion(4));
   return true;
}

/**
 * Upgrade from 44.2 to 44.3
 */
static bool H_UpgradeFromV2()
{
   CHK_EXEC(DBRenameColumn(g_dbHandle, _T("object_properties"), _T("submap_id"), _T("drilldown_object_id")));
   CHK_EXEC(SetMinorSchemaVersion(3));
   return true;
}

/**
 * Upgrade from 44.1 to 44.2
 */
static bool H_UpgradeFromV1()
{
   CHK_EXEC(CreateConfigParam(_T("Server.Security.2FA.TrustedDeviceTTL"), _T("0"), _T("TTL in seconds for 2FA trusted device."), _T("seconds"), 'I', true, false, false, false));
   CHK_EXEC(SetMinorSchemaVersion(2));
   return true;
}

/**
 * Upgrade from 44.0 to 44.1
 */
static bool H_UpgradeFromV0()
{
   CHK_EXEC(CreateTable(
      _T("CREATE TABLE am_attributes (")
      _T("   attr_name varchar(63) not null,")
      _T("   display_name varchar(255) null,")
      _T("   data_type integer not null,")
      _T("   is_mandatory char(1) not null,")
      _T("   is_unique char(1) not null,")
      _T("   autofill_script $SQL:TEXT null,")
      _T("   range_min integer not null,")
      _T("   range_max integer not null,")
      _T("   sys_type integer not null,")
      _T("   PRIMARY KEY(attr_name))")));

   CHK_EXEC(CreateTable(
      _T("CREATE TABLE am_enum_values (")
      _T("   attr_name varchar(63) not null,")
      _T("   attr_value varchar(63) not null,")
      _T("   display_name varchar(255) not null,")
      _T("   PRIMARY KEY(attr_name,attr_value))")));

   // Update access rights for predefined "Admins" group
   DB_RESULT hResult = SQLSelect(_T("SELECT system_access FROM user_groups WHERE id=1073741825"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         uint64_t access = DBGetFieldUInt64(hResult, 0, 0);
         access |= SYSTEM_ACCESS_MANAGE_AM_SCHEMA;
         TCHAR query[256];
         _sntprintf(query, 256, _T("UPDATE user_groups SET system_access=") UINT64_FMT _T(" WHERE id=1073741825"), access);
         CHK_EXEC(SQLQuery(query));
      }
      DBFreeResult(hResult);
   }
   else
   {
      if (!g_ignoreErrors)
         return false;
   }

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
   { 29, 45, 0,  H_UpgradeFromV29 },
   { 28, 44, 29, H_UpgradeFromV28 },
   { 27, 44, 28, H_UpgradeFromV27 },
   { 26, 44, 27, H_UpgradeFromV26 },
   { 25, 44, 26, H_UpgradeFromV25 },
   { 24, 44, 25, H_UpgradeFromV24 },
   { 23, 44, 24, H_UpgradeFromV23 },
   { 22, 44, 23, H_UpgradeFromV22 },
   { 21, 44, 22, H_UpgradeFromV21 },
   { 20, 44, 21, H_UpgradeFromV20 },
   { 19, 44, 20, H_UpgradeFromV19 },
   { 18, 44, 19, H_UpgradeFromV18 },
   { 17, 44, 18, H_UpgradeFromV17 },
   { 16, 44, 17, H_UpgradeFromV16 },
   { 15, 44, 16, H_UpgradeFromV15 },
   { 14, 44, 15, H_UpgradeFromV14 },
   { 13, 44, 14, H_UpgradeFromV13 },
   { 12, 44, 13, H_UpgradeFromV12 },
   { 11, 44, 12, H_UpgradeFromV11 },
   { 10, 44, 11, H_UpgradeFromV10 },
   { 9,  44, 10, H_UpgradeFromV9  },
   { 8,  44, 9,  H_UpgradeFromV8  },
   { 7,  44, 8,  H_UpgradeFromV7  },
   { 6,  44, 7,  H_UpgradeFromV6  },
   { 5,  44, 6,  H_UpgradeFromV5  },
   { 4,  44, 5,  H_UpgradeFromV4  },
   { 3,  44, 4,  H_UpgradeFromV3  },
   { 2,  44, 3,  H_UpgradeFromV2  },
   { 1,  44, 2,  H_UpgradeFromV1  },
   { 0,  44, 1,  H_UpgradeFromV0  },
   { 0,  0,  0,  nullptr }
};

/**
 * Upgrade database to new version
 */
bool MajorSchemaUpgrade_V44()
{
   int32_t major, minor;
   if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
      return false;

   while (major == 44)
   {
      // Find upgrade procedure
      int i;
      for (i = 0; s_dbUpgradeMap[i].upgradeProc != nullptr; i++)
         if (s_dbUpgradeMap[i].version == minor)
            break;
      if (s_dbUpgradeMap[i].upgradeProc == nullptr)
      {
         _tprintf(_T("Unable to find upgrade procedure for version 44.%d\n"), minor);
         return false;
      }
      _tprintf(_T("Upgrading from version 44.%d to %d.%d\n"), minor, s_dbUpgradeMap[i].nextMajor, s_dbUpgradeMap[i].nextMinor);
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
