/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2018 Victor Kirhenshtein
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
** File: localdb.cpp
**
**/

#include "nxagentd.h"

/**
 * Database driver
 */
static DB_DRIVER s_driver = NULL;

/**
 * Database handle
 */
static DB_HANDLE s_db = NULL;

/**
 * Read metadata
 */
TCHAR *ReadMetadata(const TCHAR *attr, TCHAR *buffer)
{
   if (s_db == NULL)
      return NULL;

   TCHAR query[256], *value = NULL;
   _sntprintf(query, 256, _T("SELECT value FROM metadata WHERE attribute='%s'"), attr);

   DB_RESULT hResult = DBSelect(s_db, query);
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         value = DBGetField(hResult, 0, 0, buffer, MAX_DB_STRING);
      DBFreeResult(hResult);
   }
   return value;
}

/**
 * Read integer from metadata
 */
INT32 ReadMetadataAsInt(const TCHAR *attr)
{
   TCHAR buffer[MAX_DB_STRING];
   if (ReadMetadata(attr, buffer) == NULL)
      return 0;
   return _tcstol(buffer, NULL, 0);
}

/**
 * Set value in metadata
 */
bool WriteMetadata(const TCHAR *name, const TCHAR *value)
{
   if (s_db == NULL)
      return false;

   TCHAR query[1024];
   _sntprintf(query, 1024, _T("INSERT OR REPLACE INTO metadata (attribute,value) VALUES ('%s',%s)"), 
      name, (const TCHAR *)DBPrepareString(s_db, value, 255));
   return DBQuery(s_db, query);
}

/**
 * Set value in metadata from int
 */
bool WriteMetadata(const TCHAR *name, INT32 value)
{
   TCHAR buffer[64];
   _sntprintf(buffer, 64, _T("%d"), value);
   return WriteMetadata(name, buffer);
}

/**
 * DB init queries
 */
static const TCHAR *s_dbInitQueries[] =
{
   _T("CREATE TABLE zone_config (")
   _T("  server_id number(20) not null,")
   _T("  this_node_id integer not null,")
   _T("  zone_uin integer not null,")
   _T("  shared_secret varchar(32) not null,")
   _T("  PRIMARY KEY(server_id))"),

   _T("CREATE TABLE dc_proxy (")
   _T("  server_id number(20) not null,")
   _T("  proxy_id integer not null,")
   _T("  ip_address varchar(48) not null,")
   _T("  PRIMARY KEY(server_id,proxy_id))"),

   _T("CREATE TABLE agent_policy (")
   _T("  guid varchar(36) not null,")
   _T("  type varchar(31) not null,")
   _T("  server_info varchar(64) null,")
   _T("  server_id number(20) not null,")
   _T("  version integer not null,")
   _T("  PRIMARY KEY(guid))"),

   _T("CREATE TABLE device_decoder_map (")
   _T("  guid varchar(36) not null,")
   _T("  devAddr varchar(10) null,")
   _T("  devEui varchar(10) null,")
   _T("  decoder integer not null,")
   _T("  last_contact integer null,")
   _T("  PRIMARY KEY(guid))"),

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
   _T("  backup_proxy_id integer null,")
   _T("  PRIMARY KEY(server_id,dci_id))"),

   _T("CREATE TABLE dc_queue (")
   _T("  server_id number(20) not null,")
   _T("  dci_id integer not null,")
   _T("  dci_type integer not null,")
   _T("  dci_origin integer not null,")
   _T("  snmp_target_guid varchar(36) not null,")
   _T("  timestamp integer not null,")
   _T("  value varchar not null,")
   _T("  status_code integer not null,")
   _T("  PRIMARY KEY(server_id,dci_id,timestamp))"),

   _T("CREATE INDEX idx_dc_queue_timestamp ON dc_queue(timestamp)"),

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
   _T("  PRIMARY KEY(guid))"),

   _T("CREATE TABLE registry (")
   _T("  attribute varchar(63) null,")
   _T("  value varchar null,")
   _T("  PRIMARY KEY(attribute))"),

   NULL
};

/**
 * Initialize new database
 */
static bool InitDatabase()
{
   for(int i = 0; s_dbInitQueries[i] != NULL; i++)
      if (!DBQuery(s_db, s_dbInitQueries[i]))
         return false;
   nxlog_debug(1, _T("Empty local database initialized successfully"));
   return true;
}

/**
 * Database tables
 */
static const TCHAR *s_dbTables[] = { _T("agent_policy"), _T("dc_config"), _T("dc_queue"), _T("dc_snmp_targets"), _T("registry"), NULL };

/**
 * Check database structure
 */
static bool CheckDatabaseStructure()
{
   int version;
   if (!DBIsTableExist(s_db, _T("metadata")))
   {
      DBQuery(s_db, _T("CREATE TABLE metadata (attribute varchar(63), value varchar(255), PRIMARY KEY(attribute))"));

      // assume empty database, create tables
      if (!InitDatabase())
      {
         nxlog_write(MSG_LOCAL_DB_CORRUPTED, NXLOG_ERROR, NULL);
         return false;
      }

      TCHAR query[256];
      _sntprintf(query, 256, _T("INSERT INTO metadata (attribute, value) VALUES ('SchemaVersion', '%d')"), DB_SCHEMA_VERSION);
      DBQuery(s_db, query);

      version = DB_SCHEMA_VERSION;
   }
   else
   {
      version = ReadMetadataAsInt(_T("SchemaVersion"));
   }

   if ((version <= 0) || (version > DB_SCHEMA_VERSION))
   {
      nxlog_write(MSG_LOCAL_DB_CORRUPTED, NXLOG_ERROR, NULL);
      return false;
   }

   if (version < DB_SCHEMA_VERSION)
   {
      if (!UpgradeDatabase())
      {
         version = ReadMetadataAsInt(_T("SchemaVersion"));
         nxlog_debug(1, _T("Local database schema version is %d and cannot be upgraded to %d"), version, DB_SCHEMA_VERSION);
         nxlog_write(MSG_LOCAL_DB_CORRUPTED, NXLOG_ERROR, NULL);
         return false;
      }
   }

   bool success = true;
   for(int i = 0; s_dbTables[i] != NULL; i++)
   {
      if (!DBIsTableExist(s_db, s_dbTables[i]))
      {
         nxlog_debug(1, _T("Local database table %s does not exist"), s_dbTables[i]);
         success = false;
      }
   }
   if (!success)
   {
      nxlog_write(MSG_LOCAL_DB_CORRUPTED, NXLOG_ERROR, NULL);
      return false;
   }

   return true;
}

/**
 * Custom database syntax reader
 */
static bool SyntaxReader(DB_HANDLE hdb, TCHAR *syntaxId)
{
   _tcscpy(syntaxId, _T("SQLITE"));
   return true;
}

/**
 * Open local agent database
 */
bool OpenLocalDatabase()
{
#ifdef _STATIC_AGENT
   s_driver = DBLoadDriver(_T(":self:"), _T(""), nxlog_get_debug_level() == 9, NULL, NULL);
#else
   s_driver = DBLoadDriver(_T("sqlite.ddr"), _T(""), nxlog_get_debug_level() == 9, NULL, NULL);
#endif
   if (s_driver == NULL)
   {
      return false;
   }

   DBSetSyntaxReader(SyntaxReader);

   TCHAR dbFile[MAX_PATH];
	_tcslcpy(dbFile, g_szDataDirectory, MAX_PATH - 12);
	if (dbFile[_tcslen(dbFile) - 1] != FS_PATH_SEPARATOR_CHAR)
		_tcscat(dbFile, FS_PATH_SEPARATOR);
	_tcscat(dbFile, _T("nxagentd.db"));
	if (g_dwFlags & AF_SUBAGENT_LOADER)
	{
	   _tcscat(dbFile, _T("."));
      _tcscat(dbFile, g_masterAgent);
	}

   TCHAR errorText[DBDRV_MAX_ERROR_TEXT];
   s_db = DBConnect(s_driver, NULL, dbFile, NULL, NULL, NULL, errorText);
   if (s_db == NULL)
   {
      nxlog_debug(1, _T("Local database open error: %s"), errorText);
	   g_failFlags |= FAIL_OPEN_DATABASE;
      return false;
   }

   if (!CheckDatabaseStructure())
   {
	   g_failFlags |= FAIL_UPGRADE_DATABASE;
      DBDisconnect(s_db);
      s_db = NULL;
      return false;
   }

   if (g_longRunningQueryThreshold != 0)
      DBSetLongRunningThreshold(g_longRunningQueryThreshold);

   DBQuery(s_db, _T("VACUUM"));
   DebugPrintf(1, _T("Local database opened successfully"));
   return true;
}

/**
 * Close local database
 */
void CloseLocalDatabase()
{
   DBDisconnect(s_db);
   DBUnloadDriver(s_driver);
}

/**
 * Get database handle
 */
DB_HANDLE GetLocalDatabaseHandle()
{
   return s_db;
}
