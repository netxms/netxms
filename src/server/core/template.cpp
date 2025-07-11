/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Victor Kirhenshtein
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
 * Poller thread pool
 */
extern ThreadPool *g_pollerThreadPool;

/**
 * Redefined status calculation for template group
 */
void TemplateGroup::calculateCompoundStatus(bool forcedRecalc)
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
   if (!_tcsicmp(type, _T("LogParserConfig")))
      return new LogParserPolicy(guid, ownerId);
   return new GenericAgentPolicy(guid, type, ownerId);
}

/**
 * Create policy object of correct class based on type ID
 */
static GenericAgentPolicy *CreatePolicy(const TCHAR *name, const TCHAR *type, UINT32 ownerId)
{
   if (!_tcsicmp(type, _T("FileDelivery")))
      return new FileDeliveryPolicy(name, ownerId);
   if (!_tcsicmp(type, _T("LogParserConfig")))
      return new LogParserPolicy(name, ownerId);
   return new GenericAgentPolicy(name, type, ownerId);
}

/**
 * Default constructor
 */
Template::Template() : super(), AutoBindTarget(this), Pollable(this, Pollable::AUTOBIND), VersionableObject(this)
{
}

/**
 * Create new template
 */
Template::Template(const TCHAR *name, const uuid& guid) : super(name, guid), AutoBindTarget(this), Pollable(this, Pollable::AUTOBIND), VersionableObject(this)
{
}

/**
 * Destructor
 */
Template::~Template()
{
}

/**
 * Redefined status calculation for template
 */
void Template::calculateCompoundStatus(bool forcedRecalc)
{
   m_status = STATUS_NORMAL;
}

/**
 * Save template object to database
 */
bool Template::saveToDatabase(DB_HANDLE hdb)
{
   bool success = super::saveToDatabase(hdb);
   if (success)
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

      for(int i = 0; (i < m_deletedPolicyList.size()) && success; i++)
         success = m_deletedPolicyList.get(i)->deleteFromDatabase(hdb);

      if (success)
         m_deletedPolicyList.clear();

      for (int i = 0; i < m_policyList.size() && success; i++)
      {
         GenericAgentPolicy *object = m_policyList.get(i);
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
   for (int i = 0; i < m_policyList.size() && success; i++)
   {
      GenericAgentPolicy *object = m_policyList.get(i);
      success = object->deleteFromDatabase(hdb);
   }
   for (int i = 0; i < m_deletedPolicyList.size() && success; i++)
   {
      GenericAgentPolicy *object = m_deletedPolicyList.get(i);
      success = object->deleteFromDatabase(hdb);
   }
   return success;
}

/**
 * Load template object from database
 */
bool Template::loadFromDatabase(DB_HANDLE hdb, uint32_t id, DB_STATEMENT *preparedStatements)
{
   bool success = super::loadFromDatabase(hdb, id, preparedStatements);

   if (success)
      success = AutoBindTarget::loadFromDatabase(hdb, id);

   if (success)
      success = Pollable::loadFromDatabase(hdb, id);

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
               m_policyList.add(curr);
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
                  linkObjects(self(), object);
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
void Template::createExportRecord(TextFileWriter& xml)
{
   xml.appendUtf8String("\t\t<template id=\"");
   xml.append(m_id);
   xml.appendUtf8String("\">\n\t\t\t<guid>");
   xml.append(m_guid);
   xml.appendUtf8String("</guid>\n\t\t\t<name>");
   xml.append(EscapeStringForXML2(m_name));
   xml.appendUtf8String("</name>\n\t\t\t<comments>");
   xml.append(EscapeStringForXML2(m_comments));
   xml.appendUtf8String("</comments>\n");

   // Path in groups
   StringList path;
   unique_ptr<SharedObjectArray<NetObj>> list = getParents(OBJECT_TEMPLATEGROUP);
   TemplateGroup *parent = nullptr;
   while(list->size() > 0)
   {
      parent = static_cast<TemplateGroup*>(list->get(0));
      path.add(parent->getName());
      list = parent->getParents(OBJECT_TEMPLATEGROUP);
   }

   xml.appendUtf8String("\t\t\t<path>\n");
   for(int j = path.size() - 1, id = 1; j >= 0; j--, id++)
   {
      xml.appendUtf8String("\t\t\t\t<element id=\"");
      xml.append(id);
      xml.appendUtf8String("\">");
      xml.append(EscapeStringForXML2(path.get(j)));
      xml.appendUtf8String("</element>\n");
   }
   xml.appendUtf8String("\t\t\t</path>\n\t\t\t<dataCollection>\n");

   readLockDciAccess();
   for(int i = 0; i < m_dcObjects.size(); i++)
      m_dcObjects.get(i)->createExportRecord(xml);
   unlockDciAccess();

   xml.appendUtf8String("\t\t\t</dataCollection>\n\t\t\t<agentPolicies>\n");

   lockProperties();
   for (int i = 0; i < m_policyList.size(); i++)
   {
      m_policyList.get(i)->createExportRecord(xml, i + 1);
   }
   unlockProperties();

   xml.appendUtf8String("\t\t\t</agentPolicies>\n");

   AutoBindTarget::createExportRecord(xml);
   xml.appendUtf8String("\t\t</template>\n");
}

/**
 * Get list of events used by DCIs and policies
 */
HashSet<uint32_t> *Template::getRelatedEventsList() const
{
   HashSet<uint32_t> *eventList = super::getRelatedEventsList();

   lockProperties();
   for (int i = 0; i < m_policyList.size(); i++)
   {
      m_policyList.get(i)->getEventList(eventList);
   }
   unlockProperties();

   return eventList;
}

/**
 * Apply template to data collection target
 */
bool Template::applyToTarget(const shared_ptr<DataCollectionTarget>& target)
{
   // Print in log that policies will be installed
   nxlog_debug_tag(DEBUG_TAG_DC_TEMPLATES, 4, _T("Apply %d policy items from template \"%s\" to target \"%s\""),
         m_policyList.size(), m_name, target->getName());

   if (target->getObjectClass() == OBJECT_NODE)
      forceDeployPolicies(static_pointer_cast<Node>(target));

   return super::applyToTarget(target);
}

/**
 * Initiate forced policy installation
 */
void Template::forceDeployPolicies(const shared_ptr<Node>& target)
{
   TCHAR key[64];
   _sntprintf(key, 64, _T("AgentPolicyDeployment_%u"), target->getId());

   lockProperties();
   for (int i = 0; i < m_policyList.size(); i++)
   {
      const shared_ptr<GenericAgentPolicy>& policy = m_policyList.getShared(i);
      auto data = make_shared<AgentPolicyDeploymentData>(target, target->isNewPolicyTypeFormatSupported());
      data->forceInstall = true;
      _sntprintf(data->debugId, 256, _T("%s [%u] from %s/%s"), target->getName(), target->getId(), getName(), policy->getName());
      ThreadPoolExecuteSerialized(g_mainThreadPool, key, policy, &GenericAgentPolicy::deploy, data);
   }
   setModified(MODIFY_POLICY, false);
   unlockProperties();
}

/**
 * Unlock data collection items list
 */
void Template::applyDCIChanges(bool forcedChange)
{
   lockProperties();
   if (m_dciListModified || forcedChange)
   {
      updateVersion();
      setModified(MODIFY_OTHER);
   }
   unlockProperties();
   super::applyDCIChanges(forcedChange);
}

/**
 * Create NXCP message with object's data
 */
void Template::fillMessageLocked(NXCPMessage *msg, uint32_t userId)
{
   super::fillMessageLocked(msg, userId);
   VersionableObject::fillMessage(msg);
   msg->setField(VID_POLICY_COUNT, m_policyList.size());
}

/**
 * Create NXCP message with object's data
 */
void Template::fillMessageUnlocked(NXCPMessage *msg, uint32_t userId)
{
   super::fillMessageUnlocked(msg, userId);
   AutoBindTarget::fillMessage(msg);
}

/**
 * Modify template object from NXCP message
 */
uint32_t Template::modifyFromMessageInternal(const NXCPMessage& msg, ClientSession *session)
{
   AutoBindTarget::modifyFromMessage(msg);
   return super::modifyFromMessageInternal(msg, session);
}

/**
 * Update object from imported configuration
 */
void Template::updateFromImport(ConfigEntry *config, ImportContext *context, bool nxslV5)
{
   super::updateFromImport(config, context, nxslV5);

   // Templates exported by older server version will have auto bind flags in separate field
   // In that case, bit 0 in flags will be set to 1
   uint32_t flags = config->getSubEntryValueAsUInt(_T("flags"), 0, 0);
   AutoBindTarget::updateFromImport(*config, (flags & 1) != 0, nxslV5);

   VersionableObject::updateFromImport(config);

   lockProperties();

   m_policyList.clear();
   ConfigEntry *policyRoot = config->findEntry(_T("agentPolicies"));
   if (policyRoot != nullptr)
   {
      unique_ptr<ObjectArray<ConfigEntry>> policies = policyRoot->getSubEntries(_T("agentPolicy#*"));
      for(int i = 0; i < policies->size(); i++)
      {
         ConfigEntry *e = policies->get(i);
         uuid guid = e->getSubEntryValueAsUUID(_T("guid"));
         GenericAgentPolicy *curr = CreatePolicy(guid, e->getSubEntryValue(_T("type"), 0, _T("Unknown")), m_id);
         curr->updateFromImport(e, context);
         for (int i = 0; i < m_policyList.size(); i++)
         {
            if (m_policyList.get(i)->getGuid().equals(guid))
               break;
         }
         if (i != m_policyList.size())
            m_policyList.replace(i, curr);
         else
            m_policyList.add(curr);
      }
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
   for (int i = 0; i < m_policyList.size(); i++)
   {
      GenericAgentPolicy *object = m_policyList.get(i);
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
   TCHAR key[64];
   _sntprintf(key, 64, _T("AgentPolicyDeployment_%u"), node->getId());

   for (int i = 0; i < m_policyList.size(); i++)
   {
      GenericAgentPolicy *policy = m_policyList.get(i);
      auto data = make_shared<AgentPolicyRemovalData>(node->self(), policy->getGuid(), policy->getType(), node->isNewPolicyTypeFormatSupported());
      _sntprintf(data->debugId, 256, _T("%s [%u] from %s/%s"), node->getName(), node->getId(), getName(), policy->getName());
      ThreadPoolExecuteSerialized(g_mainThreadPool, key, RemoveAgentPolicy, data);
   }
}

/**
 * Collects information about all agent policies that are using specified event
 */
void Template::getEventReferences(uint32_t eventCode, ObjectArray<EventReference>* eventReferences) const
{
   DataCollectionOwner::getEventReferences(eventCode, eventReferences);

   lockProperties();
   for (int i = 0; i < m_policyList.size(); i++)
   {
      if (m_policyList.get(i)->isUsingEvent(eventCode))
      {
         eventReferences->add(new EventReference(
            EventReferenceType::AGENT_POLICY,
            0,
            m_policyList.get(i)->getGuid(),
            m_policyList.get(i)->getName(),
            m_id,
            m_guid,
            m_name));
      }
   }
   unlockProperties();
}

/**
 * Check policy exist
 */
bool Template::hasPolicy(const uuid& guid) const
{
   lockProperties();
   bool hasPolicy = false;
   for (int i = 0; i < m_policyList.size(); i++)
   {
      if (m_policyList.get(i)->getGuid().equals(guid))
      {
         hasPolicy = true;
         break;
      }
   }
   unlockProperties();
   return hasPolicy;
}

/**
 * Fill given NXCP message with details for specific policy
 */
bool Template::fillPolicyDetailsMessage(NXCPMessage *msg, const uuid& guid) const
{
   bool hasPolicy = false;
   lockProperties();
   for (int i = 0; i < m_policyList.size(); i++)
   {
      if(m_policyList.get(i)->getGuid().equals(guid))
      {
         m_policyList.get(i)->fillMessage(msg, VID_AGENT_POLICY_BASE);
         hasPolicy = true;
         break;
      }
   }
   unlockProperties();
   return hasPolicy;
}

/**
 * Fill NXCP message with data for all policies
 */
void Template::fillPolicyListMessage(NXCPMessage *msg) const
{
   lockProperties();
   uint32_t fieldId = VID_AGENT_POLICY_BASE;
   int count = 0;
   for (int i = 0; i < m_policyList.size(); i++)
   {
      GenericAgentPolicy *object = m_policyList.get(i);
      object->fillMessage(msg, fieldId);
      fieldId += 100;
      count++;
   }
   msg->setField(VID_POLICY_COUNT, count);
   unlockProperties();
}

/**
 * Update policy if GUID is provided and create policy if GUID is nullptr
 */
uuid Template::updatePolicyFromMessage(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_UPDATE_AGENT_POLICY, 0);
   bool updated = false;

   lockProperties();

   uuid guid;
   if (request.isFieldExist(VID_GUID))
   {
      guid = request.getFieldAsGUID(VID_GUID);
      GenericAgentPolicy *policy = nullptr;
      for (int i = 0; i < m_policyList.size(); i++)
      {
         if (m_policyList.get(i)->getGuid().equals(guid))
         {
            policy = m_policyList.get(i);
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
      GenericAgentPolicy *curr = CreatePolicy(request.getFieldAsString(VID_NAME, name, MAX_DB_STRING), request.getFieldAsString(VID_POLICY_TYPE, policyType, 32), m_id);
      curr->modifyFromMessage(request);
      m_policyList.add(curr);
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

   if (updated)
   {
      msg.setField(VID_TEMPLATE_ID, m_id);
      NotifyClientsOnPolicyUpdate(msg, *this);
   }
   return guid;
}

/**
 * Remove policy by uuid
 */
bool Template::removePolicy(const uuid& guid)
{
   bool success = false;
   shared_ptr<GenericAgentPolicy> policy;

   lockProperties();
   for (int i = 0; i < m_policyList.size(); i++)
   {
      if (m_policyList.get(i)->getGuid().equals(guid))
      {
         policy = m_policyList.getShared(i);
         m_deletedPolicyList.add(policy);
         m_policyList.remove(i);
         break;
      }
   }
   unlockProperties();

   if (policy != nullptr)
   {
      readLockChildList();
      for(int i = 0; i < getChildList().size(); i++)
      {
         shared_ptr<NetObj> object = getChildList().getShared(i);
         if (object->getObjectClass() == OBJECT_NODE)
         {
            auto data = make_shared<AgentPolicyRemovalData>(static_pointer_cast<Node>(object), policy->getGuid(), policy->getType(), static_cast<Node*>(object.get())->isNewPolicyTypeFormatSupported());
            _sntprintf(data->debugId, 256, _T("%s [%u] from %s/%s"), object->getName(), object->getId(), getName(), policy->getName());
            TCHAR key[64];
            _sntprintf(key, 64, _T("AgentPolicyDeployment_%u"), object->getId());
            ThreadPoolExecute(g_pollerThreadPool, RemoveAgentPolicy, data);
         }
      }
      unlockChildList();

      updateVersion();

      NotifyClientsOnPolicyDelete(guid, *this);
      setModified(MODIFY_POLICY, false);

      success = true;
   }

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
         for (int i = 0; i < m_policyList.size(); i++)
         {
            const shared_ptr<GenericAgentPolicy>& policy = m_policyList.getShared(i);
            auto data = make_shared<AgentPolicyDeploymentData>(node, node->isNewPolicyTypeFormatSupported());
            data->forceInstall = true;
            _sntprintf(data->debugId, 256, _T("%s [%u] from %s/%s"), node->getName(), node->getId(), getName(), policy->getName());

            TCHAR key[64];
            _sntprintf(key, 64, _T("AgentPolicyDeployment_%u"), node->getId());
            ThreadPoolExecuteSerialized(g_mainThreadPool, key, policy, &GenericAgentPolicy::deploy, data);
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
   if (node->getAgentConnection() == nullptr)
      return;

   TCHAR key[64];
   _sntprintf(key, 64, _T("AgentPolicyDeployment_%u"), node->getId());

   lockProperties();
   for (int i = 0; i < m_policyList.size(); i++)
   {
      const shared_ptr<GenericAgentPolicy>& policy = m_policyList.getShared(i);
      auto data = make_shared<AgentPolicyDeploymentData>(node, node->isNewPolicyTypeFormatSupported());

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

      node->sendPollerMsg(_T("      Scheduling deployment policy %s from template %s\r\n"), policy->getName(), m_name);
      _sntprintf(data->debugId, 256, _T("%s [%u] from %s/%s"), node->getName(), node->getId(), m_name, policy->getName());
      ThreadPoolExecuteSerialized(g_pollerThreadPool, key, policy, &GenericAgentPolicy::deploy, data);
   }
   unlockProperties();
}

/**
 * Initiate validation of policies within this template
 */
void Template::initiatePolicyValidation()
{
   lockProperties();
   for (int i = 0; i < m_policyList.size(); i++)
   {
      ThreadPoolExecute(g_mainThreadPool, m_policyList.getShared(i), &GenericAgentPolicy::validate);
   }
   unlockProperties();
}

/**
 * Create NXSL object for this object
 */
NXSL_Value *Template::createNXSLObject(NXSL_VM *vm)
{
   return vm->createValue(vm->createObject(&g_nxslTemplateClass, new shared_ptr<Template>(self())));
}

/**
 * Perform automatic object binding
 */
void Template::autobindPoll(PollerInfo *poller, ClientSession *session, uint32_t rqId)
{
   poller->setStatus(_T("wait for lock"));
   pollerLock(autobind);

   if (IsShutdownInProgress())
   {
      pollerUnlock();
      return;
   }

   m_pollRequestor = session;
   m_pollRequestId = rqId;
   nxlog_debug_tag(DEBUG_TAG_AUTOBIND_POLL, 5, _T("Starting autobind poll of template %s [%u]"), m_name, m_id);
   poller->setStatus(_T("checking objects"));

   if (!isAutoBindEnabled())
   {
      sendPollerMsg(_T("Automatic object binding is disabled\r\n"));
      nxlog_debug_tag(DEBUG_TAG_AUTOBIND_POLL, 5, _T("Finished autobind poll of template %s [%u])"), m_name, m_id);
      pollerUnlock();
      return;
   }

   NXSL_VM *cachedFilterVM = nullptr;
   unique_ptr<SharedObjectArray<NetObj>> objects = getObjectsForAutoBind(_T("TemplateAutoApply"));
   for (int i = 0; i < objects->size(); i++)
   {
      shared_ptr<NetObj> object = objects->getShared(i);

      AutoBindDecision decision = isApplicable(&cachedFilterVM, object, this);
      if ((decision == AutoBindDecision_Ignore) || ((decision == AutoBindDecision_Unbind) && !isAutoUnbindEnabled()))
         continue;   // Decision cannot affect checks

      if (decision == AutoBindDecision_Bind)
      {
         TCHAR key[64];
         _sntprintf(key, 64, _T("Delete.Template.%u.NetObj.%u"), m_id, object->getId());
         if (DeleteScheduledTasksByKey(key) > 0)
         {
            sendPollerMsg(_T("   Pending removal from \"%s\" cancelled\r\n"), object->getName());
            nxlog_debug_tag(DEBUG_TAG_AUTOBIND_POLL, 4, _T("Template::autobindPoll(): pending removal of template \"%s\" [%u] from object \"%s\" [%u] cancelled"),
                  m_name, m_id, object->getName(), object->getId());
         }

         if (!isDirectChild(object->getId()))
         {
            sendPollerMsg(_T("   Applying to \"%s\"\r\n"), object->getName());
            nxlog_debug_tag(DEBUG_TAG_AUTOBIND_POLL, 4, _T("Template::autobindPoll(): binding object \"%s\" [%u] to template \"%s\" [%u]"), object->getName(), object->getId(), m_name, m_id);
            applyToTarget(static_pointer_cast<DataCollectionTarget>(object));
            EventBuilder(EVENT_TEMPLATE_AUTOAPPLY, g_dwMgmtNode)
               .param(_T("nodeId"), object->getId(), EventBuilder::OBJECT_ID_FORMAT)
               .param(_T("nodeName"), object->getName())
               .param(_T("templateId"), m_id, EventBuilder::OBJECT_ID_FORMAT)
               .param(_T("templateName"), m_name)
               .post();
         }
      }
      else if ((decision == AutoBindDecision_Unbind) && isDirectChild(object->getId()))
      {
         static_cast<DataCollectionTarget&>(*object).scheduleTemplateRemoval(this, this);
      }
   }
   delete cachedFilterVM;

   pollerUnlock();
   nxlog_debug_tag(DEBUG_TAG_AUTOBIND_POLL, 5, _T("Finished autobind poll of template \"%s\" [%u])"), m_name, m_id);
}
