/*
** nxdbmgr - NetXMS database manager
** Copyright (C) 2004-2016 Victor Kirhenshtein
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
   WriteToTerminalEx(_T("Connected to source database\n"));

   // Get database syntax
	s_sourceSyntax = DBGetSyntax(s_hdbSource);
	if (s_sourceSyntax == DB_SYNTAX_UNKNOWN)
	{
      _tprintf(_T("Unable to determine source database syntax\n"));
      return false;
   }

   // Check source schema version
	INT32 major, minor;
   if (!DBGetSchemaVersion(s_hdbSource, &major, &minor))
   {
      _tprintf(_T("Unable to determine source database version.\n"));
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

	if (!DBBegin(g_hCoreDB))
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
      DBRollback(g_hCoreDB);
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

   DB_STATEMENT hStmt = DBPrepareEx(g_hCoreDB, query, errorText);
   if (hStmt != NULL)
   {
      success = true;
      int rows = 0, totalRows = 0;
      while(DBFetch(hResult))
      {
			for(int i = 0; i < columnCount; i++)
         {
            DBBind(hStmt, i + 1, DB_SQLTYPE_VARCHAR, DBGetField(hResult, i, NULL, 0), DB_BIND_DYNAMIC);
         }
         if (!SQLExecute(hStmt))
         {
            _tprintf(_T("Failed input record:\n"));
            for(int i = 0; i < columnCount; i++)
            {
               DBGetColumnName(hResult, i, buffer, 256);
               TCHAR *value = DBGetField(hResult, i, NULL, 0);
               _tprintf(_T("   %s = \"%s\"\n"), buffer, CHECK_NULL(value));
               safe_free(value);
            }
            success = false;
            break;
         }

         rows++;
         if (rows >= g_migrationTxnSize)
         {
            rows = 0;
            DBCommit(g_hCoreDB);
            DBBegin(g_hCoreDB);
         }

         totalRows++;
         if ((totalRows & 0xFF) == 0)
         {
            _tprintf(_T("%8d\r"), totalRows);
            fflush(stdout);
         }
      }

      if (success)
         DBCommit(g_hCoreDB);
      else
         DBRollback(g_hCoreDB);
      DBFreeStatement(hStmt);
   }
   else
   {
		_tprintf(_T("ERROR: cannot prepare INSERT statement (%s)\n"), errorText);
      DBRollback(g_hCoreDB);
   }
   DBFreeResult(hResult);
	return success;
}

/**
 * Migrate data tables
 */
static bool MigrateDataTables()
{
	int i, count;
	TCHAR buffer[1024];

	DB_RESULT hResult = SQLSelect(_T("SELECT id FROM nodes"));
	if (hResult == NULL)
		return FALSE;

	// Create and import idata_xx and tdata_xx tables for each node in "nodes" table
	count = DBGetNumRows(hResult);
	for(i = 0; i < count; i++)
	{
		DWORD id = DBGetFieldULong(hResult, i, 0);
      if (!g_dataOnlyMigration)
      {
		   if (!CreateIDataTable(id))
			   break;	// Failed to create idata_xx table
      }

      if (!g_skipDataMigration)
      {
		   _sntprintf(buffer, 1024, _T("idata_%d"), id);
		   if (!MigrateTable(buffer))
			   break;
      }

      if (!g_dataOnlyMigration)
      {
         if (!CreateTDataTable(id))
			   break;	// Failed to create tdata tables
      }

      if (!g_skipDataMigration)
      {
		   _sntprintf(buffer, 1024, _T("tdata_%d"), id);
		   if (!MigrateTable(buffer))
			   break;
      }
	}

	DBFreeResult(hResult);
	return i == count;
}

/**
 * Migrate database
 */
void MigrateDatabase(const TCHAR *sourceConfig, TCHAR *destConfFields)
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
	_sntprintf(sourceConfFields, 2048, _T("\tDB Name: %s\n\tDB Server: %s\n\tDB Login: %s"), s_dbName, s_dbServer, s_dbLogin);
	if (!GetYesNo(_T("Source: \n%s \nCurrent: \n%s\n\nConfirm database migration?"), sourceConfFields, destConfFields))
	{
	   goto cleanup;
	}

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
		   if (!MigrateTable(g_tables[i]))
			   goto cleanup;
	   }
   }

   if (!g_skipDataSchemaMigration)
   {
      if (!MigrateDataTables())
         goto cleanup;
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
