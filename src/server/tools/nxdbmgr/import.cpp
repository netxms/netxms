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
** File: import.cpp
**
**/

#include "nxdbmgr.h"
#include "sqlite3.h"


//
// Tables to import
//

extern const TCHAR *g_tables[];


//
// Callback for import table
//

static int ImportTableCB(void *arg, int cols, char **data, char **names)
{
	String query;
	int i;

	query.addFormattedString("INSERT INTO %s (", arg);
	for(i = 0; i < cols; i++)
	{
		query += names[i];
		query += ",";
	}
	query.shrink();
	query += ") VALUES (";
	for(i = 0; i < cols; i++)
		query.addFormattedString("'%s',", data[i]);
	query.shrink();
	query += ")";

	return SQLQuery(query) ? 0 : 1;
}


//
// Import single database table
//

static BOOL ImportTable(sqlite3 *db, const char *table)
{
	char query[256], *errmsg;
	int rc;

	printf("Importing table %s\n", table);

	if (DBBegin(g_hCoreDB))
	{
		snprintf(query, 256, "SELECT * FROM %s", table);
		rc = sqlite3_exec(db, query, ImportTableCB, (void *)table, &errmsg);
		if (rc == SQLITE_OK)
		{
			DBCommit(g_hCoreDB);
		}
		else
		{
			printf("ERROR: SQL query \"%s\" on import file failed (%s)\n", query, errmsg);
			sqlite3_free(errmsg);
			DBRollback(g_hCoreDB);
		}
	}
	else
	{
		printf("ERROR: unable to start transaction in target database\n");
		rc = -1;
	}
	return rc == SQLITE_OK;
}


//
// Import idata_xx tables
//

static BOOL ImportIData(sqlite3 *db)
{
	DB_RESULT hResult;
	int i, j, count;
	DWORD id;
	TCHAR buffer[1024], queryTemplate[5][MAX_DB_STRING];

	// Query for idata tables creation should be stored already in
	// target database metadata table
	if (!MetaDataReadStr(_T("IDataTableCreationCommand"), queryTemplate[0], MAX_DB_STRING, _T("")))
	{
		printf("ERROR: unable to determine correct query for idata tables creation\n");
		return FALSE;
	}

	// There can be up to 4 index creation queries for idata tables
	for(i = 1; i < 5; i++)
	{
		_sntprintf(buffer, 1024, _T("IDataIndexCreationCommand_%d"), i - 1);
		if (!MetaDataReadStr(buffer, queryTemplate[i], MAX_DB_STRING, _T("")))
			break;
	}
	for(; i < 5; i++)
		queryTemplate[i][0] = 0;

	hResult = SQLSelect(_T("SELECT id FROM nodes"));
	if (hResult == NULL)
		return FALSE;

	// Create and import idata_xx tables for each node in "nodes" table
	count = DBGetNumRows(hResult);
	for(i = 0; i < count; i++)
	{
		id = DBGetFieldULong(hResult, i, 0);
		_sntprintf(buffer, 1024, queryTemplate[0], id);
		if (!SQLQuery(buffer))
			break;	// Failed to create idata_xx table

		// Create indexes
		for(j = 1; (j < 5) && (queryTemplate[j][0] != 0); j++)
		{
			_sntprintf(buffer, 1024, queryTemplate[j], id, id);
			if (!SQLQuery(buffer))
				goto cleanup;	// Failed to create index for idata_xx table
		}

		_sntprintf(buffer, 1024, _T("idata_%d"), DBGetFieldULong(hResult, i, 0));
		if (!ImportTable(db, buffer))
			break;
	}

cleanup:
	DBFreeResult(hResult);
	return i == count;
}


//
// Callback for getting schema version
//

static int GetSchemaVersionCB(void *arg, int cols, char **data, char **names)
{
	*((int *)arg) = strtol(data[0], NULL, 10);
	return 0;
}


//
// Import database
//

void ImportDatabase(const char *file)
{
	sqlite3 *db;
	char *errmsg;
	int i;
	BOOL success = FALSE;

	// Open SQLite database
	if (sqlite3_open(file, &db) != SQLITE_OK)
	{
		printf("ERROR: unable to open output file\nDatabase import failed.\n");
		return;
	}

	// Check schema version
	int version = 0;
	if (sqlite3_exec(db, "SELECT var_value FROM metadata WHERE var_name='SchemaVersion'", GetSchemaVersionCB, &version, &errmsg) != SQLITE_OK)
	{
		printf("ERROR: SQL query failed (%s)\n", errmsg);
		sqlite3_free(errmsg);
		goto cleanup;
	}

	if (version != DB_FORMAT_VERSION)
	{
		printf("ERROR: Import file was created for database format version %d, but this tool was compiled for database format version %d\n", version, DB_FORMAT_VERSION);
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
	if (!ImportIData(db))
		goto cleanup;

	success = TRUE;

cleanup:
	sqlite3_close(db);
	printf(success ? "Database import complete.\n" : "Database import failed.\n");
}
