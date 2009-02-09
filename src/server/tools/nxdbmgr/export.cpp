/* 
** nxdbmgr - NetXMS database manager
** Copyright (C) 2004-2009 Victor Kirhenshtein
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


//
// Export single database table
//

static BOOL ExportTable(sqlite3 *db, const char *name, ...)
{
	va_list args;
	String srcQuery, dstQuery, dstQueryTemplate;
	const char *field;
	char *errmsg, *data;
	DB_ASYNC_RESULT hResult;
	int i, fieldCount = 0;
	BOOL success = TRUE;

	srcQuery = "SELECT ";
	dstQueryTemplate.AddFormattedString("INSERT INTO %s (", name);
	va_start(args, name);
	
	while((field = va_arg(args, const char *)) != NULL)
	{
		srcQuery += field;
		srcQuery += ",";

		dstQueryTemplate += field;
		dstQueryTemplate += ",";

		fieldCount++;
	}
	srcQuery.Shrink();
	dstQueryTemplate.Shrink();

	va_end(args);

	srcQuery.AddFormattedString(" FROM %s", name);
	dstQueryTemplate += ") VALUES (";

	hResult = DBAsyncSelect(g_hCoreDB, srcQuery);
	if (hResult != NULL)
	{
		while(DBFetch(hResult))
		{
			dstQuery = (const TCHAR *)dstQueryTemplate;
			for(i = 0; i < fieldCount; i++)
			{
				dstQuery += "'";
				data = (char *)malloc(8192);
				dstQuery.AddDynamicString(DBGetFieldAsync(hResult, i, data, 8192));
				dstQuery += "',";
			}
			dstQuery.Shrink();
			dstQuery += ")";

			if (sqlite3_exec(db, dstQuery, NULL, NULL, &errmsg) != SQLITE_OK)
			{
				printf("ERROR: SQLite query failed: %s\n   Query: %s\n", errmsg, (const TCHAR *)dstQuery);
				sqlite3_free(errmsg);
				success = FALSE;
				break;
			}
		}
		DBFreeAsyncResult(g_hCoreDB, hResult);
		success = TRUE;
	}

	return success;
}


//
// Export database
//

void ExportDatabase(const char *file)
{
	sqlite3 *db;
	char *errmsg, buffer[MAX_PATH], *data;
	DWORD size;
	BOOL success = FALSE;

	if (!ValidateDatabase())
		return;

	// Create new SQLite database
	_unlink(file);
	if (sqlite3_open(file, &db) != SQLITE_OK)
	{
		_tprintf(_T("ERROR: unable to open output file\n"));
		return;
	}

	// Create table to hold export metadata
	if (sqlite3_exec(db, "CREATE TABLE metadata (label varchar(63), value varchar)", NULL, NULL, &errmsg) != SQLITE_OK)
	{
		_tprintf(_T("ERROR: unable to add metadata to output file: %s\n"), errmsg);
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
		printf("ERROR: unable to determine path to schema file\n");
		goto cleanup;
	}

	success = FALSE;	// Reset success flag
#else
	strcpy(buffer, DATADIR "/sql/dbschema_sqlite.sql");
#endif

	data = (char *)LoadFile(buffer, &size);
	if (data == NULL)
	{
		printf("ERROR: cannot load schema file \"%s\"\n", buffer);
		goto cleanup;
	}

	if (sqlite3_exec(db, data, NULL, NULL, &errmsg) != SQLITE_OK)
	{
		_tprintf(_T("ERROR: unable to apply database schema: %s\n"), errmsg);
		sqlite3_free(errmsg);
		goto cleanup;
	}

	free(data);

	// Check that dbschema_sqlite.sql and database have the same schema version
	/* TODO */

	// Clear config table in destination database
	if (sqlite3_exec(db, "DELETE FROM config", NULL, NULL, &errmsg) != SQLITE_OK)
	{
		_tprintf(_T("ERROR: unable to clear config table: %s\n"), errmsg);
		sqlite3_free(errmsg);
		goto cleanup;
	}

	// Export tables
	ExportTable(db, "config", "var_name", "var_value", "is_visible", "need_server_restart", NULL);
	ExportTable(db, "config_clob", "var_name", "var_value", NULL);

	success = TRUE;

cleanup:
	sqlite3_close(db);
	printf(success ? "Database export complete.\n" : "Database export failed.\n");
}
