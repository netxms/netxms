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
** File: rptparam.cpp
**
**/

#include "nxcore.h"


//
// Create from NXCP message
//

ReportParameter::ReportParameter(CSCPMessage *msg, DWORD baseId)
{
	m_name = msg->GetVariableStr(baseId);
	m_description = msg->GetVariableStr(baseId + 1);
	m_defaultValue = msg->GetVariableStr(baseId + 2);
	m_dataType = (int)msg->GetVariableShort(baseId + 3);
}


//
// Create from database result set
//

ReportParameter::ReportParameter(DB_RESULT hResult, int row)
{
	m_name = DBGetField(hResult, row, 0, NULL, 0);
	m_description = DBGetField(hResult, row, 1, NULL, 0);
	m_dataType = DBGetFieldLong(hResult, row, 2);
	m_defaultValue = DBGetField(hResult, row, 3, NULL, 0);
}


//
// Destructor
//

ReportParameter::~ReportParameter()
{
	safe_free(m_name);
	safe_free(m_description);
	safe_free(m_defaultValue);
}


//
// Fill NXCP message
//

void ReportParameter::fillMessage(CSCPMessage *msg, DWORD baseId)
{
	msg->SetVariable(baseId, m_name);
	msg->SetVariable(baseId + 1, CHECK_NULL_EX(m_description));
	msg->SetVariable(baseId + 2, CHECK_NULL_EX(m_defaultValue));
	msg->SetVariable(baseId + 3, (WORD)m_dataType);
}
