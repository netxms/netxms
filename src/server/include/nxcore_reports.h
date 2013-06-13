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
** File: nxcore_reports.h
**
**/

#ifndef _nxcore_reports_h_
#define _nxcore_reports_h_


//
// Report execution job
//

class Report;

class ReportJob : public ServerJob
{
protected:
	Report *m_report;
	TCHAR *m_definition;
	StringMap *m_parameters;

	virtual bool run();

public:
	ReportJob(Report *report, StringMap *parameters, UINT32 userId);
	virtual ~ReportJob();

	TCHAR *buildDataFileName(TCHAR *buffer, size_t bufferSize) { return ReportJob::buildDataFileName(getId(), NULL, buffer, bufferSize); }
	static TCHAR *buildDataFileName(UINT32 jobId, const TCHAR *suffix, TCHAR *buffer, size_t bufferSize);
};

#endif
