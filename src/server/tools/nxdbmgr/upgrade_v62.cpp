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

   DB_RESULT hResult = SQLSelect(L"SELECT tool_id,tool_name,icon FROM object_tools WHERE icon IS NOT NULL AND icon<>''");
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
               WriteToTerminalEx(L"Cannot read image file \"%s\"\n", path);
               if (g_ignoreErrors)
                  continue;
               success = false;
               break;
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
