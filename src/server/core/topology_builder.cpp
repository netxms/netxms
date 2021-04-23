/*
** NetXMS - Network Management System
** Copyright (C) 2021 Raden Solutions
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

/**
 * Peer information
 */
struct PeerInfo
{
   shared_ptr<Node> node;
   String linkName;
   bool vpnLink;

   PeerInfo(shared_ptr<Node> _node, const TCHAR *_linkName, bool _vpnLink) : node(_node), linkName(_linkName)
   {
      vpnLink = _vpnLink;
   }
};

/**
 * Build IP topology internal version
 */
static void BuildIPTopology(NetworkMapObjectList *topology, const shared_ptr<Node>& seed, int nDepth, bool includeEndNodes);

/**
 * Create link for two objects with link name, interface names and vpn flag
 */
static void LinkObjectsWithInterfaces(NetworkMapObjectList *topology, const shared_ptr<Node>& seed, const shared_ptr<Node>& node, int vpnLink, const TCHAR* linkName)
{
   shared_ptr<LinkLayerNeighbors> nbs = seed->getLinkLayerNeighbors();
   const TCHAR* interface1 = _T("Interface1");
   const TCHAR* interface2 = _T("Interface2");
   if (nbs != nullptr)
   {
      for (int i = 0; i < nbs->size(); i++)
      {
         LL_NEIGHBOR_INFO *info = nbs->getConnection(i);
         if (info != nullptr)
         {
            if (info->objectId == node->getId())
            {
               shared_ptr<Interface> ifLocal = seed->findInterfaceByIndex(info->ifLocal);
               shared_ptr<Interface> ifRemote = node->findInterfaceByIndex(info->ifRemote);
               interface1 = (ifLocal != nullptr) ? ifLocal->getName() : _T("N/A");
               interface2 = (ifRemote != nullptr) ? ifRemote->getName() : _T("N/A");
            }
         }
      }
   }
   topology->linkObjects(seed->getId(), node->getId(), vpnLink, linkName, interface1, interface2);
}

/**
 * Build IP topology for parent subnets
 */
static void ProcessParentSubnets(NetworkMapObjectList *topology, const shared_ptr<Node>& seed, int nDepth, ObjectArray<PeerInfo>& peers, bool includeEndNodes)
{
   SharedObjectArray<NetObj> *parents = seed->getParents();
   for(int i = 0; i < parents->size(); i++)
   {
      shared_ptr<NetObj> parentNetObject = parents->getShared(i);
      if (parentNetObject->getObjectClass() == OBJECT_SUBNET && parentNetObject->getChildCount() > 1)
      {
         // Check if subnet actually connects two point-to-point interfaces
         SharedObjectArray<NetObj> *children = parentNetObject->getChildren();
         if (children->size() == 2)
         {
            shared_ptr<Interface> iface = seed->findInterfaceBySubnet(static_pointer_cast<Subnet>(parentNetObject)->getIpAddress());
            if (((iface != nullptr) && iface->isPointToPoint()) || static_pointer_cast<Subnet>(parentNetObject)->isPointToPoint())
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
                  peers.add(new PeerInfo(node, parentNetObject->getName(), false));
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
                     BuildIPTopology(topology, node, nDepth - 1, includeEndNodes);
                     topology->linkObjects(parentNetObject->getId(), node->getId());
                  }
               }
            }
         }
         topology->linkObjects(seed->getId(), parentNetObject->getId());

         delete children;
      }
   }
   delete parents;
}

/**
 * Build IP topology for peers
 */
static void ProcessPeers(NetworkMapObjectList *topology, const shared_ptr<Node>& seed, int nDepth, ObjectArray<PeerInfo>& peers, bool includeEndNodes)
{
   SharedObjectArray<NetObj> *children = seed->getChildren();
   for (int i = 0; i < children->size(); i++)
   {
      NetObj *object = children->get(i);
      if (object->getObjectClass() == OBJECT_VPNCONNECTOR)
      {
         shared_ptr<Node> node = static_pointer_cast<Node>(FindObjectById(static_cast<VPNConnector*>(object)->getPeerGatewayId(), OBJECT_NODE));
         if ((node != nullptr) && !topology->isObjectExist(node->getId()))
         {
            peers.add(new PeerInfo(node, object->getName(), true));
         }
      }
   }
   delete children;

   for (int i = 0; i < peers.size(); i++)
   {
      auto p = peers.get(i);
      BuildIPTopology(topology, p->node, nDepth - 1, includeEndNodes);
      LinkObjectsWithInterfaces(topology, seed, p->node, p->vpnLink, p->linkName);
   }
}

/**
 * Build IP topology internal version
 */
static void BuildIPTopology(NetworkMapObjectList *topology, const shared_ptr<Node>& seed, int nDepth, bool includeEndNodes)
{
   if (topology->isObjectExist(seed->getId()))
      return;

   topology->addObject(seed->getId());
   if (nDepth > 0)
   {
      ObjectArray<PeerInfo> peers(0, 64, Ownership::True);
      ProcessParentSubnets(topology, seed, nDepth, peers, includeEndNodes);
      ProcessPeers(topology, seed, nDepth, peers, includeEndNodes);
   }
}

/**
 * Build IP topology
 */
NetworkMapObjectList* BuildIPTopology(const shared_ptr<Node>& root, UINT32 *pdwStatus, int radius, bool includeEndNodes)
{
   int maxDepth = (radius < 0) ? ConfigReadInt(_T("Topology.DefaultDiscoveryRadius"), 5) : radius;
   NetworkMapObjectList *topology = new NetworkMapObjectList();
   BuildIPTopology(topology, root, maxDepth, includeEndNodes);
   return topology;
}
