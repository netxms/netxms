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

/**
 * Agent policy default constructor
 */
AgentPolicyLogParser::AgentPolicyLogParser()
                  : AgentPolicy(AGENT_POLICY_LOG_PARSER)
{
	m_fileContent = NULL;
}


/**
 * Constructor for user-initiated object creation
 */
AgentPolicyLogParser::AgentPolicyLogParser(const TCHAR *name)
                  : AgentPolicy(name, AGENT_POLICY_LOG_PARSER)
{
	m_fileContent = NULL;
}


/**
 * Destructor
 */
AgentPolicyLogParser::~AgentPolicyLogParser()
{
	safe_free(m_fileContent);
}


/**
 * Save to database
 */
bool AgentPolicyLogParser::saveToDatabase(DB_HANDLE hdb)
{
	lockProperties();

	bool success = savePolicyCommonProperties(hdb);
	if (success)
	{
		String data = DBPrepareString(hdb, m_fileContent);
		size_t len = data.length() + 256;
		TCHAR *query = (TCHAR *)malloc(len * sizeof(TCHAR));

		_sntprintf(query, len, _T("SELECT policy_id FROM ap_log_parser WHERE policy_id=%d"), m_id);
		DB_RESULT hResult = DBSelect(hdb, query);
		if (hResult != NULL)
		{
			bool isNew = (DBGetNumRows(hResult) == 0);
			DBFreeResult(hResult);

			if (isNew)
				_sntprintf(query, len, _T("INSERT INTO ap_log_parser (policy_id,file_content) VALUES (%d,%s)"),
				           m_id, (const TCHAR *)data);
			else
				_sntprintf(query, len, _T("UPDATE ap_log_parser SET file_content=%s WHERE policy_id=%d"),
				           (const TCHAR *)data, m_id);
			success = DBQuery(hdb, query);
		}
		free(query);
	}

	// Clear modifications flag and unlock object
	if (success)
		m_modified = 0;
   unlockProperties();

   return success;
}

/**
 * Delete from database
 */
bool AgentPolicyLogParser::deleteFromDatabase(DB_HANDLE hdb)
{
	bool success = AgentPolicy::deleteFromDatabase(hdb);
   if (success)
   {
      success = executeQueryOnObject(hdb, _T("DELETE FROM ap_log_parser WHERE policy_id=?"));
   }
   return success;
}

/**
 * Load from database
 */
bool AgentPolicyLogParser::loadFromDatabase(DB_HANDLE hdb, UINT32 dwId)
{
	bool success = false;

	if (AgentPolicy::loadFromDatabase(hdb, dwId))
	{
		TCHAR query[256];

		_sntprintf(query, 256, _T("SELECT file_content FROM ap_log_parser WHERE policy_id=%d"), dwId);
		DB_RESULT hResult = DBSelect(hdb, query);
		if (hResult != NULL)
		{
			if (DBGetNumRows(hResult) > 0)
			{
				m_fileContent = DBGetField(hResult, 0, 0, NULL, 0);
				success = true;
			}
			DBFreeResult(hResult);
		}
	}
	return success;
}

/**
 * Create NXCP message with policy data
 */
void AgentPolicyLogParser::fillMessageInternal(NXCPMessage *msg, UINT32 userId)
{
   AgentPolicy::fillMessageInternal(msg, userId);
	msg->setField(VID_CONFIG_FILE_DATA, CHECK_NULL_EX(m_fileContent));
}

/**
 * Modify policy from message
 */
UINT32 AgentPolicyLogParser::modifyFromMessageInternal(NXCPMessage *pRequest)
{
	if (pRequest->isFieldExist(VID_CONFIG_FILE_DATA))
	{
		safe_free(m_fileContent);
		m_fileContent = pRequest->getFieldAsString(VID_CONFIG_FILE_DATA);
	}

   return AgentPolicy::modifyFromMessageInternal(pRequest);
}

/**
 * Create deployment message
 */
bool AgentPolicyLogParser::createDeploymentMessage(NXCPMessage *msg)
{
	if (!AgentPolicy::createDeploymentMessage(msg))
		return false;

	if (m_fileContent == NULL)
		return false;  // Policy cannot be deployed

#ifdef UNICODE
	char *fd = MBStringFromWideStringSysLocale(m_fileContent);
	msg->setField(VID_CONFIG_FILE_DATA, (BYTE *)fd, (UINT32)strlen(fd));
	free(fd);
#else
	msg->setField(VID_CONFIG_FILE_DATA, (BYTE *)m_fileContent, (UINT32)strlen(m_fileContent));
#endif
	return true;
}


/**
 * Create uninstall message
 */
bool AgentPolicyLogParser::createUninstallMessage(NXCPMessage *msg)
{
	return AgentPolicy::createUninstallMessage(msg);
}

/**
 * Serialize object to JSON
 */
json_t *AgentPolicyLogParser::toJson()
{
   json_t *root = AgentPolicy::toJson();
   json_object_set_new(root, "fileContent", json_string_t(m_fileContent));
   return root;
}
