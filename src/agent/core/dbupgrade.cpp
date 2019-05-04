/*
** NetXMS multiplatform core agent
** Copyright (C) 2016-2018 Raden Solutions
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
** File: dbupgrade.cpp
**
**/

#include "nxagentd.h"
#include <nxstat.h>

/**
 * Execute with error check
 */
#define CHK_EXEC(x) do { if (!(x)) if (!g_ignoreAgentDbErrors) return false; } while (0)

bool g_ignoreAgentDbErrors = FALSE;

#define Query(sql) DBQuery(s_db, sql)

/**
 * Database handle
 */
static DB_HANDLE s_db = NULL;

/**
 * Upgrade from V7 to V8
 */
static BOOL H_UpgradeFromV7(int currVersion, int newVersion)
{
   CHK_EXEC(Query(_T("ALTER TABLE dc_config ADD backup_proxy_id integer")));

   TCHAR createDcProxy[] =
            _T("CREATE TABLE dc_proxy (")
            _T("  server_id number(20) not null,")
            _T("  proxy_id integer not null,")
            _T("  ip_address varchar(48) not null,")
            _T("  PRIMARY KEY(server_id,proxy_id))");
   CHK_EXEC(Query(createDcProxy));

   TCHAR createZoneConfig[] =
            _T("CREATE TABLE zone_config (")
            _T("  server_id number(20) not null,")
            _T("  this_node_id integer not null,")
            _T("  zone_uin integer not null,")
            _T("  shared_secret varchar(32) not null,")
            _T("  PRIMARY KEY(server_id))");
   CHK_EXEC(Query(createZoneConfig));

   CHK_EXEC(WriteMetadata(_T("SchemaVersion"), 8));
   return TRUE;
}

/**
 * Upgrade from V6 to V7
 */
static BOOL H_UpgradeFromV6(int currVersion, int newVersion)
{
   const TCHAR *policyNames[] = { _T("None"), _T("AgentConfig"), _T("LogParserConfig")};

   //Select all types
   DB_RESULT hResult = DBSelect(s_db, _T("SELECT guid,type FROM agent_policy"));
   if (hResult != NULL)
   {
      CHK_EXEC(DBDropColumn(s_db, _T("agent_policy"), _T("type")));
      CHK_EXEC(Query(_T("ALTER TABLE agent_policy ADD type varchar(31)")));
      DB_STATEMENT hStmt = DBPrepare(s_db, _T("UPDATE agent_policy SET type=? WHERE guid=?"));
      if(hStmt != NULL)
      {
         for(int row = 0; row < DBGetNumRows(hResult); row++)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, policyNames[DBGetFieldULong(hResult, row, 1)], DB_BIND_STATIC);
            DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, DBGetFieldGUID(hResult, row, 0));
            CHK_EXEC(DBExecute(hStmt));
         }
         DBFreeStatement(hStmt);
      }
   }
   else
   {
      if (!g_ignoreAgentDbErrors)
         return FALSE;
   }
   CHK_EXEC(WriteMetadata(_T("SchemaVersion"), 7));
   return TRUE;
}

/**
 * Upgrade from V5 to V6
 */
static BOOL H_UpgradeFromV5(int currVersion, int newVersion)
{
   TCHAR upgradeQueries[] =
            _T("CREATE TABLE device_decoder_map (")
            _T("  guid varchar(36) not null,")
            _T("  devAddr varchar(10) null,")
            _T("  devEui varchar(10) null,")
            _T("  decoder integer not null,")
            _T("  last_contact integer null,")
            _T("  PRIMARY KEY(guid))");
   CHK_EXEC(Query(upgradeQueries));
   CHK_EXEC(WriteMetadata(_T("SchemaVersion"), 6));
   return TRUE;
}

/**
 * Upgrade from V4 to V5
 */
static BOOL H_UpgradeFromV4(int currVersion, int newVersion)
{
   CHK_EXEC(Query(_T("CREATE INDEX idx_dc_queue_timestamp ON dc_queue(timestamp)")));
   CHK_EXEC(WriteMetadata(_T("SchemaVersion"), 5));
   return TRUE;
}

/**
 * Upgrade from V3 to V4
 */
static BOOL H_UpgradeFromV3(int currVersion, int newVersion)
{
   CHK_EXEC(Query(_T("ALTER TABLE dc_queue ADD status_code integer")));
   CHK_EXEC(WriteMetadata(_T("SchemaVersion"), 4));
   return TRUE;
}

/**
 * Upgrade from V2 to V3
 */
static BOOL H_UpgradeFromV2(int currVersion, int newVersion)
{
   //This upgrade procedure moves config policy from old folder to a new one that is specially created form this purposes
   DB_RESULT hResult = DBSelect(s_db, _T("SELECT guid FROM agent_policy WHERE type=1"));
   if (hResult != NULL)
   {
      for(int row = 0; row < DBGetNumRows(hResult); row++)
      {
         uuid guid = DBGetFieldGUID(hResult, row, 0);
         TCHAR oldPath[MAX_PATH], newPath[MAX_PATH], name[64], tail;

         tail = g_szConfigPolicyDir[_tcslen(g_szConfigPolicyDir) - 1];
         _sntprintf(newPath, MAX_PATH, _T("%s%s%s.conf"), g_szConfigPolicyDir,
                    ((tail != '\\') && (tail != '/')) ? FS_PATH_SEPARATOR : _T(""),
                    guid.toString(name));
         tail = g_szConfigIncludeDir[_tcslen(g_szConfigIncludeDir) - 1];
         _sntprintf(oldPath, MAX_PATH, _T("%s%s%s.conf"), g_szConfigIncludeDir,
 	           ((tail != '\\') && (tail != '/')) ? FS_PATH_SEPARATOR : _T(""),
               guid.toString(name));

         //check that file exists and move it to other folder
         NX_STAT_STRUCT st;
         if (CALL_STAT(oldPath, &st) != 0)
         {
            continue;
         }

#ifdef _WIN32
         MoveFileEx(oldPath, newPath, MOVEFILE_COPY_ALLOWED);
#else
         if (_trename(oldPath, newPath) != 0)
         {
            int oldFile = _topen(oldPath, O_RDONLY | O_BINARY);
            bool success = true;
            if (oldFile == -1)
               continue;

            int newFile = _topen(newPath, O_CREAT | O_BINARY | O_WRONLY, st.st_mode); // should be copied with the same acess rights
            if (newFile == -1)
            {
               close(oldFile);
               continue;
            }

            int size = 16384, in, out;
            BYTE *bytes = (BYTE *)malloc(size);

            while((in = read(oldFile, bytes, size)) > 0)
            {
               out = write(newFile, bytes, (ssize_t)in);
               if (out != in)
               {
                  break;
               }
            }

            close(oldFile);
            close(newFile);
            free(bytes);
            if(success)
               _tremove(oldPath);
         }
#endif /* _WIN32 */
      }
      DBFreeResult(hResult);
   }

   CHK_EXEC(WriteMetadata(_T("SchemaVersion"), 3));
   return TRUE;
}

/**
 * Upgrade from V1 to V2
 */
static BOOL H_UpgradeFromV1(int currVersion, int newVersion)
{
   /*
   This upgrade contains:
     1. check that version and depending on version of DATACOLL_SCHEMA_VERSION apply second or both patches
        move upgrade of data collection database to this function
     2. remove DATACOLL_SCHEMA_VERSION from metadata
     3. create policy table (guid, type, server, version) unique giud
     4. Move policy information from registry to database
     5. Create table registry that will store (key, value) unique key
     6. Move upgrade file storage pleace from registry to db
     7. Delete registry file (remove unused functions for registry)
   */

   // Data collection upgrade procedure
   const TCHAR *s_upgradeQueries[] =
   {
      _T("CREATE TABLE dc_queue (")
      _T("  server_id number(20) not null,")
      _T("  dci_id integer not null,")
      _T("  dci_type integer not null,")
      _T("  dci_origin integer not null,")
      _T("  snmp_target_guid varchar(36) not null,")
      _T("  timestamp integer not null,")
      _T("  value varchar not null,")
      _T("  PRIMARY KEY(server_id,dci_id,timestamp))"),

      _T("CREATE TABLE dc_config (")
      _T("  server_id number(20) not null,")
      _T("  dci_id integer not null,")
      _T("  type integer not null,")
      _T("  origin integer not null,")
      _T("  name varchar(1023) null,")
      _T("  polling_interval integer not null,")
      _T("  last_poll integer not null,")
      _T("  snmp_port integer not null,")
      _T("  snmp_target_guid varchar(36) not null,")
      _T("  snmp_raw_type integer not null,")
      _T("  PRIMARY KEY(server_id,dci_id))"),

      _T("CREATE TABLE dc_snmp_targets (")
      _T("  guid varchar(36) not null,")
      _T("  server_id number(20) not null,")
      _T("  ip_address varchar(48) not null,")
      _T("  snmp_version integer not null,")
      _T("  port integer not null,")
      _T("  auth_type integer not null,")
      _T("  enc_type integer not null,")
      _T("  auth_name varchar(63),")
      _T("  auth_pass varchar(63),")
      _T("  enc_pass varchar(63),")
      _T("  PRIMARY KEY(guid))")
   };

   UINT32 dbVersion = ReadMetadataAsInt(_T("DataCollectionSchemaVersion"));
   while(dbVersion < 3)
   {
      CHK_EXEC(Query(s_upgradeQueries[dbVersion]));
      dbVersion++;
   }

   CHK_EXEC(Query(_T("DELETE FROM metadata WHERE attribute='DataCollectionSchemaVersion'")));

   //Policy upgrade procedure
   TCHAR createPolicyTable[] = _T("CREATE TABLE agent_policy (")
                              _T("  guid varchar(36) not null,")
                              _T("  type integer not null,")
                              _T("  server_info varchar(64) null,")
                              _T("  server_id number(20) not null,")
                              _T("  version integer not null,")
                              _T("  PRIMARY KEY(guid))");
   CHK_EXEC(Query(createPolicyTable));

   //Create registry table
   TCHAR crateRegistryTable[] = _T("CREATE TABLE registry (")
                              _T("  attribute varchar(63) null,")
                              _T("  value varchar null,")
                              _T("  PRIMARY KEY(attribute))");
   CHK_EXEC(Query(crateRegistryTable));

   // Initialize persistent storage
   bool registryExists = false;
   Config *registry = new Config;
	registry->setTopLevelTag(_T("registry"));
	TCHAR regPath[MAX_PATH];
	nx_strncpy(regPath, g_szDataDirectory, MAX_PATH - _tcslen(_T("registry.dat")) - 1);
	if (regPath[_tcslen(regPath) - 1] != FS_PATH_SEPARATOR_CHAR)
		_tcscat(regPath, FS_PATH_SEPARATOR);
	_tcscat(regPath, _T("registry.dat"));
	registryExists = registry->loadXmlConfig(regPath, "registry");
	if (!registryExists)
   {
      DebugPrintf(1, _T("Registry file doesn't exist. No data will be moved from registry to database\n"));
      CHK_EXEC(WriteMetadata(_T("SchemaVersion"), 2));
      return TRUE;
   }

   //Move policy data form registry file to agent database
	ObjectArray<ConfigEntry> *list = registry->getSubEntries(_T("/policyRegistry"), NULL);
   DB_STATEMENT hStmt = DBPrepare(s_db,
                 _T("INSERT INTO agent_policy (guid,type,server_info,server_id,version)")
                 _T(" VALUES (?,?,?,0,0)"));
	if(hStmt != NULL)
	{
	   if (list != NULL)
      {
         for(int i = 0; i < list->size(); i++)
         {
            ConfigEntry *e = list->get(i);
            if (MatchString(_T("policy-*"), e->getName(), TRUE))
            {
               DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, uuid::parse(&(e->getName()[7])));
               DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, e->getSubEntryValueAsInt(_T("type")));
               DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, e->getSubEntryValue(_T("server")), DB_BIND_STATIC);
               CHK_EXEC(DBExecute(hStmt));
            }
         }
         delete list;
      }
      DBFreeStatement(hStmt);
	}

   // Move upgrade file url variable to database
   const TCHAR *fullPath = registry->getValue(_T("/upgrade/file"));
   if (fullPath != NULL && fullPath[0] != 0)
   {
      TCHAR upgradeFileInsert[256];
      _sntprintf(upgradeFileInsert, 256, _T("INSERT INTO registry (attribute,value) VALUES ('upgrade.file','%s')"), fullPath);
      CHK_EXEC(Query(upgradeFileInsert));
   }
   delete registry;
   //Delete registry file
   _tremove(regPath);

   CHK_EXEC(WriteMetadata(_T("SchemaVersion"), 2));
   return TRUE;
}

/**
 * Upgrade map
 */
static struct
{
   int version;
   int newVersion;
   BOOL (* fpProc)(int, int);
} m_dbUpgradeMap[] =
{
   { 1, 2, H_UpgradeFromV1 },
   { 2, 3, H_UpgradeFromV2 },
   { 3, 4, H_UpgradeFromV3 },
   { 4, 5, H_UpgradeFromV4 },
   { 5, 6, H_UpgradeFromV5 },
   { 6, 7, H_UpgradeFromV6 },
   { 7, 8, H_UpgradeFromV7 },
   { 0, 0, NULL }
};


/**
 * Upgrade database to new version
 */
bool UpgradeDatabase()
{
   int i;
   UINT32 version = 0;
   BOOL bLocked = FALSE;
   s_db = GetLocalDatabaseHandle();

   // Get database format version
	version = ReadMetadataAsInt(_T("SchemaVersion"));
   if (version == DB_SCHEMA_VERSION)
   {
      DebugPrintf(1, _T("Database format is up to date"));
   }
   else if (version > DB_SCHEMA_VERSION)
   {
        DebugPrintf(1, _T("Your database has format version %d, this agent is compiled for version %d.\n"), version, DB_SCHEMA_VERSION);
         DebugPrintf(1, _T("You need to upgrade your agent before using this database.\n"));

   }
   else
   {
      // Upgrade database
      while(version < DB_SCHEMA_VERSION)
      {
         // Find upgrade procedure
         for(i = 0; m_dbUpgradeMap[i].fpProc != NULL; i++)
            if (m_dbUpgradeMap[i].version == version)
               break;
         if (m_dbUpgradeMap[i].fpProc == NULL)
         {
            DebugPrintf(1, _T("Unable to find upgrade procedure for version %d"), version);
            break;
         }
         DebugPrintf(1, _T("Upgrading from version %d to %d"), version, m_dbUpgradeMap[i].newVersion);
         DBBegin(s_db);
         if (m_dbUpgradeMap[i].fpProc(version, m_dbUpgradeMap[i].newVersion))
         {
            DBCommit(s_db);
            version = ReadMetadataAsInt(_T("SchemaVersion"));
         }
         else
         {
            DebugPrintf(1, _T("Rolling back last stage due to upgrade errors..."));
            DBRollback(s_db);
            break;
         }
      }

      DebugPrintf(1, _T("Database upgrade %s"), (version == DB_SCHEMA_VERSION) ? _T("succeeded") : _T("failed"));
   }
   return version == DB_SCHEMA_VERSION;
}
