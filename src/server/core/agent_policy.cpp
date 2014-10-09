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

/**
 * Redefined status calculation for policy group
 */
void PolicyGroup::calculateCompoundStatus(BOOL bForcedRecalc)
{
   m_iStatus = STATUS_NORMAL;
}

/**
 * Called by client session handler to check if threshold summary should be shown for this object.
 */
bool PolicyGroup::showThresholdSummary()
{
	return false;
}

/**
 * Agent policy default constructor
 */
AgentPolicy::AgentPolicy(int type)
            : NetObj()
{
	m_version = 0x00010000;
	m_policyType = type;
	m_description = NULL;
}


//
// Constructor for user-initiated object creation
//

AgentPolicy::AgentPolicy(const TCHAR *name, int type)
            : NetObj()
{
	nx_strncpy(m_szName, name, MAX_OBJECT_NAME);
	m_version = 0x00010000;
	m_policyType = type;
	m_description = NULL;
}


//
// Destructor
//

AgentPolicy::~AgentPolicy()
{
	safe_free(m_description);
}


//
// Save common policy properties to database
//

BOOL AgentPolicy::savePolicyCommonProperties(DB_HANDLE hdb)
{
	TCHAR query[8192];

	saveCommonProperties(hdb);

   // Check for object's existence in database
	bool isNewObject = true;
   _sntprintf(query, 256, _T("SELECT id FROM ap_common WHERE id=%d"), m_dwId);
   DB_RESULT hResult = DBSelect(hdb, query);
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         isNewObject = false;
      DBFreeResult(hResult);
   }
   if (isNewObject)
      _sntprintf(query, 8192,
                 _T("INSERT INTO ap_common (id,policy_type,version,description) VALUES (%d,%d,%d,%s)"),
                 m_dwId, m_policyType, m_version, (const TCHAR *)DBPrepareString(hdb, m_description));
   else
      _sntprintf(query, 8192,
                 _T("UPDATE ap_common SET policy_type=%d,version=%d,description=%s WHERE id=%d"),
                 m_policyType, m_version, (const TCHAR *)DBPrepareString(hdb, m_description), m_dwId);
   BOOL success = DBQuery(hdb, query);

   // Save access list
   saveACLToDB(hdb);

   // Update node bindings
   _sntprintf(query, 256, _T("DELETE FROM ap_bindings WHERE policy_id=%d"), m_dwId);
   DBQuery(hdb, query);
   LockChildList(FALSE);
   for(UINT32 i = 0; i < m_dwChildCount; i++)
   {
      _sntprintf(query, 256, _T("INSERT INTO ap_bindings (policy_id,node_id) VALUES (%d,%d)"), m_dwId, m_pChildList[i]->Id());
      DBQuery(hdb, query);
   }
   UnlockChildList();

	return success;
}


/**
 * Save to database
 */
BOOL AgentPolicy::SaveToDB(DB_HANDLE hdb)
{
	LockData();

	BOOL success = savePolicyCommonProperties(hdb);

	// Clear modifications flag and unlock object
	if (success)
		m_isModified = false;
   UnlockData();

   return success;
}

/**
 * Delete from database
 */
bool AgentPolicy::deleteFromDB(DB_HANDLE hdb)
{
   bool success = NetObj::deleteFromDB(hdb);
   if (success)
   {
      success = executeQueryOnObject(hdb, _T("DELETE FROM ap_common WHERE id=?"));
      if (success)
         success = executeQueryOnObject(hdb, _T("DELETE FROM ap_bindings WHERE policy_id=?"));
   }
   return success;
}

/**
 * Load from database
 */
BOOL AgentPolicy::CreateFromDB(UINT32 dwId)
{
	m_dwId = dwId;

	if (!loadCommonProperties())
   {
      DbgPrintf(2, _T("Cannot load common properties for agent policy object %d"), dwId);
      return FALSE;
   }

   if (!m_isDeleted)
   {
		TCHAR query[256];

	   loadACLFromDB();

		_sntprintf(query, 256, _T("SELECT version,description FROM ap_common WHERE id=%d"), dwId);
		DB_RESULT hResult = DBSelect(g_hCoreDB, query);
		if (hResult == NULL)
			return FALSE;

		m_version = DBGetFieldULong(hResult, 0, 0);
		m_description = DBGetField(hResult, 0, 1, NULL, 0);
		DBFreeResult(hResult);

	   // Load related nodes list
      _sntprintf(query, 256, _T("SELECT node_id FROM ap_bindings WHERE policy_id=%d"), m_dwId);
      hResult = DBSelect(g_hCoreDB, query);
      if (hResult != NULL)
      {
         int numNodes = DBGetNumRows(hResult);
         for(int i = 0; i < numNodes; i++)
         {
            UINT32 nodeId = DBGetFieldULong(hResult, i, 0);
            NetObj *object = FindObjectById(nodeId);
            if (object != NULL)
            {
               if (object->Type() == OBJECT_NODE)
               {
                  AddChild(object);
                  object->AddParent(this);
               }
               else
               {
                  nxlog_write(MSG_AP_BINDING_NOT_NODE, EVENTLOG_ERROR_TYPE, "dd", m_dwId, nodeId);
               }
            }
            else
            {
               nxlog_write(MSG_INVALID_AP_BINDING, EVENTLOG_ERROR_TYPE, "dd", m_dwId, nodeId);
            }
         }
         DBFreeResult(hResult);
      }
	}

	return TRUE;
}


//
// Create NXCP message with policy data
//

void AgentPolicy::CreateMessage(CSCPMessage *msg)
{
	NetObj::CreateMessage(msg);
	msg->SetVariable(VID_POLICY_TYPE, (WORD)m_policyType);
	msg->SetVariable(VID_VERSION, m_version);
	msg->SetVariable(VID_DESCRIPTION, CHECK_NULL_EX(m_description));
}


//
// Modify policy from message
//

UINT32 AgentPolicy::ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked)
{
   if (!bAlreadyLocked)
      LockData();

	if (pRequest->isFieldExist(VID_VERSION))
		m_version = pRequest->GetVariableLong(VID_VERSION);

	if (pRequest->isFieldExist(VID_DESCRIPTION))
	{
		safe_free(m_description);
		m_description = pRequest->GetVariableStr(VID_DESCRIPTION);
	}

   return NetObj::ModifyFromMessage(pRequest, TRUE);
}


//
// Create deployment message
//

bool AgentPolicy::createDeploymentMessage(CSCPMessage *msg)
{
	msg->SetVariable(VID_POLICY_TYPE, (WORD)m_policyType);
	msg->SetVariable(VID_GUID, m_guid, UUID_LENGTH);
	return true;
}


//
// Create uninstall message
//

bool AgentPolicy::createUninstallMessage(CSCPMessage *msg)
{
	msg->SetVariable(VID_POLICY_TYPE, (WORD)m_policyType);
	msg->SetVariable(VID_GUID, m_guid, UUID_LENGTH);
	return true;
}
