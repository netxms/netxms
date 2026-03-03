/*
** nxdbmgr - NetXMS database manager
** Copyright (C) 2004-2026 Victor Kirhenshtein
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
extern const wchar_t *g_tables[];

/**
 * Delete idata_xx tables
 */
static bool DeleteDataTables()
{
	wchar_t query[256];

	IntegerArray<uint32_t> *targets = GetDataCollectionTargets();
   for(int i = 0; i < targets->size(); i++)
   {
      uint32_t id = targets->get(i);
      if (IsDataTableExist(_T("idata_%u"), id))
      {
         nx_swprintf(query, 256, L"DROP TABLE idata_%u", id);
         CHK_EXEC(SQLQuery(query));
      }
      if (IsDataTableExist(_T("tdata_%u"), id))
      {
         nx_swprintf(query, 256, L"DROP TABLE tdata_%u", id);
         CHK_EXEC(SQLQuery(query));
      }
   }
   delete targets;

	return true;
}

/**
 * Clear table
 */
static bool ClearTable(const TCHAR *table, void *userData)
{
   wchar_t query[256];
   if ((g_dbSyntax == DB_SYNTAX_ORACLE) || (g_dbSyntax == DB_SYNTAX_PGSQL) || (g_dbSyntax == DB_SYNTAX_TSDB))
      nx_swprintf(query, 256, L"TRUNCATE TABLE %s", table);
   else
      nx_swprintf(query, 256, L"DELETE FROM %s", table);
   return SQLQuery(query);
}

/**
 * Clear tables
 */
static bool ClearTables()
{
	for(int i = 0; g_tables[i] != nullptr; i++)
	{
	   if ((g_dbSyntax == DB_SYNTAX_TSDB) && (!wcsicmp(g_tables[i], L"idata") || !wcsicmp(g_tables[i], L"tdata")))
	      continue;   // idata and tdata are views in TSDB schema
      if ((g_dbSyntax != DB_SYNTAX_TSDB) && (!wcsnicmp(g_tables[i], L"idata_sc_", 9) || !wcsnicmp(g_tables[i], L"tdata_sc_", 9)))
         continue;   // tables named idata_sc_* and tdata_sc_* exist only in TSDB schema
	   CHK_EXEC(ClearTable(g_tables[i], nullptr));
	}
	return EnumerateModuleTables(ClearTable, nullptr);
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

	WriteToTerminal(L"\n\n\x1b[1mWARNING!!!\x1b[0m\n");
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
			WriteToTerminal(success ? L"Database successfully cleared\n" : L"ERROR: cannot commit transaction\n");
		}
		else
		{
			DBRollback(g_dbHandle);
		}
	}
	else
	{
	   WriteToTerminal(L"ERROR: cannot start transaction\n");
	}

	return success;
}
