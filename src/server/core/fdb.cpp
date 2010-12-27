/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2010 Victor Kirhenshtein
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
	m_count = 0;
	m_allocated = 0;
	m_timestamp = time(NULL);
	m_refCount = 1;
}


//
// Destructor
//

ForwardingDatabase::~ForwardingDatabase()
{
	safe_free(m_fdb);
}


//
// Decrement reference count. If it reaches 0, destroy object.
//

void ForwardingDatabase::decRefCount()
{
	m_refCount--;
	if (m_refCount == 0)
		delete this;
}


//
// Add entry
//

void ForwardingDatabase::addEntry(FDB_ENTRY *entry)
{
	if (m_count == m_allocated)
	{
		m_allocated += 32;
		m_fdb = (FDB_ENTRY *)realloc(m_fdb, sizeof(FDB_ENTRY) * m_allocated);
	}
	memcpy(&m_fdb[m_count], entry, sizeof(FDB_ENTRY));
	m_count++;
}


//
// FDB walker's callback
//

static DWORD FDBHandler(DWORD dwVersion, SNMP_Variable *pVar, SNMP_Transport *pTransport, void *arg)
{
   SNMP_ObjectId *pOid = pVar->GetName();
   TCHAR szOid[MAX_OID_LEN * 4], szSuffix[MAX_OID_LEN * 4];
   SNMPConvertOIDToText(pOid->Length() - 11, (DWORD *)&(pOid->GetValue())[11], szSuffix, MAX_OID_LEN * 4);

	// Get interface
   SNMP_PDU *pRqPDU = new SNMP_PDU(SNMP_GET_REQUEST, SnmpNewRequestId(), dwVersion);

	_tcscpy(szOid, _T(".1.3.6.1.2.1.17.4.3.1.2"));	// Interface index
   _tcscat(szOid, szSuffix);
	pRqPDU->bindVariable(new SNMP_Variable(szOid));

   SNMP_PDU *pRespPDU;
   DWORD rcc = pTransport->doRequest(pRqPDU, &pRespPDU, g_dwSNMPTimeout, 3);
	delete pRqPDU;

	if (rcc == SNMP_ERR_SUCCESS)
   {
		FDB_ENTRY entry;

		memset(&entry, 0, sizeof(FDB_ENTRY));
		entry.ifIndex = pRespPDU->getVariable(0)->GetValueAsUInt();
		pVar->getRawValue(entry.macAddr, 6);
		Node *node = FindNodeByMAC(entry.macAddr);
		entry.nodeObject = (node != NULL) ? node->Id() : 0;
		((ForwardingDatabase *)arg)->addEntry(&entry);
      delete pRespPDU;
	}

	return rcc;
}


//
// Get switch forwarding database from node
//

ForwardingDatabase *GetSwitchForwardingDatabase(Node *node)
{
	if (!node->isBridge())
		return NULL;

	ForwardingDatabase *fdb = new ForwardingDatabase();
	node->CallSnmpEnumerate(_T(".1.3.6.1.2.1.17.4.3.1.1"), FDBHandler, fdb);
	return fdb;
}
