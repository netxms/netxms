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
** File: agent_policy.cpp
**
**/

#include "nxcore.h"


//
// Redefined status calculation for policy group
//

void PolicyGroup::CalculateCompoundStatus(BOOL bForcedRecalc)
{
   m_iStatus = STATUS_NORMAL;
}


//
// Agent policy default constructor
//

AgentPolicy::AgentPolicy(int type)
            : NetObj()
{
	m_version = 0x00010000;
	m_policyType = type;
	m_data = NULL;
}


//
// Destructor
//

AgentPolicy::~AgentPolicy()
{
	safe_free(m_data);
}


//
// Save to database
//

BOOL AgentPolicy::SaveToDB(DB_HANDLE hdb)
{
	return FALSE;
}


//
// Delete from database
//

BOOL AgentPolicy::DeleteFromDB()
{
	return FALSE;
}


//
// Load from database
//

BOOL AgentPolicy::CreateFromDB(DWORD dwId)
{
	return FALSE;
}


//
// Create NXCP message with policy data
//

void AgentPolicy::CreateMessage(CSCPMessage *pMsg)
{
}


//
// Modify policy from message
//

DWORD AgentPolicy::ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked)
{
   if (!bAlreadyLocked)
      LockData();

   return NetObj::ModifyFromMessage(pRequest, TRUE);
}
