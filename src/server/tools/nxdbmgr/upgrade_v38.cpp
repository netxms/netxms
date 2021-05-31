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
** File: upgrade_v38.cpp
**
**/

#include "nxdbmgr.h"
#include <nxevent.h>

/**
 * Upgrade from 38.17 to 39.0
 */
static bool H_UpgradeFromV17()
{
   CHK_EXEC(SetMajorSchemaVersion(39, 0));
   return true;
}

/**
 * Upgrade from 38.16 to 38.17
 */
static bool H_UpgradeFromV16()
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

   CHK_EXEC(SetMinorSchemaVersion(17));
   return true;
}

/**
 * Upgrade from 38.15 to 38.16
 */
static bool H_UpgradeFromV15()
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

   CHK_EXEC(SetMinorSchemaVersion(16));
   return true;
}

/**
 * Upgrade from 38.14 to 38.15
 */
static bool H_UpgradeFromV14()
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

   CHK_EXEC(SetMinorSchemaVersion(15));
   return true;
}

/**
 * Upgrade from 38.13 to 38.14
 */
static bool H_UpgradeFromV13()
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
   CHK_EXEC(SetMinorSchemaVersion(14));
   return true;
}

/**
 * Upgrade from 38.12 to 38.13
 */
static bool H_UpgradeFromV12()
{
   int ruleId = NextFreeEPPruleID();

   TCHAR query[1024];
   _sntprintf(query, 1024, _T("INSERT INTO event_policy (rule_id,rule_guid,flags,comments,alarm_message,alarm_severity,alarm_key,script,alarm_timeout,alarm_timeout_event) ")
            _T("VALUES (%d,'b517ca11-cdf8-4813-87fa-9a2ace070564',7944,'Generate alarm on agent policy validation failure','%%m',5,'POLICY_ERROR_%%2_%%5','',0,%d)"),
            ruleId, EVENT_ALARM_TIMEOUT);
   CHK_EXEC(SQLQuery(query));

   _sntprintf(query, 256, _T("INSERT INTO policy_event_list (rule_id,event_code) VALUES (%d,%d)"), ruleId, EVENT_POLICY_VALIDATION_ERROR);
   CHK_EXEC(SQLQuery(query));

   CHK_EXEC(SetMinorSchemaVersion(13));
   return true;
}

/**
 * Upgrade from 38.11 to 38.12
 */
static bool H_UpgradeFromV11()
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
   CHK_EXEC(SetMinorSchemaVersion(12));
   return true;
}

/**
 * Upgrade from 38.10 to 38.11
 */
static bool H_UpgradeFromV10()
{
   if ((g_dbSyntax == DB_SYNTAX_DB2) || (g_dbSyntax == DB_SYNTAX_INFORMIX) || (g_dbSyntax == DB_SYNTAX_ORACLE))
      CHK_EXEC(SQLQuery(_T("UPDATE nodes SET last_agent_comm_time=0 WHERE (BITAND(capabilities, 2) = 0) AND (last_agent_comm_time > 0)")));
   else
      CHK_EXEC(SQLQuery(_T("UPDATE nodes SET last_agent_comm_time=0 WHERE ((capabilities & 2) = 0) AND (last_agent_comm_time > 0)")));
   CHK_EXEC(SetMinorSchemaVersion(11));
   return true;
}

/**
 * Upgrade from 38.9 to 38.10
 */
static bool H_UpgradeFromV9()
{
   static const TCHAR *batch =
      _T("ALTER TABLE interfaces ADD last_known_oper_state integer\n")
      _T("ALTER TABLE interfaces ADD last_known_admin_state integer\n")
      _T("UPDATE interfaces SET last_known_oper_state=0,last_known_admin_state=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("interfaces"), _T("last_known_oper_state")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("interfaces"), _T("last_known_admin_state")));
   CHK_EXEC(SetMinorSchemaVersion(10));
   return true;
}

/**
 * Upgrade from 38.8 to 38.9
 */
static bool H_UpgradeFromV8()
{
   CHK_EXEC(SQLQuery(_T("DELETE FROM config WHERE var_name='JobHistoryRetentionTime'")));
   CHK_EXEC(SetMinorSchemaVersion(9));
   return true;
}

/**
 * Upgrade from 38.7 to 38.8
 */
static bool H_UpgradeFromV7()
{
   CHK_EXEC(SQLQuery(_T("ALTER TABLE raw_dci_values ADD cache_timestamp integer")));
   CHK_EXEC(SQLQuery(_T("UPDATE raw_dci_values SET cache_timestamp=0")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("raw_dci_values"), _T("cache_timestamp")));
   CHK_EXEC(SetMinorSchemaVersion(8));
   return true;
}

/**
 * Upgrade from 38.6 to 38.7
 */
static bool H_UpgradeFromV6()
{
   CHK_EXEC(SQLQuery(_T("DROP TABLE job_history")));
   CHK_EXEC(SetMinorSchemaVersion(7));
   return true;
}

/**
 * Upgrade from 38.5 to 38.6
 */
static bool H_UpgradeFromV5()
{
   static const TCHAR *batch =
         _T("ALTER TABLE policy_action_list ADD snooze_time integer\n")
         _T("UPDATE policy_action_list SET snooze_time=0\n")
         _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("policy_action_list"), _T("snooze_time")));
   CHK_EXEC(SetMinorSchemaVersion(6));
   return true;
}


/**
 * Upgrade from 38.4 to 38.5
 */
static bool H_UpgradeFromV4()
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
   CHK_EXEC(SetMinorSchemaVersion(5));
   return true;
}

/**
 * Upgrade from 38.3 to 38.4
 */
static bool H_UpgradeFromV3()
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

   CHK_EXEC(SetMinorSchemaVersion(4));
   return true;
}

/**
 * Upgrade from 38.2 to 38.3
 */
static bool H_UpgradeFromV2()
{
   if (DBIsTableExist(g_dbHandle, _T("report_results")))
   {
      CHK_EXEC(SQLQuery(_T("DROP TABLE report_results")));
   }
   CHK_EXEC(SetMinorSchemaVersion(3));
   return true;
}

/**
 * Upgrade from 38.1 to 38.2
 */
static bool H_UpgradeFromV1()
{
   if (DBIsTableExist(g_dbHandle, _T("report_notifications")))
   {
      CHK_EXEC(SQLQuery(_T("DROP TABLE report_notifications")));
   }
   CHK_EXEC(SetMinorSchemaVersion(2));
   return true;
}

/**
 * Upgrade from 38.0 to 38.3
 * Upgrades straight to version 3, versions 1 and 2 skipped because in version 3 all reporting tables are already dropped.
 */
static bool H_UpgradeFromV0()
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
   CHK_EXEC(SetMinorSchemaVersion(3));
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
   { 17, 39, 0,  H_UpgradeFromV17 },
   { 16, 38, 17, H_UpgradeFromV16 },
   { 15, 38, 16, H_UpgradeFromV15 },
   { 14, 38, 15, H_UpgradeFromV14 },
   { 13, 38, 14, H_UpgradeFromV13 },
   { 12, 38, 13, H_UpgradeFromV12 },
   { 11, 38, 12, H_UpgradeFromV11 },
   { 10, 38, 11, H_UpgradeFromV10 },
   { 9,  38, 10, H_UpgradeFromV9  },
   { 8,  38, 9,  H_UpgradeFromV8  },
   { 7,  38, 8,  H_UpgradeFromV7  },
   { 6,  38, 7,  H_UpgradeFromV6  },
   { 5,  38, 6,  H_UpgradeFromV5  },
   { 4,  38, 5,  H_UpgradeFromV4  },
   { 3,  38, 4,  H_UpgradeFromV3  },
   { 2,  38, 3,  H_UpgradeFromV2  },
   { 1,  38, 2,  H_UpgradeFromV1  },
   { 0,  38, 3,  H_UpgradeFromV0  },
   { 0,  0,  0,  nullptr          }
};

/**
 * Upgrade database to new version
 */
bool MajorSchemaUpgrade_V38()
{
   int32_t major, minor;
   if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
      return false;

   while(major == 38)
   {
      // Find upgrade procedure
      int i;
      for(i = 0; s_dbUpgradeMap[i].upgradeProc != nullptr; i++)
         if (s_dbUpgradeMap[i].version == minor)
            break;
      if (s_dbUpgradeMap[i].upgradeProc == nullptr)
      {
         _tprintf(_T("Unable to find upgrade procedure for version 38.%d\n"), minor);
         return false;
      }
      _tprintf(_T("Upgrading from version 38.%d to %d.%d\n"), minor, s_dbUpgradeMap[i].nextMajor, s_dbUpgradeMap[i].nextMinor);
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
