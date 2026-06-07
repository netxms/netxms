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
** File: nxcore_discovery.h
**
**/

#ifndef _nxcore_discovery_h_
#define _nxcore_discovery_h_

/**
 * Source type for discovered address
 */
enum DiscoveredAddressSourceType
{
   DA_SRC_ARP_CACHE = 0,
   DA_SRC_ROUTING_TABLE = 1,
   DA_SRC_AGENT_REGISTRATION = 2,
   DA_SRC_SNMP_TRAP = 3,
   DA_SRC_SYSLOG = 4,
   DA_SRC_ACTIVE_DISCOVERY = 5
};

/**
 * Node information for autodiscovery filter
 */
class DiscoveryFilterData : public NObject
{
public:
   InetAddress ipAddr;
   NetworkDeviceDriver *driver;
   DriverData *driverData;
   InterfaceList *ifList;
   shared_ptr<AgentConnectionEx> agentConnection;
   int32_t zoneUIN;
   uint32_t flags;
   SNMP_Version snmpVersion;
   bool dnsNameResolved;
   TCHAR dnsName[MAX_DNS_NAME];
   SNMP_ObjectId snmpObjectId;
   TCHAR agentVersion[MAX_AGENT_VERSION_LEN];
   TCHAR platform[MAX_PLATFORM_NAME_LEN];

   DiscoveryFilterData(const InetAddress& _ipAddr, int32_t _zoneUIN) : NObject(), ipAddr(_ipAddr)
   {
      driver = nullptr;
      driverData = nullptr;
      ifList = nullptr;
      zoneUIN = _zoneUIN;
      flags = 0;
      snmpVersion = SNMP_VERSION_1;
      dnsNameResolved = false;
      memset(dnsName, 0, sizeof(dnsName));
      memset(agentVersion, 0, sizeof(agentVersion));
      memset(platform, 0, sizeof(platform));
   }

   virtual ~DiscoveryFilterData()
   {
      delete ifList;
      delete driverData;
   }
};

/**
 * Discovered address information
 */
struct DiscoveredAddress
{
   MacAddress macAddr;
   InetAddress ipAddr;
   int32_t zoneUIN;
   uint32_t sourceNodeId;
   DiscoveredAddressSourceType sourceType;
   bool ignoreFilter;
   DiscoveryFilterData *data;
   SNMP_Transport *snmpTransport;
   shared_ptr<AgentConnectionEx> agentConnection;
   uint16_t agentPort;
   TCHAR agentSecret[MAX_SECRET_LENGTH];
   SSHCredentials sshCredentials;
   uint16_t sshPort;

   DiscoveredAddress(const InetAddress& _ipAddr, int32_t _zoneUIN, uint32_t _sourceNodeId, DiscoveredAddressSourceType _sourceType) : ipAddr(_ipAddr)
   {
      zoneUIN = _zoneUIN;
      sourceNodeId = _sourceNodeId;
      sourceType = _sourceType;
      ignoreFilter = false;
      data = nullptr;
      snmpTransport = nullptr;
      agentPort = AGENT_LISTEN_PORT;
      agentSecret[0] = 0;
      memset(&sshCredentials, 0, sizeof(SSHCredentials));
      sshPort = 0;
   }

   ~DiscoveredAddress()
   {
      delete data;
      delete snmpTransport;
   }
};

/**
 * "DiscoveredNode" NXSL class
 */
class NXSL_DiscoveredNodeClass : public NXSL_Class
{
public:
   NXSL_DiscoveredNodeClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const NXSL_Identifier& attr) override;
};

/**
 * "DiscoveredInterface" NXSL class
 */
class NXSL_DiscoveredInterfaceClass : public NXSL_Class
{
public:
   NXSL_DiscoveredInterfaceClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const NXSL_Identifier& attr) override;
};

extern NXSL_DiscoveredNodeClass g_nxslDiscoveredNodeClass;
extern NXSL_DiscoveredInterfaceClass g_nxslDiscoveredInterfaceClass;

void CheckPotentialNode(const InetAddress& ipAddr, int32_t zoneUIN, DiscoveredAddressSourceType sourceType, uint32_t sourceNodeId);
void EnqueueDiscoveredAddress(DiscoveredAddress *address);

int64_t GetDiscoveryPollerQueueSize();

/**
 * Per-host record produced by interactive network range scan. Accumulates
 * protocol detection state as parallel scanners report hits for the same IP.
 */
struct InteractiveScanRecord
{
   uint32_t protocolFlags;
   uint32_t rtt;
   uint16_t agentPort;
   int16_t snmpVersion;
   IntegerArray<uint16_t> openTcpPorts;

   InteractiveScanRecord() : openTcpPorts(0, 4)
   {
      protocolFlags = 0;
      rtt = 0;
      agentPort = 0;
      snmpVersion = -1;
   }
};

/**
 * Caller-supplied context for an interactive network range scan. The emitter
 * is invoked (potentially from multiple worker threads) each time a probe hit
 * updates the per-host record; the snapshot passed in reflects the latest
 * accumulated state for that address. cancelCheck (if set) is polled between
 * protocol phases to allow early termination.
 */
class NXCORE_EXPORTABLE InteractiveScanContext
{
public:
   InetAddress startAddress;
   InetAddress endAddress;
   int32_t zoneUIN;
   uint32_t flags;
   IntegerArray<uint16_t> tcpPorts;
   std::function<void(const InetAddress&, const InteractiveScanRecord&)> emitter;
   std::function<bool()> cancelCheck;
   std::function<void(uint32_t)> errorReporter;
   std::function<void(uint32_t)> warningReporter;

   InteractiveScanContext() : tcpPorts(0, 4)
   {
      zoneUIN = 0;
      flags = 0;
   }
};

void NXCORE_EXPORTABLE ScanNetworkRangeInteractive(const InteractiveScanContext& context, uint32_t *reportedHosts);

#endif
