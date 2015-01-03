/*
** NetXMS - Network Management System
** Copyright (C) 2003-2013 Victor Kirhenshtein
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

#define PING_TIME_ON_TIMEOUT 10000

/**
 * Default constructor for Interface object
 */
Interface::Interface() : NetObj()
{
	m_flags = 0;
	nx_strncpy(m_description, m_name, MAX_DB_STRING);
   m_alias[0] = 0;
   m_ipNetMask = 0;
   m_index = 0;
   m_type = IFTYPE_OTHER;
   m_mtu = 0;
	m_bridgePortNumber = 0;
	m_slotNumber = 0;
	m_portNumber = 0;
	m_peerNodeId = 0;
	m_peerInterfaceId = 0;
   m_peerDiscoveryProtocol = LL_PROTO_UNKNOWN;
	m_dot1xPaeAuthState = PAE_STATE_UNKNOWN;
	m_dot1xBackendAuthState = BACKEND_STATE_UNKNOWN;
   memset(m_macAddr, 0, MAC_ADDR_LENGTH);
   m_lastDownEventId = 0;
	m_pendingStatus = -1;
	m_pollCount = 0;
	m_requiredPollCount = 0;	// Use system default
	m_zoneId = 0;
	m_pingTime = PING_TIME_ON_TIMEOUT;
}

/**
 * Constructor for "fake" interface object
 */
Interface::Interface(UINT32 dwAddr, UINT32 dwNetMask, UINT32 zoneId, bool bSyntheticMask) : NetObj()
{
	m_flags = bSyntheticMask ? IF_SYNTHETIC_MASK : 0;
	if ((dwAddr & 0xFF000000) == 0x7F000000)
		m_flags |= IF_LOOPBACK;

	_tcscpy(m_name, _T("unknown"));
   _tcscpy(m_description, _T("unknown"));
   m_alias[0] = 0;
   m_dwIpAddr = dwAddr;
   m_ipNetMask = dwNetMask;
   m_index = 1;
   m_type = IFTYPE_OTHER;
   m_mtu = 0;
	m_bridgePortNumber = 0;
	m_slotNumber = 0;
	m_portNumber = 0;
	m_peerNodeId = 0;
	m_peerInterfaceId = 0;
   m_peerDiscoveryProtocol = LL_PROTO_UNKNOWN;
	m_dot1xPaeAuthState = PAE_STATE_UNKNOWN;
	m_dot1xBackendAuthState = BACKEND_STATE_UNKNOWN;
   memset(m_macAddr, 0, MAC_ADDR_LENGTH);
   m_lastDownEventId = 0;
	m_pendingStatus = -1;
	m_pollCount = 0;
	m_requiredPollCount = 0;	// Use system default
	m_zoneId = zoneId;
   m_isHidden = true;
	m_pingTime = PING_TIME_ON_TIMEOUT;
}

/**
 * Constructor for normal interface object
 */
Interface::Interface(const TCHAR *name, const TCHAR *descr, UINT32 index, UINT32 ipAddr, UINT32 ipNetMask, UINT32 ifType, UINT32 zoneId)
          : NetObj()
{
	if (((ipAddr & 0xFF000000) == 0x7F000000) || (ifType == IFTYPE_SOFTWARE_LOOPBACK))
		m_flags = IF_LOOPBACK;
	else
		m_flags = 0;

   nx_strncpy(m_name, name, MAX_OBJECT_NAME);
   nx_strncpy(m_description, descr, MAX_DB_STRING);
   m_alias[0] = 0;
   m_index = index;
   m_type = ifType;
   m_mtu = 0;
   m_dwIpAddr = ipAddr;
   m_ipNetMask = ipNetMask;
	m_bridgePortNumber = 0;
	m_slotNumber = 0;
	m_portNumber = 0;
	m_peerNodeId = 0;
	m_peerInterfaceId = 0;
   m_peerDiscoveryProtocol = LL_PROTO_UNKNOWN;
	m_dot1xPaeAuthState = PAE_STATE_UNKNOWN;
	m_dot1xBackendAuthState = BACKEND_STATE_UNKNOWN;
   memset(m_macAddr, 0, MAC_ADDR_LENGTH);
   m_lastDownEventId = 0;
	m_pendingStatus = -1;
	m_pollCount = 0;
	m_requiredPollCount = 0;	// Use system default
	m_zoneId = zoneId;
   m_isHidden = true;
	m_pingTime = PING_TIME_ON_TIMEOUT;
}

/**
 * Interface class destructor
 */
Interface::~Interface()
{
}

/**
 * Returns last ping time
 */
UINT32 Interface::getPingTime()
{
   if((time(NULL) - m_pingLastTimeStamp) > g_dwStatusPollingInterval)
   {
      updatePingData();
   }
   return m_pingTime;
}

/**
 * Create object from database record
 */
BOOL Interface::loadFromDatabase(UINT32 dwId)
{
   BOOL bResult = FALSE;

   m_id = dwId;

   if (!loadCommonProperties())
      return FALSE;

	DB_STATEMENT hStmt = DBPrepare(g_hCoreDB,
		_T("SELECT ip_addr,ip_netmask,if_type,if_index,node_id,")
		_T("mac_addr,flags,required_polls,bridge_port,phy_slot,")
		_T("phy_port,peer_node_id,peer_if_id,description,")
		_T("dot1x_pae_state,dot1x_backend_state,admin_state,")
      _T("oper_state,peer_proto,alias,mtu FROM interfaces WHERE id=?"));
	if (hStmt == NULL)
		return FALSE;
	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);

	DB_RESULT hResult = DBSelectPrepared(hStmt);
   if (hResult == NULL)
	{
		DBFreeStatement(hStmt);
      return FALSE;     // Query failed
	}

   if (DBGetNumRows(hResult) != 0)
   {
      m_dwIpAddr = DBGetFieldIPAddr(hResult, 0, 0);
      m_ipNetMask = DBGetFieldIPAddr(hResult, 0, 1);
      m_type = DBGetFieldULong(hResult, 0, 2);
      m_index = DBGetFieldULong(hResult, 0, 3);
      UINT32 nodeId = DBGetFieldULong(hResult, 0, 4);
		DBGetFieldByteArray2(hResult, 0, 5, m_macAddr, MAC_ADDR_LENGTH, 0);
		m_flags = DBGetFieldULong(hResult, 0, 6);
      m_requiredPollCount = DBGetFieldLong(hResult, 0, 7);
		m_bridgePortNumber = DBGetFieldULong(hResult, 0, 8);
		m_slotNumber = DBGetFieldULong(hResult, 0, 9);
		m_portNumber = DBGetFieldULong(hResult, 0, 10);
		m_peerNodeId = DBGetFieldULong(hResult, 0, 11);
		m_peerInterfaceId = DBGetFieldULong(hResult, 0, 12);
		DBGetField(hResult, 0, 13, m_description, MAX_DB_STRING);
		m_dot1xPaeAuthState = (WORD)DBGetFieldLong(hResult, 0, 14);
		m_dot1xBackendAuthState = (WORD)DBGetFieldLong(hResult, 0, 15);
		m_adminState = (WORD)DBGetFieldLong(hResult, 0, 16);
		m_operState = (WORD)DBGetFieldLong(hResult, 0, 17);
		m_peerDiscoveryProtocol = (LinkLayerProtocol)DBGetFieldLong(hResult, 0, 18);
		DBGetField(hResult, 0, 19, m_alias, MAX_DB_STRING);
		m_mtu = DBGetFieldULong(hResult, 0, 20);

      // Link interface to node
      if (!m_isDeleted)
      {
         NetObj *object = FindObjectById(nodeId);
         if (object == NULL)
         {
            nxlog_write(MSG_INVALID_NODE_ID, EVENTLOG_ERROR_TYPE, "dd", dwId, nodeId);
         }
         else if (object->getObjectClass() != OBJECT_NODE)
         {
            nxlog_write(MSG_NODE_NOT_NODE, EVENTLOG_ERROR_TYPE, "dd", dwId, nodeId);
         }
         else
         {
            object->AddChild(this);
            AddParent(object);
				m_zoneId = ((Node *)object)->getZoneId();
            bResult = TRUE;
         }
      }
      else
      {
         bResult = TRUE;
      }
   }

   DBFreeResult(hResult);
	DBFreeStatement(hStmt);

   // Load access list
   loadACLFromDB();

	// Validate loopback flag
	if (((m_dwIpAddr & 0xFF000000) == 0x7F000000) || (m_type == IFTYPE_SOFTWARE_LOOPBACK))
		m_flags |= IF_LOOPBACK;

   return bResult;
}

/**
 * Save interface object to database
 */
BOOL Interface::saveToDatabase(DB_HANDLE hdb)
{
   TCHAR szMacStr[16], szIpAddr[16], szNetMask[16];
   UINT32 dwNodeId;

   lockProperties();

   if (!saveCommonProperties(hdb))
	{
		unlockProperties();
		return FALSE;
	}

   // Determine owning node's ID
   Node *pNode = getParentNode();
   if (pNode != NULL)
      dwNodeId = pNode->getId();
   else
      dwNodeId = 0;

   // Form and execute INSERT or UPDATE query
	DB_STATEMENT hStmt;
   if (IsDatabaseRecordExist(hdb, _T("interfaces"), _T("id"), m_id))
	{
		hStmt = DBPrepare(hdb,
			_T("UPDATE interfaces SET ip_addr=?,ip_netmask=?,")
         _T("node_id=?,if_type=?,if_index=?,mac_addr=?,flags=?,")
			_T("required_polls=?,bridge_port=?,phy_slot=?,phy_port=?,")
			_T("peer_node_id=?,peer_if_id=?,description=?,admin_state=?,")
			_T("oper_state=?,dot1x_pae_state=?,dot1x_backend_state=?,")
         _T("peer_proto=?,alias=?,mtu=? WHERE id=?"));
	}
   else
	{
		hStmt = DBPrepare(hdb,
			_T("INSERT INTO interfaces (ip_addr,ip_netmask,node_id,if_type,if_index,mac_addr,")
			_T("flags,required_polls,bridge_port,phy_slot,phy_port,peer_node_id,peer_if_id,description,")
         _T("admin_state,oper_state,dot1x_pae_state,dot1x_backend_state,peer_proto,alias,mtu,id) ")
			_T("VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)"));
	}
	if (hStmt == NULL)
	{
		unlockProperties();
		return FALSE;
	}

	DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, IpToStr(m_dwIpAddr, szIpAddr), DB_BIND_STATIC);
	DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, IpToStr(m_ipNetMask, szNetMask), DB_BIND_STATIC);
	DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, dwNodeId);
	DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, m_type);
	DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, m_index);
	DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, BinToStr(m_macAddr, MAC_ADDR_LENGTH, szMacStr), DB_BIND_STATIC);
	DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, m_flags);
	DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, (LONG)m_requiredPollCount);
	DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, m_bridgePortNumber);
	DBBind(hStmt, 10, DB_SQLTYPE_INTEGER, m_slotNumber);
	DBBind(hStmt, 11, DB_SQLTYPE_INTEGER, m_portNumber);
	DBBind(hStmt, 12, DB_SQLTYPE_INTEGER, m_peerNodeId);
	DBBind(hStmt, 13, DB_SQLTYPE_INTEGER, m_peerInterfaceId);
	DBBind(hStmt, 14, DB_SQLTYPE_VARCHAR, m_description, DB_BIND_STATIC);
	DBBind(hStmt, 15, DB_SQLTYPE_INTEGER, (UINT32)m_adminState);
	DBBind(hStmt, 16, DB_SQLTYPE_INTEGER, (UINT32)m_operState);
	DBBind(hStmt, 17, DB_SQLTYPE_INTEGER, (UINT32)m_dot1xPaeAuthState);
	DBBind(hStmt, 18, DB_SQLTYPE_INTEGER, (UINT32)m_dot1xBackendAuthState);
	DBBind(hStmt, 19, DB_SQLTYPE_INTEGER, (INT32)m_peerDiscoveryProtocol);
	DBBind(hStmt, 20, DB_SQLTYPE_VARCHAR, m_alias, DB_BIND_STATIC);
	DBBind(hStmt, 21, DB_SQLTYPE_INTEGER, m_mtu);
	DBBind(hStmt, 22, DB_SQLTYPE_INTEGER, m_id);

	BOOL success = DBExecute(hStmt);
	DBFreeStatement(hStmt);

   // Save access list
	if (success)
		success = saveACLToDB(hdb);

   // Clear modifications flag and unlock object
	if (success)
		m_isModified = false;
   unlockProperties();

   return success;
}

/**
 * Delete interface object from database
 */
bool Interface::deleteFromDatabase(DB_HANDLE hdb)
{
   bool success = NetObj::deleteFromDatabase(hdb);
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM interfaces WHERE id=?"));
   return success;
}

/**
 * Perform status poll on interface
 */
void Interface::statusPoll(ClientSession *session, UINT32 rqId, Queue *eventQueue, bool clusterSync, SNMP_Transport *snmpTransport, UINT32 nodeIcmpProxy)
{
   m_pollRequestor = session;
   Node *pNode = getParentNode();
   if (pNode == NULL)
   {
      m_iStatus = STATUS_UNKNOWN;
      return;     // Cannot find parent node, which is VERY strange
   }

   sendPollerMsg(rqId, _T("   Starting status poll on interface %s\r\n"), m_name);
   sendPollerMsg(rqId, _T("      Current interface status is %s\r\n"), GetStatusAsText(m_iStatus, true));

	int adminState = IF_ADMIN_STATE_UNKNOWN;
	int operState = IF_OPER_STATE_UNKNOWN;
   BOOL bNeedPoll = TRUE;

   // Poll interface using different methods
   if ((pNode->getFlags() & NF_IS_NATIVE_AGENT) &&
       (!(pNode->getFlags() & NF_DISABLE_NXCP)) && (!(pNode->getRuntimeFlags() & NDF_AGENT_UNREACHABLE)))
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

   if (bNeedPoll && (pNode->getFlags() & NF_IS_SNMP) &&
       (!(pNode->getFlags() & NF_DISABLE_SNMP)) && (!(pNode->getRuntimeFlags() & NDF_SNMP_UNREACHABLE)) &&
		 (snmpTransport != NULL))
   {
      sendPollerMsg(rqId, _T("      Retrieving interface status from SNMP agent\r\n"));
      pNode->getInterfaceStatusFromSNMP(snmpTransport, m_index, &adminState, &operState);
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
		// Pings cannot be used for cluster sync interfaces
      if ((pNode->getFlags() & NF_DISABLE_ICMP) || clusterSync || (m_dwIpAddr == 0) || isLoopback())
      {
			// Interface doesn't have an IP address, so we can't ping it
			sendPollerMsg(rqId, POLLER_WARNING _T("      Interface status cannot be determined\r\n"));
			DbgPrintf(7, _T("Interface::StatusPoll(%d,%s): cannot use ping for status check"), m_id, m_name);
      }
      else
      {
         // Use ICMP ping as a last option
			UINT32 icmpProxy = nodeIcmpProxy;

			if (IsZoningEnabled() && (m_zoneId != 0) && (icmpProxy == 0))
			{
				Zone *zone = (Zone *)g_idxZoneByGUID.get(m_zoneId);
				if (zone != NULL)
				{
					icmpProxy = zone->getIcmpProxy();
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
						TCHAR parameter[64], buffer[64];

						_sntprintf(parameter, 64, _T("Icmp.Ping(%s)"), IpToStr(m_dwIpAddr, buffer));
						if (conn->getParameter(parameter, 64, buffer) == ERR_SUCCESS)
						{
							DbgPrintf(7, _T("Interface::StatusPoll(%d,%s): proxy response: \"%s\""), m_id, m_name, buffer);
							TCHAR *eptr;
							long value = _tcstol(buffer, &eptr, 10);
							if ((*eptr == 0) && (value >= 0))
							{
								if (value < 10000)
								{
									adminState = IF_ADMIN_STATE_UP;
									operState = IF_OPER_STATE_UP;
								}
								else
								{
									adminState = IF_ADMIN_STATE_UNKNOWN;
									operState = IF_OPER_STATE_DOWN;
								}
							}
						}
						conn->disconnect();
						delete conn;
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
				DbgPrintf(7, _T("Interface::StatusPoll(%d,%s): calling IcmpPing(0x%08X,3,%d,%d,%d)"), m_id, m_name, htonl(m_dwIpAddr), g_icmpPingTimeout, m_pingTime, g_icmpPingSize);
				UINT32 dwPingStatus = IcmpPing(htonl(m_dwIpAddr), 3, g_icmpPingTimeout, &m_pingTime, g_icmpPingSize);
            m_pingLastTimeStamp = time(NULL);
				if (dwPingStatus == ICMP_SUCCESS)
				{
					adminState = IF_ADMIN_STATE_UP;
					operState = IF_OPER_STATE_UP;
				}
				else
				{
               m_pingTime = PING_TIME_ON_TIMEOUT;
					adminState = IF_ADMIN_STATE_UNKNOWN;
					operState = IF_OPER_STATE_DOWN;
				}
				DbgPrintf(7, _T("Interface::StatusPoll(%d,%s): ping result %d, adminState=%d, operState=%d"), m_id, m_name, dwPingStatus, adminState, operState);
			}
      }
   }

	// Calculate interface object status based on admin state, oper state, and expected state
   int oldStatus = m_iStatus;
	int newStatus;
	int expectedState = (m_flags & IF_EXPECTED_STATE_MASK) >> 28;
	switch(adminState)
	{
		case IF_ADMIN_STATE_UP:
		case IF_ADMIN_STATE_UNKNOWN:
			switch(operState)
			{
				case IF_OPER_STATE_UP:
					newStatus = ((expectedState == IF_EXPECTED_STATE_DOWN) ? STATUS_CRITICAL : STATUS_NORMAL);
					break;
				case IF_OPER_STATE_DOWN:
					newStatus = ((expectedState == IF_EXPECTED_STATE_UP) ? STATUS_CRITICAL : STATUS_NORMAL);
					break;
				case IF_OPER_STATE_TESTING:
					newStatus = STATUS_TESTING;
					break;
				default:
					newStatus = STATUS_UNKNOWN;
					break;
			}
			break;
		case IF_ADMIN_STATE_DOWN:
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
	if ((pNode->getFlags() & NF_IS_8021X) && isPhysicalPort() && (snmpTransport != NULL))
	{
		DbgPrintf(5, _T("StatusPoll(%s): Checking 802.1x state for interface %s"), pNode->getName(), m_name);
		paeStatusPoll(session, rqId, snmpTransport, pNode);
		if ((m_dot1xPaeAuthState == PAE_STATE_FORCE_UNAUTH) && (newStatus < STATUS_MAJOR))
			newStatus = STATUS_MAJOR;
	}

	// Reset status to unknown if node has known network connectivity problems
	if ((newStatus == STATUS_CRITICAL) && (pNode->getRuntimeFlags() & NDF_NETWORK_PATH_PROBLEM))
	{
		newStatus = STATUS_UNKNOWN;
		DbgPrintf(6, _T("StatusPoll(%s): Status for interface %s reset to UNKNOWN"), pNode->getName(), m_name);
	}

	if (newStatus == m_pendingStatus)
	{
		m_pollCount++;
	}
	else
	{
		m_pendingStatus = newStatus;
		m_pollCount = 1;
	}

	int requiredPolls = (m_requiredPollCount > 0) ? m_requiredPollCount : g_requiredPolls;
	sendPollerMsg(rqId, _T("      Interface is %s for %d poll%s (%d poll%s required for status change)\r\n"),
	              GetStatusAsText(newStatus, true), m_pollCount, (m_pollCount == 1) ? _T("") : _T("s"),
	              requiredPolls, (requiredPolls == 1) ? _T("") : _T("s"));
	DbgPrintf(7, _T("Interface::StatusPoll(%d,%s): newStatus=%d oldStatus=%d pollCount=%d requiredPolls=%d"),
	          m_id, m_name, newStatus, oldStatus, m_pollCount, requiredPolls);

   if ((newStatus != oldStatus) && (m_pollCount >= requiredPolls) && (expectedState != IF_EXPECTED_STATE_IGNORE))
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

		DbgPrintf(7, _T("Interface::StatusPoll(%d,%s): status changed from %d to %d"), m_id, m_name, m_iStatus, newStatus);
		m_iStatus = newStatus;
		m_pendingStatus = -1;	// Invalidate pending status
      if (!m_isSystem)
      {
		   sendPollerMsg(rqId, _T("      Interface status changed to %s\r\n"), GetStatusAsText(m_iStatus, true));
		   PostEventEx(eventQueue,
		               (expectedState == IF_EXPECTED_STATE_DOWN) ? statusToEventInverted[m_iStatus] : statusToEvent[m_iStatus],
						   pNode->getId(), "dsaad", m_id, m_name, m_dwIpAddr, m_ipNetMask, m_index);
      }
   }
	else if (expectedState == IF_EXPECTED_STATE_IGNORE)
	{
		m_iStatus = (newStatus <= STATUS_CRITICAL) ? STATUS_NORMAL : newStatus;
		if (m_iStatus != oldStatus)
			m_pendingStatus = -1;	// Invalidate pending status
	}

	lockProperties();
	if ((m_iStatus != oldStatus) || (adminState != (int)m_adminState) || (operState != (int)m_operState))
	{
		m_adminState = (WORD)adminState;
		m_operState = (WORD)operState;
		setModified();
	}
	unlockProperties();

	sendPollerMsg(rqId, _T("      Interface status after poll is %s\r\n"), GetStatusAsText(m_iStatus, true));
	sendPollerMsg(rqId, _T("   Finished status poll on interface %s\r\n"), m_name);
}

/**
 * Updates last ping time and ping time
 */
void Interface::updatePingData()
{
   UINT32 dwPingStatus = IcmpPing(htonl(m_dwIpAddr), 3, g_icmpPingTimeout, &m_pingTime, g_icmpPingSize);
   if (dwPingStatus != ICMP_SUCCESS)
   {
      m_pingTime = PING_TIME_ON_TIMEOUT;
   }
   m_pingLastTimeStamp = time(NULL);
}

/**
 * PAE (802.1x) status poll
 */
void Interface::paeStatusPoll(ClientSession *pSession, UINT32 rqId, SNMP_Transport *pTransport, Node *node)
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
		setModified();
		unlockProperties();
	}
}

/**
 * Create NXCP message with object's data
 */
void Interface::fillMessage(NXCPMessage *pMsg)
{
   NetObj::fillMessage(pMsg);
   pMsg->setField(VID_IF_INDEX, m_index);
   pMsg->setField(VID_IF_TYPE, m_type);
   pMsg->setField(VID_MTU, m_mtu);
   pMsg->setField(VID_IF_SLOT, m_slotNumber);
   pMsg->setField(VID_IF_PORT, m_portNumber);
   pMsg->setField(VID_IP_NETMASK, m_ipNetMask);
   pMsg->setField(VID_MAC_ADDR, m_macAddr, MAC_ADDR_LENGTH);
	pMsg->setField(VID_FLAGS, m_flags);
	pMsg->setField(VID_REQUIRED_POLLS, (WORD)m_requiredPollCount);
	pMsg->setField(VID_PEER_NODE_ID, m_peerNodeId);
	pMsg->setField(VID_PEER_INTERFACE_ID, m_peerInterfaceId);
	pMsg->setField(VID_PEER_PROTOCOL, (INT16)m_peerDiscoveryProtocol);
	pMsg->setField(VID_DESCRIPTION, m_description);
   pMsg->setField(VID_ALIAS, m_alias);
	pMsg->setField(VID_ADMIN_STATE, m_adminState);
	pMsg->setField(VID_OPER_STATE, m_operState);
	pMsg->setField(VID_DOT1X_PAE_STATE, m_dot1xPaeAuthState);
	pMsg->setField(VID_DOT1X_BACKEND_STATE, m_dot1xBackendAuthState);
	pMsg->setField(VID_ZONE_ID, m_zoneId);
}

/**
 * Modify object from message
 */
UINT32 Interface::modifyFromMessage(NXCPMessage *pRequest, BOOL bAlreadyLocked)
{
   if (!bAlreadyLocked)
      lockProperties();

   // Number of required polls
   if (pRequest->isFieldExist(VID_REQUIRED_POLLS))
      m_requiredPollCount = (int)pRequest->getFieldAsUInt16(VID_REQUIRED_POLLS);

	// Expected interface state
	if (pRequest->isFieldExist(VID_EXPECTED_STATE))
	{
      UINT32 expectedState = pRequest->getFieldAsUInt16(VID_EXPECTED_STATE);
		m_flags &= ~IF_EXPECTED_STATE_MASK;
		m_flags |= expectedState << 28;
	}

	// Flags
	if (pRequest->isFieldExist(VID_FLAGS))
	{
      UINT32 newFlags = pRequest->getFieldAsUInt32(VID_FLAGS);
      newFlags &= IF_USER_FLAGS_MASK;
		m_flags &= ~IF_USER_FLAGS_MASK;
		m_flags |= newFlags;
	}

   return NetObj::modifyFromMessage(pRequest, TRUE);
}

/**
 * Set expected state for interface
 */
void Interface::setExpectedState(int state)
{
	lockProperties();
	m_flags &= ~IF_EXPECTED_STATE_MASK;
	m_flags |= (UINT32)state << 28;
	setModified();
	unlockProperties();
}

/**
 * Wake up node bound to this interface by sending magic packet
 */
UINT32 Interface::wakeUp()
{
   UINT32 dwAddr, dwResult = RCC_NO_MAC_ADDRESS;

   if (memcmp(m_macAddr, "\x00\x00\x00\x00\x00\x00", 6))
   {
      dwAddr = htonl(m_dwIpAddr | ~m_ipNetMask);
      if (SendMagicPacket(dwAddr, m_macAddr, 5))
         dwResult = RCC_SUCCESS;
      else
         dwResult = RCC_COMM_FAILURE;
   }
   return dwResult;
}

/**
 * Get interface's parent node
 */
Node *Interface::getParentNode()
{
   UINT32 i;
   Node *pNode = NULL;

   LockParentList(FALSE);
   for(i = 0; i < m_dwParentCount; i++)
      if (m_pParentList[i]->getObjectClass() == OBJECT_NODE)
      {
         pNode = (Node *)m_pParentList[i];
         break;
      }
   UnlockParentList();
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
 * Change interface's IP address
 */
void Interface::setIpAddr(UINT32 dwNewAddr)
{
   UpdateInterfaceIndex(m_dwIpAddr, dwNewAddr, this);
   lockProperties();
   m_dwIpAddr = dwNewAddr;
   setModified();
   unlockProperties();
}

/**
 * Change interface's IP subnet mask
 */
void Interface::setIpNetMask(UINT32 dwNetMask)
{
   UINT32 oldNetMask = m_ipNetMask;
   lockProperties();
   m_ipNetMask = dwNetMask;
   setModified();
   unlockProperties();
   PostEvent(EVENT_IF_MASK_CHANGED, m_id, "dsaada", m_id,
                m_name, m_dwIpAddr,
                m_ipNetMask, m_index, oldNetMask);
}

/**
 * Update zone ID. New zone ID taken from parent node.
 */
void Interface::updateZoneId()
{
	Node *node = getParentNode();
	if (node != NULL)
	{
		// Unregister from old zone
		Zone *zone = (Zone *)g_idxZoneByGUID.get(m_zoneId);
		if (zone != NULL)
			zone->removeFromIndex(this);

		lockProperties();
		m_zoneId = node->getZoneId();
		setModified();
		unlockProperties();

		// Register in new zone
		zone = (Zone *)g_idxZoneByGUID.get(m_zoneId);
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
		setModified();
		unlockProperties();
	}
	NetObj::onObjectDelete(dwObjectId);
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
         setModified();
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
   setModified();
   if (!m_isSystem)
   {
      static const TCHAR *names[] = { _T("localIfId"), _T("localIfIndex"), _T("localIfName"),
         _T("localIfIP"), _T("localIfMAC"), _T("remoteNodeId"), _T("remoteNodeName"),
         _T("remoteIfId"), _T("remoteIfIndex"), _T("remoteIfName"), _T("remoteIfIP"),
         _T("remoteIfMAC"), _T("protocol") };
      PostEventWithNames(EVENT_IF_PEER_CHANGED, getParentNodeId(), "ddsahdsddsah", names,
         m_id, m_index, m_name, m_dwIpAddr, m_macAddr, node->getId(), node->getName(),
         iface->getId(), iface->getIfIndex(), iface->getName(), iface->IpAddr(), iface->getMacAddr(),
         protocol);
   }
}

/**
 * Set MAC address for interface
 */
void Interface::setMacAddr(const BYTE *pbNewMac) 
{
   lockProperties();
   MacDbRemove(m_macAddr);
   memcpy(m_macAddr, pbNewMac, MAC_ADDR_LENGTH);
   MacDbAddInterface(this);
   setModified(); 
   unlockProperties();
}
