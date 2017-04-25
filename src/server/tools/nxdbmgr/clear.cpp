/*
** nxdbmgr - NetXMS database manager
** Copyright (C) 2004-2017 Victor Kirhenshtein
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
** File: clear.cpp
**
**/

#include "nxdbmgr.h"

/**
 * Tables to clear
 */
extern const TCHAR *g_tables[];

/**
 * Delete idata_xx tables
 */
static bool DeleteDataTables()
{
	DB_RESULT hResult;
	TCHAR query[256];
	int i, count;

	hResult = SQLSelect(_T("SELECT id FROM nodes"));
	if (hResult != NULL)
	{
		count = DBGetNumRows(hResult);
		for(i = 0; i < count; i++)
		{
         UINT32 id = DBGetFieldULong(hResult, i, 0);
         if (IsDataTableExist(_T("idata_%d"), id))
         {
            _sntprintf(query, 256, _T("DROP TABLE idata_%d"), id);
            CHK_EXEC(SQLQuery(query));
         }
         if (IsDataTableExist(_T("tdata_%d"), id))
         {
            _sntprintf(query, 256, _T("DROP TABLE tdata_%d"), id);
            CHK_EXEC(SQLQuery(query));
         }
		}
		DBFreeResult(hResult);
	}
	else
	{
		if (!g_bIgnoreErrors)
			return false;
	}

	return true;
}

/**
 * Clear tables
 */
static bool ClearTables()
{
	TCHAR query[256];
	int i;

	for(i = 0; g_tables[i] != NULL; i++)
	{
		_sntprintf(query, 256, _T("DELETE FROM %s"), g_tables[i]);
		CHK_EXEC(SQLQuery(query));
	}
	return true;
}

/**
 * Warning texts
 */
static const TCHAR *s_warningTextClear = _T("This operation will clear all configuration and collected data from database.\nAre you sure?");
static const TCHAR *s_warningTextMigration = _T("This operation will clear all configuration and collected data from destination database before migration.\nAre you sure?");
static const TCHAR *s_warningTextMigrationNoData = _T("This operation will clear all configuration from destination database before migration (collected data will be kept).\nAre you sure?");

/**
 * Clear database
 */
bool ClearDatabase(bool preMigration)
{
	if (!ValidateDatabase())
		return false;

	WriteToTerminal(_T("\n\n\x1b[1mWARNING!!!\x1b[0m\n"));
	if (!GetYesNo(preMigration ? (g_skipDataSchemaMigration ? s_warningTextMigrationNoData : s_warningTextMigration) : s_warningTextClear))
		return false;

	bool success = false;

	if (DBBegin(g_hCoreDB))
	{
	   if (!g_skipDataSchemaMigration)
	      success = DeleteDataTables();
	   else
	      success = true;

	   if (success)
	      success = ClearTables();

	   if (success)
		{
			success = DBCommit(g_hCoreDB);
			_tprintf(success ? _T("Database successfully cleared\n") : _T("ERROR: cannot commit transaction\n"));
		}
		else
		{
			DBRollback(g_hCoreDB);
		}
	}
	else
	{
		_tprintf(_T("ERROR: cannot start transaction\n"));
	}

	return success;
}
