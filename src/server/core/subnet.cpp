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
** File: subnet.cpp
**/

#include "nxcore.h"

/**
 * Subnet class default constructor
 */
Subnet::Subnet() : super()
{
   m_zoneUIN = 0;
	m_bSyntheticMask = false;
}

/**
 * Subnet class constructor
 */
Subnet::Subnet(const InetAddress& addr, UINT32 zoneUIN, bool bSyntheticMask) : super()
{
   TCHAR szBuffer[64];
   _sntprintf(m_name, MAX_OBJECT_NAME, _T("%s/%d"), addr.toString(szBuffer), addr.getMaskBits());
   m_ipAddress = addr;
   m_zoneUIN = zoneUIN;
	m_bSyntheticMask = bSyntheticMask;
   setCreationTime();
}

/**
 * Subnet class destructor
 */
Subnet::~Subnet()
{
}

/**
 * Create object from database data
 */
bool Subnet::loadFromDatabase(DB_HANDLE hdb, UINT32 dwId)
{
   TCHAR szQuery[256];
   DB_RESULT hResult;

   m_id = dwId;

   if (!loadCommonProperties(hdb))
      return false;

   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("SELECT ip_addr,ip_netmask,zone_guid,synthetic_mask FROM subnets WHERE id=%d"), dwId);
   hResult = DBSelect(hdb, szQuery);
   if (hResult == 0)
      return false;     // Query failed

   if (DBGetNumRows(hResult) == 0)
   {
      DBFreeResult(hResult);
      return false;
   }

   m_ipAddress = DBGetFieldInetAddr(hResult, 0, 0);
   m_ipAddress.setMaskBits(DBGetFieldLong(hResult, 0, 1));
   m_zoneUIN = DBGetFieldULong(hResult, 0, 2);
	m_bSyntheticMask = DBGetFieldLong(hResult, 0, 3) ? true : false;

   DBFreeResult(hResult);

   // Load access list
   loadACLFromDB(hdb);

   return true;
}

/**
 * Save subnet object to database
 */
bool Subnet::saveToDatabase(DB_HANDLE hdb)
{
   TCHAR szQuery[1024], szIpAddr[64];

   // Lock object's access
   lockProperties();

   bool success = saveCommonProperties(hdb);

   if (success && (m_modified & MODIFY_OTHER))
   {
      // Form and execute INSERT or UPDATE query
      if (IsDatabaseRecordExist(hdb, _T("subnets"), _T("id"), m_id))
         _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR),
                    _T("UPDATE subnets SET ip_addr='%s',ip_netmask=%d,zone_guid=%d,synthetic_mask=%d WHERE id=%d"),
                    m_ipAddress.toString(szIpAddr), m_ipAddress.getMaskBits(), m_zoneUIN, m_bSyntheticMask ? 1 : 0, m_id);
      else
         _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR),
                    _T("INSERT INTO subnets (id,ip_addr,ip_netmask,zone_guid,synthetic_mask) VALUES (%d,'%s',%d,%d,%d)"),
                    m_id, m_ipAddress.toString(szIpAddr), m_ipAddress.getMaskBits(), m_zoneUIN, m_bSyntheticMask ? 1 : 0);
      success = DBQuery(hdb, szQuery);
   }

   // Update node to subnet mapping
   if (success && (m_modified & MODIFY_RELATIONS))
   {
      _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM nsmap WHERE subnet_id=%d"), m_id);
      DBQuery(hdb, szQuery);
      readLockChildList();
      for(int i = 0; success && (i < getChildList().size()); i++)
      {
         _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("INSERT INTO nsmap (subnet_id,node_id) VALUES (%d,%d)"), m_id, getChildList().get(i)->getId());
         success = DBQuery(hdb, szQuery);
      }
      unlockChildList();
   }

   // Save access list
   if (success)
      success = saveACLToDB(hdb);

   unlockProperties();

   return success;
}

/**
 * Delete subnet object from database
 */
bool Subnet::deleteFromDatabase(DB_HANDLE hdb)
{
   bool success = super::deleteFromDatabase(hdb);
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM subnets WHERE id=?"));
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM nsmap WHERE subnet_id=?"));
   return success;
}

/**
 * Create CSCP message with object's data
 */
void Subnet::fillMessageInternal(NXCPMessage *pMsg, UINT32 userId)
{
   super::fillMessageInternal(pMsg, userId);
   pMsg->setField(VID_IP_ADDRESS, m_ipAddress);
   pMsg->setField(VID_ZONE_UIN, m_zoneUIN);
	pMsg->setField(VID_SYNTHETIC_MASK, (WORD)(m_bSyntheticMask ? 1 : 0));
}

/**
 * Set correct netmask for subnet
 */
void Subnet::setCorrectMask(const InetAddress& addr)
{
	TCHAR szName[MAX_OBJECT_NAME], szBuffer[64];

	lockProperties();

	// Check if name is default
	_sntprintf(szName, MAX_OBJECT_NAME, _T("%s/%d"), m_ipAddress.toString(szBuffer), m_ipAddress.getMaskBits());
	if (!_tcsicmp(szName, m_name))
	{
		// Change name
      _sntprintf(m_name, MAX_OBJECT_NAME, _T("%s/%d"), addr.toString(szBuffer), addr.getMaskBits());
	}

	bool reAdd = !m_ipAddress.equals(addr);
   if (reAdd)
   {
      g_idxSubnetByAddr.remove(m_ipAddress);
   }

	m_ipAddress = addr;
	m_bSyntheticMask = false;

	if (reAdd)
   {
      g_idxSubnetByAddr.put(m_ipAddress, self());
   }
	setModified(MODIFY_OTHER);
	unlockProperties();
}

/**
 * Find MAC address for given IP address. May take a long time because it involves reading ARP caches
 * from nodes in this subnet.
 *
 * @param ipAddr IP address (host byte order)
 * @param macAddr buffer for found MAC address
 * @return true if MAC address found
 */
MacAddress Subnet::findMacAddress(const InetAddress& ipAddr)
{
   TCHAR buffer[64];
   nxlog_debug_tag(DEBUG_TAG_TOPO_ARP, 6, _T("Subnet[%s]::findMacAddress: searching for IP address %s"), m_name, ipAddr.toString(buffer));

   SharedObjectArray<Node> nodes(256, 256);
   readLockChildList();
   for(int i = 0; i < getChildList().size(); i++)
   {
      shared_ptr<NetObj> o = getChildList().getShared(i);
      if ((o->getObjectClass() == OBJECT_NODE) &&
          (o->getStatus() != STATUS_UNMANAGED) &&
          (static_cast<Node*>(o.get())->isNativeAgent() || static_cast<Node*>(o.get())->isSNMPSupported()))
      {
         nodes.add(static_pointer_cast<Node>(o));
      }
   }
   unlockChildList();

   bool success = false;
   MacAddress macAddr(MacAddress::ZERO);
   int i;
   for(i = 0; (i < nodes.size()) && !success; i++)
   {
		Node *node = nodes.get(i);
		nxlog_debug_tag(DEBUG_TAG_TOPO_ARP, 6, _T("Subnet[%s]::findMacAddress: reading ARP cache for node %s [%u]"), m_name, node->getName(), node->getId());
		ArpCache *arpCache = node->getArpCache();
		if (arpCache != NULL)
		{
         const ArpEntry *e = arpCache->findByIP(ipAddr);
         if (e != NULL)
         {
            nxlog_debug_tag(DEBUG_TAG_TOPO_ARP, 6, _T("Subnet[%s]::findMacAddress: found MAC address for IP address %s"), m_name, ipAddr.toString(buffer));
            macAddr = e->macAddr;
            success = true;
         }
         arpCache->decRefCount();
		}
		else
      {
         nxlog_debug_tag(DEBUG_TAG_TOPO_ARP, 7, _T("Subnet[%s]::findMacAddress: cannot read ARP cache for node %s [%u]"), m_name, node->getName(), node->getId());
      }
   }

	return macAddr;
}

/**
 * Build IP topology
 */
void Subnet::buildIPTopologyInternal(NetworkMapObjectList &topology, int nDepth, UINT32 seedNode, bool includeEndNodes)
{
	SharedObjectArray<Node> nodes;
	readLockChildList();
	for(int i = 0; i < getChildList().size(); i++)
	{
	   shared_ptr<NetObj> object = getChildList().getShared(i);
		if ((object->getId() == seedNode) || (object->getObjectClass() != OBJECT_NODE))
			continue;
		if (!includeEndNodes && !((Node *)object.get())->isRouter())
			continue;
		nodes.add(static_pointer_cast<Node>(object));
	}
	unlockChildList();

	for(int j = 0; j < nodes.size(); j++)
	{
		Node *n = nodes.get(j);
		n->buildIPTopologyInternal(topology, nDepth - 1, m_id, NULL, false, includeEndNodes);
	}
}

/**
 * Called by client session handler to check if threshold summary should be shown for this object.
 */
bool Subnet::showThresholdSummary() const
{
	return true;
}

/**
 * Build address map - mark used and free addresses
 */
UINT32 *Subnet::buildAddressMap(int *length)
{
   *length = 1 << (32 - m_ipAddress.getMaskBits());
   if ((*length < 2) || (*length > 65536))
      return NULL;
   UINT32 *map = MemAllocArrayNoInit<UINT32>(*length);

   if (m_ipAddress.getHostBits() > 1)
   {
      map[0] = 0xFFFFFFFF; // subnet
      map[*length - 1] = 0xFFFFFFFF;   // broadcast
      UINT32 addr = m_ipAddress.getAddressV4() + 1;
      for(int i = 1; i < *length - 1; i++, addr++)
      {
         shared_ptr<Node> node = FindNodeByIP(m_zoneUIN, addr);
         map[i] = (node != NULL) ? node->getId() : 0;
      }
   }
   else
   {
      UINT32 addr = m_ipAddress.getAddressV4();
      for(int i = 0; i < 2; i++)
      {
         shared_ptr<Node> node = FindNodeByIP(m_zoneUIN, addr++);
         map[i] = (node != NULL) ? node->getId() : 0;
      }
   }
   return map;
}

/**
 * Prepare node object for deletion
 */
void Subnet::prepareForDeletion()
{
   PostSystemEvent(EVENT_SUBNET_DELETED, g_dwMgmtNode, "isAd", m_id, m_name, &m_ipAddress, m_ipAddress.getMaskBits());
   super::prepareForDeletion();
}

/**
 * Get child node other than given one (useful for subnets representing point to point links)
 */
shared_ptr<Node> Subnet::getOtherNode(uint32_t nodeId)
{
   shared_ptr<Node> node;
   readLockChildList();
   for(int i = 0; i < getChildList().size(); i++)
   {
      auto curr = getChildList().getShared(i);
      if (curr->getId() == nodeId)
         continue;
      if (curr->getObjectClass() == OBJECT_NODE)
      {
         node = static_pointer_cast<Node>(curr);
         break;
      }
   }
   unlockChildList();
   return node;
}

/**
 * Check if this subnet is point-to-point
 */
bool Subnet::isPointToPoint() const
{
   lockProperties();
   bool result = (!m_ipAddress.isMulticast() &&
        (((m_ipAddress.getFamily() == AF_INET) && (m_ipAddress.getMaskBits() >= 30)) ||
         ((m_ipAddress.getFamily() == AF_INET6) && (m_ipAddress.getMaskBits() >= 126))));
   unlockProperties();
   return result;
}

/**
 * Serialize object to JSON
 */
json_t *Subnet::toJson()
{
   json_t *root = super::toJson();

   lockProperties();

   json_object_set_new(root, "ipAddress", m_ipAddress.toJson());
   json_object_set_new(root, "zoneUIN", json_integer(m_zoneUIN));
   json_object_set_new(root, "syntheticMask", json_boolean(m_bSyntheticMask));

   unlockProperties();
   return root;
}

/**
 * Create NXSL object for this object
 */
NXSL_Value *Subnet::createNXSLObject(NXSL_VM *vm) const
{
   return vm->createValue(new NXSL_Object(vm, &g_nxslSubnetClass, new shared_ptr<Subnet>(self())));
}
