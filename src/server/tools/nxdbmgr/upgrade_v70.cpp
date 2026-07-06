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
