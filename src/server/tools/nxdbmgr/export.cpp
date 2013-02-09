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
static TCHAR *EscapeString(const TCHAR *str)
{
	int len = (int)_tcslen(str) + 3;   // + two quotes and \0 at the end
	int bufferSize = len + 128;
	TCHAR *out = (TCHAR *)malloc(bufferSize * sizeof(TCHAR));
	out[0] = _T('\'');

	const TCHAR *src = str;
	int outPos;
	for(outPos = 1; *src != 0; src++)
	{
		if (*src == _T('\''))
		{
			len++;
			if (len >= bufferSize)
			{
				bufferSize += 128;
				out = (TCHAR *)realloc(out, bufferSize * sizeof(TCHAR));
			}
			out[outPos++] = _T('\'');
			out[outPos++] = _T('\'');
		}
		else
		{
			out[outPos++] = *src;
		}
	}
	out[outPos++] = _T('\'');
	out[outPos++] = 0;

	return out;
}

/**
 * Export single database table
 */
static BOOL ExportTable(sqlite3 *db, const TCHAR *name)
{
	String query;
	TCHAR buffer[256];
	char *errmsg;
	DB_ASYNC_RESULT hResult;
	int i, columnCount = 0;
	BOOL success = TRUE;

	_tprintf(_T("Exporting table %s\n"), name);

	if (sqlite3_exec(db, "BEGIN", NULL, NULL, &errmsg) == SQLITE_OK)
	{
		_sntprintf(buffer, 256, _T("SELECT * FROM %s"), name);

		hResult = SQLAsyncSelect(buffer);
		if (hResult != NULL)
		{
			while(DBFetch(hResult))
			{
				query = _T("");

				// Column names
				columnCount = DBGetColumnCountAsync(hResult);
				query.addFormattedString(_T("INSERT INTO %s ("), name);
				for(i = 0; i < columnCount; i++)
				{
					DBGetColumnNameAsync(hResult, i, buffer, 256);
					query += buffer;
					query += _T(",");
				}
				query.shrink();
				query += _T(") VALUES (");

				// Data
				TCHAR data[8192];
				for(i = 0; i < columnCount; i++)
				{
					TCHAR *escapedString = EscapeString(DBGetFieldAsync(hResult, i, data, 8192));
					query.addDynamicString(escapedString);
					query += _T(",");
				}
				query.shrink();
				query += _T(")");

				char *utf8query = query.getUTF8String();
				if (sqlite3_exec(db, utf8query, NULL, NULL, &errmsg) != SQLITE_OK)
				{
					free(utf8query);
					_tprintf(_T("ERROR: SQLite query failed: %hs\n   Query: %s\n"), errmsg, (const TCHAR *)query);
					sqlite3_free(errmsg);
					success = FALSE;
					break;
				}
				free(utf8query);
			}
			DBFreeAsyncResult(hResult);

			if (success)
			{
				if (sqlite3_exec(db, "COMMIT", NULL, NULL, &errmsg) != SQLITE_OK)
				{
					_tprintf(_T("ERROR: Cannot commit transaction: %hs"), errmsg);
					sqlite3_free(errmsg);
					success = FALSE;
				}
			}
			else
			{
				if (sqlite3_exec(db, "ROLLBACK", NULL, NULL, &errmsg) != SQLITE_OK)
				{
					_tprintf(_T("ERROR: Cannot rollback transaction: %hs"), errmsg);
					sqlite3_free(errmsg);
				}
			}
		}
		else
		{
			success = FALSE;
			if (sqlite3_exec(db, "ROLLBACK", NULL, NULL, &errmsg) != SQLITE_OK)
			{
				_tprintf(_T("ERROR: Cannot rollback transaction: %hs"), errmsg);
				sqlite3_free(errmsg);
			}
		}
	}
	else
	{
		success = FALSE;
		_tprintf(_T("ERROR: Cannot start transaction: %hs"), errmsg);
		sqlite3_free(errmsg);
	}

	return success;
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
 * Callback for getting idata_xx table creation query
 */
static int GetIDataQueryCB(void *arg, int cols, char **data, char **names)
{
	strncpy((char *)arg, data[0], MAX_DB_STRING);
	((char *)arg)[MAX_DB_STRING - 1] = 0;
	return 0;
}

/**
 * Export database
 */
void ExportDatabase(const char *file)
{
	sqlite3 *db;
	char *errmsg, buffer[MAX_PATH], queryTemplate[MAX_DB_STRING], *data;
	TCHAR idataTable[128];
	DWORD size;
	int i, rowCount, version = 0;
	DB_RESULT hResult;
	BOOL success = FALSE;

	if (!ValidateDatabase())
		return;

	// Create new SQLite database
	unlink(file);
	if (sqlite3_open(file, &db) != SQLITE_OK)
	{
		_tprintf(_T("ERROR: unable to open output file\n"));
		return;
	}

   if (sqlite3_exec(db, "PRAGMA page_size=65536", NULL, NULL, &errmsg) != SQLITE_OK)
   {
      _tprintf(_T("ERROR: cannot set page size for export file (%hs)\n"), errmsg);
      sqlite3_free(errmsg);
      goto cleanup;
   }

	// Setup database schema
#ifdef _WIN32
	HKEY hKey;

   // Read installation data from registry
   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Software\\NetXMS\\Server"), 0,
                    KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
   {
      size = (MAX_PATH - 16) * sizeof(TCHAR);
      if (RegQueryValueEx(hKey, _T("InstallPath"), NULL, NULL, 
                          (BYTE *)buffer, &size) == ERROR_SUCCESS)
      {
			strcat(buffer, "\\lib\\sql\\dbschema_sqlite.sql");
			success = TRUE;
      }
      RegCloseKey(hKey);
   }

	if (!success)
	{
		// Try to use path to nxdbmgr executable as base
		if (GetModuleFileNameA(NULL, buffer, MAX_PATH - 32) != 0)
		{
			char *p;

			p = strrchr(buffer, '\\');
			if (p != NULL)
				*p = 0;
			p = strrchr(buffer, '\\');
			if (p != NULL)
				*p = 0;
			strcat(buffer, "\\lib\\sql\\dbschema_sqlite.sql");
			success = TRUE;
		}
	}

	if (!success)
	{
		_tprintf(_T("ERROR: unable to determine path to schema file\n"));
		goto cleanup;
	}

	success = FALSE;	// Reset success flag
#else
#ifdef UNICODE
	WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, DATADIR, -1, buffer, MAX_PATH - 32, NULL, NULL);
	strcat(buffer, "/sql/dbschema_sqlite.sql");
#else
	strcpy(buffer, DATADIR "/sql/dbschema_sqlite.sql");
#endif
#endif

	data = (char *)LoadFileA(buffer, &size);
	if (data == NULL)
	{
		_tprintf(_T("ERROR: cannot load schema file \"%hs\"\n"), buffer);
		goto cleanup;
	}

	if (sqlite3_exec(db, data, NULL, NULL, &errmsg) != SQLITE_OK)
	{
		_tprintf(_T("ERROR: unable to apply database schema: %hs\n"), errmsg);
		sqlite3_free(errmsg);
		goto cleanup;
	}

	free(data);

	// Check that dbschema_sqlite.sql and database have the same schema version
	if (sqlite3_exec(db, "SELECT var_value FROM metadata WHERE var_name='SchemaVersion'", GetSchemaVersionCB, &version, &errmsg) != SQLITE_OK)
	{
		_tprintf(_T("ERROR: SQL query failed (%hs)\n"), errmsg);
		sqlite3_free(errmsg);
		goto cleanup;
	}
	if (version != DBGetSchemaVersion(g_hCoreDB))
	{
		_tprintf(_T("ERROR: Schema version mismatch between dbschema_sqlite.sql and your database. Please check that NetXMS server installed correctly.\n"));
		goto cleanup;
	}

	// Export tables
	for(i = 0; g_tables[i] != NULL; i++)
	{
		if (!ExportTable(db, g_tables[i]))
			goto cleanup;
	}

	// Export tables with collected DCI data
	if (sqlite3_exec(db, "SELECT var_value FROM metadata WHERE var_name='IDataTableCreationCommand'",
	                 GetIDataQueryCB, queryTemplate, &errmsg) != SQLITE_OK)
	{
		_tprintf(_T("ERROR: SQL query failed (%hs)\n"), errmsg);
		sqlite3_free(errmsg);
		goto cleanup;
	}

	hResult = SQLSelect(_T("SELECT id FROM nodes"));
	if (hResult == NULL)
		goto cleanup;

	rowCount = DBGetNumRows(hResult);
	for(i = 0; i < rowCount; i++)
	{
		snprintf(buffer, MAX_PATH, queryTemplate, DBGetFieldLong(hResult, i, 0));
		if (sqlite3_exec(db, buffer, NULL, NULL, &errmsg) != SQLITE_OK)
		{
			_tprintf(_T("ERROR: SQL query failed: %hs (%hs)\n"), buffer, errmsg);
			sqlite3_free(errmsg);
			DBFreeResult(hResult);
			goto cleanup;
		}

		_sntprintf(idataTable, 128, _T("idata_%d"), DBGetFieldLong(hResult, i, 0));
		if (!ExportTable(db, idataTable))
		{
			DBFreeResult(hResult);
			goto cleanup;
		}
	}

	DBFreeResult(hResult);

	success = TRUE;

cleanup:
	sqlite3_close(db);
	_tprintf(success ? _T("Database export complete.\n") : _T("Database export failed.\n"));
}
