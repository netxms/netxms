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
** File: tracert.cpp
**/

#include "nxcore.h"

/**
 * Network path constructor
 */
NetworkPath::NetworkPath(const InetAddress& srcAddr) : m_path(32, 32, Ownership::True)
{
   m_sourceAddress = srcAddr;
	m_complete = false;
}

/**
 * Network path destructor
 */
NetworkPath::~NetworkPath()
{
}

/**
 * Add routing hop to path
 */
void NetworkPath::addHop(const shared_ptr<NetObj>& currentObject, const InetAddress& nextHop, const InetAddress& route, uint32_t ifIndex, const TCHAR *name)
{
   auto element = new NetworkPathElement();
   element->type = NetworkPathElementType::ROUTE;
   element->nextHop = nextHop;
   element->route = route;
   element->object = currentObject;
   element->ifIndex = ifIndex;
   _tcslcpy(element->name, name, MAX_OBJECT_NAME);
   m_path.add(element);
}

/**
 * Add VPM/proxy hop to path
 */
void NetworkPath::addHop(const shared_ptr<NetObj>& currentObject, NetworkPathElementType type, uint32_t nextHopId, const TCHAR *name)
{
   auto element = new NetworkPathElement();
   element->type = type;
   element->object = currentObject;
   element->ifIndex = nextHopId;
   _tcslcpy(element->name, name, MAX_OBJECT_NAME);
   m_path.add(element);
}

/**
 * Fill NXCP message with trace data
 */
void NetworkPath::fillMessage(NXCPMessage *msg) const
{
	msg->setField(VID_HOP_COUNT, static_cast<uint16_t>(m_path.size()));
	msg->setField(VID_IS_COMPLETE, m_complete);
	uint32_t fieldId = VID_NETWORK_PATH_BASE;
	for(int i = 0; i < m_path.size(); i++, fieldId += 5)
	{
	   const NetworkPathElement *e = m_path.get(i);
		msg->setField(fieldId++, e->object->getId());
		msg->setField(fieldId++, e->nextHop);
		msg->setField(fieldId++, e->ifIndex);
		msg->setField(fieldId++, static_cast<uint16_t>(e->type));
		msg->setField(fieldId++, e->name);
	}
}

/**
 * Convert network path to JSON object
 */
json_t *NetworkPath::toJson() const
{
   json_t *path = json_object();
   json_t *hops = json_array();
   for(int i = 0; i < m_path.size(); i++)
   {
      const NetworkPathElement *e = m_path.get(i);
      json_t *jsonHop = json_object();
      json_object_set_new(jsonHop, "objectId", json_integer(e->object->getId()));
      json_object_set_new(jsonHop, "objectName", json_string_t(e->object->getName()));
      switch(e->type)
      {
         case NetworkPathElementType::PROXY:
            json_object_set_new(jsonHop, "type", json_string("PROXY"));
            json_object_set_new(jsonHop, "proxyNodeId", json_integer(e->ifIndex));
            json_object_set_new(jsonHop, "proxyNodeName", json_string_t(GetObjectName(e->ifIndex, L"unknown")));
            break;
         case NetworkPathElementType::VPN:
            json_object_set_new(jsonHop, "type", json_string("VPN"));
            json_object_set_new(jsonHop, "vpnConnectorId", json_integer(e->ifIndex));
            json_object_set_new(jsonHop, "vpnConnectorName", json_string_t(GetObjectName(e->ifIndex, L"unknown")));
            break;
         case NetworkPathElementType::ROUTE:
            json_object_set_new(jsonHop, "type", json_string("ROUTE"));
            if (e->ifIndex != 0)
            {
               json_object_set_new(jsonHop, "ifIndex", json_integer(e->ifIndex));
            }
            json_object_set_new(jsonHop, "nextHop", e->nextHop.toJson());
            json_object_set_new(jsonHop, "route", e->route.toJson());
            break;
         case NetworkPathElementType::DUMMY:
            json_object_set_new(jsonHop, "type", json_string("DUMMY"));
            break;
      }
      json_object_set_new(jsonHop, "name", json_string_t(e->name));
      json_array_append_new(hops, jsonHop);
   }
   json_object_set_new(path, "hops", hops);
   json_object_set_new(path, "isComplete", json_boolean(m_complete));
   return path;
}

/**
 * Print network path to server console
 */
void NetworkPath::print(ServerConsole *console, int padding) const
{
   shared_ptr<Interface> iface;
   TCHAR nextHop[64];
   for(int i = 0; i < m_path.size(); i++)
   {
      NetworkPathElement *hop = m_path.get(i);
      switch(hop->type)
      {
         case NetworkPathElementType::DUMMY:
            console->printf(_T("%*s[%u] %s\n"),
                  padding, _T(""),
                  hop->object->getId(),
                  hop->object->getName());
            break;
         case NetworkPathElementType::PROXY:
            console->printf(_T("%*s[%u] %s ==> proxy link via %s [%u]\n"),
                  padding, _T(""),
                  hop->object->getId(),
                  hop->object->getName(),
                  GetObjectName(hop->ifIndex, _T("(unknown)")),
                  hop->ifIndex);
            break;
         case NetworkPathElementType::ROUTE:
            iface = static_cast<Node&>(*hop->object).findInterfaceByIndex(hop->ifIndex);
            console->printf(_T("%*s[%u] %s ==> %s via %s [%u]\n"),
                  padding, _T(""),
                  hop->object->getId(),
                  hop->object->getName(),
                  hop->nextHop.toString(nextHop),
                  (iface != nullptr) ? iface->getName() : _T("unknown"),
                  hop->ifIndex);
            break;
         case NetworkPathElementType::VPN:
            console->printf(_T("%*s[%u] %s ==> VPN via %u\n"),
                  padding, _T(""),
                  hop->object->getId(),
                  hop->object->getName(),
                  GetObjectName(hop->ifIndex, _T("(unknown)")),
                  hop->ifIndex);
            break;
      }
   }
}

/**
 * Trace route between two nodes
 */
shared_ptr<NetworkPath> NXCORE_EXPORTABLE TraceRoute(const shared_ptr<Node>& src, const shared_ptr<Node>& dest)
{
   uint32_t srcIfIndex;
   InetAddress srcAddr;
   if (!src->getOutwardInterface(dest->getIpAddress(), &srcAddr, &srcIfIndex))
      srcAddr = src->getIpAddress();

   auto path = make_shared<NetworkPath>(srcAddr);

   int hopCount = 0;
   shared_ptr<Node> curr(src), next;

   // Check if source node is NetXMS server itself - then check for proxy settings on destination
   if ((curr->getId() == g_dwMgmtNode) && IsZoningEnabled() && (dest->getZoneUIN() != 0))
   {
      shared_ptr<Zone> zone = FindZoneByUIN(dest->getZoneUIN());
      if ((zone != nullptr) && !zone->isProxyNode(dest->getId()))
      {
         uint32_t proxyId = zone->getAvailableProxyNodeId(dest.get());
         shared_ptr<Node> proxy = static_pointer_cast<Node>(FindObjectById(proxyId, OBJECT_NODE));
         if (proxy != nullptr)
         {
            path->addHop(curr, NetworkPathElementType::PROXY, proxy->getId(), proxy->getName());
            curr = proxy;
         }
      }
   }

   for(; (curr != dest) && (curr != nullptr) && (hopCount < 30); curr = next, hopCount++)
   {
      uint32_t ifIndex;
      InetAddress nextHop;
      InetAddress route;
      bool isVpn;
      TCHAR name[MAX_OBJECT_NAME];
      if (curr->getNextHop(srcAddr, dest->getIpAddress(), &nextHop, &route, &ifIndex, &isVpn, name))
      {
         next = FindNodeByIP(dest->getZoneUIN(), nextHop);
         if (isVpn)
            path->addHop(curr, NetworkPathElementType::VPN, ifIndex, name);
         else
            path->addHop(curr, nextHop, route, ifIndex, name);
         if ((next == curr) || !nextHop.isValid())
            next.reset();     // Directly connected subnet or too many hops, stop trace
      }
      else
      {
         next.reset();
      }
   }
   if (curr == dest)
   {
      path->addHop(curr, NetworkPathElementType::DUMMY, 0, _T(""));
      path->setComplete();
   }

   return path;
}

/**
 * Select best route to given destination from routing table
 */
const ROUTE *SelectBestRoute(const RoutingTable& routes, const InetAddress& destination)
{
   const ROUTE *bestRoute = nullptr;
   for(int i = 0; i < routes.size(); i++)
   {
      const ROUTE *route = routes.get(i);
      if (!route->destination.contains(destination))
         continue;

      if ((bestRoute == nullptr) ||
          (bestRoute->destination.getMaskBits() < route->destination.getMaskBits()) ||
          ((bestRoute->destination.getMaskBits() == route->destination.getMaskBits()) && (bestRoute->metric > route->metric)))
      {
         bestRoute = route;
         if (bestRoute->destination.getHostBits() == 0)
            break;   // Host route found
      }
   }
   return bestRoute;
}
