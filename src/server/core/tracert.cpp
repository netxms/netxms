/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2020 Victor Kirhenshtein
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
 * Add hop to path
 */
void NetworkPath::addHop(const InetAddress& nextHop, const InetAddress& route, const shared_ptr<NetObj>& currentObject, uint32_t ifIndex, bool isVpn, const TCHAR *name)
{
   NetworkPathElement *element = new NetworkPathElement();
   element->nextHop = nextHop;
   element->route = route;
   element->object = currentObject;
   element->ifIndex = ifIndex;
   element->isVpn = isVpn;
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
		msg->setField(fieldId++, e->isVpn);
		msg->setField(fieldId++, e->name);
	}
}

/**
 * Trace route between two nodes
 */
NetworkPath *TraceRoute(const shared_ptr<Node>& src, const shared_ptr<Node>& dest)
{
   UINT32 srcIfIndex;
   InetAddress srcAddr;
   if (!src->getOutwardInterface(dest->getIpAddress(), &srcAddr, &srcIfIndex))
      srcAddr = src->getIpAddress();

   NetworkPath *path = new NetworkPath(srcAddr);

   int hopCount = 0;
   shared_ptr<Node> curr(src), next;
   for(; (curr != dest) && (curr != nullptr) && (hopCount < 30); curr = next, hopCount++)
   {
      UINT32 dwIfIndex;
      InetAddress nextHop;
      InetAddress route;
      bool isVpn;
      TCHAR name[MAX_OBJECT_NAME];
      if (curr->getNextHop(srcAddr, dest->getIpAddress(), &nextHop, &route, &dwIfIndex, &isVpn, name))
      {
			next = FindNodeByIP(dest->getZoneUIN(), nextHop);
			path->addHop(nextHop, route, curr, dwIfIndex, isVpn, name);
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
      path->addHop(InetAddress::INVALID, InetAddress::INVALID, curr, 0, false, _T(""));
      path->setComplete();
   }

   return path;
}
