/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 Victor Kirhenshtein
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
** File: ef.cpp
**
**/

#include "nxcore.h"


//
// Setup event forwarding session
//

void *EF_SetupSession(CSCPMessage *request)
{
	return NULL;
}


//
// Close event forwarding session
//

void EF_CloseSession(void *sd)
{
}


//
// Process event forwarding session message
//

BOOL EF_ProcessMessage(void *sd, CSCPMessage *request, CSCPMessage *response)
{
	DWORD code, objectIp;

	if (request->GetCode() == CMD_FORWARD_EVENT)
	{
		code = request->GetVariableLong(VID_EVENT_CODE);
	}
	else
	{
		response->SetVariable(VID_RCC, ISC_ERR_NOT_IMPLEMENTED);
	}
	return FALSE;	// Don't close session
}
