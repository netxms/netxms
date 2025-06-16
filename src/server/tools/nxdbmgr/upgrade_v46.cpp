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
** File: upgrade_v46.cpp
**
**/

#include "nxdbmgr.h"
#include <nxevent.h>

/**
 * Upgrade from 46.3 to 50.0
 */
static bool H_UpgradeFromV3()
{
   CHK_EXEC(SetMajorSchemaVersion(50, 0));
   return true;
}

/**
 * Upgrade from 46.2 to 46.3
 */
static bool H_UpgradeFromV2()
{
   if (GetSchemaLevelForMajorVersion(45) < 5)
   {
      CHK_EXEC(CreateEventTemplate(EVENT_RESPONSIBLE_USER_ADDED, _T("SYS_RESPONSIBLE_USER_ADDED"),
            EVENT_SEVERITY_NORMAL, 0, _T("27e38dfb-1027-454a-8cd9-fbc49dcf0a9c"),
            _T("New responsible user %<userName> (ID: %<userId>, Tag: \"%<tag>\") added to object %<objectName> (ID: %<objectId>) by %<operator>"),
            _T("Generated when new responsible user added to the object.\r\n")
            _T("Parameters:\r\n")
            _T("   1) userId - User ID\r\n")
            _T("   2) userName - User name\r\n")
            _T("   3) tag - User tag\r\n")
            _T("   4) objectId - Object ID\r\n")
            _T("   5) objectName - Object name\r\n")
            _T("   6) operator - Operator (user who made change to the object)")
         ));

      CHK_EXEC(CreateEventTemplate(EVENT_RESPONSIBLE_USER_REMOVED, _T("SYS_RESPONSIBLE_USER_REMOVED"),
            EVENT_SEVERITY_NORMAL, 0, _T("c17409f9-1213-4c48-8249-62caa79a01c5"),
            _T("Responsible user %<userName> (ID: %<userId>, Tag: \"%<tag>\") removed from object %<objectName> (ID: %<objectId>) by %<operator>"),
            _T("Generated when new responsible user added to the object.\r\n")
            _T("Parameters:\r\n")
            _T("   1) userId - User ID\r\n")
            _T("   2) userName - User name\r\n")
            _T("   3) tag - User tag\r\n")
            _T("   4) objectId - Object ID\r\n")
            _T("   5) objectName - Object name\r\n")
            _T("   6) operator - Operator (user who made change to the object)")
         ));

      CHK_EXEC(CreateEventTemplate(EVENT_RESPONSIBLE_USER_MODIFIED, _T("SYS_RESPONSIBLE_USER_MODIFIED"),
            EVENT_SEVERITY_NORMAL, 0, _T("84baa8d0-f70c-4d92-b243-7fe5e7df0fed"),
            _T("Responsible user %<userName> (ID: %<userId>) tag on object %<objectName> (ID: %<objectId>) changed from \"%<prevTag>\" to \"%<tag>\" by %<operator>"),
            _T("Generated when the responsible user's record for the object is modified.\r\n")
            _T("Parameters:\r\n")
            _T("   1) userId - User ID\r\n")
            _T("   2) userName - User name\r\n")
            _T("   3) tag - User tag\r\n")
            _T("   4) prevTag - Old user tag\r\n")
            _T("   5) objectId - Object ID\r\n")
            _T("   6) objectName - Object name\r\n")
            _T("   7) operator - Operator (user who made change to the object)")
         ));

      CHK_EXEC(SetSchemaLevelForMajorVersion(45, 5));
   }
   CHK_EXEC(SetMinorSchemaVersion(3));
   return true;
}

/**
 * Upgrade from 46.1 to 46.2
 */
static bool H_UpgradeFromV1()
{
   CHK_EXEC(CreateConfigParam(_T("AgentTunnels.BindByIPAddress"),
                              _T("0"),
                              _T("nable/disable agent tunnel binding by IP address. If enabled and agent certificate is not provided, tunnel will be bound to node with IP address matching tunnel source IP address."),
                              nullptr, 'B', true, false, false, false));
   CHK_EXEC(SetMinorSchemaVersion(2));
   return true;
}

/**
 * Upgrade from 46.0 to 46.1
 */
static bool H_UpgradeFromV0()
{
   static const TCHAR *batch =
      _T("ALTER TABLE users ADD ui_access_rules varchar(2000)\n")
      _T("ALTER TABLE user_groups ADD ui_access_rules varchar(2000)\n")
      _T("UPDATE user_groups SET ui_access_rules='*' WHERE id=1073741824\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
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
   { 3,  50, 0,  H_UpgradeFromV3  },
   { 2,  46, 3,  H_UpgradeFromV2  },
   { 1,  46, 2,  H_UpgradeFromV1  },
   { 0,  46, 1,  H_UpgradeFromV0  },
   { 0,  0,  0,  nullptr }
};

/**
 * Upgrade database to new version
 */
bool MajorSchemaUpgrade_V46()
{
   int32_t major, minor;
   if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
      return false;

   while (major == 46)
   {
      // Find upgrade procedure
      int i;
      for (i = 0; s_dbUpgradeMap[i].upgradeProc != nullptr; i++)
         if (s_dbUpgradeMap[i].version == minor)
            break;
      if (s_dbUpgradeMap[i].upgradeProc == nullptr)
      {
         _tprintf(_T("Unable to find upgrade procedure for version 46.%d\n"), minor);
         return false;
      }
      _tprintf(_T("Upgrading from version 46.%d to %d.%d\n"), minor, s_dbUpgradeMap[i].nextMajor, s_dbUpgradeMap[i].nextMinor);
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
