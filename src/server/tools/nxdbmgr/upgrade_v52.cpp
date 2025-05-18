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
** File: upgrade_v52.cpp
**
**/

#include "nxdbmgr.h"
#include <nxevent.h>

/**
 * Upgrade from 52.15 to 52.16
 */
static bool H_UpgradeFromV15()
{
   static const wchar_t *batch =
      L"ALTER TABLE icmp_statistics ADD jitter integer\n"
      L"UPDATE icmp_statistics SET jitter=0\n"
      L"<END>";
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, L"icmp_statistics", L"jitter"));
   CHK_EXEC(SetMinorSchemaVersion(16));
   return true;
}

/**
 * Upgrade from 52.14 to 52.15
 */
static bool H_UpgradeFromV14()
{
   CHK_EXEC(SQLQuery(L"UPDATE config SET need_server_restart=0 WHERE var_name IN ('DataCollection.InstancePollingInterval','Objects.Conditions.PollingInterval','Topology.RoutingTableUpdateInterval','Topology.PollingInterval')"));
   CHK_EXEC(SetMinorSchemaVersion(15));
   return true;
}

/**
 * Upgrade from 52.13 to 52.14
 */
static bool H_UpgradeFromV13()
{
   CHK_EXEC(ConvertXmlToJson(L"dashboard_elements", L"dashboard_id", L"element_id", L"element_data"));
   CHK_EXEC(ConvertXmlToJson(L"dashboard_elements", L"dashboard_id", L"element_id", L"layout_data"));
   CHK_EXEC(SetMinorSchemaVersion(14));
   return true;
}

/**
 * Upgrade from 52.12 to 52.13
 */
static bool H_UpgradeFromV12()
{
   if (GetSchemaLevelForMajorVersion(51) < 26)
   {
      CHK_EXEC(DBDropPrimaryKey(g_dbHandle, L"scheduled_tasks"));
      CHK_EXEC(DBRenameColumn(g_dbHandle, L"scheduled_tasks", L"id", L"old_id"));

      static const wchar_t *batch =
         L"ALTER TABLE scheduled_tasks ADD id $SQL:INT64\n"
         L"UPDATE scheduled_tasks SET id=old_id\n"
         L"<END>";
      CHK_EXEC(SQLBatch(batch));

      CHK_EXEC(DBDropColumn(g_dbHandle, L"scheduled_tasks", L"old_id"));
      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, L"scheduled_tasks", L"id"));
      CHK_EXEC(DBAddPrimaryKey(g_dbHandle, L"scheduled_tasks", L"id"));

      CHK_EXEC(SetSchemaLevelForMajorVersion(51, 26));
   }
   CHK_EXEC(SetMinorSchemaVersion(13));
   return true;
}

/**
 * Upgrade from 52.11 to 52.12
 */
static bool H_UpgradeFromV11()
{
   //Object tool insert moved to XML import scripts
   CHK_EXEC(SetMinorSchemaVersion(12));
   return true;
}

/**
 * Upgrade from 52.10 to 52.11
 */
static bool H_UpgradeFromV10()
{
   CHK_EXEC(CreateConfigParam(L"RADIUS.NASIdentifier",
            L"",
            L"Value for NAS-Identifier attribute in RADIUS reuest (will not be sent if empty).",
            nullptr, 'S', true, false, false, false));
   CHK_EXEC(CreateConfigParam(L"RADIUS.ServiceType",
            L"8",
            L"Value for Service-Type attribute in RADIUS request. Value of 0 will exclude service type from request attributes.",
            nullptr, 'I', true, false, false, false));
   CHK_EXEC(SetMinorSchemaVersion(11));
   return true;
}

/**
 * Upgrade from 52.9 to 52.10
 */
static bool H_UpgradeFromV9()
{
   static const wchar_t *batch =
      L"ALTER TABLE nodes ADD expected_capabilities $SQL:INT64\n"
      L"UPDATE nodes SET expected_capabilities=0\n"
      L"<END>";
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, L"nodes", L"expected_capabilities"));
   CHK_EXEC(SetMinorSchemaVersion(10));
   return true;
}

/**
 * Upgrade from 52.8 to 52.9
 */
static bool H_UpgradeFromV8()
{
   if (GetSchemaLevelForMajorVersion(51) < 25)
   {
      CHK_EXEC(SQLQuery(L"ALTER TABLE licenses ADD guid varchar(36)"));

      DB_RESULT hResult = SQLSelect(L"SELECT id FROM licenses");
      if (hResult != nullptr)
      {
         int count = DBGetNumRows(hResult);
         for(int i = 0; i < count; i++)
         {
            int32_t id = htonl(DBGetFieldInt32(hResult, i, 0));
            uint8_t guid[UUID_LENGTH];
            memset(guid, 0, sizeof(uuid_t) - sizeof(int32_t));
            memcpy(&guid[UUID_LENGTH - sizeof(int32_t)], &id, sizeof(int32_t));

            wchar_t query[128];
            _sntprintf(query, 128, L"UPDATE licenses SET guid='%s' WHERE id=%d", uuid(guid).toString().cstr(), ntohl(id));
            CHK_EXEC(SQLQuery(query));
         }
         DBFreeResult(hResult);
      }
      else if (!g_ignoreErrors)
      {
         return false;
      }

      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, L"licenses", L"guid"));
      CHK_EXEC(SetSchemaLevelForMajorVersion(51, 25));
   }
   CHK_EXEC(SetMinorSchemaVersion(9));
   return true;
}

/**
 * Upgrade from 52.7 to 52.8
 */
static bool H_UpgradeFromV7()
{
   static const wchar_t *batch =
      L"ALTER TABLE notification_log ADD event_code integer\n"
      L"ALTER TABLE notification_log ADD event_id $SQL:INT64\n"
      L"ALTER TABLE notification_log ADD rule_id varchar(36)\n"
      L"<END>";
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(SetMinorSchemaVersion(8));
   return true;
}

/**
 * Upgrade from 52.6 to 52.7
 */
static bool H_UpgradeFromV6()
{
   CHK_EXEC(ConvertXmlToJson(L"network_map_links", L"map_id", L"link_id", L"element_data"));
   CHK_EXEC(ConvertXmlToJson(L"network_map_elements", L"map_id", L"element_id", L"element_data"));
   CHK_EXEC(SetMinorSchemaVersion(7));
   return true;
}

/**
 * Upgrade from 52.5 to 52.6
 */
static bool H_UpgradeFromV5()
{
   CHK_EXEC(CreateTable(
         L"CREATE TABLE package_deployment_jobs ("
         L"   id integer not null,"
         L"   pkg_id integer not null,"
         L"   node_id integer not null,"
         L"   user_id integer not null,"
         L"   creation_time integer not null,"
         L"   execution_time integer not null,"
         L"   completion_time integer not null,"
         L"   status integer not null,"
         L"   failure_reason varchar(255) null,"
         L"   PRIMARY KEY(id))"));

   if (g_dbSyntax == DB_SYNTAX_TSDB)
   {
      CHK_EXEC(CreateTable(
            L"CREATE TABLE package_deployment_log ("
            L"   job_id integer not null,"
            L"   execution_time timestamptz not null,"
            L"   completion_time timestamptz not null,"
            L"   node_id integer not null,"
            L"   user_id integer not null,"
            L"   status integer not null,"
            L"   failure_reason varchar(255) null,"
            L"   pkg_id integer not null,"
            L"   pkg_type varchar(15) null,"
            L"   pkg_name varchar(63) null,"
            L"   version varchar(31) null,"
            L"   platform varchar(63) null,"
            L"   pkg_file varchar(255) null,"
            L"   command varchar(255) null,"
            L"   description varchar(255) null,"
            L"   PRIMARY KEY(job_id,execution_time))"));
      CHK_EXEC(SQLQuery(L"SELECT create_hypertable('package_deployment_log', 'execution_time', chunk_time_interval => interval '86400 seconds')"));
   }
   else
   {
      CHK_EXEC(CreateTable(
            L"CREATE TABLE package_deployment_log ("
            L"   job_id integer not null,"
            L"   execution_time integer not null,"
            L"   completion_time integer not null,"
            L"   node_id integer not null,"
            L"   user_id integer not null,"
            L"   status integer not null,"
            L"   failure_reason varchar(255) null,"
            L"   pkg_id integer not null,"
            L"   pkg_type varchar(15) null,"
            L"   pkg_name varchar(63) null,"
            L"   version varchar(31) null,"
            L"   platform varchar(63) null,"
            L"   pkg_file varchar(255) null,"
            L"   command varchar(255) null,"
            L"   description varchar(255) null,"
            L"   PRIMARY KEY(job_id))"));
   }

   CHK_EXEC(SQLQuery(L"CREATE INDEX idx_pkglog_exec_time ON package_deployment_log(execution_time)"));
   CHK_EXEC(SQLQuery(L"CREATE INDEX idx_pkglog_node ON package_deployment_log(node_id)"));

   CHK_EXEC(SQLQuery(L"DELETE FROM config WHERE var_name='Agent.Upgrade.NumberOfThreads'"));
   CHK_EXEC(SQLQuery(L"UPDATE config SET description='Retention time in days for logged SNMP traps. All SNMP trap records older than specified will be deleted by housekeeping process.' WHERE var_name='SNMP.Traps.LogRetentionTime'"));

   CHK_EXEC(CreateConfigParam(L"PackageDeployment.JobRetentionTime",
            L"7",
            L"Retention time in days for completed package deployment jobs. All completed jobs older than specified will be deleted by housekeeping process.",
            L"days", 'I', true, false, false, false));
   CHK_EXEC(CreateConfigParam(L"PackageDeployment.LogRetentionTime",
            L"90",
            L"Retention time in days for package deployment log. All records older than specified will be deleted by housekeeping process.",
            L"days", 'I', true, false, false, false));
   CHK_EXEC(CreateConfigParam(L"PackageDeployment.MaxThreads",
            L"25",
            L"Maximum number of threads used for package deployment.",
            L"threads", 'I', true, true, false, false));

   CHK_EXEC(SetMinorSchemaVersion(6));
   return true;
}

/**
 * Upgrade from 52.4 to 52.5
 */
static bool H_UpgradeFromV4()
{
   CHK_EXEC(SQLQuery(L"ALTER TABLE network_maps ADD display_priority integer"));
   CHK_EXEC(SQLQuery(L"UPDATE network_maps SET display_priority=0"));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, L"network_maps", L"display_priority"));
   CHK_EXEC(SetMinorSchemaVersion(5));
   return true;
}

/**
 * Upgrade from 52.3 to 52.4
 */
static bool H_UpgradeFromV3()
{
   CHK_EXEC(DBResizeColumn(g_dbHandle, L"items", L"system_tag", 63, true));
   CHK_EXEC(DBResizeColumn(g_dbHandle, L"dc_tables", L"system_tag", 63, true));
   CHK_EXEC(SQLQuery(L"ALTER TABLE items ADD user_tag varchar(63)"));
   CHK_EXEC(SQLQuery(L"ALTER TABLE dc_tables ADD user_tag varchar(63)"));
   CHK_EXEC(SetMinorSchemaVersion(4));
   return true;
}

/**
 * Upgrade from 52.2 to 52.3
 */
static bool H_UpgradeFromV2()
{
   if (GetSchemaLevelForMajorVersion(51) < 24)
   {
      CHK_EXEC(CreateConfigParam(L"Objects.Nodes.ConfigurationPoll.AlwaysCheckSNMP",
               L"1",
               L"Always check possible SNMP credentials during configuration poll, even if node is marked as unreachable via SNMP.",
               nullptr, 'B', true, false, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(51, 24));
   }
   CHK_EXEC(SetMinorSchemaVersion(3));
   return true;
}

/**
 * Upgrade from 52.1 to 52.2
 */
static bool H_UpgradeFromV1()
{
   if (GetSchemaLevelForMajorVersion(51) < 23)
   {
      CHK_EXEC(CreateConfigParam(L"Client.MinVersion",
            L"",
            L"Minimal client version allowed for connection to this server.",
            nullptr, 'S', true, false, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(51, 23));
   }
   CHK_EXEC(SetMinorSchemaVersion(2));
   return true;
}

/**
 * Upgrade from 52.0 to 52.1
 */
static bool H_UpgradeFromV0()
{
   CHK_EXEC(SQLQuery(L"INSERT INTO script_library (guid,script_id,script_name,script_code) "
         L"VALUES ('b322c142-fdd5-433f-a820-05b2aa3daa39',27,'Hook::RegisterForConfigurationBackup','/* Available global variables:\r\n *  $node - node to be tested (object of ''Node'' class)\r\n *\r\n * Expected return value:\r\n *  true/false - boolean - whether this node should be registered for configuration backup\r\n */\r\nreturn $node.isSNMP;\r\n')"));

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
   { 15, 52, 16, H_UpgradeFromV15 },
   { 14, 52, 15, H_UpgradeFromV14 },
   { 13, 52, 14, H_UpgradeFromV13 },
   { 12, 52, 13, H_UpgradeFromV12 },
   { 11, 52, 12, H_UpgradeFromV11 },
   { 10, 52, 11, H_UpgradeFromV10 },
   { 9,  52, 10, H_UpgradeFromV9  },
   { 8,  52, 9,  H_UpgradeFromV8  },
   { 7,  52, 8,  H_UpgradeFromV7  },
   { 6,  52, 7,  H_UpgradeFromV6  },
   { 5,  52, 6,  H_UpgradeFromV5  },
   { 4,  52, 5,  H_UpgradeFromV4  },
   { 3,  52, 4,  H_UpgradeFromV3  },
   { 2,  52, 3,  H_UpgradeFromV2  },
   { 1,  52, 2,  H_UpgradeFromV1  },
   { 0,  52, 1,  H_UpgradeFromV0  },
   { 0,  0,  0,  nullptr }
};

/**
 * Upgrade database to new version
 */
bool MajorSchemaUpgrade_V52()
{
   int32_t major, minor;
   if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
      return false;

   while ((major == 52) && (minor < DB_SCHEMA_VERSION_V52_MINOR))
   {
      // Find upgrade procedure
      int i;
      for (i = 0; s_dbUpgradeMap[i].upgradeProc != nullptr; i++)
         if (s_dbUpgradeMap[i].version == minor)
            break;
      if (s_dbUpgradeMap[i].upgradeProc == nullptr)
      {
         WriteToTerminalEx(L"Unable to find upgrade procedure for version 51.%d\n", minor);
         return false;
      }
      WriteToTerminalEx(L"Upgrading from version 52.%d to %d.%d\n", minor, s_dbUpgradeMap[i].nextMajor, s_dbUpgradeMap[i].nextMinor);
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
