/* 
** nxdbmgr - NetXMS database manager
** Copyright (C) 2004-2013 Victor Kirhenshtein
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
static BOOL DeleteDataTables()
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
			_sntprintf(query, 256, _T("DROP TABLE idata_%d"), id);
			CHK_EXEC(SQLQuery(query));
			_sntprintf(query, 256, _T("DROP TABLE tdata_rows_%d"), id);
			CHK_EXEC(SQLQuery(query));
			_sntprintf(query, 256, _T("DROP TABLE tdata_records_%d"), id);
			CHK_EXEC(SQLQuery(query));
			_sntprintf(query, 256, _T("DROP TABLE tdata_%d"), id);
			CHK_EXEC(SQLQuery(query));
		}
		DBFreeResult(hResult);
	}
	else
	{
		if (!g_bIgnoreErrors)
			return FALSE;
	}

	return TRUE;
}

/**
 * Clear tables
 */
static BOOL ClearTables()
{
	TCHAR query[256];
	int i;

	for(i = 0; g_tables[i] != NULL; i++)
	{
		_sntprintf(query, 256, _T("DELETE FROM %s"), g_tables[i]);
		CHK_EXEC(SQLQuery(query));
	}
	return TRUE;
}

/**
 * Clear database
 */
BOOL ClearDatabase()
{
	if (!ValidateDatabase())
		return FALSE;

	WriteToTerminal(_T("\n\n\x1b[1mWARNING!!!\x1b[0m\n"));
	if (!GetYesNo(_T("This operation will clear all configuration and collected data from database.\nAre you sure?")))
		return FALSE;

	BOOL success = FALSE;

	if (DBBegin(g_hCoreDB))
	{
		if (DeleteDataTables() &&	ClearTables())
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
