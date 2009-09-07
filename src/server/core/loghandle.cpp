/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2009 Victor Kirhenshtein
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
** File: loghandle.cpp
**
**/

#include "nxcore.h"


//
// Constructor
//

LogHandle::LogHandle(NXCORE_LOG *info)
{
	m_log = info;
	m_isDataReady = false;

   switch(g_nDBSyntax)
	{
	case DB_SYNTAX_MSSQL:
		_sntprintf(m_tempTable, 64, _T("#log_%p"), this);
		break;
	default:
		_sntprintf(m_tempTable, 64, _T("log_%p"), this);
		break;
	}

	m_rowCount = 0;
	m_filter = NULL;
	m_lock = MutexCreate();

	m_dbHandle = DBConnect();

	m_queryColumns[0] = 0;
}


//
// Destructor
//

LogHandle::~LogHandle()
{
	deleteQueryResults();
	MutexDestroy(m_lock);

	DBDisconnect(m_dbHandle);
}


//
// Delete query results
//

void LogHandle::deleteQueryResults()
{
	if (!m_isDataReady)
		return;

	TCHAR query[MAX_DB_STRING];

	_sntprintf(query, MAX_DB_STRING, _T("DROP TABLE %s"), m_tempTable);
	DBQuery(m_dbHandle, query);

	delete_and_null(m_filter);
	m_isDataReady = false;
}


//
// Do query according to filter
//
// TODO: refactor and split into smaller methods
//

bool LogHandle::query(LogFilter *filter)
{
	deleteQueryResults();
	m_filter = filter;

	if (m_log == NULL)
	{
		return false;
	}

	printf("LogHandle::query()\n");

	m_queryColumns[0] = 0;
	LOG_COLUMN *column = m_log->columns;
	bool first = true;
	while ((*column).name != NULL)
	{
		if (!first)
		{
			_tcscat(m_queryColumns, _T(","));
		}
		else
		{
			first = false;
		}
		_tcscat(m_queryColumns, (*column).name);
		column++;
	}

   TCHAR query[MAX_DB_STRING];
   switch(g_nDBSyntax)
	{
	case DB_SYNTAX_MSSQL:
		_sntprintf(query, MAX_DB_STRING, _T("SELECT %s INTO %s from %s "), m_queryColumns, m_tempTable, m_log->table);
		break;
   case DB_SYNTAX_ORACLE:
   case DB_SYNTAX_SQLITE:
   case DB_SYNTAX_PGSQL:
   case DB_SYNTAX_MYSQL:
		_sntprintf(query, MAX_DB_STRING, _T("CREATE TEMPORARY TABLE %s AS SELECT %s FROM %s"), m_tempTable, m_queryColumns, m_log->table);
		break;
	}

	int filterSize = filter->getNumColumnFilter();
	if (filterSize > 0)
	{
		_tcscat(query, " WHERE ");
	}

	for (int i = 0; i < filterSize; i++)
	{
		ColumnFilter *cf = filter->getColumnFilter(i);

		if (i > 0)
		{
			_tcscat(query, " AND ");
		}

		TCHAR *sql = cf->generateSql();
		_tcscat(query, sql);
		delete sql;
	}

	bool ret = DBQuery(m_dbHandle, query) != FALSE;

	if (ret)
	{
		m_isDataReady = true;
	}

	return ret;
}


//
// Get data from query result
//

Table *LogHandle::getData(INT64 startRow, INT64 numRows)
{
	if (!m_isDataReady)
		return NULL;

	Table *table = new Table();

	LOG_COLUMN *column = m_log->columns;
	bool first = true;
	int columnCount = 0;
	while ((*column).name != NULL)
	{
		table->addColumn((TCHAR *)((*column).name));

		column++;
		columnCount++;
	}

	TCHAR query[MAX_DB_STRING];

   switch(g_nDBSyntax)
	{
	case DB_SYNTAX_MSSQL:
		_sntprintf(query, MAX_DB_STRING, _T("SELECT TOP %d %s FROM %s"),
			startRow + numRows, m_queryColumns, m_tempTable);
		break;
   case DB_SYNTAX_ORACLE:
		_sntprintf(query, MAX_DB_STRING, _T("SELECT %s FROM %s WHERE ROWNUM BETWEEN %d AND %d"),
			m_queryColumns, m_tempTable, startRow, startRow + numRows);
		break;
   case DB_SYNTAX_PGSQL:
   case DB_SYNTAX_SQLITE:
   case DB_SYNTAX_MYSQL:
		_sntprintf(query, MAX_DB_STRING, _T("SELECT %s FROM %s LIMIT %d OFFSET %d"),
			m_queryColumns, m_tempTable, startRow, numRows);
		break;
	}

	DB_RESULT result = DBSelect(m_dbHandle, query);
	int resultSize = DBGetNumRows(result);
	int i = max(0, resultSize - numRows); // MSSQL workaround
	for (; i < resultSize; i++)
	{
		table->addRow();
		for (int j = 0; j < columnCount; j++)
		{
			table->set(i, DBGetField(m_dbHandle, i, j, NULL, 0));
		}
	}
	DBFreeResult(result);

	return table;
}
