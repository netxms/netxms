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
** File: vpnconn.cpp
**
**/

#include "nxcore.h"

/**
 * Default constructor for VPNConnector object
 */
VPNConnector::VPNConnector() : NetObj()
{
   m_dwPeerGateway = 0;
   m_dwNumLocalNets = 0;
   m_dwNumRemoteNets = 0;
   m_pLocalNetList = NULL;
   m_pRemoteNetList = NULL;
}

/**
 * Constructor for new VPNConnector object
 */
VPNConnector::VPNConnector(bool hidden) : NetObj()
{
   m_dwPeerGateway = 0;
   m_dwNumLocalNets = 0;
   m_dwNumRemoteNets = 0;
   m_pLocalNetList = NULL;
   m_pRemoteNetList = NULL;
   m_isHidden = hidden;
}

/**
 * VPNConnector class destructor
 */
VPNConnector::~VPNConnector()
{
   safe_free(m_pLocalNetList);
   safe_free(m_pRemoteNetList);
}

/**
 * Create object from database data
 */
BOOL VPNConnector::loadFromDatabase(UINT32 dwId)
{
   TCHAR szQuery[256];
   DB_RESULT hResult;
   UINT32 i, dwNodeId;
   NetObj *pObject;
   BOOL bResult = FALSE;

   m_id = dwId;

   if (!loadCommonProperties())
      return FALSE;

   // Load network lists
   _sntprintf(szQuery, 256, _T("SELECT ip_addr,ip_netmask FROM vpn_connector_networks WHERE vpn_id=%d AND network_type=0"), m_id);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult == NULL)
      return FALSE;     // Query failed
   m_dwNumLocalNets = DBGetNumRows(hResult);
   m_pLocalNetList = (IP_NETWORK *)malloc(sizeof(IP_NETWORK) * m_dwNumLocalNets);
   for(i = 0; i < m_dwNumLocalNets; i++)
   {
      m_pLocalNetList[i].dwAddr = DBGetFieldIPAddr(hResult, i, 0);
      m_pLocalNetList[i].dwMask = DBGetFieldIPAddr(hResult, i, 1);
   }
   DBFreeResult(hResult);

   _sntprintf(szQuery, 256, _T("SELECT ip_addr,ip_netmask FROM vpn_connector_networks WHERE vpn_id=%d AND network_type=1"), m_id);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult == NULL)
      return FALSE;     // Query failed
   m_dwNumRemoteNets = DBGetNumRows(hResult);
   m_pRemoteNetList = (IP_NETWORK *)malloc(sizeof(IP_NETWORK) * m_dwNumRemoteNets);
   for(i = 0; i < m_dwNumRemoteNets; i++)
   {
      m_pRemoteNetList[i].dwAddr = DBGetFieldIPAddr(hResult, i, 0);
      m_pRemoteNetList[i].dwMask = DBGetFieldIPAddr(hResult, i, 1);
   }
   DBFreeResult(hResult);
   
   // Load custom properties
   _sntprintf(szQuery, 256, _T("SELECT node_id,peer_gateway FROM vpn_connectors WHERE id=%d"), dwId);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult == NULL)
      return FALSE;     // Query failed

   if (DBGetNumRows(hResult) != 0)
   {
      dwNodeId = DBGetFieldULong(hResult, 0, 0);
      m_dwPeerGateway = DBGetFieldULong(hResult, 0, 1);

      // Link VPN connector to node
      if (!m_isDeleted)
      {
         pObject = FindObjectById(dwNodeId);
         if (pObject == NULL)
         {
            nxlog_write(MSG_INVALID_NODE_ID_EX, NXLOG_ERROR, "dds", dwId, dwNodeId, _T("VPN connector"));
         }
         else if (pObject->getObjectClass() != OBJECT_NODE)
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
   loadACLFromDB();

   return bResult;
}

/**
 * Save VPN connector object to database
 */
BOOL VPNConnector::saveToDatabase(DB_HANDLE hdb)
{
   TCHAR szQuery[1024], szIpAddr[16], szNetMask[16];
   BOOL bNewObject = TRUE;
   Node *pNode;
   UINT32 i, dwNodeId;
   DB_RESULT hResult;

   // Lock object's access
   lockProperties();

   saveCommonProperties(hdb);

   // Check for object's existence in database
   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("SELECT id FROM vpn_connectors WHERE id=%d"), m_id);
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
      dwNodeId = pNode->getId();
   else
      dwNodeId = 0;

   // Form and execute INSERT or UPDATE query
   if (bNewObject)
      _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("INSERT INTO vpn_connectors (id,node_id,peer_gateway) VALUES (%d,%d,%d)"),
              m_id, dwNodeId, m_dwPeerGateway);
   else
      _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("UPDATE vpn_connectors SET node_id=%d,peer_gateway=%d WHERE id=%d"),
              dwNodeId, m_dwPeerGateway, m_id);
   DBQuery(hdb, szQuery);

   // Save network list
   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM vpn_connector_networks WHERE vpn_id=%d"), m_id);
   DBQuery(hdb, szQuery);
   for(i = 0; i < m_dwNumLocalNets; i++)
   {
      _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), 
		           _T("INSERT INTO vpn_connector_networks (vpn_id,network_type,ip_addr,ip_netmask) VALUES (%d,0,'%s','%s')"),
              m_id, IpToStr(m_pLocalNetList[i].dwAddr, szIpAddr),
              IpToStr(m_pLocalNetList[i].dwMask, szNetMask));
      DBQuery(hdb, szQuery);
   }
   for(i = 0; i < m_dwNumRemoteNets; i++)
   {
      _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR),
			        _T("INSERT INTO vpn_connector_networks (vpn_id,network_type,ip_addr,ip_netmask) VALUES (%d,1,'%s','%s')"),
              m_id, IpToStr(m_pRemoteNetList[i].dwAddr, szIpAddr),
              IpToStr(m_pRemoteNetList[i].dwMask, szNetMask));
      DBQuery(hdb, szQuery);
   }

   // Save access list
   saveACLToDB(hdb);

   // Clear modifications flag and unlock object
   m_isModified = false;
   unlockProperties();

   return TRUE;
}

/**
 * Delete VPN connector object from database
 */
bool VPNConnector::deleteFromDatabase(DB_HANDLE hdb)
{
   bool success = NetObj::deleteFromDatabase(hdb);
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM vpn_connectors WHERE id=?"));
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM vpn_connector_networks WHERE vpn_id=?"));
   return success;
}

/**
 * Get connector's parent node
 */
Node *VPNConnector::getParentNode()
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
 * Create NXCP message with object's data
 */
void VPNConnector::fillMessage(CSCPMessage *pMsg)
{
   UINT32 i, dwId;

   NetObj::fillMessage(pMsg);
   pMsg->SetVariable(VID_PEER_GATEWAY, m_dwPeerGateway);
   pMsg->SetVariable(VID_NUM_LOCAL_NETS, m_dwNumLocalNets);
   pMsg->SetVariable(VID_NUM_REMOTE_NETS, m_dwNumRemoteNets);

   for(i = 0, dwId = VID_VPN_NETWORK_BASE; i < m_dwNumLocalNets; i++)
   {
      pMsg->SetVariable(dwId++, m_pLocalNetList[i].dwAddr);
      pMsg->SetVariable(dwId++, m_pLocalNetList[i].dwMask);
   }

   for(i = 0; i < m_dwNumRemoteNets; i++)
   {
      pMsg->SetVariable(dwId++, m_pRemoteNetList[i].dwAddr);
      pMsg->SetVariable(dwId++, m_pRemoteNetList[i].dwMask);
   }
}

/**
 * Modify object from message
 */
UINT32 VPNConnector::modifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked)
{
   if (!bAlreadyLocked)
      lockProperties();

   // Peer gateway
   if (pRequest->isFieldExist(VID_PEER_GATEWAY))
      m_dwPeerGateway = pRequest->GetVariableLong(VID_PEER_GATEWAY);

   // Network list
   if ((pRequest->isFieldExist(VID_NUM_LOCAL_NETS)) &&
       (pRequest->isFieldExist(VID_NUM_REMOTE_NETS)))
   {
      UINT32 i, dwId = VID_VPN_NETWORK_BASE;

      m_dwNumLocalNets = pRequest->GetVariableLong(VID_NUM_LOCAL_NETS);
      if (m_dwNumLocalNets > 0)
      {
         m_pLocalNetList = (IP_NETWORK *)realloc(m_pLocalNetList, sizeof(IP_NETWORK) * m_dwNumLocalNets);
         for(i = 0; i < m_dwNumLocalNets; i++)
         {
            m_pLocalNetList[i].dwAddr = pRequest->GetVariableLong(dwId++);
            m_pLocalNetList[i].dwMask = pRequest->GetVariableLong(dwId++);
         }
      }
      else
      {
         safe_free(m_pLocalNetList);
         m_pLocalNetList = NULL;
      }

      m_dwNumRemoteNets = pRequest->GetVariableLong(VID_NUM_REMOTE_NETS);
      if (m_dwNumRemoteNets > 0)
      {
         m_pRemoteNetList = (IP_NETWORK *)realloc(m_pRemoteNetList, sizeof(IP_NETWORK) * m_dwNumRemoteNets);
         for(i = 0; i < m_dwNumRemoteNets; i++)
         {
            m_pRemoteNetList[i].dwAddr = pRequest->GetVariableLong(dwId++);
            m_pRemoteNetList[i].dwMask = pRequest->GetVariableLong(dwId++);
         }
      }
      else
      {
         safe_free(m_pRemoteNetList);
         m_pRemoteNetList = NULL;
      }
   }

   return NetObj::modifyFromMessage(pRequest, TRUE);
}

/**
 * Check if given address falls into one of the local nets
 */
BOOL VPNConnector::isLocalAddr(UINT32 dwIpAddr)
{
   UINT32 i;
   BOOL bResult = FALSE;

   lockProperties();

   for(i = 0; i < m_dwNumLocalNets; i++)
      if ((dwIpAddr & m_pLocalNetList[i].dwMask) == m_pLocalNetList[i].dwAddr)
      {
         bResult = TRUE;
         break;
      }

   unlockProperties();
   return bResult;
}

/**
 * Check if given address falls into one of the remote nets
 */
BOOL VPNConnector::isRemoteAddr(UINT32 dwIpAddr)
{
   UINT32 i;
   BOOL bResult = FALSE;

   lockProperties();

   for(i = 0; i < m_dwNumRemoteNets; i++)
      if ((dwIpAddr & m_pRemoteNetList[i].dwMask) == m_pRemoteNetList[i].dwAddr)
      {
         bResult = TRUE;
         break;
      }

   unlockProperties();
   return bResult;
}

/**
 * Get address of peer gateway
 */
UINT32 VPNConnector::getPeerGatewayAddr()
{
   NetObj *pObject;
   UINT32 dwAddr = 0;

   pObject = FindObjectById(m_dwPeerGateway);
   if (pObject != NULL)
   {
      if (pObject->getObjectClass() == OBJECT_NODE)
         dwAddr = pObject->IpAddr();
   }
   return dwAddr;
}
