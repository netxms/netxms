/*
** nxdbmgr - NetXMS database manager
** Copyright (C) 2020-2021 Raden Solutions
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
** File: upgrade_v39.cpp
**
**/

#include "nxdbmgr.h"
#include <nxevent.h>

/**
 * Upgrade from 39.17 to 40.0
 */
static bool H_UpgradeFromV17()
{
   CHK_EXEC(SetMajorSchemaVersion(40, 0));
   return true;
}

/**
 * Upgrade from 39.16 to 39.17
 */
static bool H_UpgradeFromV16()
{
   CHK_EXEC(CreateConfigParam(_T("Jira.Webhook.Path"),
         _T("/jira-webhook"),
         _T("Path part of Jira webhook URL (must start with /)."),
         nullptr, 'S', true, true, false, false));
   CHK_EXEC(CreateConfigParam(_T("Jira.Webhook.Port"),
         _T("8008"),
         _T("Jira webhook listener port (0 to disable webhook)."),
         nullptr, 'I', true, true, false, false));
   CHK_EXEC(CreateConfigParam(_T("Jira.ResolvedStatus"),
         _T("Done"),
         _T("Comma separated list of issue status codes indicating that issue is resolved."),
         nullptr, 'S', true, false, false, false));

   static const TCHAR batch[] =
      _T("UPDATE config SET var_name='Jira.Login',need_server_restart='0' WHERE var_name='JiraLogin'\n")
      _T("UPDATE config SET var_name='Jira.Password',need_server_restart='0' WHERE var_name='JiraPassword'\n")
      _T("UPDATE config SET var_name='Jira.ServerURL',need_server_restart='0',default_value='https://jira.atlassian.com' WHERE var_name='JiraServerURL'\n")
      _T("UPDATE config SET var_name='Jira.IssueType' WHERE var_name='JiraIssueType'\n")
      _T("UPDATE config SET var_name='Jira.ProjectCode' WHERE var_name='JiraProjectCode'\n")
      _T("UPDATE config SET var_name='Jira.ProjectComponent' WHERE var_name='JiraProjectComponent'\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   CHK_EXEC(SetMinorSchemaVersion(17));
   return true;
}

/**
 * Upgrade from 39.15 to 39.16
 */
static bool H_UpgradeFromV15()
{
   CHK_EXEC(SQLQuery(_T("ALTER TABLE responsible_users ADD tag varchar(31)")));
   if (g_dbSyntax == DB_SYNTAX_SQLITE)
      CHK_EXEC(SQLQuery(_T("UPDATE responsible_users SET tag='level'||escalation_level")));
   else
      CHK_EXEC(SQLQuery(_T("UPDATE responsible_users SET tag=CONCAT('level',escalation_level)")));
   CHK_EXEC(DBDropColumn(g_dbHandle, _T("responsible_users"), _T("escalation_level")));

   StringBuffer tags;
   DB_RESULT hResult = SQLSelect(_T("SELECT DISTINCT(tag) FROM responsible_users"));
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         TCHAR tag[32];
         DBGetField(hResult, i, 0, tag, 32);
         if (!tags.isEmpty())
            tags.append(_T(","));
         tags.append(tag);
      }
      DBFreeResult(hResult);
   }
   else if (!g_ignoreErrors)
   {
      return false;
   }

   CHK_EXEC(CreateConfigParam(_T("Objects.ResponsibleUsers.AllowedTags"),
         _T(""),
         _T("Allowed tags for responsible users (comma separated list)."),
         nullptr, 'S', true, false, false, false));

   if (!tags.isEmpty())
   {
      TCHAR query[4096];
      _sntprintf(query, 4096, _T("UPDATE config SET var_value='%s' WHERE var_name='Objects.ResponsibleUsers.AllowedTags'"), tags.cstr());
      CHK_EXEC(SQLQuery(query));
   }

   CHK_EXEC(SetMinorSchemaVersion(16));
   return true;
}

/**
 * Upgrade from 39.14 to 39.15
 */
static bool H_UpgradeFromV14()
{
   static const TCHAR *batch =
      _T("ALTER TABLE agent_pkg ADD pkg_type varchar(15)\n")
      _T("ALTER TABLE agent_pkg ADD command varchar(255)\n")
      _T("UPDATE agent_pkg SET pkg_type='agent-installer'\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(SetMinorSchemaVersion(15));
   return true;
}

/**
 * Upgrade from 39.13 to 39.14
 */
static bool H_UpgradeFromV13()
{
   static const TCHAR *batch =
      _T("ALTER TABLE object_properties ADD region varchar(63)\n")
      _T("ALTER TABLE object_properties ADD district varchar(63)\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(SetMinorSchemaVersion(14));
   return true;
}

/**
 * Upgrade from 39.12 to 39.13
 */
static bool H_UpgradeFromV12()
{
   CHK_EXEC(SQLQuery(_T("ALTER TABLE responsible_users ADD escalation_level integer")));
   CHK_EXEC(SQLQuery(_T("UPDATE responsible_users SET escalation_level=0")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("responsible_users"), _T("escalation_level")));
   CHK_EXEC(SetMinorSchemaVersion(13));
   return true;
}

/**
 * Upgrade from 39.11 to 39.12
 */
static bool H_UpgradeFromV11()
{
   CHK_EXEC_NO_SP(DBDropColumn(g_dbHandle, _T("nodes"), _T("status_poll_type")));
   CHK_EXEC(SetMinorSchemaVersion(12));
   return true;
}

/**
 * Upgrade from 39.10 to 39.11
 */
static bool H_UpgradeFromV10()
{
   // Drop incorrectly created notification and action execution log tables in upgrade for version 39.6
   if (DBIsTableExist(g_dbHandle, _T("notification_log")) == DBIsTableExist_Found)
      CHK_EXEC_NO_SP(SQLQuery(_T("DROP TABLE notification_log")));

   if (DBIsTableExist(g_dbHandle, _T("server_action_execution_log")) == DBIsTableExist_Found)
      CHK_EXEC_NO_SP(SQLQuery(_T("DROP TABLE server_action_execution_log")));

   if (g_dbSyntax == DB_SYNTAX_TSDB)
   {
      CHK_EXEC(CreateTable(
         _T("CREATE TABLE notification_log (")
         _T("   id $SQL:INT64 not null,")
         _T("   notification_timestamp timestamptz not null,")
         _T("   notification_channel varchar(63) not null,")
         _T("   recipient varchar(2000) null,")
         _T("   subject varchar(2000) null,")
         _T("   message varchar(2000) null,")
         _T("   success char(1) not null,")
         _T("PRIMARY KEY(id,notification_timestamp))")));
      CHK_EXEC(SQLQuery(_T("SELECT create_hypertable('notification_log', 'notification_timestamp', chunk_time_interval => interval '86400 seconds')")));
   }
   else
   {
      CHK_EXEC(CreateTable(
         _T("CREATE TABLE notification_log (")
         _T("   id $SQL:INT64 not null,")
         _T("   notification_channel varchar(63) not null,")
         _T("   notification_timestamp integer not null,")
         _T("   recipient varchar(2000) null,")
         _T("   subject varchar(2000) null,")
         _T("   message varchar(2000) null,")
         _T("   success char(1) not null,")
         _T("PRIMARY KEY(id))")));
   }
   CHK_EXEC(SQLQuery(_T("CREATE INDEX idx_notification_log_timestamp ON notification_log(notification_timestamp)")));

   if (g_dbSyntax == DB_SYNTAX_TSDB)
   {
      CHK_EXEC(CreateTable(
         _T("CREATE TABLE server_action_execution_log (")
         _T("   id $SQL:INT64 not null,")
         _T("   action_timestamp timestamptz not null,")
         _T("   action_id integer not null,")
         _T("   action_name varchar(63) null,")
         _T("   channel_name varchar(63) null,")
         _T("   recipient varchar(2000) null,")
         _T("   subject varchar(2000) null,")
         _T("   action_data varchar(2000) null,")
         _T("   event_id $SQL:INT64 not null,")
         _T("   event_code integer not null,")
         _T("   success char(1) not null,")
         _T("PRIMARY KEY(id,action_timestamp))")));
      CHK_EXEC(SQLQuery(_T("SELECT create_hypertable('server_action_execution_log', 'action_timestamp', chunk_time_interval => interval '86400 seconds')")));
   }
   else
   {
      CHK_EXEC(CreateTable(
         _T("CREATE TABLE server_action_execution_log (")
         _T("   id $SQL:INT64 not null,")
         _T("   action_timestamp integer not null,")
         _T("   action_id integer not null,")
         _T("   action_name varchar(63) null,")
         _T("   channel_name varchar(63) null,")
         _T("   recipient varchar(2000) null,")
         _T("   subject varchar(2000) null,")
         _T("   action_data varchar(2000) null,")
         _T("   event_id $SQL:INT64 not null,")
         _T("   event_code integer not null,")
         _T("   success char(1) not null,")
         _T("PRIMARY KEY(id))")));
   }
   CHK_EXEC(SQLQuery(_T("CREATE INDEX idx_srv_action_log_timestamp ON server_action_execution_log(action_timestamp)")));

   CHK_EXEC(CreateConfigParam(_T("ActionExecutionLog.RetentionTime"),
         _T("90"),
         _T("Retention time in days for the records in server action execution log. All records older than specified will be deleted by housekeeping process."),
         _T("days"),
         'I', true, false, false, false));
   CHK_EXEC(CreateConfigParam(_T("NotificationLog.RetentionTime"),
         _T("90"),
         _T("Retention time in days for the records in notification log. All records older than specified will be deleted by housekeeping process."),
         _T("days"),
         'I', true, false, false, false));

   CHK_EXEC(SQLQuery(_T("DELETE FROM config WHERE var_name='JobHistoryRetentionTime'")));

   CHK_EXEC(SetMinorSchemaVersion(11));
   return true;
}

/**
 * Upgrade from 39.9 to 39.10
 */
static bool H_UpgradeFromV9()
{
   CHK_EXEC(CreateTable(
         _T("CREATE TABLE network_map_deleted_nodes (")
         _T("   map_id integer not null,")
         _T("   object_id integer not null,")
         _T("   element_index integer not null,")
         _T("   position_x integer not null,")
         _T("   position_y integer not null,")
         _T("PRIMARY KEY(map_id, object_id))")));

   CHK_EXEC(SetMinorSchemaVersion(10));
   return true;
}

/**
 * Convert network map links to new format
 */
static bool ConvertMapLinks()
{
   DB_HANDLE hdb = ConnectToDatabase();
   if (hdb == nullptr)
      return false;

   DB_UNBUFFERED_RESULT hResult = DBSelectUnbuffered(hdb, _T("SELECT map_id,element1,element2,link_type,link_name,connector_name1,connector_name2,flags,element_data FROM network_map_links"));
   if (hResult == nullptr)
   {
      DBDisconnect(hdb);
      return false;
   }

   DB_STATEMENT hStmt = DBPrepare(g_dbHandle, _T("INSERT INTO new_network_map_links (map_id,link_id,element1,element2,link_type,link_name,connector_name1,connector_name2,flags,color_source,color,element_data) VALUES (?,?,?,?,?,?,?,?,?,?,?,?)"), true);
   if (hStmt != nullptr)
   {
      uint32_t id = 1;
      while(DBFetch(hResult))
      {
         int32_t color = 0, colorSource = 0;
         char *xml = DBGetFieldUTF8(hResult, 8, nullptr, 0);
         if (xml != nullptr)
         {
            Config config;
            if (config.loadXmlConfigFromMemory(xml, strlen(xml), nullptr, "config", false))
            {
               color = config.getValueAsInt(_T("/color"), 0);
               colorSource = config.getValueAsInt(_T("/colorSource"), 0);
            }
            MemFree(xml);
         }

         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, DBGetFieldULong(hResult, 0));
         DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, id++);
         DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, DBGetFieldULong(hResult, 1));
         DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, DBGetFieldULong(hResult, 2));
         DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, DBGetFieldULong(hResult, 3));
         DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, DBGetField(hResult, 4, nullptr, 0), DB_BIND_DYNAMIC);
         DBBind(hStmt, 7, DB_SQLTYPE_VARCHAR, DBGetField(hResult, 5, nullptr, 0), DB_BIND_DYNAMIC);
         DBBind(hStmt, 8, DB_SQLTYPE_VARCHAR, DBGetField(hResult, 6, nullptr, 0), DB_BIND_DYNAMIC);
         DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, DBGetFieldULong(hResult, 7));
         DBBind(hStmt, 10, DB_SQLTYPE_INTEGER, colorSource);
         DBBind(hStmt, 11, DB_SQLTYPE_INTEGER, color);
         DBBind(hStmt, 12, DB_SQLTYPE_VARCHAR, DBGetField(hResult, 8, nullptr, 0), DB_BIND_DYNAMIC);

         if (!DBExecute(hStmt) && !g_ignoreErrors)
         {
            DBFreeStatement(hStmt);
            DBFreeResult(hResult);
            DBDisconnect(hdb);
            return false;
         }
      }
      DBFreeStatement(hStmt);
   }
   else
   {
      DBFreeResult(hResult);
      DBDisconnect(hdb);
      return false;
   }
   DBFreeResult(hResult);

   DBDisconnect(hdb);
   return true;
}

/**
 * Convert network map links to new format (SQLite version)
 */
static bool ConvertMapLinks_SQLite()
{
   DB_RESULT hResult = SQLSelect(_T("SELECT map_id,element1,element2,link_type,link_name,connector_name1,connector_name2,flags,element_data FROM network_map_links"));
   if (hResult == nullptr)
      return false;

   DB_STATEMENT hStmt = DBPrepare(g_dbHandle, _T("INSERT INTO new_network_map_links (map_id,link_id,element1,element2,link_type,link_name,connector_name1,connector_name2,flags,color_source,color,element_data) VALUES (?,?,?,?,?,?,?,?,?,?,?,?)"), true);
   if (hStmt != nullptr)
   {
      uint32_t id = 1;
      int count = DBGetNumRows(hResult);
      for(int row = 0; row < count; row++)
      {
         int32_t color = 0, colorSource = 0;
         char *xml = DBGetFieldUTF8(hResult, row, 8, nullptr, 0);
         if (xml != nullptr)
         {
            Config config;
            if (config.loadXmlConfigFromMemory(xml, strlen(xml), nullptr, "config", false))
            {
               color = config.getValueAsInt(_T("/color"), 0);
               colorSource = config.getValueAsInt(_T("/colorSource"), 0);
            }
            MemFree(xml);
         }

         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, DBGetFieldULong(hResult, row, 0));
         DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, id++);
         DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, DBGetFieldULong(hResult, row, 1));
         DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, DBGetFieldULong(hResult, row, 2));
         DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, DBGetFieldULong(hResult, row, 3));
         DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, DBGetField(hResult, row, 4, nullptr, 0), DB_BIND_DYNAMIC);
         DBBind(hStmt, 7, DB_SQLTYPE_VARCHAR, DBGetField(hResult, row, 5, nullptr, 0), DB_BIND_DYNAMIC);
         DBBind(hStmt, 8, DB_SQLTYPE_VARCHAR, DBGetField(hResult, row, 6, nullptr, 0), DB_BIND_DYNAMIC);
         DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, DBGetFieldULong(hResult, row, 7));
         DBBind(hStmt, 10, DB_SQLTYPE_INTEGER, colorSource);
         DBBind(hStmt, 11, DB_SQLTYPE_INTEGER, color);
         DBBind(hStmt, 12, DB_SQLTYPE_VARCHAR, DBGetField(hResult, row, 8, nullptr, 0), DB_BIND_DYNAMIC);

         if (!DBExecute(hStmt) && !g_ignoreErrors)
         {
            DBFreeStatement(hStmt);
            DBFreeResult(hResult);
            return false;
         }
      }
      DBFreeStatement(hStmt);
   }
   else
   {
      DBFreeResult(hResult);
      return false;
   }
   DBFreeResult(hResult);

   return true;
}

/**
 * Upgrade from 39.8 to 39.9
 */
static bool H_UpgradeFromV8()
{
   CHK_EXEC(CreateTable(
      _T("CREATE TABLE new_network_map_links (")
      _T("   map_id integer not null,")
      _T("   link_id integer not null,")
      _T("   element1 integer not null,")
      _T("   element2 integer not null,")
      _T("   link_type integer not null,")
      _T("   link_name varchar(255) null,")
      _T("   connector_name1 varchar(255) null,")
      _T("   connector_name2 varchar(255) null,")
      _T("   element_data $SQL:TEXT null,")
      _T("   flags integer not null,")
      _T("   color_source integer not null,")
      _T("   color integer not null,")
      _T("   color_provider varchar(255) null,")
      _T("   PRIMARY KEY(map_id,link_id))")));

   CHK_EXEC((g_dbSyntax == DB_SYNTAX_SQLITE) ? ConvertMapLinks_SQLite() : ConvertMapLinks());
   CHK_EXEC(SQLQuery(_T("DROP TABLE network_map_links")));
   CHK_EXEC(DBRenameTable(g_dbHandle, _T("new_network_map_links"), _T("network_map_links")));
   CHK_EXEC(SQLQuery(_T("CREATE INDEX idx_network_map_links_map_id ON network_map_links(map_id)")));

   CHK_EXEC(SetMinorSchemaVersion(9));
   return true;
}

/**
 * Upgrade from 39.7 to 39.8
 */
static bool H_UpgradeFromV7()
{
   CHK_EXEC(CreateEventTemplate(EVENT_DUPLICATE_MAC_ADDRESS, _T("SYS_DUPLICATE_MAC_ADDRESS"),
         EVENT_SEVERITY_MAJOR, EF_LOG, _T("c19fbb37-98c9-43a5-90f2-7a54ba9116fa"),
         _T("Duplicate MAC address found (%1 on %2)"),
         _T("Generated when duplicate MAC address found.\r\n")
         _T("Parameters:\r\n")
         _T("   1) MAC address\r\n")
         _T("   2) List of interfaces where MAC address was found")));

   int ruleId = NextFreeEPPruleID();

   TCHAR query[1024];
   _sntprintf(query, 1024, _T("INSERT INTO event_policy (rule_id,rule_guid,flags,comments,alarm_message,alarm_severity,alarm_key,script,alarm_timeout,alarm_timeout_event) ")
                           _T("VALUES (%d,'fb915441-f9d8-4dab-b9e6-1296f3f8ec9f',7944,'Generate alarm when duplicate MAC address detected','%%m',5,'DUPLICATE_MAC_ADDRESS_%%1','',0,%d)"),
         ruleId, EVENT_ALARM_TIMEOUT);
   CHK_EXEC(SQLQuery(query));

   _sntprintf(query, 1024, _T("INSERT INTO policy_event_list (rule_id,event_code) VALUES (%d,%d)"), ruleId, EVENT_DUPLICATE_MAC_ADDRESS);
   CHK_EXEC(SQLQuery(query));

   CHK_EXEC(SetMinorSchemaVersion(8));
   return true;
}

/**
 * Upgrade from 39.6 to 39.7
 */
static bool H_UpgradeFromV6()
{
   // This upgrade was creating notification and server action logs with incorrect structure
   // Now those tables created in upgrade procedure for version 39.10
   CHK_EXEC(SetMinorSchemaVersion(7));
   return true;
}

/**
 * Upgrade from 39.5 to 39.6
 */
static bool H_UpgradeFromV5()
{
   CHK_EXEC(CreateConfigParam(_T("LDAP.Mapping.Email"),
         _T(""),
         _T("The name of an attribute whose value will be used as a user's email."),
         nullptr, 'S', true, false, false, false));
   CHK_EXEC(CreateConfigParam(_T("LDAP.Mapping.PhoneNumber"),
         _T(""),
         _T("The name of an attribute whose value will be used as a user's phone number."),
         nullptr, 'S', true, false, false, false));

   static const TCHAR *batch =
         _T("UPDATE config SET var_name='LDAP.Mapping.Description' WHERE var_name='LDAP.MappingDescription'\n")
         _T("UPDATE config SET var_name='LDAP.Mapping.FullName' WHERE var_name='LDAP.MappingFullName'\n")
         _T("UPDATE config SET var_name='LDAP.Mapping.GroupName' WHERE var_name='LDAP.GroupMappingName'\n")
         _T("UPDATE config SET var_name='LDAP.Mapping.UserName' WHERE var_name='LDAP.UserMappingName'\n")
         _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   CHK_EXEC(SQLQuery(_T("INSERT INTO script_library (guid,script_id,script_name,script_code) ")
         _T("VALUES ('4fae91b5-8802-4f6c-aace-a03f9f7fa8ef',25,'Hook::LDAPSynchronization','/* Available global variables:\r\n *  $ldapObject - LDAP object being synchronized (object of ''LDAPObject'' class)\r\n *\r\n * Expected return value:\r\n *  true/false - boolean - whether processing of this LDAP object should continue\r\n */\r\n')")));

   CHK_EXEC(SetMinorSchemaVersion(6));
   return true;
}

/**
 * Upgrade from 39.4 to 39.5
 */
static bool H_UpgradeFromV4()
{
   CHK_EXEC(CreateConfigParam(_T("Geolocation.History.RetentionTime"),
         _T("90"),
         _T("Retention time in days for object's geolocation history. All records older than specified will be deleted by housekeeping process."),
         _T("days"),
         'I', true, false, false, false));

   CHK_EXEC(SetMinorSchemaVersion(5));
   return true;
}

/**
 * Upgrade from 39.3 to 39.4
 */
static bool H_UpgradeFromV3()
{
   CHK_EXEC(SQLQuery(_T("ALTER TABLE input_fields ADD flags integer")));

   DB_RESULT hResult = SQLSelect(_T("SELECT category,owner_id,name,config FROM input_fields"));
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         TCHAR category[2], name[MAX_OBJECT_NAME];
         DBGetField(hResult, i, 0, category, 2);
         uint32_t ownerId = DBGetFieldLong(hResult, i, 1);
         DBGetField(hResult, i, 2, name, MAX_OBJECT_NAME);
         TCHAR *config = DBGetField(hResult, i, 3, nullptr, 0);

         TCHAR query[1024];
         _sntprintf(query, 1024, _T("UPDATE input_fields SET flags=%d WHERE category='%s' AND owner_id=%u AND name=%s"),
                  (_tcsstr(config, _T("<validatePassword>true</validatePassword>")) != nullptr) ? 1 : 0, category, ownerId, DBPrepareString(g_dbHandle, name).cstr());
         MemFree(config);

         if (!SQLQuery(query) && !g_ignoreErrors)
         {
            DBFreeResult(hResult);
            return false;
         }
      }
      DBFreeResult(hResult);
   }
   else if (!g_ignoreErrors)
   {
      return false;
   }

   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("input_fields"), _T("flags")));
   CHK_EXEC(DBDropColumn(g_dbHandle, _T("input_fields"), _T("config")));

   CHK_EXEC(SetMinorSchemaVersion(4));
   return true;
}

/**
 * Upgrade from 39.2 to 39.3
 */
static bool H_UpgradeFromV2()
{
   CHK_EXEC(DBDropPrimaryKey(g_dbHandle, _T("object_tools_input_fields")));
   CHK_EXEC(DBRenameTable(g_dbHandle, _T("object_tools_input_fields"), _T("input_fields")));
   CHK_EXEC(DBRenameColumn(g_dbHandle, _T("input_fields"), _T("tool_id"), _T("owner_id")));
   CHK_EXEC(SQLQuery(_T("ALTER TABLE input_fields ADD category char(1)")));
   CHK_EXEC(SQLQuery(_T("UPDATE input_fields SET category='T'")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("input_fields"), _T("category")));
   CHK_EXEC(DBAddPrimaryKey(g_dbHandle, _T("input_fields"), _T("category,owner_id,name")));
   CHK_EXEC(SetMinorSchemaVersion(3));
   return true;
}

/**
 * Upgrade from 39.1 to 39.2
 */
static bool H_UpgradeFromV1()
{
   if (DBIsTableExist(g_dbHandle, _T("zmq_subscription")) == DBIsTableExist_Found)
      CHK_EXEC(SQLQuery(_T("DROP TABLE zmq_subscription")));
   CHK_EXEC(SetMinorSchemaVersion(2));
   return true;
}

/**
 * Upgrade from 39.0 to 39.1
 */
static bool H_UpgradeFromV0()
{
   CHK_EXEC(CreateTable(
         _T("CREATE TABLE object_queries (")
         _T("   id integer not null,")
         _T("   guid varchar(36) not null,")
         _T("   name varchar(63) not null,")
         _T("   description varchar(255) null,")
         _T("   script $SQL:TEXT null,")
         _T("PRIMARY KEY(id))")));

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
   { 17, 40, 0,  H_UpgradeFromV17 },
   { 16, 39, 17, H_UpgradeFromV16 },
   { 15, 39, 16, H_UpgradeFromV15 },
   { 14, 39, 15, H_UpgradeFromV14 },
   { 13, 39, 14, H_UpgradeFromV13 },
   { 12, 39, 13, H_UpgradeFromV12 },
   { 11, 39, 12, H_UpgradeFromV11 },
   { 10, 39, 11, H_UpgradeFromV10 },
   { 9,  39, 10, H_UpgradeFromV9  },
   { 8,  39, 9,  H_UpgradeFromV8  },
   { 7,  39, 8,  H_UpgradeFromV7  },
   { 6,  39, 7,  H_UpgradeFromV6  },
   { 5,  39, 6,  H_UpgradeFromV5  },
   { 4,  39, 5,  H_UpgradeFromV4  },
   { 3,  39, 4,  H_UpgradeFromV3  },
   { 2,  39, 3,  H_UpgradeFromV2  },
   { 1,  39, 2,  H_UpgradeFromV1  },
   { 0,  39, 1,  H_UpgradeFromV0  },
   { 0,  0,  0,  nullptr          }
};

/**
 * Upgrade database to new version
 */
bool MajorSchemaUpgrade_V39()
{
   int32_t major, minor;
   if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
      return false;

   while(major == 39)
   {
      // Find upgrade procedure
      int i;
      for(i = 0; s_dbUpgradeMap[i].upgradeProc != nullptr; i++)
         if (s_dbUpgradeMap[i].version == minor)
            break;
      if (s_dbUpgradeMap[i].upgradeProc == nullptr)
      {
         _tprintf(_T("Unable to find upgrade procedure for version 39.%d\n"), minor);
         return false;
      }
      _tprintf(_T("Upgrading from version 39.%d to %d.%d\n"), minor, s_dbUpgradeMap[i].nextMajor, s_dbUpgradeMap[i].nextMinor);
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
