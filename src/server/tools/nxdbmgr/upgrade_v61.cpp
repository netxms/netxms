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
   { 3,  61,  4,  H_UpgradeFromV3 },
   { 2,  61,  3,  H_UpgradeFromV2 },
   { 1,  61,  2,  H_UpgradeFromV1 },
   { 0,  61,  1,  H_UpgradeFromV0 },
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
