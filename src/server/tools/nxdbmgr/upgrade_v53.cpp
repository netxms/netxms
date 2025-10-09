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
** File: upgrade_v53.cpp
**
**/

#include "nxdbmgr.h"

/**
 * Upgrade from 53.5 to 53.6
 */
static bool H_UpgradeFromV5()
{
   static const TCHAR *batch =
      _T("ALTER TABLE items ADD thresholds_disable_end_time integer\n")
      _T("UPDATE items SET thresholds_disable_end_time=0\n")
      _T("ALTER TABLE dc_tables ADD thresholds_disable_end_time integer\n")
      _T("UPDATE dc_tables SET thresholds_disable_end_time=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("items"), _T("thresholds_disable_end_time")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("dc_tables"), _T("thresholds_disable_end_time")));

   CHK_EXEC(CreateConfigParam(L"Objects.Nodes.ForceUpdateIp",
            L"0",
            L"Always force the primary IP address to change during a DNS lookup even if it is invalid, useful for DHCP clients with random address when they release their DHCP lease.",
            nullptr, 'B', true, false, false, false));

   CHK_EXEC(SetMinorSchemaVersion(6));
   return true;
}

/**
 * Upgrade from 53.4 to 53.5
 */
static bool H_UpgradeFromV4()
{
   CHK_EXEC(SQLQuery(_T("DELETE FROM config WHERE var_name='Objects.Security.ReadAccessViaMap'")));
   CHK_EXEC(SetMinorSchemaVersion(5));
   return true;
}

/**
 * Upgrade from 53.3 to 53.4
 */
static bool H_UpgradeFromV3()
{
   CHK_EXEC(SQLQuery(L"ALTER TABLE interfaces ADD state_before_maintenance varchar(2000)"));
   CHK_EXEC(SetMinorSchemaVersion(4));
   return true;
}

/**
 * Upgrade from 53.2 to 53.3
 */
static bool H_UpgradeFromV2()
{
   CHK_EXEC(DBDropColumn(g_dbHandle, _T("object_properties"), _T("is_system")));
   CHK_EXEC(SetMinorSchemaVersion(3));
   return true;
}

/**
 * Upgrade from 53.1 to 53.2
 */
static bool H_UpgradeFromV1()
{
   if (GetSchemaLevelForMajorVersion(52) < 20)
   {
      CHK_EXEC(CreateConfigParam(L"DataCollection.Scheduler.RequireConnectivity",
               L"0",
               L"Skip data collection scheduling if communication channel is unavailable.",
               nullptr, 'B', true, false, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(52, 20));
   }
   CHK_EXEC(SetMinorSchemaVersion(2));
   return true;
}

/**
 * Upgrade from 53.0 to 53.1
 */
static bool H_UpgradeFromV0()
{
   static const TCHAR *batch =
      _T("ALTER TABLE interfaces ADD peer_last_updated integer\n")
      _T("UPDATE interfaces SET peer_last_updated=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("interfaces"), _T("peer_last_updated")));

   CHK_EXEC(CreateConfigParam(L"Objects.Interfaces.PeerRetentionTime",
            L"30",
            L"Retention time for peer information which is no longer confirmed by polls.",
            L"days", 'I', true, false, false, false));

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
   { 5,  53, 6,  H_UpgradeFromV5  },
   { 4,  53, 5,  H_UpgradeFromV4  },
   { 3,  53, 4,  H_UpgradeFromV3  },
   { 2,  53, 3,  H_UpgradeFromV2  },
   { 1,  53, 2,  H_UpgradeFromV1  },
   { 0,  53, 1,  H_UpgradeFromV0  },
   { 0,  0,  0,  nullptr }
};

/**
 * Upgrade database to new version
 */
bool MajorSchemaUpgrade_V53()
{
   int32_t major, minor;
   if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
      return false;

   while ((major == 53) && (minor < DB_SCHEMA_VERSION_V53_MINOR))
   {
      // Find upgrade procedure
      int i;
      for (i = 0; s_dbUpgradeMap[i].upgradeProc != nullptr; i++)
         if (s_dbUpgradeMap[i].version == minor)
            break;
      if (s_dbUpgradeMap[i].upgradeProc == nullptr)
      {
         WriteToTerminalEx(L"Unable to find upgrade procedure for version 53.%d\n", minor);
         return false;
      }
      WriteToTerminalEx(L"Upgrading from version 53.%d to %d.%d\n", minor, s_dbUpgradeMap[i].nextMajor, s_dbUpgradeMap[i].nextMinor);
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
