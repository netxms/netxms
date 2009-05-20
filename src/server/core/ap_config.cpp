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
// Agent policy default constructor
//

AgentPolicyConfig::AgentPolicyConfig()
                  : AgentPolicy(AGENT_POLICY_CONFIG)
{
}


//
// Destructor
//

AgentPolicyConfig::~AgentPolicyConfig()
{
}


//
// Save to database
//

BOOL AgentPolicyConfig::SaveToDB(DB_HANDLE hdb)
{
	TCHAR query[256];

	LockData();

	SaveCommonProperties(hdb);

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
      _sntprintf(query, 256,
                 _T("INSERT INTO ap_common (id,policy_type,version) VALUES (%d,%d,%d)"),
                 m_dwId, m_policyType, m_version);
   else
      _sntprintf(query, 256,
                 _T("UPDATE ap_common SET policy_type=%d,version=%d WHERE id=%d"),
                 m_policyType, m_version, m_dwId);
   BOOL success = DBQuery(hdb, query);


   // Save access list
   SaveACLToDB(hdb);

   // Update node bindings
   _sntprintf(query, 256, _T("DELETE FROM ap_bindings WHERE policy_id=%d"), m_dwId);
   DBQuery(hdb, query);
   LockChildList(FALSE);
   for(DWORD i = 0; i < m_dwChildCount; i++)
   {
      _sntprintf(query, 256, _T("INSERT INTO ap_bindings (policy_id,node_id) VALUES (%d,%d)"), m_dwId, m_pChildList[i]->Id());
      DBQuery(hdb, query);
   }
   UnlockChildList();

	// Clear modifications flag and unlock object
	if (success)
		m_bIsModified = FALSE;
   UnlockData();

   return success;
}


//
// Delete from database
//

BOOL AgentPolicyConfig::DeleteFromDB()
{
	TCHAR query[256];

	_sntprintf(query, 256, _T("DELETE FROM ap_common WHERE id=%d"), m_dwId);
	QueueSQLRequest(query);
	_sntprintf(query, 256, _T("DELETE FROM ap_bindings WHERE policy_id=%d"), m_dwId);
	QueueSQLRequest(query);
	return TRUE;
}


//
// Load from database
//

BOOL AgentPolicyConfig::CreateFromDB(DWORD dwId)
{
	m_dwId = dwId;

	if (!LoadCommonProperties())
   {
      DbgPrintf(2, "Cannot load common properties for agent policy object %d", dwId);
      return FALSE;
   }

   if (!m_bIsDeleted)
   {
		TCHAR query[256];

	   LoadACLFromDB();

		_sntprintf(query, 256, _T("SELECT version, description FROM ap_common WHERE id=%d"), dwId);
		DB_RESULT hResult = DBSelect(g_hCoreDB, query);
		if (hResult == NULL)
			return FALSE;

		m_version = DBGetFieldULong(hResult, 0, 0);
		m_description = DBGetField(hResult, 0, 1, NULL, 0);
		DecodeSQLString(m_description);
		DBFreeResult(hResult);

	   // Load related nodes list
      _sntprintf(query, 256, _T("SELECT node_id FROM ap_bindings WHERE policy_id=%d"), m_dwId);
      hResult = DBSelect(g_hCoreDB, query);
      if (hResult != NULL)
      {
         int numNodes = DBGetNumRows(hResult);
         for(int i = 0; i < numNodes; i++)
         {
            DWORD nodeId = DBGetFieldULong(hResult, i, 0);
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

void AgentPolicyConfig::CreateMessage(CSCPMessage *msg)
{
	AgentPolicy::CreateMessage(msg);
}


//
// Modify policy from message
//

DWORD AgentPolicyConfig::ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked)
{
   if (!bAlreadyLocked)
      LockData();

   return AgentPolicy::ModifyFromMessage(pRequest, TRUE);
}
