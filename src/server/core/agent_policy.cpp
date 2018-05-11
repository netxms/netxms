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
AgentPolicy::AgentPolicy(int type) : NetObj()
{
	m_version = 0x00010000;
	m_policyType = type;
	m_flags = 0;
   m_deployFilter = NULL;
   m_deployFilterSource = NULL;
   m_status = STATUS_NORMAL;
}

/**
 * Constructor for user-initiated object creation
 */
AgentPolicy::AgentPolicy(const TCHAR *name, int type) : NetObj()
{
	nx_strncpy(m_name, name, MAX_OBJECT_NAME);
	m_version = 0x00010000;
	m_policyType = type;
   m_flags = 0;
   m_deployFilter = NULL;
   m_deployFilterSource = NULL;
   m_status = STATUS_NORMAL;
}

/**
 * Destrutor
 */
AgentPolicy::~AgentPolicy()
{
   free(m_deployFilterSource);
   delete m_deployFilter;
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

   DB_STATEMENT hStmt;
   if (!IsDatabaseRecordExist(hdb, _T("ap_common"), _T("id"), m_id))
      hStmt = DBPrepare(hdb, _T("INSERT INTO ap_common (policy_type,version,flags,deploy_filter,id) VALUES (?,?,?,?,?)"));
   else
      hStmt = DBPrepare(hdb, _T("UPDATE ap_common SET policy_type=?,version=?,flags=?,deploy_filter=? WHERE id=?"));
   if (hStmt == NULL)
      return false;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_policyType);
   DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_version);
   DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_flags);
   DBBind(hStmt, 4, DB_SQLTYPE_TEXT, m_deployFilterSource, DB_BIND_STATIC);
   DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, m_id);
   bool success = DBExecute(hStmt);
   DBFreeStatement(hStmt);

   // Save access list
   if (success)
      success = saveACLToDB(hdb);

   // Update node bindings
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM ap_bindings WHERE policy_id=?"));

   lockChildList(false);
   if (success && (m_childList->size() > 0))
   {
      hStmt = DBPrepare(hdb, _T("INSERT INTO ap_bindings (policy_id,node_id) VALUES (?,?)"), m_childList->size() > 1);
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
bool AgentPolicy::loadFromDatabase(DB_HANDLE hdb, UINT32 dwId)
{
	m_id = dwId;

	if (!loadCommonProperties(hdb))
   {
      DbgPrintf(2, _T("Cannot load common properties for agent policy object %d"), dwId);
      return false;
   }

   if (m_isDeleted)
      return true;

   TCHAR query[256];

   loadACLFromDB(hdb);

   _sntprintf(query, 256, _T("SELECT version,flags,deploy_filter FROM ap_common WHERE id=%d"), dwId);
   DB_RESULT hResult = DBSelect(hdb, query);
   if (hResult == NULL)
      return false;

   m_version = DBGetFieldULong(hResult, 0, 0);
   m_flags = DBGetFieldULong(hResult, 0, 1);
   m_deployFilterSource = DBGetField(hResult, 0, 2, NULL, 0);
   if ((m_deployFilterSource != NULL) && (*m_deployFilterSource != 0))
   {
      TCHAR error[256];
      m_deployFilter = NXSLCompile(m_deployFilterSource, error, 256, NULL);
      if (m_deployFilter == NULL)
      {
         TCHAR buffer[1024];
         _sntprintf(buffer, 1024, _T("AgentPolicy::%s::%d"), m_name, m_id);
         PostEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", buffer, error, m_id);
         nxlog_write(MSG_POLICY_SCRIPT_COMPILATION_ERROR, EVENTLOG_WARNING_TYPE, "dss", m_id, m_name, error);
      }
   }
   DBFreeResult(hResult);

   // Load related nodes list
   _sntprintf(query, 256, _T("SELECT node_id FROM ap_bindings WHERE policy_id=%d"), m_id);
   hResult = DBSelect(hdb, query);
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
	return true;
}

/**
 * Create NXCP message with policy data
 */
void AgentPolicy::fillMessageInternal(NXCPMessage *msg, UINT32 userId)
{
	NetObj::fillMessageInternal(msg, userId);
	msg->setField(VID_POLICY_TYPE, (WORD)m_policyType);
	msg->setField(VID_VERSION, m_version);
   msg->setField(VID_FLAGS, m_flags);
   msg->setField(VID_AUTOBIND_FILTER, CHECK_NULL_EX(m_deployFilterSource));
}

/**
 * Modify policy from message
 */
UINT32 AgentPolicy::modifyFromMessageInternal(NXCPMessage *request)
{
   // Change flags
   if (request->isFieldExist(VID_FLAGS))
      m_flags = request->getFieldAsUInt32(VID_FLAGS);

   // Change apply filter
   if (request->isFieldExist(VID_AUTOBIND_FILTER))
   {
      free(m_deployFilterSource);
      delete m_deployFilter;
      m_deployFilterSource = request->getFieldAsString(VID_AUTOBIND_FILTER);
      if ((m_deployFilterSource != NULL) && (*m_deployFilterSource != 0))
      {
         TCHAR error[256];
         m_deployFilter = NXSLCompile(m_deployFilterSource, error, 256, NULL);
         if (m_deployFilter == NULL)
         {
            TCHAR buffer[1024];
            _sntprintf(buffer, 1024, _T("AgentPolicy::%s::%d"), m_name, m_id);
            PostEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", buffer, error, m_id);
            nxlog_write(MSG_POLICY_SCRIPT_COMPILATION_ERROR, EVENTLOG_WARNING_TYPE, "dss", m_id, m_name, error);
         }
      }
      else
      {
         m_deployFilter = NULL;
      }
   }

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
   json_object_set_new(root, "version", json_integer(m_version));
   json_object_set_new(root, "policyType", json_integer(m_policyType));
   json_object_set_new(root, "deployFilter", json_string_t(m_deployFilterSource));
   return root;
}

/**
 * Check if policy should be automatically deployed to given node
 * Returns AutoBindDecision_Bind if applicable, AutoBindDecision_Unbind if not,
 * AutoBindDecision_Ignore if no change required (script error or no auto deploy)
 */
AutoBindDecision AgentPolicy::isApplicable(Node *node)
{
   AutoBindDecision result = AutoBindDecision_Ignore;

   NXSL_VM *filter = NULL;
   lockProperties();
   if ((m_flags & PF_AUTO_DEPLOY) && (m_deployFilter != NULL))
   {
      filter = new NXSL_VM(new NXSL_ServerEnv());
      if (!filter->load(m_deployFilter))
      {
         TCHAR buffer[1024];
         _sntprintf(buffer, 1024, _T("AgentPolicy::%s::%d"), m_name, m_id);
         PostEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", buffer, filter->getErrorText(), m_id);
         nxlog_write(MSG_POLICY_SCRIPT_EXECUTION_ERROR, EVENTLOG_WARNING_TYPE, "dss", m_id, m_name, filter->getErrorText());
         delete_and_null(filter);
      }
   }
   unlockProperties();

   if (filter == NULL)
      return result;

   filter->setGlobalVariable(_T("$object"), node->createNXSLObject());
   filter->setGlobalVariable(_T("$node"), node->createNXSLObject());
   if (filter->run())
   {
      NXSL_Value *value = filter->getResult();
      if (!value->isNull())
         result = ((value != NULL) && (value->getValueAsInt32() != 0)) ? AutoBindDecision_Bind : AutoBindDecision_Unbind;
   }
   else
   {
      lockProperties();
      TCHAR buffer[1024];
      _sntprintf(buffer, 1024, _T("AgentPolicy::%s::%d"), m_name, m_id);
      PostEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", buffer, filter->getErrorText(), m_id);
      nxlog_write(MSG_POLICY_SCRIPT_EXECUTION_ERROR, EVENTLOG_WARNING_TYPE, "dss", m_id, m_name, filter->getErrorText());
      unlockProperties();
   }
   delete filter;
   return result;
}
