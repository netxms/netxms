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
** File: upgrade_v36.cpp
**
**/

#include "nxdbmgr.h"
#include <nxevent.h>

/**
 * Upgrade from 36.3 to 40.0
 */
static bool H_UpgradeFromV3()
{
   CHK_EXEC(SetMajorSchemaVersion(40, 0));
   return true;
}

/**
 * Upgrade from 36.2 to 36.3
 */
static bool H_UpgradeFromV2()
{
   CHK_EXEC(SQLQuery(_T("DROP TABLE certificates")));
   CHK_EXEC(SetMinorSchemaVersion(3));
   return true;
}

/**
 * Upgrade from 36.1 to 36.2
 */
static bool H_UpgradeFromV1()
{
   CHK_EXEC(CreateConfigParam(_T("AgentTunnels.TLS.MinVersion"), _T("2"), _T("Minimal version of TLS protocol used on agent tunnel connection."), nullptr, 'C', true, false, false, false));

   static const TCHAR *batch =
         _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('AgentTunnels.TLS.MinVersion','0','1.0')\n")
         _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('AgentTunnels.TLS.MinVersion','1','1.1')\n")
         _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('AgentTunnels.TLS.MinVersion','2','1.2')\n")
         _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('AgentTunnels.TLS.MinVersion','3','1.3')\n")
         _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   CHK_EXEC(SetMinorSchemaVersion(2));
   return true;
}

/**
 * Upgrade from 36.0 to 36.1
 */
static bool H_UpgradeFromV0()
{
   CHK_EXEC(SQLQuery(_T("ALTER TABLE object_properties ADD name_on_map varchar(63)")));
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
   { 3,  40, 0,  H_UpgradeFromV3  },
   { 2,  36, 3,  H_UpgradeFromV2  },
   { 1,  36, 2,  H_UpgradeFromV1  },
   { 0,  36, 1,  H_UpgradeFromV0  },
   { 0,  0,  0,  nullptr          }
};

/**
 * Upgrade database to new version
 */
bool MajorSchemaUpgrade_V36()
{
   int32_t major, minor;
   if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
      return false;

   while(major == 36)
   {
      // Find upgrade procedure
      int i;
      for(i = 0; s_dbUpgradeMap[i].upgradeProc != nullptr; i++)
         if (s_dbUpgradeMap[i].version == minor)
            break;
      if (s_dbUpgradeMap[i].upgradeProc == nullptr)
      {
         _tprintf(_T("Unable to find upgrade procedure for version 36.%d\n"), minor);
         return false;
      }
      _tprintf(_T("Upgrading from version 36.%d to %d.%d\n"), minor, s_dbUpgradeMap[i].nextMajor, s_dbUpgradeMap[i].nextMinor);
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
