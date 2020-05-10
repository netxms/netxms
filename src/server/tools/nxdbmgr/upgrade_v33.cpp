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
 * Upgrade from 33.12 to 34.0
 */
static bool H_UpgradeFromV12()
{
   CHK_EXEC(SetMajorSchemaVersion(34, 0));
   return true;
}

/**
 * Upgrade from 33.11 to 33.12
 */
static bool H_UpgradeFromV11()
{
   static const TCHAR *batch =
      _T("ALTER TABLE object_properties ADD maint_initiator integer\n")
      _T("UPDATE object_properties SET maint_initiator=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("object_properties"), _T("maint_initiator")));

   DB_STATEMENT hStmt = DBPrepare(g_dbHandle, _T("UPDATE event_cfg SET description=? WHERE guid=?"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_TEXT, _T("Generated when node, cluster, or mobile device entered maintenance mode.\r\nParameters:\r\n   1) Comments\r\n   2) Initiating user ID\r\n   3) Initiating user name"), DB_BIND_STATIC);
      DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, _T("5f6c8b1c-f162-413e-8028-80e7ad2c362d"), DB_BIND_STATIC);
      CHK_EXEC(SQLExecute(hStmt));

      DBBind(hStmt, 1, DB_SQLTYPE_TEXT, _T("Generated when node, cluster, or mobile device left maintenance mode.\r\nParameters:\r\n   1) Initiating user ID\r\n   2) Initiating user name"), DB_BIND_STATIC);
      DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, _T("cab06848-a622-430d-8b4c-addeea732657"), DB_BIND_STATIC);
      CHK_EXEC(SQLExecute(hStmt));

      DBFreeStatement(hStmt);
   }
   else if (!g_ignoreErrors)
   {
      return false;
   }

   CHK_EXEC(SetMinorSchemaVersion(12));
   return true;
}

/**
 * Upgrade from 33.10 to 33.11
 */
static bool H_UpgradeFromV10()
{
   CHK_EXEC(CreateEventTemplate(EVENT_HOUSEKEEPER_STARTED, _T("SYS_HOUSEKEEPER_STARTED"),
            SEVERITY_NORMAL, EF_LOG, _T("cbbf0da4-1784-49a5-af9b-e8c59451d3ce"),
            _T("Housekeeper task started"),
            _T("Generated when internal housekeeper task starts.\r\n")
            _T("Parameters:\r\n")
            _T("   No message-specific parameters")
            ));
   CHK_EXEC(CreateEventTemplate(EVENT_HOUSEKEEPER_COMPLETED, _T("SYS_HOUSEKEEPER_COMPLETED"),
            SEVERITY_NORMAL, EF_LOG, _T("69743397-6f2b-4777-8bde-879a6e3e47f9"),
            _T("Housekeeper task completed in %1 seconds"),
            _T("Generated when internal housekeeper task completes.\r\n")
            _T("Parameters:\r\n")
            _T("   1) Housekeeper execution time in seconds")
            ));
   CHK_EXEC(SetMinorSchemaVersion(11));
   return true;
}

/**
 * Upgrade from 33.9 to 33.10
 */
static bool H_UpgradeFromV9()
{
   static const TCHAR *batch =
      _T("ALTER TABLE websvc_definitions ADD flags integer\n")
      _T("UPDATE websvc_definitions SET flags=2\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("websvc_definitions"), _T("flags")));

   CHK_EXEC(SetMinorSchemaVersion(10));
   return true;
}

/**
 * Upgrade from 33.8 to 33.9
 */
static bool H_UpgradeFromV8()
{
   CHK_EXEC(SQLQuery(_T("ALTER TABLE nodes ADD hardware_id varchar(40)")));
   CHK_EXEC(SetMinorSchemaVersion(9));
   return true;
}

/**
 * Upgrade from 33.7 to 33.8
 */
static bool H_UpgradeFromV7()
{
   CHK_EXEC(CreateTable(
         _T("CREATE TABLE snmp_ports (")
         _T("id integer not null,")
         _T("port integer not null,")
         _T("zone integer not null,")
         _T("PRIMARY KEY(id,zone))")));

   //get default ports
   TCHAR ports[2000];
   ports[0] = 0;
   DB_RESULT hResult = SQLSelect(_T("SELECT var_value FROM config WHERE var_name='SNMPPorts'"));
   if (hResult != NULL)
   {
      if(DBGetNumRows(hResult) > 0)
      {
         DBGetField(hResult, 0, 0, ports, 2000);
      }
      DBFreeResult(hResult);
   }

   int id = 1;
   if (ports[0] != 0)
   {
      Trim(ports);
      StringList portList(ports, _T(","));
      for (int i = 0; i < portList.size(); i++)
      {
         UINT16 port = (UINT16)_tcstoul(portList.get(i), NULL, 10);
         if (port == 0)
            continue;

         TCHAR query[1024];
         _sntprintf(query, 1024, _T("INSERT INTO snmp_ports (id,port,zone) VALUES(%d,%d,-1)"), id++, port);
         CHK_EXEC(SQLQuery(query));
      }
   }

   //collect zone ports
   hResult = SQLSelect(_T("SELECT zone_guid,snmp_ports FROM zones"));
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      for (int j = 0; j < count; j++)
      {
         ports[0] = 0;
         int zoneId = DBGetFieldULong(hResult, j, 0);
         DBGetField(hResult, j, 1, ports, 2000);

         Trim(ports);
         StringList portList(ports, _T(","));
         for (int i = 0; i < portList.size(); i++)
         {
            UINT16 port = (UINT16)_tcstoul(portList.get(i), NULL, 10);
            if (port == 0)
               continue;

            TCHAR query[1024];
            _sntprintf(query, 1024, _T("INSERT INTO snmp_ports (id,port,zone) VALUES(%d,%d,%d)"), id++, port, zoneId);
            CHK_EXEC(SQLQuery(query));
         }
      }
      DBFreeResult(hResult);
   }

   CHK_EXEC(SQLQuery(_T("DELETE FROM config WHERE var_name='SNMPPorts'")));
   CHK_EXEC(DBDropColumn(g_dbHandle, _T("zones"), _T("snmp_ports")));

   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("snmp_communities"), _T("zone")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("usm_credentials"), _T("zone")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("shared_secrets"), _T("zone")));

   CHK_EXEC(DBDropPrimaryKey(g_dbHandle, _T("snmp_communities")));
   CHK_EXEC(DBDropPrimaryKey(g_dbHandle, _T("usm_credentials")));
   CHK_EXEC(DBDropPrimaryKey(g_dbHandle, _T("shared_secrets")));

   CHK_EXEC(DBAddPrimaryKey(g_dbHandle, _T("snmp_communities"), _T("id,zone")));
   CHK_EXEC(DBAddPrimaryKey(g_dbHandle, _T("usm_credentials"), _T("id,zone")));
   CHK_EXEC(DBAddPrimaryKey(g_dbHandle, _T("shared_secrets"),  _T("id,zone")));

   CHK_EXEC(SetMinorSchemaVersion(8));
   return true;
}

/**
 * Upgrade from 33.6 to 33.7
 */
static bool H_UpgradeFromV6()
{
   CHK_EXEC(CreateConfigParam(_T("DBWriter.MaxQueueSize"), _T("0"),
            _T("Maximum size for DCI data writer queue (0 to disable size limit). If writer queue size grows above that threshold any new data will be dropped until queue size drops below threshold again."),
            _T("elements"), 'I', true, false, false, false));

   CHK_EXEC(CreateEventTemplate(EVENT_DBWRITER_QUEUE_OVERFLOW, _T("SYS_DBWRITER_QUEUE_OVERFLOW"),
            SEVERITY_MAJOR, EF_LOG, _T("fba287e7-a8ea-4503-a494-3d6e5a3d83df"),
            _T("Size of background database writer queue exceeds threshold (new performance data will be discarded)"),
            _T("Generated when background database writer queue size exceeds threshold.\r\n")
            _T("Parameters:\r\n")
            _T("   No message-specific parameters")
            ));
   CHK_EXEC(CreateEventTemplate(EVENT_DBWRITER_QUEUE_NORMAL, _T("SYS_DBWRITER_QUEUE_NORMAL"),
            SEVERITY_NORMAL, EF_LOG, _T("73950d5f-37c0-4b87-9467-c83d7e662401"),
            _T("Size of background database writer queue is below threshold"),
            _T("Generated when background database writer queue size drops below threshold.\r\n")
            _T("Parameters:\r\n")
            _T("   No message-specific parameters")
            ));

   CHK_EXEC(SetMinorSchemaVersion(7));
   return true;
}

/**
 * Alter TimescaleDB table
 */
static bool AlterTSDBTable(const TCHAR *table, bool tableData)
{
   TCHAR query[256];
   _sntprintf(query, 256, _T("ALTER TABLE %s RENAME TO v33_5_%s"), table, table);
   if (!SQLQuery(query))
      return false;

   if (tableData)
   {
      _sntprintf(query, 256,
            _T("CREATE TABLE %s (")
            _T("item_id integer not null,")
            _T("tdata_timestamp timestamptz not null,")
            _T("tdata_value text null,")
            _T("PRIMARY KEY(item_id,tdata_timestamp))"), table);
   }
   else
   {
      _sntprintf(query, 256,
            _T("CREATE TABLE %s (")
            _T("item_id integer not null,")
            _T("idata_timestamp timestamptz not null,")
            _T("idata_value varchar(255) null,")
            _T("raw_value varchar(255) null,")
            _T("PRIMARY KEY(item_id,idata_timestamp))"), table);
   }
   if (!SQLQuery(query))
      return false;

   _sntprintf(query, 256, _T("SELECT create_hypertable('%s', '%s', 'item_id', chunk_time_interval => interval '86400 seconds', number_partitions => 100);"),
            table, tableData ? _T("tdata_timestamp") : _T("idata_timestamp"));
   return SQLQuery(query);
}

/**
 * Upgrade from 33.5 to 33.6
 */
static bool H_UpgradeFromV5()
{
   if (g_dbSyntax == DB_SYNTAX_TSDB)
   {
      CHK_EXEC(SQLQuery(_T("DROP VIEW idata CASCADE")));
      CHK_EXEC(SQLQuery(_T("DROP VIEW tdata CASCADE")));

      CHK_EXEC(AlterTSDBTable(_T("idata_sc_default"), false));
      CHK_EXEC(AlterTSDBTable(_T("idata_sc_7"), false));
      CHK_EXEC(AlterTSDBTable(_T("idata_sc_30"), false));
      CHK_EXEC(AlterTSDBTable(_T("idata_sc_90"), false));
      CHK_EXEC(AlterTSDBTable(_T("idata_sc_180"), false));
      CHK_EXEC(AlterTSDBTable(_T("idata_sc_other"), false));
      CHK_EXEC(AlterTSDBTable(_T("tdata_sc_default"), true));
      CHK_EXEC(AlterTSDBTable(_T("tdata_sc_7"), true));
      CHK_EXEC(AlterTSDBTable(_T("tdata_sc_30"), true));
      CHK_EXEC(AlterTSDBTable(_T("tdata_sc_90"), true));
      CHK_EXEC(AlterTSDBTable(_T("tdata_sc_180"), true));
      CHK_EXEC(AlterTSDBTable(_T("tdata_sc_other"), true));

      CHK_EXEC(SQLQuery(
            _T("CREATE VIEW idata AS")
            _T("   SELECT * FROM idata_sc_default")
            _T("   UNION ALL")
            _T("   SELECT * FROM idata_sc_7")
            _T("   UNION ALL")
            _T("   SELECT * FROM idata_sc_30")
            _T("   UNION ALL")
            _T("   SELECT * FROM idata_sc_90")
            _T("   UNION ALL")
            _T("   SELECT * FROM idata_sc_180")
            _T("   UNION ALL")
            _T("   SELECT * FROM idata_sc_other")));
      CHK_EXEC(SQLQuery(
            _T("CREATE VIEW tdata AS")
            _T("   SELECT * FROM tdata_sc_default")
            _T("   UNION ALL")
            _T("   SELECT * FROM tdata_sc_7")
            _T("   UNION ALL")
            _T("   SELECT * FROM tdata_sc_30")
            _T("   UNION ALL")
            _T("   SELECT * FROM tdata_sc_90")
            _T("   UNION ALL")
            _T("   SELECT * FROM tdata_sc_180")
            _T("   UNION ALL")
            _T("   SELECT * FROM tdata_sc_other")));

      RegisterOnlineUpgrade(33, 6);
   }
   CHK_EXEC(SetMinorSchemaVersion(6));
   return true;
}

/**
 * Upgrade from 33.4 to 33.5
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
 * Upgrade from 33.3 to 33.4
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
 * Upgrade from 33.2 to 33.3
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
 * Upgrade from 33.1 to 33.2
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
   { 12, 34, 0,  H_UpgradeFromV12 },
   { 11, 33, 12, H_UpgradeFromV11 },
   { 10, 33, 11, H_UpgradeFromV10 },
   { 9,  33, 10, H_UpgradeFromV9  },
   { 8,  33, 9,  H_UpgradeFromV8  },
   { 7,  33, 8,  H_UpgradeFromV7  },
   { 6,  33, 7,  H_UpgradeFromV6  },
   { 5,  33, 6,  H_UpgradeFromV5  },
   { 5,  34, 6,  H_UpgradeFromV5  },
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

   while(major == 33)
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
