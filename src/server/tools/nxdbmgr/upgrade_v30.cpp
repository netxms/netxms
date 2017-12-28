/*
** nxdbmgr - NetXMS database manager
** Copyright (C) 2004-2017 Victor Kirhenshtein
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
** File: upgrade_v30.cpp
**
**/

#include "nxdbmgr.h"

/**
 * Upgrade from 30.17 to 30.18 (changes also included into 22.9)
 */
static bool H_UpgradeFromV17()
{
   if (GetSchemaLevelForMajorVersion(22) < 9)
   {
      CHK_EXEC(DBResizeColumn(g_hCoreDB, _T("nodes"), _T("lldp_id"), 255, true));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 9));
   }
   CHK_EXEC(SetMinorSchemaVersion(18));
   return true;
}

/**
 * Upgrade from 30.16 to 30.17 (changes also included into 22.8)
 */
static bool H_UpgradeFromV16()
{
   if (GetSchemaLevelForMajorVersion(22) < 8)
   {
      static TCHAR batch[] =
         _T("ALTER TABLE nodes ADD rack_image_rear varchar(36)\n")
         _T("ALTER TABLE chassis ADD rack_image_rear varchar(36)\n")
         _T("UPDATE nodes SET rack_image_rear='00000000-0000-0000-0000-000000000000'\n")
         _T("UPDATE chassis SET rack_image_rear='00000000-0000-0000-0000-000000000000'\n")
         _T("<END>");
      CHK_EXEC(SQLBatch(batch));
      CHK_EXEC(DBRenameColumn(g_hCoreDB, _T("nodes"), _T("rack_image"), _T("rack_image_front")));
      CHK_EXEC(DBRenameColumn(g_hCoreDB, _T("chassis"), _T("rack_image"), _T("rack_image_front")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 8));
   }
   CHK_EXEC(SetMinorSchemaVersion(17));
   return true;
}

/**
 * Upgrade from 30.15 to 30.16
 */
static bool H_UpgradeFromV15()
{
   static TCHAR batch[] =
      _T("UPDATE config SET default_value='2' WHERE var_name='DefaultEncryptionPolicy'\n")
      _T("UPDATE config SET var_value='2' WHERE var_name='DefaultEncryptionPolicy' AND var_value!='3'\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(SetMinorSchemaVersion(16));
   return true;
}

/**
 * Upgrade from 30.14 to 30.15
 */
static bool H_UpgradeFromV14()
{
   if (GetSchemaLevelForMajorVersion(22) < 6)
   {
      static TCHAR batch[] =
         _T("ALTER TABLE racks ADD passive_elements $SQL:TEXT\n")
         _T("<END>");
      CHK_EXEC(SQLBatch(batch));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 6));
   }
   CHK_EXEC(SetMinorSchemaVersion(15));
   return true;
}

/**
 * Upgrade from 30.13 to 30.14 (changes also included into 22.5)
 */
static bool H_UpgradeFromV13()
{
   if (GetSchemaLevelForMajorVersion(22) < 5)
   {
      static const TCHAR *batch =
               _T("ALTER TABLE items ADD instance_retention_time integer\n")
               _T("ALTER TABLE dc_tables ADD instance_retention_time integer\n")
               _T("UPDATE items SET instance_retention_time=-1\n")
               _T("UPDATE dc_tables SET instance_retention_time=-1\n")
               _T("INSERT INTO config (var_name,var_value,default_value,is_visible,need_server_restart,is_public,data_type,description) ")
               _T("VALUES ('InstanceRetentionTime','0','0',1,1,'Y','I','Default retention time (in days) for missing DCI instances')\n")
               _T("<END>");
      CHK_EXEC(SQLBatch(batch));

      CHK_EXEC(DBSetNotNullConstraint(g_hCoreDB, _T("items"), _T("instance_retention_time")));
      CHK_EXEC(DBSetNotNullConstraint(g_hCoreDB, _T("dc_tables"), _T("instance_retention_time")));

      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 5));
   }
   CHK_EXEC(SetMinorSchemaVersion(14));
   return true;
}

/**
 * Upgrade from 30.12 to 30.13  (changes also included into 21.5 and 22.4)
 */
static bool H_UpgradeFromV12()
{
   if ((GetSchemaLevelForMajorVersion(21) < 5) && (GetSchemaLevelForMajorVersion(22) < 4))
   {
      static const TCHAR *batch =
               _T("ALTER TABLE nodes ADD rack_orientation integer\n")
               _T("ALTER TABLE chassis ADD rack_orientation integer\n")
               _T("UPDATE nodes SET rack_orientation=0\n")
               _T("UPDATE chassis SET rack_orientation=0\n")
               _T("<END>");
      CHK_EXEC(SQLBatch(batch));

      CHK_EXEC(DBSetNotNullConstraint(g_hCoreDB, _T("nodes"), _T("rack_orientation")));
      CHK_EXEC(DBSetNotNullConstraint(g_hCoreDB, _T("chassis"), _T("rack_orientation")));

      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 4));
      CHK_EXEC(SetSchemaLevelForMajorVersion(21, 5));
   }
   CHK_EXEC(SetMinorSchemaVersion(13));
   return true;
}

/**
 * Upgrade from 30.11 to 30.12
 */
static bool H_UpgradeFromV11()
{
   if (GetSchemaLevelForMajorVersion(22) < 3)
   {
      CHK_EXEC(CreateTable(
            _T("CREATE TABLE dci_access (")
            _T("   dci_id integer not null,")
            _T("   user_id integer not null,")
            _T("   PRIMARY KEY(dci_id,user_id))")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 3));
   }

   CHK_EXEC(SetMinorSchemaVersion(12));
   return true;
}

/**
 * Upgrade from 30.10 to 30.11
 */
static bool H_UpgradeFromV10()
{
   CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='CREATE TABLE idata_%d (item_id integer not null,idata_timestamp integer not null,idata_value varchar(255) null,raw_value varchar(255) null)' WHERE var_name='IDataTableCreationCommand'")));

   IntegerArray<UINT32> *targets = GetDataCollectionTargets();
   for(int i = 0; i < targets->size(); i++)
   {
      TCHAR query[256];
      _sntprintf(query, 256, _T("ALTER TABLE idata_%d ADD raw_value varchar(255)"), targets->get(i));
      CHK_EXEC(SQLQuery(query));
   }
   delete targets;

   CHK_EXEC(SetMinorSchemaVersion(11));
   return true;
}

/**
 * Upgrade from 30.9 to 30.10
 */
static bool H_UpgradeFromV9()
{
   static const TCHAR *batch =
            _T("ALTER TABLE snmp_communities ADD zone integer null\n")
            _T("ALTER TABLE usm_credentials ADD zone integer null\n")
            _T("ALTER TABLE zones ADD snmp_ports varchar(512) null\n")
            _T("UPDATE snmp_communities SET zone=-1\n")
            _T("UPDATE usm_credentials SET zone=-1\n")
            _T("UPDATE zones SET snmp_ports=''\n")
            _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   DBSetNotNullConstraint(g_hCoreDB, _T("snmp_communities"), _T("zone"));
   DBSetNotNullConstraint(g_hCoreDB, _T("usm_credentials"), _T("zone"));

   CHK_EXEC(SetMinorSchemaVersion(10));
   return true;
}

/**
 * Upgrade from 30.8 to 30.9 (changes also included into 22.2)
 */
static bool H_UpgradeFromV8()
{
   if (GetSchemaLevelForMajorVersion(22) < 2)
   {
      CHK_EXEC(CreateConfigParam(_T("DBWriter.MaxRecordsPerTransaction"), _T("1000"), _T("Maximum number of records per one transaction for delayed database writes."), 'I', true, true, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 2));
   }
   CHK_EXEC(SetMinorSchemaVersion(9));
   return true;
}

/**
 * Upgrade from 30.7 to 30.8 (changes also included into 22.1)
 */
static bool H_UpgradeFromV7()
{
   if (GetSchemaLevelForMajorVersion(22) < 1)
   {
      int count = ConfigReadInt(_T("NumberOfDataCollectors"), 250);
      TCHAR value[64];
      _sntprintf(value, 64,_T("%d"), std::max(250, count));
      CHK_EXEC(CreateConfigParam(_T("DataCollector.ThreadPool.BaseSize"), _T("10"), _T("Base size for data collector thread pool."), 'I', true, true, false, false));
      CHK_EXEC(CreateConfigParam(_T("DataCollector.ThreadPool.MaxSize"), value, _T("Maximum size for data collector thread pool."), 'I', true, true, false, false));
      CHK_EXEC(SQLQuery(_T("UPDATE config SET default_value='250' WHERE var_name='DataCollector.ThreadPool.MaxSize'")));
      CHK_EXEC(SQLQuery(_T("DELETE FROM config WHERE var_name='NumberOfDataCollectors'")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 1));
   }
   CHK_EXEC(SetMinorSchemaVersion(8));
   return true;
}

/**
 * Upgrade from 30.6 to 30.7 (changes also included into 21.4 and all 22.x)
 */
static bool H_UpgradeFromV6()
{
   if ((GetSchemaLevelForMajorVersion(21) < 4) && (GetSchemaLevelForMajorVersion(22) < 1))
   {
      DB_RESULT hResult = DBSelect(g_hCoreDB, _T("SELECT access_rights,object_id FROM acl WHERE user_id=-2147483647")); // Get group Admins object acl
      if (hResult != NULL)
      {
         DB_STATEMENT hStmt = DBPrepare(g_hCoreDB, _T("UPDATE acl SET access_rights=? WHERE user_id=-2147483647 AND object_id=? "));
         if (hStmt != NULL)
         {
            int nRows = DBGetNumRows(hResult);
            UINT32 rights;
            for(int i = 0; i < nRows; i++)
            {
               rights = DBGetFieldULong(hResult, i, 0);
               if (rights & OBJECT_ACCESS_READ)
               {
                  rights |= (OBJECT_ACCESS_READ_AGENT | OBJECT_ACCESS_READ_SNMP | OBJECT_ACCESS_SCREENSHOT);
                  DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, rights);
                  DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, DBGetFieldULong(hResult, i, 1));

                  if (!SQLExecute(hStmt))
                  {
                     if (!g_bIgnoreErrors)
                     {
                        DBFreeStatement(hStmt);
                        DBFreeResult(hResult);
                        return FALSE;
                     }
                  }
               }
            }

            DBFreeStatement(hStmt);
         }
         else if (!g_bIgnoreErrors)
            return FALSE;
         DBFreeResult(hResult);
      }
      else if (!g_bIgnoreErrors)
         return false;
      CHK_EXEC(SetSchemaLevelForMajorVersion(21, 4));
   }
   CHK_EXEC(SetMinorSchemaVersion(7));
   return true;
}

/**
 * Upgrade from 30.5 to 30.6 (changes also included into 21.3 and all 22.x)
 */
static bool H_UpgradeFromV5()
{
   if ((GetSchemaLevelForMajorVersion(21) < 3) && (GetSchemaLevelForMajorVersion(22) < 1))
   {
      static const TCHAR *batch =
               _T("UPDATE nodes SET fail_time_snmp=0 WHERE fail_time_snmp IS NULL\n")
               _T("UPDATE nodes SET fail_time_agent=0 WHERE fail_time_agent IS NULL\n")
               _T("<END>");
      CHK_EXEC(SQLBatch(batch));

      CHK_EXEC(DBSetNotNullConstraint(g_hCoreDB, _T("nodes"), _T("fail_time_snmp")));
      CHK_EXEC(DBSetNotNullConstraint(g_hCoreDB, _T("nodes"), _T("fail_time_agent")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(21, 3));
   }
   CHK_EXEC(SetMinorSchemaVersion(6));
   return true;
}

/**
 * Upgrade from 30.4 to 30.5 (changes also included into 21.2 and all 22.x)
 */
static bool H_UpgradeFromV4()
{
   if ((GetSchemaLevelForMajorVersion(21) < 2) && (GetSchemaLevelForMajorVersion(22) < 1))
   {
      static const TCHAR *batch =
               _T("ALTER TABLE nodes ADD fail_time_snmp integer\n")
               _T("ALTER TABLE nodes ADD fail_time_agent integer\n")
               _T("<END>");
      CHK_EXEC(SQLBatch(batch));
      CHK_EXEC(SetSchemaLevelForMajorVersion(21, 2));
   }
   CHK_EXEC(SetMinorSchemaVersion(5));
   return true;
}

/**
 * Move object flags from old to new tables
 */
static BOOL MoveFlagsFromOldTables(const TCHAR *tableName)
{
   TCHAR query[256];
   _sntprintf(query, 256, _T("SELECT id,flags FROM %s"), tableName);
   DB_RESULT hResult = DBSelect(g_hCoreDB, query);
   DB_STATEMENT hStmt = DBPrepare(g_hCoreDB, _T("UPDATE object_properties SET flags=? WHERE object_id=?"));
   if (hResult != NULL)
   {
      if (hStmt != NULL)
      {
         int nRows = DBGetNumRows(hResult);
         for(int i = 0; i < nRows; i++)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, DBGetFieldULong(hResult, i, 1));
            DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, DBGetFieldULong(hResult, i, 0));

            if (!SQLExecute(hStmt))
            {
               if (!g_bIgnoreErrors)
               {
                  DBFreeStatement(hStmt);
                  DBFreeResult(hResult);
                  return FALSE;
               }
            }
         }
         DBFreeStatement(hStmt);
      }
      else if (!g_bIgnoreErrors)
      {
         return FALSE;
      }
      DBFreeResult(hResult);
   }
   else if (!g_bIgnoreErrors)
   {
      return FALSE;
   }

   CHK_EXEC(DBDropColumn(g_hCoreDB, tableName, _T("flags")));
   return TRUE;
}

/**
 * Move single flag
 */
inline void MoveFlag(UINT32 oldVar, UINT32 *newVar, UINT32 oldFlag, UINT32 newFlag)
{
   *newVar |= ((oldVar & oldFlag) != 0) ? newFlag : 0;
}

/**
 * Move node flags
 */
static void MoveNodeFlags(UINT32 oldFlag, UINT32 *flags)
{
   MoveFlag(oldFlag, flags, 0x10000000, DCF_DISABLE_STATUS_POLL);
   MoveFlag(oldFlag, flags, 0x20000000, DCF_DISABLE_CONF_POLL);
   MoveFlag(oldFlag, flags, 0x80000000, DCF_DISABLE_DATA_COLLECT);
   MoveFlag(oldFlag, flags, 0x00000080, NF_REMOTE_AGENT);
   MoveFlag(oldFlag, flags, 0x00400000, NF_DISABLE_DISCOVERY_POLL);
   MoveFlag(oldFlag, flags, 0x00800000, NF_DISABLE_TOPOLOGY_POLL);
   MoveFlag(oldFlag, flags, 0x01000000, NF_DISABLE_SNMP);
   MoveFlag(oldFlag, flags, 0x02000000, NF_DISABLE_NXCP);
   MoveFlag(oldFlag, flags, 0x04000000, NF_DISABLE_ICMP);
   MoveFlag(oldFlag, flags, 0x08000000, NF_FORCE_ENCRYPTION);
   MoveFlag(oldFlag, flags, 0x40000000, NF_DISABLE_ROUTE_POLL);
}

/**
 * Move node capabilities flags
 */
static void MoveNodeCapabilities(UINT32 oldFlag, UINT32 *capabilities)
{
   MoveFlag(oldFlag, capabilities, 0x00000001, NC_IS_SNMP);
   MoveFlag(oldFlag, capabilities, 0x00000002, NC_IS_NATIVE_AGENT);
   MoveFlag(oldFlag, capabilities, 0x00000004, NC_IS_BRIDGE);
   MoveFlag(oldFlag, capabilities, 0x00000008, NC_IS_ROUTER);
   MoveFlag(oldFlag, capabilities, 0x00000010, NC_IS_LOCAL_MGMT);
   MoveFlag(oldFlag, capabilities, 0x00000020, NC_IS_PRINTER);
   MoveFlag(oldFlag, capabilities, 0x00000040, NC_IS_OSPF);
   MoveFlag(oldFlag, capabilities, 0x00000100, NC_IS_CPSNMP);
   MoveFlag(oldFlag, capabilities, 0x00000200, NC_IS_CDP);
   MoveFlag(oldFlag, capabilities, 0x00000400, NC_IS_NDP);
   MoveFlag(oldFlag, capabilities, 0x00000800, NC_IS_LLDP);
   MoveFlag(oldFlag, capabilities, 0x00001000, NC_IS_VRRP);
   MoveFlag(oldFlag, capabilities, 0x00002000, NC_HAS_VLANS);
   MoveFlag(oldFlag, capabilities, 0x00004000, NC_IS_8021X);
   MoveFlag(oldFlag, capabilities, 0x00008000, NC_IS_STP);
   MoveFlag(oldFlag, capabilities, 0x00010000, NC_HAS_ENTITY_MIB);
   MoveFlag(oldFlag, capabilities, 0x00020000, NC_HAS_IFXTABLE);
   MoveFlag(oldFlag, capabilities, 0x00040000, NC_HAS_AGENT_IFXCOUNTERS);
   MoveFlag(oldFlag, capabilities, 0x00080000, NC_HAS_WINPDH);
   MoveFlag(oldFlag, capabilities, 0x00100000, NC_IS_WIFI_CONTROLLER);
   MoveFlag(oldFlag, capabilities, 0x00200000, NC_IS_SMCLP);
}

/**
 * Move node state flags
 */
static void MoveNodeState(UINT32 oldRuntime, UINT32 *state)
{
   MoveFlag(oldRuntime, state, 0x000004, DCSF_UNREACHABLE);
   MoveFlag(oldRuntime, state, 0x000008, NSF_AGENT_UNREACHABLE);
   MoveFlag(oldRuntime, state, 0x000010, NSF_SNMP_UNREACHABLE);
   MoveFlag(oldRuntime, state, 0x000200, NSF_CPSNMP_UNREACHABLE);
   MoveFlag(oldRuntime, state, 0x008000, DCSF_NETWORK_PATH_PROBLEM);
   MoveFlag(oldRuntime, state, 0x020000, NSF_CACHE_MODE_NOT_SUPPORTED);
}

/**
 * Move sensor state flags
 */
static void MoveSensorState(UINT32 oldFlag, UINT32 oldRuntime, UINT32 *status)
{
   MoveFlag(oldFlag, status, 0x00000001, SSF_PROVISIONED);
   MoveFlag(oldFlag, status, 0x00000002, SSF_REGISTERED);
   MoveFlag(oldFlag, status, 0x00000004, SSF_ACTIVE);
   MoveFlag(oldFlag, status, 0x00000008, SSF_CONF_UPDATE_PENDING);
   MoveFlag(oldRuntime, status, 0x000004, DCSF_UNREACHABLE);
}

/**
 * Upgrade from 30.3 to 30.4
 */
static bool H_UpgradeFromV3()
{
   static const TCHAR *batch =
            _T("ALTER TABLE object_properties ADD flags integer null\n")
            _T("ALTER TABLE object_properties ADD state integer null\n")
            _T("ALTER TABLE nodes ADD capabilities integer null\n")
            _T("UPDATE object_properties set flags=0,state=0\n")
            _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   //move flags from old tables to the new one
   CHK_EXEC(MoveFlagsFromOldTables(_T("interfaces")));
   CHK_EXEC(MoveFlagsFromOldTables(_T("templates")));
   CHK_EXEC(MoveFlagsFromOldTables(_T("chassis")));
   CHK_EXEC(MoveFlagsFromOldTables(_T("object_containers")));
   CHK_EXEC(MoveFlagsFromOldTables(_T("network_maps")));
   //create special behavior for node and sensor, cluster
   //node
   DB_RESULT hResult = DBSelect(g_hCoreDB, _T("SELECT id,runtime_flags FROM nodes"));
   DB_STATEMENT stmtNetObj = DBPrepare(g_hCoreDB, _T("UPDATE object_properties SET flags=?, state=? WHERE object_id=?"));
   DB_STATEMENT stmtNode = DBPrepare(g_hCoreDB, _T("UPDATE nodes SET capabilities=? WHERE id=?"));
   if (hResult != NULL)
   {
      if (stmtNetObj != NULL && stmtNode != NULL)
      {
         int nRows = DBGetNumRows(hResult);
         for(int i = 0; i < nRows; i++)
         {
            UINT32 id = DBGetFieldULong(hResult, i, 0);
            UINT32 oldFlags = 0;
            UINT32 oldRuntime = DBGetFieldULong(hResult, i, 1);
            UINT32 flags = 0;
            UINT32 state = 0;
            UINT32 capabilities = 0;
            TCHAR query[256];
            _sntprintf(query, 256, _T("SELECT node_flags FROM nodes WHERE id=%d"), id);
            DB_RESULT flagResult = DBSelect(g_hCoreDB, query);
            if(DBGetNumRows(flagResult) >= 1)
            {
               oldFlags = DBGetFieldULong(flagResult, 0, 0);
            }
            else
            {
               if(!g_bIgnoreErrors)
               {
                  DBFreeStatement(stmtNetObj);
                  DBFreeStatement(stmtNode);
                  DBFreeResult(hResult);
                  return FALSE;
               }
            }
            MoveNodeFlags(oldFlags, &flags);
            MoveNodeCapabilities(oldFlags, &capabilities);
            MoveNodeState(oldRuntime, &state);

            DBBind(stmtNetObj, 1, DB_SQLTYPE_INTEGER, flags);
            DBBind(stmtNetObj, 2, DB_SQLTYPE_INTEGER, state);
            DBBind(stmtNetObj, 3, DB_SQLTYPE_INTEGER, id);

            DBBind(stmtNode, 1, DB_SQLTYPE_INTEGER, capabilities);
            DBBind(stmtNode, 2, DB_SQLTYPE_INTEGER, id);

            if (!(SQLExecute(stmtNetObj)))
            {
               if (!g_bIgnoreErrors)
               {
                  DBFreeStatement(stmtNetObj);
                  DBFreeStatement(stmtNode);
                  DBFreeResult(hResult);
                  return FALSE;
               }
            }

            if (!SQLExecute(stmtNode))
            {
               if (!g_bIgnoreErrors)
               {
                  DBFreeStatement(stmtNetObj);
                  DBFreeStatement(stmtNode);
                  DBFreeResult(hResult);
                  return FALSE;
               }
            }
         }
         DBFreeStatement(stmtNetObj);
         DBFreeStatement(stmtNode);
     }
     else
     {
        if(stmtNetObj != NULL)
           DBFreeStatement(stmtNetObj);

        if(stmtNode != NULL)
           DBFreeStatement(stmtNode);
        if (!g_bIgnoreErrors)
        {
           return FALSE;
        }
     }
     DBFreeResult(hResult);
   }
   CHK_EXEC(DBDropColumn(g_hCoreDB, _T("nodes"), _T("runtime_flags")));
   CHK_EXEC(DBDropColumn(g_hCoreDB, _T("nodes"), _T("node_flags")));

   //sensor
   hResult = DBSelect(g_hCoreDB, _T("SELECT id,runtime_flags,flags FROM sensors"));
   DB_STATEMENT stmt = DBPrepare(g_hCoreDB, _T("UPDATE object_properties SET status=? WHERE object_id=?"));
   if (hResult != NULL)
   {
      if (stmt != NULL)
      {
         int nRows = DBGetNumRows(hResult);
         for(int i = 0; i < nRows; i++)
         {
            UINT32 status = 0;
            MoveSensorState(DBGetFieldULong(hResult, i, 2), DBGetFieldULong(hResult, i, 1), &status);

            DBBind(stmt, 1, DB_SQLTYPE_INTEGER, status);
            DBBind(stmt, 2, DB_SQLTYPE_INTEGER, DBGetFieldULong(hResult, i, 0));

            if (!(SQLExecute(stmt)))
            {
               if (!g_bIgnoreErrors)
               {
                  DBFreeStatement(stmt);
                  DBFreeResult(hResult);
                  return FALSE;
               }
            }
         }
         DBFreeStatement(stmt);
      }
      else if (!g_bIgnoreErrors)
      {
         return FALSE;
      }
      DBFreeResult(hResult);
   }
   CHK_EXEC(DBDropColumn(g_hCoreDB, _T("sensors"), _T("runtime_flags")));
   CHK_EXEC(DBDropColumn(g_hCoreDB, _T("sensors"), _T("flags")));

   CHK_EXEC(SetMinorSchemaVersion(4));
   return true;
}

/**
 * Upgrade from 30.2 to 30.3
 */
static bool H_UpgradeFromV2()
{
   CHK_EXEC(DBDropColumn(g_hCoreDB, _T("event_groups"), _T("range_start")));
   CHK_EXEC(DBDropColumn(g_hCoreDB, _T("event_groups"), _T("range_end")));

   static const TCHAR *batch =
            _T("ALTER TABLE event_groups ADD guid varchar(36) null\n")
            _T("UPDATE event_groups SET guid='04b326c0-5cc0-411f-8587-2836cb87c920' WHERE id=-2147483647\n")
            _T("UPDATE event_groups SET guid='b61859c6-1768-4a61-a0cf-eed07d688f66' WHERE id=-2147483646\n")
            _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   DBSetNotNullConstraint(g_hCoreDB, _T("event_groups"), _T("guid"));

   CHK_EXEC(SetMinorSchemaVersion(3));
   return true;
}

/**
 * Upgrade from 30.1 to 30.2
 */
static bool H_UpgradeFromV1()
{
   static const TCHAR *batch =
            _T("ALTER TABLE users ADD created integer null\n")
            _T("ALTER TABLE user_groups ADD created integer null\n")
            _T("UPDATE users SET created=0\n")
            _T("UPDATE user_groups SET created=0\n")
            _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   CHK_EXEC(DBSetNotNullConstraint(g_hCoreDB, _T("users"), _T("created")));
   CHK_EXEC(DBSetNotNullConstraint(g_hCoreDB, _T("user_groups"), _T("created")));

   CHK_EXEC(SetMinorSchemaVersion(2));
   return true;
}

/**
 * Upgrade from 30.0 to 30.1
 */
static bool H_UpgradeFromV0()
{
   CHK_EXEC(CreateTable(
      _T("CREATE TABLE sensors (")
      _T("  id integer not null,")
      _T("  proxy_node integer not null,")
      _T("  flags integer not null,")
      _T("  mac_address varchar(16) null,")
      _T("  device_class integer not null,")
      _T("  vendor varchar(128) null,")
      _T("  communication_protocol integer not null,")
      _T("  xml_config varchar(4000) null,")
      _T("  xml_reg_config varchar(4000) null,")
      _T("  serial_number varchar(256) null,")
      _T("  device_address varchar(256) null,")
      _T("  meta_type varchar(256) null,")
      _T("  description varchar(512) null,")
      _T("  last_connection_time integer not null,")
      _T("  frame_count integer not null,")
      _T("  signal_strenght integer not null,")
      _T("  signal_noise integer not null,")
      _T("  frequency integer not null,")
      _T("  runtime_flags integer null,")
      _T("PRIMARY KEY(id))")));

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
   { 17, 30, 18, H_UpgradeFromV17 },
   { 16, 30, 17, H_UpgradeFromV16 },
   { 15, 30, 16, H_UpgradeFromV15 },
   { 14, 30, 15, H_UpgradeFromV14 },
   { 13, 30, 14, H_UpgradeFromV13 },
   { 12, 30, 13, H_UpgradeFromV12 },
   { 11, 30, 12, H_UpgradeFromV11 },
   { 10, 30, 11, H_UpgradeFromV10 },
   { 9, 30, 10, H_UpgradeFromV9 },
   { 8, 30, 9, H_UpgradeFromV8 },
   { 7, 30, 8, H_UpgradeFromV7 },
   { 6, 30, 7, H_UpgradeFromV6 },
   { 5, 30, 6, H_UpgradeFromV5 },
   { 4, 30, 5, H_UpgradeFromV4 },
   { 3, 30, 4, H_UpgradeFromV3 },
   { 2, 30, 3, H_UpgradeFromV2 },
   { 1, 30, 2, H_UpgradeFromV1 },
   { 0, 30, 1, H_UpgradeFromV0 },
   { 0, 0, 0, NULL }
};

/**
 * Upgrade database to new version
 */
bool MajorSchemaUpgrade_V30()
{
   INT32 major, minor;
   if (!DBGetSchemaVersion(g_hCoreDB, &major, &minor))
      return false;

   while((major == 30) && (minor < DB_SCHEMA_VERSION_V30_MINOR))
   {
      // Find upgrade procedure
      int i;
      for(i = 0; s_dbUpgradeMap[i].upgradeProc != NULL; i++)
         if (s_dbUpgradeMap[i].version == minor)
            break;
      if (s_dbUpgradeMap[i].upgradeProc == NULL)
      {
         _tprintf(_T("Unable to find upgrade procedure for version 30.%d\n"), minor);
         return false;
      }
      _tprintf(_T("Upgrading from version 30.%d to %d.%d\n"), minor, s_dbUpgradeMap[i].nextMajor, s_dbUpgradeMap[i].nextMinor);
      DBBegin(g_hCoreDB);
      if (s_dbUpgradeMap[i].upgradeProc())
      {
         DBCommit(g_hCoreDB);
         if (!DBGetSchemaVersion(g_hCoreDB, &major, &minor))
            return false;
      }
      else
      {
         _tprintf(_T("Rolling back last stage due to upgrade errors...\n"));
         DBRollback(g_hCoreDB);
         return false;
      }
   }
   return true;
}
