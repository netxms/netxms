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
#include <nms_threads.h>
#include <nxqueue.h>

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
 * Connection pair for parallel migration
 */
struct ConnectionPair
{
   DB_HANDLE hSource;
   DB_HANDLE hDest;
   int slot;
};

/**
 * Parallel migration state
 */
static Mutex s_outputMutex;
static VolatileCounter s_migrationErrors = 0;
static VolatileCounter s_pendingTasks = 0;
static Condition s_completionCondition(false);
static bool s_interactiveOutput = false;
static int s_numWorkerSlots = 0;
static ObjectQueue<ConnectionPair> s_connectionPool(16, Ownership::False);
static Mutex s_completionLock;
static bool s_progressInitialized = false;

/**
 * Update progress display for a worker slot
 */
static void UpdateProgress(int slot, const TCHAR *table, int rows, bool done)
{
   LockGuard lock(s_outputMutex);
   if (s_interactiveOutput && s_numWorkerSlots > 1)
   {
      int linesUp = s_numWorkerSlots - slot;
      _tprintf(_T("\033[%dA\r\033[2K[%d] "), linesUp, slot + 1);
      if (done)
         _tprintf(_T("Migrated \x1b[1m%s\x1b[0m (%d rows)"), table, rows);
      else
         _tprintf(_T("Migrating \x1b[1m%s\x1b[0m %d rows"), table, rows);
      _tprintf(_T("\033[%dB\r"), linesUp);
      fflush(stdout);
   }
   else if (s_numWorkerSlots > 1)
   {
      // Non-interactive multi-threaded: one line per completed table
      if (done)
         _tprintf(_T("Migrating %s... done (%d rows)\n"), table, rows);
   }
   else
   {
      // Single-threaded: simple counter
      if (!done)
      {
         _tprintf(_T("%8d\r"), rows);
         fflush(stdout);
      }
   }
}

/**
 * Initialize progress display area for parallel migration (called once)
 */
static void InitializeProgressDisplay()
{
   if (s_progressInitialized)
      return;
   s_progressInitialized = true;
   WriteToTerminalEx(L"Using %d parallel migration workers\n", g_migrationWorkers);
   if (s_interactiveOutput)
   {
      for (int i = 0; i < s_numWorkerSlots; i++)
         _tprintf(_T("[%d] Idle\n"), i + 1);
   }
}

/**
 * Create additional source database connection
 */
static DB_HANDLE CreateSourceConnection()
{
   wchar_t errorText[DBDRV_MAX_ERROR_TEXT];
   DB_HANDLE h = DBConnect(s_driver, s_dbServer, s_dbName, s_dbLogin, s_dbPassword, s_dbSchema, errorText);
   if (h == nullptr)
      WriteToTerminalEx(L"\x1b[31;1mERROR:\x1b[0m Unable to create additional source connection: %s\n", errorText);
   return h;
}

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
static bool MigrateTable(const wchar_t *table, DB_HANDLE hSource, DB_HANDLE hDest, int progressSlot = -1)
{
   if (progressSlot < 0)
      WriteToTerminalEx(L"%s table \x1b[1m%s\x1b[0m\n", s_import ? L"Importing" : L"Migrating", table);
   else
      UpdateProgress(progressSlot, table, 0, false);

   if (!DBBegin(hDest))
   {
      _tprintf(_T("ERROR: unable to start transaction in target database\n"));
      return false;
   }

   bool success = false;
   TCHAR buffer[512], errorText[DBDRV_MAX_ERROR_TEXT];

   if ((s_sourceSyntax == DB_SYNTAX_TSDB) && IsTimestampConversionNeeded(table))
   {
      _sntprintf(buffer, 512, _T("SELECT * FROM %s WHERE 1=0"), table);
      DB_RESULT hMetaResult = DBSelectEx(hSource, buffer, errorText);
      if (hMetaResult != nullptr)
      {
         StringBuffer columns;
         int columnCount = DBGetColumnCount(hMetaResult);
         for (int i = 0; i < columnCount; i++)
         {
            if (!columns.isEmpty())
               columns.append(_T(","));

            char columnName[256];
            DBGetColumnNameA(hMetaResult, i, columnName, 256);
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
         DBFreeResult(hMetaResult);
         _sntprintf(buffer, 512, _T("SELECT %s FROM %s"), columns.cstr(), table);
      }
      else
      {
         _tprintf(_T("ERROR: unable to read data from source table (%s)\n"), errorText);
         DBRollback(hDest);
         return false;
      }
   }
   else
   {
      _sntprintf(buffer, 512, _T("SELECT * FROM %s"), table);
   }

   DB_UNBUFFERED_RESULT hResult = DBSelectUnbufferedEx(hSource, buffer, errorText);
   if (hResult == nullptr)
   {
      _tprintf(_T("ERROR: unable to read data from source table (%s)\n"), errorText);
      DBRollback(hDest);
      return false;
   }

   int columnCount = DBGetColumnCount(hResult);

   // Pre-compute column metadata
   bool *integerFixNeeded = MemAllocArray<bool>(columnCount);
   bool *tsdbTimestamp = MemAllocArray<bool>(columnCount);
   StringList columnNames;
   for (int i = 0; i < columnCount; i++)
   {
      DBGetColumnName(hResult, i, buffer, 512);
      columnNames.add(buffer);
      char cname[256];
      DBGetColumnNameA(hResult, i, cname, 256);
      integerFixNeeded[i] = IsColumnIntegerFixNeeded(table, cname);
      tsdbTimestamp[i] = (g_dbSyntax == DB_SYNTAX_TSDB) && IsTimestampColumn(table, cname);
   }

   int totalRows = 0;

   if (g_dbSyntax == DB_SYNTAX_ORACLE)
   {
      // Oracle path: prepared statement with batch API
      StringBuffer query = _T("INSERT INTO ");
      query.append(table);
      query.append(_T(" ("));
      for (int i = 0; i < columnCount; i++)
      {
         if (i > 0)
            query.append(_T(","));
         query.append(columnNames.get(i));
      }
      query.append(_T(") VALUES ("));
      for (int i = 0; i < columnCount; i++)
      {
         if (i > 0)
            query.append(_T(","));
         query.append(_T("?"));
      }
      query.append(_T(")"));

      DB_STATEMENT hStmt = DBPrepareEx(hDest, query, true, errorText);
      if (hStmt != nullptr)
      {
         DBOpenBatch(hStmt);
         success = true;
         int rows = 0;
         while (DBFetch(hResult))
         {
            for (int i = 0; i < columnCount; i++)
            {
               TCHAR *value = DBGetField(hResult, i, nullptr, 0);
               if ((value == nullptr) || (*value == 0))
               {
                  DBBind(hStmt, i + 1, DB_SQLTYPE_VARCHAR, integerFixNeeded[i] ? _T("0") : _T(""), DB_BIND_STATIC);
                  MemFree(value);
               }
               else
               {
                  DBBind(hStmt, i + 1, DB_SQLTYPE_VARCHAR, value, DB_BIND_DYNAMIC);
               }
            }
            DBNextBatchRow(hStmt);

            rows++;
            totalRows++;

            if (rows >= g_migrationTxnSize)
            {
               if (!SQLExecute(hStmt))
               {
                  success = false;
                  break;
               }
               DBCommit(hDest);
               DBBegin(hDest);
               DBOpenBatch(hStmt);
               rows = 0;
            }

            if ((totalRows & 0xFF) == 0)
               UpdateProgress(progressSlot, table, totalRows, false);
         }

         if (success && rows > 0)
         {
            if (!SQLExecute(hStmt))
               success = false;
         }

         if (success)
            DBCommit(hDest);
         else
            DBRollback(hDest);
         DBFreeStatement(hStmt);
      }
      else
      {
         _tprintf(_T("ERROR: cannot prepare INSERT statement (%s)\n"), errorText);
         DBRollback(hDest);
      }
   }
   else
   {
      // Non-Oracle path: multi-row INSERT with escaped values
      StringBuffer insertPrefix = _T("INSERT INTO ");
      insertPrefix.append(table);
      insertPrefix.append(_T(" ("));
      for (int i = 0; i < columnCount; i++)
      {
         if (i > 0)
            insertPrefix.append(_T(","));
         insertPrefix.append(columnNames.get(i));
      }
      insertPrefix.append(_T(") VALUES "));

      // MSSQL limits VALUES clause to 1000 rows
      int batchSize = g_migrationTxnSize;
      if ((g_dbSyntax == DB_SYNTAX_MSSQL) && (batchSize > 1000))
         batchSize = 1000;

      StringBuffer query;
      int rowsInBatch = 0, txnRows = 0;
      success = true;

      while (DBFetch(hResult))
      {
         if (rowsInBatch == 0)
            query = insertPrefix;
         else
            query.append(_T(","));

         query.append(_T("("));
         for (int i = 0; i < columnCount; i++)
         {
            if (i > 0)
               query.append(_T(","));

            TCHAR *value = DBGetField(hResult, i, nullptr, 0);
            if ((value == nullptr) || (*value == 0))
            {
               if (integerFixNeeded[i])
                  query.append(_T("'0'"));
               else
                  query.append(_T("''"));
               MemFree(value);
            }
            else
            {
               if (tsdbTimestamp[i])
                  query.append(_T("to_timestamp("));
               query.append(DBPrepareString(hDest, value));
               if (tsdbTimestamp[i])
                  query.append(_T(")"));
               MemFree(value);
            }
         }
         query.append(_T(")"));

         rowsInBatch++;
         totalRows++;
         txnRows++;

         if (rowsInBatch >= batchSize)
         {
            if (!DBQueryEx(hDest, query, errorText))
            {
               _tprintf(_T("ERROR: cannot insert data into table %s (%s)\n"), table, errorText);
               success = false;
               break;
            }
            rowsInBatch = 0;
         }

         if (txnRows >= g_migrationTxnSize)
         {
            DBCommit(hDest);
            DBBegin(hDest);
            txnRows = 0;
         }

         if ((totalRows & 0xFF) == 0)
            UpdateProgress(progressSlot, table, totalRows, false);
      }

      // Flush remaining rows
      if (success && rowsInBatch > 0)
      {
         if (!DBQueryEx(hDest, query, errorText))
         {
            _tprintf(_T("ERROR: cannot insert data into table %s (%s)\n"), table, errorText);
            success = false;
         }
      }

      if (success)
         DBCommit(hDest);
      else
         DBRollback(hDest);
   }

   MemFree(integerFixNeeded);
   MemFree(tsdbTimestamp);
   DBFreeResult(hResult);

   if (progressSlot >= 0)
      UpdateProgress(progressSlot, table, totalRows, true);

   return success;
}

/**
 * Migrate data tables
 */
static bool MigrateDataTables(DB_HANDLE hSource, DB_HANDLE hDest, bool ignoreDataMigrationErrors)
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
         wchar_t table[32];
         nx_swprintf(table, 32, L"idata_%u", id);
         if (!MigrateTable(table, hSource, hDest) && !ignoreDataMigrationErrors)
            break;
      }

      if (!g_dataOnlyMigration)
      {
         if (!CreateTDataTable(id))
            break;   // Failed to create tdata tables
      }

      if (!g_skipDataMigration)
      {
         wchar_t table[32];
         nx_swprintf(table, 32, L"tdata_%u", id);
         if (!MigrateTable(table, hSource, hDest) && !ignoreDataMigrationErrors)
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
static bool LoadDCIStorageClasses(DB_HANDLE hSource, const wchar_t *table)
{
   TCHAR query[256];
   _sntprintf(query, 256, _T("SELECT item_id,retention_time FROM %s"), table);

   DB_RESULT hResult = DBSelect(hSource, query);
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
static bool LoadDCIStorageClasses(DB_HANDLE hSource)
{
   return LoadDCIStorageClasses(hSource, L"items") && LoadDCIStorageClasses(hSource, L"dc_tables");
}

/**
 * Migrate collected data from multi-table to single table configuration
 */
static bool MigrateDataToSingleTable(DB_HANDLE hSource, DB_HANDLE hDest, uint32_t nodeId, bool tdata)
{
   const wchar_t *prefix = tdata ? L"tdata" : L"idata";
   s_outputMutex.lock();
   WriteToTerminalEx(_T("%s table \x1b[1m%s_%u\x1b[0m to \x1b[1m%s\x1b[0m\n"), s_import ? _T("Importing") : _T("Migrating"), prefix, nodeId, prefix);
   s_outputMutex.unlock();

   if (!DBBegin(hDest))
   {
      _tprintf(_T("ERROR: unable to start transaction in target database\n"));
      return false;
   }

   bool success = false;
   TCHAR buffer[256], errorText[DBDRV_MAX_ERROR_TEXT];
   _sntprintf(buffer, 256, _T("SELECT item_id,%s_timestamp,%s_value%s FROM %s_%u"),
            prefix, prefix, tdata ? _T("") : _T(",raw_value"), prefix, nodeId);
   DB_UNBUFFERED_RESULT hResult = DBSelectUnbufferedEx(hSource, buffer, errorText);
   if (hResult == nullptr)
   {
      _tprintf(_T("ERROR: unable to read data from source table (%s)\n"), errorText);
      DBRollback(hDest);
      return false;
   }

   DB_STATEMENT hStmt = DBPrepareEx(hDest,
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
            s_outputMutex.lock();
            _tprintf(_T("Failed input record:\n"));
            int columnCount = DBGetColumnCount(hResult);
            for(int i = 0; i < columnCount; i++)
            {
               DBGetColumnName(hResult, i, buffer, 256);
               TCHAR *value = DBGetField(hResult, i, nullptr, 0);
               _tprintf(_T("   %s = \"%s\"\n"), buffer, CHECK_NULL(value));
               MemFree(value);
            }
            s_outputMutex.unlock();
            success = false;
            break;
         }

         rows++;
         if (rows >= g_migrationTxnSize)
         {
            rows = 0;
            DBCommit(hDest);
            DBBegin(hDest);
         }

         totalRows++;
         if ((totalRows & 0xFF) == 0 && s_numWorkerSlots <= 1)
         {
            _tprintf(_T("%8d\r"), totalRows);
            fflush(stdout);
         }
      }

      if (success)
         DBCommit(hDest);
      else
         DBRollback(hDest);
      DBFreeStatement(hStmt);
   }
   else
   {
      s_outputMutex.lock();
      _tprintf(_T("ERROR: cannot prepare INSERT statement (%s)\n"), errorText);
      s_outputMutex.unlock();
      DBRollback(hDest);
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
static bool MigrateDataToSingleTable_TSDB(DB_HANDLE hdbSource, DB_HANDLE hDest, uint32_t nodeId, bool tdata)
{
   const wchar_t *prefix = tdata ? L"tdata" : L"idata";
   s_outputMutex.lock();
   WriteToTerminalEx(L"%s table \x1b[1m%s_%u\x1b[0m to \x1b[1m%s\x1b[0m\n", s_import ? L"Importing" : L"Migrating", prefix, nodeId, prefix);
   s_outputMutex.unlock();

   bool success = false;
   TCHAR buffer[256], errorText[DBDRV_MAX_ERROR_TEXT];
   _sntprintf(buffer, 256, _T("SELECT item_id,%s_timestamp,%s_value%s FROM %s_%u"), prefix, prefix, tdata ? _T("") : _T(",raw_value"), prefix, nodeId);
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
      wchar_t *value = DBGetField(hResult, 2, nullptr, 0);
      query.append(DBPrepareString(hDest, value));
      MemFree(value);
      if (!tdata)
      {
         query.append(L',');
         value = DBGetField(hResult, 3, nullptr, 0);
         query.append(DBPrepareString(hDest, value));
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

            queries[i].append(L" ON CONFLICT DO NOTHING");
            DBBegin(hDest);
            if (DBQueryEx(hDest, queries[i], errorText))
            {
               DBCommit(hDest);
            }
            else
            {
               s_outputMutex.lock();
               WriteToTerminalEx(L"ERROR: unable to insert data to destination table (%s)\n", errorText);
               s_outputMutex.unlock();
               DBRollback(hDest);
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
      if ((totalRows & 0xFF) == 0 && s_numWorkerSlots <= 1)
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

         queries[i].append(L" ON CONFLICT DO NOTHING");
         DBBegin(hDest);
         if (DBQueryEx(hDest, queries[i], errorText))
         {
            DBCommit(hDest);
         }
         else
         {
            s_outputMutex.lock();
            WriteToTerminalEx(L"ERROR: unable to insert data to destination table (%s)\n", errorText);
            s_outputMutex.unlock();
            DBRollback(hDest);
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
static bool MigrateSingleDataTableToTSDB(DB_HANDLE hSource, DB_HANDLE hDest, bool tdata)
{
   const TCHAR *prefix = tdata ? _T("tdata") : _T("idata");
   WriteToTerminalEx(_T("%s table \x1b[1m%s\x1b[0m\n"), s_import ? _T("Importing") : _T("Migrating"), prefix);

   bool success = false;
   TCHAR buffer[256], errorText[DBDRV_MAX_ERROR_TEXT];
   _sntprintf(buffer, 256, _T("SELECT item_id,%s_timestamp,%s_value%s FROM %s"), prefix, prefix, tdata ? _T("") : _T(",raw_value"), prefix);
   DB_UNBUFFERED_RESULT hResult = DBSelectUnbufferedEx(hSource, buffer, errorText);
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
      wchar_t *value = DBGetField(hResult, 2, nullptr, 0);
      query.append(DBPrepareString(hDest, value));
      MemFree(value);
      if (!tdata)
      {
         query.append(_T(','));
         value = DBGetField(hResult, 3, nullptr, 0);
         query.append(DBPrepareString(hDest, value));
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

            queries[i].append(L" ON CONFLICT DO NOTHING");
            DBBegin(hDest);
            if (DBQueryEx(hDest, queries[i], errorText))
            {
               DBCommit(hDest);
            }
            else
            {
               _tprintf(_T("ERROR: unable to insert data to destination table (%s)\n"), errorText);
               DBRollback(hDest);
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

         queries[i].append(L" ON CONFLICT DO NOTHING");
         DBBegin(hDest);
         if (DBQueryEx(hDest, queries[i], errorText))
         {
            DBCommit(hDest);
         }
         else
         {
            _tprintf(_T("ERROR: unable to insert data to destination table (%s)\n"), errorText);
            DBRollback(hDest);
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
static bool MigrateDataTablesToSingleTable(DB_HANDLE hSource, DB_HANDLE hDest, bool ignoreDataMigrationErrors)
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
         if (!MigrateDataToSingleTable_TSDB(hSource, hDest, id, false) && !ignoreDataMigrationErrors)
            break;
         if (!MigrateDataToSingleTable_TSDB(hSource, hDest, id, true) && !ignoreDataMigrationErrors)
            break;
      }
      else
      {
         if (!MigrateDataToSingleTable(hSource, hDest, id, false) && !ignoreDataMigrationErrors)
            break;
         if (!MigrateDataToSingleTable(hSource, hDest, id, true) && !ignoreDataMigrationErrors)
            break;
      }
   }

   delete targets;
   return i == count;
}

/**
 * Migrate collected data from single table to multi-single table configuration
 */
static bool MigrateDataFromSingleTable(DB_HANDLE hSource, DB_HANDLE hDest, uint32_t nodeId, bool tdata)
{
   const TCHAR *prefix = tdata ? _T("tdata") : _T("idata");
   s_outputMutex.lock();
   WriteToTerminalEx(_T("%s table \x1b[1m%s_%u\x1b[0m from \x1b[1m%s\x1b[0m\n"), s_import ? _T("Importing") : _T("Migrating"), prefix, nodeId, prefix);
   s_outputMutex.unlock();

   if (!DBBegin(hDest))
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
   DB_UNBUFFERED_RESULT hResult = DBSelectUnbufferedEx(hSource, buffer, errorText);
   if (hResult == nullptr)
   {
      _tprintf(_T("ERROR: unable to read data from source table (%s)\n"), errorText);
      DBRollback(hDest);
      return false;
   }

   if (tdata)
      _sntprintf(buffer, 256, _T("INSERT INTO tdata_%u (item_id,tdata_timestamp,tdata_value) VALUES (?,?,?)"), nodeId);
   else
      _sntprintf(buffer, 256, _T("INSERT INTO idata_%u (item_id,idata_timestamp,idata_value,raw_value) VALUES (?,?,?,?)"), nodeId);
   DB_STATEMENT hStmt = DBPrepareEx(hDest, buffer, true, errorText);
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
            s_outputMutex.lock();
            WriteToTerminal(L"Failed input record:\n");
            int columnCount = DBGetColumnCount(hResult);
            for(int i = 0; i < columnCount; i++)
            {
               DBGetColumnName(hResult, i, buffer, 256);
               TCHAR *value = DBGetField(hResult, i, nullptr, 0);
               _tprintf(_T("   %s = \"%s\"\n"), buffer, CHECK_NULL(value));
               MemFree(value);
            }
            s_outputMutex.unlock();
            success = false;
            break;
         }

         rows++;
         if (rows >= g_migrationTxnSize)
         {
            rows = 0;
            DBCommit(hDest);
            DBBegin(hDest);
         }

         totalRows++;
         if ((totalRows & 0xFF) == 0 && s_numWorkerSlots <= 1)
         {
            _tprintf(_T("%8d\r"), totalRows);
            fflush(stdout);
         }
      }

      if (success)
         DBCommit(hDest);
      else
         DBRollback(hDest);
      DBFreeStatement(hStmt);
   }
   else
   {
      s_outputMutex.lock();
      WriteToTerminalEx(L"ERROR: cannot prepare INSERT statement (%s)\n", errorText);
      s_outputMutex.unlock();
      DBRollback(hDest);
   }
   DBFreeResult(hResult);
   return success;
}

/**
 * Migrate data tables from single table
 */
static bool MigrateDataTablesFromSingleTable(DB_HANDLE hSource, DB_HANDLE hDest, bool ignoreDataMigrationErrors)
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
         if (!MigrateDataFromSingleTable(hSource, hDest, id, false) && !ignoreDataMigrationErrors)
            break;
      }

      if (!g_dataOnlyMigration)
      {
         if (!CreateTDataTable(id))
            break;   // Failed to create tdata tables
      }

      if (!g_skipDataMigration)
      {
         if (!MigrateDataFromSingleTable(hSource, hDest, id, true) && !ignoreDataMigrationErrors)
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
   return MigrateTable(table, s_hdbSource, g_dbHandle);
}

/**
 * Worker function for parallel table migration
 */
static void ParallelMigrationWorker(void *arg)
{
   TCHAR *tableName = static_cast<TCHAR*>(arg);

   // Acquire connection pair from pool
   ConnectionPair *pair = s_connectionPool.getOrBlock();

   bool result = MigrateTable(tableName, pair->hSource, pair->hDest, pair->slot);
   if (!result)
      InterlockedIncrement(&s_migrationErrors);

   // Return connection pair to pool
   s_connectionPool.put(pair);

   MemFree(tableName);

   // Signal completion
   if (InterlockedDecrement(&s_pendingTasks) == 0)
   {
      s_completionLock.lock();
      s_completionCondition.set();
      s_completionLock.unlock();
   }
}

/**
 * Dispatch a table migration to the thread pool
 */
static void DispatchTableMigration(ThreadPool *threadPool, const TCHAR *table)
{
   TCHAR *tableCopy = MemCopyString(table);
   InterlockedIncrement(&s_pendingTasks);
   ThreadPoolExecute(threadPool, ParallelMigrationWorker, tableCopy);
}

/**
 * Wait for all pending parallel migration tasks to complete
 */
static void WaitForPendingMigrations()
{
   while (s_pendingTasks > 0)
      ThreadSleepMs(50);
}

/**
 * Per-node data migration function type
 */
typedef bool (*DataMigrationFunction)(DB_HANDLE, DB_HANDLE, uint32_t, bool);

/**
 * Per-node data migration task
 */
struct DataMigrationTask
{
   uint32_t nodeId;
   bool tdata;
   DataMigrationFunction migrateFunc;
};

/**
 * Worker function for parallel per-node data migration
 */
static void ParallelDataMigrationWorker(void *arg)
{
   DataMigrationTask *task = static_cast<DataMigrationTask*>(arg);

   // Acquire connection pair from pool
   ConnectionPair *pair = s_connectionPool.getOrBlock();

   bool result = task->migrateFunc(pair->hSource, pair->hDest, task->nodeId, task->tdata);
   if (!result)
      InterlockedIncrement(&s_migrationErrors);

   // Return connection pair to pool
   s_connectionPool.put(pair);

   delete task;

   // Signal completion
   if (InterlockedDecrement(&s_pendingTasks) == 0)
   {
      s_completionLock.lock();
      s_completionCondition.set();
      s_completionLock.unlock();
   }
}

/**
 * Dispatch a per-node data migration to the thread pool
 */
static void DispatchDataMigration(ThreadPool *threadPool, uint32_t nodeId, bool tdata, DataMigrationFunction func)
{
   DataMigrationTask *task = new DataMigrationTask();
   task->nodeId = nodeId;
   task->tdata = tdata;
   task->migrateFunc = func;
   InterlockedIncrement(&s_pendingTasks);
   ThreadPoolExecute(threadPool, ParallelDataMigrationWorker, task);
}

/**
 * Do database import or migration
 */
static bool ImportOrMigrateDatabase(const StringList& excludedTables, const StringList& includedTables, bool ignoreDataMigrationErrors)
{
   if (!ConnectToSource())
      return false;

   bool useParallel = !s_import && (g_migrationWorkers > 1);
   ThreadPool *threadPool = nullptr;
   int numConnections = 0;
   ObjectArray<ConnectionPair> poolConnections(16, 16, Ownership::False);

   if (useParallel)
   {
      // Create connection pool with N source+destination pairs
      bool poolCreated = true;
      for (int i = 0; i < g_migrationWorkers; i++)
      {
         DB_HANDLE hSrc = CreateSourceConnection();
         if (hSrc == nullptr)
         {
            poolCreated = false;
            break;
         }
         DB_HANDLE hDst = ConnectToDatabase();
         if (hDst == nullptr)
         {
            DBDisconnect(hSrc);
            poolCreated = false;
            break;
         }
         ConnectionPair *pair = new ConnectionPair();
         pair->hSource = hSrc;
         pair->hDest = hDst;
         pair->slot = i;
         s_connectionPool.put(pair);
         poolConnections.add(pair);
         numConnections++;
      }

      if (!poolCreated)
      {
         _tprintf(_T("WARNING: failed to create all parallel connections, falling back to single-threaded migration\n"));
         // Drain connection pool and close connections
         ConnectionPair *pair;
         while ((pair = s_connectionPool.get()) != nullptr)
         {
            DBDisconnect(pair->hSource);
            DBDisconnect(pair->hDest);
            delete pair;
         }
         poolConnections.clear();
         numConnections = 0;
         useParallel = false;
      }
      else
      {
         threadPool = ThreadPoolCreate(_T("MIGRATE"), g_migrationWorkers, g_migrationWorkers);
         s_numWorkerSlots = g_migrationWorkers;
         s_interactiveOutput = _isatty(_fileno(stdout)) != 0;
         s_migrationErrors = 0;
         s_pendingTasks = 0;
         s_completionCondition.reset();
      }
   }

   bool success = false;
   if (!g_dataOnlyMigration)
   {
      // Do not clear destination if only explicitly listed tables are being imported/migrated
      if (includedTables.isEmpty())
      {
         if (!ClearDatabase(!s_import))
            goto cleanup;
      }

      if (useParallel)
      {
         InitializeProgressDisplay();

         // Parallel table migration
         for (int i = 0; g_tables[i] != nullptr; i++)
         {
            const TCHAR *table = g_tables[i];
            if (!_tcsncmp(table, _T("idata"), 5) || !_tcsncmp(table, _T("tdata"), 5))
               continue;

            if (((g_skipDataMigration || g_skipDataSchemaMigration) && !_tcscmp(table, _T("raw_dci_values"))) ||
                excludedTables.contains(table) ||
                (!includedTables.isEmpty() && !includedTables.contains(table)))
            {
               if (!s_interactiveOutput)
                  _tprintf(_T("Skipping table %s\n"), table);
               continue;
            }

            if (s_migrationErrors > 0)
               goto cleanup;

            DispatchTableMigration(threadPool, table);
         }

         WaitForPendingMigrations();

         if (s_migrationErrors > 0)
            goto cleanup;

         // Module tables - migrate serially using main connection
         std::pair<const StringList*, const StringList*> data(&excludedTables, &includedTables);
         if (!EnumerateModuleTables(MigrateTableCallback, &data))
            goto cleanup;
      }
      else
      {
         // Serial table migration
         for (int i = 0; g_tables[i] != nullptr; i++)
         {
            const TCHAR *table = g_tables[i];
            if (!_tcsncmp(table, _T("idata"), 5) || !_tcsncmp(table, _T("tdata"), 5))
               continue;

            if (((g_skipDataMigration || g_skipDataSchemaMigration) && !_tcscmp(table, _T("raw_dci_values"))) ||
                excludedTables.contains(table) ||
                (!includedTables.isEmpty() && !includedTables.contains(table)))
            {
               WriteToTerminalEx(L"Skipping table \x1b[1m%s\x1b[0m\n", table);
               continue;
            }

            if (!MigrateTable(table, s_hdbSource, g_dbHandle))
               goto cleanup;
         }

         std::pair<const StringList*, const StringList*> data(&excludedTables, &includedTables);
         if (!EnumerateModuleTables(MigrateTableCallback, &data))
            goto cleanup;
      }
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
               if (useParallel)
               {
                  InitializeProgressDisplay();
                  static const TCHAR *tsdbDataTables[] = {
                     _T("idata_sc_default"), _T("idata_sc_7"), _T("idata_sc_30"),
                     _T("idata_sc_90"), _T("idata_sc_180"), _T("idata_sc_other"),
                     _T("tdata_sc_default"), _T("tdata_sc_7"), _T("tdata_sc_30"),
                     _T("tdata_sc_90"), _T("tdata_sc_180"), _T("tdata_sc_other"),
                     nullptr
                  };
                  for (int i = 0; tsdbDataTables[i] != nullptr; i++)
                     DispatchTableMigration(threadPool, tsdbDataTables[i]);

                  WaitForPendingMigrations();
                  if (s_migrationErrors > 0 && !ignoreDataMigrationErrors)
                     goto cleanup;
               }
               else
               {
                  if (!MigrateTable(_T("idata_sc_default"), s_hdbSource, g_dbHandle) && !ignoreDataMigrationErrors)
                     goto cleanup;
                  if (!MigrateTable(_T("idata_sc_7"), s_hdbSource, g_dbHandle) && !ignoreDataMigrationErrors)
                     goto cleanup;
                  if (!MigrateTable(_T("idata_sc_30"), s_hdbSource, g_dbHandle) && !ignoreDataMigrationErrors)
                     goto cleanup;
                  if (!MigrateTable(_T("idata_sc_90"), s_hdbSource, g_dbHandle) && !ignoreDataMigrationErrors)
                     goto cleanup;
                  if (!MigrateTable(_T("idata_sc_180"), s_hdbSource, g_dbHandle) && !ignoreDataMigrationErrors)
                     goto cleanup;
                  if (!MigrateTable(_T("idata_sc_other"), s_hdbSource, g_dbHandle) && !ignoreDataMigrationErrors)
                     goto cleanup;
                  if (!MigrateTable(_T("tdata_sc_default"), s_hdbSource, g_dbHandle) && !ignoreDataMigrationErrors)
                     goto cleanup;
                  if (!MigrateTable(_T("tdata_sc_7"), s_hdbSource, g_dbHandle) && !ignoreDataMigrationErrors)
                     goto cleanup;
                  if (!MigrateTable(_T("tdata_sc_30"), s_hdbSource, g_dbHandle) && !ignoreDataMigrationErrors)
                     goto cleanup;
                  if (!MigrateTable(_T("tdata_sc_90"), s_hdbSource, g_dbHandle) && !ignoreDataMigrationErrors)
                     goto cleanup;
                  if (!MigrateTable(_T("tdata_sc_180"), s_hdbSource, g_dbHandle) && !ignoreDataMigrationErrors)
                     goto cleanup;
                  if (!MigrateTable(_T("tdata_sc_other"), s_hdbSource, g_dbHandle) && !ignoreDataMigrationErrors)
                     goto cleanup;
               }
            }
            else
            {
               if (!LoadDCIStorageClasses(s_hdbSource))
                  goto cleanup;
               if (!MigrateSingleDataTableToTSDB(s_hdbSource, g_dbHandle, false) && !ignoreDataMigrationErrors)
                  goto cleanup;
               if (!MigrateSingleDataTableToTSDB(s_hdbSource, g_dbHandle, true) && !ignoreDataMigrationErrors)
                  goto cleanup;
            }
         }
         else
         {
            if (useParallel)
            {
               InitializeProgressDisplay();
               DispatchTableMigration(threadPool, _T("idata"));
               DispatchTableMigration(threadPool, _T("tdata"));
               WaitForPendingMigrations();
               if (s_migrationErrors > 0 && !ignoreDataMigrationErrors)
                  goto cleanup;
            }
            else
            {
               if (!MigrateTable(_T("idata"), s_hdbSource, g_dbHandle) && !ignoreDataMigrationErrors)
                  goto cleanup;
               if (!MigrateTable(_T("tdata"), s_hdbSource, g_dbHandle) && !ignoreDataMigrationErrors)
                  goto cleanup;
            }
         }
      }
      else if (singleTableSource && !singleTableDestination)
      {
         if (useParallel)
         {
            IntegerArray<uint32_t> *targets = GetDataCollectionTargets();
            if (targets == nullptr)
               goto cleanup;

            // Phase 1: Create tables serially (DDL on destination)
            bool tableCreationOk = true;
            if (!g_dataOnlyMigration)
            {
               for (int ti = 0; ti < targets->size(); ti++)
               {
                  uint32_t id = targets->get(ti);
                  if (!CreateIDataTable(id) || !CreateTDataTable(id))
                  {
                     tableCreationOk = false;
                     break;
                  }
               }
            }
            if (!tableCreationOk)
            {
               delete targets;
               goto cleanup;
            }

            // Phase 2: Dispatch data migrations in parallel
            if (!g_skipDataMigration)
            {
               InitializeProgressDisplay();
               for (int ti = 0; ti < targets->size(); ti++)
               {
                  uint32_t id = targets->get(ti);
                  DispatchDataMigration(threadPool, id, false, MigrateDataFromSingleTable);
                  DispatchDataMigration(threadPool, id, true, MigrateDataFromSingleTable);
               }
               WaitForPendingMigrations();
               if (s_migrationErrors > 0 && !ignoreDataMigrationErrors)
               {
                  delete targets;
                  goto cleanup;
               }
            }
            delete targets;
         }
         else
         {
            if (!MigrateDataTablesFromSingleTable(s_hdbSource, g_dbHandle, ignoreDataMigrationErrors))
               goto cleanup;
         }
      }
      else if (!singleTableSource && singleTableDestination && !g_skipDataMigration)
      {
         if (g_dbSyntax == DB_SYNTAX_TSDB)
         {
            if (!LoadDCIStorageClasses(s_hdbSource))
               goto cleanup;
         }
         if (useParallel)
         {
            IntegerArray<uint32_t> *targets = GetDataCollectionTargets();
            if (targets == nullptr)
               goto cleanup;

            InitializeProgressDisplay();
            for (int ti = 0; ti < targets->size(); ti++)
            {
               uint32_t id = targets->get(ti);
               if (g_dbSyntax == DB_SYNTAX_TSDB)
               {
                  DispatchDataMigration(threadPool, id, false, MigrateDataToSingleTable_TSDB);
                  DispatchDataMigration(threadPool, id, true, MigrateDataToSingleTable_TSDB);
               }
               else
               {
                  DispatchDataMigration(threadPool, id, false, MigrateDataToSingleTable);
                  DispatchDataMigration(threadPool, id, true, MigrateDataToSingleTable);
               }
            }
            WaitForPendingMigrations();
            if (s_migrationErrors > 0 && !ignoreDataMigrationErrors)
            {
               delete targets;
               goto cleanup;
            }
            delete targets;
         }
         else
         {
            if (!MigrateDataTablesToSingleTable(s_hdbSource, g_dbHandle, ignoreDataMigrationErrors))
               goto cleanup;
         }
      }
      else if (!singleTableSource && !singleTableDestination)
      {
         if (useParallel)
         {
            IntegerArray<uint32_t> *targets = GetDataCollectionTargets();
            if (targets == nullptr)
               goto cleanup;

            // Phase 1: Create tables serially (DDL on destination)
            bool tableCreationOk = true;
            if (!g_dataOnlyMigration)
            {
               for (int ti = 0; ti < targets->size(); ti++)
               {
                  uint32_t id = targets->get(ti);
                  if (!CreateIDataTable(id) || !CreateTDataTable(id))
                  {
                     tableCreationOk = false;
                     break;
                  }
               }
            }
            if (!tableCreationOk)
            {
               delete targets;
               goto cleanup;
            }

            // Phase 2: Dispatch data migrations in parallel
            if (!g_skipDataMigration)
            {
               InitializeProgressDisplay();
               for (int ti = 0; ti < targets->size(); ti++)
               {
                  uint32_t id = targets->get(ti);
                  wchar_t table[32];
                  nx_swprintf(table, 32, L"idata_%u", id);
                  DispatchTableMigration(threadPool, table);
                  nx_swprintf(table, 32, L"tdata_%u", id);
                  DispatchTableMigration(threadPool, table);
               }
               WaitForPendingMigrations();
               if (s_migrationErrors > 0 && !ignoreDataMigrationErrors)
               {
                  delete targets;
                  goto cleanup;
               }
            }
            delete targets;
         }
         else
         {
            if (!MigrateDataTables(s_hdbSource, g_dbHandle, ignoreDataMigrationErrors))
               goto cleanup;
         }
      }
   }

   success = true;

cleanup:
   // Clean up parallel infrastructure
   if (threadPool != nullptr)
   {
      // Wait for any remaining workers before destroying pool
      WaitForPendingMigrations();
      ThreadPoolDestroy(threadPool);
   }

   // Close and free connection pool
   for (int i = 0; i < poolConnections.size(); i++)
   {
      ConnectionPair *pair = poolConnections.get(i);
      DBDisconnect(pair->hSource);
      DBDisconnect(pair->hDest);
      delete pair;
   }

   // Move cursor below progress area for clean output
   if (s_interactiveOutput && s_numWorkerSlots > 1)
      _tprintf(_T("\n"));

   s_numWorkerSlots = 0;
   s_interactiveOutput = false;
   s_progressInitialized = false;

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

	if (!GetYesNo(_T("Source:\n%s\n\nTarget:\n%s\n\nOptions:\n%s\n\nConfirm database migration?"), sourceConfFields, destConfFields, options.cstr()))
	   return;

   DecryptPassword(s_dbLogin, s_dbPassword, s_dbPassword, MAX_PASSWORD);
   bool success = ImportOrMigrateDatabase(excludedTables, includedTables, ignoreDataMigrationErrors);
	_tprintf(success ? _T("Database migration complete.\n") : _T("Database migration failed.\n"));
}

/**
 * Migrate database
 */
void ImportDatabase(const char *file, const StringList& excludedTables, const StringList& includedTables, bool ignoreDataMigrationErrors)
{
   _tcscpy(s_dbDriver, _T("sqlite.ddr"));
#ifdef UNICODE
   MultiByteToWideCharSysLocale(file, s_dbName, MAX_DB_NAME);
#else
   strlcpy(s_dbName, file, MAX_DB_NAME);
#endif
   s_import = true;
   bool success = ImportOrMigrateDatabase(excludedTables, includedTables, ignoreDataMigrationErrors);
   _tprintf(success ? _T("Database import complete.\n") : _T("Database import failed.\n"));
}
