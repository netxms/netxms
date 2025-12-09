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
** File: accesspoint.cpp
**/

#include "nxcore.h"
#include <asset_management.h>

/**
 * Default constructor
 */
AccessPoint::AccessPoint() : super(Pollable::CONFIGURATION), m_macAddress(MacAddress::ZERO), m_radioInterfaces(0, 4)
{
   m_domainId = 0;
   m_controllerId = 0;
   m_index = 0;
	m_vendor = nullptr;
	m_model = nullptr;
	m_serialNumber = nullptr;
	m_apState = AP_UP;
   m_prevState = m_apState;
   m_gracePeriodStartTime = 0;
   m_peerNodeId = 0;
   m_peerInterfaceId = 0;
   m_peerDiscoveryProtocol = LL_PROTO_UNKNOWN;
   m_downSince = 0;
}

/**
 * Constructor for creating new access point object
 */
AccessPoint::AccessPoint(const TCHAR *name, uint32_t index, const MacAddress& macAddr) : super(name, Pollable::CONFIGURATION), m_macAddress(macAddr), m_radioInterfaces(0, 4)
{
	m_domainId = 0;
   m_controllerId = 0;
   m_index = index;
	m_vendor = nullptr;
	m_model = nullptr;
	m_serialNumber = nullptr;
	m_apState = AP_UP;
   m_prevState = m_apState;
   m_gracePeriodStartTime = 0;
	m_isHidden = true;
   m_peerNodeId = 0;
   m_peerInterfaceId = 0;
   m_peerDiscoveryProtocol = LL_PROTO_UNKNOWN;
   m_downSince = 0;
}

/**
 * Destructor
 */
AccessPoint::~AccessPoint()
{
	MemFree(m_vendor);
	MemFree(m_model);
	MemFree(m_serialNumber);
}

/**
 * Create object from database data
 */
bool AccessPoint::loadFromDatabase(DB_HANDLE hdb, uint32_t id, DB_STATEMENT *preparedStatements)
{
   m_id = id;

   if (!loadCommonProperties(hdb, preparedStatements) || !super::loadFromDatabase(hdb, id, preparedStatements))
      return false;

   if (Pollable::loadFromDatabase(hdb, m_id))
   {
      if (static_cast<uint32_t>(time(nullptr) - m_configurationPollState.getLastCompleted()) < g_configurationPollingInterval)
         m_runtimeFlags |= ODF_CONFIGURATION_POLL_PASSED;
   }

   DB_STATEMENT hStmt = PrepareObjectLoadStatement(hdb, preparedStatements, LSI_ACCESS_POINT,
      _T("SELECT mac_address,vendor,model,serial_number,domain_id,controller_id,ap_state,ap_index,grace_period_start,peer_node_id,peer_if_id,peer_proto,down_since FROM access_points WHERE id=?"));
   if (hStmt == nullptr)
      return false;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, id);
   DB_RESULT hResult = DBSelectPrepared(hStmt);
	if (hResult == nullptr)
		return false;

	m_macAddress = DBGetFieldMacAddr(hResult, 0, 0);
	m_vendor = DBGetField(hResult, 0, 1, nullptr, 0);
	m_model = DBGetField(hResult, 0, 2, nullptr, 0);
	m_serialNumber = DBGetField(hResult, 0, 3, nullptr, 0);
	m_domainId = DBGetFieldULong(hResult, 0, 4);
   m_controllerId = DBGetFieldULong(hResult, 0, 5);
	m_apState = (AccessPointState)DBGetFieldLong(hResult, 0, 6);
   m_prevState = (m_apState != AP_DOWN) ? m_apState : AP_UP;
   m_index = DBGetFieldULong(hResult, 0, 7);
   m_gracePeriodStartTime = DBGetFieldULong(hResult, 0, 8);
   m_peerNodeId = DBGetFieldULong(hResult, 0, 9);
   m_peerInterfaceId = DBGetFieldULong(hResult, 0, 10);
   m_peerDiscoveryProtocol = static_cast<LinkLayerProtocol>(DBGetFieldLong(hResult, 0, 11));
   m_downSince = DBGetFieldLong(hResult, 0, 12);
	DBFreeResult(hResult);

   // Load DCI and access list
   loadACLFromDB(hdb, preparedStatements);
   loadItemsFromDB(hdb, preparedStatements);
   for(int i = 0; i < m_dcObjects.size(); i++)
      if (!m_dcObjects.get(i)->loadThresholdsFromDB(hdb, preparedStatements))
         return false;
   loadDCIListForCleanup(hdb);

   // Load radio interfaces
   hResult = executeSelectOnObject(hdb, _T("SELECT radio_index,if_index,name,bssid,ssid,frequency,band,channel,power_dbm,power_mw FROM radios WHERE owner_id={id}"));
   if (hResult == nullptr)
      return false;

   int count = DBGetNumRows(hResult);
   for(int i = 0; i < count; i++)
   {
      RadioInterfaceInfo *rif = m_radioInterfaces.addPlaceholder();
      rif->index = DBGetFieldULong(hResult, i, 0);
      rif->ifIndex = DBGetFieldULong(hResult, i, 1);
      DBGetField(hResult, i, 2, rif->name, MAX_OBJECT_NAME);

      TCHAR bssid[16];
      DBGetField(hResult, i, 3, bssid, 16);
      StrToBin(bssid, rif->bssid, MAC_ADDR_LENGTH);

      DBGetField(hResult, i, 4, rif->ssid, MAX_SSID_LENGTH);
      rif->frequency = DBGetFieldULong(hResult, i, 5);
      rif->band = static_cast<RadioBand>(DBGetFieldLong(hResult, i, 6));
      rif->channel = DBGetFieldULong(hResult, i, 7);
      rif->powerDBm = DBGetFieldLong(hResult, i, 8);
      rif->powerMW = DBGetFieldLong(hResult, i, 9);
   }
   DBFreeResult(hResult);

   return true;
}

/**
 * Save object to database
 */
bool AccessPoint::saveToDatabase(DB_HANDLE hdb)
{
   bool success = super::saveToDatabase(hdb);

   // Lock object's access
   if (success && (m_modified & MODIFY_AP_PROPERTIES))
   {
      static const TCHAR *columns[] = { _T("mac_address"), _T("vendor"), _T("model"), _T("serial_number"), _T("domain_id"), _T("controller_id"), _T("ap_state"), _T("ap_index"),
                                        _T("grace_period_start"), _T("peer_node_id"), _T("peer_if_id"), _T("peer_proto"), _T("down_since"), nullptr };
      DB_STATEMENT hStmt = DBPrepareMerge(hdb, _T("access_points"), _T("id"), m_id, columns);
      if (hStmt != nullptr)
      {
         lockProperties();
         DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, m_macAddress);
         DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, m_vendor, DB_BIND_STATIC);
         DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, m_model, DB_BIND_STATIC);
         DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, m_serialNumber, DB_BIND_STATIC);
         DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, m_domainId);
         DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, m_controllerId);
         DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, static_cast<int32_t>(m_apState));
         DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, m_index);
         DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_gracePeriodStartTime));
         DBBind(hStmt, 10, DB_SQLTYPE_INTEGER, m_peerNodeId);
         DBBind(hStmt, 11, DB_SQLTYPE_INTEGER, m_peerInterfaceId);
         DBBind(hStmt, 12, DB_SQLTYPE_INTEGER, static_cast<int32_t>(m_peerDiscoveryProtocol));
         DBBind(hStmt, 13, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_downSince));
         DBBind(hStmt, 14, DB_SQLTYPE_INTEGER, m_id);
         success = DBExecute(hStmt);
         DBFreeStatement(hStmt);
         unlockProperties();
      }
      else
      {
         success = false;
      }
   }

   if (success && (m_modified & MODIFY_RADIO_INTERFACES))
   {
      success = executeQueryOnObject(hdb, _T("DELETE FROM radios WHERE owner_id=?"));
      lockProperties();
      if (success && !m_radioInterfaces.isEmpty())
      {
         DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO radios (owner_id,radio_index,if_index,name,bssid,ssid,frequency,band,channel,power_dbm,power_mw) VALUES (?,?,?,?,?,?,?,?,?,?,?)"));
         if (hStmt != nullptr)
         {
            TCHAR bssid[16];
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
            for(int i = 0; (i < m_radioInterfaces.size()) && success; i++)
            {
               RadioInterfaceInfo *rif = m_radioInterfaces.get(i);
               DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, rif->index);
               DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, rif->ifIndex);
               DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, rif->name, DB_BIND_STATIC);
               DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, BinToStr(rif->bssid, MAC_ADDR_LENGTH, bssid), DB_BIND_STATIC);
               DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, rif->ssid, DB_BIND_STATIC);
               DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, rif->frequency);
               DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, rif->band);
               DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, rif->channel);
               DBBind(hStmt, 10, DB_SQLTYPE_INTEGER, rif->powerDBm);
               DBBind(hStmt, 11, DB_SQLTYPE_INTEGER, rif->powerMW);
               success = DBExecute(hStmt);
            }
            DBFreeStatement(hStmt);
         }
         else
         {
            success = false;
         }
      }
      unlockProperties();
   }

   return success;
}

/**
 * Delete object from database
 */
bool AccessPoint::deleteFromDatabase(DB_HANDLE hdb)
{
   bool success = super::deleteFromDatabase(hdb);
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM access_points WHERE id=?"));
   return success;
}

/**
 * Create NXCP message with essential object's data
 */
void AccessPoint::fillMessageLockedEssential(NXCPMessage *msg, uint32_t userId)
{
   super::fillMessageLockedEssential(msg, userId);
   msg->setField(VID_MODEL, CHECK_NULL_EX(m_model));
   msg->setField(VID_MAC_ADDR, m_macAddress);

   msg->setField(VID_RADIO_COUNT, static_cast<uint16_t>(m_radioInterfaces.size()));
   uint32_t fieldId = VID_RADIO_LIST_BASE;
   for(int i = 0; i < m_radioInterfaces.size(); i++)
   {
      RadioInterfaceInfo *rif = m_radioInterfaces.get(i);
      msg->setField(fieldId++, rif->index);
      msg->setField(fieldId++, rif->name);
      msg->setField(fieldId++, rif->bssid, MAC_ADDR_LENGTH);
      msg->setField(fieldId++, rif->frequency);
      msg->setField(fieldId++, static_cast<int16_t>(rif->band));
      msg->setField(fieldId++, rif->channel);
      msg->setField(fieldId++, rif->powerDBm);
      msg->setField(fieldId++, rif->powerMW);
      msg->setField(fieldId++, rif->ssid);
      fieldId++;
   }
}

/**
 * Create NXCP message with object's data
 */
void AccessPoint::fillMessageLocked(NXCPMessage *msg, uint32_t userId)
{
   super::fillMessageLocked(msg, userId);
   msg->setField(VID_DOMAIN_ID, m_domainId);
   msg->setField(VID_CONTROLLER_ID, m_controllerId);
   msg->setField(VID_IP_ADDRESS, m_ipAddress);
	msg->setField(VID_VENDOR, CHECK_NULL_EX(m_vendor));
	msg->setField(VID_SERIAL_NUMBER, CHECK_NULL_EX(m_serialNumber));
   msg->setField(VID_STATE, static_cast<uint16_t>(m_apState));
   msg->setField(VID_AP_INDEX, m_index);
   msg->setField(VID_PEER_NODE_ID, m_peerNodeId);
   msg->setField(VID_PEER_INTERFACE_ID, m_peerInterfaceId);
   msg->setField(VID_PEER_PROTOCOL, static_cast<int16_t>(m_peerDiscoveryProtocol));
}

/**
 * Attach access point to wireless domain
 */
void AccessPoint::attachToDomain(uint32_t domainId, uint32_t controllerId)
{
	if ((m_domainId == domainId) && (m_controllerId == controllerId))
		return;

	if (m_domainId != domainId)
	{
      if (m_domainId != 0)
      {
         shared_ptr<NetObj> currDomain = FindObjectById(m_domainId, OBJECT_WIRELESSDOMAIN);
         if (currDomain != nullptr)
         {
            unlinkObjects(currDomain.get(), this);
         }
      }

      shared_ptr<NetObj> newDomain = FindObjectById(domainId, OBJECT_WIRELESSDOMAIN);
      if (newDomain != nullptr)
      {
         linkObjects(newDomain, self());
      }
	}

	lockProperties();
	m_domainId = domainId;
	m_controllerId = controllerId;
	setModified(MODIFY_AP_PROPERTIES);
	unlockProperties();
}

/**
 * Update radio interfaces information
 */
void AccessPoint::updateRadioInterfaces(const StructArray<RadioInterfaceInfo>& ri)
{
	lockProperties();
	m_radioInterfaces.clear();
	m_radioInterfaces.addAll(ri);
	unlockProperties();
}

/**
 * Check if given radio interface index (radio ID) is on this access point
 */
bool AccessPoint::isMyRadio(uint32_t rfIndex)
{
	bool result = false;
	lockProperties();
   for(int i = 0; i < m_radioInterfaces.size(); i++)
   {
      if (m_radioInterfaces.get(i)->index == rfIndex)
      {
         result = true;
         break;
      }
   }
	unlockProperties();
	return result;
}

/**
 * Check if given radio MAC address (BSSID) is on this access point
 */
bool AccessPoint::isMyRadio(const BYTE *bssid)
{
	bool result = false;
	lockProperties();
   for(int i = 0; i < m_radioInterfaces.size(); i++)
   {
      if (!memcmp(m_radioInterfaces.get(i)->bssid, bssid, MAC_ADDR_LENGTH))
      {
         result = true;
         break;
      }
   }
	unlockProperties();
	return result;
}

/**
 * Get radio name
 */
void AccessPoint::getRadioName(uint32_t rfIndex, TCHAR *buffer, size_t bufSize) const
{
	buffer[0] = 0;
	lockProperties();
   for(int i = 0; i < m_radioInterfaces.size(); i++)
   {
      if (m_radioInterfaces.get(i)->index == rfIndex)
      {
         _tcslcpy(buffer, m_radioInterfaces.get(i)->name, bufSize);
         break;
      }
   }
	unlockProperties();
}

/**
 * Get name of controller node object
 */
String AccessPoint::getControllerName() const
{
   shared_ptr<NetObj> node = FindObjectById(m_controllerId, OBJECT_NODE);
   return (node != nullptr) ? String(node->getName()) : String(_T("<none>"));
}

/**
 * Get name of wireless domain object
 */
String AccessPoint::getWirelessDomainName() const
{
   shared_ptr<NetObj> domain = FindObjectById(m_domainId, OBJECT_WIRELESSDOMAIN);
   return (domain != nullptr) ? String(domain->getName()) : String(_T("<none>"));
}

/**
 * Update access point information
 */
void AccessPoint::updateInfo(const TCHAR *vendor, const TCHAR *model, const TCHAR *serialNumber)
{
	lockProperties();

	MemFree(m_vendor);
	m_vendor = MemCopyString(vendor);

	MemFree(m_model);
	m_model = MemCopyString(model);

	MemFree(m_serialNumber);
	m_serialNumber = MemCopyString(serialNumber);

	setModified(MODIFY_AP_PROPERTIES);
	unlockProperties();
}

/**
 * Update access point state
 */
void AccessPoint::updateState(AccessPointState state)
{
   if (state == m_apState)
   {
      if ((m_status == STATUS_UNKNOWN) && (state != AP_UNKNOWN))
         calculateCompoundStatus(); // Fix incorrect object status
      return;
   }

	lockProperties();
   if (state == AP_DOWN)
      m_prevState = m_apState;
   if ((state != AP_UP) && (m_downSince == 0))
   {
      m_downSince = time(nullptr);
   }
   else
   {
      m_downSince = 0;
   }
   m_apState = state;
   setModified(MODIFY_AP_PROPERTIES);
	unlockProperties();

   if (m_status != STATUS_UNMANAGED)
   {
      uint32_t eventCode;
      switch(state)
      {
         case AP_UP:
            eventCode = EVENT_AP_UP;
            break;
         case AP_UNPROVISIONED:
            eventCode = EVENT_AP_UNPROVISIONED;
            break;
         case AP_DOWN:
            eventCode = EVENT_AP_DOWN;
            break;
         default:
            eventCode = EVENT_AP_UNKNOWN;
            break;
      }
      EventBuilder(eventCode, m_id)
         .param(_T("domainId"), m_domainId, EventBuilder::OBJECT_ID_FORMAT)
         .param(_T("controllerId"), m_controllerId, EventBuilder::OBJECT_ID_FORMAT)
         .param(_T("macAddr"), m_macAddress)
         .param(_T("ipAddr"), m_ipAddress)
         .param(_T("vendor"), CHECK_NULL_EX(m_vendor))
         .param(_T("model"), CHECK_NULL_EX(m_model))
         .param(_T("serialNumber"), CHECK_NULL_EX(m_serialNumber))
         .param(_T("state"), state)
         .post();
   }

   calculateCompoundStatus();
}

/**
 * Additional status information based on state
 */
int AccessPoint::getAdditionalMostCriticalStatus()
{
   switch(m_apState)
   {
      case AP_UP:
         return STATUS_NORMAL;
      case AP_UNPROVISIONED:
         return STATUS_MAJOR;
      case AP_DOWN:
         return STATUS_CRITICAL;
      default:
         return STATUS_UNKNOWN;
   }
}

/**
 * Do status poll. Expected to be called only within wireless domain status poll.
 */
void AccessPoint::statusPollFromController(ClientSession *session, uint32_t requestId, Node *controller, SNMP_Transport *snmpTransport, WirelessControllerBridge *bridge)
{
   m_pollRequestor = session;
   m_pollRequestId = requestId;

   sendPollerMsg(_T("   Starting status poll on access point %s\r\n"), m_name);
   sendPollerMsg(_T("      Current access point status is %s\r\n"), GetStatusAsText(m_status, true));

   AccessPointState state;
   if (bridge != nullptr)
      state = bridge->getAccessPointState(getWirelessDomain().get(), m_index, m_macAddress, m_ipAddress, m_serialNumber, m_radioInterfaces);
   else if (controller != nullptr)
      state = controller->getAccessPointState(this, snmpTransport, m_radioInterfaces);
   else
      state = AP_UNKNOWN;

   if (state != AP_UNKNOWN)
   {
      nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("AccessPoint::statusPoll(%s [%u]): get state %s from %s"), m_name, m_id, GetAPStateAsText(state),
         (controller != nullptr) ? _T("controller") : _T("bridge"));
      sendPollerMsg(_T("      State reported by %s is %s\r\n"), (controller != nullptr) ? _T("controller") : _T("bridge interface"), GetAPStateAsText(state));
   }
   else if (m_gracePeriodStartTime == 0)
   {
      nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("AccessPoint::statusPoll(%s [%u]): unable to get AP state from %s"), m_name, m_id, (controller != nullptr) ? _T("controller") : _T("bridge"));
      sendPollerMsg(POLLER_WARNING _T("      Unable to get AP state from %s\r\n"), (controller != nullptr) ? _T("controller") : _T("bridge interface"));

      if (m_ipAddress.isValid() && (bridge == nullptr))  // do not attempt to ping APs created via WLC bridge interface
      {
         uint32_t icmpProxy = 0;

         if (IsZoningEnabled() && (controller->getZoneUIN() != 0))
         {
            shared_ptr<Zone> zone = FindZoneByUIN(controller->getZoneUIN());
            if (zone != nullptr)
            {
               icmpProxy = zone->getAvailableProxyNodeId(this);
            }
         }

         if (icmpProxy != 0)
         {
            sendPollerMsg(_T("      Starting ICMP ping via proxy\r\n"));
            nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 7, _T("AccessPoint::StatusPoll(%s [%u]): ping via proxy [%u]"), m_name, m_id, icmpProxy);
            shared_ptr<Node> proxyNode = static_pointer_cast<Node>(g_idxNodeById.get(icmpProxy));
            if ((proxyNode != nullptr) && proxyNode->isNativeAgent() && !proxyNode->isDown())
            {
               nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 7, _T("AccessPoint::StatusPoll(%s [%u]): proxy node found: %s"), m_name, m_id, proxyNode->getName());
               shared_ptr<AgentConnection> conn = proxyNode->createAgentConnection();
               if (conn != nullptr)
               {
                  TCHAR parameter[64], buffer[64];

                  _sntprintf(parameter, 64, _T("Icmp.Ping(%s)"), m_ipAddress.toString(buffer));
                  if (conn->getParameter(parameter, buffer, 64) == ERR_SUCCESS)
                  {
                     nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 7, _T("AccessPoint::StatusPoll(%s [%u]): proxy response: \"%s\""), m_name, m_id, buffer);
                     TCHAR *eptr;
                     long value = _tcstol(buffer, &eptr, 10);
                     if ((*eptr == 0) && (value >= 0))
                     {
                        if (value < 10000)
                        {
                           sendPollerMsg(POLLER_ERROR _T("      responded to ICMP ping\r\n"));
                           if (m_apState == AP_DOWN)
                              state = m_prevState;  /* FIXME: get actual AP state here */
                        }
                        else
                        {
                           sendPollerMsg(POLLER_ERROR _T("      no response to ICMP ping\r\n"));
                           state = AP_DOWN;
                        }
                     }
                  }
                  conn->disconnect();
               }
               else
               {
                  nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 7, _T("AccessPoint::statusPoll(%s [%u]): cannot connect to agent on proxy node"), m_name, m_id);
                  sendPollerMsg(POLLER_ERROR _T("      Unable to establish connection with proxy node\r\n"));
               }
            }
            else
            {
               nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 7, _T("AccessPoint::statusPoll(%s [%u]): proxy node not available"), m_name, m_id);
               sendPollerMsg(POLLER_ERROR _T("      ICMP proxy not available\r\n"));
            }
         }
         else	// not using ICMP proxy
         {
            TCHAR buffer[64];
            sendPollerMsg(_T("      Starting ICMP ping\r\n"));
            nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 7, _T("AccessPoint::statusPoll(%s [%u]): calling IcmpPing on %s, timeout=%u, size=%d"),
                  m_name, m_id, m_ipAddress.toString(buffer), g_icmpPingTimeout, g_icmpPingSize);
            uint32_t responseTime;
            uint32_t pingStatus = IcmpPing(m_ipAddress, 3, g_icmpPingTimeout, &responseTime, g_icmpPingSize, false);
            if (pingStatus == ICMP_SUCCESS)
            {
               sendPollerMsg(POLLER_ERROR _T("      responded to ICMP ping\r\n"));
               if (m_apState == AP_DOWN)
                  state = m_prevState;  /* FIXME: get actual AP state here */
            }
            else
            {
               sendPollerMsg(POLLER_ERROR _T("      no response to ICMP ping\r\n"));
               state = AP_DOWN;
            }
            nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 7, _T("AccessPoint::StatusPoll(%s [%u]): ping result %d, state=%d"), m_name, m_id, pingStatus, state);
         }
      }
   }

   if ((state == AP_UNKNOWN) && (m_gracePeriodStartTime != 0))
      state = AP_DOWN;
   updateState(state);

   sendPollerMsg(_T("      Access point status after poll is %s\r\n"), GetStatusAsText(m_status, true));
	sendPollerMsg(_T("   Finished status poll on access point %s\r\n"), m_name);
}

/**
 * Perform configuration poll on this access point.
 */
void AccessPoint::configurationPoll(PollerInfo *poller, ClientSession *session, uint32_t rqId)
{
   lockProperties();
   if (m_isDeleteInitiated || IsShutdownInProgress())
   {
      m_configurationPollState.complete(0);
      unlockProperties();
      return;
   }
   unlockProperties();

   poller->setStatus(_T("wait for lock"));
   pollerLock(configuration);

   if (IsShutdownInProgress())
   {
      pollerUnlock();
      return;
   }

   m_pollRequestor = session;
   m_pollRequestId = rqId;
   nxlog_debug(5, _T("Starting configuration poll of access point %s (ID: %d), m_flags: %d"), m_name, m_id, m_flags);

   // Execute hook script
   poller->setStatus(_T("hook"));
   executeHookScript(_T("ConfigurationPoll"));

   poller->setStatus(_T("autobind"));
   if (ConfigReadBoolean(_T("Objects.AccessPoints.TemplateAutoApply"), false))
      applyTemplates();
   if (ConfigReadBoolean(_T("Objects.AccessPoints.ContainerAutoBind"), false))
      updateContainerMembership();

   sendPollerMsg(_T("Finished configuration poll of access point %s\r\n"), m_name);

   UpdateAssetLinkage(this);
   autoFillAssetProperties();

   lockProperties();
   m_runtimeFlags &= ~ODF_CONFIGURATION_POLL_PENDING;
   m_runtimeFlags |= ODF_CONFIGURATION_POLL_PASSED;
   unlockProperties();

   pollerUnlock();
   nxlog_debug(5, _T("Finished configuration poll of access point %s (ID: %d)"), m_name, m_id);
}

/**
 * Create NXSL object for this object
 */
NXSL_Value *AccessPoint::createNXSLObject(NXSL_VM *vm)
{
   return vm->createValue(vm->createObject(&g_nxslAccessPointClass, new shared_ptr<AccessPoint>(self())));
}

/**
 * Create NXSL array with radio interfaces
 */
NXSL_Value *AccessPoint::getRadioInterfacesForNXSL(NXSL_VM *vm) const
{
   auto a = new NXSL_Array(vm);
   lockProperties();
   for(int i = 0; i < m_radioInterfaces.size(); i++)
   {
      a->append(vm->createValue(vm->createObject(&g_nxslRadioInterfaceClass, new RadioInterfaceInfo(*m_radioInterfaces.get(i)))));
   }
   unlockProperties();
   return vm->createValue(a);
}

/**
 * Get BSSIDs of all radios
 */
std::vector<MacAddress> AccessPoint::getRadioBSSID() const
{
   std::vector<MacAddress> bssids;
   lockProperties();
   for(int i = 0; i < m_radioInterfaces.size(); i++)
   {
      bssids.push_back(MacAddress(m_radioInterfaces.get(i)->bssid, MAC_ADDR_LENGTH));
   }
   unlockProperties();
   return bssids;
}

/**
 * Get access point's zone UIN
 */
int32_t AccessPoint::getZoneUIN() const
{
   shared_ptr<Node> controller = getController();
   return (controller != nullptr) ? controller->getZoneUIN() : 0;
}

/**
 * Mark access point as disappeared
 */
void AccessPoint::markAsDisappeared()
{
   lockProperties();
   if (m_gracePeriodStartTime == 0)
      m_gracePeriodStartTime = time(nullptr);
   setModified(MODIFY_AP_PROPERTIES, false);
   unlockProperties();
   updateState(AP_DOWN);
}

/**
 * Get internal metric
 */
DataCollectionError AccessPoint::getInternalMetric(const TCHAR *name, TCHAR *buffer, size_t size)
{
   DataCollectionError rc = super::getInternalMetric(name, buffer, size);
   if (rc != DCE_NOT_SUPPORTED)
      return rc;

   if (MatchString(_T("WLCBridge(*)"), name, false))
   {
      shared_ptr<WirelessDomain> wirelessDomain = getWirelessDomain();
      if (wirelessDomain != nullptr)
      {
         WirelessControllerBridge *bridge = wirelessDomain->getBridgeInterface();
         if (bridge != nullptr)
         {
            TCHAR attribute[256];
            AgentGetParameterArg(name, 1, attribute, 256);
            rc = bridge->getAccessPointMetric(wirelessDomain.get(), m_index, m_macAddress, m_ipAddress, m_serialNumber, attribute, buffer, size);
         }
      }
   }

   return rc;
}

/**
 * Get wireless stations registered on this AP.
 * Returned list must be destroyed by caller.
 */
ObjectArray<WirelessStationInfo> *AccessPoint::getWirelessStations() const
{
   shared_ptr<Node> controller = getController();
   if (controller != nullptr)
      return controller->getWirelessStations(m_id);

   shared_ptr<WirelessDomain> wirelessDomain = getWirelessDomain();
   if (wirelessDomain == nullptr)
      return nullptr;

   WirelessControllerBridge *bridge = wirelessDomain->getBridgeInterface();
   if (bridge == nullptr)
      return nullptr;

   ObjectArray<WirelessStationInfo> *wirelessStations = bridge->getAccessPointWirelessStations(wirelessDomain.get(), m_index, m_macAddress, m_ipAddress, m_serialNumber);
   if (wirelessStations == nullptr)
      return nullptr;

   for(int i = 0; i < wirelessStations->size(); i++)
   {
      WirelessStationInfo *ws = wirelessStations->get(i);
      ws->apObjectId = m_id;
      getRadioName(ws->rfIndex, ws->rfName, MAX_OBJECT_NAME);
   }
   return wirelessStations;
}

/**
 * Get wireless stations registered on this AP as NXSL array.
 */
NXSL_Value *AccessPoint::getWirelessStationsForNXSL(NXSL_VM *vm) const
{
   shared_ptr<Node> controller = getController();
   if (controller != nullptr)
      return controller->getWirelessStationsForNXSL(vm, m_id);

   shared_ptr<WirelessDomain> wirelessDomain = getWirelessDomain();
   if (wirelessDomain == nullptr)
      return vm->createValue();

   WirelessControllerBridge *bridge = wirelessDomain->getBridgeInterface();
   if (bridge == nullptr)
      return vm->createValue();

   ObjectArray<WirelessStationInfo> *wirelessStations = bridge->getAccessPointWirelessStations(wirelessDomain.get(), m_index, m_macAddress, m_ipAddress, m_serialNumber);
   if (wirelessStations == nullptr)
      return vm->createValue();

   NXSL_Array *wsList = new NXSL_Array(vm);
   for(int i = 0; i < wirelessStations->size(); i++)
   {
      WirelessStationInfo *ws = wirelessStations->get(i);
      ws->apObjectId = m_id;
      getRadioName(ws->rfIndex, ws->rfName, MAX_OBJECT_NAME);
      wsList->append(vm->createValue(vm->createObject(&g_nxslWirelessStationClass, wirelessStations->get(i))));
   }

   wirelessStations->setOwner(Ownership::False);   // Array elements are now owned by NXSL objects
   delete wirelessStations;

   return vm->createValue(wsList);
}

/**
 * Write list of registered wireless stations to NXCP message
 */
bool AccessPoint::writeWsListToMessage(NXCPMessage *msg) const
{
   shared_ptr<Node> controller = getController();
   if (controller != nullptr)
   {
      controller->writeWsListToMessage(msg, m_id);
      return true;
   }

   shared_ptr<WirelessDomain> wirelessDomain = getWirelessDomain();
   if (wirelessDomain == nullptr)
      return false;

   WirelessControllerBridge *bridge = wirelessDomain->getBridgeInterface();
   if (bridge == nullptr)
      return false;

   ObjectArray<WirelessStationInfo> *wirelessStations = bridge->getAccessPointWirelessStations(wirelessDomain.get(), m_index, m_macAddress, m_ipAddress, m_serialNumber);
   if (wirelessStations == nullptr)
      return false;

   uint32_t fieldId = VID_ELEMENT_LIST_BASE;
   for(int i = 0; i < wirelessStations->size(); i++)
   {
      WirelessStationInfo *ws = wirelessStations->get(i);
      ws->apObjectId = m_id;
      getRadioName(ws->rfIndex, ws->rfName, MAX_OBJECT_NAME);
      ws->fillMessage(msg, fieldId);
      fieldId += 10;
   }
   msg->setField(VID_NUM_ELEMENTS, wirelessStations->size());
   delete wirelessStations;
   return true;
}

/**
 * Serialize object to JSON
 */
json_t *AccessPoint::toJson()
{
   json_t *root = super::toJson();

   lockProperties();
   json_object_set_new(root, "index", json_integer(m_index));
   json_object_set_new(root, "ipAddress", m_ipAddress.toJson());
   json_object_set_new(root, "domainId", json_integer(m_domainId));
   json_object_set_new(root, "controllerId", json_integer(m_controllerId));
   json_object_set_new(root, "macAddress", m_macAddress.toJson());
   json_object_set_new(root, "vendor", json_string_t(m_vendor));
   json_object_set_new(root, "model", json_string_t(m_model));
   json_object_set_new(root, "serialNumber", json_string_t(m_serialNumber));
   json_object_set_new(root, "radioInterfaces", json_struct_array(m_radioInterfaces));
   json_object_set_new(root, "state", json_integer(m_apState));
   json_object_set_new(root, "prevState", json_integer(m_prevState));
   json_object_set_new(root, "peerNodeId", json_integer(m_peerNodeId));
   json_object_set_new(root, "peerInterfaceId", json_integer(m_peerInterfaceId));
   json_object_set_new(root, "peerDiscoveryProtocol", json_integer(m_peerDiscoveryProtocol));
   json_object_set_new(root, "downSince", json_integer(m_downSince));
   unlockProperties();

   return root;
}

/**
 * Get instances for instance discovery DCO
 */
StringMap *AccessPoint::getInstanceList(DCObject *dco)
{
   shared_ptr<Node> sourceNode;
   uint32_t sourceNodeId = getEffectiveSourceNode(dco);
   if (sourceNodeId != 0)
   {
      sourceNode = static_pointer_cast<Node>(FindObjectById(dco->getSourceNode(), OBJECT_NODE));
      if (sourceNode == nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG_INSTANCE_POLL, 6, _T("AccessPoint::getInstanceList(%s [%u]): source node [%u] not found"), dco->getName().cstr(), dco->getId(), sourceNodeId);
         return nullptr;
      }
   }

   StringList *instances = nullptr;
   StringMap *instanceMap = nullptr;
   shared_ptr<Table> instanceTable;
   wchar_t tableName[MAX_DB_STRING], nameColumn[MAX_DB_STRING];
   switch(dco->getInstanceDiscoveryMethod())
   {
      case IDM_INTERNAL_TABLE:
         parseInstanceDiscoveryTableName(dco->getInstanceDiscoveryData(), tableName, nameColumn);
         if (sourceNode != nullptr)
         {
            sourceNode->getInternalTable(tableName, &instanceTable);
         }
         else
         {
            getInternalTable(tableName, &instanceTable);
         }
         break;
      case IDM_SCRIPT:
         if (sourceNode != nullptr)
         {
            sourceNode->getStringMapFromScript(dco->getInstanceDiscoveryData(), &instanceMap, this);
         }
         else
         {
            getStringMapFromScript(dco->getInstanceDiscoveryData(), &instanceMap, this);
         }
         break;
      case IDM_WEB_SERVICE:
         if (sourceNode != nullptr)
         {
            sourceNode->getListFromWebService(dco->getInstanceDiscoveryData(), &instances);
         }
         break;
      default:
         break;
   }
   if ((instances == nullptr) && (instanceMap == nullptr) && (instanceTable == nullptr))
      return nullptr;

   if (instanceTable != nullptr)
   {
      instanceMap = new StringMap();
      wchar_t buffer[1024];
      int nameColumnIndex = (nameColumn[0] != 0) ? instanceTable->getColumnIndex(nameColumn) : -1;
      for(int i = 0; i < instanceTable->getNumRows(); i++)
      {
         instanceTable->buildInstanceString(i, buffer, 1024);
         if (nameColumnIndex != -1)
         {
            const wchar_t *name = instanceTable->getAsString(i, nameColumnIndex, buffer);
            if (name != nullptr)
               instanceMap->set(buffer, name);
            else
               instanceMap->set(buffer, buffer);
         }
         else
         {
            instanceMap->set(buffer, buffer);
         }
      }
   }
   else if (instanceMap == nullptr)
   {
      instanceMap = new StringMap;
      for(int i = 0; i < instances->size(); i++)
         instanceMap->set(instances->get(i), instances->get(i));
   }
   delete instances;
   return instanceMap;
}

/**
 * Set peer information
 */
void AccessPoint::setPeer(Node *node, Interface *iface, LinkLayerProtocol protocol)
{
   lockProperties();

   if ((m_peerNodeId != node->getId()) || (m_peerInterfaceId != iface->getId()) || (m_peerDiscoveryProtocol != protocol))
   {
      m_peerNodeId = node->getId();
      m_peerInterfaceId = iface->getId();
      m_peerDiscoveryProtocol = protocol;
      setModified(MODIFY_AP_PROPERTIES);
   }

   unlockProperties();
}

/**
 * Clear ethernet peer information
 */
void AccessPoint::clearPeer()
{
   lockProperties();
   m_peerNodeId = 0;
   m_peerInterfaceId = 0;
   m_peerDiscoveryProtocol = LL_PROTO_UNKNOWN;
   setModified(MODIFY_AP_PROPERTIES);
   unlockProperties();
}
