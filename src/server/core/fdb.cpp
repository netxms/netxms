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
** File: fdb.cpp
**
**/

#include "nxcore.h"

/**
 * Constructor
 */
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

/**
 * Destructor
 */
ForwardingDatabase::~ForwardingDatabase()
{
	safe_free(m_fdb);
	safe_free(m_portMap);
}

/**
 * Add port mapping entry
 */
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

/**
 * Get interface index for given port number
 */
UINT32 ForwardingDatabase::ifIndexFromPort(UINT32 port)
{
	for(int i = 0; i < m_pmSize; i++)
		if (m_portMap[i].port == port)
			return m_portMap[i].ifIndex;
	return 0;
}

/**
 * Add entry
 */
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

/**
 * FDB entry comparator
 */
static int EntryComparator(const void *p1, const void *p2)
{
	return memcmp(((FDB_ENTRY *)p1)->macAddr, ((FDB_ENTRY *)p2)->macAddr, MAC_ADDR_LENGTH);
}

/**
 * Find MAC address
 * Returns interface index or 0 if MAC address not found
 */
UINT32 ForwardingDatabase::findMacAddress(const BYTE *macAddr)
{
	FDB_ENTRY key;
	memcpy(key.macAddr, macAddr, MAC_ADDR_LENGTH);
	FDB_ENTRY *entry = (FDB_ENTRY *)bsearch(&key, m_fdb, m_fdbSize, sizeof(FDB_ENTRY), EntryComparator);
	return (entry != NULL) ? entry->ifIndex : 0;
}

/**
 * Check if port has only one MAC in FDB
 * If macAddr parameter is not NULL, MAC address found on port
 * copied into provided buffer
 */
bool ForwardingDatabase::isSingleMacOnPort(UINT32 ifIndex, BYTE *macAddr)
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

/**
 * Get number of MAC addresses on given port
 */
int ForwardingDatabase::getMacCountOnPort(UINT32 ifIndex)
{
	int count = 0;
	for(int i = 0; i < m_fdbSize; i++)
		if (m_fdb[i].ifIndex == ifIndex)
		{
			count++;
		}

	return count;
}

/**
 * Print to console
 */
void ForwardingDatabase::print(CONSOLE_CTX ctx, Node *owner)
{
   TCHAR macAddrStr[24];

   ConsolePrintf(ctx, _T("\x1b[1mMAC address\x1b[0m       | \x1b[1mIfIndex\x1b[0m | \x1b[1mInterface\x1b[0m            | \x1b[1mPort\x1b[0m | \x1b[1mNode\x1b[0m  | \x1b[1mNode name\x1b[0m\n"));
   ConsolePrintf(ctx, _T("------------------+---------+----------------------+------+-------+-----------------------------\n"));
	for(int i = 0; i < m_fdbSize; i++)
   {
      Node *node = (Node *)FindObjectById(m_fdb[i].nodeObject, OBJECT_NODE);
      Interface *iface = owner->findInterface(m_fdb[i].ifIndex, INADDR_ANY);
      ConsolePrintf(ctx, _T("%s | %7d | %-20s | %4d | %5d | %s\n"), MACToStr(m_fdb[i].macAddr, macAddrStr),
         m_fdb[i].ifIndex, (iface != NULL) ? iface->Name() : _T("\x1b[31;1mUNKNOWN\x1b[0m"), 
         m_fdb[i].port, m_fdb[i].nodeObject, (node != NULL) ? node->Name() : _T("\x1b[31;1mUNKNOWN\x1b[0m"));
   }
   ConsolePrintf(ctx, _T("\n%d entries\n\n"), m_fdbSize);
}

/**
 * Fill NXCP message with FDB data
 */
void ForwardingDatabase::fillMessage(CSCPMessage *msg)
{
   msg->SetVariable(VID_NUM_ELEMENTS, (UINT32)m_fdbSize);
   UINT32 fieldId = VID_ELEMENT_LIST_BASE;
	for(int i = 0; i < m_fdbSize; i++)
   {
      msg->SetVariable(fieldId++, m_fdb[i].macAddr, MAC_ADDR_LENGTH);
      msg->SetVariable(fieldId++, m_fdb[i].ifIndex);
      msg->SetVariable(fieldId++, m_fdb[i].port);
      msg->SetVariable(fieldId++, m_fdb[i].nodeObject);
      msg->SetVariable(fieldId++, m_fdb[i].vlanId);
      msg->SetVariable(fieldId++, m_fdb[i].type);
      fieldId += 4;
   }
}

/**
 * Sort FDB
 */
void ForwardingDatabase::sort()
{
	qsort(m_fdb, m_fdbSize, sizeof(FDB_ENTRY), EntryComparator);
}

/**
 * FDB walker's callback
 */
static UINT32 FDBHandler(UINT32 dwVersion, SNMP_Variable *pVar, SNMP_Transport *pTransport, void *arg)
{
   SNMP_ObjectId *pOid = pVar->getName();
	size_t oidLen = pOid->getLength();
	UINT32 oid[MAX_OID_LEN];
	memcpy(oid, pOid->getValue(), oidLen * sizeof(UINT32));

	// Get port number and status
   SNMP_PDU *pRqPDU = new SNMP_PDU(SNMP_GET_REQUEST, SnmpNewRequestId(), dwVersion);

	oid[10] = 2;	// .1.3.6.1.2.1.17.4.3.1.2 - port number
	pRqPDU->bindVariable(new SNMP_Variable(oid, oidLen));

	oid[10] = 3;	// .1.3.6.1.2.1.17.4.3.1.3 - status
	pRqPDU->bindVariable(new SNMP_Variable(oid, oidLen));

   SNMP_PDU *pRespPDU;
   UINT32 rcc = pTransport->doRequest(pRqPDU, &pRespPDU, g_dwSNMPTimeout, 3);
	delete pRqPDU;

	if (rcc == SNMP_ERR_SUCCESS)
   {
      SNMP_Variable *varPort = pRespPDU->getVariable(0);
      SNMP_Variable *varStatus = pRespPDU->getVariable(1);
      if (varPort != NULL && varStatus != NULL)
      {
         int port = varPort->getValueAsInt();
         int status = varStatus->getValueAsInt();
         if ((port > 0) && (status == 3))  // status: 3 == learned
         {
            FDB_ENTRY entry;

            memset(&entry, 0, sizeof(FDB_ENTRY));
            entry.port = (UINT32)port;
            pVar->getRawValue(entry.macAddr, MAC_ADDR_LENGTH);
            Node *node = FindNodeByMAC(entry.macAddr);
            entry.nodeObject = (node != NULL) ? node->Id() : 0;
            entry.vlanId = ((ForwardingDatabase *)arg)->getCurrentVlanId();
            entry.type = (UINT16)status;
            ((ForwardingDatabase *)arg)->addEntry(&entry);
         }
      }
      delete pRespPDU;
	}

	return rcc;
}

/**
 * dot1qTpFdbEntry walker's callback
 */
static UINT32 Dot1qTpFdbHandler(UINT32 dwVersion, SNMP_Variable *pVar, SNMP_Transport *pTransport, void *arg)
{
	int port = pVar->getValueAsInt();
	if (port == 0)
		return SNMP_ERR_SUCCESS;

   SNMP_ObjectId *pOid = pVar->getName();
	size_t oidLen = pOid->getLength();
	UINT32 oid[MAX_OID_LEN];
	memcpy(oid, pOid->getValue(), oidLen * sizeof(UINT32));

	// Get port number and status
   SNMP_PDU *pRqPDU = new SNMP_PDU(SNMP_GET_REQUEST, SnmpNewRequestId(), dwVersion);

	oid[12] = 3;	// .1.3.6.1.2.1.17.7.1.2.2.1.3 - status
	pRqPDU->bindVariable(new SNMP_Variable(oid, oidLen));

   SNMP_PDU *pRespPDU;
   UINT32 rcc = pTransport->doRequest(pRqPDU, &pRespPDU, g_dwSNMPTimeout, 3);
	delete pRqPDU;

	if (rcc == SNMP_ERR_SUCCESS)
   {
		int status = pRespPDU->getVariable(0)->getValueAsInt();
		if (status == 3) // status: 3 == learned
		{
			FDB_ENTRY entry;

			memset(&entry, 0, sizeof(FDB_ENTRY));
			entry.port = (UINT32)port;
			for(size_t i = oidLen - MAC_ADDR_LENGTH, j = 0; i < oidLen; i++)
				entry.macAddr[j++] = (BYTE)oid[i];
			Node *node = FindNodeByMAC(entry.macAddr);
			entry.nodeObject = (node != NULL) ? node->Id() : 0;
         entry.vlanId = (UINT16)oid[oidLen - MAC_ADDR_LENGTH - 1];
         entry.type = (UINT16)status;
			((ForwardingDatabase *)arg)->addEntry(&entry);
		}
      delete pRespPDU;
	}

	return rcc;
}

/**
 * dot1dBasePortTable walker's callback
 */
static UINT32 Dot1dPortTableHandler(UINT32 dwVersion, SNMP_Variable *pVar, SNMP_Transport *pTransport, void *arg)
{
   SNMP_ObjectId *pOid = pVar->getName();
	PORT_MAPPING_ENTRY pm;
	pm.port = pOid->getValue()[pOid->getLength() - 1];
	pm.ifIndex = pVar->getValueAsUInt();
	((ForwardingDatabase *)arg)->addPortMapping(&pm);
	return SNMP_ERR_SUCCESS;
}

/**
 * Get switch forwarding database from node
 */
ForwardingDatabase *GetSwitchForwardingDatabase(Node *node)
{
	if (!node->isBridge())
		return NULL;

	ForwardingDatabase *fdb = new ForwardingDatabase();
	node->callSnmpEnumerate(_T(".1.3.6.1.2.1.17.1.4.1.2"), Dot1dPortTableHandler, fdb);
	node->callSnmpEnumerate(_T(".1.3.6.1.2.1.17.7.1.2.2.1.2"), Dot1qTpFdbHandler, fdb);
	int size = fdb->getSize();
	DbgPrintf(5, _T("FDB: %d entries read from dot1qTpFdbTable"), size);

   fdb->setCurrentVlanId(1);
	node->callSnmpEnumerate(_T(".1.3.6.1.2.1.17.4.3.1.1"), FDBHandler, fdb);
	DbgPrintf(5, _T("FDB: %d entries read from dot1dTpFdbTable"), fdb->getSize() - size);
	size = fdb->getSize();

	if (node->isPerVlanFdbSupported())
	{
		VlanList *vlans = node->getVlans();
		if (vlans != NULL)
		{
			for(int i = 0; i < vlans->getSize(); i++)
			{
				TCHAR context[16];
				_sntprintf(context, 16, _T("%s%d"), (node->getSNMPVersion() < SNMP_VERSION_3) ? _T("") : _T("vlan-"), vlans->get(i)->getVlanId());
            fdb->setCurrentVlanId((UINT16)vlans->get(i)->getVlanId());
				node->callSnmpEnumerate(_T(".1.3.6.1.2.1.17.4.3.1.1"), FDBHandler, fdb, context);
				DbgPrintf(5, _T("FDB: %d entries read from dot1dTpFdbTable in context %s"), fdb->getSize() - size, context);
				size = fdb->getSize();
			}
			vlans->decRefCount();
		}
	}

	fdb->sort();
	return fdb;
}
