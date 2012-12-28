/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2012 Victor Kirhenshtein
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
Interface::Interface() : NetObj()
{
	m_flags = 0;
	nx_strncpy(m_description, m_szName, MAX_DB_STRING);
   m_dwIpNetMask = 0;
   m_dwIfIndex = 0;
   m_dwIfType = IFTYPE_OTHER;
	m_bridgePortNumber = 0;
	m_slotNumber = 0;
	m_portNumber = 0;
	m_peerNodeId = 0;
	m_peerInterfaceId = 0;
	m_dot1xPaeAuthState = PAE_STATE_UNKNOWN;
	m_dot1xBackendAuthState = BACKEND_STATE_UNKNOWN;
   m_qwLastDownEventId = 0;
	m_iPendingStatus = -1;
	m_iPollCount = 0;
	m_iRequiredPollCount = 0;	// Use system default
	m_zoneId = 0;
}


//
// Constructor for "fake" interface object
//

Interface::Interface(DWORD dwAddr, DWORD dwNetMask, DWORD zoneId, bool bSyntheticMask) : NetObj()
{
	m_flags = bSyntheticMask ? IF_SYNTHETIC_MASK : 0;
	if ((dwAddr & 0xFF000000) == 0x7F000000)
		m_flags |= IF_LOOPBACK;

	_tcscpy(m_szName, _T("unknown"));
   _tcscpy(m_description, _T("unknown"));
   m_dwIpAddr = dwAddr;
   m_dwIpNetMask = dwNetMask;
   m_dwIfIndex = 1;
   m_dwIfType = IFTYPE_OTHER;
	m_bridgePortNumber = 0;
	m_slotNumber = 0;
	m_portNumber = 0;
	m_peerNodeId = 0;
	m_peerInterfaceId = 0;
	m_dot1xPaeAuthState = PAE_STATE_UNKNOWN;
	m_dot1xBackendAuthState = BACKEND_STATE_UNKNOWN;
   memset(m_bMacAddr, 0, MAC_ADDR_LENGTH);
   m_qwLastDownEventId = 0;
	m_iPendingStatus = -1;
	m_iPollCount = 0;
	m_iRequiredPollCount = 0;	// Use system default
	m_zoneId = zoneId;
   m_bIsHidden = TRUE;
}


//
// Constructor for normal interface object
//

Interface::Interface(const TCHAR *name, const TCHAR *descr, DWORD index, DWORD ipAddr, DWORD ipNetMask, DWORD ifType, DWORD zoneId)
          : NetObj()
{
	if (((ipAddr & 0xFF000000) == 0x7F000000) || (ifType == IFTYPE_SOFTWARE_LOOPBACK))
		m_flags = IF_LOOPBACK;
	else
		m_flags = 0;

   nx_strncpy(m_szName, name, MAX_OBJECT_NAME);
   nx_strncpy(m_description, descr, MAX_DB_STRING);
   m_dwIfIndex = index;
   m_dwIfType = ifType;
   m_dwIpAddr = ipAddr;
   m_dwIpNetMask = ipNetMask;
	m_bridgePortNumber = 0;
	m_slotNumber = 0;
	m_portNumber = 0;
	m_peerNodeId = 0;
	m_peerInterfaceId = 0;
	m_dot1xPaeAuthState = PAE_STATE_UNKNOWN;
	m_dot1xBackendAuthState = BACKEND_STATE_UNKNOWN;
   memset(m_bMacAddr, 0, MAC_ADDR_LENGTH);
   m_qwLastDownEventId = 0;
	m_iPendingStatus = -1;
	m_iPollCount = 0;
	m_iRequiredPollCount = 0;	// Use system default
	m_zoneId = zoneId;
   m_bIsHidden = TRUE;
}


//
// Interface class destructor
//

Interface::~Interface()
{
}


//
// Create object from database data
//

BOOL Interface::CreateFromDB(DWORD dwId)
{
   BOOL bResult = FALSE;

   m_dwId = dwId;

   if (!loadCommonProperties())
      return FALSE;

	DB_STATEMENT hStmt = DBPrepare(g_hCoreDB, 
		_T("SELECT ip_addr,ip_netmask,if_type,if_index,node_id,")
		_T("mac_addr,flags,required_polls,bridge_port,phy_slot,")
		_T("phy_port,peer_node_id,peer_if_id,description,")
		_T("dot1x_pae_state,dot1x_backend_state,admin_state,oper_state FROM interfaces WHERE id=?"));
	if (hStmt == NULL)
		return FALSE;
	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_dwId);

	DB_RESULT hResult = DBSelectPrepared(hStmt);
   if (hResult == NULL)
	{
		DBFreeStatement(hStmt);
      return FALSE;     // Query failed
	}

   if (DBGetNumRows(hResult) != 0)
   {
      m_dwIpAddr = DBGetFieldIPAddr(hResult, 0, 0);
      m_dwIpNetMask = DBGetFieldIPAddr(hResult, 0, 1);
      m_dwIfType = DBGetFieldULong(hResult, 0, 2);
      m_dwIfIndex = DBGetFieldULong(hResult, 0, 3);
      DWORD nodeId = DBGetFieldULong(hResult, 0, 4);
		DBGetFieldByteArray2(hResult, 0, 5, m_bMacAddr, MAC_ADDR_LENGTH, 0);
		m_flags = DBGetFieldULong(hResult, 0, 6);
      m_iRequiredPollCount = DBGetFieldLong(hResult, 0, 7);
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

      // Link interface to node
      if (!m_bIsDeleted)
      {
         NetObj *object = FindObjectById(nodeId);
         if (object == NULL)
         {
            nxlog_write(MSG_INVALID_NODE_ID, EVENTLOG_ERROR_TYPE, "dd", dwId, nodeId);
         }
         else if (object->Type() != OBJECT_NODE)
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
	if (((m_dwIpAddr & 0xFF000000) == 0x7F000000) || (m_dwIfType == IFTYPE_SOFTWARE_LOOPBACK))
		m_flags |= IF_LOOPBACK;

   return bResult;
}


//
// Save interface object to database
//

BOOL Interface::SaveToDB(DB_HANDLE hdb)
{
   TCHAR szMacStr[16], szIpAddr[16], szNetMask[16];
   DWORD dwNodeId;

   LockData();

   if (!saveCommonProperties(hdb))
	{
		UnlockData();
		return FALSE;
	}

   // Determine owning node's ID
   Node *pNode = getParentNode();
   if (pNode != NULL)
      dwNodeId = pNode->Id();
   else
      dwNodeId = 0;

   // Form and execute INSERT or UPDATE query
   BinToStr(m_bMacAddr, MAC_ADDR_LENGTH, szMacStr);
	DB_STATEMENT hStmt;
   if (IsDatabaseRecordExist(hdb, _T("interfaces"), _T("id"), m_dwId))
	{
		hStmt = DBPrepare(hdb,
			_T("UPDATE interfaces SET ip_addr=?,ip_netmask=?,")
         _T("node_id=?,if_type=?,if_index=?,mac_addr=?,flags=?,")
			_T("required_polls=?,bridge_port=?,phy_slot=?,phy_port=?,")
			_T("peer_node_id=?,peer_if_id=?,description=?,admin_state=?,")
			_T("oper_state=?,dot1x_pae_state=?,dot1x_backend_state=? WHERE id=?"));
	}
   else
	{
		hStmt = DBPrepare(hdb,
			_T("INSERT INTO interfaces (ip_addr,ip_netmask,node_id,if_type,if_index,mac_addr,")
			_T("flags,required_polls,bridge_port,phy_slot,phy_port,peer_node_id,peer_if_id,description,")
         _T("admin_state,oper_state,dot1x_pae_state,dot1x_backend_state,id) ")
			_T("VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)"));
	}
	if (hStmt == NULL)
	{
		UnlockData();
		return FALSE;
	}

	DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, IpToStr(m_dwIpAddr, szIpAddr), DB_BIND_STATIC);
	DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, IpToStr(m_dwIpNetMask, szNetMask), DB_BIND_STATIC);
	DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, dwNodeId);
	DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, m_dwIfType);
	DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, m_dwIfIndex);
	DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, szMacStr, DB_BIND_STATIC);
	DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, m_flags);
	DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, (LONG)m_iRequiredPollCount);
	DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, m_bridgePortNumber);
	DBBind(hStmt, 10, DB_SQLTYPE_INTEGER, m_slotNumber);
	DBBind(hStmt, 11, DB_SQLTYPE_INTEGER, m_portNumber);
	DBBind(hStmt, 12, DB_SQLTYPE_INTEGER, m_peerNodeId);
	DBBind(hStmt, 13, DB_SQLTYPE_INTEGER, m_peerInterfaceId);
	DBBind(hStmt, 14, DB_SQLTYPE_VARCHAR, m_description, DB_BIND_STATIC);
	DBBind(hStmt, 15, DB_SQLTYPE_INTEGER, (DWORD)m_adminState);
	DBBind(hStmt, 16, DB_SQLTYPE_INTEGER, (DWORD)m_operState);
	DBBind(hStmt, 17, DB_SQLTYPE_INTEGER, (DWORD)m_dot1xPaeAuthState);
	DBBind(hStmt, 18, DB_SQLTYPE_INTEGER, (DWORD)m_dot1xBackendAuthState);
	DBBind(hStmt, 19, DB_SQLTYPE_INTEGER, m_dwId);

	BOOL success = DBExecute(hStmt);
	DBFreeStatement(hStmt);

   // Save access list
	if (success)
		success = saveACLToDB(hdb);

   // Clear modifications flag and unlock object
	if (success)
		m_bIsModified = FALSE;
   UnlockData();

   return success;
}


//
// Delete interface object from database
//

BOOL Interface::DeleteFromDB()
{
   TCHAR szQuery[128];
   BOOL bSuccess;

   bSuccess = NetObj::DeleteFromDB();
   if (bSuccess)
   {
      _sntprintf(szQuery, 128, _T("DELETE FROM interfaces WHERE id=%d"), m_dwId);
      QueueSQLRequest(szQuery);
   }
   return bSuccess;
}

/**
 * Perform status poll on interface
 */
void Interface::StatusPoll(ClientSession *pSession, DWORD dwRqId,	Queue *pEventQueue, 
									BOOL bClusterSync, SNMP_Transport *pTransport)
{
   m_pPollRequestor = pSession;
   Node *pNode = getParentNode();
   if (pNode == NULL)
   {
      m_iStatus = STATUS_UNKNOWN;
      return;     // Cannot find parent node, which is VERY strange
   }

   SendPollerMsg(dwRqId, _T("   Starting status poll on interface %s\r\n"), m_szName);
   SendPollerMsg(dwRqId, _T("      Current interface status is %s\r\n"), g_szStatusText[m_iStatus]);

	int adminState = IF_ADMIN_STATE_UNKNOWN;
	int operState = IF_OPER_STATE_UNKNOWN;
   BOOL bNeedPoll = TRUE;

   // Poll interface using different methods
   if ((pNode->getFlags() & NF_IS_NATIVE_AGENT) &&
       (!(pNode->getFlags() & NF_DISABLE_NXCP)) && (!(pNode->getRuntimeFlags() & NDF_AGENT_UNREACHABLE)))
   {
      SendPollerMsg(dwRqId, _T("      Retrieving interface status from NetXMS agent\r\n"));
      pNode->getInterfaceStatusFromAgent(m_dwIfIndex, &adminState, &operState);
		DbgPrintf(7, _T("Interface::StatusPoll(%d,%s): new state from NetXMS agent: adinState=%d operState=%d"), m_dwId, m_szName, adminState, operState);
		if ((adminState != IF_ADMIN_STATE_UNKNOWN) && (operState != IF_OPER_STATE_UNKNOWN))
		{
			SendPollerMsg(dwRqId, POLLER_INFO _T("      Interface status retrieved from NetXMS agent\r\n"));
         bNeedPoll = FALSE;
		}
		else
		{
			SendPollerMsg(dwRqId, POLLER_WARNING _T("      Unable to retrieve interface status from NetXMS agent\r\n"));
		}
   }

   if (bNeedPoll && (pNode->getFlags() & NF_IS_SNMP) &&
       (!(pNode->getFlags() & NF_DISABLE_SNMP)) && (!(pNode->getRuntimeFlags() & NDF_SNMP_UNREACHABLE)) &&
		 (pTransport != NULL))
   {
      SendPollerMsg(dwRqId, _T("      Retrieving interface status from SNMP agent\r\n"));
      pNode->getInterfaceStatusFromSNMP(pTransport, m_dwIfIndex, &adminState, &operState);
		DbgPrintf(7, _T("Interface::StatusPoll(%d,%s): new state from SNMP: adminState=%d operState=%d"), m_dwId, m_szName, adminState, operState);
		if ((adminState != IF_ADMIN_STATE_UNKNOWN) && (operState != IF_OPER_STATE_UNKNOWN))
		{
			SendPollerMsg(dwRqId, POLLER_INFO _T("      Interface status retrieved from SNMP agent\r\n"));
         bNeedPoll = FALSE;
		}
		else
		{
			SendPollerMsg(dwRqId, POLLER_WARNING _T("      Unable to retrieve interface status from SNMP agent\r\n"));
		}
   }
   
   if (bNeedPoll)
   {
		// Pings cannot be used for cluster sync interfaces
      if ((pNode->getFlags() & NF_DISABLE_ICMP) || bClusterSync || (m_dwIpAddr == 0) || isLoopback())
      {
			// Interface doesn't have an IP address, so we can't ping it
			SendPollerMsg(dwRqId, POLLER_WARNING _T("      Interface status cannot be determined\r\n"));
			DbgPrintf(7, _T("Interface::StatusPoll(%d,%s): cannot use ping for status check"), m_dwId, m_szName);
      }
      else
      {
         // Use ICMP ping as a last option
			DWORD icmpProxy = 0;

			if (IsZoningEnabled() && (m_zoneId != 0))
			{
				Zone *zone = (Zone *)g_idxZoneByGUID.get(m_zoneId);
				if (zone != NULL)
				{
					icmpProxy = zone->getIcmpProxy();
				}
			}

			if (icmpProxy != 0)
			{
				SendPollerMsg(dwRqId, _T("      Starting ICMP ping via proxy\r\n"));
				DbgPrintf(7, _T("Interface::StatusPoll(%d,%s): ping via proxy [%u]"), m_dwId, m_szName, icmpProxy);
				Node *proxyNode = (Node *)g_idxNodeById.get(icmpProxy);
				if ((proxyNode != NULL) && proxyNode->isNativeAgent() && !proxyNode->isDown())
				{
					DbgPrintf(7, _T("Interface::StatusPoll(%d,%s): proxy node found: %s"), m_dwId, m_szName, proxyNode->Name());
					AgentConnection *conn = proxyNode->createAgentConnection();
					if (conn != NULL)
					{
						TCHAR parameter[64], buffer[64];

						_sntprintf(parameter, 64, _T("Icmp.Ping(%s)"), IpToStr(m_dwIpAddr, buffer));
						if (conn->getParameter(parameter, 64, buffer) == ERR_SUCCESS)
						{
							DbgPrintf(7, _T("Interface::StatusPoll(%d,%s): proxy response: \"%s\""), m_dwId, m_szName, buffer);
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
						DbgPrintf(7, _T("Interface::StatusPoll(%d,%s): cannot connect to agent on proxy node"), m_dwId, m_szName);
						SendPollerMsg(dwRqId, POLLER_ERROR _T("      Unable to establish connection with proxy node\r\n"));
					}
				}
				else
				{
					DbgPrintf(7, _T("Interface::StatusPoll(%d,%s): proxy node not available"), m_dwId, m_szName);
					SendPollerMsg(dwRqId, POLLER_ERROR _T("      ICMP proxy not available\r\n"));
				}
			}
			else	// not using ICMP proxy
			{
				SendPollerMsg(dwRqId, _T("      Starting ICMP ping\r\n"));
				DbgPrintf(7, _T("Interface::StatusPoll(%d,%s): calling IcmpPing(0x%08X,3,1500,NULL,%d)"), m_dwId, m_szName, htonl(m_dwIpAddr), g_dwPingSize);
				DWORD dwPingStatus = IcmpPing(htonl(m_dwIpAddr), 3, 1500, NULL, g_dwPingSize);
				if (dwPingStatus == ICMP_RAW_SOCK_FAILED)
					nxlog_write(MSG_RAW_SOCK_FAILED, EVENTLOG_WARNING_TYPE, NULL);
				if (dwPingStatus == ICMP_SUCCESS)
				{
					adminState = IF_ADMIN_STATE_UP;
					operState = IF_OPER_STATE_UP;
				}
				else
				{
					adminState = IF_ADMIN_STATE_UNKNOWN;
					operState = IF_OPER_STATE_DOWN;
				}
				DbgPrintf(7, _T("Interface::StatusPoll(%d,%s): ping result %d, adminState=%d, operState=%d"), m_dwId, m_szName, dwPingStatus, adminState, operState);
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
	if ((pNode->getFlags() & NF_IS_8021X) && isPhysicalPort())
	{
		DbgPrintf(5, _T("StatusPoll(%s): Checking 802.1x state for interface %s"), pNode->Name(), m_szName);
		paeStatusPoll(pSession, dwRqId, pTransport, pNode);
		if ((m_dot1xPaeAuthState == PAE_STATE_FORCE_UNAUTH) && (newStatus < STATUS_MAJOR))
			newStatus = STATUS_MAJOR;
	}

	// Reset status to unknown if node has known network connectivity problems
	if ((newStatus == STATUS_CRITICAL) && (pNode->getRuntimeFlags() & NDF_NETWORK_PATH_PROBLEM))
	{
		newStatus = STATUS_UNKNOWN;
		DbgPrintf(6, _T("StatusPoll(%s): Status for interface %s reset to UNKNOWN"), pNode->Name(), m_szName);
	}
   
	if (newStatus == m_iPendingStatus)
	{
		m_iPollCount++;
	}
	else
	{
		m_iPendingStatus = newStatus;
		m_iPollCount = 1;
	}

	int requiredPolls = (m_iRequiredPollCount > 0) ? m_iRequiredPollCount : g_nRequiredPolls;
	SendPollerMsg(dwRqId, _T("      Interface is %s for %d poll%s (%d poll%s required for status change)\r\n"),
	              g_szStatusText[newStatus], m_iPollCount, (m_iPollCount == 1) ? _T("") : _T("s"),
	              requiredPolls, (requiredPolls == 1) ? _T("") : _T("s"));
	DbgPrintf(7, _T("Interface::StatusPoll(%d,%s): newStatus=%d oldStatus=%d pollCount=%d requiredPolls=%d"),
	          m_dwId, m_szName, newStatus, oldStatus, m_iPollCount, requiredPolls);

   if ((newStatus != oldStatus) && (m_iPollCount >= requiredPolls) && (expectedState != IF_EXPECTED_STATE_IGNORE))
   {
		static DWORD statusToEvent[] =
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
		static DWORD statusToEventInverted[] =
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

		DbgPrintf(7, _T("Interface::StatusPoll(%d,%s): status changed from %d to %d"), m_dwId, m_szName, m_iStatus, newStatus);
		m_iStatus = newStatus;
		m_iPendingStatus = -1;	// Invalidate pending status
		SendPollerMsg(dwRqId, _T("      Interface status changed to %s\r\n"), g_szStatusText[m_iStatus]);
		PostEventEx(pEventQueue, 
		            (expectedState == IF_EXPECTED_STATE_DOWN) ? statusToEventInverted[m_iStatus] : statusToEvent[m_iStatus],
						pNode->Id(), "dsaad", m_dwId, m_szName, m_dwIpAddr, m_dwIpNetMask, m_dwIfIndex);
   }
	else if (expectedState == IF_EXPECTED_STATE_IGNORE)
	{
		m_iStatus = (newStatus <= STATUS_CRITICAL) ? STATUS_NORMAL : newStatus;
		if (m_iStatus != oldStatus)
			m_iPendingStatus = -1;	// Invalidate pending status
	}

	LockData();
	if ((m_iStatus != oldStatus) || (adminState != (int)m_adminState) || (operState != (int)m_operState))
	{
		m_adminState = (WORD)adminState;
		m_operState = (WORD)operState;
		Modify();
	}
	UnlockData();
	
	SendPollerMsg(dwRqId, _T("      Interface status after poll is %s\r\n"), g_szStatusText[m_iStatus]);
	SendPollerMsg(dwRqId, _T("   Finished status poll on interface %s\r\n"), m_szName);
}


//
// PAE (802.1x) status poll
//

void Interface::paeStatusPoll(ClientSession *pSession, DWORD dwRqId, SNMP_Transport *pTransport, Node *node)
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
#define PAE_STATE_TEXT(x) ((((x) <= PAE_STATE_RESTART) && ((x) >= 0)) ? paeStateText[x] : paeStateText[0])
#define BACKEND_STATE_TEXT(x) ((((x) <= BACKEND_STATE_IGNORE) && ((x) >= 0)) ? backendStateText[x] : backendStateText[0])

   SendPollerMsg(dwRqId, _T("      Checking port 802.1x status...\r\n"));

	TCHAR oid[256];
	LONG paeState = PAE_STATE_UNKNOWN, backendState = BACKEND_STATE_UNKNOWN;
	bool modified = false;

	_sntprintf(oid, 256, _T(".1.0.8802.1.1.1.1.2.1.1.1.%d"), m_dwIfIndex);
	SnmpGet(pTransport->getSnmpVersion(), pTransport, oid, NULL, 0, &paeState, sizeof(LONG), 0);

	_sntprintf(oid, 256, _T(".1.0.8802.1.1.1.1.2.1.1.2.%d"), m_dwIfIndex);
	SnmpGet(pTransport->getSnmpVersion(), pTransport, oid, NULL, 0, &backendState, sizeof(LONG), 0);

	if (m_dot1xPaeAuthState != (WORD)paeState)
	{
	   SendPollerMsg(dwRqId, _T("      Port PAE state changed to %s...\r\n"), PAE_STATE_TEXT(paeState));
		modified = true;

		PostEvent(EVENT_8021X_PAE_STATE_CHANGED, node->Id(), "dsdsds", paeState, PAE_STATE_TEXT(paeState),
		          (DWORD)m_dot1xPaeAuthState, PAE_STATE_TEXT(m_dot1xPaeAuthState), m_dwId, m_szName);

		if (paeState == PAE_STATE_FORCE_UNAUTH)
		{
			PostEvent(EVENT_8021X_PAE_FORCE_UNAUTH, node->Id(), "ds", m_dwId, m_szName);
		}
	}

	if (m_dot1xBackendAuthState != (WORD)backendState)
	{
	   SendPollerMsg(dwRqId, _T("      Port backend state changed to %s...\r\n"), BACKEND_STATE_TEXT(backendState));
		modified = true;

		PostEvent(EVENT_8021X_BACKEND_STATE_CHANGED, node->Id(), "dsdsds", backendState, BACKEND_STATE_TEXT(backendState),
		          (DWORD)m_dot1xBackendAuthState, BACKEND_STATE_TEXT(m_dot1xBackendAuthState), m_dwId, m_szName);

		if (backendState == BACKEND_STATE_FAIL)
		{
			PostEvent(EVENT_8021X_AUTH_FAILED, node->Id(), "ds", m_dwId, m_szName);
		}
		else if (backendState == BACKEND_STATE_TIMEOUT)
		{
			PostEvent(EVENT_8021X_AUTH_TIMEOUT, node->Id(), "ds", m_dwId, m_szName);
		}
	}

	if (modified)
	{
		LockData();
		m_dot1xPaeAuthState = (WORD)paeState;
		m_dot1xBackendAuthState = (WORD)backendState;
		Modify();
		UnlockData();
	}
}


//
// Create CSCP message with object's data
//

void Interface::CreateMessage(CSCPMessage *pMsg)
{
   NetObj::CreateMessage(pMsg);
   pMsg->SetVariable(VID_IF_INDEX, m_dwIfIndex);
   pMsg->SetVariable(VID_IF_TYPE, m_dwIfType);
   pMsg->SetVariable(VID_IF_SLOT, m_slotNumber);
   pMsg->SetVariable(VID_IF_PORT, m_portNumber);
   pMsg->SetVariable(VID_IP_NETMASK, m_dwIpNetMask);
   pMsg->SetVariable(VID_MAC_ADDR, m_bMacAddr, MAC_ADDR_LENGTH);
	pMsg->SetVariable(VID_FLAGS, m_flags);
	pMsg->SetVariable(VID_REQUIRED_POLLS, (WORD)m_iRequiredPollCount);
	pMsg->SetVariable(VID_PEER_NODE_ID, m_peerNodeId);
	pMsg->SetVariable(VID_PEER_INTERFACE_ID, m_peerInterfaceId);
	pMsg->SetVariable(VID_DESCRIPTION, m_description);
	pMsg->SetVariable(VID_ADMIN_STATE, m_adminState);
	pMsg->SetVariable(VID_OPER_STATE, m_operState);
	pMsg->SetVariable(VID_DOT1X_PAE_STATE, m_dot1xPaeAuthState);
	pMsg->SetVariable(VID_DOT1X_BACKEND_STATE, m_dot1xBackendAuthState);
	pMsg->SetVariable(VID_ZONE_ID, m_zoneId);
}


//
// Modify object from message
//

DWORD Interface::ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked)
{
   if (!bAlreadyLocked)
      LockData();

   // Number of required polls
   if (pRequest->IsVariableExist(VID_REQUIRED_POLLS))
      m_iRequiredPollCount = (int)pRequest->GetVariableShort(VID_REQUIRED_POLLS);

	// Expected interface state
	if (pRequest->IsVariableExist(VID_EXPECTED_STATE))
	{
      DWORD expectedState = pRequest->GetVariableShort(VID_EXPECTED_STATE);
		m_flags &= ~IF_EXPECTED_STATE_MASK;
		m_flags |= expectedState << 28;
	}

   return NetObj::ModifyFromMessage(pRequest, TRUE);
}


//
// Wake up node bound to this interface by sending magic packet
//

DWORD Interface::wakeUp()
{
   DWORD dwAddr, dwResult = RCC_NO_MAC_ADDRESS;

   if (memcmp(m_bMacAddr, "\x00\x00\x00\x00\x00\x00", 6))
   {
      dwAddr = htonl(m_dwIpAddr | ~m_dwIpNetMask);
      if (SendMagicPacket(dwAddr, m_bMacAddr, 5))
         dwResult = RCC_SUCCESS;
      else
         dwResult = RCC_COMM_FAILURE;
   }
   return dwResult;
}


//
// Get interface's parent node
//

Node *Interface::getParentNode()
{
   DWORD i;
   Node *pNode = NULL;

   LockParentList(FALSE);
   for(i = 0; i < m_dwParentCount; i++)
      if (m_pParentList[i]->Type() == OBJECT_NODE)
      {
         pNode = (Node *)m_pParentList[i];
         break;
      }
   UnlockParentList();
   return pNode;
}


//
// Change interface's IP address
//

void Interface::setIpAddr(DWORD dwNewAddr) 
{
   UpdateInterfaceIndex(m_dwIpAddr, dwNewAddr, this);
   LockData();
   m_dwIpAddr = dwNewAddr;
   Modify();
   UnlockData();
}


//
// Change interface's IP subnet mask
//

void Interface::setIpNetMask(DWORD dwNetMask) 
{
   LockData();
   m_dwIpNetMask = dwNetMask;
   Modify();
   UnlockData();
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

		LockData();
		m_zoneId = node->getZoneId();
		Modify();
		UnlockData();

		// Register in new zone
		zone = (Zone *)g_idxZoneByGUID.get(m_zoneId);
		if (zone != NULL)
			zone->addToIndex(this);
	}
}


//
// Handler for object deletion notification
//

void Interface::OnObjectDelete(DWORD dwObjectId)
{
	if ((m_peerNodeId == dwObjectId) || (m_peerInterfaceId == dwObjectId))
	{
		LockData();
		m_peerNodeId = 0;
		m_peerInterfaceId = 0;
		Modify();
		UnlockData();
	}
	NetObj::OnObjectDelete(dwObjectId);
}
