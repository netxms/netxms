/*
** nxdbmgr - NetXMS database manager
** Copyright (C) 2004-2015 Victor Kirhenshtein
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
	int i, columnCount = 0;
	BOOL success = TRUE;

	_tprintf(_T("Exporting table %s\n"), name);

	if (sqlite3_exec(db, "BEGIN", NULL, NULL, &errmsg) == SQLITE_OK)
	{
		_sntprintf(buffer, 256, _T("SELECT * FROM %s"), name);

		DB_UNBUFFERED_RESULT hResult = SQLSelectUnbuffered(buffer);
		if (hResult != NULL)
		{
			while(DBFetch(hResult))
			{
				query = _T("");

				// Column names
				columnCount = DBGetColumnCount(hResult);
				query.appendFormattedString(_T("INSERT INTO %s ("), name);
				for(i = 0; i < columnCount; i++)
				{
					DBGetColumnName(hResult, i, buffer, 256);
					query += buffer;
					query += _T(",");
				}
				query.shrink();
				query += _T(") VALUES (");

				// Data
				TCHAR data[8192];
				for(i = 0; i < columnCount; i++)
				{
					TCHAR *escapedString = EscapeString(DBGetField(hResult, i, data, 8192));
					query.appendPreallocated(escapedString);
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
			DBFreeResult(hResult);

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
void ExportDatabase(char *file, bool skipAudit, bool skipAlarms, bool skipEvent, bool skipSysLog, bool skipTrapLog)
{
	sqlite3 *db;
	char *errmsg, queryTemplate[11][MAX_DB_STRING], *data;
	TCHAR idataTable[128];
   int legacy = 0, major = 0, minor = 0;
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
   TCHAR schemaFile[MAX_PATH];
   GetNetXMSDirectory(nxDirShare, schemaFile);
   _tcslcat(schemaFile, FS_PATH_SEPARATOR _T("sql") FS_PATH_SEPARATOR _T("dbschema_sqlite.sql"), MAX_PATH);

   UINT32 size;
	data = (char *)LoadFile(schemaFile, &size);
	if (data == NULL)
	{
		_tprintf(_T("ERROR: cannot load schema file \"%s\"\n"), schemaFile);
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
	if ((sqlite3_exec(db, "SELECT var_value FROM metadata WHERE var_name='SchemaVersion'", GetSchemaVersionCB, &legacy, &errmsg) != SQLITE_OK) ||
	    (sqlite3_exec(db, "SELECT var_value FROM metadata WHERE var_name='SchemaVersionMajor'", GetSchemaVersionCB, &major, &errmsg) != SQLITE_OK) ||
	    (sqlite3_exec(db, "SELECT var_value FROM metadata WHERE var_name='SchemaVersionMinor'", GetSchemaVersionCB, &minor, &errmsg) != SQLITE_OK))
	{
		_tprintf(_T("ERROR: SQL query failed (%hs)\n"), errmsg);
		sqlite3_free(errmsg);
		goto cleanup;
	}

	INT32 dbmajor, dbminor;
	if (!DBGetSchemaVersion(g_hCoreDB, &dbmajor, &dbminor))
	{
      _tprintf(_T("ERROR: Cannot determine database schema version. Please check that NetXMS server installed correctly.\n"));
      goto cleanup;
	}
	if ((dbmajor != major) || (dbminor != minor))
	{
		_tprintf(_T("ERROR: Schema version mismatch between dbschema_sqlite.sql and your database. Please check that NetXMS server installed correctly.\n"));
		goto cleanup;
	}

	// Export tables
	for(int i = 0; g_tables[i] != NULL; i++)
	{
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
      if (!ExportTable(db, g_tables[i]))
			goto cleanup;
	}

	if (!g_skipDataMigration || !g_skipDataSchemaMigration)
	{
	   int i;

      // Export tables with collected DCI data
      memset(queryTemplate, 0, sizeof(queryTemplate));

      if (sqlite3_exec(db, "SELECT var_value FROM metadata WHERE var_name='IDataTableCreationCommand'",
                       GetIDataQueryCB, queryTemplate[0], &errmsg) != SQLITE_OK)
      {
         _tprintf(_T("ERROR: SQLite query failed (%hs)\n"), errmsg);
         sqlite3_free(errmsg);
         goto cleanup;
      }

      for(i = 0; i < 10; i++)
      {
         char query[256];
         sprintf(query, "SELECT var_value FROM metadata WHERE var_name='TDataTableCreationCommand_%d'", i);
         if (sqlite3_exec(db, query, GetIDataQueryCB, queryTemplate[i + 1], &errmsg) != SQLITE_OK)
         {
            _tprintf(_T("ERROR: SQLite query failed (%hs)\n"), errmsg);
            sqlite3_free(errmsg);
            goto cleanup;
         }
         if (queryTemplate[i + 1][0] == 0)
            break;
      }

      IntegerArray<UINT32> *targets = GetDataCollectionTargets();
      if (targets == NULL)
         goto cleanup;
      for(i = 0; i < targets->size(); i++)
      {
         UINT32 id = targets->get(i);

         if (!g_skipDataSchemaMigration)
         {
            for(int j = 0; j < 11; j++)
            {
               if (queryTemplate[j][0] == 0)
                  break;

               char query[1024];
               snprintf(query, 1024, queryTemplate[j], id, id);
               if (sqlite3_exec(db, query, NULL, NULL, &errmsg) != SQLITE_OK)
               {
                  _tprintf(_T("ERROR: SQLite query failed: %hs (%hs)\n"), query, errmsg);
                  sqlite3_free(errmsg);
                  goto cleanup;
               }
            }
         }

         if (!g_skipDataMigration)
         {
            _sntprintf(idataTable, 128, _T("idata_%d"), id);
            if (!ExportTable(db, idataTable))
            {
               goto cleanup;
            }

            _sntprintf(idataTable, 128, _T("tdata_%d"), id);
            if (!ExportTable(db, idataTable))
            {
               goto cleanup;
            }
         }
      }

      delete targets;
	}

	success = TRUE;

cleanup:
	sqlite3_close(db);
	_tprintf(success ? _T("Database export complete.\n") : _T("Database export failed.\n"));
}
