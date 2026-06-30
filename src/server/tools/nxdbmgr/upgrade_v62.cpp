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
** File: upgrade_v62.cpp
**
**/

#include "nxdbmgr.h"
#include <nxevent.h>
#include <nxtools.h>

/**
 * Upgrade from 62.29 to 62.30
 */
static bool H_UpgradeFromV29()
{
   // Additional STP bridge ID reported by device driver (e.g. MC-LAG / V-STP shared virtual bridge ID).
   CHK_EXEC(SQLQuery(L"ALTER TABLE nodes ADD stp_bridge_id varchar(15)"));
   CHK_EXEC(CreateConfigParam(L"Topology.EnableSTPDiscovery", L"1",
            L"Enable use of Spanning Tree Protocol (STP) information for layer 2 topology discovery. Can be overridden per node with custom attribute SysConfig:Topology.EnableSTPDiscovery.",
            nullptr, 'B', true, false, false));
   CHK_EXEC(SetMinorSchemaVersion(30));
   return true;
}

/**
 * Upgrade from 62.28 to 62.29
 */
static bool H_UpgradeFromV28()
{
   // Topology excluded subnets use zone UIN -1 (ALL_ZONES) to indicate "all zones".
   // Previously zone UIN 0 was used as the "all zones" marker, which conflicted with the default zone.
   CHK_EXEC(SQLQuery(L"UPDATE address_lists SET zone_uin=-1 WHERE list_type=3 AND zone_uin=0"));
   CHK_EXEC(SetMinorSchemaVersion(29));
   return true;
}

/**
 * Upgrade from 62.27 to 62.28
 */
static bool H_UpgradeFromV27()
{
   CHK_EXEC(CreateConfigParam(L"AI.MaxInteractiveIterations", L"30",
      L"Maximum number of LLM tool-call iterations for interactive AI assistant chats before the user is asked whether to continue.",
      nullptr, 'I', true, false, false, false));
   CHK_EXEC(CreateConfigParam(L"AI.MaxBackgroundIterations", L"64",
      L"Maximum number of LLM tool-call iterations for background AI assistant requests (event processing, scheduled tasks, incident analysis) before the request is stopped.",
      nullptr, 'I', true, false, false, false));
   CHK_EXEC(SetMinorSchemaVersion(28));
   return true;
}

/**
 * Upgrade from 62.26 to 62.27
 */
static bool H_UpgradeFromV26()
{
   CHK_EXEC(SQLQuery(L"ALTER TABLE dashboard_elements ADD element_guid char(36)"));

   DB_RESULT hResult = SQLSelect(L"SELECT dashboard_id,element_id FROM dashboard_elements");
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         wchar_t query[256];
         _sntprintf(query, 256, L"UPDATE dashboard_elements SET element_guid='%s' WHERE dashboard_id=%u AND element_id=%u",
            uuid::generate().toString().cstr(), DBGetFieldULong(hResult, i, 0), DBGetFieldULong(hResult, i, 1));
         CHK_EXEC(SQLQuery(query));
      }
      DBFreeResult(hResult);
   }
   else if (!g_ignoreErrors)
   {
      return false;
   }

   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, L"dashboard_elements", L"element_guid"));
   CHK_EXEC(SetMinorSchemaVersion(27));
   return true;
}

/**
 * Upgrade from 62.25 to 62.26
 */
static bool H_UpgradeFromV25()
{
   CHK_EXEC(CreateConfigParam(L"Events.ProcessingMetadata", L"0",
      L"Enable/disable recording of event processing metadata (matched event processing policy rules and the effects they produced) into the event log.",
      nullptr, 'B', true, false, false, false));
   CHK_EXEC(SetMinorSchemaVersion(26));
   return true;
}

/**
 * Upgrade from 62.24 to 62.25
 */
static bool H_UpgradeFromV24()
{
   static const wchar_t *batch =
      L"ALTER TABLE interfaces ADD last_known_speed $SQL:INT64\n"
      L"UPDATE interfaces SET last_known_speed=speed\n"
      L"<END>";
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, L"interfaces", L"last_known_speed"));

   // Add interface speed parameters to interface state change events
   CHK_EXEC(SQLQuery(L"UPDATE event_cfg SET "
      L"message='Interface \"%2\" changed state to UP (IP Addr: %3/%4, IfIndex: %5, Speed: %7)',"
      L"description='Generated when interface goes up.\r\n"
      L"Please note that source of event is node, not an interface itself.\r\n"
      L"Parameters:\r\n"
      L"   1) interfaceObjectId - Interface object ID\r\n"
      L"   2) interfaceName - Interface name\r\n"
      L"   3) interfaceIpAddress - Interface IP address\r\n"
      L"   4) interfaceNetMask - Interface netmask\r\n"
      L"   5) interfaceIndex - Interface index\r\n"
      L"   6) interfaceSpeed - Interface speed in bits per second\r\n"
      L"   7) interfaceSpeedText - Interface speed (human-readable)'"
      L" WHERE event_code=4"));   // SYS_IF_UP

   CHK_EXEC(SQLQuery(L"UPDATE event_cfg SET "
      L"message='Interface \"%2\" changed state to DOWN (IP Addr: %3/%4, IfIndex: %5, Speed: %7)',"
      L"description='Generated when interface goes down.\r\n"
      L"Please note that source of event is node, not an interface itself.\r\n"
      L"Parameters:\r\n"
      L"   1) interfaceObjectId - Interface object ID\r\n"
      L"   2) interfaceName - Interface name\r\n"
      L"   3) interfaceIpAddress - Interface IP address\r\n"
      L"   4) interfaceNetMask - Interface netmask\r\n"
      L"   5) interfaceIndex - Interface index\r\n"
      L"   6) interfaceSpeed - Last known interface speed in bits per second\r\n"
      L"   7) interfaceSpeedText - Last known interface speed (human-readable)'"
      L" WHERE event_code=5"));   // SYS_IF_DOWN

   CHK_EXEC(SQLQuery(L"UPDATE event_cfg SET "
      L"message='Interface \"%2\" changed state to UNKNOWN (IP Addr: %3/%4, IfIndex: %5, Speed: %7)',"
      L"description='Generated when interface goes to unknown state.\r\n"
      L"Please note that source of event is node, not an interface itself.\r\n"
      L"Parameters:\r\n"
      L"   1) interfaceObjectId - Interface object ID\r\n"
      L"   2) interfaceName - Interface name\r\n"
      L"   3) interfaceIpAddress - Interface IP address\r\n"
      L"   4) interfaceNetMask - Interface netmask\r\n"
      L"   5) interfaceIndex - Interface index\r\n"
      L"   6) interfaceSpeed - Last known interface speed in bits per second\r\n"
      L"   7) interfaceSpeedText - Last known interface speed (human-readable)'"
      L" WHERE event_code=45"));   // SYS_IF_UNKNOWN

   CHK_EXEC(SQLQuery(L"UPDATE event_cfg SET "
      L"message='Interface \"%2\" disabled (IP Addr: %3/%4, IfIndex: %5, Speed: %7)',"
      L"description='Generated when interface administratively disabled.\r\n"
      L"Please note that source of event is node, not an interface itself.\r\n"
      L"Parameters:\r\n"
      L"   1) interfaceObjectId - Interface object ID\r\n"
      L"   2) interfaceName - Interface name\r\n"
      L"   3) interfaceIpAddress - Interface IP address\r\n"
      L"   4) interfaceNetMask - Interface netmask\r\n"
      L"   5) interfaceIndex - Interface index\r\n"
      L"   6) interfaceSpeed - Last known interface speed in bits per second\r\n"
      L"   7) interfaceSpeedText - Last known interface speed (human-readable)'"
      L" WHERE event_code=46"));   // SYS_IF_DISABLED

   CHK_EXEC(SQLQuery(L"UPDATE event_cfg SET "
      L"message='Interface \"%2\" is testing (IP Addr: %3/%4, IfIndex: %5, Speed: %7)',"
      L"description='Generated when interface goes to testing state.\r\n"
      L"Please note that source of event is node, not an interface itself.\r\n"
      L"Parameters:\r\n"
      L"   1) interfaceObjectId - Interface object ID\r\n"
      L"   2) interfaceName - Interface name\r\n"
      L"   3) interfaceIpAddress - Interface IP address\r\n"
      L"   4) interfaceNetMask - Interface netmask\r\n"
      L"   5) interfaceIndex - Interface index\r\n"
      L"   6) interfaceSpeed - Last known interface speed in bits per second\r\n"
      L"   7) interfaceSpeedText - Last known interface speed (human-readable)'"
      L" WHERE event_code=47"));   // SYS_IF_TESTING

   CHK_EXEC(SQLQuery(L"UPDATE event_cfg SET "
      L"message='Interface \"%2\" unexpectedly changed state to UP (IP Addr: %3/%4, IfIndex: %5, Speed: %7)',"
      L"description='Generated when interface goes up but it''s expected state set to DOWN.\r\n"
      L"Please note that source of event is node, not an interface itself.\r\n"
      L"Parameters:\r\n"
      L"   1) interfaceObjectId - Interface object ID\r\n"
      L"   2) interfaceName - Interface name\r\n"
      L"   3) interfaceIpAddress - Interface IP address\r\n"
      L"   4) interfaceNetMask - Interface netmask\r\n"
      L"   5) interfaceIndex - Interface index\r\n"
      L"   6) interfaceSpeed - Interface speed in bits per second\r\n"
      L"   7) interfaceSpeedText - Interface speed (human-readable)'"
      L" WHERE event_code=62"));   // SYS_IF_UNEXPECTED_UP

   CHK_EXEC(SQLQuery(L"UPDATE event_cfg SET "
      L"message='Interface \"%2\" with expected state DOWN changed state to DOWN (IP Addr: %3/%4, IfIndex: %5, Speed: %7)',"
      L"description='Generated when interface goes down and it''s expected state is DOWN.\r\n"
      L"Please note that source of event is node, not an interface itself.\r\n"
      L"Parameters:\r\n"
      L"   1) interfaceObjectId - Interface object ID\r\n"
      L"   2) interfaceName - Interface name\r\n"
      L"   3) interfaceIpAddress - Interface IP address\r\n"
      L"   4) interfaceNetMask - Interface netmask\r\n"
      L"   5) interfaceIndex - Interface index\r\n"
      L"   6) interfaceSpeed - Last known interface speed in bits per second\r\n"
      L"   7) interfaceSpeedText - Last known interface speed (human-readable)'"
      L" WHERE event_code=63"));   // SYS_IF_EXPECTED_DOWN

   CHK_EXEC(SetMinorSchemaVersion(25));
   return true;
}

/**
 * Upgrade from 62.23 to 62.24
 */
static bool H_UpgradeFromV23()
{
   CHK_EXEC(CreateTable(
      L"CREATE TABLE localized_strings ("
      L"  entity_class varchar(32) not null,"
      L"  entity_id integer not null,"
      L"  field_tag varchar(32) not null,"
      L"  language varchar(8) not null,"
      L"  value $SQL:TEXT not null,"
      L"  PRIMARY KEY(entity_class,entity_id,field_tag,language))"));
   CHK_EXEC(SQLQuery(L"CREATE INDEX idx_localized_strings_entity ON localized_strings(entity_class,entity_id)"));
   CHK_EXEC(SetMinorSchemaVersion(24));
   return true;
}

/**
 * Upgrade from 62.22 to 62.23
 */
static bool H_UpgradeFromV22()
{
   CHK_EXEC(CreateEventTemplate(EVENT_PENDING_DATABASE_UPGRADE, _T("SYS_PENDING_DATABASE_UPGRADE"),
      EVENT_SEVERITY_WARNING, EF_LOG, _T("52a088b7-487f-413e-af37-5610f6a80f25"),
      _T("Pending background database upgrade detected"),
      _T("Generated on server startup when one or more background (online) database upgrade procedures are still pending.\r\n")
      _T("Run \"nxdbmgr background-upgrade\" to complete the upgrade.")));

   int ruleId = NextFreeEPPruleID();
   TCHAR query[2048];
   _sntprintf(query, 2048,
      _T("INSERT INTO event_policy (rule_id,rule_guid,flags,comments,alarm_message,alarm_severity,alarm_key,filter_script,alarm_timeout,alarm_timeout_event,incident_delay,incident_ai_depth) ")
      _T("VALUES (%d,'ed8fe63b-70d1-49ef-a622-31e567fa101b',7944,'Generate alarm when background database upgrade is pending',")
      _T("'%%m',1,'PENDING_DB_UPGRADE','',0,%d,0,0)"),
      ruleId, EVENT_ALARM_TIMEOUT);
   CHK_EXEC(SQLQuery(query));

   _sntprintf(query, 2048, _T("INSERT INTO policy_event_list (rule_id,event_code) VALUES (%d,%d)"), ruleId, EVENT_PENDING_DATABASE_UPGRADE);
   CHK_EXEC(SQLQuery(query));

   CHK_EXEC(SetMinorSchemaVersion(23));
   return true;
}

/**
 * Upgrade from 62.21 to 62.22
 */
static bool H_UpgradeFromV21()
{
   CHK_EXEC(CreateConfigParam(L"OTLP.MetricCatalogRetention", L"86400",
      L"Retention time in seconds for observed OTLP metric names used by the metric selector. Metrics not seen within this window are dropped from the catalog.",
      L"seconds", 'I', true, false, false, false));
   CHK_EXEC(SetMinorSchemaVersion(22));
   return true;
}

/**
 * Upgrade from 62.20 to 62.21
 */
static bool H_UpgradeFromV20()
{
   CHK_EXEC(CreateTable(
      L"CREATE TABLE event_forwarders ("
      L"  name varchar(63) not null,"
      L"  driver_name varchar(63) not null,"
      L"  description varchar(255) null,"
      L"  configuration $SQL:TEXT null,"
      L"  PRIMARY KEY(name))"));

   // Migrate existing hardcoded FORWARD_EVENT actions into "isc" event forwarder instances.
   // The old implementation stored the target server address in actions.rcpt_addr; create one
   // isc forwarder per distinct address and reference it from the action via channel_name.
   DB_RESULT hResult = SQLSelect(L"SELECT DISTINCT rcpt_addr FROM actions WHERE action_type=4 AND rcpt_addr IS NOT NULL AND rcpt_addr<>''");
   if (hResult != nullptr)
   {
      bool success = true;

      DB_STATEMENT hInsert = DBPrepare(g_dbHandle, L"INSERT INTO event_forwarders (name,driver_name,description,configuration) VALUES (?,'isc',?,?)");
      DB_STATEMENT hUpdate = DBPrepare(g_dbHandle, L"UPDATE actions SET channel_name=?,rcpt_addr=NULL WHERE action_type=4 AND rcpt_addr=?");
      if ((hInsert != nullptr) && (hUpdate != nullptr))
      {
         int count = DBGetNumRows(hResult);
         for(int i = 0; (i < count) && success; i++)
         {
            wchar_t address[256];
            DBGetField(hResult, i, 0, address, 256);

            wchar_t name[64];
            _sntprintf(name, 64, L"isc-%s", address);
            name[63] = 0;

            json_t *config = json_object();
            json_object_set_new(config, "hostname", json_string_t(address));
            char *configText = json_dumps(config, JSON_COMPACT);
            json_decref(config);

            DBBind(hInsert, 1, DB_SQLTYPE_VARCHAR, name, DB_BIND_STATIC);
            DBBind(hInsert, 2, DB_SQLTYPE_VARCHAR, L"Migrated event forwarding target", DB_BIND_STATIC);
            DBBind(hInsert, 3, DB_SQLTYPE_TEXT, DB_CTYPE_UTF8_STRING, configText, DB_BIND_DYNAMIC);
            if (!SQLExecute(hInsert) && !g_ignoreErrors)
            {
               success = false;
               break;
            }

            DBBind(hUpdate, 1, DB_SQLTYPE_VARCHAR, name, DB_BIND_STATIC);
            DBBind(hUpdate, 2, DB_SQLTYPE_VARCHAR, address, DB_BIND_STATIC);
            if (!SQLExecute(hUpdate) && !g_ignoreErrors)
            {
               success = false;
               break;
            }
         }
      }
      else
      {
         success = false;
      }

      if (hInsert != nullptr)
         DBFreeStatement(hInsert);
      if (hUpdate != nullptr)
         DBFreeStatement(hUpdate);
      DBFreeResult(hResult);

      if (!success && !g_ignoreErrors)
         return false;
   }
   else if (!g_ignoreErrors)
   {
      return false;
   }

   CHK_EXEC(SetMinorSchemaVersion(21));
   return true;
}

/**
 * Upgrade from 62.19 to 62.20
 */
static bool H_UpgradeFromV19()
{
   if (g_dbSyntax == DB_SYNTAX_TSDB)
   {
      CHK_EXEC(CreateTable(
         L"CREATE TABLE otel_log ("
         L"  id bigint not null,"
         L"  log_timestamp timestamptz not null,"
         L"  origin_timestamp bigint not null,"
         L"  observed_timestamp bigint not null,"
         L"  node_id integer not null,"
         L"  zone_uin integer not null,"
         L"  service_name varchar(127) null,"
         L"  scope_name varchar(127) null,"
         L"  severity_number integer not null,"
         L"  severity_text varchar(31) null,"
         L"  trace_id varchar(32) null,"
         L"  span_id varchar(16) null,"
         L"  flags integer not null,"
         L"  dropped_attributes_count integer not null,"
         L"  body text null,"
         L"  attributes text null,"
         L"  PRIMARY KEY(id,log_timestamp))"));
      CHK_EXEC(SQLQuery(L"SELECT create_hypertable('otel_log', 'log_timestamp', chunk_time_interval => interval '86400 seconds')"));
   }
   else
   {
      CHK_EXEC(CreateTable(
         L"CREATE TABLE otel_log ("
         L"  id $SQL:INT64 not null,"
         L"  log_timestamp $SQL:INT64 not null,"
         L"  origin_timestamp $SQL:INT64 not null,"
         L"  observed_timestamp $SQL:INT64 not null,"
         L"  node_id integer not null,"
         L"  zone_uin integer not null,"
         L"  service_name varchar(127) null,"
         L"  scope_name varchar(127) null,"
         L"  severity_number integer not null,"
         L"  severity_text varchar(31) null,"
         L"  trace_id varchar(32) null,"
         L"  span_id varchar(16) null,"
         L"  flags integer not null,"
         L"  dropped_attributes_count integer not null,"
         L"  body $SQL:TEXT null,"
         L"  attributes $SQL:TEXT null,"
         L"  PRIMARY KEY(id))"));
   }

   CHK_EXEC(SQLQuery(L"CREATE INDEX idx_otel_log_timestamp ON otel_log(log_timestamp)"));
   CHK_EXEC(SQLQuery(L"CREATE INDEX idx_otel_log_node ON otel_log(node_id)"));
   CHK_EXEC(SQLQuery(L"CREATE INDEX idx_otel_log_trace ON otel_log(trace_id)"));

   CHK_EXEC(CreateConfigParam(L"OTLP.Logs.EnableStorage", L"1",
      L"Enable/disable local storage of received OpenTelemetry log records in NetXMS database.",
      nullptr, 'B', true, false, false, false));
   CHK_EXEC(CreateConfigParam(L"OTLP.Logs.RetentionTime", L"90",
      L"Retention time in days for stored OpenTelemetry log records. All records older than specified will be deleted by housekeeping process.",
      L"days", 'I', true, false, false, false));

   CHK_EXEC(SetMinorSchemaVersion(20));
   return true;
}

/**
 * Upgrade from 62.18 to 62.19
 */
static bool H_UpgradeFromV18()
{
   // Add SYSTEM_ACCESS_SCAN_NETWORK (bit 57 = 0x200000000000000 = 144115188075855872) to Admins group
   if ((g_dbSyntax == DB_SYNTAX_DB2) || (g_dbSyntax == DB_SYNTAX_INFORMIX) || (g_dbSyntax == DB_SYNTAX_ORACLE))
   {
      CHK_EXEC(SQLQuery(L"UPDATE user_groups SET system_access=system_access+144115188075855872 WHERE id=1073741825 AND BITAND(system_access, 144115188075855872)=0"));
   }
   else if (g_dbSyntax == DB_SYNTAX_MSSQL)
   {
      CHK_EXEC(SQLQuery(L"UPDATE user_groups SET system_access=system_access+144115188075855872 WHERE id=1073741825 AND (CAST(system_access AS bigint) & CAST(144115188075855872 AS bigint))=0"));
   }
   else
   {
      CHK_EXEC(SQLQuery(L"UPDATE user_groups SET system_access=system_access+144115188075855872 WHERE id=1073741825 AND (system_access & 144115188075855872)=0"));
   }

   CHK_EXEC(SetMinorSchemaVersion(19));
   return true;
}

/**
 * Upgrade from 62.17 to 62.18
 */
static bool H_UpgradeFromV17()
{
   CHK_EXEC(CreateEventTemplate(EVENT_SNMP_TRAP_AUTH_FAILURE, _T("SYS_SNMP_TRAP_AUTH_FAILURE"),
      EVENT_SEVERITY_WARNING, 1, _T("6a4b7d6e-49de-4fbf-823e-1e2750a1f2e5"),
      _T("SNMP trap from %<sourceIP> rejected: %<reason> (SNMP version %<snmpVersion>)"),
      _T("Generated when an SNMP trap is dropped because its credentials do not match the expected credentials configured for the source node. ")
      _T("Rate-limited to one event per 24 hours per node; the timer is reset when SNMP credentials are changed on the node.\r\n")
      _T("Parameters:\r\n")
      _T("   1) reasonCode - Machine-readable reason (\"community-mismatch\", \"username-mismatch\", \"security-level-mismatch\", \"version-mismatch\")\r\n")
      _T("   2) sourceIP - Source IP address of the trap\r\n")
      _T("   3) reason - Human-readable reason\r\n")
      _T("   4) snmpVersion - SNMP version of the rejected trap (\"1\", \"2c\", or \"3\")\r\n")
      _T("   5) trapOid - SNMP trap OID (may be empty if not available, e.g. on v3 authentication failure)")));

   int ruleId = NextFreeEPPruleID();
   TCHAR query[2048];
   _sntprintf(query, 2048,
      _T("INSERT INTO event_policy (rule_id,rule_guid,flags,comments,alarm_message,alarm_severity,alarm_key,filter_script,alarm_timeout,alarm_timeout_event,incident_delay,incident_ai_depth) ")
      _T("VALUES (%d,'2620ee65-0ef5-4d28-9f45-14c19b34ff31',7944,'Generate alarm when SNMP trap is rejected due to credential mismatch',")
      _T("'%%m',1,'SNMP_TRAP_AUTH_FAILURE_%%i','',0,%d,0,0)"),
      ruleId, EVENT_ALARM_TIMEOUT);
   CHK_EXEC(SQLQuery(query));

   _sntprintf(query, 2048, _T("INSERT INTO policy_event_list (rule_id,event_code) VALUES (%d,%d)"), ruleId, EVENT_SNMP_TRAP_AUTH_FAILURE);
   CHK_EXEC(SQLQuery(query));

   CHK_EXEC(SetMinorSchemaVersion(18));
   return true;
}

/**
 * Upgrade from 62.16 to 62.17
 */
static bool H_UpgradeFromV16()
{
   CHK_EXEC(SQLQuery(L"UPDATE config SET units='days',description='Grace period (in days) before removing an unbound template from a data collection target. If set to 0, templates are removed immediately.' WHERE var_name='DataCollection.TemplateRemovalGracePeriod'"));
   CHK_EXEC(SetMinorSchemaVersion(17));
   return true;
}

/**
 * Upgrade from 62.15 to 62.16
 */
static bool H_UpgradeFromV15()
{
   if (DBMgrMetaDataReadInt32(L"BooleanAggregationMode", 0) == 0)
   {
      CHK_EXEC(DBDropColumn(g_dbHandle, L"items", L"aggregation_mode"));
      CHK_EXEC(SQLQuery(L"ALTER TABLE items ADD aggregation_disabled char(1)"));
      CHK_EXEC(SQLQuery(L"UPDATE items SET aggregation_disabled='0'"));
      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, L"items", L"aggregation_disabled"));
   }
   CHK_EXEC(SQLQuery(L"DELETE FROM metadata WHERE var_name='BooleanAggregationMode'"));
   CHK_EXEC(SetMinorSchemaVersion(16));
   return true;
}

/**
 * Sniff image MIME type from leading magic bytes
 */
static const char *SniffImageMimeType(const BYTE *data, size_t size)
{
   if ((size >= 8) && (memcmp(data, "\x89PNG\r\n\x1a\n", 8) == 0))
      return "image/png";
   if ((size >= 6) && ((memcmp(data, "GIF87a", 6) == 0) || (memcmp(data, "GIF89a", 6) == 0)))
      return "image/gif";
   if ((size >= 3) && (memcmp(data, "\xff\xd8\xff", 3) == 0))
      return "image/jpeg";
   if ((size >= 2) && (memcmp(data, "BM", 2) == 0))
      return "image/bmp";
   if ((size >= 1) && (data[0] == '<'))
      return "image/svg+xml";
   return "image/png";
}

/**
 * Copy object tool name with menu mnemonic markers removed ("&" stripped, "&&" collapsed to "&").
 */
static void RemoveToolNameMnemonics(const wchar_t *toolName, wchar_t *buffer, size_t size)
{
   size_t j = 0;
   for(size_t i = 0; (toolName[i] != 0) && (j < size - 1); i++)
   {
      if (toolName[i] == L'&')
      {
         if (toolName[i + 1] != L'&')
            continue;
         i++;
      }
      buffer[j++] = toolName[i];
   }
   buffer[j] = 0;
}

/**
 * Build unique image library name for migrated object tool icon.
 * Image library has UNIQUE(name, category) with name varchar(63).
 * Returns true on success.
 */
static bool BuildObjectToolImageName(uint32_t toolId, const wchar_t *toolName, wchar_t *outName, size_t outSize)
{
   wchar_t cleanName[256];
   RemoveToolNameMnemonics(toolName, cleanName, 256);
   nx_swprintf(outName, outSize, L"Object tool: %.48ls", cleanName);

   DB_STATEMENT hStmt = DBPrepare(g_dbHandle, L"SELECT guid FROM images WHERE name=? AND category='Object Tools'");
   if (hStmt == nullptr)
      return false;
   DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, outName, DB_BIND_STATIC);
   DB_RESULT hResult = DBSelectPrepared(hStmt);
   bool conflict = (hResult != nullptr) && (DBGetNumRows(hResult) > 0);
   if (hResult != nullptr)
      DBFreeResult(hResult);
   DBFreeStatement(hStmt);

   if (conflict)
      nx_swprintf(outName, outSize, L"Object tool: %.36ls-%u", cleanName, toolId);
   return true;
}

/**
 * Upgrade from 62.14 to 62.15
 */
static bool H_UpgradeFromV14()
{
   CHK_EXEC(SQLQuery(L"ALTER TABLE object_tools ADD icon_guid varchar(36)"));

   // Oracle cannot compare CLOB with <>, but empty strings are stored as NULL there anyway
   DB_RESULT hResult = SQLSelect((g_dbSyntax == DB_SYNTAX_ORACLE) ?
      L"SELECT tool_id,tool_name,icon FROM object_tools WHERE icon IS NOT NULL" :
      L"SELECT tool_id,tool_name,icon FROM object_tools WHERE icon IS NOT NULL AND icon<>''");
   if (hResult != nullptr)
   {
      bool success = true;

      DB_STATEMENT hInsertImage = DBPrepare(g_dbHandle, L"INSERT INTO images (guid,name,category,mimetype,protected,image_data) VALUES (?,?,'Object Tools',?,0,?)");
      DB_STATEMENT hUpdateTool = DBPrepare(g_dbHandle, L"UPDATE object_tools SET icon_guid=? WHERE tool_id=?");

      if ((hInsertImage != nullptr) && (hUpdateTool != nullptr))
      {
         int count = DBGetNumRows(hResult);
         for (int i = 0; (i < count) && success; i++)
         {
            uint32_t toolId = DBGetFieldULong(hResult, i, 0);
            wchar_t toolName[256];
            DBGetField(hResult, i, 1, toolName, 256);

            char *hexIcon = DBGetFieldUTF8(hResult, i, 2, nullptr, 0);
            if ((hexIcon == nullptr) || (hexIcon[0] == 0))
            {
               MemFree(hexIcon);
               continue;
            }

            size_t binSize = strlen(hexIcon) / 2;
            BYTE *binData = MemAllocArray<BYTE>(binSize);
            binSize = StrToBinA(hexIcon, binData, binSize);
            MemFree(hexIcon);

            if (binSize == 0)
            {
               WriteToTerminalEx(L"Cannot decode icon for object tool [%u] \"%s\"\n", toolId, toolName);
               MemFree(binData);
               if (g_ignoreErrors)
                  continue;
               success = false;
               break;
            }

            wchar_t imageName[64];
            if (!BuildObjectToolImageName(toolId, toolName, imageName, 64))
            {
               MemFree(binData);
               success = false;
               break;
            }

            uuid imageGuid = uuid::generate();
            const char *mimeType = SniffImageMimeType(binData, binSize);

            DBBind(hInsertImage, 1, DB_SQLTYPE_VARCHAR, imageGuid);
            DBBind(hInsertImage, 2, DB_SQLTYPE_VARCHAR, imageName, DB_BIND_STATIC);
            DBBind(hInsertImage, 3, DB_SQLTYPE_VARCHAR, DB_CTYPE_UTF8_STRING, mimeType, DB_BIND_STATIC);
            DBBind(hInsertImage, 4, DB_SQLTYPE_TEXT, binData, binSize, DB_BIND_DYNAMIC);
            if (!SQLExecute(hInsertImage) && !g_ignoreErrors)
            {
               success = false;
               break;
            }

            DBBind(hUpdateTool, 1, DB_SQLTYPE_VARCHAR, imageGuid);
            DBBind(hUpdateTool, 2, DB_SQLTYPE_INTEGER, toolId);
            if (!SQLExecute(hUpdateTool) && !g_ignoreErrors)
            {
               success = false;
               break;
            }
         }
      }
      else
      {
         success = false;
      }

      if (hInsertImage != nullptr)
         DBFreeStatement(hInsertImage);
      if (hUpdateTool != nullptr)
         DBFreeStatement(hUpdateTool);
      DBFreeResult(hResult);

      if (!success && !g_ignoreErrors)
         return false;
   }
   else if (!g_ignoreErrors)
   {
      return false;
   }

   CHK_EXEC(DBDropColumn(g_dbHandle, L"object_tools", L"icon"));
   CHK_EXEC(DBRenameColumn(g_dbHandle, L"object_tools", L"icon_guid", L"icon"));

   CHK_EXEC(SetMinorSchemaVersion(15));
   return true;
}

/**
 * Upgrade from 62.13 to 62.14
 */
static bool H_UpgradeFromV13()
{
   static const wchar_t *batch =
      L"ALTER TABLE network_maps ADD canvas_type integer\n"
      L"UPDATE network_maps SET canvas_type=0\n"
      L"ALTER TABLE network_maps ADD initial_view_mode integer\n"
      L"UPDATE network_maps SET initial_view_mode=0\n"
      L"<END>";
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, L"network_maps", L"canvas_type"));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, L"network_maps", L"initial_view_mode"));
   CHK_EXEC(SetMinorSchemaVersion(14));
   return true;
}

/**
 * Base64-encoded PNG payloads for the protected stock images, frozen as of
 * schema version 62.13. These mirror the data seeded by sql/images.in at
 * database initialization and are used to backfill image_data when the
 * on-disk source file is missing - for example when a package upgrade removed
 * the previously package-owned stock image files before the migration ran
 * (see issue #3359).
 */
static const struct
{
   const wchar_t *guid;
   const char *data;
} s_stockImages[] =
{
   // Service
   { L"092e4b35-4e7c-42df-b9b7-d5805bfac64e",
     "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" },
   // ATM
   { L"1ddb76a3-a05f-4a42-acda-22021768feaf",
     "iVBORw0KGgoAAAANSUhEUgAAADAAAAAwCAYAAABXAvmHAAAACXBIWXMAAAsTAAALEwEAmpwYAAADDklEQVR4nO2Y309SURzAv6ApsmW1a5NM/BEIikkgipDLoVNIB7YeUBFJYlpzOs22dNY0FqteKl/64eZjW231Um1tbT700tYD/0P/Qg891vZt3+s1JIR5vT/Z+G6fcfa9555zPtxzzzkAUIpSyBPWjljSbJvYlgNrRywp6uAt9tg6AKCcWB3xu6IJtLRHnlOj1cwQMnVTklLNDLECXJ/iCshJi5gCNkckRY0ajF5stgQlxWD0sgJcn+JEp2fsFjUandnCtUfpf5jaQlhjcAjC1BbKapP6oL64PqUVoJxOX4snGPuR0Olr2TYUFag3h9E7/B7doVV0BK4fCqpL99Sbw+oR0Gi1h35Bqa7qBAAAfbMJTKa/FYTqUN2SAJ8oPYF0kUyhc+4uHFyYKwjVUaUA09B06FWI6qpOwOt/he6BVBaUY6/loV5NAhWV+pxvmnJ0zdW/hY2tMWywRlksjmX1CQAAnneOYCTxgoXKe3Pd4ryNGk1mo9MfbyyOnVjD7bhFMYW6Lq1gu3MiC8qpXkBXzKdRkwi/B8y2UeUE/mdh9TP2Dy9if2DhYIYX2TqF2ogqKXA18rjgsZquUR3ZBTo6R+LUaPjaZnrtYXqHD0sb33+MJ15jcGzzQOLz76QXAIAKAJjhPvlGuVZ77E++p1JWXiWqAAMAHwBgR0RmAaDtwsWN3zb3Ou7n9Nk+0VchH/vN1BkRTBbhVLHHi6/UsD/88acc+4CPFejyIviDwjnFKCQAoqKAQGIeIbUpnGZzjkBP4A12+l7KILB8DyEwmpkOVKacQIGm1ml25ZFcoIxhcpe8mhrBAnuDlv4JaDQ45T+JX542s1CZckUjoDVbcp6AtsVaPFMIkk8QZhcR4nO7UJlyAgU8gbcyvcQpaVYh+ZbR8el9K1AI4c793UHxySsqUHsmM//piEwDpIHyySsmcPlKZu4LwVC3J1Dp6nvwK99hLsL9k0EMjCwJEnBJcJT4xJ1Ied3X3Ts5CUeMbgAYFBE6olfYe248s7tvbh+E0zO2sp/u3onpow6+FKUAfvEXPg2qbxL853MAAAAASUVORK5CYII=" },
   // Unknown
   { L"7cd999e9-fbe0-45c3-a695-f84523b3a50c",
     "iVBORw0KGgoAAAANSUhEUgAAADAAAAAwCAYAAABXAvmHAAAACXBIWXMAAAsTAAALEwEAmpwYAAAFP0lEQVR4nO2Z/U9bVRjHG+f8YT9u+tOm/4VG/xAifTnn3LkMf3FR6O1te2/bc1/6xluBlo0xgguJmQtmI1EwhukyjbhMTJwLM2PZjDCyuEHZZDAogzzmuZTECknvS6ma8CTf3z/f0+c857nfejz7tV/75bqGhxsOFDTyTl6jco9GL3Vr9Ha3She7VLJuitPFLk6mciq9lONU7uTsbc75S55/u04bwut5jWTzGpnL6xRQPdqWurelUugqK8dRzFRngj3oTNBMlvuP1R28J3X8tbxB+gs6LRXK4LbgtwxAByrOSm0J2tfBva/WBb6gE1/BIEUEdw2fYNCOiptaaI+xxj0D7+9vOthrkIGCsQVeY3hoK6s1xs72NzUdrC08bzrUq5Mv6wAPqKxCxzhvOlS7k68nfKwsmV3hvOEV1wbq1DZQAa8wyJiifS7hmd8ufEecQKvsh1TYC0bYC7rYCLrkhWTEBxmFQFuMWoRnkJYZpBXyriP4QpocyRt03ip8RzwAGTkAw0M5uDn5Lcw/moPV1RXY2NiApaeLMPPbHRgfvQDt6vuQkgPW4GUGqSgtcicjtjznLZ98UmqEJ8XHUK2erzyDobMpSEYD1eG3FWWnbcH3cP8xu49UMuwHq1UqrUHOOAVphVqBh2SElbJh8ob109dI1u6F3c3A+noJNjc3djUxcW0MjGigOnxZeoRmLMHjklXQ6QO708YoG/jj4QyMXOwze50HGyGtvAc/TozvMDA3cw9U0WsJ3kADYTY33NBwoKoBc6t0MCoNyQdfj31q/hKZGDEvKU6b1hgFLeSH1efLFQbmHz0EHvRagjci+AswSEr0LQvtQ2Uncz4Z8UNGCUDbLnOei15YWV6qMHBv+hbwkM8yPEoLs0h1Azq57OSRao/TXR+pVJTAYK+6o4XGxy6CFg5YhtcjAhr4rKqBHpVM1eqFTUcJdKc+gqdPFirg8ddIKydxxtuAF0ANs1vVDWikWAt4PPl8tgWW/lysgN/YeAEfn0mCHia24MsG5qsa6NZoyS18RqbmnF/+R9+vl9bgfF/avNT24QXgEluzZsDlYqaFvHD/7lRl26w8g75czOx7J/CqZNWASopu4LMKhc7kqQr4zc1NOJNTHLaNYMKjEpKFFsL0wM1KjOvB+b5khYHpX29CIuhzBc9RIQuXuIuTy272eTQwkOcVBiavXwUu+t3BSwLERQtjFHMbNx8j+LIa0eNwY+IK3Ph+S199fgHiLV5X8ImQqXBVAxg6uf2SwvmuSX5TalnY/y7hIR4U3rS0zHVyOuv8M3BrNTAiBBKiH+KiD1QpYEK7gY+JwozlNA8TMzfwePLnChpM3/4Zfr9/x2yhWIsPdKcnHzL7P+2xWhj3YWLmBD4ZpdCufQAv1tcrLvLI8CCoIeIMPsTW5GZ61GOnMO6zC7/VOhSGzrXtWN4mf7gK8aDfAbwAMZEVbMGbbcRPHG6LsXk78KnyL5BSTu5Ynz8Z7AYuEdvw8aCwwFuanOWmmFXagd+e86pEoMP4EL77ZhR+un4NhgY6QGn22YcXBVBCQoPHTWFWaQd+e1RqEQo8FIC46Hd28qIAsaDQ63Fb+B2aUeiIHXjHcz70N3hRGOWcv+ypRWHQmpXpWL3glSD7ombh7nZh5I1Z5Z6ffFDordnJ71aYVaaibL7m8EH22PWFtWwiQo5g3GdE6Zp7eLaGc543nzjsqXclZXoUEzM9TGftw7NZXA9sv7B7UbhkYeiEuQ1GH5geqGFWVENCCZWQWDEhCb/gPo8rMW6V/4m/Wfdrvzz///oLf/m7MOZIiFwAAAAASUVORK5CYII=" },
   // Node
   { L"904e7291-ee3f-41b7-8132-2bd29288ecc8",
     "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" },
   // HSM
   { L"b314cf44-b2aa-478e-b23a-73bc5bb9a624",
     "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" },
   // Server
   { L"ba6ab507-f62d-4b8f-824c-ca9d46f22375",
     "iVBORw0KGgoAAAANSUhEUgAAADAAAAAwCAYAAABXAvmHAAAACXBIWXMAAAsTAAALEwEAmpwYAAADqUlEQVR4nO2ZSU9TURTHC1RaSstUxpaH4gh8DVxIjDFxBwT8GgqCMpVBcQREwLF8AxVwSFgZtwwbkJZCCygtULVGlsfcN7T3Pd5Q4NyN6Un++9/v5JxzX/JMplSlKlVJVZX3z+WaydhGzeQfqPbGoOqNkAuvf/M5/4rkF5x7KeTsi59w5jlJFE5PCKkc3+NzaoxkF04+24WKUZId4J4KKR8Jh9wj4ToTdlVPxkLVXi14ObgWvAC+x4ML8DsUfATKRyLgHo6Ae2g7iC/gjcXh9bouwet1XYLn4l2n4IfD4BoKA7qA3sjodV1/ZCIUfDgOX/Zkm4WAHLyieRq4+nd8yuvfC2kgmeLjbpgWMyPmA7j4fARXo5RPUEZyfZYHl+CZCCi7ztW/xRNo/AyuIQG89DHJD3wBAk6PzHEFLn6NQu2XaFyAhi95xEBAuajHFSDwtZSABE5S/PA7CwH5omKPUAkFz0RAeWUqmmdQl7hYBC96QLKFL2B0HpO57cpFLVF0XYIvvM9AQA0+Aa5925XnUQ2+iAIncQ5u4gtUjtOfA7vAIb8DhRQ8IwH5yGAvsVMEL7hHsoEvoPwIw34HnBR8/l0GAge+IJHfgQIRnCRvIIQvoFxU7BHKp+Bz+5kIyK8M14T3DpQ2z0LeQAI+tz+ILyC/7eEj3Xbn4NaBWc8Xuy7B5/QF+aALYNx2JXyerOsJeEfvOr6A/EUNA9c0hTpCOTy4AG/3MBGQdx17iR0UvN2zhi+gHBnsd8Ahwmf3rPFBF1DOOvY7YKfgbd0BfAHlomKPUDYPLsBndTERkJ9HrmkadYltFDwTAe3bvpn0bZcvqnxkskRwa+cqH3SBo932oOy2KxfV1q0Ob+lgIKAGr9d1CV676wl4KwVu6fBD5h0/voBe1+mPMO2RMe66RYQ/cZuRAA2fANcYGU8AstsXhXSvasJbKHAB3scHXeBQi+oJwLWJRdiM7sNGdB+uji2Arcuv2/VMCt7czkAg2Y8wEtJ1Ai5VaO8vWFvndUbGFwc3t69ARtsKvoDeokrw0qLa2hZUBOYMu24W4dNvMRIwvu3CopKZJ2NDJAj8ldF5sHb4VOHNFLgA/40PusChb3unnx8bS8ucCK/f9QwRPq1VCLpAcrc9oHvblfAZcXAafhlMLcv4AmpdP+xt1xuZNNJ1Ai4Fuxy96yGjF9XotisXNZ0eGRr+5hL+Tz5HX6DO3rMWMvoI015Un2rX06SRoeFvLF1CF0hVqv7T+gdOdplkD8PfkAAAAABJRU5ErkJggg==" },
   // Router
   { L"bacde727-b183-4e6c-8dca-ab024c88b999",
     "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" },
   // Printer
   { L"f5214d16-1ab1-4577-bb21-063cfd45d7af",
     "iVBORw0KGgoAAAANSUhEUgAAADAAAAAwCAYAAABXAvmHAAAACXBIWXMAAAsTAAALEwEAmpwYAAACeUlEQVR4nO2YS2saURiG5zeUdFFo9+1vKIX2b3RXGgyBQutEHUetoY4THS+TauJtJtFRo7mo7UmaRS5EsAtNMSGpFNoUAr24SJptA4UETjkBR5MG6qTFo8l54Vmf53XOfDIfRV2F8OHph6H0fC02vbjZCULp+Ro6878V8MgzpvreAexU6nsHEJ1JCjRCnoDG1MkVOhNyhXBeoX5WvGcXE4VOj1G7mCigs/+5gM4sSowgwU4XYAQJorOvToFBR+jO0Ij81iqmd2wv07sNDK6JfYMrdshHskedxOCKHaKzW12Qm8Etl3TDwdun5a2+W6wv9TW/tg1B6UNXk1/bhhZ/8ku/KXBTLUDzUn5udQu7HGgT5Eo75ZxagBHi73BLAY2YPIl1tYDZp2zgFgIaQc6Xu4COHe0qACnAkicAyRUqXcaX+Il97JtNnPrFhXOwleeBTFfBnfFDzsidMgnxddwzHVwQozBZoWheBq+KNewyQCOFYg0OOaXXlJ6L2lKLFexCQCPJhQrUO6IMNcgGHoxlVo5xCwGNBKeWj08+OXW0r48L537gFgIa4cbn9pH7ySSy+FOfcAsBjVj9qY/q/0AvTiKTEC+3fJH11iQqFN9DmpfzaoGnjog19aaMXQy0ibJQhs9eRAxqgQFm9P54dvUQtxhok2Bm6ecAK95t7n5oXx8fzdVxi4E24UKz3x8P+6+dWq1Y/MoObjHQJmZvsjmBGjG6J3pmEhndk80J1Iiel+SZ5Q3scuAvZJeqUO+MBf8ooLOIN8weZbebF1yzK5vQ7FE+P2ID16nzgtZ19IhUYLxKlfUlt7oJxpuoom0c+qHPlSchISEhoS6Q32iejTVZG/2ZAAAAAElFTkSuQmCC" },
   // Switch
   { L"f9105c54-8dcf-483a-b387-b4587dfd3cba",
     "iVBORw0KGgoAAAANSUhEUgAAADAAAAAwCAYAAABXAvmHAAAACXBIWXMAAAsTAAALEwEAmpwYAAACB0lEQVR4nO1ZyU7DMBTsf1BRh+2AQAj4KSoEtVnuwEdwZV+SOlz7B8AFhGgP7JtYBJT1Bx6aCAEKKdglcYPkkd6tct48z9ijOpOxsLCwSC2G2vysYLLImXwVjk8mi+ObTPrDLbK97ua54z+YblyEiaCHrNekTQCTb3Tz4rNcbQImZMMdSYO5Bcrn5ok7xR/k5D/r74CByQ4zl7ypDTrYvCbeuUgF5tX8bUoJrFJpeoeA8/I9jfYs1yTRUAIF5lI+N0f55tlvVXonAFxUqjTWuxJJoqEE0PzN0ROp4HLvkcb7vpOIlQAMB+PBgDDibwQGmmdIB+eVarB+IgQwGRgOxoMBYcR/Q6CAbe1aopPt2+BD0C+MqCKh60M1CV3tP9J4fwISwoIj3Ut0unP38TEQiDImGoZxwybGToSrZMLEWBBTwXRUAMOiYZ1j9Gw3wWMUeoQudYDp/k7AJXfSwEUWECjfx06AK0aJeCTUtxKc0SqAYVUkJDQqMRNHGTNs4obvgAgdo8dbesdoqsJc4ctF5k6sK11kqQtzXDNK2DDn2DDnR3rAhrkI2DBX8ya2Yc6PvMhcG+ZqwIY5Vn/EEP8lzAnTBOIOcyJRAky+xBXm/lq8rj93mfRNTFaoFJNFbQIjzlqHcGQ1FQ8crV6bNoFgF7JeEx4XfpJTglN/CR5Z6m3ewsLCImMCb5lm1G6OCpLEAAAAAElFTkSuQmCC" },
};

/**
 * Base64-encoded PNG placeholder inserted in place of any non-stock (user)
 * image whose source file is missing, so the migration completes without
 * leaving image_data NULL and without aborting the whole upgrade.
 */
static const char *s_missingImagePlaceholder = "iVBORw0KGgoAAAANSUhEUgAAACEAAAAiCAMAAADmrkDzAAAAQlBMVEXAwMB4eHiJiYn///8AAABUVFSZEGc6ZWa8vLwlmmXbzf+KZc6ampqsrKy6mf//a83xz5nNzc3AnGX0/8wA/Zt2Zpv+3VcNAAAAAXRSTlMAQObYZgAAAAlwSFlzAAAOwwAADsMBx2+oZAAAAP5JREFUeF6N0emOwyAMhdFeL0C2dtb3f9WJA821qlad7y8HOyKXf4QXJaFPA96KTyTRngTtJIsVQBaDUADTdDWj6IRCoN8VNYlOKDB9fWCqQSi0IYsq2w+FjZK4ou7xQ/woidXqFMDlTiSiiLHV9hyNqWbRJM7d5bVoggEoXCiiVSCCkTVVPwV3rw06z/MyL+bqIttdGCejg1/VvmMIJcEBlliRhZIggAaIHRQ3Nc7oA/o5xUkQ9zkgiRJEj9K5ZXFTO27nBZpECRO397YTJFFK0fECrCVxK94XvBK2pz7mEyTR3L1vYI9/X/VxQxZ8AqY9ivMJCCgsegDGLu/7A3nNEZAW2MWMAAAAAElFTkSuQmCC";

/**
 * Find embedded base64-encoded payload for a protected stock image by GUID.
 * Returns nullptr if the GUID is not one of the known stock images.
 */
static const char *FindStockImage(const wchar_t *guid)
{
   for (size_t i = 0; i < sizeof(s_stockImages) / sizeof(s_stockImages[0]); i++)
   {
      if (!wcsicmp(guid, s_stockImages[i].guid))
         return s_stockImages[i].data;
   }
   return nullptr;
}

/**
 * Upgrade from 62.12 to 62.13
 */
static bool H_UpgradeFromV12()
{
   CHK_EXEC(SQLQuery(L"ALTER TABLE images ADD image_data $SQL:TEXT"));

   wchar_t dataDir[MAX_PATH];
   GetNetXMSDirectory(nxDirData, dataDir);

   DB_RESULT hResult = SQLSelect(L"SELECT guid FROM images");
   if (hResult != nullptr)
   {
      bool success = true;

      DB_STATEMENT hStmt = DBPrepare(g_dbHandle, L"UPDATE images SET image_data=? WHERE guid=?");
      if (hStmt != nullptr)
      {
         int count = DBGetNumRows(hResult);
         for (int i = 0; (i < count) && success; i++)
         {
            wchar_t guid[64];
            DBGetField(hResult, i, 0, guid, 64);

            wchar_t path[MAX_PATH];
            nx_swprintf(path, MAX_PATH, L"%s" DDIR_IMAGES L"/%s", dataDir, guid);

            size_t fileSize = 0;
            BYTE *data = LoadFile(path, &fileSize);
            if (data == nullptr)
            {
               // Source file is missing - e.g. removed by a package upgrade before this
               // migration ran (issue #3359). Backfill protected stock images from their
               // embedded copies; for user images, warn and insert a placeholder so the
               // upgrade can complete instead of aborting on a single lost file.
               const char *encoded = FindStockImage(guid);
               if (encoded == nullptr)
               {
                  WriteToTerminalEx(L"WARNING: image file \"%s\" is missing; inserting placeholder image\n", path);
                  encoded = s_missingImagePlaceholder;
               }

               char *decoded = nullptr;
               size_t decodedSize = 0;
               if (!base64_decode_alloc(encoded, strlen(encoded), &decoded, &decodedSize) || (decoded == nullptr))
               {
                  WriteToTerminalEx(L"Cannot decode embedded image data for \"%s\"\n", path);
                  if (g_ignoreErrors)
                     continue;
                  success = false;
                  break;
               }
               data = reinterpret_cast<BYTE*>(decoded);
               fileSize = decodedSize;
            }

            DBBind(hStmt, 1, DB_SQLTYPE_TEXT, data, fileSize, DB_BIND_DYNAMIC);
            DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, guid, DB_BIND_STATIC);
            if (!SQLExecute(hStmt) && !g_ignoreErrors)
               success = false;
         }

         DBFreeStatement(hStmt);
      }

      DBFreeResult(hResult);

      if (!success && !g_ignoreErrors)
         return false;
   }
   else if (!g_ignoreErrors)
   {
      return false;
   }

   CHK_EXEC(SetMinorSchemaVersion(13));
   return true;
}

/**
 * Upgrade from 62.11 to 62.12
 */
static bool H_UpgradeFromV11()
{
   CHK_EXEC(SQLQuery(L"ALTER TABLE items ADD mapping_table_id integer"));
   CHK_EXEC(SQLQuery(L"UPDATE items SET mapping_table_id=0"));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, L"items", L"mapping_table_id"));
   CHK_EXEC(SetMinorSchemaVersion(12));
   return true;
}

/**
 * Upgrade from 62.10 to 62.11
 */
static bool H_UpgradeFromV10()
{
   // Add agent_platform_name column to nodes table.
   // Existing platform_name values reflect the agent binary architecture
   // (because the agent reported architecture via GetSystemInfo, which is
   // WOW64-aware). Seed agent_platform_name from platform_name so that
   // agent-installer package matching keeps working immediately after the
   // upgrade. On the next configuration poll, platform_name will be
   // overwritten with the operating system architecture reported via the
   // new System.OSPlatformName parameter.
   CHK_EXEC(SQLQuery(L"ALTER TABLE nodes ADD agent_platform_name varchar(63)"));
   CHK_EXEC(SQLQuery(L"UPDATE nodes SET agent_platform_name=platform_name"));
   CHK_EXEC(SetMinorSchemaVersion(11));
   return true;
}

/**
 * Upgrade from 62.9 to 62.10
 */
static bool H_UpgradeFromV9()
{
   CHK_EXEC(CreateConfigParam(L"Server.Security.2FA.TokenTimeout",
            L"120",
            L"Time (in seconds) the user has to respond to a 2FA challenge before the client cancels the prompt. Set to 0 to disable the auto-cancel timer.",
            L"seconds", 'I', true, false, false, false));
   CHK_EXEC(SetMinorSchemaVersion(10));
   return true;
}

/**
 * Create per-storage-class hourly and daily continuous aggregates plus the
 * matching union views (TSDB only). Mirrors the DDL in sql/schema.in.
 */
static bool CreateAggregateCAGGsForStorageClass(const wchar_t *cls)
{
   wchar_t query[1024];

   nx_swprintf(query, 1024,
      L"CREATE MATERIALIZED VIEW idata_1h_sc_%ls WITH (timescaledb.continuous) AS "
      L"SELECT item_id, time_bucket(interval '1 hour', idata_timestamp) AS bucket_start, "
      L"min(idata_value::double precision) AS min_value, "
      L"max(idata_value::double precision) AS max_value, "
      L"avg(idata_value::double precision) AS avg_value, count(*) AS sample_count "
      L"FROM idata_sc_%ls "
      L"WHERE idata_value ~ '^[-+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?$' "
      L"GROUP BY item_id, time_bucket(interval '1 hour', idata_timestamp) "
      L"WITH NO DATA", cls, cls);
   if (!SQLQuery(query))
      return false;

   nx_swprintf(query, 1024,
      L"CREATE MATERIALIZED VIEW idata_1d_sc_%ls WITH (timescaledb.continuous) AS "
      L"SELECT item_id, time_bucket(interval '1 day', bucket_start) AS bucket_start, "
      L"min(min_value) AS min_value, max(max_value) AS max_value, "
      L"sum(avg_value * sample_count) / sum(sample_count) AS avg_value, "
      L"sum(sample_count) AS sample_count "
      L"FROM idata_1h_sc_%ls "
      L"GROUP BY item_id, time_bucket(interval '1 day', bucket_start) "
      L"WITH NO DATA", cls, cls);
   if (!SQLQuery(query))
      return false;
   return true;
}

/**
 * Upgrade from 62.8 to 62.9
 */
static bool H_UpgradeFromV8()
{
   if (g_dbSyntax == DB_SYNTAX_TSDB)
   {
      // Hierarchical CAGGs (daily-over-hourly) require TimescaleDB >= 2.10.
      // Fail the upgrade on older versions so the admin upgrades TimescaleDB
      // and re-runs nxdbmgr; never bump the schema version with the CAGGs missing.
      DB_RESULT hResult = SQLSelect(L"SELECT extversion FROM pg_extension WHERE extname='timescaledb'");
      if (hResult == nullptr)
         return false;

      bool versionOk = false;
      char version[64] = "";
      if (DBGetNumRows(hResult) > 0)
      {
         DBGetFieldA(hResult, 0, 0, version, 64);
         int major, minor;
         if (sscanf(version, "%d.%d.", &major, &minor) == 2)
            versionOk = (major > 2) || ((major == 2) && (minor >= 10));
      }
      DBFreeResult(hResult);

      if (!versionOk)
      {
         WriteToTerminalEx(L"\x1b[31mTimescaleDB %hs detected; DCI data aggregation requires TimescaleDB 2.10 or later. Upgrade TimescaleDB and re-run nxdbmgr.\x1b[0m\n",
            (version[0] != 0) ? version : "(unknown)");
         return false;
      }

      static const wchar_t *storageClasses[] = { L"default", L"7", L"30", L"90", L"180", L"other" };
      for(size_t i = 0; i < sizeof(storageClasses) / sizeof(storageClasses[0]); i++)
      {
         if (!CreateAggregateCAGGsForStorageClass(storageClasses[i]))
            return false;
      }

      CHK_EXEC(SQLQuery(
         L"CREATE VIEW idata_1h AS "
         L"SELECT * FROM idata_1h_sc_default UNION ALL "
         L"SELECT * FROM idata_1h_sc_7 UNION ALL "
         L"SELECT * FROM idata_1h_sc_30 UNION ALL "
         L"SELECT * FROM idata_1h_sc_90 UNION ALL "
         L"SELECT * FROM idata_1h_sc_180 UNION ALL "
         L"SELECT * FROM idata_1h_sc_other"));
      CHK_EXEC(SQLQuery(
         L"CREATE VIEW idata_1d AS "
         L"SELECT * FROM idata_1d_sc_default UNION ALL "
         L"SELECT * FROM idata_1d_sc_7 UNION ALL "
         L"SELECT * FROM idata_1d_sc_30 UNION ALL "
         L"SELECT * FROM idata_1d_sc_90 UNION ALL "
         L"SELECT * FROM idata_1d_sc_180 UNION ALL "
         L"SELECT * FROM idata_1d_sc_other"));
   }

   CHK_EXEC(SetMinorSchemaVersion(9));
   return true;
}

/**
 * Upgrade from 62.7 to 62.8
 */
static bool H_UpgradeFromV7()
{
   CHK_EXEC(CreateConfigParam(L"Server.Security.PasswordHash.MemoryCostKB",
            L"65536",
            L"Argon2id memory cost in KiB used when hashing user passwords. Higher values strengthen resistance to GPU/ASIC cracking at the cost of login latency and server memory.",
            L"KB", 'I', true, false, false, false));
   CHK_EXEC(CreateConfigParam(L"Server.Security.PasswordHash.TimeCost",
            L"3",
            L"Argon2id iteration count used when hashing user passwords.",
            nullptr, 'I', true, false, false, false));
   CHK_EXEC(CreateConfigParam(L"Server.Security.PasswordHash.Parallelism",
            L"1",
            L"Argon2id parallelism (lanes) used when hashing user passwords.",
            nullptr, 'I', true, false, false, false));

   // Users still on legacy unsalted SHA-1 hashes cannot be migrated to Argon2id at-rest
   // (the plaintext is needed). Force a password change on next login so dormant accounts
   // do not remain on unsalted SHA-1 indefinitely.
   if ((g_dbSyntax == DB_SYNTAX_DB2) || (g_dbSyntax == DB_SYNTAX_INFORMIX) || (g_dbSyntax == DB_SYNTAX_ORACLE))
   {
      CHK_EXEC(SQLQuery(L"UPDATE users SET flags=flags+8 WHERE BITAND(flags,8)=0 AND password NOT LIKE '$%' AND password<>''"));
   }
   else if (g_dbSyntax == DB_SYNTAX_MSSQL)
   {
      CHK_EXEC(SQLQuery(L"UPDATE users SET flags=flags+8 WHERE (CAST(flags AS bigint) & CAST(8 AS bigint))=0 AND password NOT LIKE '$%' AND password<>''"));
   }
   else
   {
      CHK_EXEC(SQLQuery(L"UPDATE users SET flags=flags+8 WHERE (flags & 8)=0 AND password NOT LIKE '$%' AND password<>''"));
   }

   CHK_EXEC(SetMinorSchemaVersion(8));
   return true;
}

/**
 * Upgrade from 62.6 to 62.7
 */
static bool H_UpgradeFromV6()
{
   // Per-object idata/tdata table and index creation statements are now
   // emitted from code (see src/server/include/dci_table_creation.h),
   // not read from metadata rows. Drop the now-unused template rows.
   static const wchar_t *batch =
      L"DELETE FROM metadata WHERE var_name='IDataTableCreationCommand'\n"
      L"DELETE FROM metadata WHERE var_name LIKE 'IDataIndexCreationCommand_%'\n"
      L"DELETE FROM metadata WHERE var_name LIKE 'TDataTableCreationCommand_%'\n"
      L"DELETE FROM metadata WHERE var_name LIKE 'TDataIndexCreationCommand_%'\n"
      L"<END>";
   CHK_EXEC(SQLBatch(batch));

   CHK_EXEC(SetMinorSchemaVersion(7));
   return true;
}

/**
 * Upgrade from 62.5 to 62.6
 */
static bool H_UpgradeFromV5()
{
   static const wchar_t *batch =
      L"ALTER TABLE items ADD aggregation_disabled char(1)\n"
      L"ALTER TABLE items ADD hourly_retention integer\n"
      L"ALTER TABLE items ADD daily_retention integer\n"
      L"ALTER TABLE items ADD aggregation_watermark $SQL:INT64\n"
      L"UPDATE items SET aggregation_disabled='0'\n"
      L"<END>";
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, L"items", L"aggregation_disabled"));
   CHK_EXEC(DBMgrMetaDataWriteInt32(L"BooleanAggregationMode", 1));

   CHK_EXEC(CreateConfigParam(L"DataCollection.Aggregation.Enabled",
            L"0",
            L"Master switch for DCI data aggregation. When enabled, the server rolls up raw DCI values into hourly and daily aggregates for eligible items.",
            nullptr, 'B', true, true, false, false));
   CHK_EXEC(CreateConfigParam(L"DataCollection.Aggregation.DefaultHourlyRetentionTime",
            L"365",
            L"Default retention time for hourly DCI aggregates. Individual DCIs can override this setting.",
            L"days", 'I', true, false, false, false));
   CHK_EXEC(CreateConfigParam(L"DataCollection.Aggregation.DefaultDailyRetentionTime",
            L"1825",
            L"Default retention time for daily DCI aggregates. Individual DCIs can override this setting.",
            L"days", 'I', true, false, false, false));
   CHK_EXEC(CreateConfigParam(L"DataCollection.Aggregation.HourlyCloseWindow",
            L"300",
            L"Lag in seconds before a closed hour is rolled up into the hourly aggregate, giving late samples time to arrive.",
            L"seconds", 'I', true, false, false, false));
   CHK_EXEC(CreateConfigParam(L"DataCollection.Aggregation.DailyCloseWindow",
            L"1800",
            L"Lag in seconds before a closed day is rolled up into the daily aggregate, giving late samples time to arrive.",
            L"seconds", 'I', true, false, false, false));
   CHK_EXEC(CreateConfigParam(L"DataCollection.Aggregation.BackfillOnEnable",
            L"1",
            L"When enabling aggregation, initialize per-DCI watermarks to the earliest retained raw timestamp so existing history is backfilled on the next rollup pass.",
            nullptr, 'B', true, false, false, false));
   CHK_EXEC(CreateConfigParam(L"DataCollection.Aggregation.MaxAutoSelectPoints",
            L"5000",
            L"Upper bound on the number of points returned by auto-selected aggregate tier when serving DCI history queries.",
            nullptr, 'I', true, false, false, false));
   CHK_EXEC(CreateConfigParam(L"DataCollection.Aggregation.TSDB.RefreshStartOffset",
            L"30",
            L"TimescaleDB continuous aggregate refresh lookback window. Caps the outage length that can be recovered via late-arriving data on TSDB backends.",
            L"days", 'I', true, false, false, false));
   CHK_EXEC(CreateConfigParam(L"DataCollection.Aggregation.TSDB.RefreshScheduleInterval",
            L"600",
            L"TimescaleDB continuous aggregate refresh cadence.",
            L"seconds", 'I', true, false, false, false));

   CHK_EXEC(SetMinorSchemaVersion(6));
   return true;
}

/**
 * Upgrade from 62.4 to 62.5
 */
static bool H_UpgradeFromV4()
{
   if (GetSchemaLevelForMajorVersion(61) < 36)
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
      CHK_EXEC(SetSchemaLevelForMajorVersion(61, 36));
   }
   CHK_EXEC(SetMinorSchemaVersion(5));
   return true;
}

/**
 * Upgrade from 62.3 to 62.4
 */
static bool H_UpgradeFromV3()
{
   CHK_EXEC(CreateConfigParam(L"DebugConsole.AllowedScriptLocations",
            L"@library",
            L"Ordered, comma-separated list of locations searched by the debug console \"exec\" command. Use @library for the server script library or an absolute directory path (non-recursive). Empty value disables \"exec\".",
            nullptr, 'S', true, false, false, false));
   CHK_EXEC(SetMinorSchemaVersion(4));
   return true;
}

/**
 * Upgrade from 62.2 to 62.3
 */
static bool H_UpgradeFromV2()
{
   // Add SYSTEM_ACCESS_VIEW_NOTIFICATION_LOG (bit 55 = 0x80000000000000 = 36028797018963968)
   // and SYSTEM_ACCESS_VIEW_ACTION_LOG (bit 56 = 0x100000000000000 = 72057594037927936) to Admins group
   // (combined mask = 0x180000000000000 = 108086391056891904)
   if ((g_dbSyntax == DB_SYNTAX_DB2) || (g_dbSyntax == DB_SYNTAX_INFORMIX) || (g_dbSyntax == DB_SYNTAX_ORACLE))
   {
      CHK_EXEC(SQLQuery(L"UPDATE user_groups SET system_access=system_access+108086391056891904 WHERE id=1073741825 AND BITAND(system_access, 108086391056891904)=0"));
   }
   else if (g_dbSyntax == DB_SYNTAX_MSSQL)
   {
      CHK_EXEC(SQLQuery(L"UPDATE user_groups SET system_access=system_access+108086391056891904 WHERE id=1073741825 AND (CAST(system_access AS bigint) & CAST(108086391056891904 AS bigint))=0"));
   }
   else
   {
      CHK_EXEC(SQLQuery(L"UPDATE user_groups SET system_access=system_access+108086391056891904 WHERE id=1073741825 AND (system_access & 108086391056891904)=0"));
   }

   CHK_EXEC(SetMinorSchemaVersion(3));
   return true;
}

/**
 * Upgrade from 62.1 to 62.2
 */
static bool H_UpgradeFromV1()
{
   static const wchar_t *batch =
      L"ALTER TABLE object_tools ADD applicable_classes integer\n"
      L"UPDATE object_tools SET applicable_classes=1\n"
      L"<END>";
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, L"object_tools", L"applicable_classes"));
   CHK_EXEC(SetMinorSchemaVersion(2));
   return true;
}

/**
 * Upgrade from 62.0 to 62.1
 */
static bool H_UpgradeFromV0()
{
   CHK_EXEC(CreateConfigParam(L"Scripts.RestrictWriteAccess",
            L"1",
            L"Restrict write access for transformation, filter, predicate, and analysis scripts (DCI transformations, autobind filters, thresholds, conditions, RCA, etc.). When enabled, such scripts cannot modify objects.",
            nullptr, 'B', true, false, false, false));
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
   { 29, 62, 30, H_UpgradeFromV29 },
   { 28, 62, 29, H_UpgradeFromV28 },
   { 27, 62, 28, H_UpgradeFromV27 },
   { 26, 62, 27, H_UpgradeFromV26 },
   { 25, 62, 26, H_UpgradeFromV25 },
   { 24, 62, 25, H_UpgradeFromV24 },
   { 23, 62, 24, H_UpgradeFromV23 },
   { 22, 62, 23, H_UpgradeFromV22 },
   { 21, 62, 22, H_UpgradeFromV21 },
   { 20, 62, 21, H_UpgradeFromV20 },
   { 19, 62, 20, H_UpgradeFromV19 },
   { 18, 62, 19, H_UpgradeFromV18 },
   { 17, 62, 18, H_UpgradeFromV17 },
   { 16, 62, 17, H_UpgradeFromV16 },
   { 15, 62, 16, H_UpgradeFromV15 },
   { 14, 62, 15, H_UpgradeFromV14 },
   { 13, 62, 14, H_UpgradeFromV13 },
   { 12, 62, 13, H_UpgradeFromV12 },
   { 11, 62, 12, H_UpgradeFromV11 },
   { 10, 62, 11, H_UpgradeFromV10 },
   { 9,  62, 10, H_UpgradeFromV9  },
   { 8,  62, 9,  H_UpgradeFromV8  },
   { 7,  62, 8,  H_UpgradeFromV7  },
   { 6,  62, 7,  H_UpgradeFromV6  },
   { 5,  62, 6,  H_UpgradeFromV5  },
   { 4,  62, 5,  H_UpgradeFromV4  },
   { 3,  62, 4,  H_UpgradeFromV3  },
   { 2,  62, 3,  H_UpgradeFromV2  },
   { 1,  62, 2,  H_UpgradeFromV1  },
   { 0,  62, 1,  H_UpgradeFromV0  },
   { 0,  0,  0,  nullptr }
};

/**
 * Upgrade database to new version
 */
bool MajorSchemaUpgrade_V62()
{
   int32_t major, minor;
   if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
      return false;

   while ((major == 62) && (minor < DB_SCHEMA_VERSION_V62_MINOR))
   {
      // Find upgrade procedure
      int i;
      for (i = 0; s_dbUpgradeMap[i].upgradeProc != nullptr; i++)
         if (s_dbUpgradeMap[i].version == minor)
            break;
      if (s_dbUpgradeMap[i].upgradeProc == nullptr)
      {
         WriteToTerminalEx(L"Unable to find upgrade procedure for version 62.%d\n", minor);
         return false;
      }
      WriteToTerminalEx(L"Upgrading from version 62.%d to %d.%d\n", minor, s_dbUpgradeMap[i].nextMajor, s_dbUpgradeMap[i].nextMinor);
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
