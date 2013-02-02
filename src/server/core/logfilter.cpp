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
** File: logfilter.cpp
**
**/

#include "nxcore.h"

/**
 * Create log filter object from NXCP message
 */
LogFilter::LogFilter(CSCPMessage *msg)
{
	m_numColumnFilters = (int)msg->GetVariableLong(VID_NUM_FILTERS);
	m_columnFilters = (ColumnFilter **)malloc(sizeof(ColumnFilter *) * m_numColumnFilters);
	DWORD varId = VID_COLUMN_FILTERS_BASE;
	for(int i = 0; i < m_numColumnFilters; i++)
	{
		TCHAR column[256];
		msg->GetVariableStr(varId++, column, 256);
		m_columnFilters[i] = new ColumnFilter(msg, column, varId);
		varId += m_columnFilters[i]->getVariableCount();
	}

	m_numOrderingColumns = (int)msg->GetVariableLong(VID_NUM_ORDERING_COLUMNS);
	m_orderingColumns = (OrderingColumn *)malloc(sizeof(OrderingColumn) * m_numOrderingColumns);
	varId = VID_ORDERING_COLUMNS_BASE;
	for(int i = 0; i < m_numOrderingColumns; i++)
	{
		msg->GetVariableStr(varId++, m_orderingColumns[i].name, MAX_COLUMN_NAME_LEN);
		m_orderingColumns[i].descending = msg->GetVariableShort(varId++) ? true : false;
	}
}

/**
 * Destructor
 */
LogFilter::~LogFilter()
{
	for(int i = 0; i < m_numColumnFilters; i++)
		delete m_columnFilters[i];
	safe_free(m_columnFilters);
	safe_free(m_orderingColumns);
}

/**
 * Build ORDER BY clause
 */
String LogFilter::buildOrderClause()
{
	String result;

	if (m_numOrderingColumns > 0)
	{
		result += _T(" ORDER BY ");
		for(int i = 0; i < m_numOrderingColumns; i++)
		{
			if (i > 0)
				result += _T(",");
			result += m_orderingColumns[i].name;
			if (m_orderingColumns[i].descending)
				result += _T(" DESC");
		}
	}

	return result;
}
