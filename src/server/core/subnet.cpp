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
   m_dwIpNetMask = 0;
   m_zoneId = 0;
	m_bSyntheticMask = false;
}

/**
 * Subnet class constructor
 */
Subnet::Subnet(UINT32 dwAddr, UINT32 dwNetMask, UINT32 dwZone, bool bSyntheticMask) : NetObj()
{
   TCHAR szBuffer[32];

   m_dwIpAddr = dwAddr;
   m_dwIpNetMask = dwNetMask;
   _sntprintf(m_szName, MAX_OBJECT_NAME, _T("%s/%d"), IpToStr(dwAddr, szBuffer), BitsInMask(dwNetMask));
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
BOOL Subnet::CreateFromDB(UINT32 dwId)
{
   TCHAR szQuery[256];
   DB_RESULT hResult;

   m_dwId = dwId;

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

   m_dwIpAddr = DBGetFieldIPAddr(hResult, 0, 0);
   m_dwIpNetMask = DBGetFieldIPAddr(hResult, 0, 1);
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
BOOL Subnet::SaveToDB(DB_HANDLE hdb)
{
   TCHAR szQuery[1024], szIpAddr[16], szNetMask[16];
   DB_RESULT hResult;
   UINT32 i;
   BOOL bNewObject = TRUE;

   // Lock object's access
   LockData();

   saveCommonProperties(hdb);

   // Check for object's existence in database
   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("SELECT id FROM subnets WHERE id=%d"), m_dwId);
   hResult = DBSelect(hdb, szQuery);
   if (hResult != 0)
   {
      if (DBGetNumRows(hResult) > 0)
         bNewObject = FALSE;
      DBFreeResult(hResult);
   }

   // Form and execute INSERT or UPDATE query
   if (bNewObject)
      _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), 
		           _T("INSERT INTO subnets (id,ip_addr,ip_netmask,zone_guid,synthetic_mask) VALUES (%d,'%s','%s',%d,%d)"),
                 m_dwId, IpToStr(m_dwIpAddr, szIpAddr),
					  IpToStr(m_dwIpNetMask, szNetMask), m_zoneId, m_bSyntheticMask ? 1 : 0);
   else
      _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), 
		           _T("UPDATE subnets SET ip_addr='%s',ip_netmask='%s',zone_guid=%d,synthetic_mask=%d WHERE id=%d"),
                 IpToStr(m_dwIpAddr, szIpAddr),
					  IpToStr(m_dwIpNetMask, szNetMask), m_zoneId, m_bSyntheticMask ? 1 : 0, m_dwId);
   DBQuery(hdb, szQuery);

   // Update node to subnet mapping
   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM nsmap WHERE subnet_id=%d"), m_dwId);
   DBQuery(hdb, szQuery);
   LockChildList(FALSE);
   for(i = 0; i < m_dwChildCount; i++)
   {
      _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("INSERT INTO nsmap (subnet_id,node_id) VALUES (%d,%d)"), m_dwId, m_pChildList[i]->Id());
      DBQuery(hdb, szQuery);
   }
   UnlockChildList();

   // Save access list
   saveACLToDB(hdb);

   // Clear modifications flag and unlock object
   m_isModified = false;
   UnlockData();

   return TRUE;
}

/**
 * Delete subnet object from database
 */
bool Subnet::deleteFromDB(DB_HANDLE hdb)
{
   bool success = NetObj::deleteFromDB(hdb);
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM subnets WHERE id=?"));
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM nsmap WHERE subnet_id=?"));
   return success;
}

/**
 * Create CSCP message with object's data
 */
void Subnet::CreateMessage(CSCPMessage *pMsg)
{
   NetObj::CreateMessage(pMsg);
   pMsg->SetVariable(VID_IP_NETMASK, m_dwIpNetMask);
   pMsg->SetVariable(VID_ZONE_ID, m_zoneId);
	pMsg->SetVariable(VID_SYNTHETIC_MASK, (WORD)(m_bSyntheticMask ? 1 : 0));
}

/**
 * Set correct netmask for subnet
 */
void Subnet::setCorrectMask(UINT32 dwAddr, UINT32 dwMask)
{
	TCHAR szName[MAX_OBJECT_NAME], szBuffer[32];

	LockData();
	
	// Check if name is default
	_sntprintf(szName, MAX_OBJECT_NAME, _T("%s/%d"), IpToStr(m_dwIpAddr, szBuffer), BitsInMask(m_dwIpNetMask));
	if (!_tcsicmp(szName, m_szName))
	{
		// Change name
		_sntprintf(m_szName, MAX_OBJECT_NAME, _T("%s/%d"), IpToStr(dwAddr, szBuffer), BitsInMask(dwMask));
	}
	
	m_dwIpAddr = dwAddr;
	m_dwIpNetMask = dwMask;
	m_bSyntheticMask = false;

	Modify();
	UnlockData();
}

/**
 * Find MAC address for given IP address. May take a long time because it involves reading ARP caches
 * from nodes in this subnet.
 *
 * @param ipAddr IP address (host byte order)
 * @param macAddr buffer for found MAC address
 * @return true if MAC address found
 */
bool Subnet::findMacAddress(UINT32 ipAddr, BYTE *macAddr)
{
	bool success = false;

	LockChildList(FALSE);

   for(UINT32 i = 0; (i < m_dwChildCount) && !success; i++)
   {
      if (m_pChildList[i]->Type() != OBJECT_NODE)
			continue;

		Node *node = (Node *)m_pChildList[i];
		DbgPrintf(6, _T("Subnet[%s]::findMacAddress: reading ARP cache for node %s [%u]"), m_szName, node->Name(), node->Id());
		ARP_CACHE *arpCache = node->getArpCache();
		if (arpCache == NULL)
			continue;

		for(UINT32 j = 0; j < arpCache->dwNumEntries; j++)
		{
			if (arpCache->pEntries[j].dwIpAddr == ipAddr)
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
		if ((m_pChildList[i]->Id() == seedNode) || (m_pChildList[i]->Type() != OBJECT_NODE))
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
		n->buildIPTopologyInternal(topology, nDepth - 1, m_dwId, includeEndNodes);
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
   *length = 1 << (32 - BitsInMask(m_dwIpNetMask));
   if ((*length < 2) || (*length > 65536))
      return NULL;
   UINT32 *map = (UINT32 *)malloc(*length * sizeof(UINT32));

   map[0] = 0xFFFFFFFF; // subnet
   map[*length - 1] = 0xFFFFFFFF;   // broadcast
   UINT32 addr = m_dwIpAddr + 1;
   for(int i = 1; i < *length - 1; i++, addr++)
   {
      Node *node = FindNodeByIP(m_zoneId, addr);
      map[i] = (node != NULL) ? node->Id() : 0;
   }

   return map;
}

/**
 * Prepare node object for deletion
 */
void Subnet::prepareForDeletion()
{
   PostEvent(EVENT_SUBNET_DELETED, g_dwMgmtNode, "isaa", m_dwId, m_szName, m_dwIpAddr, m_dwIpNetMask);
   NetObj::prepareForDeletion();
}
