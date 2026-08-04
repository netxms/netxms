/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2020 Victor Kirhenshtein
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
** File: logfilter.cpp
**
**/

#include "nxcore.h"
#include <nxcore_logs.h>

/**
 * Create log filter object from JSON document. Expected document format:
 *
 * {
 *    "filters": [ { "column": "name", "type": "equals", ... }, ... ],
 *    "orderBy": [ { "column": "name", "descending": true }, ... ]
 * }
 *
 * Column names are validated against log definition, because they are inserted into generated SQL as is.
 */
LogFilter::LogFilter(json_t *json, LogHandle *log)
{
   m_valid = true;

   json_t *filters = json_object_get(json, "filters");
   m_numColumnFilters = json_is_array(filters) ? static_cast<int>(json_array_size(filters)) : 0;
   m_columnFilters = MemAllocArray<ColumnFilter*>(m_numColumnFilters);
   for(int i = 0; i < m_numColumnFilters; i++)
   {
      json_t *filter = json_array_get(filters, i);
      String column = json_object_get_string(filter, "column", L"");
      if (log->getColumnDefinition(column) == nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG_LOGS, 4, L"LogFilter: unknown filter column \"%s\"", column.cstr());
         m_valid = false;
         m_numColumnFilters = i;   // do not leave uninitialized elements behind
         break;
      }
      m_columnFilters[i] = new ColumnFilter(filter, column, log);
      if (!m_columnFilters[i]->isValid())
      {
         m_valid = false;
         m_numColumnFilters = i + 1;
         break;
      }
   }

   json_t *orderingColumns = json_object_get(json, "orderBy");
   m_numOrderingColumns = json_is_array(orderingColumns) ? static_cast<int>(json_array_size(orderingColumns)) : 0;
   m_orderingColumns = MemAllocArray<OrderingColumn>(m_numOrderingColumns);
   for(int i = 0; i < m_numOrderingColumns; i++)
   {
      json_t *orderingColumn = json_array_get(orderingColumns, i);
      String column = json_object_get_string(orderingColumn, "column", L"");
      const LOG_COLUMN *cd = log->getColumnDefinition(column);
      if (cd == nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG_LOGS, 4, L"LogFilter: unknown ordering column \"%s\"", column.cstr());
         m_valid = false;
         m_numOrderingColumns = i;
         break;
      }
      wcslcpy(m_orderingColumns[i].name, cd->name, MAX_COLUMN_NAME_LEN);
      m_orderingColumns[i].descending = json_object_get_boolean(orderingColumn, "descending", false);
   }
}

/**
 * Create log filter object from NXCP message
 */
LogFilter::LogFilter(const NXCPMessage& msg, LogHandle *log)
{
   m_valid = true;

	m_numColumnFilters = msg.getFieldAsInt32(VID_NUM_FILTERS);
	m_columnFilters = MemAllocArray<ColumnFilter*>(m_numColumnFilters);
	uint32_t fieldId = VID_COLUMN_FILTERS_BASE;
	for(int i = 0; i < m_numColumnFilters; i++)
	{
		TCHAR column[256];
		msg.getFieldAsString(fieldId++, column, 256);
		m_columnFilters[i] = new ColumnFilter(msg, column, fieldId, log);
		fieldId += m_columnFilters[i]->getVariableCount();
	}

	m_numOrderingColumns = msg.getFieldAsInt32(VID_NUM_ORDERING_COLUMNS);
	m_orderingColumns = MemAllocArray<OrderingColumn>(m_numOrderingColumns);
	fieldId = VID_ORDERING_COLUMNS_BASE;
	for(int i = 0; i < m_numOrderingColumns; i++)
	{
		msg.getFieldAsString(fieldId++, m_orderingColumns[i].name, MAX_COLUMN_NAME_LEN);
		m_orderingColumns[i].descending = msg.getFieldAsUInt16(fieldId++) ? true : false;
	}
}

/**
 * Destructor
 */
LogFilter::~LogFilter()
{
	for(int i = 0; i < m_numColumnFilters; i++)
		delete m_columnFilters[i];
	MemFree(m_columnFilters);
	MemFree(m_orderingColumns);
}

/**
 * Build ORDER BY clause
 */
StringBuffer LogFilter::buildOrderClause()
{
	StringBuffer result;

	if (m_numOrderingColumns > 0)
	{
		result.append(_T(" ORDER BY "));
		for(int i = 0; i < m_numOrderingColumns; i++)
		{
			if (i > 0)
				result.append(_T(","));
			result.append(m_orderingColumns[i].name);
			if (m_orderingColumns[i].descending)
				result.append(_T(" DESC"));
		}
	}

	return result;
}
