/*
** nxdbmgr - NetXMS database manager
** Copyright (C) 2022-2024 Raden Solutions
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
