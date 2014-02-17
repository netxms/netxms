/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2013 Victor Kirhenshtein
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


/**
 * Constructor
 */
LogHandle::LogHandle(NXCORE_LOG *info)
{
	m_log = info;
	m_filter = NULL;
	m_lock = MutexCreate();
	m_resultSet = NULL;
	m_rowCountLimit = 1000;
}

/**
 * Destructor
 */
LogHandle::~LogHandle()
{
	deleteQueryResults();
	delete m_filter;
	MutexDestroy(m_lock);
}

/**
 * Get column information
 */
void LogHandle::getColumnInfo(CSCPMessage &msg)
{
	UINT32 count = 0;
	UINT32 varId = VID_COLUMN_INFO_BASE;
	for(int i = 0; m_log->columns[i].name != NULL; i++, count++, varId += 7)
	{
		msg.SetVariable(varId++, m_log->columns[i].name);
		msg.SetVariable(varId++, (WORD)m_log->columns[i].type);
		msg.SetVariable(varId++, m_log->columns[i].description);
	}
	msg.SetVariable(VID_NUM_COLUMNS, count);
}

/**
 * Delete query results
 */
void LogHandle::deleteQueryResults()
{
	if (m_resultSet != NULL)
	{
		DBFreeResult(m_resultSet);
		m_resultSet = NULL;
	}
}

/**
 * Build query column list
 */
void LogHandle::buildQueryColumnList()
{
	m_queryColumns = _T("");
	LOG_COLUMN *column = m_log->columns;
	bool first = true;
	while(column->name != NULL)
	{
		if (!first)
		{
			m_queryColumns += _T(",");
		}
		else
		{
			first = false;
		}
		m_queryColumns += column->name;
		column++;
	}
}

/**
 * Do query according to filter
 */
bool LogHandle::query(LogFilter *filter, INT64 *rowCount, const UINT32 userId)
{
	deleteQueryResults();
	delete m_filter;
	m_filter = filter;

	buildQueryColumnList();

	m_maxRecordId = -1;
	TCHAR query[256];
	_sntprintf(query, 256, _T("SELECT coalesce(max(%s),0) FROM %s"), m_log->idColumn, m_log->table);
	DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
	DB_RESULT hResult = DBSelect(hdb, query);
	if (hResult != NULL)
	{
		if (DBGetNumRows > 0)
			m_maxRecordId = DBGetFieldInt64(hResult, 0, 0);
		DBFreeResult(hResult);
	}
	DBConnectionPoolReleaseConnection(hdb);
	if (m_maxRecordId < 0)
		return false;

	return queryInternal(rowCount, userId);
}

/**
 * Creates a SQL WHERE clause for restricting log to only objects accessible by given user.
 */
String LogHandle::buildObjectAccessConstraint(const UINT32 userId) 
{
   String constraint;
	ObjectArray<NetObj> *objects = g_idxObjectById.getObjects(true);
   IntegerArray<UINT32> *allowed = new IntegerArray<UINT32>(objects->size());
   IntegerArray<UINT32> *restricted = new IntegerArray<UINT32>(objects->size());
	for(int i = 0; i < objects->size(); i++)
	{
		NetObj *object = objects->get(i);
      if (object->isEventSource())
      {
		   if (object->checkAccessRights(userId, OBJECT_ACCESS_READ))
		   {
            allowed->add(object->Id());
		   }
         else
         {
            restricted->add(object->Id());
         }
      }
      object->decRefCount();
	}
   delete objects;

   if (restricted->size() == 0)
   {
      // no restriction
   }
   else if (allowed->size() == 0)
   {
      constraint += _T("1=0");   // always false
   }
   else
   {
      IntegerArray<UINT32> *list;
      if (allowed->size() < restricted->size())
      {
         list = allowed;
      }
      else
      {
         list = restricted;
         constraint += _T("NOT (");
      }

      if (list->size() < 1000)
      {
         constraint.addFormattedString(_T("%s IN ("), m_log->relatedObjectIdColumn);
         for(int i = 0; i < list->size(); i++)
         {
            TCHAR buffer[32];
            _sntprintf(buffer, 32, _T("%d,"), list->get(i));
            constraint += buffer;
         }
         constraint.shrink();
         constraint += _T(")");
      }
      else
      {
         for(int i = 0; i < list->size(); i++)
         {
            TCHAR buffer[32];
   			constraint.addFormattedString(_T("(%s=%d)OR"), m_log->relatedObjectIdColumn, list->get(i));
            constraint += buffer;
         }
         constraint.shrink(2);
      }
      if (allowed->size() >= restricted->size())
      {
         constraint += _T(")");
      }
   }
   delete allowed;
   delete restricted;
	return constraint;
}

/**
 * Do query with current filter and column set
 */
bool LogHandle::queryInternal(INT64 *rowCount, const UINT32 userId)
{
	QWORD qwTimeStart = GetCurrentTimeMs();
	String query;
	switch(g_nDBSyntax)
	{
		case DB_SYNTAX_MSSQL:
			query.addFormattedString(_T("SELECT TOP %u %s FROM %s "), m_rowCountLimit, (const TCHAR *)m_queryColumns, m_log->table);
			break;
		case DB_SYNTAX_INFORMIX:
			query.addFormattedString(_T("SELECT FIRST %u %s FROM %s "), m_rowCountLimit, (const TCHAR *)m_queryColumns, m_log->table);
			break;
		case DB_SYNTAX_ORACLE:
			query.addFormattedString(_T("SELECT * FROM (SELECT %s FROM %s"), (const TCHAR *)m_queryColumns, m_log->table);
			break;
		case DB_SYNTAX_SQLITE:
		case DB_SYNTAX_PGSQL:
		case DB_SYNTAX_MYSQL:
		case DB_SYNTAX_DB2:
			query.addFormattedString(_T("SELECT %s FROM %s"), (const TCHAR *)m_queryColumns, m_log->table);
			break;
	}

	query.addFormattedString(_T(" WHERE %s<=") INT64_FMT, m_log->idColumn, m_maxRecordId);

	int filterSize = m_filter->getNumColumnFilter();
	if (filterSize > 0)
	{
		for(int i = 0; i < filterSize; i++)
		{
			ColumnFilter *cf = m_filter->getColumnFilter(i);
			query += _T(" AND (");
			query += cf->generateSql();
			query += _T(")");
		}
	}

	if ((userId != 0) && ConfigReadInt(_T("ExtendedLogQueryAccessControl"), 0))
   {
		String constraint = buildObjectAccessConstraint(userId);
		if (!constraint.isEmpty()) 
      {
			query += _T(" AND (");
			query += constraint;
			query += _T(")");
		}
	}

	query += m_filter->buildOrderClause();

	// Limit record count
	switch(g_nDBSyntax)
	{
		case DB_SYNTAX_MYSQL:
		case DB_SYNTAX_PGSQL:
		case DB_SYNTAX_SQLITE:
			query.addFormattedString(_T(" LIMIT %u"), m_rowCountLimit);
			break;
		case DB_SYNTAX_ORACLE:
			query.addFormattedString(_T(") WHERE ROWNUM<=%u"), m_rowCountLimit);
			break;
		case DB_SYNTAX_DB2:
			query.addFormattedString(_T(" FETCH FIRST %u ROWS ONLY"), m_rowCountLimit);
			break;
	}

	DbgPrintf(4, _T("LOG QUERY: %s"), (const TCHAR *)query);

	DB_HANDLE dbHandle = DBConnectionPoolAcquireConnection();
	bool ret = false;
	DbgPrintf(7, _T("LogHandle::query(): DB connection acquired"));
	m_resultSet = DBSelect(dbHandle, (const TCHAR *)query);
	if (m_resultSet != NULL)
	{
		*rowCount = DBGetNumRows(m_resultSet);
		ret = true;
		DbgPrintf(4, _T("Log query successfull, %d rows fetched in %d ms"), (int)(*rowCount), GetCurrentTimeMs() - qwTimeStart);
	}
	DBConnectionPoolReleaseConnection(dbHandle);

	return ret;
}

/**
 * Create table for sending data to client
 */
Table *LogHandle::createTable()
{
	Table *table = new Table();

	LOG_COLUMN *column = m_log->columns;
	bool first = true;
	int columnCount = 0;
	while (column->name != NULL)
	{
		table->addColumn(column->name);
		column++;
		columnCount++;
	}

	return table;
}

/**
 * Get data from query result
 */
Table *LogHandle::getData(INT64 startRow, INT64 numRows, bool refresh, const UINT32 userId)
{
	DbgPrintf(4, _T("Log data request: startRow=%d, numRows=%d, refresh=%s, userId=%d"),
	          (int)startRow, (int)numRows, refresh ? _T("true") : _T("false"), userId);

	if (m_resultSet == NULL)
		return createTable();	// send empty table to indicate end of data

	int resultSize = DBGetNumRows(m_resultSet);
	if (((int)(startRow + numRows) >= resultSize) || refresh)
	{
		if ((resultSize < (int)m_rowCountLimit) && !refresh)
		{
			if (startRow >= resultSize)
				return createTable();	// send empty table to indicate end of data
		}
		else
		{
			// possibly we have more rows or refresh was requested
			UINT32 newLimit = (UINT32)(startRow + numRows);
			if (newLimit > m_rowCountLimit)
				m_rowCountLimit = (newLimit - m_rowCountLimit < 1000) ? (m_rowCountLimit + 1000) : newLimit;
			deleteQueryResults();
			INT64 rowCount;
			if (!queryInternal(&rowCount, userId))
				return NULL;
			resultSize = DBGetNumRows(m_resultSet);
		}
	}

	Table *table = createTable();
	int maxRow = (int)min((int)(startRow + numRows), resultSize);
	for(int i = (int)startRow; i < maxRow; i++)
	{
		table->addRow();
		for(int j = 0; j < table->getNumColumns(); j++)
		{
			table->setPreallocated(j, DBGetField(m_resultSet, i, j, NULL, 0));
		}
	}

	return table;
}
