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
** File: upgrade_v45.cpp
**
**/

#include "nxdbmgr.h"
#include <nxevent.h>

/**
 * Upgrade from 45.5 to 46.0
 */
static bool H_UpgradeFromV5()
{
   CHK_EXEC(SetMajorSchemaVersion(46, 0));
   return true;
}

/**
 * Upgrade from 45.4 to 45.5
 */
static bool H_UpgradeFromV4()
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

   CHK_EXEC(SetMinorSchemaVersion(5));
   return true;
}

/**
 * Upgrade from 45.3 to 45.4
 */
static bool H_UpgradeFromV3()
{
   CHK_EXEC(CreateConfigParam(_T("Client.FirstPacketTimeout"),
         _T("2000"),
         _T("Timeout for receiving first packet from client after establishing TCP connection."),
         _T("milliseconds"), 'I', true, false, false, false));
   CHK_EXEC(SetMinorSchemaVersion(4));
   return true;
}

/**
 * Upgrade from 45.2 to 45.3
 */
static bool H_UpgradeFromV2()
{
   if (GetSchemaLevelForMajorVersion(44) < 29)
   {
      CHK_EXEC(DBDropPrimaryKey(g_dbHandle, _T("policy_action_list")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(44, 29));
   }
   CHK_EXEC(SetMinorSchemaVersion(3));
   return true;
}

/**
 * Upgrade from 45.1 to 45.2
 */
static bool H_UpgradeFromV1()
{
   CHK_EXEC(CreateConfigParam(_T("Objects.Interfaces.IgnoreIfNotPresent"),
         _T("0"),
         _T("If enabled, interfaces in \"NOT PRESENT\" state will be ignored when read from device."),
         nullptr, 'B', true, false, false, false));
   CHK_EXEC(SetMinorSchemaVersion(2));
   return true;
}

/**
 * Upgrade from 45.0 to 45.1
 */
static bool H_UpgradeFromV0()
{
   CHK_EXEC(SQLQuery(_T("UPDATE acl SET access_rights=access_rights+1048576 WHERE user_id=1073741825")));
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
   { 5,  46, 0,  H_UpgradeFromV5  },
   { 4,  45, 5,  H_UpgradeFromV4  },
   { 3,  45, 4,  H_UpgradeFromV3  },
   { 2,  45, 3,  H_UpgradeFromV2  },
   { 1,  45, 2,  H_UpgradeFromV1  },
   { 0,  45, 1,  H_UpgradeFromV0  },
   { 0,  0,  0,  nullptr }
};

/**
 * Upgrade database to new version
 */
bool MajorSchemaUpgrade_V45()
{
   int32_t major, minor;
   if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
      return false;

   while (major == 45)
   {
      // Find upgrade procedure
      int i;
      for (i = 0; s_dbUpgradeMap[i].upgradeProc != nullptr; i++)
         if (s_dbUpgradeMap[i].version == minor)
            break;
      if (s_dbUpgradeMap[i].upgradeProc == nullptr)
      {
         _tprintf(_T("Unable to find upgrade procedure for version 45.%d\n"), minor);
         return false;
      }
      _tprintf(_T("Upgrading from version 45.%d to %d.%d\n"), minor, s_dbUpgradeMap[i].nextMajor, s_dbUpgradeMap[i].nextMinor);
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
