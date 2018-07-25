/*
** NetXMS - Network Management System
** Copyright (C) 2003-2016 Victor Kirhenshtein
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
   m_status = STATUS_NORMAL;
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
AgentPolicy::AgentPolicy(int type) : NetObj(), AutoBindTarget(this), VersionableObject(this)
{
	m_policyType = type;
   m_status = STATUS_NORMAL;
}

/**
 * Constructor for user-initiated object creation
 */
AgentPolicy::AgentPolicy(const TCHAR *name, int type) : NetObj(), AutoBindTarget(this), VersionableObject(this)
{
	nx_strncpy(m_name, name, MAX_OBJECT_NAME);
	m_policyType = type;
   m_status = STATUS_NORMAL;
}

/**
 * Must return true if object is an agent policy (derived from AgentPolicy class)
 */
bool AgentPolicy::isAgentPolicy()
{
   return true;
}

/**
 * Save common policy properties to database
 */
bool AgentPolicy::savePolicyCommonProperties(DB_HANDLE hdb)
{
	if (!saveCommonProperties(hdb))
      return false;

	bool success = false;
   if (!IsDatabaseRecordExist(hdb, _T("ap_common"), _T("id"), m_id)) //Policy can be only created. Policy type can't be changed.
   {
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO ap_common (policy_type,id) VALUES (?,?)"));
      if (hStmt == NULL)
         return false;

      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_policyType);
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_id);
      success = DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }
   else
   {
      success = true;
   }

   // Save access list
   if (success)
      success = saveACLToDB(hdb);

   // Update node bindings
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM ap_bindings WHERE policy_id=?"));


   lockChildList(false);
   if (success && (m_childList->size() > 0))
   {
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO ap_bindings (policy_id,node_id) VALUES (?,?)"), m_childList->size() > 1);
      if (hStmt != NULL)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
         for(int i = 0; (i < m_childList->size()) && success; i++)
         {
            DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_childList->get(i)->getId());
            success = DBExecute(hStmt);
         }
         DBFreeStatement(hStmt);
      }
      else
      {
         success = false;
      }
   }
   unlockChildList();

   if (success)
      success = AutoBindTarget::saveToDatabase(hdb);
   if (success)
      success = VersionableObject::saveToDatabase(hdb);

	return success;
}

/**
 * Save to database
 */
bool AgentPolicy::saveToDatabase(DB_HANDLE hdb)
{
	lockProperties();
   bool success = savePolicyCommonProperties(hdb);
	// Clear modifications flag and unlock object
	if (success)
		m_modified = 0;
   unlockProperties();
   return success;
}

/**
 * Delete from database
 */
bool AgentPolicy::deleteFromDatabase(DB_HANDLE hdb)
{
   bool success = NetObj::deleteFromDatabase(hdb);
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM ap_common WHERE id=?"));
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM ap_bindings WHERE policy_id=?"));
   if(success)
      success = AutoBindTarget::deleteFromDatabase(hdb);
   if(success)
      success = VersionableObject::deleteFromDatabase(hdb);
   return success;
}

/**
 * Load from database
 */
bool AgentPolicy::loadFromDatabase(DB_HANDLE hdb, UINT32 id)
{
	m_id = id;

	if (!loadCommonProperties(hdb))
   {
      DbgPrintf(2, _T("Cannot load common properties for agent policy object %d"), id);
      return false;
   }

   if (m_isDeleted)
      return true;

   TCHAR query[256];

   bool success = loadACLFromDB(hdb);

   // Load related nodes list
   if(success)
   {
      _sntprintf(query, 256, _T("SELECT node_id FROM ap_bindings WHERE policy_id=%d"), m_id);
      DB_RESULT hResult = DBSelect(hdb, query);
      if (hResult != NULL)
      {
         int numNodes = DBGetNumRows(hResult);
         for(int i = 0; i < numNodes; i++)
         {
            UINT32 nodeId = DBGetFieldULong(hResult, i, 0);
            NetObj *object = FindObjectById(nodeId);
            if (object != NULL)
            {
               if (object->getObjectClass() == OBJECT_NODE)
               {
                  addChild(object);
                  object->addParent(this);
               }
               else
               {
                  nxlog_write(MSG_AP_BINDING_NOT_NODE, EVENTLOG_ERROR_TYPE, "dd", m_id, nodeId);
               }
            }
            else
            {
               nxlog_write(MSG_INVALID_AP_BINDING, EVENTLOG_ERROR_TYPE, "dd", m_id, nodeId);
            }
         }
         DBFreeResult(hResult);
      }
      else
         success = false;
   }

   if(success)
      success = AutoBindTarget::loadFromDatabase(hdb, id);
   if(success)
      success = VersionableObject::loadFromDatabase(hdb, id);
	return true;
}

/**
 * Create NXCP message with policy data
 */
void AgentPolicy::fillMessageInternal(NXCPMessage *msg, UINT32 userId)
{
	NetObj::fillMessageInternal(msg, userId);
   AutoBindTarget::fillMessageInternal(msg, userId);
   VersionableObject::fillMessageInternal(msg, userId);
}

/**
 * Modify policy from message
 */
UINT32 AgentPolicy::modifyFromMessageInternal(NXCPMessage *request)
{
   updateVersion();
   AutoBindTarget::modifyFromMessageInternal(request);
   return NetObj::modifyFromMessageInternal(request);
}

/**
 * Create deployment message
 */
bool AgentPolicy::createDeploymentMessage(NXCPMessage *msg)
{
	msg->setField(VID_POLICY_TYPE, (WORD)m_policyType);
	msg->setField(VID_GUID, m_guid);
	return true;
}

/**
 * Create uninstall message
 */
bool AgentPolicy::createUninstallMessage(NXCPMessage *msg)
{
	msg->setField(VID_POLICY_TYPE, (WORD)m_policyType);
	msg->setField(VID_GUID, m_guid);
	return true;
}

/**
 * Serialize object to JSON
 */
json_t *AgentPolicy::toJson()
{
   json_t *root = NetObj::toJson();
   json_object_set_new(root, "policyType", json_integer(m_policyType));
   AutoBindTarget::toJson(root);
   VersionableObject::toJson(root);
   return root;
}
