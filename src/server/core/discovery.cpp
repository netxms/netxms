/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Victor Kirhenshtein
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
 * Check if host at given IP address is reachable by NetXMS server
 */
static bool HostIsReachable(const InetAddress& ipAddr, int32_t zoneUIN, bool fullCheck, SNMP_Transport **transport,
      shared_ptr<AgentConnection> *preparedAgentConnection, SSHCredentials *sshCredentials, uint16_t *sshPort)
{
   bool reachable = false;

   if (transport != nullptr)
      *transport = nullptr;
   if (preparedAgentConnection != nullptr)
      preparedAgentConnection->reset();
   if (sshCredentials != nullptr)
      memset(sshCredentials, 0, sizeof(SSHCredentials));
   if (sshPort != nullptr)
      *sshPort = 0;

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
      if (!agentConnection->connect(g_serverKey, &rcc))
      {
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
         if (preparedAgentConnection != nullptr)
            *preparedAgentConnection = agentConnection;
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
         if (transport != nullptr)
         {
            snmpTransport->setSnmpVersion(version);
            *transport = snmpTransport;
            snmpTransport = nullptr;   // prevent deletion
         }
         reachable = true;
         delete snmpTransport;
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
      if (SSHCheckCommSettings((zoneProxy != 0) ? zoneProxy : g_dwMgmtNode, ipAddr, zoneUIN, sshCredentials, sshPort))
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

         if (!HostIsReachable(address->ipAddr, address->zoneUIN, false, nullptr, nullptr, nullptr, nullptr))
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
            if (iface->getIpAddressList()->hasAddress(oldNode->getIpAddress()))
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
   SNMP_Transport *snmpTransport = nullptr;
   shared_ptr<AgentConnection> agentConnection;
   SSHCredentials sshCredentials;
   uint16_t sshPort;
   if (!HostIsReachable(address->ipAddr, address->zoneUIN, true, &snmpTransport, &agentConnection, &sshCredentials, &sshPort))
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNodeStage1(%s): host is not reachable"), ipAddrText);
      return false;
   }

   // Initialize discovered node data
   auto data = new DiscoveryFilterData(address->ipAddr, address->zoneUIN);
   address->data = data;

   // Basic communication settings
   if (snmpTransport != nullptr)
   {
      address->snmpTransport = snmpTransport;

      data->flags |= NNF_IS_SNMP;
      data->snmpVersion = snmpTransport->getSnmpVersion();

      // Get SNMP OID
      SnmpGet(data->snmpVersion, snmpTransport, { 1, 3, 6, 1, 2, 1, 1, 2, 0 }, &data->snmpObjectId, 0, SG_OBJECT_ID_RESULT);

      data->driver = FindDriverForNode(ipAddrText, data->snmpObjectId, nullptr, snmpTransport);
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNodeStage1(%s): selected device driver %s"), ipAddrText, data->driver->getName());
   }
   if (agentConnection != nullptr)
   {
      data->flags |= NNF_IS_AGENT;
      agentConnection->getParameter(_T("Agent.Version"), data->agentVersion, MAX_AGENT_VERSION_LEN);
      agentConnection->getParameter(_T("System.PlatformName"), data->platform, MAX_PLATFORM_NAME_LEN);
   }
   if (sshPort != 0)
   {
      data->flags |= NNF_IS_SSH;
   }

   // Read interface list if possible
   if (data->flags & NNF_IS_AGENT)
   {
      data->ifList = agentConnection->getInterfaceList();
   }
   if ((data->ifList == nullptr) && (data->flags & NNF_IS_SNMP))
   {
      data->driver->analyzeDevice(snmpTransport, data->snmpObjectId, data, &data->driverData);
      data->ifList = data->driver->getInterfaces(snmpTransport, data, data->driverData, ConfigReadBoolean(_T("Objects.Interfaces.UseIfXTable"), true));
   }

   // Check if node is a router
   if (data->flags & NNF_IS_SNMP)
   {
      uint32_t value;
      if (SnmpGet(data->snmpVersion, snmpTransport, { 1, 3, 6, 1, 2, 1, 4, 1, 0 }, &value, sizeof(uint32_t), 0) == SNMP_ERR_SUCCESS)
      {
         if (value == 1)
            data->flags |= NNF_IS_ROUTER;
      }
   }
   else if (data->flags & NNF_IS_AGENT)
   {
      // Check IP forwarding status
      TCHAR buffer[16];
      if (agentConnection->getParameter(_T("Net.IP.Forwarding"), buffer, 16) == ERR_SUCCESS)
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
      if (SnmpGet(data->snmpVersion, snmpTransport, { 1, 3, 6, 1, 2, 1, 17, 1, 1, 0 }, buffer, sizeof(buffer), 0) == SNMP_ERR_SUCCESS)
      {
         data->flags |= NNF_IS_BRIDGE;
      }

      // Check for CDP (Cisco Discovery Protocol) support
      uint32_t value;
      if (SnmpGet(data->snmpVersion, snmpTransport, { 1, 3, 6, 1, 4, 1, 9, 9, 23, 1, 3, 1, 0 }, &value, sizeof(uint32_t), 0) == SNMP_ERR_SUCCESS)
      {
         if (value == 1)
            data->flags |= NNF_IS_CDP;
      }

      // Check for NDP/SONMP (Nortel Networks topology discovery protocol) support
      if (SnmpGet(data->snmpVersion, snmpTransport, { 1, 3, 6, 1, 4, 1, 45, 1, 6, 13, 1, 2, 0 }, &value, sizeof(uint32_t), 0) == SNMP_ERR_SUCCESS)
      {
         if (value == 1)
            data->flags |= NNF_IS_SONMP;
      }

      // Check for LLDP (Link Layer Discovery Protocol) support
      if (SnmpGet(data->snmpVersion, snmpTransport, { 1, 0, 8802, 1, 1, 2, 1, 3, 2, 0 }, buffer, sizeof(buffer), 0) == SNMP_ERR_SUCCESS)
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

   // Check for filter script
   if ((filterFlags & (DFF_CHECK_PROTOCOLS | DFF_CHECK_ADDRESS_RANGE | DFF_EXECUTE_SCRIPT)) == 0)
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNodeStage2(%s): no filtering, node accepted"), ipAddrText);
      return true;   // No filtering
   }

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

   // Execute filter script
   if (result && (filterFlags & DFF_EXECUTE_SCRIPT))
   {
      TCHAR filterScript[MAX_CONFIG_VALUE_LENGTH];
      ConfigReadStr(_T("NetworkDiscovery.Filter.Script"), filterScript, MAX_CONFIG_VALUE_LENGTH, _T(""));
      Trim(filterScript);

      NXSL_VM *vm = CreateServerScriptVM(filterScript, shared_ptr<NetObj>());
      if (vm != nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNodeStage2(%s): Running filter script %s"), ipAddrText, filterScript);

         if (address->snmpTransport != nullptr)
         {
            vm->setGlobalVariable("$snmp", vm->createValue(vm->createObject(&g_nxslSnmpTransportClass, address->snmpTransport)));
            address->snmpTransport = nullptr;   // Transport will be deleted by NXSL object destructor
         }
         // TODO: make agent and SSH connection available in script
         vm->setGlobalVariable("$node", vm->createValue(vm->createObject(&g_nxslDiscoveredNodeClass, data)));

         NXSL_Value *param = vm->createValue(vm->createObject(&g_nxslDiscoveredNodeClass, data));
         if (vm->run(1, &param))
         {
            result = vm->getResult()->getValueAsBoolean();
            nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNodeStage2(%s): Filter script result is %s"), ipAddrText, BooleanToString(result));
         }
         else
         {
            result = false;   // Consider script runtime error to be a negative result
            nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNodeStage2(%s): Filter script execution error: %s"), ipAddrText, vm->getErrorText());
            ReportScriptError(SCRIPT_CONTEXT_OBJECT, nullptr, 0, vm->getErrorText(), filterScript);
         }
         delete vm;
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNodeStage2(%s): Cannot find filter script %s"), ipAddrText, filterScript);
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
            if ((iface != nullptr) && macAddr.isValid() && !iface->getMacAddress().equals(macAddr))
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
      if (!iface->getIpAddressList()->hasAddress(ipAddr))
      {
         const InetAddress& interfaceAddress = iface->getIpAddressList()->findSameSubnetAddress(ipAddr);
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
   if ((iface != nullptr) && iface->getIpAddressList()->findSameSubnetAddress(route->destination).isValidUnicast())
   {
      CheckPotentialNode(node, route->destination, route->ifIndex, MacAddress::NONE, DA_SRC_ROUTING_TABLE, node->getId());
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("Interface object not found for host route"));
   }
}

/**
 * Discovery poller
 */
void DiscoveryPoller(PollerInfo *poller)
{
   poller->startExecution();
   int64_t startTime = GetCurrentTimeMs();

   Node *node = static_cast<Node*>(poller->getObject());
   if (node->isDeleteInitiated() || IsShutdownInProgress())
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("Discovery poll of node %s (%s) in zone %d aborted"),
                node->getName(), node->getIpAddress().toString().cstr(), node->getZoneUIN());
      node->completeDiscoveryPoll(GetCurrentTimeMs() - startTime);
      delete poller;
      return;
   }

   nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("Starting discovery poll of node %s (%s) in zone %d"),
             node->getName(), node->getIpAddress().toString().cstr(), node->getZoneUIN());

   // Retrieve and analyze node's ARP cache
   shared_ptr<ArpCache> arpCache = node->getArpCache(true);
   if (arpCache != nullptr)
   {
      for(int i = 0; i < arpCache->size(); i++)
      {
         const ArpEntry *e = arpCache->get(i);
         if (!e->macAddr.isBroadcast() && !e->macAddr.isNull())
            CheckPotentialNode(node, e->ipAddr, e->ifIndex, e->macAddr, DA_SRC_ARP_CACHE, node->getId());
      }
   }

   if (node->isDeleteInitiated() || IsShutdownInProgress())
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("Discovery poll of node %s (%s) in zone %d aborted"),
                node->getName(), (const TCHAR *)node->getIpAddress().toString(), (int)node->getZoneUIN());
      node->completeDiscoveryPoll(GetCurrentTimeMs() - startTime);
      delete poller;
      return;
   }

   // Retrieve and analyze node's routing table
   if (!(node->getFlags() & NF_DISABLE_ROUTE_POLL))
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 5, _T("Discovery poll of node %s (%s) - reading routing table"), node->getName(), node->getIpAddress().toString().cstr());
      shared_ptr<RoutingTable> rt = node->getRoutingTable();
      if (rt != nullptr)
      {
         for(int i = 0; i < rt->size(); i++)
         {
            const ROUTE *route = rt->get(i);
            CheckPotentialNode(node, route->nextHop, route->ifIndex, MacAddress::NONE, DA_SRC_ROUTING_TABLE, node->getId());
            if (route->destination.isValidUnicast() && (route->destination.getHostBits() == 0))
               CheckHostRoute(node, route);
         }
      }
   }

   node->executeHookScript(_T("DiscoveryPoll"));

   nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("Finished discovery poll of node %s (%s)"), node->getName(), node->getIpAddress().toString().cstr());
   node->completeDiscoveryPoll(GetCurrentTimeMs() - startTime);
   delete poller;
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
static void ScanAddressRangeSNMPProxy(AgentConnection *conn, uint32_t from, uint32_t to, uint16_t port, SNMP_Version snmpVersion, const TCHAR *community,
      void (*callback)(const InetAddress&, int32_t, const Node*, uint32_t, const TCHAR*, ServerConsole*, void*), int32_t zoneUIN, const Node *proxy, ServerConsole *console)
{
   TCHAR request[1024], ipAddr1[MAX_IP_ADDR_TEXT_LEN], ipAddr2[MAX_IP_ADDR_TEXT_LEN];
   _sntprintf(request, 1024, _T("SNMP.ScanAddressRange(%s,%s,%u,%d,\"%s\")"), IpToStr(from, ipAddr1), IpToStr(to, ipAddr2), port, snmpVersion, CHECK_NULL_EX(community));
   StringList *list;
   if (conn->getList(request, &list) == ERR_SUCCESS)
   {
      for(int i = 0; i < list->size(); i++)
      {
         callback(InetAddress::parse(list->get(i)), zoneUIN, proxy, 0, _T("SNMP"), console, nullptr);
      }
      delete list;
   }
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
static void ScanAddressRangeTCPProxy(AgentConnection *conn, uint32_t from, uint32_t to, uint16_t port,
      void (*callback)(const InetAddress&, int32_t, const Node*, uint32_t, const TCHAR*, ServerConsole*, void*), int32_t zoneUIN, const Node *proxy, ServerConsole *console)
{
   TCHAR request[1024], ipAddr1[MAX_IP_ADDR_TEXT_LEN], ipAddr2[MAX_IP_ADDR_TEXT_LEN];
   _sntprintf(request, 1024, _T("TCP.ScanAddressRange(%s,%s,%u)"), IpToStr(from, ipAddr1), IpToStr(to, ipAddr2), port);
   StringList *list;
   if (conn->getList(request, &list) == ERR_SUCCESS)
   {
      for(int i = 0; i < list->size(); i++)
      {
         callback(InetAddress::parse(list->get(i)), zoneUIN, proxy, 0, _T("TCP"), console, nullptr);
      }
      delete list;
   }
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

   if ((range.getZoneUIN() != 0) || (range.getProxyId() != 0))
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
      conn->setCommandTimeout(10000);

      TCHAR ipAddr1[MAX_IP_ADDR_TEXT_LEN], ipAddr2[MAX_IP_ADDR_TEXT_LEN], rangeText[128];
      _sntprintf(rangeText, 128, _T("%s - %s"), IpToStr(from, ipAddr1), IpToStr(to, ipAddr2));
      ConsoleDebugPrintf(console, DEBUG_TAG_DISCOVERY, 4, _T("Starting active discovery check on range %s via proxy %s [%u]"), rangeText, proxy->getName(), proxy->getId());
      while((from <= to) && !IsShutdownInProgress())
      {
         if (interBlockDelay > 0)
            ThreadSleepMs(interBlockDelay);

         uint32_t blockEndAddr = std::min(to, from + blockSize - 1);

         TCHAR request[256];
         _sntprintf(request, 256, _T("ICMP.ScanRange(%s,%s)"), IpToStr(from, ipAddr1), IpToStr(blockEndAddr, ipAddr2));
         StringList *list;
         if (conn->getList(request, &list) == ERR_SUCCESS)
         {
            for(int i = 0; i < list->size(); i++)
            {
               callback(InetAddress::parse(list->get(i)), range.getZoneUIN(), proxy.get(), 0, _T("ICMP"), console, nullptr);
            }
            delete list;
         }

         if (snmpScanEnabled && !IsShutdownInProgress())
         {
            ConsoleDebugPrintf(console, DEBUG_TAG_DISCOVERY, 5, _T("Starting SNMP check on range %s - %s via proxy %s [%u] (snmp=%s tcp=%s bs=%u delay=%u)"),
                  IpToStr(from, ipAddr1), IpToStr(to, ipAddr2), proxy->getName(), proxy->getId(), BooleanToString(snmpScanEnabled),
                  BooleanToString(tcpScanEnabled), blockSize, interBlockDelay);
            IntegerArray<uint16_t> ports = GetWellKnownPorts(_T("snmp"), 0);
            unique_ptr<StringList> communities = SnmpGetKnownCommunities(0);
            for(int i = 0; (i < ports.size()) && !IsShutdownInProgress(); i++)
            {
               uint16_t port = ports.get(i);
               for(int j = 0; (j < communities->size()) && !IsShutdownInProgress(); j++)
               {
                  const TCHAR *community = communities->get(j);
                  ScanAddressRangeSNMPProxy(conn.get(), from, blockEndAddr, port, SNMP_VERSION_1, community, callback, range.getZoneUIN(), proxy.get(), console);
                  ScanAddressRangeSNMPProxy(conn.get(), from, blockEndAddr, port, SNMP_VERSION_2C, community, callback, range.getZoneUIN(), proxy.get(), console);
               }
               ScanAddressRangeSNMPProxy(conn.get(), from, blockEndAddr, port, SNMP_VERSION_3, nullptr, callback, range.getZoneUIN(), proxy.get(), console);
            }
         }

         if (tcpScanEnabled && !IsShutdownInProgress())
         {
            ConsoleDebugPrintf(console, DEBUG_TAG_DISCOVERY, 5, _T("Starting TCP check on range %s - %s via proxy %s [%u]"),
                  IpToStr(from, ipAddr1), IpToStr(to, ipAddr2), proxy->getName(), proxy->getId());
            IntegerArray<uint16_t> ports = GetWellKnownPorts(_T("agent"), 0);
            ports.addAll(GetWellKnownPorts(_T("ssh"), 0));
            for(int i = 0; (i < ports.size()) && !IsShutdownInProgress(); i++)
               ScanAddressRangeTCPProxy(conn.get(), from, blockEndAddr, ports.get(i), callback, range.getZoneUIN(), proxy.get(), console);
            ScanAddressRangeTCPProxy(conn.get(), from, blockEndAddr, ETHERNET_IP_DEFAULT_PORT, callback, range.getZoneUIN(), proxy.get(), console);
         }

         from += blockSize;
      }
      ConsoleDebugPrintf(console, DEBUG_TAG_DISCOVERY, 4, _T("Finished active discovery check on range %s via proxy %s [%u]"), rangeText, proxy->getName(), proxy->getId());
   }
   else
   {
      TCHAR ipAddr1[MAX_IP_ADDR_TEXT_LEN], ipAddr2[MAX_IP_ADDR_TEXT_LEN], rangeText[128];
      _sntprintf(rangeText, 128, _T("%s - %s"), IpToStr(from, ipAddr1), IpToStr(to, ipAddr2));
      ConsoleDebugPrintf(console, DEBUG_TAG_DISCOVERY, 4, _T("Starting active discovery check on range %s (snmp=%s tcp=%s bs=%u delay=%u)"),
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
            for(int i = 0; (i < ports.size()) && !IsShutdownInProgress(); i++)
               ScanAddressRangeTCP(from, blockEndAddr, ports.get(i), callback, console, nullptr);
            ScanAddressRangeTCP(from, blockEndAddr, ETHERNET_IP_DEFAULT_PORT, callback, console, nullptr);
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
 * Get total size of discovery poller queue (all stages)
 */
int64_t GetDiscoveryPollerQueueSize()
{
   ThreadPoolInfo info;
   ThreadPoolGetInfo(g_discoveryThreadPool, &info);
   int poolQueueSize = ((info.activeRequests > info.curThreads) ? info.activeRequests - info.curThreads : 0) + info.serializedRequests;
   return s_nodePollerQueue.size() + poolQueueSize;
}
