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
	_sntprintf(m_tempTable, 64, _T("log_%p"), this);
	m_rowCount = 0;
	m_filter = NULL;
	m_lock = MutexCreate();

	m_queryColumns[0] = 0;
}


//
// Destructor
//

LogHandle::~LogHandle()
{
	deleteQueryResults();
	MutexDestroy(m_lock);
}


//
// Delete query results
//

void LogHandle::deleteQueryResults()
{
	if (!m_isDataReady)
	{
		return;
	}

	String query;

	query.AddFormattedString(_T("DROP TABLE %s"), m_tempTable);

	DB_HANDLE dbHandle = DBConnectionPoolAcquireConnection();
	if (dbHandle != NULL)
	{
		DBQuery(dbHandle, query);

		DBConnectionPoolReleaseConnection(dbHandle);
	}

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

	String query;
	switch(g_nDBSyntax)
	{
	case DB_SYNTAX_MSSQL:
		query.AddFormattedString(_T("SELECT %s INTO %s from %s "), m_queryColumns, m_tempTable, m_log->table);
		break;
	case DB_SYNTAX_ORACLE:
	case DB_SYNTAX_SQLITE:
	case DB_SYNTAX_PGSQL:
	case DB_SYNTAX_MYSQL:
		query.AddFormattedString(_T("CREATE TABLE %s AS SELECT %s FROM %s"), m_tempTable, m_queryColumns, m_log->table);
		break;
	}

	int filterSize = filter->getNumColumnFilter();
	if (filterSize > 0)
	{
		query += _T(" WHERE ");
	}

	for (int i = 0; i < filterSize; i++)
	{
		ColumnFilter *cf = filter->getColumnFilter(i);

		if (i > 0)
		{
			query += _T(" AND ");
		}

		query += cf->generateSql();
	}

	DB_HANDLE dbHandle = DBConnectionPoolAcquireConnection();

	bool ret = false;
	if (dbHadle != NULL)
	{
		ret = DBQuery(dbHandle, query) != FALSE;
		DBConnectionPoolReleaseConnection(dbHandle);
	}

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
	{
		return NULL;
	}

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

	String query;

	switch(g_nDBSyntax)
	{
	case DB_SYNTAX_MSSQL:
		query.AddFormattedString(_T("SELECT TOP ") INT64_FMT _T(" %s FROM %s"), startRow + numRows, m_queryColumns, m_tempTable);
		break;
	case DB_SYNTAX_ORACLE:
		query.AddFormattedString(_T("SELECT %s FROM %s WHERE ROWNUM BETWEEN ") INT64_FMT _T(" AND ") INT64_FMT, m_queryColumns, m_tempTable, startRow, startRow + numRows);
		break;
	case DB_SYNTAX_PGSQL:
	case DB_SYNTAX_SQLITE:
	case DB_SYNTAX_MYSQL:
		query.AddFormattedString(_T("SELECT %s FROM %s LIMIT ") INT64_FMT _T(" OFFSET ") INT64_FMT, m_queryColumns, m_tempTable, numRows, startRow);
		break;
	}

	DB_HANDLE dbHandle = DBConnectionPoolAcquireConnection();
	if (dbHandle != NULL)
	{
		DB_RESULT result = DBSelect(dbHandle, query);

		if (result != NULL)
		{
			int resultSize = DBGetNumRows(result);
			int offset = (int)max(0, resultSize - (int)numRows); // MSSQL workaround
			for (int i = 0; i < resultSize; i++)
			{
				table->addRow();
				for (int j = 0; j < columnCount; j++)
				{
					table->set(j, DBGetField(result, offset + i, j, NULL, 0));
				}
			}

			DBFreeResult(result);
		}
		else
		{
			delete_and_null(table);
		}

		DBConnectionPoolReleaseConnection(dbHandle);
	}
	else
	{
		delete_and_null(table);
	}

	return table;
}
