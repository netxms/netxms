/*
** nxdbmgr - NetXMS database manager
** Copyright (C) 2004-2019 Victor Kirhenshtein
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
#include <nxevent.h>

/**
* Upgrade from 30.56 to 30.57
*/
static bool H_UpgradeFromV56()
{
   CHK_EXEC(CreateTable(
         _T("CREATE TABLE software_inventory (")
         _T("   node_id integer not null,")
         _T("   name varchar(255) not null,")
         _T("   version varchar(63) not null,")
         _T("   vendor varchar(63) null,")
         _T("   date integer not null,")
         _T("   url varchar(255) null,")
         _T("   description varchar(255) null,")
         _T("   PRIMARY KEY(node_id,version,name))")));

   CHK_EXEC(CreateTable(
         _T("CREATE TABLE hardware_inventory (")
         _T("   node_id integer not null,")
         _T("   component_type varchar(63) not null,")
         _T("   component_index integer not null,")
         _T("   vendor varchar(63) null,")
         _T("   model varchar(63) null,")
         _T("   capacity integer not null,")
         _T("   serial varchar(63) null,")
         _T("   PRIMARY KEY(node_id,component_type,component_index))")));

   CHK_EXEC(SetMinorSchemaVersion(57));
   return true;
}

/**
 * Upgrade from 30.55 to 30.56
 */
static bool H_UpgradeFromV55()
{

   //Rename old ap_common
   DBRenameTable(g_dbHandle, _T("ap_common"), _T("ap_common_old"));
   //create new ap_common
   CHK_EXEC(CreateTable(
      _T("CREATE TABLE ap_common (")
      _T("   policy_name varchar(63) not null,")
      _T("   owner_id integer not null,")
      _T("   policy_type varchar(31) not null,")
      _T("   file_content $SQL:TEXT null,")
      _T("   version integer not null,")
      _T("   guid varchar(36) not null,")
      _T("   PRIMARY KEY(guid))")));

   bool success = true;

   DB_RESULT hResult = DBSelect(g_dbHandle, _T("SELECT id,policy_type,name,guid FROM ap_common_old a INNER JOIN object_properties p ON p.object_id=a.id WHERE policy_type IN (1,2)"));
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      for(int i=0; i < count; i++)
      {
         UINT32 id = DBGetFieldLong(hResult, i, 0);
         UINT32 type = DBGetFieldLong(hResult, i, 1);
         TCHAR name[MAX_OBJECT_NAME];
         DBGetField(hResult, i, 2, name, MAX_OBJECT_NAME);
         TCHAR guid[64];
         DBGetField(hResult, i, 3, guid, 64);

         TCHAR *content = NULL;

         TCHAR query[512];
         if (type == 1)
         {
            _sntprintf(query, 512, _T("SELECT file_content FROM ap_config_files WHERE policy_id=%d"), id);
            DB_RESULT policy = DBSelect(g_dbHandle, query);
            if (policy != NULL)
            {
               if(DBGetNumRows(policy) > 0)
                  content = DBGetField(policy, 0, 0, NULL, 0);
               DBFreeResult(policy);
            }
            else
            {
               success = false;
               break;
            }
         }
         else
         {
            _sntprintf(query, 512, _T("SELECT file_content FROM ap_log_parser WHERE policy_id=%d"), id);
            DB_RESULT policy = DBSelect(g_dbHandle, query);
            if (policy != NULL)
            {
               if(DBGetNumRows(policy) > 0)
                  content = DBGetField(policy, 0, 0, NULL, 0);
               DBFreeResult(policy);
            }
            else
            {
               success = false;
               break;
            }
         }

         //Change every policy to template
         DB_STATEMENT hStmt = DBPrepare(g_dbHandle, _T("INSERT INTO templates (id) VALUES (?)"));
         if (hStmt != NULL)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, id);
            if (!(SQLExecute(hStmt)))
            {
               if (!g_ignoreErrors)
               {
                  success = false;
                  break;
               }
            }
            DBFreeStatement(hStmt);
         }
         else
         {
            success = false;
            break;
         }

         //Move common info to the new table
         DB_STATEMENT apCommon = DBPrepare(g_dbHandle, _T("INSERT INTO ap_common (policy_name,owner_id,policy_type,file_content,version,guid) VALUES (?,?,?,?,?,?)"));
         if (apCommon == NULL)
         {
            success = false;
            break;
         }

         DBBind(apCommon, 1, DB_SQLTYPE_TEXT, name, DB_BIND_STATIC);
         DBBind(apCommon, 2, DB_SQLTYPE_INTEGER, id);
         DBBind(apCommon, 3, DB_SQLTYPE_VARCHAR, type == 1 ? _T("AgentConfig") : _T("LogParserConfig"), DB_BIND_STATIC);
         DBBind(apCommon, 4, DB_SQLTYPE_VARCHAR, content, DB_BIND_STATIC);
         DBBind(apCommon, 5, DB_SQLTYPE_INTEGER, 0);
         DBBind(apCommon, 6, DB_SQLTYPE_VARCHAR, guid, DB_BIND_STATIC);
         if (!(SQLExecute(apCommon)))
         {
            if (!g_ignoreErrors)
            {
               success = false;
               break;
            }
         }
         DBFreeStatement(apCommon);
         MemFree(content);
      }
      DBFreeResult(hResult);
   }

   if (!success || (hResult == NULL))
   {
      if (!g_ignoreErrors)
         return false;
   }

   //ap_bindings move to template bindings dct_node_map (template_id,node_id)
   hResult = DBSelect(g_dbHandle, _T("SELECT policy_id,node_id FROM ap_bindings"));
   if (hResult == NULL)
   {
      if (!g_ignoreErrors)
         return false;
   }

   int count = DBGetNumRows(hResult);
   for(int i=0; i < count; i++)
   {
      DB_STATEMENT apBinding = DBPrepare(g_dbHandle, _T("INSERT INTO dct_node_map (template_id,node_id) VALUES (?,?)"));
      if(apBinding != NULL)
      {
         DBBind(apBinding, 1, DB_SQLTYPE_INTEGER, DBGetFieldLong(hResult, i, 0));
         DBBind(apBinding, 2, DB_SQLTYPE_INTEGER, DBGetFieldLong(hResult, i, 1));
         if (!(SQLExecute(apBinding)))
         {
            if (!g_ignoreErrors)
            {
               success = false;
               break;
            }
         }
         DBFreeStatement(apBinding);
      }
   }
   DBFreeResult(hResult);

   if (!success && !g_ignoreErrors)
      return false;

   CHK_EXEC(SQLQuery(_T("UPDATE container_members SET container_id=3 WHERE container_id=5")));
   CHK_EXEC(SQLQuery(_T("UPDATE object_containers SET object_class=9 WHERE object_class=15")));
   CHK_EXEC(SQLQuery(_T("DELETE FROM object_containers WHERE id=5")));
   CHK_EXEC(SQLQuery(_T("DELETE FROM object_custom_attributes WHERE object_id=5")));
   CHK_EXEC(SQLQuery(_T("DELETE FROM object_urls WHERE object_id=5")));
   CHK_EXEC(SQLQuery(_T("DELETE FROM responsible_users WHERE object_id=5")));
   CHK_EXEC(SQLQuery(_T("DELETE FROM acl WHERE object_id=5")));
   CHK_EXEC(SQLQuery(_T("DELETE FROM object_properties WHERE object_id=5")));

   CHK_EXEC(SQLQuery(_T("DELETE FROM scheduled_tasks WHERE taskid='Policy.Deploy'")));
   CHK_EXEC(SQLQuery(_T("DELETE FROM scheduled_tasks WHERE taskid='Policy.Uninstall'")));

   CHK_EXEC(SQLQuery(_T("DROP TABLE ap_common_old")));
   CHK_EXEC(SQLQuery(_T("DROP TABLE ap_log_parser")));
   CHK_EXEC(SQLQuery(_T("DROP TABLE ap_config_files")));
   CHK_EXEC(SQLQuery(_T("DROP TABLE ap_bindings")));

   CHK_EXEC(SetMinorSchemaVersion(56));
   return true;
}

/**
 * Upgrade from 30.54 to 30.55 (changes also included into 22.43)
 */
static bool H_UpgradeFromV54()
{
   if (GetSchemaLevelForMajorVersion(22) < 43)
   {
      CHK_EXEC(CreateConfigParam(_T("NetworkDiscovery.EnableParallelProcessing"), _T("0"), _T("Enable/disable parallel processing of discovered addresses."), NULL, 'B', true, false, false, false));
      CHK_EXEC(CreateConfigParam(_T("ThreadPool.Discovery.BaseSize"), _T("1"), _T("Base size for network discovery thread pool."), NULL, 'I', true, true, false, false));
      CHK_EXEC(CreateConfigParam(_T("ThreadPool.Discovery.MaxSize"), _T("16"), _T("Maximum size for network discovery thread pool."), NULL, 'I', true, true, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 43));
   }
   CHK_EXEC(SetMinorSchemaVersion(55));
   return true;
}

/**
 * Upgrade from 30.53 to 30.54 (changes also included into 22.42)
 */
static bool H_UpgradeFromV53()
{
   if (GetSchemaLevelForMajorVersion(22) < 42)
   {
      CHK_EXEC(DBDropPrimaryKey(g_dbHandle, _T("address_lists")));

      static const TCHAR *batch =
         _T("ALTER TABLE address_lists ADD zone_uin integer\n")
         _T("ALTER TABLE address_lists ADD proxy_id integer\n")
         _T("UPDATE address_lists SET zone_uin=0,proxy_id=0\n")
         _T("<END>");
      CHK_EXEC(SQLBatch(batch));

      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("address_lists"), _T("proxy_id")));
      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("address_lists"), _T("zone_uin")));
      CHK_EXEC(DBAddPrimaryKey(g_dbHandle, _T("address_lists"), _T("list_type,community_id,zone_uin,addr_type,addr1,addr2")));

      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 42));
   }
   CHK_EXEC(SetMinorSchemaVersion(54));
   return true;
}

/**
 * Upgrade from 30.52 to 30.53
 */
static bool H_UpgradeFromV52()
{
   CHK_EXEC(CreateConfigParam(_T("DataCollection.StartupDelay"), _T("0"), _T("Enable/disable randomized data collection delays on server startup for evening server load distrubution."), NULL, 'B', true, true, false, false));
   CHK_EXEC(SetMinorSchemaVersion(53));
   return true;
}

/**
 * Upgrade from 30.51 to 30.52 (changes also included into 22.41)
 */
static bool H_UpgradeFromV51()
{
   if (GetSchemaLevelForMajorVersion(22) < 41)
   {
      CHK_EXEC(DBRenameColumn(g_dbHandle, _T("dct_threshold_instances"), _T("row_number"), _T("tt_row_number")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 41));
   }
   CHK_EXEC(SetMinorSchemaVersion(52));
   return true;
}

/**
 * Upgrade from 30.50 to 30.51 (changes also included into 22.40)
 */
static bool H_UpgradeFromV50()
{
   if (GetSchemaLevelForMajorVersion(22) < 40)
   {
      static const TCHAR *batch =
         _T("ALTER TABLE items ADD grace_period_start integer\n")
         _T("ALTER TABLE dc_tables ADD grace_period_start integer\n")
         _T("UPDATE items SET grace_period_start=0\n")
         _T("UPDATE dc_tables SET grace_period_start=0\n")
         _T("<END>");
      CHK_EXEC(SQLBatch(batch));

      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("items"), _T("grace_period_start")));
      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("dc_tables"), _T("grace_period_start")));

      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 40));
   }
   CHK_EXEC(SetMinorSchemaVersion(51));
   return true;
}

/**
 * Upgrade from 30.49 to 30.50 (changes also included into 22.39)
 */
static bool H_UpgradeFromV49()
{
   if (GetSchemaLevelForMajorVersion(22) < 39)
   {
      TCHAR tmp[MAX_CONFIG_VALUE] = _T("");
      DB_RESULT hResult = DBSelect(g_dbHandle, _T("SELECT var_value from config WHERE var_name='LdapMappingName'"));
      if (hResult != NULL)
      {
         if(DBGetNumRows(hResult) > 0)
            DBGetField(hResult, 0, 0, tmp, MAX_CONFIG_VALUE);
      }

      CHK_EXEC(SQLQuery(_T("UPDATE config SET var_name='LdapUserMappingName' WHERE var_name='LdapMappingName'")));
      CHK_EXEC(CreateConfigParam(_T("LdapGroupMappingName"), _T(""), _T("The name of an attribute whose value will be used as a group''s login name."), NULL, 'S', true, true, false, false));

      DB_STATEMENT hStmt = DBPrepare(g_dbHandle, _T("UPDATE config SET var_value=? WHERE var_name='LdapGroupMappingName'"));
      if (hStmt != NULL)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_TEXT, tmp, DB_BIND_STATIC);
         CHK_EXEC(SQLExecute(hStmt));
         DBFreeStatement(hStmt);
      }
      else
      {
         if (!g_ignoreErrors)
            return false;
      }

      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 39));
   }
   CHK_EXEC(SetMinorSchemaVersion(50));
   return true;
}

/**
 * Upgrade from 30.48 to 30.49 (changes also included into 22.38)
 */
static bool H_UpgradeFromV48()
{
   if (GetSchemaLevelForMajorVersion(22) < 38)
   {
      CHK_EXEC(CreateEventTemplate(EVENT_SERVER_STARTED, _T("SYS_SERVER_STARTED"), SEVERITY_NORMAL, EF_LOG, _T("32f3305b-1c1b-4597-9eb5-b74eca54330d"),
               _T("Server started"),
               _T("Generated when server initialization is completed.")
               ));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 38));
   }
   CHK_EXEC(SetMinorSchemaVersion(49));
   return true;
}

/**
 * Upgrade from 30.47 to 30.48 (changes also included into 22.36)
 */
static bool H_UpgradeFromV47()
{
   if (GetSchemaLevelForMajorVersion(22) < 36)
   {
      static const TCHAR *batch =
         _T("UPDATE object_properties SET state_before_maint=0 WHERE state_before_maint IS NULL\n")
         _T("UPDATE nodes SET port_rows=0 WHERE port_rows IS NULL\n")
         _T("UPDATE nodes SET port_numbering_scheme=0 WHERE port_numbering_scheme IS NULL\n")
         _T("UPDATE dct_threshold_instances SET row_number=0 WHERE row_number IS NULL\n")
         _T("<END>");
      CHK_EXEC(SQLBatch(batch));

      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("object_properties"), _T("state_before_maint")));
      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("nodes"), _T("port_rows")));
      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("nodes"), _T("port_numbering_scheme")));
      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("dct_threshold_instances"), _T("row_number")));

      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 36));
   }
   CHK_EXEC(SetMinorSchemaVersion(48));
   return true;
}

/**
 * Upgrade from 30.46 to 30.47 (changes also included into 22.35)
 */
static bool H_UpgradeFromV46()
{
   if (GetSchemaLevelForMajorVersion(22) < 35)
   {
      CHK_EXEC(SQLQuery(_T("UPDATE config SET var_name='AgentTunnels.ListenPort',default_value='4703',description='TCP port number to listen on for incoming agent tunnel connections.' WHERE var_name='AgentTunnelListenPort'")));
      CHK_EXEC(CreateConfigParam(_T("AgentTunnels.ListenPort"), _T("4703"), _T("TCP port number to listen on for incoming agent tunnel connections."), NULL, 'I', true, true, false, false));
      CHK_EXEC(CreateConfigParam(_T("Events.Correlation.TopologyBased"), _T("1"), _T("Enable/disable topology based event correlation."), NULL, 'B', true, false, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 35));
   }
   CHK_EXEC(SetMinorSchemaVersion(47));
   return true;
}

/*
 * Upgrade from 30.45 to 30.46
 */
static bool H_UpgradeFromV45()
{
   CHK_EXEC(CreateTable(
      _T("CREATE TABLE auto_bind_target (")
      _T("   object_id integer not null,")
      _T("   bind_filter $SQL:TEXT null,")
      _T("   bind_flag char(1) null,")
      _T("   unbind_flag char(1) null,")
      _T("   PRIMARY KEY(object_id))")));

   CHK_EXEC(CreateTable(
      _T("CREATE TABLE versionable_object (")
      _T("   object_id integer not null,")
      _T("   version integer not null,")
      _T("   PRIMARY KEY(object_id))")));


   DB_STATEMENT stmtAutoBind = DBPrepare(g_dbHandle, _T("INSERT INTO auto_bind_target (object_id,bind_filter,bind_flag,unbind_flag) VALUES (?,?,?,?)"));
   DB_STATEMENT stmtVersion = DBPrepare(g_dbHandle, _T("INSERT INTO versionable_object (object_id,version) VALUES (?,?)"));
   DB_STATEMENT stmtFlags = DBPrepare(g_dbHandle, _T("UPDATE object_properties SET flags=? WHERE object_id=?"));
   //Template table upgrade
   bool success = true;
   DB_RESULT result = SQLSelect(_T("SELECT id,version,apply_filter FROM templates"));
   if (result != NULL)
   {
      if (stmtAutoBind != NULL && stmtVersion != NULL && stmtFlags != NULL)
      {
         int nRows = DBGetNumRows(result);
         for(int i = 0; i < nRows; i++)
         {
            UINT32 id = DBGetFieldULong(result, i, 0);
            //version
            DBBind(stmtVersion, 1, DB_SQLTYPE_INTEGER, id);
            DBBind(stmtVersion, 2, DB_SQLTYPE_INTEGER, DBGetFieldULong(result, i, 1));
            if (!(SQLExecute(stmtVersion)))
            {
               if (!g_ignoreErrors)
               {
                  success = false;
                  break;
               }
            }

            //flags
            UINT32 flags = 0;
            if(success)
            {
               TCHAR query[512];
               _sntprintf(query, 512, _T("SELECT flags FROM object_properties WHERE object_id=%d"), id);
               DB_RESULT flagResult = SQLSelect(query);
               if (flagResult != NULL)
               {
                  flags = DBGetFieldULong(flagResult, 0, 0);
                  DBBind(stmtFlags, 1, DB_SQLTYPE_INTEGER, (flags & ~3)); //remove auto apply flags
                  DBBind(stmtFlags, 2, DB_SQLTYPE_INTEGER, id);
                  if (!(SQLExecute(stmtFlags)))
                  {
                     if (!g_ignoreErrors)
                     {
                        DBFreeResult(flagResult);
                        success = false;
                        break;
                     }
                  }
                  DBFreeResult(flagResult);
               }
               else if (!g_ignoreErrors)
               {
                  success = false;
                  break;
               }
            }

            //autobind
            if(success)
            {
               DBBind(stmtAutoBind, 1, DB_SQLTYPE_INTEGER, id);
               DBBind(stmtAutoBind, 2, DB_SQLTYPE_TEXT, DBGetField(result, i, 2, NULL, 0), DB_BIND_DYNAMIC);
               DBBind(stmtAutoBind, 3, DB_SQLTYPE_VARCHAR, ((flags & 1) ? _T("1") : _T("0")), DB_BIND_STATIC);
               DBBind(stmtAutoBind, 4, DB_SQLTYPE_VARCHAR, ((flags & 2) ? _T("1") : _T("0")), DB_BIND_STATIC);
               if (!(SQLExecute(stmtAutoBind)))
               {
                  if (!g_ignoreErrors)
                  {
                     success = false;
                     break;
                  }
               }
            }
         }
      }
      else if (!g_ignoreErrors)
      {
         success = false;
      }
      DBFreeResult(result);
   }
   else if (!g_ignoreErrors)
      success = false;

   //Template table upgrade
   if(success)
   {
      result = SQLSelect(_T("SELECT id,auto_bind_filter FROM object_containers WHERE object_class=5"));
      if (result != NULL)
      {
         if (stmtAutoBind != NULL && stmtFlags != NULL)
         {
            int nRows = DBGetNumRows(result);
            for(int i = 0; i < nRows; i++)
            {
               UINT32 id = DBGetFieldULong(result, i, 0);
               //flags
               UINT32 flags = 0;
               TCHAR query[512];
               _sntprintf(query, 512, _T("SELECT flags FROM object_properties WHERE object_id=%d"), id);
               DB_RESULT flagResult = DBSelect(g_dbHandle, query);
               if (flagResult != NULL)
               {
                  flags = DBGetFieldULong(flagResult, 0, 0);
                  DBBind(stmtFlags, 1, DB_SQLTYPE_INTEGER, (flags & !3)); //remove auto apply flags
                  DBBind(stmtFlags, 2, DB_SQLTYPE_INTEGER, id);
                  if (!(SQLExecute(stmtFlags)))
                  {
                     if (!g_ignoreErrors)
                     {
                        DBFreeResult(flagResult);
                        success = false;
                        break;
                     }
                  }
                  DBFreeResult(flagResult);
               }
               else if (!g_ignoreErrors)
               {
                  success = false;
                  break;
               }

               //autobind
               if(success)
               {
                  DBBind(stmtAutoBind, 1, DB_SQLTYPE_INTEGER, id);
                  DBBind(stmtAutoBind, 2, DB_SQLTYPE_TEXT, DBGetField(result, i, 1, NULL, 0), DB_BIND_DYNAMIC);
                  DBBind(stmtAutoBind, 3, DB_SQLTYPE_VARCHAR, ((flags & 1) ? _T("1") : _T("0")), DB_BIND_STATIC);
                  DBBind(stmtAutoBind, 4, DB_SQLTYPE_VARCHAR, ((flags & 2) ? _T("1") : _T("0")), DB_BIND_STATIC);
                  if (!(SQLExecute(stmtAutoBind)))
                  {
                     if (!g_ignoreErrors)
                     {
                        success = false;
                        break;
                     }
                  }
               }
            }
         }
         DBFreeResult(result);
      }
      else if (!g_ignoreErrors)
         success = false;
   }

   //Policy table upgrade
   if(success)
   {
      result = SQLSelect(_T("SELECT id,version,deploy_filter FROM ap_common"));
      if (result != NULL)
      {
         if (stmtAutoBind != NULL && stmtVersion != NULL && stmtFlags != NULL)
         {
            int nRows = DBGetNumRows(result);
            for(int i = 0; i < nRows; i++)
            {
               UINT32 id = DBGetFieldULong(result, i, 0);
               //version
               DBBind(stmtVersion, 1, DB_SQLTYPE_INTEGER, id);
               DBBind(stmtVersion, 2, DB_SQLTYPE_INTEGER, DBGetFieldULong(result, i, 1));
               if (!(SQLExecute(stmtVersion)))
               {
                  if (!g_ignoreErrors)
                  {
                     success = false;
                     break;
                  }
               }

               //flags
               UINT32 flags = 0;
               if(success)
               {
                  TCHAR query[512];
                  _sntprintf(query, 512, _T("SELECT flags FROM object_properties WHERE object_id=%d"), id);
                  DB_RESULT flagResult = SQLSelect(query);
                  if (flagResult != NULL)
                  {
                     flags = DBGetFieldULong(flagResult, 0, 0);
                     DBBind(stmtFlags, 1, DB_SQLTYPE_INTEGER, (flags & ~3)); //remove auto apply flags
                     DBBind(stmtFlags, 2, DB_SQLTYPE_INTEGER, id);
                     if (!(SQLExecute(stmtFlags)))
                     {
                        if (!g_ignoreErrors)
                        {
                           DBFreeResult(flagResult);
                           success = false;
                           break;
                        }
                     }
                     DBFreeResult(flagResult);
                  }
                  else if (!g_ignoreErrors)
                  {
                     success = false;
                     break;
                  }
               }

               //autobind
               if(success)
               {
                  DBBind(stmtAutoBind, 1, DB_SQLTYPE_INTEGER, id);
                  DBBind(stmtAutoBind, 2, DB_SQLTYPE_TEXT, DBGetField(result, i, 2, NULL, 0), DB_BIND_DYNAMIC);
                  DBBind(stmtAutoBind, 3, DB_SQLTYPE_VARCHAR, ((flags & 1) ? _T("1") : _T("0")), DB_BIND_STATIC);
                  DBBind(stmtAutoBind, 4, DB_SQLTYPE_VARCHAR, ((flags & 2) ? _T("1") : _T("0")), DB_BIND_STATIC);
                  if (!(SQLExecute(stmtAutoBind)))
                  {
                     if (!g_ignoreErrors)
                     {
                        success = false;
                        break;
                     }
                  }
               }
            }
         }
         else if (!g_ignoreErrors)
         {
            success = false;
         }
         DBFreeResult(result);
      }
      else if (!g_ignoreErrors)
         success = false;
   }

   if(stmtAutoBind != NULL)
      DBFreeStatement(stmtAutoBind);
   if(stmtVersion != NULL)
      DBFreeStatement(stmtVersion);
   if(stmtFlags != NULL)
      DBFreeStatement(stmtFlags);

   if(!success)
      return false;

   CHK_EXEC(DBDropColumn(g_dbHandle, _T("templates"), _T("version")));
   CHK_EXEC(DBDropColumn(g_dbHandle, _T("templates"), _T("apply_filter")));
   CHK_EXEC(DBDropColumn(g_dbHandle, _T("ap_common"), _T("version")));
   CHK_EXEC(DBDropColumn(g_dbHandle, _T("ap_common"), _T("deploy_filter")));
   CHK_EXEC(DBDropColumn(g_dbHandle, _T("object_containers"), _T("auto_bind_filter")));

   CHK_EXEC(SetMinorSchemaVersion(46));
   return true;
}

/**
 * Upgrade from 30.44 to 30.45 (changes also included into 22.34)
 */
static bool H_UpgradeFromV44()
{
   if (GetSchemaLevelForMajorVersion(22) < 34)
   {
      static const TCHAR *batch =
         _T("ALTER TABLE event_log ADD raw_data $SQL:TEXT\n")
         _T("ALTER TABLE scheduled_tasks ADD task_key varchar(255)\n")
         _T("<END>");
      CHK_EXEC(SQLBatch(batch));

      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 34));
   }
   CHK_EXEC(SetMinorSchemaVersion(45));
   return true;
}

/**
 * Upgrade from 30.43 to 30.44 (changes also included into 22.33)
 */
static bool H_UpgradeFromV43()
{
   if (GetSchemaLevelForMajorVersion(22) < 33)
   {
      static const TCHAR *batch =
         _T("ALTER TABLE policy_action_list ADD timer_delay integer\n")
         _T("ALTER TABLE policy_action_list ADD timer_key varchar(127)\n")
         _T("UPDATE policy_action_list SET timer_delay=0\n")
         _T("<END>");
      CHK_EXEC(SQLBatch(batch));
      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("policy_action_list"), _T("timer_delay")));

      CHK_EXEC(CreateTable(
         _T("CREATE TABLE policy_timer_cancellation_list (")
         _T("   rule_id integer not null,")
         _T("   timer_key varchar(127) not null,")
         _T("   PRIMARY KEY(rule_id,timer_key))")));

      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 33));
   }

   CHK_EXEC(SetMinorSchemaVersion(44));
   return true;
}

/**
 * Upgrade from 30.42 to 30.43 (changes also included into 22.32)
 */
static bool H_UpgradeFromV42()
{
   if (GetSchemaLevelForMajorVersion(22) < 32)
   {
      CHK_EXEC(CreateLibraryScript(17, _T("ee6dd107-982b-4ad1-980b-fc0cc7a03911"), _T("Hook::DiscoveryPoll"),
               _T("/* Available global variables:\r\n *  $node - current node, object of 'Node' type\r\n *\r\n * Expected return value:\r\n *  none - returned value is ignored\r\n */\r\n")));
      CHK_EXEC(CreateLibraryScript(18, _T("a02ea666-e1e9-4f98-a746-1c3ce19428e9"), _T("Hook::PostObjectCreate"),
               _T("/* Available global variables:\r\n *  $object - current object, one of 'NetObj' subclasses\r\n *  $node - current object if it is 'Node' class\r\n *\r\n * Expected return value:\r\n *  none - returned value is ignored\r\n */\r\n")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 32));
   }
   CHK_EXEC(SetMinorSchemaVersion(43));
   return true;
}

/**
 * Upgrade from 30.41 to 30.42 (changes also included into 22.31)
 */
static bool H_UpgradeFromV41()
{
   if (GetSchemaLevelForMajorVersion(22) < 31)
   {
      CHK_EXEC(CreateTable(
         _T("CREATE TABLE interface_vlan_list (")
         _T("   iface_id integer not null,")
         _T("   vlan_id integer not null,")
         _T("   PRIMARY KEY(iface_id,vlan_id))")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 31));
   }

   CHK_EXEC(SetMinorSchemaVersion(42));
   return true;
}

/**
 * Upgrade from 30.40 to 30.41 (changes also included into 22.30)
 */
static bool H_UpgradeFromV40()
{
   if (GetSchemaLevelForMajorVersion(22) < 30)
   {
      CHK_EXEC(SQLQuery(_T("ALTER TABLE nodes ADD hypervisor_type varchar(31)")));
      CHK_EXEC(SQLQuery(_T("ALTER TABLE nodes ADD hypervisor_info varchar(255)")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 30));
   }
   CHK_EXEC(SetMinorSchemaVersion(41));
   return true;
}

/**
 * Upgrade from 30.39 to 30.40 (changes also included into 22.29)
 */
static bool H_UpgradeFromV39()
{
   if (GetSchemaLevelForMajorVersion(22) < 29)
   {
      DB_STATEMENT hStmt = DBPrepare(g_dbHandle, _T("UPDATE event_cfg SET description=? WHERE event_code=?"));
      if (hStmt != NULL)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_TEXT,
                  _T("Generated when agent tunnel is open and bound.\r\n")
                  _T("Parameters:\r\n")
                  _T("   1) Tunnel ID (tunnelId)\r\n")
                  _T("   2) Remote system IP address (ipAddress)\r\n")
                  _T("   3) Remote system name (systemName)\r\n")
                  _T("   4) Remote system FQDN (hostName)\r\n")
                  _T("   5) Remote system platform (platformName)\r\n")
                  _T("   6) Remote system information (systemInfo)\r\n")
                  _T("   7) Agent version (agentVersion)\r\n")
                  _T("   8) Agent ID (agentId)"),
                  DB_BIND_STATIC);
         DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, EVENT_TUNNEL_OPEN);
         bool success = DBExecute(hStmt);
         if (!success && !g_ignoreErrors)
         {
            DBFreeStatement(hStmt);
            return false;
         }

         DBBind(hStmt, 1, DB_SQLTYPE_TEXT,
                  _T("Generated when agent tunnel is closed.\r\n")
                  _T("Parameters:\r\n")
                  _T("   1) Tunnel ID (tunnelId)\r\n")
                  _T("   2) Remote system IP address (ipAddress)\r\n")
                  _T("   3) Remote system name (systemName)\r\n")
                  _T("   4) Remote system FQDN (hostName)\r\n")
                  _T("   5) Remote system platform (platformName)\r\n")
                  _T("   6) Remote system information (systemInfo)\r\n")
                  _T("   7) Agent version (agentVersion)\r\n")
                  _T("   8) Agent ID (agentId)"),
                  DB_BIND_STATIC);
         DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, EVENT_TUNNEL_CLOSED);
         success = DBExecute(hStmt);
         if (!success && !g_ignoreErrors)
         {
            DBFreeStatement(hStmt);
            return false;
         }

         DBBind(hStmt, 1, DB_SQLTYPE_TEXT,
                  _T("Generated when unbound agent tunnel is not bound or closed for more than configured threshold.\r\n")
                  _T("Parameters:\r\n")
                  _T("   1) Tunnel ID (tunnelId)\r\n")
                  _T("   2) Remote system IP address (ipAddress)\r\n")
                  _T("   3) Remote system name (systemName)\r\n")
                  _T("   4) Remote system FQDN (hostName)\r\n")
                  _T("   5) Remote system platform (platformName)\r\n")
                  _T("   6) Remote system information (systemInfo)\r\n")
                  _T("   7) Agent version (agentVersion)\r\n")
                  _T("   8) Agent ID (agentId)\r\n")
                  _T("   9) Configured idle timeout (idleTimeout)"),
                  DB_BIND_STATIC);
         DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, EVENT_UNBOUND_TUNNEL);
         success = DBExecute(hStmt);
         if (!success && !g_ignoreErrors)
         {
            DBFreeStatement(hStmt);
            return false;
         }

         DBFreeStatement(hStmt);
      }
      else
      {
         if (!g_ignoreErrors)
            return false;
      }

      CHK_EXEC(CreateEventTemplate(EVENT_TUNNEL_AGENT_ID_MISMATCH, _T("SYS_TUNNEL_AGENT_ID_MISMATCH"), SEVERITY_WARNING, EF_LOG, _T("30792e3d-f94a-4866-9616-457ba3ac276a"),
               _T("Agent ID %<nodeAgentId> on node do not match agent ID %<tunnelAgentId> on tunnel from %<systemName> (%<ipAddress>) at bind"),
               _T("Generated when agent ID mismatch detected during tunnel bind.\r\n")
               _T("Parameters:\r\n")
               _T("   1) Tunnel ID (tunnelId)\r\n")
               _T("   2) Remote system IP address (ipAddress)\r\n")
               _T("   3) Remote system name (systemName)\r\n")
               _T("   4) Remote system FQDN (hostName)\r\n")
               _T("   5) Remote system platform (platformName)\r\n")
               _T("   6) Remote system information (systemInfo)\r\n")
               _T("   7) Agent version (agentVersion)\r\n")
               _T("   8) Tunnel agent ID (tunnelAgentId)\r\n")
               _T("   9) Node agent ID (nodeAgentId)")
               ));

      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 29));
   }
   CHK_EXEC(SetMinorSchemaVersion(40));
   return true;
}

/**
 * Upgrade from 30.38 to 30.39 (changes also included into 22.28)
 */
static bool H_UpgradeFromV38()
{
   if (GetSchemaLevelForMajorVersion(22) < 28)
   {
      CHK_EXEC(CreateEventTemplate(EVENT_TUNNEL_OPEN, _T("SYS_TUNNEL_OPEN"), SEVERITY_NORMAL, EF_LOG, _T("2569c729-1f8c-4a13-9e75-a1d0c1995bc2"),
               _T("Agent tunnel from %<systemName> (%<ipAddress>) is open"),
               _T("Generated when agent tunnel is open and bound.\r\n")
               _T("Parameters:\r\n")
               _T("   1) Tunnel ID (tunnelId)\r\n")
               _T("   2) Remote system IP address (ipAddress)\r\n")
               _T("   3) Remote system name (systemName)\r\n")
               _T("   4) Remote system FQDN (hostName)\r\n")
               _T("   5) Remote system platform (platformName)\r\n")
               _T("   6) Remote system information (systemInfo)\r\n")
               _T("   7) Agent version (agentVersion)")
               ));
      CHK_EXEC(CreateEventTemplate(EVENT_TUNNEL_CLOSED, _T("SYS_TUNNEL_CLOSED"), SEVERITY_WARNING, EF_LOG, _T("50a61266-710d-48d7-b620-9eaa0f85a94f"),
               _T("Agent tunnel from %<systemName> (%<ipAddress>) is closed"),
               _T("Generated when agent tunnel is closed.\r\n")
               _T("Parameters:\r\n")
               _T("   1) Tunnel ID (tunnelId)\r\n")
               _T("   2) Remote system IP address (ipAddress)\r\n")
               _T("   3) Remote system name (systemName)\r\n")
               _T("   4) Remote system FQDN (hostName)\r\n")
               _T("   5) Remote system platform (platformName)\r\n")
               _T("   6) Remote system information (systemInfo)\r\n")
               _T("   7) Agent version (agentVersion)")
               ));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 28));
   }
   CHK_EXEC(SetMinorSchemaVersion(39));
   return true;
}

/**
 * Upgrade from 30.37 to 30.38 (changes also included into 22.27)
 */
static bool H_UpgradeFromV37()
{
   if (GetSchemaLevelForMajorVersion(22) < 27)
   {
      CHK_EXEC(SQLQuery(_T("UPDATE config SET var_name='Client.AlarmList.DisplayLimit' WHERE var_name='AlarmListDisplayLimit'")));
      CHK_EXEC(CreateConfigParam(_T("Client.ObjectBrowser.AutoApplyFilter"), _T("1"), _T("Enable or disable object browser's filter applying as user types (if disabled, user has to press ENTER to apply filter)."), NULL, 'B', true, false, false, false));
      CHK_EXEC(CreateConfigParam(_T("Client.ObjectBrowser.FilterDelay"), _T("300"), _T("Delay between typing in object browser''s filter and applying it to object tree."), _T("milliseconds"), 'I', true, false, false, false));
      CHK_EXEC(CreateConfigParam(_T("Client.ObjectBrowser.MinFilterStringLength"), _T("1"), _T("Minimal length of filter string in object browser required for automatic apply."), _T("characters"), 'I', true, false, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 27));
   }
   else
   {
      static const TCHAR *batch =
         _T("UPDATE config SET units='milliseconds' WHERE var_name='Client.ObjectBrowser.FilterDelay'\n")
         _T("UPDATE config SET units='characters' WHERE var_name='Client.ObjectBrowser.MinFilterStringLength'\n")
         _T("<END>");
      CHK_EXEC(SQLBatch(batch));
   }
   CHK_EXEC(SetMinorSchemaVersion(38));
   return true;
}

/**
 * Upgrade from 30.36 to 30.37 (changes also included into 22.26)
 */
static bool H_UpgradeFromV36()
{
   if (GetSchemaLevelForMajorVersion(22) < 26)
   {
      CHK_EXEC(CreateTable(
         _T("CREATE TABLE certificate_action_log (")
         _T("   record_id integer not null,")
         _T("   timestamp integer not null,")
         _T("   operation integer not null,")
         _T("   user_id integer not null,")
         _T("   node_id integer not null,")
         _T("   node_guid varchar(36) null,")
         _T("   cert_type integer not null,")
         _T("   subject varchar(255) null,")
         _T("   serial integer null,")
         _T("   PRIMARY KEY(record_id))")));

      CHK_EXEC(SQLQuery(_T("ALTER TABLE nodes ADD agent_id varchar(36)")));
      CHK_EXEC(SQLQuery(_T("ALTER TABLE nodes ADD agent_cert_subject varchar(500)")));

      CHK_EXEC(CreateEventTemplate(EVENT_AGENT_ID_CHANGED, _T("SYS_AGENT_ID_CHANGED"), SEVERITY_WARNING, EF_LOG,
               _T("741f0abc-1e69-46e4-adbc-bf1c4ed8549a"),
               _T("Agent ID changed from %1 to %2"),
               _T("Generated when agent ID change detected.\r\n")
               _T("Parameters:\r\n")
               _T("   1) Old agent ID\r\n")
               _T("   2) New agent ID")
               ));

      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 26));
   }
   CHK_EXEC(SetMinorSchemaVersion(37));
   return true;
}

/**
 * Upgrade from 30.35 to 30.36 (changes also included into 22.25)
 */
static bool H_UpgradeFromV35()
{
   if (GetSchemaLevelForMajorVersion(22) < 25)
   {
      CHK_EXEC(SQLQuery(_T("ALTER TABLE actions ADD guid varchar(36)")));
      CHK_EXEC(GenerateGUID(_T("actions"), _T("action_id"), _T("guid"), NULL));
      DBSetNotNullConstraint(g_dbHandle, _T("actions"), _T("guid"));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 25));
   }
   CHK_EXEC(SetMinorSchemaVersion(36));
   return true;
}

/**
 * Upgrade from 30.34 to 30.35 (changes also included into 22.24)
 */
static bool H_UpgradeFromV34()
{
   if (GetSchemaLevelForMajorVersion(22) < 24)
   {
      CHK_EXEC(DBRenameColumn(g_dbHandle, _T("dct_threshold_instances"), _T("row"), _T("row_number")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 24));
   }
   CHK_EXEC(SetMinorSchemaVersion(35));
   return true;
}

/**
 * Upgrade from 30.33 to 30.35 (changes also included into 22.23)
 */
static bool H_UpgradeFromV33()
{
   if (GetSchemaLevelForMajorVersion(22) < 23)
   {
      static const TCHAR *batch =
         _T("ALTER TABLE object_properties ADD state_before_maint integer\n")
         _T("ALTER TABLE dct_threshold_instances ADD row_number integer\n")
         _T("ALTER TABLE dct_threshold_instances ADD maint_copy char(1)\n")
         _T("ALTER TABLE thresholds ADD state_before_maint char(1)\n")
         _T("<END>");
      CHK_EXEC(SQLBatch(batch));

      CHK_EXEC(DBDropColumn(g_dbHandle, _T("object_properties"), _T("maint_mode")));
      CHK_EXEC(SQLQuery(_T("UPDATE dct_threshold_instances SET maint_copy='0'")));
      CHK_EXEC(DBDropPrimaryKey(g_dbHandle, _T("dct_threshold_instances")));
      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("dct_threshold_instances"), _T("maint_copy")));
      CHK_EXEC(DBAddPrimaryKey(g_dbHandle, _T("dct_threshold_instances"), _T("threshold_id,instance,maint_copy")));

      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 24));
   }
   CHK_EXEC(SetMinorSchemaVersion(35));
   return true;
}

/**
 * Upgrade from 30.32 to 30.33 (changes also included into 22.22)
 */
static bool H_UpgradeFromV32()
{
   if (GetSchemaLevelForMajorVersion(22) < 22)
   {
      CHK_EXEC(CreateConfigParam(_T("Alarms.IgnoreHelpdeskState"), _T("0"), _T("If set alarm helpdesk state will be ignored when resolving or terminating."), NULL, 'B', true, false, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 22));
   }
   CHK_EXEC(SetMinorSchemaVersion(33));
   return true;
}

/**
 * Upgrade from 30.31 to 30.32 (changes also included into 22.21)
 */
static bool H_UpgradeFromV31()
{
   if (GetSchemaLevelForMajorVersion(22) < 21)
   {
      static const TCHAR *batch =
         _T("ALTER TABLE alarms ADD zone_uin integer\n")
         _T("ALTER TABLE event_log ADD zone_uin integer\n")
         _T("ALTER TABLE snmp_trap_log ADD zone_uin integer\n")
         _T("ALTER TABLE syslog ADD zone_uin integer\n")
         _T("<END>");
      CHK_EXEC(SQLBatch(batch));

      RegisterOnlineUpgrade(22, 21);

      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 21));
   }
   CHK_EXEC(SetMinorSchemaVersion(32));
   return true;
}

/**
 * Upgrade from 30.30 to 30.31 (changes also included into 22.20)
 */
static bool H_UpgradeFromV30()
{
   if (GetSchemaLevelForMajorVersion(22) < 20)
   {
      CHK_EXEC(SQLQuery(_T("UPDATE config_values SET var_description='Don''t use aliases' WHERE var_name='UseInterfaceAliases' AND var_value='0'")));
      CHK_EXEC(SQLQuery(_T("UPDATE config SET description='Resolve node name using DNS, SNMP system name, or host name if current node name is it''s IP address.' WHERE var_name='ResolveNodeNames'")));
      CHK_EXEC(SQLQuery(_T("UPDATE config SET description='Number of hops from seed node to be added to topology map.' WHERE var_name='TopologyDiscoveryRadius'")));
      CHK_EXEC(SQLQuery(_T("UPDATE config SET description='Network topology information expiration time. Server will use cached topology information if it is newer than given interval.' WHERE var_name='TopologyExpirationTime'")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 20));
   }
   CHK_EXEC(SetMinorSchemaVersion(31));
   return true;
}

/**
 * Upgrade from 30.29 to 30.30 (changes also included into 22.19)
 */
static bool H_UpgradeFromV29()
{
   if (GetSchemaLevelForMajorVersion(22) < 19)
   {
      CHK_EXEC(CreateConfigParam(_T("ThreadPool.Syncer.BaseSize"), _T("1"), _T("Base size for syncer thread pool."), NULL, 'I', true, true, false, false));
      CHK_EXEC(CreateConfigParam(_T("ThreadPool.Syncer.MaxSize"), _T("1"), _T("Maximum size for syncer thread pool (value of 1 will disable pool creation)."), NULL, 'I', true, true, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 19));
   }
   CHK_EXEC(SQLQuery(_T("UPDATE config SET units='' WHERE var_name like 'ThreadPool.%'")));
   CHK_EXEC(SetMinorSchemaVersion(30));
   return true;
}

/**
 * Upgrade from 30.28 to 30.29 (changes also included into 22.18)
 */
static bool H_UpgradeFromV28()
{
   if (GetSchemaLevelForMajorVersion(22) < 18)
   {
      CHK_EXEC(CreateConfigParam(_T("DBWriter.RawDataFlushInterval"), _T("30"), _T("Interval between writes of accumulated raw DCI data to database."), _T("seconds"), 'I', true, true, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 18));
   }
   CHK_EXEC(SQLQuery(_T("UPDATE config SET units='seconds' WHERE var_name='DBWriter.RawDataFlushInterval'")));
   CHK_EXEC(SetMinorSchemaVersion(29));
   return true;
}

/**
 * Upgrade from 30.27 to 30.28 (changes also included into 22.17)
 */
static bool H_UpgradeFromV27()
{
   if (GetSchemaLevelForMajorVersion(22) < 17)
   {
      CHK_EXEC(CreateConfigParam(_T("DBWriter.DataQueues"), _T("1"), _T("Number of queues for DCI data writer."), NULL, 'I', true, true, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 17));
   }
   CHK_EXEC(SetMinorSchemaVersion(28));
   return true;
}

/**
 * Upgrade from 30.26 to 30.27
 */
static bool H_UpgradeFromV26()
{
   CHK_EXEC(CreateTable(
         _T("CREATE TABLE responsible_users (")
         _T("   object_id integer not null,")
         _T("   user_id integer not null,")
         _T("   PRIMARY KEY(object_id,user_id))")));
   CHK_EXEC(SetMinorSchemaVersion(27));
   return true;
}

/**
 * Upgrade from 30.25 to 30.26 (changes also included into 22.16)
 */
static bool H_UpgradeFromV25()
{
   if (GetSchemaLevelForMajorVersion(22) < 16)
   {
      CHK_EXEC(CreateConfigParam(_T("DataCollection.ScriptErrorReportInterval"), _T("86400"), _T("Minimal interval between reporting errors in data collection related script."), _T("seconds"), 'I', true, false, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 16));
   }
   CHK_EXEC(SQLQuery(_T("UPDATE config SET units='seconds' WHERE var_name='DataCollection.ScriptErrorReportInterval'")));
   CHK_EXEC(SetMinorSchemaVersion(26));
   return true;
}

/**
 * Upgrade from 30.24 to 30.25 (changes also included into 22.15)
 */
static bool H_UpgradeFromV24()
{
   if (GetSchemaLevelForMajorVersion(22) < 15)
   {
      CHK_EXEC(CreateConfigParam(_T("NXSL.EnableFileIOFunctions"), _T("0"), _T("Enable/disable server-side NXSL functions for file I/O (such as OpenFile, DeleteFile, etc.)."), NULL, 'B', true, true, false, false));
      CHK_EXEC(SQLQuery(_T("UPDATE config SET var_name='NXSL.EnableContainerFunctions',description='Enable/disable server-side NXSL functions for container management (such as CreateContainer, RemoveContainer, BindObject, UnbindObject).' WHERE var_name='EnableNXSLContainerFunctions'")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 15));
   }
   CHK_EXEC(SetMinorSchemaVersion(25));
   return true;
}

/**
 * Upgrade from 30.23 to 30.24 (changes also included into 22.14)
 */
static bool H_UpgradeFromV23()
{
   if (GetSchemaLevelForMajorVersion(22) < 14)
   {
      CHK_EXEC(CreateConfigParam(_T("DataCollection.OnDCIDelete.TerminateRelatedAlarms"), _T("1"), _T("Enable/disable automatic termination of related alarms when data collection item is deleted."), NULL, 'B', true, false, false, false));
      CHK_EXEC(SQLQuery(_T("UPDATE config SET data_type='I',description='Inactivity time after which user account will be blocked (0 to disable blocking).' WHERE var_name='BlockInactiveUserAccounts'")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 14));
   }
   CHK_EXEC(SQLQuery(_T("UPDATE config SET units='days' WHERE var_name='BlockInactiveUserAccounts'")));
   CHK_EXEC(SetMinorSchemaVersion(24));
   return true;
}

/**
 * Upgrade from 30.22 to 30.23 (changes also included into 22.13)
 */
static bool H_UpgradeFromV22()
{
   if (GetSchemaLevelForMajorVersion(22) < 13)
   {
      CHK_EXEC(CreateEventTemplate(EVENT_UNBOUND_TUNNEL, _T("SYS_UNBOUND_TUNNEL"), SEVERITY_NORMAL, EF_LOG, _T("7f781ec2-a8f5-4c02-ad7f-9e5b0a223b87"),
               _T("Unbound agent tunnel from %<systemName> (%<ipAddress>) is idle for more than %<idleTimeout> seconds"),
               _T("Generated when unbound agent tunnel is not bound or closed for more than configured threshold.\r\n")
               _T("Parameters:\r\n")
               _T("   1) Tunnel ID (tunnelId)\r\n")
               _T("   2) Remote system IP address (ipAddress)\r\n")
               _T("   3) Remote system name (systemName)\r\n")
               _T("   4) Remote system FQDN (hostName)\r\n")
               _T("   5) Remote system platform (platformName)\r\n")
               _T("   6) Remote system information (systemInfo)\r\n")
               _T("   7) Agent version (agentVersion)\r\n")
               _T("   8) Configured idle timeout (idleTimeout)")
               ));

      CHK_EXEC(CreateConfigParam(_T("AgentTunnels.NewNodesContainer"), _T(""), _T("Name of the container where nodes created automatically for unbound tunnels will be placed. If empty or missing, such nodes will be created in infrastructure services root."), NULL, 'S', true, false, false, false));
      CHK_EXEC(CreateConfigParam(_T("AgentTunnels.UnboundTunnelTimeout"), _T("3600"), _T("Unbound agent tunnels inactivity timeout. If tunnel is not bound or closed after timeout, action defined by AgentTunnels.UnboundTunnelTimeoutAction parameter will be taken."), _T("seconds"), 'I', true, false, false, false));
      CHK_EXEC(CreateConfigParam(_T("AgentTunnels.UnboundTunnelTimeoutAction"), _T("0"), _T("Action to be taken when unbound agent tunnel idle timeout expires."), NULL, 'C', true, false, false, false));

      static TCHAR batch[] =
         _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('AgentTunnels.UnboundTunnelTimeoutAction','0','Reset tunnel')\n")
         _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('AgentTunnels.UnboundTunnelTimeoutAction','1','Generate event')\n")
         _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('AgentTunnels.UnboundTunnelTimeoutAction','2','Bind tunnel to existing node')\n")
         _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('AgentTunnels.UnboundTunnelTimeoutAction','3','Bind tunnel to existing node or create new node')\n")
         _T("<END>");
      CHK_EXEC(SQLBatch(batch));

      CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET description='Generated when new node object added to the database.\r\nParameters:\r\n   1) Node origin (0 = created manually, 1 = created by network discovery, 2 = created by tunnel auto bind)' WHERE event_code=1")));

      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 13));
   }
   CHK_EXEC(SetMinorSchemaVersion(23));
   return true;
}

/**
 * Upgrade from 30.21 to 30.22
 */
static bool H_UpgradeFromV21()
{
   static TCHAR batch[] =
      _T("ALTER TABLE config ADD units varchar(36)\n")
      _T("UPDATE config SET units='milliseconds' WHERE var_name='AgentCommandTimeout'\n")
      _T("UPDATE config SET units='seconds' WHERE var_name='AgentTunnels.UnboundTunnelTimeout'\n")
      _T("UPDATE config SET units='seconds' WHERE var_name='AgentUpgradeWaitTime'\n")
      _T("UPDATE config SET units='seconds' WHERE var_name='AlarmHistoryRetentionTime'\n")
      _T("UPDATE config SET units='alarms' WHERE var_name='AlarmListDisplayLimit'\n")
      _T("UPDATE config SET units='days' WHERE var_name='AuditLogRetentionTime'\n")
      _T("UPDATE config SET units='milliseconds' WHERE var_name='BeaconPollingInterval'\n")
      _T("UPDATE config SET units='milliseconds' WHERE var_name='BeaconTimeout'\n")
      _T("UPDATE config SET units='seconds' WHERE var_name='CapabilityExpirationTime'\n")
      _T("UPDATE config SET units='seconds' WHERE var_name='ConditionPollingInterval'\n")
      _T("UPDATE config SET units='seconds' WHERE var_name='ConfigurationPollingInterval'\n")
      _T("UPDATE config SET units='connections' WHERE var_name='DBConnectionPoolBaseSize'\n")
      _T("UPDATE config SET units='seconds' WHERE var_name='DBConnectionPoolCooldownTime'\n")
      _T("UPDATE config SET units='seconds' WHERE var_name='DBConnectionPoolMaxLifetime'\n")
      _T("UPDATE config SET units='connections' WHERE var_name='DBConnectionPoolMaxSize'\n")
      _T("UPDATE config SET units='records/transaction' WHERE var_name='DBWriter.MaxRecordsPerTransaction'\n")
      _T("UPDATE config SET units='seconds' WHERE var_name='DefaultDCIPollingInterval'\n")
      _T("UPDATE config SET units='days' WHERE var_name='DefaultDCIRetentionTime'\n")
      _T("UPDATE config SET units='days' WHERE var_name='DeleteUnreachableNodesPeriod'\n")
      _T("UPDATE config SET units='seconds' WHERE var_name='DiscoveryPollingInterval'\n")
      _T("UPDATE config SET units='days' WHERE var_name='EventLogRetentionTime'\n")
      _T("UPDATE config SET units='events/second' WHERE var_name='EventStormEventsPerSecond'\n")
      _T("UPDATE config SET units='logins' WHERE var_name='GraceLoginCount'\n")
      _T("UPDATE config SET units='size' WHERE var_name='IcmpPingSize'\n")
      _T("UPDATE config SET units='milliseconds' WHERE var_name='IcmpPingTimeout'\n")
      _T("UPDATE config SET units='seconds' WHERE var_name='InstancePollingInterval'\n")
      _T("UPDATE config SET units='days' WHERE var_name='InstanceRetentionTime'\n")
      _T("UPDATE config SET units='seconds' WHERE var_name='IntruderLockoutTime'\n")
      _T("UPDATE config SET units='days' WHERE var_name='JobHistoryRetentionTime'\n")
      _T("UPDATE config SET units='retries' WHERE var_name='JobRetryCount'\n")
      _T("UPDATE config SET units='seconds' WHERE var_name='KeepAliveInterval'\n")
      _T("UPDATE config SET units='milliseconds' WHERE var_name='LongRunningQueryThreshold'\n")
      _T("UPDATE config SET units='characters' WHERE var_name='MinPasswordLength'\n")
      _T("UPDATE config SET units='seconds' WHERE var_name='MinViewRefreshInterval'\n")
      _T("UPDATE config SET units='threads' WHERE var_name='NumberOfUpgradeThreads'\n")
      _T("UPDATE config SET units='seconds' WHERE var_name='OfflineDataRelevanceTime'\n")
      _T("UPDATE config SET units='polls' WHERE var_name='PollCountForStatusChange'\n")
      _T("UPDATE config SET units='retries' WHERE var_name='RADIUSNumRetries'\n")
      _T("UPDATE config SET units='seconds' WHERE var_name='RADIUSTimeout'\n")
      _T("UPDATE config SET units='seconds' WHERE var_name='RoutingTableUpdateInterval'\n")
      _T("UPDATE config SET units='seconds' WHERE var_name='ServerCommandOutputTimeout'\n")
      _T("UPDATE config SET units='seconds' WHERE var_name='StatusPollingInterval'\n")
      _T("UPDATE config SET units='seconds' WHERE var_name='SyncInterval'\n")
      _T("UPDATE config SET units='days' WHERE var_name='SyslogRetentionTime'\n")
      _T("UPDATE config SET units='size' WHERE var_name='ThreadPool.Agent.BaseSize'\n")
      _T("UPDATE config SET units='size' WHERE var_name='ThreadPool.Agent.MaxSize'\n")
      _T("UPDATE config SET units='size' WHERE var_name='ThreadPool.DataCollector.BaseSize'\n")
      _T("UPDATE config SET units='size' WHERE var_name='ThreadPool.DataCollector.MaxSize'\n")
      _T("UPDATE config SET units='size' WHERE var_name='ThreadPool.Main.BaseSize'\n")
      _T("UPDATE config SET units='size' WHERE var_name='ThreadPool.Main.MaxSize'\n")
      _T("UPDATE config SET units='size' WHERE var_name='ThreadPool.Poller.BaseSize'\n")
      _T("UPDATE config SET units='size' WHERE var_name='ThreadPool.Poller.MaxSize'\n")
      _T("UPDATE config SET units='size' WHERE var_name='ThreadPool.Scheduler.BaseSize'\n")
      _T("UPDATE config SET units='size' WHERE var_name='ThreadPool.Scheduler.MaxSize'\n")
      _T("UPDATE config SET units='seconds' WHERE var_name='ThresholdRepeatInterval'\n")
      _T("UPDATE config SET units='seconds' WHERE var_name='TopologyExpirationTime'\n")
      _T("UPDATE config SET units='seconds' WHERE var_name='TopologyPollingInterval'\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   CHK_EXEC(SetMinorSchemaVersion(22));
   return true;
}

/**
 * Upgrade from 30.20 to 30.21 (changes also included into 22.12)
 */
static bool H_UpgradeFromV20()
{
   if (GetSchemaLevelForMajorVersion(22) < 12)
   {
      CHK_EXEC(SQLQuery(_T("ALTER TABLE ap_common ADD deploy_filter $SQL:TEXT")));

      CHK_EXEC(CreateEventTemplate(EVENT_POLICY_AUTODEPLOY, _T("SYS_POLICY_AUTODEPLOY"), SEVERITY_NORMAL, EF_LOG, _T("f26d70b3-d48d-472c-b2ec-bfa7c20958ea"),
               _T("Agent policy %4 automatically deployed to node %2"),
               _T("Generated when agent policy deployed to node by autodeploy rule.\r\n")
               _T("Parameters:\r\n")
               _T("   1) Node ID\r\n")
               _T("   2) Node name\r\n")
               _T("   3) Policy ID\r\n")
               _T("   4) Policy name")
               ));

      CHK_EXEC(CreateEventTemplate(EVENT_POLICY_AUTOUNINSTALL, _T("SYS_POLICY_AUTOUNINSTALL"), SEVERITY_NORMAL, EF_LOG, _T("2fbac886-2cfa-489f-bef1-364a38fa7062"),
               _T("Agent policy %4 automatically uninstalled from node %2"),
               _T("Generated when agent policy uninstalled from node by autodeploy rule.\r\n")
               _T("Parameters:\r\n")
               _T("   1) Node ID\r\n")
               _T("   2) Node name\r\n")
               _T("   3) Policy ID\r\n")
               _T("   4) Policy name")
               ));

      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 12));
   }
   CHK_EXEC(SetMinorSchemaVersion(21));
   return true;
}

/**
 * Upgrade from 30.19 to 30.20 (changes also included into 22.11)
 */
static bool H_UpgradeFromV19()
{
   if (GetSchemaLevelForMajorVersion(22) < 11)
   {
      CHK_EXEC(SQLQuery(_T("UPDATE config SET var_name='Housekeeper.StartTime' WHERE var_name='HousekeeperStartTime'")));
      CHK_EXEC(CreateConfigParam(_T("Housekeeper.Throttle.HighWatermark"), _T("250000"), _T("High watermark for housekeeper throttling"), NULL, 'I', true, false, false, false));
      CHK_EXEC(CreateConfigParam(_T("Housekeeper.Throttle.LowWatermark"), _T("50000"), _T("Low watermark for housekeeper throttling"), NULL, 'I', true, false, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 11));
   }
   CHK_EXEC(SetMinorSchemaVersion(20));
   return true;
}

/**
 * Upgrade from 30.18 to 30.19 (changes also included into 22.10)
 */
static bool H_UpgradeFromV18()
{
   if (GetSchemaLevelForMajorVersion(22) < 10)
   {
      static TCHAR batch[] =
         _T("UPDATE config SET var_name='ThreadPool.DataCollector.BaseSize' WHERE var_name='DataCollector.ThreadPool.BaseSize'\n")
         _T("UPDATE config SET var_name='ThreadPool.DataCollector.MaxSize' WHERE var_name='DataCollector.ThreadPool.MaxSize'\n")
         _T("UPDATE config SET var_name='ThreadPool.Poller.BaseSize',description='Base size for poller thread pool' WHERE var_name='PollerThreadPoolBaseSize'\n")
         _T("UPDATE config SET var_name='ThreadPool.Poller.MaxSize',description='Maximum size for poller thread pool' WHERE var_name='PollerThreadPoolMaxSize'\n")
         _T("<END>");
      CHK_EXEC(SQLBatch(batch));
      CHK_EXEC(CreateConfigParam(_T("ThreadPool.Agent.BaseSize"), _T("4"), _T("Base size for agent connector thread pool"), NULL, 'I', true, true, false, false));
      CHK_EXEC(CreateConfigParam(_T("ThreadPool.Agent.MaxSize"), _T("256"), _T("Maximum size for agent connector thread pool"), NULL, 'I', true, true, false, false));
      CHK_EXEC(CreateConfigParam(_T("ThreadPool.Main.BaseSize"), _T("8"), _T("Base size for main server thread pool"), NULL, 'I', true, true, false, false));
      CHK_EXEC(CreateConfigParam(_T("ThreadPool.Main.MaxSize"), _T("256"), _T("Maximum size for main server thread pool"), NULL, 'I', true, true, false, false));
      CHK_EXEC(CreateConfigParam(_T("ThreadPool.Scheduler.BaseSize"), _T("1"), _T("Base size for scheduler thread pool"), NULL, 'I', true, true, false, false));
      CHK_EXEC(CreateConfigParam(_T("ThreadPool.Scheduler.MaxSize"), _T("64"), _T("Maximum size for scheduler thread pool"), NULL, 'I', true, true, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(22, 10));
   }
   CHK_EXEC(SetMinorSchemaVersion(19));
   return true;
}

/**
 * Upgrade from 30.17 to 30.18 (changes also included into 22.9)
 */
static bool H_UpgradeFromV17()
{
   if (GetSchemaLevelForMajorVersion(22) < 9)
   {
      CHK_EXEC(DBResizeColumn(g_dbHandle, _T("nodes"), _T("lldp_id"), 255, true));
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
      CHK_EXEC(DBRenameColumn(g_dbHandle, _T("nodes"), _T("rack_image"), _T("rack_image_front")));
      CHK_EXEC(DBRenameColumn(g_dbHandle, _T("chassis"), _T("rack_image"), _T("rack_image_front")));
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

      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("items"), _T("instance_retention_time")));
      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("dc_tables"), _T("instance_retention_time")));

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

      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("nodes"), _T("rack_orientation")));
      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("chassis"), _T("rack_orientation")));

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

   DBSetNotNullConstraint(g_dbHandle, _T("snmp_communities"), _T("zone"));
   DBSetNotNullConstraint(g_dbHandle, _T("usm_credentials"), _T("zone"));

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
      CHK_EXEC(CreateConfigParam(_T("DBWriter.MaxRecordsPerTransaction"), _T("1000"), _T("Maximum number of records per one transaction for delayed database writes."), NULL, 'I', true, true, false, false));
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
      int count = DBMgrConfigReadInt32(_T("NumberOfDataCollectors"), 250);
      TCHAR value[64];
      _sntprintf(value, 64,_T("%d"), std::max(250, count));
      CHK_EXEC(CreateConfigParam(_T("DataCollector.ThreadPool.BaseSize"), _T("10"), _T("Base size for data collector thread pool."), NULL, 'I', true, true, false, false));
      CHK_EXEC(CreateConfigParam(_T("DataCollector.ThreadPool.MaxSize"), value, _T("Maximum size for data collector thread pool."), NULL, 'I', true, true, false, false));
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
      DB_RESULT hResult = DBSelect(g_dbHandle, _T("SELECT access_rights,object_id FROM acl WHERE user_id=-2147483647")); // Get group Admins object acl
      if (hResult != NULL)
      {
         DB_STATEMENT hStmt = DBPrepare(g_dbHandle, _T("UPDATE acl SET access_rights=? WHERE user_id=-2147483647 AND object_id=? "));
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
                     if (!g_ignoreErrors)
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
         else if (!g_ignoreErrors)
            return FALSE;
         DBFreeResult(hResult);
      }
      else if (!g_ignoreErrors)
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

      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("nodes"), _T("fail_time_snmp")));
      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("nodes"), _T("fail_time_agent")));
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
   DB_RESULT hResult = DBSelect(g_dbHandle, query);
   DB_STATEMENT hStmt = DBPrepare(g_dbHandle, _T("UPDATE object_properties SET flags=? WHERE object_id=?"));
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
               if (!g_ignoreErrors)
               {
                  DBFreeStatement(hStmt);
                  DBFreeResult(hResult);
                  return FALSE;
               }
            }
         }
         DBFreeStatement(hStmt);
      }
      else if (!g_ignoreErrors)
      {
         return FALSE;
      }
      DBFreeResult(hResult);
   }
   else if (!g_ignoreErrors)
   {
      return FALSE;
   }

   CHK_EXEC(DBDropColumn(g_dbHandle, tableName, _T("flags")));
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
static void MoveNodeFlags(UINT32 oldFlag, UINT32 *flags, bool withSnmpConfLock)
{
   MoveFlag(oldFlag, flags, 0x10000000, DCF_DISABLE_STATUS_POLL);
   MoveFlag(oldFlag, flags, 0x20000000, DCF_DISABLE_CONF_POLL);
   MoveFlag(oldFlag, flags, 0x80000000, DCF_DISABLE_DATA_COLLECT);
   MoveFlag(oldFlag, flags, 0x00000080, NF_REMOTE_AGENT);
   if (withSnmpConfLock)
   {
      MoveFlag(oldFlag, flags, 0x00200000, NF_SNMP_SETTINGS_LOCKED);
   }
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
static void MoveNodeCapabilities(UINT32 oldFlag, UINT32 *capabilities, bool withSnmpConfLock)
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
   if (!withSnmpConfLock)
   {
      MoveFlag(oldFlag, capabilities, 0x00200000, NC_IS_SMCLP);
   }
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

   // move flags from old tables to the new one
   CHK_EXEC(MoveFlagsFromOldTables(_T("interfaces")));
   CHK_EXEC(MoveFlagsFromOldTables(_T("templates")));
   CHK_EXEC(MoveFlagsFromOldTables(_T("chassis")));
   CHK_EXEC(MoveFlagsFromOldTables(_T("object_containers")));
   CHK_EXEC(MoveFlagsFromOldTables(_T("network_maps")));
   if (GetSchemaLevelForMajorVersion(22) >= 12)
   {
      CHK_EXEC(MoveFlagsFromOldTables(_T("ap_common")));
   }

   // create special behavior for node and sensor, cluster node
   bool withSnmpConfLock = (GetSchemaLevelForMajorVersion(22) >= 37);
   DB_RESULT hResult = DBSelect(g_dbHandle, _T("SELECT id,runtime_flags FROM nodes"));
   DB_STATEMENT stmtNetObj = DBPrepare(g_dbHandle, _T("UPDATE object_properties SET flags=?, state=? WHERE object_id=?"));
   DB_STATEMENT stmtNode = DBPrepare(g_dbHandle, _T("UPDATE nodes SET capabilities=? WHERE id=?"));
   if (hResult != NULL)
   {
      if ((stmtNetObj != NULL) && (stmtNode != NULL))
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
            DB_RESULT flagResult = DBSelect(g_dbHandle, query);
            if (DBGetNumRows(flagResult) >= 1)
            {
               oldFlags = DBGetFieldULong(flagResult, 0, 0);
            }
            else
            {
               if(!g_ignoreErrors)
               {
                  DBFreeStatement(stmtNetObj);
                  DBFreeStatement(stmtNode);
                  DBFreeResult(hResult);
                  return FALSE;
               }
            }
            MoveNodeFlags(oldFlags, &flags, withSnmpConfLock);
            MoveNodeCapabilities(oldFlags, &capabilities, withSnmpConfLock);
            MoveNodeState(oldRuntime, &state);

            DBBind(stmtNetObj, 1, DB_SQLTYPE_INTEGER, flags);
            DBBind(stmtNetObj, 2, DB_SQLTYPE_INTEGER, state);
            DBBind(stmtNetObj, 3, DB_SQLTYPE_INTEGER, id);

            DBBind(stmtNode, 1, DB_SQLTYPE_INTEGER, capabilities);
            DBBind(stmtNode, 2, DB_SQLTYPE_INTEGER, id);

            if (!(SQLExecute(stmtNetObj)))
            {
               if (!g_ignoreErrors)
               {
                  DBFreeStatement(stmtNetObj);
                  DBFreeStatement(stmtNode);
                  DBFreeResult(hResult);
                  return FALSE;
               }
            }

            if (!SQLExecute(stmtNode))
            {
               if (!g_ignoreErrors)
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
        if (!g_ignoreErrors)
        {
           return FALSE;
        }
     }
     DBFreeResult(hResult);
   }
   CHK_EXEC(DBDropColumn(g_dbHandle, _T("nodes"), _T("runtime_flags")));
   CHK_EXEC(DBDropColumn(g_dbHandle, _T("nodes"), _T("node_flags")));

   //sensor
   hResult = DBSelect(g_dbHandle, _T("SELECT id,runtime_flags,flags FROM sensors"));
   DB_STATEMENT stmt = DBPrepare(g_dbHandle, _T("UPDATE object_properties SET status=? WHERE object_id=?"));
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
               if (!g_ignoreErrors)
               {
                  DBFreeStatement(stmt);
                  DBFreeResult(hResult);
                  return FALSE;
               }
            }
         }
         DBFreeStatement(stmt);
      }
      else if (!g_ignoreErrors)
      {
         return FALSE;
      }
      DBFreeResult(hResult);
   }
   CHK_EXEC(DBDropColumn(g_dbHandle, _T("sensors"), _T("runtime_flags")));
   CHK_EXEC(DBDropColumn(g_dbHandle, _T("sensors"), _T("flags")));

   CHK_EXEC(SetMinorSchemaVersion(4));
   return true;
}

/**
 * Upgrade from 30.2 to 30.3
 */
static bool H_UpgradeFromV2()
{
   CHK_EXEC(DBDropColumn(g_dbHandle, _T("event_groups"), _T("range_start")));
   CHK_EXEC(DBDropColumn(g_dbHandle, _T("event_groups"), _T("range_end")));

   static const TCHAR *batch =
            _T("ALTER TABLE event_groups ADD guid varchar(36) null\n")
            _T("UPDATE event_groups SET guid='04b326c0-5cc0-411f-8587-2836cb87c920' WHERE id=-2147483647\n")
            _T("UPDATE event_groups SET guid='b61859c6-1768-4a61-a0cf-eed07d688f66' WHERE id=-2147483646\n")
            _T("<END>");
   CHK_EXEC(SQLBatch(batch));
   DBSetNotNullConstraint(g_dbHandle, _T("event_groups"), _T("guid"));

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
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("users"), _T("created")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("user_groups"), _T("created")));

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
   { 56, 30, 57, H_UpgradeFromV56 },
   { 55, 30, 56, H_UpgradeFromV55 },
   { 54, 30, 55, H_UpgradeFromV54 },
   { 53, 30, 54, H_UpgradeFromV53 },
   { 52, 30, 53, H_UpgradeFromV52 },
   { 51, 30, 52, H_UpgradeFromV51 },
   { 50, 30, 51, H_UpgradeFromV50 },
   { 49, 30, 50, H_UpgradeFromV49 },
   { 48, 30, 49, H_UpgradeFromV48 },
   { 47, 30, 48, H_UpgradeFromV47 },
   { 46, 30, 47, H_UpgradeFromV46 },
   { 45, 30, 46, H_UpgradeFromV45 },
   { 44, 30, 45, H_UpgradeFromV44 },
   { 43, 30, 44, H_UpgradeFromV43 },
   { 42, 30, 43, H_UpgradeFromV42 },
   { 41, 30, 42, H_UpgradeFromV41 },
   { 40, 30, 41, H_UpgradeFromV40 },
   { 39, 30, 40, H_UpgradeFromV39 },
   { 38, 30, 39, H_UpgradeFromV38 },
   { 37, 30, 38, H_UpgradeFromV37 },
   { 36, 30, 37, H_UpgradeFromV36 },
   { 35, 30, 36, H_UpgradeFromV35 },
   { 34, 30, 35, H_UpgradeFromV34 },
   { 33, 30, 35, H_UpgradeFromV33 },
   { 32, 30, 33, H_UpgradeFromV32 },
   { 31, 30, 32, H_UpgradeFromV31 },
   { 30, 30, 31, H_UpgradeFromV30 },
   { 29, 30, 30, H_UpgradeFromV29 },
   { 28, 30, 28, H_UpgradeFromV28 },
   { 27, 30, 28, H_UpgradeFromV27 },
   { 26, 30, 27, H_UpgradeFromV26 },
   { 25, 30, 26, H_UpgradeFromV25 },
   { 24, 30, 25, H_UpgradeFromV24 },
   { 23, 30, 24, H_UpgradeFromV23 },
   { 22, 30, 23, H_UpgradeFromV22 },
   { 21, 30, 22, H_UpgradeFromV21 },
   { 20, 30, 21, H_UpgradeFromV20 },
   { 19, 30, 20, H_UpgradeFromV19 },
   { 18, 30, 19, H_UpgradeFromV18 },
   { 17, 30, 18, H_UpgradeFromV17 },
   { 16, 30, 17, H_UpgradeFromV16 },
   { 15, 30, 16, H_UpgradeFromV15 },
   { 14, 30, 15, H_UpgradeFromV14 },
   { 13, 30, 14, H_UpgradeFromV13 },
   { 12, 30, 13, H_UpgradeFromV12 },
   { 11, 30, 12, H_UpgradeFromV11 },
   { 10, 30, 11, H_UpgradeFromV10 },
   { 9,  30, 10, H_UpgradeFromV9 },
   { 8,  30, 9,  H_UpgradeFromV8 },
   { 7,  30, 8,  H_UpgradeFromV7 },
   { 6,  30, 7,  H_UpgradeFromV6 },
   { 5,  30, 6,  H_UpgradeFromV5 },
   { 4,  30, 5,  H_UpgradeFromV4 },
   { 3,  30, 4,  H_UpgradeFromV3 },
   { 2,  30, 3,  H_UpgradeFromV2 },
   { 1,  30, 2,  H_UpgradeFromV1 },
   { 0,  30, 1,  H_UpgradeFromV0 },
   { 0,  0,  0, NULL }
};

/**
 * Upgrade database to new version
 */
bool MajorSchemaUpgrade_V30()
{
   INT32 major, minor;
   if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
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
