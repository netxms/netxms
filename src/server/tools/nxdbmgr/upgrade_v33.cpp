/*
** nxdbmgr - NetXMS database manager
** Copyright (C) 2004-2020 Raden Solutions
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
** File: upgrade_v33.cpp
**
**/

#include "nxdbmgr.h"
#include <nxevent.h>

/**
 * Upgrade from 32.4 to 32.5
 */
static bool H_UpgradeFromV4()
{
   CHK_EXEC(SQLQuery(_T("UPDATE nodes SET secret='' WHERE auth_method=0")));
   CHK_EXEC(DBDropColumn(g_dbHandle, _T("nodes"), _T("auth_method")));
   CHK_EXEC(DBResizeColumn(g_dbHandle, _T("nodes"), _T("secret"), 88, true));
   CHK_EXEC(DBResizeColumn(g_dbHandle, _T("shared_secrets"), _T("secret"), 88, true));
   CHK_EXEC(SetMinorSchemaVersion(5));
   return true;
}

/**
 * Upgrade from 32.3 to 32.4
 */
static bool H_UpgradeFromV3()
{
   CHK_EXEC(CreateTable(
         _T("CREATE TABLE shared_secrets (")
         _T("id integer not null,")
         _T("secret varchar(64) null,")
         _T("zone integer null,")
         _T("PRIMARY KEY(id))")));

   TCHAR secret[MAX_SECRET_LENGTH];
   secret[0] = 0;
   DB_RESULT hResult = SQLSelect(_T("SELECT var_value FROM config WHERE var_name='AgentDefaultSharedSecret'"));
   if (hResult != NULL)
   {
      if(DBGetNumRows(hResult) > 0)
      {
         DBGetField(hResult, 0, 0, secret, MAX_SECRET_LENGTH);
      }
      DBFreeResult(hResult);
   }

   if (secret[0] != 0)
   {
      TCHAR query[1024];
      _sntprintf(query, 1024, _T("INSERT INTO shared_secrets (id,secret,zone) VALUES(%d,'%s',%d)"), 1, secret, -1);
      CHK_EXEC(SQLQuery(query));
   }

   CHK_EXEC(SQLQuery(_T("DELETE FROM config WHERE var_name='AgentDefaultSharedSecret'")));

   CHK_EXEC(SetMinorSchemaVersion(4));
   return true;
}

/**
 * Upgrade from 32.2 to 32.3
 */
static bool H_UpgradeFromV2()
{
   static const TCHAR *batch =
      _T("UPDATE config SET var_name='EventStorm.Duration',description='Time period for events per second to be above threshold that defines event storm condition.',units='seconds' WHERE var_name='EventStormDuration'\n")
      _T("UPDATE config SET var_name='EventStorm.EnableDetection',description='Enable/disable event storm detection.' WHERE var_name='EnableEventStormDetection'\n")
      _T("UPDATE config SET var_name='EventStorm.EventsPerSecond',description='Threshold for number of events per second that defines event storm condition.',units='events/second',default_value='1000' WHERE var_name='EventStormEventsPerSecond'\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   CHK_EXEC(SetMinorSchemaVersion(3));
   return true;
}

/**
 * Upgrade from 32.1 to 32.2
 */
static bool H_UpgradeFromV1()
{
   static const TCHAR *batch =
      _T("ALTER TABLE ap_common ADD flags integer\n")
      _T("UPDATE ap_common SET flags=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("ap_common"), _T("flags")));

   CHK_EXEC(SetMinorSchemaVersion(2));
   return true;
}

/**
 * Upgrade from 33.0 to 33.1
 */
static bool H_UpgradeFromV0()
{
   CHK_EXEC(CreateConfigParam(_T("NetworkDeviceDrivers.BlackList"), _T(""), _T("Comma separated list of blacklisted network device drivers."), nullptr, 'S', true, true, false, false));
   CHK_EXEC(CreateConfigParam(_T("SNMP.Discovery.SeparateProbeRequests"), _T("0"), _T("Use separate SNMP request for each test OID."), nullptr, 'B', true, false, false, false));
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
   bool (* upgradeProc)();
} s_dbUpgradeMap[] =
{
   { 4,  33, 5,  H_UpgradeFromV4  },
   { 3,  33, 4,  H_UpgradeFromV3  },
   { 2,  33, 3,  H_UpgradeFromV2  },
   { 1,  33, 2,  H_UpgradeFromV1  },
   { 0,  33, 1,  H_UpgradeFromV0  },
   { 0,  0,  0,  nullptr          }
};

/**
 * Upgrade database to new version
 */
bool MajorSchemaUpgrade_V33()
{
   INT32 major, minor;
   if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
      return false;

   while((major == 33) && (minor < DB_SCHEMA_VERSION_V33_MINOR))
   {
      // Find upgrade procedure
      int i;
      for(i = 0; s_dbUpgradeMap[i].upgradeProc != nullptr; i++)
         if (s_dbUpgradeMap[i].version == minor)
            break;
      if (s_dbUpgradeMap[i].upgradeProc == nullptr)
      {
         _tprintf(_T("Unable to find upgrade procedure for version 33.%d\n"), minor);
         return false;
      }
      _tprintf(_T("Upgrading from version 33.%d to %d.%d\n"), minor, s_dbUpgradeMap[i].nextMajor, s_dbUpgradeMap[i].nextMinor);
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
