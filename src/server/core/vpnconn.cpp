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
   m_localNetworks = new ObjectArray<InetAddress>(8, 8, true);
   m_remoteNetworks = new ObjectArray<InetAddress>(8, 8, true);
}

/**
 * Constructor for new VPNConnector object
 */
VPNConnector::VPNConnector(bool hidden) : NetObj()
{
   m_dwPeerGateway = 0;
   m_localNetworks = new ObjectArray<InetAddress>(8, 8, true);
   m_remoteNetworks = new ObjectArray<InetAddress>(8, 8, true);
   m_isHidden = hidden;
}

/**
 * VPNConnector class destructor
 */
VPNConnector::~VPNConnector()
{
   delete m_localNetworks;
   delete m_remoteNetworks;
}

/**
 * Create object from database data
 */
BOOL VPNConnector::loadFromDatabase(UINT32 dwId)
{
   m_id = dwId;

   if (!loadCommonProperties())
      return FALSE;

   // Load network lists
   TCHAR szQuery[256];
   _sntprintf(szQuery, 256, _T("SELECT ip_addr,ip_netmask,network_type FROM vpn_connector_networks WHERE vpn_id=%d"), m_id);
   DB_RESULT hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult == NULL)
      return FALSE;     // Query failed
   int count = DBGetNumRows(hResult);
   for(int i = 0; i < count; i++)
   {
      InetAddress addr = DBGetFieldInetAddr(hResult, i, 0);
      addr.setMaskBits(DBGetFieldLong(hResult, i, 1));
      if (DBGetFieldLong(hResult, i, 2) == 0)
         m_localNetworks->add(new InetAddress(addr));
      else
         m_remoteNetworks->add(new InetAddress(addr));
   }
   DBFreeResult(hResult);

   // Load custom properties
   _sntprintf(szQuery, 256, _T("SELECT node_id,peer_gateway FROM vpn_connectors WHERE id=%d"), dwId);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult == NULL)
      return FALSE;     // Query failed

   BOOL success = FALSE;
   if (DBGetNumRows(hResult) != 0)
   {
      UINT32 dwNodeId = DBGetFieldULong(hResult, 0, 0);
      m_dwPeerGateway = DBGetFieldULong(hResult, 0, 1);

      // Link VPN connector to node
      if (!m_isDeleted)
      {
         NetObj *pObject = FindObjectById(dwNodeId);
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
            success = TRUE;
         }
      }
      else
      {
         success = TRUE;
      }
   }

   DBFreeResult(hResult);

   // Load access list
   loadACLFromDB();

   return success;
}

/**
 * Save VPN connector object to database
 */
BOOL VPNConnector::saveToDatabase(DB_HANDLE hdb)
{
   // Lock object's access
   lockProperties();

   saveCommonProperties(hdb);

   // Determine owning node's ID
   Node *pNode = getParentNode();
   UINT32 dwNodeId = (pNode != NULL) ? pNode->getId() : 0;

   // Form and execute INSERT or UPDATE query
   TCHAR szQuery[1024];
   if (IsDatabaseRecordExist(hdb, _T("vpn_connectors"), _T("id"), m_id))
      _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("UPDATE vpn_connectors SET node_id=%d,peer_gateway=%d WHERE id=%d"),
              dwNodeId, m_dwPeerGateway, m_id);
   else
      _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("INSERT INTO vpn_connectors (id,node_id,peer_gateway) VALUES (%d,%d,%d)"),
              m_id, dwNodeId, m_dwPeerGateway);
   DBQuery(hdb, szQuery);

   // Save network list
   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM vpn_connector_networks WHERE vpn_id=%d"), m_id);
   DBQuery(hdb, szQuery);

   int i;
   TCHAR buffer[64];
   for(i = 0; i < m_localNetworks->size(); i++)
   {
      InetAddress *addr = m_localNetworks->get(i);
      _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR),
		           _T("INSERT INTO vpn_connector_networks (vpn_id,network_type,ip_addr,ip_netmask) VALUES (%d,0,'%s',%d)"),
                 (int)m_id, addr->toString(buffer), addr->getMaskBits());
      DBQuery(hdb, szQuery);
   }
   for(i = 0; i < m_remoteNetworks->size(); i++)
   {
      InetAddress *addr = m_remoteNetworks->get(i);
      _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR),
			        _T("INSERT INTO vpn_connector_networks (vpn_id,network_type,ip_addr,ip_netmask) VALUES (%d,1,'%s',%d)"),
                 (int)m_id, addr->toString(buffer), addr->getMaskBits());
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
void VPNConnector::fillMessageInternal(NXCPMessage *pMsg)
{
   NetObj::fillMessageInternal(pMsg);
   pMsg->setField(VID_PEER_GATEWAY, m_dwPeerGateway);
   pMsg->setField(VID_NUM_LOCAL_NETS, (UINT32)m_localNetworks->size());
   pMsg->setField(VID_NUM_REMOTE_NETS, (UINT32)m_remoteNetworks->size());

   UINT32 fieldId;
   int i;

   for(i = 0, fieldId = VID_VPN_NETWORK_BASE; i < m_localNetworks->size(); i++)
      pMsg->setField(fieldId++, *m_localNetworks->get(i));

   for(i = 0; i < m_remoteNetworks->size(); i++)
      pMsg->setField(fieldId++, *m_remoteNetworks->get(i));
}

/**
 * Modify object from message
 */
UINT32 VPNConnector::modifyFromMessageInternal(NXCPMessage *pRequest)
{
   // Peer gateway
   if (pRequest->isFieldExist(VID_PEER_GATEWAY))
      m_dwPeerGateway = pRequest->getFieldAsUInt32(VID_PEER_GATEWAY);

   // Network list
   if ((pRequest->isFieldExist(VID_NUM_LOCAL_NETS)) &&
       (pRequest->isFieldExist(VID_NUM_REMOTE_NETS)))
   {
      int i;
      UINT32 fieldId = VID_VPN_NETWORK_BASE;

      m_localNetworks->clear();
      int count = pRequest->getFieldAsInt32(VID_NUM_LOCAL_NETS);
      for(i = 0; i < count; i++)
         m_localNetworks->add(new InetAddress(pRequest->getFieldAsInetAddress(fieldId++)));

      count = pRequest->getFieldAsInt32(VID_NUM_REMOTE_NETS);
      for(i = 0; i < count; i++)
         m_remoteNetworks->add(new InetAddress(pRequest->getFieldAsInetAddress(fieldId++)));
   }

   return NetObj::modifyFromMessageInternal(pRequest);
}

/**
 * Check if given address falls into one of the local nets
 */
bool VPNConnector::isLocalAddr(const InetAddress& addr)
{
   bool result = false;

   lockProperties();

   for(int i = 0; i < m_localNetworks->size(); i++)
      if (m_localNetworks->get(i)->contain(addr))
      {
         result = true;
         break;
      }

   unlockProperties();
   return result;
}

/**
 * Check if given address falls into one of the remote nets
 */
bool VPNConnector::isRemoteAddr(const InetAddress& addr)
{
   bool result = false;

   lockProperties();

   for(int i = 0; i < m_remoteNetworks->size(); i++)
      if (m_remoteNetworks->get(i)->contain(addr))
      {
         result = true;
         break;
      }

   unlockProperties();
   return result;
}

/**
 * Get address of peer gateway
 */
InetAddress VPNConnector::getPeerGatewayAddr()
{
   NetObj *pObject;

   pObject = FindObjectById(m_dwPeerGateway);
   if (pObject != NULL)
   {
      if (pObject->getObjectClass() == OBJECT_NODE)
         return ((Node *)pObject)->getIpAddress();
   }
   return InetAddress();
}
