/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2011 Victor Kirhenshtein
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
** File: report_job.cpp
**
**/

#include "nxcore.h"


//
// Constructor
//

ReportJob::ReportJob(Report *report, StringMap *parameters, DWORD userId) 
	: ServerJob(_T("EXECUTE_REPORT"), _T("Execute report"), g_dwMgmtNode, userId, false)
{
	m_parameters = parameters;
	m_definition = _tcsdup(report->getDefinition());

	TCHAR buffer[1024];
	_sntprintf(buffer, 1024, _T("Execute report %s"), report->Name());
	setDescription(buffer);
}


//
// Destructor
//

ReportJob::~ReportJob()
{
	safe_free(m_definition);
	delete m_parameters;
}


//
// Run report
//

bool ReportJob::run()
{
	if ((m_definition == NULL) || (m_definition[0] == 0))
	{
		setFailureMessage(_T("Report definition is missing or invalid"));
		return false;
	}

	// TODO: execute report here


	return true;
}


//
// Build name of data file
//

TCHAR *ReportJob::buildDataFileName(DWORD jobId, TCHAR *buffer, size_t bufferSize)
{
	_sntprintf(buffer, bufferSize, _T("%s") DDIR_REPORTS FS_PATH_SEPARATOR _T("job_%u"), g_szDataDir, jobId);
	return buffer;
}
