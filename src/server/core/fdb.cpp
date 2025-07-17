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
** File: fdb.cpp
**
**/

#include "nxcore.h"

/**
 * FDB entry comparator
 */
static int EntryComparator(const void *p1, const void *p2)
{
   return memcmp(static_cast<const ForwardingDatabaseEntry*>(p1)->macAddr.value(), static_cast<const ForwardingDatabaseEntry*>(p2)->macAddr.value(), MAC_ADDR_LENGTH);
}

/**
 * Get interface index for given bridge port number
 */
static uint32_t IfIndexFromPort(uint32_t port, const StructArray<BridgePort> *bridgePorts, const Node *node)
{
   if (bridgePorts != nullptr)
   {
      for(int i = 0; i < bridgePorts->size(); i++)
      {
         BridgePort *p = bridgePorts->get(i);
         if (p->portNumber == port)
            return p->ifIndex;
      }
   }

   if (node != nullptr)
   {
      shared_ptr<Interface> iface = node->findBridgePort(port);
      if (iface != nullptr)
         return iface->getIfIndex();
   }

   return 0;
}

/**
 * Constructor
 */
ForwardingDatabase::ForwardingDatabase(uint32_t nodeId, const StructArray<ForwardingDatabaseEntry>& entries, const StructArray<BridgePort> *bridgePorts) : m_fdb(0, std::min(64, entries.size()))
{
   m_nodeId = nodeId;
	m_timestamp = time(nullptr);

	// Copy entries and remove duplicates
	for(int i = 0; i < entries.size(); i++)
	{
	   bool found = false;
	   ForwardingDatabaseEntry *e = entries.get(i);
	   for(int j = 0; j < m_fdb.size(); j++)
	   {
	      if (m_fdb.get(j)->macAddr.equals(e->macAddr))
	      {
	         found = true;
	         break;
	      }
	   }
	   if (!found)
	   {
	      m_fdb.add(*e);
	   }
	}

	// Resolve bridge port numbers to interface indexes and MAC addresses to nodes
	shared_ptr<NetObj> node = FindObjectById(nodeId, OBJECT_NODE);
	for(int i = 0; i < m_fdb.size(); i++)
	{
	   ForwardingDatabaseEntry *e = m_fdb.get(i);
	   if ((e->ifIndex == 0) && (e->bridgePort != 0))
	      e->ifIndex = IfIndexFromPort(e->bridgePort, bridgePorts, static_cast<Node*>(node.get()));

	   shared_ptr<Node> node = FindNodeByMAC(e->macAddr);
	   if (node != nullptr)
	      e->nodeObject = node->getId();
	}

   m_fdb.sort(EntryComparator);
}

/**
 * Find MAC address
 * Returns interface index or 0 if MAC address not found
 */
uint32_t ForwardingDatabase::findMacAddress(const MacAddress& macAddr, bool *isStatic)
{
   ForwardingDatabaseEntry key;
	key.macAddr = macAddr;
	ForwardingDatabaseEntry *entry = static_cast<ForwardingDatabaseEntry*>(m_fdb.bsearch(&key, EntryComparator));
   if ((entry != nullptr) && (isStatic != nullptr))
      *isStatic = (entry->type == 5);
	return (entry != nullptr) ? entry->ifIndex : 0;
}

/**
 * Find all MAC addresses that contains given pattern
 */
void ForwardingDatabase::findMacAddressByPattern(const BYTE *macPattern, size_t macPatternSize, HashSet<MacAddress> *hs)
{
   for (int i = 0; i < m_fdb.size(); i++)
   {
      ForwardingDatabaseEntry *e = m_fdb.get(i);
      if (memmem(e->macAddr.value(), MAC_ADDR_LENGTH, macPattern, macPatternSize))
         hs->put(e->macAddr);
   }
}

/**
 * Check if port has only one MAC in FDB
 * If macAddr parameter is not nullptr, MAC address found on port
 * copied into provided buffer
 */
bool ForwardingDatabase::isSingleMacOnPort(uint32_t ifIndex, MacAddress *macAddr)
{
	int count = 0;
	for(int i = 0; i < m_fdb.size(); i++)
	{
	   ForwardingDatabaseEntry *e = m_fdb.get(i);
		if (e->ifIndex == ifIndex)
		{
			count++;
			if (count > 1)
				return false;
			if (macAddr != nullptr)
				*macAddr = e->macAddr;
		}
	}

	return count == 1;
}

/**
 * Get number of MAC addresses on given port
 */
int ForwardingDatabase::getMacCountOnPort(uint32_t ifIndex)
{
	int count = 0;
	for(int i = 0; i < m_fdb.size(); i++)
		if (m_fdb.get(i)->ifIndex == ifIndex)
		{
			count++;
		}

	return count;
}

/**
 * Print to console
 */
void ForwardingDatabase::print(ServerConsole *console, const Node& owner)
{
   TCHAR macAddrStr[64];

   console->printf(_T("\x1b[1mMAC address\x1b[0m       | \x1b[1mIfIndex\x1b[0m | \x1b[1mInterface\x1b[0m            | \x1b[1mPort\x1b[0m | \x1b[1mType\x1b[0m    | \x1b[1mNode\x1b[0m  | \x1b[1mNode name\x1b[0m\n"));
   console->printf(_T("------------------+---------+----------------------+------+-------+-----------------------------\n"));
	for(int i = 0; i < m_fdb.size(); i++)
   {
	   ForwardingDatabaseEntry *e = m_fdb.get(i);
      shared_ptr<NetObj> node = FindObjectById(e->nodeObject, OBJECT_NODE);
      shared_ptr<Interface> iface = owner.findInterfaceByIndex(e->ifIndex);
      console->printf(_T("%s | %7d | %-20s | %4d | %-7s | %5d | %s\n"), e->macAddr.toString(macAddrStr),
         e->ifIndex, (iface != nullptr) ? iface->getName() : _T("\x1b[31;1mUNKNOWN\x1b[0m"),
         e->bridgePort, (e->type == 3) ? _T("dynamic") : ((e->type == 5) ? _T("static") : ((e->type == 6) ? _T("secure") : _T("unknown"))),
         e->nodeObject, (node != nullptr) ? node->getName() : _T("\x1b[31;1mUNKNOWN\x1b[0m"));
   }
	console->printf(_T("\n%d entries\n\n"), m_fdb.size());
}

/**
 * Get interface name from index
 */
String ForwardingDatabase::interfaceIndexToName(const shared_ptr<NetObj>& node, uint32_t index)
{
   StringBuffer name;
   shared_ptr<Interface> iface = (node != nullptr) ? static_cast<Node*>(node.get())->findInterfaceByIndex(index) : shared_ptr<Interface>();
   if (iface != nullptr)
   {
     if (iface->getParentInterfaceId() != 0)
     {
        shared_ptr<Interface> parentIface = static_pointer_cast<Interface>(FindObjectById(iface->getParentInterfaceId(), OBJECT_INTERFACE));
        if ((parentIface != nullptr) &&
            ((parentIface->getIfType() == IFTYPE_ETHERNET_CSMACD) || (parentIface->getIfType() == IFTYPE_IEEE8023ADLAG)))
        {
           name = parentIface->getName();
        }
        else
        {
           name = iface->getName();
        }
     }
     else
     {
        name = iface->getName();
     }
   }
   else
   {
      name.appendFormattedString(_T("[%u]"), index);
   }
   return name;
}

/**
 * Fill NXCP message with FDB data
 */
void ForwardingDatabase::fillMessage(NXCPMessage *msg)
{
   shared_ptr<NetObj> node = FindObjectById(m_nodeId, OBJECT_NODE);
   msg->setField(VID_NUM_ELEMENTS, static_cast<uint32_t>(m_fdb.size()));
   uint32_t fieldId = VID_ELEMENT_LIST_BASE;
	for(int i = 0; i < m_fdb.size(); i++)
   {
	   ForwardingDatabaseEntry *e = m_fdb.get(i);
      msg->setField(fieldId++, e->macAddr);
      msg->setField(fieldId++, e->ifIndex);
      msg->setField(fieldId++, e->bridgePort);
      msg->setField(fieldId++, e->nodeObject);
      msg->setField(fieldId++, e->vlanId);
      msg->setField(fieldId++, e->type);
      msg->setField(fieldId++, interfaceIndexToName(node, e->ifIndex));
      fieldId += 3;
   }
}

/**
 * Get Table filled with switch forwarding database information
 */
shared_ptr<Table> ForwardingDatabase::getAsTable()
{
   shared_ptr<Table> result = make_shared<Table>();
   result->addColumn(_T("MAC_ADDRESS"), DCI_DT_STRING, _T("Mac address"), true);
   result->addColumn(_T("PORT"), DCI_DT_UINT, _T("Port"));
   result->addColumn(_T("IF_INDEX"), DCI_DT_UINT, _T("Interface index"));
   result->addColumn(_T("IF_NAME"), DCI_DT_STRING, _T("Interface name"));
   result->addColumn(_T("VLAN"), DCI_DT_UINT, _T("VLAN"));
   result->addColumn(_T("NODE_ID"), DCI_DT_UINT, _T("Node id"));
   result->addColumn(_T("NODE_NAME"), DCI_DT_STRING, _T("Node name"));
   result->addColumn(_T("ZONE_UIN"), DCI_DT_INT, _T("Zone UIN"));
   result->addColumn(_T("ZONE_NAME"), DCI_DT_STRING, _T("Zone name"));
   result->addColumn(_T("TYPE"), DCI_DT_STRING, _T("Type"));

   shared_ptr<NetObj> sourceNode = FindObjectById(m_nodeId, OBJECT_NODE);
   for(int i = 0; i < m_fdb.size(); i++)
   {
      ForwardingDatabaseEntry *e = m_fdb.get(i);

      result->addRow();

      TCHAR mac[64];
      result->set(0, e->macAddr.toString(mac));
      result->set(1, e->bridgePort);
      result->set(2, e->ifIndex);
      result->set(3, interfaceIndexToName(sourceNode, e->ifIndex));
      result->set(4, e->vlanId);
      result->set(5, e->nodeObject);
      shared_ptr<Node> node = static_pointer_cast<Node>(FindObjectById(e->nodeObject, OBJECT_NODE));
      if (node != nullptr)
      {
         result->set(6, node->getName());
         result->set(7, node->getZoneUIN());
         shared_ptr<Zone> zone = FindZoneByUIN(node->getZoneUIN());
         if (zone != nullptr)
         {
            result->set(8, zone->getName());
         }
         else
         {
            result->set(8, _T(""));
         }
      }
      else
      {
         result->set(6, _T(""));
         result->set(7, _T(""));
         result->set(8, _T(""));
      }
      result->set(9, (e->type) == 3 ? _T("Dynamic") : ((e->type == 5) ? _T("Static") : ((e->type == 6) ? _T("Secure") : _T("Unknown"))));
   }

   return result;
}
