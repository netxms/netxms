/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004, 2005, 2006, 2007 Victor Kirhenshtein
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
	DWORD dwIfIndex;
	TCHAR szIfName[MAX_CONNECTOR_NAME];
};

class PeerList
{
public:
	DWORD m_dwNumPeers;
	PEER *m_pPeerList;

	PeerList() { m_dwNumPeers = 0; m_pPeerList = NULL; }
	~PeerList() { safe_free(m_pPeerList); }

	void Add(DWORD dwIpAddr, DWORD dwIfIndex, TCHAR *pszIfName)
	{
		m_pPeerList = (PEER *)realloc(m_pPeerList, sizeof(PEER) * (m_dwNumPeers + 1));
		m_pPeerList[m_dwNumPeers].dwAddr = dwIpAddr;
		m_pPeerList[m_dwNumPeers].dwIfIndex = dwIfIndex;
		nx_strncpy(m_pPeerList[m_dwNumPeers].szIfName, pszIfName, MAX_CONNECTOR_NAME);
		m_dwNumPeers++;
	}
};


//
// Topology table walker's callback for Nortel topology table
//

static DWORD NortelTopoHandler(DWORD dwVersion, const char *pszCommunity,
										 SNMP_Variable *pVar, SNMP_Transport *pTransport,
										 void *pArg)
{
   SNMP_ObjectId *pOid;
   TCHAR szOid[MAX_OID_LEN * 4], szSuffix[MAX_OID_LEN * 4], szIfName[MAX_CONNECTOR_NAME];
   SNMP_PDU *pRqPDU, *pRespPDU;
	DWORD dwResult;

   pOid = pVar->GetName();
   SNMPConvertOIDToText(pOid->Length() - 14, (DWORD *)&(pOid->GetValue())[14], szSuffix, MAX_OID_LEN * 4);

	// Get interface
   pRqPDU = new SNMP_PDU(SNMP_GET_REQUEST, (char *)pszCommunity, SnmpNewRequestId(), dwVersion);
   _tcscpy(szOid, ".1.3.6.1.4.1.45.1.6.13.2.1.1.1");	// Slot
   _tcscat(szOid, szSuffix);
	pRqPDU->BindVariable(new SNMP_Variable(szOid));
   _tcscpy(szOid, ".1.3.6.1.4.1.45.1.6.13.2.1.1.2");	// Port
   _tcscat(szOid, szSuffix);
	pRqPDU->BindVariable(new SNMP_Variable(szOid));
   dwResult = pTransport->DoRequest(pRqPDU, &pRespPDU, 1000, 3);
   delete pRqPDU;

   if (dwResult == SNMP_ERR_SUCCESS)
   {
		_stprintf(szIfName, _T("%d/%d"), pRespPDU->GetVariable(0)->GetValueAsUInt(),
		          pRespPDU->GetVariable(1)->GetValueAsUInt());
		((PeerList *)pArg)->Add(ntohl(pVar->GetValueAsUInt()),
		                        pRespPDU->GetVariable(1)->GetValueAsUInt(), szIfName);
      delete pRespPDU;
	}

	return dwResult;
}


//
// Build layer 2 topology for switch
//

DWORD BuildL2Topology(nxObjList &topology, Node *pRoot, Node *pParent, int nDepth, TCHAR *pszParentIfName)
{
	PeerList *pList;
	DWORD i, dwResult = RCC_SNMP_FAILURE;
	TCHAR szPeerIfName[MAX_CONNECTOR_NAME];
	Node *pNode;

	pList = new PeerList;
	if (pRoot->Flags() & NF_IS_NORTEL_TOPO)
	{
		if (pRoot->CallSnmpEnumerate(".1.3.6.1.4.1.45.1.6.13.2.1.1.3", NortelTopoHandler, pList) != SNMP_ERR_SUCCESS)
			goto cleanup;
	}
	else if (pRoot->Flags() & NF_IS_CDP)
	{
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
			else if (nDepth > 0)
			{
				dwResult = BuildL2Topology(topology, pNode, pRoot, nDepth - 1, szPeerIfName);
				if (dwResult != RCC_SUCCESS)
					break;
				topology.AddObject(pNode->Id());
				topology.LinkObjectsEx(pRoot->Id(), pNode->Id(), pList->m_pPeerList[i].szIfName, szPeerIfName);
			}
		}
	}

cleanup:
	delete pList;
	return dwResult;
}
