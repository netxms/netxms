/* 
** nxdbmgr - NetXMS database manager
** Copyright (C) 2004-2010 Victor Kirhenshtein
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
** File: import.cpp
**
**/

#include "nxdbmgr.h"
#include "sqlite3.h"

/**
 * Tables to import
 */
extern const TCHAR *g_tables[];

/**
 * Callback for import table
 */
static int ImportTableCB(void *arg, int cols, char **data, char **names)
{
	String query;
	int i;

	query.addFormattedString(_T("INSERT INTO %s ("), arg);
	for(i = 0; i < cols; i++)
	{
		query.addMultiByteString(names[i], (DWORD)strlen(names[i]), CP_UTF8);
		query += _T(",");
	}
	query.shrink();
	query += _T(") VALUES (");
	for(i = 0; i < cols; i++)
	{
		if (data[i] != NULL)
		{
#ifdef UNICODE
			WCHAR *wcData = WideStringFromUTF8String(data[i]);
			String prepData = DBPrepareString(g_hCoreDB, wcData);
			free(wcData);
#else
			String prepData = DBPrepareString(g_hCoreDB, data[i]);
#endif
			query += (const TCHAR *)prepData;
			query += _T(",");
		}
		else
		{
			query += _T("NULL,");
		}
	}
	query.shrink();
	query += _T(")");

	return SQLQuery(query) ? 0 : 1;
}

/**
 * Import single database table
 */
static BOOL ImportTable(sqlite3 *db, const TCHAR *table)
{
	char query[256], *errmsg;
	int rc;

	_tprintf(_T("Importing table %s\n"), table);

	if (DBBegin(g_hCoreDB))
	{
#ifdef UNICODE
		char *mbTable = MBStringFromWideString(table);
		snprintf(query, 256, "SELECT * FROM %s", mbTable);
		free(mbTable);
#else
		snprintf(query, 256, "SELECT * FROM %s", table);
#endif
		rc = sqlite3_exec(db, query, ImportTableCB, (void *)table, &errmsg);
		if (rc == SQLITE_OK)
		{
			DBCommit(g_hCoreDB);
		}
		else
		{
			_tprintf(_T("ERROR: SQL query \"%hs\" on import file failed (%hs)\n"), query, errmsg);
			sqlite3_free(errmsg);
			DBRollback(g_hCoreDB);
		}
	}
	else
	{
		_tprintf(_T("ERROR: unable to start transaction in target database\n"));
		rc = -1;
	}
	return rc == SQLITE_OK;
}

/**
 * Import data tables
 */
static BOOL ImportDataTables(sqlite3 *db)
{
	int i, count;
	TCHAR buffer[1024];

	DB_RESULT hResult = SQLSelect(_T("SELECT id FROM nodes"));
	if (hResult == NULL)
		return FALSE;

	// Create and import idata_xx tables for each node in "nodes" table
	count = DBGetNumRows(hResult);
	for(i = 0; i < count; i++)
	{
		DWORD id = DBGetFieldULong(hResult, i, 0);
		if (!CreateIDataTable(id))
			break;	// Failed to create idata_xx table

		_sntprintf(buffer, 1024, _T("idata_%d"), id);
		if (!ImportTable(db, buffer))
			break;

      if (!CreateTDataTables(id))
			break;	// Failed to create tdata tables

		_sntprintf(buffer, 1024, _T("tdata_%d"), id);
		if (!ImportTable(db, buffer))
			break;

      _sntprintf(buffer, 1024, _T("tdata_records_%d"), id);
		if (!ImportTable(db, buffer))
			break;

      _sntprintf(buffer, 1024, _T("tdata_rows_%d"), id);
		if (!ImportTable(db, buffer))
			break;
	}

	DBFreeResult(hResult);
	return i == count;
}

/**
 * Callback for getting schema version
 */
static int GetSchemaVersionCB(void *arg, int cols, char **data, char **names)
{
	*((int *)arg) = strtol(data[0], NULL, 10);
	return 0;
}

/**
 * Import database
 */
void ImportDatabase(const char *file)
{
	sqlite3 *db;
	char *errmsg;
	int i;
	BOOL success = FALSE;

	// Open SQLite database
	if (sqlite3_open(file, &db) != SQLITE_OK)
	{
		_tprintf(_T("ERROR: unable to open output file\nDatabase import failed.\n"));
		return;
	}

	// Check schema version
	int version = 0;
	if (sqlite3_exec(db, "SELECT var_value FROM metadata WHERE var_name='SchemaVersion'", GetSchemaVersionCB, &version, &errmsg) != SQLITE_OK)
	{
		_tprintf(_T("ERROR: SQL query failed (%hs)\n"), errmsg);
		sqlite3_free(errmsg);
		goto cleanup;
	}

	if (version != DB_FORMAT_VERSION)
	{
		_tprintf(_T("ERROR: Import file was created for database format version %d, but this tool was compiled for database format version %d\n"), version, DB_FORMAT_VERSION);
		goto cleanup;
	}

	if (!ClearDatabase())
		goto cleanup;

	// Import tables
	for(i = 0; g_tables[i] != NULL; i++)
	{
		if (!ImportTable(db, g_tables[i]))
			goto cleanup;
	}
	if (!ImportDataTables(db))
		goto cleanup;

	success = TRUE;

cleanup:
	sqlite3_close(db);
	_tprintf(success ? _T("Database import complete.\n") : _T("Database import failed.\n"));
}
