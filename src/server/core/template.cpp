/*
** NetXMS - Network Management System
** Copyright (C) 2003-2020 Victor Kirhenshtein
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
** File: template.cpp
**
**/

#include "nxcore.h"

/**
 * Redefined status calculation for template group
 */
void TemplateGroup::calculateCompoundStatus(BOOL bForcedRecalc)
{
   m_status = STATUS_NORMAL;
}

/**
 * Called by client session handler to check if threshold summary should be shown for this object.
 */
bool TemplateGroup::showThresholdSummary()
{
   return false;
}

/**
 * Create policy object of correct class based on type ID
 */
static GenericAgentPolicy *CreatePolicy(uuid guid, const TCHAR *type, UINT32 ownerId)
{
   if (!_tcsicmp(type, _T("FileDelivery")))
      return new FileDeliveryPolicy(guid, ownerId);
   return new GenericAgentPolicy(guid, type, ownerId);
}

/**
 * Create policy object of correct class based on type ID
 */
static GenericAgentPolicy *CreatePolicy(const TCHAR *name, const TCHAR *type, UINT32 ownerId)
{
   if (!_tcsicmp(type, _T("FileDelivery")))
      return new FileDeliveryPolicy(name, ownerId);
   return new GenericAgentPolicy(name, type, ownerId);
}

Template::Template() : super(), AutoBindTarget(this), VersionableObject(this)
{
   m_policyList = new HashMap<uuid, GenericAgentPolicy>(Ownership::True);
   m_deletedPolicyList = new ObjectArray<GenericAgentPolicy>(0, 16, Ownership::True);
}

/**
 * Create template object from import file
 */
Template::Template(ConfigEntry *config) : super(config), AutoBindTarget(this, config), VersionableObject(this, config)
{
   UINT32 flags = config->getSubEntryValueAsUInt(_T("flags"), 0, 0);
   if (flags != 0)
   {
      if (flags & AAF_AUTO_APPLY)
         m_autoBindFlag = true;
      if (flags & AAF_AUTO_REMOVE)
         m_autoUnbindFlag = true;
      m_flags &= !(AAF_AUTO_APPLY | AAF_AUTO_REMOVE);
   }

   m_policyList = new HashMap<uuid, GenericAgentPolicy>(Ownership::True);
   m_deletedPolicyList = new ObjectArray<GenericAgentPolicy>(0, 16, Ownership::True);
   ObjectArray<ConfigEntry> *dcis = config->getSubEntries(_T("agentPolicy#*"));
   for(int i = 0; i < dcis->size(); i++)
   {
      ConfigEntry *e = dcis->get(i);
      uuid guid = e->getSubEntryValueAsUUID(_T("guid"));
      GenericAgentPolicy *curr = CreatePolicy(guid, e->getSubEntryValue(_T("policyType")), m_id);
      curr->updateFromImport(e);
      m_policyList->set(guid, curr);
   }
   delete dcis;

   _tcslcpy(m_name, config->getSubEntryValue(_T("name"), 0, _T("Unnamed Template")), MAX_OBJECT_NAME);
}

/**
 * Create new template
 */
Template::Template(const TCHAR *pszName) : super(pszName), AutoBindTarget(this), VersionableObject(this)
{
   m_policyList = new HashMap<uuid, GenericAgentPolicy>(Ownership::True);
   m_deletedPolicyList = new ObjectArray<GenericAgentPolicy>(0, 16, Ownership::True);
}

/**
 * Destructor
 */
Template::~Template()
{
   delete m_policyList;
   delete m_deletedPolicyList;
}

/**
 * Redefined status calculation for template
 */
void Template::calculateCompoundStatus(BOOL bForcedRecalc)
{
   m_status = STATUS_NORMAL;
}

/**
 * Save template object to database
 */
bool Template::saveToDatabase(DB_HANDLE hdb)
{
   bool success = super::saveToDatabase(hdb);
   if(success)
   {
      if ((m_modified & MODIFY_OTHER) && !IsDatabaseRecordExist(hdb, _T("templates"), _T("id"), m_id))
      {
         DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO templates (id) VALUES (?)"));
         if (hStmt != NULL)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
            success = DBExecute(hStmt);
            DBFreeStatement(hStmt);
         }
      }
   }

   if (success && (m_modified & MODIFY_RELATIONS))
   {
      // Update members list
      success = executeQueryOnObject(hdb, _T("DELETE FROM dct_node_map WHERE template_id=?"));
      lockChildList(false);
      if (success && !getChildList()->isEmpty())
      {
         DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO dct_node_map (template_id,node_id) VALUES (?,?)"), getChildList()->size() > 1);
         if (hStmt != NULL)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
            for(int i = 0; success && (i < getChildList()->size()); i++)
            {
               DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, getChildList()->get(i)->getId());
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
   }

   if (success && (m_modified & MODIFY_OTHER))
      success = AutoBindTarget::saveToDatabase(hdb);
   if (success && (m_modified & MODIFY_OTHER))
      success = VersionableObject::saveToDatabase(hdb);

   if (success && (m_modified & MODIFY_POLICY))
   {
      lockProperties();

      for(int i = 0; (i < m_deletedPolicyList->size()) && success; i++)
         success = m_deletedPolicyList->get(i)->deleteFromDatabase(hdb);

      if (success)
         m_deletedPolicyList->clear();

      Iterator<GenericAgentPolicy> *it = m_policyList->iterator();
      while(it->hasNext() && success)
      {
         GenericAgentPolicy *object = it->next();
         success = object->saveToDatabase(hdb);
      }
      delete it;

      unlockProperties();
   }

   return success;
}

/**
 * Delete template object from database
 */
bool Template::deleteFromDatabase(DB_HANDLE hdb)
{
   bool success = super::deleteFromDatabase(hdb);
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM templates WHERE id=?"));
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM dct_node_map WHERE template_id=?"));
   if (success)
      success = AutoBindTarget::deleteFromDatabase(hdb);
   if (success)
      success = VersionableObject::deleteFromDatabase(hdb);
   if (success)
   {
      Iterator<GenericAgentPolicy> *it = m_policyList->iterator();
      while(it->hasNext() && success)
      {
         GenericAgentPolicy *object = it->next();
         success = object->deleteFromDatabase(hdb);
      }
      delete it;
   }
   return success;
}

/**
 * Load template object from database
 */
bool Template::loadFromDatabase(DB_HANDLE hdb, UINT32 id)
{
   bool success = super::loadFromDatabase(hdb, id);

   if (success)
      success = AutoBindTarget::loadFromDatabase(hdb, id);
   if (success)
      success = VersionableObject::loadFromDatabase(hdb, id);
   if (success)
   {
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT guid,policy_type FROM ap_common WHERE owner_id=?"));
      if (hStmt != NULL)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
         DB_RESULT hResult = DBSelectPrepared(hStmt);
         if (hResult != NULL)
         {
            int count = DBGetNumRows(hResult);
            for(int i = 0; i < count && success; i++)
            {
               TCHAR type[32];
               uuid guid = DBGetFieldGUID(hResult, i, 0);
               DBGetField(hResult, i, 1, type, 32);
               GenericAgentPolicy *curr = CreatePolicy(guid, type, m_id);
               success = curr->loadFromDatabase(hdb);
               m_policyList->set(guid, curr);
            }
            DBFreeResult(hResult);
         }
         else
         {
            success = false;
         }
         DBFreeStatement(hStmt);
      }
      else
      {
         success = false;
      }
   }

   // Load related nodes list
   if (!m_isDeleted)
   {
      TCHAR szQuery[256];
      _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("SELECT node_id FROM dct_node_map WHERE template_id=%d"), m_id);
      DB_RESULT hResult = DBSelect(hdb, szQuery);
      if (hResult != NULL)
      {
         int count = DBGetNumRows(hResult);
         for(int i = 0; i < count; i++)
         {
            UINT32 dwNodeId = DBGetFieldULong(hResult, i, 0);
            NetObj *pObject = FindObjectById(dwNodeId);
            if (pObject != NULL)
            {
               if (pObject-isDataCollectionTarget())
               {
                  addChild(pObject);
                  pObject->addParent(this);
               }
               else
               {
                  nxlog_write(NXLOG_ERROR,
                           _T("Inconsistent database: template object %s [%u] has reference to object %s [%u] which is not data collection target"),
                           m_name, m_id, pObject->getName(), pObject->getId());
               }
            }
            else
            {
               nxlog_write(NXLOG_ERROR, _T("Inconsistent database: template object %s [%u] has reference to non-existent object [%u]"),
                        m_name, m_id, dwNodeId);
            }
         }
         DBFreeResult(hResult);
      }
   }

   return success;
}

/**
 * Create management pack record
 */
void Template::createExportRecord(StringBuffer &xml)
{
   xml.append(_T("\t\t<template id=\""));
   xml.append(m_id);
   xml.append(_T("\">\n\t\t\t<guid>"));
   xml.append(m_guid.toString());
   xml.append(_T("</guid>\n\t\t\t<name>"));
   xml.append(EscapeStringForXML2(m_name));
   xml.append(_T("</name>\n\t\t\t<flags>"));
   xml.append(m_flags);
   xml.append(_T("</flags>\n\t\t\t<comments>"));
   xml.append(EscapeStringForXML2(m_comments));
   xml.append(_T("</comments>\n"));

   // Path in groups
   StringList path;
   ObjectArray<NetObj> *list = getParents(OBJECT_TEMPLATEGROUP);
   TemplateGroup *parent = NULL;
   while(list->size() > 0)
   {
      parent = static_cast<TemplateGroup*>(list->get(0));
      path.add(parent->getName());
      delete list;
      list = parent->getParents(OBJECT_TEMPLATEGROUP);
   }
   delete list;

   xml.append(_T("\t\t\t<path>\n"));
   for(int j = path.size() - 1, id = 1; j >= 0; j--, id++)
   {
      xml.append(_T("\t\t\t\t<element id=\""));
      xml.append(id);
      xml.append(_T("\">"));
      xml.append(EscapeStringForXML2(path.get(j)));
      xml.append(_T("</element>\n"));
   }
   xml.append(_T("\t\t\t</path>\n\t\t\t<dataCollection>\n"));

   lockDciAccess(false);
   for(int i = 0; i < m_dcObjects->size(); i++)
      m_dcObjects->get(i)->createExportRecord(xml);
   unlockDciAccess();

   xml.append(_T("\t\t\t</dataCollection>\n"));

   lockProperties();
   Iterator<GenericAgentPolicy> *it = m_policyList->iterator();
   while(it->hasNext())
   {
      GenericAgentPolicy *object = it->next();
      object->createExportRecord(xml);
   }
   delete it;
   unlockProperties();

   AutoBindTarget::createExportRecord(xml);
   xml.append(_T("\t\t</template>\n"));
}

/**
 * Apply template to data collection target
 */
BOOL Template::applyToTarget(DataCollectionTarget *target)
{
   //Print in log that policies will be installed
   nxlog_debug_tag(_T("obj.dc"), 2, _T("Apply %d policy items from template \"%s\" to target \"%s\""),
                   m_policyList->size(), m_name, target->getName());

   forceInstallPolicy(target);

   return super::applyToTarget(target);
}

/**
 * Initiate forced policy installation
 */
void Template::forceInstallPolicy(DataCollectionTarget *target)
{
   lockProperties();
   Iterator<GenericAgentPolicy> *it = m_policyList->iterator();
   while(it->hasNext())
   {
      GenericAgentPolicy *object = it->next();
      ServerJob *job = new PolicyInstallJob(target, m_id, object->getGuid(), object->getName(), 0);
      if (!AddJob(job))
      {
         delete job;
         nxlog_debug_tag(_T("obj.dc"), 2, _T("Unable to create job for %s(%s) policy installation on %s[%d] node"), object->getName(),
               (const TCHAR *)object->getGuid().toString(), target->getName(), target->getId());
      }
   }
   delete it;
   setModified(MODIFY_POLICY, false);
   unlockProperties();
}

/**
 * Unlock data collection items list
 */
void Template::applyDCIChanges()
{
   lockProperties();
   if (m_dciListModified)
   {
      updateVersion();
      setModified(MODIFY_OTHER);
   }
   unlockProperties();
   super::applyDCIChanges();
}

/**
 * Create NXCP message with object's data
 */
void Template::fillMessageInternal(NXCPMessage *pMsg, UINT32 userId)
{
   super::fillMessageInternal(pMsg, userId);
   AutoBindTarget::fillMessage(pMsg);
   VersionableObject::fillMessage(pMsg);
}

/**
 * Modify template object from NXCP message
 */
UINT32 Template::modifyFromMessageInternal(NXCPMessage *pRequest)
{
   // Change flags
   if (pRequest->isFieldExist(VID_FLAGS))
   {
      UINT32 mask = pRequest->isFieldExist(VID_FLAGS_MASK) ? pRequest->getFieldAsUInt32(VID_FLAGS_MASK) : 0xFFFFFFFF;
      m_flags &= ~mask;
      m_flags |= pRequest->getFieldAsUInt32(VID_FLAGS) & mask;
   }
   AutoBindTarget::modifyFromMessage(pRequest);

   return super::modifyFromMessageInternal(pRequest);
}

/**
 * Update object from imported configuration
 */
void Template::updateFromImport(ConfigEntry *config)
{
   super::updateFromImport(config);
   AutoBindTarget::updateFromImport(config);
   VersionableObject::updateFromImport(config);

   lockProperties();

   m_policyList->clear();
   ObjectArray<ConfigEntry> *policies = config->getSubEntries(_T("agentPolicy#*"));
   for(int i = 0; i < policies->size(); i++)
   {
      ConfigEntry *e = policies->get(i);
      uuid guid = e->getSubEntryValueAsUUID(_T("guid"));
      GenericAgentPolicy *curr = CreatePolicy(guid, e->getSubEntryValue(_T("policyType")), m_id);
      curr->updateFromImport(e);
      m_policyList->set(guid, curr);
   }
   delete policies;

   setModified(MODIFY_ALL);

   unlockProperties();
}

/**
 * Serialize object to JSON
 */
json_t *Template::toJson()
{
   json_t *root = super::toJson();
   AutoBindTarget::toJson(root);
   VersionableObject::toJson(root);

   lockProperties();
   Iterator<GenericAgentPolicy> *it = m_policyList->iterator();
   while(it->hasNext())
   {
      GenericAgentPolicy *object = it->next();
      json_object_set_new(root, "agentPolicy", object->toJson());
   }
   delete it;
   unlockProperties();

   return root;
}

/**
 * Called when data collection configuration changed
 */
void Template::onDataCollectionChange()
{
   queueUpdate();
}

/**
 * Prepare template for deletion
 */
void Template::prepareForDeletion()
{
   lockChildList(false);
   for(int i = 0; i < getChildList()->size(); i++)
   {
      NetObj *object = getChildList()->get(i);
      if (object->isDataCollectionTarget())
         queueRemoveFromTarget(object->getId(), true);
      if (object->getObjectClass() == OBJECT_NODE)
      {
         removeAllPolicies((Node *)object);
      }
   }
   unlockChildList();
   super::prepareForDeletion();
}

void Template::removeAllPolicies(Node *node)
{
   Iterator<GenericAgentPolicy> *it = m_policyList->iterator();
   while(it->hasNext())
   {
      GenericAgentPolicy *policy = it->next();
      ServerJob *job = new PolicyUninstallJob(node, policy->getType(), policy->getGuid(), 0);
      if (!AddJob(job))
         delete job;
   }
   delete it;
}

/**
 * Check policy exist
 */
bool Template::hasPolicy(const uuid& guid)
{
   lockProperties();
   bool hasPolicy = m_policyList->contains(guid);
   unlockProperties();
   return hasPolicy;
}

/**
 * Get agent policy copy
 */
GenericAgentPolicy *Template::getAgentPolicyCopy(const uuid& guid)
{
   lockProperties();
   GenericAgentPolicy *policy = m_policyList->get(guid);
   if (policy != NULL)
      policy = policy->clone();
   unlockProperties();
   return policy;
}

/**
 * Create NXCP message with object's data
 */
void Template::fillPolicyMessage(NXCPMessage *pMsg)
{
   lockProperties();
   UINT32 fieldId = VID_AGENT_POLICY_BASE;
   Iterator<GenericAgentPolicy> *it = m_policyList->iterator();
   int count = 0;
   while(it->hasNext())
   {
      GenericAgentPolicy *object = it->next();
      object->fillMessage(pMsg, fieldId);
      fieldId += 100;
      count++;
   }
   delete it;
   pMsg->setField(VID_POLICY_COUNT, count);
   unlockProperties();
}

/**
 * Update policy if GUID is provided and create policy if GUID is NULL
 */
uuid Template::updatePolicyFromMessage(NXCPMessage *request)
{
   NXCPMessage msg(CMD_UPDATE_AGENT_POLICY, m_id);
   bool updated = false;

   lockProperties();

   uuid guid;
   if (request->isFieldExist(VID_GUID))
   {
      guid = request->getFieldAsGUID(VID_GUID);
      GenericAgentPolicy *policy = m_policyList->get(guid);
      if (policy != NULL)
      {
         policy->modifyFromMessage(request);
         policy->fillUpdateMessage(&msg);
         updated = true;
      }
      else
      {
         guid = uuid::NULL_UUID;
      }
   }
   else
   {
      TCHAR name[MAX_DB_STRING], policyType[32];
      GenericAgentPolicy *curr = CreatePolicy(request->getFieldAsString(VID_NAME, name, MAX_DB_STRING), request->getFieldAsString(VID_POLICY_TYPE, policyType, 32), m_id);
      curr->modifyFromMessage(request);
      m_policyList->set(curr->getGuid(), curr);
      curr->fillUpdateMessage(&msg);
      guid = curr->getGuid();
      updated = true;
   }

   if (updated)
   {
      updateVersion();
      setModified(MODIFY_POLICY, false);
   }

   unlockProperties();

   if(updated)
   {
      msg.setField(VID_TEMPLATE_ID, m_id);
      NotifyClientsOnPolicyUpdate(&msg, this);
   }
   return guid;
}

/**
 * Remove policy by uuid
 */
bool Template::removePolicy(const uuid& guid)
{
   lockProperties();
   bool success = false;
   GenericAgentPolicy *policy = m_policyList->get(guid);
   if (policy != NULL)
   {
      lockChildList(false);
      for(int i = 0; i < getChildList()->size(); i++)
      {
         NetObj *object = getChildList()->get(i);
         if (object->getObjectClass() == OBJECT_NODE)
         {
            ServerJob *job = new PolicyUninstallJob((Node *)object, policy->getType(), guid, 0);
            if (!AddJob(job))
               delete job;
         }
      }
      unlockChildList();

      m_policyList->unlink(guid);
      m_deletedPolicyList->add(policy);
      updateVersion();

      NotifyClientsOnPolicyDelete(guid, this);
      setModified(MODIFY_POLICY, false);
      success = true;
   }
   unlockProperties();
   return success;
}

/**
 * Verify agent policy list with current policy versions
 */
void Template::applyPolicyChanges()
{
   lockChildList(false);
   for(int i = 0; i < getChildList()->size(); i++)
   {
      NetObj *object = getChildList()->get(i);
      if (object->getObjectClass() == OBJECT_NODE)
      {
         AgentPolicyInfo *ap;
         AgentConnection *conn = static_cast<Node*>(object)->getAgentConnection();
         if (conn != NULL)
         {
            UINT32 rcc = conn->getPolicyInventory(&ap);
            if (rcc == RCC_SUCCESS)
            {
               checkPolicyBind(static_cast<Node*>(object), ap);
               delete ap;
            }
            conn->decRefCount();
         }
      }
   }
   unlockChildList();
}

/**
 * Verify agent policy list with current policy versions
 */
void Template::forceApplyPolicyChanges()
{
   ObjectArray<Node> nodes(64, 64);
   lockChildList(false);
   for(int i = 0; i < getChildList()->size(); i++)
   {
      NetObj *object = getChildList()->get(i);
      if (object->getObjectClass() == OBJECT_NODE)
      {
         object->incRefCount();
         nodes.add(static_cast<Node*>(object));
      }
   }
   unlockChildList();

   if (!nodes.isEmpty())
   {
      lockProperties();
      for(int i = 0; i < nodes.size(); i++)
      {
         Node *node = nodes.get(i);
         Iterator<GenericAgentPolicy> *it = m_policyList->iterator();
         while(it->hasNext())
         {
            GenericAgentPolicy *policy = it->next();
            ServerJob *job = new PolicyInstallJob(node, m_id, policy->getGuid(), policy->getName(), 0);
            if (AddJob(job))
            {
               DbgPrintf(5, _T("Template::forceApplyPolicyChanges(%s): \"%s\" policy deploy scheduled for \"%s\" node"), node->getName(), m_name, node->getName());
            }
            else
            {
               delete job;
               DbgPrintf(5, _T("Template::forceApplyPolicyChanges(%s): \"%s\" policy deploy is not possible to scheduled for \"%s\" node"), node->getName(), m_name, node->getName());
            }
         }
         delete it;
         node->decRefCount();
      }
      unlockProperties();
   }
}

/**
 * Verify agent policy list with current policy versions
 */
void Template::applyPolicyChanges(DataCollectionTarget *object)
{
   lockChildList(false);
   if (object->getObjectClass() == OBJECT_NODE)
   {
      AgentPolicyInfo *ap;
      AgentConnection *conn = static_cast<Node*>(object)->getAgentConnection();
      if (conn != NULL)
      {
         UINT32 rcc = conn->getPolicyInventory(&ap);
         if (rcc == RCC_SUCCESS)
         {
            checkPolicyBind(static_cast<Node*>(object), ap);
            delete ap;
         }
         conn->decRefCount();
      }
   }
   unlockChildList();
}

/**
 * Check agent policy binding for exact template
 */
void Template::checkPolicyBind(Node *node, AgentPolicyInfo *ap)
{
   lockProperties();
   Iterator<GenericAgentPolicy> *it = m_policyList->iterator();
   while(it->hasNext())
   {
      GenericAgentPolicy *object = it->next();
      int j;
      for(j = 0; j < ap->size(); j++)
      {
         if (ap->getGuid(j).equals(object->getGuid()) && ap->getVersion(j) == object->getVersion())
            break;
      }
      if (j == ap->size())
      {
         ServerJob *job = new PolicyInstallJob(node, m_id, object->getGuid(), object->getName(), 0);
         if (AddJob(job))
         {
            DbgPrintf(5, _T("Template::checkPolicyBind(%s): \"%s\" policy deploy scheduled for \"%s\" node"), node->getName(), m_name, node->getName());
         }
         else
         {
            delete job;
            DbgPrintf(5, _T("Template::checkPolicyBind(%s): \"%s\" policy deploy is not possible to scheduled for \"%s\" node"), node->getName(), m_name, node->getName());
         }
      }
   }
   delete it;
   unlockProperties();
}
