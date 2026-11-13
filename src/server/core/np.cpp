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
** File: np.cpp
**
**/

#include "nxcore.h"
#include <ethernet_ip.h>

#define DEBUG_TAG _T("obj.poll.node")

/**
 * Constructor for NewNodeData
 */
NewNodeData::NewNodeData(const InetAddress& ipAddress) : ipAddr(ipAddress)
{
   creationFlags = 0;
   agentPort = AGENT_LISTEN_PORT;
   snmpPort = SNMP_DEFAULT_PORT;
   eipPort = ETHERNET_IP_DEFAULT_PORT;
   modbusTcpPort = 502;
   modbusUnitId = 255;
   name[0] = 0;
   agentProxyId = 0;
   snmpProxyId = 0;
   mqttProxyId = 0;
   modbusProxyId = 0;
   eipProxyId = 0;
   icmpProxyId = 0;
   sshProxyId = 0;
   sshLogin[0] = 0;
   sshPassword[0] = 0;
   sshPort = SSH_PORT;
   sshKeyId = 0;
   vncProxyId = 0;
   vncPassword[0] = 0;
   vncPort = 5900;
   zoneUIN = 0;
   doConfPoll = false;
   origin = NODE_ORIGIN_MANUAL;
   agentSecret[0] = 0;
   snmpSecurity = nullptr;
   snmpVersion = SNMP_VERSION_2C;
   webServiceProxyId = 0;
}

/**
 * Constructor for NewNodeData
 */
NewNodeData::NewNodeData(const InetAddress& ipAddress, const MacAddress& macAddress) : ipAddr(ipAddress), macAddr(macAddress)
{
   creationFlags = 0;
   agentPort = AGENT_LISTEN_PORT;
   snmpPort = SNMP_DEFAULT_PORT;
   eipPort = ETHERNET_IP_DEFAULT_PORT;
   modbusTcpPort = 502;
   modbusUnitId = 255;
   name[0] = 0;
   agentProxyId = 0;
   snmpProxyId = 0;
   mqttProxyId = 0;
   modbusProxyId = 0;
   eipProxyId = 0;
   icmpProxyId = 0;
   sshProxyId = 0;
   sshLogin[0] = 0;
   sshPassword[0] = 0;
   sshPort = SSH_PORT;
   sshKeyId = 0;
   vncProxyId = 0;
   vncPassword[0] = 0;
   vncPort = 5900;
   zoneUIN = 0;
   doConfPoll = false;
   origin = NODE_ORIGIN_MANUAL;
   agentSecret[0] = 0;
   snmpSecurity = nullptr;
   snmpVersion = SNMP_VERSION_2C;
   webServiceProxyId = 0;
}

/**
 * Create NewNodeData from NXCPMessage
 */
NewNodeData::NewNodeData(const NXCPMessage& msg, const InetAddress& ipAddress) : ipAddr(ipAddress)
{
   ipAddr.setMaskBits(0);  // netmask for new node is always autodetected
   creationFlags = msg.getFieldAsUInt32(VID_CREATION_FLAGS);
   agentPort = msg.getFieldAsUInt16(VID_AGENT_PORT);
   snmpPort = msg.getFieldAsUInt16(VID_SNMP_PORT);
   eipPort = msg.getFieldAsUInt16(VID_ETHERNET_IP_PORT);
   eipAddress = msg.getFieldAsInetAddress(VID_ETHERNET_IP_ADDRESS);
   modbusTcpPort = msg.getFieldAsUInt16(VID_MODBUS_TCP_PORT);
   modbusUnitId = msg.getFieldAsUInt16(VID_MODBUS_UNIT_ID);
   msg.getFieldAsString(VID_OBJECT_NAME, name, MAX_OBJECT_NAME);
   agentProxyId = msg.getFieldAsUInt32(VID_AGENT_PROXY);
   snmpProxyId = msg.getFieldAsUInt32(VID_SNMP_PROXY);
   mqttProxyId = msg.getFieldAsUInt32(VID_MQTT_PROXY);
   modbusProxyId = msg.getFieldAsUInt32(VID_MODBUS_PROXY);
   eipProxyId = msg.getFieldAsUInt32(VID_ETHERNET_IP_PROXY);
   icmpProxyId = msg.getFieldAsUInt32(VID_ICMP_PROXY);
   sshProxyId = msg.getFieldAsUInt32(VID_SSH_PROXY);
   msg.getFieldAsString(VID_SSH_LOGIN, sshLogin, MAX_USER_NAME);
   msg.getFieldAsString(VID_SSH_PASSWORD, sshPassword, MAX_PASSWORD);
   sshPort = msg.getFieldAsUInt16(VID_SSH_PORT);
   sshKeyId = 0;
   vncProxyId = msg.getFieldAsUInt32(VID_VNC_PROXY);
   msg.getFieldAsString(VID_VNC_PASSWORD, vncPassword, MAX_PASSWORD);
   vncPort = msg.getFieldAsUInt16(VID_VNC_PORT);
   zoneUIN = msg.getFieldAsUInt32(VID_ZONE_UIN);
   doConfPoll = false;
   origin = NODE_ORIGIN_MANUAL;
   agentSecret[0] = 0;
   snmpSecurity = nullptr;
   snmpVersion = SNMP_VERSION_2C;
   webServiceProxyId = msg.getFieldAsUInt32(VID_WEB_SERVICE_PROXY);
}

/**
 * Create NewNodeData from JSON definition (REST API)
 */
NewNodeData::NewNodeData(json_t *json, const InetAddress& ipAddress) : ipAddr(ipAddress)
{
   ipAddr.setMaskBits(0);  // netmask for new node is always autodetected
   creationFlags = json_object_get_uint32(json, "creationFlags");
   wcslcpy(name, json_object_get_string(json, "name", L"").cstr(), MAX_OBJECT_NAME);

   // Per-protocol settings use the same property groups as object modification; values not
   // consumed here are applied when creation document is passed to modifyFromJSON()
   json_t *agent = json_object_get(json, "agent");
   agentPort = static_cast<uint16_t>(json_object_get_uint32(agent, "port", AGENT_LISTEN_PORT));
   agentProxyId = json_object_get_uint32(agent, "proxy");

   json_t *snmp = json_object_get(json, "snmp");
   snmpPort = static_cast<uint16_t>(json_object_get_uint32(snmp, "port", SNMP_DEFAULT_PORT));
   snmpProxyId = json_object_get_uint32(snmp, "proxy");

   json_t *eip = json_object_get(json, "etherNetIp");
   eipPort = static_cast<uint16_t>(json_object_get_uint32(eip, "port", ETHERNET_IP_DEFAULT_PORT));
   eipAddress = InetAddress::parse(json_object_get_string(eip, "address", L"").cstr());
   eipProxyId = json_object_get_uint32(eip, "proxy");

   json_t *modbus = json_object_get(json, "modbus");
   modbusTcpPort = static_cast<uint16_t>(json_object_get_uint32(modbus, "tcpPort", 502));
   modbusUnitId = static_cast<uint16_t>(json_object_get_uint32(modbus, "unitId", 255));
   modbusProxyId = json_object_get_uint32(modbus, "proxy");

   icmpProxyId = json_object_get_uint32(json_object_get(json, "icmp"), "proxy");

   json_t *ssh = json_object_get(json, "ssh");
   sshProxyId = json_object_get_uint32(ssh, "proxy");
   wcslcpy(sshLogin, json_object_get_string(ssh, "login", L"").cstr(), MAX_USER_NAME);
   wcslcpy(sshPassword, json_object_get_string(ssh, "password", L"").cstr(), MAX_PASSWORD);
   sshPort = static_cast<uint16_t>(json_object_get_uint32(ssh, "port", SSH_PORT));
   sshKeyId = json_object_get_uint32(ssh, "keyId");

   json_t *vnc = json_object_get(json, "vnc");
   vncProxyId = json_object_get_uint32(vnc, "proxy");
   wcslcpy(vncPassword, json_object_get_string(vnc, "password", L"").cstr(), MAX_PASSWORD);
   vncPort = static_cast<uint16_t>(json_object_get_uint32(vnc, "port", 5900));

   mqttProxyId = json_object_get_uint32(json, "mqttProxy");
   zoneUIN = json_object_get_int32(json, "zoneUIN");
   doConfPoll = false;
   origin = NODE_ORIGIN_MANUAL;
   agentSecret[0] = 0;
   snmpSecurity = nullptr;
   snmpVersion = SNMP_VERSION_2C;
   webServiceProxyId = json_object_get_uint32(json, "webServiceProxy");
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
 * Returns pointer to new node object on success or nullptr on failure
 *
 * @param newNodeData data of new node
 */
shared_ptr<Node> NXCORE_EXPORTABLE PollNewNode(NewNodeData *newNodeData, uint32_t *rcc)
{
   TCHAR ipAddrText[64];
   nxlog_debug_tag(DEBUG_TAG, 4, _T("PollNode(%s/%d) zone %d"), newNodeData->ipAddr.toString(ipAddrText),
            newNodeData->ipAddr.getMaskBits(), newNodeData->zoneUIN);

   if (!(g_flags & AF_UNLIMITED_NODES) && (g_idxNodeById.size() >= GetMaxAllowedNodeCount()))
   {
      // Re-read license keys from the database in case a new license was added at runtime
      CheckNodeCountRestrictions();

      int maxNodes = GetMaxAllowedNodeCount();
      int count = 0;
      g_idxNodeById.forEach(
         [&count](NetObj *node) -> EnumerationCallbackResult
      {
         if (node->getStatus() != STATUS_UNMANAGED)
            count++;
         return _CONTINUE;
      });
      if (count >= maxNodes)
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("PollNode: creation of node \"%s\" blocked by license check"), ipAddrText);
         if (rcc != nullptr)
            *rcc = RCC_LICENSE_VIOLATION;
         return shared_ptr<Node>();
      }
   }

   // Check for node existence
   if ((newNodeData->creationFlags & NXC_NCF_EXTERNAL_GATEWAY) == 0 &&
       ((FindNodeByIP(newNodeData->zoneUIN, newNodeData->ipAddr) != nullptr) ||
       (FindSubnetByIP(newNodeData->zoneUIN, newNodeData->ipAddr) != nullptr)))
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("PollNode: Node %s already exist in database"), ipAddrText);
      if (rcc != nullptr)
         *rcc = RCC_IP_ADDRESS_CONFLICT;
      return shared_ptr<Node>();
   }

   uint32_t flags = 0;
   if (newNodeData->creationFlags & NXC_NCF_DISABLE_ICMP)
      flags |= NF_DISABLE_ICMP;
   if (newNodeData->creationFlags & NXC_NCF_DISABLE_SNMP)
      flags |= NF_DISABLE_SNMP;
   if (newNodeData->creationFlags & NXC_NCF_DISABLE_ETHERNET_IP)
      flags |= NF_DISABLE_ETHERNET_IP;
   if (newNodeData->creationFlags & NXC_NCF_DISABLE_MODBUS_TCP)
      flags |= NF_DISABLE_MODBUS_TCP;
   if (newNodeData->creationFlags & NXC_NCF_DISABLE_NXCP)
      flags |= NF_DISABLE_NXCP;
   if (newNodeData->creationFlags & NXC_NCF_DISABLE_SSH)
      flags |= NF_DISABLE_SSH;
   if (newNodeData->creationFlags & NXC_NCF_DISABLE_VNC)
      flags |= NF_DISABLE_VNC;
   if (newNodeData->creationFlags & NXC_NCF_SNMP_SETTINGS_LOCKED)
      flags |= NF_SNMP_SETTINGS_LOCKED;
   if (newNodeData->creationFlags & NXC_NCF_EXTERNAL_GATEWAY)
      flags |= NF_EXTERNAL_GATEWAY;
   shared_ptr<Node> node = make_shared<Node>(newNodeData, flags);
   NetObjInsert(node, true, false);

   if (newNodeData->creationFlags & NXC_NCF_ENTER_MAINTENANCE)
      node->enterMaintenanceMode(0, _T("Automatic maintenance for new node"));

	// Use DNS name as primary name if required
	if ((newNodeData->origin == NODE_ORIGIN_NETWORK_DISCOVERY) && ConfigReadBoolean(_T("NetworkDiscovery.UseDNSNameForDiscoveredNodes"), false))
	{
      TCHAR dnsName[MAX_DNS_NAME];
      bool addressResolved = false;
	   if (IsZoningEnabled() && (newNodeData->zoneUIN != 0))
	   {
	      shared_ptr<Zone> zone = FindZoneByUIN(newNodeData->zoneUIN);
	      if (zone != nullptr)
	      {
            shared_ptr<AgentConnectionEx> conn = zone->acquireConnectionToProxy();
            if (conn != nullptr)
            {
               addressResolved = (conn->getHostByAddr(newNodeData->ipAddr, dnsName, MAX_DNS_NAME) != nullptr);
            }
	      }
	   }
	   else
		{
	      addressResolved = (newNodeData->ipAddr.getHostByAddr(dnsName, MAX_DNS_NAME) != nullptr);
		}

      if (addressResolved && ResolveHostName(newNodeData->zoneUIN, dnsName).equals(newNodeData->ipAddr))
      {
         // We have valid DNS name which resolves back to node's IP address, use it as primary name
         node->setPrimaryHostName(dnsName);
         nxlog_debug_tag(DEBUG_TAG, 4, _T("PollNode: Using DNS name %s as primary name for node %s"), dnsName, ipAddrText);
      }
	}

	// Bind node to cluster before first configuration poll
	if (newNodeData->cluster != nullptr)
	{
	   newNodeData->cluster->applyToTarget(node);
	}

   if (newNodeData->creationFlags & NXC_NCF_CREATE_UNMANAGED)
   {
      node->setMgmtStatus(FALSE);
      node->checkSubnetBinding();
      if (newNodeData->ipAddr.isValidUnicast())
      {
         InetAddress ifaceAddr = newNodeData->ipAddr;
         ifaceAddr.setMaskBits(0);  // autodetect, so pseudo-interface is created with synthetic mask
         node->createNewInterface(ifaceAddr, newNodeData->macAddr, true);
      }
   }

   if (IsZoningEnabled() && (newNodeData->creationFlags & NXC_NCF_AS_ZONE_PROXY) && (newNodeData->zoneUIN != 0))
   {
      shared_ptr<Zone> zone = FindZoneByUIN(newNodeData->zoneUIN);
      if (zone != nullptr)
      {
         zone->addProxy(*node);
      }
   }

	if (newNodeData->doConfPoll)
	{
      static_cast<Pollable&>(*node).doForcedConfigurationPoll(RegisterPoller(PollerType::CONFIGURATION, node));
   }

   node->publish();
   EventBuilder(EVENT_NODE_ADDED, node->getId())
      .param(_T("nodeOrigin"), static_cast<int>(newNodeData->origin))
      .post();

   return node;
}
