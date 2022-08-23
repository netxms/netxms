/*
** nxdbmgr - NetXMS database manager
** Copyright (C) 2022 Raden Solutions
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
** File: upgrade_v42.cpp
**
**/

#include "nxdbmgr.h"

/**
 * Upgrade from 42.11 to 42.12
 */
static bool H_UpgradeFromV11()
{
   CHK_EXEC(DBDropColumn(g_dbHandle, _T("dashboards"), _T("options")));
   CHK_EXEC(SetMinorSchemaVersion(12));
   return true;
}

/**
 * Upgrade from 42.10 to 42.11
 */
static bool H_UpgradeFromV10()
{
   CHK_EXEC(CreateConfigParam(_T("DBWriter.UpdateParallelismDegree"), _T("1"), _T("Degree of parallelism for UPDATE statements executed by raw DCI data writer."), nullptr, 'I', true, true, false, false));
   CHK_EXEC(SetMinorSchemaVersion(11));
   return true;
}

/**
 * Upgrade from 42.9 to 42.10
 */
static bool H_UpgradeFromV9()
{
   CHK_EXEC(CreateConfigParam(_T("DBWriter.InsertParallelismDegree"), _T("1"), _T("Degree of parallelism for INSERT statements executed by DCI data writer (only valid for TimescaleDB)."), nullptr, 'I', true, true, false, false));
   CHK_EXEC(SetMinorSchemaVersion(10));
   return true;
}

/**
 * Upgrade from 42.8 to 42.9
 */
static bool H_UpgradeFromV8()
{
   CHK_EXEC(CreateConfigParam(_T("DBWriter.BackgroundWorkers"), _T("1"), _T("Number of background workers for DCI data writer."), nullptr, 'I', true, true, false, false));
   CHK_EXEC(SetMinorSchemaVersion(9));
   return true;
}

/**
 * Upgrade from 42.7 to 42.8
 */
static bool H_UpgradeFromV7()
{
   CHK_EXEC(SQLQuery(_T("UPDATE script_library SET script_name='Hook::OpenUnboundTunnel' WHERE guid='9c2dba59-493b-4645-9159-2ad7a28ea611'")));
   CHK_EXEC(SQLQuery(_T("UPDATE script_library SET script_name='Hook::OpenBoundTunnel' WHERE guid='64c90b92-27e9-4a96-98ea-d0e152d71262'")));
   CHK_EXEC(SetMinorSchemaVersion(8));
   return true;
}

/**
 * Upgrade from 42.6 to 42.7
 */
static bool H_UpgradeFromV6()
{
   CHK_EXEC(SQLQuery(_T("UPDATE config SET need_server_restart=0 WHERE var_name='Objects.Subnets.DeleteEmpty'")));
   CHK_EXEC(SetMinorSchemaVersion(7));
   return true;
}

/**
 * Upgrade from 42.5 to 42.6
 */
static bool H_UpgradeFromV5()
{
   if (GetSchemaLevelForMajorVersion(41) < 20)
   {
      CHK_EXEC(CreateConfigParam(_T("DBWriter.RawDataFlushInterval"), _T("30"), _T("Interval between writes of accumulated raw DCI data to database."), _T("seconds"), 'I', true, true, false, false));

      // The following update needed for situation when parameter was already created by user
      CHK_EXEC(SQLQuery(_T("UPDATE config SET default_value='30',units='seconds',data_type='I',need_server_restart=1,description='Interval between writes of accumulated raw DCI data to database.' WHERE var_name='DBWriter.RawDataFlushInterval'")));

      CHK_EXEC(SetSchemaLevelForMajorVersion(41, 20));
   }
   CHK_EXEC(SetMinorSchemaVersion(6));
   return true;
}

/**
 * Upgrade from 42.4 to 42.5
 */
static bool H_UpgradeFromV4()
{
   if (GetSchemaLevelForMajorVersion(41) < 19)
   {
      CHK_EXEC(SQLQuery(_T("UPDATE config SET default_value='90' WHERE var_name='BusinessServices.History.RetentionTime'")));
      CHK_EXEC(SQLQuery(_T("UPDATE config SET var_value='90' WHERE var_name='BusinessServices.History.RetentionTime' AND var_value='1'")));

      CHK_EXEC(SetSchemaLevelForMajorVersion(41, 19));
   }
   CHK_EXEC(SetMinorSchemaVersion(5));
   return true;
}

/**
 * Upgrade from 42.3 to 42.4
 */
static bool H_UpgradeFromV3()
{
   DB_RESULT hResult = SQLSelect(_T("SELECT network_maps.id, object_properties.flags FROM network_maps INNER JOIN object_properties ON network_maps.id=object_properties.object_id"));
   if (hResult != nullptr)
   {
      TCHAR query[1024];
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
        _sntprintf(query, 1024, _T("UPDATE object_properties SET flags=%u WHERE object_id=%u"),
              DBGetFieldULong(hResult, i, 1) | MF_TRANSLUCENT_LABEL_BKGND, DBGetFieldULong(hResult, i, 0));
        CHK_EXEC(SQLQuery(query));
      }
      DBFreeResult(hResult);
   }
   else if (!g_ignoreErrors)
   {
      return false;
   }
   CHK_EXEC(SetMinorSchemaVersion(4));
   return true;
}

/**
 * Upgrade from 42.2 to 42.3
 */
static bool H_UpgradeFromV2()
{
   CHK_EXEC(CreateConfigParam(_T("Objects.AutobindOnConfigurationPoll"),
         _T("1"),
         _T("Enable/disable automatic object binding on configuration polls."),
         nullptr, 'B', true, false, false, false));
   CHK_EXEC(SetMinorSchemaVersion(3));
   return true;
}

/**
 * Upgrade from 42.1 to 42.2
 */
static bool H_UpgradeFromV1()
{
   if (GetSchemaLevelForMajorVersion(41) < 18)
   {
      CHK_EXEC(SQLQuery(_T("UPDATE config SET default_value='8' WHERE var_name='ThreadPool.Discovery.BaseSize'")));
      CHK_EXEC(SQLQuery(_T("UPDATE config SET default_value='64' WHERE var_name='ThreadPool.Discovery.MaxSize'")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(41, 18));
   }
   CHK_EXEC(SetMinorSchemaVersion(2));
   return true;
}

/**
 * Upgrade from 42.0 to 42.1
 */
static bool H_UpgradeFromV0()
{
   CHK_EXEC(CreateConfigParam(_T("SNMP.EngineId"),
         _T("80:00:DF:4B:05:20:10:08:04:02:01:00"),
         _T("Server's SNMP engine ID."),
         nullptr, 'S', true, true, false, false));
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
   { 11, 42, 12, H_UpgradeFromV11 },
   { 10, 42, 11, H_UpgradeFromV10 },
   { 9,  42, 10, H_UpgradeFromV9  },
   { 8,  42, 9,  H_UpgradeFromV8  },
   { 7,  42, 8,  H_UpgradeFromV7  },
   { 6,  42, 7,  H_UpgradeFromV6  },
   { 5,  42, 6,  H_UpgradeFromV5  },
   { 4,  42, 5,  H_UpgradeFromV4  },
   { 3,  42, 4,  H_UpgradeFromV3  },
   { 2,  42, 3,  H_UpgradeFromV2  },
   { 1,  42, 2,  H_UpgradeFromV1  },
   { 0,  42, 1,  H_UpgradeFromV0  },
   { 0,  0,  0,  nullptr }
};

/**
 * Upgrade database to new version
 */
bool MajorSchemaUpgrade_V42()
{
   int32_t major, minor;
   if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
      return false;

   while ((major == 42) && (minor < DB_SCHEMA_VERSION_V42_MINOR))
   {
      // Find upgrade procedure
      int i;
      for (i = 0; s_dbUpgradeMap[i].upgradeProc != nullptr; i++)
         if (s_dbUpgradeMap[i].version == minor)
            break;
      if (s_dbUpgradeMap[i].upgradeProc == nullptr)
      {
         _tprintf(_T("Unable to find upgrade procedure for version 42.%d\n"), minor);
         return false;
      }
      _tprintf(_T("Upgrading from version 42.%d to %d.%d\n"), minor, s_dbUpgradeMap[i].nextMajor, s_dbUpgradeMap[i].nextMinor);
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
