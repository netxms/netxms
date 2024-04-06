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

   DiscoveredAddress(const InetAddress& _ipAddr, int32_t _zoneUIN, uint32_t _sourceNodeId, DiscoveredAddressSourceType _sourceType) : ipAddr(_ipAddr)
   {
      zoneUIN = _zoneUIN;
      sourceNodeId = _sourceNodeId;
      sourceType = _sourceType;
      ignoreFilter = false;
      data = nullptr;
      snmpTransport = nullptr;
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

#endif
