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
** File: cloud_domain.cpp
**/

#include "nxcore.h"
#include <cloud-connector.h>

#define POLL_CANCELLATION_CHECKPOINT() \
         do { if (g_flags & AF_SHUTDOWN) { pollerUnlock(); return; } } while(0)

/**
 * Debug tag for cloud discovery
 */
#define DEBUG_TAG_CLOUD_DISCOVERY   L"cloud.discovery"

/**
 * Default constructor for CloudDomain class
 */
CloudDomain::CloudDomain() : super(Pollable::STATUS | Pollable::CONFIGURATION)
{
   m_credentials = nullptr;
   m_parsedCredentials = nullptr;
   m_removalPolicy = 0;
   m_gracePeriod = 30;
   m_lastDiscoveryStatus = 0;
   m_lastDiscoveryTime = 0;
   m_status = STATUS_NORMAL;
}

/**
 * Constructor from NXCP message
 */
CloudDomain::CloudDomain(const wchar_t *name, const NXCPMessage& request) : super(name, Pollable::STATUS | Pollable::CONFIGURATION)
{
   m_runtimeFlags |= ODF_CONFIGURATION_POLL_PENDING;
   m_connectorName = request.getFieldAsSharedString(VID_CONNECTOR_NAME);
   m_credentials = request.getFieldAsUtf8String(VID_CLOUD_CREDENTIALS);
   m_parsedCredentials = nullptr;
   parseCredentials();
   m_discoveryFilter = request.getFieldAsSharedString(VID_DISCOVERY_FILTER);
   m_removalPolicy = request.getFieldAsInt16(VID_REMOVAL_POLICY);
   m_gracePeriod = request.getFieldAsUInt32(VID_GRACE_PERIOD);
   m_lastDiscoveryStatus = 0;
   m_lastDiscoveryTime = 0;
   m_status = STATUS_NORMAL;
}

/**
 * Parse credentials JSON string and cache the result
 */
void CloudDomain::parseCredentials()
{
   json_decref(m_parsedCredentials);
   m_parsedCredentials = nullptr;
   if (m_credentials != nullptr)
   {
      json_error_t error;
      m_parsedCredentials = json_loads(m_credentials, 0, &error);
      if (m_parsedCredentials == nullptr)
         nxlog_write_tag(NXLOG_WARNING, L"cloud", L"CloudDomain(%s [%u]): failed to parse credentials JSON on line %d: %hs", m_name, m_id, error.line, error.text);
   }
}

/**
 * CloudDomain destructor
 */
CloudDomain::~CloudDomain()
{
   MemFree(m_credentials);
   json_decref(m_parsedCredentials);
}

/**
 * Load from database
 */
bool CloudDomain::loadFromDatabase(DB_HANDLE hdb, uint32_t id, DB_STATEMENT *preparedStatements)
{
   m_id = id;

   if (!loadCommonProperties(hdb, preparedStatements) || !super::loadFromDatabase(hdb, id, preparedStatements))
      return false;

   if (Pollable::loadFromDatabase(hdb, m_id))
   {
      if (static_cast<uint32_t>(time(nullptr) - m_configurationPollState.getLastCompleted()) < getCustomAttributeAsUInt32(L"SysConfig:Objects.ConfigurationPollingInterval", g_configurationPollingInterval))
         m_runtimeFlags |= ODF_CONFIGURATION_POLL_PASSED;
   }

   DB_STATEMENT hStmt = DBPrepare(hdb, L"SELECT connector_name,credentials,discovery_filter,removal_policy,grace_period,last_discovery_status,last_discovery_time,last_discovery_msg FROM cloud_domains WHERE id=?");
   if (hStmt == nullptr)
      return false;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
   DB_RESULT hResult = DBSelectPrepared(hStmt);
   DBFreeStatement(hStmt);
   if (hResult == nullptr)
      return false;

   m_connectorName = DBGetFieldAsSharedString(hResult, 0, 0);
   m_credentials = DBGetFieldUTF8(hResult, 0, 1, nullptr, 0);
   parseCredentials();
   m_discoveryFilter = DBGetFieldAsSharedString(hResult, 0, 2);
   m_removalPolicy = static_cast<int16_t>(DBGetFieldLong(hResult, 0, 3));
   m_gracePeriod = DBGetFieldULong(hResult, 0, 4);
   m_lastDiscoveryStatus = static_cast<int16_t>(DBGetFieldLong(hResult, 0, 5));
   m_lastDiscoveryTime = DBGetFieldULong(hResult, 0, 6);
   char *lastDiscoveryMsg = DBGetFieldUTF8(hResult, 0, 7, nullptr, 0);
   m_lastDiscoveryMessage = CHECK_NULL_EX_A(lastDiscoveryMsg);
   MemFree(lastDiscoveryMsg);
   DBFreeResult(hResult);

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
bool CloudDomain::saveToDatabase(DB_HANDLE hdb)
{
   bool success = super::saveToDatabase(hdb);

   if (success && (m_modified & MODIFY_CLOUD_DOMAIN_PROPERTIES))
   {
      static const TCHAR *columns[] = {
         L"connector_name", L"credentials",
         L"discovery_filter", L"removal_policy", L"grace_period",
         L"last_discovery_status", L"last_discovery_time", L"last_discovery_msg", nullptr
      };
      DB_STATEMENT hStmt = DBPrepareMerge(hdb, L"cloud_domains", L"id", m_id, columns);
      if (hStmt != nullptr)
      {
         lockProperties();
         DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, m_connectorName, DB_BIND_TRANSIENT);
         DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, DB_CTYPE_UTF8_STRING, m_credentials, DB_BIND_STATIC);
         DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, m_discoveryFilter, DB_BIND_TRANSIENT);
         DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, static_cast<int32_t>(m_removalPolicy));
         DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, m_gracePeriod);
         DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, static_cast<int32_t>(m_lastDiscoveryStatus));
         DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_lastDiscoveryTime));
         DBBind(hStmt, 8, DB_SQLTYPE_VARCHAR, DB_CTYPE_UTF8_STRING, m_lastDiscoveryMessage.c_str(), DB_BIND_STATIC);
         DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, m_id);
         unlockProperties();

         success = DBExecute(hStmt);
         DBFreeStatement(hStmt);
      }
      else
      {
         success = false;
      }
   }
   return success;
}

/**
 * Delete from database
 */
bool CloudDomain::deleteFromDatabase(DB_HANDLE hdb)
{
   bool success = super::deleteFromDatabase(hdb);
   if (success)
      success = executeQueryOnObject(hdb, L"DELETE FROM cloud_domains WHERE id=?");
   return success;
}

/**
 * Create NXSL object for this object
 */
NXSL_Value *CloudDomain::createNXSLObject(NXSL_VM *vm)
{
   return vm->createValue(vm->createObject(&g_nxslCloudDomainClass, new shared_ptr<CloudDomain>(self())));
}

/**
 * Serialize to JSON
 */
json_t *CloudDomain::toJson()
{
   json_t *root = super::toJson();

   lockProperties();
   json_object_set_new(root, "connectorName", json_string_t(m_connectorName));
   json_object_set_new(root, "removalPolicy", json_integer(m_removalPolicy));
   json_object_set_new(root, "gracePeriod", json_integer(m_gracePeriod));
   json_object_set_new(root, "lastDiscoveryStatus", json_integer(m_lastDiscoveryStatus));
   json_object_set_new(root, "lastDiscoveryTime", json_integer(m_lastDiscoveryTime));
   json_object_set_new(root, "lastDiscoveryMessage", json_string(m_lastDiscoveryMessage.c_str()));
   unlockProperties();

   return root;
}

/**
 * Fill NXCP message
 */
void CloudDomain::fillMessageLocked(NXCPMessage *msg, uint32_t userId)
{
   super::fillMessageLocked(msg, userId);
   msg->setField(VID_CONNECTOR_NAME, m_connectorName);
   msg->setField(VID_CLOUD_CREDENTIALS, m_credentials != nullptr);
   msg->setField(VID_DISCOVERY_FILTER, m_discoveryFilter);
   msg->setField(VID_REMOVAL_POLICY, m_removalPolicy);
   msg->setField(VID_GRACE_PERIOD, m_gracePeriod);
   msg->setField(VID_LAST_DISCOVERY_STATUS, m_lastDiscoveryStatus);
   msg->setField(VID_LAST_DISCOVERY_TIME, static_cast<uint32_t>(m_lastDiscoveryTime));
   msg->setFieldFromUtf8String(VID_LAST_DISCOVERY_MESSAGE, m_lastDiscoveryMessage.c_str());
}

/**
 * Modify object from NXCP message
 */
uint32_t CloudDomain::modifyFromMessageInternal(const NXCPMessage& msg, ClientSession *session)
{
   if (msg.isFieldExist(VID_CONNECTOR_NAME))
      m_connectorName = msg.getFieldAsSharedString(VID_CONNECTOR_NAME);
   if (msg.isFieldExist(VID_CLOUD_CREDENTIALS))
   {
      MemFree(m_credentials);
      m_credentials = msg.getFieldAsUtf8String(VID_CLOUD_CREDENTIALS);
      parseCredentials();
   }
   if (msg.isFieldExist(VID_DISCOVERY_FILTER))
      m_discoveryFilter = msg.getFieldAsSharedString(VID_DISCOVERY_FILTER);
   if (msg.isFieldExist(VID_REMOVAL_POLICY))
      m_removalPolicy = msg.getFieldAsInt16(VID_REMOVAL_POLICY);
   if (msg.isFieldExist(VID_GRACE_PERIOD))
      m_gracePeriod = msg.getFieldAsUInt32(VID_GRACE_PERIOD);

   return super::modifyFromMessageInternal(msg, session);
}

/**
 * Calculate compound status
 */
void CloudDomain::calculateCompoundStatus(bool forcedRecalc)
{
   int oldStatus = m_status;
   super::calculateCompoundStatus(forcedRecalc);
   if (oldStatus != m_status)
      setModified(MODIFY_RUNTIME);
}

/**
 * Status poll
 */
void CloudDomain::statusPoll(PollerInfo *poller, ClientSession *session, uint32_t rqId)
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
   sendPollerMsg(L"Starting status poll of cloud domain %s\r\n", m_name);
   nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, L"Starting status poll of cloud domain \"%s\" [%u]", m_name, m_id);

   // Query state for child resources
   poller->setStatus(L"query state");
   SharedString connectorName = getConnectorName();
   CloudConnectorInterface *connector = FindCloudConnector(connectorName);
   if (connector != nullptr)
   {
      json_t *credentials = getCredentials();

      unique_ptr<SharedObjectArray<NetObj>> children = getChildren(OBJECT_RESOURCE);
      nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, L"CloudDomain::statusPoll(%s [%u]): querying state for %d resources", m_name, m_id, children->size());

      for (int i = 0; i < children->size(); i++)
      {
         POLL_CANCELLATION_CHECKPOINT();

         shared_ptr<Resource> resource = static_pointer_cast<Resource>(children->getShared(i));
         SharedString resourceId = resource->getCloudResourceId();

         char providerStateBuf[256];
         int16_t newState = connector->QueryState(resourceId, providerStateBuf, 256, credentials);
         if (newState >= 0)
         {
            if (resource->getResourceState() != newState || strcmp(resource->getProviderState().c_str(), providerStateBuf))
            {
               resource->updateState(newState, providerStateBuf);
               sendPollerMsg(L"   Resource \"%s\" state changed to %d (%s)\r\n", resource->getName(), newState, providerStateBuf);
            }
         }
         else
         {
            sendPollerMsg(POLLER_WARNING L"   Failed to query state for resource \"%s\"\r\n", resource->getName());
            nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, L"Failed to query state for resource \"%s\" [%u]", resource->getName(), resource->getId());
         }
      }

      json_decref(credentials);
   }
   else
   {
      if (!connectorName.isEmpty())
      {
         sendPollerMsg(POLLER_WARNING L"   Cloud connector \"%s\" not found\r\n", static_cast<const TCHAR*>(connectorName));
         nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, L"Cloud connector \"%s\" not found for cloud domain \"%s\" [%u]", static_cast<const TCHAR*>(connectorName), m_name, m_id);
      }
   }

   calculateCompoundStatus(true);

   poller->setStatus(L"hook");
   executeHookScript(L"StatusPoll");

   sendPollerMsg(L"Finished status poll of cloud domain %s\r\n", m_name);
   sendPollerMsg(L"Cloud domain status after poll is %s\r\n", GetStatusAsText(m_status, true));

   pollerUnlock();
   nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, L"Finished status poll of cloud domain \"%s\" [%u]", m_name, m_id);
}

/**
 * Configuration poll
 */
void CloudDomain::configurationPoll(PollerInfo *poller, ClientSession *session, uint32_t rqId)
{
   lockProperties();
   if (m_isDeleteInitiated || IsShutdownInProgress())
   {
      m_configurationPollState.complete(0);
      unlockProperties();
      return;
   }
   unlockProperties();

   poller->setStatus(L"wait for lock");
   pollerLock(configuration);

   if (IsShutdownInProgress())
   {
      pollerUnlock();
      return;
   }

   m_pollRequestor = session;
   m_pollRequestId = rqId;
   nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, L"Starting configuration poll of cloud domain \"%s\" [%u]", m_name, m_id);
   sendPollerMsg(L"Starting configuration poll of cloud domain %s\r\n", m_name);

   // Find cloud connector
   poller->setStatus(L"discover");
   SharedString connectorName = getConnectorName();
   CloudConnectorInterface *connector = FindCloudConnector(connectorName);
   if (connector != nullptr)
   {
      sendPollerMsg(L"   Using cloud connector \"%s\"\r\n", static_cast<const TCHAR*>(connectorName));

      // Call connector discover
      json_t *credentials = getCredentials();
      SharedString filter = GetAttributeWithLock(m_discoveryFilter, m_mutexProperties);
      ResourceDescriptor *discoveredTree = connector->Discover(credentials, filter);
      json_decref(credentials);
      if (discoveredTree != nullptr)
      {
         sendPollerMsg(L"   Discovery successful, reconciling resources\r\n");
         nxlog_debug_tag(DEBUG_TAG_CLOUD_DISCOVERY, 5, L"Discovery returned results for cloud domain \"%s\" [%u]", m_name, m_id);

         // Collect all discovered cloud resource IDs into a set for tracking
         StringSet discoveredIds;
         std::function<void(const ResourceDescriptor *, uint32_t)> reconcileResources;

         reconcileResources = [this, &connector, &discoveredIds, &reconcileResources](const ResourceDescriptor *desc, uint32_t parentObjId) {
            for (const ResourceDescriptor *d = desc; d != nullptr; d = d->next)
            {
               if (IsShutdownInProgress())
                  return;

               // Find existing resource child by cloud resource ID
               shared_ptr<Resource> resource;
               shared_ptr<NetObj> parentObj = FindObjectById(parentObjId);
               if (parentObj == nullptr)
                  return;
               unique_ptr<SharedObjectArray<NetObj>> children = parentObj->getChildren(OBJECT_RESOURCE);
               WCHAR wResourceId[1024];
               utf8_to_wchar(d->resourceId, -1, wResourceId, 1024);
               for (int i = 0; i < children->size(); i++)
               {
                  shared_ptr<Resource> r = static_pointer_cast<Resource>(children->getShared(i));
                  if (!wcscmp(r->getCloudResourceId(), wResourceId))
                  {
                     resource = r;
                     break;
                  }
               }

               if (resource != nullptr)
               {
                  // Update existing resource
                  resource->updateFromDiscovery(d, parentObjId);
                  resource->setName(WideStringFromUTF8String(d->name));
                  nxlog_debug_tag(DEBUG_TAG_CLOUD_DISCOVERY, 6, L"Updated existing resource \"%s\" [%u] (cloud ID: %hs)", resource->getName(), resource->getId(), d->resourceId);
               }
               else
               {
                  // Create new resource
                  resource = make_shared<Resource>();
                  resource->setName(WideStringFromUTF8String(d->name));
                  NetObjInsert(resource, true, false);
                  resource->updateFromDiscovery(d, parentObjId);
                  resource->setOwnerDomain(self());

                  // Link to parent
                  linkObjects(parentObj, resource);
                  resource->publish();

                  sendPollerMsg(L"   Created new resource \"%hs\" (cloud ID: %hs)\r\n", d->name, d->resourceId);
                  nxlog_debug_tag(DEBUG_TAG_CLOUD_DISCOVERY, 5, L"Created new resource \"%hs\" [%u] (cloud ID: %hs) under parent [%u]",
                     d->name, resource->getId(), d->resourceId, parentObjId);
               }

               // Node linking via linkHint
               if (d->linkHint[0] != 0)
               {
                  shared_ptr<Node> linkedNode;
                  WCHAR wLinkHint[256];
                  utf8_to_wchar(d->linkHint, -1, wLinkHint, 256);
                  InetAddress addr = InetAddress::resolveHostName(wLinkHint);
                  if (addr.isValidUnicast())
                  {
                     linkedNode = FindNodeByIP(0, true, addr);
                  }
                  if (linkedNode == nullptr)
                  {
                     unique_ptr<SharedObjectArray<NetObj>> nodes = FindNodesByHostname(0, wLinkHint);
                     if (nodes != nullptr && nodes->size() > 0)
                        linkedNode = static_pointer_cast<Node>(nodes->getShared(0));
                  }
                  if (linkedNode != nullptr)
                  {
                     resource->setLinkedNodeId(linkedNode->getId());
                     nxlog_debug_tag(DEBUG_TAG_CLOUD_DISCOVERY, 6, L"Linked resource \"%s\" [%u] to node \"%s\" [%u]",
                        resource->getName(), resource->getId(), linkedNode->getName(), linkedNode->getId());
                  }
               }

               discoveredIds.add(wResourceId);

               // Recurse for children
               if (d->children != nullptr)
                  reconcileResources(d->children, resource->getId());
            }
         };

         reconcileResources(discoveredTree, m_id);

         POLL_CANCELLATION_CHECKPOINT();

         // Remove resources not found in discovery
         poller->setStatus(L"reconcile");
         lockProperties();
         int16_t removalPolicy = m_removalPolicy;
         uint32_t gracePeriod = m_gracePeriod;
         unlockProperties();

         std::function<void(uint32_t)> removeStaleResources;
         removeStaleResources = [this, &discoveredIds, &removeStaleResources, removalPolicy, gracePeriod](uint32_t parentObjId) {
            shared_ptr<NetObj> parentObj = FindObjectById(parentObjId);
            if (parentObj == nullptr)
               return;

            unique_ptr<SharedObjectArray<NetObj>> children = parentObj->getChildren(OBJECT_RESOURCE);
            for (int i = 0; i < children->size(); i++)
            {
               shared_ptr<Resource> resource = static_pointer_cast<Resource>(children->getShared(i));

               // First recurse into children
               removeStaleResources(resource->getId());

               if (!discoveredIds.contains(resource->getCloudResourceId()))
               {
                  switch (removalPolicy)
                  {
                     case 0: // Mark inactive
                        resource->updateState(RESOURCE_STATE_INACTIVE, "missing");
                        nxlog_debug_tag(DEBUG_TAG_CLOUD_DISCOVERY, 5, L"Marked resource \"%s\" [%u] as inactive (not found in discovery)", resource->getName(), resource->getId());
                        break;
                     case 1: // Delete after grace period
                     {
                        time_t lastSeen = resource->getLastDiscoveryTime();
                        if (lastSeen != 0 && (time(nullptr) - lastSeen > static_cast<time_t>(gracePeriod) * 86400))
                        {
                           nxlog_debug_tag(DEBUG_TAG_CLOUD_DISCOVERY, 5, L"Deleting resource \"%s\" [%u] (grace period expired)", resource->getName(), resource->getId());
                           resource->deleteObject();
                        }
                        else
                        {
                           resource->updateState(RESOURCE_STATE_INACTIVE, "missing");
                           nxlog_debug_tag(DEBUG_TAG_CLOUD_DISCOVERY, 6, L"Marked resource \"%s\" [%u] as inactive (grace period not expired)", resource->getName(), resource->getId());
                        }
                        break;
                     }
                     case 2: // Delete immediately
                        nxlog_debug_tag(DEBUG_TAG_CLOUD_DISCOVERY, 5, L"Deleting resource \"%s\" [%u] (immediate removal policy)", resource->getName(), resource->getId());
                        resource->deleteObject();
                        break;
                     case 3: // Ignore
                        break;
                  }
               }
            }
         };

         removeStaleResources(m_id);

         // Update discovery status - success
         lockProperties();
         m_lastDiscoveryStatus = 0;
         m_lastDiscoveryTime = time(nullptr);
         m_lastDiscoveryMessage = "";
         setModified(MODIFY_CLOUD_DOMAIN_PROPERTIES);
         unlockProperties();

         delete discoveredTree;
      }
      else
      {
         sendPollerMsg(POLLER_ERROR L"   Discovery returned no results\r\n");
         nxlog_debug_tag(DEBUG_TAG_CLOUD_DISCOVERY, 5, L"Discovery returned no results for cloud domain \"%s\" [%u]", m_name, m_id);

         lockProperties();
         m_lastDiscoveryStatus = 2;
         m_lastDiscoveryTime = time(nullptr);
         m_lastDiscoveryMessage = "Discovery returned no results";
         setModified(MODIFY_CLOUD_DOMAIN_PROPERTIES);
         unlockProperties();
      }
   }
   else
   {
      sendPollerMsg(POLLER_ERROR L"   Cloud connector \"%s\" not found\r\n", static_cast<const TCHAR*>(connectorName));
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, L"Cloud connector \"%s\" not found for cloud domain \"%s\" [%u]", static_cast<const TCHAR*>(connectorName), m_name, m_id);

      lockProperties();
      m_lastDiscoveryStatus = 2;
      m_lastDiscoveryTime = time(nullptr);
      m_lastDiscoveryMessage = "Cloud connector not found";
      setModified(MODIFY_CLOUD_DOMAIN_PROPERTIES);
      unlockProperties();
   }

   // Execute hook script
   poller->setStatus(L"hook");
   executeHookScript(L"ConfigurationPoll");

   sendPollerMsg(L"Finished configuration poll of cloud domain %s\r\n", m_name);

   lockProperties();
   m_runtimeFlags &= ~ODF_CONFIGURATION_POLL_PENDING;
   m_runtimeFlags |= ODF_CONFIGURATION_POLL_PASSED;
   unlockProperties();

   pollerUnlock();
   nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, L"Finished configuration poll of cloud domain \"%s\" [%u]", m_name, m_id);
}

/**
 * Prepare for deletion
 */
void CloudDomain::prepareForDeletion()
{
   nxlog_debug_tag(DEBUG_TAG_OBJECT_LIFECYCLE, 4, L"CloudDomain::prepareForDeletion(%s [%u]): waiting for outstanding polls to finish", m_name, m_id);
   while (m_statusPollState.isPending() || m_configurationPollState.isPending())
      ThreadSleepMs(100);
   nxlog_debug_tag(DEBUG_TAG_OBJECT_LIFECYCLE, 4, L"CloudDomain::prepareForDeletion(%s [%u]): no outstanding polls left", m_name, m_id);

   super::prepareForDeletion();
}
