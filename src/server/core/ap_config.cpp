/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2010 Victor Kirhenshtein
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
** File: ap_config.cpp
**
**/

#include "nxcore.h"


//
// Agent policy default constructor
//

AgentPolicyConfig::AgentPolicyConfig()
                  : AgentPolicy(AGENT_POLICY_CONFIG)
{
	m_fileContent = NULL;
}


//
// Constructor for user-initiated object creation
//

AgentPolicyConfig::AgentPolicyConfig(const TCHAR *name)
                  : AgentPolicy(name, AGENT_POLICY_CONFIG)
{
	m_fileContent = NULL;
}


//
// Destructor
//

AgentPolicyConfig::~AgentPolicyConfig()
{
	safe_free(m_fileContent);
}


//
// Save to database
//

BOOL AgentPolicyConfig::SaveToDB(DB_HANDLE hdb)
{
	LockData();

	BOOL success = savePolicyCommonProperties(hdb);
	if (success)
	{
		String data = DBPrepareString(hdb, m_fileContent);
		int len = (int)data.getSize() + 256;
		TCHAR *query = (TCHAR *)malloc(len * sizeof(TCHAR));

		_sntprintf(query, len, _T("SELECT policy_id FROM ap_config_files WHERE policy_id=%d"), m_dwId);
		DB_RESULT hResult = DBSelect(hdb, query);
		if (hResult != NULL)
		{
			BOOL isNew = (DBGetNumRows(hResult) == 0);
			DBFreeResult(hResult);

			if (isNew)
				_sntprintf(query, len, _T("INSERT INTO ap_config_files (policy_id,file_content) VALUES (%d,%s)"),
				           m_dwId, (const TCHAR *)data);
			else
				_sntprintf(query, len, _T("UPDATE ap_config_files SET file_content=%s WHERE policy_id=%d"),
				           (const TCHAR *)data, m_dwId);
			success = DBQuery(hdb, query);
		}
		free(query);
	}

	// Clear modifications flag and unlock object
	if (success)
		m_bIsModified = FALSE;
   UnlockData();

   return success;
}

/**
 * Delete from database
 */
bool AgentPolicyConfig::deleteFromDB(DB_HANDLE hdb)
{
	bool success = AgentPolicy::deleteFromDB(hdb);
   if (success)
   {
      success = executeQueryOnObject(hdb, _T("DELETE FROM ap_config_files WHERE policy_id=?"));
   }
   return success;
}

/**
 * Load from database
 */
BOOL AgentPolicyConfig::CreateFromDB(UINT32 dwId)
{
	BOOL success = FALSE;

	if (AgentPolicy::CreateFromDB(dwId))
	{
		TCHAR query[256];

		_sntprintf(query, 256, _T("SELECT file_content FROM ap_config_files WHERE policy_id=%d"), dwId);
		DB_RESULT hResult = DBSelect(g_hCoreDB, query);
		if (hResult != NULL)
		{
			if (DBGetNumRows(hResult) > 0)
			{
				m_fileContent = DBGetField(hResult, 0, 0, NULL, 0);
				success = TRUE;
			}
			DBFreeResult(hResult);
		}
	}
	return success;
}


//
// Create NXCP message with policy data
//

void AgentPolicyConfig::CreateMessage(CSCPMessage *msg)
{
	AgentPolicy::CreateMessage(msg);
	msg->SetVariable(VID_CONFIG_FILE_DATA, CHECK_NULL_EX(m_fileContent));
}


//
// Modify policy from message
//

UINT32 AgentPolicyConfig::ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked)
{
   if (!bAlreadyLocked)
      LockData();

	if (pRequest->IsVariableExist(VID_CONFIG_FILE_DATA))
	{
		safe_free(m_fileContent);
		m_fileContent = pRequest->GetVariableStr(VID_CONFIG_FILE_DATA);
	}

	return AgentPolicy::ModifyFromMessage(pRequest, TRUE);
}


//
// Create deployment message
//

bool AgentPolicyConfig::createDeploymentMessage(CSCPMessage *msg)
{
	if (!AgentPolicy::createDeploymentMessage(msg))
		return false;

	if (m_fileContent == NULL)
		return false;  // Policy cannot be deployed

#ifdef UNICODE
	char *fd = MBStringFromWideString(m_fileContent);
	msg->SetVariable(VID_CONFIG_FILE_DATA, (BYTE *)fd, (UINT32)strlen(fd));
	free(fd);
#else
	msg->SetVariable(VID_CONFIG_FILE_DATA, (BYTE *)m_fileContent, (UINT32)strlen(m_fileContent));
#endif
	return true;
}


//
// Create uninstall message
//

bool AgentPolicyConfig::createUninstallMessage(CSCPMessage *msg)
{
	return AgentPolicy::createUninstallMessage(msg);
}
