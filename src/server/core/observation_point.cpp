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
** File: observation_point.cpp
**/

#include "nxcore.h"
#include <traffic-connector.h>

/**
 * Default constructor for ObservationPoint class
 */
ObservationPoint::ObservationPoint() : super(Pollable::STATUS)
{
   m_observerId = 0;
   m_inScope = false;
   m_zoneUIN = -1;      // inherit from observer
   m_samplingRate = 0;  // unknown
   m_state = OBSERVATION_POINT_STATE_UNKNOWN;
   m_lastDiscoveryTime = 0;
   m_status = STATUS_NORMAL;
}

/**
 * ObservationPoint destructor
 */
ObservationPoint::~ObservationPoint()
{
}

/**
 * Load from database
 */
bool ObservationPoint::loadFromDatabase(DB_HANDLE hdb, uint32_t id, DB_STATEMENT *preparedStatements)
{
   m_id = id;

   if (!loadCommonProperties(hdb, preparedStatements) || !super::loadFromDatabase(hdb, id, preparedStatements))
      return false;

   Pollable::loadFromDatabase(hdb, m_id);

   DB_STATEMENT hStmt = DBPrepare(hdb, L"SELECT observer_id,external_id,point_type,in_scope,zone_uin,sampling_rate,local_networks,state,provider_state,last_discovery_time FROM observation_points WHERE id=?");
   if (hStmt == nullptr)
      return false;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
   DB_RESULT hResult = DBSelectPrepared(hStmt);
   DBFreeStatement(hStmt);
   if (hResult == nullptr)
      return false;

   m_observerId = DBGetFieldULong(hResult, 0, 0);
   char *externalId = DBGetFieldUTF8(hResult, 0, 1, nullptr, 0);
   m_externalId = CHECK_NULL_EX_A(externalId);
   MemFree(externalId);
   char *pointType = DBGetFieldUTF8(hResult, 0, 2, nullptr, 0);
   m_pointType = CHECK_NULL_EX_A(pointType);
   MemFree(pointType);
   wchar_t inScope[2];
   DBGetField(hResult, 0, 3, inScope, 2);
   m_inScope = (inScope[0] == L'1');
   m_zoneUIN = DBGetFieldLong(hResult, 0, 4);
   m_samplingRate = DBGetFieldULong(hResult, 0, 5);
   char *localNetworks = DBGetFieldUTF8(hResult, 0, 6, nullptr, 0);
   m_localNetworks = CHECK_NULL_EX_A(localNetworks);
   MemFree(localNetworks);
   m_state = static_cast<int16_t>(DBGetFieldLong(hResult, 0, 7));
   char *providerState = DBGetFieldUTF8(hResult, 0, 8, nullptr, 0);
   m_providerState = CHECK_NULL_EX_A(providerState);
   MemFree(providerState);
   m_lastDiscoveryTime = DBGetFieldULong(hResult, 0, 9);
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
bool ObservationPoint::saveToDatabase(DB_HANDLE hdb)
{
   bool success = super::saveToDatabase(hdb);

   if (success && (m_modified & MODIFY_OBSERVATION_POINT_PROPERTIES))
   {
      static const wchar_t *columns[] = {
         L"observer_id", L"external_id", L"point_type", L"in_scope", L"zone_uin",
         L"sampling_rate", L"local_networks", L"state", L"provider_state", L"last_discovery_time", nullptr
      };
      DB_STATEMENT hStmt = DBPrepareMerge(hdb, L"observation_points", L"id", m_id, columns);
      if (hStmt != nullptr)
      {
         lockProperties();
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_observerId);
         DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, DB_CTYPE_UTF8_STRING, m_externalId.c_str(), DB_BIND_STATIC);
         DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, DB_CTYPE_UTF8_STRING, m_pointType.c_str(), DB_BIND_STATIC);
         DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, m_inScope ? L"1" : L"0", DB_BIND_STATIC);
         DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, m_zoneUIN);
         DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, m_samplingRate);
         DBBind(hStmt, 7, DB_SQLTYPE_VARCHAR, DB_CTYPE_UTF8_STRING, m_localNetworks.c_str(), DB_BIND_STATIC);
         DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, static_cast<int32_t>(m_state));
         DBBind(hStmt, 9, DB_SQLTYPE_VARCHAR, DB_CTYPE_UTF8_STRING, m_providerState.c_str(), DB_BIND_STATIC);
         DBBind(hStmt, 10, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_lastDiscoveryTime));
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
bool ObservationPoint::deleteFromDatabase(DB_HANDLE hdb)
{
   bool success = super::deleteFromDatabase(hdb);
   if (success)
      success = executeQueryOnObject(hdb, L"DELETE FROM observation_points WHERE id=?");
   if (success)
      success = executeQueryOnObject(hdb, L"DELETE FROM observation_point_hosts WHERE point_id=?");
   return success;
}

/**
 * Create NXSL object for this object
 */
NXSL_Value *ObservationPoint::createNXSLObject(NXSL_VM *vm)
{
   return vm->createValue(vm->createObject(&g_nxslObservationPointClass, new shared_ptr<ObservationPoint>(self())));
}

/**
 * Serialize to JSON
 */
json_t *ObservationPoint::toJson(bool includeSensitiveData)
{
   json_t *root = super::toJson(includeSensitiveData);

   lockProperties();
   json_object_set_new(root, "observerId", json_integer(m_observerId));
   json_object_set_new(root, "externalId", json_string(m_externalId.c_str()));
   json_object_set_new(root, "pointType", json_string(m_pointType.c_str()));
   json_object_set_new(root, "inScope", json_boolean(m_inScope));
   json_object_set_new(root, "zoneUIN", json_integer(m_zoneUIN));
   json_object_set_new(root, "samplingRate", json_integer(m_samplingRate));
   json_object_set_new(root, "localNetworks", json_string(m_localNetworks.c_str()));
   json_object_set_new(root, "state", json_integer(m_state));
   json_object_set_new(root, "providerState", json_string(m_providerState.c_str()));
   json_object_set_new(root, "lastDiscoveryTime", json_integer(m_lastDiscoveryTime));
   unlockProperties();

   return root;
}

/**
 * Fill NXCP message
 */
void ObservationPoint::fillMessageLocked(NXCPMessage *msg, uint32_t userId)
{
   super::fillMessageLocked(msg, userId);
   msg->setField(VID_TRAFFIC_OBSERVER_ID, m_observerId);
   msg->setFieldFromUtf8String(VID_CLOUD_RESOURCE_ID, m_externalId.c_str());
   msg->setFieldFromUtf8String(VID_RESOURCE_TYPE, m_pointType.c_str());
   msg->setField(VID_IN_SCOPE, m_inScope);
   msg->setField(VID_ZONE_UIN, m_zoneUIN);
   msg->setField(VID_SAMPLING_RATE, m_samplingRate);
   msg->setFieldFromUtf8String(VID_LOCAL_NETWORKS, m_localNetworks.c_str());
   msg->setField(VID_RESOURCE_STATE, m_state);
   msg->setFieldFromUtf8String(VID_PROVIDER_STATE, m_providerState.c_str());
   msg->setField(VID_LAST_DISCOVERY_TIME, static_cast<uint32_t>(m_lastDiscoveryTime));
}

/**
 * Modify object from NXCP message
 */
uint32_t ObservationPoint::modifyFromMessageInternal(const NXCPMessage& msg, ClientSession *session)
{
   if (msg.isFieldExist(VID_IN_SCOPE))
      m_inScope = msg.getFieldAsBoolean(VID_IN_SCOPE);
   if (msg.isFieldExist(VID_ZONE_UIN))
      m_zoneUIN = msg.getFieldAsInt32(VID_ZONE_UIN);

   return super::modifyFromMessageInternal(msg, session);
}

/**
 * Calculate compound status
 */
void ObservationPoint::calculateCompoundStatus(bool forcedRecalc)
{
   int oldStatus = m_status;
   super::calculateCompoundStatus(forcedRecalc);
   if (oldStatus != m_status)
      setModified(MODIFY_RUNTIME);
}

/**
 * Get most critical additional status from object-specific state
 */
int ObservationPoint::getAdditionalMostCriticalStatus(StringBuffer *explanation)
{
   switch(m_state)
   {
      case OBSERVATION_POINT_STATE_ACTIVE:
         if (explanation != nullptr)
            explanation->append(L"Observation point is active");
         return STATUS_NORMAL;
      case OBSERVATION_POINT_STATE_INACTIVE:
         if (explanation != nullptr)
            explanation->append(L"Observation point is inactive");
         return STATUS_MINOR;
      default:
         if (explanation != nullptr)
            explanation->append(L"Observation point state is unknown");
         return STATUS_UNKNOWN;
   }
}

/**
 * Get effective zone for host matching (resolves inheritance from owning observer)
 */
int32_t ObservationPoint::getEffectiveZoneUIN() const
{
   if (m_zoneUIN != -1)
      return m_zoneUIN;
   shared_ptr<TrafficObserver> observer = m_owner.lock();
   return (observer != nullptr) ? observer->getZoneUIN() : -1;
}

/**
 * Get point-level metric via the traffic connector
 */
DataCollectionError ObservationPoint::getMetricFromConnector(const wchar_t *metric, wchar_t *buffer, size_t size)
{
   shared_ptr<TrafficObserver> observer = getOwner();
   if (observer == nullptr)
      return DCE_NOT_SUPPORTED;

   TrafficConnectorInterface *connector = FindTrafficConnector(observer->getConnectorName());
   if ((connector == nullptr) || (connector->GetPointMetric == nullptr))
      return DCE_NOT_SUPPORTED;

   json_t *credentials = observer->getCredentials();
   DataCollectionError rc = connector->GetPointMetric(getExternalId().c_str(), metric, buffer, size, credentials);
   json_decref(credentials);
   return rc;
}

/**
 * Get point-level table via the traffic connector
 */
DataCollectionError ObservationPoint::getTableFromConnector(const wchar_t *metric, shared_ptr<Table> *table)
{
   shared_ptr<TrafficObserver> observer = getOwner();
   if (observer == nullptr)
      return DCE_NOT_SUPPORTED;

   TrafficConnectorInterface *connector = FindTrafficConnector(observer->getConnectorName());
   if ((connector == nullptr) || (connector->GetPointTable == nullptr))
      return DCE_NOT_SUPPORTED;

   json_t *credentials = observer->getCredentials();
   DataCollectionError rc = connector->GetPointTable(getExternalId().c_str(), metric, table, credentials);
   json_decref(credentials);
   return rc;
}

/**
 * Update observation point from discovery data
 */
void ObservationPoint::updateFromDiscovery(const ObservationPointDescriptor *desc, uint32_t observerId)
{
   lockProperties();
   m_observerId = observerId;
   m_externalId = desc->externalId;
   m_pointType = desc->type;
   m_samplingRate = desc->samplingRate;
   if (desc->localNetworks != nullptr)
   {
      char buffer[4000] = "";
      size_t pos = 0;
      for (int i = 0; i < desc->localNetworks->size(); i++)
      {
         char network[256];
         wchar_to_utf8(desc->localNetworks->get(i), -1, network, sizeof(network));
         size_t len = strlen(network);
         if (pos + len + 2 > sizeof(buffer))
            break;
         if (pos > 0)
            buffer[pos++] = ',';
         memcpy(&buffer[pos], network, len + 1);
         pos += len;
      }
      m_localNetworks = buffer;
   }
   else
   {
      m_localNetworks = "";
   }
   m_lastDiscoveryTime = time(nullptr);
   setModified(MODIFY_OBSERVATION_POINT_PROPERTIES);
   unlockProperties();

   updateState(desc->state, desc->providerState);
}

/**
 * Update observation point state
 */
void ObservationPoint::updateState(int16_t newState, const char *providerState)
{
   lockProperties();
   int16_t oldState = m_state;
   bool changed = (m_state != newState) || strcmp(m_providerState.c_str(), providerState);
   if (changed)
   {
      m_state = newState;
      m_providerState = providerState;
      setModified(MODIFY_OBSERVATION_POINT_PROPERTIES);
   }
   unlockProperties();

   if (changed && (oldState != newState))
   {
      wchar_t wExternalId[128], wProviderState[64];
      utf8_to_wchar(getExternalId().c_str(), -1, wExternalId, 128);
      utf8_to_wchar(getProviderState().c_str(), -1, wProviderState, 64);
      EventBuilder(EVENT_OBSERVATION_POINT_STATE_CHANGED, m_id)
         .param(L"externalId", wExternalId)
         .param(L"oldState", static_cast<int32_t>(oldState))
         .param(L"newState", static_cast<int32_t>(newState))
         .param(L"providerState", wProviderState)
         .post();
   }
}

/**
 * Post-load hook. Link observation point to owning observer after all objects are loaded.
 */
void ObservationPoint::postLoad()
{
   super::postLoad();
   if (m_observerId != 0)
   {
      shared_ptr<NetObj> observer = FindObjectById(m_observerId, OBJECT_TRAFFICOBSERVER);
      if (observer != nullptr)
      {
         linkObjects(observer, self());
         m_owner = static_pointer_cast<TrafficObserver>(observer);
      }
      else
      {
         nxlog_write(NXLOG_ERROR, L"Inconsistent database: observation point \"%s\" [%u] has reference to non-existing traffic observer [%u]", m_name, m_id, m_observerId);
      }
   }
}

/**
 * Status poll
 */
void ObservationPoint::statusPoll(PollerInfo *poller, ClientSession *session, uint32_t rqId)
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
   sendPollerMsg(L"Starting status poll of observation point %s\r\n", m_name);
   nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, L"Starting status poll of observation point \"%s\" [%u]", m_name, m_id);

   calculateCompoundStatus(true);

   poller->setStatus(L"hook");
   executeHookScript(L"StatusPoll");

   sendPollerMsg(L"Finished status poll of observation point %s\r\n", m_name);
   sendPollerMsg(L"Observation point status after poll is %s\r\n", GetStatusAsText(m_status, true));

   pollerUnlock();
   nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, L"Finished status poll of observation point \"%s\" [%u]", m_name, m_id);
}

/**
 * Prepare for deletion
 */
void ObservationPoint::prepareForDeletion()
{
   nxlog_debug_tag(DEBUG_TAG_OBJECT_LIFECYCLE, 4, L"ObservationPoint::prepareForDeletion(%s [%u]): waiting for outstanding polls to finish", m_name, m_id);
   while (m_statusPollState.isPending())
      ThreadSleepMs(100);
   nxlog_debug_tag(DEBUG_TAG_OBJECT_LIFECYCLE, 4, L"ObservationPoint::prepareForDeletion(%s [%u]): no outstanding polls left", m_name, m_id);

   DropObservationPointHosts(m_id);

   super::prepareForDeletion();
}
