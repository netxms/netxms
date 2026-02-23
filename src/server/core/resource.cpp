/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Raden Solutions
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
** File: resource.cpp
**/

#include "nxcore.h"
#include <cloud-connector.h>

/**
 * Default constructor for Resource class
 */
Resource::Resource() : super(Pollable::STATUS)
{
   m_parentId = 0;
   m_state = 0;
   m_linkedNodeId = 0;
   m_tags = nullptr;
   m_lastDiscoveryTime = 0;
   m_status = STATUS_NORMAL;
}

/**
 * Resource destructor
 */
Resource::~Resource()
{
   delete m_tags;
}

/**
 * Load from database
 */
bool Resource::loadFromDatabase(DB_HANDLE hdb, uint32_t id, DB_STATEMENT *preparedStatements)
{
   m_id = id;

   if (!loadCommonProperties(hdb, preparedStatements) || !super::loadFromDatabase(hdb, id, preparedStatements))
      return false;

   Pollable::loadFromDatabase(hdb, m_id);

   DB_STATEMENT hStmt = DBPrepare(hdb, L"SELECT parent_id,cloud_resource_id,connector_name,resource_type,region,state,provider_state,linked_node_id,account_id,last_discovery_time,connector_data FROM resources WHERE id=?");
   if (hStmt == nullptr)
      return false;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
   DB_RESULT hResult = DBSelectPrepared(hStmt);
   DBFreeStatement(hStmt);
   if (hResult == nullptr)
      return false;

   char buffer[256];

   m_parentId = DBGetFieldULong(hResult, 0, 0);
   m_cloudResourceId = DBGetFieldAsSharedString(hResult, 0, 1);
   m_connectorName = DBGetFieldAsSharedString(hResult, 0, 2);
   m_resourceType = DBGetFieldAsSharedString(hResult, 0, 3);
   m_region = DBGetFieldAsSharedString(hResult, 0, 4);
   m_state = static_cast<int16_t>(DBGetFieldLong(hResult, 0, 5));
   m_providerState = DBGetFieldUTF8(hResult, 0, 6, buffer, 256);
   m_linkedNodeId = DBGetFieldULong(hResult, 0, 7);
   m_accountId = DBGetFieldAsSharedString(hResult, 0, 8);
   m_lastDiscoveryTime = DBGetFieldULong(hResult, 0, 9);
   m_connectorData = DBGetFieldAsSharedString(hResult, 0, 10);
   DBFreeResult(hResult);

   // Load tags
   hStmt = DBPrepare(hdb, L"SELECT tag_key,tag_value FROM resource_tags WHERE resource_id=?");
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
      hResult = DBSelectPrepared(hStmt);
      DBFreeStatement(hStmt);
      if (hResult != nullptr)
      {
         int count = DBGetNumRows(hResult);
         if (count > 0)
         {
            m_tags = new StringMap();
            for (int i = 0; i < count; i++)
            {
               m_tags->set(DBGetFieldAsSharedString(hResult, i, 0), DBGetFieldAsSharedString(hResult, i, 1));
            }
         }
         DBFreeResult(hResult);
      }
   }

   // Load DCI and access list
   loadACLFromDB(hdb, preparedStatements);
   loadItemsFromDB(hdb, preparedStatements);
   for(int i = 0; i < m_dcObjects.size(); i++)
      if (!m_dcObjects.get(i)->loadThresholdsFromDB(hdb, preparedStatements))
         return false;
   loadDCIListForCleanup(hdb);

   return true;
}

/**
 * Save to database
 */
bool Resource::saveToDatabase(DB_HANDLE hdb)
{
   bool success = super::saveToDatabase(hdb);

   if (success && (m_modified & MODIFY_RESOURCE_PROPERTIES))
   {
      static const TCHAR *columns[] = {
         L"parent_id", L"cloud_resource_id", L"connector_name", L"resource_type", L"region",
         L"state", L"provider_state", L"linked_node_id", L"account_id",
         L"last_discovery_time", L"connector_data", nullptr
      };
      DB_STATEMENT hStmt = DBPrepareMerge(hdb, L"resources", L"id", m_id, columns);
      if (hStmt != nullptr)
      {
         lockProperties();
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_parentId);
         DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, m_cloudResourceId, DB_BIND_TRANSIENT);
         DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, m_connectorName, DB_BIND_TRANSIENT);
         DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, m_resourceType, DB_BIND_TRANSIENT);
         DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, m_region, DB_BIND_TRANSIENT);
         DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, static_cast<int32_t>(m_state));
         DBBind(hStmt, 7, DB_CTYPE_UTF8_STRING, DB_SQLTYPE_VARCHAR, m_providerState.c_str(), DB_BIND_STATIC);
         DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, m_linkedNodeId);
         DBBind(hStmt, 9, DB_SQLTYPE_VARCHAR, m_accountId, DB_BIND_TRANSIENT);
         DBBind(hStmt, 10, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_lastDiscoveryTime));
         DBBind(hStmt, 11, DB_SQLTYPE_TEXT, m_connectorData, DB_BIND_TRANSIENT);
         DBBind(hStmt, 12, DB_SQLTYPE_INTEGER, m_id);
         unlockProperties();

         success = DBExecute(hStmt);
         DBFreeStatement(hStmt);
      }
      else
      {
         success = false;
      }

      // Save tags
      if (success)
      {
         success = executeQueryOnObject(hdb, L"DELETE FROM resource_tags WHERE resource_id=?");
         if (success && (m_tags != nullptr) && (m_tags->size() > 0))
         {
            hStmt = DBPrepare(hdb, L"INSERT INTO resource_tags (resource_id,tag_key,tag_value) VALUES (?,?,?)", m_tags->size() > 1);
            if (hStmt != nullptr)
            {
               lockProperties();
               for(auto it : *m_tags)
               {
                  DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
                  DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, it->key, DB_BIND_STATIC);
                  DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, it->value, DB_BIND_STATIC);
                  if (!DBExecute(hStmt))
                  {
                     success = false;
                     break;
                  }
               }
               unlockProperties();
               DBFreeStatement(hStmt);
            }
            else
            {
               success = false;
            }
         }
      }
   }
   return success;
}

/**
 * Delete from database
 */
bool Resource::deleteFromDatabase(DB_HANDLE hdb)
{
   bool success = super::deleteFromDatabase(hdb);
   if (success)
      success = executeQueryOnObject(hdb, L"DELETE FROM resource_tags WHERE resource_id=?");
   if (success)
      success = executeQueryOnObject(hdb, L"DELETE FROM resources WHERE id=?");
   return success;
}

/**
 * Create NXSL object for this object
 */
NXSL_Value *Resource::createNXSLObject(NXSL_VM *vm)
{
   return vm->createValue(vm->createObject(&g_nxslResourceClass, new shared_ptr<Resource>(self())));
}

/**
 * Serialize to JSON
 */
json_t *Resource::toJson()
{
   json_t *root = super::toJson();

   lockProperties();
   json_object_set_new(root, "resourceId", json_string_t(m_cloudResourceId));
   json_object_set_new(root, "connectorName", json_string_t(m_connectorName));
   json_object_set_new(root, "resourceType", json_string_t(m_resourceType));
   json_object_set_new(root, "region", json_string_t(m_region));
   json_object_set_new(root, "state", json_integer(m_state));
   json_object_set_new(root, "providerState", json_string(m_providerState.c_str()));
   json_object_set_new(root, "linkedNodeId", json_integer(m_linkedNodeId));
   json_object_set_new(root, "accountId", json_string_t(m_accountId));
   json_object_set_new(root, "lastDiscoveryTime", json_integer(m_lastDiscoveryTime));
   if (m_tags != nullptr)
      json_object_set_new(root, "tags", m_tags->toJson());
   unlockProperties();

   return root;
}

/**
 * Fill NXCP message
 */
void Resource::fillMessageLocked(NXCPMessage *msg, uint32_t userId)
{
   super::fillMessageLocked(msg, userId);
   msg->setField(VID_CLOUD_RESOURCE_ID, m_cloudResourceId);
   msg->setField(VID_CONNECTOR_NAME, m_connectorName);
   msg->setField(VID_RESOURCE_TYPE, m_resourceType);
   msg->setField(VID_CLOUD_REGION, m_region);
   msg->setField(VID_RESOURCE_STATE, m_state);
   msg->setFieldFromUtf8String(VID_PROVIDER_STATE, m_providerState.c_str());
   msg->setField(VID_LINKED_NODE_ID, m_linkedNodeId);
   msg->setField(VID_ACCOUNT_ID, m_accountId);
   msg->setField(VID_LAST_DISCOVERY_TIME, static_cast<uint32_t>(m_lastDiscoveryTime));
   msg->setField(VID_CONNECTOR_DATA, m_connectorData);

   if (m_tags != nullptr)
      m_tags->fillMessage(msg, VID_RESOURCE_TAG_LIST_BASE, VID_NUM_TAGS);
   else
      msg->setField(VID_NUM_TAGS, static_cast<uint32_t>(0));
}

/**
 * Modify object from NXCP message
 */
uint32_t Resource::modifyFromMessageInternal(const NXCPMessage& msg, ClientSession *session)
{
   if (msg.isFieldExist(VID_LINKED_NODE_ID))
      m_linkedNodeId = msg.getFieldAsUInt32(VID_LINKED_NODE_ID);

   return super::modifyFromMessageInternal(msg, session);
}

/**
 * Calculate compound status
 */
void Resource::calculateCompoundStatus(bool forcedRecalc)
{
   int oldStatus = m_status;
   super::calculateCompoundStatus(forcedRecalc);
   if (oldStatus != m_status)
      setModified(MODIFY_RUNTIME);
}

/**
 * Status poll
 */
void Resource::statusPoll(PollerInfo *poller, ClientSession *session, uint32_t rqId)
{
   lockProperties();
   if (m_isDeleteInitiated || IsShutdownInProgress())
   {
      m_statusPollState.complete(0);
      unlockProperties();
      return;
   }
   unlockProperties();

   poller->setStatus(L"wait for lock");
   pollerLock(status);

   if (IsShutdownInProgress())
   {
      pollerUnlock();
      return;
   }

   m_pollRequestor = session;
   m_pollRequestId = rqId;
   sendPollerMsg(L"Starting status poll of resource %s\r\n", m_name);
   nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, L"Starting status poll of resource \"%s\" [%u]", m_name, m_id);

   poller->setStatus(L"query state");
   shared_ptr<CloudDomain> domain = m_ownerDomain.lock();
   if (domain != nullptr)
   {
      SharedString connectorName = domain->getConnectorName();
      CloudConnectorInterface *connector = FindCloudConnector(connectorName);
      if (connector != nullptr)
      {
         json_t *credentials = domain->getCredentials();

         char providerStateBuf[256];
         SharedString resourceId = getCloudResourceId();
         int16_t newState = connector->QueryState(resourceId, providerStateBuf, 256, credentials);
         if (newState >= 0)
         {
            if (m_state != newState || strcmp(m_providerState.c_str(), providerStateBuf))
            {
               updateState(newState, providerStateBuf);
               sendPollerMsg(L"   Resource state changed to %d (%s)\r\n", newState, providerStateBuf);
            }
            else
            {
               sendPollerMsg(L"   Resource state is unchanged (%d, %s)\r\n", newState, providerStateBuf);
            }
         }
         else
         {
            sendPollerMsg(POLLER_WARNING L"   Failed to query resource state\r\n");
            nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, L"Failed to query state for resource \"%s\" [%u]", m_name, m_id);
         }

         json_decref(credentials);
      }
      else
      {
         sendPollerMsg(POLLER_WARNING L"   Cloud connector \"%s\" not found\r\n", connectorName.cstr());
      }
   }
   else
   {
      sendPollerMsg(POLLER_WARNING L"   Owner cloud domain not available\r\n");
      nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, L"Owner cloud domain not available for resource \"%s\" [%u]", m_name, m_id);
   }

   calculateCompoundStatus(true);

   poller->setStatus(L"hook");
   executeHookScript(L"StatusPoll");

   sendPollerMsg(L"Finished status poll of resource %s\r\n", m_name);
   sendPollerMsg(L"Resource status after poll is %s\r\n", GetStatusAsText(m_status, true));

   pollerUnlock();
   nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, L"Finished status poll of resource \"%s\" [%u]", m_name, m_id);
}

/**
 * Get most critical additional status from any object specific function or component.
 */
int Resource::getAdditionalMostCriticalStatus(StringBuffer *explanation)
{
   switch(m_state)
   {
      case RESOURCE_STATE_ACTIVE:
         if (explanation != nullptr)
            explanation->append(L"Resource is active");
         return STATUS_NORMAL;
      case RESOURCE_STATE_CREATING:
         if (explanation != nullptr)
            explanation->append(L"Resource is being created");
         return STATUS_NORMAL;
      case RESOURCE_STATE_DEGRADED:
         if (explanation != nullptr)
            explanation->append(L"Resource is degraded");
         return STATUS_WARNING;
      case RESOURCE_STATE_DELETING:
         if (explanation != nullptr)
            explanation->append(L"Resource is being deleted");
         return STATUS_WARNING;
      case RESOURCE_STATE_FAILED:
         if (explanation != nullptr)
            explanation->append(L"Resource has failed");
         return STATUS_CRITICAL;
      case RESOURCE_STATE_INACTIVE:
         if (explanation != nullptr)
            explanation->append(L"Resource is inactive");
         return STATUS_MINOR;
      case RESOURCE_STATE_UNREACHABLE:
         if (explanation != nullptr)
            explanation->append(L"Resource is unreachable");
         return STATUS_MAJOR;
      default:
         if (explanation != nullptr)
            explanation->append(L"Resource state is unknown");
         return STATUS_UNKNOWN;
   }
}

/**
 * Update resource properties from discovery descriptor
 */
void Resource::updateFromDiscovery(const ResourceDescriptor *desc, uint32_t parentId, const TCHAR *connectorName)
{
   lockProperties();
   m_cloudResourceId = desc->resourceId;
   m_resourceType = desc->type;
   m_region = desc->region;
   m_state = desc->state;
   m_providerState = desc->providerState;
   m_parentId = parentId;
   m_connectorName = connectorName;
   m_lastDiscoveryTime = time(nullptr);

   delete m_tags;
   if (desc->tags != nullptr)
   {
      m_tags = new StringMap(*desc->tags);
   }
   else
   {
      m_tags = nullptr;
   }

   setModified(MODIFY_RESOURCE_PROPERTIES);
   unlockProperties();
}

/**
 * Set linked node ID
 */
void Resource::setLinkedNodeId(uint32_t nodeId)
{
   lockProperties();
   if (m_linkedNodeId != nodeId)
   {
      m_linkedNodeId = nodeId;
      setModified(MODIFY_RESOURCE_PROPERTIES);
   }
   unlockProperties();
}

/**
 * Update resource state
 */
void Resource::updateState(int16_t newState, const char *providerState)
{
   lockProperties();
   if (m_state != newState || strcmp(m_providerState.c_str(), providerState))
   {
      m_state = newState;
      m_providerState = providerState;
      setModified(MODIFY_RESOURCE_PROPERTIES);
   }
   unlockProperties();
}

/**
 * Post-load hook. Link resource to parent object after all objects are loaded.
 */
void Resource::postLoad()
{
   super::postLoad();
   if (m_parentId != 0)
   {
      shared_ptr<NetObj> parent = FindObjectById(m_parentId);
      if (parent != nullptr)
      {
         linkObjects(parent, self());

         // Walk parent chain to find CloudDomain ancestor
         shared_ptr<NetObj> current = parent;
         while (current != nullptr)
         {
            if (current->getObjectClass() == OBJECT_CLOUDDOMAIN)
            {
               m_ownerDomain = static_pointer_cast<CloudDomain>(current);
               break;
            }
            // Move up to next parent (Resource objects have a single logical parent)
            unique_ptr<SharedObjectArray<NetObj>> parents = current->getParents();
            shared_ptr<NetObj> nextParent;
            for (int i = 0; i < parents->size(); i++)
            {
               shared_ptr<NetObj> p = parents->getShared(i);
               if (p->getObjectClass() == OBJECT_CLOUDDOMAIN || p->getObjectClass() == OBJECT_RESOURCE)
               {
                  nextParent = p;
                  break;
               }
            }
            current = nextParent;
         }
      }
      else
      {
         nxlog_write(NXLOG_ERROR, L"Inconsistent database: resource \"%s\" [%u] has reference to non-existing parent object [%u]", m_name, m_id, m_parentId);
      }
   }
}

/**
 * Prepare for deletion
 */
void Resource::prepareForDeletion()
{
   nxlog_debug_tag(DEBUG_TAG_OBJECT_LIFECYCLE, 4, L"Resource::prepareForDeletion(%s [%u]): waiting for outstanding polls to finish", m_name, m_id);
   while (m_statusPollState.isPending())
      ThreadSleepMs(100);
   nxlog_debug_tag(DEBUG_TAG_OBJECT_LIFECYCLE, 4, L"Resource::prepareForDeletion(%s [%u]): no outstanding polls left", m_name, m_id);

   super::prepareForDeletion();
}

/**
 * Get metric value from cloud connector
 */
DataCollectionError Resource::getMetricFromCloudConnector(const wchar_t *metric, wchar_t *buffer, size_t size)
{
   shared_ptr<CloudDomain> domain = m_ownerDomain.lock();
   if (domain == nullptr)
      return DCE_NOT_SUPPORTED;

   SharedString connectorName = domain->getConnectorName();
   CloudConnectorInterface *connector = FindCloudConnector(connectorName);
   if (connector == nullptr)
      return DCE_NOT_SUPPORTED;

   json_t *credentials = domain->getCredentials();
   DataCollectionError rc = connector->Collect(m_cloudResourceId, metric, 0, buffer, size, credentials);
   json_decref(credentials);
   nxlog_debug_tag(L"dc.cloud", 7, _T("Resource(%s)->getMetricFromCloudConnector(%s): result=%d"), m_name, metric, rc);
   return rc;
}
