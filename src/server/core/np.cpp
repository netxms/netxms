/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004, 2005, 2006 Victor Kirhenshtein
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
** File: np.cpp
**
**/

#include "nxcore.h"


//
// Externals
//

extern Queue g_nodePollerQueue;


//
// Discovery class
//

class NXSL_DiscoveryClass : public NXSL_Class
{
public:
   NXSL_DiscoveryClass();

   virtual NXSL_Value *getAttr(NXSL_Object *pObject, const TCHAR *pszAttr);
};


//
// Implementation of discovery class
//

NXSL_DiscoveryClass::NXSL_DiscoveryClass()
                     :NXSL_Class()
{
   strcpy(m_szName, "NewNode");
}

NXSL_Value *NXSL_DiscoveryClass::getAttr(NXSL_Object *pObject, const TCHAR *pszAttr)
{
   DISCOVERY_FILTER_DATA *pData;
   NXSL_Value *pValue = NULL;
   char szBuffer[256];

   pData = (DISCOVERY_FILTER_DATA *)pObject->getData();
   if (!strcmp(pszAttr, "ipAddr"))
   {
      IpToStr(pData->dwIpAddr, szBuffer);
      pValue = new NXSL_Value(szBuffer);
   }
   else if (!strcmp(pszAttr, "netMask"))
   {
      IpToStr(pData->dwNetMask, szBuffer);
      pValue = new NXSL_Value(szBuffer);
   }
   else if (!strcmp(pszAttr, "subnet"))
   {
      IpToStr(pData->dwSubnetAddr, szBuffer);
      pValue = new NXSL_Value(szBuffer);
   }
   else if (!strcmp(pszAttr, "isAgent"))
   {
      pValue = new NXSL_Value((LONG)((pData->dwFlags & NNF_IS_AGENT) ? 1 : 0));
   }
   else if (!strcmp(pszAttr, "isSNMP"))
   {
      pValue = new NXSL_Value((LONG)((pData->dwFlags & NNF_IS_SNMP) ? 1 : 0));
   }
   else if (!strcmp(pszAttr, "isBridge"))
   {
      pValue = new NXSL_Value((LONG)((pData->dwFlags & NNF_IS_BRIDGE) ? 1 : 0));
   }
   else if (!strcmp(pszAttr, "isRouter"))
   {
      pValue = new NXSL_Value((LONG)((pData->dwFlags & NNF_IS_ROUTER) ? 1 : 0));
   }
   else if (!strcmp(pszAttr, "isPrinter"))
   {
      pValue = new NXSL_Value((LONG)((pData->dwFlags & NNF_IS_PRINTER) ? 1 : 0));
   }
   else if (!strcmp(pszAttr, "isCDP"))
   {
      pValue = new NXSL_Value((LONG)((pData->dwFlags & NNF_IS_CDP) ? 1 : 0));
   }
   else if (!strcmp(pszAttr, "isSONMP"))
   {
      pValue = new NXSL_Value((LONG)((pData->dwFlags & NNF_IS_SONMP) ? 1 : 0));
   }
   else if (!strcmp(pszAttr, "isLLDP"))
   {
      pValue = new NXSL_Value((LONG)((pData->dwFlags & NNF_IS_LLDP) ? 1 : 0));
   }
   else if (!strcmp(pszAttr, "snmpVersion"))
   {
      pValue = new NXSL_Value((LONG)pData->nSNMPVersion);
   }
   else if (!strcmp(pszAttr, "snmpOID"))
   {
      pValue = new NXSL_Value(pData->szObjectId);
   }
   else if (!strcmp(pszAttr, "agentVersion"))
   {
      pValue = new NXSL_Value(pData->szAgentVersion);
   }
   else if (!strcmp(pszAttr, "platformName"))
   {
      pValue = new NXSL_Value(pData->szPlatform);
   }
   return pValue;
}


//
// Discovery class object
//

static NXSL_DiscoveryClass m_nxslDiscoveryClass;


//
// Poll new node for configuration
// Returns pointer to new node object on success or NULL on failure
//

Node *PollNewNode(DWORD dwIpAddr, DWORD dwNetMask, DWORD dwCreationFlags,
                  TCHAR *pszName, DWORD dwProxyNode, DWORD dwSNMPProxy,
                  Cluster *pCluster)
{
   Node *pNode;
   char szIpAddr1[32], szIpAddr2[32];
   DWORD dwFlags = 0;

   DbgPrintf(4, "PollNode(%s,%s)",  
             IpToStr(dwIpAddr, szIpAddr1), IpToStr(dwNetMask, szIpAddr2));
   // Check for node existence
   if ((FindNodeByIP(dwIpAddr) != NULL) ||
       (FindSubnetByIP(dwIpAddr) != NULL))
   {
      DbgPrintf(4, "PollNode: Node %s already exist in database", 
                IpToStr(dwIpAddr, szIpAddr1));
      return NULL;
   }

   if (dwCreationFlags & NXC_NCF_DISABLE_ICMP)
      dwFlags |= NF_DISABLE_ICMP;
   if (dwCreationFlags & NXC_NCF_DISABLE_SNMP)
      dwFlags |= NF_DISABLE_SNMP;
   if (dwCreationFlags & NXC_NCF_DISABLE_NXCP)
      dwFlags |= NF_DISABLE_NXCP;
   pNode = new Node(dwIpAddr, dwFlags, dwProxyNode, dwSNMPProxy, 0);
   NetObjInsert(pNode, TRUE);
   if (pszName != NULL)
      pNode->SetName(pszName);

	// Bind node to cluster before first configuration poll
	if (pCluster != NULL)
	{
		pCluster->ApplyToNode(pNode);
	}

   if (dwCreationFlags & NXC_NCF_CREATE_UNMANAGED)
   {
      pNode->SetMgmtStatus(FALSE);
   }
   pNode->Unhide();
   PostEvent(EVENT_NODE_ADDED, pNode->Id(), NULL);

   // Add default DCIs
   pNode->addItem(new DCItem(CreateUniqueId(IDG_ITEM), _T("Status"), DS_INTERNAL, DCI_DT_INT, 60, 30, pNode));
   return pNode;
}


//
// Check if newly discovered node should be added
//

static BOOL AcceptNewNode(DWORD dwIpAddr, DWORD dwNetMask)
{
   DISCOVERY_FILTER_DATA data;
   TCHAR szFilter[MAX_DB_STRING], szBuffer[256], szIpAddr[16];
   DWORD dwTemp;
   AgentConnection *pAgentConn;
   NXSL_Program *pScript;
   NXSL_Value *pValue;
   BOOL bResult = FALSE;
	SNMP_UDPTransport *pTransport;

	IpToStr(dwIpAddr, szIpAddr);
   if ((FindNodeByIP(dwIpAddr) != NULL) ||
       (FindSubnetByIP(dwIpAddr) != NULL))
	{
		DbgPrintf(4, "AcceptNewNode(%s): node already exist in database", szIpAddr);
      return FALSE;  // Node already exist in database
	}

   // Read configuration
   ConfigReadStr("DiscoveryFilter", szFilter, MAX_DB_STRING, "");
   StrStrip(szFilter);

   // Run filter script
   if ((szFilter[0] == 0) || (!_tcsicmp(szFilter, "none")))
	{
		DbgPrintf(4, "AcceptNewNode(%s): no filtering, node accepted", szIpAddr);
      return TRUE;   // No filtering
	}

   // Initialize new node data
   memset(&data, 0, sizeof(DISCOVERY_FILTER_DATA));
   data.dwIpAddr = dwIpAddr;
   data.dwNetMask = dwNetMask;
   data.dwSubnetAddr = dwIpAddr & dwNetMask;

   // Check SNMP support
	DbgPrintf(4, "AcceptNewNode(%s): checking SNMP support", szIpAddr);
	pTransport = new SNMP_UDPTransport;
	pTransport->createUDPTransport(NULL, htonl(dwIpAddr), 161);
	SNMP_SecurityContext *ctx = SnmpCheckCommSettings(pTransport, &data.nSNMPVersion, NULL);
	if (ctx != NULL)
	{
      data.dwFlags |= NNF_IS_SNMP;
		delete ctx;
	}

   // Check NetXMS agent support
	DbgPrintf(4, "AcceptNewNode(%s): checking NetXMS agent", szIpAddr);
   pAgentConn = new AgentConnection(htonl(dwIpAddr), AGENT_LISTEN_PORT,
                                    AUTH_NONE, "");
   if (pAgentConn->connect(g_pServerKey))
   {
      data.dwFlags |= NNF_IS_AGENT;
      pAgentConn->GetParameter("Agent.Version", MAX_AGENT_VERSION_LEN, data.szAgentVersion);
      pAgentConn->GetParameter("System.PlatformName", MAX_PLATFORM_NAME_LEN, data.szPlatform);
   }

   // Check if node is a router
   if (data.dwFlags & NNF_IS_SNMP)
   {
      if (SnmpGet(data.nSNMPVersion, pTransport,
                  ".1.3.6.1.2.1.4.1.0", NULL, 0, &dwTemp, sizeof(DWORD),
                  FALSE, FALSE) == SNMP_ERR_SUCCESS)
      {
         if (dwTemp == 1)
            data.dwFlags |= NNF_IS_ROUTER;
      }
   }
   else if (data.dwFlags & NNF_IS_AGENT)
   {
      // Check IP forwarding status
      if (pAgentConn->GetParameter("Net.IP.Forwarding", 16, szBuffer) == ERR_SUCCESS)
      {
         if (_tcstoul(szBuffer, NULL, 10) != 0)
            data.dwFlags |= NNF_IS_ROUTER;
      }
   }

   // Check various SNMP device capabilities
   if (data.dwFlags & NNF_IS_SNMP)
   {
		// Get SNMP OID
		SnmpGet(data.nSNMPVersion, pTransport,
		        ".1.3.6.1.2.1.1.2.0", NULL, 0, data.szObjectId, MAX_OID_LEN * 4,
		        FALSE, FALSE);

      // Check if node is a bridge
      if (SnmpGet(data.nSNMPVersion, pTransport,
                  ".1.3.6.1.2.1.17.1.1.0", NULL, 0, szBuffer, 256,
                  FALSE, FALSE) == SNMP_ERR_SUCCESS)
      {
         data.dwFlags |= NNF_IS_BRIDGE;
      }

      // Check for CDP (Cisco Discovery Protocol) support
      if (SnmpGet(data.nSNMPVersion, pTransport,
                  ".1.3.6.1.4.1.9.9.23.1.3.1.0", NULL, 0, &dwTemp, sizeof(DWORD),
                  FALSE, FALSE) == SNMP_ERR_SUCCESS)
      {
         if (dwTemp == 1)
            data.dwFlags |= NNF_IS_CDP;
      }

      // Check for SONMP (Nortel topology discovery protocol) support
      if (SnmpGet(data.nSNMPVersion, pTransport,
                  ".1.3.6.1.4.1.45.1.6.13.1.2.0", NULL, 0, &dwTemp, sizeof(DWORD),
                  FALSE, FALSE) == SNMP_ERR_SUCCESS)
      {
         if (dwTemp == 1)
            data.dwFlags |= NNF_IS_SONMP;
      }

      // Check for LLDP (Link Layer Discovery Protocol) support
      if (SnmpGet(data.nSNMPVersion, pTransport,
                  ".1.0.8802.1.1.2.1.3.2.0", NULL, 0, szBuffer, 256,
                  FALSE, FALSE) == SNMP_ERR_SUCCESS)
      {
         data.dwFlags |= NNF_IS_LLDP;
      }
   }

   // Cleanup
   delete pAgentConn;
	delete pTransport;

   // Check if we use simple filter instead of script
   if (!_tcsicmp(szFilter, "auto"))
   {
      DWORD dwFlags;
      DB_RESULT hResult;
      int i, nRows, nType;

      dwFlags = ConfigReadULong(_T("DiscoveryFilterFlags"), DFF_ALLOW_AGENT | DFF_ALLOW_SNMP);
		DbgPrintf(4, "AcceptNewNode(%s): auto filter, dwFlags=%04X", szIpAddr, dwFlags);

      if ((dwFlags & (DFF_ALLOW_AGENT | DFF_ALLOW_SNMP)) == 0)
      {
         bResult = TRUE;
      }
      else
      {
         if (dwFlags & DFF_ALLOW_AGENT)
         {
            if (data.dwFlags & NNF_IS_AGENT)
               bResult = TRUE;
         }

         if (dwFlags & DFF_ALLOW_SNMP)
         {
            if (data.dwFlags & NNF_IS_SNMP)
               bResult = TRUE;
         }
      }

      // Check range
      if ((dwFlags & DFF_ONLY_RANGE) && bResult)
      {
			DbgPrintf(4, "AcceptNewNode(%s): auto filter - checking range", szIpAddr);
         hResult = DBSelect(g_hCoreDB, _T("SELECT addr_type,addr1,addr2 FROM address_lists WHERE list_type=2"));
         if (hResult != NULL)
         {
            nRows = DBGetNumRows(hResult);
            for(i = 0, bResult = FALSE; (i < nRows) && (!bResult); i++)
            {
               nType = DBGetFieldLong(hResult, i, 0);
               if (nType == 0)
               {
                  // Subnet
                  bResult = (data.dwIpAddr & DBGetFieldIPAddr(hResult, i, 2)) == DBGetFieldIPAddr(hResult, i, 1);
               }
               else
               {
                  // Range
                  bResult = ((data.dwIpAddr >= DBGetFieldIPAddr(hResult, i, 1)) &&
                             (data.dwIpAddr <= DBGetFieldIPAddr(hResult, i, 2)));
               }
            }
            DBFreeResult(hResult);
         }
      }
		DbgPrintf(4, "AcceptNewNode(%s): auto filter - bResult=%d", szIpAddr, bResult);
   }
   else
   {
      g_pScriptLibrary->lock();
      pScript = g_pScriptLibrary->findScript(szFilter);
      if (pScript != NULL)
      {
         DbgPrintf(4, "AcceptNewNode(%s): Running filter script %s", szIpAddr, szFilter);
         pValue = new NXSL_Value(new NXSL_Object(&m_nxslDiscoveryClass, &data));
         if (pScript->run(new NXSL_ServerEnv, 1, &pValue) == 0)
         {
            bResult = (pScript->getResult()->getValueAsInt32() != 0) ? TRUE : FALSE;
            DbgPrintf(4, "AcceptNewNode(%s): Filter script result: %d", szIpAddr, bResult);
         }
         else
         {
            DbgPrintf(4, "AcceptNewNode(%s): Filter script execution error: %s",
                      szIpAddr, pScript->getErrorText());
            PostEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, _T("ssd"), szFilter,
                      pScript->getErrorText(), 0);
         }
      }
      else
      {
         DbgPrintf(4, "AcceptNewNode(%s): Cannot find filter script %s", szIpAddr, szFilter);
      }
      g_pScriptLibrary->unlock();
   }

   return bResult;
}


//
// Node poller thread (poll new nodes and put them into the database)
//

THREAD_RESULT THREAD_CALL NodePoller(void *arg)
{
   NEW_NODE *pInfo;
	TCHAR szIpAddr[16], szNetMask[16];

   DbgPrintf(1, "Node poller started");

   while(!ShutdownInProgress())
   {
      pInfo = (NEW_NODE *)g_nodePollerQueue.GetOrBlock();
      if (pInfo == INVALID_POINTER_VALUE)
         break;   // Shutdown indicator received

		DbgPrintf(4, "NodePoller: processing node %s/%s",
		          IpToStr(pInfo->dwIpAddr, szIpAddr),
		          IpToStr(pInfo->dwNetMask, szNetMask));
      if (AcceptNewNode(pInfo->dwIpAddr, pInfo->dwNetMask))
		{
         Node *node = PollNewNode(pInfo->dwIpAddr, pInfo->dwNetMask, 0, NULL, 0, 0, NULL);
			if (node != NULL)
			{
				// We should do configuration poll before taking new node from the queue
				// to prevent possible node duplication
				node->configurationPoll(NULL, 0, -1, pInfo->dwNetMask);
			}
		}
      free(pInfo);
   }
   DbgPrintf(1, "Node poller thread terminated");
   return THREAD_OK;
}
