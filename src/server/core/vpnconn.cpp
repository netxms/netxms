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
** File: vpnconn.cpp
**
**/

#include "nxcore.h"

/**
 * Default constructor for VPNConnector object
 */
VPNConnector::VPNConnector() : super()
{
   m_dwPeerGateway = 0;
   m_localNetworks = new ObjectArray<InetAddress>(8, 8, Ownership::True);
   m_remoteNetworks = new ObjectArray<InetAddress>(8, 8, Ownership::True);
}

/**
 * Constructor for new VPNConnector object
 */
VPNConnector::VPNConnector(bool hidden) : super()
{
   m_dwPeerGateway = 0;
   m_localNetworks = new ObjectArray<InetAddress>(8, 8, Ownership::True);
   m_remoteNetworks = new ObjectArray<InetAddress>(8, 8, Ownership::True);
   m_isHidden = hidden;
   setCreationTime();
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
bool VPNConnector::loadFromDatabase(DB_HANDLE hdb, uint32_t id, DB_STATEMENT *preparedStatements)
{
   m_id = id;

   if (!loadCommonProperties(hdb, preparedStatements))
      return false;

   // Load network lists
   TCHAR szQuery[256];
   _sntprintf(szQuery, 256, _T("SELECT ip_addr,ip_netmask,network_type FROM vpn_connector_networks WHERE vpn_id=%d"), m_id);
   DB_RESULT hResult = DBSelect(hdb, szQuery);
   if (hResult == nullptr)
      return false;     // Query failed
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
   _sntprintf(szQuery, 256, _T("SELECT node_id,peer_gateway FROM vpn_connectors WHERE id=%d"), id);
   hResult = DBSelect(hdb, szQuery);
   if (hResult == nullptr)
      return false;     // Query failed

   bool success = false;
   if (DBGetNumRows(hResult) != 0)
   {
      uint32_t nodeId = DBGetFieldULong(hResult, 0, 0);
      m_dwPeerGateway = DBGetFieldULong(hResult, 0, 1);

      // Link VPN connector to node
      if (!m_isDeleted)
      {
         shared_ptr<NetObj> node = FindObjectById(nodeId, OBJECT_NODE);
         if (node != nullptr)
         {
            linkObjects(node, self());
            success = true;
         }
         else
         {
            nxlog_write(NXLOG_ERROR, _T("Inconsistent database: VPN connector %s [%u] linked to non-existent node [%u]"), m_name, m_id, nodeId);
         }
      }
      else
      {
         success = true;
      }
   }

   DBFreeResult(hResult);

   // Load access list
   loadACLFromDB(hdb, preparedStatements);

   return success;
}

/**
 * Save VPN connector object to database
 */
bool VPNConnector::saveToDatabase(DB_HANDLE hdb)
{
   bool success = super::saveToDatabase(hdb);
   if (success && (m_modified & MODIFY_OTHER))
   {
      // Determine owning node's ID
      shared_ptr<Node> pNode = getParentNode();
      UINT32 dwNodeId = (pNode != nullptr) ? pNode->getId() : 0;

      lockProperties();

      // Form and execute INSERT or UPDATE query
      TCHAR szQuery[1024];
      if (IsDatabaseRecordExist(hdb, _T("vpn_connectors"), _T("id"), m_id))
         _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("UPDATE vpn_connectors SET node_id=%d,peer_gateway=%d WHERE id=%d"),
                 dwNodeId, m_dwPeerGateway, m_id);
      else
         _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("INSERT INTO vpn_connectors (id,node_id,peer_gateway) VALUES (%d,%d,%d)"),
                 m_id, dwNodeId, m_dwPeerGateway);
      success = DBQuery(hdb, szQuery);

      // Save network list
      if (success)
         success = executeQueryOnObject(hdb, _T("DELETE FROM vpn_connector_networks WHERE vpn_id=?"));

      int i;
      TCHAR buffer[64];
      for(i = 0; success && (i < m_localNetworks->size()); i++)
      {
         InetAddress *addr = m_localNetworks->get(i);
         _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR),
                    _T("INSERT INTO vpn_connector_networks (vpn_id,network_type,ip_addr,ip_netmask) VALUES (%d,0,'%s',%d)"),
                    (int)m_id, addr->toString(buffer), addr->getMaskBits());
         success = DBQuery(hdb, szQuery);
      }
      for(i = 0; success && (i < m_remoteNetworks->size()); i++)
      {
         InetAddress *addr = m_remoteNetworks->get(i);
         _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR),
                    _T("INSERT INTO vpn_connector_networks (vpn_id,network_type,ip_addr,ip_netmask) VALUES (%d,1,'%s',%d)"),
                    (int)m_id, addr->toString(buffer), addr->getMaskBits());
         success = DBQuery(hdb, szQuery);
      }

      unlockProperties();
   }
   return success;
}

/**
 * Delete VPN connector object from database
 */
bool VPNConnector::deleteFromDatabase(DB_HANDLE hdb)
{
   bool success = super::deleteFromDatabase(hdb);
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM vpn_connectors WHERE id=?"));
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM vpn_connector_networks WHERE vpn_id=?"));
   return success;
}

/**
 * Get connector's parent node
 */
shared_ptr<Node> VPNConnector::getParentNode() const
{
   shared_ptr<Node> pNode;
   readLockParentList();
   for(int i = 0; i < getParentList().size(); i++)
   {
      NetObj *object = getParentList().get(i);
      if (object->getObjectClass() == OBJECT_NODE)
      {
         pNode = static_pointer_cast<Node>(getParentList().getShared(i));
         break;
      }
   }
   unlockParentList();
   return pNode;
}

/**
 * Create NXCP message with object's data
 */
void VPNConnector::fillMessageLocked(NXCPMessage *msg, uint32_t userId)
{
   super::fillMessageLocked(msg, userId);
   msg->setField(VID_PEER_GATEWAY, m_dwPeerGateway);
   msg->setField(VID_NUM_LOCAL_NETS, (UINT32)m_localNetworks->size());
   msg->setField(VID_NUM_REMOTE_NETS, (UINT32)m_remoteNetworks->size());

   uint32_t fieldId;
   int i;

   for(i = 0, fieldId = VID_VPN_NETWORK_BASE; i < m_localNetworks->size(); i++)
      msg->setField(fieldId++, *m_localNetworks->get(i));

   for(i = 0; i < m_remoteNetworks->size(); i++)
      msg->setField(fieldId++, *m_remoteNetworks->get(i));
}

/**
 * Modify object from message
 */
uint32_t VPNConnector::modifyFromMessageInternal(const NXCPMessage& msg, ClientSession *session)
{
   // Peer gateway
   if (msg.isFieldExist(VID_PEER_GATEWAY))
      m_dwPeerGateway = msg.getFieldAsUInt32(VID_PEER_GATEWAY);

   // Network list
   if ((msg.isFieldExist(VID_NUM_LOCAL_NETS)) &&
       (msg.isFieldExist(VID_NUM_REMOTE_NETS)))
   {
      int i;
      uint32_t fieldId = VID_VPN_NETWORK_BASE;

      m_localNetworks->clear();
      int count = msg.getFieldAsInt32(VID_NUM_LOCAL_NETS);
      for(i = 0; i < count; i++)
         m_localNetworks->add(new InetAddress(msg.getFieldAsInetAddress(fieldId++)));

      m_remoteNetworks->clear();
      count = msg.getFieldAsInt32(VID_NUM_REMOTE_NETS);
      for(i = 0; i < count; i++)
         m_remoteNetworks->add(new InetAddress(msg.getFieldAsInetAddress(fieldId++)));
   }

   return super::modifyFromMessageInternal(msg, session);
}

/**
 * Check if given address falls into one of the local nets
 */
bool VPNConnector::isLocalAddr(const InetAddress& addr) const
{
   bool result = false;

   lockProperties();

   for(int i = 0; i < m_localNetworks->size(); i++)
      if (m_localNetworks->get(i)->contains(addr))
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
bool VPNConnector::isRemoteAddr(const InetAddress& addr) const
{
   bool result = false;

   lockProperties();

   for(int i = 0; i < m_remoteNetworks->size(); i++)
      if (m_remoteNetworks->get(i)->contains(addr))
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
InetAddress VPNConnector::getPeerGatewayAddr() const
{
   shared_ptr<NetObj> node = FindObjectById(m_dwPeerGateway, OBJECT_NODE);
   return (node != nullptr) ? static_cast<Node&>(*node).getIpAddress() : InetAddress();
}

/**
 * Serialize object to JSON
 */
json_t *VPNConnector::toJson()
{
   json_t *root = super::toJson();

   lockProperties();

   json_object_set_new(root, "peerGateway", json_integer(m_dwPeerGateway));
   json_object_set_new(root, "localNetworks", json_object_array(m_localNetworks));
   json_object_set_new(root, "remoteNetworks", json_object_array(m_remoteNetworks));

   unlockProperties();
   return root;
}
