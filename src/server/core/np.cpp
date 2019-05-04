/*
** NetXMS - Network Management System
** Copyright (C) 2003-2019 Victor Kirhenshtein
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

#define DEBUG_TAG _T("obj.poll.node")

/**
 * Externals
 */
extern ObjectQueue<DiscoveredAddress> g_nodePollerQueue;

/**
 * Thread pool
 */
ThreadPool *g_discoveryThreadPool = NULL;

/**
 * IP addresses being processed by node poller
 */
static ObjectArray<DiscoveredAddress> s_processingList(64, 64, false);
static Mutex s_processingListLock;

/**
 * Check if given address is in processing by new node poller
 */
bool IsNodePollerActiveAddress(const InetAddress& addr)
{
   bool result = false;
   s_processingListLock.lock();
   for(int i = 0; i < s_processingList.size(); i++)
   {
      if (s_processingList.get(i)->ipAddr.equals(addr))
      {
         result = true;
         break;
      }
   }
   s_processingListLock.unlock();
   return result;
}

/**
 * Constructor for NewNodeData
 */
NewNodeData::NewNodeData(const InetAddress& ipAddr)
{
   this->ipAddr = ipAddr;
   creationFlags = 0;
   agentPort = AGENT_LISTEN_PORT;
   snmpPort = SNMP_DEFAULT_PORT;
   name[0] = 0;
   agentProxyId = 0;
   snmpProxyId = 0;
   icmpProxyId = 0;
   sshProxyId = 0;
   sshLogin[0] = 0;
   sshPassword[0] = 0;
   cluster = NULL;
   zoneUIN = 0;
   doConfPoll = false;
   origin = NODE_ORIGIN_MANUAL;
   snmpSecurity = NULL;
}

/**
 * Create NewNodeData from NXCPMessage
 */
NewNodeData::NewNodeData(const NXCPMessage *msg, const InetAddress& ipAddr)
{
   this->ipAddr = ipAddr;
   this->ipAddr.setMaskBits(msg->getFieldAsInt32(VID_IP_NETMASK));
   creationFlags = msg->getFieldAsUInt32(VID_CREATION_FLAGS);
   agentPort = msg->getFieldAsUInt16(VID_AGENT_PORT);
   snmpPort = msg->getFieldAsUInt16(VID_SNMP_PORT);
   msg->getFieldAsString(VID_OBJECT_NAME, name, MAX_OBJECT_NAME);
   agentProxyId = msg->getFieldAsUInt32(VID_AGENT_PROXY);
   snmpProxyId = msg->getFieldAsUInt32(VID_SNMP_PROXY);
   icmpProxyId = msg->getFieldAsUInt32(VID_ICMP_PROXY);
   sshProxyId = msg->getFieldAsUInt32(VID_SSH_PROXY);
   msg->getFieldAsString(VID_SSH_LOGIN, sshLogin, MAX_SSH_LOGIN_LEN);
   msg->getFieldAsString(VID_SSH_PASSWORD, sshPassword, MAX_SSH_PASSWORD_LEN);
   cluster = NULL;
   zoneUIN = msg->getFieldAsUInt32(VID_ZONE_UIN);
   doConfPoll = false;
   origin = NODE_ORIGIN_MANUAL;
   snmpSecurity = NULL;
}

/**
 * Destructor for NewNodeData
 */
NewNodeData::~NewNodeData()
{
   delete(snmpSecurity);
}

/**
 * Discovery class
 */
class NXSL_DiscoveryClass : public NXSL_Class
{
public:
   NXSL_DiscoveryClass();

   virtual NXSL_Value *getAttr(NXSL_Object *pObject, const char *pszAttr);
};

/**
 * Implementation of discovery class
 */
NXSL_DiscoveryClass::NXSL_DiscoveryClass() : NXSL_Class()
{
   setName(_T("NewNode"));
}

NXSL_Value *NXSL_DiscoveryClass::getAttr(NXSL_Object *object, const char *pszAttr)
{
   DISCOVERY_FILTER_DATA *pData;
   NXSL_Value *pValue = NULL;
   TCHAR szBuffer[256];

   NXSL_VM *vm = object->vm();
   pData = (DISCOVERY_FILTER_DATA *)object->getData();
   if (!strcmp(pszAttr, "ipAddr"))
   {
      pValue = vm->createValue(pData->ipAddr.toString(szBuffer));
   }
   else if (!strcmp(pszAttr, "netMask"))
   {
      pValue = vm->createValue(pData->ipAddr.getMaskBits());
   }
   else if (!strcmp(pszAttr, "subnet"))
   {
      pValue = vm->createValue(pData->ipAddr.getSubnetAddress().toString(szBuffer));
   }
   else if (!strcmp(pszAttr, "isAgent"))
   {
      pValue = vm->createValue((LONG)((pData->dwFlags & NNF_IS_AGENT) ? 1 : 0));
   }
   else if (!strcmp(pszAttr, "isSNMP"))
   {
      pValue = vm->createValue((LONG)((pData->dwFlags & NNF_IS_SNMP) ? 1 : 0));
   }
   else if (!strcmp(pszAttr, "isBridge"))
   {
      pValue = vm->createValue((LONG)((pData->dwFlags & NNF_IS_BRIDGE) ? 1 : 0));
   }
   else if (!strcmp(pszAttr, "isRouter"))
   {
      pValue = vm->createValue((LONG)((pData->dwFlags & NNF_IS_ROUTER) ? 1 : 0));
   }
   else if (!strcmp(pszAttr, "isPrinter"))
   {
      pValue = vm->createValue((LONG)((pData->dwFlags & NNF_IS_PRINTER) ? 1 : 0));
   }
   else if (!strcmp(pszAttr, "isCDP"))
   {
      pValue = vm->createValue((LONG)((pData->dwFlags & NNF_IS_CDP) ? 1 : 0));
   }
   else if (!strcmp(pszAttr, "isSONMP"))
   {
      pValue = vm->createValue((LONG)((pData->dwFlags & NNF_IS_SONMP) ? 1 : 0));
   }
   else if (!strcmp(pszAttr, "isLLDP"))
   {
      pValue = vm->createValue((LONG)((pData->dwFlags & NNF_IS_LLDP) ? 1 : 0));
   }
   else if (!strcmp(pszAttr, "snmpVersion"))
   {
      pValue = vm->createValue((LONG)pData->nSNMPVersion);
   }
   else if (!strcmp(pszAttr, "snmpOID"))
   {
      pValue = vm->createValue(pData->szObjectId);
   }
   else if (!strcmp(pszAttr, "agentVersion"))
   {
      pValue = vm->createValue(pData->szAgentVersion);
   }
   else if (!strcmp(pszAttr, "platformName"))
   {
      pValue = vm->createValue(pData->szPlatform);
   }
   else if (!strcmp(pszAttr, "zone"))
   {
      Zone *zone = FindZoneByUIN(pData->zoneUIN);
      pValue = (zone != NULL) ? zone->createNXSLObject(vm) : vm->createValue();
   }
   else if (!strcmp(pszAttr, "zoneUIN"))
   {
      pValue = vm->createValue(pData->zoneUIN);
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
 * @param newNodeData data of new node
 */
Node NXCORE_EXPORTABLE *PollNewNode(NewNodeData *newNodeData)
{
   TCHAR ipAddrText[64];
   nxlog_debug_tag(DEBUG_TAG, 4, _T("PollNode(%s/%d) zone %d"), newNodeData->ipAddr.toString(ipAddrText),
                                               newNodeData->ipAddr.getMaskBits(),
                                               (int)newNodeData->zoneUIN);

   // Check for node existence
   if ((FindNodeByIP(newNodeData->zoneUIN, newNodeData->ipAddr) != NULL) ||
       (FindSubnetByIP(newNodeData->zoneUIN, newNodeData->ipAddr) != NULL))
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("PollNode: Node %s already exist in database"), ipAddrText);
      return NULL;
   }

   UINT32 flags = 0;
   if (newNodeData->creationFlags & NXC_NCF_DISABLE_ICMP)
      flags |= NF_DISABLE_ICMP;
   if (newNodeData->creationFlags & NXC_NCF_DISABLE_SNMP)
      flags |= NF_DISABLE_SNMP;
   if (newNodeData->creationFlags & NXC_NCF_DISABLE_NXCP)
      flags |= NF_DISABLE_NXCP;
   Node *node = new Node(newNodeData, flags);
   NetObjInsert(node, true, false);

   if (newNodeData->creationFlags & NXC_NCF_ENTER_MAINTENANCE)
      node->enterMaintenanceMode(_T("Automatic maintenance for new node"));

	// Use DNS name as primary name if required
	if ((newNodeData->origin == NODE_ORIGIN_NETWORK_DISCOVERY) && ConfigReadBoolean(_T("UseDNSNameForDiscoveredNodes"), false))
	{
      TCHAR dnsName[MAX_DNS_NAME];
      bool addressResolved = false;
	   if (IsZoningEnabled() && (newNodeData->zoneUIN != 0))
	   {
	      Zone *zone = FindZoneByUIN(newNodeData->zoneUIN);
	      if (zone != NULL)
	      {
            AgentConnectionEx *conn = zone->acquireConnectionToProxy();
            if (conn != NULL)
            {
               addressResolved = (conn->getHostByAddr(newNodeData->ipAddr, dnsName, MAX_DNS_NAME) != NULL);
               conn->decRefCount();
            }
	      }
	   }
	   else
		{
	      addressResolved = (newNodeData->ipAddr.getHostByAddr(dnsName, MAX_DNS_NAME) != NULL);
		}

      if (addressResolved && ResolveHostName(newNodeData->zoneUIN, dnsName).equals(newNodeData->ipAddr))
      {
         // We have valid DNS name which resolves back to node's IP address, use it as primary name
         node->setPrimaryName(dnsName);
         nxlog_debug_tag(DEBUG_TAG, 4, _T("PollNode: Using DNS name %s as primary name for node %s"), dnsName, ipAddrText);
      }
	}

	// Bind node to cluster before first configuration poll
	if (newNodeData->cluster != NULL)
	{
	   newNodeData->cluster->applyToTarget(node);
	}

   if (newNodeData->creationFlags & NXC_NCF_CREATE_UNMANAGED)
   {
      node->setMgmtStatus(FALSE);
      node->checkSubnetBinding();
   }

	if (newNodeData->doConfPoll)
   {
	   node->configurationPollWorkerEntry(RegisterPoller(POLLER_TYPE_CONFIGURATION, node));
   }

   node->unhide();
   PostEvent(EVENT_NODE_ADDED, node->getId(), "d", static_cast<int>(newNodeData->origin));

   return node;
}

/**
 * Find existing node by MAC address to detect IP address change for already known node.
 *
 * @param ipAddr new (discovered) IP address
 * @param zoneUIN zone ID
 * @param bMacAddr MAC address of discovered node, or NULL if not known
 *
 * @return pointer to existing interface object with given MAC address or NULL if no such interface found
 */
static Interface *GetOldNodeWithNewIP(const InetAddress& ipAddr, UINT32 zoneUIN, BYTE *bMacAddr)
{
	Subnet *subnet;
	BYTE nodeMacAddr[MAC_ADDR_LENGTH];
	TCHAR szIpAddr[64], szMacAddr[20];

   ipAddr.toString(szIpAddr);
	MACToStr(bMacAddr, szMacAddr);
	DbgPrintf(6, _T("GetOldNodeWithNewIP: ip=%s mac=%s"), szIpAddr, szMacAddr);

	if (bMacAddr == NULL)
	{
		subnet = FindSubnetForNode(zoneUIN, ipAddr);
		if (subnet != NULL)
		{
			BOOL found = subnet->findMacAddress(ipAddr, nodeMacAddr);
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
static bool HostIsReachable(const InetAddress& ipAddr, UINT32 zoneUIN, bool fullCheck, SNMP_Transport **transport, AgentConnection **agentConn)
{
	bool reachable = false;

	if (transport != NULL)
		*transport = NULL;
	if (agentConn != NULL)
		*agentConn = NULL;

	UINT32 zoneProxy = 0;

	if (IsZoningEnabled() && (zoneUIN != 0))
	{
		Zone *zone = FindZoneByUIN(zoneUIN);
		if (zone != NULL)
		{
			zoneProxy = zone->getProxyNodeId(NULL);
		}
	}

	// *** ICMP PING ***
	if (zoneProxy != 0)
	{
		Node *proxyNode = (Node *)g_idxNodeById.get(zoneProxy);
		if ((proxyNode != NULL) && proxyNode->isNativeAgent() && !proxyNode->isDown())
		{
			AgentConnection *conn = proxyNode->createAgentConnection();
			if (conn != NULL)
			{
				TCHAR parameter[128], buffer[64];

				_sntprintf(parameter, 128, _T("Icmp.Ping(%s)"), ipAddr.toString(buffer));
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
				conn->decRefCount();
			}
		}
	}
	else	// not using ICMP proxy
	{
		if (IcmpPing(ipAddr, 3, g_icmpPingTimeout, NULL, g_icmpPingSize, false) == ICMP_SUCCESS)
			reachable = true;
	}

	if (reachable && !fullCheck)
		return true;

	// *** NetXMS agent ***
   AgentConnection *pAgentConn = new AgentConnectionEx(0, ipAddr, AGENT_LISTEN_PORT, AUTH_NONE, _T(""));
	if (zoneProxy != 0)
	{
		Node *proxyNode = (Node *)g_idxNodeById.get(zoneProxy);
      if (proxyNode != NULL)
      {
         pAgentConn->setProxy(proxyNode->getIpAddress(), proxyNode->getAgentPort(),
                              proxyNode->getAgentAuthMethod(), proxyNode->getSharedSecret());
      }
	}
	pAgentConn->setCommandTimeout(g_agentCommandTimeout);
   UINT32 rcc;
   if (!pAgentConn->connect(g_pServerKey, &rcc))
   {
      // If there are authentication problem, try default shared secret
      if ((rcc == ERR_AUTH_REQUIRED) || (rcc == ERR_AUTH_FAILED))
      {
         TCHAR secret[MAX_SECRET_LENGTH];
         ConfigReadStr(_T("AgentDefaultSharedSecret"), secret, MAX_SECRET_LENGTH, _T("netxms"));
         DecryptPassword(_T("netxms"), secret, secret, MAX_SECRET_LENGTH);

         pAgentConn->setAuthData(AUTH_SHA1_HASH, secret);
         pAgentConn->connect(g_pServerKey, &rcc);
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
   if (pAgentConn != NULL)
      pAgentConn->decRefCount();

	if (reachable && !fullCheck)
		return true;

	// *** SNMP ***
   INT16 version;
   StringList oids;
   oids.add(_T(".1.3.6.1.2.1.1.2.0"));
   oids.add(_T(".1.3.6.1.2.1.1.1.0"));
   AddDriverSpecificOids(&oids);
   SNMP_Transport *pTransport = SnmpCheckCommSettings(zoneProxy, ipAddr, &version, 0, NULL, &oids, zoneUIN);
   if (pTransport != NULL)
   {
      if (transport != NULL)
      {
         pTransport->setSnmpVersion(version);
         *transport = pTransport;
         pTransport = NULL;	// prevent deletion
      }
      reachable = true;
	   delete pTransport;
   }

	return reachable;
}

/**
 * Check if newly discovered node should be added
 */
static BOOL AcceptNewNode(NewNodeData *newNodeData, BYTE *macAddr)
{
   DISCOVERY_FILTER_DATA data;
   TCHAR szFilter[MAX_CONFIG_VALUE], szBuffer[256], szIpAddr[64];
   UINT32 dwTemp;
   AgentConnection *pAgentConn;
   BOOL bResult = FALSE;
	SNMP_Transport *pTransport;

	newNodeData->ipAddr.toString(szIpAddr);
   if ((FindNodeByIP(newNodeData->zoneUIN, newNodeData->ipAddr) != NULL) ||
       (FindSubnetByIP(newNodeData->zoneUIN, newNodeData->ipAddr) != NULL))
	{
      nxlog_debug_tag(DEBUG_TAG, 4, _T("AcceptNewNode(%s): node already exist in database"), szIpAddr);
      return FALSE;  // Node already exist in database
	}

   if (!memcmp(macAddr, "\xFF\xFF\xFF\xFF\xFF\xFF", 6))
   {
		DbgPrintf(4, _T("AcceptNewNode(%s): broadcast MAC address"), szIpAddr);
      return FALSE;  // Broadcast MAC
   }

   NXSL_VM *hook = FindHookScript(_T("AcceptNewNode"), NULL);
   if (hook != NULL)
   {
      bool stop = false;
      hook->setGlobalVariable("$ipAddr", hook->createValue(szIpAddr));
      hook->setGlobalVariable("$ipNetMask", hook->createValue(newNodeData->ipAddr.getMaskBits()));
      MACToStr(macAddr, szBuffer);
      hook->setGlobalVariable("$macAddr", hook->createValue(szBuffer));
      hook->setGlobalVariable("$zoneUIN", hook->createValue(newNodeData->zoneUIN));
      if (hook->run())
      {
         NXSL_Value *result = hook->getResult();
         if (result->isZero())
         {
            stop = true;
            nxlog_debug_tag(DEBUG_TAG, 4, _T("AcceptNewNode(%s): rejected by hook script"), szIpAddr);
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("AcceptNewNode(%s): hook script execution error: %s"), szIpAddr, hook->getErrorText());
      }
      delete hook;
      if (stop)
         return FALSE;  // blocked by hook
   }

	Interface *iface = GetOldNodeWithNewIP(newNodeData->ipAddr, newNodeData->zoneUIN, macAddr);
	if (iface != NULL)
	{
		if (!HostIsReachable(newNodeData->ipAddr, newNodeData->zoneUIN, false, NULL, NULL))
		{
		   nxlog_debug_tag(DEBUG_TAG, 4, _T("AcceptNewNode(%s): found existing interface with same MAC address, but new IP is not reachable"), szIpAddr);
			return FALSE;
		}

		Node *oldNode = iface->getParentNode();
      if (iface->getIpAddressList()->hasAddress(oldNode->getIpAddress()))
		{
			// we should change node's primary IP only if old IP for this MAC was also node's primary IP
			TCHAR szOldIpAddr[16];
			oldNode->getIpAddress().toString(szOldIpAddr);
			nxlog_debug_tag(DEBUG_TAG, 4, _T("AcceptNewNode(%s): node already exist in database with ip %s, will change to new"), szIpAddr, szOldIpAddr);
			oldNode->changeIPAddress(newNodeData->ipAddr);
		}
		return FALSE;
	}

   // Allow filtering by loaded modules
   for(UINT32 i = 0; i < g_dwNumModules; i++)
	{
		if (g_pModuleList[i].pfAcceptNewNode != NULL)
		{
			if (!g_pModuleList[i].pfAcceptNewNode(newNodeData->ipAddr, newNodeData->zoneUIN, macAddr))
				return FALSE;	// filtered out by module
		}
	}

   // Read configuration
   ConfigReadStr(_T("DiscoveryFilter"), szFilter, MAX_CONFIG_VALUE, _T(""));
   StrStrip(szFilter);

   // Check for filter script
   if ((szFilter[0] == 0) || (!_tcsicmp(szFilter, _T("none"))))
	{
		if (!HostIsReachable(newNodeData->ipAddr, newNodeData->zoneUIN, false, NULL, NULL))
		{
		   nxlog_debug_tag(DEBUG_TAG, 4, _T("AcceptNewNode(%s): host is not reachable"), szIpAddr);
			return FALSE;
		}
		nxlog_debug_tag(DEBUG_TAG, 4, _T("AcceptNewNode(%s): no filtering, node accepted"), szIpAddr);
      return TRUE;   // No filtering
	}

   // Initialize new node data
   memset(&data, 0, sizeof(DISCOVERY_FILTER_DATA));
   data.ipAddr = newNodeData->ipAddr;
   data.zoneUIN = newNodeData->zoneUIN;

   // Check for address range if we use simple filter instead of script
	UINT32 autoFilterFlags;
   if (!_tcsicmp(szFilter, _T("auto")))
   {
      autoFilterFlags = ConfigReadULong(_T("DiscoveryFilterFlags"), DFF_ALLOW_AGENT | DFF_ALLOW_SNMP);
      nxlog_debug_tag(DEBUG_TAG, 4, _T("AcceptNewNode(%s): auto filter, flags=%04X"), szIpAddr, autoFilterFlags);

		if (autoFilterFlags & DFF_ONLY_RANGE)
      {
		   nxlog_debug_tag(DEBUG_TAG, 4, _T("AcceptNewNode(%s): auto filter - checking range"), szIpAddr);
	      ObjectArray<InetAddressListElement> *list = LoadServerAddressList(2);
	      if (list != NULL)
         {
            for(int i = 0; (i < list->size()) && (!bResult); i++)
            {
               bResult = list->get(i)->contains(data.ipAddr);
            }
            delete list;
         }
         nxlog_debug_tag(DEBUG_TAG, 4, _T("AcceptNewNode(%s): auto filter - range check result is %d"), szIpAddr, bResult);
			if (!bResult)
				return FALSE;
      }
   }

	// Check if host is reachable
	if (!HostIsReachable(newNodeData->ipAddr, newNodeData->zoneUIN, true, &pTransport, &pAgentConn))
	{
	   nxlog_debug_tag(DEBUG_TAG, 4, _T("AcceptNewNode(%s): host is not reachable"), szIpAddr);
      return FALSE;
	}
	if (pTransport != NULL)
	{
      data.dwFlags |= NNF_IS_SNMP;
		data.nSNMPVersion = pTransport->getSnmpVersion();
		newNodeData->snmpSecurity = new SNMP_SecurityContext(pTransport->getSecurityContext());
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
   if (pAgentConn != NULL)
      pAgentConn->decRefCount();
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
      nxlog_debug_tag(DEBUG_TAG, 4, _T("AcceptNewNode(%s): auto filter - bResult=%d"), szIpAddr, bResult);
   }
   else
   {
      NXSL_VM *vm = CreateServerScriptVM(szFilter, NULL);
      if (vm != NULL)
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("AcceptNewNode(%s): Running filter script %s"), szIpAddr, szFilter);
         NXSL_Value *pValue = vm->createValue(new NXSL_Object(vm, &m_nxslDiscoveryClass, &data));
         if (vm->run(1, &pValue))
         {
            bResult = (vm->getResult()->getValueAsInt32() != 0) ? TRUE : FALSE;
            nxlog_debug_tag(DEBUG_TAG, 4, _T("AcceptNewNode(%s): Filter script result: %d"), szIpAddr, bResult);
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 4, _T("AcceptNewNode(%s): Filter script execution error: %s"),
                      szIpAddr, vm->getErrorText());
            PostEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", szFilter, vm->getErrorText(), 0);
         }
         delete vm;
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("AcceptNewNode(%s): Cannot find filter script %s"), szIpAddr, szFilter);
      }
   }

   return bResult;
}

/**
 * Process discovered address
 */
static void ProcessDiscoveredAddress(DiscoveredAddress *address)
{
   NewNodeData newNodeData(address->ipAddr);
   newNodeData.zoneUIN = address->zoneUIN;
   newNodeData.origin = NODE_ORIGIN_NETWORK_DISCOVERY;
   newNodeData.doConfPoll = true;

   if (address->ignoreFilter || AcceptNewNode(&newNodeData, address->bMacAddr))
   {
      ObjectTransactionStart();
      PollNewNode(&newNodeData);
      ObjectTransactionEnd();
   }

   s_processingListLock.lock();
   s_processingList.remove(address);
   s_processingListLock.unlock();
   MemFree(address);
}

/**
 * Node poller thread (poll new nodes and put them into the database)
 */
THREAD_RESULT THREAD_CALL NodePoller(void *arg)
{
	TCHAR szIpAddr[64];

   ThreadSetName("NodePoller");
   nxlog_debug(1, _T("Node poller started"));

   while(!IsShutdownInProgress())
   {
      DiscoveredAddress *address = g_nodePollerQueue.getOrBlock();
      if (address == INVALID_POINTER_VALUE)
         break;   // Shutdown indicator received

		nxlog_debug_tag(DEBUG_TAG, 4, _T("NodePoller: processing address %s/%d in zone %d (source type %d, source node [%u])"),
		         address->ipAddr.toString(szIpAddr), address->ipAddr.getMaskBits(), (int)address->zoneUIN,
		         address->sourceType, address->sourceNodeId);

		s_processingListLock.lock();
		s_processingList.add(address);
      s_processingListLock.unlock();

      if (g_discoveryThreadPool != NULL)
      {
         if (g_flags & AF_PARALLEL_NETWORK_DISCOVERY)
         {
            ThreadPoolExecute(g_discoveryThreadPool, ProcessDiscoveredAddress, address);
         }
         else
         {
            TCHAR key[32];
            _sntprintf(key, 32, _T("Zone%u"), address->zoneUIN);
            ThreadPoolExecuteSerialized(g_discoveryThreadPool, key, ProcessDiscoveredAddress, address);
         }
      }
      else
      {
         ProcessDiscoveredAddress(address);
      }
   }
   nxlog_debug(1, _T("Node poller thread terminated"));
   return THREAD_OK;
}
