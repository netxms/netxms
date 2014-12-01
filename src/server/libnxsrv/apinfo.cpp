/*
** NetXMS - Network Management System
** Copyright (C) 2003-2013 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: apinfo.cpp
**
**/

#include "libnxsrv.h"


//
// Constructor
//

AgentPolicyInfo::AgentPolicyInfo(NXCPMessage *msg)
{
	m_size = msg->getFieldAsUInt32(VID_NUM_ELEMENTS);
	if (m_size > 0)
	{
		m_guidList = (BYTE *)malloc(UUID_LENGTH * m_size);
		m_typeList = (int *)malloc(sizeof(int) * m_size);
		m_serverList = (TCHAR **)malloc(sizeof(TCHAR *) * m_size);

		UINT32 varId = VID_ELEMENT_LIST_BASE;
		for(int i = 0; i < m_size; i++, varId += 7)
		{
			msg->getFieldAsBinary(varId++, &m_guidList[i * UUID_LENGTH], UUID_LENGTH);
			m_typeList[i] = (int)msg->getFieldAsUInt16(varId++);
			m_serverList[i] = msg->getFieldAsString(varId++);
		}
	}
	else
	{
		m_guidList = NULL;
		m_typeList = NULL;
		m_serverList = NULL;
	}
}


//
// Destructor
//

AgentPolicyInfo::~AgentPolicyInfo()
{
	for(int i = 0; i < m_size; i++)
		safe_free(m_serverList[i]);
	safe_free(m_serverList);
	safe_free(m_typeList);
	safe_free(m_guidList);
}


//
// Get GUID
//

bool AgentPolicyInfo::getGuid(int index, uuid_t guid)
{
	if ((index >= 0) && (index < m_size))
	{
		memcpy(guid, &m_guidList[index * UUID_LENGTH], UUID_LENGTH);
		return true;
	}
	else
	{
		return false;
	}
}
