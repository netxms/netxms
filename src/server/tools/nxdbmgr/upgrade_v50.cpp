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
** File: upgrade_v50.cpp
**
**/

#include "nxdbmgr.h"
#include <nxevent.h>
#include <pugixml.h>
#include <nxsl.h>

/**
 * Upgrade from 50.49 to 51.0
 */
static bool H_UpgradeFromV49()
{
   CHK_EXEC(SetMajorSchemaVersion(51, 0));
   return true;
}

/**
 * Upgrade from 50.48 to 50.49
 */
static bool H_UpgradeFromV48()
{
   if (GetSchemaLevelForMajorVersion(46) < 3)
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

      CHK_EXEC(SetSchemaLevelForMajorVersion(46, 3));
   }
   CHK_EXEC(SetMinorSchemaVersion(49));
   return true;
}

/**
 * Upgrade from 50.47 to 50.48
 */
static bool H_UpgradeFromV47()
{
   CHK_EXEC(CreateConfigParam(_T("Client.MinVersion"),
                              _T(""),
                              _T("Minimal client version allowed for connection to this server"),
                              nullptr, 'S', true, false, false, false));
   CHK_EXEC(SetMinorSchemaVersion(48));
   return true;
}

/**
 * Upgrade from 50.46 to 50.47
 */
static bool H_UpgradeFromV46()
{
   CHK_EXEC(CreateConfigParam(_T("ReportingServer.JDBC.Properties"),
                              _T(""),
                              _T("Additional properties for JDBC connector on reporting server."),
                              nullptr, 'S', true, false, false, false));
   CHK_EXEC(SetMinorSchemaVersion(47));
   return true;
}

/**
 * Upgrade from 50.45 to 50.46
 */
static bool H_UpgradeFromV45()
{
   CHK_EXEC(CreateConfigParam(_T("Client.InactivityTimeout"),
                              _T("0"),
                              _T("User inactivity timeout in seconds. Client will disconnect if no user activity detected within given time interval. Value of 0 disables inactivity timeout."),
                              _T("seconds"), 'I', true, false, false, false));
   CHK_EXEC(SetMinorSchemaVersion(46));
   return true;
}

/**
 * Upgrade from 50.44 to 50.45
 */
static bool H_UpgradeFromV44()
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

   CHK_EXEC(SetMinorSchemaVersion(45));
   return true;
}

/**
 * Upgrade from 50.43 to 50.44
 */
static bool H_UpgradeFromV43()
{
   if (GetSchemaLevelForMajorVersion(46) < 2)
   {
      CHK_EXEC(CreateConfigParam(_T("AgentTunnels.BindByIPAddress"),
                                 _T("0"),
                                 _T("nable/disable agent tunnel binding by IP address. If enabled and agent certificate is not provided, tunnel will be bound to node with IP address matching tunnel source IP address."),
                                 nullptr, 'B', true, false, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(46, 2));
   }
   CHK_EXEC(SetMinorSchemaVersion(44));
   return true;
}

/**
 * Upgrade from 50.42 to 50.43
 */
static bool H_UpgradeFromV42()
{
   CHK_EXEC(CreateConfigParam(_T("Objects.Nodes.ReadWinPerfCountersOnDemand"), _T("1"), _T("If set to true, list of supported Windows performance counters will be read only when requested by client, otherwise at each configuration poll."), nullptr, 'B', true, false, false, false));
   CHK_EXEC(SetMinorSchemaVersion(43));
   return true;
}

/**
 * Upgrade from 50.41 to 50.42
 */
static bool H_UpgradeFromV41()
{
   CHK_EXEC(SQLQuery( _T("UPDATE config SET data_type='I' WHERE var_name='Objects.NetworkMaps.UpdateInterval'")));
   CHK_EXEC(SetMinorSchemaVersion(42));
   return true;
}

/**
 * Upgrade from 50.40 to 50.41
 */
static bool H_UpgradeFromV40()
{
   static const TCHAR *batch =
      _T("ALTER TABLE icmp_statistics ADD packet_loss integer\n")
      _T("UPDATE icmp_statistics SET packet_loss=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("icmp_statistics"), _T("packet_loss")));
   CHK_EXEC(SetMinorSchemaVersion(41));
   return true;
}

/**
 * Upgrade from 50.39 to 50.40
 */
static bool H_UpgradeFromV39()
{
   //Anomaly detected flag was incorrectly read form database and thus incorrectly written back
   CHK_EXEC(SQLQuery( _T("UPDATE raw_dci_values SET anomaly_detected='0'")));
   CHK_EXEC(SetMinorSchemaVersion(40));
   return true;
}

/**
 * Upgrade from 50.38 to 50.39
 */
static bool H_UpgradeFromV38()
{
   CHK_EXEC(CreateTable(
         _T("CREATE TABLE active_downtimes (")
         _T("   object_id integer not null,")
         _T("   downtime_tag varchar(15) not null,")
         _T("   start_time integer not null,")
         _T("   PRIMARY KEY(object_id,downtime_tag))")));

   CHK_EXEC(CreateTable(
         _T("CREATE TABLE downtime_log (")
         _T("   object_id integer not null,")
         _T("   start_time integer not null,")
         _T("   end_time integer not null,")
         _T("   downtime_tag varchar(15) not null,")
         _T("   PRIMARY KEY(object_id,start_time,downtime_tag))")));

   CHK_EXEC(SQLQuery(_T("ALTER TABLE event_policy ADD downtime_tag varchar(15)")));

   CHK_EXEC(CreateConfigParam(_T("DowntimeLog.RetentionTime"), _T("90"), _T("Retention time in days for the records in downtime log. All records older than specified will be deleted by housekeeping process."), _T("days"), 'I', true, false, false, false));

   CHK_EXEC(SetMinorSchemaVersion(39));
   return true;
}

/**
 * Upgrade from 50.37 to 50.38
 */
static bool H_UpgradeFromV37()
{
   static const TCHAR *batch =
      _T("ALTER TABLE thresholds ADD is_disabled char(1)\n")
      _T("UPDATE thresholds SET is_disabled='0'\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("thresholds"), _T("is_disabled")));
   CHK_EXEC(SetMinorSchemaVersion(38));
   return true;
}

/**
 * Upgrade from 50.36 to 50.37
 */
static bool H_UpgradeFromV36()
{
   CHK_EXEC(SQLQuery(_T("DELETE FROM config WHERE var_name IN ('JobRetryCount', 'MaxActiveUploadJobs')")));
   CHK_EXEC(CreateConfigParam(_T("ThreadPool.FileTransfer.BaseSize"), _T("2"), _T("Base size for file transfer thread pool."), nullptr, 'I', true, true, false, false));
   CHK_EXEC(CreateConfigParam(_T("ThreadPool.FileTransfer.MaxSize"), _T("16"), _T("Maximum size for file transfer thread pool."), nullptr, 'I', true, true, false, false));
   CHK_EXEC(SetMinorSchemaVersion(37));
   return true;
}

/**
 * Upgrade from 50.35 to 50.36
 */
static bool H_UpgradeFromV35()
{
   if (g_dbSyntax == DB_SYNTAX_MSSQL)
   {
      DB_RESULT hResult = SQLSelect(_T("SELECT table_name,column_name,is_nullable FROM information_schema.columns WHERE data_type='text'"));
      if (hResult != nullptr)
      {
         int count = DBGetNumRows(hResult);
         for (int i = 0; i < count; i++)
         {
            TCHAR table[128], column[128], nullable[16];
            DBGetField(hResult, i, 0, table, 128);
            DBGetField(hResult, i, 1, column, 128);
            DBGetField(hResult, i, 2, nullable, 16);
            WriteToTerminalEx(_T("Converting column \x1b[36;1m%s.%s\x1b[0m\n"), table, column);

            StringBuffer query(_T("ALTER TABLE "));
            query.append(table);
            query.append(_T(" ALTER COLUMN "));
            query.append(column);
            query.append(_T(" varchar(max) "));
            query.append(!_tcsicmp(nullable, _T("YES")) ? _T("null") : _T("not null"));
            CHK_EXEC(SQLQuery(query));
         }
         DBFreeResult(hResult);
      }
      else
      {
         if (!g_ignoreErrors)
            return false;
      }
   }
   CHK_EXEC(SetMinorSchemaVersion(36));
   return true;
}

/**
 * Upgrade from 50.34 to 50.35
 */
static bool H_UpgradeFromV34()
{
   static const TCHAR *batch =
      _T("ALTER TABLE raw_dci_values ADD anomaly_detected char(1)\n")
      _T("UPDATE raw_dci_values SET anomaly_detected='0'\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("raw_dci_values"), _T("anomaly_detected")));
   CHK_EXEC(SetMinorSchemaVersion(35));
   return true;
}

/**
 * Check if table has event id and update it to new if in use
 */
static bool CheckEventUsage(const TCHAR *tableName, const TCHAR *eventColumn, uint32_t currentId, uint32_t *newId)
{
   TCHAR query[1024];
   _sntprintf(query, 1024,_T("SELECT %s FROM %s WHERE %s=%d"), eventColumn, tableName, eventColumn, currentId);

   DB_RESULT result = SQLSelect(query);
   if (result == nullptr)
      return false;

   int count = DBGetNumRows(result);
   DBFreeResult(result);
   if (count > 0)
   {
      if (*newId == 0)
      {
         result = SQLSelect(_T("SELECT max(event_code) FROM event_cfg"));
         if (result == nullptr)
            return false;

         *newId = DBGetFieldLong(result, 0, 0) + 1;
         DBFreeResult(result);

         _sntprintf(query, 1024,_T("UPDATE event_cfg SET event_code=%d WHERE event_code=%d"), *newId, currentId);
         if (!SQLQuery(query))
            return false;
      }
      _sntprintf(query, 1024,_T("UPDATE %s SET %s=%d WHERE %s=%d"), tableName, eventColumn, *newId, eventColumn, currentId);
      if (!SQLQuery(query))
         return false;
   }

   return true;
}

/**
 * Upgrade from 50.33 to 50.34
 */
static bool H_UpgradeFromV33()
{
   DB_RESULT result = SQLSelect(_T("SELECT event_code FROM event_cfg WHERE event_code>=4000 AND event_code<=4011"));
   if (result != nullptr)
   {
      int count = DBGetNumRows(result);
      for (int i = 0; i < count; i++)
      {
         uint32_t newId = 0;
         uint32_t currentId = DBGetFieldUInt32(result, i, 0);

         CHK_EXEC(CheckEventUsage(_T("thresholds"), _T("event_code"), currentId, &newId));
         CHK_EXEC(CheckEventUsage(_T("thresholds"), _T("rearm_event_code"), currentId, &newId));
         CHK_EXEC(CheckEventUsage(_T("dct_thresholds"), _T("activation_event"), currentId, &newId));
         CHK_EXEC(CheckEventUsage(_T("dct_thresholds"), _T("deactivation_event"), currentId, &newId));
         CHK_EXEC(CheckEventUsage(_T("snmp_trap_cfg"), _T("event_code"), currentId, &newId));
         CHK_EXEC(CheckEventUsage(_T("snmp_trap_cfg"), _T("event_code"), currentId, &newId));

         if (newId == 0)
         {
            TCHAR query[1024];
            _sntprintf(query, 1024, _T("DELETE FROM policy_event_list WHERE event_code=%d"), currentId);
            CHK_EXEC(SQLQuery(query));
            _sntprintf(query, 1024, _T("DELETE FROM event_cfg WHERE event_code=%d"), currentId);
            CHK_EXEC(SQLQuery(query));
         }
         else
         {
            CHK_EXEC(CheckEventUsage(_T("policy_event_list"), _T("event_code"), currentId, &newId));
         }
      }
      DBFreeResult(result);
   }
   else
   {
      if (!g_ignoreErrors)
         return false;
   }
   CHK_EXEC(SetMinorSchemaVersion(34));
   return true;
}

/**
 * Convert NXSL script to version 5
 */
static bool ConvertNXSLScriptsToV5(const TCHAR *tableName, const TCHAR *idColumn, const TCHAR *sourceColumn, bool textKey = false)
{
   TCHAR query[1024];
   _sntprintf(query, 1024,_T("SELECT %s,%s FROM %s"), idColumn, sourceColumn, tableName);

   DB_RESULT result = SQLSelect(query);
   if (result == nullptr)
      return false;

   int count = DBGetNumRows(result);
   if (count == 0)
   {
      DBFreeResult(result);
      return true;
   }

   bool success;

   _sntprintf(query, 1024,_T("UPDATE %s SET %s=? WHERE %s=?"), tableName, sourceColumn, idColumn);
   DB_STATEMENT stmt = DBPrepare(g_dbHandle, query, count > 1);
   if (stmt != nullptr)
   {
      success = true;
      for (int i = 0; (i < count) && success; i++)
      {
         TCHAR *source = DBGetField(result, i, 1, nullptr, 0);
         if ((source == nullptr) || (*source == 0))
         {
            // Script is empty, nothing to convert
            MemFree(source);
            continue;
         }

         StringBuffer updatedSource = NXSLConvertToV5(source);
         MemFree(source);

         DBBind(stmt, 1, DB_SQLTYPE_TEXT, updatedSource, DB_BIND_STATIC);
         if (textKey)
            DBBind(stmt, 2, DB_SQLTYPE_VARCHAR, DBGetFieldAsString(result, i, 0), DB_BIND_TRANSIENT);
         else
            DBBind(stmt, 2, DB_SQLTYPE_INTEGER, DBGetFieldULong(result, i, 0));

         int64_t s = CreateSavePoint();
         success = SQLExecute(stmt);
         if (!success)
         {
            RollbackToSavePoint(s);
            if (g_ignoreErrors)
               success = true;
         }
         ReleaseSavePoint(s);
      }
      DBFreeStatement(stmt);
   }
   else
   {
      success = false;
   }
   DBFreeResult(result);
   return success;
}

/**
 * Write XML to string
 */
class xml_string_writer : public pugi::xml_writer
{
public:
   StringBuffer result;

   virtual void write(const void* data, size_t size) override
   {
      result.appendUtf8String(static_cast<const char*>(data), size);
   }
};

/**
 * Upgrade from 50.32 to 50.33
 */
static bool H_UpgradeFromV32()
{
   CHK_EXEC(ConvertNXSLScriptsToV5(_T("dci_summary_tables"), _T("id"), _T("node_filter")));
   CHK_EXEC(ConvertNXSLScriptsToV5(_T("conditions"), _T("id"), _T("script")));
   CHK_EXEC(ConvertNXSLScriptsToV5(_T("items"), _T("item_id"), _T("transformation")));
   CHK_EXEC(ConvertNXSLScriptsToV5(_T("items"), _T("item_id"), _T("instd_filter")));
   CHK_EXEC(ConvertNXSLScriptsToV5(_T("thresholds"), _T("threshold_id"), _T("script")));
   CHK_EXEC(ConvertNXSLScriptsToV5(_T("dc_tables"), _T("item_id"), _T("transformation_script")));
   CHK_EXEC(ConvertNXSLScriptsToV5(_T("dc_tables"), _T("item_id"), _T("instd_filter")));
   CHK_EXEC(ConvertNXSLScriptsToV5(_T("event_policy"), _T("rule_id"), _T("filter_script")));
   CHK_EXEC(ConvertNXSLScriptsToV5(_T("event_policy"), _T("rule_id"), _T("action_script")));
   CHK_EXEC(ConvertNXSLScriptsToV5(_T("snmp_trap_cfg"), _T("trap_id"), _T("transformation_script")));
   CHK_EXEC(ConvertNXSLScriptsToV5(_T("script_library"), _T("script_id"), _T("script_code")));
   CHK_EXEC(ConvertNXSLScriptsToV5(_T("network_maps"), _T("id"), _T("link_styling_script")));
   CHK_EXEC(ConvertNXSLScriptsToV5(_T("network_maps"), _T("id"), _T("filter")));
   CHK_EXEC(ConvertNXSLScriptsToV5(_T("object_queries"), _T("id"), _T("script")));
   CHK_EXEC(ConvertNXSLScriptsToV5(_T("auto_bind_target"), _T("object_id"), _T("bind_filter_1")));
   CHK_EXEC(ConvertNXSLScriptsToV5(_T("auto_bind_target"), _T("object_id"), _T("bind_filter_2")));
   CHK_EXEC(ConvertNXSLScriptsToV5(_T("business_service_prototypes"), _T("id"), _T("instance_filter")));
   CHK_EXEC(ConvertNXSLScriptsToV5(_T("business_service_checks"), _T("id"), _T("content")));
   CHK_EXEC(ConvertNXSLScriptsToV5(_T("agent_configs"), _T("config_id"), _T("config_filter")));
   CHK_EXEC(ConvertNXSLScriptsToV5(_T("am_attributes"), _T("attr_name"), _T("autofill_script"), true));

   // Dashboard scripted charts (bar and pie), dashboard status indicator
   DB_RESULT result = SQLSelect(_T("SELECT dashboard_id,element_id,element_data FROM dashboard_elements WHERE element_type=30 OR element_type=31 OR element_type=6"));
   if (result != nullptr)
   {
      int count = DBGetNumRows(result);
      DB_STATEMENT stmt = DBPrepare(g_dbHandle, _T("UPDATE dashboard_elements SET element_data=? WHERE dashboard_id=? AND element_id=?"), count > 1);
      if (stmt != nullptr)
      {
         for (int i = 0; i < count; i++)
         {
            char *text = DBGetFieldUTF8(result, i, 2, nullptr, 0);
            pugi::xml_document xml;
            if (!xml.load_buffer(text, strlen(text)))
            {
               _tprintf(_T("Failed to load XML. Ignore dashboard %d element %d\n"), DBGetFieldULong(result, i, 0), DBGetFieldULong(result, i, 1));
            }

            pugi::xml_node node = xml.select_node("/element/script").node();
            const char *source = node.text().as_string();
            wchar_t *tmp = WideStringFromUTF8String(source);
            StringBuffer updatedScript = NXSLConvertToV5(tmp);
            MemFree(tmp);

            char *newScript = updatedScript.getUTF8String();
            node.last_child().set_value(newScript);
            MemFree(newScript);

            xml_string_writer writer;
            xml.print(writer);

            DBBind(stmt, 1, DB_SQLTYPE_TEXT, writer.result, DB_BIND_STATIC);
            DBBind(stmt, 2, DB_SQLTYPE_INTEGER, DBGetFieldULong(result, i, 0));
            DBBind(stmt, 3, DB_SQLTYPE_INTEGER, DBGetFieldULong(result, i, 1));

            MemFree(text);
            if (!SQLExecute(stmt) && !g_ignoreErrors)
            {
               DBFreeResult(result);
               DBFreeStatement(stmt);
               return false;
            }
         }
      }
      else if (!g_ignoreErrors)
      {
         DBFreeResult(result);
         return false;
      }
      DBFreeResult(result);
      DBFreeStatement(stmt);
   }
   else if (!g_ignoreErrors)
   {
      return false;
   }

   // Dashboard object query
   result = SQLSelect(_T("SELECT dashboard_id,element_id,element_data FROM dashboard_elements WHERE element_type=28"));
   if (result != nullptr)
   {
      int count = DBGetNumRows(result);
      DB_STATEMENT stmt = DBPrepare(g_dbHandle, _T("UPDATE dashboard_elements SET element_data=? WHERE dashboard_id=? AND element_id=?"), count > 1);
      if (stmt != nullptr)
      {
         for (int i = 0; i < count; i++)
         {
            char *text = DBGetFieldUTF8(result, i, 2, nullptr, 0);
            pugi::xml_document xml;
            if (!xml.load_buffer(text, strlen(text)))
            {
               _tprintf(_T("Failed to load XML. Ignore dashboard %d element %d\n"), DBGetFieldULong(result, i, 0), DBGetFieldULong(result, i, 1));
            }

            pugi::xml_node node = xml.select_node("/element/query").node();
            const char *source = node.text().as_string();
            wchar_t *tmp = WideStringFromUTF8String(source);
            StringBuffer updatedScript = NXSLConvertToV5(tmp);
            MemFree(tmp);

            char *newScript = updatedScript.getUTF8String();
            node.last_child().set_value(newScript);
            MemFree(newScript);

            xml_string_writer writer;
            xml.print(writer);

            DBBind(stmt, 1, DB_SQLTYPE_TEXT, writer.result, DB_BIND_STATIC);
            DBBind(stmt, 2, DB_SQLTYPE_INTEGER, DBGetFieldULong(result, i, 0));
            DBBind(stmt, 3, DB_SQLTYPE_INTEGER, DBGetFieldULong(result, i, 1));

            MemFree(text);
            if (!SQLExecute(stmt) && !g_ignoreErrors)
            {
               DBFreeResult(result);
               DBFreeStatement(stmt);
               return false;
            }
         }
      }
      else if (!g_ignoreErrors)
      {
         DBFreeResult(result);
         return false;
      }
      DBFreeResult(result);
      DBFreeStatement(stmt);
   }
   else if (!g_ignoreErrors)
   {
      return false;
   }

   CHK_EXEC(SetMinorSchemaVersion(33));
   return true;
}

/**
 * Upgrade from 50.31 to 50.32
 */
static bool H_UpgradeFromV31()
{
   CHK_EXEC(SQLQuery(_T("DELETE FROM config WHERE var_name='NetworkDiscovery.EnableParallelProcessing'")));
   CHK_EXEC(SetMinorSchemaVersion(32));
   return true;
}

/**
 * Upgrade from 50.30 to 50.31
 */
static bool H_UpgradeFromV30()
{
   CHK_EXEC(SQLQuery(_T("DELETE FROM config WHERE var_name='Objects.Security.ReadAccessViaMap'")));
   CHK_EXEC(SetMinorSchemaVersion(31));
   return true;
}

/**
 * Upgrade from 50.29 to 50.30
 */
static bool H_UpgradeFromV29()
{
   if (GetSchemaLevelForMajorVersion(46) < 1)
   {
      static const TCHAR *batch =
         _T("ALTER TABLE users ADD ui_access_rules varchar(2000)\n")
         _T("ALTER TABLE user_groups ADD ui_access_rules varchar(2000)\n")
         _T("UPDATE user_groups SET ui_access_rules='*' WHERE id=1073741824\n")
         _T("<END>");
      CHK_EXEC(SQLBatch(batch));
      CHK_EXEC(SetSchemaLevelForMajorVersion(46, 1));
   }
   CHK_EXEC(SetMinorSchemaVersion(30));
   return true;
}

/**
 * Upgrade from 50.28 to 50.29
 */
static bool H_UpgradeFromV28()
{
   CHK_EXEC(SQLQuery(_T("UPDATE network_maps SET radius=0 WHERE (radius < 0) OR (radius >= 255)")));
   CHK_EXEC(SetMinorSchemaVersion(29));
   return true;
}

/**
 * Upgrade from 50.27 to 50.28
 */
static bool H_UpgradeFromV27()
{
   CHK_EXEC(CreateConfigParam(_T("Jira.VerifyPeer"),
         _T("1"),
         _T("Enable/disable peer certificate verification."),
         nullptr, 'B', true, false, false, false));
   CHK_EXEC(CreateConfigParam(_T("Redmine.VerifyPeer"),
         _T("1"),
         _T("Enable/disable  peer certificate verification"),
         nullptr, 'B', true, false, false, false));

   static const TCHAR batch[] =
      _T("UPDATE config SET var_name='Redmine.ServerURL' WHERE var_name='RedmineServerURL'\n")
      _T("UPDATE config SET var_name='Redmine.ApiKey' WHERE var_name='RedmineApiKey'\n")
      _T("UPDATE config SET var_name='Redmine.ProjectId' WHERE var_name='RedmineProjectId'\n")
      _T("UPDATE config SET var_name='Redmine.TrackerId' WHERE var_name='RedmineTrackerId'\n")
      _T("UPDATE config SET var_name='Redmine.StatusId' WHERE var_name='RedmineStatusId'\n")
      _T("UPDATE config SET var_name='Redmine.PriorityId' WHERE var_name='RedminePriorityId'\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   CHK_EXEC(SetMinorSchemaVersion(28));
   return true;
}

/**
 * Upgrade from 50.26 to 50.27
 */
static bool H_UpgradeFromV26()
{
   static const TCHAR *batch =
      _T("ALTER TABLE access_points ADD peer_node_id integer\n")
      _T("ALTER TABLE access_points ADD peer_if_id integer\n")
      _T("ALTER TABLE access_points ADD peer_proto integer\n")
      _T("UPDATE access_points SET peer_node_id=0,peer_if_id=0,peer_proto=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("access_points"), _T("peer_node_id")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("access_points"), _T("peer_if_id")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("access_points"), _T("peer_proto")));
   CHK_EXEC(SetMinorSchemaVersion(27));
   return true;
}

/**
 * Upgrade from 50.25 to 50.26
 */
static bool H_UpgradeFromV25()
{
   CHK_EXEC(CreateConfigParam(_T("SNMP.Traps.UnmatchedTrapEvent"),
         _T("1"),
         _T("Enable/disable generation of default event for unmatched SNMP traps."),
         nullptr, 'B', true, false, false, false));
   CHK_EXEC(SetMinorSchemaVersion(26));
   return true;
}

/**
 * Upgrade from 50.24 to 50.25
 */
static bool H_UpgradeFromV24()
{
   CHK_EXEC(CreateConfigParam(_T("Objects.Collectors.ContainerAutoBind"),
         _T("0"),
         _T("Enable/disable container auto binding for collectors."),
         nullptr, 'B', true, false, false, false));
   CHK_EXEC(CreateConfigParam(_T("Objects.Collectors.TemplateAutoApply"),
         _T("0"),
         _T("Enable/disable template auto apply for collectors."),
         nullptr, 'B', true, false, false, false));
   CHK_EXEC(SetMinorSchemaVersion(25));
   return true;
}

/**
 * Upgrade from 50.23 to 50.24
 */
static bool H_UpgradeFromV23()
{
   static const TCHAR *batch =
      _T("ALTER TABLE radios ADD frequency integer\n")
      _T("ALTER TABLE radios ADD band integer\n")
      _T("UPDATE radios SET frequency=0,band=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("radios"), _T("frequency")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("radios"), _T("band")));
   CHK_EXEC(SetMinorSchemaVersion(24));
   return true;
}

/**
 * Upgrade from 50.22 to 50.23
 */
static bool H_UpgradeFromV22()
{
   CHK_EXEC(DBDropPrimaryKey(g_dbHandle, _T("radios")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("radios"), _T("bssid")));
   CHK_EXEC(DBAddPrimaryKey(g_dbHandle, _T("radios"), _T("owner_id,radio_index,bssid")));
   CHK_EXEC(SetMinorSchemaVersion(23));
   return true;
}

/**
 * Upgrade from 50.21 to 50.22
 */
static bool H_UpgradeFromV21()
{
   CHK_EXEC(SQLQuery(_T("DELETE FROM event_cfg WHERE event_code IN (72, 73, 74)")));

   CHK_EXEC(CreateEventTemplate(EVENT_AP_UP, _T("SYS_AP_UP"),
         EVENT_SEVERITY_NORMAL, EF_LOG, _T("5aaee261-0c5d-44e0-b2f0-223bbba5297d"),
         _T("Access point state changed to UP"),
         _T("Generated when access point state changes to UP.\r\n")
         _T("Parameters:\r\n")
         _T("   1) domainId - Access point domain ID\r\n")
         _T("   2) controllerId - Access point controller ID\r\n")
         _T("   3) macAddr - Access point MAC address\r\n")
         _T("   4) ipAddr - Access point IP address\r\n")
         _T("   5) vendor - Access point vendor name\r\n")
         _T("   6) model - Access point model\r\n")
         _T("   7) serialNumber - Access point serial number\r\n")
         _T("   8) state - Access point state")
      ));

   CHK_EXEC(CreateEventTemplate(EVENT_AP_UNPROVISIONED, _T("SYS_AP_UNPROVISIONED"),
         EVENT_SEVERITY_MAJOR, EF_LOG, _T("846a3581-aad1-4e17-9c55-9bd2e6b1247b"),
         _T("Access point state changed to UNPROVISIONED"),
         _T("Generated when access point state changes to UNPROVISIONED.\r\n")
         _T("Parameters:\r\n")
         _T("   1) domainId - Access point domain ID\r\n")
         _T("   2) controllerId - Access point controller ID\r\n")
         _T("   3) macAddr - Access point MAC address\r\n")
         _T("   4) ipAddr - Access point IP address\r\n")
         _T("   5) vendor - Access point vendor name\r\n")
         _T("   6) model - Access point model\r\n")
         _T("   7) serialNumber - Access point serial number\r\n")
         _T("   8) state - Access point state")
      ));

   CHK_EXEC(CreateEventTemplate(EVENT_AP_DOWN, _T("SYS_AP_DOWN"),
         EVENT_SEVERITY_CRITICAL, EF_LOG, _T("2c8c6208-d3ab-4b8c-926a-872f4d8abcee"),
         _T("Access point state changed to DOWN"),
         _T("Generated when access point state changes to DOWN.\r\n")
         _T("Parameters:\r\n")
         _T("   1) domainId - Access point domain ID\r\n")
         _T("   2) controllerId - Access point controller ID\r\n")
         _T("   3) macAddr - Access point MAC address\r\n")
         _T("   4) ipAddr - Access point IP address\r\n")
         _T("   5) vendor - Access point vendor name\r\n")
         _T("   6) model - Access point model\r\n")
         _T("   7) serialNumber - Access point serial number\r\n")
         _T("   8) state - Access point state")
      ));

   CHK_EXEC(CreateEventTemplate(EVENT_AP_UNKNOWN, _T("SYS_AP_UNKNOWN"),
         EVENT_SEVERITY_NORMAL, EF_LOG, _T("dcbdd08c-d5ff-48b8-90c2-4990eb974c95"),
         _T("Access point state changed to UNKNOWN"),
         _T("Generated when access point state changes to UNKNOWN.\r\n")
         _T("Parameters:\r\n")
         _T("   1) domainId - Access point domain ID\r\n")
         _T("   2) controllerId - Access point controller ID\r\n")
         _T("   3) macAddr - Access point MAC address\r\n")
         _T("   4) ipAddr - Access point IP address\r\n")
         _T("   5) vendor - Access point vendor name\r\n")
         _T("   6) model - Access point model\r\n")
         _T("   7) serialNumber - Access point serial number\r\n")
         _T("   8) state - Access point state")
      ));

   CHK_EXEC(SetMinorSchemaVersion(22));
   return true;
}

/**
 * Upgrade from 50.20 to 50.21
 */
static bool H_UpgradeFromV20()
{
   CHK_EXEC(DBRenameColumn(g_dbHandle, _T("access_points"), _T("node_id"), _T("controller_id")));

   static const TCHAR *batch =
      _T("ALTER TABLE access_points ADD domain_id integer\n")
      _T("ALTER TABLE access_points ADD grace_period_start integer\n")
      _T("UPDATE access_points SET domain_id=0,grace_period_start=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("access_points"), _T("domain_id")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("access_points"), _T("grace_period_start")));

   CHK_EXEC(CreateTable(
         _T("CREATE TABLE radios (")
         _T("   owner_id integer not null,")
         _T("   radio_index integer not null,")
         _T("   if_index integer not null,")
         _T("   name varchar(63) null,")
         _T("   bssid char(12) null,")
         _T("   ssid varchar(32) null,")
         _T("   channel integer not null,")
         _T("   power_dbm integer not null,")
         _T("   power_mw integer not null,")
         _T("   PRIMARY KEY(owner_id,radio_index))")));

   CHK_EXEC(CreateConfigParam(_T("Objects.AccessPoints.RetentionTime"),
         _T("72"),
         _T("Retention time for disappeared access points."),
         _T("hours"), 'I', true, false, false, false));

   CHK_EXEC(SetMinorSchemaVersion(21));
   return true;
}

/**
 * Upgrade from 50.19 to 50.20
 */
static bool H_UpgradeFromV19()
{
   CHK_EXEC(CreateConfigParam(_T("Client.BaseURL"),
         _T("https://{server-name}"),
         _T("Base URL for forming direct access URLs. Macro {server-name} can be used to insert name of the server where user is currently logged in."),
         nullptr, 'S', true, false, false, false));
   CHK_EXEC(SetMinorSchemaVersion(20));
   return true;
}

/**
 * Upgrade from 50.18 to 50.19
 */
static bool H_UpgradeFromV18()
{
   CHK_EXEC(CreateTable(
         _T("CREATE TABLE auth_tokens (")
         _T("   id integer not null,")
         _T("   user_id integer not null,")
         _T("   issuing_time integer not null,")
         _T("   expiration_time integer not null,")
         _T("   description varchar(127) null,")
         _T("   token_data char(64) not null,")
         _T("   PRIMARY KEY(id))")));

   DB_RESULT hResult = DBSelect(g_dbHandle, _T("SELECT max(id) FROM users"));
   if (hResult != nullptr)
   {
      TCHAR query[1024];
      _sntprintf(query, 1024,
         _T("INSERT INTO users ")
         _T("(id,name,password,system_access,flags,full_name,description,grace_logins,auth_method,guid,cert_mapping_method,")
         _T("cert_mapping_data,auth_failures,last_passwd_change,min_passwd_length,disabled_until,last_login,created) ")
         _T("VALUES (%d,'anonymous','$$',0,1104,'','Anonymous account',5,0,'%s',0,'',0,0,0,0,0,0)"),
         DBGetFieldLong(hResult, 0, 0) + 1, uuid::generate().toString().cstr());
      DBFreeResult(hResult);
      CHK_EXEC(SQLQuery(query));
   }
   else if (!g_ignoreErrors)
   {
      return false;
   }

   CHK_EXEC(SetMinorSchemaVersion(19));
   return true;
}

/**
 * Upgrade from 50.17 to 50.18
 */
static bool H_UpgradeFromV17()
{
   CHK_EXEC(CreateConfigParam(_T("Objects.Security.ReadAccessViaMap"),
         _T("0"),
         _T("If enabled, user can get limited read only access to objects that are not normally accessible but referenced on network map that is accessible by the user."),
         nullptr, 'B', true, false, false, false));
   CHK_EXEC(SetMinorSchemaVersion(18));
   return true;
}

/**
 * Upgrade from 50.16 to 50.17
 */
static bool H_UpgradeFromV16()
{
   if (GetSchemaLevelForMajorVersion(45) < 4)
   {
      CHK_EXEC(CreateConfigParam(_T("Client.FirstPacketTimeout"),
            _T("2000"),
            _T("Timeout for receiving first packet from client after establishing TCP connection."),
            _T("milliseconds"), 'I', true, false, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(45, 4));
   }
   CHK_EXEC(SetMinorSchemaVersion(17));
   return true;
}

/**
 * Upgrade from 50.15 to 50.16
 */
static bool H_UpgradeFromV15()
{
   if (GetSchemaLevelForMajorVersion(45) < 3)
   {
      CHK_EXEC(DBDropPrimaryKey(g_dbHandle, _T("policy_action_list")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(45, 3));
   }
   CHK_EXEC(SetMinorSchemaVersion(16));
   return true;
}

/**
 * Upgrade from 50.14 to 50.15
 */
static bool H_UpgradeFromV14()
{
   static const TCHAR *batch =
      _T("ALTER TABLE network_map_links ADD interface1 integer\n")
      _T("ALTER TABLE network_map_links ADD interface2 integer\n")
      _T("UPDATE network_map_links SET interface1=0, interface2=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("network_map_links"), _T("interface1")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("network_map_links"), _T("interface2")));

   CHK_EXEC(SetMinorSchemaVersion(15));
   return true;
}

/**
 * Upgrade from 50.13 to 50.14
 */
static bool H_UpgradeFromV13()
{
   if (GetSchemaLevelForMajorVersion(45) < 2)
   {
      CHK_EXEC(CreateConfigParam(_T("Objects.Interfaces.IgnoreIfNotPresent"),
            _T("0"),
            _T("If enabled, interfaces in \"NOT PRESENT\" state will be ignored when read from device."),
            nullptr, 'B', true, false, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(45, 2));
   }
   CHK_EXEC(SetMinorSchemaVersion(14));
   return true;
}

/**
 * Upgrade from 50.12 to 50.13
 */
static bool H_UpgradeFromV12()
{
   if (GetSchemaLevelForMajorVersion(45) < 1)
   {
      CHK_EXEC(SQLQuery(_T("UPDATE acl SET access_rights=access_rights+1048576 WHERE user_id=1073741825")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(45, 1));
   }
   CHK_EXEC(SetMinorSchemaVersion(13));
   return true;
}

/**
 * Upgrade from 50.11 to 50.12
 */
static bool H_UpgradeFromV11()
{
   CHK_EXEC(CreateConfigParam(_T("Objects.NetworkMaps.UpdateInterval"),
         _T("60"),
         _T("Interval in seconds between automatic map updates."),
         _T("seconds"), 'B', true, false, false, false));
   CHK_EXEC(SetMinorSchemaVersion(12));
   return true;
}

/**
 * Upgrade from 50.10 to 50.11
 */
static bool H_UpgradeFromV10()
{
   static const TCHAR *batch =
      _T("ALTER TABLE network_maps ADD link_width integer\n")
      _T("ALTER TABLE network_maps ADD link_style integer\n")
      _T("UPDATE network_maps SET link_width=2, link_style=1\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("network_maps"), _T("link_width")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("network_maps"), _T("link_style")));

   CHK_EXEC(SetMinorSchemaVersion(11));
   return true;
}

/**
 * Upgrade from 50.9 to 50.10
 */
static bool H_UpgradeFromV9()
{
   if (GetSchemaLevelForMajorVersion(44) < 28)
   {
      CHK_EXEC(CreateConfigParam(_T("Syslog.ParseUnknownSourceMessages"),
            _T("0"),
            _T("Enable or disable parsing of syslog messages received from unknown sources."),
            nullptr, 'B', true, false, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(44, 28));
   }
   CHK_EXEC(SetMinorSchemaVersion(10));
   return true;
}

/**
 * Upgrade from 50.8 to 50.9
 */
static bool H_UpgradeFromV8()
{
   CHK_EXEC(DBDropColumn(g_dbHandle, _T("sensors"), _T("communication_protocol")));
   CHK_EXEC(DBDropColumn(g_dbHandle, _T("sensors"), _T("xml_reg_config")));
   CHK_EXEC(DBDropColumn(g_dbHandle, _T("sensors"), _T("xml_config")));
   CHK_EXEC(DBDropColumn(g_dbHandle, _T("sensors"), _T("meta_type")));
   CHK_EXEC(DBDropColumn(g_dbHandle, _T("sensors"), _T("description")));
   CHK_EXEC(DBDropColumn(g_dbHandle, _T("sensors"), _T("last_connection_time")));
   CHK_EXEC(DBDropColumn(g_dbHandle, _T("sensors"), _T("frame_count")));
   CHK_EXEC(DBDropColumn(g_dbHandle, _T("sensors"), _T("signal_strenght")));
   CHK_EXEC(DBDropColumn(g_dbHandle, _T("sensors"), _T("signal_noise")));
   CHK_EXEC(DBDropColumn(g_dbHandle, _T("sensors"), _T("frequency")));

   CHK_EXEC(DBResizeColumn(g_dbHandle, _T("sensors"), _T("serial_number"), 63, true));
   CHK_EXEC(DBResizeColumn(g_dbHandle, _T("sensors"), _T("device_address"), 127, true));

   CHK_EXEC(DBRenameColumn(g_dbHandle, _T("sensors"), _T("proxy_node"), _T("gateway_node")));

   static const TCHAR *batch =
      _T("ALTER TABLE sensors ADD model varchar(127)\n")
      _T("ALTER TABLE sensors ADD modbus_unit_id integer\n")
      _T("ALTER TABLE sensors ADD capabilities integer\n")
      _T("UPDATE sensors SET modbus_unit_id=255,capabilities=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("sensors"), _T("modbus_unit_id")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("sensors"), _T("capabilities")));

   CHK_EXEC(SetMinorSchemaVersion(9));
   return true;
}

/**
 * Upgrade from 50.7 to 50.8
 */
static bool H_UpgradeFromV7()
{
   if (GetSchemaLevelForMajorVersion(44) < 27)
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

      CHK_EXEC(SetSchemaLevelForMajorVersion(44, 27));
   }

   CHK_EXEC(SetMinorSchemaVersion(8));
   return true;
}

/**
 * Upgrade from 50.6 to 50.7
 */
static bool H_UpgradeFromV6()
{
   static int defaultWeight = 1300;
   static int defaultHeight = 850;

   CHK_EXEC(CreateConfigParam(_T("Objects.NetworkMaps.DefaultWidth"),
         _T("1300"),
         _T("Default network map width"),
         _T("pixels"),
         'I', true, false, false, false));
   CHK_EXEC(CreateConfigParam(_T("Objects.NetworkMaps.DefaultHeight"),
         _T("850"),
         _T("Default network map height"),
         _T("pixels"),
         'I', true, false, false, false));

   static const TCHAR *batch =
      _T("ALTER TABLE network_maps ADD width integer\n")
      _T("ALTER TABLE network_maps ADD height integer\n")
      _T("UPDATE network_maps SET height=0,width=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("network_maps"), _T("width")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("network_maps"), _T("height")));

   DB_RESULT result = SQLSelect(_T("SELECT element_data,map_id FROM network_map_elements ORDER BY map_id"));
   if (result != nullptr)
   {
      int size = DBGetNumRows(result);
      uint32_t mpaId = size > 0 ? DBGetFieldULong(result, 0, 1) : 0;
      int maxWidth = defaultWeight;
      int maxHeight = defaultHeight;
      TCHAR query[256];
      for (int i = 0; i < size; i++)
      {
         if (mpaId != DBGetFieldLong(result, i, 1))
         {
            // Save map
            if ((maxWidth > defaultWeight) || (maxHeight > defaultHeight))
            {
               _sntprintf(query, 256, _T("UPDATE network_maps SET width=%d,height=%d WHERE id=%u"), maxWidth, maxHeight, mpaId);
               CHK_EXEC(SQLQuery(query));
            }

            //Reset counters
            maxWidth = defaultWeight;
            maxHeight = defaultHeight;
            mpaId = DBGetFieldULong(result, i, 1);
         }

         Config config;
         TCHAR *data = DBGetField(result, i, 0, nullptr, 0);
         if (data != nullptr)
         {
            char *utf8data = UTF8StringFromWideString(data);
            config.loadXmlConfigFromMemory(utf8data, (int)strlen(utf8data), _T("<database>"), "element");
            MemFree(utf8data);
            MemFree(data);
            if (config.getValueAsInt(_T("type"), 1) == 2)
            {
               maxWidth = std::max(config.getValueAsInt(_T("/posX"), 0) + config.getValueAsInt(_T("/width"), 0), maxWidth);
               maxHeight = std::max(config.getValueAsInt(_T("/posY"), 0) + config.getValueAsInt(_T("/height"), 0), maxHeight);
            }
            else
            {
               maxWidth = std::max(config.getValueAsInt(_T("/posX"), 0) + 50, maxWidth);
               maxHeight = std::max(config.getValueAsInt(_T("/posY"), 0) + 50, maxHeight);
            }
         }
      }

      if ((maxWidth > defaultWeight) || (maxHeight > defaultHeight))
      {
         _sntprintf(query, 256, _T("UPDATE network_maps SET width=%d,height=%d WHERE id=%u"), maxWidth, maxHeight, mpaId);
         CHK_EXEC(SQLQuery(query));
      }
      DBFreeResult(result);
   }
   else
   {
      if (!g_ignoreErrors)
         return false;
   }

   CHK_EXEC(SetMinorSchemaVersion(7));
   return true;
}

/**
 * Upgrade from 50.5 to 50.6
 */
static bool H_UpgradeFromV5()
{
   CHK_EXEC_NO_SP(SQLQuery(_T("DROP TABLE dct_column_names")));
   CHK_EXEC(SetMinorSchemaVersion(6));
   return true;
}

/**
 * Upgrade from 50.4 to 50.5
 */
static bool H_UpgradeFromV4()
{
   CHK_EXEC(CreateConfigParam(_T("SNMP.Agent.AllowedVersions"),
         _T("7"),
         _T("A bitmask for SNMP versions allowed by built-in SNMP agent (sum the values to allow multiple versions at once): 1 = version 1, 2 = version 2c, 4 = version 3)."),
         nullptr, 'I', true, false, false, false));
   CHK_EXEC(CreateConfigParam(_T("SNMP.Agent.CommunityString"),
         _T("public"),
         _T("Community string for SNMPv1/v2 requests to built-in SNMP agent."),
         nullptr, 'S', true, false, false, false));
   CHK_EXEC(CreateConfigParam(_T("SNMP.Agent.Enable"),
         _T("0"),
         _T("Enable/disable built-in SNMP agent."),
         nullptr, 'B', true, true, false, false));
   CHK_EXEC(CreateConfigParam(_T("SNMP.Agent.ListenerPort"),
         _T("161"),
         _T("Port used by built-in SNMP agent."),
         nullptr, 'I', true, false, false, false));
   CHK_EXEC(CreateConfigParam(_T("SNMP.Agent.V3.AuthenticationMethod"),
         _T("0"),
         _T("Authentication method for SNMPv3 requests to built-in SNMP agent."),
         nullptr, 'C', true, false, false, false));
   CHK_EXEC(CreateConfigParam(_T("SNMP.Agent.V3.AuthenticationPassword"),
         _T(""),
         _T("Authentication password for SNMPv3 requests to built-in SNMP agent."),
         nullptr, 'S', true, false, false, false));
   CHK_EXEC(CreateConfigParam(_T("SNMP.Agent.V3.EncryptionMethod"),
         _T("0"),
         _T("Encryption method for SNMPv3 requests to built-in SNMP agent."),
         nullptr, 'C', true, false, false, false));
   CHK_EXEC(CreateConfigParam(_T("SNMP.Agent.V3.EncryptionPassword"),
         _T(""),
         _T("Encryption password for SNMPv3 requests to built-in SNMP agent."),
         nullptr, 'S', true, false, false, false));
   CHK_EXEC(CreateConfigParam(_T("SNMP.Agent.V3.UserName"),
         _T("netxms"),
         _T("User name for SNMPv3 requests to built-in SNMP agent."),
         nullptr, 'S', true, false, false, false));

   static const TCHAR *configBatch =
      _T("INSERT INTO config_values (var_name,var_value) VALUES ('SNMP.Agent.ListenerPort','65535')\n")
      _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('SNMP.Agent.V3.AuthenticationMethod','0','None')\n")
      _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('SNMP.Agent.V3.AuthenticationMethod','1','MD5')\n")
      _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('SNMP.Agent.V3.AuthenticationMethod','2','SHA-1')\n")
      _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('SNMP.Agent.V3.AuthenticationMethod','3','SHA-224')\n")
      _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('SNMP.Agent.V3.AuthenticationMethod','4','SHA-256')\n")
      _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('SNMP.Agent.V3.AuthenticationMethod','5','SHA-384')\n")
      _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('SNMP.Agent.V3.AuthenticationMethod','6','SHA-512')\n")
      _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('SNMP.Agent.V3.EncryptionMethod','0','None')\n")
      _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('SNMP.Agent.V3.EncryptionMethod','1','DES')\n")
      _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('SNMP.Agent.V3.EncryptionMethod','2','AES-128')\n")
      _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('SNMP.Agent.V3.EncryptionMethod','3','AES-192')\n")
      _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('SNMP.Agent.V3.EncryptionMethod','4','AES-256')\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(configBatch));

   CHK_EXEC(SetMinorSchemaVersion(5));
   return true;
}

/**
 * Upgrade from 50.3 to 50.4
 */
static bool H_UpgradeFromV3()
{
   if (GetSchemaLevelForMajorVersion(44) < 26)
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
      CHK_EXEC(SetSchemaLevelForMajorVersion(44, 26));
   }

   CHK_EXEC(SetMinorSchemaVersion(4));
   return true;
}

/**
 * Upgrade from 50.2 to 50.3
 */
static bool H_UpgradeFromV2()
{
   if (GetSchemaLevelForMajorVersion(44) < 25)
   {
      CHK_EXEC(CreateConfigParam(_T("Objects.Maintenance.PredefinedPeriods"),
            _T("1h,8h,1d"),
            _T("Predefined object maintenance periods. Use m for minutes, h for hours and d for days."),
            _T(""),
            'S', true, false, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(44, 25));
   }

   CHK_EXEC(SetMinorSchemaVersion(3));
   return true;
}

/**
 * Upgrade from 50.1 to 50.2
 */
static bool H_UpgradeFromV1()
{
   CHK_EXEC(SQLQuery(_T("ALTER TABLE network_maps ADD link_styling_script $SQL:TEXT")));
   CHK_EXEC(SetMinorSchemaVersion(2));
   return true;
}

/**
 * Upgrade from 50.0 to 50.1
 */
static bool H_UpgradeFromV0()
{
   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when system detects loss of network connectivity based on beacon probing.\r\n")
      _T("Parameters:\r\n")
      _T("   1) numberOfBeacons - Number of beacons'")
      _T("WHERE event_code=50")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when system detects restoration of network connectivity based on beacon probing.\r\n")
      _T("Parameters:\r\n")
      _T("   1) numberOfBeacons - Number of beacons'")
      _T("WHERE event_code=51")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when cluster resource goes up.\r\n")
      _T("Parameters:\r\n")
      _T("   1) resourceId - Resource ID\r\n")
      _T("   2) resourceName - Resource name\r\n")
      _T("   3) newOwnerNodeId - New owner node ID\r\n")
      _T("   4) newOwnerNodeName - New owner node name'")
      _T("WHERE event_code=40")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when cluster resource moved between nodes.\r\n")
      _T("Parameters:\r\n")
      _T("   1) resourceId - Resource ID\r\n")
      _T("   2) resourceName - Resource name\r\n")
      _T("   3) previousOwnerNodeId - Previous owner node ID\r\n")
      _T("   4) previousOwnerNodeName - Previous owner node name\r\n")
      _T("   5) newOwnerNodeId - New owner node ID\r\n")
      _T("   6) newOwnerNodeName - New owner node name'")
      _T("WHERE event_code=38")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when node, cluster, or mobile device entered maintenance mode.\r\n")
      _T("Parameters:\r\n")
      _T("   1) comments - Comments\r\n")
      _T("   2) userId - Initiating user ID\r\n")
      _T("   3) userName - Initiating user name'")
      _T("WHERE event_code=78")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when node, cluster, or mobile device left maintenance mode.\r\n")
      _T("Parameters:\r\n")
      _T("   1) userId - Initiating user ID\r\n")
      _T("   2) userName - Initiating user name'")
      _T("WHERE event_code=79")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when template applied to node by autoapply rule.\r\n")
      _T("Parameters:\r\n")
      _T("   1) nodeId - Node ID\r\n")
      _T("   2) nodeName - Node name\r\n")
      _T("   3) templateId - Template ID\r\n")
      _T("   4) templateName - Template name'")
      _T("WHERE event_code=66")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when node bound to container object by autobind rule.\r\n")
      _T("Parameters:\r\n")
      _T("   1) nodeId - Node ID\r\n")
      _T("   2) nodeName - Node name\r\n")
      _T("   3) containerId - Container ID\r\n")
      _T("   4) containerName - Container name'")
      _T("WHERE event_code=64")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when cluster resource goes down.\r\n")
      _T("Parameters:\r\n")
      _T("   1) resourceId - Resource ID\r\n")
      _T("   2) resourceName - Resource name\r\n")
      _T("   3) lastOwnerNodeId - Last owner node ID\r\n")
      _T("   4) lastOwnerNodeName - Last owner node name'")
      _T("WHERE event_code=39")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when node added to cluster object by autoadd rule.\r\n")
      _T("Parameters:\r\n")
      _T("   1) nodeId - Node ID\r\n")
      _T("   2) nodeName - Node name\r\n")
      _T("   3) clusterId - Cluster ID\r\n")
      _T("   4) clusterName - Cluster name'")
      _T("WHERE event_code=114")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when node removed from cluster object by autoadd rule.\r\n")
      _T("Parameters:\r\n")
      _T("   1) nodeId - Node ID\r\n")
      _T("   2) nodeName - Node name\r\n")
      _T("   3) clusterId - Cluster ID\r\n")
      _T("   4) clusterName - Cluster name'")
      _T("WHERE event_code=115")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when node bound to container object by autobind rule.\r\n")
      _T("Parameters:\r\n")
      _T("   1) nodeId - Node ID\r\n")
      _T("   2) nodeName - Node name\r\n")
      _T("   3) containerId - Container ID\r\n")
      _T("   4) containerName - Container name'")
      _T("WHERE event_code=64")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when node unbound from container object by autobind rule.\r\n")
      _T("Parameters:\r\n")
      _T("   1) nodeId - Node ID\r\n")
      _T("   2) nodeName - Node name\r\n")
      _T("   3) containerId - Container ID\r\n")
      _T("   4) containerName - Container name'")
      _T("WHERE event_code=65")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when template removed from node by autoapply rule.\r\n")
      _T("Parameters:\r\n")
      _T("   1) nodeId - Node ID\r\n")
      _T("   2) nodeName - Node name\r\n")
      _T("   3) templateId - Template ID\r\n")
      _T("   4) templateName - Template name'")
      _T("WHERE event_code=67")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when device geolocation change is detected.\r\n")
      _T("Parameters:\r\n")
      _T("   1) newLatitude - Latitude of new location in degrees\r\n")
      _T("   2) newLongitude - Longitude of new location in degrees\r\n")
      _T("   3) newLatitudeAsString- Latitude of new location in textual form\r\n")
      _T("   4) newLongitudeAsString - Longitude of new location in textual form\r\n")
      _T("   5) previousLatitude - Latitude of previous location in degrees\r\n")
      _T("   6) previousLongitude - Longitude of previous location in degrees\r\n")
      _T("   7) previousLatitudeAsString - Latitude of previous location in textual form\r\n")
      _T("   8) previoudLongitudeAsString - Longitude of previous location in textual form'")
      _T("WHERE event_code=111")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when new device geolocation is within restricted area.\r\n")
      _T("Parameters:\r\n")
      _T("   1) newLatitude - Latitude of new location in degrees\r\n")
      _T("   2) newLongitude - Longitude of new location in degrees\r\n")
      _T("   3) newLatitudeAsString- Latitude of new location in textual form\r\n")
      _T("   4) newLongitudeAsString - Longitude of new location in textual form\r\n")
      _T("   5) areaName - Area name\r\n")
      _T("   6) areaId - Area ID'")
      _T("WHERE event_code=112")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when new device geolocation is ouside allowed areas.\r\n")
      _T("Parameters:\r\n")
      _T("   1) newLatitude - Latitude of new location in degrees\r\n")
      _T("   2) newLongitude - Longitude of new location in degrees\r\n")
      _T("   3) newLatitudeAsString- Latitude of new location in textual form\r\n")
      _T("   4) newLongitudeAsString - Longitude of new location in textual form'")
      _T("WHERE event_code=113")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when duplicate MAC address found.\r\n")
      _T("Parameters:\r\n")
      _T("   1) macAddress - MAC address\r\n")
      _T("   2) listOfInterfaces - List of interfaces where MAC address was found'")
      _T("WHERE event_code=119")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when system detects an event storm.\r\n")
      _T("Parameters:\r\n")
      _T("   1) eps - Events per second\r\n")
      _T("   2) duration - Duration\r\n")
      _T("   3) threshold - Threshold'")
      _T("WHERE event_code=48")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when system clears event storm condition.\r\n")
      _T("Parameters:\r\n")
      _T("   1) eps - Events per second\r\n")
      _T("   2) duration - Duration\r\n")
      _T("   3) threshold - Threshold'")
      _T("WHERE event_code=49")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when internal housekeeper task completes.\r\n")
      _T("Parameters:\r\n")
      _T("   1) elapsedTime - Housekeeper execution time in seconds'")
      _T("WHERE event_code=107")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when possibly duplicate IP address is detected by network discovery process.\r\n")
      _T("Parameters:\r\n")
      _T("   1) ipAddress - IP address\r\n")
      _T("   2) knownNodeId - Known node object ID\r\n")
      _T("   3) knownNodeName - Known node name\r\n")
      _T("   4) knownInterfaceName - Known interface name\r\n")
      _T("   5) knownMacAddress - Known MAC address\r\n")
      _T("   6) discoveredMacAddress - Discovered MAC address\r\n")
      _T("   7) discoverySourceNodeId - Discovery source node object ID\r\n")
      _T("   8) discoverySourceNodeName - Discovery source node name\r\n")
      _T("   9) discoveryDataSource - Discovery data source'")
      _T("WHERE event_code=101")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when switch port PAE state changed to FORCE UNAUTHORIZE.\r\n")
      _T("Parameters:\r\n")
      _T("   1) interfaceIndex - Interface index\r\n")
      _T("   2) interfaceName - Interface name'")
      _T("WHERE event_code=59")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when switch port PAE state changed.\r\n")
      _T("Parameters:\r\n")
      _T("   1) newPaeStateCode - New PAE state code\r\n")
      _T("   2) newPaeStateText - New PAE state as text\r\n")
      _T("   3) oldPaeStateCode - Old PAE state code\r\n")
      _T("   4) oldPaeStateText - Old PAE state as text\r\n")
      _T("   5) interfaceIndex - Interface index\r\n")
      _T("   6) interfaceName - Interface name'")
      _T("WHERE event_code=57")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when LDAP synchronization error occurs.\r\n")
      _T("Parameters:\r\n")
      _T("   1) userId - User ID\r\n")
      _T("   2) userGuid - User GUID\r\n")
      _T("   3) userLdapDn - User LDAP DN\r\n")
      _T("   4) userName - User name\r\n")
      _T("   5) description - Problem description'")
      _T("WHERE event_code=80")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when switch port backend authentication state changed.\r\n")
      _T("Parameters:\r\n")
      _T("   1) newBackendStateCode - New backend state code\r\n")
      _T("   2) newBackendStateText - New backend state as text\r\n")
      _T("   3) oldBackendStateCode - Old backend state code\r\n")
      _T("   4) oldBackendStateText - Old backend state as text\r\n")
      _T("   5) interfaceIndex - Interface index\r\n")
      _T("   6) interfaceName - Interface name'")
      _T("WHERE event_code=58")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when licensing problem is detected by extension module.\r\n")
      _T("Parameters:\r\n")
      _T("   1) description - Problem description'")
      _T("WHERE event_code=108")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when switch port backend authentication state changed to FAIL.\r\n")
      _T("Parameters:\r\n")
      _T("   1) interfaceIndex - Interface index\r\n")
      _T("   2) interfaceName - Interface name'")
      _T("WHERE event_code=60")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when switch port backend authentication state changed to TIMEOUT.\r\n")
      _T("Parameters:\r\n")
      _T("   1) interfaceIndex - Interface index\r\n")
      _T("   2) interfaceName - Interface name'")
      _T("WHERE event_code=61")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when node status changed to normal.\r\n")
      _T("Parameters:\r\n")
      _T("   1) previousNodeStatus - Previous node status'")
      _T("WHERE event_code=6")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when node status changed to \"Warning\".\r\n")
      _T("Parameters:\r\n")
      _T("   1) previousNodeStatus - Previous node status'")
      _T("WHERE event_code=7")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when node status changed to \"Minor Problem\" (informational).\r\n")
      _T("Parameters:\r\n")
      _T("   1) previousNodeStatus - Previous node status'")
      _T("WHERE event_code=8")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when node status changed to \"Major Problem\".\r\n")
      _T("Parameters:\r\n")
      _T("   1) previousNodeStatus - Previous node status'")
      _T("WHERE event_code=9")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when node status changed to critical.s\r\n")
      _T("Parameters:\r\n")
      _T("   1) previousNodeStatus - Previous node status'")
      _T("WHERE event_code=10")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when node status changed to unknown.\r\n")
      _T("Parameters:\r\n")
      _T("   1) previousNodeStatus - Previous node status'")
      _T("WHERE event_code=11")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when node status changed to unmanaged.\r\n")
      _T("Parameters:\r\n")
      _T("   1) previousNodeStatus - Previous node status'")
      _T("WHERE event_code=12")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when new interface object added to the database.\r\n")
      _T("Parameters:\r\n")
      _T("   1) interfaceObjectId - Interface object ID\r\n")
      _T("   2) interfaceName - Interface name\r\n")
      _T("   3) interfaceIpAddress - Interface IP address\r\n")
      _T("   4) interfaceNetmask - Interface netmask\r\n")
      _T("   5) interfaceIndex - Interface index'")
      _T("WHERE event_code=3")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when interface expected state set to UP.\r\n")
      _T("Parameters:\r\n")
      _T("   1) interfaceIndex - Interface index\r\n")
      _T("   2) intefaceName - Interface name'")
      _T("WHERE event_code=83")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when interface expected state set to DOWN.\r\n")
      _T("Parameters:\r\n")
      _T("   1) interfaceIndex - Interface index\r\n")
      _T("   2) intefaceName - Interface name'")
      _T("WHERE event_code=84")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when interface expected state set to IGNORE.\r\n")
      _T("Parameters:\r\n")
      _T("   1) interfaceIndex - Interface index\r\n")
      _T("   2) intefaceName - Interface name'")
      _T("WHERE event_code=85")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when communication with the node re-established.\r\n")
      _T("Parameters:\r\n")
      _T("   1) reason - Reason'")
      _T("WHERE event_code=29")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated on status poll if agent reports problem with log file.\r\n")
      _T("Parameters:\r\n")
      _T("   1) statusCode - Status code\r\n")
      _T("   2) description - Description'")
      _T("WHERE event_code=81")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when server is unable to send notification.\r\n")
      _T("Parameters:\r\n")
      _T("   1) notificationChannelName - Notification channel name\r\n")
      _T("   2) recipientAddress - Recipient address\r\n")
      _T("   3) notificationSubject - Notification subject\r\n")
      _T("   4) notificationMessage - Notification message'")
      _T("WHERE event_code=22")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when notification channel fails health check.\r\n")
      _T("Parameters:\r\n")
      _T("   1) channelName - Notification channel name\r\n")
      _T("   2) channelDriverName - Notification channel driver name'")
      _T("WHERE event_code=125")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when notification channel passes health check.\r\n")
      _T("Parameters:\r\n")
      _T("   1) channelName - Notification channel name\r\n")
      _T("   2) channelDriverName - Notification channel driver name'")
      _T("WHERE event_code=126")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when one of the system threads hangs or stops unexpectedly.\r\n")
      _T("Parameters:\r\n")
      _T("   1) threadName - Thread name'")
      _T("WHERE event_code=20")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when one of the system threads which previously hangs or stops unexpectedly was returned to running state.\r\n")
      _T("Parameters:\r\n")
      _T("   1) threadName - Thread name'")
      _T("WHERE event_code=21")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Default event for condition activation.\r\n")
      _T("Parameters:\r\n")
      _T("   1) conditionObjectId - Condition object ID\r\n")
      _T("   2) conditionObjectName - Condition object name\r\n")
      _T("   3) previousConditionStatus - Previous condition status\r\n")
      _T("   4) currentConditionStatus - Current condition status'")
      _T("WHERE event_code=34")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Default event for condition deactivation.\r\n")
      _T("Parameters:\r\n")
      _T("   1) conditionObjectId - Condition object ID\r\n")
      _T("   2) conditionObjectName - Condition object name\r\n")
      _T("   3) previousConditionStatus - Previous condition status\r\n")
      _T("   4) currentConditionStatus - Current condition status'")
      _T("WHERE event_code=35")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when new node object added to the database.\r\n")
      _T("Parameters:\r\n")
      _T("   1) nodeOrigin - Node origin (0 = created manually, 1 = created by network discovery, 2 = created by tunnel auto bind)'")
      _T("WHERE event_code=1")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when new subnet object added to the database.\r\n")
      _T("Parameters:\r\n")
      _T("   1) subnetObjectId - Subnet object ID\r\n")
      _T("   2) subnetName - Subnet name\r\n")
      _T("   3) ipAddress - IP address\r\n")
      _T("   4) networkMask - Network mask'")
      _T("WHERE event_code=2")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when subnet object deleted from the database.\r\n")
      _T("Parameters:\r\n")
      _T("   1) subnetObjectId - Subnet object ID\r\n")
      _T("   2) subnetName - Subnet name\r\n")
      _T("   3) ipAddress - IP address\r\n")
      _T("   4) networkMask - Network mask'")
      _T("WHERE event_code=19")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when alarm timeout expires.\r\n")
      _T("Parameters:\r\n")
      _T("   1) alarmId - Alarm ID\r\n")
      _T("   2) alarmMessage - Alarm message\r\n")
      _T("   3) alarmKey - Alarm key\r\n")
      _T("   4) originalEventCode - Original event code\r\n")
      _T("   5) originalEventName - Original event name'")
      _T("WHERE event_code=43")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated on status poll if agent reports local database problem.\r\n")
      _T("Parameters:\r\n")
      _T("   1) statusCode - Status code\r\n")
      _T("   2) description - Description'")
      _T("WHERE event_code=82")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when node capabilities changed.\r\n")
      _T("Parameters:\r\n")
      _T("   1) oldCapabilities - Old capabilities\r\n")
      _T("   2) newCapabilities - New capabilities'")
      _T("WHERE event_code=13")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when IP address added to interface.\r\n")
      _T("Parameters:\r\n")
      _T("   1) interfaceObjectId - object ID\r\n")
      _T("   2) interfaceName - Interface name\r\n")
      _T("   3) ipAddress - IP address\r\n")
      _T("   4) networkMask - Network mask\r\n")
      _T("   5) interfaceIndex - Interface index'")
      _T("WHERE event_code=76")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when IP address deleted from interface.\r\n")
      _T("Parameters:\r\n")
      _T("   1) interfaceObjectId - object ID\r\n")
      _T("   2) interfaceName - Interface name\r\n")
      _T("   3) ipAddress - IP address\r\n")
      _T("   4) networkMask - Network mask\r\n")
      _T("   5) interfaceIndex - Interface index'")
      _T("WHERE event_code=77")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when primary IP address changed (usually because of primary name change or DNS change).\r\n")
      _T("Parameters:\r\n")
      _T("   1) newIpAddress - New IP address\r\n")
      _T("   2) oldIpAddress - Old IP address\r\n")
      _T("   3) primaryHostName - Primary host name'")
      _T("WHERE event_code=56")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when node is deleted by network discovery de-duplication process.\r\n")
      _T("Parameters:\r\n")
      _T("   1) originalNodeObjectId - Original node object ID\r\n")
      _T("   2) originalNodeName - Original node name\r\n")
      _T("   3) originalNodePrimaryHostName - Original node primary host name\r\n")
      _T("   4) duplicateNodeObjectId - Duplicate node object ID\r\n")
      _T("   5) duplicateNodeName - Duplicate node name\r\n")
      _T("   6) duplicateNodePrimaryHostName - Duplicate node primary host name\r\n")
      _T("   7) reason - Reason'")
      _T("WHERE event_code=100")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when system detects an SNMP trap flood.\r\n")
      _T("Parameters:\r\n")
      _T("   1) snmpTrapsPerSecond - SNMP traps per second\r\n")
      _T("   2) duration - Duration\r\n")
      _T("   3) threshold - Threshold'")
      _T("WHERE event_code=109")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated after SNMP trap flood state is cleared.\r\n")
      _T("Parameters:\r\n")
      _T("   1) snmpTrapsPerSecond - SNMP traps per second\r\n")
      _T("   2) duration - Duration\r\n")
      _T("   3) threshold - Threshold'")
      _T("WHERE event_code=110")));
      
   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when agent ID change detected.\r\n")
      _T("Parameters:\r\n")
      _T("   1) oldAgentId - Old agent ID\r\n")
      _T("   2) newAgentId - New agent ID'")
      _T("WHERE event_code=93")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when interface object deleted from the database.\r\n")
      _T("Parameters:\r\n")
      _T("   1) interfaceIndex - Interface index\r\n")
      _T("   2) interfaceName - Interface name\r\n")
      _T("   3) interfaceIpAddress -Interface IP address\r\n")
      _T("   4) interfaceMask - Interface netmask'")
      _T("WHERE event_code=16")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when server detects change of interface''s MAC address.\r\n")
      _T("Parameters:\r\n")
      _T("   1) interfaceObjectId - Interface object ID\r\n")
      _T("   2) interfaceIndex - Interface index\r\n")
      _T("   3) interfaceName - Interface name\r\n")
      _T("   4) oldMacAddress - Old MAC address\r\n")
      _T("   5) newMacAddress - New MAC address'")
      _T("WHERE event_code=23")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when when network mask on interface is corrected.\r\n")
      _T("Parameters:\r\n")
      _T("   1) interfaceObjectId - Interface object ID\r\n")
      _T("   2) interfaceName - Interface name\r\n")
      _T("   3) interfaceIpAddress - Interface IP address\r\n")
      _T("   4) interfaceNetmask - Interface netmask\r\n")
      _T("   5) interfaceIndex - Interface index\r\n")
      _T("   6) interfaceOldMask - Interface old mask'")
      _T("WHERE event_code=75")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when server detects invalid network mask on an interface.\r\n")
      _T("Parameters:\r\n")
      _T("   1) interfaceObjectId - Interface object ID\r\n")
      _T("   2) interfaceIndex - Interface index\r\n")
      _T("   3) interfaceName - Interface name\r\n")
      _T("   4) actualNetworkMask - Actual network mask on interface\r\n")
      _T("   5) correctNetworkMask - Correct network mask'")
      _T("WHERE event_code=24")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when interface goes to unknown state.\r\n")
      _T("Please note that source of event is node, not an interface itself.\r\n")
      _T("Parameters:\r\n")
      _T("   1) interfaceObjectId - Interface object ID\r\n")
      _T("   2) interfaceName - Interface name\r\n")
      _T("   3) interfaceIpAddress - Interface IP address\r\n")
      _T("   4) interfaceNetMask - Interface netmask\r\n")
      _T("   5) interfaceIndex - Interface index'")
      _T("WHERE event_code=45")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when interface administratively disabled.\r\n")
      _T("Please note that source of event is node, not an interface itself.\r\n")
      _T("Parameters:\r\n")
      _T("   1) interfaceObjectId - Interface object ID\r\n")
      _T("   2) interfaceName - Interface name\r\n")
      _T("   3) interfaceIpAddress - Interface IP address\r\n")
      _T("   4) interfaceNetMask - Interface netmask\r\n")
      _T("   5) interfaceIndex - Interface index'")
      _T("WHERE event_code=46")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when interface goes to testing state.\r\n")
      _T("Please note that source of event is node, not an interface itself.\r\n")
      _T("Parameters:\r\n")
      _T("   1) interfaceObjectId - Interface object ID\r\n")
      _T("   2) interfaceName - Interface name\r\n")
      _T("   3) interfaceIpAddress - Interface IP address\r\n")
      _T("   4) interfaceNetMask - Interface netmask\r\n")
      _T("   5) interfaceIndex - Interface index'")
      _T("WHERE event_code=47")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when interface goes up but it''s expected state set to DOWN.\r\n")
      _T("Please note that source of event is node, not an interface itself.\r\n")
      _T("Parameters:\r\n")
      _T("   1) interfaceObjectId - Interface object ID\r\n")
      _T("   2) interfaceName - Interface name\r\n")
      _T("   3) interfaceIpAddress - Interface IP address\r\n")
      _T("   4) interfaceNetMask - Interface netmask\r\n")
      _T("   5) interfaceIndex - Interface index'")
      _T("WHERE event_code=62")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when interface goes down and it''s expected state is DOWN.\r\n")
      _T("Please note that source of event is node, not an interface itself.\r\n")
      _T("Parameters:\r\n")
      _T("   1) interfaceObjectId - Interface object ID\r\n")
      _T("   2) interfaceName - Interface name\r\n")
      _T("   3) interfaceIpAddress - Interface IP address\r\n")
      _T("   4) interfaceNetMask - Interface netmask\r\n")
      _T("   5) interfaceIndex - Interface index'")
      _T("WHERE event_code=63")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when interface goes up.\r\n")
      _T("Please note that source of event is node, not an interface itself.\r\n")
      _T("Parameters:\r\n")
      _T("   1) interfaceObjectId - Interface object ID\r\n")
      _T("   2) interfaceName - Interface name\r\n")
      _T("   3) interfaceIpAddress - Interface IP address\r\n")
      _T("   4) interfaceNetMask - Interface netmask\r\n")
      _T("   5) interfaceIndex - Interface index'")
      _T("WHERE event_code=4")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when interface goes down.\r\n")
      _T("Please note that source of event is node, not an interface itself.\r\n")
      _T("Parameters:\r\n")
      _T("   1) interfaceObjectId - Interface object ID\r\n")
      _T("   2) interfaceName - Interface name\r\n")
      _T("   3) interfaceIpAddress - Interface IP address\r\n")
      _T("   4) interfaceNetMask - Interface netmask\r\n")
      _T("   5) interfaceIndex - Interface index'")
      _T("WHERE event_code=5")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when network service is not responding to management server as expected.\r\n")
      _T("Parameters:\r\n")
      _T("   1) serviceName - Service name\r\n")
      _T("   2) serviceObjectId - Service object ID\r\n")
      _T("   3) serviceType - Service type'")
      _T("WHERE event_code=25")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when network service responds as expected after failure.\r\n")
      _T("Parameters:\r\n")
      _T("   1) serviceName - Service name\r\n")
      _T("   2) serviceObjectId - Service object ID\r\n")
      _T("   3) serviceType - Service type'")
      _T("WHERE event_code=26")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when management server is unable to determine state of the network service due to agent or server-to-agent communication failure.\r\n")
      _T("Parameters:\r\n")
      _T("   1) serviceName - Service name\r\n")
      _T("   2) serviceObjectId - Service object ID\r\n")
      _T("   3) serviceType - Service type'")
      _T("WHERE event_code=27")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when agent policy within template fails validation.\r\n")
      _T("Parameters:\r\n")
      _T("   1) templateName - Template name\r\n")
      _T("   2) templateId - Template ID\r\n")
      _T("   3) policyName - Policy name\r\n")
      _T("   4) policyType - Policy type\r\n")
      _T("   5) policyId - Policy ID\r\n")
      _T("   6) additionalInfo - Additional info'")
      _T("WHERE event_code=117")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when peer information for interface changes.\r\n")
      _T("Parameters:\r\n")
      _T("   1) localIfId - Local interface object ID\r\n")
      _T("   2) localIfIndex - Local interface index\r\n")
      _T("   3) localIfName - Local interface name\r\n")
      _T("   4) localIfIP - Local interface IP address\r\n")
      _T("   5) localIfMAC - Local interface MAC address\r\n")
      _T("   6) remoteNodeId - Peer node object ID\r\n")
      _T("   7) remoteNodeName - Peer node name\r\n")
      _T("   8) remoteIfId - interface object ID\r\n")
      _T("   9) remoteIfIndex - Peer interface index\r\n")
      _T("   10) remoteIfName - Peer interface name\r\n")
      _T("   11) remoteIfIP - Peer interface IP address\r\n")
      _T("   12) remoteIfMAC - Peer interface MAC address\r\n")
      _T("   13) protocol - Discovery protocol'")
      _T("WHERE event_code=71")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when node is unreachable by management server because of network failure.\r\n")
      _T("Parameters:\r\n")
      _T("   1) reasonCode - Reason Code\r\n")
      _T("   2) reason - Reason\r\n")
      _T("   3) rootCauseNodeId - Root Cause Node ID\r\n")
      _T("   4) rootCauseNodeName - Root Cause Node Name\r\n")
      _T("   5) rootCauseInterfaceId - Root Cause Interface ID\r\n")
      _T("   6) rootCauseInterfaceName - Root Cause Interface Name\r\n")
      _T("   7) description - Description'")
      _T("WHERE event_code=68")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when server detects routing loop during network path trace.\r\n")
      _T("Parameters:\r\n")
      _T("   1) protocol - Protovol (IPv4 or IPv6)\r\n")
      _T("   2) destNodeId - Path trace destination node ID\r\n")
      _T("   3) destAddress - Path trace destination address\r\n")
      _T("   4) sourceNodeId - Path trace source node ID\r\n")
      _T("   5) sourceAddress - Path trace source node address\r\n")
      _T("   6) prefix - Routing prefix (subnet address)\r\n")
      _T("   7) prefixLength - Routing prefix length (subnet mask length)\r\n")
      _T("   8) nextHopNodeId - Next hop node ID\r\n")
      _T("   9) nextHopAddress - Next hop address'")
      _T("WHERE event_code=86")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when new hardware component is found.\r\n")
      _T("Parameters:\r\n")
      _T("   1) category - Category\r\n")
      _T("   2) type - Type\r\n")
      _T("   3) vendor - Vendor\r\n")
      _T("   4) model - Model\r\n")
      _T("   5) location - Location\r\n")
      _T("   6) partNumber - Part number\r\n")
      _T("   7) serialNumber - Serial number\r\n")
      _T("   8) capacity - Capacity\r\n")
      _T("   9) description - Description'")
      _T("WHERE event_code=98")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when hardware component removal is detected.\r\n")
      _T("Parameters:\r\n")
      _T("   1) category - Category\r\n")
      _T("   2) type - Type\r\n")
      _T("   3) vendor - Vendor\r\n")
      _T("   4) model - Model\r\n")
      _T("   5) location - Location\r\n")
      _T("   6) partNumber - Part number\r\n")
      _T("   7) serialnumber - Serial number\r\n")
      _T("   8) capacity - Capacity\r\n")
      _T("   9) description - Description'")
      _T("WHERE event_code=99")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when new software package is found.\r\n")
      _T("Parameters:\r\n")
      _T("   1) name - Package name\r\n")
      _T("   2) version - Package version'")
      _T("WHERE event_code=87")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when software package version change is detected.\r\n")
      _T("Parameters:\r\n")
      _T("   1) name - Package name\r\n")
      _T("   2) version - New package version\r\n")
      _T("   3) previousVersion - Old package version'")
      _T("WHERE event_code=88")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when software package removal is detected.\r\n")
      _T("Parameters:\r\n")
      _T("   1) name - Package name\r\n")
      _T("   2) version - Last known package version'")
      _T("WHERE event_code=89")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when agent tunnel is open and bound.\r\n")
      _T("Parameters:\r\n")
      _T("   1) tunnelId - Tunnel ID\r\n")
      _T("   2) ipAddress - Remote system IP address\r\n")
      _T("   3) systemName - Remote system name\r\n")
      _T("   4) hostName - Remote system FQDN\r\n")
      _T("   5) platformName - Remote system platform\r\n")
      _T("   6) systemInfo - Remote system information\r\n")
      _T("   7) agentVersion - Agent version\r\n")
      _T("   8) agentId - Agent ID'")
      _T("WHERE event_code=94")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when agent tunnel is closed.\r\n")
      _T("Parameters:\r\n")
      _T("   1) tunnelId - Tunnel ID\r\n")
      _T("   2) ipAddress - Remote system IP address\r\n")
      _T("   3) systemName - Remote system name\r\n")
      _T("   4) hostName - Remote system FQDN\r\n")
      _T("   5) platformName - Remote system platform\r\n")
      _T("   6) systemInfo - Remote system information\r\n")
      _T("   7) agentVersion - Agent version\r\n")
      _T("   8) agentId - Agent ID'")
      _T("WHERE event_code=94")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when new tunnel is replacing existing one and host data mismatch is detected.\r\n")
      _T("Parameters:\r\n")
      _T("   1) tunnelId - Tunnel ID\r\n")
      _T("   2) oldIPAddress - Old remote system IP address\r\n")
      _T("   3) newIPAddress - New remote system IP address\r\n")
      _T("   4) oldSystemName - Old remote system name\r\n")
      _T("   5) newSystemName- New remote system name\r\n")
      _T("   6) oldHostName - Old remote system FQDN\r\n")
      _T("   7) newHostName - New remote system FQDN\r\n")
      _T("   8) oldHardwareId - Old hardware ID\r\n")
      _T("   9) newHardwareId - New hardware ID'")
      _T("WHERE event_code=116")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when agent ID mismatch detected during tunnel bind.\r\n")
      _T("Parameters:\r\n")
      _T("   1) tunnelId - Tunnel ID\r\n")
      _T("   2) ipAddress - Remote system IP address\r\n")
      _T("   3) systemName - Remote system name\r\n")
      _T("   4) hostName - Remote system FQDN\r\n")
      _T("   5) platformName - Remote system platform\r\n")
      _T("   6) systemInfo - Remote system information\r\n")
      _T("   7) agentVersion - Agent version\r\n")
      _T("   8) tunnelAgentId - Tunnel agent ID\r\n")
      _T("   9) nodeAgentId - Node agent ID'")
      _T("WHERE event_code=96")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when unbound agent tunnel is not bound or closed for more than configured threshold.\r\n")
      _T("Parameters:\r\n")
      _T("   1) tunnelId - Tunnel ID\r\n")
      _T("   2) ipAddress - Remote system IP address\r\n")
      _T("   3) systemName - Remote system name\r\n")
      _T("   4) hostName - Remote system FQDN\r\n")
      _T("   5) platformName - Remote system platform\r\n")
      _T("   6) systemInfo - Remote system information\r\n")
      _T("   7) agentVersion - Agent version\r\n")
      _T("   8) agentId - Agent ID\r\n")
      _T("   9) idleTimeout - Configured idle timeout'")
      _T("WHERE event_code=92")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when access point state changes to ADOPTED.\r\n")
      _T("Parameters:\r\n")
      _T("   1) id - Access point object ID\r\n")
      _T("   2) name - Access point name\r\n")
      _T("   3) macAddr - Access point MAC address\r\n")
      _T("   4) ipAddr - Access point IP address\r\n")
      _T("   5) vendor - Access point vendor name\r\n")
      _T("   6) model - Access point model\r\n")
      _T("   7) serialNumber - Access point serial number'")
      _T("WHERE event_code=72")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when access point state changes to UNADOPTED.\r\n")
      _T("Parameters:\r\n")
      _T("   1) id - Access point object ID\r\n")
      _T("   2) name - Access point name\r\n")
      _T("   3) macAddr - Access point MAC address\r\n")
      _T("   4) ipAddr - Access point IP address\r\n")
      _T("   5) vendor - Access point vendor name\r\n")
      _T("   6) model - Access point model\r\n")
      _T("   7) serialNumber - Access point serial number'")
      _T("WHERE event_code=73")));

   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET ")
      _T("description='Generated when access point state changes to DOWN.\r\n")
      _T("Parameters:\r\n")
      _T("   1) id - Access point object ID\r\n")
      _T("   2) name - Access point name\r\n")
      _T("   3) macAddr - Access point MAC address\r\n")
      _T("   4) ipAddr - Access point IP address\r\n")
      _T("   5) vendor - Access point vendor name\r\n")
      _T("   6) model - Access point model\r\n")
      _T("   7) serialNumber - Access point serial number'")
      _T("WHERE event_code=74")));

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
   { 49, 51, 0,  H_UpgradeFromV49 },
   { 48, 50, 49, H_UpgradeFromV48 },
   { 47, 50, 48, H_UpgradeFromV47 },
   { 46, 50, 47, H_UpgradeFromV46 },
   { 45, 50, 46, H_UpgradeFromV45 },
   { 44, 50, 45, H_UpgradeFromV44 },
   { 43, 50, 44, H_UpgradeFromV43 },
   { 42, 50, 43, H_UpgradeFromV42 },
   { 41, 50, 42, H_UpgradeFromV41 },
   { 40, 50, 41, H_UpgradeFromV40 },
   { 39, 50, 40, H_UpgradeFromV39 },
   { 38, 50, 39, H_UpgradeFromV38 },
   { 37, 50, 38, H_UpgradeFromV37 },
   { 36, 50, 37, H_UpgradeFromV36 },
   { 35, 50, 36, H_UpgradeFromV35 },
   { 34, 50, 35, H_UpgradeFromV34 },
   { 33, 50, 34, H_UpgradeFromV33 },
   { 32, 50, 33, H_UpgradeFromV32 },
   { 31, 50, 32, H_UpgradeFromV31 },
   { 30, 50, 31, H_UpgradeFromV30 },
   { 29, 50, 30, H_UpgradeFromV29 },
   { 28, 50, 29, H_UpgradeFromV28 },
   { 27, 50, 28, H_UpgradeFromV27 },
   { 26, 50, 27, H_UpgradeFromV26 },
   { 25, 50, 26, H_UpgradeFromV25 },
   { 24, 50, 25, H_UpgradeFromV24 },
   { 23, 50, 24, H_UpgradeFromV23 },
   { 22, 50, 23, H_UpgradeFromV22 },
   { 21, 50, 22, H_UpgradeFromV21 },
   { 20, 50, 21, H_UpgradeFromV20 },
   { 19, 50, 20, H_UpgradeFromV19 },
   { 18, 50, 19, H_UpgradeFromV18 },
   { 17, 50, 18, H_UpgradeFromV17 },
   { 16, 50, 17, H_UpgradeFromV16 },
   { 15, 50, 16, H_UpgradeFromV15 },
   { 14, 50, 15, H_UpgradeFromV14 },
   { 13, 50, 14, H_UpgradeFromV13 },
   { 12, 50, 13, H_UpgradeFromV12 },
   { 11, 50, 12, H_UpgradeFromV11 },
   { 10, 50, 11, H_UpgradeFromV10 },
   { 9,  50, 10, H_UpgradeFromV9  },
   { 8,  50, 9,  H_UpgradeFromV8  },
   { 7,  50, 8,  H_UpgradeFromV7  },
   { 6,  50, 7,  H_UpgradeFromV6  },
   { 5,  50, 6,  H_UpgradeFromV5  },
   { 4,  50, 5,  H_UpgradeFromV4  },
   { 3,  50, 4,  H_UpgradeFromV3  },
   { 2,  50, 3,  H_UpgradeFromV2  },
   { 1,  50, 2,  H_UpgradeFromV1  },
   { 0,  50, 1,  H_UpgradeFromV0  },
   { 0,  0,  0,  nullptr }
};

/**
 * Upgrade database to new version
 */
bool MajorSchemaUpgrade_V50()
{
   int32_t major, minor;
   if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
      return false;

   while (major == 50)
   {
      // Find upgrade procedure
      int i;
      for (i = 0; s_dbUpgradeMap[i].upgradeProc != nullptr; i++)
         if (s_dbUpgradeMap[i].version == minor)
            break;
      if (s_dbUpgradeMap[i].upgradeProc == nullptr)
      {
         _tprintf(_T("Unable to find upgrade procedure for version 50.%d\n"), minor);
         return false;
      }
      _tprintf(_T("Upgrading from version 50.%d to %d.%d\n"), minor, s_dbUpgradeMap[i].nextMajor, s_dbUpgradeMap[i].nextMinor);
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
