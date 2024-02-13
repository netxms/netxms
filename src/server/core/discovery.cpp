/*
** NetXMS - Network Management System
** Copyright (C) 2003-2024 Victor Kirhenshtein
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

#define DEBUG_TAG_DISCOVERY      _T("poll.discovery")

/**
 * Node poller queue (polls new nodes)
 */
ObjectQueue<DiscoveredAddress> g_nodePollerQueue;

/**
 * Thread pool
 */
ThreadPool *g_discoveryThreadPool = nullptr;

/**
 * Discovery source type names
 */
static const TCHAR *s_discoveredAddrSourceTypeAsText[] = {
   _T("ARP Cache"),
   _T("Routing Table"),
   _T("Agent Registration"),
   _T("SNMP Trap"),
   _T("Syslog"),
   _T("Active Discovery"),
};

/**
 * IP addresses being processed by node poller
 */
static ObjectArray<DiscoveredAddress> s_processingList(64, 64, Ownership::False);
static Mutex s_processingListLock;

/**
 * Check if given address is in processing by new node poller
 */
static bool IsNodePollerActiveAddress(const InetAddress& addr)
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

            _sntprintf(parameter, 128, _T("Icmp.Ping(%s)"), ipAddr.toString(buffer));
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
                 for(int i = 0; i < count; i++)
                    secrets.addPreallocated(DBGetField(hResult, i, 0, nullptr, 0));
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
      TCHAR ipAddrText[64];
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("HostIsReachable(%s): all SNMP probes disabled"), ipAddr.toString(ipAddrText));
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
      TCHAR ipAddrText[64];
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("HostIsReachable(%s): SSH connection probe disabled"), ipAddr.toString(ipAddrText));
   }

   return reachable;
}

/**
 * Check if newly discovered node should be added
 */
static bool AcceptNewNode(NewNodeData *newNodeData)
{
   TCHAR buffer[256], ipAddrText[64];

   newNodeData->ipAddr.toString(ipAddrText);
   if (FindNodeByIP(newNodeData->zoneUIN, newNodeData->ipAddr) != nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNode(%s): node already exist in database"), ipAddrText);
      return false;  // Node already exist in database
   }

   shared_ptr<Subnet> subnet = FindSubnetForNode(newNodeData->zoneUIN, newNodeData->ipAddr);
   if (subnet != nullptr)
   {
      if (subnet->getIpAddress().equals(newNodeData->ipAddr))
      {
         nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNode(%s): IP address already known as subnet address"), ipAddrText);
         return false;
      }
      if (newNodeData->ipAddr.isSubnetBroadcast(subnet->getIpAddress().getMaskBits()))
      {
         nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNode(%s): IP address already known as subnet broadcast address"), ipAddrText);
         return false;
      }
   }

   if (newNodeData->macAddr.isBroadcast())
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNode(%s): broadcast MAC address"), ipAddrText);
      return false;  // Broadcast MAC
   }

   // If MAC address is not known try to find it in ARP cache
   if (!newNodeData->macAddr.isValid())
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("AcceptNewNode(%s): MAC address is not known, will lookup in ARP caches"), ipAddrText);
      shared_ptr<Subnet> subnet = FindSubnetForNode(newNodeData->zoneUIN, newNodeData->ipAddr);
      if (subnet != nullptr)
      {
         MacAddress macAddr = subnet->findMacAddress(newNodeData->ipAddr);
         if (macAddr.isValid())
         {
            if (macAddr.isBroadcast())
            {
               nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNode(%s): broadcast MAC address"), ipAddrText);
               return false;  // Broadcast MAC
            }
            nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("AcceptNewNode(%s): found MAC address %s"), ipAddrText, macAddr.toString().cstr());
            newNodeData->macAddr = macAddr;
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("AcceptNewNode(%s): MAC address not found"), ipAddrText);
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("AcceptNewNode(%s): cannot find matching subnet"), ipAddrText);
      }
   }

   NXSL_VM *hook = FindHookScript(_T("AcceptNewNode"), shared_ptr<NetObj>());
   if (hook != nullptr)
   {
      bool stop = false;
      hook->setGlobalVariable("$ipAddr", hook->createValue(ipAddrText));
      hook->setGlobalVariable("$ipNetMask", hook->createValue(newNodeData->ipAddr.getMaskBits()));
      hook->setGlobalVariable("$macAddr", newNodeData->macAddr.isValid() ? hook->createValue(newNodeData->macAddr.toString(buffer)) : hook->createValue());
      hook->setGlobalVariable("$zoneUIN", hook->createValue(newNodeData->zoneUIN));
      if (hook->run())
      {
         NXSL_Value *result = hook->getResult();
         if (result->isFalse())
         {
            stop = true;
            nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNode(%s): rejected by hook script"), ipAddrText);
         }
      }
      else
      {
         stop = true;
         nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNode(%s): rejected because of hook script execution error (%s)"), ipAddrText, hook->getErrorText());
      }
      delete hook;
      if (stop)
         return false;  // blocked by hook
   }

   if (newNodeData->macAddr.isValid())
   {
      int retryCount = 5;
      while (retryCount-- > 0)
      {
         shared_ptr<Interface> iface = FindInterfaceByMAC(newNodeData->macAddr);
         if (iface == nullptr)
            break;

         if (!HostIsReachable(newNodeData->ipAddr, newNodeData->zoneUIN, false, nullptr, nullptr, nullptr, nullptr))
         {
            nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNode(%s): found existing interface with same MAC address, but new IP is not reachable"), ipAddrText);
            return false;
         }

         // Interface could be deleted by configuration poller while HostIsReachable was running
         shared_ptr<Node> oldNode = iface->getParentNode();
         if (!iface->isDeleted() && (oldNode != nullptr))
         {
            TCHAR oldIpAddrText[16];
            nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNode(%s): node with MAC address %s already exist in database with IP %s and name %s"),
                  ipAddrText, newNodeData->macAddr.toString(buffer), oldNode->getIpAddress().toString(oldIpAddrText), oldNode->getName());

            // we should change node's primary IP only if old IP for this MAC was also node's primary IP
            if (iface->getIpAddressList()->hasAddress(oldNode->getIpAddress()))
            {
               // Try to re-read interface list using old IP address and check if node has botl old and new IPs
               bool bothAddressesFound = false;
               InterfaceList *ifList = oldNode->getInterfaceList();
               if (ifList != nullptr)
               {
                  bothAddressesFound = (ifList->findByIpAddress(newNodeData->ipAddr) != nullptr) && (ifList->findByIpAddress(oldNode->getIpAddress()) != nullptr);
                  delete ifList;
               }
               if (!bothAddressesFound)
               {
                  oldNode->changeIPAddress(newNodeData->ipAddr);
                  nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNode(%s): primary IP address for existing node %s [%u] changed from %s to %s"),
                        ipAddrText, oldNode->getName(), oldNode->getId(), oldIpAddrText, ipAddrText);
               }
               else
               {
                  nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNode(%s): forcing configuration poll for node %s [%u]"), ipAddrText, oldNode->getName(), oldNode->getId());
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
         nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNode(%s): found existing but marked for deletion interface with same MAC address"), ipAddrText);
         return false;
      }
   }

   // Allow filtering by loaded modules
   ENUMERATE_MODULES(pfAcceptNewNode)
   {
      if (!CURRENT_MODULE.pfAcceptNewNode(newNodeData->ipAddr, newNodeData->zoneUIN, newNodeData->macAddr))
      {
         nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNode(%s): rejected by module %s"), ipAddrText, CURRENT_MODULE.szName);
         return false;  // filtered out by module
      }
   }

   // Read filter configuration
   uint32_t filterFlags = ConfigReadULong(_T("NetworkDiscovery.Filter.Flags"), 0);
   nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNode(%s): filter flags=%04X"), ipAddrText, filterFlags);

   // Initialize discovered node data
   DiscoveryFilterData data(newNodeData->ipAddr, newNodeData->zoneUIN);

   // Check for address range
   if (filterFlags & DFF_CHECK_ADDRESS_RANGE)
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNode(%s): filter - checking address range"), ipAddrText);
      ObjectArray<InetAddressListElement> *list = LoadServerAddressList(2);
      bool result = false;
      if (list != nullptr)
      {
         for(int i = 0; (i < list->size()) && !result; i++)
         {
            result = list->get(i)->contains(data.ipAddr);
         }
         delete list;
      }
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNode(%s): filter - range check result is %s"), ipAddrText, BooleanToString(result));
      if (!result)
         return false;
   }

   // Check if host is reachable
   SNMP_Transport *snmpTransport = nullptr;
   shared_ptr<AgentConnection> agentConnection;
   SSHCredentials sshCredentials;
   uint16_t sshPort;
   if (!HostIsReachable(newNodeData->ipAddr, newNodeData->zoneUIN, true, &snmpTransport, &agentConnection, &sshCredentials, &sshPort))
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNode(%s): host is not reachable"), ipAddrText);
      return false;
   }

   // Basic communication settings
   if (snmpTransport != nullptr)
   {
      data.flags |= NNF_IS_SNMP;
      data.snmpVersion = snmpTransport->getSnmpVersion();
      newNodeData->snmpSecurity = new SNMP_SecurityContext(snmpTransport->getSecurityContext());

      // Get SNMP OID
      SnmpGet(data.snmpVersion, snmpTransport, _T(".1.3.6.1.2.1.1.2.0"), nullptr, 0, data.snmpObjectId, MAX_OID_LEN * 4, 0);

      data.driver = FindDriverForNode(ipAddrText, data.snmpObjectId, nullptr, snmpTransport);
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNode(%s): selected device driver %s"), ipAddrText, data.driver->getName());
   }
   if (agentConnection != nullptr)
   {
      data.flags |= NNF_IS_AGENT;
      agentConnection->getParameter(_T("Agent.Version"), data.agentVersion, MAX_AGENT_VERSION_LEN);
      agentConnection->getParameter(_T("System.PlatformName"), data.platform, MAX_PLATFORM_NAME_LEN);
   }
   if (sshPort != 0)
   {
      data.flags |= NNF_IS_SSH;
   }

   // Read interface list if possible
   if (data.flags & NNF_IS_AGENT)
   {
      data.ifList = agentConnection->getInterfaceList();
   }
   if ((data.ifList == nullptr) && (data.flags & NNF_IS_SNMP))
   {
      data.driver->analyzeDevice(snmpTransport, data.snmpObjectId, &data, &data.driverData);
      data.ifList = data.driver->getInterfaces(snmpTransport, &data, data.driverData, ConfigReadBoolean(_T("Objects.Interfaces.UseIfXTable"), true));
   }

   // Check all interfaces for matching existing nodes
   if (data.ifList != nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 5, _T("AcceptNewNode(%s): interface list retrieved, checking for duplicates"), ipAddrText);

      bool duplicate = false;
      for (int i = 0; (i < data.ifList->size()) && !duplicate; i++)
      {
         InterfaceInfo *iface = data.ifList->get(i);
         if (iface->type == IFTYPE_SOFTWARE_LOOPBACK)
            continue;

         for (int j = 0; j < iface->ipAddrList.size(); j++)
         {
            InetAddress addr = iface->ipAddrList.get(j);
            if (!addr.isValidUnicast())
               continue;

            shared_ptr<Node> existingNode = FindNodeByIP(newNodeData->zoneUIN, addr);
            if (existingNode != nullptr)
            {
               nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNode(%s): IP address %s on interface %s matches existing node %s [%u]"),
                  ipAddrText, addr.toString().cstr(), iface->name, existingNode->getName(), existingNode->getId());
               duplicate = true;
               break;
            }

            // Do not do subnet checks for /32 and /128 addresses
            if (addr.getHostBits() > 1)
            {
               subnet = FindSubnetForNode(newNodeData->zoneUIN, addr);
               if (subnet != nullptr)
               {
                  if (subnet->getIpAddress().equals(addr))
                  {
                     nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNode(%s): IP address %s on interface %s already known as subnet address"), ipAddrText, addr.toString().cstr(), iface->name);
                     duplicate = true;
                     break;
                  }
                  if (newNodeData->ipAddr.isSubnetBroadcast(subnet->getIpAddress().getMaskBits()))
                  {
                     nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNode(%s): IP address %s on interface %s already known as subnet broadcast address"), ipAddrText, addr.toString().cstr(), iface->name);
                     duplicate = true;
                     break;
                  }
               }

               InetAddress subnetAddress = addr.getSubnetAddress();
               if (subnetAddress.contains(newNodeData->ipAddr) && newNodeData->ipAddr.isSubnetBroadcast(subnetAddress.getMaskBits()))
               {
                  nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNode(%s): IP address is a broadcast address for subnet %s/%d"), ipAddrText, subnetAddress.toString().cstr(), subnetAddress.getMaskBits());
                  duplicate = true;
                  break;
               }
               if (subnetAddress.equals(newNodeData->ipAddr))
               {
                  nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNode(%s): IP address is a subnet address for subnet %s/%d"), ipAddrText, subnetAddress.toString().cstr(), subnetAddress.getMaskBits());
                  duplicate = true;
                  break;
               }
            }
            else
            {
               nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNode(%s): skipping subnet check for IP address %s/%d"), ipAddrText, addr.toString().cstr(), addr.getMaskBits());
            }
         }
      }

      if (duplicate)
      {
         delete snmpTransport;
         return false;
      }
   }

   // Check for filter script
   if ((filterFlags & (DFF_CHECK_PROTOCOLS | DFF_CHECK_ADDRESS_RANGE | DFF_EXECUTE_SCRIPT)) == 0)
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNode(%s): no filtering, node accepted"), ipAddrText);
      delete snmpTransport;
      return true;   // No filtering
   }

   // Check if node is a router
   if (data.flags & NNF_IS_SNMP)
   {
      uint32_t value;
      if (SnmpGet(data.snmpVersion, snmpTransport, _T(".1.3.6.1.2.1.4.1.0"), nullptr, 0, &value, sizeof(UINT32), 0) == SNMP_ERR_SUCCESS)
      {
         if (value == 1)
            data.flags |= NNF_IS_ROUTER;
      }
   }
   else if (data.flags & NNF_IS_AGENT)
   {
      // Check IP forwarding status
      if (agentConnection->getParameter(_T("Net.IP.Forwarding"), buffer, 16) == ERR_SUCCESS)
      {
         if (_tcstoul(buffer, nullptr, 10) != 0)
            data.flags |= NNF_IS_ROUTER;
      }
   }

   // Check various SNMP device capabilities
   if (data.flags & NNF_IS_SNMP)
   {
      // Check if node is a bridge
      if (SnmpGet(data.snmpVersion, snmpTransport, _T(".1.3.6.1.2.1.17.1.1.0"), nullptr, 0, buffer, sizeof(buffer), 0) == SNMP_ERR_SUCCESS)
      {
         data.flags |= NNF_IS_BRIDGE;
      }

      // Check for CDP (Cisco Discovery Protocol) support
      uint32_t value;
      if (SnmpGet(data.snmpVersion, snmpTransport, _T(".1.3.6.1.4.1.9.9.23.1.3.1.0"), nullptr, 0, &value, sizeof(UINT32), 0) == SNMP_ERR_SUCCESS)
      {
         if (value == 1)
            data.flags |= NNF_IS_CDP;
      }

      // Check for SONMP (Nortel topology discovery protocol) support
      if (SnmpGet(data.snmpVersion, snmpTransport, _T(".1.3.6.1.4.1.45.1.6.13.1.2.0"), nullptr, 0, &value, sizeof(UINT32), 0) == SNMP_ERR_SUCCESS)
      {
         if (value == 1)
            data.flags |= NNF_IS_SONMP;
      }

      // Check for LLDP (Link Layer Discovery Protocol) support
      if (SnmpGet(data.snmpVersion, snmpTransport, _T(".1.0.8802.1.1.2.1.3.2.0"), nullptr, 0, buffer, sizeof(buffer), 0) == SNMP_ERR_SUCCESS)
      {
         data.flags |= NNF_IS_LLDP;
      }
   }

   bool result = true;

   // Check supported communication protocols
   if (filterFlags & DFF_CHECK_PROTOCOLS)
   {
      result = false;

      if ((filterFlags & DFF_PROTOCOL_AGENT) && (data.flags & NNF_IS_AGENT))
         result = true;

      if ((filterFlags & DFF_PROTOCOL_SNMP) && (data.flags & NNF_IS_SNMP))
         result = true;

      if ((filterFlags & DFF_PROTOCOL_SSH) && (data.flags & NNF_IS_SSH))
         result = true;

      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNode(%s): protocol check result is %s"), ipAddrText, BooleanToString(result));
   }

   // Execute filter script
   if (result && (filterFlags & DFF_EXECUTE_SCRIPT))
   {
      TCHAR filterScript[MAX_CONFIG_VALUE];
      ConfigReadStr(_T("NetworkDiscovery.Filter.Script"), filterScript, MAX_CONFIG_VALUE, _T(""));
      Trim(filterScript);

      NXSL_VM *vm = CreateServerScriptVM(filterScript, shared_ptr<NetObj>());
      if (vm != nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNode(%s): Running filter script %s"), ipAddrText, filterScript);

         if (snmpTransport != nullptr)
         {
            vm->setGlobalVariable("$snmp", vm->createValue(vm->createObject(&g_nxslSnmpTransportClass, snmpTransport)));
            snmpTransport = nullptr;   // Transport will be deleted by NXSL object destructor
         }
         // TODO: make agent and SSH connection available in script
         vm->setGlobalVariable("$node", vm->createValue(vm->createObject(&g_nxslDiscoveredNodeClass, &data)));

         NXSL_Value *param = vm->createValue(vm->createObject(&g_nxslDiscoveredNodeClass, &data));
         if (vm->run(1, &param))
         {
            result = vm->getResult()->getValueAsBoolean();
            nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNode(%s): Filter script result is %s"), ipAddrText, BooleanToString(result));
         }
         else
         {
            result = false;   // Consider script runtime error to be a negative result
            nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNode(%s): Filter script execution error: %s"), ipAddrText, vm->getErrorText());
            ReportScriptError(SCRIPT_CONTEXT_OBJECT, nullptr, 0, vm->getErrorText(), filterScript);
         }
         delete vm;
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("AcceptNewNode(%s): Cannot find filter script %s"), ipAddrText, filterScript);
      }
   }

   // Cleanup
   delete snmpTransport;

   return result;
}

/**
 * Create discovered node object
 */
static void CreateDiscoveredNode(NewNodeData *newNodeData)
{
   TCHAR ipAddrText[64];
   newNodeData->ipAddr.toString(ipAddrText);
   nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("CreateDiscoveredNode(%s): processing creation request"), ipAddrText);

   // Double check IP address because parallel discovery may already create that node
   if ((FindNodeByIP(newNodeData->zoneUIN, newNodeData->ipAddr) == nullptr) && (FindSubnetByIP(newNodeData->zoneUIN, newNodeData->ipAddr) == nullptr))
   {
      PollNewNode(newNodeData);
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("CreateDiscoveredNode(%s): initial poll completed"), ipAddrText);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("CreateDiscoveredNode(%s): node already exist in database"), ipAddrText);
   }
   delete newNodeData;
}

/**
 * Process discovered address
 */
static void ProcessDiscoveredAddress(DiscoveredAddress *address)
{
   s_processingListLock.lock();
   bool addressInList = s_processingList.contains(address);
   s_processingListLock.unlock();

   if (!IsShutdownInProgress() && addressInList)//no shutdown process and address is in list
   {
      auto newNodeData = new NewNodeData(address->ipAddr, address->macAddr);
      newNodeData->zoneUIN = address->zoneUIN;
      newNodeData->origin = NODE_ORIGIN_NETWORK_DISCOVERY;
      newNodeData->doConfPoll = true;

      if (address->ignoreFilter || AcceptNewNode(newNodeData))
      {
         if (g_discoveryThreadPool != nullptr)
         {
            TCHAR key[32];
            _sntprintf(key, 32, _T("CREATE/%u"), address->zoneUIN);
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
   else
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("ProcessDiscoveredAddress(%s): address discarded."), address->ipAddr.toString().cstr());
   }

   s_processingListLock.lock();
   s_processingList.remove(address);
   s_processingListLock.unlock();
   delete address;
}

/**
 * Node poller thread (poll new nodes and put them into the database)
 */
void NodePoller()
{
   TCHAR szIpAddr[64];

   ThreadSetName("NodePoller");
   nxlog_debug(1, _T("Node poller started"));

   while(!IsShutdownInProgress())
   {
      DiscoveredAddress *address = g_nodePollerQueue.getOrBlock();
      if (address == INVALID_POINTER_VALUE)
         break;   // Shutdown indicator received

      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 4, _T("NodePoller: processing address %s/%d in zone %d (source type %s, source node [%u])"),
               address->ipAddr.toString(szIpAddr), address->ipAddr.getMaskBits(), (int)address->zoneUIN,
               s_discoveredAddrSourceTypeAsText[address->sourceType], address->sourceNodeId);

      s_processingListLock.lock();
      s_processingList.add(address);
      s_processingListLock.unlock();

      if (g_discoveryThreadPool != nullptr)
      {
         if (g_flags & AF_PARALLEL_NETWORK_DISCOVERY)
         {
            ThreadPoolExecute(g_discoveryThreadPool, ProcessDiscoveredAddress, address);
         }
         else
         {
            TCHAR key[32];
            _sntprintf(key, 32, _T("PROC/%u"), address->zoneUIN);
            ThreadPoolExecuteSerialized(g_discoveryThreadPool, key, ProcessDiscoveredAddress, address);
         }
      }
      else
      {
         ProcessDiscoveredAddress(address);
      }
   }
   nxlog_debug(1, _T("Node poller thread terminated"));
}

/**
 * Comparator for poller queue elements
 */
static bool PollerQueueElementComparator(const InetAddress *key, const DiscoveredAddress *element)
{
   return key->equals(element->ipAddr);
}

/**
 * Check potential new node from sysog, SNMP trap, or address range scan
 */
void CheckPotentialNode(const InetAddress& ipAddr, int32_t zoneUIN, DiscoveredAddressSourceType sourceType, uint32_t sourceNodeId)
{
   TCHAR buffer[64];
   nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("Checking address %s in zone %d (source: %s)"),
            ipAddr.toString(buffer), zoneUIN, s_discoveredAddrSourceTypeAsText[sourceType]);
   if (!ipAddr.isValid() || ipAddr.isBroadcast() || ipAddr.isLoopback() || ipAddr.isMulticast())
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("Potential node %s rejected (IP address is not a valid unicast address)"), ipAddr.toString(buffer));
      return;
   }

   shared_ptr<Node> curr = FindNodeByIP(zoneUIN, ipAddr);
   if (curr != nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("Potential node %s rejected (IP address already known at node %s [%d])"),
               ipAddr.toString(buffer), curr->getName(), curr->getId());
      return;
   }

   if (IsClusterIP(zoneUIN, ipAddr))
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("Potential node %s rejected (IP address is known as cluster resource address)"), ipAddr.toString(buffer));
      return;
   }

   if (IsNodePollerActiveAddress(ipAddr) || (g_nodePollerQueue.find(&ipAddr, PollerQueueElementComparator) != nullptr))
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("Potential node %s rejected (IP address already queued for polling)"), ipAddr.toString(buffer));
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
         g_nodePollerQueue.put(addressInfo);
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
      g_nodePollerQueue.put(addressInfo);
   }
}

/**
 * Check potential new node from ARP cache or routing table
 */
static void CheckPotentialNode(Node *node, const InetAddress& ipAddr, uint32_t ifIndex, const MacAddress& macAddr, DiscoveredAddressSourceType sourceType, uint32_t sourceNodeId)
{
   TCHAR buffer[64];
   nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("Checking potential node %s at %s:%u (source: %s)"),
            ipAddr.toString(buffer), node->getName(), ifIndex, s_discoveredAddrSourceTypeAsText[sourceType]);
   if (!ipAddr.isValidUnicast())
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("Potential node %s rejected (IP address is not a valid unicast address)"), ipAddr.toString(buffer));
      return;
   }

   if (IsClusterIP(node->getZoneUIN(), ipAddr))
   {
      nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 6, _T("Potential node %s rejected (IP address is known as cluster resource address)"), ipAddr.toString(buffer));
      return;
   }

   if (IsNodePollerActiveAddress(ipAddr) || (g_nodePollerQueue.find(&ipAddr, PollerQueueElementComparator) != nullptr))
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
               g_nodePollerQueue.put(addressInfo);
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
   TCHAR buffer[16];
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
   nxlog_debug_tag(DEBUG_TAG_DISCOVERY, 5, _T("Discovery poll of node %s (%s) - reading routing table"), node->getName(), node->getIpAddress().toString().cstr());
   RoutingTable *rt = node->getRoutingTable();
   if (rt != nullptr)
   {
      for(int i = 0; i < rt->size(); i++)
      {
         ROUTE *route = rt->get(i);
         CheckPotentialNode(node, route->nextHop, route->ifIndex, MacAddress::NONE, DA_SRC_ROUTING_TABLE, node->getId());
         if (route->destination.isValidUnicast() && (route->destination.getHostBits() == 0))
            CheckHostRoute(node, route);
      }
      delete rt;
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
   TCHAR ipAddrText[64];
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
   TCHAR request[1024], ipAddr1[64], ipAddr2[64];
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
   TCHAR request[1024], ipAddr1[64], ipAddr2[64];
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

      TCHAR ipAddr1[64], ipAddr2[64], rangeText[128];
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

         if (snmpScanEnabled)
         {
            ConsoleDebugPrintf(console, DEBUG_TAG_DISCOVERY, 5, _T("Starting SNMP check on range %s - %s via proxy %s [%u] (snmp=%s tcp=%s bs=%u delay=%u)"),
                  IpToStr(from, ipAddr1), IpToStr(to, ipAddr2), proxy->getName(), proxy->getId(), BooleanToString(snmpScanEnabled),
                  BooleanToString(tcpScanEnabled), blockSize, interBlockDelay);
            IntegerArray<uint16_t> ports = GetWellKnownPorts(_T("snmp"), 0);
            unique_ptr<StringList> communities = SnmpGetKnownCommunities(0);
            for(int i = 0; i < ports.size(); i++)
            {
               uint16_t port = ports.get(i);
               for(int j = 0; j < communities->size(); j++)
               {
                  const TCHAR *community = communities->get(j);
                  ScanAddressRangeSNMPProxy(conn.get(), from, blockEndAddr, port, SNMP_VERSION_1, community, callback, range.getZoneUIN(), proxy.get(), console);
                  ScanAddressRangeSNMPProxy(conn.get(), from, blockEndAddr, port, SNMP_VERSION_2C, community, callback, range.getZoneUIN(), proxy.get(), console);
               }
               ScanAddressRangeSNMPProxy(conn.get(), from, blockEndAddr, port, SNMP_VERSION_3, nullptr, callback, range.getZoneUIN(), proxy.get(), console);
            }
         }

         if (tcpScanEnabled)
         {
            ConsoleDebugPrintf(console, DEBUG_TAG_DISCOVERY, 5, _T("Starting TCP check on range %s - %s via proxy %s [%u]"),
                  IpToStr(from, ipAddr1), IpToStr(to, ipAddr2), proxy->getName(), proxy->getId());
            IntegerArray<uint16_t> ports = GetWellKnownPorts(_T("agent"), 0);
            ports.addAll(GetWellKnownPorts(_T("ssh"), 0));
            for(int i = 0; i < ports.size(); i++)
               ScanAddressRangeTCPProxy(conn.get(), from, blockEndAddr, ports.get(i), callback, range.getZoneUIN(), proxy.get(), console);
            ScanAddressRangeTCPProxy(conn.get(), from, blockEndAddr, ETHERNET_IP_DEFAULT_PORT, callback, range.getZoneUIN(), proxy.get(), console);
         }

         from += blockSize;
      }
      ConsoleDebugPrintf(console, DEBUG_TAG_DISCOVERY, 4, _T("Finished active discovery check on range %s via proxy %s [%u]"), rangeText, proxy->getName(), proxy->getId());
   }
   else
   {
      TCHAR ipAddr1[64], ipAddr2[64], rangeText[128];
      _sntprintf(rangeText, 128, _T("%s - %s"), IpToStr(from, ipAddr1), IpToStr(to, ipAddr2));
      ConsoleDebugPrintf(console, DEBUG_TAG_DISCOVERY, 4, _T("Starting active discovery check on range %s (snmp=%s tcp=%s bs=%u delay=%u)"),
            rangeText, BooleanToString(snmpScanEnabled), BooleanToString(tcpScanEnabled), blockSize, interBlockDelay);
      while((from <= to) && !IsShutdownInProgress())
      {
         if (interBlockDelay > 0)
            ThreadSleepMs(interBlockDelay);

         uint32_t blockEndAddr = std::min(to, from + blockSize - 1);

         ScanAddressRangeICMP(from, blockEndAddr, callback, console, nullptr);

         if (snmpScanEnabled)
         {
            ConsoleDebugPrintf(console, DEBUG_TAG_DISCOVERY, 5, _T("Starting SNMP check on range %s - %s"), IpToStr(from, ipAddr1), IpToStr(blockEndAddr, ipAddr2));
            IntegerArray<uint16_t> ports = GetWellKnownPorts(_T("snmp"), 0);
            unique_ptr<StringList> communities = SnmpGetKnownCommunities(0);
            for(int i = 0; i < ports.size(); i++)
            {
               uint16_t port = ports.get(i);
               for(int j = 0; j < communities->size(); j++)
               {
#ifdef UNICODE
                  char community[256];
                  wchar_to_mb(communities->get(j), -1, community, 256);
#else
                  const char *community = communities->get(j);
#endif
                  ScanAddressRangeSNMP(from, blockEndAddr, port, SNMP_VERSION_1, community, callback, console, nullptr);
                  ScanAddressRangeSNMP(from, blockEndAddr, port, SNMP_VERSION_2C, community, callback, console, nullptr);
               }
               ScanAddressRangeSNMP(from, blockEndAddr, port, SNMP_VERSION_3, nullptr, callback, console, nullptr);
            }
         }

         if (tcpScanEnabled)
         {
            ConsoleDebugPrintf(console, DEBUG_TAG_DISCOVERY, 5, _T("Starting TCP check on range %s - %s"), IpToStr(from, ipAddr1), IpToStr(blockEndAddr, ipAddr2));
            IntegerArray<uint16_t> ports = GetWellKnownPorts(_T("agent"), 0);
            ports.addAll(GetWellKnownPorts(_T("ssh"), 0));
            for(int i = 0; i < ports.size(); i++)
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
   DiscoveredAddress *addressInfo;
   while((addressInfo = g_nodePollerQueue.get()) != nullptr)
   {
      if (addressInfo != INVALID_POINTER_VALUE)
         delete addressInfo;
   }

   s_processingListLock.lock();
   s_processingList.clear();
   s_processingListLock.unlock();
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
   g_nodePollerQueue.put(INVALID_POINTER_VALUE);
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
   int poolQueueSize;
   if (g_discoveryThreadPool != nullptr)
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
