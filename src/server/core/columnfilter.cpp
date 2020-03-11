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
** File: columnfilter.cpp
**
**/

#include "nxcore.h"

/**
 * Create column filter object from NXCP message
 */
ColumnFilter::ColumnFilter(NXCPMessage *msg, const TCHAR *column, UINT32 baseId)
{
	UINT32 varId;

	m_column = _tcsdup(column);
	m_type = (int)msg->getFieldAsUInt16(baseId);
	switch(m_type)
	{
		case FILTER_EQUALS:
		case FILTER_LESS:
		case FILTER_GREATER:
		case FILTER_CHILDOF:
			m_value.numericValue = msg->getFieldAsUInt64(baseId + 1);
			m_negated = msg->getFieldAsUInt16(baseId + 2) ? true : false;
			m_varCount = 3;
			break;
		case FILTER_RANGE:
			m_value.range.start = msg->getFieldAsUInt64(baseId + 1);
			m_value.range.end = msg->getFieldAsUInt64(baseId + 2);
			m_negated = msg->getFieldAsUInt16(baseId + 3) ? true : false;
			m_varCount = 4;
			break;
		case FILTER_LIKE:
			m_value.like = msg->getFieldAsString(baseId + 1);
			m_negated = msg->getFieldAsUInt16(baseId + 2) ? true : false;
			m_varCount = 3;
			break;
		case FILTER_SET:
			m_value.set.operation = msg->getFieldAsUInt16(baseId + 1);
			m_value.set.count = msg->getFieldAsUInt16(baseId + 2);
			m_varCount = 3;

			m_value.set.filters = (ColumnFilter **)malloc(sizeof(ColumnFilter *) * m_value.set.count);
			varId = baseId + 3;
			for(int i = 0; i < m_value.set.count; i++)
			{
				ColumnFilter *filter = new ColumnFilter(msg, column, varId);
				m_value.set.filters[i] = filter;
				varId += filter->getVariableCount();
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
			sql.appendFormattedString(_T("%s = ") INT64_FMT, m_column, m_value.numericValue);
			break;
		case FILTER_LESS:
			if (m_negated)
            sql.append(_T("NOT "));
			sql.appendFormattedString(_T("%s < ") INT64_FMT, m_column, m_value.numericValue);
			break;
		case FILTER_GREATER:
			if (m_negated)
            sql.append(_T("NOT "));
			sql.appendFormattedString(_T("%s > ") INT64_FMT, m_column, m_value.numericValue);
			break;
		case FILTER_RANGE:
			if (m_negated)
            sql.append(_T("NOT "));
			sql.appendFormattedString(_T("%s BETWEEN ") INT64_FMT _T(" AND ") INT64_FMT, m_column, m_value.range.start, m_value.range.end);
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
				sql.appendFormattedString(_T("%s LIKE %s"), m_column, (const TCHAR *)DBPrepareString(g_dbDriver, m_value.like));
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
					SharedObjectArray<NetObj> *childObjects = object->getAllChildren(true);
					if (childObjects->size() > 0)
					{
						sql.append(m_column);
						sql.append(_T(" IN ("));
						for(int i = 0; i < childObjects->size(); i++)
						{
							if (i > 0)
								sql.append(_T(", "));
							TCHAR buffer[32];
							_sntprintf(buffer, 32, _T("%u"), childObjects->get(i)->getId());
							sql.append(buffer);
						}
						sql.append(_T(")"));
					}
					else
					{
						sql.append(_T("0=1"));
					}
					delete childObjects;
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
