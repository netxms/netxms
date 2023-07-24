/*
** NetXMS - Network Management System
** Copyright (C) 2003-2023 Victor Kirhenshtein
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
** File: interface.cpp
**
**/

#include "nxcore.h"
#include <ieee8021x.h>

/**
 * Default constructor for Interface object
 */
Interface::Interface() : super(), m_macAddr(MacAddress::ZERO)
{
   m_parentInterfaceId = 0;
   m_index = 0;
   m_type = IFTYPE_OTHER;
   m_mtu = 0;
   m_speed = 0;
	m_bridgePortNumber = 0;
	m_peerNodeId = 0;
	m_peerInterfaceId = 0;
   m_peerDiscoveryProtocol = LL_PROTO_UNKNOWN;
   m_adminState = IF_ADMIN_STATE_UNKNOWN;
   m_operState = IF_OPER_STATE_UNKNOWN;
   m_pendingOperState = IF_OPER_STATE_UNKNOWN;
   m_confirmedOperState = IF_OPER_STATE_UNKNOWN;
	m_dot1xPaeAuthState = PAE_STATE_UNKNOWN;
	m_dot1xBackendAuthState = BACKEND_STATE_UNKNOWN;
   m_lastDownEventId = 0;
	m_pendingStatus = -1;
	m_statusPollCount = 0;
   m_operStatePollCount = 0;
	m_requiredPollCount = 0;	// Use system default
	m_zoneUIN = 0;
   m_ifTableSuffixLen = 0;
   m_ifTableSuffix = nullptr;
   m_vlans = nullptr;
   m_lastKnownOperState = IF_OPER_STATE_UNKNOWN;
   m_lastKnownAdminState = IF_ADMIN_STATE_UNKNOWN;
   m_ospfArea = 0;
   m_ospfType = OSPFInterfaceType::UNKNOWN;
   m_ospfState = OSPFInterfaceState::UNKNOWN;
   m_stpPortState = SpanningTreePortState::UNKNOWN;
}

/**
 * Constructor for "fake" interface object
 */
Interface::Interface(const InetAddressList& addrList, int32_t zoneUIN, bool bSyntheticMask) : super(), m_macAddr(MacAddress::ZERO), m_description(_T("unknown")), m_ifName(_T("unknown"))
{
   m_parentInterfaceId = 0;
	m_flags = bSyntheticMask ? IF_SYNTHETIC_MASK : 0;
   if (addrList.isLoopbackOnly())
		m_flags |= IF_LOOPBACK;

	_tcscpy(m_name, _T("unknown"));
   m_ipAddressList.add(addrList);
   m_index = 1;
   m_type = IFTYPE_OTHER;
   m_mtu = 0;
   m_speed = 0;
	m_bridgePortNumber = 0;
	m_peerNodeId = 0;
	m_peerInterfaceId = 0;
   m_peerDiscoveryProtocol = LL_PROTO_UNKNOWN;
   m_adminState = IF_ADMIN_STATE_UNKNOWN;
   m_operState = IF_OPER_STATE_UNKNOWN;
   m_pendingOperState = IF_OPER_STATE_UNKNOWN;
   m_confirmedOperState = IF_OPER_STATE_UNKNOWN;
	m_dot1xPaeAuthState = PAE_STATE_UNKNOWN;
	m_dot1xBackendAuthState = BACKEND_STATE_UNKNOWN;
   m_lastDownEventId = 0;
	m_pendingStatus = -1;
   m_statusPollCount = 0;
   m_operStatePollCount = 0;
	m_requiredPollCount = 0;	// Use system default
	m_zoneUIN = zoneUIN;
   m_isHidden = true;
   m_ifTableSuffixLen = 0;
   m_ifTableSuffix = nullptr;
   m_vlans = nullptr;
   m_lastKnownOperState = IF_OPER_STATE_UNKNOWN;
   m_lastKnownAdminState = IF_ADMIN_STATE_UNKNOWN;
   m_ospfArea = 0;
   m_ospfType = OSPFInterfaceType::UNKNOWN;
   m_ospfState = OSPFInterfaceState::UNKNOWN;
   m_stpPortState = SpanningTreePortState::UNKNOWN;
   setCreationTime();
}

/**
 * Constructor for normal interface object
 */
Interface::Interface(const TCHAR *objectName, const TCHAR *ifName, const TCHAR *description, uint32_t index, const InetAddressList& addrList, uint32_t ifType, int32_t zoneUIN)
          : super(), m_macAddr(MacAddress::ZERO), m_description(description), m_ifName(ifName)
{
   if ((ifType == IFTYPE_SOFTWARE_LOOPBACK) || addrList.isLoopbackOnly())
      m_flags = IF_LOOPBACK;
   else
      m_flags = 0;

   m_parentInterfaceId = 0;
   _tcslcpy(m_name, objectName, MAX_OBJECT_NAME);
   m_index = index;
   m_type = ifType;
   m_mtu = 0;
   m_speed = 0;
   m_ipAddressList.add(addrList);
	m_bridgePortNumber = 0;
	m_peerNodeId = 0;
	m_peerInterfaceId = 0;
   m_adminState = IF_ADMIN_STATE_UNKNOWN;
   m_operState = IF_OPER_STATE_UNKNOWN;
   m_pendingOperState = IF_OPER_STATE_UNKNOWN;
   m_confirmedOperState = IF_OPER_STATE_UNKNOWN;
   m_peerDiscoveryProtocol = LL_PROTO_UNKNOWN;
	m_dot1xPaeAuthState = PAE_STATE_UNKNOWN;
	m_dot1xBackendAuthState = BACKEND_STATE_UNKNOWN;
   m_lastDownEventId = 0;
	m_pendingStatus = -1;
   m_statusPollCount = 0;
   m_operStatePollCount = 0;
	m_requiredPollCount = 0;	// Use system default
	m_zoneUIN = zoneUIN;
   m_isHidden = true;
   m_ifTableSuffixLen = 0;
   m_ifTableSuffix = nullptr;
   m_vlans = nullptr;
   m_lastKnownOperState = IF_OPER_STATE_UNKNOWN;
   m_lastKnownAdminState = IF_ADMIN_STATE_UNKNOWN;
   m_ospfArea = 0;
   m_ospfType = OSPFInterfaceType::UNKNOWN;
   m_ospfState = OSPFInterfaceState::UNKNOWN;
   m_stpPortState = SpanningTreePortState::UNKNOWN;
   setCreationTime();
}

/**
 * Interface class destructor
 */
Interface::~Interface()
{
   MemFree(m_ifTableSuffix);
   delete m_vlans;
}

/**
 * Create object from database record
 */
bool Interface::loadFromDatabase(DB_HANDLE hdb, UINT32 id)
{
   bool success = false;

   m_id = id;

   if (!loadCommonProperties(hdb))
      return false;

	DB_STATEMENT hStmt = DBPrepare(hdb,
		_T("SELECT if_type,if_index,node_id,mac_addr,required_polls,bridge_port,phy_chassis,phy_module,")
		_T("phy_pic,phy_port,peer_node_id,peer_if_id,description,if_name,if_alias,dot1x_pae_state,dot1x_backend_state,")
		_T("admin_state,oper_state,peer_proto,mtu,speed,parent_iface,last_known_oper_state,last_known_admin_state,")
      _T("ospf_area,ospf_if_type,ospf_if_state,stp_port_state,iftable_suffix FROM interfaces WHERE id=?"));
	if (hStmt == nullptr)
		return false;
	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);

	DB_RESULT hResult = DBSelectPrepared(hStmt);
   if (hResult == nullptr)
	{
		DBFreeStatement(hStmt);
      return false;     // Query failed
	}

   if (DBGetNumRows(hResult) != 0)
   {
      m_type = DBGetFieldULong(hResult, 0, 0);
      m_index = DBGetFieldULong(hResult, 0, 1);
      uint32_t nodeId = DBGetFieldULong(hResult, 0, 2);
      m_macAddr = DBGetFieldMacAddr(hResult, 0, 3);
      m_requiredPollCount = DBGetFieldLong(hResult, 0, 4);
		m_bridgePortNumber = DBGetFieldULong(hResult, 0, 5);
		m_physicalLocation.chassis = DBGetFieldULong(hResult, 0, 6);
		m_physicalLocation.module = DBGetFieldULong(hResult, 0, 7);
		m_physicalLocation.pic = DBGetFieldULong(hResult, 0, 8);
		m_physicalLocation.port = DBGetFieldULong(hResult, 0, 9);
		m_peerNodeId = DBGetFieldULong(hResult, 0, 10);
		m_peerInterfaceId = DBGetFieldULong(hResult, 0, 11);
		m_description = DBGetFieldAsSharedString(hResult, 0, 12);
      m_ifName = DBGetFieldAsSharedString(hResult, 0, 13);
      m_ifAlias = DBGetFieldAsSharedString(hResult, 0, 14);
		m_dot1xPaeAuthState = static_cast<int16_t>(DBGetFieldLong(hResult, 0, 15));
		m_dot1xBackendAuthState = static_cast<int16_t>(DBGetFieldLong(hResult, 0, 16));
		m_adminState = static_cast<int16_t>(DBGetFieldLong(hResult, 0, 17));
		m_operState = static_cast<int16_t>(DBGetFieldLong(hResult, 0, 18));
		m_confirmedOperState = m_operState;
		m_peerDiscoveryProtocol = static_cast<LinkLayerProtocol>(DBGetFieldLong(hResult, 0, 19));
		m_mtu = DBGetFieldULong(hResult, 0, 20);
      m_speed = DBGetFieldUInt64(hResult, 0, 21);
      m_parentInterfaceId = DBGetFieldULong(hResult, 0, 22);
      m_lastKnownOperState = static_cast<int16_t>(DBGetFieldLong(hResult, 0, 23));
      m_lastKnownAdminState = static_cast<int16_t>(DBGetFieldLong(hResult, 0, 24));
      m_ospfArea = DBGetFieldIPAddr(hResult, 0, 25);
      m_ospfType = static_cast<OSPFInterfaceType>(DBGetFieldLong(hResult, 0, 26));
      m_ospfState = static_cast<OSPFInterfaceState>(DBGetFieldLong(hResult, 0, 27));
      m_stpPortState = static_cast<SpanningTreePortState>(DBGetFieldLong(hResult, 0, 28));

      TCHAR suffixText[128];
      DBGetField(hResult, 0, 29, suffixText, 128);
      Trim(suffixText);
      if (suffixText[0] == 0)
      {
         uint32_t suffix[16];
         size_t l = SnmpParseOID(suffixText, suffix, 16);
         if (l > 0)
         {
            m_ifTableSuffixLen = (int)l;
            m_ifTableSuffix = MemCopyArray(suffix, l);
         }
      }

      // Link interface to node
      if (!m_isDeleted)
      {
         shared_ptr<NetObj> object = FindObjectById(nodeId, OBJECT_NODE);
         if (object != nullptr)
         {
            object->addChild(self());
            addParent(object);
            m_zoneUIN = static_cast<Node*>(object.get())->getZoneUIN();
            success = true;
         }
         else
         {
            nxlog_write(NXLOG_ERROR, _T("Inconsistent database: interface %s [%u] linked to non-existent node [%u]"), m_name, m_id, nodeId);
         }
      }
      else
      {
         success = true;
      }
   }

   DBFreeResult(hResult);
	DBFreeStatement(hStmt);

	// Read VLANs
   hStmt = DBPrepare(hdb, _T("SELECT vlan_id FROM interface_vlan_list WHERE iface_id=? ORDER BY vlan_id"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
      hResult = DBSelectPrepared(hStmt);
      if (hResult != nullptr)
      {
         int count = DBGetNumRows(hResult);
         if (count > 0)
         {
            m_vlans = new IntegerArray<uint32_t>(count);
            for (int i = 0; i < count; i++)
            {
               m_vlans->add(DBGetFieldULong(hResult, i, 0));
            }
         }
         DBFreeResult(hResult);
      }
      DBFreeStatement(hStmt);
   }

   // Read IP addresses
   hStmt = DBPrepare(hdb, _T("SELECT ip_addr,ip_netmask FROM interface_address_list WHERE iface_id=?"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
      hResult = DBSelectPrepared(hStmt);
      if (hResult != nullptr)
      {
         int count = DBGetNumRows(hResult);
         for(int i = 0; i < count; i++)
         {
            InetAddress addr = DBGetFieldInetAddr(hResult, i, 0);
            addr.setMaskBits(DBGetFieldLong(hResult, i, 1));
            if (addr.isValid())
               m_ipAddressList.add(addr);
         }
         DBFreeResult(hResult);
      }
      DBFreeStatement(hStmt);
   }

   // Load access list
   loadACLFromDB(hdb);

	// Validate loopback flag
	if (m_type == IFTYPE_SOFTWARE_LOOPBACK)
   {
		m_flags |= IF_LOOPBACK;
   }
   else
   {
      int count = 0;
      for(int i = 0; i < m_ipAddressList.size(); i++)
      {
         if (m_ipAddressList.get(i).isLoopback())
            count++;
      }
      if ((count > 0) && (count == m_ipAddressList.size())) // all loopback addresses
   		m_flags |= IF_LOOPBACK;
      else
         m_flags &= ~IF_LOOPBACK;
   }

   return success;
}

/**
 * Save interface object to database
 */
bool Interface::saveToDatabase(DB_HANDLE hdb)
{
   bool success = super::saveToDatabase(hdb);
   if (success && (m_modified & MODIFY_INTERFACE_PROPERTIES))
   {
      uint32_t nodeId = getParentNodeId();

      static const TCHAR *columns[] = {
         _T("node_id"), _T("if_type"), _T("if_index"), _T("mac_addr"), _T("required_polls"), _T("bridge_port"),
         _T("phy_chassis"), _T("phy_module"), _T("phy_pic"), _T("phy_port"), _T("peer_node_id"), _T("peer_if_id"),
         _T("description"), _T("admin_state"), _T("oper_state"), _T("dot1x_pae_state"), _T("dot1x_backend_state"),
         _T("peer_proto"), _T("mtu"), _T("speed"), _T("parent_iface"), _T("iftable_suffix"), _T("last_known_oper_state"),
         _T("last_known_admin_state"), _T("if_alias"), _T("ospf_area"), _T("ospf_if_type"), _T("ospf_if_state"),
         _T("stp_port_state"), _T("if_name"), nullptr
      };

      DB_STATEMENT hStmt = DBPrepareMerge(hdb, _T("interfaces"), _T("id"), m_id, columns);
      if (hStmt != nullptr)
      {
         TCHAR ospfArea[16];

         lockProperties();

         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, nodeId);
         DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_type);
         DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_index);
         DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, m_macAddr);
         DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, m_requiredPollCount);
         DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, m_bridgePortNumber);
         DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, m_physicalLocation.chassis);
         DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, m_physicalLocation.module);
         DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, m_physicalLocation.pic);
         DBBind(hStmt, 10, DB_SQLTYPE_INTEGER, m_physicalLocation.port);
         DBBind(hStmt, 11, DB_SQLTYPE_INTEGER, m_peerNodeId);
         DBBind(hStmt, 12, DB_SQLTYPE_INTEGER, m_peerInterfaceId);
         DBBind(hStmt, 13, DB_SQLTYPE_VARCHAR, m_description, DB_BIND_STATIC, MAX_DB_STRING - 1);
         DBBind(hStmt, 14, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_adminState));
         DBBind(hStmt, 15, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_operState));
         DBBind(hStmt, 16, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_dot1xPaeAuthState));
         DBBind(hStmt, 17, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_dot1xBackendAuthState));
         DBBind(hStmt, 18, DB_SQLTYPE_INTEGER, static_cast<int32_t>(m_peerDiscoveryProtocol));
         DBBind(hStmt, 19, DB_SQLTYPE_INTEGER, m_mtu);
         DBBind(hStmt, 20, DB_SQLTYPE_BIGINT, m_speed);
         DBBind(hStmt, 21, DB_SQLTYPE_INTEGER, m_parentInterfaceId);
         if (m_ifTableSuffixLen > 0)
         {
            TCHAR buffer[128];
            DBBind(hStmt, 22, DB_SQLTYPE_VARCHAR, SnmpConvertOIDToText(m_ifTableSuffixLen, m_ifTableSuffix, buffer, 128), DB_BIND_TRANSIENT);
         }
         else
         {
            DBBind(hStmt, 22, DB_SQLTYPE_VARCHAR, _T(""), DB_BIND_STATIC);
         }
         DBBind(hStmt, 23, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_lastKnownOperState));
         DBBind(hStmt, 24, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_lastKnownAdminState));
         DBBind(hStmt, 25, DB_SQLTYPE_VARCHAR, m_ifAlias, DB_BIND_STATIC, MAX_DB_STRING - 1);
         if (m_flags & IF_OSPF_INTERFACE)
            DBBind(hStmt, 26, DB_SQLTYPE_VARCHAR, IpToStr(m_ospfArea, ospfArea), DB_BIND_STATIC);
         else
            DBBind(hStmt, 26, DB_SQLTYPE_VARCHAR, _T(""), DB_BIND_STATIC);
         DBBind(hStmt, 27, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_ospfType));
         DBBind(hStmt, 28, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_ospfState));
         DBBind(hStmt, 29, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_stpPortState));
         DBBind(hStmt, 30, DB_SQLTYPE_VARCHAR, m_ifName, DB_BIND_STATIC, MAX_DB_STRING - 1);
         DBBind(hStmt, 31, DB_SQLTYPE_INTEGER, m_id);

         success = DBExecute(hStmt);
         DBFreeStatement(hStmt);

         unlockProperties();
      }
      else
      {
         success = false;
      }

      // Save VLAN list
      if (success)
      {
         success = executeQueryOnObject(hdb, _T("DELETE FROM interface_vlan_list WHERE iface_id=?"));
         lockProperties();
         if (success && (m_vlans != nullptr) && !m_vlans->isEmpty())
         {
            hStmt = DBPrepare(hdb, _T("INSERT INTO interface_vlan_list (iface_id,vlan_id) VALUES (?,?)"), m_vlans->size() > 1);
            if (hStmt != nullptr)
            {
               DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
               for(int i = 0; (i < m_vlans->size()) && success; i++)
               {
                  DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_vlans->get(i));
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

      // Save IP addresses
      if (success)
      {
         success = executeQueryOnObject(hdb, _T("DELETE FROM interface_address_list WHERE iface_id = ?"));
         lockProperties();
         if (success && (m_ipAddressList.size() > 0))
         {
            hStmt = DBPrepare(hdb, _T("INSERT INTO interface_address_list (iface_id,ip_addr,ip_netmask) VALUES (?,?,?)"), m_ipAddressList.size() > 1);
            if (hStmt != nullptr)
            {
               DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
               const ObjectArray<InetAddress>& list = m_ipAddressList.getList();
               for(int i = 0; (i < m_ipAddressList.size()) && success; i++)
               {
                  InetAddress *a = list.get(i);
                  TCHAR buffer[64];
                  DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, a->toString(buffer), DB_BIND_STATIC);
                  DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, a->getMaskBits());
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
   }
   else
   {
      success = true;
   }
   return success;
}

/**
 * Delete interface object from database
 */
bool Interface::deleteFromDatabase(DB_HANDLE hdb)
{
   bool success = super::deleteFromDatabase(hdb);
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM interfaces WHERE id=?"));
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM interface_address_list WHERE iface_id=?"));
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM interface_vlan_list WHERE iface_id=?"));
   return success;
}

/**
 * Perform status poll on interface
 */
void Interface::statusPoll(ClientSession *session, uint32_t rqId, ObjectQueue<Event> *eventQueue, Cluster *cluster, SNMP_Transport *snmpTransport, uint32_t nodeIcmpProxy)
{
   if (IsShutdownInProgress())
      return;

   m_pollRequestor = session;
   m_pollRequestId = rqId;

   shared_ptr<Node> node = getParentNode();
   if (node == nullptr)
   {
      m_status = STATUS_UNKNOWN;
      return;     // Cannot find parent node, which is VERY strange
   }

   sendPollerMsg(_T("   Starting status poll on interface %s\r\n"), m_name);
   sendPollerMsg(_T("      Current interface status is %s\r\n"), GetStatusAsText(m_status, true));

	InterfaceAdminState adminState = IF_ADMIN_STATE_UNKNOWN;
	InterfaceOperState operState = IF_OPER_STATE_UNKNOWN;
   bool needPoll = true;

   // Poll interface using different methods
   if (!(m_flags & IF_DISABLE_AGENT_STATUS_POLL) && (node->getCapabilities() & NC_IS_NATIVE_AGENT) &&
       (!(node->getFlags() & NF_DISABLE_NXCP)) && (!(node->getState() & NSF_AGENT_UNREACHABLE)))
   {
      sendPollerMsg(_T("      Retrieving interface status from NetXMS agent\r\n"));
      node->getInterfaceStatusFromAgent(m_index, &adminState, &operState);
		DbgPrintf(7, _T("Interface::StatusPoll(%d,%s): new state from NetXMS agent: adinState=%d operState=%d"), m_id, m_name, adminState, operState);
		if ((adminState != IF_ADMIN_STATE_UNKNOWN) && (operState != IF_OPER_STATE_UNKNOWN))
		{
			sendPollerMsg(POLLER_INFO _T("      Interface status retrieved from NetXMS agent\r\n"));
         needPoll = false;
		}
		else
		{
			sendPollerMsg(POLLER_WARNING _T("      Unable to retrieve interface status from NetXMS agent\r\n"));
		}
   }

   if (needPoll && !(m_flags & IF_DISABLE_SNMP_STATUS_POLL) && (node->getCapabilities() & NC_IS_SNMP) &&
       (!(node->getFlags() & NF_DISABLE_SNMP)) && (!(node->getState() & NSF_SNMP_UNREACHABLE)) &&
		 (snmpTransport != nullptr))
   {
      sendPollerMsg(_T("      Retrieving interface status from SNMP agent\r\n"));
      node->getInterfaceStatusFromSNMP(snmpTransport, m_index, m_ifTableSuffixLen, m_ifTableSuffix, &adminState, &operState);
		DbgPrintf(7, _T("Interface::StatusPoll(%d,%s): new state from SNMP: adminState=%d operState=%d"), m_id, m_name, adminState, operState);
		if ((adminState != IF_ADMIN_STATE_UNKNOWN) && (operState != IF_OPER_STATE_UNKNOWN))
		{
			sendPollerMsg(POLLER_INFO _T("      Interface status retrieved from SNMP agent\r\n"));
         needPoll = false;
		}
		else
		{
			sendPollerMsg(POLLER_WARNING _T("      Unable to retrieve interface status from SNMP agent\r\n"));
		}
   }

   if (needPoll && !(m_flags & IF_DISABLE_ICMP_STATUS_POLL))
   {
		// Ping cannot be used for cluster sync interfaces
      if ((node->getFlags() & NF_DISABLE_ICMP) || isLoopback() || !m_ipAddressList.hasValidUnicastAddress())
      {
			// Interface doesn't have an IP address, so we can't ping it
			sendPollerMsg(POLLER_WARNING _T("      Interface status cannot be determined\r\n"));
			nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 7, _T("Interface::StatusPoll(%d,%s): cannot use ping for status check"), m_id, m_name);
      }
      else
      {
         icmpStatusPoll(rqId, nodeIcmpProxy, cluster, &adminState, &operState);
      }
   }

	// Calculate interface object status based on admin state, oper state, and expected state
   int oldStatus = m_status;
	int newStatus;
	int expectedState = (m_flags & IF_EXPECTED_STATE_MASK) >> 28;
	switch(adminState)
	{
		case IF_ADMIN_STATE_UP:
		case IF_ADMIN_STATE_UNKNOWN:
			switch(operState)
			{
				case IF_OPER_STATE_UP:
		         if (expectedState == IF_EXPECTED_STATE_AUTO)
		         {
		            nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, _T("Interface::StatusPoll(%d,%s): auto expected state changed to UP"), m_id, m_name);
		            expectedState = IF_EXPECTED_STATE_UP;
		            setExpectedState(expectedState);
		         }
					newStatus = ((expectedState == IF_EXPECTED_STATE_DOWN) ? STATUS_CRITICAL : STATUS_NORMAL);
					break;
				case IF_OPER_STATE_DOWN:
               if (expectedState == IF_EXPECTED_STATE_AUTO)
               {
                  nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, _T("Interface::StatusPoll(%d,%s): auto expected state changed to DOWN"), m_id, m_name);
                  expectedState = IF_EXPECTED_STATE_DOWN;
                  setExpectedState(expectedState);
               }
					newStatus = (expectedState == IF_EXPECTED_STATE_UP) ? STATUS_CRITICAL : STATUS_NORMAL;
					break;
				case IF_OPER_STATE_TESTING:
					newStatus = STATUS_TESTING;
					break;
				case IF_OPER_STATE_DORMANT:
               newStatus = (expectedState == IF_EXPECTED_STATE_UP) ? STATUS_MINOR : STATUS_NORMAL;
               break;
            case IF_OPER_STATE_NOT_PRESENT:
               newStatus = STATUS_DISABLED;
               break;
				default:
					newStatus = STATUS_UNKNOWN;
					break;
			}
			break;
		case IF_ADMIN_STATE_DOWN:
         if (expectedState == IF_EXPECTED_STATE_AUTO)
         {
            nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, _T("Interface::StatusPoll(%d,%s): auto expected state changed to DOWN"), m_id, m_name);
            expectedState = IF_EXPECTED_STATE_DOWN;
            setExpectedState(expectedState);
         }
			newStatus = STATUS_DISABLED;
			break;
		case IF_ADMIN_STATE_TESTING:
			newStatus = STATUS_TESTING;
			break;
		default:
			newStatus = STATUS_UNKNOWN;
			break;
	}

	// Check STP state
	if ((node->getCapabilities() & NC_IS_BRIDGE) && isPhysicalPort() && (snmpTransport != nullptr))
	{
		nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, _T("StatusPoll(%s): Checking Spanning Tree state for interface %s"), node->getName(), m_name);
		stpStatusPoll(rqId, snmpTransport, *node);
	}

   // Check 802.1x state
   if ((node->getCapabilities() & NC_IS_8021X) && isPhysicalPort() && (snmpTransport != nullptr) && node->is8021xPollingEnabled())
   {
      nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, _T("StatusPoll(%s): Checking 802.1x state for interface %s"), node->getName(), m_name);
      paeStatusPoll(rqId, snmpTransport, *node);
      if ((m_dot1xPaeAuthState == PAE_STATE_FORCE_UNAUTH) && (newStatus < STATUS_MAJOR))
         newStatus = STATUS_MAJOR;
   }

	// Reset status to unknown if node has known network connectivity problems
	if ((newStatus == STATUS_CRITICAL) && (node->getState() & DCSF_NETWORK_PATH_PROBLEM))
	{
		newStatus = STATUS_UNKNOWN;
		nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("StatusPoll(%s): Status for interface %s reset to UNKNOWN"), node->getName(), m_name);
	}

	if (newStatus == m_pendingStatus)
	{
		m_statusPollCount++;
	}
	else
	{
		m_pendingStatus = newStatus;
		m_statusPollCount = 1;
	}

	if (operState == m_pendingOperState)
	{
	   m_operStatePollCount++;
	}
	else
	{
	   m_pendingOperState = operState;
	   m_operStatePollCount = 1;
	}

	int requiredPolls = (m_requiredPollCount > 0) ? m_requiredPollCount :
	                    ((node->getRequiredPollCount() > 0) ? node->getRequiredPollCount() : g_requiredPolls);
	sendPollerMsg(_T("      Interface is %s for %d poll%s (%d poll%s required for status change)\r\n"),
	      GetStatusAsText(newStatus, true), m_statusPollCount, (m_statusPollCount == 1) ? _T("") : _T("s"),
	      requiredPolls, (requiredPolls == 1) ? _T("") : _T("s"));
	nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 7, _T("Interface::StatusPoll(%d,%s): newStatus=%d oldStatus=%d pollCount=%d requiredPolls=%d"),
	      m_id, m_name, newStatus, oldStatus, m_statusPollCount, requiredPolls);

   if ((operState != m_confirmedOperState) && (m_operStatePollCount >= requiredPolls))
   {
      DbgPrintf(6, _T("Interface::StatusPoll(%d,%s): confirmedOperState=%d pollCount=%d requiredPolls=%d"),
                m_id, m_name, (int)operState, m_operStatePollCount, requiredPolls);
      m_confirmedOperState = operState;
   }

   if ((newStatus != oldStatus) && (m_statusPollCount >= requiredPolls) && (expectedState != IF_EXPECTED_STATE_IGNORE))
   {
		static uint32_t statusToEvent[] =
		{
			EVENT_INTERFACE_UP,       // Normal
			EVENT_INTERFACE_UP,       // Warning
			EVENT_INTERFACE_UP,       // Minor
			EVENT_INTERFACE_DOWN,     // Major
			EVENT_INTERFACE_DOWN,     // Critical
			EVENT_INTERFACE_UNKNOWN,  // Unknown
			EVENT_INTERFACE_UNKNOWN,  // Unmanaged
			EVENT_INTERFACE_DISABLED, // Disabled
			EVENT_INTERFACE_TESTING   // Testing
		};
		static uint32_t statusToEventInverted[] =
		{
			EVENT_INTERFACE_EXPECTED_DOWN, // Normal
			EVENT_INTERFACE_EXPECTED_DOWN, // Warning
			EVENT_INTERFACE_EXPECTED_DOWN, // Minor
			EVENT_INTERFACE_UNEXPECTED_UP, // Major
			EVENT_INTERFACE_UNEXPECTED_UP, // Critical
			EVENT_INTERFACE_UNKNOWN,  // Unknown
			EVENT_INTERFACE_UNKNOWN,  // Unmanaged
			EVENT_INTERFACE_DISABLED, // Disabled
			EVENT_INTERFACE_TESTING   // Testing
		};

		nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 7, _T("Interface::StatusPoll(%d,%s): status changed from %d to %d"), m_id, m_name, m_status, newStatus);
		m_status = newStatus;
		m_pendingStatus = -1;	// Invalidate pending status
      if (!m_isSystem)
      {
		   sendPollerMsg(_T("      Interface status changed to %s\r\n"), GetStatusAsText(m_status, true));

         // Post system event if it was not already sent before unknown state
         if ((m_lastKnownOperState == IF_OPER_STATE_UNKNOWN || m_lastKnownOperState != static_cast<int16_t>(operState)) ||
         (m_lastKnownAdminState == IF_ADMIN_STATE_UNKNOWN || m_lastKnownAdminState != static_cast<int16_t>(adminState)))
         {
            const InetAddress& addr = m_ipAddressList.getFirstUnicastAddress();
            EventBuilder((expectedState == IF_EXPECTED_STATE_DOWN) ? statusToEventInverted[m_status] : statusToEvent[m_status], node->getId())
               .param(_T("interfaceObjectId"), m_id)
               .param(_T("interfaceName"), m_name)
               .param(_T("interfaceIpAddress"), addr)
               .param(_T("interfaceNetMask"), addr.getMaskBits())
               .param(_T("interfaceIndex"), m_index)
               .post(eventQueue);
         }
         if (static_cast<int16_t>(operState) != IF_OPER_STATE_UNKNOWN)
            m_lastKnownOperState = static_cast<int16_t>(operState);
         if (static_cast<int16_t>(adminState) != IF_ADMIN_STATE_UNKNOWN)
            m_lastKnownAdminState = static_cast<int16_t>(adminState);
      }
   }
	else if (expectedState == IF_EXPECTED_STATE_IGNORE)
	{
		m_status = (newStatus <= STATUS_CRITICAL) ? STATUS_NORMAL : newStatus;
		if (m_status != oldStatus)
			m_pendingStatus = -1;	// Invalidate pending status
	}

	lockProperties();
	if ((m_status != oldStatus) || (adminState != (int)m_adminState) || (operState != (int)m_operState))
	{
		m_adminState = static_cast<int16_t>(adminState);
		m_operState = static_cast<int16_t>(operState);
		setModified(MODIFY_INTERFACE_PROPERTIES);
	}
	unlockProperties();

	sendPollerMsg(_T("      Interface status after poll is %s\r\n"), GetStatusAsText(m_status, true));
	sendPollerMsg(_T("   Finished status poll on interface %s\r\n"), m_name);
}

/**
 * Do ICMP part of status poll
 */
void Interface::icmpStatusPoll(uint32_t rqId, uint32_t nodeIcmpProxy, Cluster *cluster, InterfaceAdminState *adminState, InterfaceOperState *operState)
{
   // Use ICMP ping as a last option
	uint32_t icmpProxy = nodeIcmpProxy;

	if (IsZoningEnabled() && (m_zoneUIN != 0) && (icmpProxy == 0))
	{
		shared_ptr<Zone> zone = FindZoneByUIN(m_zoneUIN);
		if (zone != nullptr)
		{
			icmpProxy = zone->getProxyNodeId(getParentNode().get());
		}
	}

	if (icmpProxy != 0)
	{
		sendPollerMsg(_T("      Starting ICMP ping via proxy\r\n"));
		nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 7, _T("Interface::StatusPoll(%d,%s): ping via proxy [%u]"), m_id, m_name, icmpProxy);
		shared_ptr<Node> proxyNode = static_pointer_cast<Node>(g_idxNodeById.get(icmpProxy));
		if ((proxyNode != nullptr) && proxyNode->isNativeAgent() && !proxyNode->isDown())
		{
		   nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 7, _T("Interface::StatusPoll(%d,%s): proxy node found: %s"), m_id, m_name, proxyNode->getName());
			shared_ptr<AgentConnection> conn = proxyNode->createAgentConnection();
			if (conn != nullptr)
			{
				TCHAR parameter[128], buffer[64];
            const ObjectArray<InetAddress>& list = m_ipAddressList.getList();
            long value = -1;
            for(int i = 0; (i < list.size()) && ((value == 10000) || (value == -1)); i++)
            {
               const InetAddress *a = list.get(i);
               if (a->isValidUnicast() && ((cluster == nullptr) || !cluster->isSyncAddr(*a)))
               {
				      _sntprintf(parameter, 128, _T("Icmp.Ping(%s)"), a->toString(buffer));
				      if (conn->getParameter(parameter, buffer, 64) == ERR_SUCCESS)
				      {
				         nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 7, _T("Interface::StatusPoll(%d,%s): proxy response: \"%s\""), m_id, m_name, buffer);
					      TCHAR *eptr;
					      value = _tcstol(buffer, &eptr, 10);
                     if (*eptr != 0)
                     {
                        value = -1;
                     }
                  }
               }
            }

            nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 7, _T("Interface::StatusPoll(%d,%s): response time %d"), m_id, m_name, (int)value);
				if (value >= 0)
				{
					if (value < 10000)
					{
						*adminState = IF_ADMIN_STATE_UP;
						*operState = IF_OPER_STATE_UP;
					}
					else
					{
						*adminState = IF_ADMIN_STATE_UNKNOWN;
						*operState = IF_OPER_STATE_DOWN;
					}
				}
			}
			else
			{
			   nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 7, _T("Interface::StatusPoll(%d,%s): cannot connect to agent on proxy node"), m_id, m_name);
				sendPollerMsg(POLLER_ERROR _T("      Unable to establish connection with proxy node\r\n"));
			}
		}
		else
		{
		   nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 7, _T("Interface::StatusPoll(%d,%s): proxy node not available"), m_id, m_name);
			sendPollerMsg(POLLER_ERROR _T("      ICMP proxy not available\r\n"));
		}
	}
	else	// not using ICMP proxy
	{
		sendPollerMsg(_T("      Starting ICMP ping\r\n"));
      const ObjectArray<InetAddress>& list = m_ipAddressList.getList();
      uint32_t dwPingStatus = ICMP_TIMEOUT;
      for(int i = 0; (i < list.size()) && (dwPingStatus != ICMP_SUCCESS); i++)
      {
         const InetAddress *a = list.get(i);
         if (a->isValidUnicast() && ((cluster == nullptr) || !cluster->isSyncAddr(*a)))
         {
            nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 7, _T("Interface::StatusPoll(%d,%s): calling IcmpPing(%s,3,%d,%d)"),
               m_id, m_name, a->toString().cstr(), g_icmpPingTimeout, g_icmpPingSize);
		      dwPingStatus = IcmpPing(*a, 3, g_icmpPingTimeout, nullptr, g_icmpPingSize, false);
         }
      }
		if (dwPingStatus == ICMP_SUCCESS)
		{
			*adminState = IF_ADMIN_STATE_UP;
			*operState = IF_OPER_STATE_UP;
		}
		else
		{
			*adminState = IF_ADMIN_STATE_UNKNOWN;
			*operState = IF_OPER_STATE_DOWN;
		}
		nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 7, _T("Interface::StatusPoll(%d,%s): ping result %d, adminState=%d, operState=%d"), m_id, m_name, dwPingStatus, *adminState, *operState);
	}
}

/**
 * Spanning tree status poll
 */
void Interface::stpStatusPoll(uint32_t rqId, SNMP_Transport *transport, const Node& node)
{
   sendPollerMsg(_T("      Checking port STP state...\r\n"));

   int32_t v = static_cast<int32_t>(SpanningTreePortState::UNKNOWN);
   uint32_t oid[12] = { 1, 3, 6, 1, 2, 1, 17, 2, 15, 1, 3, m_bridgePortNumber };
   SnmpGet(transport->getSnmpVersion(), transport, nullptr, oid, 12, &v, sizeof(int32_t), 0);
   SpanningTreePortState stpState = static_cast<SpanningTreePortState>(v);

   if (m_stpPortState != stpState)
   {
      sendPollerMsg(_T("      Port Spanning Tree state changed to %s\r\n"), STPPortStateToText(stpState));
      nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, _T("Interface::stpStatusPoll(%s [%u]): Port STP state changed to %s"), m_name, m_id, STPPortStateToText(stpState));

      lockProperties();
      SpanningTreePortState oldState = m_stpPortState;
      m_stpPortState = stpState;
      setModified(MODIFY_INTERFACE_PROPERTIES);
      unlockProperties();

      if (!m_isSystem)
      {
         EventBuilder(EVENT_IF_STP_STATE_CHANGED, node)
            .param(_T("ifIndex"), m_index)
            .param(_T("ifName"), m_name)
            .param(_T("oldState"), static_cast<int>(oldState))
            .param(_T("oldStateText"), STPPortStateToText(oldState))
            .param(_T("newState"), static_cast<int>(stpState))
            .param(_T("newStateText"), STPPortStateToText(stpState))
            .post();
      }
   }
}

/**
 * PAE (802.1x) status poll
 */
void Interface::paeStatusPoll(uint32_t rqId, SNMP_Transport *transport, const Node& node)
{
	static const TCHAR *paeStateText[] =
	{
		_T("UNKNOWN"),
		_T("INITIALIZE"),
		_T("DISCONNECTED"),
		_T("CONNECTING"),
		_T("AUTHENTICATING"),
		_T("AUTHENTICATED"),
		_T("ABORTING"),
		_T("HELD"),
		_T("FORCE AUTH"),
		_T("FORCE UNAUTH"),
		_T("RESTART")
	};
	static const TCHAR *backendStateText[] =
	{
		_T("UNKNOWN"),
		_T("REQUEST"),
		_T("RESPONSE"),
		_T("SUCCESS"),
		_T("FAIL"),
		_T("TIMEOUT"),
		_T("IDLE"),
		_T("INITIALIZE"),
		_T("IGNORE")
	};
#define PAE_STATE_TEXT(x) ((((int)(x) <= PAE_STATE_RESTART) && ((int)(x) >= 0)) ? paeStateText[(int)(x)] : paeStateText[0])
#define BACKEND_STATE_TEXT(x) ((((int)(x) <= BACKEND_STATE_IGNORE) && ((int)(x) >= 0)) ? backendStateText[(int)(x)] : backendStateText[0])

   sendPollerMsg(_T("      Checking port 802.1x status...\r\n"));

	TCHAR oid[256];
	int32_t paeState = PAE_STATE_UNKNOWN, backendState = BACKEND_STATE_UNKNOWN;
	bool modified = false;

	_sntprintf(oid, 256, _T(".1.0.8802.1.1.1.1.2.1.1.1.%d"), m_index);
	SnmpGet(transport->getSnmpVersion(), transport, oid, nullptr, 0, &paeState, sizeof(int32_t), 0);

	_sntprintf(oid, 256, _T(".1.0.8802.1.1.1.1.2.1.1.2.%d"), m_index);
	SnmpGet(transport->getSnmpVersion(), transport, oid, nullptr, 0, &backendState, sizeof(int32_t), 0);

	if (m_dot1xPaeAuthState != (WORD)paeState)
	{
	   sendPollerMsg(_T("      Port PAE state changed to %s\r\n"), PAE_STATE_TEXT(paeState));
      nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, _T("Interface::paeStatusPoll(%s [%u]): Port PAE state changed to %s"), m_name, m_id, PAE_STATE_TEXT(paeState));
		modified = true;
      if (!m_isSystem)
      {
         EventBuilder(EVENT_8021X_PAE_STATE_CHANGED, node.getId())
            .param(_T("newPaeStateCode"), paeState)
            .param(_T("newPaeStateText"), PAE_STATE_TEXT(paeState))
            .param(_T("oldPaeStateCode"), static_cast<uint32_t>(m_dot1xPaeAuthState))
            .param(_T("oldPaeStateText"), PAE_STATE_TEXT(m_dot1xPaeAuthState))
            .param(_T("interfaceIndex"), m_id)
            .param(_T("interfaceName"), m_name)
            .post();

		   if (paeState == PAE_STATE_FORCE_UNAUTH)
		   {
            EventBuilder(EVENT_8021X_PAE_FORCE_UNAUTH, node.getId())
               .param(_T("interfaceIndex"), m_id)
               .param(_T("interfaceName"), m_name)
               .post();
		   }
      }
	}

	if (m_dot1xBackendAuthState != static_cast<int16_t>(backendState))
	{
	   sendPollerMsg(_T("      Port backend state changed to %s\r\n"), BACKEND_STATE_TEXT(backendState));
	   nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, _T("Interface::paeStatusPoll(%s [%u]): Port backend state changed to %s"),
	         m_name, m_id, BACKEND_STATE_TEXT(backendState));
		modified = true;
      if (!m_isSystem)
      {
         EventBuilder(EVENT_8021X_BACKEND_STATE_CHANGED, node.getId())
            .param(_T("newBackendStateCode"), backendState)
            .param(_T("newBackendStateText"), BACKEND_STATE_TEXT(backendState))
            .param(_T("oldBackendStateCode"), (UINT32)m_dot1xBackendAuthState)
            .param(_T("oldBackendStateText"), BACKEND_STATE_TEXT(m_dot1xBackendAuthState))
            .param(_T("interfaceIndex"), m_id)
            .param(_T("interfaceName"), m_name)
            .post();
		   if (backendState == BACKEND_STATE_FAIL)
		   {
            EventBuilder(EVENT_8021X_AUTH_FAILED, node.getId())
               .param(_T("interfaceIndex"), m_id)
               .param(_T("interfaceName"), m_name)
               .post();
		   }
		   else if (backendState == BACKEND_STATE_TIMEOUT)
		   {
            EventBuilder(EVENT_8021X_AUTH_TIMEOUT, node.getId())
               .param(_T("interfaceIndex"), m_id)
               .param(_T("interfaceName"), m_name)
               .post();
		   }
      }
	}

	if (modified)
	{
		lockProperties();
		m_dot1xPaeAuthState = static_cast<int16_t>(paeState);
		m_dot1xBackendAuthState = static_cast<int16_t>(backendState);
		setModified(MODIFY_INTERFACE_PROPERTIES);
		unlockProperties();
	}
}

/**
 * Create NXCP message with object's data
 */
void Interface::fillMessageInternal(NXCPMessage *msg, uint32_t userId)
{
   super::fillMessageInternal(msg, userId);

   m_ipAddressList.fillMessage(msg, VID_IP_ADDRESS_LIST_BASE, VID_IP_ADDRESS_COUNT);
   msg->setField(VID_IF_INDEX, m_index);
   msg->setField(VID_IF_TYPE, m_type);
   msg->setField(VID_MTU, m_mtu);
   msg->setField(VID_SPEED, m_speed);
   msg->setField(VID_PHY_CHASSIS, m_physicalLocation.chassis);
   msg->setField(VID_PHY_MODULE, m_physicalLocation.module);
   msg->setField(VID_PHY_PIC, m_physicalLocation.pic);
   msg->setField(VID_PHY_PORT, m_physicalLocation.port);
   msg->setField(VID_MAC_ADDR, m_macAddr);
	msg->setField(VID_REQUIRED_POLLS, static_cast<int16_t>(m_requiredPollCount));
	msg->setField(VID_PEER_NODE_ID, m_peerNodeId);
	msg->setField(VID_PEER_INTERFACE_ID, m_peerInterfaceId);
	msg->setField(VID_PEER_PROTOCOL, static_cast<int16_t>(m_peerDiscoveryProtocol));
	msg->setField(VID_DESCRIPTION, m_description);
   msg->setField(VID_IF_ALIAS, m_ifAlias);
	msg->setField(VID_ADMIN_STATE, m_adminState);
	msg->setField(VID_OPER_STATE, m_operState);
   msg->setField(VID_STP_PORT_STATE, static_cast<uint16_t>(m_stpPortState));
	msg->setField(VID_DOT1X_PAE_STATE, m_dot1xPaeAuthState);
	msg->setField(VID_DOT1X_BACKEND_STATE, m_dot1xBackendAuthState);
	msg->setField(VID_ZONE_UIN, m_zoneUIN);
   msg->setFieldFromInt32Array(VID_IFTABLE_SUFFIX, m_ifTableSuffixLen, m_ifTableSuffix);
   msg->setField(VID_PARENT_INTERFACE, m_parentInterfaceId);
   msg->setFieldFromInt32Array(VID_VLAN_LIST, m_vlans);
   msg->setField(VID_OSPF_AREA, InetAddress(m_ospfArea));
   msg->setField(VID_OSPF_INTERFACE_TYPE, static_cast<uint16_t>(m_ospfType));
   msg->setField(VID_OSPF_INTERFACE_STATE, static_cast<uint16_t>(m_ospfState));
}

/**
 * Modify object from message
 */
uint32_t Interface::modifyFromMessageInternal(const NXCPMessage& msg)
{
   // Number of required polls
   if (msg.isFieldExist(VID_REQUIRED_POLLS))
      m_requiredPollCount = msg.getFieldAsInt16(VID_REQUIRED_POLLS);

	// Expected interface state
	if (msg.isFieldExist(VID_EXPECTED_STATE))
		setExpectedStateInternal(msg.getFieldAsInt16(VID_EXPECTED_STATE));

   return super::modifyFromMessageInternal(msg);
}

/**
 * Set expected state for interface
 */
void Interface::setExpectedStateInternal(int state)
{
   static const uint32_t eventCode[] = { EVENT_IF_EXPECTED_STATE_UP, EVENT_IF_EXPECTED_STATE_DOWN, EVENT_IF_EXPECTED_STATE_IGNORE };

	int curr = (m_flags & IF_EXPECTED_STATE_MASK) >> 28;
	if (curr != state)
	{
      m_flags &= ~IF_EXPECTED_STATE_MASK;
      m_flags |= (UINT32)state << 28;
      setModified(MODIFY_COMMON_PROPERTIES);
      if (state != IF_EXPECTED_STATE_AUTO)
      {
         // Node ID can be 0 when interface object is just created and not attached to node object yet
         // Expected state change event is meaningless in that case
         uint32_t nodeId = getParentNodeId();
         if (nodeId != 0)
            EventBuilder(eventCode[state], nodeId)
               .param(_T("interfaceIndex"), m_index)
               .param(_T("intefaceName"), m_name)
               .post();
      }
	}
}

/**
 * Set "exclude from topology" flag
 */
void Interface::setExcludeFromTopology(bool excluded)
{
   lockProperties();
   if (excluded)
      m_flags |= IF_EXCLUDE_FROM_TOPOLOGY;
   else
      m_flags &= ~IF_EXCLUDE_FROM_TOPOLOGY;
   setModified(MODIFY_COMMON_PROPERTIES);
   unlockProperties();
}

/**
 * Set "include in ICMP poll" flag
 */
void Interface::setIncludeInIcmpPoll(bool included)
{
   lockProperties();
   if (included)
      m_flags |= IF_INCLUDE_IN_ICMP_POLL;
   else
      m_flags &= ~IF_INCLUDE_IN_ICMP_POLL;
   setModified(MODIFY_COMMON_PROPERTIES);
   unlockProperties();
}

/**
 * Wake up node bound to this interface by sending magic packet
 */
uint32_t Interface::wakeUp()
{
   uint32_t rcc = RCC_NO_MAC_ADDRESS;

   if (m_macAddr.isValid())
   {
      const InetAddress addr = m_ipAddressList.getFirstUnicastAddressV4();
      if (addr.isValid())
      {
         InetAddress destAddr(addr.getAddressV4() | ~(0xFFFFFFFF << (32 - addr.getMaskBits())));
         if (SendMagicPacket(destAddr, m_macAddr, 5))
            rcc = RCC_SUCCESS;
         else
            rcc = RCC_COMM_FAILURE;
      }
   }
   return rcc;
}

/**
 * Get interface's parent node
 */
shared_ptr<Node> Interface::getParentNode() const
{
   shared_ptr<Node> node;

   readLockParentList();
   for(int i = 0; i < getParentList().size(); i++)
      if (getParentList().get(i)->getObjectClass() == OBJECT_NODE)
      {
         node = static_pointer_cast<Node>(getParentList().getShared(i));
         break;
      }
   unlockParentList();
   return node;
}

/**
 * Get ID of parent node object
 */
uint32_t Interface::getParentNodeId() const
{
   shared_ptr<Node> node = getParentNode();
   return (node != nullptr) ? node->getId() : 0;
}

/**
 * Get name of parent node object
 */
String Interface::getParentNodeName() const
{
   shared_ptr<Node> node = getParentNode();
   return (node != nullptr) ? String(node->getName()) : String(_T("<none>"));
}

/**
 * Update zone UIN. New zone UIN taken from parent node.
 */
void Interface::updateZoneUIN()
{
   shared_ptr<Node> node = getParentNode();
   if (node == nullptr)
      return;

   if (isExcludedFromTopology())
   {
      lockProperties();
      m_zoneUIN = node->getZoneUIN();
      setModified(MODIFY_INTERFACE_PROPERTIES);
      unlockProperties();
      return;
   }

   // Unregister from old zone
   shared_ptr<Zone> zone = FindZoneByUIN(m_zoneUIN);
   if (zone != nullptr)
      zone->removeFromIndex(*this);

   int32_t newZoneUIN = node->getZoneUIN();
   lockProperties();
   m_zoneUIN = newZoneUIN;
   setModified(MODIFY_INTERFACE_PROPERTIES);
   unlockProperties();

   // Register in new zone
   zone = FindZoneByUIN(newZoneUIN);
   if (zone != nullptr)
      zone->addToIndex(self());
}

/**
 * Handler for object deletion notification
 */
void Interface::onObjectDelete(const NetObj& object)
{
   uint32_t objectId = object.getId();
   lockProperties();
	if ((m_peerNodeId == objectId) || (m_peerInterfaceId == objectId))
	{
		m_peerNodeId = 0;
		m_peerInterfaceId = 0;
		setModified(MODIFY_INTERFACE_PROPERTIES);
	}
   unlockProperties();
	super::onObjectDelete(object);
}

/**
 * Prepare object for deletion
 */
void Interface::prepareForDeletion()
{
   DeleteObjectFromPhysicalLinks(m_id);
}

/**
 * Set peer information
 */
void Interface::setPeer(Node *node, Interface *iface, LinkLayerProtocol protocol, bool reflection)
{
   lockProperties();

   if ((m_peerNodeId == node->getId()) && (m_peerInterfaceId == iface->getId()) && (m_peerDiscoveryProtocol == protocol))
   {
      if ((m_flags & IF_PEER_REFLECTION) && !reflection)
      {
         // set peer information as confirmed
         m_flags &= ~IF_PEER_REFLECTION;
         setModified(MODIFY_COMMON_PROPERTIES);
      }
   }
   else
   {
      m_peerNodeId = node->getId();
      m_peerInterfaceId = iface->getId();
      m_peerDiscoveryProtocol = protocol;
      if (reflection)
         m_flags |= IF_PEER_REFLECTION;
      else
         m_flags &= ~IF_PEER_REFLECTION;
      setModified(MODIFY_INTERFACE_PROPERTIES | MODIFY_COMMON_PROPERTIES);
      if (!m_isSystem)
      {
         EventBuilder(EVENT_IF_PEER_CHANGED, getParentNodeId())
            .param(_T("localInterfaceObjectId"), m_id)
            .param(_T("localInterfaceIndex"), m_index)
            .param(_T("localInterfaceName"), m_name)
            .param(_T("localInterfaceIpAddress"), m_ipAddressList.getFirstUnicastAddress())
            .param(_T("localInterfaceMacAddress"), m_macAddr)
            .param(_T("peerNodeObjectId"), node->getId())
            .param(_T("peerNodeName"), node->getName())
            .param(_T("peerInterfaceObjectId"), iface->getId())
            .param(_T("peerInterfaceIndex"), iface->getIfIndex())
            .param(_T("peerInterfaceName"), iface->getName())
            .param(_T("peerInterfaceIpAddress"), iface->getIpAddressList()->getFirstUnicastAddress())
            .param(_T("peerInterfaceMacAddress"), iface->getMacAddr())
            .param(_T("discoveryProtocol"), protocol)
            .post();
      }
   }

   unlockProperties();
}

/**
 * Set MAC address for interface
 */
void Interface::setMacAddr(const MacAddress& macAddr, bool updateMacDB)
{
   lockProperties();
   if (updateMacDB)
      MacDbRemoveInterface(*this);
   m_macAddr = macAddr;
   if (updateMacDB)
      MacDbAddInterface(self());
   setModified(MODIFY_INTERFACE_PROPERTIES);
   unlockProperties();
}

/**
 * Set IP address (should be used only for fake interfaces with single IP)
 */
void Interface::setIpAddress(const InetAddress& addr)
{
   lockProperties();
   if (m_ipAddressList.size() == 1)
   {
      UpdateInterfaceIndex(m_ipAddressList.get(0), addr, self());
      m_ipAddressList.clear();
      m_ipAddressList.add(addr);
      setModified(MODIFY_INTERFACE_PROPERTIES);
   }
   unlockProperties();
}

/**
 * Get first usable IP address
 */
InetAddress Interface::getFirstIpAddress() const
{
   lockProperties();
   const InetAddress& t = m_ipAddressList.getFirstUnicastAddress();
   InetAddress a = t.isValid() ? t : m_ipAddressList.get(0);
   unlockProperties();
   return a;
}

/**
 * Add IP address
 */
void Interface::addIpAddress(const InetAddress& addr)
{
   lockProperties();
   m_ipAddressList.add(addr);
   setModified(MODIFY_INTERFACE_PROPERTIES);
   unlockProperties();
   if (!isExcludedFromTopology())
   {
		if (IsZoningEnabled())
		{
			shared_ptr<Zone> zone = FindZoneByUIN(m_zoneUIN);
			if (zone != nullptr)
			{
				zone->addToIndex(addr, self());
			}
			else
			{
				nxlog_write(NXLOG_WARNING, _T("Cannot find zone object with UIN %u for interface object %s [%u]"), m_zoneUIN, m_name, m_id);
			}
		}
		else
		{
         g_idxInterfaceByAddr.put(addr, self());
      }
   }
}

/**
 * Delete IP address
 */
void Interface::deleteIpAddress(InetAddress addr)
{
   lockProperties();
   m_ipAddressList.remove(addr);
   setModified(MODIFY_INTERFACE_PROPERTIES);
   unlockProperties();
   if (!isExcludedFromTopology())
   {
		if (IsZoningEnabled())
		{
			shared_ptr<Zone> zone = FindZoneByUIN(m_zoneUIN);
			if (zone != nullptr)
			{
            zone->removeFromInterfaceIndex(addr);
			}
			else
			{
				DbgPrintf(2, _T("Cannot find zone object with GUID=%d for interface object %s [%d]"), (int)m_zoneUIN, m_name, (int)m_id);
			}
		}
		else
		{
         g_idxInterfaceByAddr.remove(addr);
      }
   }
}

/**
 * Change network mask for IP address
 */
void Interface::setNetMask(const InetAddress& addr)
{
   lockProperties();
   m_ipAddressList.replace(addr);
   setModified(MODIFY_INTERFACE_PROPERTIES);
   unlockProperties();
}

/**
 * Set interface physical location
 */
void Interface::setPhysicalLocation(const InterfacePhysicalLocation& location)
{
   lockProperties();
   m_physicalLocation = location;
   setModified(MODIFY_INTERFACE_PROPERTIES);
   unlockProperties();
}

/**
 * Update VLAN list. Interface object will take ownership of provided VLAN list.
 */
void Interface::updateVlans(IntegerArray<uint32_t> *vlans)
{
   lockProperties();
   if (((m_vlans == nullptr) && (vlans != nullptr)) ||
       ((m_vlans != nullptr) && (vlans == nullptr)) ||
       ((m_vlans != nullptr) && (vlans != nullptr) && !m_vlans->equals(*vlans)))
   {
      delete m_vlans;
      m_vlans = vlans;
      setModified(MODIFY_INTERFACE_PROPERTIES);
   }
   else
   {
      // No changes
      delete vlans;
   }
   unlockProperties();
}

/**
 * Create NXSL object for this object
 */
NXSL_Value *Interface::createNXSLObject(NXSL_VM *vm)
{
   return vm->createValue(vm->createObject(&g_nxslInterfaceClass, new shared_ptr<Interface>(self())));
}

/**
 * Get VLAN list as NXSL array
 */
NXSL_Value *Interface::getVlanListForNXSL(NXSL_VM *vm)
{
   NXSL_Array *a = new NXSL_Array(vm);
   lockProperties();
   if (m_vlans != nullptr)
   {
      for (int i = 0; i < m_vlans->size(); i++)
      {
         a->append(vm->createValue(m_vlans->get(i)));
      }
   }
   unlockProperties();
   return vm->createValue(a);
}

/**
 * Check if this interface is a point-to-point link (from IP protocol point of view).
 */
bool Interface::isPointToPoint() const
{
   static uint32_t ptpTypes[] = {
            IFTYPE_LAPB, IFTYPE_E1, IFTYPE_PROP_PTP_SERIAL, IFTYPE_PPP, IFTYPE_SLIP,
            IFTYPE_RS232, IFTYPE_V35, IFTYPE_ADSL, IFTYPE_RADSL, IFTYPE_SDSL, IFTYPE_VDSL,
            IFTYPE_HDLC, IFTYPE_MSDSL, IFTYPE_IDSL, IFTYPE_HDSL2, IFTYPE_SHDSL, 0
   };
   for(int i = 0; ptpTypes[i] != 0; i++)
      if (m_type == ptpTypes[i])
         return true;

   if (m_ipAddressList.size() == 0)
      return false;

   for(int i = 0; i < m_ipAddressList.size(); i++)
   {
      InetAddress a = m_ipAddressList.get(i);
      if (a.isMulticast())
         return false;
      if ((a.getFamily() == AF_INET) && (a.getMaskBits() < 30))
         return false;
      if ((a.getFamily() == AF_INET6) && (a.getMaskBits() < 126))
         return false;
   }
   return true;
}

/**
 * Build expanded interface name from original name. Expanded name buffer must be at least MAX_OBJECT_NAME characters.
 */
void Interface::expandName(const TCHAR *originalName, TCHAR *expandedName)
{
   TCHAR namePattern[MAX_DB_STRING];
   ConfigReadStr(_T("Objects.Interfaces.NamePattern"), namePattern, MAX_DB_STRING, _T(""));
   Trim(namePattern);
   if (namePattern[0] == 0)
   {
      _tcslcpy(expandedName, originalName, MAX_OBJECT_NAME);
      return;
   }

   StringBuffer e = expandText(namePattern, nullptr, nullptr, shared_ptr<DCObjectInfo>(), nullptr, originalName, nullptr, nullptr, nullptr);
   _tcslcpy(expandedName, e.cstr(), MAX_OBJECT_NAME);
   Trim(expandedName);
   if (expandedName[0] == 0)
      _tcslcpy(expandedName, originalName, MAX_OBJECT_NAME);

   nxlog_debug(6, _T("Interface [%u] on node %s [%u] name expanded: \"%s\" -> \"%s\""),
            m_id, getParentNodeName().cstr(), getParentNodeId(), originalName, expandedName);
}

/**
 * Set OSPF information
 */
void Interface::setOSPFInformation(const OSPFInterface& ospfInterface)
{
   lockProperties();
   bool modified = ((m_flags & IF_OSPF_INTERFACE) == 0) || (m_ospfArea != ospfInterface.areaId) || (m_ospfType != ospfInterface.type) || (m_ospfState != ospfInterface.state);
   m_flags |= IF_OSPF_INTERFACE;
   m_ospfArea = ospfInterface.areaId;
   m_ospfType = ospfInterface.type;
   m_ospfState = ospfInterface.state;
   unlockProperties();
   if (modified)
      setModified(MODIFY_INTERFACE_PROPERTIES | MODIFY_COMMON_PROPERTIES);
}

/**
 * Clear OSPF information
 */
void Interface::clearOSPFInformation()
{
   lockProperties();
   if (m_flags & IF_OSPF_INTERFACE)
   {
      m_flags &= ~IF_OSPF_INTERFACE;
      m_ospfArea = 0;
      m_ospfType = OSPFInterfaceType::UNKNOWN;
      m_ospfState = OSPFInterfaceState::UNKNOWN;
      setModified(MODIFY_INTERFACE_PROPERTIES | MODIFY_COMMON_PROPERTIES);
   }
   unlockProperties();
}

/**
 * Serialize object to JSON
 */
json_t *Interface::toJson()
{
   json_t *root = super::toJson();

   lockProperties();

   json_object_set_new(root, "index", json_integer(m_index));
   TCHAR text[64];
   json_object_set_new(root, "macAddr", json_string_t(m_macAddr.toString(text)));
   json_object_set_new(root, "ipAddressList", m_ipAddressList.toJson());
   json_object_set_new(root, "flags", json_integer(m_flags));
   json_object_set_new(root, "description", json_string_t(m_description));
   json_object_set_new(root, "ifAlias", json_string_t(m_ifAlias));
   json_object_set_new(root, "type", json_integer(m_type));
   json_object_set_new(root, "mtu", json_integer(m_mtu));
   json_object_set_new(root, "speed", json_integer(m_speed));
   json_object_set_new(root, "bridgePortNumber", json_integer(m_bridgePortNumber));
   json_object_set_new(root, "peerNodeId", json_integer(m_peerNodeId));
   json_object_set_new(root, "peerInterfaceId", json_integer(m_peerInterfaceId));
   json_object_set_new(root, "peerDiscoveryProtocol", json_integer(m_peerDiscoveryProtocol));
   json_object_set_new(root, "adminState", json_integer(m_adminState));
   json_object_set_new(root, "operState", json_integer(m_operState));
   json_object_set_new(root, "stpPortState", json_integer(static_cast<json_int_t>(m_stpPortState)));
   json_object_set_new(root, "lastKnownOperState", json_integer(m_lastKnownOperState));
   json_object_set_new(root, "lastKnownAdminState", json_integer(m_lastKnownAdminState));
   json_object_set_new(root, "pendingOperState", json_integer(m_pendingOperState));
   json_object_set_new(root, "confirmedOperState", json_integer(m_confirmedOperState));
   json_object_set_new(root, "dot1xPaeAuthState", json_integer(m_dot1xPaeAuthState));
   json_object_set_new(root, "dot1xBackendAuthState", json_integer(m_dot1xBackendAuthState));
   json_object_set_new(root, "lastDownEventId", json_integer(m_lastDownEventId));
   json_object_set_new(root, "pendingStatus", json_integer(m_pendingStatus));
   json_object_set_new(root, "statusPollCount", json_integer(m_statusPollCount));
   json_object_set_new(root, "operStatePollCount", json_integer(m_operStatePollCount));
   json_object_set_new(root, "requiredPollCount", json_integer(m_requiredPollCount));
   json_object_set_new(root, "zoneUIN", json_integer(m_zoneUIN));
   json_object_set_new(root, "ifTableSuffixLen", json_integer(m_ifTableSuffixLen));
   json_object_set_new(root, "ifTableSuffix", json_integer_array(m_ifTableSuffix, m_ifTableSuffixLen));

   if (m_flags & IF_OSPF_INTERFACE)
   {
      json_object_set_new(root, "ospfArea", json_string_t(IpToStr(m_ospfArea, text)));
      json_object_set_new(root, "ospfType", json_integer(static_cast<json_int_t>(m_ospfType)));
      json_object_set_new(root, "ospfState", json_integer(static_cast<json_int_t>(m_ospfState)));
   }

   json_t *loc = json_object();
   json_object_set_new(loc, "chassis", json_integer(m_physicalLocation.chassis));
   json_object_set_new(loc, "module", json_integer(m_physicalLocation.module));
   json_object_set_new(loc, "pic", json_integer(m_physicalLocation.pic));
   json_object_set_new(loc, "port", json_integer(m_physicalLocation.port));
   json_object_set_new(root, "physicalLocation", loc);

   unlockProperties();
   return root;
}
