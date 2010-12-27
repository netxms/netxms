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
** File: layer2.cpp
**
**/

#include "nxcore.h"


//
// Peer list
//

struct PEER
{
	DWORD dwAddr;
	TCHAR szIfName[MAX_CONNECTOR_NAME];
};

class PeerList
{
public:
	DWORD m_dwNumPeers;
	PEER *m_pPeerList;

	PeerList() { m_dwNumPeers = 0; m_pPeerList = NULL; }
	~PeerList() { safe_free(m_pPeerList); }

	void Add(DWORD dwIpAddr, TCHAR *pszIfName)
	{
		m_pPeerList = (PEER *)realloc(m_pPeerList, sizeof(PEER) * (m_dwNumPeers + 1));
		m_pPeerList[m_dwNumPeers].dwAddr = dwIpAddr;
		nx_strncpy(m_pPeerList[m_dwNumPeers].szIfName, pszIfName, MAX_CONNECTOR_NAME);
		m_dwNumPeers++;
	}
};


//
// Topology table walker's callback for SONMP topology table
//

static DWORD SONMPTopoHandler(DWORD dwVersion, SNMP_Variable *pVar, SNMP_Transport *pTransport, void *pArg)
{
   SNMP_ObjectId *pOid;
   TCHAR szOid[MAX_OID_LEN * 4], szSuffix[MAX_OID_LEN * 4], szIfName[MAX_CONNECTOR_NAME];
   SNMP_PDU *pRqPDU, *pRespPDU;
	DWORD dwResult;

   pOid = pVar->GetName();
   SNMPConvertOIDToText(pOid->Length() - 14, (DWORD *)&(pOid->GetValue())[14], szSuffix, MAX_OID_LEN * 4);

	// Get interface
   pRqPDU = new SNMP_PDU(SNMP_GET_REQUEST, SnmpNewRequestId(), dwVersion);
   _tcscpy(szOid, _T(".1.3.6.1.4.1.45.1.6.13.2.1.1.1"));	// Slot
   _tcscat(szOid, szSuffix);
	pRqPDU->bindVariable(new SNMP_Variable(szOid));
   _tcscpy(szOid, _T(".1.3.6.1.4.1.45.1.6.13.2.1.1.2"));	// Port
   _tcscat(szOid, szSuffix);
	pRqPDU->bindVariable(new SNMP_Variable(szOid));
   dwResult = pTransport->doRequest(pRqPDU, &pRespPDU, g_dwSNMPTimeout, 3);
   delete pRqPDU;

   if (dwResult == SNMP_ERR_SUCCESS)
   {
		_sntprintf(szIfName, MAX_CONNECTOR_NAME, _T("%d/%d"), pRespPDU->getVariable(0)->GetValueAsUInt(),
		           pRespPDU->getVariable(1)->GetValueAsUInt());
		((PeerList *)pArg)->Add(ntohl(pVar->GetValueAsUInt()), szIfName);
      delete pRespPDU;
	}

	return dwResult;
}


//
// Topology table walker's callback for CDP topology table
//

static DWORD CDPTopoHandler(DWORD dwVersion, SNMP_Variable *pVar, SNMP_Transport *pTransport, void *pArg)
{
   SNMP_ObjectId *pOid;
   TCHAR szOid[MAX_OID_LEN * 4], szSuffix[MAX_OID_LEN * 4],
	      szIfName[MAX_CONNECTOR_NAME], szIpAddr[16];
   SNMP_PDU *pRqPDU, *pRespPDU;
	DWORD dwResult;

   pOid = pVar->GetName();
   SNMPConvertOIDToText(pOid->Length() - 14, (DWORD *)&(pOid->GetValue())[14], szSuffix, MAX_OID_LEN * 4);

	// Get interface
   pRqPDU = new SNMP_PDU(SNMP_GET_REQUEST, SnmpNewRequestId(), dwVersion);
   _tcscpy(szOid, _T(".1.3.6.1.4.1.9.9.23.1.2.1.1.7"));	// Remote port name
   _tcscat(szOid, szSuffix);
	pRqPDU->bindVariable(new SNMP_Variable(szOid));
   dwResult = pTransport->doRequest(pRqPDU, &pRespPDU, g_dwSNMPTimeout, 3);
   delete pRqPDU;

   if (dwResult == SNMP_ERR_SUCCESS)
   {
		((PeerList *)pArg)->Add(ntohl(_t_inet_addr(pVar->GetValueAsIPAddr(szIpAddr))),
		                        pRespPDU->getVariable(0)->GetValueAsString(szIfName, MAX_CONNECTOR_NAME));
      delete pRespPDU;
	}

	return dwResult;
}


//
// Build layer 2 topology for switch
//

DWORD BuildL2Topology(nxmap_ObjList &topology, Node *pRoot, Node *pParent, int nDepth, TCHAR *pszParentIfName)
{
	PeerList *pList;
	DWORD i, dwResult = RCC_SNMP_FAILURE;
	TCHAR szPeerIfName[MAX_CONNECTOR_NAME] = _T("<N/A>");
	Node *pNode;

	pList = new PeerList;
	if (pRoot->getFlags() & NF_IS_SONMP)
	{
		if (pRoot->CallSnmpEnumerate(_T(".1.3.6.1.4.1.45.1.6.13.2.1.1.3"), SONMPTopoHandler, pList) != SNMP_ERR_SUCCESS)
			goto cleanup;
	}
	else if (pRoot->getFlags() & NF_IS_CDP)
	{
		if (pRoot->CallSnmpEnumerate(_T(".1.3.6.1.4.1.9.9.23.1.2.1.1.4"), CDPTopoHandler, pList) != SNMP_ERR_SUCCESS)
			goto cleanup;
	}

	dwResult = RCC_SUCCESS;
	topology.AddObject(pRoot->Id());

	for(i = 0; i < pList->m_dwNumPeers; i++)
	{
		pNode = FindNodeByIP(pList->m_pPeerList[i].dwAddr);
		if (pNode != NULL)
		{
			if ((pParent != NULL) && (pParent->Id() == pNode->Id()))
			{
				_tcscpy(pszParentIfName, pList->m_pPeerList[i].szIfName);
			}
			else if ((pNode->Id() != pRoot->Id()) && (nDepth > 0))
			{
				dwResult = BuildL2Topology(topology, pNode, pRoot, nDepth - 1, szPeerIfName);
				if (dwResult != RCC_SUCCESS)
					break;
				topology.AddObject(pNode->Id());

				// If we use CDP, szIfName member contains remote port name, not a local port name
				if (pRoot->getFlags() & NF_IS_CDP)
				{
					topology.LinkObjectsEx(pRoot->Id(), pNode->Id(), szPeerIfName, pList->m_pPeerList[i].szIfName);
				}
				else
				{
					topology.LinkObjectsEx(pRoot->Id(), pNode->Id(), pList->m_pPeerList[i].szIfName, szPeerIfName);
				}
			}
		}
	}

cleanup:
	delete pList;
	return dwResult;
}


//
// Find connection point for interface
//

Interface *FindInterfaceConnectionPoint(const BYTE *macAddr)
{
	TCHAR macAddrText[32];
	DbgPrintf(6, _T("Called FindInterfaceConnectionPoint(%s)"), MACToStr(macAddr, macAddrText));

	if (g_pNodeIndexByAddr == NULL)
      return NULL;

	Interface *iface = NULL;
   RWLockReadLock(g_rwlockNodeIndex, INFINITE);
   for (DWORD i = 0; (i < g_dwNodeAddrIndexSize) && (iface == NULL); i++)
	{
		ForwardingDatabase *fdb = ((Node *)g_pNodeIndexByAddr[i].pObject)->getSwitchForwardingDatabase();
		if (fdb != NULL)
		{
			DbgPrintf(6, _T("FindInterfaceConnectionPoint(%s): FDB obtained for node %s [%d]"), macAddrText,
				((Node *)g_pNodeIndexByAddr[i].pObject)->Name(), (int)((Node *)g_pNodeIndexByAddr[i].pObject)->Id());
			DWORD ifIndex = fdb->findMacAddress(macAddr);
			if ((ifIndex != 0) && fdb->isSingleMacOnPort(ifIndex))
			{
				iface = ((Node *)g_pNodeIndexByAddr[i].pObject)->findInterface(ifIndex, INADDR_ANY);
				if (iface != NULL)
				{
					DbgPrintf(4, _T("FindInterfaceConnectionPoint(%s): found interface %s [%d] on node %s [%d]"), macAddrText,
						iface->Name(), (int)iface->Id(), iface->GetParentNode()->Name(), (int)iface->GetParentNode()->Id());
				}
				else
				{
					DbgPrintf(4, _T("FindInterfaceConnectionPoint(%s): cannot find interface object for node %s [%d] ifIndex %d"),
						macAddrText, ((Node *)g_pNodeIndexByAddr[i].pObject)->Name(), ((Node *)g_pNodeIndexByAddr[i].pObject)->Id(), ifIndex);
				}
			}
			fdb->decRefCount();
		}
	}
   RWLockUnlock(g_rwlockNodeIndex);
	return iface;
}
