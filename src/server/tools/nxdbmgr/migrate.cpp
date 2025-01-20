/*
** nxdbmgr - NetXMS database manager
** Copyright (C) 2004-2025 Victor Kirhenshtein
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
** File: migrate.cpp
**
**/

#include "nxdbmgr.h"

/**
 * Tables to migrate
 */
extern const TCHAR *g_tables[];

/**
 * Source config
 */
static TCHAR s_dbDriver[MAX_PATH] = _T("");
static TCHAR s_dbDriverOptions[MAX_PATH] = _T("");
static TCHAR s_dbServer[MAX_PATH] = _T("127.0.0.1");
static TCHAR s_dbLogin[MAX_DB_LOGIN] = _T("netxms");
static TCHAR s_dbPassword[MAX_PASSWORD] = _T("");
static TCHAR s_dbName[MAX_DB_NAME] = _T("netxms_db");
static TCHAR s_dbSchema[MAX_DB_NAME] = _T("");
static NX_CFG_TEMPLATE m_cfgTemplate[] =
{
   { _T("DBDriver"), CT_STRING, 0, 0, MAX_PATH, 0, s_dbDriver },
   { _T("DBDriverOptions"), CT_STRING, 0, 0, MAX_PATH, 0, s_dbDriverOptions },
   { _T("DBLogin"), CT_STRING, 0, 0, MAX_DB_LOGIN, 0, s_dbLogin },
   { _T("DBName"), CT_STRING, 0, 0, MAX_DB_NAME, 0, s_dbName },
   { _T("DBPassword"), CT_STRING, 0, 0, MAX_PASSWORD, 0, s_dbPassword },
   { _T("DBSchema"), CT_STRING, 0, 0, MAX_PATH, 0, s_dbSchema },
   { _T("DBServer"), CT_STRING, 0, 0, MAX_PATH, 0, s_dbServer },
   /* deprecated parameters */
   { _T("DBDrvParams"), CT_STRING, 0, 0, MAX_PATH, 0, s_dbDriverOptions },
   { _T("DBEncryptedPassword"), CT_STRING, 0, 0, MAX_PASSWORD, 0, s_dbPassword },
   { _T(""), CT_END_OF_LIST, 0, 0, 0, 0, nullptr }
};

/**
 * Source connection
 */
static DB_DRIVER s_driver = nullptr;
static DB_HANDLE s_hdbSource = nullptr;
static int s_sourceSyntax = DB_SYNTAX_UNKNOWN;

/**
 * Import/migrate flag
 */
static bool s_import = false;

/**
 * Well-known integer fields to be fixed during import
 */
static COLUMN_IDENTIFIER s_integerFixColumns[] =
{
   { _T("dct_threshold_instances"), "tt_row_number" },
   { _T("graphs"), "flags" },
   { _T("network_maps"), "bg_zoom" },
   { _T("nodes"), "capabilities" },
   { _T("nodes"), "port_rows" },
   { _T("nodes"), "port_numbering_scheme" },
   { _T("object_properties"), "state_before_maint" },
   { _T("snmp_communities"), "zone" },
   { _T("thresholds"), "state_before_maint" },
   { _T("usm_credentials"), "zone" },
   { nullptr, nullptr },
};

/**
 * Well-known timestamp columns that should be converted for TimescaleDB
 */
static COLUMN_IDENTIFIER s_timestampColumns[] =
{
   { _T("asset_change_log"), "operation_timestamp" },
   { _T("certificate_action_log"), "operation_timestamp" },
   { _T("event_log"), "event_timestamp" },
   { _T("maintenance_journal"), "creation_time" },
   { _T("maintenance_journal"), "modification_time" },
   { _T("notification_log"), "notification_timestamp" },
   { _T("server_action_execution_log"), "action_timestamp" },
   { _T("syslog"), "msg_timestamp" },
   { _T("snmp_trap_log"), "trap_timestamp" },
   { _T("win_event_log"), "event_timestamp" },
   { nullptr, nullptr },
};

/**
 * Check if fix is needed for column
 */
static bool IsColumnInList(const COLUMN_IDENTIFIER *list, const TCHAR *table, const char *name)
{
   for(int n = 0; list[n].table != nullptr; n++)
   {
      if (!_tcsicmp(list[n].table, table) && !stricmp(list[n].column, name))
         return true;
   }
   return false;
}

/**
 * Check if integer fix is needed for column
 */
bool IsColumnIntegerFixNeeded(const TCHAR *table, const char *name)
{
   return IsColumnInList(s_integerFixColumns, table, name);
}

/**
 * Check if timestamp conversion is needed for column
 */
bool IsTimestampColumn(const TCHAR *table, const char *name)
{
   return IsColumnInList(s_timestampColumns, table, name);
}

/**
 * Check if timestamp converting is needed for this table
 */
bool IsTimestampConversionNeeded(const TCHAR* table)
{
   for(int n = 0; s_timestampColumns[n].table != nullptr; n++)
   {
      if (!_tcsicmp(s_timestampColumns[n].table, table))
         return true;
   }
   return false;
}

/**
 * Connect to source database
 */
static bool ConnectToSource()
{
   s_driver = DBLoadDriver(s_dbDriver, s_dbDriverOptions, nullptr, nullptr);
   if (s_driver == nullptr)
   {
	   WriteToTerminalEx(_T("\x1b[31;1mERROR:\x1b[0m Unable to load and initialize database driver \"%s\"\n"), s_dbDriver);
      return false;
   }
   WriteToTerminalEx(_T("Database driver \x1b[1m%s\x1b[0m loaded\n"), s_dbDriver);

   TCHAR errorText[DBDRV_MAX_ERROR_TEXT];
   s_hdbSource = DBConnect(s_driver, s_dbServer, s_dbName, s_dbLogin, s_dbPassword, s_dbSchema, errorText);
   if (s_hdbSource == nullptr)
   {
      if (s_import)
         WriteToTerminal(_T("\x1b[31;1mERROR:\x1b[0m Cannot open import file\n"));
      else
         WriteToTerminalEx(_T("\x1b[31;1mERROR:\x1b[0m Unable to connect to database %s@%s as %s: %s\n"), s_dbName, s_dbServer, s_dbLogin, errorText);
      return false;
   }
   if (!s_import)
      WriteToTerminal(_T("Connected to source database\n"));

   // Get database syntax
   if (s_import)
   {
      s_sourceSyntax = DB_SYNTAX_SQLITE;
   }
   else
   {
      s_sourceSyntax = DBGetSyntax(s_hdbSource);
      if (s_sourceSyntax == DB_SYNTAX_UNKNOWN)
      {
         WriteToTerminal(_T("\x1b[31;1mERROR:\x1b[0m Unable to determine source database syntax\n"));
         return false;
      }
   }

   // Check source schema version
	int32_t major, minor;
   if (!DBGetSchemaVersion(s_hdbSource, &major, &minor))
   {
      WriteToTerminal(s_import ? _T("\x1b[31;1mERROR:\x1b[0m Import file is corrupted.\n") : _T("\x1b[31;1mERROR:\x1b[0m Unable to determine source database version.\n"));
      return false;
   }
   if ((major > DB_SCHEMA_VERSION_MAJOR) || ((major == DB_SCHEMA_VERSION_MAJOR) && (minor > DB_SCHEMA_VERSION_MINOR)))
   {
      _tprintf(_T("%s has format version %d.%d, this tool is compiled for version %d.%d.\n")
               _T("You need to upgrade your server before using this database.\n"),
             s_import ? _T("Import file") : _T("Source database"), major, minor, DB_SCHEMA_VERSION_MAJOR, DB_SCHEMA_VERSION_MINOR);
       return false;
   }
   if ((major < DB_SCHEMA_VERSION_MAJOR) || ((major == DB_SCHEMA_VERSION_MAJOR) && (minor < DB_SCHEMA_VERSION_MINOR)))
   {
      if (s_import)
         _tprintf(_T("Import file has format version %d.%d, this tool is compiled for version %d.%d.\n"),
               major, minor, DB_SCHEMA_VERSION_MAJOR, DB_SCHEMA_VERSION_MINOR);
      else
         _tprintf(_T("Source database has format version %d.%d, this tool is compiled for version %d.%d.\nUse \"upgrade\" command to upgrade source database first.\n"),
               major, minor, DB_SCHEMA_VERSION_MAJOR, DB_SCHEMA_VERSION_MINOR);
      return false;
   }

   return true;
}

/**
 * Migrate single database table
 */
static bool MigrateTable(const TCHAR *table)
{
   WriteToTerminalEx(_T("%s table \x1b[1m%s\x1b[0m\n"), s_import ? _T("Importing") : _T("Migrating"), table);

   if (!DBBegin(g_dbHandle))
   {
      _tprintf(_T("ERROR: unable to start transaction in target database\n"));
      return false;
   }

   bool success = false;
   TCHAR buffer[512], errorText[DBDRV_MAX_ERROR_TEXT];

   if ((s_sourceSyntax == DB_SYNTAX_TSDB) && IsTimestampConversionNeeded(table))
   {
      _sntprintf(buffer, 512, _T("SELECT * FROM %s WHERE 1=0"), table);
      DB_RESULT hResult = DBSelectEx(s_hdbSource, buffer, errorText);
      if (hResult != nullptr)
      {
         StringBuffer columns;
         int columnCount = DBGetColumnCount(hResult);
         for (int i = 0; i < columnCount; i++)
         {
            if (!columns.isEmpty())
               columns.append(_T(","));

            char columnName[256];
            DBGetColumnNameA(hResult, i, columnName, 256);
            if (IsTimestampColumn(table, columnName))
            {
               columns.append(_T("date_part('epoch',"));
               columns.appendUtf8String(columnName);
               columns.append(_T(")::int AS "));
               columns.appendUtf8String(columnName);
            }
            else
            {
               columns.appendUtf8String(columnName);
            }
         }
         DBFreeResult(hResult);
         _sntprintf(buffer, 512, _T("SELECT %s FROM %s"), columns.cstr(), table);
      }
      else
      {
         _tprintf(_T("ERROR: unable to read data from source table (%s)\n"), errorText);
         DBRollback(g_dbHandle);
         return false;
      }
   }
   else
   {
      _sntprintf(buffer, 512, _T("SELECT * FROM %s"), table);
   }

   DB_UNBUFFERED_RESULT hResult = DBSelectUnbufferedEx(s_hdbSource, buffer, errorText);
   if (hResult == nullptr)
   {
      _tprintf(_T("ERROR: unable to read data from source table (%s)\n"), errorText);
      DBRollback(g_dbHandle);
      return false;
   }

   // build INSERT query
   StringBuffer query = _T("INSERT INTO ");
   query.append(table);
   query.append(_T(" ("));
	int columnCount = DBGetColumnCount(hResult);
	for(int i = 0; i < columnCount; i++)
	{
		DBGetColumnName(hResult, i, buffer, 512);
		query.append(buffer);
		query.append(_T(","));
	}
	query.shrink();
	query.append(_T(") VALUES ("));
	if (g_dbSyntax == DB_SYNTAX_TSDB)
	{
      for(int i = 0; i < columnCount; i++)
      {
         char cname[256];
         DBGetColumnNameA(hResult, i, cname, 256);
         if (IsTimestampColumn(table, cname))
            query.append(_T("to_timestamp(?),"));
         else
            query.append(_T("?,"));
      }
	}
	else
	{
      for(int i = 0; i < columnCount; i++)
         query.append(_T("?,"));
	}
   query.shrink();
   query += _T(")");

   DB_STATEMENT hStmt = DBPrepareEx(g_dbHandle, query, true, errorText);
   if (hStmt != nullptr)
   {
      success = true;
      int rows = 0, totalRows = 0;
      while(DBFetch(hResult))
      {
         for(int i = 0; i < columnCount; i++)
         {
			   TCHAR *value = DBGetField(hResult, i, nullptr, 0);
			   if ((value == nullptr) || (*value == 0))
			   {
			      char cname[256];
			      DBGetColumnNameA(hResult, i, cname, 256);
               DBBind(hStmt, i + 1, DB_SQLTYPE_VARCHAR, IsColumnIntegerFixNeeded(table, cname) ? _T("0") : _T(""), DB_BIND_STATIC);
               MemFree(value);
			   }
			   else
			   {
			      DBBind(hStmt, i + 1, DB_SQLTYPE_VARCHAR, value, DB_BIND_DYNAMIC);
			   }
         }
         if (!SQLExecute(hStmt))
         {
            _tprintf(_T("Failed input record:\n"));
            for(int i = 0; i < columnCount; i++)
            {
               DBGetColumnName(hResult, i, buffer, 512);
               TCHAR *value = DBGetField(hResult, i, nullptr, 0);
               _tprintf(_T("   %s = \"%s\"\n"), buffer, CHECK_NULL(value));
               MemFree(value);
            }
            success = false;
            break;
         }

         rows++;
         if (rows >= g_migrationTxnSize)
         {
            rows = 0;
            DBCommit(g_dbHandle);
            DBBegin(g_dbHandle);
         }

         totalRows++;
         if ((totalRows & 0xFF) == 0)
         {
            _tprintf(_T("%8d\r"), totalRows);
            fflush(stdout);
         }
      }

      if (success)
         DBCommit(g_dbHandle);
      else
         DBRollback(g_dbHandle);
      DBFreeStatement(hStmt);
   }
   else
   {
		_tprintf(_T("ERROR: cannot prepare INSERT statement (%s)\n"), errorText);
      DBRollback(g_dbHandle);
   }
   DBFreeResult(hResult);
	return success;
}

/**
 * Migrate data tables
 */
static bool MigrateDataTables(bool ignoreDataMigrationErrors)
{
   IntegerArray<uint32_t> *targets = GetDataCollectionTargets();
   if (targets == nullptr)
      return false;

	// Create and import idata_xx and tdata_xx tables for each data collection target
	int count = targets->size();
   int i;
	for(i = 0; i < count; i++)
	{
	   uint32_t id = targets->get(i);
      if (!g_dataOnlyMigration)
      {
		   if (!CreateIDataTable(id))
			   break;	// Failed to create idata_xx table
      }

      if (!g_skipDataMigration)
      {
         TCHAR table[32];
		   _sntprintf(table, 32, _T("idata_%u"), id);
		   if (!MigrateTable(table) && !ignoreDataMigrationErrors)
			   break;
      }

      if (!g_dataOnlyMigration)
      {
         if (!CreateTDataTable(id))
			   break;	// Failed to create tdata tables
      }

      if (!g_skipDataMigration)
      {
         TCHAR table[32];
		   _sntprintf(table, 32, _T("tdata_%u"), id);
		   if (!MigrateTable(table) && !ignoreDataMigrationErrors)
			   break;
      }
	}

	delete targets;
	return i == count;
}

/**
 * DCI storage classes
 */
static HashMap<uint32_t, int> s_dciStorageClasses(Ownership::False);
static int s_storageClassNumbers[6] = { 0, 1, 2, 3, 4, 5 };

/**
 * Load DCI storage classes
 */
static bool LoadDCIStorageClasses(const TCHAR *table)
{
   TCHAR query[256];
   _sntprintf(query, 256, _T("SELECT item_id,retention_time FROM %s"), table);

   DB_RESULT hResult = DBSelect(s_hdbSource, query);
   if (hResult == NULL)
      return false;

   int count = DBGetNumRows(hResult);
   for(int i = 0; i < count; i++)
   {
      UINT32 id = DBGetFieldULong(hResult, i, 0);
      int days = DBGetFieldLong(hResult, i, 1);
      if (days == 0)
         s_dciStorageClasses.set(id, &s_storageClassNumbers[0]);
      else if (days <= 7)
         s_dciStorageClasses.set(id, &s_storageClassNumbers[1]);
      else if (days <= 30)
         s_dciStorageClasses.set(id, &s_storageClassNumbers[2]);
      else if (days <= 90)
         s_dciStorageClasses.set(id, &s_storageClassNumbers[3]);
      else if (days <= 180)
         s_dciStorageClasses.set(id, &s_storageClassNumbers[4]);
      else
         s_dciStorageClasses.set(id, &s_storageClassNumbers[5]);
   }
   DBFreeResult(hResult);
   return true;
}

/**
 * Load DCI storage classes
 */
static bool LoadDCIStorageClasses()
{
   return LoadDCIStorageClasses(_T("items")) && LoadDCIStorageClasses(_T("dc_tables"));
}

/**
 * Migrate collected data from multi-table to single table configuration
 */
static bool MigrateDataToSingleTable(uint32_t nodeId, bool tdata)
{
   const TCHAR *prefix = tdata ? _T("tdata") : _T("idata");
   WriteToTerminalEx(_T("%s table \x1b[1m%s_%u\x1b[0m to \x1b[1m%s\x1b[0m\n"), s_import ? _T("Importing") : _T("Migrating"), prefix, nodeId, prefix);

   if (!DBBegin(g_dbHandle))
   {
      _tprintf(_T("ERROR: unable to start transaction in target database\n"));
      return false;
   }

   bool success = false;
   TCHAR buffer[256], errorText[DBDRV_MAX_ERROR_TEXT];
   _sntprintf(buffer, 256, _T("SELECT item_id,%s_timestamp,%s_value%s FROM %s_%u"),
            prefix, prefix, tdata ? _T("") : _T(",raw_value"), prefix, nodeId);
   DB_UNBUFFERED_RESULT hResult = DBSelectUnbufferedEx(s_hdbSource, buffer, errorText);
   if (hResult == nullptr)
   {
      _tprintf(_T("ERROR: unable to read data from source table (%s)\n"), errorText);
      DBRollback(g_dbHandle);
      return false;
   }

   DB_STATEMENT hStmt = DBPrepareEx(g_dbHandle,
            tdata ?
               _T("INSERT INTO tdata (item_id,tdata_timestamp,tdata_value) VALUES (?,?,?)")
               : _T("INSERT INTO idata (item_id,idata_timestamp,idata_value,raw_value) VALUES (?,?,?,?)"),
            true, errorText);
   if (hStmt != nullptr)
   {
      success = true;
      int rows = 0, totalRows = 0;
      while(DBFetch(hResult))
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, DBGetFieldULong(hResult, 0));
         DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, DBGetFieldULong(hResult, 1));
         if (tdata)
         {
            DBBind(hStmt, 3, DB_SQLTYPE_TEXT, DBGetField(hResult, 2, nullptr, 0), DB_BIND_DYNAMIC);
         }
         else
         {
            DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, DBGetField(hResult, 2, nullptr, 0), DB_BIND_DYNAMIC);
            DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, DBGetField(hResult, 3, nullptr, 0), DB_BIND_DYNAMIC);
         }
         if (!SQLExecute(hStmt))
         {
            _tprintf(_T("Failed input record:\n"));
            int columnCount = DBGetColumnCount(hResult);
            for(int i = 0; i < columnCount; i++)
            {
               DBGetColumnName(hResult, i, buffer, 256);
               TCHAR *value = DBGetField(hResult, i, nullptr, 0);
               _tprintf(_T("   %s = \"%s\"\n"), buffer, CHECK_NULL(value));
               MemFree(value);
            }
            success = false;
            break;
         }

         rows++;
         if (rows >= g_migrationTxnSize)
         {
            rows = 0;
            DBCommit(g_dbHandle);
            DBBegin(g_dbHandle);
         }

         totalRows++;
         if ((totalRows & 0xFF) == 0)
         {
            _tprintf(_T("%8d\r"), totalRows);
            fflush(stdout);
         }
      }

      if (success)
         DBCommit(g_dbHandle);
      else
         DBRollback(g_dbHandle);
      DBFreeStatement(hStmt);
   }
   else
   {
      _tprintf(_T("ERROR: cannot prepare INSERT statement (%s)\n"), errorText);
      DBRollback(g_dbHandle);
   }
   DBFreeResult(hResult);
   return success;
}

/**
 * Build data insert query for TSDB
 */
static inline StringBuffer BuildDataInsertQuery(bool tdata, const TCHAR *sclass)
{
   StringBuffer query = _T("INSERT INTO ");
   query.append(tdata ? _T("tdata") : _T("idata"));
   query.append(_T("_sc_"));
   query.append(sclass);
   if (tdata)
      query.append(_T(" (item_id,tdata_timestamp,tdata_value) VALUES"));
   else
      query.append(_T(" (item_id,idata_timestamp,idata_value,raw_value) VALUES"));
   return query;
}

/**
 * Migrate collected data from multi-table to single table configuration (TSDB version)
 */
bool MigrateDataToSingleTable_TSDB(DB_HANDLE hdbSource, uint32_t nodeId, bool tdata)
{
   const TCHAR *prefix = tdata ? _T("tdata") : _T("idata");
   WriteToTerminalEx(_T("%s table \x1b[1m%s_%u\x1b[0m to \x1b[1m%s\x1b[0m\n"), s_import ? _T("Importing") : _T("Migrating"), prefix, nodeId, prefix);

   bool success = false;
   TCHAR buffer[256], errorText[DBDRV_MAX_ERROR_TEXT];
   _sntprintf(buffer, 256, _T("SELECT item_id,%s_timestamp,%s_value%s FROM %s_%u"), prefix, prefix, tdata ? _T("") : _T(",raw_value"), prefix, nodeId);
   DB_UNBUFFERED_RESULT hResult = DBSelectUnbufferedEx(hdbSource, buffer, errorText);
   if (hResult == nullptr)
   {
      _tprintf(_T("ERROR: unable to read data from source table (%s)\n"), errorText);
      return false;
   }

   StringBuffer queries[6];
   queries[0] = BuildDataInsertQuery(tdata, _T("default"));
   queries[1] = BuildDataInsertQuery(tdata, _T("7"));
   queries[2] = BuildDataInsertQuery(tdata, _T("30"));
   queries[3] = BuildDataInsertQuery(tdata, _T("90"));
   queries[4] = BuildDataInsertQuery(tdata, _T("180"));
   queries[5] = BuildDataInsertQuery(tdata, _T("other"));

   bool hasContent[6];
   memset(hasContent, 0, sizeof(hasContent));

   success = true;
   int rows = 0, totalRows = 0;
   while(DBFetch(hResult))
   {
      uint32_t dciId = DBGetFieldULong(hResult, 0);
      int *sclassPtr = s_dciStorageClasses.get(dciId);
      int sclass = (sclassPtr != nullptr) ? *sclassPtr : 0;

      StringBuffer& query = queries[sclass];
      query.append(hasContent[sclass] ? _T(",(") : _T(" ("));
      query.append(dciId);
      query.append(_T(",to_timestamp("));
      query.append(DBGetFieldULong(hResult, 1));
      query.append(_T("),"));
      TCHAR *value = DBGetField(hResult, 2, nullptr, 0);
      query.append(DBPrepareString(g_dbHandle, value));
      MemFree(value);
      if (!tdata)
      {
         query.append(_T(','));
         TCHAR *value = DBGetField(hResult, 3, nullptr, 0);
         query.append(DBPrepareString(g_dbHandle, value));
         MemFree(value);
      }
      query.append(_T(')'));

      hasContent[sclass] = true;

      rows++;
      if (rows >= g_migrationTxnSize)
      {
         for(int i = 0; i < 6; i++)
         {
            if (!hasContent[i])
               continue;

            queries[i].append(_T(" ON CONFLICT DO NOTHING"));
            DBBegin(g_dbHandle);
            if (DBQueryEx(g_dbHandle, queries[i], errorText))
            {
               DBCommit(g_dbHandle);
            }
            else
            {
               _tprintf(_T("ERROR: unable to insert data to destination table (%s)\n"), errorText);
               DBRollback(g_dbHandle);
               success = false;
            }
         }

         rows = 0;
         queries[0] = BuildDataInsertQuery(tdata, _T("default"));
         queries[1] = BuildDataInsertQuery(tdata, _T("7"));
         queries[2] = BuildDataInsertQuery(tdata, _T("30"));
         queries[3] = BuildDataInsertQuery(tdata, _T("90"));
         queries[4] = BuildDataInsertQuery(tdata, _T("180"));
         queries[5] = BuildDataInsertQuery(tdata, _T("other"));
         memset(hasContent, 0, sizeof(hasContent));
      }

      totalRows++;
      if ((totalRows & 0xFF) == 0)
      {
         _tprintf(_T("%8d\r"), totalRows);
         fflush(stdout);
      }
   }

   if (rows > 0)
   {
      for(int i = 0; i < 6; i++)
      {
         if (!hasContent[i])
            continue;

         queries[i].append(_T(" ON CONFLICT DO NOTHING"));
         DBBegin(g_dbHandle);
         if (DBQueryEx(g_dbHandle, queries[i], errorText))
         {
            DBCommit(g_dbHandle);
         }
         else
         {
            _tprintf(_T("ERROR: unable to insert data to destination table (%s)\n"), errorText);
            DBRollback(g_dbHandle);
            success = false;
         }
      }
   }

   DBFreeResult(hResult);
   return success;
}

/**
 * Migrate collected data from single table to single table configuration (TSDB version)
 */
static bool MigrateSingleDataTableToTSDB(bool tdata)
{
   const TCHAR *prefix = tdata ? _T("tdata") : _T("idata");
   WriteToTerminalEx(_T("%s table \x1b[1m%s\x1b[0m\n"), s_import ? _T("Importing") : _T("Migrating"), prefix);

   bool success = false;
   TCHAR buffer[256], errorText[DBDRV_MAX_ERROR_TEXT];
   _sntprintf(buffer, 256, _T("SELECT item_id,%s_timestamp,%s_value%s FROM %s"), prefix, prefix, tdata ? _T("") : _T(",raw_value"), prefix);
   DB_UNBUFFERED_RESULT hResult = DBSelectUnbufferedEx(s_hdbSource, buffer, errorText);
   if (hResult == nullptr)
   {
      _tprintf(_T("ERROR: unable to read data from source table (%s)\n"), errorText);
      return false;
   }

   StringBuffer queries[6];
   queries[0] = BuildDataInsertQuery(tdata, _T("default"));
   queries[1] = BuildDataInsertQuery(tdata, _T("7"));
   queries[2] = BuildDataInsertQuery(tdata, _T("30"));
   queries[3] = BuildDataInsertQuery(tdata, _T("90"));
   queries[4] = BuildDataInsertQuery(tdata, _T("180"));
   queries[5] = BuildDataInsertQuery(tdata, _T("other"));

   bool hasContent[6];
   memset(hasContent, 0, sizeof(hasContent));

   success = true;
   int rows = 0, totalRows = 0;
   while(DBFetch(hResult))
   {
      uint32_t dciId = DBGetFieldULong(hResult, 0);
      int *sclassPtr = s_dciStorageClasses.get(dciId);
      int sclass = (sclassPtr != nullptr) ? *sclassPtr : 0;

      StringBuffer& query = queries[sclass];
      query.append(hasContent[sclass] ? _T(",(") : _T(" ("));
      query.append(dciId);
      query.append(_T(",to_timestamp("));
      query.append(DBGetFieldULong(hResult, 1));
      query.append(_T("),"));
      TCHAR *value = DBGetField(hResult, 2, nullptr, 0);
      query.append(DBPrepareString(g_dbHandle, value));
      MemFree(value);
      if (!tdata)
      {
         query.append(_T(','));
         TCHAR *value = DBGetField(hResult, 3, nullptr, 0);
         query.append(DBPrepareString(g_dbHandle, value));
         MemFree(value);
      }
      query.append(_T(')'));

      hasContent[sclass] = true;

      rows++;
      if (rows >= g_migrationTxnSize)
      {
         for(int i = 0; i < 6; i++)
         {
            if (!hasContent[i])
               continue;

            queries[i].append(_T(" ON CONFLICT DO NOTHING"));
            DBBegin(g_dbHandle);
            if (DBQueryEx(g_dbHandle, queries[i], errorText))
            {
               DBCommit(g_dbHandle);
            }
            else
            {
               _tprintf(_T("ERROR: unable to insert data to destination table (%s)\n"), errorText);
               DBRollback(g_dbHandle);
               success = false;
            }
         }

         rows = 0;
         queries[0] = BuildDataInsertQuery(tdata, _T("default"));
         queries[1] = BuildDataInsertQuery(tdata, _T("7"));
         queries[2] = BuildDataInsertQuery(tdata, _T("30"));
         queries[3] = BuildDataInsertQuery(tdata, _T("90"));
         queries[4] = BuildDataInsertQuery(tdata, _T("180"));
         queries[5] = BuildDataInsertQuery(tdata, _T("other"));
         memset(hasContent, 0, sizeof(hasContent));
      }

      totalRows++;
      if ((totalRows & 0xFF) == 0)
      {
         _tprintf(_T("%8d\r"), totalRows);
         fflush(stdout);
      }
   }

   if (rows > 0)
   {
      for(int i = 0; i < 6; i++)
      {
         if (!hasContent[i])
            continue;

         queries[i].append(_T(" ON CONFLICT DO NOTHING"));
         DBBegin(g_dbHandle);
         if (DBQueryEx(g_dbHandle, queries[i], errorText))
         {
            DBCommit(g_dbHandle);
         }
         else
         {
            _tprintf(_T("ERROR: unable to insert data to destination table (%s)\n"), errorText);
            DBRollback(g_dbHandle);
            success = false;
         }
      }
   }

   DBFreeResult(hResult);
   return success;
}

/**
 * Migrate data tables to single table
 */
static bool MigrateDataTablesToSingleTable(bool ignoreDataMigrationErrors)
{
   IntegerArray<uint32_t> *targets = GetDataCollectionTargets();
   if (targets == nullptr)
      return false;

   // Copy data from idata_xx and tdata_xx tables for each node in "nodes" table
   int count = targets->size();
   int i;
   for(i = 0; i < count; i++)
   {
      uint32_t id = targets->get(i);
      if (g_dbSyntax == DB_SYNTAX_TSDB)
      {
         if (!MigrateDataToSingleTable_TSDB(s_hdbSource, id, false) && !ignoreDataMigrationErrors)
            break;
         if (!MigrateDataToSingleTable_TSDB(s_hdbSource, id, true) && !ignoreDataMigrationErrors)
            break;
      }
      else
      {
         if (!MigrateDataToSingleTable(id, false) && !ignoreDataMigrationErrors)
            break;
         if (!MigrateDataToSingleTable(id, true) && !ignoreDataMigrationErrors)
            break;
      }
   }

   delete targets;
   return i == count;
}

/**
 * Migrate collected data from single table to multi-single table configuration
 */
static bool MigrateDataFromSingleTable(uint32_t nodeId, bool tdata)
{
   const TCHAR *prefix = tdata ? _T("tdata") : _T("idata");
   WriteToTerminalEx(_T("%s table \x1b[1m%s_%u\x1b[0m from \x1b[1m%s\x1b[0m\n"), s_import ? _T("Importing") : _T("Migrating"), prefix, nodeId, prefix);

   if (!DBBegin(g_dbHandle))
   {
      _tprintf(_T("ERROR: unable to start transaction in target database\n"));
      return false;
   }

   bool success = false;
   TCHAR buffer[256], errorText[DBDRV_MAX_ERROR_TEXT];
   if (s_sourceSyntax == DB_SYNTAX_TSDB)
   {
      _sntprintf(buffer, 256, _T("SELECT item_id,date_part('epoch',%s_timestamp)::int,%s_value%s FROM %s WHERE item_id IN (SELECT item_id FROM %s WHERE node_id=%u)"),
               prefix, prefix, tdata ? _T("") : _T(",raw_value"), prefix, tdata ? _T("dc_tables") : _T("items"), nodeId);
   }
   else
   {
      _sntprintf(buffer, 256, _T("SELECT item_id,%s_timestamp,%s_value%s FROM %s WHERE item_id IN (SELECT item_id FROM %s WHERE node_id=%u)"),
               prefix, prefix, tdata ? _T("") : _T(",raw_value"), prefix, tdata ? _T("dc_tables") : _T("items"), nodeId);
   }
   DB_UNBUFFERED_RESULT hResult = DBSelectUnbufferedEx(s_hdbSource, buffer, errorText);
   if (hResult == nullptr)
   {
      _tprintf(_T("ERROR: unable to read data from source table (%s)\n"), errorText);
      DBRollback(g_dbHandle);
      return false;
   }

   if (tdata)
      _sntprintf(buffer, 256, _T("INSERT INTO tdata_%u (item_id,tdata_timestamp,tdata_value) VALUES (?,?,?)"), nodeId);
   else
      _sntprintf(buffer, 256, _T("INSERT INTO idata_%u (item_id,idata_timestamp,idata_value,raw_value) VALUES (?,?,?,?)"), nodeId);
   DB_STATEMENT hStmt = DBPrepareEx(g_dbHandle, buffer, true, errorText);
   if (hStmt != nullptr)
   {
      success = true;
      int rows = 0, totalRows = 0;
      while(DBFetch(hResult))
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, DBGetFieldULong(hResult, 0));
         DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, DBGetFieldULong(hResult, 1));
         if (tdata)
         {
            DBBind(hStmt, 3, DB_SQLTYPE_TEXT, DBGetField(hResult, 2, nullptr, 0), DB_BIND_DYNAMIC);
         }
         else
         {
            DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, DBGetField(hResult, 2, nullptr, 0), DB_BIND_DYNAMIC);
            DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, DBGetField(hResult, 3, nullptr, 0), DB_BIND_DYNAMIC);
         }
         if (!SQLExecute(hStmt))
         {
            _tprintf(_T("Failed input record:\n"));
            int columnCount = DBGetColumnCount(hResult);
            for(int i = 0; i < columnCount; i++)
            {
               DBGetColumnName(hResult, i, buffer, 256);
               TCHAR *value = DBGetField(hResult, i, nullptr, 0);
               _tprintf(_T("   %s = \"%s\"\n"), buffer, CHECK_NULL(value));
               MemFree(value);
            }
            success = false;
            break;
         }

         rows++;
         if (rows >= g_migrationTxnSize)
         {
            rows = 0;
            DBCommit(g_dbHandle);
            DBBegin(g_dbHandle);
         }

         totalRows++;
         if ((totalRows & 0xFF) == 0)
         {
            _tprintf(_T("%8d\r"), totalRows);
            fflush(stdout);
         }
      }

      if (success)
         DBCommit(g_dbHandle);
      else
         DBRollback(g_dbHandle);
      DBFreeStatement(hStmt);
   }
   else
   {
      _tprintf(_T("ERROR: cannot prepare INSERT statement (%s)\n"), errorText);
      DBRollback(g_dbHandle);
   }
   DBFreeResult(hResult);
   return success;
}

/**
 * Migrate data tables from single table
 */
static bool MigrateDataTablesFromSingleTable(bool ignoreDataMigrationErrors)
{
   IntegerArray<uint32_t> *targets = GetDataCollectionTargets();
   if (targets == nullptr)
      return false;

   // Create and import idata_xx and tdata_xx tables for each data collection target
   int count = targets->size();
   int i;
   for(i = 0; i < count; i++)
   {
      uint32_t id = targets->get(i);

      if (!g_dataOnlyMigration)
      {
         if (!CreateIDataTable(id))
            break;   // Failed to create idata_xx table
      }

      if (!g_skipDataMigration)
      {
         if (!MigrateDataFromSingleTable(id, false) && !ignoreDataMigrationErrors)
            break;
      }

      if (!g_dataOnlyMigration)
      {
         if (!CreateTDataTable(id))
            break;   // Failed to create tdata tables
      }

      if (!g_skipDataMigration)
      {
         if (!MigrateDataFromSingleTable(id, true) && !ignoreDataMigrationErrors)
            break;
      }
   }

   delete targets;
   return i == count;
}

/**
 * Callback for migrating module table
 */
static bool MigrateTableCallback(const TCHAR *table, void *context)
{
   auto lists = static_cast<std::pair<const StringList*, const StringList*>*>(context);
   if (lists->first->contains(table) || (!lists->second->isEmpty() && !lists->second->contains(table)))
   {
      WriteToTerminalEx(_T("Skipping table \x1b[1m%s\x1b[0m\n"), table);
      return true;
   }
   return MigrateTable(table);
}

/**
 * Do database import or migration
 */
static bool ImportOrMigrateDatabase(const StringList& excludedTables, const StringList& includedTables, bool ignoreDataMigrationErrors)
{
   if (!ConnectToSource())
      return false;

   bool success = false;
   if (!g_dataOnlyMigration)
   {
      // Do not clear destination if only explicitly listed tables are being imported/migrated
      if (includedTables.isEmpty())
      {
         if (!ClearDatabase(!s_import))
            goto cleanup;
      }

      // Migrate tables
      for(int i = 0; g_tables[i] != nullptr; i++)
      {
         const TCHAR *table = g_tables[i];
         if (!_tcsncmp(table, _T("idata"), 5) ||
             !_tcsncmp(table, _T("tdata"), 5))
            continue;  // idata and tdata migrated separately

         if (((g_skipDataMigration || g_skipDataSchemaMigration) && !_tcscmp(table, _T("raw_dci_values"))) ||
             excludedTables.contains(table) ||
             (!includedTables.isEmpty() && !includedTables.contains(table)))
         {
            WriteToTerminalEx(_T("Skipping table \x1b[1m%s\x1b[0m\n"), table);
            continue;
         }

         if (!MigrateTable(table))
            goto cleanup;
      }

      std::pair<const StringList*, const StringList*> data(&excludedTables, &includedTables);
      if (!EnumerateModuleTables(MigrateTableCallback, &data))
         goto cleanup;
   }

   if (!g_skipDataSchemaMigration)
   {
      bool singleTableDestination = (DBMgrMetaDataReadInt32(_T("SingeTablePerfData"), 0) != 0);
      bool singleTableSource = (DBMgrMetaDataReadInt32Ex(s_hdbSource, _T("SingeTablePerfData"), 0) != 0);

      if (singleTableSource && singleTableDestination && !g_skipDataMigration)
      {
         if (g_dbSyntax == DB_SYNTAX_TSDB)
         {
            if (s_sourceSyntax == DB_SYNTAX_TSDB)
            {
               if (!MigrateTable(_T("idata_sc_default")) && !ignoreDataMigrationErrors)
                  goto cleanup;
               if (!MigrateTable(_T("idata_sc_7")) && !ignoreDataMigrationErrors)
                  goto cleanup;
               if (!MigrateTable(_T("idata_sc_30")) && !ignoreDataMigrationErrors)
                  goto cleanup;
               if (!MigrateTable(_T("idata_sc_90")) && !ignoreDataMigrationErrors)
                  goto cleanup;
               if (!MigrateTable(_T("idata_sc_180")) && !ignoreDataMigrationErrors)
                  goto cleanup;
               if (!MigrateTable(_T("idata_sc_other")) && !ignoreDataMigrationErrors)
                  goto cleanup;
               if (!MigrateTable(_T("tdata_sc_default")) && !ignoreDataMigrationErrors)
                  goto cleanup;
               if (!MigrateTable(_T("tdata_sc_7")) && !ignoreDataMigrationErrors)
                  goto cleanup;
               if (!MigrateTable(_T("tdata_sc_30")) && !ignoreDataMigrationErrors)
                  goto cleanup;
               if (!MigrateTable(_T("tdata_sc_90")) && !ignoreDataMigrationErrors)
                  goto cleanup;
               if (!MigrateTable(_T("tdata_sc_180")) && !ignoreDataMigrationErrors)
                  goto cleanup;
               if (!MigrateTable(_T("tdata_sc_other")) && !ignoreDataMigrationErrors)
                  goto cleanup;
            }
            else
            {
               if (!LoadDCIStorageClasses())
                  goto cleanup;
               if (!MigrateSingleDataTableToTSDB(false) && !ignoreDataMigrationErrors)
                  goto cleanup;
               if (!MigrateSingleDataTableToTSDB(true) && !ignoreDataMigrationErrors)
                  goto cleanup;
            }
         }
         else
         {
            if (!MigrateTable(_T("idata")) && !ignoreDataMigrationErrors)
               goto cleanup;
            if (!MigrateTable(_T("tdata")) && !ignoreDataMigrationErrors)
               goto cleanup;
         }
      }
      else if (singleTableSource && !singleTableDestination)
      {
         if (!MigrateDataTablesFromSingleTable(ignoreDataMigrationErrors))
            goto cleanup;
      }
      else if (!singleTableSource && singleTableDestination && !g_skipDataMigration)
      {
         if (g_dbSyntax == DB_SYNTAX_TSDB)
         {
            if (!LoadDCIStorageClasses())
               goto cleanup;
         }
         if (!MigrateDataTablesToSingleTable(ignoreDataMigrationErrors))
            goto cleanup;
      }
      else if (!singleTableSource && !singleTableDestination)
      {
         if (!MigrateDataTables(ignoreDataMigrationErrors))
            goto cleanup;
      }
   }

   success = true;

cleanup:
   if (s_hdbSource != nullptr)
      DBDisconnect(s_hdbSource);
   if (s_driver != nullptr)
      DBUnloadDriver(s_driver);
   return success;
}

/**
 * Migrate database
 */
void MigrateDatabase(const TCHAR *sourceConfig, TCHAR *destConfFields, const StringList& excludedTables, const StringList& includedTables, bool ignoreDataMigrationErrors)
{
   // Load source config
	Config config;
	if (!config.loadIniConfig(sourceConfig, _T("server")) || !config.parseTemplate(_T("server"), m_cfgTemplate))
   {
      _tprintf(_T("Error loading source configuration from %s\n"), sourceConfig);
      return;
   }

	TCHAR sourceConfFields[2048];
	_sntprintf(sourceConfFields, 2048, _T("\tDriver: %s\n\tDB Name: %s\n\tDB Server: %s\n\tDB Login: %s"), s_dbDriver, s_dbName, s_dbServer, s_dbLogin);

	StringBuffer options;
	if (g_dataOnlyMigration)
	{
	   options.append(_T("\tData only migration"));
	}
	else
	{
	   options.append(_T("\tSkip collected data........: "));
	   options.append(g_skipDataMigration ? _T("yes\n") : _T("no\n"));
      options.append(_T("\tSkip data collection schema: "));
      options.append(g_skipDataSchemaMigration ? _T("yes") : _T("no"));
      if (!includedTables.isEmpty())
      {
         options.append(_T("\n\tMigrate only these tables..: "));
         options.appendPreallocated(includedTables.join(_T(", ")));
      }
      else if (!excludedTables.isEmpty())
      {
         options.append(_T("\n\tSkip tables................: "));
         options.appendPreallocated(excludedTables.join(_T(", ")));
      }
	}

	if (!GetYesNo(L"Source:\n%s\n\nTarget:\n%s\n\nOptions:\n%s\n\nConfirm database migration?", sourceConfFields, destConfFields, options.cstr()))
	   return;

   DecryptPassword(s_dbLogin, s_dbPassword, s_dbPassword, MAX_PASSWORD);
   bool success = ImportOrMigrateDatabase(excludedTables, includedTables, ignoreDataMigrationErrors);
   WriteToTerminal(success ? L"Database migration completed.\n" : L"Database migration failed.\n");
}

/**
 * Migrate database
 */
void ImportDatabase(const char *file, const StringList& excludedTables, const StringList& includedTables, bool ignoreDataMigrationErrors)
{
   wcscpy(s_dbDriver, L"sqlite.ddr");
   MultiByteToWideCharSysLocale(file, s_dbName, MAX_DB_NAME);
   s_import = true;
   bool success = ImportOrMigrateDatabase(excludedTables, includedTables, ignoreDataMigrationErrors);
   WriteToTerminal(success ? L"Database import completed.\n" : L"Database import failed.\n");
}
