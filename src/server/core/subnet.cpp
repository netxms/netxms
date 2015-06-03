/*
** NetXMS - Network Management System
** Copyright (C) 2003-2013 Victor Kirhenshtein
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
Subnet::Subnet() : NetObj()
{
   m_zoneId = 0;
	m_bSyntheticMask = false;
}

/**
 * Subnet class constructor
 */
Subnet::Subnet(const InetAddress& addr, UINT32 dwZone, bool bSyntheticMask) : NetObj()
{
   TCHAR szBuffer[64];
   _sntprintf(m_name, MAX_OBJECT_NAME, _T("%s/%d"), addr.toString(szBuffer), addr.getMaskBits());
   m_ipAddress = addr;
   m_zoneId = dwZone;
	m_bSyntheticMask = bSyntheticMask;
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
BOOL Subnet::loadFromDatabase(UINT32 dwId)
{
   TCHAR szQuery[256];
   DB_RESULT hResult;

   m_id = dwId;

   if (!loadCommonProperties())
      return FALSE;

   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("SELECT ip_addr,ip_netmask,zone_guid,synthetic_mask FROM subnets WHERE id=%d"), dwId);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult == 0)
      return FALSE;     // Query failed

   if (DBGetNumRows(hResult) == 0)
   {
      DBFreeResult(hResult);
      return FALSE;
   }

   m_ipAddress = DBGetFieldInetAddr(hResult, 0, 0);
   m_ipAddress.setMaskBits(DBGetFieldLong(hResult, 0, 1));
   m_zoneId = DBGetFieldULong(hResult, 0, 2);
	m_bSyntheticMask = DBGetFieldLong(hResult, 0, 3) ? true : false;

   DBFreeResult(hResult);

   // Load access list
   loadACLFromDB();

   return TRUE;
}

/**
 * Save subnet object to database
 */
BOOL Subnet::saveToDatabase(DB_HANDLE hdb)
{
   TCHAR szQuery[1024], szIpAddr[64];
   UINT32 i;

   // Lock object's access
   lockProperties();

   saveCommonProperties(hdb);

   // Form and execute INSERT or UPDATE query
   if (IsDatabaseRecordExist(hdb, _T("subnets"), _T("id"), m_id))
      _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR),
		           _T("UPDATE subnets SET ip_addr='%s',ip_netmask=%d,zone_guid=%d,synthetic_mask=%d WHERE id=%d"),
                 m_ipAddress.toString(szIpAddr), m_ipAddress.getMaskBits(), m_zoneId, m_bSyntheticMask ? 1 : 0, m_id);
   else
      _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR),
		           _T("INSERT INTO subnets (id,ip_addr,ip_netmask,zone_guid,synthetic_mask) VALUES (%d,'%s',%d,%d,%d)"),
                 m_id, m_ipAddress.toString(szIpAddr), m_ipAddress.getMaskBits(), m_zoneId, m_bSyntheticMask ? 1 : 0);
   DBQuery(hdb, szQuery);

   // Update node to subnet mapping
   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM nsmap WHERE subnet_id=%d"), m_id);
   DBQuery(hdb, szQuery);
   LockChildList(FALSE);
   for(i = 0; i < m_dwChildCount; i++)
   {
      _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("INSERT INTO nsmap (subnet_id,node_id) VALUES (%d,%d)"), m_id, m_pChildList[i]->getId());
      DBQuery(hdb, szQuery);
   }
   UnlockChildList();

   // Save access list
   saveACLToDB(hdb);

   // Clear modifications flag and unlock object
   m_isModified = false;
   unlockProperties();

   return TRUE;
}

/**
 * Delete subnet object from database
 */
bool Subnet::deleteFromDatabase(DB_HANDLE hdb)
{
   bool success = NetObj::deleteFromDatabase(hdb);
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM subnets WHERE id=?"));
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM nsmap WHERE subnet_id=?"));
   return success;
}

/**
 * Create CSCP message with object's data
 */
void Subnet::fillMessageInternal(NXCPMessage *pMsg)
{
   NetObj::fillMessageInternal(pMsg);
   pMsg->setField(VID_IP_ADDRESS, m_ipAddress);
   pMsg->setField(VID_ZONE_ID, m_zoneId);
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
      g_idxSubnetByAddr.put(m_ipAddress, this);
   }
	setModified();
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
bool Subnet::findMacAddress(const InetAddress& ipAddr, BYTE *macAddr)
{
	bool success = false;

	LockChildList(FALSE);

   for(UINT32 i = 0; (i < m_dwChildCount) && !success; i++)
   {
      if (m_pChildList[i]->getObjectClass() != OBJECT_NODE)
			continue;

		Node *node = (Node *)m_pChildList[i];
		DbgPrintf(6, _T("Subnet[%s]::findMacAddress: reading ARP cache for node %s [%u]"), m_name, node->getName(), node->getId());
		ARP_CACHE *arpCache = node->getArpCache();
		if (arpCache == NULL)
			continue;

		for(UINT32 j = 0; j < arpCache->dwNumEntries; j++)
		{
			if (arpCache->pEntries[j].ipAddr.equals(ipAddr))
			{
				memcpy(macAddr, arpCache->pEntries[j].bMacAddr, MAC_ADDR_LENGTH);
				success = true;
				break;
			}
		}

		DestroyArpCache(arpCache);
   }

	UnlockChildList();

	return success;
}

/**
 * Build IP topology
 */
void Subnet::buildIPTopologyInternal(nxmap_ObjList &topology, int nDepth, UINT32 seedNode, bool includeEndNodes)
{
	ObjectArray<Node> nodes;
	LockChildList(FALSE);
	for(UINT32 i = 0; i < m_dwChildCount; i++)
	{
		if ((m_pChildList[i]->getId() == seedNode) || (m_pChildList[i]->getObjectClass() != OBJECT_NODE))
			continue;
		if (!includeEndNodes && !((Node *)m_pChildList[i])->isRouter())
			continue;
		m_pChildList[i]->incRefCount();
		nodes.add((Node *)m_pChildList[i]);
	}
	UnlockChildList();

	for(int j = 0; j < nodes.size(); j++)
	{
		Node *n = nodes.get(j);
		n->buildIPTopologyInternal(topology, nDepth - 1, m_id, false, includeEndNodes);
		n->decRefCount();
	}
}

/**
 * Called by client session handler to check if threshold summary should be shown for this object.
 */
bool Subnet::showThresholdSummary()
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
   UINT32 *map = (UINT32 *)malloc(*length * sizeof(UINT32));

   map[0] = 0xFFFFFFFF; // subnet
   map[*length - 1] = 0xFFFFFFFF;   // broadcast
   UINT32 addr = m_ipAddress.getAddressV4() + 1;
   for(int i = 1; i < *length - 1; i++, addr++)
   {
      Node *node = FindNodeByIP(m_zoneId, addr);
      map[i] = (node != NULL) ? node->getId() : 0;
   }

   return map;
}

/**
 * Prepare node object for deletion
 */
void Subnet::prepareForDeletion()
{
   PostEvent(EVENT_SUBNET_DELETED, g_dwMgmtNode, "isAd", m_id, m_name, &m_ipAddress, m_ipAddress.getMaskBits());
   NetObj::prepareForDeletion();
}
