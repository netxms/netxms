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

static const TCHAR *PolicyNames[] = { _T("None"), _T("AgentConfig"), _T("LogParserConfig")};

/**
 * Constructor
 */
AgentPolicyInfo::AgentPolicyInfo(NXCPMessage *msg)
{
   m_newPolicyType = msg->getFieldAsBoolean(VID_NEW_POLICY_TYPE);
	m_size = msg->getFieldAsUInt32(VID_NUM_ELEMENTS);
	if (m_size > 0)
	{
		m_guidList = (BYTE *)malloc(UUID_LENGTH * m_size);
		m_typeList = (TCHAR **)malloc(sizeof(TCHAR *) * m_size);
		m_serverIdList = (UINT64*)malloc(sizeof(UINT64) * m_size);
		m_serverInfoList = (TCHAR **)malloc(sizeof(TCHAR *) * m_size);
		m_version = (int *)malloc(sizeof(int) * m_size);

		UINT32 varId = VID_ELEMENT_LIST_BASE;
		for(int i = 0; i < m_size; i++, varId += 5)
		{
			msg->getFieldAsBinary(varId++, &m_guidList[i * UUID_LENGTH], UUID_LENGTH);
			if(!m_newPolicyType)
			{
	         int type = (int)msg->getFieldAsUInt32(varId++);
	         m_typeList[i] = _tcsdup(PolicyNames[type]);
			}
			else
			   m_typeList[i] = msg->getFieldAsString(varId++);

			m_serverInfoList[i] = msg->getFieldAsString(varId++);
			m_serverIdList[i] = msg->getFieldAsUInt64(varId++);
			m_version[i] = (int)msg->getFieldAsUInt32(varId++);
		}
	}
	else
	{
		m_guidList = NULL;
		m_typeList = NULL;
		m_serverInfoList = NULL;
		m_serverIdList = NULL;
		m_version = NULL;
	}
}

/**
 * Destructor
 */
AgentPolicyInfo::~AgentPolicyInfo()
{
   for(int i = 0; i < m_size; i++)
      free(m_typeList[i]);
   free(m_typeList);
   for(int i = 0; i < m_size; i++)
		free(m_serverInfoList[i]);
   free(m_serverInfoList);
	free(m_serverIdList);
	free(m_guidList);
	free(m_version);
}

/**
 * Get GUID
 */
uuid AgentPolicyInfo::getGuid(int index)
{
	if ((index >= 0) && (index < m_size))
	{
		return uuid(&m_guidList[index * UUID_LENGTH]);
	}
	else
	{
      return uuid::NULL_UUID;
	}
}
