/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2023 Victor Kirhenshtein
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
#include <nxstat.h>

/**
 * Database driver
 */
static DB_DRIVER s_driver = nullptr;

/**
 * Database handle
 */
static DB_HANDLE s_db = nullptr;

/**
 * Read metadata
 */
TCHAR *ReadMetadata(const TCHAR *attr, TCHAR *buffer)
{
   if (s_db == nullptr)
      return nullptr;

   TCHAR query[256], *value = nullptr;
   _sntprintf(query, 256, _T("SELECT value FROM metadata WHERE attribute='%s'"), attr);

   DB_RESULT hResult = DBSelect(s_db, query);
   if (hResult != nullptr)
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
int32_t ReadMetadataAsInt(const TCHAR *attr)
{
   TCHAR buffer[MAX_DB_STRING];
   if (ReadMetadata(attr, buffer) == nullptr)
      return 0;
   return _tcstol(buffer, nullptr, 0);
}

/**
 * Set value in metadata
 */
bool WriteMetadata(const TCHAR *name, const TCHAR *value)
{
   if (s_db == nullptr)
      return false;

   TCHAR query[1024];
   _sntprintf(query, 1024, _T("INSERT OR REPLACE INTO metadata (attribute,value) VALUES ('%s',%s)"), 
      name, (const TCHAR *)DBPrepareString(s_db, value, 255));
   return DBQuery(s_db, query);
}

/**
 * Set value in metadata from int
 */
bool WriteMetadata(const TCHAR *name, int32_t value)
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
   _T("CREATE TABLE dc_schedules (")
   _T("  server_id number(20) not null,")
   _T("  dci_id integer not null,")
   _T("  schedule varchar(255) not null)"),

   _T("CREATE INDEX idx_dc_schedules ON dc_schedules(server_id,dci_id)"),

   _T("CREATE TABLE user_agent_notifications (")
   _T("  server_id number(20) not null,")
   _T("  notification_id integer not null,")
   _T("  message varchar(1023) not null,")
   _T("  start_time integer not null,")
   _T("  end_time integer not null,")
   _T("  on_startup char(1) not null,")
   _T("  PRIMARY KEY(server_id,notification_id))"),

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
   _T("  content_hash varchar(32) not null,")
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
   _T("  snmp_version integer not null,")
   _T("  snmp_target_guid varchar(36) not null,")
   _T("  snmp_raw_type integer not null,")
   _T("  backup_proxy_id integer null,")
   _T("  schedule_type integer not null,")
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

   _T("CREATE TABLE dc_snmp_table_columns (")
   _T("  server_id number(20) not null,")
   _T("  dci_id integer not null,")
   _T("  column_id integer not null,")
   _T("  name varchar(63) not null,")
   _T("  display_name varchar(255) null,")
   _T("  snmp_oid varchar(1023) null,")
   _T("  flags integer not null,")
   _T("  PRIMARY KEY(server_id,dci_id,column_id))"),

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

   _T("CREATE TABLE notification_servers (")
   _T("  server_id number(20) not null,")
   _T("  last_connection_time integer not null,")
   _T("  PRIMARY KEY(server_id))"),

   _T("CREATE TABLE notification_data (")
   _T("  server_id number(20) not null,")
   _T("  id integer not null,")
   _T("  serialized_data TEXT not null,")
   _T("  PRIMARY KEY(server_id,id))"),

   _T("CREATE TABLE file_integrity (")
   _T("  path varchar(4096) not null,")
   _T("  hash varchar(64) not null,")
   _T("  mod_time integer not null,")
   _T("  permissions integer not null,")
   _T("  PRIMARY KEY(path))"),

   _T("CREATE TABLE logwatch_files (")
   _T("  name varchar(256) not null,") //name or guid for policy or hash for regular file
   _T("  path varchar(4096) not null,") //File name assuming there are no policy where same file is defined twice
   _T("  size number(20) not null,")
   _T("  last_update_time integer not null,")
   _T("  PRIMARY KEY(name,path))"),

   nullptr
};

/**
 * Initialize new database
 */
static bool InitDatabase()
{
   for(int i = 0; s_dbInitQueries[i] != nullptr; i++)
      if (!DBQuery(s_db, s_dbInitQueries[i]))
         return false;
   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_LOCALDB, _T("Empty local database successfully initialized"));
   return true;
}

/**
 * Database tables
 */
static const TCHAR *s_dbTables[] =
{
   _T("agent_policy"),
   _T("dc_config"),
   _T("dc_queue"),
   _T("dc_proxy"),
   _T("dc_schedules"),
   _T("dc_snmp_table_columns"),
   _T("dc_snmp_targets"),
   _T("device_decoder_map"),
   _T("file_integrity"),
   _T("logwatch_files"),
   _T("notification_data"),
   _T("notification_servers"),
   _T("registry"),
   _T("user_agent_notifications"),
   _T("zone_config"),
   nullptr
};

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
         nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_LOCALDB, _T("Local database is corrupted and cannot be used (initialization failed)"));
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
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_LOCALDB, _T("Local database is corrupted and cannot be used (invalid schema version)"));
      return false;
   }

   if (version < DB_SCHEMA_VERSION)
   {
      if (!UpgradeDatabase())
      {
         version = ReadMetadataAsInt(_T("SchemaVersion"));
         nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_LOCALDB, _T("Local database is corrupted and cannot be used (database schema upgrade from version %d tp %d failed)"), version, DB_SCHEMA_VERSION);
         return false;
      }
   }

   bool success = true;
   for(int i = 0; s_dbTables[i] != nullptr; i++)
   {
      if (!DBIsTableExist(s_db, s_dbTables[i]))
      {
         nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_LOCALDB, _T("Local database table %s does not exist"), s_dbTables[i]);
         success = false;
      }
   }
   if (!success)
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_LOCALDB, _T("Local database is corrupted and cannot be used (missing tables)"));
      return false;
   }

   return true;
}

/**
 * Database file name
 */
static TCHAR s_dbFileName[MAX_PATH] = _T("");

/**
 * Open local agent database
 */
bool OpenLocalDatabase()
{
#ifdef _STATIC_AGENT
   s_driver = DBLoadDriver(_T(":self:"), _T(""), nullptr, nullptr);
#else
   s_driver = DBLoadDriver(_T("sqlite.ddr"), _T(""), nullptr, nullptr);
#endif
   if (s_driver == nullptr)
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_LOCALDB, _T("Cannot load SQLite database driver"));
      return false;
   }

   DBSetSyntaxReader(
      [] (DB_HANDLE hdb, TCHAR *syntaxId) -> bool
      {
         _tcscpy(syntaxId, _T("SQLITE"));
         return true;
      });

	_tcslcpy(s_dbFileName, g_szDataDirectory, MAX_PATH - 12);
	if (s_dbFileName[_tcslen(s_dbFileName) - 1] != FS_PATH_SEPARATOR_CHAR)
		_tcscat(s_dbFileName, FS_PATH_SEPARATOR);
	_tcslcat(s_dbFileName, _T("nxagentd.db"), MAX_PATH);
	if (g_dwFlags & AF_SUBAGENT_LOADER)
	{
	   _tcslcat(s_dbFileName, _T("."), MAX_PATH);
      _tcslcat(s_dbFileName, g_masterAgent, MAX_PATH);
	}

   TCHAR errorText[DBDRV_MAX_ERROR_TEXT];
   s_db = DBConnect(s_driver, nullptr, s_dbFileName, nullptr, nullptr, nullptr, errorText);
   if (s_db == nullptr)
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_LOCALDB, _T("Cannot open database (%s)"), errorText);
	   g_failFlags |= FAIL_OPEN_DATABASE;
      RegisterProblem(SEVERITY_MAJOR, _T("localdb-open"), _T("Agent cannot open local database"));
      return false;
   }

   if (!CheckDatabaseStructure())
   {
	   g_failFlags |= FAIL_UPGRADE_DATABASE;
      DBDisconnect(s_db);
      s_db = nullptr;
      RegisterProblem(SEVERITY_MAJOR, _T("localdb-upgrade"), _T("Schema upgrade for agent local database failed"));
      return false;
   }

   if (g_longRunningQueryThreshold != 0)
      DBSetLongRunningThreshold(g_longRunningQueryThreshold);

   DBQuery(s_db, _T("VACUUM"));
   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_LOCALDB, _T("Local database opened successfully"));
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

/**
 * Get size of local database file
 */
int64_t GetLocalDatabaseFileSize()
{
   if (s_dbFileName[0] == 0)
      return -1;

   NX_STAT_STRUCT fileInfo;
   if (CALL_STAT(s_dbFileName, &fileInfo) == -1)
   {
      nxlog_debug_tag(DEBUG_TAG_LOCALDB, 5, _T("GetLocalDatabaseFileSize: stat() call on \"%s\" failed (%s)"), s_dbFileName, _tcserror(errno));
      return -1;
   }

   return static_cast<int64_t>(fileInfo.st_size);
}
