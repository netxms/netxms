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
** File: upgrade_v61.cpp
**
**/

#include "nxdbmgr.h"
#include <nxevent.h>

/**
 * Upgrade from 61.12 to 61.13
 */
static bool H_UpgradeFromV12()
{
   CHK_EXEC(CreateTable(
      L"CREATE TABLE trusted_devices ("
      L"  id integer not null,"
      L"  user_id integer not null,"
      L"  token_hash char(64) not null,"
      L"  description varchar(255) null,"
      L"  creation_time integer not null,"
      L"  PRIMARY KEY(id))"));
   CHK_EXEC(SQLQuery(L"CREATE INDEX idx_trusted_devices_user_id ON trusted_devices(user_id)"));
   CHK_EXEC(SetMinorSchemaVersion(13));
   return true;
}

/**
 * Upgrade from 61.11 to 61.12
 */
static bool H_UpgradeFromV11()
{
   if (GetSchemaLevelForMajorVersion(60) < 36)
   {
      CHK_EXEC(CreateConfigParam(L"Syslog.ResolverCacheTTL", L"300",
         L"TTL in seconds for syslog hostname resolver cache. Caches DNS resolution results to avoid repeated lookups for the same hostname. Set to 0 to disable caching.",
         L"seconds", 'I', true, false, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(60, 36));
   }
   CHK_EXEC(SetMinorSchemaVersion(12));
   return true;
}

/**
 * Upgrade from 61.10 to 61.11
 */
static bool H_UpgradeFromV10()
{
   CHK_EXEC(SQLQuery(L"ALTER TABLE object_tools ADD remote_host varchar(255)"));
   CHK_EXEC(SetMinorSchemaVersion(11));
   return true;
}

/**
 * Upgrade from 61.9 to 61.10
 */
static bool H_UpgradeFromV9()
{
   CHK_EXEC(CreateConfigParam(L"NotificationChannels.RateLimit.ChannelBurst", L"0",
      L"Default channel-level burst size for rate limiting (0 = disabled).",
      L"messages", 'I', true, false, false, false));
   CHK_EXEC(CreateConfigParam(L"NotificationChannels.RateLimit.ChannelRate", L"0",
      L"Default channel-level sustained rate for rate limiting (0 = disabled).",
      L"messages", 'I', true, false, false, false));
   CHK_EXEC(CreateConfigParam(L"NotificationChannels.RateLimit.ChannelRateUnit", L"minute",
      L"Time unit for channel rate limit: second, minute, or hour.",
      nullptr, 'S', true, false, false, false));
   CHK_EXEC(CreateConfigParam(L"NotificationChannels.RateLimit.DigestInterval", L"300",
      L"Interval in seconds between digest notification deliveries.",
      L"seconds", 'I', true, false, false, false));
   CHK_EXEC(CreateConfigParam(L"NotificationChannels.RateLimit.DigestThreshold", L"50",
      L"Queue depth that triggers digest mode. When throttled messages accumulate beyond this count, they are batched into a digest instead of being delivered individually (0 = disabled).",
      L"messages", 'I', true, false, false, false));
   CHK_EXEC(CreateConfigParam(L"NotificationChannels.RateLimit.RecipientBurst", L"0",
      L"Default per-recipient burst size for rate limiting (0 = disabled).",
      L"messages", 'I', true, false, false, false));
   CHK_EXEC(CreateConfigParam(L"NotificationChannels.RateLimit.RecipientRate", L"0",
      L"Default per-recipient sustained rate for rate limiting (0 = disabled).",
      L"messages", 'I', true, false, false, false));
   CHK_EXEC(CreateConfigParam(L"NotificationChannels.RateLimit.RecipientRateUnit", L"minute",
      L"Time unit for recipient rate limit: second, minute, or hour.",
      nullptr, 'S', true, false, false, false));
   CHK_EXEC(SetMinorSchemaVersion(10));
   return true;
}

/**
 * Upgrade from 61.8 to 61.9
 */
static bool H_UpgradeFromV8()
{
   CHK_EXEC(CreateEventTemplate(EVENT_DEVICE_REPLACED, _T("SYS_DEVICE_REPLACED"),
      EVENT_SEVERITY_MAJOR, EF_LOG, _T("d5a3f1e6-7b8c-4d9a-be2f-3c4d5e6f7a8b"),
      _T("Device replaced (serial number: %<oldSerialNumber> -> %<newSerialNumber>)"),
      _T("Generated when device replacement is detected during configuration poll.\r\n")
      _T("Parameters:\r\n")
      _T("   1) oldSerialNumber - Serial number of the old device\r\n")
      _T("   2) newSerialNumber - Serial number of the new device\r\n")
      _T("   3) oldHardwareId - Hardware ID of the old device\r\n")
      _T("   4) newHardwareId - Hardware ID of the new device\r\n")
      _T("   5) oldSnmpObjectId - SNMP object ID of the old device\r\n")
      _T("   6) newSnmpObjectId - SNMP object ID of the new device")));
   CHK_EXEC(SetMinorSchemaVersion(9));
   return true;
}

/**
 * Upgrade from 61.7 to 61.8
 */
static bool H_UpgradeFromV7()
{
   if (GetSchemaLevelForMajorVersion(60) < 35)
   {
      CHK_EXEC(CreateConfigParam(L"Agent.UploadBandwidthLimit", L"0",
         L"Bandwidth limit for file uploads from server to agent (0 = unlimited).",
         L"KB/s", 'I', true, false, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(60, 35));
   }
   CHK_EXEC(SetMinorSchemaVersion(8));
   return true;
}

/**
 * Upgrade from 61.6 to 61.7
 */
static bool H_UpgradeFromV6()
{
   CHK_EXEC(SQLQuery(L"ALTER TABLE mapping_tables ADD guid varchar(36)"));

   DB_RESULT hResult = SQLSelect(L"SELECT id FROM mapping_tables");
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         uuid guid = uuid::generate();
         wchar_t query[256];
         _sntprintf(query, 256, L"UPDATE mapping_tables SET guid='%s' WHERE id=%d", guid.toString().cstr(), DBGetFieldInt32(hResult, i, 0));
         CHK_EXEC(SQLQuery(query));
      }
      DBFreeResult(hResult);
   }
   else if (!g_ignoreErrors)
   {
      return false;
   }

   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, L"mapping_tables", L"guid"));
   CHK_EXEC(SetMinorSchemaVersion(7));
   return true;
}

/**
 * Upgrade from 61.5 to 61.6
 */
static bool H_UpgradeFromV5()
{
   CHK_EXEC(DBResizeColumn(g_dbHandle, L"syslog", L"msg_tag", 48, true));
   static const wchar_t *batch =
      L"ALTER TABLE syslog ADD msg_procid varchar(128)\n"
      L"ALTER TABLE syslog ADD msg_msgid varchar(32)\n"
      L"ALTER TABLE syslog ADD msg_sd $SQL:TEXT\n"
      L"<END>";
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(SetMinorSchemaVersion(6));
   return true;
}

/**
 * Upgrade from 61.4 to 61.5
 */
static bool H_UpgradeFromV4()
{
   CHK_EXEC(CreateTable(
      L"CREATE TABLE cloud_domains ("
      L"  id integer not null,"
      L"  connector_name varchar(63) not null,"
      L"  credentials varchar(4000) null,"
      L"  discovery_filter varchar(4000) null,"
      L"  removal_policy smallint not null,"
      L"  grace_period integer not null,"
      L"  last_discovery_status smallint not null,"
      L"  last_discovery_time integer not null,"
      L"  last_discovery_msg varchar(4000) null,"
      L"  PRIMARY KEY(id))"));

   CHK_EXEC(CreateTable(
      L"CREATE TABLE resources ("
      L"  id integer not null,"
      L"  parent_id integer not null,"
      L"  cloud_resource_id varchar(1024) not null,"
      L"  resource_type varchar(256) not null,"
      L"  region varchar(128) null,"
      L"  state smallint not null,"
      L"  provider_state varchar(256) null,"
      L"  linked_node_id integer not null,"
      L"  account_id varchar(256) null,"
      L"  last_discovery_time integer not null,"
      L"  connector_data $SQL:TEXT null,"
      L"  PRIMARY KEY(id))"));

   CHK_EXEC(CreateTable(
      L"CREATE TABLE resource_tags ("
      L"  resource_id integer not null,"
      L"  tag_key varchar(256) not null,"
      L"  tag_value varchar(1024) null,"
      L"  PRIMARY KEY(resource_id,tag_key))"));

   CHK_EXEC(SetMinorSchemaVersion(5));
   return true;
}

/**
 * Upgrade from 61.3 to 61.4
 */
static bool H_UpgradeFromV3()
{
   static const wchar_t *batch =
      L"ALTER TABLE users ADD tfa_grace_logins integer\n"
      L"UPDATE users SET tfa_grace_logins=5\n"
      L"<END>";
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, L"users", L"tfa_grace_logins"));
   CHK_EXEC(CreateConfigParam(L"Server.Security.2FA.EnforceForAll", L"0", L"Enforce two-factor authentication for all users", nullptr, 'B', true, false, false, false));
   CHK_EXEC(CreateConfigParam(L"Server.Security.2FA.GraceLoginCount", L"5", L"Number of grace logins allowed for users who have not configured two-factor authentication when enforcement is active", nullptr, 'I', true, false, false, false));
   CHK_EXEC(SetMinorSchemaVersion(4));
   return true;
}

/**
 * Upgrade from 61.2 to 61.3
 */
static bool H_UpgradeFromV2()
{
   static const wchar_t *batch =
      L"ALTER TABLE thresholds ADD regenerate_on_value_change char(1)\n"
      L"UPDATE thresholds SET regenerate_on_value_change='0'\n"
      L"<END>";
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, L"thresholds", L"regenerate_on_value_change"));
   CHK_EXEC(SetMinorSchemaVersion(3));
   return true;
}

/**
 * Upgrade from 61.1 to 61.2
 */
static bool H_UpgradeFromV1()
{
   CHK_EXEC(CreateEventTemplate(EVENT_RUNNING_CONFIG_CHANGED, _T("SYS_RUNNING_CONFIG_CHANGED"),
      EVENT_SEVERITY_WARNING, EF_LOG, _T("b3a1e2c4-5d6f-4a8b-9c0e-1f2d3a4b5c6d"),
      _T("Device running configuration changed"),
      _T("Generated when device running configuration change is detected during backup.\r\n")
      _T("Parameters:\r\n")
      _T("   1) previousHash - SHA-256 hash of the previous configuration\r\n")
      _T("   2) newHash - SHA-256 hash of the new configuration")));

   CHK_EXEC(CreateEventTemplate(EVENT_STARTUP_CONFIG_CHANGED, _T("SYS_STARTUP_CONFIG_CHANGED"),
      EVENT_SEVERITY_WARNING, EF_LOG, _T("c4b2f3d5-6e7a-4b9c-ad1f-2e3d4a5b6c7e"),
      _T("Device startup configuration changed"),
      _T("Generated when device startup configuration change is detected during backup.\r\n")
      _T("Parameters:\r\n")
      _T("   1) previousHash - SHA-256 hash of the previous configuration\r\n")
      _T("   2) newHash - SHA-256 hash of the new configuration")));

   CHK_EXEC(SetMinorSchemaVersion(2));
   return true;
}

/**
 * Upgrade from 61.0 to 61.1
 */
static bool H_UpgradeFromV0()
{
   static const wchar_t *batch =
      L"ALTER TABLE thresholds ADD deactivation_sample_count integer\n"
      L"ALTER TABLE thresholds ADD clear_match_count integer\n"
      L"UPDATE thresholds SET deactivation_sample_count=1,clear_match_count=0\n"
      L"ALTER TABLE dct_thresholds ADD deactivation_sample_count integer\n"
      L"UPDATE dct_thresholds SET deactivation_sample_count=1\n"
      L"ALTER TABLE dct_threshold_instances ADD clear_match_count integer\n"
      L"UPDATE dct_threshold_instances SET clear_match_count=0\n"
      L"<END>";
   CHK_EXEC(SQLBatch(batch));

   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, L"thresholds", L"deactivation_sample_count"));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, L"thresholds", L"clear_match_count"));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, L"dct_thresholds", L"deactivation_sample_count"));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, L"dct_threshold_instances", L"clear_match_count"));

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
   { 12, 61, 13,  H_UpgradeFromV12 },
   { 11, 61, 12,  H_UpgradeFromV11 },
   { 10, 61, 11,  H_UpgradeFromV10 },
   { 9,  61, 10,  H_UpgradeFromV9  },
   { 8,  61,  9,  H_UpgradeFromV8  },
   { 7,  61,  8,  H_UpgradeFromV7  },
   { 6,  61,  7,  H_UpgradeFromV6  },
   { 5,  61,  6,  H_UpgradeFromV5  },
   { 4,  61,  5,  H_UpgradeFromV4  },
   { 3,  61,  4,  H_UpgradeFromV3  },
   { 2,  61,  3,  H_UpgradeFromV2  },
   { 1,  61,  2,  H_UpgradeFromV1  },
   { 0,  61,  1,  H_UpgradeFromV0  },
   { 0,  0,  0,  nullptr }
};

/**
 * Upgrade database to new version
 */
bool MajorSchemaUpgrade_V61()
{
   int32_t major, minor;
   if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
      return false;

   while ((major == 61) && (minor < DB_SCHEMA_VERSION_V61_MINOR))
   {
      // Find upgrade procedure
      int i;
      for (i = 0; s_dbUpgradeMap[i].upgradeProc != nullptr; i++)
         if (s_dbUpgradeMap[i].version == minor)
            break;
      if (s_dbUpgradeMap[i].upgradeProc == nullptr)
      {
         WriteToTerminalEx(L"Unable to find upgrade procedure for version 61.%d\n", minor);
         return false;
      }
      WriteToTerminalEx(L"Upgrading from version 61.%d to %d.%d\n", minor, s_dbUpgradeMap[i].nextMajor, s_dbUpgradeMap[i].nextMinor);
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
