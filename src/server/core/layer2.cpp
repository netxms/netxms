/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2024 Victor Kirhenshtein
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
** File: layer2.cpp
**
**/

#include "nxcore.h"
#include <netxms_maps.h>

#define DEBUG_TAG _T("topo.layer2")

/**
 * Build layer 2 topology for switch
 */
void BuildL2Topology(NetworkMapObjectList &topology, Node *root, NetworkMap *filterProvider, int depth, bool includeEndNodes, bool useL1Topology)
{
	if (topology.isObjectExist(root->getId()))
		return;  // loop in object connections

   if ((filterProvider != nullptr) && !filterProvider->isAllowedOnMap(root->self()))
      return;

	topology.addObject(root->getId());

	if (depth <= 0)
	   return;

	shared_ptr<LinkLayerNeighbors> nbs = root->getLinkLayerNeighbors();
	if (nbs == nullptr)
		return;

	for(int i = 0; i < nbs->size(); i++)
	{
		LL_NEIGHBOR_INFO *info = nbs->getConnection(i);
      shared_ptr<NetObj> peer = FindObjectById(info->objectId);
      if (peer == nullptr)
         continue;

      if ((peer->getObjectClass() == OBJECT_NODE) && (static_cast<Node&>(*peer).isBridge() || includeEndNodes))
      {
         BuildL2Topology(topology, static_cast<Node*>(peer.get()), filterProvider, depth - 1, includeEndNodes, useL1Topology);
         shared_ptr<Interface> ifLocal = root->findInterfaceByIndex(info->ifLocal);
         shared_ptr<Interface> ifRemote = static_cast<Node&>(*peer).findInterfaceByIndex(info->ifRemote);
         nxlog_debug_tag(DEBUG_TAG, 5, _T("BuildL2Topology: root=%s [%u], node=%s [%u], ifLocal=[ifIndex=%u ifName=%s], ifRemote=[ifIndex=%u ifName=%s]"),
               root->getName(), root->getId(), peer->getName(), peer->getId(),
               info->ifLocal, (ifLocal != nullptr) ? ifLocal->getName() : _T("(null)"),
               info->ifRemote, (ifRemote != nullptr) ? ifRemote->getName() : _T("(null)"));
         topology.linkObjects(root->getId(), ifLocal.get(), peer->getId(), ifRemote.get());
      }
      else if (peer->getObjectClass() == OBJECT_ACCESSPOINT)
      {
         shared_ptr<Interface> ifLocal = root->findInterfaceByIndex(info->ifLocal);
         topology.addObject(peer->getId());
         topology.linkObjects(root->getId(), ifLocal.get(), peer->getId(), nullptr);
      }
	}

	if (!useL1Topology)
	   return;

	const ObjectArray<L1_NEIGHBOR_INFO>& l1Neighbors = GetL1Neighbors(root);
   for(int i = 0; i < l1Neighbors.size(); i++)
   {
      L1_NEIGHBOR_INFO *info = l1Neighbors.get(i);
      shared_ptr<Node> node = static_pointer_cast<Node>(FindObjectById(info->objectId, OBJECT_NODE));
      if ((node != nullptr) && (node->isBridge() || includeEndNodes))
      {
         BuildL2Topology(topology, node.get(), filterProvider, depth - 1, includeEndNodes, useL1Topology);
         shared_ptr<Interface> ifLocal = static_pointer_cast<Interface>(FindObjectById(info->ifLocal, OBJECT_INTERFACE));
         shared_ptr<Interface> ifRemote = static_pointer_cast<Interface>(FindObjectById(info->ifRemote, OBJECT_INTERFACE));
         uint32_t ifLocalIndex = ifLocal->getIfIndex();
         uint32_t ifRemoteIndex = ifRemote->getIfIndex();
         nxlog_debug_tag(DEBUG_TAG, 5, _T("BuildL1Topology: root=%s [%u], node=%s [%u], ifLocal=%u %s, ifRemote=%u %s"),
               root->getName(), root->getId(), node->getName(), node->getId(), ifLocalIndex,
               (ifLocal != nullptr) ? ifLocal->getName() : _T("(null)"), ifRemoteIndex,
               (ifRemote != nullptr) ? ifRemote->getName() : _T("(null)"));
         topology.linkObjects(root->getId(), ifLocal.get(), node->getId(), ifRemote.get(), info->routeInfo);
      }
   }
}

/**
 * Find connection point for interface
 */
shared_ptr<NetObj> NXCORE_EXPORTABLE FindInterfaceConnectionPoint(const MacAddress& macAddr, int *type)
{
   TCHAR macAddrText[64];
   nxlog_debug_tag(DEBUG_TAG, 6, _T("Called FindInterfaceConnectionPoint(%s)"), macAddr.toString(macAddrText));

   *type = CP_TYPE_INDIRECT;

   if (!macAddr.isValid() || (macAddr.length() != MAC_ADDR_LENGTH))
      return shared_ptr<NetObj>();

	shared_ptr<NetObj> cp;
	unique_ptr<SharedObjectArray<NetObj>> nodes = g_idxNodeById.getObjects();

	Node *bestMatchNode = nullptr;
	uint32_t bestMatchIfIndex = 0;
	int bestMatchCount = 0x7FFFFFFF;

	for(int i = 0; (i < nodes->size()) && (cp == nullptr); i++)
	{
		Node *node = static_cast<Node*>(nodes->get(i));

      shared_ptr<ForwardingDatabase> fdb = node->getSwitchForwardingDatabase();
		if (fdb != nullptr)
		{
			nxlog_debug_tag(DEBUG_TAG, 6, _T("FindInterfaceConnectionPoint(%s): FDB obtained for node %s [%u]"), macAddrText, node->getName(), (int)node->getId());
         bool isStatic;
         uint32_t ifIndex = fdb->findMacAddress(macAddr, &isStatic);
			if (ifIndex != 0)
			{
			   nxlog_debug_tag(DEBUG_TAG, 6, _T("FindInterfaceConnectionPoint(%s): MAC address found on interface %d (%s)"),
                      macAddrText, ifIndex, isStatic ? _T("static") : _T("dynamic"));
				int count = fdb->getMacCountOnPort(ifIndex);
				if (count == 1)
				{
               if (isStatic)
               {
                  // keep it as best match and continue search for dynamic connection
   					bestMatchCount = count;
					   bestMatchNode = node;
					   bestMatchIfIndex = ifIndex;
               }
               else
               {
					   shared_ptr<Interface> iface = node->findInterfaceByIndex(ifIndex);
					   if (iface != nullptr)
					   {
						   nxlog_debug_tag(DEBUG_TAG, 4, _T("FindInterfaceConnectionPoint(%s): found interface %s [%u] on node %s [%u]"), macAddrText,
									    iface->getName(), iface->getId(), iface->getParentNodeName().cstr(), iface->getParentNodeId());
                     cp = iface;
                     *type = CP_TYPE_DIRECT;
					   }
					   else
					   {
						   nxlog_debug_tag(DEBUG_TAG, 4, _T("FindInterfaceConnectionPoint(%s): cannot find interface object for node %s [%d] ifIndex %d"),
									    macAddrText, node->getName(), node->getId(), ifIndex);
					   }
               }
				}
            else if (count < bestMatchCount)
				{
					bestMatchCount = count;
					bestMatchNode = node;
					bestMatchIfIndex = ifIndex;
					nxlog_debug_tag(DEBUG_TAG, 4, _T("FindInterfaceConnectionPoint(%s): found potential interface [ifIndex=%d] on node %s [%d], count %d"),
					          macAddrText, ifIndex, node->getName(), (int)node->getId(), count);
				}
			}
		}

      if (node->isWirelessAccessPoint())
      {
			nxlog_debug_tag(DEBUG_TAG, 6, _T("FindInterfaceConnectionPoint(%s): node %s [%u] is a wireless access point, checking associated stations"),
			          macAddrText, node->getName(), node->getId());
         ObjectArray<WirelessStationInfo> *wsList = node->getWirelessStations();
         if (wsList != nullptr)
         {
			   nxlog_debug_tag(DEBUG_TAG, 6, _T("FindInterfaceConnectionPoint(%s): %d wireless stations registered on node %s [%u]"),
                      macAddrText, wsList->size(), node->getName(), node->getId());

            for(int i = 0; i < wsList->size(); i++)
            {
               WirelessStationInfo *ws = wsList->get(i);
               if (!memcmp(ws->macAddr, macAddr.value(), MAC_ADDR_LENGTH))
               {
                  shared_ptr<Interface> iface = node->findInterfaceByIndex(ws->rfIndex);
                  if (iface != nullptr)
                  {
                     nxlog_debug_tag(DEBUG_TAG, 4, _T("FindInterfaceConnectionPoint(%s): found matching wireless station on node %s [%u] interface %s"),
                        macAddrText, node->getName(), node->getId(), iface->getName());
                     cp = iface;
                     *type = CP_TYPE_WIRELESS;
                  }
                  else
                  {
                     nxlog_debug_tag(DEBUG_TAG, 4, _T("FindInterfaceConnectionPoint(%s): found matching wireless station on node %s [%u] but cannot determine interface"),
                        macAddrText, node->getName(), node->getId());
                  }
                  break;
               }
            }

            delete wsList;
         }
         else
         {
			   nxlog_debug_tag(DEBUG_TAG, 6, _T("FindInterfaceConnectionPoint(%s): unable to get wireless stations from node %s [%u]"),
			             macAddrText, node->getName(), node->getId());
         }
      }
	}

	if ((cp == nullptr) && (bestMatchNode != nullptr))
	{
		cp = bestMatchNode->findInterfaceByIndex(bestMatchIfIndex);
      if (bestMatchCount == 1)
      {
         // static best match
         *type = CP_TYPE_DIRECT;
      }
	}

	return cp;
}

/**
 * Collects information about provided MAC addresses owner and connection point
 */
static MacAddressInfo* CollectMacAddressInfo(const MacAddress& macAddr)
{
   int type = CP_TYPE_UNKNOWN;
   shared_ptr<NetObj> cp;
   shared_ptr<NetObj> object = MacDbFind(macAddr);

   if (object != nullptr)
   {
      if (object->getObjectClass() == OBJECT_INTERFACE)
      {
         Interface *localIf = static_cast<Interface*>(object.get());
         if (localIf->getPeerInterfaceId() != 0)
         {
            type = CP_TYPE_DIRECT;
            cp = FindObjectById(localIf->getPeerInterfaceId());
         }
      }
      else if (object->getObjectClass() == OBJECT_ACCESSPOINT)
      {
         AccessPoint *accessPoint = static_cast<AccessPoint*>(object.get());
         if (accessPoint->getPeerInterfaceId() != 0)
         {
            type = CP_TYPE_DIRECT;
            cp = FindObjectById(accessPoint->getPeerInterfaceId());
         }
      }
   }

   if (cp == nullptr)
      cp = FindInterfaceConnectionPoint(macAddr, &type);

   if (cp == nullptr && object == nullptr)
      return nullptr;

   return new MacAddressInfo(macAddr, object, cp, type);
}

/**
 * Find MAC addresses that contains given pattern but no more than searchLimit
 */
static void FindMACsByPattern(const BYTE* macPattern, size_t macPatternSize, HashSet<MacAddress>* matchedMacs, int searchLimit)
{
   unique_ptr<SharedObjectArray<NetObj>> accessPoints = g_idxAccessPointById.getObjects();

   for (int i = 0; i < accessPoints->size() && matchedMacs->size() < searchLimit; i++)
   {
      AccessPoint* accessPoint = static_cast<AccessPoint*>(accessPoints->get(i));
      if (memmem(accessPoint->getMacAddress().value(), MAC_ADDR_LENGTH, macPattern, macPatternSize))
      {
         matchedMacs->put(accessPoint->getMacAddress());
      }
   }

   unique_ptr<SharedObjectArray<NetObj>> nodes = g_idxNodeById.getObjects();

   for (int i = 0; i < nodes->size() && matchedMacs->size() < searchLimit; i++)
   {
      Node* node = static_cast<Node*>(nodes->get(i));
      // Check node interfaces
      unique_ptr<SharedObjectArray<NetObj>> ifList = node->getChildren(OBJECT_INTERFACE);
      for (int i = 0; i < ifList->size(); i++)
      {
         MacAddress m = static_cast<Interface*>(ifList->get(i))->getMacAddress();
         if (memmem(m.value(), MAC_ADDR_LENGTH, macPattern, macPatternSize))
         {
            matchedMacs->put(m);
         }
      }

      // Check node forwarding database
      shared_ptr<ForwardingDatabase> fdb = node->getSwitchForwardingDatabase();
      if (fdb != nullptr)
         fdb->findMacAddressByPattern(macPattern, macPatternSize, matchedMacs);

      // Check node wireless connections
      if (node->isWirelessAccessPoint())
      {
         ObjectArray<WirelessStationInfo>* wsList = node->getWirelessStations();
         if (wsList != nullptr)
         {
            for (int i = 0; i < wsList->size(); i++)
            {
               WirelessStationInfo *ws = wsList->get(i);
               if (memmem(ws->macAddr, MAC_ADDR_LENGTH, macPattern, macPatternSize))
                  matchedMacs->put(MacAddress(ws->macAddr, MAC_ADDR_LENGTH));
            }
            delete wsList;
         }
      }
   }
}

/**
 * Find all interface connection points whose MAC address contains given pattern
 */
void NXCORE_EXPORTABLE FindMacAddresses(const BYTE* macPattern, size_t macPatternSize, ObjectArray<MacAddressInfo>* out, int searchLimit)
{
   if (macPatternSize >= MAC_ADDR_LENGTH)
   {
      MacAddress mac(macPattern, macPatternSize);
      MacAddressInfo* macInfo = CollectMacAddressInfo(mac);
      if (macInfo != nullptr)
         out->add(macInfo);
   }
   else
   {
      HashSet<MacAddress> matchedMacs;
      FindMACsByPattern(macPattern, macPatternSize, &matchedMacs, searchLimit);
      for (const MacAddress* mac : matchedMacs)
      {
         MacAddressInfo* macInfo = CollectMacAddressInfo(*mac);
         if (macInfo != nullptr)
            out->add(macInfo);
         else
            nxlog_debug_tag(DEBUG_TAG, 6, _T("FindMacAddresses(%s): found, but failed to get info"), mac->toString().cstr());
      }
   }
}

/**
 * Fill NXCP message with MAC address search results
 */
void MacAddressInfo::fillMessage(NXCPMessage* msg, uint32_t base) const
{
   // Always present
   msg->setField(base, m_macAddr);
   msg->setField(base + 1, static_cast<uint16_t>(m_type));

   // Where MAC is connected
   if (m_connectionPoint != nullptr)
   {
      const shared_ptr<NetObj>& cp = m_connectionPoint;
      shared_ptr<Node> node;
      if (cp->getObjectClass() == OBJECT_INTERFACE)
         node = static_cast<Interface&>(*cp).getParentNode();
      if (node != nullptr)
         msg->setField(base + 2, node->getId());
      else
         msg->setField(base + 2, 0);

      msg->setField(base + 3, cp->getId());
      msg->setField(base + 4, static_cast<Interface&>(*cp).getIfIndex());
   }
   else
   {
      msg->setField(base + 2, 0);
      msg->setField(base + 3, 0);
      msg->setField(base + 4, (uint32_t)0);
   }

   // MAC owner
   if (m_owner != nullptr)
   {
      if (m_owner->getObjectClass() == OBJECT_INTERFACE)
      {
         Interface *iface = static_cast<Interface*>(m_owner.get());
         msg->setField(base + 5, iface->getParentNodeId());
         msg->setField(base + 6, iface->getId());
         msg->setField(base + 7, iface->getIpAddressList()->getFirstUnicastAddress());
      }
      else if (m_owner->getObjectClass() == OBJECT_ACCESSPOINT)
      {
         AccessPoint *accessPoint = static_cast<AccessPoint*>(m_owner.get());
         msg->setField(base + 5, accessPoint->getId());
         msg->setField(base + 6, 0);
         msg->setField(base + 7, accessPoint->getPrimaryIpAddress());
      }
   }
   else
   {
      msg->setField(base + 5, 0);
      msg->setField(base + 6, 0);
      msg->setField(base + 7, InetAddress::INVALID);
   }
}

/**
 * Fill JSON object with MAC address search results
 */
void MacAddressInfo::fillJson(json_t *json, uint32_t userId, bool includeObject, std::function<json_t* (const NetObj& object)> createObjectSummary) const
{
   // Always present
   json_object_set_new(json, "localMacAddress", json_string_t(m_macAddr.toString(MacAddressNotation::COLON_SEPARATED)));
   switch(m_type)
   {
      case 0:
         json_object_set_new(json, "type", json_string("INDIRECT"));
         break;
      case 1:
         json_object_set_new(json, "type", json_string("DIRECT"));
         break;
      case 2:
         json_object_set_new(json, "type", json_string("WIRELESS"));
         break;
      case 4:
         json_object_set_new(json, "type", json_string("NOT_FOUND"));
         break;
      case 3:
      default:
         json_object_set_new(json, "type", json_string("UNKNOWN"));
         break;
   }

   // Where MAC is connected
   if (m_connectionPoint != nullptr)
   {
      const shared_ptr<NetObj>& cp = m_connectionPoint;
      shared_ptr<Node> node;
      if (cp->getObjectClass() == OBJECT_INTERFACE)
         node = static_cast<Interface&>(*cp).getParentNode();
      if (node != nullptr)
      {
         json_object_set_new(json, "nodeId", json_integer(node->getId()));
         if (includeObject && node->checkAccessRights(userId, OBJECT_ACCESS_READ))
            json_object_set_new(json, "node", createObjectSummary(*node));
      }
      else
      {
         json_object_set_new(json, "nodeId", json_integer(0));
      }

      json_object_set_new(json, "interfaceId", json_integer(cp->getId()));
      if (includeObject && node->checkAccessRights(userId, OBJECT_ACCESS_READ))
         json_object_set_new(json, "interface", createObjectSummary(*cp));
      json_object_set_new(json, "interfaceIndex", json_integer(static_cast<Interface&>(*cp).getIfIndex()));
   }
   else
   {
      json_object_set_new(json, "nodeId", json_integer(0));
      json_object_set_new(json, "interfaceId", json_integer(0));
      json_object_set_new(json, "interfaceIndex", json_integer(0));
   }

   // MAC owner
   if (m_owner != nullptr)
   {
      if (m_owner->getObjectClass() == OBJECT_INTERFACE)
      {
         Interface *iface = static_cast<Interface*>(m_owner.get());
         json_object_set_new(json, "localNodeId", json_integer(iface->getParentNodeId()));
         json_object_set_new(json, "localInterfaceId", json_integer(iface->getId()));
         json_object_set_new(json, "localIpAddress", iface->getIpAddressList()->getFirstUnicastAddress().toJson());
         if (includeObject && iface->checkAccessRights(userId, OBJECT_ACCESS_READ))
            json_object_set_new(json, "localInterface", createObjectSummary(*iface));
         shared_ptr<NetObj> node = FindObjectById(iface->getParentNodeId());
         if (includeObject && node->checkAccessRights(userId, OBJECT_ACCESS_READ))
            json_object_set_new(json, "localNode", createObjectSummary(*node));
      }
      else if (m_owner->getObjectClass() == OBJECT_ACCESSPOINT)
      {
         AccessPoint *accessPoint = static_cast<AccessPoint*>(m_owner.get());
         json_object_set_new(json, "localNodeId", json_integer(accessPoint->getId()));
         json_object_set_new(json, "localInterfaceId", json_integer(0));
         json_object_set_new(json, "localIpAddress", accessPoint->getPrimaryIpAddress().toJson());
         if (includeObject && accessPoint->checkAccessRights(userId, OBJECT_ACCESS_READ))
            json_object_set_new(json, "localNode", createObjectSummary(*accessPoint));
      }
   }
   else
   {
      json_object_set_new(json, "localNodeId", json_integer(0));
      json_object_set_new(json, "localInterfaceId", json_integer(0));
      json_object_set_new(json, "localIpAddress", json_null());
   }
}
