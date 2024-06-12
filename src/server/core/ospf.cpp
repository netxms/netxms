/*
** NetXMS - Network Management System
** Copyright (C) 2003-2024 Raden Solutions
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
** File: ospf.cpp
**
**/

#include "nxcore.h"
#include <netxms_maps.h>

#define DEBUG_TAG _T("topology.ospf")

/**
 * Convert OSPF neighbor state to text
 */
const TCHAR *OSPFNeighborStateToText(OSPFNeighborState state)
{
   static const TCHAR *states[] = { _T("DOWN"), _T("ATTEMPT"), _T("INIT"), _T("2WAY"), _T("EXCHSTART"), _T("EXCHANGE"), _T("LOADING"), _T("FULL") };
   int index = static_cast<int>(state);
   return ((index >= 1) && (index <= 8)) ? states[index - 1] : _T("UNKNOWN");
}

/**
 * Convert OSPF interface state to text
 */
const TCHAR *OSPFInterfaceStateToText(OSPFInterfaceState state)
{
   static const TCHAR *states[] = { _T("DOWN"), _T("LOOPBACK"), _T("WAITING"), _T("PT-TO-PT"), _T("DR"), _T("BDR"), _T("ODR") };
   int index = static_cast<int>(state);
   return ((index >= 1) && (index <= 7)) ? states[index - 1] : _T("UNKNOWN");
}

/**
 * Convert OSPF interface state to text
 */
const TCHAR *OSPFInterfaceTypeToText(OSPFInterfaceType type)
{
   static const TCHAR *types[] = { _T("BROADCAST"), _T("NBMA"), _T("PT-TO-PT"), _T("PT-TO-MP") };
   int index = static_cast<int>(type);
   return ((index >= 1) && (index <= 4)) ? types[index - 1] : _T("UNKNOWN");
}

/**
 * Walk handler for area tables
 */
static uint32_t HandlerAreaTable(SNMP_Variable *var, SNMP_Transport *snmp, StructArray<OSPFArea> *areas)
{
   SNMP_ObjectId oid = var->getName();

   OSPFArea *area = areas->addPlaceholder();
   area->id = (oid.getElement(10) << 24) | (oid.getElement(11) << 16) | (oid.getElement(12) << 8) | oid.getElement(13);
   area->lsaCount = var->getValueAsUInt();
   area->areaBorderRouterCount = 0;
   area->asBorderRouterCount = 0;

   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());

   oid.changeElement(9, 5);   // ospfAreaBdrRtrCount
   request.bindVariable(new SNMP_Variable(oid));

   oid.changeElement(9, 6);   // ospfAsBdrRtrCount
   request.bindVariable(new SNMP_Variable(oid));

   SNMP_PDU *response;
   if (snmp->doRequest(&request, &response) == SNMP_ERR_SUCCESS)
   {
      if (response->getNumVariables() >= 2)
      {
         area->areaBorderRouterCount = response->getVariable(0)->getValueAsUInt();
         area->asBorderRouterCount = response->getVariable(1)->getValueAsUInt();
      }
      delete response;
   }

   return SNMP_ERR_SUCCESS;
}

/**
 * Process interface table entry
 */
static void ProcessInterfaceTableEntry(SNMP_Transport *snmp, SNMP_Variable *var, Node *node, StructArray<OSPFInterface> *interfaces)
{
   SNMP_ObjectId oid = var->getName();

   OSPFInterface *iface = interfaces->addPlaceholder();
   memset(iface, 0, sizeof(OSPFInterface));

   iface->ifIndex = oid.getLastElement();
   if (iface->ifIndex == 0)
   {
      uint32_t ipAddr = (oid.getElement(10) << 24) | (oid.getElement(11) << 16) | (oid.getElement(12) << 8) | oid.getElement(13);
      shared_ptr<Interface> ifaceObject = node->findInterfaceByIP(ipAddr);
      if (ifaceObject != nullptr)
         iface->ifIndex = ifaceObject->getIfIndex();
   }

   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());

   oid.changeElement(9, 3);   // ospfIfAreaId
   request.bindVariable(new SNMP_Variable(oid));

   oid.changeElement(9, 4);   // ospfIfType
   request.bindVariable(new SNMP_Variable(oid));

   oid.changeElement(9, 12);   // ospfIfState
   request.bindVariable(new SNMP_Variable(oid));

   oid.changeElement(9, 13);   // ospfIfDesignatedRouter
   request.bindVariable(new SNMP_Variable(oid));

   oid.changeElement(9, 14);   // ospfIfBackupDesignatedRouter
   request.bindVariable(new SNMP_Variable(oid));

   SNMP_PDU *response;
   if (snmp->doRequest(&request, &response) == SNMP_ERR_SUCCESS)
   {
      if (response->getNumVariables() >= 5)
      {
         iface->areaId = ntohl(response->getVariable(0)->getValueAsUInt());
         iface->type = static_cast<OSPFInterfaceType>(response->getVariable(1)->getValueAsUInt());
         iface->state = static_cast<OSPFInterfaceState>(response->getVariable(2)->getValueAsUInt());
         iface->dr = ntohl(response->getVariable(3)->getValueAsUInt());
         iface->bdr = ntohl(response->getVariable(4)->getValueAsUInt());
      }
      delete response;
   }
}

/**
 * Walk handler for neighbor tables
 */
static void ProcessNeighborTableEntry(SNMP_Transport *snmp, SNMP_Variable *var, Node *node, StructArray<OSPFNeighbor> *neighbors)
{
   SNMP_ObjectId oid = var->getName();

   OSPFNeighbor *neighbor = neighbors->addPlaceholder();
   memset(neighbor, 0, sizeof(OSPFNeighbor));
   neighbor->ipAddress = InetAddress((oid.getElement(10) << 24) | (oid.getElement(11) << 16) | (oid.getElement(12) << 8) | oid.getElement(13));
   neighbor->routerId = ntohl(var->getValueAsUInt());
   neighbor->ifIndex = oid.getLastElement();
   if (neighbor->ifIndex == 0)
   {
      shared_ptr<Interface> iface = node->findInterfaceInSameSubnet(neighbor->ipAddress);
      if (iface != nullptr)
      {
         neighbor->ifIndex = iface->getIfIndex();
         neighbor->ifObject = iface->getId();
      }
   }
   else
   {
      shared_ptr<Interface> iface = node->findInterfaceByIndex(neighbor->ifIndex);
      if (iface != nullptr)
         neighbor->ifObject = iface->getId();
   }

   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());

   oid.changeElement(9, 6);   // ospfNbrState
   request.bindVariable(new SNMP_Variable(oid));

   SNMP_PDU *response;
   if (snmp->doRequest(&request, &response) == SNMP_ERR_SUCCESS)
   {
      if (response->getNumVariables() >= 1)
         neighbor->state = static_cast<OSPFNeighborState>(response->getVariable(0)->getValueAsUInt());
      delete response;
   }

   shared_ptr<Node> routerNode = FindNodeByIP(node->getZoneUIN(), neighbor->ipAddress);
   if (routerNode != 0)
      neighbor->nodeId = routerNode->getId();
}

/**
 * Walk handler for virtual neighbor tables
 */
static void ProcessVirtualNeighborTableEntry(SNMP_Transport *snmp, SNMP_Variable *var, Node *node, StructArray<OSPFNeighbor> *neighbors)
{
   SNMP_ObjectId oid = var->getName();

   OSPFNeighbor *neighbor = neighbors->addPlaceholder();
   memset(neighbor, 0, sizeof(OSPFNeighbor));
   neighbor->ipAddress = InetAddress(ntohl(var->getValueAsUInt()));
   neighbor->areaId = (oid.getElement(10) << 24) | (oid.getElement(11) << 16) | (oid.getElement(12) << 8) | oid.getElement(13);
   neighbor->routerId = (oid.getElement(14) << 24) | (oid.getElement(15) << 16) | (oid.getElement(16) << 8) | oid.getElement(17);
   neighbor->isVirtual = true;

   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());

   oid.changeElement(9, 5);   // ospfVirtNbrState
   request.bindVariable(new SNMP_Variable(oid));

   SNMP_PDU *response;
   if (snmp->doRequest(&request, &response) == SNMP_ERR_SUCCESS)
   {
      if (response->getNumVariables() >= 1)
         neighbor->state = static_cast<OSPFNeighborState>(response->getVariable(0)->getValueAsUInt());
      delete response;
   }

   shared_ptr<Node> routerNode = FindNodeByIP(node->getZoneUIN(), neighbor->ipAddress);
   if (routerNode != 0)
      neighbor->nodeId = routerNode->getId();
}

/**
 * Collect OSPF information from node
 */
bool CollectOSPFInformation(Node *node, StructArray<OSPFArea> *areas, StructArray<OSPFInterface> *interfaces, StructArray<OSPFNeighbor> *neighbors)
{
   TCHAR debugPrefix[256];
   _sntprintf(debugPrefix, 256, _T("CollectOSPFInformation(%s [%u]):"), node->getName(), node->getId());

   SNMP_Transport *snmp = node->createSnmpTransport();
   if (snmp == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("%s cannot create SNMP transport"), debugPrefix);
      return false;
   }

   bool success = true;

   // Areas
   if (SnmpWalk(snmp, { 1, 3, 6, 1, 2, 1, 14, 2, 1, 7 }, HandlerAreaTable, areas) != SNMP_ERR_SUCCESS)   // Walk on ospfAreaLsaCount
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("%s cannot read OSPF area table"), debugPrefix);
      success = false;
   }

   // Interfaces
   if (success)
   {
      uint32_t rc = SnmpWalk(snmp, { 1, 3, 6, 1, 2, 1, 14, 7, 1, 5 },   // Walk on ospfIfAdminStat
         [snmp, node, interfaces] (SNMP_Variable *var) -> uint32_t
         {
            if (var->getValueAsUInt() == 1)  // Only process administratively enabled interfaces
               ProcessInterfaceTableEntry(snmp, var, node, interfaces);
            return SNMP_ERR_SUCCESS;
         });
      if (rc != SNMP_ERR_SUCCESS)
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("%s cannot read OSPF interface table"), debugPrefix);
         success = false;
      }
   }

   // Neighbors
   if (success)
   {
      uint32_t rc = SnmpWalk(snmp, { 1, 3, 6, 1, 2, 1, 14, 10, 1, 3 },   // Walk on ospfNbrRtrId
         [snmp, node, neighbors] (SNMP_Variable *var) -> uint32_t
         {
            ProcessNeighborTableEntry(snmp, var, node, neighbors);
            return SNMP_ERR_SUCCESS;
         });
      if (rc != SNMP_ERR_SUCCESS)
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("%s cannot read OSPF neighbor table"), debugPrefix);
         success = false;
      }
   }

   // Virtual neighbors
   if (success)
   {
      uint32_t rc = SnmpWalk(snmp, { 1, 3, 6, 1, 2, 1, 14, 11, 1, 3 },   // Walk on ospfVirtNbrIpAddr
         [snmp, node, neighbors] (SNMP_Variable *var) -> uint32_t
         {
            ProcessVirtualNeighborTableEntry(snmp, var, node, neighbors);
            return SNMP_ERR_SUCCESS;
         });
      if (rc != SNMP_ERR_SUCCESS)
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("%s cannot read OSPF virtual neighbor table"), debugPrefix);
         success = false;
      }
   }

   delete snmp;

   if (!success)
      return false;

   nxlog_debug_tag(DEBUG_TAG, 5, _T("%s OSPF topology collection complete (%d areas, %d interfaces, %d neighbors)"), debugPrefix, areas->size(), interfaces->size(), neighbors->size());
   return true;
}

/**
 * Build OSPF topology - actual implementation
 */
static void BuildOSPFTopology(NetworkMapObjectList *topology, const shared_ptr<Node>& seed, NetworkMap *filterProvider, int depth)
{
   if (topology->isObjectExist(seed->getId()))
      return;

   if (filterProvider != nullptr && !filterProvider->isAllowedOnMap(seed))
      return;

   topology->addObject(seed->getId());
   if (depth == 0)
      return;

   StructArray<OSPFNeighbor> neighbors = seed->getOSPFNeighbors();
   for (int i = 0; i < neighbors.size(); i++)
   {
      OSPFNeighbor *n = neighbors.get(i);
      if (n->nodeId == 0)
         continue;

      shared_ptr<Node> peer = static_pointer_cast<Node>(FindObjectById(n->nodeId, OBJECT_NODE));
      if (peer == nullptr)
         continue;

      if (n->isVirtual)
      {
         BuildOSPFTopology(topology, peer, filterProvider, depth - 1);

         TCHAR area[64];
         topology->linkObjects(seed->getId(), peer->getId(), LINK_TYPE_VPN, IpToStr(n->areaId, area), _T("vlink"), _T("vlink"));
         nxlog_debug_tag(DEBUG_TAG, 5, _T("BuildOSPFTopology(depth=%d): linking %s [%u] to %s [%u] (virtual link in area %s)"), depth, seed->getName(), seed->getId(), peer->getName(), peer->getId(), area);
      }
      else
      {
         shared_ptr<Interface> iface = seed->findInterfaceByIndex(n->ifIndex);
         if (iface == nullptr)
            continue;

         BuildOSPFTopology(topology, peer, filterProvider, depth - 1);

         TCHAR area[64];
         topology->linkObjects(seed->getId(), peer->getId(), LINK_TYPE_NORMAL, IpToStr(iface->getOSPFArea(), area), iface->getName(), nullptr);
         nxlog_debug_tag(DEBUG_TAG, 5, _T("BuildOSPFTopology(depth=%d): linking %s [%u] interface %s [%u] to %s [%u]"), depth, seed->getName(), seed->getId(), iface->getName(), iface->getId(), peer->getName(), peer->getId());
      }
   }
}

/**
 * Build OSPF topology
 */
unique_ptr<NetworkMapObjectList> BuildOSPFTopology(const shared_ptr<Node>& root, NetworkMap *filterProvider, int radius)
{
   int maxDepth = (radius <= 0) ? ConfigReadInt(_T("Topology.DefaultDiscoveryRadius"), 5) : radius;
   auto topology = make_unique<NetworkMapObjectList>();
   BuildOSPFTopology(topology.get(), root, filterProvider, maxDepth);
   return topology;
}
