/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2014 Victor Kirhenshtein
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
** File: bridge.cpp
**
**/

#include "nxcore.h"

/**
 * Map port numbers from dot1dBasePortTable to interface indexes
 */
static UINT32 PortMapCallback(UINT32 snmpVersion, SNMP_Variable *var, SNMP_Transport *transport, void *arg)
{
   TCHAR oid[MAX_OID_LEN * 4], suffix[MAX_OID_LEN * 4];
   SNMP_ObjectId *pOid = var->getName();
   SNMPConvertOIDToText(pOid->getLength() - 11, (UINT32 *)&(pOid->getValue())[11], suffix, MAX_OID_LEN * 4);

	// Get interface index
   SNMP_PDU *pRqPDU = new SNMP_PDU(SNMP_GET_REQUEST, SnmpNewRequestId(), snmpVersion);
	_tcscpy(oid, _T(".1.3.6.1.2.1.17.1.4.1.2"));
   _tcscat(oid, suffix);
	pRqPDU->bindVariable(new SNMP_Variable(oid));

	SNMP_PDU *pRespPDU;
   UINT32 rcc = transport->doRequest(pRqPDU, &pRespPDU, g_dwSNMPTimeout, 3);
	delete pRqPDU;

	if (rcc == SNMP_ERR_SUCCESS)
   {
		UINT32 ifIndex = pRespPDU->getVariable(0)->getValueAsUInt();
		InterfaceList *ifList = (InterfaceList *)arg;
		for(int i = 0; i < ifList->size(); i++)
			if (ifList->get(i)->dwIndex == ifIndex)
			{
				ifList->get(i)->dwBridgePortNumber = var->getValueAsUInt();
				break;
			}
      delete pRespPDU;
	}
	return SNMP_ERR_SUCCESS;
}

void BridgeMapPorts(int snmpVersion, SNMP_Transport *transport, InterfaceList *ifList)
{
	SnmpWalk(snmpVersion, transport, _T(".1.3.6.1.2.1.17.1.4.1.1"), PortMapCallback, ifList, FALSE);
}
