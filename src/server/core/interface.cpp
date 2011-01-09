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


//
// Default constructor for Interface object
//

Interface::Interface()
          : NetObj()
{
   m_dwIpNetMask = 0;
   m_dwIfIndex = 0;
   m_dwIfType = IFTYPE_OTHER;
	m_bridgePortNumber = 0;
	m_slotNumber = 0;
	m_portNumber = 0;
   m_qwLastDownEventId = 0;
	m_bSyntheticMask = false;
	m_iPendingStatus = -1;
	m_iPollCount = 0;
	m_iRequiredPollCount = 0;	// Use system default
}


//
// Constructor for "fake" interface object
//

Interface::Interface(DWORD dwAddr, DWORD dwNetMask, bool bSyntheticMask)
          : NetObj()
{
   _tcscpy(m_szName, _T("unknown"));
   m_dwIpAddr = dwAddr;
   m_dwIpNetMask = dwNetMask;
   m_dwIfIndex = 1;
   m_dwIfType = IFTYPE_OTHER;
	m_bridgePortNumber = 0;
	m_slotNumber = 0;
	m_portNumber = 0;
   memset(m_bMacAddr, 0, MAC_ADDR_LENGTH);
   m_qwLastDownEventId = 0;
	m_bSyntheticMask = bSyntheticMask;
	m_iPendingStatus = -1;
	m_iPollCount = 0;
	m_iRequiredPollCount = 0;	// Use system default
   m_bIsHidden = TRUE;
}


//
// Constructor for normal interface object
//

Interface::Interface(const TCHAR *szName, DWORD dwIndex, DWORD dwAddr, DWORD dwNetMask, DWORD dwType)
          : NetObj()
{
   nx_strncpy(m_szName, szName, MAX_OBJECT_NAME);
   m_dwIfIndex = dwIndex;
   m_dwIfType = dwType;
   m_dwIpAddr = dwAddr;
   m_dwIpNetMask = dwNetMask;
	m_bridgePortNumber = 0;
	m_slotNumber = 0;
	m_portNumber = 0;
   memset(m_bMacAddr, 0, MAC_ADDR_LENGTH);
   m_qwLastDownEventId = 0;
	m_bSyntheticMask = false;
	m_iPendingStatus = -1;
	m_iPollCount = 0;
	m_iRequiredPollCount = 0;	// Use system default
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

   if (!LoadCommonProperties())
      return FALSE;

   _sntprintf(szQuery, 256, _T("SELECT ip_addr,ip_netmask,if_type,if_index,node_id,")
                            _T("mac_addr,synthetic_mask,required_polls FROM interfaces WHERE id=%d"), dwId);
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
		m_bSyntheticMask = DBGetFieldLong(hResult, 0, 6) ? true : false;
      m_iRequiredPollCount = DBGetFieldLong(hResult, 0, 7);

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
   LoadACLFromDB();

   return bResult;
}


//
// Save interface object to database
//

BOOL Interface::SaveToDB(DB_HANDLE hdb)
{
   TCHAR szQuery[1024], szMacStr[16], szIpAddr[16], szNetMask[16];
   BOOL bNewObject = TRUE;
   Node *pNode;
   DWORD dwNodeId;
   DB_RESULT hResult;

   // Lock object's access
   LockData();

   SaveCommonProperties(hdb);

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
      _sntprintf(szQuery, 1024, _T("INSERT INTO interfaces (id,ip_addr,")
                       _T("ip_netmask,node_id,if_type,if_index,mac_addr,synthetic_mask,required_polls) ")
                       _T("VALUES (%d,'%s','%s',%d,%d,%d,'%s',%d,%d)"),
              m_dwId, IpToStr(m_dwIpAddr, szIpAddr),
              IpToStr(m_dwIpNetMask, szNetMask), dwNodeId,
				  m_dwIfType, m_dwIfIndex, szMacStr, m_bSyntheticMask ? 1 : 0,
				  m_iRequiredPollCount);
   else
      _sntprintf(szQuery, 1024, _T("UPDATE interfaces SET ip_addr='%s',ip_netmask='%s',")
                       _T("node_id=%d,if_type=%d,if_index=%d,")
                       _T("mac_addr='%s',synthetic_mask=%d,")
							  _T("required_polls=%d WHERE id=%d"),
              IpToStr(m_dwIpAddr, szIpAddr),
              IpToStr(m_dwIpNetMask, szNetMask), dwNodeId,
				  m_dwIfType, m_dwIfIndex, szMacStr, m_bSyntheticMask ? 1 : 0,
				  m_iRequiredPollCount, m_dwId);
   DBQuery(hdb, szQuery);

   // Save access list
   SaveACLToDB(hdb);

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
   if (m_dwIfType != IFTYPE_NETXMS_NAT_ADAPTER)
   {
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
   }
   
   if (bNeedPoll)
   {
		// Pings cannot be used for cluster sync interfaces
      if ((pNode->getFlags() & NF_DISABLE_ICMP) || bClusterSync)
      {
         newStatus = STATUS_UNKNOWN;
      }
      else
      {
         // Use ICMP ping as a last option
         if ((m_dwIpAddr != 0) && 
              ((!(pNode->getFlags() & NF_BEHIND_NAT)) || 
               (m_dwIfType == IFTYPE_NETXMS_NAT_ADAPTER)))
         {
            SendPollerMsg(dwRqId, _T("      Starting ICMP ping\r\n"));
				DbgPrintf(7, _T("Interface::StatusPoll(%d,%s): calling IcmpPing(0x%08X,3,1500,NULL,%d)"), m_dwId, m_szName, htonl(m_dwIpAddr), g_dwPingSize);
            dwPingStatus = IcmpPing(htonl(m_dwIpAddr), 3, 1500, NULL, g_dwPingSize);
            if (dwPingStatus == ICMP_RAW_SOCK_FAILED)
               nxlog_write(MSG_RAW_SOCK_FAILED, EVENTLOG_WARNING_TYPE, NULL);
            newStatus = (dwPingStatus == ICMP_SUCCESS) ? STATUS_NORMAL : STATUS_CRITICAL;
				DbgPrintf(7, _T("Interface::StatusPoll(%d,%s): ping result %d, new status %d"), m_dwId, m_szName, dwPingStatus, newStatus);
         }
         else
         {
            // Interface doesn't have an IP address, so we can't ping it
				SendPollerMsg(dwRqId, POLLER_WARNING _T("      Interface status cannot be determined\r\n"));
				DbgPrintf(7, _T("Interface::StatusPoll(%d,%s): no IP address, will not ping"), m_dwId, m_szName);
            newStatus = STATUS_UNKNOWN;
         }
      }
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
			EVENT_INTERFACE_UP,       // Major
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
	pMsg->SetVariable(VID_SYNTHETIC_MASK, (WORD)(m_bSyntheticMask ? 1 : 0));
	pMsg->SetVariable(VID_REQUIRED_POLLS, (WORD)m_iRequiredPollCount);
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
