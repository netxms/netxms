/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2011 Victor Kirhenshtein
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


//
// Default constructor for Interface object
//

Interface::Interface()
          : NetObj()
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

Interface::Interface(DWORD dwAddr, DWORD dwNetMask, DWORD zoneId, bool bSyntheticMask)
          : NetObj()
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
   TCHAR szQuery[256], szBuffer[MAX_DB_STRING];
   DB_RESULT hResult;
   DWORD dwNodeId;
   NetObj *pObject;
   BOOL bResult = FALSE;

   m_dwId = dwId;

   if (!loadCommonProperties())
      return FALSE;

   _sntprintf(szQuery, 256, _T("SELECT ip_addr,ip_netmask,if_type,if_index,node_id,")
                            _T("mac_addr,flags,required_polls,bridge_port,phy_slot,")
									 _T("phy_port,peer_node_id,peer_if_id,description,")
									 _T("dot1x_pae_state,dot1x_backend_state FROM interfaces WHERE id=%d"), (int)dwId);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult == NULL)
      return FALSE;     // Query failed

   if (DBGetNumRows(hResult) != 0)
   {
      m_dwIpAddr = DBGetFieldIPAddr(hResult, 0, 0);
      m_dwIpNetMask = DBGetFieldIPAddr(hResult, 0, 1);
      m_dwIfType = DBGetFieldULong(hResult, 0, 2);
      m_dwIfIndex = DBGetFieldULong(hResult, 0, 3);
      dwNodeId = DBGetFieldULong(hResult, 0, 4);
      StrToBin(DBGetField(hResult, 0, 5, szBuffer, MAX_DB_STRING), m_bMacAddr, MAC_ADDR_LENGTH);
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

      // Link interface to node
      if (!m_bIsDeleted)
      {
         pObject = FindObjectById(dwNodeId);
         if (pObject == NULL)
         {
            nxlog_write(MSG_INVALID_NODE_ID, EVENTLOG_ERROR_TYPE, "dd", dwId, dwNodeId);
         }
         else if (pObject->Type() != OBJECT_NODE)
         {
            nxlog_write(MSG_NODE_NOT_NODE, EVENTLOG_ERROR_TYPE, "dd", dwId, dwNodeId);
         }
         else
         {
            pObject->AddChild(this);
            AddParent(pObject);
				m_zoneId = ((Node *)pObject)->getZoneId();
            bResult = TRUE;
         }
      }
      else
      {
         bResult = TRUE;
      }
   }

   DBFreeResult(hResult);

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
   TCHAR szQuery[2048], szMacStr[16], szIpAddr[16], szNetMask[16];
   BOOL bNewObject = TRUE;
   Node *pNode;
   DWORD dwNodeId;
   DB_RESULT hResult;

   // Lock object's access
   LockData();

   saveCommonProperties(hdb);

   // Check for object's existence in database
   _sntprintf(szQuery, 1024, _T("SELECT id FROM interfaces WHERE id=%d"), m_dwId);
   hResult = DBSelect(hdb, szQuery);
   if (hResult != 0)
   {
      if (DBGetNumRows(hResult) > 0)
         bNewObject = FALSE;
      DBFreeResult(hResult);
   }

   // Determine owning node's ID
   pNode = getParentNode();
   if (pNode != NULL)
      dwNodeId = pNode->Id();
   else
      dwNodeId = 0;

   // Form and execute INSERT or UPDATE query
   BinToStr(m_bMacAddr, MAC_ADDR_LENGTH, szMacStr);
   if (bNewObject)
      _sntprintf(szQuery, 2048, _T("INSERT INTO interfaces (id,ip_addr,")
                       _T("ip_netmask,node_id,if_type,if_index,mac_addr,flags,required_polls,")
							  _T("bridge_port,phy_slot,phy_port,peer_node_id,peer_if_id,description,")
                       _T("dot1x_pae_state,dot1x_backend_state) ")
							  _T("VALUES (%d,'%s','%s',%d,%d,%d,'%s',%d,%d,%d,%d,%d,%d,%d,%s,%d,%d)"),
              m_dwId, IpToStr(m_dwIpAddr, szIpAddr),
              IpToStr(m_dwIpNetMask, szNetMask), dwNodeId,
				  m_dwIfType, m_dwIfIndex, szMacStr, (int)m_flags,
				  m_iRequiredPollCount, (int)m_bridgePortNumber, (int)m_slotNumber,
				  (int)m_portNumber, (int)m_peerNodeId, (int)m_peerInterfaceId,
				  (const TCHAR *)DBPrepareString(hdb, m_description),
				  (int)m_dot1xPaeAuthState, (int)m_dot1xBackendAuthState);
   else
      _sntprintf(szQuery, 2048, _T("UPDATE interfaces SET ip_addr='%s',ip_netmask='%s',")
                       _T("node_id=%d,if_type=%d,if_index=%d,mac_addr='%s',flags=%d,")
							  _T("required_polls=%d,bridge_port=%d,phy_slot=%d,phy_port=%d,")
							  _T("peer_node_id=%d,peer_if_id=%d,description=%s,")
							  _T("dot1x_pae_state=%d,dot1x_backend_state=%d WHERE id=%d"),
              IpToStr(m_dwIpAddr, szIpAddr),
              IpToStr(m_dwIpNetMask, szNetMask), dwNodeId,
				  m_dwIfType, m_dwIfIndex, szMacStr, (int)m_flags,
				  m_iRequiredPollCount, (int)m_bridgePortNumber, (int)m_slotNumber,
				  (int)m_portNumber, (int)m_peerNodeId, (int)m_peerInterfaceId,
				  (const TCHAR *)DBPrepareString(hdb, m_description), 
				  (int)m_dot1xPaeAuthState, (int)m_dot1xBackendAuthState, (int)m_dwId);
   DBQuery(hdb, szQuery);

   // Save access list
   saveACLToDB(hdb);

   // Clear modifications flag and unlock object
   m_bIsModified = FALSE;
   UnlockData();

   return TRUE;
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


//
// Perform status poll on interface
//

void Interface::StatusPoll(ClientSession *pSession, DWORD dwRqId,
									Queue *pEventQueue, BOOL bClusterSync,
									SNMP_Transport *pTransport)
{
   int oldStatus = m_iStatus, newStatus;
   DWORD dwPingStatus;
   BOOL bNeedPoll = TRUE;
   Node *pNode;

   m_pPollRequestor = pSession;
   pNode = getParentNode();
   if (pNode == NULL)
   {
      m_iStatus = STATUS_UNKNOWN;
      return;     // Cannot find parent node, which is VERY strange
   }

   SendPollerMsg(dwRqId, _T("   Starting status poll on interface %s\r\n"), m_szName);
   SendPollerMsg(dwRqId, _T("      Current interface status is %s\r\n"), g_szStatusText[m_iStatus]);

   // Poll interface using different methods
   if ((pNode->getFlags() & NF_IS_NATIVE_AGENT) &&
       (!(pNode->getFlags() & NF_DISABLE_NXCP)) && (!(pNode->getRuntimeFlags() & NDF_AGENT_UNREACHABLE)))
   {
      SendPollerMsg(dwRqId, _T("      Retrieving interface status from NetXMS agent\r\n"));
      newStatus = pNode->getInterfaceStatusFromAgent(m_dwIfIndex);
		DbgPrintf(7, _T("Interface::StatusPoll(%d,%s): new status from NetXMS agent %d"), m_dwId, m_szName, newStatus);
      if (newStatus != STATUS_UNKNOWN)
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
      newStatus = pNode->getInterfaceStatusFromSNMP(pTransport, m_dwIfIndex);
		DbgPrintf(7, _T("Interface::StatusPoll(%d,%s): new status from SNMP %d"), m_dwId, m_szName, newStatus);
      if (newStatus != STATUS_UNKNOWN)
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
         newStatus = STATUS_UNKNOWN;
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
				newStatus = STATUS_UNKNOWN;	// Set interface status to unknown in case of any error
				Node *proxyNode = (Node *)g_idxNodeById.get(icmpProxy);
				if ((proxyNode != NULL) && proxyNode->isNativeAgent() && !proxyNode->isDown())
				{
					AgentConnection *conn = proxyNode->createAgentConnection();
					if (conn != NULL)
					{
						TCHAR parameter[64], buffer[64];

						_sntprintf(parameter, 64, _T("Icmp.Ping(%s)"), IpToStr(m_dwIpAddr, buffer));
						if (conn->getParameter(parameter, 64, buffer) == ERR_SUCCESS)
						{
							TCHAR *eptr;
							long value = _tcstol(buffer, &eptr, 10);
							if ((*eptr == 0) && (value >= 0))
							{
								newStatus = (value < 10000) ? STATUS_NORMAL : STATUS_CRITICAL;
							}
						}
						conn->disconnect();
						delete conn;
					}
				}
			}
			else	// not using ICMP proxy
			{
				SendPollerMsg(dwRqId, _T("      Starting ICMP ping\r\n"));
				DbgPrintf(7, _T("Interface::StatusPoll(%d,%s): calling IcmpPing(0x%08X,3,1500,NULL,%d)"), m_dwId, m_szName, htonl(m_dwIpAddr), g_dwPingSize);
				dwPingStatus = IcmpPing(htonl(m_dwIpAddr), 3, 1500, NULL, g_dwPingSize);
				if (dwPingStatus == ICMP_RAW_SOCK_FAILED)
					nxlog_write(MSG_RAW_SOCK_FAILED, EVENTLOG_WARNING_TYPE, NULL);
				newStatus = (dwPingStatus == ICMP_SUCCESS) ? STATUS_NORMAL : STATUS_CRITICAL;
				DbgPrintf(7, _T("Interface::StatusPoll(%d,%s): ping result %d, new status %d"), m_dwId, m_szName, dwPingStatus, newStatus);
			}
      }
   }

	if ((pNode->getFlags() & NF_IS_8021X) && isPhysicalPort())
	{
		DbgPrintf(5, _T("StatusPoll(%s): Checking 802.1x state for interface %s"), pNode->Name(), m_szName);
		paeStatusPoll(pSession, dwRqId, pTransport, pNode);
		if ((m_dot1xPaeAuthState == PAE_STATE_FORCE_UNAUTH) && (newStatus < STATUS_MAJOR))
			newStatus = STATUS_MAJOR;
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

   if ((newStatus != oldStatus) && (m_iPollCount >= requiredPolls))
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

		DbgPrintf(7, _T("Interface::StatusPoll(%d,%s): status changed from %d to %d"), m_dwId, m_szName, m_iStatus, newStatus);
		m_iStatus = newStatus;
		m_iPendingStatus = -1;	// Invalidate pending status
		SendPollerMsg(dwRqId, _T("      Interface status changed to %s\r\n"), g_szStatusText[m_iStatus]);
		PostEventEx(pEventQueue, 
						statusToEvent[m_iStatus],
						pNode->Id(), "dsaad", m_dwId, m_szName, m_dwIpAddr, m_dwIpNetMask,
						m_dwIfIndex);
		LockData();
		Modify();
		UnlockData();
   }
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
	pMsg->SetVariable(VID_DOT1X_PAE_STATE, m_dot1xPaeAuthState);
	pMsg->SetVariable(VID_DOT1X_BACKEND_STATE, m_dot1xBackendAuthState);
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
