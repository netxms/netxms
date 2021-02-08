/*
** nxdbmgr - NetXMS database manager
** Copyright (C) 2020-2021 Raden Solutions
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
** File: upgrade_v38.cpp
**
**/

#include "nxdbmgr.h"
#include <nxevent.h>

/**
 * Upgrade from 38.6 to 40.0
 */
static bool H_UpgradeFromV6()
{
   CHK_EXEC(SetMajorSchemaVersion(40, 0));
   return true;
}

/**
 * Upgrade from 38.5 to 38.6
 */
static bool H_UpgradeFromV5()
{
   static const TCHAR *batch =
         _T("ALTER TABLE policy_action_list ADD snooze_time integer\n")
         _T("UPDATE policy_action_list SET snooze_time=0\n")
         _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("policy_action_list"), _T("snooze_time")));
   CHK_EXEC(SetMinorSchemaVersion(6));
   return true;
}


/**
 * Upgrade from 38.4 to 38.5
 */
static bool H_UpgradeFromV4()
{
   CHK_EXEC(CreateEventTemplate(EVENT_TUNNEL_HOST_DATA_MISMATCH, _T("SYS_TUNNEL_HOST_DATA_MISMATCH"),
            SEVERITY_WARNING, EF_LOG, _T("874aa4f3-51b9-49ad-a8df-fb4bb89d0f81"),
            _T("Host data mismatch on tunnel reconnect"),
            _T("Generated when new tunnel is replacing existing one and host data mismatch is detected.\r\n")
            _T("Parameters:\r\n")
            _T("   1) Tunnel ID (tunnelId)\r\n")
            _T("   2) Old remote system IP address (oldIPAddress)\r\n")
            _T("   3) New remote system IP address (newIPAddress)\r\n")
            _T("   4) Old remote system name (oldSystemName)\r\n")
            _T("   5) New remote system name (newSystemName)\r\n")
            _T("   6) Old remote system FQDN (oldHostName)\r\n")
            _T("   7) New remote system FQDN (newHostName)\r\n")
            _T("   8) Old hardware ID (oldHardwareId)\r\n")
            _T("   9) New hardware ID (newHardwareId)")
            ));
   CHK_EXEC(SetMinorSchemaVersion(5));
   return true;
}

/**
 * Upgrade from 38.3 to 38.4
 */
static bool H_UpgradeFromV3()
{
   CHK_EXEC(CreateTable(
         _T("CREATE TABLE ssh_keys (")
         _T("  id integer not null,")
         _T("  name varchar(255) not null,")
         _T("  public_key varchar(700) null,")
         _T("  private_key varchar(1710) null,")
         _T("PRIMARY KEY(id))")));

   static const TCHAR *batch =
         _T("ALTER TABLE nodes ADD ssh_key_id integer\n")
         _T("UPDATE nodes SET ssh_key_id=0\n")
         _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("nodes"), _T("ssh_key_id")));

   // Update access rights for predefined "Admins" group
   DB_RESULT hResult = SQLSelect(_T("SELECT system_access FROM user_groups WHERE id=1073741825"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         uint64_t access = DBGetFieldUInt64(hResult, 0, 0);
         access |= SYSTEM_ACCESS_SSH_KEY_CONFIGURATION;
         TCHAR query[256];
         _sntprintf(query, 256, _T("UPDATE user_groups SET system_access=") UINT64_FMT _T(" WHERE id=1073741825"), access);
         CHK_EXEC(SQLQuery(query));
      }
      DBFreeResult(hResult);
   }
   else
   {
      if (!g_ignoreErrors)
         return false;
   }

   CHK_EXEC(SetMinorSchemaVersion(4));
   return true;
}

/**
 * Upgrade from 38.2 to 38.3
 */
static bool H_UpgradeFromV2()
{
   if (DBIsTableExist(g_dbHandle, _T("report_results")))
   {
      CHK_EXEC(SQLQuery(_T("DROP TABLE report_results")));
   }
   CHK_EXEC(SetMinorSchemaVersion(3));
   return true;
}

/**
 * Upgrade from 38.1 to 38.2
 */
static bool H_UpgradeFromV1()
{
   if (DBIsTableExist(g_dbHandle, _T("report_notifications")))
   {
      CHK_EXEC(SQLQuery(_T("DROP TABLE report_notifications")));
   }
   CHK_EXEC(SetMinorSchemaVersion(2));
   return true;
}

/**
 * Upgrade from 38.0 to 38.3
 * Upgrades straight to version 3, versions 1 and 2 skipped because in version 3 all reporting tables are already dropped.
 */
static bool H_UpgradeFromV0()
{
   static const TCHAR *deprecatedTables[] = {
      _T("qrtz_fired_triggers"),
      _T("qrtz_paused_trigger_grps"),
      _T("qrtz_scheduler_state"),
      _T("qrtz_locks"),
      _T("qrtz_simple_triggers"),
      _T("qrtz_cron_triggers"),
      _T("qrtz_simprop_triggers"),
      _T("qrtz_blob_triggers"),
      _T("qrtz_triggers"),
      _T("qrtz_job_details"),
      _T("qrtz_calendars"),
      _T("report_notification"),
      _T("reporting_results"),
      nullptr
   };
   for(int i = 0; deprecatedTables[i] != nullptr; i++)
   {
      if (DBIsTableExist(g_dbHandle, deprecatedTables[i]))
      {
         TCHAR query[256];
         _sntprintf(query, 256, _T("DROP TABLE %s"), deprecatedTables[i]);
         CHK_EXEC(SQLQuery(query));
      }
   }
   CHK_EXEC(SetMinorSchemaVersion(3));
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
   { 6,  40, 0,  H_UpgradeFromV6  },
   { 5,  38, 6,  H_UpgradeFromV5  },
   { 4,  38, 5,  H_UpgradeFromV4  },
   { 3,  38, 4,  H_UpgradeFromV3  },
   { 2,  38, 3,  H_UpgradeFromV2  },
   { 1,  38, 2,  H_UpgradeFromV1  },
   { 0,  38, 3,  H_UpgradeFromV0  },
   { 0,  0,  0,  nullptr          }
};

/**
 * Upgrade database to new version
 */
bool MajorSchemaUpgrade_V38()
{
   int32_t major, minor;
   if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
      return false;

   while(major == 38)
   {
      // Find upgrade procedure
      int i;
      for(i = 0; s_dbUpgradeMap[i].upgradeProc != nullptr; i++)
         if (s_dbUpgradeMap[i].version == minor)
            break;
      if (s_dbUpgradeMap[i].upgradeProc == nullptr)
      {
         _tprintf(_T("Unable to find upgrade procedure for version 38.%d\n"), minor);
         return false;
      }
      _tprintf(_T("Upgrading from version 38.%d to %d.%d\n"), minor, s_dbUpgradeMap[i].nextMajor, s_dbUpgradeMap[i].nextMinor);
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
