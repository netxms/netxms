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
** File: upgrade_v71.cpp
**
**/

#include "nxdbmgr.h"
#include <nxevent.h>

/**
 * Upgrade from 71.10 to 71.11
 */
static bool H_UpgradeFromV10()
{
   CHK_EXEC(CreateConfigParam(L"Client.RemoteControl.BlankScreenOnConnect", L"0",
         L"Request UltraVNC servers to disable local input and blank the monitor when a remote control session starts. Ignored for other VNC servers.",
         nullptr, 'B', true, false, false, false));
   CHK_EXEC(SetMinorSchemaVersion(11));
   return true;
}

/**
 * Upgrade from 71.9 to 71.10
 */
static bool H_UpgradeFromV9()
{
   static const wchar_t *batch =
      L"ALTER TABLE interfaces ADD phy_lane integer\n"
      L"ALTER TABLE interfaces ADD phy_location_valid char(1)\n"
      L"UPDATE interfaces SET phy_lane=-1,phy_location_valid='0'\n"
      L"UPDATE interfaces SET phy_location_valid='1' WHERE phy_chassis<>0 OR phy_module<>0 OR phy_pic<>0 OR phy_port<>0\n"
      L"<END>";
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, L"interfaces", L"phy_lane"));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, L"interfaces", L"phy_location_valid"));
   CHK_EXEC(SetMinorSchemaVersion(10));
   return true;
}

/**
 * Upgrade from 71.8 to 71.9
 */
static bool H_UpgradeFromV8()
{
   if (GetSchemaLevelForMajorVersion(70) < 60)
   {
      CHK_EXEC(CreateConfigParam(L"Agent.TLS.ReverseTunnels", L"2",
            L"Mode of server-initiated (reverse) TLS tunnels to agents on nodes with agent TLS mode Allowed: disabled (never used), allowed (used only when agent rejects non-TLS connection), or preferred (used whenever agent supports TLS tunnels).",
            nullptr, 'C', true, false, false, false));

      static const wchar_t *batch =
         L"INSERT INTO config_values (var_name,var_value,var_description) VALUES ('Agent.TLS.ReverseTunnels','0','Disabled')\n"
         L"INSERT INTO config_values (var_name,var_value,var_description) VALUES ('Agent.TLS.ReverseTunnels','1','Allowed')\n"
         L"INSERT INTO config_values (var_name,var_value,var_description) VALUES ('Agent.TLS.ReverseTunnels','2','Preferred')\n"
         L"<END>";
      CHK_EXEC(SQLBatch(batch));

      CHK_EXEC(SetSchemaLevelForMajorVersion(70, 60));
   }
   CHK_EXEC(SetMinorSchemaVersion(9));
   return true;
}

/**
 * Upgrade from 71.7 to 71.8
 */
static bool H_UpgradeFromV7()
{
   // Object status TESTING (8) is no longer produced by the server; convert any stored value to UNKNOWN
   CHK_EXEC(SQLQuery(L"UPDATE object_properties SET status=5 WHERE status=8"));
   CHK_EXEC(SetMinorSchemaVersion(8));
   return true;
}

/**
 * Upgrade from 71.6 to 71.7
 */
static bool H_UpgradeFromV6()
{
   CHK_EXEC(SQLQuery(L"ALTER TABLE event_policy ADD alarm_flap_count integer"));
   CHK_EXEC(SQLQuery(L"ALTER TABLE event_policy ADD alarm_flap_window integer"));
   CHK_EXEC(SQLQuery(L"UPDATE event_policy SET alarm_flap_count=0,alarm_flap_window=0"));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, L"event_policy", L"alarm_flap_count"));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, L"event_policy", L"alarm_flap_window"));

   CHK_EXEC(CreateEventTemplate(EVENT_ALARM_FLAPPING_STARTED, L"SYS_ALARM_FLAPPING_STARTED",
            EVENT_SEVERITY_WARNING, EF_LOG, L"67ff29ef-f703-4681-af2d-16922703b6cb",
            L"Alarm flapping started (ID: %1; Text: %2)",
            L"Generated when alarm flapping is detected: alarm is created again for the same key after being terminated by event processing policy the configured number of times within detection window. Alarm stays open until no transitions occur during detection window.\r\n"
            L"Parameters:\r\n"
            L"   1) alarmId - Alarm ID\r\n"
            L"   2) alarmMessage - Alarm message\r\n"
            L"   3) alarmKey - Alarm key\r\n"
            L"   4) originalEventCode - Original event code\r\n"
            L"   5) originalEventName - Original event name"));

   CHK_EXEC(CreateEventTemplate(EVENT_ALARM_FLAPPING_ENDED, L"SYS_ALARM_FLAPPING_ENDED",
            EVENT_SEVERITY_NORMAL, EF_LOG, L"557193b3-f310-4862-b24d-08d995852320",
            L"Alarm flapping ended (ID: %1; Text: %2)",
            L"Generated when alarm leaves flapping state: no transitions between active and terminated states occurred during detection window, or alarm was terminated.\r\n"
            L"Parameters:\r\n"
            L"   1) alarmId - Alarm ID\r\n"
            L"   2) alarmMessage - Alarm message\r\n"
            L"   3) alarmKey - Alarm key\r\n"
            L"   4) originalEventCode - Original event code\r\n"
            L"   5) originalEventName - Original event name"));

   CHK_EXEC(SetMinorSchemaVersion(7));
   return true;
}

/**
 * Upgrade from 71.5 to 71.6
 */
static bool H_UpgradeFromV5()
{
   // Mark existing pseudo-interfaces with IF_PSEUDO flag (0x00000800 = 2048); previously they were identified by index 1, type other(1) and name "unknown".
   // Bit is cleared on all interfaces first because interface flags saved by older versions may contain undefined bits.
   if ((g_dbSyntax == DB_SYNTAX_DB2) || (g_dbSyntax == DB_SYNTAX_ORACLE))
   {
      CHK_EXEC(SQLQuery(L"UPDATE object_properties SET flags=flags-2048 WHERE BITAND(flags, 2048)<>0 AND object_id IN (SELECT id FROM interfaces)"));
   }
   else
   {
      CHK_EXEC(SQLQuery(L"UPDATE object_properties SET flags=flags-2048 WHERE (flags & 2048)<>0 AND object_id IN (SELECT id FROM interfaces)"));
   }
   CHK_EXEC(SQLQuery(L"UPDATE object_properties SET flags=flags+2048 WHERE name='unknown' AND object_id IN (SELECT id FROM interfaces WHERE if_type=1 AND if_index=1)"));

   CHK_EXEC(SetMinorSchemaVersion(6));
   return true;
}

/**
 * Upgrade from 71.4 to 71.5
 */
static bool H_UpgradeFromV4()
{
   // Clear retired node flag NF_PING_PRIMARY_IP (0x04000000 = 67108864) - primary IP is now always checked with ICMP ping
   if ((g_dbSyntax == DB_SYNTAX_DB2) || (g_dbSyntax == DB_SYNTAX_ORACLE))
   {
      CHK_EXEC(SQLQuery(L"UPDATE object_properties SET flags=flags-67108864 WHERE BITAND(flags, 67108864)<>0 AND object_id IN (SELECT id FROM nodes)"));
   }
   else
   {
      CHK_EXEC(SQLQuery(L"UPDATE object_properties SET flags=flags-67108864 WHERE (flags & 67108864)<>0 AND object_id IN (SELECT id FROM nodes)"));
   }

   CHK_EXEC(SetMinorSchemaVersion(5));
   return true;
}

/**
 * Upgrade from 71.3 to 71.4
 */
static bool H_UpgradeFromV3()
{
   CHK_EXEC(SQLQuery(L"CREATE INDEX idx_audit_log_timestamp ON audit_log(timestamp)"));
   CHK_EXEC(SQLQuery(L"CREATE INDEX idx_audit_log_object ON audit_log(object_id,timestamp)"));
   CHK_EXEC(SetMinorSchemaVersion(4));
   return true;
}

/**
 * Upgrade from 71.2 to 71.3
 */
static bool H_UpgradeFromV2()
{
   // Add RF_ACCEPT_CORRELATED (0x4000 = 16384) to default rules "Terminate node down alarms when node is up" and "End downtime",
   // because SYS_NODE_UP can now be correlated (recovery after NetXMS server network connectivity loss)
   if ((g_dbSyntax == DB_SYNTAX_DB2) || (g_dbSyntax == DB_SYNTAX_ORACLE))
   {
      CHK_EXEC(SQLQuery(L"UPDATE event_policy SET flags=flags+16384 WHERE rule_guid IN ('4fa20604-b56f-4f12-bb5d-2c3243a126c9','256c2abd-3e14-4e02-9606-7c78c77e1a78') AND BITAND(flags, 16384)=0"));
   }
   else
   {
      CHK_EXEC(SQLQuery(L"UPDATE event_policy SET flags=flags+16384 WHERE rule_guid IN ('4fa20604-b56f-4f12-bb5d-2c3243a126c9','256c2abd-3e14-4e02-9606-7c78c77e1a78') AND (flags & 16384)=0"));
   }

   CHK_EXEC(SetMinorSchemaVersion(3));
   return true;
}

/**
 * Upgrade from 71.1 to 71.2
 */
static bool H_UpgradeFromV1()
{
   static const wchar_t *batch =
      L"UPDATE config SET var_name='AI.MaxInteractiveSteps',description='Maximum number of LLM steps (round trips with tool calls) for interactive AI assistant chats before the user is asked whether to continue.' WHERE var_name='AI.MaxInteractiveIterations'\n"
      L"UPDATE config SET var_name='AI.MaxBackgroundSteps',description='Maximum number of LLM steps (round trips with tool calls) for background AI assistant requests (event processing, scheduled tasks, incident analysis) before the request is stopped.' WHERE var_name='AI.MaxBackgroundIterations'\n"
      L"UPDATE scheduled_tasks SET taskId='Execute.AIPrompt' WHERE taskId='Execute.AIAgentTask'\n"
      L"ALTER TABLE ai_messages ADD origin_type char(1) null\n"
      L"ALTER TABLE ai_messages ADD origin_id integer null\n"
      L"<END>";
   CHK_EXEC(SQLBatch(batch));

   // Source task columns were never populated, so there is nothing to convert
   CHK_EXEC(DBDropColumn(g_dbHandle, L"ai_messages", L"source_task_id"));
   CHK_EXEC(DBDropColumn(g_dbHandle, L"ai_messages", L"source_task_type"));

   CHK_EXEC(CreateConfigParam(L"AI.Workers.MaxParallel", L"4",
      L"Maximum number of AI workers that can be started by a single run-workers call.",
      nullptr, 'I', true, false, false, false));
   CHK_EXEC(CreateConfigParam(L"AI.Workers.Timeout", L"600",
      L"Maximum time AI assistant waits for workers started by a single run-workers call. Workers that do not complete within this time are cancelled.",
      L"seconds", 'I', true, false, false, false));
   CHK_EXEC(CreateConfigParam(L"ThreadPool.AIWorkers.BaseSize", L"4", L"Base size for AI workers thread pool.", nullptr, 'I', true, true, false, false));
   CHK_EXEC(CreateConfigParam(L"ThreadPool.AIWorkers.MaxSize", L"16", L"Maximum size for AI workers thread pool.", nullptr, 'I', true, true, false, false));

   CHK_EXEC(SetMinorSchemaVersion(2));
   return true;
}

/**
 * Upgrade from 71.0 to 71.1
 */
static bool H_UpgradeFromV0()
{
   // Add SYSTEM_ACCESS_NXSL_FILE_IO (bit 61 = 0x2000000000000000 = 2305843009213693952) to Admins group
   if ((g_dbSyntax == DB_SYNTAX_DB2) || (g_dbSyntax == DB_SYNTAX_ORACLE))
   {
      CHK_EXEC(SQLQuery(L"UPDATE user_groups SET system_access=system_access+2305843009213693952 WHERE id=1073741825 AND BITAND(system_access, 2305843009213693952)=0"));
   }
   else if (g_dbSyntax == DB_SYNTAX_MSSQL)
   {
      CHK_EXEC(SQLQuery(L"UPDATE user_groups SET system_access=system_access+2305843009213693952 WHERE id=1073741825 AND (CAST(system_access AS bigint) & CAST(2305843009213693952 AS bigint))=0"));
   }
   else
   {
      CHK_EXEC(SQLQuery(L"UPDATE user_groups SET system_access=system_access+2305843009213693952 WHERE id=1073741825 AND (system_access & 2305843009213693952)=0"));
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
   { 10, 71, 11, H_UpgradeFromV10 },
   { 9,  71, 10, H_UpgradeFromV9  },
   { 8,  71, 9,  H_UpgradeFromV8  },
   { 7,  71, 8,  H_UpgradeFromV7  },
   { 6,  71, 7,  H_UpgradeFromV6  },
   { 5,  71, 6,  H_UpgradeFromV5  },
   { 4,  71, 5,  H_UpgradeFromV4  },
   { 3,  71, 4,  H_UpgradeFromV3  },
   { 2,  71, 3,  H_UpgradeFromV2  },
   { 1,  71, 2,  H_UpgradeFromV1  },
   { 0,  71, 1,  H_UpgradeFromV0  },
   { 0,  0,  0,  nullptr }
};

/**
 * Upgrade database to new version
 */
bool MajorSchemaUpgrade_V71()
{
   int32_t major, minor;
   if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
      return false;

   while ((major == 71) && (minor < DB_SCHEMA_VERSION_V71_MINOR))
   {
      // Find upgrade procedure
      int i;
      for (i = 0; s_dbUpgradeMap[i].upgradeProc != nullptr; i++)
         if (s_dbUpgradeMap[i].version == minor)
            break;
      if (s_dbUpgradeMap[i].upgradeProc == nullptr)
      {
         WriteToTerminalEx(L"Unable to find upgrade procedure for version 71.%d\n", minor);
         return false;
      }
      WriteToTerminalEx(L"Upgrading from version 71.%d to %d.%d\n", minor, s_dbUpgradeMap[i].nextMajor, s_dbUpgradeMap[i].nextMinor);
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
