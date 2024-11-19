/*
** nxdbmgr - NetXMS database manager
** Copyright (C) 2004-2024 Victor Kirhenshtein
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
         uint32_t id = DBGetFieldULong(hResult, i, 0);
         if (IsDataTableExist(_T("idata_%u"), id))
         {
            _sntprintf(query, 256, _T("DROP TABLE idata_%u"), id);
            CHK_EXEC(SQLQuery(query));
         }
         if (IsDataTableExist(_T("tdata_%u"), id))
         {
            _sntprintf(query, 256, _T("DROP TABLE tdata_%u"), id);
            CHK_EXEC(SQLQuery(query));
         }
		}
		DBFreeResult(hResult);
	}
	else
	{
		if (!g_ignoreErrors)
			return false;
	}

	return true;
}

/**
 * Clear table
 */
static bool ClearTable(const TCHAR *table, void *userData)
{
   TCHAR query[256];
   if ((g_dbSyntax == DB_SYNTAX_ORACLE) || (g_dbSyntax == DB_SYNTAX_PGSQL) || (g_dbSyntax == DB_SYNTAX_TSDB))
      _sntprintf(query, 256, _T("TRUNCATE TABLE %s"), table);
   else
      _sntprintf(query, 256, _T("DELETE FROM %s"), table);
   return SQLQuery(query);
}

/**
 * Clear tables
 */
static bool ClearTables()
{
	for(int i = 0; g_tables[i] != NULL; i++)
	{
	   if ((g_dbSyntax == DB_SYNTAX_TSDB) && (!_tcsicmp(g_tables[i], _T("idata")) || !_tcsicmp(g_tables[i], _T("tdata"))))
	      continue;   // idata and tdata are views in TSDB schema
      if ((g_dbSyntax != DB_SYNTAX_TSDB) && (!_tcsnicmp(g_tables[i], _T("idata_sc_"), 9) || !_tcsnicmp(g_tables[i], _T("tdata_sc_"), 9)))
         continue;   // tables named idata_sc_* and tdata_sc_* exist only in TSDB schema
	   CHK_EXEC(ClearTable(g_tables[i], NULL));
	}
	return EnumerateModuleTables(ClearTable, NULL);
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

	if (DBBegin(g_dbHandle))
	{
	   if (!g_skipDataSchemaMigration)
	      success = DeleteDataTables();
	   else
	      success = true;

	   if (success)
	      success = ClearTables();

	   if (success)
		{
			success = DBCommit(g_dbHandle);
			_tprintf(success ? _T("Database successfully cleared\n") : _T("ERROR: cannot commit transaction\n"));
		}
		else
		{
			DBRollback(g_dbHandle);
		}
	}
	else
	{
		_tprintf(_T("ERROR: cannot start transaction\n"));
	}

	return success;
}
