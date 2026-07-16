/*
** nxdbmgr - NetXMS database manager
** Copyright (C) 2026 Raden Solutions
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
** File: upgrade_v70.cpp
**
**/

#include "nxdbmgr.h"
#include <nxevent.h>

/**
 * Upgrade from 70.9 to 70.10
 */
static bool H_UpgradeFromV9()
{
   CHK_EXEC(CreateEventTemplate(EVENT_DEVICE_CONFIG_RESTORE_STARTED, L"SYS_DEVICE_CONFIG_RESTORE_STARTED",
      EVENT_SEVERITY_NORMAL, EF_LOG, L"7a90ba9a-71dd-4eb6-808f-ec4ae272a10c",
      L"Device configuration restore initiated by user %5",
      L"Generated when device configuration restore is started.\r\n"
      L"Parameters:\r\n"
      L"   1) sourceNodeId - Source node ID (0 for client-supplied configuration)\r\n"
      L"   2) sourceNodeName - Source node name\r\n"
      L"   3) backupId - Source backup ID\r\n"
      L"   4) configHash - SHA-256 hash of configuration being applied\r\n"
      L"   5) userName - Name of user who initiated the restore"));

   CHK_EXEC(CreateEventTemplate(EVENT_DEVICE_CONFIG_RESTORE_COMPLETED, L"SYS_DEVICE_CONFIG_RESTORE_COMPLETED",
      EVENT_SEVERITY_NORMAL, EF_LOG, L"44a8f8d3-2b0c-4462-b54e-4898a7d608d2",
      L"Device configuration restore completed successfully",
      L"Generated when device configuration restore is successfully completed.\r\n"
      L"Parameters:\r\n"
      L"   1) sourceNodeId - Source node ID (0 for client-supplied configuration)\r\n"
      L"   2) sourceNodeName - Source node name\r\n"
      L"   3) backupId - Source backup ID\r\n"
      L"   4) configHash - SHA-256 hash of applied configuration\r\n"
      L"   5) userName - Name of user who initiated the restore\r\n"
      L"   6) newConfigHash - SHA-256 hash of device running configuration after restore"));

   CHK_EXEC(CreateEventTemplate(EVENT_DEVICE_CONFIG_RESTORE_FAILED, L"SYS_DEVICE_CONFIG_RESTORE_FAILED",
      EVENT_SEVERITY_MAJOR, EF_LOG, L"3040d920-a920-46af-adb0-825b9acb99b4",
      L"Device configuration restore failed (%6)",
      L"Generated when device configuration restore fails.\r\n"
      L"Parameters:\r\n"
      L"   1) sourceNodeId - Source node ID (0 for client-supplied configuration)\r\n"
      L"   2) sourceNodeName - Source node name\r\n"
      L"   3) backupId - Source backup ID\r\n"
      L"   4) configHash - SHA-256 hash of configuration being applied\r\n"
      L"   5) userName - Name of user who initiated the restore\r\n"
      L"   6) errorMessage - Error message"));

   CHK_EXEC(SetMinorSchemaVersion(10));
   return true;
}

/**
 * Upgrade from 70.8 to 70.9
 */
static bool H_UpgradeFromV8()
{
   if (GetSchemaLevelForMajorVersion(62) < 34)
   {
      CHK_EXEC(SQLQuery(L"ALTER TABLE package_deployment_jobs ADD retry_count integer"));
      CHK_EXEC(SQLQuery(L"UPDATE package_deployment_jobs SET retry_count=0"));
      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, L"package_deployment_jobs", L"retry_count"));

      CHK_EXEC(CreateConfigParam(L"PackageDeployment.MaxRetryCount", L"16",
         L"Maximum number of automatic retry attempts for a failed package deployment (for example when target node is offline) before the job is marked as permanently failed.",
         L"attempts", 'I', true, false, false, false));

      CHK_EXEC(SetSchemaLevelForMajorVersion(62, 34));
   }
   CHK_EXEC(SetMinorSchemaVersion(9));
   return true;
}

/**
 * Upgrade from 70.7 to 70.8
 */
static bool H_UpgradeFromV7()
{
   if (GetSchemaLevelForMajorVersion(62) < 33)
   {
      CHK_EXEC(CreateConfigParam(L"PackageDeployment.JobHistorySize", L"1000",
         L"Maximum number of most recent completed package deployment jobs returned to clients from the deployment history.",
         L"jobs", 'I', true, false, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(62, 33));
   }
   CHK_EXEC(SetMinorSchemaVersion(8));
   return true;
}

/**
 * Upgrade from 70.6 to 70.7
 */
static bool H_UpgradeFromV6()
{
   CHK_EXEC(CreateTable(
      L"CREATE TABLE ai_operator_instances ("
      L"   id integer not null,"
      L"   name varchar(63) not null,"
      L"   description varchar(255) null,"
      L"   owner_user_id integer not null,"
      L"   enabled char(1) not null,"
      L"   scope_filter $SQL:TEXT null,"
      L"   model_slot varchar(63) null,"
      L"   min_interval integer not null,"
      L"   max_interval integer not null,"
      L"   daily_token_budget integer not null,"
      L"   tokens_used $SQL:INT64 not null,"
      L"   usage_day integer not null,"
      L"   persona_prompt $SQL:TEXT null,"
      L"   current_focus varchar(255) null,"
      L"   watch_list $SQL:TEXT null,"
      L"   memento $SQL:TEXT null,"
      L"   observation_retention_days integer not null,"
      L"   observation_max_records integer not null,"
      L"   last_execution_time integer not null,"
      L"   next_execution_time integer not null,"
      L"   iteration integer not null,"
      L"   created integer not null,"
      L"   modified integer not null,"
      L"   PRIMARY KEY(id))"));

   if (g_dbSyntax == DB_SYNTAX_TSDB)
   {
      CHK_EXEC(CreateTable(
         L"CREATE TABLE ai_operator_observations ("
         L"   id $SQL:INT64 not null,"
         L"   observation_timestamp timestamptz not null,"
         L"   instance_id integer not null,"
         L"   severity integer not null,"
         L"   title varchar(255) null,"
         L"   body $SQL:TEXT null,"
         L"   object_id integer not null,"
         L"   refs $SQL:TEXT null,"
         L"   state char(1) not null,"
         L"   PRIMARY KEY(id,observation_timestamp))"));
      CHK_EXEC(SQLQuery(L"SELECT create_hypertable('ai_operator_observations', 'observation_timestamp', chunk_time_interval => interval '86400 seconds')"));

      CHK_EXEC(CreateTable(
         L"CREATE TABLE ai_operator_execution_log ("
         L"   record_id $SQL:INT64 not null,"
         L"   execution_timestamp timestamptz not null,"
         L"   instance_id integer not null,"
         L"   instance_name varchar(63) null,"
         L"   status char(1) not null,"
         L"   iteration integer not null,"
         L"   duration_ms integer not null,"
         L"   input_tokens integer not null,"
         L"   output_tokens integer not null,"
         L"   explanation $SQL:TEXT null,"
         L"   PRIMARY KEY(record_id,execution_timestamp))"));
      CHK_EXEC(SQLQuery(L"SELECT create_hypertable('ai_operator_execution_log', 'execution_timestamp', chunk_time_interval => interval '86400 seconds')"));
   }
   else
   {
      CHK_EXEC(CreateTable(
         L"CREATE TABLE ai_operator_observations ("
         L"   id $SQL:INT64 not null,"
         L"   observation_timestamp integer not null,"
         L"   instance_id integer not null,"
         L"   severity integer not null,"
         L"   title varchar(255) null,"
         L"   body $SQL:TEXT null,"
         L"   object_id integer not null,"
         L"   refs $SQL:TEXT null,"
         L"   state char(1) not null,"
         L"   PRIMARY KEY(id))"));

      CHK_EXEC(CreateTable(
         L"CREATE TABLE ai_operator_execution_log ("
         L"   record_id $SQL:INT64 not null,"
         L"   execution_timestamp integer not null,"
         L"   instance_id integer not null,"
         L"   instance_name varchar(63) null,"
         L"   status char(1) not null,"
         L"   iteration integer not null,"
         L"   duration_ms integer not null,"
         L"   input_tokens integer not null,"
         L"   output_tokens integer not null,"
         L"   explanation $SQL:TEXT null,"
         L"   PRIMARY KEY(record_id))"));
   }

   CHK_EXEC(SQLQuery(L"CREATE INDEX idx_ai_op_obs_timestamp ON ai_operator_observations(observation_timestamp)"));
   CHK_EXEC(SQLQuery(L"CREATE INDEX idx_ai_op_obs_instance_id ON ai_operator_observations(instance_id)"));
   CHK_EXEC(SQLQuery(L"CREATE INDEX idx_ai_op_obs_object_id ON ai_operator_observations(object_id)"));
   CHK_EXEC(SQLQuery(L"CREATE INDEX idx_ai_op_exec_log_timestamp ON ai_operator_execution_log(execution_timestamp)"));
   CHK_EXEC(SQLQuery(L"CREATE INDEX idx_ai_op_exec_log_instance_id ON ai_operator_execution_log(instance_id)"));

   // Create AI operator account with read access to network and infrastructure root objects.
   // ID cannot be fixed on upgraded systems - allocate next free one; server code resolves this account by name.
   DB_RESULT hResult = DBSelect(g_dbHandle, L"SELECT max(id) FROM users");
   if (hResult != nullptr)
   {
      uint32_t userId = DBGetFieldLong(hResult, 0, 0) + 1;
      DBFreeResult(hResult);

      TCHAR query[1024];
      _sntprintf(query, 1024,
         L"INSERT INTO users "
         L"(id,name,password,system_access,flags,full_name,description,grace_logins,auth_method,guid,cert_mapping_method,"
         L"cert_mapping_data,auth_failures,last_passwd_change,min_passwd_length,disabled_until,last_login,created,tfa_grace_logins) "
         L"VALUES (%u,'ai-operator','$$'," UINT64_FMT L",84,'','AI operator account',5,0,'%s',0,'',0,0,0,0,0,0,5)",
         userId, SYSTEM_ACCESS_USE_AI_ASSISTANT, uuid::generate().toString().cstr());
      CHK_EXEC(SQLQuery(query));

      _sntprintf(query, 1024, L"INSERT INTO acl (object_id,user_id,access_rights) VALUES (1,%u,%d)",
         userId, OBJECT_ACCESS_READ | OBJECT_ACCESS_READ_ALARMS);
      CHK_EXEC(SQLQuery(query));
      _sntprintf(query, 1024, L"INSERT INTO acl (object_id,user_id,access_rights) VALUES (2,%u,%d)",
         userId, OBJECT_ACCESS_READ | OBJECT_ACCESS_READ_ALARMS);
      CHK_EXEC(SQLQuery(query));
   }
   else if (!g_ignoreErrors)
   {
      return false;
   }

   // Add SYSTEM_ACCESS_MANAGE_AI_OPERATORS (bit 58 = 0x400000000000000 = 288230376151711744) to Admins group
   if ((g_dbSyntax == DB_SYNTAX_DB2) || (g_dbSyntax == DB_SYNTAX_INFORMIX) || (g_dbSyntax == DB_SYNTAX_ORACLE))
   {
      CHK_EXEC(SQLQuery(L"UPDATE user_groups SET system_access=system_access+288230376151711744 WHERE id=1073741825 AND BITAND(system_access, 288230376151711744)=0"));
   }
   else if (g_dbSyntax == DB_SYNTAX_MSSQL)
   {
      CHK_EXEC(SQLQuery(L"UPDATE user_groups SET system_access=system_access+288230376151711744 WHERE id=1073741825 AND (CAST(system_access AS bigint) & CAST(288230376151711744 AS bigint))=0"));
   }
   else
   {
      CHK_EXEC(SQLQuery(L"UPDATE user_groups SET system_access=system_access+288230376151711744 WHERE id=1073741825 AND (system_access & 288230376151711744)=0"));
   }

   CHK_EXEC(CreateEventTemplate(EVENT_AI_OPERATOR_OBSERVATION, _T("SYS_AI_OPERATOR_OBSERVATION"),
      EVENT_SEVERITY_NORMAL, EF_LOG, _T("dac53e32-6d7b-48fc-967d-2b8472af88da"),
      _T("AI operator %2 observation (severity %4): %5"),
      _T("Generated when an AI operator instance records an observation.\r\n")
      _T("Parameters:\r\n")
      _T("   1) instanceId - AI operator instance ID\r\n")
      _T("   2) instanceName - AI operator instance name\r\n")
      _T("   3) observationId - Observation ID\r\n")
      _T("   4) severity - Observation severity\r\n")
      _T("   5) title - Observation title")));

   CHK_EXEC(CreateEventTemplate(EVENT_AI_OPERATOR_FAILURE, _T("SYS_AI_OPERATOR_FAILURE"),
      EVENT_SEVERITY_MAJOR, EF_LOG, _T("dab20532-cd84-466d-ad31-e3f04d4903f4"),
      _T("AI operator %2 disabled after %3 consecutive failures"),
      _T("Generated when an AI operator instance is disabled after a series of consecutive execution failures.\r\n")
      _T("Parameters:\r\n")
      _T("   1) instanceId - AI operator instance ID\r\n")
      _T("   2) instanceName - AI operator instance name\r\n")
      _T("   3) failureCount - Number of consecutive failures\r\n")
      _T("   4) lastError - Last error message")));

   CHK_EXEC(CreateConfigParam(L"AIOperator.Enabled", L"1",
      L"Enable/disable AI operator subsystem (global kill switch for all operator instances).",
      nullptr, 'B', true, false, false, false));
   CHK_EXEC(CreateConfigParam(L"AIOperator.MaxConsecutiveFailures", L"3",
      L"Number of consecutive execution failures after which an AI operator instance is automatically disabled.",
      nullptr, 'I', true, false, false, false));
   CHK_EXEC(CreateConfigParam(L"AIOperatorObservations.RetentionTime", L"90",
      L"Default retention time in days for AI operator observations. All records older than specified will be deleted by housekeeping process. Can be overridden per operator instance.",
      L"days", 'I', true, false, false, false));
   CHK_EXEC(CreateConfigParam(L"AIOperatorObservations.MaxRecordsPerInstance", L"1000",
      L"Default maximum number of retained observations per AI operator instance. Can be overridden per operator instance.",
      nullptr, 'I', true, false, false, false));
   CHK_EXEC(CreateConfigParam(L"AIOperatorExecutionLog.RetentionTime", L"90",
      L"Retention time in days for the records in AI operator execution log. All records older than specified will be deleted by housekeeping process.",
      L"days", 'I', true, false, false, false));
   CHK_EXEC(CreateConfigParam(L"ThreadPool.AIOperator.BaseSize", L"4",
      L"Base size for AI operator thread pool.", nullptr, 'I', true, true, false, false));
   CHK_EXEC(CreateConfigParam(L"ThreadPool.AIOperator.MaxSize", L"16",
      L"Maximum size for AI operator thread pool.", nullptr, 'I', true, true, false, false));

   CHK_EXEC(SetMinorSchemaVersion(7));
   return true;
}

/**
 * Upgrade from 70.5 to 70.6
 */
static bool H_UpgradeFromV5()
{
   if (GetSchemaLevelForMajorVersion(62) < 32)
   {
      CHK_EXEC(CreateConfigParam(L"AITaskExecutionLog.RetentionTime", L"90",
         L"Retention time in days for the records in AI task execution log. All records older than specified will be deleted by housekeeping process.",
         L"days", 'I', true, false, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(62, 32));
   }
   CHK_EXEC(SetMinorSchemaVersion(6));
   return true;
}

/**
 * Upgrade from 70.4 to 70.5
 */
static bool H_UpgradeFromV4()
{
   CHK_EXEC(CreateTable(
      L"CREATE TABLE ha_change_journal ("
      L"  seq $SQL:INT64 not null,"
      L"  entity_type char(1) not null,"
      L"  change_type char(1) not null,"
      L"  entity_id integer not null,"
      L"  entity_class integer not null,"
      L"  created_at $SQL:INT64 not null,"
      L"  PRIMARY KEY(seq))"));
   CHK_EXEC(CreateTable(
      L"CREATE TABLE ha_sync_state ("
      L"  node_guid varchar(36) not null,"
      L"  applied_seq $SQL:INT64 not null,"
      L"  updated_at $SQL:INT64 not null,"
      L"  PRIMARY KEY(node_guid))"));
   CHK_EXEC(SetMinorSchemaVersion(5));
   return true;
}

/**
 * Upgrade from 70.3 to 70.4
 */
static bool H_UpgradeFromV3()
{
   CHK_EXEC(SQLQuery(L"ALTER TABLE ha_lease ADD holder_address varchar(255)"));
   CHK_EXEC(SetMinorSchemaVersion(4));
   return true;
}

/**
 * Upgrade from 70.2 to 70.3
 */
static bool H_UpgradeFromV2()
{
   CHK_EXEC(CreateEventTemplate(EVENT_HA_NODE_ACTIVATED, _T("SYS_HA_NODE_ACTIVATED"),
      EVENT_SEVERITY_NORMAL, EF_LOG, _T("a4f162c9-4e09-4716-9ae8-4a4a2ab59d80"),
      _T("HA cluster node %1 activated (lease term %2)"),
      _T("Generated when a cluster node wins the HA lease and completes server activation.\r\n")
      _T("Parameters:\r\n")
      _T("   1) nodeName - Cluster node name\r\n")
      _T("   2) term - Lease term")));
   CHK_EXEC(SetMinorSchemaVersion(3));
   return true;
}

/**
 * Upgrade from 70.1 to 70.2
 */
static bool H_UpgradeFromV1()
{
   CHK_EXEC(SQLQuery(L"DELETE FROM config WHERE var_name='HelpDeskLink'"));
   CHK_EXEC(SQLQuery(L"DELETE FROM config WHERE var_name='Jira.Webhook.Path'"));
   CHK_EXEC(SQLQuery(L"DELETE FROM config WHERE var_name='Jira.Webhook.Port'"));
   CHK_EXEC(CreateConfigParam(L"Jira.Webhook.Enable", L"1", L"Enable/disable Jira webhook on the web API listener.", nullptr, 'B', true, true, false));
   CHK_EXEC(CreateConfigParam(L"Jira.Webhook.Secret", L"", L"Secret used for validation of Jira webhook calls (validation disabled if empty).", nullptr, 'S', true, false, false));
   CHK_EXEC(SetMinorSchemaVersion(2));
   return true;
}

/**
 * Upgrade from 70.0 to 70.1
 */
static bool H_UpgradeFromV0()
{
   CHK_EXEC(CreateTable(
      L"CREATE TABLE ha_lease ("
      L"  lease_id integer not null,"
      L"  format_version integer not null,"
      L"  term $SQL:INT64 not null,"
      L"  holder_guid varchar(36) null,"
      L"  holder_incarnation $SQL:INT64 not null,"
      L"  holder_name varchar(63) null,"
      L"  acquired_at $SQL:INT64 not null,"
      L"  expires_at $SQL:INT64 not null,"
      L"  PRIMARY KEY(lease_id))"));
   CHK_EXEC(SQLQuery(L"INSERT INTO ha_lease (lease_id,format_version,term,holder_guid,holder_incarnation,holder_name,acquired_at,expires_at) VALUES (1,1,0,NULL,0,NULL,0,0)"));
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
   { 9, 70, 10, H_UpgradeFromV9 },
   { 8, 70, 9, H_UpgradeFromV8 },
   { 7, 70, 8, H_UpgradeFromV7 },
   { 6, 70, 7, H_UpgradeFromV6 },
   { 5, 70, 6, H_UpgradeFromV5 },
   { 4, 70, 5, H_UpgradeFromV4 },
   { 3, 70, 4, H_UpgradeFromV3 },
   { 2, 70, 3, H_UpgradeFromV2 },
   { 1, 70, 2, H_UpgradeFromV1 },
   { 0, 70, 1, H_UpgradeFromV0 },
   { 0, 0,  0, nullptr }
};

/**
 * Upgrade database to new version
 */
bool MajorSchemaUpgrade_V70()
{
   int32_t major, minor;
   if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
      return false;

   while ((major == 70) && (minor < DB_SCHEMA_VERSION_V70_MINOR))
   {
      // Find upgrade procedure
      int i;
      for (i = 0; s_dbUpgradeMap[i].upgradeProc != nullptr; i++)
         if (s_dbUpgradeMap[i].version == minor)
            break;
      if (s_dbUpgradeMap[i].upgradeProc == nullptr)
      {
         WriteToTerminalEx(L"Unable to find upgrade procedure for version 70.%d\n", minor);
         return false;
      }
      WriteToTerminalEx(L"Upgrading from version 70.%d to %d.%d\n", minor, s_dbUpgradeMap[i].nextMajor, s_dbUpgradeMap[i].nextMinor);
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
