/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Victor Kirhenshtein
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
** File: columnfilter.cpp
**
**/

#include "nxcore.h"
#include <nxcore_logs.h>

/**
 * Create column filter object from NXCP message
 */
ColumnFilter::ColumnFilter(const NXCPMessage& msg, const wchar_t *column, uint32_t baseId, LogHandle *log)
{
	uint32_t fieldId;

	m_column = MemCopyStringW(column);
	const LOG_COLUMN *cd = log->getColumnDefinition(m_column);
	m_columnFlags = (cd != nullptr) ? cd->flags : 0;

	m_type = msg.getFieldAsInt16(baseId);
	switch(m_type)
	{
		case FILTER_EQUALS:
		case FILTER_LESS:
		case FILTER_GREATER:
		case FILTER_CHILDOF:
			m_value.numericValue = msg.getFieldAsInt64(baseId + 1);
			m_negated = msg.getFieldAsBoolean(baseId + 2);
			m_varCount = 3;
			break;
		case FILTER_RANGE:
			m_value.range.start = msg.getFieldAsInt64(baseId + 1);
			m_value.range.end = msg.getFieldAsInt64(baseId + 2);
			m_negated = msg.getFieldAsBoolean(baseId + 3);
			m_varCount = 4;
			break;
		case FILTER_LIKE:
			m_value.like = msg.getFieldAsString(baseId + 1);
			m_negated = msg.getFieldAsBoolean(baseId + 2);
			m_varCount = 3;
			break;
		case FILTER_SET:
			m_value.set.operation = msg.getFieldAsInt16(baseId + 1);
			m_value.set.count = msg.getFieldAsInt16(baseId + 2);
			m_varCount = 3;

			m_value.set.filters = MemAllocArray<ColumnFilter*>(m_value.set.count);
			fieldId = baseId + 3;
			for(int i = 0; i < m_value.set.count; i++)
			{
				ColumnFilter *filter = new ColumnFilter(msg, column, fieldId, log);
				m_value.set.filters[i] = filter;
				fieldId += filter->getVariableCount();
				m_varCount += filter->getVariableCount();
			}
			break;
		default:
			break;
	}
}

/**
 * Destructor
 */
ColumnFilter::~ColumnFilter()
{
	MemFree(m_column);
	switch(m_type)
	{
		case FILTER_LIKE:
			MemFree(m_value.like);
			break;
		case FILTER_SET:
			for(int i = 0; i < m_value.set.count; i++)
				delete m_value.set.filters[i];
			MemFree(m_value.set.filters);
			break;
	}
}

/**
 * Generate SQL for column filter
 */
StringBuffer ColumnFilter::generateSql()
{
	StringBuffer sql;

	switch(m_type)
	{
		case FILTER_EQUALS:
			if (m_negated)
				sql.append(_T("NOT "));
			sql.append(m_column);
			sql.append(_T(" = "));
			if ((m_columnFlags & LCF_TSDB_TIMESTAMPTZ) && (g_dbSyntax == DB_SYNTAX_TSDB))
			{
	         sql.append(_T("to_timestamp("));
            sql.append(m_value.numericValue);
            sql.append(_T(")"));
			}
         else if (m_columnFlags & LCF_CHAR_COLUMN)
         {
            sql.append(_T('\''));
            sql.append(m_value.numericValue);
            sql.append(_T('\''));
         }
			else
			{
			   sql.append(m_value.numericValue);
			}
			break;
		case FILTER_LESS:
			if (m_negated)
            sql.append(_T("NOT "));
         sql.append(m_column);
         sql.append(_T(" < "));
         if ((m_columnFlags & LCF_TSDB_TIMESTAMPTZ) && (g_dbSyntax == DB_SYNTAX_TSDB))
         {
            sql.append(_T("to_timestamp("));
            sql.append(m_value.numericValue);
            sql.append(_T(")"));
         }
         else
         {
            sql.append(m_value.numericValue);
         }
			break;
		case FILTER_GREATER:
			if (m_negated)
            sql.append(_T("NOT "));
         sql.append(m_column);
         sql.append(_T(" > "));
         if ((m_columnFlags & LCF_TSDB_TIMESTAMPTZ) && (g_dbSyntax == DB_SYNTAX_TSDB))
         {
            sql.append(_T("to_timestamp("));
            sql.append(m_value.numericValue);
            sql.append(_T(")"));
         }
         else
         {
            sql.append(m_value.numericValue);
         }
			break;
		case FILTER_RANGE:
			if (m_negated)
            sql.append(_T("NOT "));
         sql.append(m_column);
         sql.append(_T(" BETWEEN "));
         if ((m_columnFlags & LCF_TSDB_TIMESTAMPTZ) && (g_dbSyntax == DB_SYNTAX_TSDB))
         {
            sql.append(_T("to_timestamp("));
            sql.append(m_value.range.start);
            sql.append(_T(") AND to_timestamp("));
            sql.append(m_value.range.end);
            sql.append(_T(")"));
         }
         else
         {
            sql.append(m_value.range.start);
            sql.append(_T(" AND "));
            sql.append(m_value.range.end);
         }
			break;
		case FILTER_LIKE:
			if (m_value.like[0] == 0)
			{
				if (m_negated)
					sql.appendFormattedString(_T("(%s IS NOT NULL) AND (%s <> '')"), m_column, m_column);
				else
					sql.appendFormattedString(_T("(%s IS NULL) OR (%s = '')"), m_column, m_column);
			}
			else
			{
				if (m_negated)
					sql.append(_T("NOT "));
				sql.append(m_column);
            sql.append(_T(" LIKE "));
            sql.append(DBPrepareString(g_dbDriver, m_value.like));
			}
			break;
		case FILTER_SET:
			if (m_value.set.count > 0)
			{
				bool first = true;
				for(int i = 0; i < m_value.set.count; i++)
				{
					StringBuffer subExpr = m_value.set.filters[i]->generateSql();
					if (!subExpr.isEmpty())
					{
						if (first)
						{
							first = false;
						}
						else
						{
							sql.append((m_value.set.operation == SET_OPERATION_AND) ? _T(" AND ") : _T(" OR "));
						}
						sql.append(_T("("));
						sql.append(subExpr);
						sql.append(_T(")"));
					}
				}
			}
			break;
		case FILTER_CHILDOF:
			if (m_negated)
				sql.append(_T("NOT "));

			{
				shared_ptr<NetObj> object = FindObjectById(static_cast<uint32_t>(m_value.numericValue));
				if (object != NULL)
				{
					unique_ptr<SharedObjectArray<NetObj>> children = object->getAllChildren(true);
					if (children->size() > 0)
					{
						sql.append(m_column);
						sql.append(_T(" IN ("));
						for(int i = 0; i < children->size(); i++)
						{
							if (i > 0)
								sql.append(_T(", "));
							sql.append(children->get(i)->getId());
						}
						sql.append(_T(")"));
					}
					else
					{
						sql.append(_T("0=1"));
					}
				}
				else
				{
					sql.append(_T("0=1"));
				}
			}
			break;
	}

	return sql;
}
