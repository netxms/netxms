/*
** nxdbmgr - NetXMS database manager
** Copyright (C) 2020-2022 Raden Solutions
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
** File: upgrade_v40.cpp
**
**/

#include "nxdbmgr.h"
#include <nxevent.h>

/**
 * Upgrade from 40.103 to 41.0
 */
static bool H_UpgradeFromV103()
{
   CHK_EXEC(SetMajorSchemaVersion(41, 0));
   return true;
}

/**
 * Upgrade from 40.102 to 40.103
 */
static bool H_UpgradeFromV102()
{
   if (GetSchemaLevelForMajorVersion(39) < 16)
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

      CHK_EXEC(SetSchemaLevelForMajorVersion(39, 16));
   }

   CHK_EXEC(SetMinorSchemaVersion(103));
   return true;
}

/**
 * Upgrade from 40.101 to 40.102
 */
static bool H_UpgradeFromV101()
{
   CHK_EXEC(SQLQuery(_T("INSERT INTO script_library (guid,script_id,script_name,script_code) ")
         _T("VALUES ('f8bfa009-d4e4-4443-8bad-3ded09bdeb92',26,'Hook::Login','/* Available global variables:\r\n *  $user - user object (object of ''User'' class)\r\n *  $session - session object (object of ''ClientSession'' class)\r\n *\r\n * Expected return value:\r\n *  true/false - boolean - whether login for this session should continue\r\n */\r\n')")));
   CHK_EXEC(SetMinorSchemaVersion(102));
   return true;
}

/**
 * Upgrade from 40.100 to 40.101
 */
static bool H_UpgradeFromV100()
{
   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET message='Duplicate MAC address found (%1 on %2)' WHERE guid='c19fbb37-98c9-43a5-90f2-7a54ba9116fa' AND message='Duplicate MAC address found (%1on %2)'")));
   CHK_EXEC(SetMinorSchemaVersion(101));
   return true;
}

/**
 * Upgrade from 40.99 to 40.100
 */
static bool H_UpgradeFromV99()
{
   if (DBIsTableExist(g_dbHandle, _T("zmq_subscription")) == DBIsTableExist_Found)
   {
      CHK_EXEC(SQLQuery(_T("DROP TABLE zmq_subscription")));
   }
   CHK_EXEC(SetMinorSchemaVersion(100));
   return true;
}

/**
 * Upgrade from 40.98 to 40.99
 */
static bool H_UpgradeFromV98()
{
   CHK_EXEC(CreateTable(
         _T("CREATE TABLE alarm_state_changes (")
         _T("  record_id $SQL:INT64 not null,")
         _T("  alarm_id integer not null,")
         _T("  prev_state integer not null,")
         _T("  new_state integer not null,")
         _T("  change_time integer not null,")
         _T("  prev_state_duration integer not null,")
         _T("  change_by integer not null,")
         _T("PRIMARY KEY(record_id))")));

   static const TCHAR* batch =
      _T("CREATE INDEX idx_alarm_state_changes_by_id ON alarm_state_changes(alarm_id)\n")
      _T("ALTER TABLE alarms ADD last_state_change_time integer\n")
      _T("UPDATE alarms SET last_state_change_time=last_change_time\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("alarms"), _T("last_state_change_time")));

   CHK_EXEC(SetMinorSchemaVersion(99));
   return true;
}

/**
 * Upgrade from 40.97 to 40.98
 */
static bool H_UpgradeFromV97()
{
   CHK_EXEC(SQLQuery(_T("UPDATE config SET var_name='ICMP.PingTimeout' WHERE var_name='ICMP.PingTmeout'")));
   CHK_EXEC(SetMinorSchemaVersion(98));
   return true;
}

/**
 * Upgrade from 40.96 to 40.97
 */
static bool H_UpgradeFromV96()
{
   static const TCHAR* batch =
      _T("ALTER TABLE business_service_checks ADD prototype_service_id integer\n")
      _T("ALTER TABLE business_service_checks ADD prototype_check_id integer\n")
      _T("UPDATE business_service_checks SET prototype_service_id=service_id,prototype_check_id=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("business_service_checks"), _T("prototype_service_id")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("business_service_checks"), _T("prototype_check_id")));
   CHK_EXEC(SetMinorSchemaVersion(97));
   return true;
}

/**
 * Upgrade from 40.95 to 40.96
 */
static bool H_UpgradeFromV95()
{
   static const TCHAR* batch =
      _T("ALTER TABLE items ADD state_flags integer\n")
      _T("ALTER TABLE dc_tables ADD state_flags integer\n")
      _T("UPDATE items SET state_flags=0\n")
      _T("UPDATE dc_tables SET state_flags=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("items"), _T("state_flags")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("dc_tables"), _T("state_flags")));
   CHK_EXEC(SetMinorSchemaVersion(96));
   return true;
}

/**
 * Upgrade from 40.94 to 40.95
 */
static bool H_UpgradeFromV94()
{
   static const TCHAR *batch =
      _T("ALTER TABLE websvc_definitions ADD http_request_method integer\n")
      _T("ALTER TABLE websvc_definitions ADD request_data varchar(4000)\n")
      _T("UPDATE websvc_definitions SET http_request_method=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("websvc_definitions"), _T("http_request_method")));
   CHK_EXEC(SetMinorSchemaVersion(95));
   return true;
}

/**
 * Upgrade from 40.93 to 40.94
 */
static bool H_UpgradeFromV93()
{
   CHK_EXEC(CreateConfigParam(_T("BusinessServices.Check.AutobindClassFilter"), _T("AccessPoint,Cluster,Interface,NetworkService,Node"),
         _T("Class filter for automatic creation of business service checks."), nullptr, 'S', true, false, false, false));
   CHK_EXEC(CreateConfigParam(_T("Objects.AutobindPollingInterval"), _T("3600"),
         _T("Interval in seconds between automatic object binding polls."), _T("seconds"), 'I', true, false, false, false));
   static const TCHAR *batch =
      _T("UPDATE config SET need_server_restart=0 WHERE var_name IN ('BusinessServices.Check.Threshold.DataCollection','BusinessServices.Check.Threshold.Objects','Objects.ConfigurationPollingInterval','Objects.StatusPollingInterval')\n")
      _T("UPDATE config SET data_type='S' WHERE var_name='SNMP.Codepage' OR var_name='Syslog.Codepage'\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(SetMinorSchemaVersion(94));
   return true;
}

/**
 * Upgrade from 40.92 to 40.93
 */
static bool H_UpgradeFromV92()
{
   CHK_EXEC(CreateEventTemplate(EVENT_TOO_MANY_SCRIPT_ERRORS, _T("SYS_TOO_MANY_SCRIPT_ERRORS"),
            SEVERITY_WARNING, EF_LOG, _T("c71f2aa6-9345-43d4-90da-c5c3ee55f30d"),
            _T("Too many script errors - script error reporting temporarily suspended"),
            _T("Generated when there are too many script errors within short time period.")
            ));
   CHK_EXEC(SetMinorSchemaVersion(93));
   return true;
}

/**
 * Upgrade from 40.91 to 40.92
 */
static bool H_UpgradeFromV91()
{
   DB_RESULT hResult = SQLSelect(_T("SELECT item_id,perftab_settings FROM items WHERE perftab_settings LIKE '%{instance}%'"));
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      if (count > 0)
      {
         DB_STATEMENT hStmt = DBPrepare(g_dbHandle, _T("UPDATE items SET perftab_settings=? WHERE item_id=?"), count > 1);
         if (hStmt != nullptr)
         {
            StringBuffer sb;
            for(int i = 0; i < count; i++)
            {
               sb.clear();
               sb.appendPreallocated(DBGetField(hResult, i, 1, nullptr, 0));
               sb.replace(_T("{instance}"), _T("{instance-name}"));
               DBBind(hStmt, 1, DB_SQLTYPE_TEXT, sb, DB_BIND_STATIC);
               DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, DBGetFieldULong(hResult, i, 0));
               if (!SQLExecute(hStmt))
               {
                  if (!g_ignoreErrors)
                  {
                     DBFreeStatement(hStmt);
                     DBFreeResult(hResult);
                     return false;
                  }
               }
            }
            DBFreeStatement(hStmt);
         }
         else if (!g_ignoreErrors)
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
   CHK_EXEC(SetMinorSchemaVersion(92));
   return true;
}

/**
 * Upgrade from 40.90 to 40.91
 */
static bool H_UpgradeFromV90()
{
   CHK_EXEC(CreateEventTemplate(EVENT_AGENT_FILE_CHANGED, _T("SYS_AGENT_FILE_CHANGED"),
            SEVERITY_WARNING, EF_LOG, _T("04c3e538-a668-4c27-a238-d1891b34f3df"),
            _T("File %1 changed"),
            _T("Generated when agent monitored file is changed.\r\n")
            _T("Parameters:\r\n")
            _T("   1) Path to changed file")
            ));
   
   CHK_EXEC(CreateEventTemplate(EVENT_AGENT_FILE_ADDED, _T("SYS_AGENT_FILE_ADDED"),
            SEVERITY_WARNING, EF_LOG, _T("2f139a92-cb1c-4974-8a7c-0ad714a04394"),
            _T("File %1 added"),
            _T("Generated when new file in agent monitored directory is created.\r\n")
            _T("Parameters:\r\n")
            _T("   1) Path to new file")
            ));

   CHK_EXEC(CreateEventTemplate(EVENT_AGENT_FILE_DELETED, _T("SYS_AGENT_FILE_DELETED"),
            SEVERITY_WARNING, EF_LOG, _T("11c6a29d-94e8-4eb6-a177-deece3a83483"),
            _T("File %1 deleted"),
            _T("Generated when agent monitored file is deleted.\r\n")
            _T("Parameters:\r\n")
            _T("   1) Path to deleted file")
            ));

   CHK_EXEC(SetMinorSchemaVersion(91));
   return true;
}

/**
 * Upgrade from 40.89 to 40.90
 */
static bool H_UpgradeFromV89()
{
   static const TCHAR *batch =
      _T("ALTER TABLE policy_action_list ADD timer_delay_text varchar(127)\n")
      _T("ALTER TABLE policy_action_list ADD snooze_time_text varchar(127)\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   switch(g_dbSyntax)
   {
      case DB_SYNTAX_MYSQL:
         CHK_EXEC(SQLQuery(_T("UPDATE policy_action_list SET timer_delay_text=CAST(timer_delay AS char),snooze_time_text=CAST(snooze_time AS char)")));
         break;
      case DB_SYNTAX_ORACLE:
         CHK_EXEC(SQLQuery(_T("UPDATE policy_action_list SET timer_delay_text=CAST(timer_delay AS varchar(127)),snooze_time_text=CAST(snooze_time AS varchar(127))")));
         break;
      default:
         CHK_EXEC(SQLQuery(_T("UPDATE policy_action_list SET timer_delay_text=CAST(timer_delay AS varchar),snooze_time_text=CAST(snooze_time AS varchar)")));
         break;
   }
   CHK_EXEC(DBDropColumn(g_dbHandle, _T("policy_action_list"), _T("timer_delay")));
   CHK_EXEC(DBDropColumn(g_dbHandle, _T("policy_action_list"), _T("snooze_time")));
   CHK_EXEC(DBRenameColumn(g_dbHandle, _T("policy_action_list"), _T("timer_delay_text"), _T("timer_delay")));
   CHK_EXEC(DBRenameColumn(g_dbHandle, _T("policy_action_list"), _T("snooze_time_text"), _T("snooze_time")));
   CHK_EXEC(SetMinorSchemaVersion(90));
   return true;
}

/**
 * Upgrade from 40.88 to 40.89
 */
static bool H_UpgradeFromV88()
{
   CHK_EXEC(CreateEventTemplate(EVENT_NOTIFICATION_CHANNEL_DOWN, _T("SYS_NOTIFICATION_CHANNEL_DOWN"),
      EVENT_SEVERITY_MAJOR, EF_LOG, _T("5696d263-0a1c-4c3b-b5bb-d6be8993ac0d"),
      _T("Notification channel %1 is down"),
      _T("Generated when notification channel fails health check.\r\n")
      _T("Parameters:\r\n")
		_T("   1) Notification channel name\r\n")
		_T("   2) Notification channel driver name\r\n")));

   CHK_EXEC(CreateEventTemplate(EVENT_NOTIFICATION_CHANNEL_UP, _T("SYS_NOTIFICATION_CHANNEL_UP"),
      EVENT_SEVERITY_NORMAL, EF_LOG, _T("19536ccb-96ed-417a-8695-94dc6d504d73"),
      _T("Notification channel %1 is up"),
      _T("Generated when notification channel passes health check.\r\n")
      _T("Parameters:\r\n")
		_T("   1) Notification channel name\r\n")
		_T("   2) Notification channel driver name\r\n")));

   int ruleId = NextFreeEPPruleID();

   TCHAR query[1024];
   _sntprintf(query, 1024, _T("INSERT INTO event_policy (rule_id,rule_guid,flags,comments,alarm_message,alarm_severity,alarm_key,script,alarm_timeout,alarm_timeout_event) ")
                           _T("VALUES (%d,'96061edf-a46f-407a-a049-a55fa1ffb7e8',7944,'Generated alarm when notification channel is down','%%m',5,'NOTIFICATION_CHANNEL_DOWN_%%1','',0,%d)"),
         ruleId, EVENT_ALARM_TIMEOUT);
   CHK_EXEC(SQLQuery(query));

   _sntprintf(query, 1024, _T("INSERT INTO policy_event_list (rule_id,event_code) VALUES (%d,%d)"), ruleId, EVENT_NOTIFICATION_CHANNEL_DOWN);
   CHK_EXEC(SQLQuery(query));

   ruleId++;

   _sntprintf(query, 1024, _T("INSERT INTO event_policy (rule_id,rule_guid,flags,comments,alarm_message,alarm_severity,alarm_key,script,alarm_timeout,alarm_timeout_event) ")
                           _T("VALUES (%d,'75640293-d630-4bfc-9b5e-9655cf59fd00',7944,'Terminate notificaion channel down alarm when notification channel is back up','%%m',6,'NOTIFICATION_CHANNEL_DOWN_%%1','',0,%d)"),
         ruleId, EVENT_ALARM_TIMEOUT);
   CHK_EXEC(SQLQuery(query));

   _sntprintf(query, 1024, _T("INSERT INTO policy_event_list (rule_id,event_code) VALUES (%d,%d)"), ruleId, EVENT_NOTIFICATION_CHANNEL_UP);
   CHK_EXEC(SQLQuery(query));

   CHK_EXEC(SetMinorSchemaVersion(89));
   return true;
}

/**
 * Upgrade from 40.87 to 40.88
 */
static bool H_UpgradeFromV87()
{
   if (GetSchemaLevelForMajorVersion(39) < 15)
   {
      static const TCHAR *batch =
         _T("ALTER TABLE agent_pkg ADD pkg_type varchar(15)\n")
         _T("ALTER TABLE agent_pkg ADD command varchar(255)\n")
         _T("UPDATE agent_pkg SET pkg_type='agent-installer'\n")
         _T("<END>");
      CHK_EXEC(SQLBatch(batch));
      CHK_EXEC(SetSchemaLevelForMajorVersion(39, 15));
   }
   CHK_EXEC(SetMinorSchemaVersion(88));
   return true;
}

/**
 * Upgrade from 40.86 to 40.87
 */
static bool H_UpgradeFromV86()
{
   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET description=")
		   _T("'Generated when server encounters NXSL script execution error.\r\n")
		   _T("Parameters:\r\n")
		   _T("   1) Script name\r\n")
		   _T("   2) Error text\r\n")
		   _T("   3) DCI ID if script is DCI related script, or 0 otherwise\r\n")
		   _T("   4) Related object ID if applicable, or 0 otherwise\r\n")
		   _T("   5) Context'")
		   _T(" WHERE guid='2cc78efe-357a-4278-932f-91e36754c775'")));
   CHK_EXEC(SetMinorSchemaVersion(87));
   return true;
}

/**
 * Upgrade from 40.85 to 40.86
 */
static bool H_UpgradeFromV85()
{
   CHK_EXEC(SQLQuery(_T("ALTER TABLE nodes ADD snmp_codepage varchar(15)\n")));
   CHK_EXEC(CreateConfigParam(_T("SNMP.Codepage"), _T(""), _T("Default server SNMP codepage."), nullptr, 'S', true, false, false, false));
   CHK_EXEC(SetMinorSchemaVersion(86));
   return true;
}

/**
 * Upgrade from 40.84 to 40.85
 */
static bool H_UpgradeFromV84()
{
   if (GetSchemaLevelForMajorVersion(39) < 14)
   {
      static const TCHAR *batch =
         _T("ALTER TABLE object_properties ADD region varchar(63)\n")
         _T("ALTER TABLE object_properties ADD district varchar(63)\n")
         _T("<END>");
      CHK_EXEC(SQLBatch(batch));
      CHK_EXEC(SetSchemaLevelForMajorVersion(39, 14));
   }
   CHK_EXEC(SetMinorSchemaVersion(85));
   return true;
}

/**
 * Upgrade from 40.83 to 40.84
 */
static bool H_UpgradeFromV83()
{
   CHK_EXEC(CreateConfigParam(_T("Syslog.AllowUnknownSources"), _T("0"),
         _T("Enable or disable processing of syslog messages from unknown sources"),
         nullptr, 'B', true, false, false, false));
   CHK_EXEC(SetMinorSchemaVersion(84));
   return true;
}

/**
 * Upgrade from 40.82 to 40.83
 */
static bool H_UpgradeFromV82()
{
   CHK_EXEC(SQLQuery(_T("ALTER TABLE nodes ADD syslog_codepage varchar(15)\n")));
   CHK_EXEC(CreateConfigParam(_T("Syslog.Codepage"), _T(""), _T("Default server syslog codepage."), nullptr, 'S', true, false, false, false));
   CHK_EXEC(SetMinorSchemaVersion(83));
   return true;
}

/**
 * Upgrade from 40.81 to 40.82
 */
static bool H_UpgradeFromV81()
{
   CHK_EXEC(SQLQuery(_T("DELETE FROM config WHERE var_name='AgentDefaultSharedSecret' OR var_name='EnableAuditLog' OR var_name='MailEncoding' OR var_name LIKE 'SMTP.%'")));

   static const TCHAR *batch =
      _T("UPDATE config SET var_name='Agent.CommandTimeout' WHERE var_name='AgentCommandTimeout'\n")
      _T("UPDATE config SET var_name='Agent.DefaultCacheMode' WHERE var_name='DefaultAgentCacheMode'\n")
      _T("UPDATE config SET var_name='Agent.DefaultEncryptionPolicy' WHERE var_name='DefaultEncryptionPolicy'\n")
      _T("UPDATE config SET var_name='Agent.DefaultProtocolCompressionMode' WHERE var_name='DefaultAgentProtocolCompressionMode'\n")
      _T("UPDATE config SET var_name='Agent.EnableRegistration' WHERE var_name='EnableAgentRegistration'\n")
      _T("UPDATE config SET var_name='Agent.Upgrade.NumberOfThreads' WHERE var_name='NumberOfUpgradeThreads'\n")
      _T("UPDATE config SET var_name='Agent.Upgrade.WaitTime' WHERE var_name='AgentUpgradeWaitTime'\n")
      _T("UPDATE config SET var_name='Alarms.DeleteAlarmsOfDeletedObject' WHERE var_name='DeleteAlarmsOfDeletedObject'\n")
      _T("UPDATE config SET var_name='Alarms.EnableTimedAck' WHERE var_name='EnableTimedAlarmAck'\n")
      _T("UPDATE config SET var_name='Alarms.HistoryRetentionTime' WHERE var_name='AlarmHistoryRetentionTime'\n")
      _T("UPDATE config SET var_name='Alarms.StrictStatusFlow' WHERE var_name='StrictAlarmStatusFlow'\n")
      _T("UPDATE config SET var_name='Alarms.SummaryEmail.Enable' WHERE var_name='EnableAlarmSummaryEmails'\n")
      _T("UPDATE config SET var_name='Alarms.SummaryEmail.Recipients' WHERE var_name='AlarmSummaryEmailRecipients'\n")
      _T("UPDATE config SET var_name='Alarms.SummaryEmail.Schedule' WHERE var_name='AlarmSummaryEmailSchedule'\n")
      _T("UPDATE config SET var_name='AuditLog.External.Facility' WHERE var_name='ExternalAuditFacility'\n")
      _T("UPDATE config SET var_name='AuditLog.External.Port' WHERE var_name='ExternalAuditPort'\n")
      _T("UPDATE config SET var_name='AuditLog.External.Server' WHERE var_name='ExternalAuditServer'\n")
      _T("UPDATE config SET var_name='AuditLog.External.Severity' WHERE var_name='ExternalAuditSeverity'\n")
      _T("UPDATE config SET var_name='AuditLog.External.Tag' WHERE var_name='ExternalAuditTag'\n")
      _T("UPDATE config SET var_name='AuditLog.RetentionTime' WHERE var_name='AuditLogRetentionTime'\n")
      _T("UPDATE config SET var_name='Client.DashboardDataExport.EnableInterpolation' WHERE var_name='DashboardDataExportEnableInterpolation'\n")
      _T("UPDATE config SET var_name='Client.DefaultConsoleDateFormat' WHERE var_name='DefaultConsoleDateFormat'\n")
      _T("UPDATE config SET var_name='Client.DefaultConsoleShortTimeFormat' WHERE var_name='DefaultConsoleShortTimeFormat'\n")
      _T("UPDATE config SET var_name='Client.DefaultConsoleTimeFormat' WHERE var_name='DefaultConsoleTimeFormat'\n")
      _T("UPDATE config SET var_name='Client.KeepAliveInterval' WHERE var_name='KeepAliveInterval'\n")
      _T("UPDATE config SET var_name='Client.ListenerPort' WHERE var_name='ClientListenerPort'\n")
      _T("UPDATE config SET var_name='Client.TileServerURL' WHERE var_name='TileServerURL'\n")
      _T("UPDATE config SET var_name='DataCollection.ApplyDCIFromTemplateToDisabledDCI' WHERE var_name='ApplyDCIFromTemplateToDisabledDCI'\n")
      _T("UPDATE config SET var_name='DataCollection.DefaultDCIPollingInterval' WHERE var_name='DefaultDCIPollingInterval'\n")
      _T("UPDATE config SET var_name='DataCollection.DefaultDCIRetentionTime' WHERE var_name='DefaultDCIRetentionTime'\n")
      _T("UPDATE config SET var_name='DataCollection.InstancePollingInterval' WHERE var_name='InstancePollingInterval'\n")
      _T("UPDATE config SET var_name='DataCollection.OfflineDataRelevanceTime' WHERE var_name='OfflineDataRelevanceTime'\n")
      _T("UPDATE config SET var_name='DataCollection.ThresholdRepeatInterval' WHERE var_name='ThresholdRepeatInterval'\n")
      _T("UPDATE config SET var_name='Events.DeleteEventsOfDeletedObject' WHERE var_name='DeleteEventsOfDeletedObject'\n")
      _T("UPDATE config SET var_name='Events.LogRetentionTime' WHERE var_name='EventLogRetentionTime'\n")
      _T("UPDATE config SET var_name='Events.ReceiveForwardedEvents' WHERE var_name='ReceiveForwardedEvents'\n")
      _T("UPDATE config SET var_name='NetworkDiscovery.FilterFlags' WHERE var_name='DiscoveryFilterFlags'\n")
      _T("UPDATE config SET var_name='NetworkDiscovery.Filter' WHERE var_name='DiscoveryFilter'\n")
      _T("UPDATE config SET var_name='NetworkDiscovery.UseDNSNameForDiscoveredNodes' WHERE var_name='UseDNSNameForDiscoveredNodes'\n")
      _T("UPDATE config SET var_name='NetworkDiscovery.UseFQDNForNodeNames' WHERE var_name='UseFQDNForNodeNames'\n")
      _T("UPDATE config SET var_name='NetworkDiscovery.UseSNMPTraps' WHERE var_name='UseSNMPTrapsForDiscovery'\n")
      _T("UPDATE config SET var_name='NetworkDiscovery.UseSyslog' WHERE var_name='UseSyslogForDiscovery'\n")
      _T("UPDATE config SET var_name='Objects.Conditions.PollingInterval' WHERE var_name='ConditionPollingInterval'\n")
      _T("UPDATE config SET var_name='Objects.ConfigurationPollingInterval' WHERE var_name='ConfigurationPollingInterval'\n")
      _T("UPDATE config SET var_name='Objects.DeleteUnreachableNodesPeriod' WHERE var_name='DeleteUnreachableNodesPeriod'\n")
      _T("UPDATE config SET var_name='Objects.EnableZoning' WHERE var_name='EnableZoning'\n")
      _T("UPDATE config SET var_name='Objects.NetworkMaps.DefaultBackgroundColor' WHERE var_name='DefaultMapBackgroundColor'\n")
      _T("UPDATE config SET var_name='Objects.PollCountForStatusChange' WHERE var_name='PollCountForStatusChange'\n")
      _T("UPDATE config SET var_name='Objects.StatusCalculation.CalculationAlgorithm' WHERE var_name='StatusCalculationAlgorithm'\n")
      _T("UPDATE config SET var_name='Objects.StatusCalculation.FixedStatusValue' WHERE var_name='FixedStatusValue'\n")
      _T("UPDATE config SET var_name='Objects.StatusCalculation.PropagationAlgorithm' WHERE var_name='StatusPropagationAlgorithm'\n")
      _T("UPDATE config SET var_name='Objects.StatusCalculation.Shift' WHERE var_name='StatusShift'\n")
      _T("UPDATE config SET var_name='Objects.StatusCalculation.SingleThreshold' WHERE var_name='StatusSingleThreshold'\n")
      _T("UPDATE config SET var_name='Objects.StatusCalculation.Thresholds' WHERE var_name='StatusThresholds'\n")
      _T("UPDATE config SET var_name='Objects.StatusCalculation.Translation' WHERE var_name='StatusTranslation'\n")
      _T("UPDATE config SET var_name='Objects.StatusPollingInterval' WHERE var_name='StatusPollingInterval'\n")
      _T("UPDATE config SET var_name='Objects.Subnets.DefaultSubnetMaskIPv4' WHERE var_name='DefaultSubnetMaskIPv4'\n")
      _T("UPDATE config SET var_name='Objects.Subnets.DefaultSubnetMaskIPv6' WHERE var_name='DefaultSubnetMaskIPv6'\n")
      _T("UPDATE config SET var_name='Objects.Subnets.DeleteEmpty' WHERE var_name='DeleteEmptySubnets'\n")
      _T("UPDATE config SET var_name='Objects.SyncInterval' WHERE var_name='SyncInterval'\n")
      _T("UPDATE config SET var_name='RADIUS.AuthMethod' WHERE var_name='RADIUSAuthMethod'\n")
      _T("UPDATE config SET var_name='RADIUS.NumRetries' WHERE var_name='RADIUSNumRetries'\n")
      _T("UPDATE config SET var_name='RADIUS.Port' WHERE var_name='RADIUSPort'\n")
      _T("UPDATE config SET var_name='RADIUS.SecondaryPort' WHERE var_name='RADIUSSecondaryPort'\n")
      _T("UPDATE config SET var_name='RADIUS.SecondarySecret' WHERE var_name='RADIUSSecondarySecret'\n")
      _T("UPDATE config SET var_name='RADIUS.SecondaryServer' WHERE var_name='RADIUSSecondaryServer'\n")
      _T("UPDATE config SET var_name='RADIUS.Secret' WHERE var_name='RADIUSSecret'\n")
      _T("UPDATE config SET var_name='RADIUS.Server' WHERE var_name='RADIUSServer'\n")
      _T("UPDATE config SET var_name='RADIUS.Timeout' WHERE var_name='RADIUSTimeout'\n")
      _T("UPDATE config SET var_name='ReportingServer.Enable' WHERE var_name='EnableReportingServer'\n")
      _T("UPDATE config SET var_name='ReportingServer.Hostname' WHERE var_name='ReportingServerHostname'\n")
      _T("UPDATE config SET var_name='ReportingServer.Port' WHERE var_name='ReportingServerPort'\n")
      _T("UPDATE config SET var_name='Server.AllowedCiphers' WHERE var_name='AllowedCiphers'\n")
      _T("UPDATE config SET var_name='Server.Color' WHERE var_name='ServerColor'\n")
      _T("UPDATE config SET var_name='Server.CommandOutputTimeout' WHERE var_name='ServerCommandOutputTimeout'\n")
      _T("UPDATE config SET var_name='Server.EscapeLocalCommands' WHERE var_name='EscapeLocalCommands'\n")
      _T("UPDATE config SET var_name='Server.ImportConfigurationOnStartup' WHERE var_name='ImportConfigurationOnStartup'\n")
      _T("UPDATE config SET var_name='Server.MessageOfTheDay' WHERE var_name='MessageOfTheDay'\n")
      _T("UPDATE config SET var_name='Server.Name' WHERE var_name='ServerName'\n")
      _T("UPDATE config SET var_name='Server.RoamingMode' WHERE var_name='RoamingServer'\n")
      _T("UPDATE config SET var_name='Server.Security.CaseInsensitiveLoginNames' WHERE var_name='CaseInsensitiveLoginNames'\n")
      _T("UPDATE config SET var_name='Server.Security.ExtendedLogQueryAccessControl' WHERE var_name='ExtendedLogQueryAccessControl'\n")
      _T("UPDATE config SET var_name='Server.Security.GraceLoginCount' WHERE var_name='GraceLoginCount'\n")
      _T("UPDATE config SET var_name='Server.Security.IntruderLockoutThreshold' WHERE var_name='IntruderLockoutThreshold'\n")
      _T("UPDATE config SET var_name='Server.Security.IntruderLockoutTime' WHERE var_name='IntruderLockoutTime'\n")
      _T("UPDATE config SET var_name='Server.Security.MinPasswordLength' WHERE var_name='MinPasswordLength'\n")
      _T("UPDATE config SET var_name='Server.Security.PasswordComplexity' WHERE var_name='PasswordComplexity'\n")
      _T("UPDATE config SET var_name='Server.Security.PasswordExpiration' WHERE var_name='PasswordExpiration'\n")
      _T("UPDATE config SET var_name='Server.Security.PasswordHistoryLength' WHERE var_name='PasswordHistoryLength'\n")
      _T("UPDATE config SET var_name='SNMP.RequestTimeout' WHERE var_name='SNMPRequestTimeout'\n")
      _T("UPDATE config SET var_name='SNMP.Traps.LogRetentionTime' WHERE var_name='SNMPTrapLogRetentionTime'\n")
      _T("UPDATE config SET var_name='SNMP.Traps.SourcesInAllZones' WHERE var_name='TrapSourcesInAllZones'\n")
      _T("UPDATE config SET var_name='Topology.RoutingTableUpdateInterval' WHERE var_name='RoutingTableUpdateInterval'\n")
      _T("UPDATE config SET var_name='XMPP.Enable' WHERE var_name='EnableXMPPConnector'\n")
      _T("UPDATE config SET var_name='XMPP.Login' WHERE var_name='XMPPLogin'\n")
      _T("UPDATE config SET var_name='XMPP.Password' WHERE var_name='XMPPPassword'\n")
      _T("UPDATE config SET var_name='XMPP.Port' WHERE var_name='XMPPPort'\n")
      _T("UPDATE config SET var_name='XMPP.Server' WHERE var_name='XMPPServer'\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   static const TCHAR *batch2 =
      _T("UPDATE config_values SET var_name='Agent.DefaultCacheMode' WHERE var_name='DefaultAgentCacheMode'\n")
      _T("UPDATE config_values SET var_name='Agent.DefaultEncryptionPolicy' WHERE var_name='DefaultEncryptionPolicy'\n")
      _T("UPDATE config_values SET var_name='Agent.DefaultProtocolCompressionMode' WHERE var_name='DefaultAgentProtocolCompressionMode'\n")
      _T("UPDATE config_values SET var_name='AuditLog.External.Port' WHERE var_name='ExternalAuditPort'\n")
      _T("UPDATE config_values SET var_name='Client.ListenerPort' WHERE var_name='ClientListenerPort'\n")
      _T("UPDATE config_values SET var_name='Objects.StatusCalculation.CalculationAlgorithm' WHERE var_name='StatusCalculationAlgorithm'\n")
      _T("UPDATE config_values SET var_name='Objects.StatusCalculation.PropagationAlgorithm' WHERE var_name='StatusPropagationAlgorithm'\n")
      _T("UPDATE config_values SET var_name='RADIUS.Port' WHERE var_name='RADIUSPort'\n")
      _T("UPDATE config_values SET var_name='RADIUS.SecondaryPort' WHERE var_name='RADIUSSecondaryPort'\n")
      _T("UPDATE config_values SET var_name='ReportingServer.Port' WHERE var_name='ReportingServerPort'\n")
      _T("UPDATE config_values SET var_name='Server.AllowedCiphers' WHERE var_name='AllowedCiphers'\n")
      _T("UPDATE config_values SET var_name='Server.ImportConfigurationOnStartup' WHERE var_name='ImportConfigurationOnStartup'\n")
      _T("UPDATE config_values SET var_name='XMPP.Port' WHERE var_name='XMPPPort'\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch2));

   CHK_EXEC(SetMinorSchemaVersion(82));
   return true;
}

/**
 * Upgrade from 40.80 to 40.81
 */
static bool H_UpgradeFromV80()
{
   static const TCHAR *batch =
      _T("ALTER TABLE dc_targets ADD web_service_proxy integer\n")
      _T("UPDATE dc_targets SET web_service_proxy=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("dc_targets"), _T("web_service_proxy")));

   CHK_EXEC(SetMinorSchemaVersion(81));
   return true;
}

/**
 * Upgrade from 40.79 to 40.80
 */
static bool H_UpgradeFromV79()
{
   static const TCHAR *batch =
      _T("UPDATE event_cfg SET event_name='SYS_BUSINESS_SERVICE_OPERATIONAL' WHERE guid='ffe557a4-f572-44a8-928e-020b3fbd07b0'\n")
      _T("UPDATE event_cfg SET event_name='SYS_BUSINESS_SERVICE_DEGRADED' WHERE guid='39bd18fc-bfd7-4c31-af9c-1e96ef3aef64'\n")
      _T("UPDATE event_cfg SET event_name='SYS_BUSINESS_SERVICE_FAILED' WHERE guid='fc66c83f-c7e8-4635-ad2d-89589d8b0fc2'\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   CHK_EXEC(SetMinorSchemaVersion(80));
   return true;
}

/**
 * Upgrade from 40.78 to 40.79
 */
static bool H_UpgradeFromV78()
{
   static const TCHAR *batch =
      _T("ALTER TABLE business_service_checks ADD status integer\n")
      _T("ALTER TABLE business_service_checks ADD failure_reason varchar(255)\n")
      _T("UPDATE business_service_checks SET status=(CASE WHEN current_ticket<>0 THEN 4 ELSE 0 END)\n")
      _T("UPDATE business_service_checks SET failure_reason=(SELECT reason FROM business_service_tickets WHERE business_service_tickets.ticket_id=business_service_checks.current_ticket) WHERE current_ticket<>0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("business_service_checks"), _T("status")));

   CHK_EXEC(CreateEventTemplate(EVENT_BUSINESS_SERVICE_DEGRADED, _T("SYS_BUSINESS_SERVICE_DEGRADED"),
      EVENT_SEVERITY_MINOR, EF_LOG, _T("39bd18fc-bfd7-4c31-af9c-1e96ef3aef64"),
      _T("Business service changed state to degraded"),
      _T("Generated when business service state changes to degraded.")));

   AddEventToEPPRule(_T("aa188673-049f-4c4d-8767-c1cf443c9547"), EVENT_BUSINESS_SERVICE_DEGRADED);

   CHK_EXEC(SetMinorSchemaVersion(79));
   return true;
}

/**
 * Upgrade from 40.77 to 40.78
 */
static bool H_UpgradeFromV77()
{
   static const TCHAR *batch = 
      _T("ALTER TABLE object_properties ADD comments_source $SQL:TEXT\n")
      _T("UPDATE object_properties SET comments_source=comments\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   CHK_EXEC(SetMinorSchemaVersion(78));
   return true;
}

/**
 * Upgrade from 40.76 to 40.77
 */
static bool H_UpgradeFromV76()
{
   CHK_EXEC(CreateEventTemplate(EVENT_ICMP_OK, _T("SYS_ICMP_OK"),
      EVENT_SEVERITY_NORMAL, EF_LOG, _T("6c6f3d24-4cf0-40a1-a4c9-1686ff37533d"),
      _T("Node started responding to ICMP"),
      _T("Generated when node start responding to ICMP again")));

   CHK_EXEC(CreateEventTemplate(EVENT_ICMP_UNREACHABLE, _T("SYS_ICMP_UNREACHABLE"),
      EVENT_SEVERITY_MAJOR, EF_LOG, _T("5eaf0007-2018-44c7-8f83-d244279aca4f"),
      _T("Node is unreachable by ICMP"),
      _T("Generated when node is unreachable by ICMP.")));

   int ruleId = NextFreeEPPruleID();

   TCHAR query[1024];
   _sntprintf(query, 1024, _T("INSERT INTO event_policy (rule_id,rule_guid,flags,comments,alarm_message,alarm_severity,alarm_key,script,alarm_timeout,alarm_timeout_event) ")
                           _T("VALUES (%d,'9169a40b-cd9d-4328-b113-f690ef57773e',7944,'Generated alarm when node is unreachable by ICMP.','%%m',5,'ICMP_UNREACHABLE_%%i','',0,%d)"),
         ruleId, EVENT_ALARM_TIMEOUT);
   CHK_EXEC(SQLQuery(query));

   _sntprintf(query, 1024, _T("INSERT INTO policy_event_list (rule_id,event_code) VALUES (%d,%d)"), ruleId, EVENT_ICMP_UNREACHABLE);
   CHK_EXEC(SQLQuery(query));

   ruleId++;

   _sntprintf(query, 1024, _T("INSERT INTO event_policy (rule_id,rule_guid,flags,comments,alarm_message,alarm_severity,alarm_key,script,alarm_timeout,alarm_timeout_event) ")
                           _T("VALUES (%d,'81320995-d6db-4a3c-9670-12400eba4fe6',7944,'Terminate alarm when node started responding to ICMP again.','%%m',6,'ICMP_UNREACHABLE_%%i','',0,%d)"),
         ruleId, EVENT_ALARM_TIMEOUT);
   CHK_EXEC(SQLQuery(query));

   _sntprintf(query, 1024, _T("INSERT INTO policy_event_list (rule_id,event_code) VALUES (%d,%d)"), ruleId, EVENT_ICMP_OK);
   CHK_EXEC(SQLQuery(query));

   CHK_EXEC(SetMinorSchemaVersion(77));
   return true;
}

/**
 * Upgrade from 40.75 to 40.76
 */
static bool H_UpgradeFromV75()
{
   CHK_EXEC(DBRenameColumn(g_dbHandle, _T("network_services"), _T("check_responce"), _T("check_response")));
   CHK_EXEC(SetMinorSchemaVersion(76));
   return true;
}

/**
 * Upgrade from 40.74 to 40.75
 */
static bool H_UpgradeFromV74()
{
   CHK_EXEC(CreateEventTemplate(EVENT_BUSINESS_SERVICE_OPERATIONAL, _T("SYS_BUSINESS_SERVICE_OPERATIONAL"),
      EVENT_SEVERITY_NORMAL, EF_LOG, _T("ffe557a4-f572-44a8-928e-020b3fbd07b0"),
      _T("Business service changed state to operational"),
      _T("Generated when business service state changes to operational.")));

   CHK_EXEC(CreateEventTemplate(EVENT_BUSINESS_SERVICE_FAILED, _T("SYS_BUSINESS_SERVICE_FAILED"),
      EVENT_SEVERITY_CRITICAL, EF_LOG, _T("fc66c83f-c7e8-4635-ad2d-89589d8b0fc2"),
      _T("Business service changed state to failed"),
      _T("Generated when business service state changes to failed.")));

   int ruleId = NextFreeEPPruleID();

   TCHAR query[1024];
   _sntprintf(query, 1024, _T("INSERT INTO event_policy (rule_id,rule_guid,flags,comments,alarm_message,alarm_severity,alarm_key,script,alarm_timeout,alarm_timeout_event) ")
                           _T("VALUES (%d,'aa188673-049f-4c4d-8767-c1cf443c9547',7944,'Generated alarm when business service changes status to critical','%%m',5,'BUSINESS_SERVICE_CRITICAL_%%i','',0,%d)"),
         ruleId, EVENT_ALARM_TIMEOUT);
   CHK_EXEC(SQLQuery(query));

   _sntprintf(query, 1024, _T("INSERT INTO policy_event_list (rule_id,event_code) VALUES (%d,%d)"), ruleId, EVENT_BUSINESS_SERVICE_FAILED);
   CHK_EXEC(SQLQuery(query));

   ruleId++;

   _sntprintf(query, 1024, _T("INSERT INTO event_policy (rule_id,rule_guid,flags,comments,alarm_message,alarm_severity,alarm_key,script,alarm_timeout,alarm_timeout_event) ")
                           _T("VALUES (%d,'af00706c-b92a-4335-b441-190609c2494f',7944,'Terminate alarm when business service status changes back to normal','%%m',6,'BUSINESS_SERVICE_CRITICAL_%%i','',0,%d)"),
         ruleId, EVENT_ALARM_TIMEOUT);
   CHK_EXEC(SQLQuery(query));

   _sntprintf(query, 1024, _T("INSERT INTO policy_event_list (rule_id,event_code) VALUES (%d,%d)"), ruleId, EVENT_BUSINESS_SERVICE_OPERATIONAL);
   CHK_EXEC(SQLQuery(query));

   CHK_EXEC(SetMinorSchemaVersion(75));
   return true;
}

/**
 * Upgrade from 40.73 to 40.74
 */
static bool H_UpgradeFromV73()
{
   CHK_EXEC(SQLQuery(_T("UPDATE config SET var_name='Objects.Security.CheckTrustedNodes',need_server_restart=0 WHERE var_name='CheckTrustedNodes'")));
   CHK_EXEC(SetMinorSchemaVersion(74));
   return true;
}

/**
 * Upgrade from 40.72 to 40.73
 */
static bool H_UpgradeFromV72()
{
   CHK_EXEC(CreateConfigParam(_T("AgentPolicy.MaxFileSize"),
         _T("134217728"),
         _T("Maximum file size for exported files in agent policies"),
         _T("bytes"),
         'I', true, false, false, false));

   CHK_EXEC(SetMinorSchemaVersion(73));
   return true;
}

/**
 * Upgrade from 40.71 to 40.72
 */
static bool H_UpgradeFromV71()
{
   CHK_EXEC(CreateTable(
         _T("CREATE TABLE pollable_objects (")
         _T("  id integer not null,")
         _T("  config_poll_timestamp integer not null,")
         _T("  instance_poll_timestamp integer not null,")
         _T("PRIMARY KEY(id))")));

   static const TCHAR *batch =
      _T("INSERT INTO pollable_objects (id,config_poll_timestamp,instance_poll_timestamp) SELECT id,config_poll_timestamp,instance_poll_timestamp FROM dc_targets\n")
      _T("ALTER TABLE dc_targets DROP COLUMN config_poll_timestamp\n")
      _T("ALTER TABLE dc_targets DROP COLUMN instance_poll_timestamp\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(SetMinorSchemaVersion(72));
   return true;
}

/**
 * Upgrade from 40.70 to 40.71
 */
static bool H_UpgradeFromV70()
{
   CHK_EXEC(SQLQuery(_T("DELETE FROM config WHERE var_name='EnableObjectTransactions'")));
   CHK_EXEC(SetMinorSchemaVersion(71));
   return true;
}

/**
 * Delete all Netobj level information
 */
static bool BSCommonDeleteObject(uint32_t id)
{
   TCHAR query[1024];
   _sntprintf(query, 1024, _T("DELETE FROM acl WHERE object_id=%u"), id);
   if (!SQLQuery(query) && !g_ignoreErrors)
      return false;

   _sntprintf(query, 1024,  _T("DELETE FROM object_properties WHERE object_id=%u"), id);
   if (!SQLQuery(query) && !g_ignoreErrors)
      return false;

   _sntprintf(query, 1024,  _T("DELETE FROM object_custom_attributes WHERE object_id=%u"), id);
   if (!SQLQuery(query) && !g_ignoreErrors)
      return false;

   _sntprintf(query, 1024,  _T("DELETE FROM object_urls WHERE object_id=%u"), id);
   if (!SQLQuery(query) && !g_ignoreErrors)
      return false;

   _sntprintf(query, 1024,  _T("DELETE FROM responsible_users WHERE object_id=%u"), id);
   if (!SQLQuery(query) && !g_ignoreErrors)
      return false;

   _sntprintf(query, 1024,  _T("DELETE FROM slm_service_history WHERE service_id=%u"), id);
   if (!SQLQuery(query) && !g_ignoreErrors)
      return false;

   TCHAR table[256];
   _sntprintf(table, 256, _T("gps_history_%u"), id);
   int rc = DBIsTableExist(g_dbHandle, table);
   if (rc == DBIsTableExist_Found)
   {
      TCHAR query[256];
      _sntprintf(query, 256, _T("DROP TABLE gps_history_%u"), id);
      return DBQuery(g_dbHandle, query);
   }

   return true;
}

/**
 * Upgrade from 40.69 to 40.70
 */
static bool H_UpgradeFromV69()
{
   CHK_EXEC(CreateConfigParam(_T("BusinessServices.Check.Threshold.DataCollection"),
         _T("1"),
         _T("Default threshold for business service DCI checks"),
         _T(""),
         'C', true, false, false, false));
   CHK_EXEC(CreateConfigParam(_T("BusinessServices.Check.Threshold.Objects"),
         _T("1"),
         _T("Default threshold for business service objects checks"),
         _T(""),
         'C', true, false, false, false));
   CHK_EXEC(CreateConfigParam(_T("BusinessServices.History.RetentionTime"),
         _T("90"),
         _T("Retention time for business service historical data"),
         _T("days"),
         'I', true, false, false, false));

   static const TCHAR *configBatch =
      _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('BusinessServices.Check.Threshold.DataCollection','1','Warning')\n")
      _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('BusinessServices.Check.Threshold.DataCollection','2','Minor')\n")
      _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('BusinessServices.Check.Threshold.DataCollection','3','Major')\n")
      _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('BusinessServices.Check.Threshold.DataCollection','4','Critical')\n")
      _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('BusinessServices.Check.Threshold.Objects','1','Warning')\n")
      _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('BusinessServices.Check.Threshold.Objects','2','Minor')\n")
      _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('BusinessServices.Check.Threshold.Objects','3','Major')\n")
      _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('BusinessServices.Check.Threshold.Objects','4','Critical')\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(configBatch));

   // Business service
   static const TCHAR *businessServiceBatch =
      _T("ALTER TABLE business_services ADD prototype_id integer\n")
      _T("ALTER TABLE business_services ADD instance varchar(1023)\n")
      _T("ALTER TABLE business_services ADD object_status_threshold integer\n")
      _T("ALTER TABLE business_services ADD dci_status_threshold integer\n")
      _T("UPDATE business_services SET prototype_id=0,object_status_threshold=0,dci_status_threshold=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(businessServiceBatch));
   CHK_EXEC(DBRenameColumn(g_dbHandle, _T("business_services"), _T("service_id"), _T("id")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("business_services"), _T("prototype_id")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("business_services"), _T("object_status_threshold")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("business_services"), _T("dci_status_threshold")));

   CHK_EXEC(CreateTable(
         _T("CREATE TABLE business_service_prototypes (")
         _T("   id integer not null,")
         _T("   instance_method integer not null,")
         _T("   instance_source integer not null,")
         _T("   instance_data varchar(1023),")
         _T("   instance_filter $SQL:TEXT,")
         _T("   object_status_threshold integer not null,")
         _T("   dci_status_threshold integer not null,")
         _T("   PRIMARY KEY(id))")));

   // Delete all templates
   CHK_EXEC(SQLQuery(_T("DELETE FROM object_properties WHERE object_id IN (SELECT id FROM slm_checks WHERE is_template=1)")));
   CHK_EXEC(SQLQuery(_T("DELETE FROM container_members WHERE object_id IN (SELECT id FROM slm_checks WHERE is_template=1)")));
   CHK_EXEC(SQLQuery(_T("DELETE FROM slm_checks WHERE is_template=1")));

   // SLM Checks
   static const TCHAR *slmCheckBatch =
      _T("ALTER TABLE slm_checks ADD service_id integer\n")
      _T("ALTER TABLE slm_checks ADD related_object integer\n")
      _T("ALTER TABLE slm_checks ADD related_dci integer\n")
      _T("ALTER TABLE slm_checks ADD status_threshold integer\n")
      _T("ALTER TABLE slm_checks ADD description varchar(1023)\n")
      _T("UPDATE slm_checks SET service_id=0,related_object=0,related_dci=0,status_threshold=0\n") //Threshold is default - from server configuration
      _T("<END>");
   CHK_EXEC(SQLBatch(slmCheckBatch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("slm_checks"), _T("service_id")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("slm_checks"), _T("related_object")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("slm_checks"), _T("related_dci")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("slm_checks"), _T("status_threshold")));

   CHK_EXEC(DBDropColumn(g_dbHandle, _T("slm_checks"), _T("reason")));
   CHK_EXEC(DBDropColumn(g_dbHandle, _T("slm_checks"), _T("is_template")));
   CHK_EXEC(DBDropColumn(g_dbHandle, _T("slm_checks"), _T("template_id")));
   CHK_EXEC(DBDropColumn(g_dbHandle, _T("slm_checks"), _T("threshold_id")));

   // check if there are node links under business service parent
   // change all node links to business services
   IntegerArray<uint32_t> convertedLinks;
   DB_RESULT linkUnderMainContainer = SQLSelect(_T("SELECT cm.object_id FROM container_members cm INNER JOIN node_links nl ON nl.nodelink_id=cm.object_id WHERE cm.container_id=9"));
   if (linkUnderMainContainer != nullptr)
   {
      int slmCheckCount = DBGetNumRows(linkUnderMainContainer);
      if (slmCheckCount > 0)
      {
         //create new business service and update child/parent
         for (int i = 0; i < slmCheckCount; i++)
         {
            uint32_t linkId = DBGetFieldULong(linkUnderMainContainer, i, 0);
            TCHAR query[256];
            _sntprintf(query, 256, _T("INSERT INTO business_services (id,prototype_id,instance,object_status_threshold,dci_status_threshold) VALUES (%u,0,'',0,0)"), linkId);
            if (!SQLQuery(query) && !g_ignoreErrors)
            {
               return false;
            }
            convertedLinks.add(linkId);
         }
      }
      DBFreeResult(linkUnderMainContainer);
   }
   else if (!g_ignoreErrors)
   {
      return false;
   }

   // remove node link from relationship
   DB_RESULT nodeLinksResult = SQLSelect(_T("SELECT nodelink_id,node_id FROM node_links"));
   if (nodeLinksResult != nullptr)
   {
      int linkCount = DBGetNumRows(nodeLinksResult);
      for(int i = 0; i < linkCount; i++)
      {
         uint32_t nodeLinkId = DBGetFieldULong(nodeLinksResult, i, 0);
         //set node id as related object for slm checks
         TCHAR query[1024];
         _sntprintf(query, 1024, _T("UPDATE slm_checks SET related_object=%d WHERE id IN (SELECT object_id FROM container_members WHERE container_id=%d)"),
               DBGetFieldULong(nodeLinksResult, i, 1), nodeLinkId);

         if (!SQLQuery(query) && !g_ignoreErrors)
         {
            return false;
         }

         if (!convertedLinks.contains(nodeLinkId))
         {
            //Remove node link from relationship
            uint32_t parentId = 0;
            _sntprintf(query, 1024, _T("SELECT container_id FROM container_members WHERE object_id=%d"),
                  nodeLinkId);
            DB_RESULT result = SQLSelect(query);
            if (result != nullptr)
            {
               if (DBGetNumRows(result) > 0)
               {
                  parentId = DBGetFieldULong(result, 0, 0);
               }
               DBFreeResult(result);
            }
            else if (!g_ignoreErrors)
            {
               return false;
            }

            _sntprintf(query, 1024, _T("UPDATE container_members SET container_id=%d WHERE container_id=%d"),
                  parentId, nodeLinkId);
            if (!SQLQuery(query) && !g_ignoreErrors)
            {
               return false;
            }

            _sntprintf(query, 1024, _T("UPDATE slm_service_history SET service_id=%d WHERE service_id=%d"),
                  parentId, nodeLinkId);
            if (!SQLQuery(query) && !g_ignoreErrors)
            {
               return false;
            }

            _sntprintf(query, 1024, _T("UPDATE slm_tickets SET service_id=%d WHERE service_id=%d"),
                  parentId, nodeLinkId);
            if (!SQLQuery(query) && !g_ignoreErrors)
            {
               return false;
            }

            //Delete all additional node link information
            _sntprintf(query, 1024, _T("DELETE FROM container_members WHERE object_id=%d"), nodeLinkId);
            if (!SQLQuery(query) && !g_ignoreErrors)
               return false;

            _sntprintf(query, 1024, _T("DELETE FROM object_containers WHERE id=%d"), nodeLinkId);
            if (!SQLQuery(query) && !g_ignoreErrors)
               return false;

            CHK_EXEC(BSCommonDeleteObject(nodeLinkId));
         }
      }
      DBFreeResult(nodeLinksResult);
   }
   else if (!g_ignoreErrors)
   {
      return false;
   }

   //slm tickets
   static const TCHAR *slmTicketBatch =
      _T("ALTER TABLE slm_tickets ADD check_description varchar(1023)\n")
      _T("ALTER TABLE slm_tickets ADD original_ticket_id integer\n")
      _T("ALTER TABLE slm_tickets ADD original_service_id integer\n")
      _T("UPDATE slm_tickets SET original_ticket_id=0,original_service_id=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(slmTicketBatch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("slm_tickets"), _T("original_ticket_id")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("slm_tickets"), _T("original_service_id")));

   //Update slm check description and relationship with business service
   DB_RESULT slmDescriptionResult = SQLSelect(_T("SELECT id,name FROM object_properties op INNER JOIN slm_checks sc ON sc.id=op.object_id"));
   if (slmDescriptionResult != nullptr)
   {
      int count = DBGetNumRows(slmDescriptionResult);
      DB_STATEMENT hStmtCheck = DBPrepare(g_dbHandle, _T("UPDATE slm_checks SET description=? WHERE id=?"), true);
      DB_STATEMENT hStmtTicket = DBPrepare(g_dbHandle, _T("UPDATE slm_tickets SET check_description=? WHERE check_id=?"), true);
      if (hStmtCheck != nullptr && hStmtTicket != nullptr)
      {
         for(int i = 0; i < count; i++)
         {
            uint32_t checkId = DBGetFieldULong(slmDescriptionResult, i, 0);

            TCHAR query[1024];
            _sntprintf(query, 1024, _T("UPDATE slm_checks SET service_id=(SELECT container_id FROM container_members WHERE object_id=%d) WHERE id=%d"), checkId, checkId);
            if (!SQLQuery(query) && !g_ignoreErrors)
               return false;

            _sntprintf(query, 1024, _T("DELETE FROM container_members WHERE object_id=%d"), checkId);
            if (!SQLQuery(query) && !g_ignoreErrors)
               return false;

            DBBind(hStmtCheck, 1, DB_SQLTYPE_VARCHAR, DBGetField(slmDescriptionResult, i, 1, nullptr, 0), DB_BIND_DYNAMIC);
            DBBind(hStmtCheck, 2, DB_SQLTYPE_INTEGER, checkId);
            if (!SQLExecute(hStmtCheck) && !g_ignoreErrors)
            {
               DBFreeStatement(hStmtCheck);
               return false;
            }

            DBBind(hStmtTicket, 1, DB_SQLTYPE_VARCHAR, DBGetField(slmDescriptionResult, i, 1, nullptr, 0), DB_BIND_DYNAMIC);
            DBBind(hStmtTicket, 2, DB_SQLTYPE_INTEGER, checkId);
            if (!SQLExecute(hStmtTicket) && !g_ignoreErrors)
            {
               DBFreeStatement(hStmtTicket);
               return false;
            }
         }
         DBFreeStatement(hStmtCheck);
         DBFreeStatement(hStmtTicket);
      }
      else if (!g_ignoreErrors)
      {
         return false;
      }
      DBFreeResult(slmDescriptionResult);
   }
   else if (!g_ignoreErrors)
   {
      return false;
   }

   // Delete all slm check object information
   DB_RESULT slmCheckResult = SQLSelect(_T("SELECT id FROM slm_checks"));
   if (slmCheckResult != nullptr)
   {
      int slmCheckCount = DBGetNumRows(slmCheckResult);
      for(int i = 0; i < slmCheckCount; i++)
      {
         CHK_EXEC(BSCommonDeleteObject(DBGetFieldULong(slmCheckResult, i, 0)));
      }
      DBFreeResult(slmCheckResult);
   }
   else if (!g_ignoreErrors)
   {
      return false;
   }

   CHK_EXEC(SQLQuery(_T("DROP TABLE node_links")));
   // Update node links type that were converted to business services
   CHK_EXEC(SQLQuery(_T("UPDATE object_containers SET object_class=28 where object_class=29")));

   // Delete status for business service root
   TCHAR query[1024];
   _sntprintf(query, 1024,  _T("DELETE FROM slm_service_history WHERE service_id=%d"), 9);
   if (!SQLQuery(query) && !g_ignoreErrors)
      return false;

   // slm service history
   CHK_EXEC(CreateTable(
         _T("CREATE TABLE business_service_downtime (")
         _T("   record_id integer not null,")
         _T("   service_id integer not null,")
         _T("   from_timestamp integer not null,")
         _T("   to_timestamp integer not null,")
         _T("PRIMARY KEY(record_id))")));
   DB_RESULT slmHistoryResult = SQLSelect(_T("SELECT service_id,change_timestamp,new_status FROM slm_service_history ORDER BY service_id, change_timestamp"));
   if (slmHistoryResult != nullptr)
   {
      DB_STATEMENT hStmtInsert = DBPrepare(g_dbHandle, _T("INSERT INTO business_service_downtime (record_id,service_id,from_timestamp,to_timestamp) VALUES (?,?,?,?)"), true);
      if (hStmtInsert != nullptr)
      {
         int slmHistoryCount = DBGetNumRows(slmHistoryResult);
         if (slmHistoryCount > 0)
         {
            int periodStartTimestamp = 0;
            int periodEndTimestamp = 0;
            uint32_t serviceId = DBGetFieldULong(slmHistoryResult, 0, 0);
            DBBind(hStmtInsert, 2, DB_SQLTYPE_INTEGER, serviceId);
            int i = 0;
            for(; i < slmHistoryCount; i++)
            {
               if (serviceId == DBGetFieldULong(slmHistoryResult, i, 0))
               {
                  if (periodStartTimestamp == 0)
                  {
                     if (DBGetFieldLong(slmHistoryResult, i, 2) == STATUS_CRITICAL)
                     {
                        periodStartTimestamp = DBGetFieldLong(slmHistoryResult, i, 1);
                     }
                     continue;
                  }

                  if (DBGetFieldLong(slmHistoryResult, i, 2) < STATUS_CRITICAL)
                  {
                     periodEndTimestamp = DBGetFieldLong(slmHistoryResult, i, 1);
                  }
                  else
                     continue;
               }

               if (periodStartTimestamp != 0)
               {
                  DBBind(hStmtInsert, 1, DB_SQLTYPE_INTEGER, i+1);
                  DBBind(hStmtInsert, 3, DB_SQLTYPE_INTEGER, periodStartTimestamp);
                  DBBind(hStmtInsert, 4, DB_SQLTYPE_INTEGER, periodEndTimestamp);
                  if (!SQLExecute(hStmtInsert) && !g_ignoreErrors)
                  {
                     DBFreeStatement(hStmtInsert);
                     return false;
                  }
                  periodStartTimestamp = 0;
                  periodEndTimestamp = 0;
               }

               if (serviceId != DBGetFieldULong(slmHistoryResult, i, 0))
               {
                  serviceId = DBGetFieldULong(slmHistoryResult, i, 0);
                  DBBind(hStmtInsert, 2, DB_SQLTYPE_INTEGER, serviceId);
               }
            }

            if (periodStartTimestamp != 0)
            {
               DBBind(hStmtInsert, 1, DB_SQLTYPE_INTEGER, i+1);
               DBBind(hStmtInsert, 3, DB_SQLTYPE_INTEGER, periodStartTimestamp);
               DBBind(hStmtInsert, 4, DB_SQLTYPE_INTEGER, periodEndTimestamp);
               if (!SQLExecute(hStmtInsert) && !g_ignoreErrors)
               {
                  DBFreeStatement(hStmtInsert);
                  return false;
               }
            }
         }

         DBFreeStatement(hStmtInsert);
      }
      else if (!g_ignoreErrors)
      {
         return false;
      }
      DBFreeResult(slmHistoryResult);
   }
   else if (!g_ignoreErrors)
   {
      return false;
   }
   CHK_EXEC(SQLQuery(_T("DROP TABLE slm_service_history")));

   // Update auto apply table
   static const TCHAR *autoBindBatch =
      _T("ALTER TABLE auto_bind_target ADD bind_filter_2 $SQL:TEXT\n")
      _T("ALTER TABLE auto_bind_target ADD flags integer\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(autoBindBatch));
   CHK_EXEC(DBRenameColumn(g_dbHandle, _T("auto_bind_target"), _T("bind_filter"), _T("bind_filter_1")));

   DB_RESULT autoBindResult = SQLSelect(_T("SELECT object_id,bind_flag,unbind_flag FROM auto_bind_target"));
   if (autoBindResult != nullptr)
   {
      int autoBindCount = DBGetNumRows(autoBindResult);
      DB_STATEMENT hStmt = DBPrepare(g_dbHandle, _T("UPDATE auto_bind_target SET flags=? WHERE object_id=?"), true);
      if (hStmt != nullptr)
      {
         for(int i = 0; i < autoBindCount; i++)
         {
            int32_t flags = (DBGetFieldLong(autoBindResult, i, 1) ? AAF_AUTO_APPLY_1 : 0) | (DBGetFieldLong(autoBindResult, i, 2) ? AAF_AUTO_REMOVE_1 : 0);
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, flags);
            DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, DBGetFieldULong(autoBindResult, i, 0));
            if (!SQLExecute(hStmt) && !g_ignoreErrors)
            {
               DBFreeStatement(hStmt);
               return false;
            }
         }
         DBFreeStatement(hStmt);
      }
      else if (!g_ignoreErrors)
      {
         return false;
      }
      DBFreeResult(autoBindResult);
   }
   else if (!g_ignoreErrors)
   {
      return false;
   }
   CHK_EXEC(DBDropColumn(g_dbHandle, _T("auto_bind_target"), _T("bind_flag")));
   CHK_EXEC(DBDropColumn(g_dbHandle, _T("auto_bind_target"), _T("unbind_flag")));

   CHK_EXEC(DBRenameTable(g_dbHandle, _T("slm_checks"), _T("business_service_checks")));
   CHK_EXEC(DBRenameTable(g_dbHandle, _T("slm_tickets"), _T("business_service_tickets")));

   //Close all tickets and all downtime
   uint32_t now = static_cast<uint32_t>(time(nullptr));
   _sntprintf(query, 256, _T("UPDATE business_service_tickets SET close_timestamp=%u WHERE close_timestamp=0"), now);
   CHK_EXEC(SQLQuery(query));
   CHK_EXEC(SQLQuery(_T("UPDATE business_service_checks SET current_ticket=0")));
   _sntprintf(query, 256, _T("UPDATE business_service_downtime SET to_timestamp=%u WHERE to_timestamp=0"), now);
   CHK_EXEC(SQLQuery(query));

   CHK_EXEC(SetMinorSchemaVersion(70));
   return true;
}

/**
 * Upgrade from 40.68 to 40.69
 */
static bool H_UpgradeFromV68()
{
   if (GetSchemaLevelForMajorVersion(39) < 13)
   {
      CHK_EXEC(SQLQuery(_T("ALTER TABLE responsible_users ADD escalation_level integer")));
      CHK_EXEC(SQLQuery(_T("UPDATE responsible_users SET escalation_level=0")));
      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("responsible_users"), _T("escalation_level")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(39, 13));
   }
   CHK_EXEC(SetMinorSchemaVersion(69));
   return true;
}

/**
 * Upgrade from 40.67 to 40.68
 */
static bool H_UpgradeFromV67()
{
   if (GetSchemaLevelForMajorVersion(39) < 12)
   {
      CHK_EXEC_NO_SP(DBDropColumn(g_dbHandle, _T("nodes"), _T("status_poll_type")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(39, 12));
   }
   CHK_EXEC(SetMinorSchemaVersion(68));
   return true;
}

/**
 * Upgrade from 40.66 to 40.67
 */
static bool H_UpgradeFromV66()
{
   if (GetSchemaLevelForMajorVersion(39) < 11)
   {
      // Drop incorrectly created notification and action execution log tables in upgrade for version 40.62 or 39.6
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

      CHK_EXEC(SetSchemaLevelForMajorVersion(39, 11));
   }
   CHK_EXEC(SetMinorSchemaVersion(67));
   return true;
}

/**
 * Upgrade from 40.65 to 40.66
 */
static bool H_UpgradeFromV65()
{
   if (GetSchemaLevelForMajorVersion(39) < 10)
   {
      CHK_EXEC(CreateTable(
         _T("CREATE TABLE network_map_deleted_nodes (")
         _T("   map_id integer not null,")
         _T("   object_id integer not null,")
         _T("   element_index integer not null,")
         _T("   position_x integer not null,")
         _T("   position_y integer not null,")
         _T("PRIMARY KEY(map_id, object_id))")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(39, 10));
   }
   CHK_EXEC(SetMinorSchemaVersion(66));
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
 * Upgrade from 40.64 to 40.65
 */
static bool H_UpgradeFromV64()
{
   if (GetSchemaLevelForMajorVersion(39) < 9)
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

      CHK_EXEC(SetSchemaLevelForMajorVersion(39, 8));
   }
   CHK_EXEC(SetMinorSchemaVersion(65));
   return true;
}

/**
 * Upgrade from 40.63 to 40.64
 */
static bool H_UpgradeFromV63()
{
   if (GetSchemaLevelForMajorVersion(39) < 8)
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

      CHK_EXEC(SetSchemaLevelForMajorVersion(39, 8));
   }
   CHK_EXEC(SetMinorSchemaVersion(64));
   return true;
}

/**
 * Upgrade from 40.62 to 40.63
 */
static bool H_UpgradeFromV62()
{
   // This upgrade was creating notification and server action logs with incorrect structure
   // Now those tables created in upgrade procedure for version 40.66 or 39.10
   if (GetSchemaLevelForMajorVersion(39) < 7)
   {
      CHK_EXEC(SetSchemaLevelForMajorVersion(39, 7));
   }
   CHK_EXEC(SetMinorSchemaVersion(63));
   return true;
}

/**
 * Upgrade from 40.61 to 40.62
 */
static bool H_UpgradeFromV61()
{
   if (GetSchemaLevelForMajorVersion(39) < 6)
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

      CHK_EXEC(SetSchemaLevelForMajorVersion(39, 6));
   }
   CHK_EXEC(SetMinorSchemaVersion(62));
   return true;
}

/**
 * Upgrade from 40.60 to 40.61
 */
static bool H_UpgradeFromV60()
{
   if (GetSchemaLevelForMajorVersion(39) < 5)
   {
      CHK_EXEC(CreateConfigParam(_T("Geolocation.History.RetentionTime"),
            _T("90"),
            _T("Retention time in days for object's geolocation history. All records older than specified will be deleted by housekeeping process."),
            _T("days"), 'I', true, false, false, false));

      CHK_EXEC(SetSchemaLevelForMajorVersion(39, 5));
   }
   CHK_EXEC(SetMinorSchemaVersion(61));
   return true;
}

/**
 * Upgrade from 40.59 to 40.60
 */
static bool H_UpgradeFromV59()
{
   if (GetSchemaLevelForMajorVersion(39) < 4)
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
      CHK_EXEC(SetSchemaLevelForMajorVersion(39, 4));
   }
   CHK_EXEC(SetMinorSchemaVersion(60));
   return true;
}

/**
 * Upgrade from 40.58 to 40.59
 */
static bool H_UpgradeFromV58()
{
   if (GetSchemaLevelForMajorVersion(39) < 3)
   {
      CHK_EXEC(DBDropPrimaryKey(g_dbHandle, _T("object_tools_input_fields")));
      CHK_EXEC(DBRenameTable(g_dbHandle, _T("object_tools_input_fields"), _T("input_fields")));
      CHK_EXEC(DBRenameColumn(g_dbHandle, _T("input_fields"), _T("tool_id"), _T("owner_id")));
      CHK_EXEC(SQLQuery(_T("ALTER TABLE input_fields ADD category char(1)")));
      CHK_EXEC(SQLQuery(_T("UPDATE input_fields SET category='T'")));
      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("input_fields"), _T("category")));
      CHK_EXEC(DBAddPrimaryKey(g_dbHandle, _T("input_fields"), _T("category,owner_id,name")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(39, 3));
   }
   CHK_EXEC(SetMinorSchemaVersion(59));
   return true;
}

/**
 * Upgrade from 40.57 to 40.58
 */
static bool H_UpgradeFromV57()
{
   if (GetSchemaLevelForMajorVersion(39) < 2)
   {
      if (DBIsTableExist(g_dbHandle, _T("zmq_subscription")) == DBIsTableExist_Found)
         CHK_EXEC(SQLQuery(_T("DROP TABLE zmq_subscription")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(39, 2));
   }
   CHK_EXEC(SetMinorSchemaVersion(58));
   return true;
}

/**
 * Upgrade from 40.56 to 40.57
 */
static bool H_UpgradeFromV56()
{
   if (GetSchemaLevelForMajorVersion(39) < 1)
   {
      CHK_EXEC(CreateTable(
            _T("CREATE TABLE object_queries (")
            _T("   id integer not null,")
            _T("   guid varchar(36) not null,")
            _T("   name varchar(63) not null,")
            _T("   description varchar(255) null,")
            _T("   script $SQL:TEXT null,")
            _T("PRIMARY KEY(id))")));

      CHK_EXEC(SetSchemaLevelForMajorVersion(39, 1));
   }
   CHK_EXEC(SetMinorSchemaVersion(57));
   return true;
}

/**
 * Upgrade from 40.55 to 40.56
 */
static bool H_UpgradeFromV55()
{
   CHK_EXEC(CreateTable(
         _T("CREATE TABLE two_factor_auth_methods (")
         _T("  name varchar(63) not null,")
         _T("  driver varchar(63) null,")
         _T("  description varchar(255) null,")
         _T("  configuration $SQL:TEXT null,")
         _T("PRIMARY KEY(name))")));

   CHK_EXEC(CreateTable(
         _T("CREATE TABLE two_factor_auth_bindings (")
         _T("  user_id integer not null,")
         _T("  name varchar(63) not null,")
         _T("  configuration $SQL:TEXT null,")
         _T("PRIMARY KEY(user_id,name))")));

   if ((g_dbSyntax == DB_SYNTAX_DB2) || (g_dbSyntax == DB_SYNTAX_INFORMIX) || (g_dbSyntax == DB_SYNTAX_ORACLE))
   {
      CHK_EXEC(SQLQuery(_T("UPDATE users SET system_access=system_access+281474976710656 WHERE (BITAND(system_access, 1) = 1)")));
      CHK_EXEC(SQLQuery(_T("UPDATE user_groups SET system_access=system_access+281474976710656 WHERE (BITAND(system_access, 1) = 1)")));
   }
   else
   {
      CHK_EXEC(SQLQuery(_T("UPDATE users SET system_access=system_access+281474976710656 WHERE ((system_access & 1) = 1)")));
      CHK_EXEC(SQLQuery(_T("UPDATE user_groups SET system_access=system_access+281474976710656 WHERE ((system_access & 1) = 1)")));
   }

   CHK_EXEC(SetMinorSchemaVersion(56));
   return true;
}

/**
 * Upgrade from 40.54 to 40.55
 */
static bool H_UpgradeFromV54()
{
   if (GetSchemaLevelForMajorVersion(38) < 17)
   {
      CHK_EXEC(SQLQuery(_T("ALTER TABLE alarms ADD rule_description varchar(255)")));
      DB_STATEMENT hStmt = DBPrepare(g_dbHandle,
         (g_dbSyntax == DB_SYNTAX_MSSQL) ?
            _T("UPDATE alarms SET rule_description=(SELECT REPLACE(REPLACE(REPLACE(REPLACE(CONVERT(varchar(255), comments), ?, ' '), ?, ' '), ?, ' '), '  ', ' ') FROM event_policy WHERE event_policy.rule_guid=alarms.rule_guid)") :
            _T("UPDATE alarms SET rule_description=(SELECT REPLACE(REPLACE(REPLACE(REPLACE(comments, ?, ' '), ?, ' '), ?, ' '), '  ', ' ') FROM event_policy WHERE event_policy.rule_guid=alarms.rule_guid)"));
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, _T("\n"), DB_BIND_STATIC);
         DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, _T("\r"), DB_BIND_STATIC);
         DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, _T("\t"), DB_BIND_STATIC);
         if (!SQLExecute(hStmt) && !g_ignoreErrors)
         {
            DBFreeStatement(hStmt);
            return false;
         }
         DBFreeStatement(hStmt);
      }
      else if (!g_ignoreErrors)
      {
         return false;
      }

      CHK_EXEC(SetSchemaLevelForMajorVersion(38, 17));
   }

   CHK_EXEC(SetMinorSchemaVersion(55));
   return true;
}


/**
 * Upgrade from 40.53 to 40.54
 */
static bool H_UpgradeFromV53()
{
   if (GetSchemaLevelForMajorVersion(38) < 16)
   {
      CHK_EXEC(CreateEventTemplate(EVENT_TUNNEL_SETUP_ERROR, _T("SYS_TUNNEL_SETUP_ERROR"),
               EVENT_SEVERITY_MAJOR, EF_LOG, _T("50792874-0b2e-4eca-8c54-274b7d5e3aa2"),
               _T("Error setting up agent tunnel from %<ipAddress> (%<error>)"),
               _T("Generated on agent tunnel setup error.\r\n")
               _T("Parameters:\r\n")
               _T("   1) Source IP address (ipAddress)\r\n")
               _T("   2) Error message (error)")
               ));

      int ruleId = NextFreeEPPruleID();

      TCHAR query[1024];
      _sntprintf(query, 1024, _T("INSERT INTO event_policy (rule_id,rule_guid,flags,comments,alarm_message,alarm_severity,alarm_key,script,alarm_timeout,alarm_timeout_event) ")
               _T("VALUES (%d,'bdd8f6b1-fe3a-4b20-bc0b-bb7507b264b2',7944,'Generate alarm on agent tunnel setup error','%%m',5,'TUNNEL_SETUP_ERROR_%%1','',0,%d)"),
               ruleId, EVENT_ALARM_TIMEOUT);
      CHK_EXEC(SQLQuery(query));

      _sntprintf(query, 256, _T("INSERT INTO policy_event_list (rule_id,event_code) VALUES (%d,%d)"), ruleId, EVENT_TUNNEL_SETUP_ERROR);
      CHK_EXEC(SQLQuery(query));

      CHK_EXEC(SetSchemaLevelForMajorVersion(38, 16));
   }
   CHK_EXEC(SetMinorSchemaVersion(54));
   return true;
}

/**
 * Upgrade from 40.52 to 40.53
 */
static bool H_UpgradeFromV52()
{
   if (GetSchemaLevelForMajorVersion(38) < 15)
   {
      static const TCHAR *batch =
         _T("UPDATE config SET data_type='C',need_server_restart=0 WHERE var_name='Objects.Nodes.ResolveDNSToIPOnStatusPoll'\n")
         _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('Objects.Nodes.ResolveDNSToIPOnStatusPoll','0','Never')\n")
         _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('Objects.Nodes.ResolveDNSToIPOnStatusPoll','1','Always')\n")
         _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('Objects.Nodes.ResolveDNSToIPOnStatusPoll','2','On failure')\n")
         _T("<END>");
      CHK_EXEC(SQLBatch(batch));

      CHK_EXEC(CreateConfigParam(_T("Objects.Nodes.ResolveDNSToIPOnStatusPoll.Interval"),
            _T("0"),
            _T("Number of status polls between resolving primary host name to IP, if Objects.Nodes.ResolveDNSToIPOnStatusPoll set to \"Always\"."),
            _T("polls"),
            'I',
            true,
            false,
            false,
            false));

      CHK_EXEC(SetSchemaLevelForMajorVersion(38, 15));
   }
   CHK_EXEC(SetMinorSchemaVersion(53));
   return true;
}

/**
 * Upgrade from 40.51 to 40.52
 */
static bool H_UpgradeFromV51()
{
   if (GetSchemaLevelForMajorVersion(38) < 14)
   {
      CHK_EXEC(CreateConfigParam(_T("Agent.RestartWaitTime"),
            _T("0"),
            _T("Period of time after agent restart for which agent will not be considered unreachable"),
            _T("seconds"),
            'I',
            true,
            false,
            false,
            false));

      CHK_EXEC(SetSchemaLevelForMajorVersion(38, 14));
   }
   CHK_EXEC(SetMinorSchemaVersion(52));
   return true;
}

/**
 * Upgrade from 40.50 to 40.51
 */
static bool H_UpgradeFromV50()
{
   if (GetSchemaLevelForMajorVersion(38) < 13)
   {
      int ruleId = NextFreeEPPruleID();

      TCHAR query[1024];
      _sntprintf(query, 1024, _T("INSERT INTO event_policy (rule_id,rule_guid,flags,comments,alarm_message,alarm_severity,alarm_key,script,alarm_timeout,alarm_timeout_event) ")
               _T("VALUES (%d,'b517ca11-cdf8-4813-87fa-9a2ace070564',7944,'Generate alarm on agent policy validation failure','%%m',5,'POLICY_ERROR_%%2_%%5','',0,%d)"),
               ruleId, EVENT_ALARM_TIMEOUT);
      CHK_EXEC(SQLQuery(query));

      _sntprintf(query, 256, _T("INSERT INTO policy_event_list (rule_id,event_code) VALUES (%d,%d)"), ruleId, EVENT_POLICY_VALIDATION_ERROR);
      CHK_EXEC(SQLQuery(query));

      CHK_EXEC(SetSchemaLevelForMajorVersion(38, 13));
   }
   CHK_EXEC(SetMinorSchemaVersion(51));
   return true;
}

/**
 * Upgrade from 40.49 to 40.50
 */
static bool H_UpgradeFromV49()
{
   if (GetSchemaLevelForMajorVersion(38) < 12)
   {
      CHK_EXEC(CreateEventTemplate(EVENT_POLICY_VALIDATION_ERROR, _T("SYS_POLICY_VALIDATION_ERROR"),
            SEVERITY_WARNING, EF_LOG, _T("7a0c3a71-8125-4692-985a-a7e94fbee570"),
            _T("Failed validation of %4 policy %3 in template %1 (%6)"),
            _T("Generated when agent policy within template fails validation.\r\n")
            _T("Parameters:\r\n")
            _T("   1) Template name\r\n")
            _T("   2) Template ID\r\n")
            _T("   3) Policy name\r\n")
            _T("   4) Policy type\r\n")
            _T("   5) Policy ID\r\n")
            _T("   6) Additional info")
            ));
      CHK_EXEC(SetSchemaLevelForMajorVersion(38, 12));
   }
   CHK_EXEC(SetMinorSchemaVersion(50));
   return true;
}

/**
 * Upgrade from 40.48 to 40.49
 */
static bool H_UpgradeFromV48()
{
   if (GetSchemaLevelForMajorVersion(38) < 11)
   {
      if ((g_dbSyntax == DB_SYNTAX_DB2) || (g_dbSyntax == DB_SYNTAX_INFORMIX) || (g_dbSyntax == DB_SYNTAX_ORACLE))
         CHK_EXEC(SQLQuery(_T("UPDATE nodes SET last_agent_comm_time=0 WHERE (BITAND(capabilities, 2) = 0) AND (last_agent_comm_time > 0)")));
      else
         CHK_EXEC(SQLQuery(_T("UPDATE nodes SET last_agent_comm_time=0 WHERE ((capabilities & 2) = 0) AND (last_agent_comm_time > 0)")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(38, 11));
   }
   CHK_EXEC(SetMinorSchemaVersion(49));
   return true;
}

/**
 * Upgrade from 40.47 to 40.48
 */
static bool H_UpgradeFromV47()
{
   CHK_EXEC(CreateConfigParam(_T("DataCollection.TemplateRemovalGracePeriod"),
      _T("0"),
      _T("Setting up grace period for deleting templates from DataCollection"),
      nullptr,
      'I',
      true,
      false,
      false,
      false));
   CHK_EXEC(SetMinorSchemaVersion(48));
   return true;
}

/**
 * Upgrade from 40.46 to 40.47
 */
static bool H_UpgradeFromV46()
{
   CHK_EXEC(SQLQuery(_T("UPDATE object_properties SET flags=65536 WHERE object_id IN (SELECT id FROM subnets WHERE synthetic_mask > 0)")));
   CHK_EXEC(DBDropColumn(g_dbHandle, _T("subnets"), _T("synthetic_mask")));
   CHK_EXEC(SetMinorSchemaVersion(47));
   return true;
}

/**
 * Upgrade from 40.45 to 40.46
 */
static bool H_UpgradeFromV45()
{
   if (GetSchemaLevelForMajorVersion(38) < 10)
   {
      static const TCHAR *batch =
         _T("ALTER TABLE interfaces ADD last_known_oper_state integer\n")
         _T("ALTER TABLE interfaces ADD last_known_admin_state integer\n")
         _T("UPDATE interfaces SET last_known_oper_state=0,last_known_admin_state=0\n")
         _T("<END>");
      CHK_EXEC(SQLBatch(batch));
      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("interfaces"), _T("last_known_oper_state")));
      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("interfaces"), _T("last_known_admin_state")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(38, 10));
   }
   CHK_EXEC(SetMinorSchemaVersion(46));
   return true;
}

/**
 * Upgrade from 40.44 to 40.45
 */
static bool H_UpgradeFromV44()
{
   CHK_EXEC(CreateConfigParam(_T("AuditLog.External.UseUTF8"),
         _T("0"),
         _T("Changes audit log encoding to UTF-8"),
         nullptr,
         'B',
         true,
         false,
         false,
         false));
   CHK_EXEC(SetMinorSchemaVersion(45));
   return true;
}

/**
 * Upgrade from 40.43 to 40.44
 */
static bool H_UpgradeFromV43()
{
   if (GetSchemaLevelForMajorVersion(38) < 9)
   {
      CHK_EXEC(SQLQuery(_T("DELETE FROM config WHERE var_name='JobHistoryRetentionTime'")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(38, 9));
   }
   CHK_EXEC(SetMinorSchemaVersion(44));
   return true;
}

/**
 * Upgrade from 40.42 to 40.43
 */
static bool H_UpgradeFromV42()
{
   if (GetSchemaLevelForMajorVersion(38) < 8)
   {
      CHK_EXEC(SQLQuery(_T("ALTER TABLE raw_dci_values ADD cache_timestamp integer")));
      CHK_EXEC(SQLQuery(_T("UPDATE raw_dci_values SET cache_timestamp=0")));
      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("raw_dci_values"), _T("cache_timestamp")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(38, 8));
   }
   CHK_EXEC(SetMinorSchemaVersion(43));
   return true;
}

/**
 * Upgrade from 40.41 to 40.42
 */
static bool H_UpgradeFromV41()
{
   if (GetSchemaLevelForMajorVersion(38) < 7)
   {
      CHK_EXEC(SQLQuery(_T("DROP TABLE job_history")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(38, 7));
   }
   CHK_EXEC(SetMinorSchemaVersion(42));
   return true;
}

/**
 * Upgrade from 40.40 to 40.41
 */
static bool H_UpgradeFromV40()
{
   if (GetSchemaLevelForMajorVersion(38) < 6)
   {
      static const TCHAR *batch =
            _T("ALTER TABLE policy_action_list ADD snooze_time integer\n")
            _T("UPDATE policy_action_list SET snooze_time=0\n")
            _T("<END>");
      CHK_EXEC(SQLBatch(batch));
      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("policy_action_list"), _T("snooze_time")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(38, 6));
   }

   CHK_EXEC(SetMinorSchemaVersion(41));
   return true;
}

/**
 * Upgrade from 40.39 to 40.40
 */
static bool H_UpgradeFromV39()
{
   if (GetSchemaLevelForMajorVersion(38) < 5)
   {
      CHK_EXEC(CreateEventTemplate(EVENT_TUNNEL_HOST_DATA_MISMATCH, _T("SYS_TUNNEL_HOST_DATA_MISMATCH"),
               SEVERITY_WARNING, EF_LOG, _T("874aa4f3-51b9-49ad-a8df-fb4bb89d0f81"),
               _T("Host data mismatch on tunnel reconnect"),
               _T("Generated when new tunnel is replacing existing one and host data mismatch is detected.\r\n")
               _T("Parameters:\r\n")
               _T("   1) Tunnel ID (tunnelId)\r\n")
               _T("   2) Old remote system IP address (oldIPAddress)\r\n")
               _T("   3) New remote system IP address (newIPAddress)\r\n")
               _T("   4) Old remote system name (oldSystemName)\r\n")
               _T("   5) New remote system name (newSystemName)\r\n")
               _T("   6) Old remote system FQDN (oldHostName)\r\n")
               _T("   7) New remote system FQDN (newHostName)\r\n")
               _T("   8) Old hardware ID (oldHardwareId)\r\n")
               _T("   9) New hardware ID (newHardwareId)")
               ));
      CHK_EXEC(SetSchemaLevelForMajorVersion(38, 5));
   }

   CHK_EXEC(SetMinorSchemaVersion(40));
   return true;
}

/**
 * Upgrade from 40.38 to 40.39
 */
static bool H_UpgradeFromV38()
{
   if (GetSchemaLevelForMajorVersion(38) < 4)
   {
      CHK_EXEC(CreateTable(
            _T("CREATE TABLE ssh_keys (")
            _T("  id integer not null,")
            _T("  name varchar(255) not null,")
            _T("  public_key varchar(700) null,")
            _T("  private_key varchar(1710) null,")
            _T("PRIMARY KEY(id))")));

      static const TCHAR *batch =
            _T("ALTER TABLE nodes ADD ssh_key_id integer\n")
            _T("UPDATE nodes SET ssh_key_id=0\n")
            _T("<END>");
      CHK_EXEC(SQLBatch(batch));
      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("nodes"), _T("ssh_key_id")));

      // Update access rights for predefined "Admins" group
      DB_RESULT hResult = SQLSelect(_T("SELECT system_access FROM user_groups WHERE id=1073741825"));
      if (hResult != nullptr)
      {
         if (DBGetNumRows(hResult) > 0)
         {
            uint64_t access = DBGetFieldUInt64(hResult, 0, 0);
            access |= SYSTEM_ACCESS_SSH_KEY_CONFIGURATION;
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
      CHK_EXEC(SetSchemaLevelForMajorVersion(38, 4));
   }

   CHK_EXEC(SetMinorSchemaVersion(39));
   return true;
}

/**
 * Upgrade from 40.37 to 40.38
 */
static bool H_UpgradeFromV37()
{
   if (GetSchemaLevelForMajorVersion(38) < 3)
   {
      if (DBIsTableExist(g_dbHandle, _T("report_results")))
      {
         CHK_EXEC(SQLQuery(_T("DROP TABLE report_results")));
      }
      CHK_EXEC(SetSchemaLevelForMajorVersion(38, 3));
   }
   CHK_EXEC(SetMinorSchemaVersion(38));
   return true;
}

/**
 * Upgrade from 40.36 to 40.37
 */
static bool H_UpgradeFromV36()
{
   if (GetSchemaLevelForMajorVersion(38) < 2)
   {
      if (DBIsTableExist(g_dbHandle, _T("report_notifications")))
      {
         CHK_EXEC(SQLQuery(_T("DROP TABLE report_notifications")));
      }
      CHK_EXEC(SetSchemaLevelForMajorVersion(38, 2));
   }
   CHK_EXEC(SetMinorSchemaVersion(37));
   return true;
}

/**
 * Upgrade from 40.35 to 40.38
 * Upgrades straight to version 38, versions 36 and 37 skipped because in version 38 all reporting tables are already dropped.
 */
static bool H_UpgradeFromV35()
{
   if (GetSchemaLevelForMajorVersion(38) < 1)
   {
      static const TCHAR *deprecatedTables[] = {
         _T("qrtz_fired_triggers"),
         _T("qrtz_paused_trigger_grps"),
         _T("qrtz_scheduler_state"),
         _T("qrtz_locks"),
         _T("qrtz_simple_triggers"),
         _T("qrtz_cron_triggers"),
         _T("qrtz_simprop_triggers"),
         _T("qrtz_blob_triggers"),
         _T("qrtz_triggers"),
         _T("qrtz_job_details"),
         _T("qrtz_calendars"),
         _T("report_notification"),
         _T("reporting_results"),
         nullptr
      };
      for(int i = 0; deprecatedTables[i] != nullptr; i++)
      {
         if (DBIsTableExist(g_dbHandle, deprecatedTables[i]))
         {
            TCHAR query[256];
            _sntprintf(query, 256, _T("DROP TABLE %s"), deprecatedTables[i]);
            CHK_EXEC(SQLQuery(query));
         }
      }
      CHK_EXEC(SetSchemaLevelForMajorVersion(38, 3));
   }
   CHK_EXEC(SetMinorSchemaVersion(38));
   return true;
}

/**
 * Upgrade from 40.34 to 40.35
 */
static bool H_UpgradeFromV34()
{
   if (GetSchemaLevelForMajorVersion(37) < 6)
   {
      CHK_EXEC(SQLQuery(_T("ALTER TABLE users ADD email varchar(127)")));
      CHK_EXEC(SQLQuery(_T("ALTER TABLE users ADD phone_number varchar(63)")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(37, 6));
   }
   CHK_EXEC(SetMinorSchemaVersion(35));
   return true;
}

/**
 * Upgrade from 40.33 to 40.34
 */
static bool H_UpgradeFromV33()
{
   if (GetSchemaLevelForMajorVersion(37) < 5)
   {
      static const TCHAR *batch =
            _T("ALTER TABLE nodes ADD ssh_port integer\n")
            _T("UPDATE nodes SET ssh_port=22\n")
            _T("<END>");
      CHK_EXEC(SQLBatch(batch));
      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("nodes"), _T("ssh_port")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(37, 5));
   }
   CHK_EXEC(SetMinorSchemaVersion(34));
   return true;
}

/**
 * Upgrade from 40.32 to 40.33
 */
static bool H_UpgradeFromV32()
{
   if (GetSchemaLevelForMajorVersion(37) < 4)
   {
      CHK_EXEC(SQLQuery(_T("UPDATE config SET var_name='Objects.Clusters.ContainerAutoBind' WHERE var_name='ClusterContainerAutoBind'")));
      CHK_EXEC(SQLQuery(_T("UPDATE config SET var_name='Objects.Clusters.TemplateAutoApply' WHERE var_name='ClusterTemplateAutoApply'")));
      CHK_EXEC(CreateConfigParam(_T("Objects.AccessPoints.ContainerAutoBind"), _T("0"), _T("Enable/disable container auto binding for access points."), nullptr, 'B', true, false, false, false));
      CHK_EXEC(CreateConfigParam(_T("Objects.AccessPoints.TemplateAutoApply"), _T("0"), _T("Enable/disable template auto apply for access points."), nullptr, 'B', true, false, false, false));
      CHK_EXEC(CreateConfigParam(_T("Objects.MobileDevices.ContainerAutoBind"), _T("0"), _T("Enable/disable container auto binding for mobile devices."), nullptr, 'B', true, false, false, false));
      CHK_EXEC(CreateConfigParam(_T("Objects.MobileDevices.TemplateAutoApply"), _T("0"), _T("Enable/disable template auto apply for mobile devices."), nullptr, 'B', true, false, false, false));
      CHK_EXEC(CreateConfigParam(_T("Objects.Sensors.ContainerAutoBind"), _T("0"), _T("Enable/disable container auto binding for sensors."), nullptr, 'B', true, false, false, false));
      CHK_EXEC(CreateConfigParam(_T("Objects.Sensors.TemplateAutoApply"), _T("0"), _T("Enable/disable template auto apply for sensors."), nullptr, 'B', true, false, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(37, 4));
   }
   CHK_EXEC(SetMinorSchemaVersion(33));
   return true;
}

/**
 * Upgrade from 40.31 to 40.32
 */
static bool H_UpgradeFromV31()
{
   if (GetSchemaLevelForMajorVersion(37) < 3)
   {
      CHK_EXEC(CreateConfigParam(_T("Objects.Interfaces.Enable8021xStatusPoll"), _T("1"), _T("Globally enable or disable checking of 802.1x port state during status poll."), nullptr, 'B', true, false, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(37, 3));
   }
   CHK_EXEC(SetMinorSchemaVersion(32));
   return true;
}

/**
 * Upgrade from 40.30 to 40.31
 */
static bool H_UpgradeFromV30()
{
   if (GetSchemaLevelForMajorVersion(37) < 2)
   {
      CHK_EXEC(SQLQuery(_T("UPDATE config SET var_name='DataCollection.InstanceRetentionTime',default_value='7',need_server_restart=0 WHERE var_name='InstanceRetentionTime'")));
      CHK_EXEC(SQLQuery(_T("UPDATE config SET var_value='7' WHERE var_name='DataCollection.InstanceRetentionTime' AND var_value='0'")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(37, 2));
   }
   CHK_EXEC(SetMinorSchemaVersion(31));
   return true;
}

/**
 * Upgrade from 40.29 to 40.30
 */
static bool H_UpgradeFromV29()
{
   if (GetSchemaLevelForMajorVersion(37) < 1)
   {
      CHK_EXEC(CreateEventTemplate(EVENT_CLUSTER_AUTOADD, _T("SYS_CLUSTER_AUTOADD"),
               SEVERITY_NORMAL, EF_LOG, _T("308fce5f-69ec-450a-a42b-d1e5178512a5"),
               _T("Node %2 automatically added to cluster %4"),
               _T("Generated when node added to cluster object by autoadd rule.\r\n")
               _T("Parameters:\r\n")
               _T("   1) Node ID\r\n")
               _T("   2) Node name\r\n")
               _T("   3) Cluster ID\r\n")
               _T("   4) Cluster name")
               ));

      CHK_EXEC(CreateEventTemplate(EVENT_CLUSTER_AUTOREMOVE, _T("SYS_CLUSTER_AUTOREMOVE"),
               SEVERITY_NORMAL, EF_LOG, _T("f2cdb47a-ae37-43d7-851d-f8f85e1e9f0c"),
               _T("Node %2 automatically removed from cluster %4"),
               _T("Generated when node removed from cluster object by autoadd rule.\r\n")
               _T("Parameters:\r\n")
               _T("   1) Node ID\r\n")
               _T("   2) Node name\r\n")
               _T("   3) Cluster ID\r\n")
               _T("   4) Cluster name")
               ));
      CHK_EXEC(SetSchemaLevelForMajorVersion(37, 1));
   }
   CHK_EXEC(SetMinorSchemaVersion(30));
   return true;
}

/**
 * Upgrade from 40.28 to 40.29
 */
static bool H_UpgradeFromV28()
{
   if (GetSchemaLevelForMajorVersion(36) < 17)
   {
      CHK_EXEC(CreateConfigParam(_T("LDAP.NewUserAuthMethod"), _T("5"), _T("Authentication method to be set for user objects created by LDAP synchronization process."), nullptr, 'C', true, false, false, false));

      static const TCHAR *batch =
            _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('LDAP.NewUserAuthMethod','0','Local password')\n")
            _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('LDAP.NewUserAuthMethod','1','RADIUS password')\n")
            _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('LDAP.NewUserAuthMethod','2','Certificate')\n")
            _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('LDAP.NewUserAuthMethod','3','Certificate or local password')\n")
            _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('LDAP.NewUserAuthMethod','4','Certificate or RADIUS password')\n")
            _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('LDAP.NewUserAuthMethod','5','LDAP password')\n")
            _T("<END>");
      CHK_EXEC(SQLBatch(batch));

      DB_RESULT hResult = SQLSelect(_T("SELECT id,flags FROM users"));
      if (hResult != nullptr)
      {
         int count = DBGetNumRows(hResult);
         for(int i = 0; i < count; i++)
         {
            if (DBGetFieldULong(hResult, i, 1) & UF_LDAP_USER)
            {
               TCHAR query[256];
               _sntprintf(query, 256, _T("UPDATE users SET auth_method=5 WHERE id=%u"), DBGetFieldULong(hResult, i, 0));
               CHK_EXEC(SQLQuery(query));
            }
         }
         DBFreeResult(hResult);
      }
      else if (!g_ignoreErrors)
      {
         return false;
      }

      CHK_EXEC(SetSchemaLevelForMajorVersion(36, 17));
   }
   CHK_EXEC(SetMinorSchemaVersion(29));
   return true;
}

/**
 * Upgrade from 40.27 to 40.28
 */
static bool H_UpgradeFromV27()
{
   if (GetSchemaLevelForMajorVersion(36) < 16)
   {
      CHK_EXEC(SQLQuery(_T("UPDATE config SET var_name='WindowsEvents.LogRetentionTime',description='Retention time in days for records in Windows event log. All records older than specified will be deleted by housekeeping process.' WHERE var_name='WinEventLogRetentionTime'")));
      CHK_EXEC(CreateConfigParam(_T("WindowsEvents.EnableStorage"), _T("1"), _T("Enable/disable local storage of received Windows events in NetXMS database."), nullptr, 'B', true, false, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(36, 16));
   }
   CHK_EXEC(SetMinorSchemaVersion(28));
   return true;
}

/**
 * Upgrade from 40.26 to 40.27
 */
static bool H_UpgradeFromV26()
{
   if (GetSchemaLevelForMajorVersion(36) < 15)
   {
      static const TCHAR *batch =
            _T("UPDATE config SET var_name='Alarms.HistoryRetentionTime',units='days' WHERE var_name='AlarmHistoryRetentionTime'\n")
            _T("UPDATE config SET var_name='DBConnectionPool.CooldownTime',description='Inactivity time (in seconds) after which database connection will be closed.' WHERE var_name='DBConnectionPoolCooldownTime'\n")
            _T("UPDATE config SET var_name='DBConnectionPool.MaxLifetime',description='Maximum lifetime (in seconds) for a database connection.' WHERE var_name='DBConnectionPoolMaxLifetime'\n")
            _T("UPDATE config SET var_name='DBConnectionPool.BaseSize' WHERE var_name='DBConnectionPoolBaseSize'\n")
            _T("UPDATE config SET var_name='DBConnectionPool.MaxSize' WHERE var_name='DBConnectionPoolMaxSize'\n")
            _T("UPDATE config SET description='Name of discovery filter script from script library.' WHERE var_name='DiscoveryFilter'\n")
            _T("UPDATE config SET description='Discovery filter settings.' WHERE var_name='DiscoveryFilterFlags'\n")
            _T("UPDATE config SET description='Enable/disable reporting server.' WHERE var_name='EnableReportingServer'\n")
            _T("UPDATE config SET description='Enable/disable ability to acknowledge an alarm for a specific time.' WHERE var_name='EnableTimedAlarmAck'\n")
            _T("UPDATE config SET description='Enable/disable TAB and new line characters replacement by escape sequence in \"execute command on management server\" actions.' WHERE var_name='EscapeLocalCommands'\n")
            _T("UPDATE config SET description='Retention time in days for the records in event log. All records older than specified will be deleted by housekeeping process.' WHERE var_name='EventLogRetentionTime'\n")
            _T("UPDATE config SET description='Value for status propagation if StatusPropagationAlgorithm server configuration parameter is set to 2 (Fixed).' WHERE var_name='FixedStatusValue'\n")
            _T("UPDATE config SET description='Helpdesk driver name. If set to none, then no helpdesk driver is in use.' WHERE var_name='HelpDeskLink'\n")
            _T("UPDATE config SET description='Number of incorrect password attempts after which a user account is temporarily locked.' WHERE var_name='IntruderLockoutThreshold'\n")
            _T("UPDATE config SET description='Duration of user account temporarily lockout (in minutes) if allowed number of incorrect password attempts was exceeded.' WHERE var_name='IntruderLockoutTime'\n")
            _T("UPDATE config SET description='Jira issue type' WHERE var_name='JiraIssueType'\n")
            _T("UPDATE config SET description='Jira project component' WHERE var_name='JiraProjectComponent'\n")
            _T("UPDATE config SET description='Unique identifier for LDAP group object. If not set, LDAP users are identified by DN.' WHERE var_name='LDAP.GroupUniqueId'\n")
            _T("UPDATE config SET description='Unique identifier for LDAP user object. If not set, LDAP users are identified by DN.' WHERE var_name='LDAP.UserUniqueId'\n")
            _T("UPDATE config SET description='Listener port for connections from mobile agent.' WHERE var_name='MobileDeviceListenerPort'\n")
            _T("UPDATE config SET description='Time (in seconds) to wait for output of a local command object tool.' WHERE var_name='ServerCommandOutputTimeout'\n")
            _T("UPDATE config SET description='Default algorithm for calculation object status from it''s DCIs, alarms and child objects.' WHERE var_name='StatusCalculationAlgorithm'\n")
            _T("UPDATE config SET description='Status shift value for Relative propagation algorithm.' WHERE var_name='StatusShift'\n")
            _T("UPDATE config SET description='Threshold value for Single threshold status calculation algorithm.' WHERE var_name='StatusSingleThreshold'\n")
            _T("UPDATE config SET description='Threshold values for Multiple thresholds status calculation algorithm.' WHERE var_name='StatusThresholds'\n")
            _T("UPDATE config SET description='Values for Translated status propagation algorithm.' WHERE var_name='StatusTranslation'\n")
            _T("UPDATE config_values SET var_description='IP, then hostname' WHERE var_name='Syslog.NodeMatchingPolicy' AND var_value='0'\n")
            _T("DELETE FROM config_values WHERE var_name='StatusPropagationAlgorithm' AND var_value='0'\n")
            _T("UPDATE config SET data_type='C' WHERE var_name='StatusCalculationAlgorithm'\n")
            _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('StatusCalculationAlgorithm','1','Most critical')\n")
            _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('StatusCalculationAlgorithm','2','Single threshold')\n")
            _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('StatusCalculationAlgorithm','3','Multiple thresholds')\n")
            _T("UPDATE config_values SET var_name='Objects.Interfaces.DefaultExpectedState' WHERE var_name='DefaultInterfaceExpectedState'\n")
            _T("<END>");
      CHK_EXEC(SQLBatch(batch));

      CHK_EXEC(CreateConfigParam(_T("NXSL.EnableContainerFunctions"), _T("1"), _T("Enable/disable server-side NXSL functions for containers (such as CreateContainer, BindObject, etc.)."), nullptr, 'B', true, false, false, false));

      CHK_EXEC(SetSchemaLevelForMajorVersion(36, 15));
   }
   CHK_EXEC(SetMinorSchemaVersion(27));
   return true;
}

/**
 * Upgrade from 40.25 to 40.26
 */
static bool H_UpgradeFromV25()
{
   if (GetSchemaLevelForMajorVersion(36) < 14)
   {
      static const TCHAR *batch =
            _T("ALTER TABLE dc_targets ADD geolocation_ctrl_mode integer\n")
            _T("ALTER TABLE dc_targets ADD geo_areas varchar(2000)\n")
            _T("UPDATE dc_targets SET geolocation_ctrl_mode=0\n")
            _T("<END>");
      CHK_EXEC(SQLBatch(batch));
      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("dc_targets"), _T("geolocation_ctrl_mode")));

      // Update access rights for predefined "Admins" group
      DB_RESULT hResult = SQLSelect(_T("SELECT system_access FROM user_groups WHERE id=1073741825"));
      if (hResult != nullptr)
      {
         if (DBGetNumRows(hResult) > 0)
         {
            uint64_t access = DBGetFieldUInt64(hResult, 0, 0);
            access |= SYSTEM_ACCESS_MANAGE_GEO_AREAS;
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

      CHK_EXEC(SetSchemaLevelForMajorVersion(36, 14));
   }
   CHK_EXEC(SetMinorSchemaVersion(26));
   return true;
}

/**
 * Upgrade from 40.24 to 40.25
 */
static bool H_UpgradeFromV24()
{
   if (GetSchemaLevelForMajorVersion(36) < 13)
   {
      CHK_EXEC(CreateTable(
            _T("CREATE TABLE dc_targets (")
            _T("  id integer not null,")
            _T("  config_poll_timestamp integer not null,")
            _T("  instance_poll_timestamp integer not null,")
            _T("PRIMARY KEY(id))")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(36, 13));
   }
   CHK_EXEC(SetMinorSchemaVersion(25));
   return true;
}

/**
 * Upgrade from 40.23 to 40.24
 */
static bool H_UpgradeFromV23()
{
   if (GetSchemaLevelForMajorVersion(36) < 12)
   {
      static const TCHAR *batch =
            _T("UPDATE event_cfg SET event_name='SYS_SNMP_TRAP_FLOOD_DETECTED' WHERE event_code=109\n")
            _T("UPDATE event_cfg SET event_name='SYS_SNMP_TRAP_FLOOD_ENDED' WHERE event_code=110\n")
            _T("<END>");
      CHK_EXEC(SQLBatch(batch));

      CHK_EXEC(CreateEventTemplate(EVENT_GEOLOCATION_CHANGED, _T("SYS_GEOLOCATION_CHANGED"),
               SEVERITY_NORMAL, EF_LOG, _T("2c2f40d4-91d3-441e-92d4-62a32e98a341"),
               _T("Device geolocation changed to %3 %4 (was %7 %8)"),
               _T("Generated when device geolocation change is detected.\r\n")
               _T("Parameters:\r\n")
               _T("   1) Latitude of new location in degrees\r\n")
               _T("   2) Longitude of new location in degrees\r\n")
               _T("   3) Latitude of new location in textual form\r\n")
               _T("   4) Longitude of new location in textual form\r\n")
               _T("   5) Latitude of previous location in degrees\r\n")
               _T("   6) Longitude of previous location in degrees\r\n")
               _T("   7) Latitude of previous location in textual form\r\n")
               _T("   8) Longitude of previous location in textual form")
               ));

      CHK_EXEC(CreateEventTemplate(EVENT_GEOLOCATION_INSIDE_RESTRICTED_AREA, _T("SYS_GEOLOCATION_INSIDE_RESTRICTED_AREA"),
               SEVERITY_WARNING, EF_LOG, _T("94d7a94b-a289-4cbe-8a83-a9c52b36c650"),
               _T("Device geolocation %3 %4 is within restricted area %5"),
               _T("Generated when new device geolocation is within restricted area.\r\n")
               _T("Parameters:\r\n")
               _T("   1) Latitude of new location in degrees\r\n")
               _T("   2) Longitude of new location in degrees\r\n")
               _T("   3) Latitude of new location in textual form\r\n")
               _T("   4) Longitude of new location in textual form\r\n")
               _T("   5) Area name\r\n")
               _T("   6) Area ID")
               ));

      CHK_EXEC(CreateEventTemplate(EVENT_GEOLOCATION_OUTSIDE_ALLOWED_AREA, _T("SYS_GEOLOCATION_OUTSIDE_ALLOWED_AREA"),
               SEVERITY_WARNING, EF_LOG, _T("24dcb315-a5cc-41f5-a813-b0a5f8c31e7d"),
               _T("Device geolocation %3 %4 is outside allowed area"),
               _T("Generated when new device geolocation is within restricted area.\r\n")
               _T("Parameters:\r\n")
               _T("   1) Latitude of new location in degrees\r\n")
               _T("   2) Longitude of new location in degrees\r\n")
               _T("   3) Latitude of new location in textual form\r\n")
               _T("   4) Longitude of new location in textual form")
               ));

      CHK_EXEC(CreateTable(
            _T("CREATE TABLE geo_areas (")
            _T("   id integer not null,")
            _T("   name varchar(127) not null,")
            _T("   comments $SQL:TEXT null,")
            _T("   configuration $SQL:TEXT null,")
            _T("   PRIMARY KEY(id))")));

      CHK_EXEC(SetSchemaLevelForMajorVersion(36, 12));
   }
   CHK_EXEC(SetMinorSchemaVersion(24));
   return true;
}

/**
 * Upgrade from 40.22 to 40.23
 */
static bool H_UpgradeFromV22()
{
   if (GetSchemaLevelForMajorVersion(36) < 11)
   {
      CHK_EXEC(SQLQuery(_T("ALTER TABLE nodes ADD snmp_engine_id varchar(255)")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(36, 11));
   }
   CHK_EXEC(SetMinorSchemaVersion(23));
   return true;
}

/**
 * Upgrade from 40.21 to 40.22
 */
static bool H_UpgradeFromV21()
{
   if (GetSchemaLevelForMajorVersion(36) < 10)
   {
      static const TCHAR *batch =
            _T("ALTER TABLE object_properties ADD category integer\n")
            _T("UPDATE object_properties SET category=0\n")
            _T("<END>");
      CHK_EXEC(SQLBatch(batch));
      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("object_properties"), _T("category")));
      CHK_EXEC(DBRenameColumn(g_dbHandle, _T("object_properties"), _T("image"), _T("map_image")));

      CHK_EXEC(CreateTable(
            _T("CREATE TABLE object_categories (")
            _T("   id integer not null,")
            _T("   name varchar(63) not null,")
            _T("   icon varchar(36) null,")
            _T("   map_image varchar(36) null,")
            _T("   PRIMARY KEY(id))")));

      // Update access rights for predefined "Admins" group
      DB_RESULT hResult = SQLSelect(_T("SELECT system_access FROM user_groups WHERE id=1073741825"));
      if (hResult != nullptr)
      {
         if (DBGetNumRows(hResult) > 0)
         {
            uint64_t access = DBGetFieldUInt64(hResult, 0, 0);
            access |= SYSTEM_ACCESS_OBJECT_CATEGORIES;
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

      CHK_EXEC(SetSchemaLevelForMajorVersion(36, 10));
   }
   CHK_EXEC(SetMinorSchemaVersion(22));
   return true;
}

/**
 * Upgrade from 40.20 to 40.21
 */
static bool H_UpgradeFromV20()
{
   if (GetSchemaLevelForMajorVersion(36) < 9)
   {
      static const TCHAR *batch =
            _T("UPDATE config SET var_name='Syslog.IgnoreMessageTimestamp' WHERE var_name='SyslogIgnoreMessageTimestamp'\n")
            _T("UPDATE config SET var_name='Syslog.ListenPort' WHERE var_name='SyslogListenPort'\n")
            _T("UPDATE config_values SET var_name='Syslog.ListenPort' WHERE var_name='SyslogListenPort'\n")
            _T("UPDATE config SET var_name='Syslog.NodeMatchingPolicy' WHERE var_name='SyslogNodeMatchingPolicy'\n")
            _T("UPDATE config_values SET var_name='Syslog.NodeMatchingPolicy' WHERE var_name='SyslogNodeMatchingPolicy'\n")
            _T("UPDATE config SET var_name='Syslog.RetentionTime' WHERE var_name='SyslogRetentionTime'\n")
            _T("UPDATE config SET var_name='Syslog.EnableListener' WHERE var_name='EnableSyslogReceiver'\n")
            _T("UPDATE config SET description='Retention time in days for stored syslog messages. All messages older than specified will be deleted by housekeeping process.' WHERE var_name='Syslog.RetentionTime'\n")
            _T("<END>");
      CHK_EXEC(SQLBatch(batch));
      CHK_EXEC(CreateConfigParam(_T("Syslog.EnableStorage"), _T("1"),
               _T("Enable/disable local storage of received syslog messages in NetXMS database."),
               nullptr, 'B', true, false, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(36, 9));
   }
   CHK_EXEC(SetMinorSchemaVersion(21));
   return true;
}

/**
 * Upgrade from 40.19 to 40.20
 */
static bool H_UpgradeFromV19()
{
   if (GetSchemaLevelForMajorVersion(36) < 8)
   {
      static const TCHAR *batch =
            _T("ALTER TABLE mobile_devices ADD speed varchar(20)\n")
            _T("ALTER TABLE mobile_devices ADD direction integer\n")
            _T("ALTER TABLE mobile_devices ADD altitude integer\n")
            _T("UPDATE mobile_devices SET speed='-1',direction=-1,altitude=0\n")
            _T("<END>");
      CHK_EXEC(SQLBatch(batch));
      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("mobile_devices"), _T("speed")));
      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("mobile_devices"), _T("direction")));
      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("mobile_devices"), _T("altitude")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(36, 8));
   }
   CHK_EXEC(SetMinorSchemaVersion(20));
   return true;
}

/**
 * Upgrade from 40.18 to 40.19
 */
static bool H_UpgradeFromV18()
{
   if (GetSchemaLevelForMajorVersion(36) < 7)
   {
      CHK_EXEC(SQLQuery(_T("ALTER TABLE mobile_devices ADD comm_protocol varchar(31)")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(36, 7));
   }
   CHK_EXEC(SetMinorSchemaVersion(19));
   return true;
}

/**
 * Upgrade from 40.17 to 40.18
 */
static bool H_UpgradeFromV17()
{
   if (GetSchemaLevelForMajorVersion(36) < 6)
   {
      CHK_EXEC(CreateConfigParam(_T("WinEventLogRetentionTime"), _T("90"), _T("Retention time in days for records in windows event log. All records older than specified will be deleted by housekeeping process."), _T("days"), 'I', true, false, false, false));

      if (g_dbSyntax == DB_SYNTAX_TSDB)
      {
         CHK_EXEC(CreateTable(
               _T("CREATE TABLE win_event_log (")
               _T("  id $SQL:INT64 not null,")
               _T("  event_timestamp timestamptz not null,")
               _T("  node_id integer not null,")
               _T("  zone_uin integer not null,")
               _T("  origin_timestamp integer not null,")
               _T("  log_name varchar(63) null,")
               _T("  event_source varchar(127) null,")
               _T("  event_severity integer not null,")
               _T("  event_code integer not null,")
               _T("  message varchar(2000) null,")
               _T("  raw_data $SQL:TEXT null,")
               _T("PRIMARY KEY(id,event_timestamp))")));
         CHK_EXEC(SQLQuery(_T("SELECT create_hypertable('win_event_log', 'event_timestamp', chunk_time_interval => interval '86400 seconds')")));
      }
      else
      {
         CHK_EXEC(CreateTable(
               _T("CREATE TABLE win_event_log (")
               _T("  id $SQL:INT64 not null,")
               _T("  event_timestamp integer not null,")
               _T("  node_id integer not null,")
               _T("  zone_uin integer not null,")
               _T("  origin_timestamp integer not null,")
               _T("  log_name varchar(63) null,")
               _T("  event_source varchar(127) null,")
               _T("  event_severity integer not null,")
               _T("  event_code integer not null,")
               _T("  message varchar(2000) null,")
               _T("  raw_data $SQL:TEXT null,")
               _T("PRIMARY KEY(id))")));
      }
      CHK_EXEC(SQLQuery(_T("CREATE INDEX idx_win_event_log_timestamp ON win_event_log(event_timestamp)")));
      CHK_EXEC(SQLQuery(_T("CREATE INDEX idx_win_event_log_node ON win_event_log(node_id)")));

      CHK_EXEC(SetSchemaLevelForMajorVersion(36, 6));
   }
   CHK_EXEC(SetMinorSchemaVersion(18));
   return true;
}

/**
 * Upgrade form 40.16 to 40.17
 */
static bool H_UpgradeFromV16()
{
   if (GetSchemaLevelForMajorVersion(36) < 5)
   {
      CHK_EXEC(SQLQuery(_T("UPDATE config SET var_name='SNMP.Traps.LogAll',need_server_restart=0,description='Log all SNMP traps (even those received from addresses not belonging to any known node).' WHERE var_name='LogAllSNMPTraps'")));
      CHK_EXEC(SQLQuery(_T("UPDATE config SET var_name='SNMP.Traps.AllowVarbindsConversion',need_server_restart=0 WHERE var_name='AllowTrapVarbindsConversion'")));
      CHK_EXEC(SQLQuery(_T("UPDATE config SET need_server_restart=0 WHERE var_name='SNMP.Traps.ProcessUnmanagedNodes'")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(36, 5));
   }
   CHK_EXEC(SetMinorSchemaVersion(17));
   return true;
}

/**
 * Upgrade form 40.15 to 40.16
 */
static bool H_UpgradeFromV15()
{
   if (GetSchemaLevelForMajorVersion(36) < 4)
   {
      CHK_EXEC(SQLQuery(_T("ALTER TABLE nodes ADD agent_cert_mapping_method char(1)")));
      CHK_EXEC(SQLQuery(_T("ALTER TABLE nodes ADD agent_cert_mapping_data varchar(500)")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(36, 4));
   }
   CHK_EXEC(SetMinorSchemaVersion(16));
   return true;
}

/**
 * Upgrade form 40.14 to 40.15
 */
static bool H_UpgradeFromV14()
{
   if (GetSchemaLevelForMajorVersion(36) < 3)
   {
      CHK_EXEC(SQLQuery(_T("DROP TABLE certificates")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(36, 3));
   }
   CHK_EXEC(SetMinorSchemaVersion(15));
   return true;
}

/**
 * Upgrade form 40.13 to 40.14
 */
static bool H_UpgradeFromV13()
{
   if (GetSchemaLevelForMajorVersion(36) < 2)
   {
      CHK_EXEC(CreateConfigParam(_T("AgentTunnels.TLS.MinVersion"), _T("2"), _T("Minimal version of TLS protocol used on agent tunnel connection"), nullptr, 'C', true, false, false, false));

      static const TCHAR *batch =
            _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('AgentTunnels.TLS.MinVersion','0','1.0')\n")
            _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('AgentTunnels.TLS.MinVersion','1','1.1')\n")
            _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('AgentTunnels.TLS.MinVersion','2','1.2')\n")
            _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('AgentTunnels.TLS.MinVersion','3','1.3')\n")
            _T("<END>");
      CHK_EXEC(SQLBatch(batch));

      CHK_EXEC(SetSchemaLevelForMajorVersion(36, 2));
   }
   CHK_EXEC(SetMinorSchemaVersion(14));
   return true;
}

/**
 * Upgrade form 40.12 to 40.13
 */
static bool H_UpgradeFromV12()
{
   if (GetSchemaLevelForMajorVersion(36) < 1)
   {
      CHK_EXEC(SQLQuery(_T("ALTER TABLE object_properties ADD name_on_map varchar(63)")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(36, 1));
   }
   CHK_EXEC(SetMinorSchemaVersion(13));
   return true;
}

/**
 * Upgrade form 40.11 to 40.12
 */
static bool H_UpgradeFromV11()
{
   if (GetSchemaLevelForMajorVersion(35) < 16)
   {
      CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET event_name='SNMP_TRAP_FLOOD_ENDED' WHERE event_code=110")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(35, 16));
   }
   CHK_EXEC(SetMinorSchemaVersion(12));
   return true;
}

/**
 * Upgrade form 40.10 to 40.11
 */
static bool H_UpgradeFromV10()
{
   if (GetSchemaLevelForMajorVersion(35) < 15)
   {
      CHK_EXEC(SQLQuery(_T("ALTER TABLE object_properties ADD alias varchar(255)")));
      CHK_EXEC(SQLQuery(_T("UPDATE object_properties SET alias=(SELECT alias FROM interfaces WHERE interfaces.id=object_properties.object_id)")));
      CHK_EXEC(DBDropColumn(g_dbHandle, _T("interfaces"), _T("alias")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(35, 15));
   }
   CHK_EXEC(SetMinorSchemaVersion(11));
   return true;
}

/**
 * Upgrade form 40.9 to 40.10
 */
static bool H_UpgradeFromV9()
{
   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET event_name='SYS_NOTIFICATION_FAILURE',")
         _T("message='Unable to send notification using notification channel %1 to %2',description='")
         _T("Generated when server is unable to send notification.\r\n")
         _T("Parameters:\r\n")
         _T("   1) Notification channel name\r\n")
         _T("   2) Recipient address\r\n")
         _T("   3) Notification subject")
         _T("   4) Notification message")
         _T("' WHERE event_code=22")));

   CHK_EXEC(CreateConfigParam(_T("DefaultNotificationChannel.SMTP.Html"), _T("SMTP-HTML"), _T("Default notification channel for SMTP HTML formatted messages"), nullptr, 'S', true, false, false, false));
   CHK_EXEC(CreateConfigParam(_T("DefaultNotificationChannel.SMTP.Text"), _T("SMTP-Text"), _T("Default notification channel for SMTP Text formatted messages"), nullptr, 'S', true, false, false, false));

   TCHAR server[MAX_STRING_VALUE];
   TCHAR localHostName[MAX_STRING_VALUE];
   TCHAR fromName[MAX_STRING_VALUE];
   TCHAR fromAddr[MAX_STRING_VALUE];
   TCHAR mailEncoding[MAX_STRING_VALUE];
   DBMgrConfigReadStr(_T("SMTP.Server"), server, MAX_STRING_VALUE, _T("localhost"));
   DBMgrConfigReadStr(_T("SMTP.LocalHostName"), localHostName, MAX_STRING_VALUE, _T(""));
   DBMgrConfigReadStr(_T("SMTP.FromName"), fromName, MAX_STRING_VALUE, _T("NetXMS Server"));
   DBMgrConfigReadStr(_T("SMTP.FromAddr"), fromAddr, MAX_STRING_VALUE, _T("netxms@localhost"));
   DBMgrConfigReadStr(_T("MailEncoding"), mailEncoding, MAX_STRING_VALUE, _T("utf8"));
   uint32_t retryCount = DBMgrConfigReadUInt32(_T("SMTP.RetryCount"), 1);
   uint16_t port = static_cast<uint16_t>(DBMgrConfigReadUInt32(_T("SMTP.Port"), 25));

   const TCHAR *configTemplate = _T("Server=%s\r\n")
         _T("RetryCount=%u\r\n")
         _T("Port=%hu\r\n")
         _T("LocalHostName=%s\r\n")
         _T("FromName=%s\r\n")
         _T("FromAddr=%s\r\n")
         _T("MailEncoding=%s\r\n")
         _T("IsHTML=%s");

   DB_STATEMENT hStmt = DBPrepare(g_dbHandle, _T("INSERT INTO notification_channels (name,driver_name,description,configuration) VALUES (?,'SMTP',?,?)"), true);
   if (hStmt != NULL)
   {
      TCHAR tmpConfig[1024];
      _sntprintf(tmpConfig, 1024, configTemplate, server, retryCount, port, localHostName, fromName, fromAddr, mailEncoding, _T("no"));
     DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, _T("SMTP-Text"), DB_BIND_STATIC);
     DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, _T("Default SMTP text channel (automatically migrated)"), DB_BIND_STATIC);
     DBBind(hStmt, 3, DB_SQLTYPE_TEXT, tmpConfig, DB_BIND_STATIC);
     if (!SQLExecute(hStmt) && !g_ignoreErrors)
     {
        DBFreeStatement(hStmt);
        return false;
     }

     _sntprintf(tmpConfig, 1024, configTemplate, server, retryCount, port, localHostName, fromName, fromAddr, mailEncoding, _T("yes"));
    DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, _T("SMTP-HTML"), DB_BIND_STATIC);
    DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, _T("Default SMTP HTML channel (automatically migrated)"), DB_BIND_STATIC);
    DBBind(hStmt, 3, DB_SQLTYPE_TEXT, tmpConfig, DB_BIND_STATIC);
    if (!SQLExecute(hStmt) && !g_ignoreErrors)
    {
       DBFreeStatement(hStmt);
       return false;
    }

     DBFreeStatement(hStmt);
   }
   else if (!g_ignoreErrors)
     return false;

   //Delete old configuration parameters
   CHK_EXEC(SQLQuery(_T("DELETE FROM config WHERE var_name='SMTP.Server'")));
   CHK_EXEC(SQLQuery(_T("DELETE FROM config WHERE var_name='SMTP.LocalHostName'")));
   CHK_EXEC(SQLQuery(_T("DELETE FROM config WHERE var_name='SMTP.FromName'")));
   CHK_EXEC(SQLQuery(_T("DELETE FROM config WHERE var_name='SMTP.FromAddr'")));
   CHK_EXEC(SQLQuery(_T("DELETE FROM config WHERE var_name='SMTP.RetryCount'")));
   CHK_EXEC(SQLQuery(_T("DELETE FROM config WHERE var_name='SMTP.Port'")));
   CHK_EXEC(SQLQuery(_T("DELETE FROM config WHERE var_name='MailEncoding'")));

   CHK_EXEC(SQLQuery(_T("UPDATE actions SET action_type=3,channel_name='SMTP-Text' WHERE action_type=2")));

   CHK_EXEC(SetMinorSchemaVersion(10));
   return true;
}

/**
 * Upgrade form 40.8 to 40.9
 */
static bool H_UpgradeFromV8()
{
   if (GetSchemaLevelForMajorVersion(35) < 14)
   {
      CHK_EXEC(SQLQuery(_T("ALTER TABLE policy_action_list ADD blocking_timer_key varchar(127)")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(35, 14));
   }
   CHK_EXEC(SetMinorSchemaVersion(9));
   return true;
}

/**
 * Upgrade form 40.7 to 40.8
 */
static bool H_UpgradeFromV7()
{
   if (GetSchemaLevelForMajorVersion(35) < 13)
   {
      CHK_EXEC(SQLQuery(_T("DELETE FROM config WHERE var_name='AllowDirectNotifications'")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(35, 13));
   }
   CHK_EXEC(SetMinorSchemaVersion(8));
   return true;
}

/**
 * Upgrade form 40.6 to 40.7
 */
static bool H_UpgradeFromV6()
{
   if (GetSchemaLevelForMajorVersion(35) < 12)
   {
      CHK_EXEC(SQLQuery(_T("ALTER TABLE nodes ADD cip_vendor_code integer")));
      CHK_EXEC(SQLQuery(_T("UPDATE nodes SET cip_vendor_code=0")));
      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("nodes"), _T("cip_vendor_code")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(35, 12));
   }
   CHK_EXEC(SetMinorSchemaVersion(7));
   return true;
}

/**
 * Upgrade from 40.5 to 40.6
 */
static bool H_UpgradeFromV5()
{
   if (GetSchemaLevelForMajorVersion(35) < 11)
   {
      CHK_EXEC(CreateConfigParam(_T("Events.Processor.PoolSize"), _T("1"), _T("Number of threads for parallel event processing."), _T("threads"), 'I', true, true, false, false));
      CHK_EXEC(CreateConfigParam(_T("Events.Processor.QueueSelector"), _T("%z"), _T("Queue selector for parallel event processing."), nullptr, 'S', true, true, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(35, 11));
   }
   CHK_EXEC(SetMinorSchemaVersion(6));
   return true;
}

/**
 * Update group ID in given table
 */
bool UpdateGroupId(const TCHAR *table, const TCHAR *column);

/**
 * Upgrade from 40.4 to 40.5
 */
static bool H_UpgradeFromV4()
{
   if (GetSchemaLevelForMajorVersion(35) < 10)
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
      CHK_EXEC(SetSchemaLevelForMajorVersion(35, 10));
   }
   CHK_EXEC(SetMinorSchemaVersion(5));
   return true;
}

/**
 * Upgrade from 40.3 to 40.4
 */
static bool H_UpgradeFromV3()
{
   if (GetSchemaLevelForMajorVersion(35) < 9)
   {
      CHK_EXEC(CreateConfigParam(_T("SNMP.Traps.RateLimit.Threshold"), _T("0"), _T("Threshold for number of SNMP traps per second that defines SNMP trap flood condition. Detection is disabled if 0 is set."), _T("seconds"), 'I', true, false, false, false));
      CHK_EXEC(CreateConfigParam(_T("SNMP.Traps.RateLimit.Duration"), _T("15"), _T("Time period for SNMP traps per second to be above threshold that defines SNMP trap flood condition."), _T("seconds"), 'I', true, false, false, false));

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

      CHK_EXEC(SetSchemaLevelForMajorVersion(35, 9));
   }

   CHK_EXEC(SetMinorSchemaVersion(4));
   return true;
}

/**
 * Upgrade from 40.2 to 40.3
 */
static bool H_UpgradeFromV2()
{
   if (GetSchemaLevelForMajorVersion(35) < 8)
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

      CHK_EXEC(SetSchemaLevelForMajorVersion(35, 8));
   }
   CHK_EXEC(SetMinorSchemaVersion(3));
   return true;
}

/**
 * Upgrade from 40.1 to 40.2
 */
static bool H_UpgradeFromV1()
{
   if (GetSchemaLevelForMajorVersion(35) < 7)
   {
      CHK_EXEC(SQLQuery(
            _T("INSERT INTO script_library (guid,script_id,script_name,script_code) ")
            _T("VALUES ('9c2dba59-493b-4645-9159-2ad7a28ea611',23,'Hook::OpenBoundTunnel','")
            _T("/* Available global variables:\r\n")
            _T(" *  $tunnel - incoming tunnel information (object of ''Tunnel'' class)\r\n")
            _T(" *\r\n")
            _T(" * Expected return value:\r\n")
            _T(" *  none - returned value is ignored\r\n */\r\n')")));
      CHK_EXEC(SQLQuery(
            _T("INSERT INTO script_library (guid,script_id,script_name,script_code) ")
            _T("VALUES ('64c90b92-27e9-4a96-98ea-d0e152d71262',24,'Hook::OpenUnboundTunnel','")
            _T("/* Available global variables:\r\n")
            _T(" *  $node - node this tunnel was bound to (object of ''Node'' class)\r\n")
            _T(" *  $tunnel - incoming tunnel information (object of ''Tunnel'' class)\r\n")
            _T(" *\r\n")
            _T(" * Expected return value:\r\n")
            _T(" *  none - returned value is ignored\r\n */\r\n')")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(35, 7));
   }

   CHK_EXEC(SetMinorSchemaVersion(2));
   return true;
}

/**
 * Upgrade from 40.0 to 40.1
 */
static bool H_UpgradeFromV0()
{
   if (GetSchemaLevelForMajorVersion(35) < 6)
   {
      CHK_EXEC(CreateConfigParam(_T("RoamingServer"), _T("0"), _T("Enable/disable roaming mode for server (when server can be disconnected from one network and connected to another or IP address of the server can change)."), nullptr, 'B', true, false, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(35, 6));
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
} s_dbUpgradeMap[] =
{
   { 103, 41, 0, H_UpgradeFromV103 },
   { 102, 40, 103, H_UpgradeFromV102 },
   { 101, 40, 102, H_UpgradeFromV101 },
   { 100, 40, 101, H_UpgradeFromV100 },
   { 99, 40, 100, H_UpgradeFromV99 },
   { 98, 40, 99, H_UpgradeFromV98 },
   { 97, 40, 98, H_UpgradeFromV97 },
   { 96, 40, 97, H_UpgradeFromV96 },
   { 95, 40, 96, H_UpgradeFromV95 },
   { 94, 40, 95, H_UpgradeFromV94 },
   { 93, 40, 94, H_UpgradeFromV93 },
   { 92, 40, 93, H_UpgradeFromV92 },
   { 91, 40, 92, H_UpgradeFromV91 },
   { 90, 40, 91, H_UpgradeFromV90 },
   { 89, 40, 90, H_UpgradeFromV89 },
   { 88, 40, 89, H_UpgradeFromV88 },
   { 87, 40, 88, H_UpgradeFromV87 },
   { 86, 40, 87, H_UpgradeFromV86 },
   { 85, 40, 86, H_UpgradeFromV85 },
   { 84, 40, 85, H_UpgradeFromV84 },
   { 83, 40, 84, H_UpgradeFromV83 },
   { 82, 40, 83, H_UpgradeFromV82 },
   { 81, 40, 82, H_UpgradeFromV81 },
   { 80, 40, 81, H_UpgradeFromV80 },
   { 79, 40, 80, H_UpgradeFromV79 },
   { 78, 40, 79, H_UpgradeFromV78 },
   { 77, 40, 78, H_UpgradeFromV77 },
   { 76, 40, 77, H_UpgradeFromV76 },
   { 75, 40, 76, H_UpgradeFromV75 },
   { 74, 40, 75, H_UpgradeFromV74 },
   { 73, 40, 74, H_UpgradeFromV73 },
   { 72, 40, 73, H_UpgradeFromV72 },
   { 71, 40, 72, H_UpgradeFromV71 },
   { 70, 40, 71, H_UpgradeFromV70 },
   { 69, 40, 70, H_UpgradeFromV69 },
   { 68, 40, 69, H_UpgradeFromV68 },
   { 67, 40, 68, H_UpgradeFromV67 },
   { 66, 40, 67, H_UpgradeFromV66 },
   { 65, 40, 66, H_UpgradeFromV65 },
   { 64, 40, 65, H_UpgradeFromV64 },
   { 63, 40, 64, H_UpgradeFromV63 },
   { 62, 40, 63, H_UpgradeFromV62 },
   { 61, 40, 62, H_UpgradeFromV61 },
   { 60, 40, 61, H_UpgradeFromV60 },
   { 59, 40, 60, H_UpgradeFromV59 },
   { 58, 40, 59, H_UpgradeFromV58 },
   { 57, 40, 58, H_UpgradeFromV57 },
   { 56, 40, 57, H_UpgradeFromV56 },
   { 55, 40, 56, H_UpgradeFromV55 },
   { 54, 40, 55, H_UpgradeFromV54 },
   { 53, 40, 54, H_UpgradeFromV53 },
   { 52, 40, 53, H_UpgradeFromV52 },
   { 51, 40, 52, H_UpgradeFromV51 },
   { 50, 40, 51, H_UpgradeFromV50 },
   { 49, 40, 50, H_UpgradeFromV49 },
   { 48, 40, 49, H_UpgradeFromV48 },
   { 47, 40, 48, H_UpgradeFromV47 },
   { 46, 40, 47, H_UpgradeFromV46 },
   { 45, 40, 46, H_UpgradeFromV45 },
   { 44, 40, 45, H_UpgradeFromV44 },
   { 43, 40, 44, H_UpgradeFromV43 },
   { 42, 40, 43, H_UpgradeFromV42 },
   { 41, 40, 42, H_UpgradeFromV41 },
   { 40, 40, 41, H_UpgradeFromV40 },
   { 39, 40, 40, H_UpgradeFromV39 },
   { 38, 40, 39, H_UpgradeFromV38 },
   { 37, 40, 38, H_UpgradeFromV37 },
   { 36, 40, 37, H_UpgradeFromV36 },
   { 35, 40, 38, H_UpgradeFromV35 },
   { 34, 40, 35, H_UpgradeFromV34 },
   { 33, 40, 34, H_UpgradeFromV33 },
   { 32, 40, 33, H_UpgradeFromV32 },
   { 31, 40, 32, H_UpgradeFromV31 },
   { 30, 40, 31, H_UpgradeFromV30 },
   { 29, 40, 30, H_UpgradeFromV29 },
   { 28, 40, 29, H_UpgradeFromV28 },
   { 27, 40, 28, H_UpgradeFromV27 },
   { 26, 40, 27, H_UpgradeFromV26 },
   { 25, 40, 26, H_UpgradeFromV25 },
   { 24, 40, 25, H_UpgradeFromV24 },
   { 23, 40, 24, H_UpgradeFromV23 },
   { 22, 40, 23, H_UpgradeFromV22 },
   { 21, 40, 22, H_UpgradeFromV21 },
   { 20, 40, 21, H_UpgradeFromV20 },
   { 19, 40, 20, H_UpgradeFromV19 },
   { 18, 40, 19, H_UpgradeFromV18 },
   { 17, 40, 18, H_UpgradeFromV17 },
   { 16, 40, 17, H_UpgradeFromV16 },
   { 15, 40, 16, H_UpgradeFromV15 },
   { 14, 40, 15, H_UpgradeFromV14 },
   { 13, 40, 14, H_UpgradeFromV13 },
   { 12, 40, 13, H_UpgradeFromV12 },
   { 11, 40, 12, H_UpgradeFromV11 },
   { 10, 40, 11, H_UpgradeFromV10 },
   { 9,  40, 10, H_UpgradeFromV9  },
   { 8,  40, 9,  H_UpgradeFromV8  },
   { 7,  40, 8,  H_UpgradeFromV7  },
   { 6,  40, 7,  H_UpgradeFromV6  },
   { 5,  40, 6,  H_UpgradeFromV5  },
   { 4,  40, 5,  H_UpgradeFromV4  },
   { 3,  40, 4,  H_UpgradeFromV3  },
   { 2,  40, 3,  H_UpgradeFromV2  },
   { 1,  40, 2,  H_UpgradeFromV1  },
   { 0,  40, 1,  H_UpgradeFromV0  },
   { 0,  0,  0,  nullptr          }
};

/**
 * Upgrade database to new version
 */
bool MajorSchemaUpgrade_V40()
{
   int32_t major, minor;
   if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
      return false;

   while (major == 40)
   {
      // Find upgrade procedure
      int i;
      for(i = 0; s_dbUpgradeMap[i].upgradeProc != nullptr; i++)
         if (s_dbUpgradeMap[i].version == minor)
            break;
      if (s_dbUpgradeMap[i].upgradeProc == nullptr)
      {
         _tprintf(_T("Unable to find upgrade procedure for version 40.%d\n"), minor);
         return false;
      }
      _tprintf(_T("Upgrading from version 40.%d to %d.%d\n"), minor, s_dbUpgradeMap[i].nextMajor, s_dbUpgradeMap[i].nextMinor);
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
