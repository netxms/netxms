/* $Id$ */
/* 
** nxdbmgr - NetXMS database manager
** Copyright (C) 2004, 2005, 2006 Victor Kirhenshtein
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
** File: reindex.cpp
**
**/

#include "nxdbmgr.h"


//
// Recreate index
//

static void RecreateIndex(const TCHAR *pszIndex, const TCHAR *pszTable, const TCHAR *pszColumns)
{
	TCHAR szQuery[1024];

	_tprintf(_T("Reindexing table %s by (%s)...\n"), pszTable, pszColumns);
	switch(g_iSyntax)
	{
		case DB_SYNTAX_MSSQL:
			_stprintf(szQuery, _T("DROP INDEX %s ON %s"), pszIndex, pszTable);
			break;
		case DB_SYNTAX_MYSQL:
			_stprintf(szQuery, _T("DROP INDEX %s FROM %s"), pszIndex, pszTable);
			break;
		case DB_SYNTAX_PGSQL:
			_stprintf(szQuery, _T("DROP INDEX %s CASCADE"), pszIndex);
			break;
		default:
			_stprintf(szQuery, _T("DROP INDEX %s"), pszIndex);
			break;
	}
	DBQuery(g_hCoreDB, szQuery);
	_stprintf(szQuery, _T("CREATE INDEX %s ON %s(%s)"), pszIndex, pszTable, pszColumns);
	SQLQuery(szQuery);
}


//
// Drop all indexes from table - MySQL way
//

static void DropAllIndexesFromTable_MYSQL(const TCHAR *table)
{
	DB_RESULT hResult;
	TCHAR query[256], index[128];

	_sntprintf(query, 256, _T("SHOW INDEX FROM %s"), table);
	hResult = SQLSelect(query);
	if (hResult != NULL)
	{
		int count = DBGetNumRows(hResult);
		for(int i = 0; i < count; i++)
		{
			_sntprintf(query, 256, _T("DROP INDEX %s FROM %s"), DBGetField(hResult, i, 0, index, 128), table);
			SQLQuery(query);
		}
		DBFreeResult(hResult);
	}
}


//
// Drop all indexes from table - PostgreSQL way
//

static void DropAllIndexesFromTable_PGSQL(const TCHAR *table)
{
	DB_RESULT hResult;
	TCHAR query[256], index[128];

	_sntprintf(query, 256, _T("SELECT ci.relname FROM pg_index i,pg_class ci,pg_class ct WHERE i.indexrelid=ci.oid AND i.indrelid=ct.oid AND ct.relname='%s'"), table);
	hResult = SQLSelect(query);
	if (hResult != NULL)
	{
		int count = DBGetNumRows(hResult);
		for(int i = 0; i < count; i++)
		{
			_sntprintf(query, 256, _T("DROP INDEX %s CASCADE"), DBGetField(hResult, i, 0, index, 128));
			SQLQuery(query);
		}
		DBFreeResult(hResult);
	}
}


//
// Drop all indexes from table - SQLite way
//

static void DropAllIndexesFromTable_SQLITE(const TCHAR *table)
{
	DB_RESULT hResult;
	TCHAR query[256], index[128];

	_sntprintf(query, 256, _T("SELECT name FROM sqlite_master WHERE type='index' AND tbl_name='%s'"), table);
	hResult = SQLSelect(query);
	if (hResult != NULL)
	{
		int count = DBGetNumRows(hResult);
		for(int i = 0; i < count; i++)
		{
			_sntprintf(query, 256, _T("DROP INDEX %s"), DBGetField(hResult, i, 0, index, 128));
			SQLQuery(query);
		}
		DBFreeResult(hResult);
	}
}


//
// Drop all indexes from table - Oracle way
//

static void DropAllIndexesFromTable_ORACLE(const TCHAR *table)
{
	DB_RESULT hResult;
	TCHAR query[256], index[128];

	_sntprintf(query, 256, _T("SELECT index_name FROM user_indexes WHERE table_name=upper('%s')"), table);
	hResult = SQLSelect(query);
	if (hResult != NULL)
	{
		int count = DBGetNumRows(hResult);
		for(int i = 0; i < count; i++)
		{
			_sntprintf(query, 256, _T("DROP INDEX %s"), DBGetField(hResult, i, 0, index, 128));
			SQLQuery(query);
		}
		DBFreeResult(hResult);
	}
}


//
// Drop all indexes from table - MS SQL way
//

static void DropAllIndexesFromTable_MSSQL(const TCHAR *table)
{
	DB_RESULT hResult;
	TCHAR query[256], index[128];

	_sntprintf(query, 256, _T("SELECT i.name FROM sys.indexes AS i JOIN sys.tables AS t ON t.object_id=i.object_id WHERE t.object_id=OBJECT_ID(N'%s','U')"), table);
	hResult = SQLSelect(query);
	if (hResult != NULL)
	{
		int count = DBGetNumRows(hResult);
		for(int i = 0; i < count; i++)
		{
			_sntprintf(query, 256, _T("DROP INDEX %s ON %s"), DBGetField(hResult, i, 0, index, 128), table);
			SQLQuery(query);
		}
		DBFreeResult(hResult);
	}
}


//
// Drop all indexes from table
//

static void DropAllIndexesFromTable(const TCHAR *table)
{
	switch(g_iSyntax)
	{
		case DB_SYNTAX_MYSQL:
			DropAllIndexesFromTable_MYSQL(table);
			break;
		case DB_SYNTAX_PGSQL:
			DropAllIndexesFromTable_PGSQL(table);
			break;
		case DB_SYNTAX_MSSQL:
			DropAllIndexesFromTable_MSSQL(table);
			break;
		case DB_SYNTAX_SQLITE:
			DropAllIndexesFromTable_SQLITE(table);
			break;
		case DB_SYNTAX_ORACLE:
			DropAllIndexesFromTable_ORACLE(table);
			break;
		default:
			break;
	}
}


//
// Reindex IDATA_xx tables
//

void ReindexIData()
{
	TCHAR table[32], query[256], queryTemplate[256];

	DB_RESULT hResult = SQLSelect(_T("SELECT id FROM nodes"));
	if (hResult == NULL)
		return;

	int count = DBGetNumRows(hResult);
	for(int i = 0; i < count; i++)
	{
		DWORD id = DBGetFieldULong(hResult, i, 0);
		_sntprintf(table, 32, _T("idata_%d"), id);
		_tprintf(_T("Reindexing table %s\n"), table);

		DropAllIndexesFromTable(table);

      for(int j = 0; j < 10; j++)
      {
         _sntprintf(query, 256, _T("IDataIndexCreationCommand_%d"), j);
         MetaDataReadStr(query, queryTemplate, 256, _T(""));
         if (queryTemplate[0] != 0)
         {
            _sntprintf(query, 256, queryTemplate, id, id);
            SQLQuery(query);
         }
      }
	}

	DBFreeResult(hResult);
}


//
// Reindex database
//

void ReindexDatabase()
{
	if (!ValidateDatabase())
		return;

	if (g_iSyntax != DB_SYNTAX_ORACLE)
		RecreateIndex(_T("idx_raw_dci_values_item_id"), _T("raw_dci_values"), _T("item_id"));
	RecreateIndex(_T("idx_event_log_event_timestamp"), _T("event_log"), _T("event_timestamp"));
	RecreateIndex(_T("idx_thresholds_item_id"), _T("thresholds"), _T("item_id"));
	RecreateIndex(_T("idx_thresholds_sequence"), _T("thresholds"), _T("sequence_number"));
	RecreateIndex(_T("idx_alarm_notes_alarm_id"), _T("alarm_notes"), _T("alarm_id"));
	RecreateIndex(_T("idx_syslog_msg_timestamp"), _T("syslog"), _T("msg_timestamp"));
	RecreateIndex(_T("idx_snmp_trap_log_trap_timestamp"), _T("snmp_trap_log"), _T("trap_timestamp"));
	RecreateIndex(_T("idx_address_lists_list_type"), _T("address_lists"), _T("list_type"));

	ReindexIData();

	_tprintf(_T("Database reindexing complete.\n"));
}
