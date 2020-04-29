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
bool TemplateGroup::showThresholdSummary() const
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
   m_policyList = new SharedObjectArray<GenericAgentPolicy>(0, 16);
   m_deletedPolicyList = new SharedObjectArray<GenericAgentPolicy>(0, 16);
}

/**
 * Create new template
 */
Template::Template(const TCHAR *name, const uuid& guid) : super(name, guid), AutoBindTarget(this), VersionableObject(this)
{
   m_policyList = new SharedObjectArray<GenericAgentPolicy>(0, 16);
   m_deletedPolicyList = new SharedObjectArray<GenericAgentPolicy>(0, 16);
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
         if (hStmt != nullptr)
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
      readLockChildList();
      if (success && !getChildList().isEmpty())
      {
         DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO dct_node_map (template_id,node_id) VALUES (?,?)"), getChildList().size() > 1);
         if (hStmt != nullptr)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
            for(int i = 0; success && (i < getChildList().size()); i++)
            {
               DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, getChildList().get(i)->getId());
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

      for (int i = 0; i < m_policyList->size() && success; i++)
      {
         GenericAgentPolicy *object = m_policyList->get(i);
         success = object->saveToDatabase(hdb);
      }

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
   for (int i = 0; i < m_policyList->size() && success; i++)
   {
      GenericAgentPolicy *object = m_policyList->get(i);
      success = object->deleteFromDatabase(hdb);
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
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
         DB_RESULT hResult = DBSelectPrepared(hStmt);
         if (hResult != nullptr)
         {
            int count = DBGetNumRows(hResult);
            for(int i = 0; i < count && success; i++)
            {
               TCHAR type[32];
               uuid guid = DBGetFieldGUID(hResult, i, 0);
               DBGetField(hResult, i, 1, type, 32);
               GenericAgentPolicy *curr = CreatePolicy(guid, type, m_id);
               success = curr->loadFromDatabase(hdb);
               m_policyList->add(curr);
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
      if (hResult != nullptr)
      {
         int count = DBGetNumRows(hResult);
         for(int i = 0; i < count; i++)
         {
            uint32_t objectId = DBGetFieldULong(hResult, i, 0);
            shared_ptr<NetObj> object = FindObjectById(objectId);
            if (object != nullptr)
            {
               if (object->isDataCollectionTarget())
               {
                  addChild(object);
                  object->addParent(self());
               }
               else
               {
                  nxlog_write(NXLOG_ERROR,
                           _T("Inconsistent database: template object %s [%u] has reference to object %s [%u] which is not data collection target"),
                           m_name, m_id, object->getName(), object->getId());
               }
            }
            else
            {
               nxlog_write(NXLOG_ERROR, _T("Inconsistent database: template object %s [%u] has reference to non-existent object [%u]"),
                        m_name, m_id, objectId);
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
   SharedObjectArray<NetObj> *list = getParents(OBJECT_TEMPLATEGROUP);
   TemplateGroup *parent = nullptr;
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

   readLockDciAccess();
   for(int i = 0; i < m_dcObjects->size(); i++)
      m_dcObjects->get(i)->createExportRecord(xml);
   unlockDciAccess();

   xml.append(_T("\t\t\t</dataCollection>\n\t\t\t<agentPolicies>\n"));

   lockProperties();
   for (int i = 0; i < m_policyList->size(); i++)
   {
      m_policyList->get(i)->createExportRecord(xml, i + 1);
   }
   unlockProperties();

   xml.append(_T("\t\t\t</agentPolicies>\n"));

   AutoBindTarget::createExportRecord(xml);
   xml.append(_T("\t\t</template>\n"));
}

/**
 * Apply template to data collection target
 */
bool Template::applyToTarget(const shared_ptr<DataCollectionTarget>& target)
{
   // Print in log that policies will be installed
   nxlog_debug_tag(_T("obj.dc"), 4, _T("Apply %d policy items from template \"%s\" to target \"%s\""),
            m_policyList->size(), m_name, target->getName());

   if (target->getObjectClass() == OBJECT_NODE)
      forceDeployPolicies(static_pointer_cast<Node>(target));

   return super::applyToTarget(target);
}

/**
 * Initiate forced policy installation
 */
void Template::forceDeployPolicies(const shared_ptr<Node>& target)
{
   lockProperties();
   for (int i = 0; i < m_policyList->size(); i++)
   {
      GenericAgentPolicy *object = m_policyList->get(i);
      auto data = make_shared<AgentPolicyDeploymentData>(target->getAgentConnection(), target, target->isNewPolicyTypeFormatSupported());
      data->forceInstall = true;
      _sntprintf(data->debugId, 256, _T("%s [%u] from %s/%s"), target->getName(), target->getId(), getName(), object->getName());
      ThreadPoolExecute(g_agentConnectionThreadPool, object, &GenericAgentPolicy::deploy, data);
   }
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

   uint32_t flags = config->getSubEntryValueAsUInt(_T("flags"), 0, 0);
   if (flags != 0)
   {
      if (flags & AAF_AUTO_APPLY)
         m_autoBindFlag = true;
      if (flags & AAF_AUTO_REMOVE)
         m_autoUnbindFlag = true;
      m_flags &= !(AAF_AUTO_APPLY | AAF_AUTO_REMOVE);
   }

   m_policyList->clear();
   ConfigEntry *policyRoot = config->findEntry(_T("agentPolicies"));
   if (policyRoot != nullptr)
   {
      ObjectArray<ConfigEntry> *policies = policyRoot->getSubEntries(_T("agentPolicy#*"));
      for(int i = 0; i < policies->size(); i++)
      {
         ConfigEntry *e = policies->get(i);
         uuid guid = e->getSubEntryValueAsUUID(_T("guid"));
         GenericAgentPolicy *curr = CreatePolicy(guid, e->getSubEntryValue(_T("type"), 0, _T("Unknown")), m_id);
         curr->updateFromImport(e);
         for (int i = 0; i < m_policyList->size(); i++)
         {
            if (m_policyList->get(i)->getGuid().equals(guid))
               break;
         }
         if (i != m_policyList->size())
            m_policyList->replace(i, curr);
         else
            m_policyList->add(curr);
      }
      delete policies;
   }

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
   for (int i = 0; i < m_policyList->size(); i++)
   {
      GenericAgentPolicy *object = m_policyList->get(i);
      json_object_set_new(root, "agentPolicy", object->toJson());
   }
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
   readLockChildList();
   for(int i = 0; i < getChildList().size(); i++)
   {
      NetObj *object = getChildList().get(i);
      if (object->isDataCollectionTarget())
         queueRemoveFromTarget(object->getId(), true);
      if (object->getObjectClass() == OBJECT_NODE)
      {
         removeAllPolicies(static_cast<Node*>(object));
      }
   }
   unlockChildList();
   super::prepareForDeletion();
}

/**
 * Remove all policies of this template from given node
 */
void Template::removeAllPolicies(Node *node)
{
   for (int i = 0; i < m_policyList->size(); i++)
   {
      GenericAgentPolicy *policy = m_policyList->get(i);
      auto data = make_shared<AgentPolicyRemovalData>(node->getAgentConnection(), policy->getGuid(), policy->getType(), node->isNewPolicyTypeFormatSupported());
      _sntprintf(data->debugId, 256, _T("%s [%u] from %s/%s"), node->getName(), node->getId(), getName(), policy->getName());
      ThreadPoolExecute(g_agentConnectionThreadPool, RemoveAgentPolicy, data);
   }
}

/**
 * Check policy exist
 */
bool Template::hasPolicy(const uuid& guid)
{
   lockProperties();
   bool hasPolicy = false;
   for (int i = 0; i < m_policyList->size(); i++)
   {
      if (m_policyList->get(i)->getGuid().equals(guid))
      {
         hasPolicy = true;
         break;
      }
   }
   unlockProperties();
   return hasPolicy;
}


bool Template::fillMessageWithPolicy(NXCPMessage *msg, const uuid& guid)
{
   bool hasPolicy = false;
   lockProperties();
   for (int i = 0; i < m_policyList->size(); i++)
   {
      if(m_policyList->get(i)->getGuid().equals(guid))
      {
         m_policyList->get(i)->fillMessage(msg, VID_AGENT_POLICY_BASE);
         hasPolicy = true;
         break;
      }
   }
   unlockProperties();
   return hasPolicy;
}

/**
 * Create NXCP message with object's data
 */
void Template::fillPolicyMessage(NXCPMessage *pMsg)
{
   lockProperties();
   UINT32 fieldId = VID_AGENT_POLICY_BASE;
   int count = 0;
   for (int i = 0; i < m_policyList->size(); i++)
   {
      GenericAgentPolicy *object = m_policyList->get(i);
      object->fillMessage(pMsg, fieldId);
      fieldId += 100;
      count++;
   }
   pMsg->setField(VID_POLICY_COUNT, count);
   unlockProperties();
}

/**
 * Update policy if GUID is provided and create policy if GUID is nullptr
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
      GenericAgentPolicy *policy = nullptr;
      for (int i = 0; i < m_policyList->size(); i++)
      {
         if(m_policyList->get(i)->getGuid().equals(guid))
         {
            policy = m_policyList->get(i);
            break;
         }
      }
      if (policy != nullptr)
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
      m_policyList->add(curr);
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
      NotifyClientsOnPolicyUpdate(&msg, *this);
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
   shared_ptr<GenericAgentPolicy> policy;
   int index = -1;
   for (int i = 0; i < m_policyList->size(); i++)
   {
      if (m_policyList->get(i)->getGuid().equals(guid))
      {
         policy = m_policyList->getShared(i);
         m_policyList->remove(i);
         break;
      }
   }
   if (policy != nullptr)
   {
      readLockChildList();
      for(int i = 0; i < getChildList().size(); i++)
      {
         NetObj *object = getChildList().get(i);
         if (object->getObjectClass() == OBJECT_NODE)
         {
            auto data = make_shared<AgentPolicyRemovalData>(static_cast<Node*>(object)->getAgentConnection(), policy->getGuid(), policy->getType(), static_cast<Node*>(object)->isNewPolicyTypeFormatSupported());
            _sntprintf(data->debugId, 256, _T("%s [%u] from %s/%s"), object->getName(), object->getId(), getName(), policy->getName());
            ThreadPoolExecute(g_agentConnectionThreadPool, RemoveAgentPolicy, data);
         }
      }
      unlockChildList();

      m_deletedPolicyList->add(policy);
      updateVersion();

      NotifyClientsOnPolicyDelete(guid, *this);
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
   readLockChildList();
   for(int i = 0; i < getChildList().size(); i++)
   {
      NetObj *object = getChildList().get(i);
      if (object->getObjectClass() == OBJECT_NODE)
      {
         AgentPolicyInfo *ap;
         shared_ptr<AgentConnectionEx> conn = static_cast<Node*>(object)->getAgentConnection();
         if (conn != nullptr)
         {
            UINT32 rcc = conn->getPolicyInventory(&ap);
            if (rcc == RCC_SUCCESS)
            {
               checkPolicyDeployment(static_pointer_cast<Node>(getChildList().getShared(i)), ap);
               delete ap;
            }
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
   SharedObjectArray<Node> nodes(64, 64);
   readLockChildList();
   for(int i = 0; i < getChildList().size(); i++)
   {
      NetObj *object = getChildList().get(i);
      if (object->getObjectClass() == OBJECT_NODE)
      {
         nodes.add(static_pointer_cast<Node>(getChildList().getShared(i)));
      }
   }
   unlockChildList();

   if (!nodes.isEmpty())
   {
      lockProperties();
      for(int i = 0; i < nodes.size(); i++)
      {
         const shared_ptr<Node>& node = nodes.getShared(i);
         for (int i = 0; i < m_policyList->size(); i++)
         {
            const shared_ptr<GenericAgentPolicy>& policy = m_policyList->getShared(i);
            auto data = make_shared<AgentPolicyDeploymentData>(node->getAgentConnection(), node, node->isNewPolicyTypeFormatSupported());
            data->forceInstall = true;
            _sntprintf(data->debugId, 256, _T("%s [%u] from %s/%s"), node->getName(), node->getId(), getName(), policy->getName());
            ThreadPoolExecute(g_agentConnectionThreadPool, policy, &GenericAgentPolicy::deploy, data);
         }
      }
      unlockProperties();
   }
}

/**
 * Check agent policy deployment for exact template
 */
void Template::checkPolicyDeployment(const shared_ptr<Node>& node, AgentPolicyInfo *ap)
{
   shared_ptr<AgentConnectionEx> conn = node->getAgentConnection();
   if (conn == nullptr)
      return;

   lockProperties();
   for (int i = 0; i < m_policyList->size(); i++)
   {
      shared_ptr<GenericAgentPolicy> policy = m_policyList->getShared(i);
      auto data = make_shared<AgentPolicyDeploymentData>(conn, node, node->isNewPolicyTypeFormatSupported());
      data->forceInstall = true;

      int j;
      for(j = 0; j < ap->size(); j++)
      {
         if (ap->getGuid(j).equals(policy->getGuid()))
         {
            if (ap->getHash(j) != nullptr)
               memcpy(data->currHash, ap->getHash(j), MD5_DIGEST_SIZE);
            else
               memset(data->currHash, 0, MD5_DIGEST_SIZE);
            data->currVersion = ap->getVersion(j);
            break;
         }
      }

      _sntprintf(data->debugId, 256, _T("%s [%u] from %s/%s"), node->getName(), node->getId(), getName(), policy->getName());
      ThreadPoolExecute(g_agentConnectionThreadPool, policy, &GenericAgentPolicy::deploy, data);
   }
   unlockProperties();
}

/**
 * Create NXSL object for this object
 */
NXSL_Value *Template::createNXSLObject(NXSL_VM *vm) const
{
   return vm->createValue(new NXSL_Object(vm, &g_nxslTemplateClass, new shared_ptr<Template>(self())));
}
