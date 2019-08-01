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
 * Discovery source type names
 */
extern const TCHAR *g_discoveredAddrSourceTypeAsText[];

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
   delete snmpSecurity;
}

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

   if (IsZoningEnabled() && (newNodeData->creationFlags & NXC_NCF_AS_ZONE_PROXY) && (newNodeData->zoneUIN != 0))
   {
      Zone *zone = FindZoneByUIN(newNodeData->zoneUIN);
      if (zone != NULL)
      {
         zone->addProxy(node);
      }
   }

	if (newNodeData->doConfPoll)
   {
	   node->configurationPollWorkerEntry(RegisterPoller(PollerType::CONFIGURATION, node, true));
   }

   node->unhide();
   PostEvent(EVENT_NODE_ADDED, node->getId(), "d", static_cast<int>(newNodeData->origin));

   return node;
}

/**
 * Find existing node by MAC address to detect IP address change for already known node.
 * This function increments reference count for interface object.
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
			MacAddress macAddr = subnet->findMacAddress(ipAddr);
			if (!macAddr.isValid())
			{
				DbgPrintf(6, _T("GetOldNodeWithNewIP: MAC address not found"));
				return NULL;
			}
	      memcpy(nodeMacAddr, macAddr.value(), MAC_ADDR_LENGTH);
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

	Interface *iface = FindInterfaceByMAC(nodeMacAddr, true);

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
 * Node information for autodiscovery filter
 */
struct DiscoveryFilterData
{
   InetAddress ipAddr;
   NetworkDeviceDriver *driver;
   StringMap *driverAttributes;
   DriverData *driverData;
   InterfaceList *ifList;
   UINT32 zoneUIN;
   UINT32 flags;
   int snmpVersion;
   bool dnsNameResolved;
   TCHAR dnsName[MAX_DNS_NAME];
   TCHAR snmpObjectId[MAX_OID_LEN * 4];    // SNMP OID
   TCHAR agentVersion[MAX_AGENT_VERSION_LEN];
   TCHAR platform[MAX_PLATFORM_NAME_LEN];

   ~DiscoveryFilterData()
   {
      delete ifList;
      delete driverAttributes;
      delete driverData;
   }
};

/**
 * "DiscoveredInterface" NXSL class
 */
class NXSL_DiscoveredInterfaceClass : public NXSL_Class
{
public:
   NXSL_DiscoveredInterfaceClass();

   virtual NXSL_Value *getAttr(NXSL_Object *pObject, const char *attr);
};

/**
 * Implementation of NXSL class "DiscoveredInterface" - constructor
 */
NXSL_DiscoveredInterfaceClass::NXSL_DiscoveredInterfaceClass() : NXSL_Class()
{
   setName(_T("DiscoveredInterface"));
}

/**
 * Implementation of NXSL class "DiscoveredInterface" - get attribute
 */
NXSL_Value *NXSL_DiscoveredInterfaceClass::getAttr(NXSL_Object *object, const char *attr)
{
   NXSL_VM *vm = object->vm();
   InterfaceInfo *iface = static_cast<InterfaceInfo*>(object->getData());
   NXSL_Value *value = NULL;

   if (!strcmp(attr, "alias"))
   {
      value = vm->createValue(iface->alias);
   }
   else if (!strcmp(attr, "description"))
   {
      value = vm->createValue(iface->description);
   }
   else if (!strcmp(attr, "index"))
   {
      value = vm->createValue(iface->index);
   }
   else if (!strcmp(attr, "ipAddressList"))
   {
      NXSL_Array *a = new NXSL_Array(vm);
      for(int i = 0; i < iface->ipAddrList.size(); i++)
      {
         a->append(NXSL_InetAddressClass::createObject(vm, iface->ipAddrList.get(i)));
      }
      value = vm->createValue(a);
   }
   else if (!strcmp(attr, "isPhysicalPort"))
   {
      value = vm->createValue(iface->isPhysicalPort);
   }
   else if (!strcmp(attr, "macAddr"))
   {
      TCHAR buffer[64];
      value = vm->createValue(MACToStr(iface->macAddr, buffer));
   }
   else if (!strcmp(attr, "mtu"))
   {
      value = vm->createValue(iface->mtu);
   }
   else if (!strcmp(attr, "name"))
   {
      value = vm->createValue(iface->name);
   }
   else if (!strcmp(attr, "port"))
   {
      value = vm->createValue(iface->port);
   }
   else if (!strcmp(attr, "slot"))
   {
      value = vm->createValue(iface->slot);
   }
   else if (!strcmp(attr, "speed"))
   {
      value = vm->createValue(iface->speed);
   }
   else if (!strcmp(attr, "type"))
   {
      value = vm->createValue(iface->type);
   }
   return value;
}

/**
 * "DiscoveredInterface" class object
 */
static NXSL_DiscoveredInterfaceClass s_nxslDiscoveredInterfaceClass;

/**
 * "DiscoveredNode" NXSL class
 */
class NXSL_DiscoveredNodeClass : public NXSL_Class
{
public:
   NXSL_DiscoveredNodeClass();

   virtual NXSL_Value *getAttr(NXSL_Object *pObject, const char *attr);
};

/**
 * Implementation of NXSL class "DiscoveredNode" - constructor
 */
NXSL_DiscoveredNodeClass::NXSL_DiscoveredNodeClass() : NXSL_Class()
{
   setName(_T("DiscoveredNode"));
}

/**
 * Implementation of NXSL class "DiscoveredNode" - get attribute
 */
NXSL_Value *NXSL_DiscoveredNodeClass::getAttr(NXSL_Object *object, const char *attr)
{
   NXSL_VM *vm = object->vm();
   DiscoveryFilterData *data = static_cast<DiscoveryFilterData*>(object->getData());
   NXSL_Value *value = NULL;

   if (!strcmp(attr, "agentVersion"))
   {
      value = vm->createValue(data->agentVersion);
   }
   else if (!strcmp(attr, "dnsName"))
   {
      if (!data->dnsNameResolved)
      {
         if (IsZoningEnabled() && (data->zoneUIN != 0))
         {
            Zone *zone = FindZoneByUIN(data->zoneUIN);
            if (zone != NULL)
            {
               AgentConnectionEx *conn = zone->acquireConnectionToProxy();
               if (conn != NULL)
               {
                  conn->getHostByAddr(data->ipAddr, data->dnsName, MAX_DNS_NAME);
                  conn->decRefCount();
               }
            }
         }
         else
         {
            data->ipAddr.getHostByAddr(data->dnsName, MAX_DNS_NAME);
         }
         data->dnsNameResolved = true;
      }
      value = (data->dnsName[0] != 0) ? vm->createValue(data->dnsName) : vm->createValue();
   }
   else if (!strcmp(attr, "interfaces"))
   {
      NXSL_Array *array = new NXSL_Array(vm);
      if (data->ifList != NULL)
      {
         for(int i = 0; i < data->ifList->size(); i++)
            array->append(vm->createValue(new NXSL_Object(vm, &s_nxslDiscoveredInterfaceClass, data->ifList->get(i))));
      }
      value = vm->createValue(array);
   }
   else if (!strcmp(attr, "ipAddr"))
   {
      TCHAR buffer[64];
      value = vm->createValue(data->ipAddr.toString(buffer));
   }
   else if (!strcmp(attr, "isAgent"))
   {
      value = vm->createValue((LONG)((data->flags & NNF_IS_AGENT) ? 1 : 0));
   }
   else if (!strcmp(attr, "isBridge"))
   {
      value = vm->createValue((LONG)((data->flags & NNF_IS_BRIDGE) ? 1 : 0));
   }
   else if (!strcmp(attr, "isCDP"))
   {
      value = vm->createValue((LONG)((data->flags & NNF_IS_CDP) ? 1 : 0));
   }
   else if (!strcmp(attr, "isLLDP"))
   {
      value = vm->createValue((LONG)((data->flags & NNF_IS_LLDP) ? 1 : 0));
   }
   else if (!strcmp(attr, "isPrinter"))
   {
      value = vm->createValue((LONG)((data->flags & NNF_IS_PRINTER) ? 1 : 0));
   }
   else if (!strcmp(attr, "isRouter"))
   {
      value = vm->createValue((LONG)((data->flags & NNF_IS_ROUTER) ? 1 : 0));
   }
   else if (!strcmp(attr, "isSNMP"))
   {
      value = vm->createValue((LONG)((data->flags & NNF_IS_SNMP) ? 1 : 0));
   }
   else if (!strcmp(attr, "isSONMP"))
   {
      value = vm->createValue((LONG)((data->flags & NNF_IS_SONMP) ? 1 : 0));
   }
   else if (!strcmp(attr, "netMask"))
   {
      value = vm->createValue(data->ipAddr.getMaskBits());
   }
   else if (!strcmp(attr, "platformName"))
   {
      value = vm->createValue(data->platform);
   }
   else if (!strcmp(attr, "snmpOID"))
   {
      value = vm->createValue(data->snmpObjectId);
   }
   else if (!strcmp(attr, "snmpVersion"))
   {
      value = vm->createValue((LONG)data->snmpVersion);
   }
   else if (!strcmp(attr, "subnet"))
   {
      TCHAR buffer[64];
      value = vm->createValue(data->ipAddr.getSubnetAddress().toString(buffer));
   }
   else if (!strcmp(attr, "zone"))
   {
      Zone *zone = FindZoneByUIN(data->zoneUIN);
      value = (zone != NULL) ? zone->createNXSLObject(vm) : vm->createValue();
   }
   else if (!strcmp(attr, "zoneUIN"))
   {
      value = vm->createValue(data->zoneUIN);
   }
   return value;
}

/**
 * "DiscoveredNode" class object
 */
static NXSL_DiscoveredNodeClass s_nxslDiscoveredNodeClass;

/**
 * Check if newly discovered node should be added
 */
static bool AcceptNewNode(NewNodeData *newNodeData, BYTE *macAddr)
{
   TCHAR szFilter[MAX_CONFIG_VALUE], szBuffer[256], szIpAddr[64];
   UINT32 dwTemp;
   AgentConnection *pAgentConn;
	SNMP_Transport *pTransport;

	newNodeData->ipAddr.toString(szIpAddr);
   if ((FindNodeByIP(newNodeData->zoneUIN, newNodeData->ipAddr) != NULL) ||
       (FindSubnetByIP(newNodeData->zoneUIN, newNodeData->ipAddr) != NULL))
	{
      nxlog_debug_tag(DEBUG_TAG, 4, _T("AcceptNewNode(%s): node already exist in database"), szIpAddr);
      return false;  // Node already exist in database
	}

   if (!memcmp(macAddr, "\xFF\xFF\xFF\xFF\xFF\xFF", 6))
   {
		DbgPrintf(4, _T("AcceptNewNode(%s): broadcast MAC address"), szIpAddr);
      return false;  // Broadcast MAC
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
         return false;  // blocked by hook
   }

   int retryCount = 5;
   while (retryCount-- > 0)
   {
      Interface *iface = GetOldNodeWithNewIP(newNodeData->ipAddr, newNodeData->zoneUIN, macAddr);
      if (iface == NULL)
         break;

      if (!HostIsReachable(newNodeData->ipAddr, newNodeData->zoneUIN, false, NULL, NULL))
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("AcceptNewNode(%s): found existing interface with same MAC address, but new IP is not reachable"), szIpAddr);
         iface->decRefCount();
         return false;
      }

      // Interface could be deleted by configuration poller while HostIsReachable was running
      Node *oldNode = iface->getParentNode();
      if (!iface->isDeleted() && (oldNode != NULL))
      {
         if (iface->getIpAddressList()->hasAddress(oldNode->getIpAddress()))
         {
            // we should change node's primary IP only if old IP for this MAC was also node's primary IP
            TCHAR szOldIpAddr[16];
            oldNode->getIpAddress().toString(szOldIpAddr);
            nxlog_debug_tag(DEBUG_TAG, 4, _T("AcceptNewNode(%s): node already exist in database with IP %s, will change to new"), szIpAddr, szOldIpAddr);
            oldNode->changeIPAddress(newNodeData->ipAddr);
         }
         iface->decRefCount();
         return false;
      }

      iface->decRefCount();
      ThreadSleepMs(100);
   }
   if (retryCount == 0)
   {
      // Still getting deleted interface
      nxlog_debug_tag(DEBUG_TAG, 4, _T("AcceptNewNode(%s): found existing but marked for deletion interface with same MAC address"), szIpAddr);
      return false;
   }

   // Allow filtering by loaded modules
   for(UINT32 i = 0; i < g_dwNumModules; i++)
	{
		if (g_pModuleList[i].pfAcceptNewNode != NULL)
		{
			if (!g_pModuleList[i].pfAcceptNewNode(newNodeData->ipAddr, newNodeData->zoneUIN, macAddr))
				return false;	// filtered out by module
		}
	}

   // Read configuration
   ConfigReadStr(_T("DiscoveryFilter"), szFilter, MAX_CONFIG_VALUE, _T(""));
   StrStrip(szFilter);

   // Initialize discovered node data
   DiscoveryFilterData data;
   memset(&data, 0, sizeof(DiscoveryFilterData));
   data.ipAddr = newNodeData->ipAddr;
   data.zoneUIN = newNodeData->zoneUIN;

   // Check for address range if we use simple filter instead of script
   UINT32 autoFilterFlags = 0;
   if (!_tcsicmp(szFilter, _T("auto")))
   {
      autoFilterFlags = ConfigReadULong(_T("DiscoveryFilterFlags"), DFF_ALLOW_AGENT | DFF_ALLOW_SNMP);
      nxlog_debug_tag(DEBUG_TAG, 4, _T("AcceptNewNode(%s): auto filter, flags=%04X"), szIpAddr, autoFilterFlags);

      if (autoFilterFlags & DFF_ONLY_RANGE)
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("AcceptNewNode(%s): auto filter - checking range"), szIpAddr);
         ObjectArray<InetAddressListElement> *list = LoadServerAddressList(2);
         bool result = false;
         if (list != NULL)
         {
            for(int i = 0; (i < list->size()) && !result; i++)
            {
               result = list->get(i)->contains(data.ipAddr);
            }
            delete list;
         }
         nxlog_debug_tag(DEBUG_TAG, 4, _T("AcceptNewNode(%s): auto filter - range check result is %d"), szIpAddr, result);
         if (!result)
            return false;
      }
   }

   // Check if host is reachable
   if (!HostIsReachable(newNodeData->ipAddr, newNodeData->zoneUIN, true, &pTransport, &pAgentConn))
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("AcceptNewNode(%s): host is not reachable"), szIpAddr);
      return FALSE;
   }

   // Basic communication settings
   if (pTransport != NULL)
   {
      data.flags |= NNF_IS_SNMP;
      data.snmpVersion = pTransport->getSnmpVersion();
      newNodeData->snmpSecurity = new SNMP_SecurityContext(pTransport->getSecurityContext());

      // Get SNMP OID
      SnmpGet(data.snmpVersion, pTransport,
              _T(".1.3.6.1.2.1.1.2.0"), NULL, 0, data.snmpObjectId, MAX_OID_LEN * 4, 0);

      data.driver = FindDriverForNode(szIpAddr, data.snmpObjectId, NULL, pTransport);
      nxlog_debug_tag(DEBUG_TAG, 4, _T("AcceptNewNode(%s): selected device driver %s"), szIpAddr, data.driver->getName());
   }
   if (pAgentConn != NULL)
   {
      data.flags |= NNF_IS_AGENT;
      pAgentConn->getParameter(_T("Agent.Version"), MAX_AGENT_VERSION_LEN, data.agentVersion);
      pAgentConn->getParameter(_T("System.PlatformName"), MAX_PLATFORM_NAME_LEN, data.platform);
   }

   // Read interface list if possible
   if (data.flags & NNF_IS_AGENT)
   {
      data.ifList = pAgentConn->getInterfaceList();
   }
   if ((data.ifList == NULL) && (data.flags & NNF_IS_SNMP))
   {
      data.driverAttributes = new StringMap();
      data.driver->analyzeDevice(pTransport, data.snmpObjectId, data.driverAttributes, &data.driverData);
      data.ifList = data.driver->getInterfaces(pTransport, data.driverAttributes, data.driverData,
               ConfigReadInt(_T("UseInterfaceAliases"), 0), ConfigReadBoolean(_T("UseIfXTable"), true));
   }

   // TODO: check all interfaces for matching existing nodes

   // Check for filter script
   if ((szFilter[0] == 0) || (!_tcsicmp(szFilter, _T("none"))))
	{
		nxlog_debug_tag(DEBUG_TAG, 4, _T("AcceptNewNode(%s): no filtering, node accepted"), szIpAddr);
	   if (pAgentConn != NULL)
	      pAgentConn->decRefCount();
	   delete pTransport;
      return TRUE;   // No filtering
	}

   // Check if node is a router
   if (data.flags & NNF_IS_SNMP)
   {
      if (SnmpGet(data.snmpVersion, pTransport,
                  _T(".1.3.6.1.2.1.4.1.0"), NULL, 0, &dwTemp, sizeof(UINT32), 0) == SNMP_ERR_SUCCESS)
      {
         if (dwTemp == 1)
            data.flags |= NNF_IS_ROUTER;
      }
   }
   else if (data.flags & NNF_IS_AGENT)
   {
      // Check IP forwarding status
      if (pAgentConn->getParameter(_T("Net.IP.Forwarding"), 16, szBuffer) == ERR_SUCCESS)
      {
         if (_tcstoul(szBuffer, NULL, 10) != 0)
            data.flags |= NNF_IS_ROUTER;
      }
   }

   // Check various SNMP device capabilities
   if (data.flags & NNF_IS_SNMP)
   {
      // Check if node is a bridge
      if (SnmpGet(data.snmpVersion, pTransport,
                  _T(".1.3.6.1.2.1.17.1.1.0"), NULL, 0, szBuffer, sizeof(szBuffer), 0) == SNMP_ERR_SUCCESS)
      {
         data.flags |= NNF_IS_BRIDGE;
      }

      // Check for CDP (Cisco Discovery Protocol) support
      if (SnmpGet(data.snmpVersion, pTransport,
                  _T(".1.3.6.1.4.1.9.9.23.1.3.1.0"), NULL, 0, &dwTemp, sizeof(UINT32), 0) == SNMP_ERR_SUCCESS)
      {
         if (dwTemp == 1)
            data.flags |= NNF_IS_CDP;
      }

      // Check for SONMP (Nortel topology discovery protocol) support
      if (SnmpGet(data.snmpVersion, pTransport,
                  _T(".1.3.6.1.4.1.45.1.6.13.1.2.0"), NULL, 0, &dwTemp, sizeof(UINT32), 0) == SNMP_ERR_SUCCESS)
      {
         if (dwTemp == 1)
            data.flags |= NNF_IS_SONMP;
      }

      // Check for LLDP (Link Layer Discovery Protocol) support
      if (SnmpGet(data.snmpVersion, pTransport,
                  _T(".1.0.8802.1.1.2.1.3.2.0"), NULL, 0, szBuffer, sizeof(szBuffer), 0) == SNMP_ERR_SUCCESS)
      {
         data.flags |= NNF_IS_LLDP;
      }
   }

   bool result = false;

   // Check if we use simple filter instead of script
   if (!_tcsicmp(szFilter, _T("auto")))
   {
      if ((autoFilterFlags & (DFF_ALLOW_AGENT | DFF_ALLOW_SNMP)) == 0)
      {
         result = true;
      }
      else
      {
         if (autoFilterFlags & DFF_ALLOW_AGENT)
         {
            if (data.flags & NNF_IS_AGENT)
               result = true;
         }

         if (autoFilterFlags & DFF_ALLOW_SNMP)
         {
            if (data.flags & NNF_IS_SNMP)
               result = true;
         }
      }
      nxlog_debug_tag(DEBUG_TAG, 4, _T("AcceptNewNode(%s): auto filter - bResult=%d"), szIpAddr, result);
   }
   else
   {
      NXSL_VM *vm = CreateServerScriptVM(szFilter, NULL);
      if (vm != NULL)
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("AcceptNewNode(%s): Running filter script %s"), szIpAddr, szFilter);

         if (pTransport != NULL)
         {
            vm->setGlobalVariable("$snmp", vm->createValue(new NXSL_Object(vm, &g_nxslSnmpTransportClass, pTransport)));
            pTransport = NULL;   // Transport will be deleted by NXSL object destructor
         }
         // TODO: make agent connection available in script
         vm->setGlobalVariable("$node", vm->createValue(new NXSL_Object(vm, &s_nxslDiscoveredNodeClass, &data)));

         NXSL_Value *param = vm->createValue(new NXSL_Object(vm, &s_nxslDiscoveredNodeClass, &data));
         if (vm->run(1, &param))
         {
            result = (vm->getResult()->getValueAsInt32() != 0);
            nxlog_debug_tag(DEBUG_TAG, 4, _T("AcceptNewNode(%s): Filter script result: %d"), szIpAddr, result);
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

   // Cleanup
   if (pAgentConn != NULL)
      pAgentConn->decRefCount();
   delete pTransport;

   return result;
}

/**
 * Create discovered node object
 */
static void CreateDiscoveredNode(NewNodeData *newNodeData)
{
   // Double check IP address because parallel discovery may already create that node
   if ((FindNodeByIP(newNodeData->zoneUIN, newNodeData->ipAddr) == NULL) &&
       (FindSubnetByIP(newNodeData->zoneUIN, newNodeData->ipAddr) == NULL))
   {
      ObjectTransactionStart();
      PollNewNode(newNodeData);
      ObjectTransactionEnd();
   }
   else
   {
      TCHAR ipAddr[64];
      newNodeData->ipAddr.toString(ipAddr);
      nxlog_debug_tag(DEBUG_TAG, 4, _T("CreateDiscoveredNode(%s): node already exist in database"), ipAddr);
   }
   delete newNodeData;
}

/**
 * Process discovered address
 */
static void ProcessDiscoveredAddress(DiscoveredAddress *address)
{
   if (!IsShutdownInProgress())
   {
      NewNodeData *newNodeData = new NewNodeData(address->ipAddr);
      newNodeData->zoneUIN = address->zoneUIN;
      newNodeData->origin = NODE_ORIGIN_NETWORK_DISCOVERY;
      newNodeData->doConfPoll = true;

      if (address->ignoreFilter || AcceptNewNode(newNodeData, address->bMacAddr))
      {
         if (g_discoveryThreadPool != NULL)
         {
            TCHAR key[32];
            _sntprintf(key, 32, _T("Zone%u"), address->zoneUIN);
            ThreadPoolExecuteSerialized(g_discoveryThreadPool, key, CreateDiscoveredNode, newNodeData);
         }
         else
         {
            CreateDiscoveredNode(newNodeData);
         }
      }
      else
      {
         delete newNodeData;
      }
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

		nxlog_debug_tag(DEBUG_TAG, 4, _T("NodePoller: processing address %s/%d in zone %d (source type %s, source node [%u])"),
		         address->ipAddr.toString(szIpAddr), address->ipAddr.getMaskBits(), (int)address->zoneUIN,
		         g_discoveredAddrSourceTypeAsText[address->sourceType], address->sourceNodeId);

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

/**
 * Get total size of discovery poller queue (all stages)
 */
INT64 GetDiscoveryPollerQueueSize()
{
   int poolQueueSize;
   if (g_discoveryThreadPool != NULL)
   {
      ThreadPoolInfo info;
      ThreadPoolGetInfo(g_discoveryThreadPool, &info);
      poolQueueSize = ((info.activeRequests > info.curThreads) ? info.activeRequests - info.curThreads : 0) + info.serializedRequests;
   }
   else
   {
      poolQueueSize = 0;
   }
   return g_nodePollerQueue.size() + poolQueueSize;
}
