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
// Delete query results
//

void LogHandle::deleteQueryResults()
{
	if (!m_isDataReady)
		return;

	delete_and_null(m_filter);
	m_isDataReady = false;
}


//
// Do query according to filter
//

bool LogHandle::query(LogFilter *filter)
{
	deleteQueryResults();
	m_filter = filter;

	return false;
}


//
// Get data from query result
//

Table *LogHandle::getData(INT64 startRow, INT64 numRows)
{
	if (!m_isDataReady)
		return NULL;

	return NULL;
}
