/*
** NetXMS - Network Management System
** Copyright (C) 2021-2024 Raden Solutions
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
** File: topology_builder.cpp
**
**/

#include "nxcore.h"
#include <netxms_maps.h>

/**
 * Peer information
 */
struct PeerInfo
{
   shared_ptr<Node> node;
   shared_ptr<Interface> ifLocal;
   shared_ptr<Interface> ifRemote;
   String linkName;
   bool vpnLink;

   PeerInfo(const shared_ptr<Node>& _node, const shared_ptr<Interface>& _ifLocal, const shared_ptr<Interface>& _ifRemote, const TCHAR *_linkName) : node(_node), ifLocal(_ifLocal), ifRemote(_ifRemote), linkName(_linkName)
   {
      vpnLink = false;
   }

   PeerInfo(const shared_ptr<Node>& _node, const TCHAR *_linkName) : node(_node), linkName(_linkName)
   {
      vpnLink = true;
   }
};

/**
 * Build IP topology internal version
 */
static void BuildIPTopology(NetworkMapObjectList *topology, const shared_ptr<Node>& seed, NetworkMap *filterProvider, int nDepth, bool includeEndNodes);

/**
 * Build IP topology for parent subnets
 */
static void ProcessParentSubnets(NetworkMapObjectList *topology, NetworkMap *filterProvider, const shared_ptr<Node>& seed, int nDepth, ObjectArray<PeerInfo>& peers, bool includeEndNodes)
{
   unique_ptr<SharedObjectArray<NetObj>> parents = seed->getParents();
   for(int i = 0; i < parents->size(); i++)
   {
      NetObj* parentNetObject = parents->get(i);
      if ((parentNetObject->getObjectClass() != OBJECT_SUBNET) || (parentNetObject->getChildCount() <= 1))
         continue;

      // Get interface connecting to subnet
      shared_ptr<Interface> iface = seed->findInterfaceBySubnet(static_cast<Subnet*>(parentNetObject)->getIpAddress());

      // Check if subnet actually connects two point-to-point interfaces
      unique_ptr<SharedObjectArray<NetObj>> children = parentNetObject->getChildren();
      if (children->size() == 2)
      {
         if (((iface != nullptr) && iface->isPointToPoint()) || static_cast<Subnet*>(parentNetObject)->isPointToPoint())
         {
            shared_ptr<Node> node;
            // Let's get other node in subnet
            for (int j = 0; j < children->size(); j++)
            {
               auto curr = children->getShared(j);
               if (curr->getId() != seed->getId() && curr->getObjectClass() == OBJECT_NODE)
               {
                  node = static_pointer_cast<Node>(curr);
                  break;
               }
            }
            if ((node != nullptr) && !topology->isObjectExist(node->getId()))
            {
               peers.add(new PeerInfo(node, iface, node->findInterfaceBySubnet(static_cast<Subnet*>(parentNetObject)->getIpAddress()), parentNetObject->getName()));
            }
            continue;
         }
      }

      if (!topology->isObjectExist(parentNetObject->getId()))
      {
         topology->addObject(parentNetObject->getId());
         for (int j = 0; j < children->size(); j++)
         {
            shared_ptr<NetObj> childNetObject = children->getShared(j);
            if (childNetObject->getId() != seed->getId() && childNetObject->getObjectClass() == OBJECT_NODE)
            {
               shared_ptr<Node> node = static_pointer_cast<Node>(childNetObject);
               if (includeEndNodes || node->isRouter())
               {
                  BuildIPTopology(topology, node, filterProvider, nDepth - 1, includeEndNodes);
                  if (nDepth == 1)
                  {
                     // Link object to subnet because BuildIPTopology will only add object itself at depth 0
                     shared_ptr<Interface> iface = node->findInterfaceBySubnet(static_cast<Subnet*>(parentNetObject)->getIpAddress());
                     topology->linkObjects(node->getId(), iface.get(), parentNetObject->getId(), nullptr);
                  }
               }
            }
         }
      }
      topology->linkObjects(seed->getId(), iface.get(), parentNetObject->getId(), nullptr);
   }
}

/**
 * Build IP topology internal version
 */
static void BuildIPTopology(NetworkMapObjectList *topology, const shared_ptr<Node>& seed, NetworkMap *filterProvider, int nDepth, bool includeEndNodes)
{
   if (topology->isObjectExist(seed->getId()))
      return;

   if ((filterProvider != nullptr) && !filterProvider->isAllowedOnMap(seed))
      return;

   topology->addObject(seed->getId());
   if (nDepth == 0)
      return;

   ObjectArray<PeerInfo> peers(0, 64, Ownership::True);
   ProcessParentSubnets(topology, filterProvider, seed, nDepth, peers, includeEndNodes);

   // Process point-to-point interfaces and VPN connectors
   unique_ptr<SharedObjectArray<NetObj>> children = seed->getChildren();
   for (int i = 0; i < children->size(); i++)
   {
      NetObj *object = children->get(i);
      if (object->getObjectClass() == OBJECT_INTERFACE)
      {
         Interface *iface = static_cast<Interface*>(object);
         if (!iface->isLoopback())
         {
            const InetAddressList *ipAddrList = iface->getIpAddressList();
            for(int j = 0; j < ipAddrList->size(); j++)
            {
               InetAddress a = ipAddrList->get(j);
               if (a.getHostBits() == 1)
               {
                  InetAddress peerAddr = a.isSubnetBroadcast() ? a.getSubnetAddress() : a.getSubnetBroadcast();
                  shared_ptr<Node> node = FindNodeByIP(seed->getZoneUIN(), peerAddr);
                  if (node != nullptr)
                  {
                     shared_ptr<Interface> peerIface = node->findInterfaceByIP(peerAddr);
                     if (peerIface != nullptr)
                        peers.add(new PeerInfo(node, iface->self(), peerIface, L""));
                  }
               }
            }
         }
      }
      else if (object->getObjectClass() == OBJECT_VPNCONNECTOR)
      {
         shared_ptr<Node> node = static_pointer_cast<Node>(FindObjectById(static_cast<VPNConnector*>(object)->getPeerGatewayId(), OBJECT_NODE));
         if (node != nullptr)
         {
            peers.add(new PeerInfo(node, object->getName()));
         }
      }
   }

   // Process all collected direct peer nodes
   for (int i = 0; i < peers.size(); i++)
   {
      PeerInfo *p = peers.get(i);
      BuildIPTopology(topology, p->node, filterProvider, nDepth - 1, includeEndNodes);
      topology->linkObjects(seed->getId(), p->ifLocal.get(), p->node->getId(), p->ifRemote.get(), p->linkName, p->vpnLink ? LINK_TYPE_VPN : LINK_TYPE_NORMAL);
   }
}

/**
 * Build IP topology
 */
unique_ptr<NetworkMapObjectList> BuildIPTopology(const shared_ptr<Node>& root, NetworkMap *filterProvider, int radius, bool includeEndNodes)
{
   int maxDepth = (radius <= 0) ? ConfigReadInt(_T("Topology.DefaultDiscoveryRadius"), 5) : radius;
   auto topology = make_unique<NetworkMapObjectList>();
   BuildIPTopology(topology.get(), root, filterProvider, maxDepth, includeEndNodes);
   return topology;
}
