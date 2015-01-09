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
** File: np.cpp
**
**/

#include "nxcore.h"

/**
 * Externals
 */
extern Queue g_nodePollerQueue;

/**
 * Discovery class
 */
class NXSL_DiscoveryClass : public NXSL_Class
{
public:
   NXSL_DiscoveryClass();

   virtual NXSL_Value *getAttr(NXSL_Object *pObject, const TCHAR *pszAttr);
};

/**
 * Implementation of discovery class
 */
NXSL_DiscoveryClass::NXSL_DiscoveryClass() : NXSL_Class()
{
   _tcscpy(m_name, _T("NewNode"));
}

NXSL_Value *NXSL_DiscoveryClass::getAttr(NXSL_Object *pObject, const TCHAR *pszAttr)
{
   DISCOVERY_FILTER_DATA *pData;
   NXSL_Value *pValue = NULL;
   TCHAR szBuffer[256];

   pData = (DISCOVERY_FILTER_DATA *)pObject->getData();
   if (!_tcscmp(pszAttr, _T("ipAddr")))
   {
      IpToStr(pData->dwIpAddr, szBuffer);
      pValue = new NXSL_Value(szBuffer);
   }
   else if (!_tcscmp(pszAttr, _T("netMask")))
   {
      IpToStr(pData->dwNetMask, szBuffer);
      pValue = new NXSL_Value(szBuffer);
   }
   else if (!_tcscmp(pszAttr, _T("subnet")))
   {
      IpToStr(pData->dwSubnetAddr, szBuffer);
      pValue = new NXSL_Value(szBuffer);
   }
   else if (!_tcscmp(pszAttr, _T("isAgent")))
   {
      pValue = new NXSL_Value((LONG)((pData->dwFlags & NNF_IS_AGENT) ? 1 : 0));
   }
   else if (!_tcscmp(pszAttr, _T("isSNMP")))
   {
      pValue = new NXSL_Value((LONG)((pData->dwFlags & NNF_IS_SNMP) ? 1 : 0));
   }
   else if (!_tcscmp(pszAttr, _T("isBridge")))
   {
      pValue = new NXSL_Value((LONG)((pData->dwFlags & NNF_IS_BRIDGE) ? 1 : 0));
   }
   else if (!_tcscmp(pszAttr, _T("isRouter")))
   {
      pValue = new NXSL_Value((LONG)((pData->dwFlags & NNF_IS_ROUTER) ? 1 : 0));
   }
   else if (!_tcscmp(pszAttr, _T("isPrinter")))
   {
      pValue = new NXSL_Value((LONG)((pData->dwFlags & NNF_IS_PRINTER) ? 1 : 0));
   }
   else if (!_tcscmp(pszAttr, _T("isCDP")))
   {
      pValue = new NXSL_Value((LONG)((pData->dwFlags & NNF_IS_CDP) ? 1 : 0));
   }
   else if (!_tcscmp(pszAttr, _T("isSONMP")))
   {
      pValue = new NXSL_Value((LONG)((pData->dwFlags & NNF_IS_SONMP) ? 1 : 0));
   }
   else if (!_tcscmp(pszAttr, _T("isLLDP")))
   {
      pValue = new NXSL_Value((LONG)((pData->dwFlags & NNF_IS_LLDP) ? 1 : 0));
   }
   else if (!_tcscmp(pszAttr, _T("snmpVersion")))
   {
      pValue = new NXSL_Value((LONG)pData->nSNMPVersion);
   }
   else if (!_tcscmp(pszAttr, _T("snmpOID")))
   {
      pValue = new NXSL_Value(pData->szObjectId);
   }
   else if (!_tcscmp(pszAttr, _T("agentVersion")))
   {
      pValue = new NXSL_Value(pData->szAgentVersion);
   }
   else if (!_tcscmp(pszAttr, _T("platformName")))
   {
      pValue = new NXSL_Value(pData->szPlatform);
   }
   return pValue;
}

/**
 * Discovery class object
 */
static NXSL_DiscoveryClass m_nxslDiscoveryClass;

/**
 * Poll new node for configuration
 * Returns pointer to new node object on success or NULL on failure
 *
 * @param dwIpAddr IP address of new node
 * @param dwNetMask IP network mask or 0 if not known
 * @param dwCreationFlags
 * @param agentPort port number of NetXMS agent
 * @param snmpPort port number of SNMP agent
 * @param pszName name for new node, can be NULL
 * @param dwProxyNode agent proxy node ID or 0 to use direct communications
 * @param dwSNMPProxy SNMP proxy node ID or 0 to use direct communications
 * @param pCluster pointer to parent cluster object or NULL
 * @param zoneId zone ID
 * @param doConfPoll if set to true, Node::configurationPoll will be called before exit
 * @param discoveredNode must be set to true if node being added automatically by discovery thread
 */
Node NXCORE_EXPORTABLE *PollNewNode(UINT32 dwIpAddr, UINT32 dwNetMask, UINT32 dwCreationFlags,
                                    WORD agentPort, WORD snmpPort, const TCHAR *pszName, UINT32 dwProxyNode, UINT32 dwSNMPProxy,
                                    Cluster *pCluster, UINT32 zoneId, bool doConfPoll, bool discoveredNode)
{
   Node *pNode;
   TCHAR szIpAddr1[32], szIpAddr2[32];
   UINT32 dwFlags = 0;

   DbgPrintf(4, _T("PollNode(%s,%s) zone %d"), IpToStr(dwIpAddr, szIpAddr1), IpToStr(dwNetMask, szIpAddr2), (int)zoneId);
   // Check for node existence
   if ((FindNodeByIP(zoneId, dwIpAddr) != NULL) ||
       (FindSubnetByIP(zoneId, dwIpAddr) != NULL))
   {
      DbgPrintf(4, _T("PollNode: Node %s already exist in database"), IpToStr(dwIpAddr, szIpAddr1));
      return NULL;
   }

   if (dwCreationFlags & NXC_NCF_DISABLE_ICMP)
      dwFlags |= NF_DISABLE_ICMP;
   if (dwCreationFlags & NXC_NCF_DISABLE_SNMP)
      dwFlags |= NF_DISABLE_SNMP;
   if (dwCreationFlags & NXC_NCF_DISABLE_NXCP)
      dwFlags |= NF_DISABLE_NXCP;
   pNode = new Node(dwIpAddr, dwFlags, dwProxyNode, dwSNMPProxy, zoneId);
	if (agentPort != 0)
		pNode->setAgentPort(agentPort);
	if (snmpPort != 0)
		pNode->setSnmpPort(snmpPort);
   NetObjInsert(pNode, TRUE);
   if (pszName != NULL)
      pNode->setName(pszName);

	// Use DNS name as primary name if required
	if (discoveredNode && ConfigReadInt(_T("UseDNSNameForDiscoveredNodes"), 0))
	{
		UINT32 ip = htonl(dwIpAddr);
		struct hostent *hs = gethostbyaddr((char *)&ip, 4, AF_INET);
		if (hs != NULL)
		{
			TCHAR dnsName[MAX_DNS_NAME];
#ifdef UNICODE
			MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, hs->h_name, -1, dnsName, MAX_DNS_NAME);
			dnsName[MAX_DNS_NAME - 1] = 0;
#else
			nx_strncpy(dnsName, hs->h_name, MAX_DNS_NAME);
#endif
			if (ntohl(ResolveHostName(dnsName)) == dwIpAddr)
			{
				// We have valid DNS name which resolves back to node's IP address, use it as primary name
				pNode->setPrimaryName(dnsName);
				DbgPrintf(4, _T("PollNode: Using DNS name %s as primary name for node %s"), dnsName, IpToStr(dwIpAddr, szIpAddr1));
			}
		}
	}

	// Bind node to cluster before first configuration poll
	if (pCluster != NULL)
	{
		pCluster->applyToTarget(pNode);
	}

   if (dwCreationFlags & NXC_NCF_CREATE_UNMANAGED)
   {
      pNode->setMgmtStatus(FALSE);
      pNode->checkSubnetBinding(NULL);
   }

   // Add default DCIs
   pNode->addDCObject(new DCItem(CreateUniqueId(IDG_ITEM), _T("Status"), DS_INTERNAL, DCI_DT_INT, 
		ConfigReadInt(_T("DefaultDCIPollingInterval"), 60), 
		ConfigReadInt(_T("DefaultDCIRetentionTime"), 30), pNode));

	if (doConfPoll)
		pNode->configurationPoll(NULL, 0, -1, dwNetMask);

   pNode->unhide();
   PostEvent(EVENT_NODE_ADDED, pNode->getId(), "d", (int)(discoveredNode ? 1 : 0));

   return pNode;
}

/**
 * Find existing node by MAC address to detect IP address change for already known node.
 *
 * @param dwIpAddr new (discovered) IP address
 * @param dwNetMask network mask for new address
 * @param dwZoneID zone ID
 * @param bMacAddr MAC address of discovered node, or NULL if not known
 *
 * @return pointer to existing interface object with given MAC address or NULL if no such interface found
 */
static Interface *GetOldNodeWithNewIP(UINT32 dwIpAddr, UINT32 dwNetMask, UINT32 dwZoneId, BYTE *bMacAddr)
{
	Subnet *subnet;
	BYTE nodeMacAddr[MAC_ADDR_LENGTH];
	TCHAR szIpAddr[16], szMacAddr[20];

	IpToStr(dwIpAddr, szIpAddr);
	MACToStr(bMacAddr, szMacAddr);
	DbgPrintf(6, _T("GetOldNodeWithNewIP: ip=%s mac=%s"), szIpAddr, szMacAddr);

	if (bMacAddr == NULL)
	{
		subnet = FindSubnetForNode(dwZoneId, dwIpAddr);
		if (subnet != NULL)
		{
			BOOL found = subnet->findMacAddress(dwIpAddr, nodeMacAddr);
			if (!found)
			{
				DbgPrintf(6, _T("GetOldNodeWithNewIP: MAC address not found"));
				return NULL;
			}
		}
		else
		{
			DbgPrintf(6, _T("GetOldNodeWithNewIP: subnet not found"));
			return NULL;
		}
	}
	else
	{
		memcpy(nodeMacAddr, bMacAddr, MAC_ADDR_LENGTH);
	}

	Interface *iface = FindInterfaceByMAC(nodeMacAddr);

	if (iface == NULL)
		DbgPrintf(6, _T("GetOldNodeWithNewIP: returning null (FindInterfaceByMAC!)"));
	
	return iface;
}

/**
 * Check if host at given IP address is reachable by NetXMS server
 */
static bool HostIsReachable(UINT32 ipAddr, UINT32 zoneId, bool fullCheck, SNMP_Transport **transport, AgentConnection **agentConn)
{
	bool reachable = false;

	if (transport != NULL)
		*transport = NULL;
	if (agentConn != NULL)
		*agentConn = NULL;

	UINT32 agentProxy = 0;
	UINT32 icmpProxy = 0;
	UINT32 snmpProxy = 0;

	if (IsZoningEnabled() && (zoneId != 0))
	{
		Zone *zone = (Zone *)g_idxZoneByGUID.get(zoneId);
		if (zone != NULL)
		{
			agentProxy = zone->getAgentProxy();
			icmpProxy = zone->getIcmpProxy();
			snmpProxy = zone->getSnmpProxy();
		}
	}

	// *** ICMP PING ***
	if (icmpProxy != 0)
	{
		Node *proxyNode = (Node *)g_idxNodeById.get(icmpProxy);
		if ((proxyNode != NULL) && proxyNode->isNativeAgent() && !proxyNode->isDown())
		{
			AgentConnection *conn = proxyNode->createAgentConnection();
			if (conn != NULL)
			{
				TCHAR parameter[64], buffer[64];

				_sntprintf(parameter, 64, _T("Icmp.Ping(%s)"), IpToStr(ipAddr, buffer));
				if (conn->getParameter(parameter, 64, buffer) == ERR_SUCCESS)
				{
					TCHAR *eptr;
					long value = _tcstol(buffer, &eptr, 10);
					if ((*eptr == 0) && (value >= 0))
					{
						if (value < 10000)
						{
							reachable = true;
						}
					}
				}
				conn->disconnect();
				delete conn;
			}
		}
	}
	else	// not using ICMP proxy
	{
		if (IcmpPing(htonl(ipAddr), 3, g_icmpPingTimeout, NULL, g_icmpPingSize) == ICMP_SUCCESS)
			reachable = true;
	}

	if (reachable && !fullCheck)
		return true;

	// *** NetXMS agent ***
   AgentConnection *pAgentConn = new AgentConnectionEx(0, htonl(ipAddr), AGENT_LISTEN_PORT, AUTH_NONE, _T(""));
	if (agentProxy != 0)
	{
		Node *proxyNode = (Node *)g_idxNodeById.get(agentProxy);
      if (proxyNode != NULL)
      {
         pAgentConn->setProxy(htonl(proxyNode->IpAddr()), proxyNode->getAgentPort(),
                              proxyNode->getAgentAuthMethod(), proxyNode->getSharedSecret());
      }
	}
   UINT32 rcc;
   if (!pAgentConn->connect(g_pServerKey, FALSE, &rcc))
   {
      // If there are authentication problem, try default shared secret
      if ((rcc == ERR_AUTH_REQUIRED) || (rcc == ERR_AUTH_FAILED))
      {
         TCHAR secret[MAX_SECRET_LENGTH];
         ConfigReadStr(_T("AgentDefaultSharedSecret"), secret, MAX_SECRET_LENGTH, _T("netxms"));
         pAgentConn->setAuthData(AUTH_SHA1_HASH, secret);
         pAgentConn->connect(g_pServerKey, FALSE, &rcc);
      }
   }
   if (rcc == ERR_SUCCESS)
   {
		if (agentConn != NULL)
		{
			*agentConn = pAgentConn;
			pAgentConn = NULL;	// prevent deletion
		}
		reachable = true;
   }
	delete pAgentConn;

	if (reachable && !fullCheck)
		return true;

	// *** SNMP ***
	SNMP_Transport *pTransport = NULL;
	if (snmpProxy != 0)
	{
		Node *proxyNode = (Node *)g_idxNodeById.get(snmpProxy);
		if (proxyNode != NULL)
		{
			AgentConnection *pConn;

			pConn = proxyNode->createAgentConnection();
			if (pConn != NULL)
			{
				pTransport = new SNMP_ProxyTransport(pConn, ipAddr, 161);
			}
		}
	}
	else
	{
		pTransport = new SNMP_UDPTransport;
		((SNMP_UDPTransport *)pTransport)->createUDPTransport(NULL, htonl(ipAddr), 161);
	}
   if (pTransport != NULL)
   {
	   int version;
      StringList oids;
      oids.add(_T(".1.3.6.1.2.1.1.2.0"));
      oids.add(_T(".1.3.6.1.2.1.1.1.0"));
      AddDriverSpecificOids(&oids);
      SNMP_SecurityContext *ctx = SnmpCheckCommSettings(pTransport, &version, NULL, &oids);
	   if (ctx != NULL)
	   {
		   delete ctx;
		   if (transport != NULL)
		   {
			   pTransport->setSnmpVersion(version);
			   *transport = pTransport;
			   pTransport = NULL;	// prevent deletion
		   }
		   reachable = true;
	   }
	   delete pTransport;
   }

	return reachable;
}

/**
 * Check if newly discovered node should be added
 */
static BOOL AcceptNewNode(UINT32 dwIpAddr, UINT32 dwNetMask, UINT32 zoneId, BYTE *macAddr)
{
   DISCOVERY_FILTER_DATA data;
   TCHAR szFilter[MAX_DB_STRING], szBuffer[256], szIpAddr[16];
   UINT32 dwTemp;
   AgentConnection *pAgentConn;
   BOOL bResult = FALSE;
	SNMP_Transport *pTransport;

	IpToStr(dwIpAddr, szIpAddr);
   if ((FindNodeByIP(zoneId, dwIpAddr) != NULL) ||
       (FindSubnetByIP(zoneId, dwIpAddr) != NULL))
	{
		DbgPrintf(4, _T("AcceptNewNode(%s): node already exist in database"), szIpAddr);
      return FALSE;  // Node already exist in database
	}

   if (!memcmp(macAddr, "\xFF\xFF\xFF\xFF\xFF\xFF", 6))
   {
		DbgPrintf(4, _T("AcceptNewNode(%s): broadcast MAC address"), szIpAddr);
      return FALSE;  // Broadcast MAC
   }

   NXSL_VM *hook = FindHookScript(_T("AcceptNewNode"));
   if (hook != NULL)
   {
      bool stop = false;
      hook->setGlobalVariable(_T("$ipAddr"), new NXSL_Value(szIpAddr));
      IpToStr(dwNetMask, szBuffer);
      hook->setGlobalVariable(_T("$ipNetMask"), new NXSL_Value(szBuffer));
      MACToStr(macAddr, szBuffer);
      hook->setGlobalVariable(_T("$macAddr"), new NXSL_Value(szBuffer));
      hook->setGlobalVariable(_T("$zoneId"), new NXSL_Value(zoneId));
      if (hook->run())
      {
         NXSL_Value *result = hook->getResult();
         if (result->isZero())
         {
            stop = true;
      		DbgPrintf(4, _T("AcceptNewNode(%s): rejected by hook script"), szIpAddr);
         }
      }
      else
      {
		   DbgPrintf(4, _T("AcceptNewNode(%s): hook script execution error: %s"), szIpAddr, hook->getErrorText());
      }
      delete hook;
      if (stop)
         return FALSE;  // blocked by hook
   }

	Interface *iface = GetOldNodeWithNewIP(dwIpAddr, dwNetMask, zoneId, macAddr);
	if (iface != NULL)
	{
		if (!HostIsReachable(dwIpAddr, zoneId, false, NULL, NULL))
		{
			DbgPrintf(4, _T("AcceptNewNode(%s): found existing interface with same MAC address, but new IP is not reachable"), szIpAddr);
			return FALSE;
		}

		Node *oldNode = iface->getParentNode();
		if (iface->IpAddr() == oldNode->IpAddr())
		{
			// we should change node's primary IP only if old IP for this MAC was also node's primary IP
			TCHAR szOldIpAddr[16];
			IpToStr(oldNode->IpAddr(), szOldIpAddr);
			DbgPrintf(4, _T("AcceptNewNode(%s): node already exist in database with ip %s, will change to new"), 
				szIpAddr, szOldIpAddr);
			oldNode->changeIPAddress(dwIpAddr);
		}
		return FALSE;
	}

   // Allow filtering by loaded modules
   for(UINT32 i = 0; i < g_dwNumModules; i++)
	{
		if (g_pModuleList[i].pfAcceptNewNode != NULL)
		{
			if (!g_pModuleList[i].pfAcceptNewNode(dwIpAddr, dwNetMask, zoneId, macAddr))
				return FALSE;	// filtered out by module
		}
	}

   // Read configuration
   ConfigReadStr(_T("DiscoveryFilter"), szFilter, MAX_DB_STRING, _T(""));
   StrStrip(szFilter);

   // Check for filter script
   if ((szFilter[0] == 0) || (!_tcsicmp(szFilter, _T("none"))))
	{
		if (!HostIsReachable(dwIpAddr, zoneId, false, NULL, NULL))
		{
			DbgPrintf(4, _T("AcceptNewNode(%s): host is not reachable"), szIpAddr);
			return FALSE;
		}
		DbgPrintf(4, _T("AcceptNewNode(%s): no filtering, node accepted"), szIpAddr);
      return TRUE;   // No filtering
	}

   // Initialize new node data
   memset(&data, 0, sizeof(DISCOVERY_FILTER_DATA));
   data.dwIpAddr = dwIpAddr;
   data.dwNetMask = dwNetMask;
   data.dwSubnetAddr = dwIpAddr & dwNetMask;

   // Check for address range if we use simple filter instead of script
	UINT32 autoFilterFlags;
   if (!_tcsicmp(szFilter, _T("auto")))
   {
      autoFilterFlags = ConfigReadULong(_T("DiscoveryFilterFlags"), DFF_ALLOW_AGENT | DFF_ALLOW_SNMP);
		DbgPrintf(4, _T("AcceptNewNode(%s): auto filter, flags=%04X"), szIpAddr, autoFilterFlags);

		if (autoFilterFlags & DFF_ONLY_RANGE)
      {
			DbgPrintf(4, _T("AcceptNewNode(%s): auto filter - checking range"), szIpAddr);
         DB_RESULT hResult = DBSelect(g_hCoreDB, _T("SELECT addr_type,addr1,addr2 FROM address_lists WHERE list_type=2"));
         if (hResult != NULL)
         {
            int nRows = DBGetNumRows(hResult);
            for(int i = 0; (i < nRows) && (!bResult); i++)
            {
               int nType = DBGetFieldLong(hResult, i, 0);
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
			DbgPrintf(4, _T("AcceptNewNode(%s): auto filter - range check result is %d"), szIpAddr, bResult);
			if (!bResult)
				return FALSE;
      }
   }

	// Check if host is reachable
	if (!HostIsReachable(dwIpAddr, zoneId, true, &pTransport, &pAgentConn))
	{
		DbgPrintf(4, _T("AcceptNewNode(%s): host is not reachable"), szIpAddr);
      return FALSE;
	}
	if (pTransport != NULL)
	{
      data.dwFlags |= NNF_IS_SNMP;
		data.nSNMPVersion = pTransport->getSnmpVersion();
	}
   if (pAgentConn != NULL)
   {
      data.dwFlags |= NNF_IS_AGENT;
      pAgentConn->getParameter(_T("Agent.Version"), MAX_AGENT_VERSION_LEN, data.szAgentVersion);
      pAgentConn->getParameter(_T("System.PlatformName"), MAX_PLATFORM_NAME_LEN, data.szPlatform);
   }

   // Check if node is a router
   if (data.dwFlags & NNF_IS_SNMP)
   {
      if (SnmpGet(data.nSNMPVersion, pTransport,
                  _T(".1.3.6.1.2.1.4.1.0"), NULL, 0, &dwTemp, sizeof(UINT32), 0) == SNMP_ERR_SUCCESS)
      {
         if (dwTemp == 1)
            data.dwFlags |= NNF_IS_ROUTER;
      }
   }
   else if (data.dwFlags & NNF_IS_AGENT)
   {
      // Check IP forwarding status
      if (pAgentConn->getParameter(_T("Net.IP.Forwarding"), 16, szBuffer) == ERR_SUCCESS)
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
		        _T(".1.3.6.1.2.1.1.2.0"), NULL, 0, data.szObjectId, MAX_OID_LEN * 4, 0);

      // Check if node is a bridge
      if (SnmpGet(data.nSNMPVersion, pTransport,
                  _T(".1.3.6.1.2.1.17.1.1.0"), NULL, 0, szBuffer, sizeof(szBuffer), 0) == SNMP_ERR_SUCCESS)
      {
         data.dwFlags |= NNF_IS_BRIDGE;
      }

      // Check for CDP (Cisco Discovery Protocol) support
      if (SnmpGet(data.nSNMPVersion, pTransport,
                  _T(".1.3.6.1.4.1.9.9.23.1.3.1.0"), NULL, 0, &dwTemp, sizeof(UINT32), 0) == SNMP_ERR_SUCCESS)
      {
         if (dwTemp == 1)
            data.dwFlags |= NNF_IS_CDP;
      }

      // Check for SONMP (Nortel topology discovery protocol) support
      if (SnmpGet(data.nSNMPVersion, pTransport,
                  _T(".1.3.6.1.4.1.45.1.6.13.1.2.0"), NULL, 0, &dwTemp, sizeof(UINT32), 0) == SNMP_ERR_SUCCESS)
      {
         if (dwTemp == 1)
            data.dwFlags |= NNF_IS_SONMP;
      }

      // Check for LLDP (Link Layer Discovery Protocol) support
      if (SnmpGet(data.nSNMPVersion, pTransport,
                  _T(".1.0.8802.1.1.2.1.3.2.0"), NULL, 0, szBuffer, sizeof(szBuffer), 0) == SNMP_ERR_SUCCESS)
      {
         data.dwFlags |= NNF_IS_LLDP;
      }
   }

   // Cleanup
   delete pAgentConn;
	delete pTransport;

   // Check if we use simple filter instead of script
   if (!_tcsicmp(szFilter, _T("auto")))
   {
		bResult = FALSE;
      if ((autoFilterFlags & (DFF_ALLOW_AGENT | DFF_ALLOW_SNMP)) == 0)
      {
         bResult = TRUE;
      }
      else
      {
         if (autoFilterFlags & DFF_ALLOW_AGENT)
         {
            if (data.dwFlags & NNF_IS_AGENT)
               bResult = TRUE;
         }

         if (autoFilterFlags & DFF_ALLOW_SNMP)
         {
            if (data.dwFlags & NNF_IS_SNMP)
               bResult = TRUE;
         }
      }
		DbgPrintf(4, _T("AcceptNewNode(%s): auto filter - bResult=%d"), szIpAddr, bResult);
   }
   else
   {
      NXSL_VM *vm = g_pScriptLibrary->createVM(szFilter, new NXSL_ServerEnv);
      if (vm != NULL)
      {
         DbgPrintf(4, _T("AcceptNewNode(%s): Running filter script %s"), szIpAddr, szFilter);
         NXSL_Value *pValue = new NXSL_Value(new NXSL_Object(&m_nxslDiscoveryClass, &data));
         if (vm->run(1, &pValue))
         {
            bResult = (vm->getResult()->getValueAsInt32() != 0) ? TRUE : FALSE;
            DbgPrintf(4, _T("AcceptNewNode(%s): Filter script result: %d"), szIpAddr, bResult);
         }
         else
         {
            DbgPrintf(4, _T("AcceptNewNode(%s): Filter script execution error: %s"),
                      szIpAddr, vm->getErrorText());
            PostEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", szFilter, vm->getErrorText(), 0);
         }
         delete vm;
      }
      else
      {
         DbgPrintf(4, _T("AcceptNewNode(%s): Cannot find filter script %s"), szIpAddr, szFilter);
      }
   }

   return bResult;
}

/**
 * Node poller thread (poll new nodes and put them into the database)
 */
THREAD_RESULT THREAD_CALL NodePoller(void *arg)
{
   NEW_NODE *pInfo;
	TCHAR szIpAddr[16], szNetMask[16];

   DbgPrintf(1, _T("Node poller started"));

   while(!IsShutdownInProgress())
   {
      pInfo = (NEW_NODE *)g_nodePollerQueue.GetOrBlock();
      if (pInfo == INVALID_POINTER_VALUE)
         break;   // Shutdown indicator received

		DbgPrintf(4, _T("NodePoller: processing node %s/%s in zone %d"),
		          IpToStr(pInfo->dwIpAddr, szIpAddr), IpToStr(pInfo->dwNetMask, szNetMask), (int)pInfo->zoneId);
      if (pInfo->ignoreFilter || AcceptNewNode(pInfo->dwIpAddr, pInfo->dwNetMask, pInfo->zoneId, pInfo->bMacAddr))
		{
         ObjectTransactionStart();
         PollNewNode(pInfo->dwIpAddr, pInfo->dwNetMask, 0, 0, 0, NULL, 0, 0, NULL, pInfo->zoneId, true, true);
         ObjectTransactionEnd();
		}
      free(pInfo);
   }
   DbgPrintf(1, _T("Node poller thread terminated"));
   return THREAD_OK;
}
