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
static TCHAR s_dbDrvParams[MAX_PATH] = _T("");
static TCHAR s_dbServer[MAX_PATH] = _T("127.0.0.1");
static TCHAR s_dbLogin[MAX_DB_LOGIN] = _T("netxms");
static TCHAR s_dbPassword[MAX_PASSWORD] = _T("");
static TCHAR s_dbName[MAX_DB_NAME] = _T("netxms_db");
static TCHAR s_dbSchema[MAX_DB_NAME] = _T("");
static NX_CFG_TEMPLATE m_cfgTemplate[] =
{
   { _T("DBDriver"), CT_STRING, 0, 0, MAX_PATH, 0, s_dbDriver },
   { _T("DBDrvParams"), CT_STRING, 0, 0, MAX_PATH, 0, s_dbDrvParams },
   { _T("DBLogin"), CT_STRING, 0, 0, MAX_DB_LOGIN, 0, s_dbLogin },
   { _T("DBName"), CT_STRING, 0, 0, MAX_DB_NAME, 0, s_dbName },
   { _T("DBPassword"), CT_STRING, 0, 0, MAX_PASSWORD, 0, s_dbPassword },
   { _T("DBEncryptedPassword"), CT_STRING, 0, 0, MAX_PASSWORD, 0, s_dbPassword },
   { _T("DBSchema"), CT_STRING, 0, 0, MAX_PATH, 0, s_dbSchema },
   { _T("DBServer"), CT_STRING, 0, 0, MAX_PATH, 0, s_dbServer },
   { _T(""), CT_END_OF_LIST, 0, 0, 0, 0, NULL }
};

/**
 * Source connection
 */
static DB_DRIVER s_driver = NULL;
static DB_HANDLE s_hdbSource = NULL;
static int s_sourceSyntax = DB_SYNTAX_UNKNOWN;

/**
 * Connect to source database
 */
static bool ConnectToSource()
{
	s_driver = DBLoadDriver(s_dbDriver, s_dbDrvParams, false, NULL, NULL);
	if (s_driver == NULL)
   {
      _tprintf(_T("Unable to load and initialize database driver \"%s\"\n"), s_dbDriver);
      return false;
   }
   WriteToTerminalEx(_T("Database driver \x1b[1m%s\x1b[0m loaded\n"), s_dbDriver);

	TCHAR errorText[DBDRV_MAX_ERROR_TEXT];
   s_hdbSource = DBConnect(s_driver, s_dbServer, s_dbName, s_dbLogin, s_dbPassword, s_dbSchema, errorText);
   if (s_hdbSource == NULL)
   {
		_tprintf(_T("Unable to connect to database %s@%s as %s: %s\n"), s_dbName, s_dbServer, s_dbLogin, errorText);
      return false;
   }
   WriteToTerminal(_T("Connected to source database\n"));

   // Get database syntax
	s_sourceSyntax = DBGetSyntax(s_hdbSource);
	if (s_sourceSyntax == DB_SYNTAX_UNKNOWN)
	{
	   WriteToTerminal(_T("Unable to determine source database syntax\n"));
      return false;
   }

   // Check source schema version
	INT32 major, minor;
   if (!DBGetSchemaVersion(s_hdbSource, &major, &minor))
   {
      WriteToTerminal(_T("Unable to determine source database version.\n"));
      return false;
   }
   if ((major > DB_SCHEMA_VERSION_MAJOR) || ((major == DB_SCHEMA_VERSION_MAJOR) && (minor > DB_SCHEMA_VERSION_MINOR)))
   {
      _tprintf(_T("Source database has format version %d.%d, this tool is compiled for version %d.%d.\n")
               _T("You need to upgrade your server before using this database.\n"),
                major, minor, DB_SCHEMA_VERSION_MAJOR, DB_SCHEMA_VERSION_MINOR);
       return false;
   }
   if ((major < DB_SCHEMA_VERSION_MAJOR) || ((major == DB_SCHEMA_VERSION_MAJOR) && (minor < DB_SCHEMA_VERSION_MINOR)))
   {
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
	WriteToTerminalEx(_T("Migrating table \x1b[1m%s\x1b[0m\n"), table);

	if (!DBBegin(g_dbHandle))
	{
		_tprintf(_T("ERROR: unable to start transaction in target database\n"));
      return false;
   }

   bool success = false;
   TCHAR buffer[256], errorText[DBDRV_MAX_ERROR_TEXT];
   _sntprintf(buffer, 256, _T("SELECT * FROM %s"), table);
   DB_UNBUFFERED_RESULT hResult = DBSelectUnbufferedEx(s_hdbSource, buffer, errorText);
   if (hResult == NULL)
   {
		_tprintf(_T("ERROR: unable to read data from source table (%s)\n"), errorText);
      DBRollback(g_dbHandle);
      return false;
   }

   // build INSERT query
   String query = _T("INSERT INTO ");
   query += table;
   query += _T(" (");
	int columnCount = DBGetColumnCount(hResult);
	for(int i = 0; i < columnCount; i++)
	{
		DBGetColumnName(hResult, i, buffer, 256);
		query += buffer;
		query += _T(",");
	}
	query.shrink();
	query += _T(") VALUES (?");
	for(int i = 1; i < columnCount; i++)
      query += _T(",?");
   query += _T(")");

   DB_STATEMENT hStmt = DBPrepareEx(g_dbHandle, query, true, errorText);
   if (hStmt != NULL)
   {
      success = true;
      int rows = 0, totalRows = 0;
      while(DBFetch(hResult))
      {
			for(int i = 0; i < columnCount; i++)
         {
			   TCHAR *value = DBGetField(hResult, i, NULL, 0);
			   if ((value == NULL) || (*value == 0))
			   {
			      char cname[256];
			      DBGetColumnNameA(hResult, i, cname, 256);
               DBBind(hStmt, i + 1, DB_SQLTYPE_VARCHAR, IsColumnFixNeeded(table, cname) ? _T("0") : _T(""), DB_BIND_STATIC);
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
               DBGetColumnName(hResult, i, buffer, 256);
               TCHAR *value = DBGetField(hResult, i, NULL, 0);
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
static bool MigrateDataTables()
{
   IntegerArray<UINT32> *targets = GetDataCollectionTargets();
   if (targets == NULL)
      return false;

	// Create and import idata_xx and tdata_xx tables for each data collection target
	int count = targets->size();
   int i;
	for(i = 0; i < count; i++)
	{
		UINT32 id = targets->get(i);
      if (!g_dataOnlyMigration)
      {
		   if (!CreateIDataTable(id))
			   break;	// Failed to create idata_xx table
      }

      if (!g_skipDataMigration)
      {
         TCHAR table[32];
		   _sntprintf(table, 32, _T("idata_%u"), id);
		   if (!MigrateTable(table))
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
		   if (!MigrateTable(table))
			   break;
      }
	}

	delete targets;
	return i == count;
}

/**
 * Migrate collected data from multi-table to single table configuration
 */
static bool MigrateDataToSingleTable(UINT32 nodeId, bool tdata)
{
   const TCHAR *prefix = tdata ? _T("tdata") : _T("idata");
   WriteToTerminalEx(_T("Migrating table \x1b[1m%s_%u\x1b[0m to \x1b[1m%s\x1b[0m\n"), prefix, nodeId, prefix);

   if (!DBBegin(g_dbHandle))
   {
      _tprintf(_T("ERROR: unable to start transaction in target database\n"));
      return false;
   }

   bool success = false;
   TCHAR buffer[256], errorText[DBDRV_MAX_ERROR_TEXT];
   _sntprintf(buffer, 256, _T("SELECT item_id,%s_timestamp,%s_value FROM %s_%u"),
            prefix, prefix, prefix, nodeId);
   DB_UNBUFFERED_RESULT hResult = DBSelectUnbufferedEx(s_hdbSource, buffer, errorText);
   if (hResult == NULL)
   {
      _tprintf(_T("ERROR: unable to read data from source table (%s)\n"), errorText);
      DBRollback(g_dbHandle);
      return false;
   }

   DB_STATEMENT hStmt = DBPrepareEx(g_dbHandle,
            tdata ?
               _T("INSERT INTO tdata (node_id,item_id,tdata_timestamp,tdata_value) VALUES (?,?,?,?)")
               : _T("INSERT INTO idata (node_id,item_id,idata_timestamp,idata_value) VALUES (?,?,?,?)"),
            true, errorText);
   if (hStmt != NULL)
   {
      success = true;
      int rows = 0, totalRows = 0;
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, nodeId);
      while(DBFetch(hResult))
      {
         DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, DBGetFieldULong(hResult, 0));
         DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, DBGetFieldULong(hResult, 1));
         if (tdata)
         {
            DBBind(hStmt, 4, DB_SQLTYPE_TEXT, DBGetField(hResult, 2, NULL, 0), DB_BIND_DYNAMIC);
         }
         else
         {
            DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, DBGetField(hResult, 2, NULL, 0), DB_BIND_DYNAMIC);
         }
         if (!SQLExecute(hStmt))
         {
            _tprintf(_T("Failed input record:\n"));
            int columnCount = DBGetColumnCount(hResult);
            for(int i = 0; i < columnCount; i++)
            {
               DBGetColumnName(hResult, i, buffer, 256);
               TCHAR *value = DBGetField(hResult, i, NULL, 0);
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
 * Migrate data tables to single table
 */
static bool MigrateDataTablesToSingleTable()
{
   IntegerArray<UINT32> *targets = GetDataCollectionTargets();
   if (targets == NULL)
      return false;

   // Copy data from idata_xx and tdata_xx tables for each node in "nodes" table
   int count = targets->size();
   int i;
   for(i = 0; i < count; i++)
   {
      UINT32 id = targets->get(i);
      if (!MigrateDataToSingleTable(id, false))
         break;
      if (!MigrateDataToSingleTable(id, true))
         break;
   }

   delete targets;
   return i == count;
}

/**
 * Migrate collected data from single table to multi-single table configuration
 */
static bool MigrateDataFromSingleTable(UINT32 nodeId, bool tdata)
{
   const TCHAR *prefix = tdata ? _T("tdata") : _T("idata");
   WriteToTerminalEx(_T("Migrating table \x1b[1m%s_%u\x1b[0m from \x1b[1m%s\x1b[0m\n"), prefix, nodeId, prefix);

   if (!DBBegin(g_dbHandle))
   {
      _tprintf(_T("ERROR: unable to start transaction in target database\n"));
      return false;
   }

   bool success = false;
   TCHAR buffer[256], errorText[DBDRV_MAX_ERROR_TEXT];
   _sntprintf(buffer, 256, _T("SELECT item_id,%s_timestamp,%s_value FROM %s WHERE node_id=%u"),
            prefix, prefix, prefix, nodeId);
   DB_UNBUFFERED_RESULT hResult = DBSelectUnbufferedEx(s_hdbSource, buffer, errorText);
   if (hResult == NULL)
   {
      _tprintf(_T("ERROR: unable to read data from source table (%s)\n"), errorText);
      DBRollback(g_dbHandle);
      return false;
   }

   _sntprintf(buffer, 256, _T("INSERT INTO %s_%u (item_id,%s_timestamp,%s_value) VALUES (?,?,?)"), prefix, nodeId, prefix, prefix);
   DB_STATEMENT hStmt = DBPrepareEx(g_dbHandle, buffer, true, errorText);
   if (hStmt != NULL)
   {
      success = true;
      int rows = 0, totalRows = 0;
      while(DBFetch(hResult))
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, DBGetFieldULong(hResult, 0));
         DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, DBGetFieldULong(hResult, 1));
         if (tdata)
         {
            DBBind(hStmt, 3, DB_SQLTYPE_TEXT, DBGetField(hResult, 2, NULL, 0), DB_BIND_DYNAMIC);
         }
         else
         {
            DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, DBGetField(hResult, 2, NULL, 0), DB_BIND_DYNAMIC);
         }
         if (!SQLExecute(hStmt))
         {
            _tprintf(_T("Failed input record:\n"));
            int columnCount = DBGetColumnCount(hResult);
            for(int i = 0; i < columnCount; i++)
            {
               DBGetColumnName(hResult, i, buffer, 256);
               TCHAR *value = DBGetField(hResult, i, NULL, 0);
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
static bool MigrateDataTablesFromSingleTable()
{
   IntegerArray<UINT32> *targets = GetDataCollectionTargets();
   if (targets == NULL)
      return false;

   // Create and import idata_xx and tdata_xx tables for each data collection target
   int count = targets->size();
   int i;
   for(i = 0; i < count; i++)
   {
      UINT32 id = targets->get(i);

      if (!g_dataOnlyMigration)
      {
         if (!CreateIDataTable(id))
            break;   // Failed to create idata_xx table
      }

      if (!g_skipDataMigration)
      {
         if (!MigrateDataFromSingleTable(id, false))
            break;
      }

      if (!g_dataOnlyMigration)
      {
         if (!CreateTDataTable(id))
            break;   // Failed to create tdata tables
      }

      if (!g_skipDataMigration)
      {
         if (!MigrateDataFromSingleTable(id, true))
            break;
      }
   }

   delete targets;
   return i == count;
}

/**
 * Callback for migrating module table
 */
static bool MigrateTableCallback(const TCHAR *table, void *userData)
{
   return MigrateTable(table);
}

/**
 * Migrate database
 */
void MigrateDatabase(const TCHAR *sourceConfig, TCHAR *destConfFields, bool skipAudit, bool skipAlarms, bool skipEvent, bool skipSysLog, bool skipTrapLog)
{
   bool success = false;

   // Load source config
	Config *config = new Config();
	if (!config->loadIniConfig(sourceConfig, _T("server")) || !config->parseTemplate(_T("server"), m_cfgTemplate))
   {
      _tprintf(_T("Error loading source configuration from %s\n"), sourceConfig);
      goto cleanup;
   }

	TCHAR sourceConfFields[2048];
	_sntprintf(sourceConfFields, 2048, _T("\tDriver: %s\n\tDB Name: %s\n\tDB Server: %s\n\tDB Login: %s"), s_dbDriver, s_dbName, s_dbServer, s_dbLogin);

	TCHAR options[1024];
	if (g_dataOnlyMigration)
	{
	   _tcscpy(options, _T("\tData only migration"));
	}
	else
	{
      _sntprintf(options, 1024,
               _T("\tSkip audit log.............: %s\n")
               _T("\tSkip alarm log.............: %s\n")
               _T("\tSkip event log.............: %s\n")
               _T("\tSkip syslog................: %s\n")
               _T("\tSkip SNMP trap log.........: %s\n")
               _T("\tSkip collected data........: %s\n")
               _T("\tSkip data collection schema: %s"),
               skipAudit ? _T("yes") : _T("no"),
               skipAlarms ? _T("yes") : _T("no"),
               skipEvent ? _T("yes") : _T("no"),
               skipSysLog ? _T("yes") : _T("no"),
               skipTrapLog ? _T("yes") : _T("no"),
               g_skipDataMigration ? _T("yes") : _T("no"),
               g_skipDataSchemaMigration ? _T("yes") : _T("no")
       );
	}

	if (!GetYesNo(_T("Source:\n%s\n\nTarget:\n%s\n\nOptions:\n%s\n\nConfirm database migration?"), sourceConfFields, destConfFields, options))
	   goto cleanup;

	// Decrypt password
   DecryptPassword(s_dbLogin, s_dbPassword, s_dbPassword, MAX_PASSWORD);

   if (!ConnectToSource())
      goto cleanup;

   if (!g_dataOnlyMigration)
   {
	   if (!ClearDatabase(true))
		   goto cleanup;

	   // Migrate tables
	   for(int i = 0; g_tables[i] != NULL; i++)
	   {
	      if (!_tcscmp(g_tables[i], _T("idata")) ||
             !_tcscmp(g_tables[i], _T("tdata")))
	         continue;  // idata and tdata migrated separately
	      if (skipAudit && !_tcscmp(g_tables[i], _T("audit_log")))
	         continue;
	      if (skipEvent && !_tcscmp(g_tables[i], _T("event_log")))
	         continue;
	      if (skipAlarms && (!_tcscmp(g_tables[i], _T("alarms")) ||
	                         !_tcscmp(g_tables[i], _T("alarm_notes")) ||
	                         !_tcscmp(g_tables[i], _T("alarm_events"))))
	         continue;
	      if (skipTrapLog && !_tcscmp(g_tables[i], _T("snmp_trap_log")))
	         continue;
	      if (skipSysLog && !_tcscmp(g_tables[i], _T("syslog")))
	         continue;
	      if ((g_skipDataMigration || g_skipDataSchemaMigration) &&
	           !_tcscmp(g_tables[i], _T("raw_dci_values")))
	         continue;
		   if (!MigrateTable(g_tables[i]))
			   goto cleanup;
	   }

	   if (!EnumerateModuleTables(MigrateTableCallback, NULL))
         goto cleanup;
   }

   if (!g_skipDataSchemaMigration)
   {
      bool singleTableDestination = (DBMgrMetaDataReadInt32(_T("SingeTablePerfData"), 0) != 0);
      bool singleTableSource = (DBMgrMetaDataReadInt32Ex(s_hdbSource, _T("SingeTablePerfData"), 0) != 0);

      if (singleTableSource && singleTableDestination && !g_skipDataMigration)
      {
         if (!MigrateTable(_T("idata")))
            goto cleanup;
         if (!MigrateTable(_T("tdata")))
            goto cleanup;
      }
      else if (singleTableSource && !singleTableDestination)
      {
         if (!MigrateDataTablesFromSingleTable())
            goto cleanup;
      }
      else if (!singleTableSource && singleTableDestination && !g_skipDataMigration)
      {
         if (!MigrateDataTablesToSingleTable())
            goto cleanup;
      }
      else if (!singleTableSource && !singleTableDestination)
      {
         if (!MigrateDataTables())
            goto cleanup;
      }
   }

	success = true;

cleanup:
   if (s_hdbSource != NULL)
      DBDisconnect(s_hdbSource);
   if (s_driver != NULL)
      DBUnloadDriver(s_driver);
	delete config;
	_tprintf(success ? _T("Database migration complete.\n") : _T("Database migration failed.\n"));
}
