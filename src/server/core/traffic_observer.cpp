/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Victor Kirhenshtein
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
** File: traffic_observer.cpp
**/

#include "nxcore.h"
#include <traffic-connector.h>

#define POLL_CANCELLATION_CHECKPOINT() \
         do { if (g_flags & AF_SHUTDOWN) { pollerUnlock(); return; } } while(0)

/**
 * Debug tag for observation point discovery
 */
#define DEBUG_TAG_TRAFFIC_POLL   L"traffic.poll"

/**
 * Debug tag for configuration sync
 */
#define DEBUG_TAG_TRAFFIC_SYNC   L"traffic.sync"

/**
 * Default constructor for TrafficObserver class
 */
TrafficObserver::TrafficObserver() : super(Pollable::STATUS | Pollable::CONFIGURATION)
{
   m_credentials = nullptr;
   m_parsedCredentials = nullptr;
   m_zoneUIN = 0;
   m_linkedNodeId = 0;
   m_removalPolicy = 0;
   m_gracePeriod = 30;
   m_syncConfig = nullptr;
   m_lastDiscoveryStatus = 0;
   m_lastDiscoveryTime = 0;
   m_connectionState = TRAFFIC_OBSERVER_STATE_UNKNOWN;
   m_capabilities = 0;
   m_aliasSyncLastRun = 0;
   m_aliasSyncRecords = 0;
   m_aliasSyncErrors = 0;
   m_status = STATUS_NORMAL;
}

/**
 * Constructor from NXCP message
 */
TrafficObserver::TrafficObserver(const wchar_t *name, const NXCPMessage& request) : super(name, Pollable::STATUS | Pollable::CONFIGURATION)
{
   m_runtimeFlags |= ODF_CONFIGURATION_POLL_PENDING;
   m_connectorName = request.getFieldAsSharedString(VID_CONNECTOR_NAME);
   m_credentials = request.getFieldAsUtf8String(VID_CLOUD_CREDENTIALS);
   m_parsedCredentials = nullptr;
   parseCredentials();
   m_zoneUIN = request.isFieldExist(VID_ZONE_UIN) ? request.getFieldAsInt32(VID_ZONE_UIN) : 0;
   m_linkedNodeId = request.getFieldAsUInt32(VID_LINKED_NODE_ID);
   m_removalPolicy = request.getFieldAsInt16(VID_REMOVAL_POLICY);
   m_gracePeriod = request.getFieldAsUInt32(VID_GRACE_PERIOD);
   m_syncConfig = request.getFieldAsUtf8String(VID_SYNC_CONFIG);
   m_lastDiscoveryStatus = 0;
   m_lastDiscoveryTime = 0;
   m_connectionState = TRAFFIC_OBSERVER_STATE_UNKNOWN;
   m_capabilities = 0;
   m_aliasSyncLastRun = 0;
   m_aliasSyncRecords = 0;
   m_aliasSyncErrors = 0;
   m_status = STATUS_NORMAL;
}

/**
 * Constructor from JSON definition (REST API)
 */
TrafficObserver::TrafficObserver(const wchar_t *name, json_t *json) : super(name, Pollable::STATUS | Pollable::CONFIGURATION)
{
   m_runtimeFlags |= ODF_CONFIGURATION_POLL_PENDING;
   m_connectorName = json_object_get_string(json, "connectorName", L"");

   // Credentials may be supplied either as a raw JSON string or as a nested JSON object/array
   json_t *credentials = json_object_get(json, "credentials");
   if (json_is_object(credentials) || json_is_array(credentials))
   {
      char *text = json_dumps(credentials, JSON_COMPACT);
      m_credentials = MemCopyStringA(text);
      free(text);
   }
   else if (json_is_string(credentials))
   {
      m_credentials = MemCopyStringA(json_string_value(credentials));
   }
   else
   {
      m_credentials = nullptr;
   }
   m_parsedCredentials = nullptr;
   parseCredentials();

   m_zoneUIN = json_object_get_int32(json, "zoneUIN", 0);
   m_linkedNodeId = json_object_get_uint32(json, "linkedNodeId", 0);
   m_removalPolicy = static_cast<int16_t>(json_object_get_int32(json, "removalPolicy"));
   m_gracePeriod = json_object_get_uint32(json, "gracePeriod", 30);

   json_t *syncConfig = json_object_get(json, "syncConfig");
   if (json_is_object(syncConfig))
   {
      char *text = json_dumps(syncConfig, JSON_COMPACT);
      m_syncConfig = MemCopyStringA(text);
      free(text);
   }
   else if (json_is_string(syncConfig))
   {
      m_syncConfig = MemCopyStringA(json_string_value(syncConfig));
   }
   else
   {
      m_syncConfig = nullptr;
   }

   m_lastDiscoveryStatus = 0;
   m_lastDiscoveryTime = 0;
   m_connectionState = TRAFFIC_OBSERVER_STATE_UNKNOWN;
   m_capabilities = 0;
   m_aliasSyncLastRun = 0;
   m_aliasSyncRecords = 0;
   m_aliasSyncErrors = 0;
   m_status = STATUS_NORMAL;
}

/**
 * Parse credentials JSON string and cache the result
 */
void TrafficObserver::parseCredentials()
{
   json_decref(m_parsedCredentials);
   m_parsedCredentials = nullptr;
   if (m_credentials != nullptr)
   {
      json_error_t error;
      m_parsedCredentials = json_loads(m_credentials, 0, &error);
      if (m_parsedCredentials == nullptr)
         nxlog_write_tag(NXLOG_WARNING, L"traffic", L"TrafficObserver(%s [%u]): failed to parse credentials JSON on line %d: %hs", m_name, m_id, error.line, error.text);
   }
}

/**
 * TrafficObserver destructor
 */
TrafficObserver::~TrafficObserver()
{
   MemFree(m_credentials);
   json_decref(m_parsedCredentials);
   MemFree(m_syncConfig);
}

/**
 * Load from database
 */
bool TrafficObserver::loadFromDatabase(DB_HANDLE hdb, uint32_t id, DB_STATEMENT *preparedStatements)
{
   m_id = id;

   if (!loadCommonProperties(hdb, preparedStatements) || !super::loadFromDatabase(hdb, id, preparedStatements))
      return false;

   if (Pollable::loadFromDatabase(hdb, m_id))
   {
      if (static_cast<uint32_t>(time(nullptr) - m_configurationPollState.getLastCompleted()) < getCustomAttributeAsUInt32(L"SysConfig:Objects.ConfigurationPollingInterval", g_configurationPollingInterval))
         m_runtimeFlags |= ODF_CONFIGURATION_POLL_PASSED;
   }

   DB_STATEMENT hStmt = DBPrepare(hdb, L"SELECT connector_name,credentials,zone_uin,node_id,removal_policy,grace_period,sync_config,last_discovery_status,last_discovery_time,last_discovery_msg FROM traffic_observers WHERE id=?");
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
   m_zoneUIN = DBGetFieldLong(hResult, 0, 2);
   m_linkedNodeId = DBGetFieldULong(hResult, 0, 3);
   m_removalPolicy = static_cast<int16_t>(DBGetFieldLong(hResult, 0, 4));
   m_gracePeriod = DBGetFieldULong(hResult, 0, 5);
   m_syncConfig = DBGetFieldUTF8(hResult, 0, 6, nullptr, 0);
   m_lastDiscoveryStatus = static_cast<int16_t>(DBGetFieldLong(hResult, 0, 7));
   m_lastDiscoveryTime = DBGetFieldULong(hResult, 0, 8);
   char *lastDiscoveryMsg = DBGetFieldUTF8(hResult, 0, 9, nullptr, 0);
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
bool TrafficObserver::saveToDatabase(DB_HANDLE hdb)
{
   bool success = super::saveToDatabase(hdb);

   if (success && (m_modified & MODIFY_TRAFFIC_OBSERVER_PROPERTIES))
   {
      static const wchar_t *columns[] = {
         L"connector_name", L"credentials", L"zone_uin", L"node_id", L"removal_policy", L"grace_period",
         L"sync_config", L"last_discovery_status", L"last_discovery_time", L"last_discovery_msg", nullptr
      };
      DB_STATEMENT hStmt = DBPrepareMerge(hdb, L"traffic_observers", L"id", m_id, columns);
      if (hStmt != nullptr)
      {
         lockProperties();
         DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, m_connectorName, DB_BIND_TRANSIENT);
         DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, DB_CTYPE_UTF8_STRING, m_credentials, DB_BIND_STATIC);
         DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_zoneUIN);
         DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, m_linkedNodeId);
         DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, static_cast<int32_t>(m_removalPolicy));
         DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, m_gracePeriod);
         DBBind(hStmt, 7, DB_SQLTYPE_VARCHAR, DB_CTYPE_UTF8_STRING, m_syncConfig, DB_BIND_STATIC);
         DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, static_cast<int32_t>(m_lastDiscoveryStatus));
         DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_lastDiscoveryTime));
         DBBind(hStmt, 10, DB_SQLTYPE_VARCHAR, DB_CTYPE_UTF8_STRING, m_lastDiscoveryMessage.c_str(), DB_BIND_STATIC);
         DBBind(hStmt, 11, DB_SQLTYPE_INTEGER, m_id);

         success = DBExecute(hStmt);   // must execute under lock - STATIC-bound fields can be freed by concurrent writers
         DBFreeStatement(hStmt);
         unlockProperties();
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
bool TrafficObserver::deleteFromDatabase(DB_HANDLE hdb)
{
   bool success = super::deleteFromDatabase(hdb);
   if (success)
      success = executeQueryOnObject(hdb, L"DELETE FROM traffic_observers WHERE id=?");
   return success;
}

/**
 * Create NXSL object for this object
 */
NXSL_Value *TrafficObserver::createNXSLObject(NXSL_VM *vm)
{
   return vm->createValue(vm->createObject(&g_nxslTrafficObserverClass, new shared_ptr<TrafficObserver>(self())));
}

/**
 * Serialize to JSON
 */
json_t *TrafficObserver::toJson(bool includeSensitiveData)
{
   json_t *root = super::toJson(includeSensitiveData);

   lockProperties();
   json_object_set_new(root, "connectorName", json_string_t(m_connectorName));
   json_object_set_new(root, "zoneUIN", json_integer(m_zoneUIN));
   json_object_set_new(root, "linkedNodeId", json_integer(m_linkedNodeId));
   json_object_set_new(root, "removalPolicy", json_integer(m_removalPolicy));
   json_object_set_new(root, "gracePeriod", json_integer(m_gracePeriod));
   if (m_syncConfig != nullptr)
   {
      json_t *syncConfig = json_loads(m_syncConfig, 0, nullptr);
      json_object_set_new(root, "syncConfig", (syncConfig != nullptr) ? syncConfig : json_string(m_syncConfig));
   }
   json_object_set_new(root, "lastDiscoveryStatus", json_integer(m_lastDiscoveryStatus));
   json_object_set_new(root, "lastDiscoveryTime", json_integer(m_lastDiscoveryTime));
   json_object_set_new(root, "lastDiscoveryMessage", json_string(m_lastDiscoveryMessage.c_str()));
   json_object_set_new(root, "connectionState", json_integer(m_connectionState));
   json_object_set_new(root, "backendProduct", json_string(m_backendProduct.c_str()));
   json_object_set_new(root, "backendVersion", json_string(m_backendVersion.c_str()));
   json_object_set_new(root, "backendEdition", json_string(m_backendEdition.c_str()));
   json_object_set_new(root, "capabilities", json_integer(m_capabilities));
   unlockProperties();

   return root;
}

/**
 * Fill NXCP message
 */
void TrafficObserver::fillMessageLocked(NXCPMessage *msg, uint32_t userId)
{
   super::fillMessageLocked(msg, userId);
   msg->setField(VID_CONNECTOR_NAME, m_connectorName);
   msg->setField(VID_CLOUD_CREDENTIALS, m_credentials != nullptr);
   msg->setField(VID_ZONE_UIN, m_zoneUIN);
   msg->setField(VID_LINKED_NODE_ID, m_linkedNodeId);
   msg->setField(VID_REMOVAL_POLICY, m_removalPolicy);
   msg->setField(VID_GRACE_PERIOD, m_gracePeriod);
   msg->setFieldFromUtf8String(VID_SYNC_CONFIG, CHECK_NULL_EX_A(m_syncConfig));
   msg->setField(VID_LAST_DISCOVERY_STATUS, m_lastDiscoveryStatus);
   msg->setField(VID_LAST_DISCOVERY_TIME, static_cast<uint32_t>(m_lastDiscoveryTime));
   msg->setFieldFromUtf8String(VID_LAST_DISCOVERY_MESSAGE, m_lastDiscoveryMessage.c_str());
   msg->setField(VID_CONNECTION_STATE, m_connectionState);
   msg->setFieldFromUtf8String(VID_BACKEND_PRODUCT, m_backendProduct.c_str());
   msg->setFieldFromUtf8String(VID_VERSION, m_backendVersion.c_str());
   msg->setFieldFromUtf8String(VID_BACKEND_EDITION, m_backendEdition.c_str());
   msg->setField(VID_CAPABILITIES, m_capabilities);
}

/**
 * Modify object from NXCP message
 */
uint32_t TrafficObserver::modifyFromMessageInternal(const NXCPMessage& msg, ClientSession *session)
{
   if (msg.isFieldExist(VID_CONNECTOR_NAME))
      m_connectorName = msg.getFieldAsSharedString(VID_CONNECTOR_NAME);
   if (msg.isFieldExist(VID_CLOUD_CREDENTIALS))
   {
      MemFree(m_credentials);
      m_credentials = msg.getFieldAsUtf8String(VID_CLOUD_CREDENTIALS);
      parseCredentials();
   }
   if (msg.isFieldExist(VID_ZONE_UIN))
      m_zoneUIN = msg.getFieldAsInt32(VID_ZONE_UIN);
   if (msg.isFieldExist(VID_LINKED_NODE_ID))
      m_linkedNodeId = msg.getFieldAsUInt32(VID_LINKED_NODE_ID);
   if (msg.isFieldExist(VID_REMOVAL_POLICY))
      m_removalPolicy = msg.getFieldAsInt16(VID_REMOVAL_POLICY);
   if (msg.isFieldExist(VID_GRACE_PERIOD))
      m_gracePeriod = msg.getFieldAsUInt32(VID_GRACE_PERIOD);
   if (msg.isFieldExist(VID_SYNC_CONFIG))
   {
      MemFree(m_syncConfig);
      m_syncConfig = msg.getFieldAsUtf8String(VID_SYNC_CONFIG);
   }

   return super::modifyFromMessageInternal(msg, session);
}

/**
 * Calculate compound status
 */
void TrafficObserver::calculateCompoundStatus(bool forcedRecalc)
{
   int oldStatus = m_status;
   super::calculateCompoundStatus(forcedRecalc);
   if (oldStatus != m_status)
      setModified(MODIFY_RUNTIME);
}

/**
 * Update connection state, generating up/down events on transitions
 */
void TrafficObserver::updateConnectionState(int16_t newState, const wchar_t *details)
{
   lockProperties();
   int16_t oldState = m_connectionState;
   m_connectionState = newState;
   unlockProperties();

   if (oldState == newState)
      return;

   if ((newState == TRAFFIC_OBSERVER_STATE_UNREACHABLE) || (newState == TRAFFIC_OBSERVER_STATE_AUTH_FAILURE))
   {
      EventBuilder(EVENT_TRAFFIC_OBSERVER_UNREACHABLE, m_id)
         .param(L"connectorName", getConnectorName().cstr())
         .param(L"reason", (newState == TRAFFIC_OBSERVER_STATE_AUTH_FAILURE) ? L"authentication failure" : L"API unreachable")
         .param(L"details", CHECK_NULL_EX(details))
         .post();
   }
   else if ((newState == TRAFFIC_OBSERVER_STATE_CONNECTED) &&
            ((oldState == TRAFFIC_OBSERVER_STATE_UNREACHABLE) || (oldState == TRAFFIC_OBSERVER_STATE_AUTH_FAILURE)))
   {
      EventBuilder(EVENT_TRAFFIC_OBSERVER_RECOVERED, m_id)
         .param(L"connectorName", getConnectorName().cstr())
         .post();
   }
}

/**
 * Status poll - cheap connectivity/health check against the analyzer API
 */
void TrafficObserver::statusPoll(PollerInfo *poller, ClientSession *session, uint32_t rqId)
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
   sendPollerMsg(L"Starting status poll of traffic observer %s\r\n", m_name);
   nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, L"Starting status poll of traffic observer \"%s\" [%u]", m_name, m_id);

   poller->setStatus(L"test connection");
   SharedString connectorName = getConnectorName();
   TrafficConnectorInterface *connector = FindTrafficConnector(connectorName);
   if ((connector != nullptr) && (connector->TestConnection != nullptr))
   {
      json_t *credentials = getCredentials();
      TrafficBackendInfo info;
      TrafficConnectorStatus status = connector->TestConnection(credentials, &info);
      json_decref(credentials);

      if (status == TrafficConnectorStatus::SUCCESS)
      {
         lockProperties();
         m_backendProduct = info.product;
         m_backendVersion = info.version;
         m_backendEdition = info.edition;
         m_capabilities = info.capabilities;
         unlockProperties();
         updateConnectionState(TRAFFIC_OBSERVER_STATE_CONNECTED, nullptr);
         sendPollerMsg(L"   Analyzer API is reachable (%hs %hs %hs)\r\n", info.product, info.version, info.edition);
      }
      else
      {
         const wchar_t *errorMessage = GetTrafficConnectorErrorMessage(status);
         updateConnectionState((status == TrafficConnectorStatus::AUTH_ERROR) ? TRAFFIC_OBSERVER_STATE_AUTH_FAILURE : TRAFFIC_OBSERVER_STATE_UNREACHABLE, errorMessage);
         sendPollerMsg(POLLER_ERROR L"   Analyzer API is not reachable (%s)\r\n", errorMessage);
         nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, L"TrafficObserver::statusPoll(%s [%u]): connection test failed (%s)", m_name, m_id, errorMessage);
      }
   }
   else
   {
      if (!connectorName.isEmpty())
      {
         sendPollerMsg(POLLER_WARNING L"   Traffic connector \"%s\" not found\r\n", static_cast<const wchar_t*>(connectorName));
         nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, L"Traffic connector \"%s\" not found for traffic observer \"%s\" [%u]", static_cast<const wchar_t*>(connectorName), m_name, m_id);
      }
   }

   calculateCompoundStatus(true);

   poller->setStatus(L"hook");
   executeHookScript(L"StatusPoll");

   sendPollerMsg(L"Finished status poll of traffic observer %s\r\n", m_name);
   sendPollerMsg(L"Traffic observer status after poll is %s\r\n", GetStatusAsText(m_status, true));

   pollerUnlock();
   nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, L"Finished status poll of traffic observer \"%s\" [%u]", m_name, m_id);
}

/**
 * Configuration poll - refresh backend info and reconcile observation points
 */
void TrafficObserver::configurationPoll(PollerInfo *poller, ClientSession *session, uint32_t rqId)
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
   nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, L"Starting configuration poll of traffic observer \"%s\" [%u]", m_name, m_id);
   sendPollerMsg(L"Starting configuration poll of traffic observer %s\r\n", m_name);

   poller->setStatus(L"discover");
   SharedString connectorName = getConnectorName();
   TrafficConnectorInterface *connector = FindTrafficConnector(connectorName);
   if (connector != nullptr)
   {
      sendPollerMsg(L"   Using traffic connector \"%s\"\r\n", static_cast<const wchar_t*>(connectorName));

      // Refresh backend information and capability set
      json_t *credentials = getCredentials();
      TrafficBackendInfo info;
      TrafficConnectorStatus status = (connector->TestConnection != nullptr) ?
               connector->TestConnection(credentials, &info) : TrafficConnectorStatus::NOT_IMPLEMENTED;
      if (status == TrafficConnectorStatus::SUCCESS)
      {
         lockProperties();
         m_backendProduct = info.product;
         m_backendVersion = info.version;
         m_backendEdition = info.edition;
         m_capabilities = info.capabilities;
         unlockProperties();
         updateConnectionState(TRAFFIC_OBSERVER_STATE_CONNECTED, nullptr);
         sendPollerMsg(L"   Backend: %hs %hs (%hs), capabilities 0x%08X\r\n", info.product, info.version, info.edition, static_cast<uint32_t>(info.capabilities));
      }
      else
      {
         updateConnectionState((status == TrafficConnectorStatus::AUTH_ERROR) ? TRAFFIC_OBSERVER_STATE_AUTH_FAILURE : TRAFFIC_OBSERVER_STATE_UNREACHABLE,
            GetTrafficConnectorErrorMessage(status));
      }

      ObservationPointDescriptor *points = (connector->DiscoverPoints != nullptr) ? connector->DiscoverPoints(credentials) : nullptr;
      json_decref(credentials);
      unique_ptr<ObservationPointDescriptor> pointsHolder(points);   // owns the list even on early poll cancellation returns
      if (points != nullptr)
      {
         sendPollerMsg(L"   Discovery successful, reconciling observation points\r\n");
         nxlog_debug_tag(DEBUG_TAG_TRAFFIC_POLL, 5, L"Point discovery returned results for traffic observer \"%s\" [%u]", m_name, m_id);

         StringSet discoveredIds;
         unique_ptr<SharedObjectArray<NetObj>> children = getChildren(OBJECT_OBSERVATIONPOINT);

         for (const ObservationPointDescriptor *d = points; d != nullptr; d = d->next)
         {
            POLL_CANCELLATION_CHECKPOINT();

            // Find existing observation point by external ID
            shared_ptr<ObservationPoint> point;
            for (int i = 0; i < children->size(); i++)
            {
               shared_ptr<ObservationPoint> p = static_pointer_cast<ObservationPoint>(children->getShared(i));
               if (!strcmp(p->getExternalId().c_str(), d->externalId))
               {
                  point = p;
                  break;
               }
            }

            wchar_t wName[MAX_OBJECT_NAME];
            utf8_to_wchar(d->name, -1, wName, MAX_OBJECT_NAME);

            if (point != nullptr)
            {
               point->updateFromDiscovery(d, m_id);
               point->setName(wName);
               nxlog_debug_tag(DEBUG_TAG_TRAFFIC_POLL, 6, L"Updated existing observation point \"%s\" [%u] (external ID: %hs)", point->getName(), point->getId(), d->externalId);
            }
            else
            {
               point = make_shared<ObservationPoint>();
               point->setName(wName);
               NetObjInsert(point, true, false);
               point->updateFromDiscovery(d, m_id);
               point->setOwner(self());
               linkObjects(self(), point);
               point->publish();

               sendPollerMsg(L"   Created new observation point \"%hs\" (external ID: %hs)\r\n", d->name, d->externalId);
               nxlog_debug_tag(DEBUG_TAG_TRAFFIC_POLL, 5, L"Created new observation point \"%hs\" [%u] (external ID: %hs)", d->name, point->getId(), d->externalId);
            }

            wchar_t wExternalId[128];
            utf8_to_wchar(d->externalId, -1, wExternalId, 128);
            discoveredIds.add(wExternalId);
         }

         POLL_CANCELLATION_CHECKPOINT();

         // Handle observation points not found in discovery
         poller->setStatus(L"reconcile");
         lockProperties();
         int16_t removalPolicy = m_removalPolicy;
         uint32_t gracePeriod = m_gracePeriod;
         unlockProperties();

         children = getChildren(OBJECT_OBSERVATIONPOINT);
         for (int i = 0; i < children->size(); i++)
         {
            shared_ptr<ObservationPoint> point = static_pointer_cast<ObservationPoint>(children->getShared(i));

            wchar_t wExternalId[128];
            utf8_to_wchar(point->getExternalId().c_str(), -1, wExternalId, 128);
            if (discoveredIds.contains(wExternalId))
               continue;

            switch (removalPolicy)
            {
               case 0: // Mark inactive
                  point->updateState(OBSERVATION_POINT_STATE_INACTIVE, "missing");
                  nxlog_debug_tag(DEBUG_TAG_TRAFFIC_POLL, 5, L"Marked observation point \"%s\" [%u] as inactive (not found in discovery)", point->getName(), point->getId());
                  break;
               case 1: // Delete after grace period
               {
                  time_t lastSeen = point->getLastDiscoveryTime();
                  if (lastSeen != 0 && (time(nullptr) - lastSeen > static_cast<time_t>(gracePeriod) * 86400))
                  {
                     nxlog_debug_tag(DEBUG_TAG_TRAFFIC_POLL, 5, L"Deleting observation point \"%s\" [%u] (grace period expired)", point->getName(), point->getId());
                     point->deleteObject();
                  }
                  else
                  {
                     point->updateState(OBSERVATION_POINT_STATE_INACTIVE, "missing");
                     nxlog_debug_tag(DEBUG_TAG_TRAFFIC_POLL, 6, L"Marked observation point \"%s\" [%u] as inactive (grace period not expired)", point->getName(), point->getId());
                  }
                  break;
               }
               case 2: // Delete immediately
                  nxlog_debug_tag(DEBUG_TAG_TRAFFIC_POLL, 5, L"Deleting observation point \"%s\" [%u] (immediate removal policy)", point->getName(), point->getId());
                  point->deleteObject();
                  break;
               case 3: // Ignore
                  break;
            }
         }

         POLL_CANCELLATION_CHECKPOINT();

         // Host matching pass for in-scope active observation points
         poller->setStatus(L"host matching");
         children = getChildren(OBJECT_OBSERVATIONPOINT);
         for (int i = 0; i < children->size(); i++)
         {
            POLL_CANCELLATION_CHECKPOINT();
            shared_ptr<ObservationPoint> point = static_pointer_cast<ObservationPoint>(children->getShared(i));
            if (point->isInScope() && (point->getState() == OBSERVATION_POINT_STATE_ACTIVE))
            {
               sendPollerMsg(L"   Running host matching for observation point %s\r\n", point->getName());
               RunObservationPointHostMatching(point.get());
            }
         }

         // Update discovery status - success
         lockProperties();
         m_lastDiscoveryStatus = 0;
         m_lastDiscoveryTime = time(nullptr);
         m_lastDiscoveryMessage = "";
         setModified(MODIFY_TRAFFIC_OBSERVER_PROPERTIES);
         unlockProperties();
      }
      else
      {
         sendPollerMsg(POLLER_ERROR L"   Observation point discovery returned no results\r\n");
         nxlog_debug_tag(DEBUG_TAG_TRAFFIC_POLL, 5, L"Point discovery returned no results for traffic observer \"%s\" [%u]", m_name, m_id);

         lockProperties();
         m_lastDiscoveryStatus = 2;
         m_lastDiscoveryTime = time(nullptr);
         m_lastDiscoveryMessage = "Observation point discovery returned no results";
         setModified(MODIFY_TRAFFIC_OBSERVER_PROPERTIES);
         unlockProperties();
      }
   }
   else
   {
      sendPollerMsg(POLLER_ERROR L"   Traffic connector \"%s\" not found\r\n", static_cast<const wchar_t*>(connectorName));
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, L"Traffic connector \"%s\" not found for traffic observer \"%s\" [%u]", static_cast<const wchar_t*>(connectorName), m_name, m_id);

      lockProperties();
      m_lastDiscoveryStatus = 2;
      m_lastDiscoveryTime = time(nullptr);
      m_lastDiscoveryMessage = "Traffic connector not found";
      setModified(MODIFY_TRAFFIC_OBSERVER_PROPERTIES);
      unlockProperties();
   }

   // Execute hook script
   poller->setStatus(L"hook");
   executeHookScript(L"ConfigurationPoll");

   sendPollerMsg(L"Finished configuration poll of traffic observer %s\r\n", m_name);

   lockProperties();
   m_runtimeFlags &= ~ODF_CONFIGURATION_POLL_PENDING;
   m_runtimeFlags |= ODF_CONFIGURATION_POLL_PASSED;
   unlockProperties();

   pollerUnlock();
   nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, L"Finished configuration poll of traffic observer \"%s\" [%u]", m_name, m_id);
}

/**
 * Get observer-level metric via the traffic connector
 */
DataCollectionError TrafficObserver::getMetricFromConnector(const wchar_t *metric, wchar_t *buffer, size_t size)
{
   // Sync status metrics are served by the core, not the connector
   if (!wcsnicmp(metric, L"Sync.HostAliases.", 17))
   {
      DataCollectionError rc = DCE_SUCCESS;
      lockProperties();
      if (!wcsicmp(&metric[17], L"LastRun"))
         IntegerToString(static_cast<uint64_t>(m_aliasSyncLastRun), buffer);
      else if (!wcsicmp(&metric[17], L"RecordsSynced"))
         IntegerToString(m_aliasSyncRecords, buffer);
      else if (!wcsicmp(&metric[17], L"Errors"))
         IntegerToString(m_aliasSyncErrors, buffer);
      else
         rc = DCE_NOT_SUPPORTED;
      unlockProperties();
      return rc;
   }

   TrafficConnectorInterface *connector = FindTrafficConnector(getConnectorName());
   if ((connector == nullptr) || (connector->GetObserverMetric == nullptr))
      return DCE_NOT_SUPPORTED;

   json_t *credentials = getCredentials();
   DataCollectionError rc = connector->GetObserverMetric(metric, buffer, size, credentials);
   json_decref(credentials);
   nxlog_debug_tag(DEBUG_TAG_TRAFFIC_POLL, 7, L"TrafficObserver(%s [%u])->getMetricFromConnector(%s): rc=%d", m_name, m_id, metric, rc);
   return rc;
}

/**
 * Check if host alias sync is enabled in sync configuration; also returns the
 * configured sync interval in seconds (default 3600, minimum 60)
 */
bool TrafficObserver::isHostAliasSyncEnabled(uint32_t *interval) const
{
   bool enabled = false;
   uint32_t i = 3600;

   lockProperties();
   if (m_syncConfig != nullptr)
   {
      json_t *config = json_loads(m_syncConfig, 0, nullptr);
      if (config != nullptr)
      {
         json_t *hostAliases = json_object_get(config, "hostAliases");
         enabled = json_object_get_boolean(hostAliases, "enabled", false);
         i = json_object_get_uint32(hostAliases, "interval", 3600);
         json_decref(config);
      }
   }
   unlockProperties();

   if (interval != nullptr)
      *interval = std::max(i, static_cast<uint32_t>(60));
   return enabled;
}

/**
 * Push aliases (names of matched nodes) for all matched host records of this
 * observer's observation points to the analyzer backend
 */
void TrafficObserver::syncHostAliases()
{
   TrafficConnectorInterface *connector = FindTrafficConnector(getConnectorName());
   if ((connector == nullptr) || (connector->SyncHostAliases == nullptr))
      return;

   lockProperties();
   bool capable = (m_capabilities & TRAFFIC_CAPABILITY_SYNC_HOST_ALIASES) != 0;
   unlockProperties();
   if (!capable)
   {
      nxlog_debug_tag(DEBUG_TAG_TRAFFIC_SYNC, 6, L"TrafficObserver::syncHostAliases(%s [%u]): backend does not support host alias sync", m_name, m_id);
      return;
   }

   StringMap aliases;
   unique_ptr<SharedObjectArray<NetObj>> points = getChildren(OBJECT_OBSERVATIONPOINT);
   for (int i = 0; i < points->size(); i++)
   {
      unique_ptr<ObjectArray<ObservationPointHostRecord>> hosts = GetObservationPointMatchedHosts(points->get(i)->getId());
      for (int j = 0; j < hosts->size(); j++)
      {
         ObservationPointHostRecord *r = hosts->get(j);
         shared_ptr<NetObj> node = FindObjectById(r->nodeId, OBJECT_NODE);
         if (node != nullptr)
         {
            wchar_t hostKey[128];
            utf8_to_wchar(r->hostKey, -1, hostKey, 128);
            aliases.set(hostKey, node->getName());
         }
      }
   }

   json_t *credentials = getCredentials();
   TrafficConnectorStatus status = connector->SyncHostAliases(aliases, credentials);
   json_decref(credentials);

   lockProperties();
   m_aliasSyncLastRun = time(nullptr);
   if (status == TrafficConnectorStatus::SUCCESS)
      m_aliasSyncRecords = aliases.size();
   else
      m_aliasSyncErrors++;
   unlockProperties();

   if (status == TrafficConnectorStatus::SUCCESS)
   {
      nxlog_debug_tag(DEBUG_TAG_TRAFFIC_SYNC, 5, L"TrafficObserver::syncHostAliases(%s [%u]): %d host aliases synchronized", m_name, m_id, aliases.size());
   }
   else
   {
      const wchar_t *errorMessage = GetTrafficConnectorErrorMessage(status);
      nxlog_debug_tag(DEBUG_TAG_TRAFFIC_SYNC, 5, L"TrafficObserver::syncHostAliases(%s [%u]): sync failed (%s)", m_name, m_id, errorMessage);
      EventBuilder(EVENT_TRAFFIC_SYNC_FAILED, m_id)
         .param(L"surface", L"hostAliases")
         .param(L"error", errorMessage)
         .post();
   }
}

/**
 * Run enabled configuration sync surfaces. Unless forced, skips when the sync
 * interval has not yet elapsed or the analyzer connection is not up.
 */
void TrafficObserver::runConfigSync(bool force)
{
   uint32_t interval;
   if (!isHostAliasSyncEnabled(&interval))
      return;

   if (!force)
   {
      lockProperties();
      bool skip = (m_connectionState != TRAFFIC_OBSERVER_STATE_CONNECTED) ||
                  (time(nullptr) - m_aliasSyncLastRun < static_cast<time_t>(interval));
      unlockProperties();
      if (skip)
         return;
   }

   syncHostAliases();
}

/**
 * Scheduled task handler for configuration sync to traffic analyzers (Traffic.ConfigSync).
 * With a bound object, syncs that observer immediately; without one, acts as a dispatcher
 * running sync for every observer whose configured interval has elapsed.
 */
void SyncTrafficObserverConfig(const shared_ptr<ScheduledTaskParameters>& parameters)
{
   if (parameters->m_objectId != 0)
   {
      shared_ptr<NetObj> object = FindObjectById(parameters->m_objectId, OBJECT_TRAFFICOBSERVER);
      if (object != nullptr)
         static_cast<TrafficObserver*>(object.get())->runConfigSync(true);
      else
         nxlog_debug_tag(DEBUG_TAG_TRAFFIC_SYNC, 4, L"SyncTrafficObserverConfig: invalid object ID %u", parameters->m_objectId);
   }
   else
   {
      unique_ptr<SharedObjectArray<NetObj>> observers = g_idxObjectById.getObjects(OBJECT_TRAFFICOBSERVER);
      for (int i = 0; i < observers->size(); i++)
         static_cast<TrafficObserver*>(observers->get(i))->runConfigSync(false);
   }
}

/**
 * Prepare for deletion
 */
void TrafficObserver::prepareForDeletion()
{
   nxlog_debug_tag(DEBUG_TAG_OBJECT_LIFECYCLE, 4, L"TrafficObserver::prepareForDeletion(%s [%u]): waiting for outstanding polls to finish", m_name, m_id);
   while (m_statusPollState.isPending() || m_configurationPollState.isPending())
      ThreadSleepMs(100);
   nxlog_debug_tag(DEBUG_TAG_OBJECT_LIFECYCLE, 4, L"TrafficObserver::prepareForDeletion(%s [%u]): no outstanding polls left", m_name, m_id);

   super::prepareForDeletion();
}
