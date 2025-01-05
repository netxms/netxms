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
** File: upgrade_v42.cpp
**
**/

#include "nxdbmgr.h"


/**
 * Upgrade from 42.20 to 43.0
 */
static bool H_UpgradeFromV20()
{
   CHK_EXEC(SetMajorSchemaVersion(43, 0));
   return true;
}

/**
 * Upgrade from 42.19 to 42.20
 */
static bool H_UpgradeFromV19()
{
   const TCHAR batch[] =
      _T("ALTER TABLE object_tools ADD remote_port integer\n")
      _T("UPDATE object_tools SET remote_port=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("object_tools"), _T("remote_port")));
   CHK_EXEC(SetMinorSchemaVersion(20));
   return true;
}

/**
 * Upgrade from 42.18 to 42.19
 */
static bool H_UpgradeFromV18()
{
   const TCHAR batch[] =
      _T("ALTER TABLE nodes ADD mqtt_proxy integer\n")
      _T("UPDATE nodes SET mqtt_proxy=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("nodes"), _T("mqtt_proxy")));
   CHK_EXEC(SetMinorSchemaVersion(19));
   return true;
}

/**
 * Upgrade from 42.17 to 42.18
 */
static bool H_UpgradeFromV17()
{
   const TCHAR batch[] =
      _T("ALTER TABLE rack_passive_elements ADD height integer\n")
      _T("ALTER TABLE rack_passive_elements ADD image_front varchar(36)\n")
      _T("ALTER TABLE rack_passive_elements ADD image_rear varchar(36)\n")
      _T("UPDATE rack_passive_elements SET height=1\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("rack_passive_elements"), _T("height")));
   CHK_EXEC(SetMinorSchemaVersion(18));
   return true;
}

/**
 * Upgrade from 42.16 to 42.17
 */
static bool H_UpgradeFromV16()
{
   CHK_EXEC(CreateTable(_T("CREATE TABLE ospf_neighbors (")
      _T("node_id integer not null,")
      _T("router_id varchar(15) not null,")
      _T("area_id varchar(15) null,")
      _T("ip_address varchar(48) null,")
      _T("remote_node_id integer not null,")
      _T("if_index integer not null,")
      _T("is_virtual char(1) not null,")
      _T("neighbor_state integer not null,")
      _T("PRIMARY KEY(node_id,router_id,if_index))")));
   CHK_EXEC(SetMinorSchemaVersion(17));
   return true;
}

/**
 * Upgrade from 42.15 to 42.16
 */
static bool H_UpgradeFromV15()
{
   const TCHAR batch[] =
      _T("ALTER TABLE interfaces ADD ospf_if_type integer\n")
      _T("ALTER TABLE interfaces ADD ospf_if_state integer\n")
      _T("UPDATE interfaces SET ospf_if_type=0,ospf_if_state=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("interfaces"), _T("ospf_if_type")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("interfaces"), _T("ospf_if_state")));
   CHK_EXEC(CreateTable(_T("CREATE TABLE ospf_areas (node_id integer not null, area_id varchar(15) not null, PRIMARY KEY(node_id,area_id))")));
   CHK_EXEC(SetMinorSchemaVersion(16));
   return true;
}

/**
 * Upgrade from 42.14 to 42.15
 */
static bool H_UpgradeFromV14()
{
   CHK_EXEC(SQLQuery(_T("ALTER TABLE nodes ADD ospf_router_id varchar(15)")));
   CHK_EXEC(SQLQuery(_T("ALTER TABLE interfaces ADD ospf_area varchar(15)")));
   CHK_EXEC(SetMinorSchemaVersion(15));
   return true;
}

/**
 * Upgrade from 42.13 to 42.14
 */
static bool H_UpgradeFromV13()
{
   CHK_EXEC(DBRenameColumn(g_dbHandle, _T("items"), _T("custom_units_name"), _T("units_name")));
   CHK_EXEC(DBRenameColumn(g_dbHandle, _T("items"), _T("unit_multiplier"), _T("multiplier")));
   CHK_EXEC(DBDropColumn(g_dbHandle, _T("items"), _T("base_units")));
   CHK_EXEC(SQLQuery(_T("UPDATE items SET multiplier=0,units_name=''")));

   DB_RESULT hResult = SQLSelect(_T("SELECT dashboard_id,element_id,element_data FROM dashboard_elements"));
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      DB_STATEMENT hStmt = DBPrepare(g_dbHandle, _T("UPDATE dashboard_elements SET element_data=? WHERE dashboard_id=? AND element_id=?"), count > 1);

      if (hStmt != nullptr)
      {
         for (int i = 0; i < count; i++)
         {
            StringBuffer sb = DBGetFieldAsSharedString(hResult, i, 2);
            sb.replace(_T("<displayFormat>%s</displayFormat>"), _T("<displayFormat></displayFormat>"));
            DBBind(hStmt, 1, DB_SQLTYPE_TEXT, sb, DB_BIND_STATIC);
            DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, DBGetFieldULong(hResult, i, 0));
            DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, DBGetFieldULong(hResult, i, 1));
            if (!SQLExecute(hStmt) && !g_ignoreErrors)
            {
               DBFreeResult(hResult);
               DBFreeStatement(hStmt);
               return false;
            }

         }
         DBFreeStatement(hStmt);
      }
      else if (!g_ignoreErrors)
      {
         DBFreeResult(hResult);
         return false;
      }
      DBFreeResult(hResult);
   }
   else if (!g_ignoreErrors)
   {
      return false;
   }

   hResult = SQLSelect(_T("SELECT graph_id,config FROM graphs"));
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      DB_STATEMENT hStmt = DBPrepare(g_dbHandle, _T("UPDATE graphs SET config=? WHERE graph_id=?"), count > 1);

      if (hStmt != nullptr)
      {
         for (int i = 0; i < count && hStmt != nullptr; i++)
         {
            StringBuffer sb = DBGetFieldAsSharedString(hResult, i, 1);
            sb.replace(_T("<displayFormat>%s</displayFormat>"), _T("<displayFormat></displayFormat>"));
            DBBind(hStmt, 1, DB_SQLTYPE_TEXT, sb, DB_BIND_STATIC);
            DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, DBGetFieldULong(hResult, i, 0));
            if (!SQLExecute(hStmt) && !g_ignoreErrors)
            {
               DBFreeResult(hResult);
               DBFreeStatement(hStmt);
               return false;
            }

         }
         DBFreeStatement(hStmt);
      }
      else if (!g_ignoreErrors)
      {
         DBFreeResult(hResult);
         return false;
      }
      DBFreeResult(hResult);
   }
   else if (!g_ignoreErrors)
   {
      return false;
   }

   CHK_EXEC(SetMinorSchemaVersion(14));
   return true;
}

/**
 * Upgrade from 42.12 to 42.13
 */
static bool H_UpgradeFromV12()
{
   CHK_EXEC(SQLQuery(_T("ALTER TABLE dashboards ADD display_priority integer")));
   CHK_EXEC(SQLQuery(_T("UPDATE dashboards SET display_priority=0")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("dashboards"), _T("display_priority")));
   CHK_EXEC(SetMinorSchemaVersion(13));
   return true;
}

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
   { 20, 43, 0, H_UpgradeFromV20 },
   { 19, 42, 20, H_UpgradeFromV19 },
   { 18, 42, 19, H_UpgradeFromV18 },
   { 17, 42, 18, H_UpgradeFromV17 },
   { 16, 42, 17, H_UpgradeFromV16 },
   { 15, 42, 16, H_UpgradeFromV15 },
   { 14, 42, 15, H_UpgradeFromV14 },
   { 13, 42, 14, H_UpgradeFromV13 },
   { 12, 42, 13, H_UpgradeFromV12 },
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

   while (major == 42)
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
