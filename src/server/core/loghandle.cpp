/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2024 Victor Kirhenshtein
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
#include <nxcore_logs.h>

/**
 * Check if given column should be read for "get detains" request only
 */
static inline bool IsDetailsColumn(int type)
{
   return (type == LC_TEXT_DETAILS) || (type == LC_JSON_DETAILS);
}

/**
 * Check if given column type should be ignored in query
 */
static inline bool IsIgnoredColumn(int type)
{
   return IsDetailsColumn(type) || (!IsZoningEnabled() && (type == LC_ZONE_UIN));
}

/**
 * Constructor
 */
LogHandle::LogHandle(const NXCORE_LOG *info)
{
	m_log = info;
	m_filter = nullptr;
	m_resultSet = nullptr;
	m_rowCountLimit = 1000;
	m_maxRecordId = 0;
}

/**
 * Destructor
 */
LogHandle::~LogHandle()
{
	deleteQueryResults();
	delete m_filter;
}

/**
 * Get column information
 */
void LogHandle::getColumnInfo(NXCPMessage *msg)
{
	uint32_t count = 0;
	uint32_t fieldId = VID_COLUMN_INFO_BASE;
	bool hasDetails = false;
	for(int i = 0; m_log->columns[i].name != nullptr; i++)
	{
	   if (IsIgnoredColumn(m_log->columns[i].type))
	   {
	      hasDetails = IsDetailsColumn(m_log->columns[i].type);
	      continue;   // ignore zone columns if zoning is disabled
	   }
		msg->setField(fieldId++, m_log->columns[i].name);
		msg->setField(fieldId++, static_cast<uint16_t>(m_log->columns[i].type));
		msg->setField(fieldId++, m_log->columns[i].description);
      msg->setField(fieldId++, m_log->columns[i].flags);
      fieldId += 6;
		count++;
	}
	msg->setField(VID_NUM_COLUMNS, count);
	msg->setField(VID_RECORD_ID_COLUMN, m_log->idColumn);
   msg->setField(VID_OBJECT_ID_COLUMN, m_log->relatedObjectIdColumn);
   msg->setField(VID_HAS_DETAIL_FIELDS, hasDetails);
}

/**
 * Get column definition by column name
 */
const LOG_COLUMN *LogHandle::getColumnDefinition(const TCHAR *name) const
{
   for(int i = 0; m_log->columns[i].name != nullptr; i++)
      if (!_tcsicmp(m_log->columns[i].name, name))
         return &m_log->columns[i];
   return nullptr;
}

/**
 * Delete query results
 */
void LogHandle::deleteQueryResults()
{
	if (m_resultSet != nullptr)
	{
		DBFreeResult(m_resultSet);
		m_resultSet = nullptr;
	}
}

/**
 * Build query column list
 */
void LogHandle::buildQueryColumnList()
{
	m_queryColumns.clear();
	const LOG_COLUMN *column = m_log->columns;
	bool first = true;
	while(column->name != nullptr)
	{
	   if (IsIgnoredColumn(column->type))
	   {
	      column++;
	      continue;
	   }

		if (!first)
		{
			m_queryColumns.append(_T(","));
		}
		else
		{
			first = false;
		}
		if ((column->flags & LCF_TSDB_TIMESTAMPTZ) && (g_dbSyntax == DB_SYNTAX_TSDB))
      {
         m_queryColumns.append(_T("date_part('epoch',"));
         m_queryColumns.append(column->name);
         m_queryColumns.append(_T(")::int"));
      }
		else
		{
		   m_queryColumns.append(column->name);
		}
		column++;
	}
}

/**
 * Do query according to filter
 */
bool LogHandle::query(LogFilter *filter, int64_t *rowCount, uint32_t userId)
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
	if (hResult != nullptr)
	{
		if (DBGetNumRows(hResult) > 0)
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
StringBuffer LogHandle::buildObjectAccessConstraint(uint32_t userId)
{
   StringBuffer constraint;
	unique_ptr<SharedObjectArray<NetObj>> objects = g_idxObjectById.getObjects();
   IntegerArray<uint32_t> allowed(objects->size());
   IntegerArray<uint32_t> restricted(objects->size());
	for(int i = 0; i < objects->size(); i++)
	{
		NetObj *object = objects->get(i);
      if (object->isEventSource())
      {
		   if (object->checkAccessRights(userId, OBJECT_ACCESS_READ))
		   {
            allowed.add(object->getId());
		   }
         else
         {
            restricted.add(object->getId());
         }
      }
	}

   if (restricted.isEmpty())
   {
      // no restriction
   }
   else if (allowed.isEmpty())
   {
      constraint.append(_T("1=0"));   // always false
   }
   else
   {
      IntegerArray<uint32_t> *list;
      if (allowed.size() < restricted.size())
      {
         list = &allowed;
      }
      else
      {
         list = &restricted;
         constraint.append(_T("NOT ("));
      }

      if (list->size() < 1000)
      {
         constraint.appendFormattedString(_T("%s IN ("), m_log->relatedObjectIdColumn);
         for(int i = 0; i < list->size(); i++)
         {
            constraint.append(list->get(i));
            constraint.append(_T(','));
         }
         constraint.shrink();
         constraint.append(_T(')'));
      }
      else
      {
         for(int i = 0; i < list->size(); i++)
         {
            constraint.append(_T('('));
            constraint.append(m_log->relatedObjectIdColumn);
            constraint.append(_T('='));
            constraint.append(list->get(i));
            constraint.append(_T(')'));
            constraint.append(_T("OR"));
         }
         constraint.shrink(2);
      }
      if (allowed.size() >= restricted.size())
      {
         constraint.append(_T(')'));
      }
   }
	return constraint;
}

/**
 * Do query with current filter and column set
 */
bool LogHandle::queryInternal(int64_t *rowCount, uint32_t userId)
{
	int64_t startTime = GetCurrentTimeMs();
	StringBuffer query;
	switch(g_dbSyntax)
	{
		case DB_SYNTAX_MSSQL:
			query.appendFormattedString(_T("SELECT TOP %u %s FROM %s "), m_rowCountLimit, m_queryColumns.cstr(), m_log->table);
			break;
		case DB_SYNTAX_INFORMIX:
			query.appendFormattedString(_T("SELECT FIRST %u %s FROM %s "), m_rowCountLimit, m_queryColumns.cstr(), m_log->table);
			break;
		case DB_SYNTAX_ORACLE:
			query.appendFormattedString(_T("SELECT * FROM (SELECT %s FROM %s"), m_queryColumns.cstr(), m_log->table);
			break;
      case DB_SYNTAX_DB2:
		case DB_SYNTAX_MYSQL:
      case DB_SYNTAX_PGSQL:
      case DB_SYNTAX_SQLITE:
      case DB_SYNTAX_TSDB:
			query.appendFormattedString(_T("SELECT %s FROM %s"), m_queryColumns.cstr(), m_log->table);
			break;
	}

	query.appendFormattedString(_T(" WHERE %s<=") INT64_FMT, m_log->idColumn, m_maxRecordId);

	int filterSize = m_filter->getNumColumnFilter();
	if (filterSize > 0)
	{
		for(int i = 0; i < filterSize; i++)
		{
			ColumnFilter *cf = m_filter->getColumnFilter(i);
			query.append(_T(" AND ("));
			query.append(cf->generateSql());
			query.append(_T(")"));
		}
	}

   if ((userId != 0) && (m_log->relatedObjectIdColumn != nullptr) && ConfigReadBoolean(_T("Server.Security.ExtendedLogQueryAccessControl"), false))
   {
		String constraint = buildObjectAccessConstraint(userId);
		if (!constraint.isEmpty()) 
      {
			query += _T(" AND (");
			query += constraint;
			query += _T(")");
		}
	}

	query.append(m_filter->buildOrderClause());

	// Limit record count
	switch(g_dbSyntax)
	{
		case DB_SYNTAX_MYSQL:
		case DB_SYNTAX_PGSQL:
		case DB_SYNTAX_SQLITE:
      case DB_SYNTAX_TSDB:
			query.appendFormattedString(_T(" LIMIT %u"), m_rowCountLimit);
			break;
		case DB_SYNTAX_ORACLE:
			query.appendFormattedString(_T(") WHERE ROWNUM<=%u"), m_rowCountLimit);
			break;
		case DB_SYNTAX_DB2:
			query.appendFormattedString(_T(" FETCH FIRST %u ROWS ONLY"), m_rowCountLimit);
			break;
	}

	nxlog_debug_tag(DEBUG_TAG_LOGS, 4, _T("LOG QUERY: %s"), (const TCHAR *)query);

	DB_HANDLE dbHandle = DBConnectionPoolAcquireConnection();
	bool ret = false;
	nxlog_debug_tag(DEBUG_TAG_LOGS, 7, _T("LogHandle::query(): DB connection acquired"));
	m_resultSet = DBSelect(dbHandle, (const TCHAR *)query);
	if (m_resultSet != NULL)
	{
		*rowCount = DBGetNumRows(m_resultSet);
		ret = true;
		nxlog_debug_tag(DEBUG_TAG_LOGS, 4, _T("Log query successful, %d rows fetched in %d ms"), (int)(*rowCount), (int)(GetCurrentTimeMs() - startTime));
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

   const LOG_COLUMN *column = m_log->columns;
   while(column->name != nullptr)
   {
      if (!IsIgnoredColumn(column->type))
         table->addColumn(column->name);
      column++;
   }

   return table;
}

/**
 * Get data from query result
 */
Table *LogHandle::getData(int64_t startRow, int64_t numRows, bool refresh, uint32_t userId)
{
	nxlog_debug(4, _T("Log data request: startRow=") INT64_FMT _T(", numRows=") INT64_FMT _T(", refresh=%s, userId=%u"),
	         startRow, numRows, BooleanToString(refresh), userId);

	if (m_resultSet == nullptr)
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
			uint32_t newLimit = (UINT32)(startRow + numRows);
			if (newLimit > m_rowCountLimit)
				m_rowCountLimit = (newLimit - m_rowCountLimit < 1000) ? (m_rowCountLimit + 1000) : newLimit;
			deleteQueryResults();
			int64_t rowCount;
			if (!queryInternal(&rowCount, userId))
				return nullptr;
			resultSize = DBGetNumRows(m_resultSet);
		}
	}

	Table *table = createTable();
	int maxRow = std::min((int)(startRow + numRows), resultSize);
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

/**
 * Get details for specific record
 */
void LogHandle::getRecordDetails(int64_t recordId, NXCPMessage *msg)
{
   StringBuffer query(_T("SELECT "));

   int count = 0;
   uint32_t fieldId = VID_COLUMN_INFO_BASE;
   for(int i = 0; m_log->columns[i].name != nullptr; i++)
   {
      if (!IsDetailsColumn(m_log->columns[i].type) && !(m_log->columns[i].flags & LCF_INCLUDE_IN_DETAILS))
         continue;
      if (count > 0)
         query.append(_T(","));
      query.append(m_log->columns[i].name);
      msg->setField(fieldId++, m_log->columns[i].name);
      msg->setField(fieldId++, m_log->columns[i].type);
      msg->setField(fieldId++, m_log->columns[i].description);
      fieldId += 7;
      count++;
   }

   if (count == 0)
   {
      msg->setField(VID_RCC, RCC_RECORD_DETAILS_UNAVAILABLE);
      return;
   }
   msg->setField(VID_NUM_COLUMNS, count);

   query.append(_T(" FROM "));
   query.append(m_log->table);
   query.append(_T(" WHERE "));
   query.append(m_log->idColumn);
   query.append(_T("="));
   query.append(recordId);

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, query);
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         fieldId = VID_TABLE_DATA_BASE;
         for(int i = 0; i < count; i++)
         {
            TCHAR *v = DBGetField(hResult, 0, i, nullptr, 0);
            msg->setField(fieldId++, v);
            MemFree(v);
         }
         msg->setField(VID_RCC, RCC_SUCCESS);
      }
      else
      {
         msg->setField(VID_RCC, RCC_NO_SUCH_RECORD);
      }
      DBFreeResult(hResult);
   }
   else
   {
      msg->setField(VID_RCC, RCC_DB_FAILURE);
   }
   DBConnectionPoolReleaseConnection(hdb);
}
