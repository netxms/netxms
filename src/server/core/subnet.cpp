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
** File: subnet.cpp
**/

#include "nxcore.h"

/**
 * Subnet class default constructor
 */
Subnet::Subnet() : super()
{
   m_zoneUIN = 0;
}

/**
 * Subnet class constructor for manual subnet creation
 */
Subnet::Subnet(const TCHAR *name, const InetAddress& addr, int32_t zoneUIN) : super()
{
   if (*name == 0)
   {
      TCHAR szBuffer[64];
      _sntprintf(m_name, MAX_OBJECT_NAME, _T("%s/%d"), addr.toString(szBuffer), addr.getMaskBits());
   }
   else
   {
      _tcslcpy(m_name, name, MAX_OBJECT_NAME);
   }

   m_ipAddress = addr;
   m_zoneUIN = zoneUIN;
   m_flags |= SF_MANUALLY_CREATED;
   setCreationTime();
}

/**
 * Subnet class constructor
 */
Subnet::Subnet(const InetAddress& addr, int32_t zoneUIN, bool syntheticMask) : super()
{
   TCHAR szBuffer[64];
   _sntprintf(m_name, MAX_OBJECT_NAME, _T("%s/%d"), addr.toString(szBuffer), addr.getMaskBits());
   m_ipAddress = addr;
   m_zoneUIN = zoneUIN;
   if (syntheticMask)
      m_flags |= SF_SYNTETIC_MASK;
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
bool Subnet::loadFromDatabase(DB_HANDLE hdb, uint32_t id, DB_STATEMENT *preparedStatements)
{
   m_id = id;

   if (!loadCommonProperties(hdb, preparedStatements))
      return false;

   TCHAR szQuery[256];
   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("SELECT ip_addr,ip_netmask,zone_guid FROM subnets WHERE id=%u"), id);
   DB_RESULT hResult = DBSelect(hdb, szQuery);
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

   DBFreeResult(hResult);

   // Load access list
   loadACLFromDB(hdb, preparedStatements);

   return true;
}

/**
 * Save subnet object to database
 */
bool Subnet::saveToDatabase(DB_HANDLE hdb)
{
   bool success = super::saveToDatabase(hdb);

   if (success && (m_modified & MODIFY_OTHER))
   {
      static const TCHAR *columns[] = { _T("ip_addr"), _T("ip_netmask"), _T("zone_guid"), nullptr };
      DB_STATEMENT hStmt = DBPrepareMerge(hdb, _T("subnets"), _T("id"), m_id, columns);
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, m_ipAddress);
         DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_ipAddress.getMaskBits());
         DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_zoneUIN);
         DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, m_id);
         success = DBExecute(hStmt);
         DBFreeStatement(hStmt);
      }
      else
      {
         success = false;
      }
   }

   // Update node to subnet mapping
   if (success && (m_modified & MODIFY_RELATIONS))
   {
      success = executeQueryOnObject(hdb, _T("DELETE FROM nsmap WHERE subnet_id=?"));
      readLockChildList();
      if (success && !getChildList().isEmpty())
      {
         DB_STATEMENT hStmt = DBPrepare(hdb,  _T("INSERT INTO nsmap (subnet_id,node_id) VALUES (?,?)"), getChildList().size() > 1);
         if (hStmt != nullptr)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
            for(int i = 0; success && (i < getChildList().size()); i++)
            {
               DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, getChildList().get(i)->getId());
               success = DBExecute(hStmt);
            }
            DBFreeStatement(hStmt);
         }
         else
         {
            success = false;
         }
      }
      unlockChildList();
   }

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
void Subnet::fillMessageLocked(NXCPMessage *msg, uint32_t userId)
{
   super::fillMessageLocked(msg, userId);
   msg->setField(VID_IP_ADDRESS, m_ipAddress);
   msg->setField(VID_ZONE_UIN, m_zoneUIN);
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
	m_flags &= ~SF_SYNTETIC_MASK;

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
   for(int i = 0; (i < nodes.size()) && !success; i++)
   {
		Node *node = nodes.get(i);
		nxlog_debug_tag(DEBUG_TAG_TOPO_ARP, 6, _T("Subnet[%s]::findMacAddress: checking ARP cache on node %s [%u]"), m_name, node->getName(), node->getId());
		MacAddress m = node->findMacAddressInArpCache(ipAddr);
		if (m.isValid())
		{
		   macAddr = m;
         nxlog_debug_tag(DEBUG_TAG_TOPO_ARP, 6, _T("Subnet[%s]::findMacAddress: found MAC address %s for IP address %s"), m_name, macAddr.toString().cstr(), ipAddr.toString(buffer));
         success = true;
		}
   }

	return macAddr;
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
      return nullptr;
   UINT32 *map = MemAllocArrayNoInit<UINT32>(*length);

   if (m_ipAddress.getHostBits() > 1)
   {
      map[0] = 0xFFFFFFFF; // subnet
      map[*length - 1] = 0xFFFFFFFF;   // broadcast
      uint32_t addr = m_ipAddress.getAddressV4() + 1;
      for(int i = 1; i < *length - 1; i++, addr++)
      {
         shared_ptr<Node> node = FindNodeByIP(m_zoneUIN, addr);
         map[i] = (node != nullptr) ? node->getId() : 0;
      }
   }
   else
   {
      uint32_t addr = m_ipAddress.getAddressV4();
      for(int i = 0; i < 2; i++)
      {
         shared_ptr<Node> node = FindNodeByIP(m_zoneUIN, addr++);
         map[i] = (node != nullptr) ? node->getId() : 0;
      }
   }
   return map;
}

/**
 * Prepare node object for deletion
 */
void Subnet::prepareForDeletion()
{
   EventBuilder(EVENT_SUBNET_DELETED, g_dwMgmtNode)
      .param(_T("subnetObjectId"), m_id, EventBuilder::OBJECT_ID_FORMAT)
      .param(_T("subnetName"), m_name)
      .param(_T("ipAddress"), m_ipAddress)
      .param(_T("networkMask"), m_ipAddress.getMaskBits())
      .post();
   super::prepareForDeletion();
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

   unlockProperties();
   return root;
}

/**
 * Create NXSL object for this object
 */
NXSL_Value *Subnet::createNXSLObject(NXSL_VM *vm)
{
   return vm->createValue(vm->createObject(&g_nxslSubnetClass, new shared_ptr<Subnet>(self())));
}

/**
 * Check if given address overlaps with any subnet
 * Will return subnet id if overlap with any or 0 if not
 */
IntegerArray<uint32_t> CheckSubnetOverlap(const InetAddress &addr, int32_t uin)
{
   unique_ptr<SharedObjectArray<NetObj>> subnets;
   IntegerArray<uint32_t> overlappingSubnet;
   if (IsZoningEnabled())
   {
      auto zone = FindZoneByUIN(uin);
      if (zone != nullptr)
         subnets = zone->getSubnets();
   }
   else
   {
      subnets = g_idxSubnetByAddr.getObjects();
   }

   if (subnets != nullptr)
   {
      for (int i = 0; i < subnets->size(); i++)
      {
         auto subnet = static_cast<Subnet *>(subnets->get(i));
         auto subnetAddr = subnet->getIpAddress();
         if (addr.contains(subnetAddr) || subnetAddr.contains(addr))
         {
            overlappingSubnet.add(subnet->getId());
         }
      }
   }

   return overlappingSubnet;
}
