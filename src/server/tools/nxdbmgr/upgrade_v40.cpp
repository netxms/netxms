/*
** nxdbmgr - NetXMS database manager
** Copyright (C) 2020 Raden Solutions
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
** File: upgrade_v40.cpp
**
**/

#include "nxdbmgr.h"

/**
 * Upgrade from 40.1 to 40.2
 */
static bool H_UpgradeFromV1()
{
   if (GetSchemaLevelForMajorVersion(35) < 7)
   {
      CHK_EXEC(SQLQuery(
            _T("INSERT INTO script_library (guid,script_id,script_name,script_code) ")
            _T("VALUES ('9c2dba59-493b-4645-9159-2ad7a28ea611',23,'Hook::OpenBoundTunnel','")
            _T("/* Available global variables:\r\n")
            _T(" *  $tunnel - incoming tunnel information (object of ''Tunnel'' class)\r\n")
            _T(" *\r\n")
            _T(" * Expected return value:\r\n")
            _T(" *  none - returned value is ignored\r\n */\r\n')")));
      CHK_EXEC(SQLQuery(
            _T("INSERT INTO script_library (guid,script_id,script_name,script_code) ")
            _T("VALUES ('64c90b92-27e9-4a96-98ea-d0e152d71262',24,'Hook::OpenUnboundTunnel','")
            _T("/* Available global variables:\r\n")
            _T(" *  $node - node this tunnel was bound to (object of ''Node'' class)\r\n")
            _T(" *  $tunnel - incoming tunnel information (object of ''Tunnel'' class)\r\n")
            _T(" *\r\n")
            _T(" * Expected return value:\r\n")
            _T(" *  none - returned value is ignored\r\n */\r\n')")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(35, 7));
   }

   CHK_EXEC(SetMinorSchemaVersion(2));
   return true;
}


/**
 * Upgrade from 40.0 to 40.1
 */
static bool H_UpgradeFromV0()
{
   if (GetSchemaLevelForMajorVersion(35) < 6)
   {
      CHK_EXEC(CreateConfigParam(_T("RoamingServer"), _T("0"), _T("Enable/disable roaming mode for server (when server can be disconnected from one network and connected to another or IP address of the server can change)."), nullptr, 'B', true, false, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(35, 6));
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
} s_dbUpgradeMap[] =
{
   { 1,  40, 2,  H_UpgradeFromV1  },
   { 0,  40, 1,  H_UpgradeFromV0  },
   { 0,  0,  0,  nullptr          }
};

/**
 * Upgrade database to new version
 */
bool MajorSchemaUpgrade_V40()
{
   INT32 major, minor;
   if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
      return false;

   while((major == 40) && (minor < DB_SCHEMA_VERSION_V40_MINOR))
   {
      // Find upgrade procedure
      int i;
      for(i = 0; s_dbUpgradeMap[i].upgradeProc != nullptr; i++)
         if (s_dbUpgradeMap[i].version == minor)
            break;
      if (s_dbUpgradeMap[i].upgradeProc == nullptr)
      {
         _tprintf(_T("Unable to find upgrade procedure for version 40.%d\n"), minor);
         return false;
      }
      _tprintf(_T("Upgrading from version 40.%d to %d.%d\n"), minor, s_dbUpgradeMap[i].nextMajor, s_dbUpgradeMap[i].nextMinor);
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
