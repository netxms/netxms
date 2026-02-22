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

/**
 * Default constructor for CloudDomain class
 */
CloudDomain::CloudDomain() : super(Pollable::STATUS | Pollable::CONFIGURATION)
{
   m_removalPolicy = 0;
   m_gracePeriod = 30;
   m_defaultPollingInterval = 300;
   m_autoDiscoverChildren = true;
   m_autoProvisionDCI = true;
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
   m_accountIdentifier = request.getFieldAsSharedString(VID_ACCOUNT_IDENTIFIER);
   m_credentials = request.getFieldAsSharedString(VID_CLOUD_CREDENTIALS);
   m_discoverySchedule = request.getFieldAsSharedString(VID_DISCOVERY_SCHEDULE);
   m_discoveryFilter = request.getFieldAsSharedString(VID_DISCOVERY_FILTER);
   m_removalPolicy = request.getFieldAsInt16(VID_REMOVAL_POLICY);
   m_gracePeriod = request.getFieldAsUInt32(VID_GRACE_PERIOD);
   m_defaultPollingInterval = request.getFieldAsUInt32(VID_DEFAULT_POLL_INTERVAL);
   m_autoDiscoverChildren = request.getFieldAsBoolean(VID_AUTO_DISCOVER_CHILDREN);
   m_autoProvisionDCI = request.getFieldAsBoolean(VID_AUTO_PROVISION_DCI);
   m_lastDiscoveryStatus = 0;
   m_lastDiscoveryTime = 0;
   m_status = STATUS_NORMAL;
}

/**
 * CloudDomain destructor
 */
CloudDomain::~CloudDomain()
{
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

   DB_STATEMENT hStmt = DBPrepare(hdb, L"SELECT connector_name,account_identifier,credentials,discovery_schedule,discovery_filter,removal_policy,grace_period,default_poll_interval,auto_discover_children,auto_provision_dci,last_discovery_status,last_discovery_time,last_discovery_msg FROM cloud_domains WHERE id=?");
   if (hStmt == nullptr)
      return false;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
   DB_RESULT hResult = DBSelectPrepared(hStmt);
   DBFreeStatement(hStmt);
   if (hResult == nullptr)
      return false;

   m_connectorName = DBGetFieldAsSharedString(hResult, 0, 0);
   m_accountIdentifier = DBGetFieldAsSharedString(hResult, 0, 1);
   m_credentials = DBGetFieldAsSharedString(hResult, 0, 2);
   m_discoverySchedule = DBGetFieldAsSharedString(hResult, 0, 3);
   m_discoveryFilter = DBGetFieldAsSharedString(hResult, 0, 4);
   m_removalPolicy = static_cast<int16_t>(DBGetFieldLong(hResult, 0, 5));
   m_gracePeriod = DBGetFieldULong(hResult, 0, 6);
   m_defaultPollingInterval = DBGetFieldULong(hResult, 0, 7);
   m_autoDiscoverChildren = DBGetFieldLong(hResult, 0, 8) != 0;
   m_autoProvisionDCI = DBGetFieldLong(hResult, 0, 9) != 0;
   m_lastDiscoveryStatus = static_cast<int16_t>(DBGetFieldLong(hResult, 0, 10));
   m_lastDiscoveryTime = DBGetFieldULong(hResult, 0, 11);
   m_lastDiscoveryMessage = DBGetFieldAsSharedString(hResult, 0, 12);
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
         L"connector_name", L"account_identifier", L"credentials", L"discovery_schedule",
         L"discovery_filter", L"removal_policy", L"grace_period", L"default_poll_interval",
         L"auto_discover_children", L"auto_provision_dci", L"last_discovery_status",
         L"last_discovery_time", L"last_discovery_msg", nullptr
      };
      DB_STATEMENT hStmt = DBPrepareMerge(hdb, L"cloud_domains", L"id", m_id, columns);
      if (hStmt != nullptr)
      {
         lockProperties();
         DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, m_connectorName, DB_BIND_TRANSIENT);
         DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, m_accountIdentifier, DB_BIND_TRANSIENT);
         DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, m_credentials, DB_BIND_TRANSIENT);
         DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, m_discoverySchedule, DB_BIND_TRANSIENT);
         DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, m_discoveryFilter, DB_BIND_TRANSIENT);
         DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, static_cast<int32_t>(m_removalPolicy));
         DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, m_gracePeriod);
         DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, m_defaultPollingInterval);
         DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, m_autoDiscoverChildren ? 1 : 0);
         DBBind(hStmt, 10, DB_SQLTYPE_INTEGER, m_autoProvisionDCI ? 1 : 0);
         DBBind(hStmt, 11, DB_SQLTYPE_INTEGER, static_cast<int32_t>(m_lastDiscoveryStatus));
         DBBind(hStmt, 12, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_lastDiscoveryTime));
         DBBind(hStmt, 13, DB_SQLTYPE_VARCHAR, m_lastDiscoveryMessage, DB_BIND_TRANSIENT);
         DBBind(hStmt, 14, DB_SQLTYPE_INTEGER, m_id);
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
   json_object_set_new(root, "accountIdentifier", json_string_t(m_accountIdentifier));
   json_object_set_new(root, "removalPolicy", json_integer(m_removalPolicy));
   json_object_set_new(root, "gracePeriod", json_integer(m_gracePeriod));
   json_object_set_new(root, "defaultPollingInterval", json_integer(m_defaultPollingInterval));
   json_object_set_new(root, "autoDiscoverChildren", json_boolean(m_autoDiscoverChildren));
   json_object_set_new(root, "autoProvisionDCI", json_boolean(m_autoProvisionDCI));
   json_object_set_new(root, "lastDiscoveryStatus", json_integer(m_lastDiscoveryStatus));
   json_object_set_new(root, "lastDiscoveryTime", json_integer(m_lastDiscoveryTime));
   json_object_set_new(root, "lastDiscoveryMessage", json_string_t(m_lastDiscoveryMessage));
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
   msg->setField(VID_ACCOUNT_IDENTIFIER, m_accountIdentifier);
   msg->setField(VID_CLOUD_CREDENTIALS, !m_credentials.isNull());
   msg->setField(VID_DISCOVERY_SCHEDULE, m_discoverySchedule);
   msg->setField(VID_DISCOVERY_FILTER, m_discoveryFilter);
   msg->setField(VID_REMOVAL_POLICY, m_removalPolicy);
   msg->setField(VID_GRACE_PERIOD, m_gracePeriod);
   msg->setField(VID_DEFAULT_POLL_INTERVAL, m_defaultPollingInterval);
   msg->setField(VID_AUTO_DISCOVER_CHILDREN, m_autoDiscoverChildren);
   msg->setField(VID_AUTO_PROVISION_DCI, m_autoProvisionDCI);
   msg->setField(VID_LAST_DISCOVERY_STATUS, m_lastDiscoveryStatus);
   msg->setField(VID_LAST_DISCOVERY_TIME, static_cast<uint32_t>(m_lastDiscoveryTime));
   msg->setField(VID_LAST_DISCOVERY_MESSAGE, m_lastDiscoveryMessage);
}

/**
 * Modify object from NXCP message
 */
uint32_t CloudDomain::modifyFromMessageInternal(const NXCPMessage& msg, ClientSession *session)
{
   if (msg.isFieldExist(VID_CONNECTOR_NAME))
      m_connectorName = msg.getFieldAsSharedString(VID_CONNECTOR_NAME);
   if (msg.isFieldExist(VID_ACCOUNT_IDENTIFIER))
      m_accountIdentifier = msg.getFieldAsSharedString(VID_ACCOUNT_IDENTIFIER);
   if (msg.isFieldExist(VID_CLOUD_CREDENTIALS))
      m_credentials = msg.getFieldAsSharedString(VID_CLOUD_CREDENTIALS);
   if (msg.isFieldExist(VID_DISCOVERY_SCHEDULE))
      m_discoverySchedule = msg.getFieldAsSharedString(VID_DISCOVERY_SCHEDULE);
   if (msg.isFieldExist(VID_DISCOVERY_FILTER))
      m_discoveryFilter = msg.getFieldAsSharedString(VID_DISCOVERY_FILTER);
   if (msg.isFieldExist(VID_REMOVAL_POLICY))
      m_removalPolicy = msg.getFieldAsInt16(VID_REMOVAL_POLICY);
   if (msg.isFieldExist(VID_GRACE_PERIOD))
      m_gracePeriod = msg.getFieldAsUInt32(VID_GRACE_PERIOD);
   if (msg.isFieldExist(VID_DEFAULT_POLL_INTERVAL))
      m_defaultPollingInterval = msg.getFieldAsUInt32(VID_DEFAULT_POLL_INTERVAL);
   if (msg.isFieldExist(VID_AUTO_DISCOVER_CHILDREN))
      m_autoDiscoverChildren = msg.getFieldAsBoolean(VID_AUTO_DISCOVER_CHILDREN);
   if (msg.isFieldExist(VID_AUTO_PROVISION_DCI))
      m_autoProvisionDCI = msg.getFieldAsBoolean(VID_AUTO_PROVISION_DCI);

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

   // TODO: Phase 2 - call connector queryState() for child resources

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

   // TODO: Phase 2 - call connector discover() and reconcile resources

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
   nxlog_debug(4, L"CloudDomain::PrepareForDeletion(%s [%u]): waiting for outstanding polls to finish", m_name, m_id);
   while (m_statusPollState.isPending() || m_configurationPollState.isPending())
      ThreadSleepMs(100);
   nxlog_debug(4, L"CloudDomain::PrepareForDeletion(%s [%u]): no outstanding polls left", m_name, m_id);

   super::prepareForDeletion();
}
