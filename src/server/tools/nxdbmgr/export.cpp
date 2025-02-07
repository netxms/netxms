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
** File: export.cpp
**
**/

#include "nxdbmgr.h"
#include "sqlite3.h"

/**
 * Tables to export
 */
extern const TCHAR *g_tables[];

/**
 * Escape string for SQLite
 */
static TCHAR *EscapeString(TCHAR *str)
{
   if (NumChars(str, _T('\'')) == 0)
      return str;

	size_t len = _tcslen(str) + 1;
	size_t bufferSize = len + 128;
	TCHAR *out = MemAllocString(bufferSize);

	const TCHAR *src = str;
	int outPos;
	for(outPos = 0; *src != 0; src++)
	{
		if (*src == _T('\''))
		{
			len++;
			if (len >= bufferSize)
			{
				bufferSize += 128;
				out = MemReallocArray(out, bufferSize);
			}
			out[outPos++] = _T('\'');
			out[outPos++] = _T('\'');
		}
		else
		{
			out[outPos++] = *src;
		}
	}
	out[outPos] = 0;

	MemFree(str);
	return out;
}

/**
 * Export single database table
 */
static bool ExportTable(sqlite3 *db, const TCHAR *name)
{
   _tprintf(_T("Exporting table %s\n"), name);

   char *errmsg;
   if (sqlite3_exec(db, "BEGIN", nullptr, nullptr, &errmsg) != SQLITE_OK)
   {
      WriteToTerminalEx(_T("\x1b[31;1mERROR:\x1b[0m Cannot start transaction: %hs"), errmsg);
      sqlite3_free(errmsg);
      return false;
   }

	StringBuffer query;
	int i, columnCount = 0;
	bool success = true;
   char cname[256];
   StringBuffer selectQuery(_T("SELECT * FROM "));
   if ((g_dbSyntax == DB_SYNTAX_TSDB) && IsTimestampConversionNeeded(name))
   {
      selectQuery.append(name);
      selectQuery.append(_T(" WHERE 1=0"));
      DB_RESULT hResult = SQLSelect(selectQuery);
      if (hResult != nullptr)
      {
         columnCount = DBGetColumnCount(hResult);
         if (columnCount > 0)
         {
            selectQuery = _T("SELECT ");
            for(i = 0; i < columnCount; i++)
            {
               DBGetColumnNameA(hResult, i, cname, 256);
               if (IsTimestampColumn(name, cname))
               {
                  selectQuery.append(_T("date_part('epoch',"));
                  selectQuery.appendUtf8String(cname);
                  selectQuery.append(_T(")::int AS "));
                  selectQuery.appendUtf8String(cname);
                  selectQuery.append(_T(","));
               }
               else
               {
                  selectQuery.appendUtf8String(cname);
                  selectQuery.append(_T(","));
               }
            }
            selectQuery.shrink();
            selectQuery.append(_T(" FROM "));
            selectQuery.append(name);
         }
         DBFreeResult(hResult);
      }
   }
   else
   {
      selectQuery.append(name);
   }

   DB_UNBUFFERED_RESULT hResult = SQLSelectUnbuffered(selectQuery);
   if (hResult != nullptr)
   {
      while(DBFetch(hResult))
      {
         query.clear();

         // Column names
         columnCount = DBGetColumnCount(hResult);
         query.appendFormattedString(_T("INSERT INTO %s ("), name);
         for(i = 0; i < columnCount; i++)
         {
            DBGetColumnNameA(hResult, i, cname, 256);
            query.appendUtf8String(cname);
            query.append(_T(","));
         }
         query.shrink();
         query += _T(") VALUES (");

         // Data
         for(i = 0; i < columnCount; i++)
         {
            query.append(_T("'"));
            query.appendPreallocated(EscapeString(DBGetField(hResult, i, nullptr, 0)));
            query += _T("',");
         }
         query.shrink();
         query += _T(")");

         char *utf8query = query.getUTF8String();
         if (sqlite3_exec(db, utf8query, nullptr, nullptr, &errmsg) != SQLITE_OK)
         {
            MemFree(utf8query);
            WriteToTerminalEx(_T("\x1b[31;1mERROR:\x1b[0m SQLite query failed: %hs\n   Query: %s\n"), errmsg, (const TCHAR *)query);
            sqlite3_free(errmsg);
            success = false;
            break;
         }
         MemFree(utf8query);
      }
      DBFreeResult(hResult);

      if (success)
      {
         if (sqlite3_exec(db, "COMMIT", nullptr, nullptr, &errmsg) != SQLITE_OK)
         {
            WriteToTerminalEx(_T("\x1b[31;1mERROR:\x1b[0m Cannot commit transaction: %hs"), errmsg);
            sqlite3_free(errmsg);
            success = false;
         }
      }
      else
      {
         if (sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, &errmsg) != SQLITE_OK)
         {
            WriteToTerminalEx(_T("\x1b[31;1mERROR:\x1b[0m Cannot rollback transaction: %hs"), errmsg);
            sqlite3_free(errmsg);
         }
      }
   }
   else
   {
      success = false;
      if (sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, &errmsg) != SQLITE_OK)
      {
         WriteToTerminalEx(_T("\x1b[31;1mERROR:\x1b[0m Cannot rollback transaction: %hs"), errmsg);
         sqlite3_free(errmsg);
      }
   }

	return success;
}

/**
 * Callback for getting schema version
 */
static int GetSchemaVersionCB(void *arg, int cols, char **data, char **names)
{
	*((int *)arg) = strtol(data[0], nullptr, 10);
	return 0;
}

/**
 * Callback for getting idata_xx table creation query
 */
static int GetIDataQueryCB(void *arg, int cols, char **data, char **names)
{
	strlcpy((char *)arg, data[0], MAX_DB_STRING);
	return 0;
}

/**
 * Data for module table import
 */
struct ModuleTableExportData
{
  const StringList *excludedTables;
  const StringList *includedTables;
  sqlite3 *db;
};

/**
 * Callback for exporting module table
 */
static bool ExportModuleTable(const TCHAR *table, void *context)
{
   if (static_cast<ModuleTableExportData*>(context)->excludedTables->contains(table) ||
       (!static_cast<ModuleTableExportData*>(context)->includedTables->isEmpty() && !static_cast<ModuleTableExportData*>(context)->includedTables->contains(table)))
   {
      _tprintf(_T("Skipping table %s\n"), table);
      return true;
   }
   return ExportTable(static_cast<ModuleTableExportData*>(context)->db, table);
}

/**
 * Execute schema file
 */
static bool ExecuteSchemaFile(const TCHAR *prefix, void *userArg)
{
   TCHAR schemaFile[MAX_PATH];
   GetNetXMSDirectory(nxDirShare, schemaFile);
   _tcslcat(schemaFile, FS_PATH_SEPARATOR _T("sql") FS_PATH_SEPARATOR, MAX_PATH);
   if (prefix != nullptr)
   {
      _tcslcat(schemaFile, prefix, MAX_PATH);
      _tcslcat(schemaFile, _T("_"), MAX_PATH);
   }
   _tcslcat(schemaFile, _T("dbschema_sqlite.sql"), MAX_PATH);

   char *data = LoadFileAsUTF8String(schemaFile);
   if (data == nullptr)
   {
      WriteToTerminalEx(_T("\x1b[31;1mERROR:\x1b[0m cannot load schema file \"%s\"\n"), schemaFile);
      return false;
   }

   char *errmsg;
   bool success = (sqlite3_exec(static_cast<sqlite3*>(userArg), data, nullptr, nullptr, &errmsg) == SQLITE_OK);
   if (!success)
   {
      WriteToTerminalEx(_T("\x1b[31;1mERROR:\x1b[0m unable to apply database schema (%hs)\n"), errmsg);
      sqlite3_free(errmsg);
   }

   MemFree(data);
   return success;
}

/**
 * Process performance data read from idata or tdata table
 */
static bool ProcessPerfData(DB_UNBUFFERED_RESULT hResult, sqlite3 *db, const TCHAR *name, bool tdata)
{
   bool success = true;
   StringBuffer query;
   TCHAR id[64], ts[64];
   while(DBFetch(hResult))
   {
      query = _T("INSERT INTO ");
      query.append(name);
      query.append(tdata ? _T(" (item_id,tdata_timestamp,tdata_value) VALUES (") : _T(" (item_id,idata_timestamp,idata_value,raw_value) VALUES ("));
      query.append(DBGetField(hResult, 0, id, 64));
      query.append(_T(","));
      query.append(DBGetField(hResult, 1, ts, 64));
      query.append(_T(",'"));
      query.appendPreallocated(EscapeString(DBGetField(hResult, 2, nullptr, 0)));
      if (!tdata)
      {
         query.append(_T("','"));
         query.appendPreallocated(EscapeString(DBGetField(hResult, 3, nullptr, 0)));
      }
      query.append(_T("')"));

      char *utf8query = query.getUTF8String();
      char *errmsg;
      if (sqlite3_exec(db, utf8query, nullptr, nullptr, &errmsg) != SQLITE_OK)
      {
         MemFree(utf8query);
         WriteToTerminalEx(_T("\x1b[31;1mERROR:\x1b[0m SQLite query failed: %hs\n   Query: %s\n"), errmsg, (const TCHAR *)query);
         sqlite3_free(errmsg);
         success = false;
         break;
      }
      MemFree(utf8query);
   }
   return success;
}

/**
 * Export performance data from single table to per-node tables
 */
static bool ExportPerfDataTable(sqlite3 *db, const TCHAR *name, bool tdata, uint32_t objectId)
{
   _tprintf(_T("Exporting table %s\n"), name);

   char *errmsg;
   if (sqlite3_exec(db, "BEGIN", nullptr, nullptr, &errmsg) != SQLITE_OK)
   {
      WriteToTerminalEx(_T("\x1b[31;1mERROR:\x1b[0m Cannot start transaction: %hs"), errmsg);
      sqlite3_free(errmsg);
      return false;
   }

   bool success = true;
   if (g_dbSyntax == DB_SYNTAX_TSDB)
   {
      static const TCHAR *storageClass[] = { _T("default"), _T("7"), _T("30"), _T("90"), _T("180"), _T("other") };
      for(int i = 0; i < 6; i++)
      {
         TCHAR query[256];
         if (tdata)
         {
            _sntprintf(query, 256, _T("SELECT item_id,date_part('epoch',tdata_timestamp)::int,tdata_value FROM tdata_sc_%s WHERE item_id IN (SELECT item_id FROM dc_tables WHERE node_id=%u)"), storageClass[i], objectId);
         }
         else
         {
            _sntprintf(query, 256, _T("SELECT item_id,date_part('epoch',idata_timestamp)::int,idata_value,raw_value FROM idata_sc_%s WHERE item_id IN (SELECT item_id FROM items WHERE node_id=%u)"), storageClass[i], objectId);
         }

         DB_UNBUFFERED_RESULT hResult = SQLSelectUnbuffered(query);
         if (hResult == nullptr)
         {
            success = false;
            break;
         }

         success = ProcessPerfData(hResult, db, name, tdata);
         DBFreeResult(hResult);
         if (!success)
            break;
      }
   }
   else
   {
      TCHAR query[256];
      if (tdata)
      {
         _sntprintf(query, 256, _T("SELECT item_id,tdata_timestamp,tdata_value FROM tdata WHERE item_id IN (SELECT item_id FROM dc_tables WHERE node_id=%u)"), objectId);
      }
      else
      {
         _sntprintf(query, 256, _T("SELECT item_id,idata_timestamp,idata_value,raw_value FROM idata WHERE item_id IN (SELECT item_id FROM items WHERE node_id=%u)"), objectId);
      }
      DB_UNBUFFERED_RESULT hResult = SQLSelectUnbuffered(query);
      if (hResult != nullptr)
      {
         success = ProcessPerfData(hResult, db, name, tdata);
         DBFreeResult(hResult);
      }
      else
      {
         success = false;
      }
   }

   if (success)
   {
      if (sqlite3_exec(db, "COMMIT", nullptr, nullptr, &errmsg) != SQLITE_OK)
      {
         WriteToTerminalEx(_T("\x1b[31;1mERROR:\x1b[0m Cannot commit transaction: %hs"), errmsg);
         sqlite3_free(errmsg);
         success = false;
      }
   }
   else
   {
      if (sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, &errmsg) != SQLITE_OK)
      {
         WriteToTerminalEx(_T("\x1b[31;1mERROR:\x1b[0m Cannot rollback transaction: %hs"), errmsg);
         sqlite3_free(errmsg);
      }
   }

   return success;
}

/**
 * Export multi-table performance data
 */
static bool ExportPerfData(sqlite3 *db, const StringList& excludedTables)
{
   char queryTemplate[11][MAX_DB_STRING];
   memset(queryTemplate, 0, sizeof(queryTemplate));

   char *errmsg;
   if (sqlite3_exec(db, "SELECT var_value FROM metadata WHERE var_name='IDataTableCreationCommand'",
                    GetIDataQueryCB, queryTemplate[0], &errmsg) != SQLITE_OK)
   {
      WriteToTerminalEx(_T("\x1b[31;1mERROR:\x1b[0m SQLite query failed (%hs)\n"), errmsg);
      sqlite3_free(errmsg);
      return false;
   }

   for(int i = 0; i < 10; i++)
   {
      char query[256];
      sprintf(query, "SELECT var_value FROM metadata WHERE var_name='TDataTableCreationCommand_%d'", i);
      if (sqlite3_exec(db, query, GetIDataQueryCB, queryTemplate[i + 1], &errmsg) != SQLITE_OK)
      {
         WriteToTerminalEx(_T("\x1b[31;1mERROR:\x1b[0m SQLite query failed (%hs)\n"), errmsg);
         sqlite3_free(errmsg);
         return false;
      }
      if (queryTemplate[i + 1][0] == 0)
         break;
   }

   shared_ptr<IntegerArray<uint32_t>> targets(GetDataCollectionTargets());
   if (targets == nullptr)
   {
      WriteToTerminalEx(_T("\x1b[31;1mERROR:\x1b[0m cannot collect list of data collection targets\n"));
      return false;
   }

   bool singleTable = (DBMgrMetaDataReadInt32(_T("SingeTablePerfData"), 0) != 0);

   for(int i = 0; i < targets->size(); i++)
   {
      uint32_t id = targets->get(i);

      if (!g_skipDataSchemaMigration)
      {
         for(int j = 0; j < 11; j++)
         {
            if (queryTemplate[j][0] == 0)
               break;

            char query[1024];
            snprintf(query, 1024, queryTemplate[j], id, id);
            if (sqlite3_exec(db, query, nullptr, nullptr, &errmsg) != SQLITE_OK)
            {
               WriteToTerminalEx(_T("\x1b[31;1mERROR:\x1b[0m SQLite query failed: %hs (%hs)\n"), query, errmsg);
               sqlite3_free(errmsg);
               return false;
            }
         }
      }

      if (!g_skipDataMigration)
      {
         TCHAR idataTable[128];
         _sntprintf(idataTable, 128, _T("idata_%d"), id);
         if (!excludedTables.contains(idataTable))
         {
            if (singleTable)
            {
               if (!ExportPerfDataTable(db, idataTable, false, id))
                  return false;
            }
            else
            {
               if (!ExportTable(db, idataTable))
                  return false;
            }
         }
         else
         {
            _tprintf(_T("Skipping table %s\n"), idataTable);
         }

         _sntprintf(idataTable, 128, _T("tdata_%d"), id);
         if (!excludedTables.contains(idataTable))
         {
            if (singleTable)
            {
               if (!ExportPerfDataTable(db, idataTable, true, id))
                  return false;
            }
            else
            {
               if (!ExportTable(db, idataTable))
                  return false;
            }
         }
         else
         {
            _tprintf(_T("Skipping table %s\n"), idataTable);
         }
      }
   }
   return true;
}

/**
 * Export database
 */
void ExportDatabase(const char *file, const StringList& excludedTables, const StringList& includedTables)
{
   if (!ValidateDatabase())
      return;

	// Create new SQLite database
	remove(file);
   sqlite3 *db;
	if (sqlite3_open(file, &db) != SQLITE_OK)
	{
	   WriteToTerminalEx(_T("\x1b[31;1mERROR:\x1b[0m unable to open output file\n"));
		return;
	}

   char *errmsg;
   int legacy = 0, major = 0, minor = 0;
   bool success = false;

   if (sqlite3_exec(db, "PRAGMA page_size=65536", nullptr, nullptr, &errmsg) != SQLITE_OK)
   {
      WriteToTerminalEx(_T("\x1b[31;1mERROR:\x1b[0m cannot set page size for export file (%hs)\n"), errmsg);
      sqlite3_free(errmsg);
      goto cleanup;
   }

	// Setup database schema
   if (!ExecuteSchemaFile(nullptr, db))
      goto cleanup;
   if (!EnumerateModuleSchemas(ExecuteSchemaFile, db))
      goto cleanup;

	// Check that dbschema_sqlite.sql and database have the same schema version
	if ((sqlite3_exec(db, "SELECT var_value FROM metadata WHERE var_name='SchemaVersion'", GetSchemaVersionCB, &legacy, &errmsg) != SQLITE_OK) ||
	    (sqlite3_exec(db, "SELECT var_value FROM metadata WHERE var_name='SchemaVersionMajor'", GetSchemaVersionCB, &major, &errmsg) != SQLITE_OK) ||
	    (sqlite3_exec(db, "SELECT var_value FROM metadata WHERE var_name='SchemaVersionMinor'", GetSchemaVersionCB, &minor, &errmsg) != SQLITE_OK))
	{
	   WriteToTerminalEx(_T("\x1b[31;1mERROR:\x1b[0m SQL query failed (%hs)\n"), errmsg);
		sqlite3_free(errmsg);
		goto cleanup;
	}

	int32_t dbmajor, dbminor;
	if (!DBGetSchemaVersion(g_dbHandle, &dbmajor, &dbminor))
	{
	   WriteToTerminalEx(_T("\x1b[31;1mERROR:\x1b[0m Cannot determine database schema version. Please check that NetXMS server installed correctly.\n"));
      goto cleanup;
	}
	if ((dbmajor != major) || (dbminor != minor))
	{
	   WriteToTerminalEx(_T("\x1b[31;1mERROR:\x1b[0m Schema version mismatch between dbschema_sqlite.sql and your database. Please check that NetXMS server installed correctly.\n"));
		goto cleanup;
	}

	// Export tables
	for(int i = 0; g_tables[i] != nullptr; i++)
	{
	   const TCHAR *table = g_tables[i];
      if (!_tcsncmp(g_tables[i], _T("idata"), 5) ||
          !_tcsncmp(g_tables[i], _T("tdata"), 5))
         continue;  // idata and tdata exported separately
	   if (((g_skipDataMigration || g_skipDataSchemaMigration) && !_tcscmp(table, _T("raw_dci_values"))) ||
	       excludedTables.contains(table) ||
	       (!includedTables.isEmpty() && !includedTables.contains(table)))
	   {
	      _tprintf(_T("Skipping table %s\n"), table);
         continue;
	   }
      if (!ExportTable(db, table))
			goto cleanup;
	}

	ModuleTableExportData data;
	data.db = db;
	data.excludedTables = &excludedTables;
   data.includedTables = &includedTables;
   if (!EnumerateModuleTables(ExportModuleTable, &data))
      goto cleanup;

	if (!g_skipDataMigration || !g_skipDataSchemaMigration)
	{
      ExportPerfData(db, excludedTables);
	}

	success = true;

cleanup:
	sqlite3_close(db);
	_tprintf(success ? _T("Database export complete.\n") : _T("Database export failed.\n"));
}
