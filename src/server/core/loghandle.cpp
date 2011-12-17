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
// Get column information
//

void LogHandle::getColumnInfo(CSCPMessage &msg)
{
	DWORD count = 0;
	DWORD varId = VID_COLUMN_INFO_BASE;
	for(int i = 0; m_log->columns[i].name != NULL; i++, count++, varId += 7)
	{
		msg.SetVariable(varId++, m_log->columns[i].name);
		msg.SetVariable(varId++, (WORD)m_log->columns[i].type);
		msg.SetVariable(varId++, m_log->columns[i].description);
	}
	msg.SetVariable(VID_NUM_COLUMNS, count);
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

	query.addFormattedString(_T("DROP TABLE %s"), m_tempTable);

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

bool LogHandle::query(LogFilter *filter, INT64 *rowCount)
{
	deleteQueryResults();
	m_filter = filter;

	if (m_log == NULL)
	{
		return false;
	}

	m_queryColumns = _T("");
	LOG_COLUMN *column = m_log->columns;
	bool first = true;
	while ((*column).name != NULL)
	{
		if (!first)
		{
			m_queryColumns += _T(",");
		}
		else
		{
			first = false;
		}
		m_queryColumns += (*column).name;
		column++;
	}

	String query;
	switch(g_nDBSyntax)
	{
		case DB_SYNTAX_MSSQL:
			query.addFormattedString(_T("SELECT TOP 10000 %s INTO %s FROM %s "), (const TCHAR *)m_queryColumns, m_tempTable, m_log->table);
			break;
		case DB_SYNTAX_INFORMIX:
			query.addFormattedString(_T("SELECT FIRST 10000 %s INTO %s FROM %s "), (const TCHAR *)m_queryColumns, m_tempTable, m_log->table);
			break;
		case DB_SYNTAX_ORACLE:
		case DB_SYNTAX_SQLITE:
		case DB_SYNTAX_PGSQL:
		case DB_SYNTAX_MYSQL:
			query.addFormattedString(_T("CREATE TABLE %s AS SELECT %s FROM %s"), m_tempTable, (const TCHAR *)m_queryColumns, m_log->table);
			break;
	}

	int filterSize = filter->getNumColumnFilter();
	if (filterSize > 0)
	{
		query += _T(" WHERE ");

		for(int i = 0; i < filterSize; i++)
		{
			ColumnFilter *cf = filter->getColumnFilter(i);

			if (i > 0)
			{
				query += _T(" AND ");
			}

			query += _T("(");
			query += cf->generateSql();
			query += _T(")");
		}
	}

	query += filter->buildOrderClause();
	
	// Limit record count
	switch(g_nDBSyntax)
	{
		case DB_SYNTAX_MYSQL:
		case DB_SYNTAX_PGSQL:
		case DB_SYNTAX_SQLITE:
			query += _T(" LIMIT 10000");
			break;
		case DB_SYNTAX_DB2:
			query += _T(" FETCH FIRST 10000 ROWS ONLY");
			break;
	}

	DbgPrintf(4, _T("LOG QUERY: %s"), (const TCHAR *)query);

	DB_HANDLE dbHandle = DBConnectionPoolAcquireConnection();

	bool ret = false;
	if (dbHandle != NULL)
	{
		DbgPrintf(7, _T("LogHandle::query(): DB connection acquired"));
		ret = DBQuery(dbHandle, query) != FALSE;
		if (ret)
		{
			TCHAR query2[256];

			_sntprintf(query2, 256, _T("SELECT count(*) FROM %s"), m_tempTable);
			DB_RESULT hResult = DBSelect(dbHandle, query2);
			if (hResult != NULL)
			{
				*rowCount = DBGetFieldInt64(hResult, 0, 0);
				DBFreeResult(hResult);
				DbgPrintf(7, _T("LogHandle::query(): ") INT64_FMT _T(" records in result set"), *rowCount);
			}
			else
			{
				ret = false;
			}
		}
		DBConnectionPoolReleaseConnection(dbHandle);
	}
	else
	{
		DbgPrintf(3, _T("LogHandle::query(): unable to acquire DB connection"));
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
			query.addFormattedString(_T("SELECT TOP ") INT64_FMT _T(" %s FROM %s"), startRow + numRows, (const TCHAR *)m_queryColumns, m_tempTable);
			break;
		case DB_SYNTAX_ORACLE:
			query.addFormattedString(_T("SELECT %s FROM (SELECT %s,ROWNUM AS R FROM %s) WHERE R BETWEEN ") INT64_FMT _T(" AND ") INT64_FMT,
			                         (const TCHAR *)m_queryColumns, (const TCHAR *)m_queryColumns, m_tempTable, startRow + 1, startRow + numRows);
			break;
		case DB_SYNTAX_PGSQL:
		case DB_SYNTAX_SQLITE:
		case DB_SYNTAX_MYSQL:
			query.addFormattedString(_T("SELECT %s FROM %s LIMIT ") INT64_FMT _T(" OFFSET ") INT64_FMT, (const TCHAR *)m_queryColumns, m_tempTable, numRows, startRow);
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
			for (int i = offset; i < resultSize; i++)
			{
				table->addRow();
				for (int j = 0; j < columnCount; j++)
				{
					table->setPreallocated(j, DBGetField(result, i, j, NULL, 0));
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
