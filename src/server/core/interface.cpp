/*
** NetXMS - Network Management System
** Copyright (C) 2003-2018 Victor Kirhenshtein
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
Interface::Interface() : super()
{
   m_parentInterfaceId = 0;
	nx_strncpy(m_description, m_name, MAX_DB_STRING);
   m_alias[0] = 0;
   m_index = 0;
   m_type = IFTYPE_OTHER;
   m_mtu = 0;
   m_speed = 0;
	m_bridgePortNumber = 0;
	m_slotNumber = 0;
	m_portNumber = 0;
	m_peerNodeId = 0;
	m_peerInterfaceId = 0;
   m_peerDiscoveryProtocol = LL_PROTO_UNKNOWN;
   m_adminState = IF_ADMIN_STATE_UNKNOWN;
   m_operState = IF_OPER_STATE_UNKNOWN;
   m_pendingOperState = IF_OPER_STATE_UNKNOWN;
   m_confirmedOperState = IF_OPER_STATE_UNKNOWN;
	m_dot1xPaeAuthState = PAE_STATE_UNKNOWN;
	m_dot1xBackendAuthState = BACKEND_STATE_UNKNOWN;
   memset(m_macAddr, 0, MAC_ADDR_LENGTH);
   m_lastDownEventId = 0;
	m_pendingStatus = -1;
	m_statusPollCount = 0;
   m_operStatePollCount = 0;
	m_requiredPollCount = 0;	// Use system default
	m_zoneUIN = 0;
	m_pingTime = PING_TIME_TIMEOUT;
   m_pingLastTimeStamp = 0;
   m_ifTableSuffixLen = 0;
   m_ifTableSuffix = NULL;
   m_vlans = NULL;
}

/**
 * Constructor for "fake" interface object
 */
Interface::Interface(const InetAddressList& addrList, UINT32 zoneUIN, bool bSyntheticMask) : super()
{
   m_parentInterfaceId = 0;
	m_flags = bSyntheticMask ? IF_SYNTHETIC_MASK : 0;
   if (addrList.isLoopbackOnly())
		m_flags |= IF_LOOPBACK;

	_tcscpy(m_name, _T("unknown"));
   _tcscpy(m_description, _T("unknown"));
   m_alias[0] = 0;
   m_ipAddressList.add(addrList);
   m_index = 1;
   m_type = IFTYPE_OTHER;
   m_mtu = 0;
   m_speed = 0;
	m_bridgePortNumber = 0;
	m_slotNumber = 0;
	m_portNumber = 0;
	m_peerNodeId = 0;
	m_peerInterfaceId = 0;
   m_peerDiscoveryProtocol = LL_PROTO_UNKNOWN;
   m_adminState = IF_ADMIN_STATE_UNKNOWN;
   m_operState = IF_OPER_STATE_UNKNOWN;
   m_pendingOperState = IF_OPER_STATE_UNKNOWN;
   m_confirmedOperState = IF_OPER_STATE_UNKNOWN;
	m_dot1xPaeAuthState = PAE_STATE_UNKNOWN;
	m_dot1xBackendAuthState = BACKEND_STATE_UNKNOWN;
   memset(m_macAddr, 0, MAC_ADDR_LENGTH);
   m_lastDownEventId = 0;
	m_pendingStatus = -1;
   m_statusPollCount = 0;
   m_operStatePollCount = 0;
	m_requiredPollCount = 0;	// Use system default
	m_zoneUIN = zoneUIN;
   m_isHidden = true;
	m_pingTime = PING_TIME_TIMEOUT;
	m_pingLastTimeStamp = 0;
   m_ifTableSuffixLen = 0;
   m_ifTableSuffix = NULL;
   m_vlans = NULL;
}

/**
 * Constructor for normal interface object
 */
Interface::Interface(const TCHAR *name, const TCHAR *descr, UINT32 index, const InetAddressList& addrList, UINT32 ifType, UINT32 zoneUIN)
          : super()
{
   if ((ifType == IFTYPE_SOFTWARE_LOOPBACK) || addrList.isLoopbackOnly())
		m_flags = IF_LOOPBACK;
	else
		m_flags = 0;

   m_parentInterfaceId = 0;
   nx_strncpy(m_name, name, MAX_OBJECT_NAME);
   nx_strncpy(m_description, descr, MAX_DB_STRING);
   m_alias[0] = 0;
   m_index = index;
   m_type = ifType;
   m_mtu = 0;
   m_speed = 0;
   m_ipAddressList.add(addrList);
	m_bridgePortNumber = 0;
	m_slotNumber = 0;
	m_portNumber = 0;
	m_peerNodeId = 0;
	m_peerInterfaceId = 0;
   m_adminState = IF_ADMIN_STATE_UNKNOWN;
   m_operState = IF_OPER_STATE_UNKNOWN;
   m_pendingOperState = IF_OPER_STATE_UNKNOWN;
   m_confirmedOperState = IF_OPER_STATE_UNKNOWN;
   m_peerDiscoveryProtocol = LL_PROTO_UNKNOWN;
	m_dot1xPaeAuthState = PAE_STATE_UNKNOWN;
	m_dot1xBackendAuthState = BACKEND_STATE_UNKNOWN;
   memset(m_macAddr, 0, MAC_ADDR_LENGTH);
   m_lastDownEventId = 0;
	m_pendingStatus = -1;
   m_statusPollCount = 0;
   m_operStatePollCount = 0;
	m_requiredPollCount = 0;	// Use system default
	m_zoneUIN = zoneUIN;
   m_isHidden = true;
	m_pingTime = PING_TIME_TIMEOUT;
	m_pingLastTimeStamp = 0;
   m_ifTableSuffixLen = 0;
   m_ifTableSuffix = NULL;
   m_vlans = NULL;
}

/**
 * Interface class destructor
 */
Interface::~Interface()
{
   free(m_ifTableSuffix);
   delete m_vlans;
}

/**
 * Returns last ping time
 */
UINT32 Interface::getPingTime()
{
   if ((time(NULL) - m_pingLastTimeStamp) > g_dwStatusPollingInterval)
   {
      updatePingData();
      nxlog_debug(7, _T("Interface::getPingTime: update ping time is required! Last ping time %d."), m_pingLastTimeStamp);
   }
   return m_pingTime;
}

/**
 * Create object from database record
 */
bool Interface::loadFromDatabase(DB_HANDLE hdb, UINT32 dwId)
{
   bool bResult = false;

   m_id = dwId;

   if (!loadCommonProperties(hdb))
      return false;

	DB_STATEMENT hStmt = DBPrepare(hdb,
		_T("SELECT if_type,if_index,node_id,")
		_T("mac_addr,required_polls,bridge_port,phy_slot,")
		_T("phy_port,peer_node_id,peer_if_id,description,")
		_T("dot1x_pae_state,dot1x_backend_state,admin_state,")
      _T("oper_state,peer_proto,alias,mtu,speed,parent_iface,")
      _T("iftable_suffix FROM interfaces WHERE id=?"));
	if (hStmt == NULL)
		return false;
	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);

	DB_RESULT hResult = DBSelectPrepared(hStmt);
   if (hResult == NULL)
	{
		DBFreeStatement(hStmt);
      return false;     // Query failed
	}

   if (DBGetNumRows(hResult) != 0)
   {
      m_type = DBGetFieldULong(hResult, 0, 0);
      m_index = DBGetFieldULong(hResult, 0, 1);
      UINT32 nodeId = DBGetFieldULong(hResult, 0, 2);
		DBGetFieldByteArray2(hResult, 0, 3, m_macAddr, MAC_ADDR_LENGTH, 0);
      m_requiredPollCount = DBGetFieldLong(hResult, 0, 4);
		m_bridgePortNumber = DBGetFieldULong(hResult, 0, 5);
		m_slotNumber = DBGetFieldULong(hResult, 0, 6);
		m_portNumber = DBGetFieldULong(hResult, 0, 7);
		m_peerNodeId = DBGetFieldULong(hResult, 0, 8);
		m_peerInterfaceId = DBGetFieldULong(hResult, 0, 9);
		DBGetField(hResult, 0, 10, m_description, MAX_DB_STRING);
		m_dot1xPaeAuthState = (WORD)DBGetFieldLong(hResult, 0, 11);
		m_dot1xBackendAuthState = (WORD)DBGetFieldLong(hResult, 0, 12);
		m_adminState = (WORD)DBGetFieldLong(hResult, 0, 13);
		m_operState = (WORD)DBGetFieldLong(hResult, 0, 14);
		m_confirmedOperState = m_operState;
		m_peerDiscoveryProtocol = (LinkLayerProtocol)DBGetFieldLong(hResult, 0, 15);
		DBGetField(hResult, 0, 16, m_alias, MAX_DB_STRING);
		m_mtu = DBGetFieldULong(hResult, 0, 17);
      m_speed = DBGetFieldUInt64(hResult, 0, 18);
      m_parentInterfaceId = DBGetFieldULong(hResult, 0, 19);

      TCHAR suffixText[128];
      DBGetField(hResult, 0, 20, suffixText, 128);
      StrStrip(suffixText);
      if (suffixText[0] == 0)
      {
         UINT32 suffix[16];
         size_t l = SNMPParseOID(suffixText, suffix, 16);
         if (l > 0)
         {
            m_ifTableSuffixLen = (int)l;
            m_ifTableSuffix = (UINT32 *)nx_memdup(suffix, l * sizeof(UINT32));
         }
      }

      m_pingTime = PING_TIME_TIMEOUT;
      m_pingLastTimeStamp = 0;

      // Link interface to node
      if (!m_isDeleted)
      {
         NetObj *object = FindObjectById(nodeId, OBJECT_NODE);
         if (object == NULL)
         {
            nxlog_write(MSG_INVALID_NODE_ID, EVENTLOG_ERROR_TYPE, "dd", dwId, nodeId);
         }
         else
         {
            object->addChild(this);
            addParent(object);
				m_zoneUIN = static_cast<Node*>(object)->getZoneUIN();
            bResult = true;
         }
      }
      else
      {
         bResult = true;
      }
   }

   DBFreeResult(hResult);
	DBFreeStatement(hStmt);

	// Read VLANs
   hStmt = DBPrepare(hdb, _T("SELECT vlan_id FROM interface_vlan_list WHERE iface_id = ?"));
   if (hStmt != NULL)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
      hResult = DBSelectPrepared(hStmt);
      if (hResult != NULL)
      {
         int count = DBGetNumRows(hResult);
         if (count > 0)
         {
            m_vlans = new IntegerArray<UINT32>(count);
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
   hStmt = DBPrepare(hdb, _T("SELECT ip_addr,ip_netmask FROM interface_address_list WHERE iface_id = ?"));
   if (hStmt != NULL)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
      hResult = DBSelectPrepared(hStmt);
      if (hResult != NULL)
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
      const ObjectArray<InetAddress> *list = m_ipAddressList.getList();
      int count = 0;
      for(int i = 0; i < list->size(); i++)
      {
         if (list->get(i)->isLoopback())
            count++;
      }
      if ((count > 0) && (count == list->size())) // all loopback addresses
   		m_flags |= IF_LOOPBACK;
      else
         m_flags &= ~IF_LOOPBACK;
   }

   return bResult;
}

/**
 * Save interface object to database
 */
bool Interface::saveToDatabase(DB_HANDLE hdb)
{
   TCHAR szMacStr[16];
   UINT32 dwNodeId;

   lockProperties();

   if (!saveCommonProperties(hdb))
	{
		unlockProperties();
		return false;
	}

   // Determine owning node's ID
   bool success;
   if (m_modified & MODIFY_INTERFACE_PROPERTIES)
   {
      Node *pNode = getParentNode();
      if (pNode != NULL)
         dwNodeId = pNode->getId();
      else
         dwNodeId = 0;

      static const TCHAR *columns[] = {
         _T("node_id"), _T("if_type"), _T("if_index"), _T("mac_addr"), _T("required_polls"), _T("bridge_port"),
         _T("phy_slot"), _T("phy_port"), _T("peer_node_id"), _T("peer_if_id"), _T("description"), _T("admin_state"),
         _T("oper_state"), _T("dot1x_pae_state"), _T("dot1x_backend_state"), _T("peer_proto"), _T("alias"),
         _T("mtu"), _T("speed"), _T("parent_iface"), _T("iftable_suffix"),
         NULL
      };

      DB_STATEMENT hStmt = DBPrepareMerge(hdb, _T("interfaces"), _T("id"), m_id, columns);
      if (hStmt == NULL)
      {
         unlockProperties();
         return false;
      }

      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, dwNodeId);
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_type);
      DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_index);
      DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, BinToStr(m_macAddr, MAC_ADDR_LENGTH, szMacStr), DB_BIND_STATIC);
      DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, (LONG)m_requiredPollCount);
      DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, m_bridgePortNumber);
      DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, m_slotNumber);
      DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, m_portNumber);
      DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, m_peerNodeId);
      DBBind(hStmt, 10, DB_SQLTYPE_INTEGER, m_peerInterfaceId);
      DBBind(hStmt, 11, DB_SQLTYPE_VARCHAR, m_description, DB_BIND_STATIC);
      DBBind(hStmt, 12, DB_SQLTYPE_INTEGER, (UINT32)m_adminState);
      DBBind(hStmt, 13, DB_SQLTYPE_INTEGER, (UINT32)m_operState);
      DBBind(hStmt, 14, DB_SQLTYPE_INTEGER, (UINT32)m_dot1xPaeAuthState);
      DBBind(hStmt, 15, DB_SQLTYPE_INTEGER, (UINT32)m_dot1xBackendAuthState);
      DBBind(hStmt, 16, DB_SQLTYPE_INTEGER, (INT32)m_peerDiscoveryProtocol);
      DBBind(hStmt, 17, DB_SQLTYPE_VARCHAR, m_alias, DB_BIND_STATIC);
      DBBind(hStmt, 18, DB_SQLTYPE_INTEGER, m_mtu);
      DBBind(hStmt, 19, DB_SQLTYPE_BIGINT, m_speed);
      DBBind(hStmt, 20, DB_SQLTYPE_INTEGER, m_parentInterfaceId);
      if (m_ifTableSuffixLen > 0)
      {
         TCHAR buffer[128];
         DBBind(hStmt, 21, DB_SQLTYPE_VARCHAR, SNMPConvertOIDToText(m_ifTableSuffixLen, m_ifTableSuffix, buffer, 128), DB_BIND_TRANSIENT);
      }
      else
      {
         DBBind(hStmt, 21, DB_SQLTYPE_VARCHAR, _T(""), DB_BIND_STATIC);
      }
      DBBind(hStmt, 22, DB_SQLTYPE_INTEGER, m_id);

      success = DBExecute(hStmt);
      DBFreeStatement(hStmt);

      // Save VLAN list
      if (success)
      {
         success = executeQueryOnObject(hdb, _T("DELETE FROM interface_vlan_list WHERE iface_id = ?"));
      }

      if (success && (m_vlans != NULL) && !m_vlans->isEmpty())
      {
         hStmt = DBPrepare(hdb, _T("INSERT INTO interface_vlan_list (iface_id,vlan_id) VALUES (?,?)"), m_vlans->size() > 1);
         if (hStmt != NULL)
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

      // Save IP addresses
      if (success)
      {
         success = executeQueryOnObject(hdb, _T("DELETE FROM interface_address_list WHERE iface_id = ?"));
      }

      if (success && (m_ipAddressList.size() > 0))
      {
         hStmt = DBPrepare(hdb, _T("INSERT INTO interface_address_list (iface_id,ip_addr,ip_netmask) VALUES (?,?,?)"), m_ipAddressList.size() > 1);
         if (hStmt != NULL)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
            const ObjectArray<InetAddress> *list = m_ipAddressList.getList();
            for(int i = 0; (i < list->size()) && success; i++)
            {
               InetAddress *a = list->get(i);
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
   }
   else
   {
      success = true;
   }

   unlockProperties();

   // Save access list
	if (success)
		success = saveACLToDB(hdb);

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
void Interface::statusPoll(ClientSession *session, UINT32 rqId, Queue *eventQueue, Cluster *cluster, SNMP_Transport *snmpTransport, UINT32 nodeIcmpProxy)
{
   if (IsShutdownInProgress())
      return;

   m_pollRequestor = session;
   Node *pNode = getParentNode();
   if (pNode == NULL)
   {
      m_status = STATUS_UNKNOWN;
      return;     // Cannot find parent node, which is VERY strange
   }

   sendPollerMsg(rqId, _T("   Starting status poll on interface %s\r\n"), m_name);
   sendPollerMsg(rqId, _T("      Current interface status is %s\r\n"), GetStatusAsText(m_status, true));

	InterfaceAdminState adminState = IF_ADMIN_STATE_UNKNOWN;
	InterfaceOperState operState = IF_OPER_STATE_UNKNOWN;
   BOOL bNeedPoll = TRUE;

   // Poll interface using different methods
   if ((pNode->getCapabilities() & NC_IS_NATIVE_AGENT) &&
       (!(pNode->getFlags() & NF_DISABLE_NXCP)) && (!(pNode->getState() & NSF_AGENT_UNREACHABLE)))
   {
      sendPollerMsg(rqId, _T("      Retrieving interface status from NetXMS agent\r\n"));
      pNode->getInterfaceStatusFromAgent(m_index, &adminState, &operState);
		DbgPrintf(7, _T("Interface::StatusPoll(%d,%s): new state from NetXMS agent: adinState=%d operState=%d"), m_id, m_name, adminState, operState);
		if ((adminState != IF_ADMIN_STATE_UNKNOWN) && (operState != IF_OPER_STATE_UNKNOWN))
		{
			sendPollerMsg(rqId, POLLER_INFO _T("      Interface status retrieved from NetXMS agent\r\n"));
         bNeedPoll = FALSE;
		}
		else
		{
			sendPollerMsg(rqId, POLLER_WARNING _T("      Unable to retrieve interface status from NetXMS agent\r\n"));
		}
   }

   if (bNeedPoll && (pNode->getCapabilities() & NC_IS_SNMP) &&
       (!(pNode->getFlags() & NF_DISABLE_SNMP)) && (!(pNode->getState() & NSF_SNMP_UNREACHABLE)) &&
		 (snmpTransport != NULL))
   {
      sendPollerMsg(rqId, _T("      Retrieving interface status from SNMP agent\r\n"));
      pNode->getInterfaceStatusFromSNMP(snmpTransport, m_index, m_ifTableSuffixLen, m_ifTableSuffix, &adminState, &operState);
		DbgPrintf(7, _T("Interface::StatusPoll(%d,%s): new state from SNMP: adminState=%d operState=%d"), m_id, m_name, adminState, operState);
		if ((adminState != IF_ADMIN_STATE_UNKNOWN) && (operState != IF_OPER_STATE_UNKNOWN))
		{
			sendPollerMsg(rqId, POLLER_INFO _T("      Interface status retrieved from SNMP agent\r\n"));
         bNeedPoll = FALSE;
		}
		else
		{
			sendPollerMsg(rqId, POLLER_WARNING _T("      Unable to retrieve interface status from SNMP agent\r\n"));
		}
   }

   if (bNeedPoll)
   {
		// Ping cannot be used for cluster sync interfaces
      if ((pNode->getFlags() & NF_DISABLE_ICMP) || isLoopback() || !m_ipAddressList.hasValidUnicastAddress())
      {
			// Interface doesn't have an IP address, so we can't ping it
			sendPollerMsg(rqId, POLLER_WARNING _T("      Interface status cannot be determined\r\n"));
			DbgPrintf(7, _T("Interface::StatusPoll(%d,%s): cannot use ping for status check"), m_id, m_name);
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
		            DbgPrintf(5, _T("Interface::StatusPoll(%d,%s): auto expected state changed to UP"), m_id, m_name);
		            expectedState = IF_EXPECTED_STATE_UP;
		            setExpectedState(expectedState);
		         }
					newStatus = ((expectedState == IF_EXPECTED_STATE_DOWN) ? STATUS_CRITICAL : STATUS_NORMAL);
					break;
				case IF_OPER_STATE_DOWN:
               if (expectedState == IF_EXPECTED_STATE_AUTO)
               {
                  DbgPrintf(5, _T("Interface::StatusPoll(%d,%s): auto expected state changed to DOWN"), m_id, m_name);
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
            DbgPrintf(5, _T("Interface::StatusPoll(%d,%s): auto expected state changed to DOWN"), m_id, m_name);
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

	// Check 802.1x state
	if ((pNode->getCapabilities() & NC_IS_8021X) && isPhysicalPort() && (snmpTransport != NULL))
	{
		DbgPrintf(5, _T("StatusPoll(%s): Checking 802.1x state for interface %s"), pNode->getName(), m_name);
		paeStatusPoll(rqId, snmpTransport, pNode);
		if ((m_dot1xPaeAuthState == PAE_STATE_FORCE_UNAUTH) && (newStatus < STATUS_MAJOR))
			newStatus = STATUS_MAJOR;
	}

	// Reset status to unknown if node has known network connectivity problems
	if ((newStatus == STATUS_CRITICAL) && (pNode->getState() & DCSF_NETWORK_PATH_PROBLEM))
	{
		newStatus = STATUS_UNKNOWN;
		DbgPrintf(6, _T("StatusPoll(%s): Status for interface %s reset to UNKNOWN"), pNode->getName(), m_name);
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
	                    ((getParentNode()->getRequiredPollCount() > 0) ? getParentNode()->getRequiredPollCount() : g_requiredPolls);
	sendPollerMsg(rqId, _T("      Interface is %s for %d poll%s (%d poll%s required for status change)\r\n"),
	              GetStatusAsText(newStatus, true), m_statusPollCount, (m_statusPollCount == 1) ? _T("") : _T("s"),
	              requiredPolls, (requiredPolls == 1) ? _T("") : _T("s"));
	DbgPrintf(7, _T("Interface::StatusPoll(%d,%s): newStatus=%d oldStatus=%d pollCount=%d requiredPolls=%d"),
	          m_id, m_name, newStatus, oldStatus, m_statusPollCount, requiredPolls);

   if ((operState != m_confirmedOperState) && (m_operStatePollCount >= requiredPolls))
   {
      DbgPrintf(6, _T("Interface::StatusPoll(%d,%s): confirmedOperState=%d pollCount=%d requiredPolls=%d"),
                m_id, m_name, (int)operState, m_operStatePollCount, requiredPolls);
      m_confirmedOperState = operState;
   }

   if ((newStatus != oldStatus) && (m_statusPollCount >= requiredPolls) && (expectedState != IF_EXPECTED_STATE_IGNORE))
   {
		static UINT32 statusToEvent[] =
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
		static UINT32 statusToEventInverted[] =
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

		DbgPrintf(7, _T("Interface::StatusPoll(%d,%s): status changed from %d to %d"), m_id, m_name, m_status, newStatus);
		m_status = newStatus;
		m_pendingStatus = -1;	// Invalidate pending status
      if (!m_isSystem)
      {
		   sendPollerMsg(rqId, _T("      Interface status changed to %s\r\n"), GetStatusAsText(m_status, true));
         const InetAddress& addr = m_ipAddressList.getFirstUnicastAddress();
		   PostEventEx(eventQueue,
		               (expectedState == IF_EXPECTED_STATE_DOWN) ? statusToEventInverted[m_status] : statusToEvent[m_status],
                     pNode->getId(), "dsAdd", m_id, m_name, &addr, addr.getMaskBits(), m_index);
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
		m_adminState = (WORD)adminState;
		m_operState = (WORD)operState;
		setModified(MODIFY_INTERFACE_PROPERTIES);
	}
	unlockProperties();

	sendPollerMsg(rqId, _T("      Interface status after poll is %s\r\n"), GetStatusAsText(m_status, true));
	sendPollerMsg(rqId, _T("   Finished status poll on interface %s\r\n"), m_name);
}

/**
 * Updates last ping time and ping time
 */
void Interface::updatePingData()
{
   Node *pNode = getParentNode();
   if (pNode == NULL)
   {
      nxlog_debug(7, _T("Interface::updatePingData: Can't find parent node"));
      return;
   }
   UINT32 icmpProxy = pNode->getIcmpProxy();

   if (IsZoningEnabled() && (m_zoneUIN != 0) && (icmpProxy == 0))
   {
      Zone *zone = FindZoneByUIN(m_zoneUIN);
      if (zone != NULL)
      {
         icmpProxy = zone->getProxyNodeId(getParentNode());
      }
   }

   if (icmpProxy != 0)
   {
      nxlog_debug(7, _T("Interface::updatePingData: ping via proxy [%u]"), icmpProxy);
      Node *proxyNode = (Node *)g_idxNodeById.get(icmpProxy);
      if ((proxyNode != NULL) && proxyNode->isNativeAgent() && !proxyNode->isDown())
      {
         nxlog_debug(7, _T("Interface::updatePingData: proxy node found: %s"), proxyNode->getName());
         AgentConnection *conn = proxyNode->createAgentConnection();
         if (conn != NULL)
         {
            TCHAR parameter[128], buffer[64];
            const ObjectArray<InetAddress> *list = m_ipAddressList.getList();
            long value = -1;
            for(int i = 0; (i < list->size()) && ((value == 10000) || (value == -1)); i++)
            {
               const InetAddress *a = list->get(i);
				   _sntprintf(parameter, 128, _T("Icmp.Ping(%s)"), a->toString(buffer));
				   if (conn->getParameter(parameter, 64, buffer) == ERR_SUCCESS)
				   {
				      nxlog_debug(7, _T("Interface::updatePingData: proxy response: \"%s\""), buffer);
					   TCHAR *eptr;
					   value = _tcstol(buffer, &eptr, 10);
                  if (*eptr != 0)
                  {
                     value = -1;
                  }
               }
            }

				if (value >= 0)
				{
               m_pingTime = value;
				}
            else
            {
               m_pingTime = PING_TIME_TIMEOUT;
               nxlog_debug(7, _T("Interface::updatePingData: incorrect value or error while parsing"));
            }
            m_pingLastTimeStamp = time(NULL);
            conn->decRefCount();
         }
         else
         {
            nxlog_debug(7, _T("Interface::updatePingData: cannot connect to agent on proxy node [%u]"), icmpProxy);
            m_pingTime = PING_TIME_TIMEOUT;
         }
      }
      else
      {
         nxlog_debug(7, _T("Interface::updatePingData: proxy node not available [%u]"), icmpProxy);
         m_pingTime = PING_TIME_TIMEOUT;
      }
   }
   else	// not using ICMP proxy
   {
      const ObjectArray<InetAddress> *list = m_ipAddressList.getList();
      UINT32 dwPingStatus = ICMP_TIMEOUT;
      for(int i = 0; (i < list->size()) && (dwPingStatus != ICMP_SUCCESS); i++)
      {
         const InetAddress *a = list->get(i);
         nxlog_debug(7, _T("Interface::StatusPoll(%d,%s): calling IcmpPing(%s,3,%d,%d,%d)"),
            m_id, m_name, (const TCHAR *)a->toString(), g_icmpPingTimeout, m_pingTime, g_icmpPingSize);
		   dwPingStatus = IcmpPing(*a, 3, g_icmpPingTimeout, &m_pingTime, g_icmpPingSize, false);
      }
      if (dwPingStatus != ICMP_SUCCESS)
      {
         nxlog_debug(7, _T("Interface::updatePingData: error: %d"), dwPingStatus);
         m_pingTime = PING_TIME_TIMEOUT;
      }
      m_pingLastTimeStamp = time(NULL);
   }
}

/**
 * Do ICMP part of status poll
 */
void Interface::icmpStatusPoll(UINT32 rqId, UINT32 nodeIcmpProxy, Cluster *cluster, InterfaceAdminState *adminState, InterfaceOperState *operState)
{
   // Use ICMP ping as a last option
	UINT32 icmpProxy = nodeIcmpProxy;

	if (IsZoningEnabled() && (m_zoneUIN != 0) && (icmpProxy == 0))
	{
		Zone *zone = FindZoneByUIN(m_zoneUIN);
		if (zone != NULL)
		{
			icmpProxy = zone->getProxyNodeId(getParentNode());
		}
	}

	if (icmpProxy != 0)
	{
		sendPollerMsg(rqId, _T("      Starting ICMP ping via proxy\r\n"));
		DbgPrintf(7, _T("Interface::StatusPoll(%d,%s): ping via proxy [%u]"), m_id, m_name, icmpProxy);
		Node *proxyNode = (Node *)g_idxNodeById.get(icmpProxy);
		if ((proxyNode != NULL) && proxyNode->isNativeAgent() && !proxyNode->isDown())
		{
			DbgPrintf(7, _T("Interface::StatusPoll(%d,%s): proxy node found: %s"), m_id, m_name, proxyNode->getName());
			AgentConnection *conn = proxyNode->createAgentConnection();
			if (conn != NULL)
			{
				TCHAR parameter[128], buffer[64];
            const ObjectArray<InetAddress> *list = m_ipAddressList.getList();
            long value = -1;
            for(int i = 0; (i < list->size()) && ((value == 10000) || (value == -1)); i++)
            {
               const InetAddress *a = list->get(i);
               if (a->isValidUnicast() && ((cluster == NULL) || !cluster->isSyncAddr(*a)))
               {
				      _sntprintf(parameter, 128, _T("Icmp.Ping(%s)"), a->toString(buffer));
				      if (conn->getParameter(parameter, 64, buffer) == ERR_SUCCESS)
				      {
					      DbgPrintf(7, _T("Interface::StatusPoll(%d,%s): proxy response: \"%s\""), m_id, m_name, buffer);
					      TCHAR *eptr;
					      value = _tcstol(buffer, &eptr, 10);
                     if (*eptr != 0)
                     {
                        value = -1;
                     }
                  }
               }
            }

            DbgPrintf(7, _T("Interface::StatusPoll(%d,%s): response time %d"), m_id, m_name, (int)value);
				if (value >= 0)
				{
               m_pingTime = (UINT32)value;
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
            m_pingLastTimeStamp = time(NULL);
				conn->decRefCount();
			}
			else
			{
				DbgPrintf(7, _T("Interface::StatusPoll(%d,%s): cannot connect to agent on proxy node"), m_id, m_name);
				sendPollerMsg(rqId, POLLER_ERROR _T("      Unable to establish connection with proxy node\r\n"));
			}
		}
		else
		{
			DbgPrintf(7, _T("Interface::StatusPoll(%d,%s): proxy node not available"), m_id, m_name);
			sendPollerMsg(rqId, POLLER_ERROR _T("      ICMP proxy not available\r\n"));
		}
	}
	else	// not using ICMP proxy
	{
		sendPollerMsg(rqId, _T("      Starting ICMP ping\r\n"));
      const ObjectArray<InetAddress> *list = m_ipAddressList.getList();
      UINT32 dwPingStatus = ICMP_TIMEOUT;
      for(int i = 0; (i < list->size()) && (dwPingStatus != ICMP_SUCCESS); i++)
      {
         const InetAddress *a = list->get(i);
         if (a->isValidUnicast() && ((cluster == NULL) || !cluster->isSyncAddr(*a)))
         {
		      DbgPrintf(7, _T("Interface::StatusPoll(%d,%s): calling IcmpPing(%s,3,%d,%d,%d)"),
               m_id, m_name, (const TCHAR *)a->toString(), g_icmpPingTimeout, m_pingTime, g_icmpPingSize);
		      dwPingStatus = IcmpPing(*a, 3, g_icmpPingTimeout, &m_pingTime, g_icmpPingSize, false);
         }
      }
      m_pingLastTimeStamp = time(NULL);
		if (dwPingStatus == ICMP_SUCCESS)
		{
			*adminState = IF_ADMIN_STATE_UP;
			*operState = IF_OPER_STATE_UP;
		}
		else
		{
         m_pingTime = PING_TIME_TIMEOUT;
			*adminState = IF_ADMIN_STATE_UNKNOWN;
			*operState = IF_OPER_STATE_DOWN;
		}
		DbgPrintf(7, _T("Interface::StatusPoll(%d,%s): ping result %d, adminState=%d, operState=%d"), m_id, m_name, dwPingStatus, *adminState, *operState);
	}
}

/**
 * PAE (802.1x) status poll
 */
void Interface::paeStatusPoll(UINT32 rqId, SNMP_Transport *pTransport, Node *node)
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

   sendPollerMsg(rqId, _T("      Checking port 802.1x status...\r\n"));

	TCHAR oid[256];
	INT32 paeState = PAE_STATE_UNKNOWN, backendState = BACKEND_STATE_UNKNOWN;
	bool modified = false;

	_sntprintf(oid, 256, _T(".1.0.8802.1.1.1.1.2.1.1.1.%d"), m_index);
	SnmpGet(pTransport->getSnmpVersion(), pTransport, oid, NULL, 0, &paeState, sizeof(INT32), 0);

	_sntprintf(oid, 256, _T(".1.0.8802.1.1.1.1.2.1.1.2.%d"), m_index);
	SnmpGet(pTransport->getSnmpVersion(), pTransport, oid, NULL, 0, &backendState, sizeof(INT32), 0);

	if (m_dot1xPaeAuthState != (WORD)paeState)
	{
	   sendPollerMsg(rqId, _T("      Port PAE state changed to %s\r\n"), PAE_STATE_TEXT(paeState));
		modified = true;
      if (!m_isSystem)
      {
		   PostEvent(EVENT_8021X_PAE_STATE_CHANGED, node->getId(), "dsdsds", paeState, PAE_STATE_TEXT(paeState),
		             (UINT32)m_dot1xPaeAuthState, PAE_STATE_TEXT(m_dot1xPaeAuthState), m_id, m_name);

		   if (paeState == PAE_STATE_FORCE_UNAUTH)
		   {
			   PostEvent(EVENT_8021X_PAE_FORCE_UNAUTH, node->getId(), "ds", m_id, m_name);
		   }
      }
	}

	if (m_dot1xBackendAuthState != (WORD)backendState)
	{
	   sendPollerMsg(rqId, _T("      Port backend state changed to %s\r\n"), BACKEND_STATE_TEXT(backendState));
		modified = true;
      if (!m_isSystem)
      {
		   PostEvent(EVENT_8021X_BACKEND_STATE_CHANGED, node->getId(), "dsdsds", backendState, BACKEND_STATE_TEXT(backendState),
		             (UINT32)m_dot1xBackendAuthState, BACKEND_STATE_TEXT(m_dot1xBackendAuthState), m_id, m_name);

		   if (backendState == BACKEND_STATE_FAIL)
		   {
			   PostEvent(EVENT_8021X_AUTH_FAILED, node->getId(), "ds", m_id, m_name);
		   }
		   else if (backendState == BACKEND_STATE_TIMEOUT)
		   {
			   PostEvent(EVENT_8021X_AUTH_TIMEOUT, node->getId(), "ds", m_id, m_name);
		   }
      }
	}

	if (modified)
	{
		lockProperties();
		m_dot1xPaeAuthState = (WORD)paeState;
		m_dot1xBackendAuthState = (WORD)backendState;
		setModified(MODIFY_INTERFACE_PROPERTIES);
		unlockProperties();
	}
}

/**
 * Create NXCP message with object's data
 */
void Interface::fillMessageInternal(NXCPMessage *msg, UINT32 userId)
{
   super::fillMessageInternal(msg, userId);

   m_ipAddressList.fillMessage(msg, VID_IP_ADDRESS_COUNT, VID_IP_ADDRESS_LIST_BASE);
   msg->setField(VID_IF_INDEX, m_index);
   msg->setField(VID_IF_TYPE, m_type);
   msg->setField(VID_MTU, m_mtu);
   msg->setField(VID_SPEED, m_speed);
   msg->setField(VID_IF_SLOT, m_slotNumber);
   msg->setField(VID_IF_PORT, m_portNumber);
   msg->setField(VID_MAC_ADDR, m_macAddr, MAC_ADDR_LENGTH);
	msg->setField(VID_REQUIRED_POLLS, (WORD)m_requiredPollCount);
	msg->setField(VID_PEER_NODE_ID, m_peerNodeId);
	msg->setField(VID_PEER_INTERFACE_ID, m_peerInterfaceId);
	msg->setField(VID_PEER_PROTOCOL, (INT16)m_peerDiscoveryProtocol);
	msg->setField(VID_DESCRIPTION, m_description);
   msg->setField(VID_ALIAS, m_alias);
	msg->setField(VID_ADMIN_STATE, m_adminState);
	msg->setField(VID_OPER_STATE, m_operState);
	msg->setField(VID_DOT1X_PAE_STATE, m_dot1xPaeAuthState);
	msg->setField(VID_DOT1X_BACKEND_STATE, m_dot1xBackendAuthState);
	msg->setField(VID_ZONE_UIN, m_zoneUIN);
   msg->setFieldFromInt32Array(VID_IFTABLE_SUFFIX, m_ifTableSuffixLen, m_ifTableSuffix);
   msg->setField(VID_PARENT_INTERFACE, m_parentInterfaceId);
   msg->setFieldFromInt32Array(VID_VLAN_LIST, m_vlans);
}

/**
 * Modify object from message
 */
UINT32 Interface::modifyFromMessageInternal(NXCPMessage *request)
{
   // Flags
   if (request->isFieldExist(VID_FLAGS))
   {
      UINT32 mask = request->isFieldExist(VID_FLAGS_MASK) ? (request->getFieldAsUInt32(VID_FLAGS_MASK) & IF_USER_FLAGS_MASK) : IF_USER_FLAGS_MASK;
      m_flags &= ~mask;
      m_flags |= request->getFieldAsUInt32(VID_FLAGS) & mask;
   }

   // Number of required polls
   if (request->isFieldExist(VID_REQUIRED_POLLS))
      m_requiredPollCount = (int)request->getFieldAsUInt16(VID_REQUIRED_POLLS);

	// Expected interface state
	if (request->isFieldExist(VID_EXPECTED_STATE))
	{
		setExpectedStateInternal(request->getFieldAsInt16(VID_EXPECTED_STATE));
	}

   return super::modifyFromMessageInternal(request);
}

/**
 * Set expected state for interface
 */
void Interface::setExpectedStateInternal(int state)
{
   static UINT32 eventCode[] = { EVENT_IF_EXPECTED_STATE_UP, EVENT_IF_EXPECTED_STATE_DOWN, EVENT_IF_EXPECTED_STATE_IGNORE };

	int curr = (m_flags & IF_EXPECTED_STATE_MASK) >> 28;
	if (curr != state)
	{
      m_flags &= ~IF_EXPECTED_STATE_MASK;
      m_flags |= (UINT32)state << 28;
      setModified(MODIFY_COMMON_PROPERTIES);
      if (state != IF_EXPECTED_STATE_AUTO)
         PostEvent(eventCode[state], getParentNodeId(), "ds", m_index, m_name);
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
 * Wake up node bound to this interface by sending magic packet
 */
UINT32 Interface::wakeUp()
{
   UINT32 rcc = RCC_NO_MAC_ADDRESS;

   if (memcmp(m_macAddr, "\x00\x00\x00\x00\x00\x00", 6))
   {
      const InetAddress addr = m_ipAddressList.getFirstUnicastAddressV4();
      if (addr.isValid())
      {
         UINT32 destAddr = htonl(addr.getAddressV4() | ~(0xFFFFFFFF << (32 - addr.getMaskBits())));
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
Node *Interface::getParentNode()
{
   Node *pNode = NULL;

   lockParentList(false);
   for(int i = 0; i < m_parentList->size(); i++)
      if (m_parentList->get(i)->getObjectClass() == OBJECT_NODE)
      {
         pNode = (Node *)m_parentList->get(i);
         break;
      }
   unlockParentList();
   return pNode;
}

/**
 * Get ID of parent node object
 */
UINT32 Interface::getParentNodeId()
{
   Node *node = getParentNode();
   return (node != NULL) ? node->getId() : 0;
}

/**
 * Update zone ID. New zone ID taken from parent node.
 */
void Interface::updateZoneUIN()
{
	Node *node = getParentNode();
	if (node != NULL)
	{
		// Unregister from old zone
		Zone *zone = FindZoneByUIN(m_zoneUIN);
		if (zone != NULL)
			zone->removeFromIndex(this);

		UINT32 newZoneUIN = node->getZoneUIN();
		lockProperties();
		m_zoneUIN = newZoneUIN;
		setModified(MODIFY_INTERFACE_PROPERTIES);
		unlockProperties();

		// Register in new zone
		zone = FindZoneByUIN(newZoneUIN);
		if (zone != NULL)
			zone->addToIndex(this);
	}
}

/**
 * Handler for object deletion notification
 */
void Interface::onObjectDelete(UINT32 dwObjectId)
{
	if ((m_peerNodeId == dwObjectId) || (m_peerInterfaceId == dwObjectId))
	{
		lockProperties();
		m_peerNodeId = 0;
		m_peerInterfaceId = 0;
		setModified(MODIFY_INTERFACE_PROPERTIES);
		unlockProperties();
	}
	super::onObjectDelete(dwObjectId);
}

/**
 * Set peer information
 */
void Interface::setPeer(Node *node, Interface *iface, LinkLayerProtocol protocol, bool reflection)
{
   if ((m_peerNodeId == node->getId()) && (m_peerInterfaceId == iface->getId()) && (m_peerDiscoveryProtocol == protocol))
   {
      if ((m_flags & IF_PEER_REFLECTION) && !reflection)
      {
         // set peer information as confirmed
         m_flags &= ~IF_PEER_REFLECTION;
         setModified(MODIFY_COMMON_PROPERTIES);
      }
      return;
   }

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
      static const TCHAR *names[] = { _T("localIfId"), _T("localIfIndex"), _T("localIfName"),
         _T("localIfIP"), _T("localIfMAC"), _T("remoteNodeId"), _T("remoteNodeName"),
         _T("remoteIfId"), _T("remoteIfIndex"), _T("remoteIfName"), _T("remoteIfIP"),
         _T("remoteIfMAC"), _T("protocol") };
      PostEventWithNames(EVENT_IF_PEER_CHANGED, getParentNodeId(), "ddsAhdsddsAhd", names,
         m_id, m_index, m_name, &m_ipAddressList.getFirstUnicastAddress(), m_macAddr,
         node->getId(), node->getName(), iface->getId(), iface->getIfIndex(), iface->getName(),
         &iface->getIpAddressList()->getFirstUnicastAddress(), iface->getMacAddr(), protocol);
   }
}

/**
 * Set MAC address for interface
 */
void Interface::setMacAddr(const BYTE *macAddr, bool updateMacDB)
{
   lockProperties();
   if (updateMacDB)
      MacDbRemove(m_macAddr);
   memcpy(m_macAddr, macAddr, MAC_ADDR_LENGTH);
   if (updateMacDB)
      MacDbAddInterface(this);
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
      UpdateInterfaceIndex(m_ipAddressList.get(0), addr, this);
      m_ipAddressList.clear();
      m_ipAddressList.add(addr);
      setModified(MODIFY_INTERFACE_PROPERTIES);
   }
   unlockProperties();
}

/**
 * Get first usable IP address
 */
const InetAddress& Interface::getFirstIpAddress() const
{
   const InetAddress& a = m_ipAddressList.getFirstUnicastAddress();
   return a.isValid() ? a : m_ipAddressList.get(0);
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
			Zone *zone = FindZoneByUIN(m_zoneUIN);
			if (zone != NULL)
			{
				zone->addToIndex(addr, this);
			}
			else
			{
				DbgPrintf(2, _T("Cannot find zone object with GUID=%d for interface object %s [%d]"), (int)m_zoneUIN, m_name, (int)m_id);
			}
		}
		else
		{
         g_idxInterfaceByAddr.put(addr, this);
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
			Zone *zone = FindZoneByUIN(m_zoneUIN);
			if (zone != NULL)
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
 * Add VLAN
 */
void Interface::addVlan(UINT32 id)
{
   lockProperties();
   if (m_vlans == NULL)
   {
      m_vlans = new IntegerArray<UINT32>();
   }
   if (!m_vlans->contains(id))
   {
      m_vlans->add(id);
      setModified(MODIFY_INTERFACE_PROPERTIES);
   }
   unlockProperties();
}

/**
 * Create NXSL object for this object
 */
NXSL_Value *Interface::createNXSLObject(NXSL_VM *vm)
{
   return vm->createValue(new NXSL_Object(vm, &g_nxslInterfaceClass, this));
}

/**
 * Get VLAN list as NXSL array
 */
NXSL_Value *Interface::getVlanListForNXSL(NXSL_VM *vm)
{
   NXSL_Array *a = new NXSL_Array(vm);
   lockProperties();
   if (m_vlans != NULL)
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
 * Serialize object to JSON
 */
json_t *Interface::toJson()
{
   json_t *root = super::toJson();
   json_object_set_new(root, "index", json_integer(m_index));
   char macAddrText[64];
   json_object_set_new(root, "macAddr", json_string(BinToStrA(m_macAddr, MAC_ADDR_LENGTH, macAddrText)));
   json_object_set_new(root, "ipAddressList", m_ipAddressList.toJson());
   json_object_set_new(root, "flags", json_integer(m_flags));
   json_object_set_new(root, "description", json_string_t(m_description));
   json_object_set_new(root, "alias", json_string_t(m_alias));
   json_object_set_new(root, "type", json_integer(m_type));
   json_object_set_new(root, "mtu", json_integer(m_mtu));
   json_object_set_new(root, "speed", json_integer(m_speed));
   json_object_set_new(root, "bridgePortNumber", json_integer(m_bridgePortNumber));
   json_object_set_new(root, "slotNumber", json_integer(m_slotNumber));
   json_object_set_new(root, "portNumber", json_integer(m_portNumber));
   json_object_set_new(root, "peerNodeId", json_integer(m_peerNodeId));
   json_object_set_new(root, "peerInterfaceId", json_integer(m_peerInterfaceId));
   json_object_set_new(root, "peerDiscoveryProtocol", json_integer(m_peerDiscoveryProtocol));
   json_object_set_new(root, "adminState", json_integer(m_adminState));
   json_object_set_new(root, "operState", json_integer(m_operState));
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
   json_object_set_new(root, "pingTime", json_integer(m_pingTime));
   json_object_set_new(root, "pingLastTimeStamp", json_integer(m_pingLastTimeStamp));
   json_object_set_new(root, "ifTableSuffixLen", json_integer(m_ifTableSuffixLen));
   json_object_set_new(root, "ifTableSuffix", json_integer_array(m_ifTableSuffix, m_ifTableSuffixLen));
   return root;
}
