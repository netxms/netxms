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
// Report parameter
//

class ReportParameter
{
private:
	TCHAR *m_name;
	TCHAR *m_description;
	TCHAR *m_defaultValue;
	int m_dataType;

public:
	ReportParameter(CSCPMessage *msg, DWORD baseId);
	ReportParameter(DB_RESULT hResult, int row);
	~ReportParameter();

	const TCHAR *getName() { return m_name; }
	const TCHAR *getDescription() { return m_description; }
	const TCHAR *getDefaultValue() { return m_defaultValue; }
	int getDataType() { return m_dataType; }

	void fillMessage(CSCPMessage *msg, DWORD baseId);
};

#endif
