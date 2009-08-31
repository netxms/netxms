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
** File: logfilter.cpp
**
**/

#include "nxcore.h"


//
// Constructor
//

LogFilter::LogFilter(CSCPMessage *msg)
{
	m_numColumnFilters = msg->GetVariableLong(VID_NUM_FILTERS);
	m_columnFilters = (ColumnFilter **)malloc(sizeof(ColumnFilter *) * m_numColumnFilters);
	DWORD varId = VID_COLUMN_FILTERS_BASE;
	for(int i = 0; i < m_numColumnFilters; i++)
	{
		m_columnFilters[i] = new ColumnFilter(msg, varId);
		varId += m_columnFilters[i]->getVariableCount();
	}
}


//
// Destructor
//

LogFilter::~LogFilter()
{
	for(int i = 0; i < m_numColumnFilters; i++)
		delete m_columnFilters[i];
	safe_free(m_columnFilters);
}
