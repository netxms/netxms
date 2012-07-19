/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2012 Victor Kirhenshtein
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


//
// Constructor
//

ForwardingDatabase::ForwardingDatabase()
{
	m_fdb = NULL;
	m_fdbSize = 0;
	m_fdbAllocated = 0;
	m_portMap = NULL;
	m_pmSize = 0;
	m_pmAllocated = 0;
	m_timestamp = time(NULL);
}


//
// Destructor
//

ForwardingDatabase::~ForwardingDatabase()
{
	safe_free(m_fdb);
	safe_free(m_portMap);
}


//
// Add port mapping entry
//

void ForwardingDatabase::addPortMapping(PORT_MAPPING_ENTRY *entry)
{
	if (m_pmSize == m_pmAllocated)
	{
		m_pmAllocated += 32;
		m_portMap = (PORT_MAPPING_ENTRY *)realloc(m_portMap, sizeof(PORT_MAPPING_ENTRY) * m_pmAllocated);
	}
	memcpy(&m_portMap[m_pmSize], entry, sizeof(PORT_MAPPING_ENTRY));
	m_pmSize++;
}


//
// Get interface index for given port number
//

DWORD ForwardingDatabase::ifIndexFromPort(DWORD port)
{
	for(int i = 0; i < m_pmSize; i++)
		if (m_portMap[i].port == port)
			return m_portMap[i].ifIndex;
	return 0;
}


//
// Add entry
//

void ForwardingDatabase::addEntry(FDB_ENTRY *entry)
{
	// Check for duplicate
	for(int i = 0; i < m_fdbSize; i++)
		if (!memcmp(m_fdb[i].macAddr, entry->macAddr, MAC_ADDR_LENGTH))
		{
			memcpy(&m_fdb[i], entry, sizeof(FDB_ENTRY));
			m_fdb[i].ifIndex = ifIndexFromPort(entry->port);
			return;
		}

	if (m_fdbSize == m_fdbAllocated)
	{
		m_fdbAllocated += 32;
		m_fdb = (FDB_ENTRY *)realloc(m_fdb, sizeof(FDB_ENTRY) * m_fdbAllocated);
	}
	memcpy(&m_fdb[m_fdbSize], entry, sizeof(FDB_ENTRY));
	m_fdb[m_fdbSize].ifIndex = ifIndexFromPort(entry->port);
	m_fdbSize++;
}


//
// Find MAC address
// Returns interface index or 0 if MAC address not found
//

static int EntryComparator(const void *p1, const void *p2)
{
	return memcmp(((FDB_ENTRY *)p1)->macAddr, ((FDB_ENTRY *)p2)->macAddr, MAC_ADDR_LENGTH);
}

DWORD ForwardingDatabase::findMacAddress(const BYTE *macAddr)
{
	FDB_ENTRY key;
	memcpy(key.macAddr, macAddr, MAC_ADDR_LENGTH);
	FDB_ENTRY *entry = (FDB_ENTRY *)bsearch(&key, m_fdb, m_fdbSize, sizeof(FDB_ENTRY), EntryComparator);
	return (entry != NULL) ? entry->ifIndex : 0;
}


//
// Check if port has only one MAC in FDB
// If macAddr parameter is not NULL, MAC address found on port
// copied into provided buffer
//

bool ForwardingDatabase::isSingleMacOnPort(DWORD ifIndex, BYTE *macAddr)
{
	int count = 0;
	for(int i = 0; i < m_fdbSize; i++)
		if (m_fdb[i].ifIndex == ifIndex)
		{
			count++;
			if (count > 1)
				return false;
			if (macAddr != NULL)
				memcpy(macAddr, m_fdb[i].macAddr, MAC_ADDR_LENGTH);
		}

	return count == 1;
}


//
// Get number of MAC addresses on given port
//

int ForwardingDatabase::getMacCountOnPort(DWORD ifIndex)
{
	int count = 0;
	for(int i = 0; i < m_fdbSize; i++)
		if (m_fdb[i].ifIndex == ifIndex)
		{
			count++;
		}

	return count;
}


//
// Sort FDB
//

void ForwardingDatabase::sort()
{
	qsort(m_fdb, m_fdbSize, sizeof(FDB_ENTRY), EntryComparator);
}


//
// FDB walker's callback
//

static DWORD FDBHandler(DWORD dwVersion, SNMP_Variable *pVar, SNMP_Transport *pTransport, void *arg)
{
   SNMP_ObjectId *pOid = pVar->GetName();
	DWORD oidLen = pOid->getLength();
	DWORD oid[MAX_OID_LEN];
	memcpy(oid, pOid->getValue(), oidLen * sizeof(DWORD));

	// Get port number and status
   SNMP_PDU *pRqPDU = new SNMP_PDU(SNMP_GET_REQUEST, SnmpNewRequestId(), dwVersion);

	oid[10] = 2;	// .1.3.6.1.2.1.17.4.3.1.2 - port number
	pRqPDU->bindVariable(new SNMP_Variable(oid, oidLen));

	oid[10] = 3;	// .1.3.6.1.2.1.17.4.3.1.3 - status
	pRqPDU->bindVariable(new SNMP_Variable(oid, oidLen));

   SNMP_PDU *pRespPDU;
   DWORD rcc = pTransport->doRequest(pRqPDU, &pRespPDU, g_dwSNMPTimeout, 3);
	delete pRqPDU;

	if (rcc == SNMP_ERR_SUCCESS)
   {
		int port = pRespPDU->getVariable(0)->GetValueAsInt();
		int status = pRespPDU->getVariable(1)->GetValueAsInt();
		if ((port > 0) && (status == 3))		// status 3 == learned
		{
			FDB_ENTRY entry;

			memset(&entry, 0, sizeof(FDB_ENTRY));
			entry.port = (DWORD)port;
			pVar->getRawValue(entry.macAddr, MAC_ADDR_LENGTH);
			Node *node = FindNodeByMAC(entry.macAddr);
			entry.nodeObject = (node != NULL) ? node->Id() : 0;
			((ForwardingDatabase *)arg)->addEntry(&entry);
		}
      delete pRespPDU;
	}

	return rcc;
}


//
// dot1qTpFdbEntry walker's callback
//

static DWORD Dot1qTpFdbHandler(DWORD dwVersion, SNMP_Variable *pVar, SNMP_Transport *pTransport, void *arg)
{
	int port = pVar->GetValueAsInt();
	if (port == 0)
		return SNMP_ERR_SUCCESS;

   SNMP_ObjectId *pOid = pVar->GetName();
	DWORD oidLen = pOid->getLength();
	DWORD oid[MAX_OID_LEN];
	memcpy(oid, pOid->getValue(), oidLen * sizeof(DWORD));

	// Get port number and status
   SNMP_PDU *pRqPDU = new SNMP_PDU(SNMP_GET_REQUEST, SnmpNewRequestId(), dwVersion);

	oid[12] = 3;	// .1.3.6.1.2.1.17.7.1.2.2.1.3 - status
	pRqPDU->bindVariable(new SNMP_Variable(oid, oidLen));

   SNMP_PDU *pRespPDU;
   DWORD rcc = pTransport->doRequest(pRqPDU, &pRespPDU, g_dwSNMPTimeout, 3);
	delete pRqPDU;

	if (rcc == SNMP_ERR_SUCCESS)
   {
		int status = pRespPDU->getVariable(0)->GetValueAsInt();
		if (status == 3)	// status 3 == learned
		{
			FDB_ENTRY entry;

			memset(&entry, 0, sizeof(FDB_ENTRY));
			entry.port = (DWORD)port;
			for(DWORD i = oidLen - MAC_ADDR_LENGTH, j = 0; i < oidLen; i++)
				entry.macAddr[j++] = (BYTE)oid[i];
			Node *node = FindNodeByMAC(entry.macAddr);
			entry.nodeObject = (node != NULL) ? node->Id() : 0;
			((ForwardingDatabase *)arg)->addEntry(&entry);
		}
      delete pRespPDU;
	}

	return rcc;
}


//
// dot1dBasePortTable walker's callback
//

static DWORD Dot1dPortTableHandler(DWORD dwVersion, SNMP_Variable *pVar, SNMP_Transport *pTransport, void *arg)
{
   SNMP_ObjectId *pOid = pVar->GetName();
	PORT_MAPPING_ENTRY pm;
	pm.port = pOid->getValue()[pOid->getLength() - 1];
	pm.ifIndex = pVar->GetValueAsUInt();
	((ForwardingDatabase *)arg)->addPortMapping(&pm);
	return SNMP_ERR_SUCCESS;
}


//
// Get switch forwarding database from node
//

ForwardingDatabase *GetSwitchForwardingDatabase(Node *node)
{
	if (!node->isBridge())
		return NULL;

	ForwardingDatabase *fdb = new ForwardingDatabase();
	node->CallSnmpEnumerate(_T(".1.3.6.1.2.1.17.1.4.1.2"), Dot1dPortTableHandler, fdb);
	node->CallSnmpEnumerate(_T(".1.3.6.1.2.1.17.7.1.2.2.1.2"), Dot1qTpFdbHandler, fdb);
	DbgPrintf(5, _T("FDB: %d entries read from dot1qTpFdbTable"), fdb->getSize());
	node->CallSnmpEnumerate(_T(".1.3.6.1.2.1.17.4.3.1.1"), FDBHandler, fdb);
	DbgPrintf(5, _T("FDB: %d entries read from dot1dTpFdbTable"), fdb->getSize());
	fdb->sort();
	return fdb;
}
