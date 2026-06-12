/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Victor Kirhenshtein
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
** File: discovery.cpp
**
**/

#include "nxcore.h"
#include <agent_tunnel.h>
#include <nxcore_discovery.h>
#include <ethernet_ip.h>

#define DEBUG_TAG_DISCOVERY      L"poll.discovery"

/**
 * Thread pool
 */
ThreadPool *g_discoveryThreadPool = nullptr;

/**
 * Discovery source type names
 */
static const wchar_t *s_discoveredAddrSourceTypeAsText[] = {
   L"ARP Cache",
   L"Routing Table",
   L"Agent Registration",
   L"SNMP Trap",
   L"Syslog",
   L"Active Discovery",
};

/**
 * Node poller queue (polls new nodes)
 */
static ObjectQueue<DiscoveredAddress> s_nodePollerQueue(1024, Ownership::True);

/**
 * Processing list key
 */
struct DiscoveredAddressKey
{
   int32_t zoneUIN;
   InetAddress address;
};

/**
 * IP addresses being processed by discovery
 */
static SynchronizedHashSet<DiscoveredAddressKey> s_processingList;

/**
 * Add address to processing list
 */
static void AddActiveAddress(int32_t zoneUIN, const InetAddress& addr)
{
   DiscoveredAddressKey key;
   key.zoneUIN = zoneUIN;
   key.address = addr;
   s_processingList.put(key);
}

/**
 * Add address to processing list
 */
static void RemoveActiveAddress(int32_t zoneUIN, const InetAddress& addr)
{
   DiscoveredAddressKey key;
   key.zoneUIN = zoneUIN;
   key.address = addr;
   s_processingList.remove(key);
}

/**
 * Check if given address is in processing by new node poller
 */
static bool IsActiveAddress(int32_t zoneUIN, const InetAddress& addr)
{
   DiscoveredAddressKey key;
   key.zoneUIN = zoneUIN;
   key.address = addr;
   return s_processingList.contains(key);
}

/**
 * Check if host at given IP address is reachable by NetXMS server.
 * When fullCheck is true, all connectivity probes are performed and results are stored in the address structure.
 * When fullCheck is false, only basic reachability is checked and the function returns on first success.
 */
static bool HostIsReachable(DiscoveredAddress *address, bool fullCheck)
{
   const InetAddress& ipAddr = address->ipAddr;
   int32_t zoneUIN = address->zoneUIN;
   bool reachable = false;

   uint32_t zoneProxy = 0;
   if (IsZoningEnabled() && (zoneUIN != 0))
   {
      shared_ptr<Zone> zone = FindZoneByUIN(zoneUIN);
      if (zone != nullptr)
      {
         zoneProxy = zone->getProxyNodeId(nullptr);
      }
   }

   // *** ICMP PING ***
   if (zoneProxy != 0)
   {
      shared_ptr<Node> proxyNode = static_pointer_cast<Node>(g_idxNodeById.get(zoneProxy));
      if ((proxyNode != nullptr) && proxyNode->isNativeAgent() && !proxyNode->isDown())
      {
         shared_ptr<AgentConnectionEx> conn = proxyNode->createAgentConnection();
         if (conn != nullptr)
         {
            TCHAR parameter[128], buffer[64];

            _sntprintf(parameter, 128, L"Icmp.Ping(%s)", ipAddr.toString(buffer));
            if (conn->getParameter(parameter, buffer, 64) == ERR_SUCCESS)
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
         }
      }
   }
   else  // not using ICMP proxy
   {
      if (IcmpPing(ipAddr, 3, g_icmpPingTimeout, nullptr, g_icmpPingSize, false) == ICMP_SUCCESS)
         reachable = true;
   }

   if (reachable && !fullCheck)
      return true;

   // *** NetXMS agent ***
   if (!(g_flags & AF_DISABLE_AGENT_PROBE))
   {
      auto agentConnection = make_shared<AgentConnectionEx>(0, ipAddr, AGENT_LISTEN_PORT, nullptr);
      shared_ptr<Node> proxyNode;
      if (zoneProxy != 0)
      {
         proxyNode = static_pointer_cast<Node>(g_idxNodeById.get(zoneProxy));
         if (proxyNode != nullptr)
         {
            shared_ptr<AgentTunnel> tunnel = GetTunnelForNode(zoneProxy);
            if (tunnel != nullptr)
            {
               agentConnection->setProxy(tunnel, proxyNode->getAgentSecret());
            }
            else
            {
               agentConnection->setProxy(proxyNode->getIpAddress(), proxyNode->getAgentPort(), proxyNode->getAgentSecret());
            }
         }
      }

      agentConnection->setCommandTimeout(g_agentCommandTimeout);
      uint32_t rcc;
      uint16_t agentPort = AGENT_LISTEN_PORT;
      if (!agentConnection->connect(g_serverKey, &rcc))
      {
         // If connection failed, try well-known agent ports
         if (rcc == ERR_CONNECT_FAILED)
         {
            IntegerArray<uint16_t> knownPorts = GetWellKnownPorts(L"agent", zoneUIN);
            for (int i = 0; (i < knownPorts.size()) && !IsShutdownInProgress(); i++)
            {
               uint16_t port = knownPorts.get(i);
               if (port == AGENT_LISTEN_PORT)
                  continue;
               agentConnection->setPort(port);
               if (agentConnection->connect(g_serverKey, &rcc))
               {
                  agentPort = port;
                  break;
               }
               if ((rcc == ERR_AUTH_REQUIRED) || (rcc == ERR_AUTH_FAILED))
               {
                  agentPort = port;
                  break;
               }
            }
         }

         // If there are authentication problem, try default shared secret
         if ((rcc == ERR_AUTH_REQUIRED) || (rcc == ERR_AUTH_FAILED))
         {
            StringList secrets;
            DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
            DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT secret FROM shared_secrets WHERE zone=? OR zone=-1 ORDER BY zone DESC, id ASC"));
            if (hStmt != nullptr)
            {
              DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, (proxyNode != nullptr) ? proxyNode->getZoneUIN() : 0);

              DB_RESULT hResult = DBSelectPrepared(hStmt);
              if (hResult != nullptr)
              {
                 int count = DBGetNumRows(hResult);
                 wchar_t secret[128];
                 for(int i = 0; i < count; i++)
                    secrets.add(DBGetField(hResult, i, 0, secret, 128));
                 DBFreeResult(hResult);
              }
              DBFreeStatement(hStmt);
            }
            DBConnectionPoolReleaseConnection(hdb);

            for (int i = 0; (i < secrets.size()) && !IsShutdownInProgress(); i++)
            {
               agentConnection->setSharedSecret(secrets.get(i));
               if (agentConnection->connect(g_serverKey, &rcc))
               {
                  if (fullCheck)
                     _tcslcpy(address->agentSecret, secrets.get(i), MAX_SECRET_LENGTH);
                  break;
               }

               if (((rcc != ERR_AUTH_REQUIRED) || (rcc != ERR_AUTH_FAILED)))
               {
                  break;
               }
            }
         }
      }
      if (rcc == ERR_SUCCESS)
      {
         if (fullCheck)
         {
            address->agentConnection = agentConnection;
            address->agentPort = agentPort;
         }
         reachable = true;
      }
      else
      {
         TCHAR ipAddrText[64];
         nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("HostIsReachable(%s): agent connection check failed with error %u (%s)%s%s"),
                  ipAddr.toString(ipAddrText), rcc, AgentErrorCodeToText(rcc), (proxyNode != nullptr) ? _T(", proxy node ") : _T(""),
                  (proxyNode != nullptr) ? proxyNode->getName() : _T(""));
      }
   }
   else
   {
      TCHAR ipAddrText[64];
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("HostIsReachable(%s): agent connection probe disabled"), ipAddr.toString(ipAddrText));
   }

   if (reachable && !fullCheck)
      return true;

   // *** SNMP ***
   if ((g_flags & (AF_DISABLE_SNMP_V3_PROBE | AF_DISABLE_SNMP_V2_PROBE | AF_DISABLE_SNMP_V1_PROBE)) != (AF_DISABLE_SNMP_V3_PROBE | AF_DISABLE_SNMP_V2_PROBE | AF_DISABLE_SNMP_V1_PROBE))
   {
      SNMP_Version version;
      StringList oids;
      oids.add(_T(".1.3.6.1.2.1.1.2.0"));
      oids.add(_T(".1.3.6.1.2.1.1.1.0"));
      AddDriverSpecificOids(&oids);
      SNMP_Transport *snmpTransport = SnmpCheckCommSettings(zoneProxy, ipAddr, &version, 0, nullptr, oids, zoneUIN, true);
      if (snmpTransport != nullptr)
      {
         if (fullCheck)
         {
            snmpTransport->setSnmpVersion(version);
            address->snmpTransport = snmpTransport;
         }
         else
         {
            delete snmpTransport;
         }
         reachable = true;
      }
   }
   else
   {
      wchar_t ipAddrText[64];
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, L"HostIsReachable(%s): all SNMP probes disabled", ipAddr.toString(ipAddrText));
   }

   if (reachable && !fullCheck)
      return true;

   // *** SSH ***
   if (!(g_flags & AF_DISABLE_SSH_PROBE))
   {
      if (SSHCheckCommSettings((zoneProxy != 0) ? zoneProxy : g_dwMgmtNode, ipAddr, zoneUIN,
               fullCheck ? &address->sshCredentials : nullptr, fullCheck ? &address->sshPort : nullptr))
      {
         reachable = true;
      }
   }
   else
   {
      wchar_t ipAddrText[64];
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, L"HostIsReachable(%s): SSH connection probe disabled", ipAddr.toString(ipAddrText));
   }

   return reachable;
}

/**
 * Check if this discovered address already known on existing node
 */
static bool DuplicatesCheck(DiscoveredAddress *address)
{
   wchar_t ipAddrText[64];
   address->ipAddr.toString(ipAddrText);

   if (address->macAddr.isBroadcast())
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, L"DuplicatesCheck(%s): broadcast MAC address", ipAddrText);
      return false;  // Broadcast MAC
   }

   if (IsAddressInTopologyExcludedSubnet(address->zoneUIN, address->ipAddr))
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, L"DuplicatesCheck(%s): IP address is in topology excluded subnet, skipping duplicate checks", ipAddrText);
      return true;
   }

   if (FindNodeByIP(address->zoneUIN, address->ipAddr) != nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, L"DuplicatesCheck(%s): node already exist in database", ipAddrText);
      return false;  // Node already exist in database
   }

   shared_ptr<Subnet> subnet = FindSubnetForNode(address->zoneUIN, address->ipAddr);
   if (subnet != nullptr)
   {
      if (subnet->getIpAddress().equals(address->ipAddr))
      {
         nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("DuplicatesCheck(%s): IP address already known as subnet address"), ipAddrText);
         return false;
      }
      if (address->ipAddr.isSubnetBroadcast(subnet->getIpAddress().getMaskBits()))
      {
         nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("DuplicatesCheck(%s): IP address already known as subnet %s broadcast address"), ipAddrText, subnet->getName());
         return false;
      }
   }

   return true;
}

/**
 * Check if newly discovered node should be added (stage 1)
 */
static bool AcceptNewNodeStage1(DiscoveredAddress *address)
{
   if (!DuplicatesCheck(address))
      return false;

   TCHAR ipAddrText[MAX_IP_ADDR_TEXT_LEN];
   address->ipAddr.toString(ipAddrText);

   // If MAC address is not known try to find it in ARP cache
   if (!address->macAddr.isValid())
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("AcceptNewNodeStage1(%s): MAC address is not known, will lookup in ARP caches"), ipAddrText);
      shared_ptr<Subnet> subnet = FindSubnetForNode(address->zoneUIN, address->ipAddr);
      if (subnet != nullptr)
      {
         MacAddress macAddr = subnet->findMacAddress(address->ipAddr);
         if (macAddr.isValid())
         {
            if (macAddr.isBroadcast())
            {
               nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNodeStage1(%s): broadcast MAC address"), ipAddrText);
               return false;  // Broadcast MAC
            }
            nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("AcceptNewNodeStage1(%s): found MAC address %s"), ipAddrText, macAddr.toString().cstr());
            address->macAddr = macAddr;
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("AcceptNewNodeStage1(%s): MAC address not found"), ipAddrText);
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("AcceptNewNodeStage1(%s): cannot find matching subnet"), ipAddrText);
      }
   }

   NXSL_VM *hook = FindHookScript(_T("AcceptNewNode"), shared_ptr<NetObj>());
   if (hook != nullptr)
   {
      bool stop = false;
      hook->setGlobalVariable("$ipAddress", NXSL_InetAddressClass::createObject(hook, address->ipAddr));
      hook->setGlobalVariable("$ipAddr", hook->createValue(ipAddrText));
      hook->setGlobalVariable("$ipNetMask", hook->createValue(address->ipAddr.getMaskBits()));
      hook->setGlobalVariable("$macAddress", address->macAddr.isValid() ? NXSL_MacAddressClass::createObject(hook, address->macAddr) : hook->createValue());
      hook->setGlobalVariable("$macAddr", address->macAddr.isValid() ? hook->createValue(address->macAddr.toString()) : hook->createValue());
      hook->setGlobalVariable("$zoneUIN", hook->createValue(address->zoneUIN));
      if (hook->run())
      {
         NXSL_Value *result = hook->getResult();
         if (result->isFalse())
         {
            stop = true;
            nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNodeStage1(%s): rejected by hook script"), ipAddrText);
         }
      }
      else
      {
         stop = true;
         nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNodeStage1(%s): rejected because of hook script execution error (%s)"), ipAddrText, hook->getErrorText());
      }
      delete hook;
      if (stop)
         return false;  // blocked by hook
   }

   if (address->macAddr.isValid())
   {
      int retryCount = 5;
      while (retryCount-- > 0)
      {
         shared_ptr<Interface> iface = FindInterfaceByMAC(address->macAddr);
         if (iface == nullptr)
            break;

         if (!HostIsReachable(address, false))
         {
            nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNodeStage1(%s): found existing interface with same MAC address, but new IP is not reachable"), ipAddrText);
            return false;
         }

         // Interface could be deleted by configuration poller while HostIsReachable was running
         shared_ptr<Node> oldNode = iface->getParentNode();
         if (!iface->isDeleted() && (oldNode != nullptr))
         {
            TCHAR oldIpAddrText[MAX_IP_ADDR_TEXT_LEN], macAddrText[32];
            nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNodeStage1(%s): node with MAC address %s already exist in database with IP %s and name %s"),
                  ipAddrText, address->macAddr.toString(macAddrText), oldNode->getIpAddress().toString(oldIpAddrText), oldNode->getName());

            // we should change node's primary IP only if old IP for this MAC was also node's primary IP
            if (iface->hasIpAddress(oldNode->getIpAddress()))
            {
               // Try to re-read interface list using old IP address and check if node has botl old and new IPs
               bool bothAddressesFound = false;
               InterfaceList *ifList = oldNode->getInterfaceList();
               if (ifList != nullptr)
               {
                  bothAddressesFound = (ifList->findByIpAddress(address->ipAddr) != nullptr) && (ifList->findByIpAddress(oldNode->getIpAddress()) != nullptr);
                  delete ifList;
               }
               if (!bothAddressesFound)
               {
                  oldNode->changeIPAddress(address->ipAddr);
                  nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNodeStage1(%s): primary IP address for existing node %s [%u] changed from %s to %s"),
                        ipAddrText, oldNode->getName(), oldNode->getId(), oldIpAddrText, ipAddrText);
               }
               else
               {
                  nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNodeStage1(%s): forcing configuration poll for node %s [%u]"), ipAddrText, oldNode->getName(), oldNode->getId());
                  oldNode->forceConfigurationPoll();
               }
            }
            return false;
         }

         ThreadSleepMs(100);
      }
      if (retryCount == 0)
      {
         // Still getting deleted interface
         nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNodeStage1(%s): found existing but marked for deletion interface with same MAC address"), ipAddrText);
         return false;
      }
   }

   // Allow filtering by loaded modules
   ENUMERATE_MODULES(pfAcceptNewNode)
   {
      if (!CURRENT_MODULE.pfAcceptNewNode(address->ipAddr, address->zoneUIN, address->macAddr))
      {
         nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNodeStage1(%s): rejected by module %s"), ipAddrText, CURRENT_MODULE.name);
         return false;  // filtered out by module
      }
   }

   // Read filter configuration
   uint32_t filterFlags = ConfigReadULong(_T("NetworkDiscovery.Filter.Flags"), 0);
   nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNodeStage1(%s): filter flags=%04X"), ipAddrText, filterFlags);

   // Check for address range
   if ((filterFlags & DFF_CHECK_ADDRESS_RANGE) && !address->ignoreFilter)
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNodeStage1(%s): filter - checking address range"), ipAddrText);
      ObjectArray<InetAddressListElement> *list = LoadServerAddressList(2);
      bool result = false;
      if (list != nullptr)
      {
         for(int i = 0; (i < list->size()) && !result; i++)
         {
            result = list->get(i)->contains(address->ipAddr);
         }
         delete list;
      }
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNodeStage1(%s): filter - range check result is %s"), ipAddrText, BooleanToString(result));
      if (!result)
         return false;
   }

   // Check if host is reachable
   if (!HostIsReachable(address, true))
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNodeStage1(%s): host is not reachable"), ipAddrText);
      return false;
   }

   // Initialize discovered node data
   auto data = new DiscoveryFilterData(address->ipAddr, address->zoneUIN);
   address->data = data;

   // Basic communication settings
   if (address->snmpTransport != nullptr)
   {
      data->flags |= NNF_IS_SNMP;
      data->snmpVersion = address->snmpTransport->getSnmpVersion();

      // Get SNMP OID
      SnmpGet(data->snmpVersion, address->snmpTransport, { 1, 3, 6, 1, 2, 1, 1, 2, 0 }, &data->snmpObjectId, 0, SG_OBJECT_ID_RESULT);

      data->driver = FindDriverForNode(ipAddrText, data->snmpObjectId, nullptr, address->snmpTransport);
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNodeStage1(%s): selected device driver %s"), ipAddrText, data->driver->getName());
   }
   if (address->agentConnection != nullptr)
   {
      data->flags |= NNF_IS_AGENT;
      data->agentConnection = address->agentConnection;
      address->agentConnection->getParameter(_T("Agent.Version"), data->agentVersion, MAX_AGENT_VERSION_LEN);
      address->agentConnection->getParameter(_T("System.PlatformName"), data->platform, MAX_PLATFORM_NAME_LEN);
   }
   if (address->sshPort != 0)
   {
      data->flags |= NNF_IS_SSH;
   }

   // Read interface list if possible
   if (data->flags & NNF_IS_AGENT)
   {
      data->ifList = address->agentConnection->getInterfaceList();
   }
   if ((data->ifList == nullptr) && (data->flags & NNF_IS_SNMP))
   {
      data->driver->analyzeDevice(address->snmpTransport, data->snmpObjectId, data, &data->driverData);
      data->ifList = data->driver->getInterfaces(address->snmpTransport, data, data->driverData, ConfigReadBoolean(_T("Objects.Interfaces.UseIfXTable"), true));
   }

   // Check if node is a router
   if (data->flags & NNF_IS_SNMP)
   {
      uint32_t value;
      if (SnmpGet(data->snmpVersion, address->snmpTransport, { 1, 3, 6, 1, 2, 1, 4, 1, 0 }, &value, sizeof(uint32_t), 0) == SNMP_ERR_SUCCESS)
      {
         if (value == 1)
            data->flags |= NNF_IS_ROUTER;
      }
   }
   else if (data->flags & NNF_IS_AGENT)
   {
      // Check IP forwarding status
      TCHAR buffer[16];
      if (address->agentConnection->getParameter(_T("Net.IP.Forwarding"), buffer, 16) == ERR_SUCCESS)
      {
         if (_tcstoul(buffer, nullptr, 10) != 0)
            data->flags |= NNF_IS_ROUTER;
      }
   }

   // Check various SNMP device capabilities
   if (data->flags & NNF_IS_SNMP)
   {
      TCHAR buffer[256];

      // Check if node is a bridge
      if (SnmpGet(data->snmpVersion, address->snmpTransport, { 1, 3, 6, 1, 2, 1, 17, 1, 1, 0 }, buffer, sizeof(buffer), 0) == SNMP_ERR_SUCCESS)
      {
         data->flags |= NNF_IS_BRIDGE;
      }

      // Check for CDP (Cisco Discovery Protocol) support
      uint32_t value;
      if (SnmpGet(data->snmpVersion, address->snmpTransport, { 1, 3, 6, 1, 4, 1, 9, 9, 23, 1, 3, 1, 0 }, &value, sizeof(uint32_t), 0) == SNMP_ERR_SUCCESS)
      {
         if (value == 1)
            data->flags |= NNF_IS_CDP;
      }

      // Check for NDP/SONMP (Nortel Networks topology discovery protocol) support
      if (SnmpGet(data->snmpVersion, address->snmpTransport, { 1, 3, 6, 1, 4, 1, 45, 1, 6, 13, 1, 2, 0 }, &value, sizeof(uint32_t), 0) == SNMP_ERR_SUCCESS)
      {
         if (value == 1)
            data->flags |= NNF_IS_SONMP;
      }

      // Check for LLDP (Link Layer Discovery Protocol) support
      if (SnmpGet(data->snmpVersion, address->snmpTransport, { 1, 0, 8802, 1, 1, 2, 1, 3, 2, 0 }, buffer, sizeof(buffer), 0) == SNMP_ERR_SUCCESS)
      {
         data->flags |= NNF_IS_LLDP;
      }
   }

   return true;
}

/**
 * Check if newly discovered node should be added (stage 2)
 */
static bool AcceptNewNodeStage2(DiscoveredAddress *address)
{
   if (!DuplicatesCheck(address))
      return false;

   wchar_t ipAddrText[MAX_IP_ADDR_TEXT_LEN];
   address->ipAddr.toString(ipAddrText);

   DiscoveryFilterData *data = address->data;

   // Check all interfaces for matching existing nodes
   if (data->ifList != nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 5, _T("AcceptNewNodeStage2(%s): interface list available, checking for duplicates"), ipAddrText);

      bool duplicate = false;
      for (int i = 0; (i < data->ifList->size()) && !duplicate; i++)
      {
         InterfaceInfo *iface = data->ifList->get(i);
         if (iface->type == IFTYPE_SOFTWARE_LOOPBACK)
            continue;

         nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 5, _T("AcceptNewNodeStage2(%s): checking interface %s (ifIndex=%u type=%u)"), ipAddrText, iface->name, iface->index, iface->type);
         for (int j = 0; j < iface->ipAddrList.size(); j++)
         {
            InetAddress addr = iface->ipAddrList.get(j);
            if (!addr.isValidUnicast())
               continue;

            if (IsAddressInTopologyExcludedSubnet(address->zoneUIN, addr))
            {
               nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 5, L"AcceptNewNodeStage2(%s): IP address %s on interface %s is in topology excluded subnet, skipping",
                  ipAddrText, addr.toString().cstr(), iface->name);
               continue;
            }

            shared_ptr<Node> existingNode = FindNodeByIP(address->zoneUIN, addr);
            if (existingNode != nullptr)
            {
               nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNodeStage2(%s): IP address %s on interface %s matches existing node %s [%u]"),
                  ipAddrText, addr.toString().cstr(), iface->name, existingNode->getName(), existingNode->getId());
               duplicate = true;
               break;
            }

            // Do not do subnet checks for /0, /32 and /128 addresses
            if ((addr.getHostBits() > 1) && (addr.getMaskBits() > 0))
            {
               shared_ptr<Subnet> subnet = FindSubnetForNode(address->zoneUIN, addr);
               if (subnet != nullptr)
               {
                  if (subnet->getIpAddress().equals(addr))
                  {
                     nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNodeStage2(%s): IP address %s on interface %s already known as subnet address"),
                        ipAddrText, addr.toString().cstr(), iface->name);
                     duplicate = true;
                     break;
                  }
                  if (addr.isSubnetBroadcast(subnet->getIpAddress().getMaskBits()))
                  {
                     nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNodeStage2(%s): IP address %s on interface %s already known as subnet %s broadcast address"),
                        ipAddrText, addr.toString().cstr(), iface->name, subnet->getName());
                     duplicate = true;
                     break;
                  }
               }

               InetAddress subnetAddress = addr.getSubnetAddress();
               if (subnetAddress.contains(address->ipAddr) && address->ipAddr.isSubnetBroadcast(subnetAddress.getMaskBits()))
               {
                  nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNodeStage2(%s): IP address %s on interface %s is a broadcast address for subnet %s/%d"),
                     ipAddrText, addr.toString().cstr(), iface->name, subnetAddress.toString().cstr(), subnetAddress.getMaskBits());
                  duplicate = true;
                  break;
               }
               if (subnetAddress.equals(address->ipAddr))
               {
                  nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNodeStage2(%s): IP address %s on interface %s is a subnet address for subnet %s/%d"),
                     ipAddrText, addr.toString().cstr(), iface->name, subnetAddress.toString().cstr(), subnetAddress.getMaskBits());
                  duplicate = true;
                  break;
               }
            }
            else
            {
               nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNodeStage2(%s): skipping subnet check for IP address %s/%d"), ipAddrText, addr.toString().cstr(), addr.getMaskBits());
            }
         }
      }

      if (duplicate)
         return false;
   }

   if (address->ignoreFilter)
      return true;

   // Read filter configuration
   uint32_t filterFlags = ConfigReadULong(_T("NetworkDiscovery.Filter.Flags"), 0);
   nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNodeStage2(%s): filter flags=%04X"), ipAddrText, filterFlags);

   bool result = true;

   // Check supported communication protocols
   if (filterFlags & DFF_CHECK_PROTOCOLS)
   {
      result = false;

      if ((filterFlags & DFF_PROTOCOL_AGENT) && (data->flags & NNF_IS_AGENT))
         result = true;

      if ((filterFlags & DFF_PROTOCOL_SNMP) && (data->flags & NNF_IS_SNMP))
         result = true;

      if ((filterFlags & DFF_PROTOCOL_SSH) && (data->flags & NNF_IS_SSH))
         result = true;

      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNodeStage2(%s): protocol check result is %s"), ipAddrText, BooleanToString(result));
   }

   // Execute discovery filter hook script
   if (result)
   {
      NXSL_VM *hook = FindHookScript(_T("DiscoveryFilter"), shared_ptr<NetObj>());
      if (hook != nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNodeStage2(%s): running discovery filter hook script"), ipAddrText);

         if (address->snmpTransport != nullptr)
         {
            hook->setGlobalVariable("$snmp", hook->createValue(hook->createObject(&g_nxslSnmpTransportClass, address->snmpTransport)));
            address->snmpTransport = nullptr;   // Transport will be deleted by NXSL object destructor
         }
         hook->setGlobalVariable("$node", hook->createValue(hook->createObject(&g_nxslDiscoveredNodeClass, data)));

         NXSL_Value *param = hook->createValue(hook->createObject(&g_nxslDiscoveredNodeClass, data));
         if (hook->run(1, &param))
         {
            result = hook->getResult()->getValueAsBoolean();
            nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNodeStage2(%s): discovery filter hook script result is %s"), ipAddrText, BooleanToString(result));
         }
         else
         {
            result = false;   // Consider script runtime error to be a negative result
            nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNodeStage2(%s): discovery filter hook script execution error: %s"), ipAddrText, hook->getErrorText());
            ReportScriptError(SCRIPT_CONTEXT_OBJECT, nullptr, 0, hook->getErrorText(), _T("Hook::DiscoveryFilter"));
         }
         delete hook;
      }
   }

   return result;
}

/**
 * Create discovered node object
 */
static void CreateDiscoveredNode(NewNodeData *newNodeData)
{
   TCHAR ipAddrText[MAX_IP_ADDR_TEXT_LEN];
   newNodeData->ipAddr.toString(ipAddrText);
   nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("CreateDiscoveredNode(%s): processing creation request"), ipAddrText);

   // Double check IP address because parallel discovery may already create that node
   if ((FindNodeByIP(newNodeData->zoneUIN, newNodeData->ipAddr) == nullptr) && (FindSubnetByIP(newNodeData->zoneUIN, newNodeData->ipAddr) == nullptr))
   {
      if (PollNewNode(newNodeData) != nullptr)
         nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("CreateDiscoveredNode(%s): initial poll completed"), ipAddrText);
      else
         nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("CreateDiscoveredNode(%s): node object creation failed"), ipAddrText);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("CreateDiscoveredNode(%s): node already exist in database"), ipAddrText);
   }
   delete newNodeData;
}

/**
 * Process discovered address (stage 2)
 */
static void ProcessDiscoveredAddressStage2(DiscoveredAddress *address)
{
   if (IsShutdownInProgress() || !IsActiveAddress(address->zoneUIN, address->ipAddr))
      return;

   if (AcceptNewNodeStage2(address))
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("ProcessDiscoveredAddressStage2(%s): address accepted"), address->ipAddr.toString().cstr());

      auto newNodeData = new NewNodeData(address->ipAddr, address->macAddr);
      newNodeData->zoneUIN = address->zoneUIN;
      newNodeData->origin = NODE_ORIGIN_NETWORK_DISCOVERY;
      newNodeData->doConfPoll = true;
      if (address->snmpTransport != nullptr)
      {
         newNodeData->snmpSecurity = new SNMP_SecurityContext(address->snmpTransport->getSecurityContext());
         newNodeData->snmpPort = address->snmpTransport->getPort();
         newNodeData->snmpVersion = address->snmpTransport->getSnmpVersion();
      }
      newNodeData->agentPort = address->agentPort;
      _tcslcpy(newNodeData->agentSecret, address->agentSecret, MAX_SECRET_LENGTH);
      if (address->sshPort != 0)
      {
         _tcslcpy(newNodeData->sshLogin, address->sshCredentials.login, MAX_USER_NAME);
         _tcslcpy(newNodeData->sshPassword, address->sshCredentials.password, MAX_PASSWORD);
         newNodeData->sshKeyId = address->sshCredentials.keyId;
         newNodeData->sshPort = address->sshPort;
      }
      CreateDiscoveredNode(newNodeData);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("ProcessDiscoveredAddressStage2(%s): address discarded"), address->ipAddr.toString().cstr());
   }
}

/**
 * Discovered address poller thread (poll new nodes and put them into the database)
 */
void DiscoveredAddressPoller()
{
   ThreadSetName("NodePoller");
   nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 1, _T("Discovered address poller started"));

   while(!IsShutdownInProgress())
   {
      DiscoveredAddress *address = s_nodePollerQueue.getOrBlock();
      if (address == INVALID_POINTER_VALUE)
         break;   // Shutdown indicator received

      wchar_t ipAddrText[MAX_IP_ADDR_TEXT_LEN];
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("DiscoveredAddressPoller: stage 2 processing for address %s/%d in zone %d (source type %s, source node [%u])"),
               address->ipAddr.toString(ipAddrText), address->ipAddr.getMaskBits(), address->zoneUIN,
               s_discoveredAddrSourceTypeAsText[address->sourceType], address->sourceNodeId);

      ProcessDiscoveredAddressStage2(address);
      RemoveActiveAddress(address->zoneUIN, address->ipAddr);
      delete address;
   }
   nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 1, _T("Discovered address poller thread terminated"));
}

/**
 * Process discovered address (stage 1)
 */
static void ProcessDiscoveredAddressStage1(DiscoveredAddress *address)
{
   if (IsShutdownInProgress() || !IsActiveAddress(address->zoneUIN, address->ipAddr))
   {
      delete address;
      return;
   }

   if (AcceptNewNodeStage1(address))
   {
      // queue for stage 2
      s_nodePollerQueue.put(address);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("ProcessDiscoveredAddressStage1(%s): address discarded"), address->ipAddr.toString().cstr());
      RemoveActiveAddress(address->zoneUIN, address->ipAddr);
      delete address;
   }
}

/**
 * Enqueue discovered address for processing
 */
void EnqueueDiscoveredAddress(DiscoveredAddress *address)
{
   AddActiveAddress(address->zoneUIN, address->ipAddr);
   ThreadPoolExecute(g_discoveryThreadPool, ProcessDiscoveredAddressStage1, address);
}

/**
 * Check potential new node from syslog, SNMP trap, or address range scan
 */
void CheckPotentialNode(const InetAddress& ipAddr, int32_t zoneUIN, DiscoveredAddressSourceType sourceType, uint32_t sourceNodeId)
{
   TCHAR buffer[MAX_IP_ADDR_TEXT_LEN];
   nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("Checking address %s in zone %d (source: %s)"),
            ipAddr.toString(buffer), zoneUIN, s_discoveredAddrSourceTypeAsText[sourceType]);
   if (!ipAddr.isValid() || ipAddr.isBroadcast() || ipAddr.isLoopback() || ipAddr.isMulticast())
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("Potential node %s int zone %d rejected (IP address is not a valid unicast address)"), ipAddr.toString(buffer), zoneUIN);
      return;
   }

   shared_ptr<Node> curr = FindNodeByIP(zoneUIN, ipAddr);
   if (curr != nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("Potential node %s in zone %d rejected (IP address already known at node %s [%u])"),
               ipAddr.toString(buffer), zoneUIN, curr->getName(), curr->getId());
      return;
   }

   uint32_t clusterId;
   bool isResource;
   if (IsClusterIP(zoneUIN, ipAddr, &clusterId, &isResource))
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, L"Potential node %s in zone %d rejected (IP address is known as %s address in cluster \"%s\" [%u])",
         ipAddr.toString(buffer), zoneUIN, isResource ? _T("resource") : _T("synchronization"), GetObjectName(clusterId, L"unknown"), clusterId);
      return;
   }

   if (IsActiveAddress(zoneUIN, ipAddr))
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("Potential node %s in zone %d rejected (IP address already queued for polling)"), ipAddr.toString(buffer), zoneUIN);
      return;
   }

   shared_ptr<Subnet> subnet = FindSubnetForNode(zoneUIN, ipAddr);
   if (subnet != nullptr)
   {
      if (!subnet->getIpAddress().equals(ipAddr) && !ipAddr.isSubnetBroadcast(subnet->getIpAddress().getMaskBits()))
      {
         DiscoveredAddress *addressInfo = new DiscoveredAddress(ipAddr, zoneUIN, sourceNodeId, sourceType);
         addressInfo->ipAddr.setMaskBits(subnet->getIpAddress().getMaskBits());
         nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 5, _T("New node queued: %s/%d"), addressInfo->ipAddr.toString(buffer), addressInfo->ipAddr.getMaskBits());
         EnqueueDiscoveredAddress(addressInfo);
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("Potential node %s rejected (IP address is a base or broadcast address of existing subnet)"), ipAddr.toString(buffer));
      }
   }
   else
   {
      DiscoveredAddress *addressInfo = new DiscoveredAddress(ipAddr, zoneUIN, sourceNodeId, sourceType);
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 5, _T("New node queued: %s/%d"), addressInfo->ipAddr.toString(buffer), addressInfo->ipAddr.getMaskBits());
      EnqueueDiscoveredAddress(addressInfo);
   }
}

/**
 * Check potential new node from ARP cache or routing table
 */
static void CheckPotentialNode(Node *node, const InetAddress& ipAddr, uint32_t ifIndex, const MacAddress& macAddr, DiscoveredAddressSourceType sourceType, uint32_t sourceNodeId)
{
   if (IsShutdownInProgress())
      return;

   wchar_t buffer[MAX_IP_ADDR_TEXT_LEN];
   nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("Checking potential node %s at %s:%u (source: %s)"),
            ipAddr.toString(buffer), node->getName(), ifIndex, s_discoveredAddrSourceTypeAsText[sourceType]);
   if (!ipAddr.isValidUnicast())
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("Potential node %s rejected (IP address is not a valid unicast address)"), ipAddr.toString(buffer));
      return;
   }

   uint32_t clusterId;
   bool isResource;
   if (IsClusterIP(node->getZoneUIN(), ipAddr, &clusterId, &isResource))
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, L"Potential node %s rejected (IP address is known as %s address in cluster \"%s\" [%u])",
         ipAddr.toString(buffer), isResource ? _T("resource") : _T("synchronization"), GetObjectName(clusterId, L"unknown"), clusterId);
      return;
   }

   if (IsActiveAddress(node->getZoneUIN(), ipAddr))
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("Potential node %s rejected (IP address already queued for polling)"), ipAddr.toString(buffer));
      return;
   }

   shared_ptr<Node> curr = FindNodeByIP(node->getZoneUIN(), ipAddr);
   if (curr != nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("IP address %s found at node %s [%u], re-checking interface list"), ipAddr.toString(buffer), curr->getName(), curr->getId());
      InterfaceList *ifList = curr->getInterfaceList();
      if (ifList != nullptr)
      {
         if (ifList->findByIpAddress(ipAddr) != nullptr)
         {
            // Confirmed with current interface list
            nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("Potential node %s rejected (IP address already known at node %s [%u])"), ipAddr.toString(buffer), curr->getName(), curr->getId());

            // Check for duplicate IP address
            shared_ptr<Interface> iface = curr->findInterfaceByIP(ipAddr);
            if ((iface != nullptr) && macAddr.isValid() && iface->getMacAddress().isValid() && !iface->getMacAddress().equals(macAddr))
            {
               EventBuilder(EVENT_DUPLICATE_IP_ADDRESS, g_dwMgmtNode)
                  .param(_T("ipAddress"), ipAddr)
                  .param(_T("knownNodeId"), curr->getId())
                  .param(_T("knownNodeName"), curr->getName())
                  .param(_T("knownInterfaceName"), iface->getName())
                  .param(_T("knownMacAddress"), iface->getMacAddress())
                  .param(_T("discoveredMacAddress"), macAddr)
                  .param(_T("discoverySourceNodeId"), node->getId())
                  .param(_T("discoverySourceNodeName"), node->getName())
                  .param(_T("discoveryDataSource"), s_discoveredAddrSourceTypeAsText[sourceType])
                  .post();
            }
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("IP address %s already known at node %s [%u] but not found in actual interface list, forcing configuration poll"), ipAddr.toString(buffer), curr->getName(), curr->getId());
            curr->forceConfigurationPoll();
         }
         delete ifList;
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("Potential node %s rejected (IP address already known at node %s [%u] but interface list for that node is unavailable)"), ipAddr.toString(buffer), curr->getName(), curr->getId());
      }
      return;
   }

   shared_ptr<Interface> iface = node->findInterfaceByIndex(ifIndex);
   if ((iface != nullptr) && !iface->isExcludedFromTopology())
   {
      // Check if given IP address is not configured on source interface itself
      // Some Juniper devices can report addresses from internal interfaces in ARP cache
      if (!iface->hasIpAddress(ipAddr))
      {
         InetAddress interfaceAddress = iface->findSameSubnetAddress(ipAddr);
         if (interfaceAddress.isValidUnicast())
         {
            nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("Interface found: %s [%u] addr=%s/%d ifIndex=%u"),
                  iface->getName(), iface->getId(), interfaceAddress.toString(buffer), interfaceAddress.getMaskBits(), iface->getIfIndex());
            if (!ipAddr.isSubnetBroadcast(interfaceAddress.getMaskBits()))
            {
               DiscoveredAddress *addressInfo = new DiscoveredAddress(ipAddr, node->getZoneUIN(), sourceNodeId, sourceType);
               addressInfo->ipAddr.setMaskBits(interfaceAddress.getMaskBits());
               addressInfo->macAddr = macAddr;
               nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 5, _T("New node queued: %s/%d"), addressInfo->ipAddr.toString(buffer), addressInfo->ipAddr.getMaskBits());
               EnqueueDiscoveredAddress(addressInfo);
            }
            else
            {
               nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("Potential node %s rejected - broadcast/multicast address"), ipAddr.toString(buffer));
            }
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("Interface object found but IP address not found"));
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("IP address %s found on local interface %s [%u]"), ipAddr.toString(buffer), iface->getName(), iface->getId());
      }
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("Interface with index %u not found or marked as excluded from network topology"), ifIndex);
   }
}

/**
 * Check host route
 * Host will be added if it is directly connected
 */
static void CheckHostRoute(Node *node, const ROUTE *route)
{
   TCHAR buffer[MAX_IP_ADDR_TEXT_LEN];
   nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("Checking host route %s at %d"), route->destination.toString(buffer), route->ifIndex);
   shared_ptr<Interface> iface = node->findInterfaceByIndex(route->ifIndex);
   if ((iface != nullptr) && iface->findSameSubnetAddress(route->destination).isValidUnicast())
   {
      CheckPotentialNode(node, route->destination, route->ifIndex, MacAddress::NONE, DA_SRC_ROUTING_TABLE, node->getId());
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("Interface object not found for host route"));
   }
}

/**
 * Discovery poll
 */
void Node::discoveryPoll(PollerInfo *poller, ClientSession *session, uint32_t rqId)
{
   poller->setStatus(_T("wait for lock"));
   pollerLock(discovery);

   m_pollRequestor = session;
   m_pollRequestId = rqId;

   if (isDeleteInitiated() || IsShutdownInProgress())
   {
      sendPollerMsg(L"Node is being deleted or server shutdown is in progress, aborting discovery poll\r\n");
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, L"Discovery poll of node %s (%s) in zone %d aborted", m_name, m_ipAddress.toString().cstr(), m_zoneUIN);
      pollerUnlock();
      return;
   }

   nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, L"Starting discovery poll of node %s (%s) in zone %d", m_name, m_ipAddress.toString().cstr(), m_zoneUIN);
   sendPollerMsg(L"Starting discovery poll\r\n");

   // Retrieve and analyze node's ARP cache
   sendPollerMsg(L"Reading ARP cache...\r\n");
   shared_ptr<ArpCache> arpCache = getArpCache(true);
   if (arpCache != nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 5, L"Discovery poll of node %s (%s) - read %d entries from ARP cache", m_name, m_ipAddress.toString().cstr(), arpCache->size());
      sendPollerMsg(L"Read %d entries from ARP cache\r\n", arpCache->size());
      for(int i = 0; i < arpCache->size(); i++)
      {
         const ArpEntry *e = arpCache->get(i);
         if (!e->macAddr.isBroadcast() && !e->macAddr.isNull())
         {
            sendPollerMsg(L"   Checking ARP cache entry: IP=%s MAC=%s ifIndex=%u\r\n", e->ipAddr.toString().cstr(), e->macAddr.toString().cstr(), e->ifIndex);
            CheckPotentialNode(this, e->ipAddr, e->ifIndex, e->macAddr, DA_SRC_ARP_CACHE, m_id);
         }
      }
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 5, L"Discovery poll of node %s (%s) - failed to read ARP cache", m_name, m_ipAddress.toString().cstr());
      sendPollerMsg(L"Failed to read ARP cache\r\n");
   }

   if (isDeleteInitiated() || IsShutdownInProgress())
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, L"Discovery poll of node %s (%s) in zone %d aborted", m_name, m_ipAddress.toString().cstr(), m_zoneUIN);
      pollerUnlock();
      return;
   }

   // Retrieve and analyze node's routing table
   if (!(getFlags() & NF_DISABLE_ROUTE_POLL))
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 5, _T("Discovery poll of node %s (%s) - reading routing table"), m_name, m_ipAddress.toString().cstr());
      sendPollerMsg(L"Reading routing table...\r\n");
      shared_ptr<RoutingTable> rt = getRoutingTable();
      if (rt != nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 5, _T("Discovery poll of node %s (%s) - read %d entries from routing table"), m_name, m_ipAddress.toString().cstr(), rt->size());
         sendPollerMsg(L"Read %d entries from routing table\r\n", rt->size());
         for(int i = 0; i < rt->size(); i++)
         {
            const ROUTE *route = rt->get(i);
            if (route->nextHop.isValidUnicast())
            {
               sendPollerMsg(L"   Checking gateway address %s\r\n", route->nextHop.toString().cstr());
               CheckPotentialNode(this, route->nextHop, route->ifIndex, MacAddress::NONE, DA_SRC_ROUTING_TABLE, getId());
               if (route->destination.isValidUnicast() && (route->destination.getHostBits() == 0))
               {
                  sendPollerMsg(L"   Checking host route %s\r\n", route->destination.toString().cstr());
                  CheckHostRoute(this, route);
               }
            }
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 5, _T("Discovery poll of node %s (%s) - failed to read routing table"), m_name, m_ipAddress.toString().cstr());
         sendPollerMsg(L"Failed to read routing table\r\n");
      }
   }

   executeHookScript(L"DiscoveryPoll");

   nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, L"Finished discovery poll of node %s (%s)", m_name, m_ipAddress.toString().cstr());
   sendPollerMsg(L"Finished discovery poll\r\n");
   pollerUnlock();
}

/**
 * Callback for address range scan
 */
void RangeScanCallback(const InetAddress& addr, int32_t zoneUIN, const Node *proxy, uint32_t rtt, const TCHAR *proto, ServerConsole *console, void *context)
{
   TCHAR ipAddrText[MAX_IP_ADDR_TEXT_LEN];
   if (proxy != nullptr)
   {
      ConsoleDebugPrintf(console, DEBUG_TAG_DISCOVERY, 5, _T("Active discovery - node %s responded to %s probe via proxy %s [%u]"),
            addr.toString(ipAddrText), proto, proxy->getName(), proxy->getId());
      CheckPotentialNode(addr, zoneUIN, DA_SRC_ACTIVE_DISCOVERY, proxy->getId());
   }
   else
   {
      ConsoleDebugPrintf(console, DEBUG_TAG_DISCOVERY, 5, _T("Active discovery - node %s responded to %s probe"), addr.toString(ipAddrText), proto);
      CheckPotentialNode(addr, zoneUIN, DA_SRC_ACTIVE_DISCOVERY, 0);
   }
}

/**
 * Callback data for ScanAddressRangeSNMP and ScanAddressRangeTCP
 */
struct ScanCallbackData
{
   void (*callback)(const InetAddress&, int32_t, const Node*, uint32_t, const TCHAR*, ServerConsole*, void*);
   ServerConsole *console;
   void *context;
   const TCHAR *protocol;
};

/**
 * Callback for ScanAddressRangeSNMP and ScanAddressRangeTCP
 */
static void ScanCallback(const InetAddress& addr, uint32_t rtt, void *context)
{
   auto cd = static_cast<ScanCallbackData*>(context);
   cd->callback(addr, 0, nullptr, rtt, cd->protocol, cd->console, cd->context);
}

/**
 * Scan address range via SNMP
 */
static void ScanAddressRangeSNMP(const InetAddress& from, const InetAddress& to, uint16_t port, SNMP_Version snmpVersion, const char *community,
      void (*callback)(const InetAddress&, int32_t, const Node*, uint32_t, const TCHAR*, ServerConsole*, void*), ServerConsole *console, void *context)
{
   ScanCallbackData cd;
   cd.callback = callback;
   cd.console = console;
   cd.context = context;
   cd.protocol = _T("SNMP");
   SnmpScanAddressRange(from, to, port, snmpVersion, community, ScanCallback, &cd);
}

/**
 * Scan address range via SNMP proxy
 */
static uint32_t ScanAddressRangeSNMPProxy(AgentConnection *conn, uint32_t from, uint32_t to, uint16_t port, SNMP_Version snmpVersion, const TCHAR *community,
      void (*callback)(const InetAddress&, int32_t, const Node*, uint32_t, const TCHAR*, ServerConsole*, void*), int32_t zoneUIN, const Node *proxy, ServerConsole *console, void *context)
{
   TCHAR request[1024], ipAddr1[MAX_IP_ADDR_TEXT_LEN], ipAddr2[MAX_IP_ADDR_TEXT_LEN];
   _sntprintf(request, 1024, _T("SNMP.ScanAddressRange(%s,%s,%u,%d,\"%s\")"), IpToStr(from, ipAddr1), IpToStr(to, ipAddr2), port, snmpVersion, CHECK_NULL_EX(community));
   StringList *list;
   uint32_t rcc = conn->getList(request, &list);
   if (rcc == ERR_SUCCESS)
   {
      for(int i = 0; i < list->size(); i++)
      {
         callback(InetAddress::parse(list->get(i)), zoneUIN, proxy, 0, _T("SNMP"), console, context);
      }
      delete list;
   }
   return rcc;
}

/**
 * Scan address range via TCP
 */
static void ScanAddressRangeTCP(const InetAddress& from, const InetAddress& to, uint16_t port,
      void (*callback)(const InetAddress&, int32_t, const Node*, uint32_t, const TCHAR*, ServerConsole*, void*), ServerConsole *console, void *context)
{
   ScanCallbackData cd;
   cd.callback = callback;
   cd.console = console;
   cd.context = context;
   cd.protocol = _T("TCP");
   TCPScanAddressRange(from, to, port, ScanCallback, &cd);
}

/**
 * Scan address range via TCP proxy
 */
static uint32_t ScanAddressRangeTCPProxy(AgentConnection *conn, uint32_t from, uint32_t to, uint16_t port,
      void (*callback)(const InetAddress&, int32_t, const Node*, uint32_t, const TCHAR*, ServerConsole*, void*), int32_t zoneUIN, const Node *proxy, ServerConsole *console, void *context)
{
   TCHAR request[1024], ipAddr1[MAX_IP_ADDR_TEXT_LEN], ipAddr2[MAX_IP_ADDR_TEXT_LEN];
   _sntprintf(request, 1024, _T("TCP.ScanAddressRange(%s,%s,%u)"), IpToStr(from, ipAddr1), IpToStr(to, ipAddr2), port);
   StringList *list;
   uint32_t rcc = conn->getList(request, &list);
   if (rcc == ERR_SUCCESS)
   {
      for(int i = 0; i < list->size(); i++)
      {
         callback(InetAddress::parse(list->get(i)), zoneUIN, proxy, 0, _T("TCP"), console, context);
      }
      delete list;
   }
   return rcc;
}

/**
 * Scan address range via ICMP proxy
 */
static uint32_t ScanAddressRangeICMPProxy(AgentConnection *conn, uint32_t from, uint32_t to,
      void (*callback)(const InetAddress&, int32_t, const Node*, uint32_t, const TCHAR*, ServerConsole*, void*), int32_t zoneUIN, const Node *proxy, ServerConsole *console, void *context)
{
   TCHAR request[256], ipAddr1[MAX_IP_ADDR_TEXT_LEN], ipAddr2[MAX_IP_ADDR_TEXT_LEN];
   _sntprintf(request, 256, _T("ICMP.ScanRange(%s,%s,,true)"), IpToStr(from, ipAddr1), IpToStr(to, ipAddr2));
   StringList *list;
   uint32_t rcc = conn->getList(request, &list);
   if (rcc == ERR_SUCCESS)
   {
      for(int i = 0; i < list->size(); i++)
      {
         // Agents starting with 6.2 return "address rtt" when RTT output is requested,
         // older agents ignore the request and return just the address
         const TCHAR *line = list->get(i);
         uint32_t rtt = 0;
         TCHAR addrText[64];
         const TCHAR *separator = _tcschr(line, _T(' '));
         if (separator != nullptr)
         {
            size_t len = std::min(static_cast<size_t>(separator - line), static_cast<size_t>(63));
            memcpy(addrText, line, len * sizeof(TCHAR));
            addrText[len] = 0;
            rtt = _tcstoul(separator + 1, nullptr, 10);
            line = addrText;
         }
         callback(InetAddress::parse(line), zoneUIN, proxy, rtt, _T("ICMP"), console, context);
      }
      delete list;
   }
   return rcc;
}

/**
 * Calculate agent connection command timeout for proxy range scan commands.
 * Scan commands execute synchronously on the agent, and TCP scan probes
 * addresses in sub-blocks of 32 with up to 2 seconds wait for each (see
 * TCPScanAddressRange in libnxagent), so a single command covering a large
 * block can run for more than a minute. Fixed margin covers ICMP and SNMP
 * scan commands (which are bounded by per-address pacing plus response
 * timeout) and NXCP round-trip overhead.
 */
static uint32_t ProxyScanCommandTimeout(uint32_t blockSize)
{
   return (blockSize + 31) / 32 * 2000 + 5000;
}

/**
 * Check given address range with ICMP ping for new nodes
 */
void CheckRange(const InetAddressListElement& range, void (*callback)(const InetAddress&, int32_t, const Node*, uint32_t, const TCHAR*, ServerConsole*, void*), ServerConsole *console, void *context)
{
   if (range.getBaseAddress().getFamily() != AF_INET)
   {
      ConsoleDebugPrintf(console, DEBUG_TAG_DISCOVERY, 4, _T("Active discovery on range %s skipped - only IPv4 ranges supported"), (const TCHAR *)range.toString());
      return;
   }

   uint32_t from = range.getBaseAddress().getAddressV4();
   uint32_t to;
   if (range.getType() == InetAddressListElement_SUBNET)
   {
      from++;
      to = range.getBaseAddress().getSubnetBroadcast().getAddressV4() - 1;
   }
   else
   {
      to = range.getEndAddress().getAddressV4();
   }

   if (from > to)
   {
      ConsoleDebugPrintf(console, DEBUG_TAG_DISCOVERY, 4, _T("Invalid address range %s"), range.toString().cstr());
      return;
   }

   uint32_t blockSize = ConfigReadULong(_T("NetworkDiscovery.ActiveDiscovery.BlockSize"), 1024);
   uint32_t interBlockDelay = ConfigReadULong(_T("NetworkDiscovery.ActiveDiscovery.InterBlockDelay"), 0);
   bool snmpScanEnabled = ConfigReadBoolean(_T("NetworkDiscovery.ActiveDiscovery.EnableSNMPProbing"), true);
   bool tcpScanEnabled = ConfigReadBoolean(_T("NetworkDiscovery.ActiveDiscovery.EnableTCPProbing"), false);

   bool useProxy = (range.getProxyId() != 0);
   if (!useProxy && (range.getZoneUIN() != 0))
   {
      shared_ptr<Zone> zone = FindZoneByUIN(range.getZoneUIN());
      if (zone == nullptr)
      {
         ConsoleDebugPrintf(console, DEBUG_TAG_DISCOVERY, 4, _T("Invalid zone UIN for address range %s"), range.toString().cstr());
         return;
      }
      if (!zone->getAllProxyNodes().isEmpty())
      {
         useProxy = true;
      }
      else
      {
         ConsoleDebugPrintf(console, DEBUG_TAG_DISCOVERY, 4, _T("Zone %s [uin=%d] has no proxy nodes configured, using direct scan for address range %s"),
               zone->getName(), zone->getUIN(), range.toString().cstr());
      }
   }

   if (useProxy)
   {
      uint32_t proxyId;
      if (range.getProxyId() != 0)
      {
         proxyId = range.getProxyId();
      }
      else
      {
         shared_ptr<Zone> zone = FindZoneByUIN(range.getZoneUIN());
         if (zone == nullptr)
         {
            ConsoleDebugPrintf(console, DEBUG_TAG_DISCOVERY, 4, _T("Invalid zone UIN for address range %s"), range.toString().cstr());
            return;
         }
         proxyId = zone->getProxyNodeId(nullptr);
      }

      shared_ptr<Node> proxy = static_pointer_cast<Node>(FindObjectById(proxyId, OBJECT_NODE));
      if (proxy == nullptr)
      {
         ConsoleDebugPrintf(console, DEBUG_TAG_DISCOVERY, 4, _T("Cannot find zone proxy node for address range %s"), range.toString().cstr());
         return;
      }

      shared_ptr<AgentConnectionEx> conn = proxy->createAgentConnection();
      if (conn == nullptr)
      {
         ConsoleDebugPrintf(console, DEBUG_TAG_DISCOVERY, 4, _T("Cannot connect to proxy agent for address range %s"), (const TCHAR *)range.toString());
         return;
      }
      conn->setCommandTimeout(ProxyScanCommandTimeout(std::min(blockSize, to - from + 1)));

      TCHAR ipAddr1[MAX_IP_ADDR_TEXT_LEN], ipAddr2[MAX_IP_ADDR_TEXT_LEN], rangeText[128];
      _sntprintf(rangeText, 128, _T("%s - %s"), IpToStr(from, ipAddr1), IpToStr(to, ipAddr2));
      ConsoleDebugPrintf(console, DEBUG_TAG_DISCOVERY, 4, _T("Starting active discovery check on range %s via proxy %s [%u]"), rangeText, proxy->getName(), proxy->getId());
      while((from <= to) && !IsShutdownInProgress())
      {
         if (interBlockDelay > 0)
            ThreadSleepMs(interBlockDelay);

         uint32_t blockEndAddr = std::min(to, from + blockSize - 1);

         ScanAddressRangeICMPProxy(conn.get(), from, blockEndAddr, callback, range.getZoneUIN(), proxy.get(), console, nullptr);

         if (snmpScanEnabled && !IsShutdownInProgress())
         {
            ConsoleDebugPrintf(console, DEBUG_TAG_DISCOVERY, 5, _T("Starting SNMP check on range %s - %s via proxy %s [%u] (snmp=%s tcp=%s bs=%u delay=%u)"),
                  IpToStr(from, ipAddr1), IpToStr(to, ipAddr2), proxy->getName(), proxy->getId(), BooleanToString(snmpScanEnabled),
                  BooleanToString(tcpScanEnabled), blockSize, interBlockDelay);
            IntegerArray<uint16_t> ports = GetWellKnownPorts(_T("snmp"), 0);

            // Filter blocked UDP ports
            IntegerArray<uint16_t> blockedUdpPorts;
            shared_ptr<Zone> stopListZone = FindZoneByUIN(range.getZoneUIN());
            if (stopListZone != nullptr)
               stopListZone->getEffectivePortStopList(nullptr, &blockedUdpPorts);
            for (int i = ports.size() - 1; i >= 0; i--)
            {
               if (blockedUdpPorts.contains(ports.get(i)))
                  ports.remove(i);
            }

            unique_ptr<StringList> communities = SnmpGetKnownCommunities(0);
            for(int i = 0; (i < ports.size()) && !IsShutdownInProgress(); i++)
            {
               uint16_t port = ports.get(i);
               for(int j = 0; (j < communities->size()) && !IsShutdownInProgress(); j++)
               {
                  const TCHAR *community = communities->get(j);
                  ScanAddressRangeSNMPProxy(conn.get(), from, blockEndAddr, port, SNMP_VERSION_1, community, callback, range.getZoneUIN(), proxy.get(), console, nullptr);
                  ScanAddressRangeSNMPProxy(conn.get(), from, blockEndAddr, port, SNMP_VERSION_2C, community, callback, range.getZoneUIN(), proxy.get(), console, nullptr);
               }
               ScanAddressRangeSNMPProxy(conn.get(), from, blockEndAddr, port, SNMP_VERSION_3, nullptr, callback, range.getZoneUIN(), proxy.get(), console, nullptr);
            }
         }

         if (tcpScanEnabled && !IsShutdownInProgress())
         {
            ConsoleDebugPrintf(console, DEBUG_TAG_DISCOVERY, 5, _T("Starting TCP check on range %s - %s via proxy %s [%u]"),
                  IpToStr(from, ipAddr1), IpToStr(to, ipAddr2), proxy->getName(), proxy->getId());
            IntegerArray<uint16_t> ports = GetWellKnownPorts(_T("agent"), 0);
            ports.addAll(GetWellKnownPorts(_T("ssh"), 0));
            ports.add(ETHERNET_IP_DEFAULT_PORT);

            // Filter blocked TCP ports
            shared_ptr<Zone> zone = FindZoneByUIN(range.getZoneUIN());
            if (zone != nullptr)
            {
               IntegerArray<uint16_t> blockedTcpPorts;
               zone->getEffectivePortStopList(&blockedTcpPorts, nullptr);
               for (int i = ports.size() - 1; i >= 0; i--)
               {
                  if (blockedTcpPorts.contains(ports.get(i)))
                     ports.remove(i);
               }
            }

            for(int i = 0; (i < ports.size()) && !IsShutdownInProgress(); i++)
               ScanAddressRangeTCPProxy(conn.get(), from, blockEndAddr, ports.get(i), callback, range.getZoneUIN(), proxy.get(), console, nullptr);
         }

         from += blockSize;
      }
      ConsoleDebugPrintf(console, DEBUG_TAG_DISCOVERY, 4, _T("Finished active discovery check on range %s via proxy %s [%u]"), rangeText, proxy->getName(), proxy->getId());
   }
   else
   {
      wchar_t ipAddr1[MAX_IP_ADDR_TEXT_LEN], ipAddr2[MAX_IP_ADDR_TEXT_LEN], rangeText[128];
      nx_swprintf(rangeText, 128, L"%s - %s", IpToStr(from, ipAddr1), IpToStr(to, ipAddr2));
      ConsoleDebugPrintf(console, DEBUG_TAG_DISCOVERY, 4, L"Starting active discovery check on range %s (snmp=%s tcp=%s bs=%u delay=%u)",
            rangeText, BooleanToString(snmpScanEnabled), BooleanToString(tcpScanEnabled), blockSize, interBlockDelay);
      while((from <= to) && !IsShutdownInProgress())
      {
         if (interBlockDelay > 0)
            ThreadSleepMs(interBlockDelay);

         uint32_t blockEndAddr = std::min(to, from + blockSize - 1);

         ScanAddressRangeICMP(from, blockEndAddr, callback, console, nullptr);

         if (snmpScanEnabled && !IsShutdownInProgress())
         {
            ConsoleDebugPrintf(console, DEBUG_TAG_DISCOVERY, 5, _T("Starting SNMP check on range %s - %s"), IpToStr(from, ipAddr1), IpToStr(blockEndAddr, ipAddr2));
            IntegerArray<uint16_t> ports = GetWellKnownPorts(_T("snmp"), 0);

            // Filter blocked UDP ports
            IntegerArray<uint16_t> blockedUdpPorts;
            shared_ptr<Zone> stopListZone = FindZoneByUIN(range.getZoneUIN());
            if (stopListZone != nullptr)
               stopListZone->getEffectivePortStopList(nullptr, &blockedUdpPorts);
            for (int i = ports.size() - 1; i >= 0; i--)
            {
               if (blockedUdpPorts.contains(ports.get(i)))
                  ports.remove(i);
            }

            unique_ptr<StringList> communities = SnmpGetKnownCommunities(0);
            for(int i = 0; (i < ports.size()) && !IsShutdownInProgress(); i++)
            {
               uint16_t port = ports.get(i);
               for(int j = 0; (j < communities->size()) && !IsShutdownInProgress(); j++)
               {
                  char community[256];
                  wchar_to_mb(communities->get(j), -1, community, 256);
                  ScanAddressRangeSNMP(from, blockEndAddr, port, SNMP_VERSION_1, community, callback, console, nullptr);
                  ScanAddressRangeSNMP(from, blockEndAddr, port, SNMP_VERSION_2C, community, callback, console, nullptr);
               }
               ScanAddressRangeSNMP(from, blockEndAddr, port, SNMP_VERSION_3, nullptr, callback, console, nullptr);
            }
         }

         if (tcpScanEnabled && !IsShutdownInProgress())
         {
            ConsoleDebugPrintf(console, DEBUG_TAG_DISCOVERY, 5, _T("Starting TCP check on range %s - %s"), IpToStr(from, ipAddr1), IpToStr(blockEndAddr, ipAddr2));
            IntegerArray<uint16_t> ports = GetWellKnownPorts(_T("agent"), 0);
            ports.addAll(GetWellKnownPorts(_T("ssh"), 0));
            ports.add(ETHERNET_IP_DEFAULT_PORT);

            // Filter blocked TCP ports
            shared_ptr<Zone> zone = FindZoneByUIN(range.getZoneUIN());
            if (zone != nullptr)
            {
               IntegerArray<uint16_t> blockedTcpPorts;
               zone->getEffectivePortStopList(&blockedTcpPorts, nullptr);
               for (int i = ports.size() - 1; i >= 0; i--)
               {
                  if (blockedTcpPorts.contains(ports.get(i)))
                     ports.remove(i);
               }
            }

            for(int i = 0; (i < ports.size()) && !IsShutdownInProgress(); i++)
               ScanAddressRangeTCP(from, blockEndAddr, ports.get(i), callback, console, nullptr);
         }

         from += blockSize;
      }
      ConsoleDebugPrintf(console, DEBUG_TAG_DISCOVERY, 4, _T("Finished active discovery check on range %s"), rangeText);
   }
}

/**
 * Active discovery thread wakeup condition
 */
static Condition s_activeDiscoveryWakeup(false);

/**
 * Active discovery state
 */
static bool s_activeDiscoveryActive = false;
static ObjectArray<InetAddressListElement> *s_activeDiscoveryRanges = nullptr;
static int s_activeDiscoveryCurrentRange = 0;
static Mutex s_activeDiscoveryStateLock(MutexType::FAST);

/**
 * Active discovery poller thread
 */
void ActiveDiscoveryPoller()
{
   ThreadSetName("ActiveDiscovery");

   nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 2, _T("Active discovery thread started"));

   time_t lastRun = 0;
   uint32_t sleepTime = 60000;

   // Main loop
   WaitForServerStartupCompletion();
   while(!IsShutdownInProgress())
   {
      s_activeDiscoveryWakeup.wait(sleepTime);
      if (IsShutdownInProgress())
         break;

      if (!(g_flags & AF_ACTIVE_NETWORK_DISCOVERY))
      {
         sleepTime = INFINITE;
         continue;
      }

      time_t now = time(nullptr);

      uint32_t interval = ConfigReadULong(_T("NetworkDiscovery.ActiveDiscovery.Interval"), 7200);
      if (interval != 0)
      {
         if (static_cast<uint32_t>(now - lastRun) < interval)
         {
            sleepTime = (interval - static_cast<uint32_t>(lastRun - now)) * 1000;
            continue;
         }
      }
      else
      {
         TCHAR schedule[256];
         ConfigReadStr(_T("NetworkDiscovery.ActiveDiscovery.Schedule"), schedule, 256, _T(""));
         if (schedule[0] != 0)
         {
#if HAVE_LOCALTIME_R
            struct tm tmbuffer;
            localtime_r(&now, &tmbuffer);
            struct tm *ltm = &tmbuffer;
#else
            struct tm *ltm = localtime(&now);
#endif
            if (!MatchSchedule(schedule, nullptr, ltm, now))
            {
               sleepTime = 60000;
               continue;
            }
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("Empty active discovery schedule"));
            sleepTime = INFINITE;
            continue;
         }
      }

      s_activeDiscoveryStateLock.lock();
      s_activeDiscoveryRanges = LoadServerAddressList(1);
      if (s_activeDiscoveryRanges != nullptr)
      {
         if (!s_activeDiscoveryRanges->isEmpty())
         {
            s_activeDiscoveryActive = true;
            s_activeDiscoveryStateLock.unlock();
            for(s_activeDiscoveryCurrentRange = 0; (s_activeDiscoveryCurrentRange < s_activeDiscoveryRanges->size()) && !IsShutdownInProgress(); s_activeDiscoveryCurrentRange++)
            {
               CheckRange(*s_activeDiscoveryRanges->get(s_activeDiscoveryCurrentRange), RangeScanCallback, nullptr, nullptr);
            }
            s_activeDiscoveryStateLock.lock();
            s_activeDiscoveryActive = false;
         }
         delete_and_null(s_activeDiscoveryRanges);
      }
      s_activeDiscoveryStateLock.unlock();

      interval = ConfigReadInt(_T("NetworkDiscovery.ActiveDiscovery.Interval"), 7200);
      sleepTime = (interval > 0) ? interval * 1000 : 60000;
   }

   nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 2, _T("Active discovery thread terminated"));
}

/**
 * Show active discovery state on server console
 */
void ShowActiveDiscoveryState(ServerConsole *console)
{
   s_activeDiscoveryStateLock.lock();
   if (s_activeDiscoveryActive)
   {
      console->print(_T("Active discovery poller is \x1b[1;32mRUNNING\x1b[0m\n"));
      console->print(_T("Address ranges:\n"));
      for(int i = 0; i < s_activeDiscoveryRanges->size(); i++)
      {
         InetAddressListElement *range = s_activeDiscoveryRanges->get(i);
         console->printf(_T("   %-36s %s\n"), range->toString().cstr(),
            (i < s_activeDiscoveryCurrentRange) ? _T("\x1b[32mcompleted\x1b[0m") : ((i == s_activeDiscoveryCurrentRange) ? _T("\x1b[33mprocessing\x1b[0m") : _T("\x1b[36mpending\x1b[0m")));
      }
   }
   else
   {
      console->print(_T("Active discovery poller is \x1b[1;34mSLEEPING\x1b[0m\n"));
   }
   s_activeDiscoveryStateLock.unlock();
}

/**
 * Check if active discovery is currently running
 */
bool IsActiveDiscoveryRunning()
{
   return s_activeDiscoveryActive;
}

/**
 * Get textual description of active discovery range currently being processed (empty string if none)
 */
String GetCurrentActiveDiscoveryRange()
{
   s_activeDiscoveryStateLock.lock();
   String range = s_activeDiscoveryActive ? s_activeDiscoveryRanges->get(s_activeDiscoveryCurrentRange)->toString() : String();
   s_activeDiscoveryStateLock.unlock();
   return range;
}

/**
 * Clear discovery poller queue
 */
static void ClearDiscoveryPollerQueue()
{
   s_nodePollerQueue.clear();
   s_processingList.clear();
}

/**
 * Reset discovery poller after configuration change
 */
void ResetDiscoveryPoller()
{
   ClearDiscoveryPollerQueue();

   // Reload discovery parameters
   g_discoveryPollingInterval = ConfigReadInt(_T("NetworkDiscovery.PassiveDiscovery.Interval"), 900);

   switch(ConfigReadInt(_T("NetworkDiscovery.Type"), 0))
   {
      case 0:  // Disabled
         InterlockedAnd64(&g_flags, ~(AF_PASSIVE_NETWORK_DISCOVERY | AF_ACTIVE_NETWORK_DISCOVERY));
         break;
      case 1:  // Passive only
         InterlockedOr64(&g_flags, AF_PASSIVE_NETWORK_DISCOVERY);
         InterlockedAnd64(&g_flags, ~AF_ACTIVE_NETWORK_DISCOVERY);
         break;
      case 2:  // Active only
         InterlockedAnd64(&g_flags, ~AF_PASSIVE_NETWORK_DISCOVERY);
         InterlockedOr64(&g_flags, AF_ACTIVE_NETWORK_DISCOVERY);
         break;
      case 3:  // Active and passive
         InterlockedOr64(&g_flags, AF_PASSIVE_NETWORK_DISCOVERY | AF_ACTIVE_NETWORK_DISCOVERY);
         break;
      default:
         break;
   }

   if (ConfigReadBoolean(_T("NetworkDiscovery.UseSNMPTraps"), false))
      InterlockedOr64(&g_flags, AF_SNMP_TRAP_DISCOVERY);
   else
      InterlockedAnd64(&g_flags, ~AF_SNMP_TRAP_DISCOVERY);

   if (ConfigReadBoolean(_T("NetworkDiscovery.UseSyslog"), false))
      InterlockedOr64(&g_flags, AF_SYSLOG_DISCOVERY);
   else
      InterlockedAnd64(&g_flags, ~AF_SYSLOG_DISCOVERY);

   s_activeDiscoveryWakeup.set();
}

/**
 * Stop discovery poller
 */
void StopDiscoveryPoller()
{
   ClearDiscoveryPollerQueue();
   s_nodePollerQueue.put(INVALID_POINTER_VALUE);
}

/**
 * Wakeup active discovery thread
 */
void WakeupActiveDiscoveryThread()
{
   s_activeDiscoveryWakeup.set();
}

/**
 * Manual active discovery starter
 */
void StartManualActiveDiscovery(ObjectArray<InetAddressListElement> *addressList)
{
   for(int i = 0; (i < addressList->size()) && !IsShutdownInProgress(); i++)
   {
      CheckRange(*addressList->get(i), RangeScanCallback, nullptr, nullptr);
   }
   delete addressList;
}

/**
 * Internal aggregation state for a single interactive network range scan.
 * Owned by ScanNetworkRangeInteractive; referenced from per-call callback
 * contexts; protected by resultLock during record updates.
 */
namespace {

struct InteractiveScanState
{
   const InteractiveScanContext *publicCtx;
   Mutex resultLock;
   HashMap<uint32_t, InteractiveScanRecord> records;

   InteractiveScanState() : resultLock(MutexType::FAST), records(Ownership::True) { }
};

struct InteractiveScanPortContext
{
   InteractiveScanState *state;
   uint32_t protocolFlag;
   uint16_t port;
   int16_t snmpVersion;
};

} // anonymous namespace

/**
 * Merge one probe hit into the aggregated per-host record and emit a snapshot
 * of the current cumulative state. Snapshot is built under lock and passed to
 * the user-supplied emitter outside it to keep socket I/O out of the critical
 * section.
 */
static void RecordInteractiveScanHit(const InetAddress& addr, uint32_t protocolFlag, uint32_t rtt,
      uint16_t port, int16_t snmpVersion, InteractiveScanState *state)
{
   if (addr.getFamily() != AF_INET)
      return;

   uint32_t key = addr.getAddressV4();

   state->resultLock.lock();

   InteractiveScanRecord *record = state->records.get(key);
   if (record == nullptr)
   {
      record = new InteractiveScanRecord();
      shared_ptr<Node> node = FindNodeByIP(state->publicCtx->zoneUIN, addr);
      if (node != nullptr)
         record->nodeId = node->getId();
      state->records.set(key, record);
   }

   record->protocolFlags |= protocolFlag;
   if ((protocolFlag == NSCAN_HOST_REACHABLE) && (rtt != 0))
      record->rtt = rtt;
   if (protocolFlag == NSCAN_HAS_AGENT)
      record->agentPort = port;
   if (protocolFlag == NSCAN_HAS_SNMP)
      record->snmpVersion = snmpVersion;
   if ((protocolFlag == NSCAN_HAS_TCP_PORT_OPEN) && (port != 0) && !record->openTcpPorts.contains(port))
      record->openTcpPorts.add(port);

   InteractiveScanRecord snapshot;
   snapshot.protocolFlags = record->protocolFlags;
   snapshot.rtt = record->rtt;
   snapshot.nodeId = record->nodeId;
   snapshot.agentPort = record->agentPort;
   snapshot.snmpVersion = record->snmpVersion;
   for(int i = 0; i < record->openTcpPorts.size(); i++)
      snapshot.openTcpPorts.add(record->openTcpPorts.get(i));

   state->resultLock.unlock();

   if (state->publicCtx->emitter)
      state->publicCtx->emitter(addr, snapshot);
}

/**
 * Callback for ScanAddressRangeICMP (7-argument server form).
 */
static void InteractiveScanIcmpCallback(const InetAddress& addr, int32_t zoneUIN, const Node *proxy,
      uint32_t rtt, const TCHAR *proto, ServerConsole *console, void *context)
{
   RecordInteractiveScanHit(addr, NSCAN_HOST_REACHABLE, rtt, 0, 0, static_cast<InteractiveScanState*>(context));
}

/**
 * Callback for TCPScanAddressRange / SnmpScanAddressRange (3-argument form, direct).
 */
static void InteractiveScanPortCallback(const InetAddress& addr, uint32_t rtt, void *context)
{
   auto pctx = static_cast<InteractiveScanPortContext*>(context);
   RecordInteractiveScanHit(addr, pctx->protocolFlag, 0, pctx->port, pctx->snmpVersion, pctx->state);
}

/**
 * Callback for ScanAddressRangeTCPProxy / ScanAddressRangeSNMPProxy (7-argument form).
 */
static void InteractiveScanProxyPortCallback(const InetAddress& addr, int32_t zoneUIN, const Node *proxy,
      uint32_t rtt, const TCHAR *proto, ServerConsole *console, void *context)
{
   auto pctx = static_cast<InteractiveScanPortContext*>(context);
   RecordInteractiveScanHit(addr, pctx->protocolFlag, rtt, pctx->port, pctx->snmpVersion, pctx->state);
}

/**
 * Run an interactive network range scan, driving the parallel
 * ScanAddressRangeICMP / TCPScanAddressRange / SnmpScanAddressRange primitives
 * and aggregating per-host hits. The caller-supplied emitter is invoked each
 * time a host's record is updated. Designed to be transport-agnostic so that
 * it can be driven from both NXCP client sessions and the REST API.
 */
void ScanNetworkRangeInteractive(const InteractiveScanContext& context, uint32_t *reportedHosts)
{
   InteractiveScanState state;
   state.publicCtx = &context;

   auto shouldCancel = [&context]() -> bool { return context.cancelCheck && context.cancelCheck(); };

   // Resolve zone proxy if a non-default zone was requested
   shared_ptr<Node> proxy;
   shared_ptr<AgentConnectionEx> proxyConn;
   if (context.zoneUIN != 0)
   {
      shared_ptr<Zone> zone = FindZoneByUIN(context.zoneUIN);
      if (zone != nullptr)
      {
         uint32_t proxyId = zone->getProxyNodeId(nullptr);
         if (proxyId != 0)
         {
            proxy = static_pointer_cast<Node>(FindObjectById(proxyId, OBJECT_NODE));
            if (proxy != nullptr)
            {
               proxyConn = proxy->createAgentConnection();
               if (proxyConn == nullptr)
               {
                  // Zone has a configured proxy but it is unreachable - abort instead of
                  // silently falling back to a server-side scan that cannot reach the zone.
                  nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, L"ScanNetworkRangeInteractive: cannot connect to zone %d proxy node [%u]",
                        context.zoneUIN, proxyId);
                  if (context.errorReporter)
                     context.errorReporter(RCC_ZONE_PROXY_NOT_AVAILABLE);
                  if (reportedHosts != nullptr)
                     *reportedHosts = 0;
                  return;
               }
            }
            else
            {
               nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, L"ScanNetworkRangeInteractive: zone %d proxy node [%u] not found",
                     context.zoneUIN, proxyId);
               if (context.errorReporter)
                  context.errorReporter(RCC_ZONE_PROXY_NOT_AVAILABLE);
               if (reportedHosts != nullptr)
                  *reportedHosts = 0;
               return;
            }
         }
      }
   }

   if (proxy != nullptr && proxyConn != nullptr)
   {
      uint32_t from = context.startAddress.getAddressV4();
      uint32_t to = context.endAddress.getAddressV4();
      uint32_t blockSize = ConfigReadULong(L"NetworkDiscovery.ActiveDiscovery.BlockSize", 1024);
      proxyConn->setCommandTimeout(ProxyScanCommandTimeout(std::min(blockSize, to - from + 1)));

      // Track the first error per probe type reported by the proxy agent (e.g. when
      // EnableTCPProxy is not set in its configuration), so the caller can be warned
      // which probes produced incomplete results and why.
      struct ProbeError
      {
         uint32_t probe;   // probe identified by its NSCAN_* result flag
         uint32_t error;   // agent error code
      };
      ProbeError proxyScanErrors[8];
      int proxyScanErrorCount = 0;
      auto recordProxyScanResult = [&proxyScanErrors, &proxyScanErrorCount](uint32_t probe, uint32_t error)
      {
         if (error == ERR_SUCCESS)
            return;
         for(int i = 0; i < proxyScanErrorCount; i++)
            if (proxyScanErrors[i].probe == probe)
               return;
         if (proxyScanErrorCount < static_cast<int>(sizeof(proxyScanErrors) / sizeof(ProbeError)))
            proxyScanErrors[proxyScanErrorCount++] = { probe, error };
      };

      while((from <= to) && !shouldCancel())
      {
         uint32_t blockEnd = std::min(to, from + blockSize - 1);

         recordProxyScanResult(NSCAN_HOST_REACHABLE, ScanAddressRangeICMPProxy(proxyConn.get(), from, blockEnd, InteractiveScanIcmpCallback,
               context.zoneUIN, proxy.get(), nullptr, &state));

         if ((context.flags & NSCAN_PROBE_AGENT) && !shouldCancel())
         {
            InteractiveScanPortContext pctx{ &state, NSCAN_HAS_AGENT, AGENT_LISTEN_PORT, 0 };
            recordProxyScanResult(NSCAN_HAS_AGENT, ScanAddressRangeTCPProxy(proxyConn.get(), from, blockEnd, AGENT_LISTEN_PORT,
                  InteractiveScanProxyPortCallback, context.zoneUIN, proxy.get(), nullptr, &pctx));
         }

         if ((context.flags & NSCAN_PROBE_MODBUS) && !shouldCancel())
         {
            InteractiveScanPortContext pctx{ &state, NSCAN_HAS_MODBUS, MODBUS_TCP_DEFAULT_PORT, 0 };
            recordProxyScanResult(NSCAN_HAS_MODBUS, ScanAddressRangeTCPProxy(proxyConn.get(), from, blockEnd, MODBUS_TCP_DEFAULT_PORT,
                  InteractiveScanProxyPortCallback, context.zoneUIN, proxy.get(), nullptr, &pctx));
         }

         if ((context.flags & NSCAN_PROBE_ETHERNET_IP) && !shouldCancel())
         {
            InteractiveScanPortContext pctx{ &state, NSCAN_HAS_ETHERNET_IP, ETHERNET_IP_DEFAULT_PORT, 0 };
            recordProxyScanResult(NSCAN_HAS_ETHERNET_IP, ScanAddressRangeTCPProxy(proxyConn.get(), from, blockEnd, ETHERNET_IP_DEFAULT_PORT,
                  InteractiveScanProxyPortCallback, context.zoneUIN, proxy.get(), nullptr, &pctx));
         }

         for(int i = 0; (i < context.tcpPorts.size()) && !shouldCancel(); i++)
         {
            uint16_t port = context.tcpPorts.get(i);
            InteractiveScanPortContext pctx{ &state, NSCAN_HAS_TCP_PORT_OPEN, port, 0 };
            recordProxyScanResult(NSCAN_HAS_TCP_PORT_OPEN, ScanAddressRangeTCPProxy(proxyConn.get(), from, blockEnd, port,
                  InteractiveScanProxyPortCallback, context.zoneUIN, proxy.get(), nullptr, &pctx));
         }

         if ((context.flags & NSCAN_PROBE_SNMP) && !shouldCancel())
         {
            IntegerArray<uint16_t> snmpPorts = GetWellKnownPorts(L"snmp", 0);
            if (snmpPorts.isEmpty())
               snmpPorts.add(161);
            unique_ptr<StringList> communities = SnmpGetKnownCommunities(0);
            for(int i = 0; (i < snmpPorts.size()) && !shouldCancel(); i++)
            {
               uint16_t port = snmpPorts.get(i);
               for(int j = 0; (j < communities->size()) && !shouldCancel(); j++)
               {
                  const wchar_t *community = communities->get(j);
                  InteractiveScanPortContext pctxV1{ &state, NSCAN_HAS_SNMP, port, SNMP_VERSION_1 };
                  recordProxyScanResult(NSCAN_HAS_SNMP, ScanAddressRangeSNMPProxy(proxyConn.get(), from, blockEnd, port, SNMP_VERSION_1, community,
                        InteractiveScanProxyPortCallback, context.zoneUIN, proxy.get(), nullptr, &pctxV1));
                  InteractiveScanPortContext pctxV2{ &state, NSCAN_HAS_SNMP, port, SNMP_VERSION_2C };
                  recordProxyScanResult(NSCAN_HAS_SNMP, ScanAddressRangeSNMPProxy(proxyConn.get(), from, blockEnd, port, SNMP_VERSION_2C, community,
                        InteractiveScanProxyPortCallback, context.zoneUIN, proxy.get(), nullptr, &pctxV2));
               }
               InteractiveScanPortContext pctxV3{ &state, NSCAN_HAS_SNMP, port, SNMP_VERSION_3 };
               recordProxyScanResult(NSCAN_HAS_SNMP, ScanAddressRangeSNMPProxy(proxyConn.get(), from, blockEnd, port, SNMP_VERSION_3, nullptr,
                     InteractiveScanProxyPortCallback, context.zoneUIN, proxy.get(), nullptr, &pctxV3));
            }
         }

         from += blockSize;
      }

      if (!shouldCancel())
      {
         for(int i = 0; i < proxyScanErrorCount; i++)
         {
            nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, L"ScanNetworkRangeInteractive: probe 0x%02X scan command on proxy node %s [%u] failed (%s)",
                  proxyScanErrors[i].probe, proxy->getName(), proxy->getId(), AgentErrorCodeToText(proxyScanErrors[i].error));
            if (context.warningReporter)
               context.warningReporter(proxyScanErrors[i].probe, AgentErrorToRCC(proxyScanErrors[i].error));
         }
      }
   }
   else
   {
      ScanAddressRangeICMP(context.startAddress, context.endAddress, InteractiveScanIcmpCallback, nullptr, &state);

      if ((context.flags & NSCAN_PROBE_AGENT) && !shouldCancel())
      {
         InteractiveScanPortContext pctx{ &state, NSCAN_HAS_AGENT, AGENT_LISTEN_PORT, 0 };
         TCPScanAddressRange(context.startAddress, context.endAddress, AGENT_LISTEN_PORT, InteractiveScanPortCallback, &pctx);
      }

      if ((context.flags & NSCAN_PROBE_MODBUS) && !shouldCancel())
      {
         InteractiveScanPortContext pctx{ &state, NSCAN_HAS_MODBUS, MODBUS_TCP_DEFAULT_PORT, 0 };
         TCPScanAddressRange(context.startAddress, context.endAddress, MODBUS_TCP_DEFAULT_PORT, InteractiveScanPortCallback, &pctx);
      }

      if ((context.flags & NSCAN_PROBE_ETHERNET_IP) && !shouldCancel())
      {
         InteractiveScanPortContext pctx{ &state, NSCAN_HAS_ETHERNET_IP, ETHERNET_IP_DEFAULT_PORT, 0 };
         TCPScanAddressRange(context.startAddress, context.endAddress, ETHERNET_IP_DEFAULT_PORT, InteractiveScanPortCallback, &pctx);
      }

      for(int i = 0; (i < context.tcpPorts.size()) && !shouldCancel(); i++)
      {
         uint16_t port = context.tcpPorts.get(i);
         InteractiveScanPortContext pctx{ &state, NSCAN_HAS_TCP_PORT_OPEN, port, 0 };
         TCPScanAddressRange(context.startAddress, context.endAddress, port, InteractiveScanPortCallback, &pctx);
      }

      if ((context.flags & NSCAN_PROBE_SNMP) && !shouldCancel())
      {
         IntegerArray<uint16_t> snmpPorts = GetWellKnownPorts(L"snmp", 0);
         if (snmpPorts.isEmpty())
            snmpPorts.add(161);
         unique_ptr<StringList> communities = SnmpGetKnownCommunities(0);
         for(int i = 0; (i < snmpPorts.size()) && !shouldCancel(); i++)
         {
            uint16_t port = snmpPorts.get(i);
            for(int j = 0; (j < communities->size()) && !shouldCancel(); j++)
            {
               char community[256];
               wchar_to_mb(communities->get(j), -1, community, 256);
               InteractiveScanPortContext pctxV1{ &state, NSCAN_HAS_SNMP, port, SNMP_VERSION_1 };
               SnmpScanAddressRange(context.startAddress, context.endAddress, port, SNMP_VERSION_1, community, InteractiveScanPortCallback, &pctxV1);
               InteractiveScanPortContext pctxV2{ &state, NSCAN_HAS_SNMP, port, SNMP_VERSION_2C };
               SnmpScanAddressRange(context.startAddress, context.endAddress, port, SNMP_VERSION_2C, community, InteractiveScanPortCallback, &pctxV2);
            }
            InteractiveScanPortContext pctxV3{ &state, NSCAN_HAS_SNMP, port, SNMP_VERSION_3 };
            SnmpScanAddressRange(context.startAddress, context.endAddress, port, SNMP_VERSION_3, nullptr, InteractiveScanPortCallback, &pctxV3);
         }
      }
   }

   if (reportedHosts != nullptr)
      *reportedHosts = static_cast<uint32_t>(state.records.size());
}

/**
 * Get total size of discovery poller queue (all stages)
 */
int64_t GetDiscoveryPollerQueueSize()
{
   ThreadPoolInfo info;
   ThreadPoolGetInfo(g_discoveryThreadPool, &info);
   int poolQueueSize = ((info.activeRequests > info.curThreads) ? info.activeRequests - info.curThreads : 0) + info.serializedRequests;
   return s_nodePollerQueue.size() + poolQueueSize;
}
