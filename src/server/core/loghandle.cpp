/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Victor Kirhenshtein
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
 * Check if given column should not be visible to client at all
 */
static inline bool IsHiddenColumn(int type)
{
   return !IsZoningEnabled() && (type == LC_ZONE_UIN);
}

/**
 * Check if given column type should be ignored in query
 */
static inline bool IsIgnoredColumn(int type)
{
   return IsDetailsColumn(type) || IsHiddenColumn(type);
}

/**
 * Append column to query column list, projecting TimescaleDB timestamptz columns as epoch values
 */
static void AppendQueryColumn(StringBuffer *columnList, const LOG_COLUMN& column)
{
   if ((column.flags & LCF_TSDB_TIMESTAMPTZ) && (g_dbSyntax == DB_SYNTAX_TSDB))
   {
      if (column.type == LC_TIMESTAMP_MS)
      {
         // Millisecond timestamp column - project timestamptz as epoch milliseconds
         columnList->append(L"timestamptz_to_ms(");
         columnList->append(column.name);
         columnList->append(L")");
      }
      else
      {
         // Project timestamptz as epoch seconds
         columnList->append(L"date_part('epoch',");
         columnList->append(column.name);
         columnList->append(L")::int");
      }
   }
   else
   {
      columnList->append(column.name);
   }
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
		AppendQueryColumn(&m_queryColumns, *column);
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
 * Build SQL query from filter
 *
 * @param filter log filter to use
 * @param maxRecordId maximum record ID to include (pass -1 to omit this constraint)
 * @param userId user ID for access control (pass 0 to skip access control check)
 * @return SQL query string
 */
StringBuffer LogHandle::buildQuerySql(LogFilter *filter, int64_t maxRecordId, uint32_t userId)
{
   buildQueryColumnList();

   StringBuffer query;
   switch(g_dbSyntax)
   {
      case DB_SYNTAX_MSSQL:
         query.appendFormattedString(_T("SELECT TOP %u %s FROM %s"), m_rowCountLimit, m_queryColumns.cstr(), m_log->table);
         break;
      case DB_SYNTAX_INFORMIX:
         query.appendFormattedString(_T("SELECT FIRST %u %s FROM %s"), m_rowCountLimit, m_queryColumns.cstr(), m_log->table);
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

   bool hasWhereClause = false;
   if (maxRecordId >= 0)
   {
      query.appendFormattedString(_T(" WHERE %s<=") INT64_FMT, m_log->idColumn, maxRecordId);
      hasWhereClause = true;
   }

   int filterSize = filter->getNumColumnFilter();
   if (filterSize > 0)
   {
      for(int i = 0; i < filterSize; i++)
      {
         ColumnFilter *cf = filter->getColumnFilter(i);
         query.append(hasWhereClause ? _T(" AND (") : _T(" WHERE ("));
         query.append(cf->generateSql());
         query.append(_T(")"));
         hasWhereClause = true;
      }
   }

   if ((userId != 0) && (m_log->relatedObjectIdColumn != nullptr) && ConfigReadBoolean(_T("Server.Security.ExtendedLogQueryAccessControl"), false))
   {
      String constraint = buildObjectAccessConstraint(userId);
      if (!constraint.isEmpty())
      {
         query.append(hasWhereClause ? _T(" AND (") : _T(" WHERE ("));
         query.append(constraint);
         query.append(_T(")"));
      }
   }

   query.append(filter->buildOrderClause());

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

   return query;
}

/**
 * Do query with current filter and column set
 */
bool LogHandle::queryInternal(int64_t *rowCount, uint32_t userId)
{
   int64_t startTime = GetCurrentTimeMs();
   StringBuffer query = buildQuerySql(m_filter, m_maxRecordId, userId);

   nxlog_debug_tag(DEBUG_TAG_LOGS, 4, _T("LOG QUERY: %s"), query.cstr());

   DB_HANDLE dbHandle = DBConnectionPoolAcquireConnection();
   bool ret = false;
   nxlog_debug_tag(DEBUG_TAG_LOGS, 7, _T("LogHandle::query(): DB connection acquired"));
   m_resultSet = DBSelect(dbHandle, query.cstr());
   if (m_resultSet != nullptr)
   {
      *rowCount = DBGetNumRows(m_resultSet);
      ret = true;
      nxlog_debug_tag(DEBUG_TAG_LOGS, 4, _T("Log query successful, %d rows fetched in %d ms"), static_cast<int>(*rowCount), static_cast<int>(GetCurrentTimeMs() - startTime));
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

/**
 * Get column information as JSON array
 */
json_t *LogHandle::getColumnInfoAsJson() const
{
   json_t *columns = json_array();
   for(int i = 0; m_log->columns[i].name != nullptr; i++)
   {
      const LOG_COLUMN& column = m_log->columns[i];
      if (IsHiddenColumn(column.type))
         continue;

      json_t *json = json_object();
      json_object_set_new(json, "name", json_string_w(column.name));
      json_object_set_new(json, "description", json_string_w(column.description));
      json_object_set_new(json, "type", json_string(LogColumnTypeName(column.type)));
      json_object_set_new(json, "recordId", json_boolean((column.flags & LCF_RECORD_ID) != 0));
      json_object_set_new(json, "detail", json_boolean(IsDetailsColumn(column.type)));
      json_array_append_new(columns, json);
   }
   return columns;
}

/**
 * Convert single value from query result to JSON according to column type
 */
static json_t *ColumnValueToJson(DB_RESULT hResult, int row, int column, const LOG_COLUMN& definition)
{
   switch(definition.type)
   {
      case LC_TIMESTAMP:
         return json_time_string(static_cast<time_t>(DBGetFieldInt64(hResult, row, column)));
      case LC_TIMESTAMP_MS:
         return json_time_string_ms(DBGetFieldInt64(hResult, row, column));
      case LC_ACTION_CODE:
      case LC_AI_OP_EXEC_STATUS:
      case LC_AI_TASK_STATUS:
      case LC_ALARM_HD_STATE:
      case LC_ALARM_STATE:
      case LC_ASSET_OPERATION:
      case LC_ATM_TXN_CODE:
      case LC_COMPLETION_STATUS:
      case LC_CONNECTION_EVENT:
      case LC_DEPLOYMENT_STATUS:
      case LC_EVENT_CODE:
      case LC_EVENT_ORIGIN:
      case LC_INTEGER:
      case LC_OBJECT_ID:
      case LC_OBSERVATION_STATE:
      case LC_SEVERITY:
      case LC_USER_ID:
      case LC_ZONE_UIN:
         return json_integer(DBGetFieldInt64(hResult, row, column));
      case LC_JSON_DETAILS:
         {
            char *value = DBGetFieldUTF8(hResult, row, column, nullptr, 0);
            if (value == nullptr)
               return json_null();
            json_t *json = json_loads(value, 0, nullptr);
            if (json == nullptr)
               json = json_string(value);   // not a valid JSON document - pass through as text
            MemFree(value);
            return json;
         }
      default:
         {
            wchar_t *value = DBGetField(hResult, row, column, nullptr, 0);
            json_t *json = json_string_w(value);
            MemFree(value);
            return json;
         }
   }
}

/**
 * Create JSON document for single record from query result. Column set must match the one
 * used to build the query (detail columns are omitted from log queries).
 */
json_t *LogHandle::createRecordFromDBResult(DB_RESULT hResult, int row, bool includeDetailColumns)
{
   json_t *record = json_object();
   int index = 0;
   for(int i = 0; m_log->columns[i].name != nullptr; i++)
   {
      const LOG_COLUMN& column = m_log->columns[i];
      if (includeDetailColumns ? IsHiddenColumn(column.type) : IsIgnoredColumn(column.type))
         continue;

      char name[MAX_COLUMN_NAME_LEN * 3];
      wchar_to_utf8(column.name, -1, name, sizeof(name));
      json_object_set_new(record, name, ColumnValueToJson(hResult, row, index++, column));
   }
   return record;
}

/**
 * Build SQL query for given filter and page of records. Sets row count limit to cover
 * requested page, so that records to be skipped are still selected by the query.
 */
StringBuffer LogHandle::buildPagedQuerySql(LogFilter *filter, int64_t offset, int64_t limit, uint32_t userId)
{
   m_rowCountLimit = static_cast<uint32_t>(offset + limit);
   return buildQuerySql(filter, -1, userId);
}

/**
 * Execute query and return requested page of records as JSON array. Returns nullptr on database failure.
 */
json_t *LogHandle::queryAsJson(LogFilter *filter, int64_t offset, int64_t limit, uint32_t userId)
{
   StringBuffer query = buildPagedQuerySql(filter, offset, limit, userId);
   nxlog_debug_tag(DEBUG_TAG_LOGS, 4, L"LOG QUERY: %s", query.cstr());

   int64_t startTime = GetCurrentTimeMs();
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, query);
   json_t *records;
   if (hResult != nullptr)
   {
      records = json_array();
      int numRows = DBGetNumRows(hResult);
      for(int i = static_cast<int>(offset); i < numRows; i++)
         json_array_append_new(records, createRecordFromDBResult(hResult, i, false));
      DBFreeResult(hResult);
      nxlog_debug_tag(DEBUG_TAG_LOGS, 4, L"Log query successful, %d records returned in %d ms",
               static_cast<int>(json_array_size(records)), static_cast<int>(GetCurrentTimeMs() - startTime));
   }
   else
   {
      records = nullptr;
   }
   DBConnectionPoolReleaseConnection(hdb);
   return records;
}

/**
 * Get single log record, including detail columns, as JSON document
 */
uint32_t LogHandle::getRecordAsJson(int64_t recordId, json_t **record)
{
   StringBuffer query(L"SELECT ");
   bool first = true;
   for(int i = 0; m_log->columns[i].name != nullptr; i++)
   {
      if (IsHiddenColumn(m_log->columns[i].type))
         continue;
      if (!first)
         query.append(L",");
      else
         first = false;
      AppendQueryColumn(&query, m_log->columns[i]);
   }
   query.append(L" FROM ");
   query.append(m_log->table);
   query.append(L" WHERE ");
   query.append(m_log->idColumn);
   query.append(L"=");
   query.append(recordId);

   uint32_t rcc;
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, query);
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         *record = createRecordFromDBResult(hResult, 0, true);
         rcc = RCC_SUCCESS;
      }
      else
      {
         rcc = RCC_NO_SUCH_RECORD;
      }
      DBFreeResult(hResult);
   }
   else
   {
      rcc = RCC_DB_FAILURE;
   }
   DBConnectionPoolReleaseConnection(hdb);
   return rcc;
}
