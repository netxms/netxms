/*
** NetXMS - Network Management System
** Copyright (C) 2003-2020 Raden Solutions
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
   m_newTypeFormat = msg->getFieldAsBoolean(VID_NEW_POLICY_TYPE);
	m_size = msg->getFieldAsUInt32(VID_NUM_ELEMENTS);
	if (m_size > 0)
	{
		m_guidList = MemAllocArray<uint8_t>(UUID_LENGTH * m_size);
		m_typeList = MemAllocArray<TCHAR*>(m_size);
		m_serverIdList = MemAllocArray<uint64_t>(m_size);
		m_serverInfoList = MemAllocArray<TCHAR*>(m_size);
		m_version = MemAllocArray<int>(m_size);
		m_hashList = MemAllocArray<uint8_t>(MD5_DIGEST_SIZE * m_size);
		memset(m_hashList, 0, MD5_DIGEST_SIZE * m_size);

		uint32_t fieldId = VID_ELEMENT_LIST_BASE;
		for(int i = 0; i < m_size; i++, fieldId += 4)
		{
			msg->getFieldAsBinary(fieldId++, &m_guidList[i * UUID_LENGTH], UUID_LENGTH);
			if (m_newTypeFormat)
			{
            m_typeList[i] = msg->getFieldAsString(fieldId++);
			}
			else
			{
	         int type = (int)msg->getFieldAsUInt32(fieldId++);
	         m_typeList[i] = MemCopyString(PolicyNames[type]);
			}

			m_serverInfoList[i] = msg->getFieldAsString(fieldId++);
			m_serverIdList[i] = msg->getFieldAsUInt64(fieldId++);
			m_version[i] = (int)msg->getFieldAsUInt32(fieldId++);
			msg->getFieldAsBinary(fieldId++, &m_hashList[i * MD5_DIGEST_SIZE], UUID_LENGTH);
		}
	}
	else
	{
		m_guidList = nullptr;
		m_typeList = nullptr;
		m_serverInfoList = nullptr;
		m_serverIdList = nullptr;
		m_version = nullptr;
		m_hashList = nullptr;
	}
}

/**
 * Destructor
 */
AgentPolicyInfo::~AgentPolicyInfo()
{
   for(int i = 0; i < m_size; i++)
      MemFree(m_typeList[i]);
   MemFree(m_typeList);
   for(int i = 0; i < m_size; i++)
      MemFree(m_serverInfoList[i]);
   MemFree(m_serverInfoList);
   MemFree(m_serverIdList);
   MemFree(m_guidList);
   MemFree(m_version);
   MemFree(m_hashList);
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

/**
 * Get hash
 */
const uint8_t *AgentPolicyInfo::getHash(int index)
{
   if ((index >= 0) && (index < m_size))
   {
      return m_hashList + (index * MD5_DIGEST_SIZE);
   }
   else
   {
      return nullptr;
   }
}
