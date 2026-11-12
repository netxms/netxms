/*
** NetXMS - Network Management System
** Drivers for Dell devices
** Copyright (C) 2003-2026 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: dhcp.cpp
**
**/

#include "dell.h"
#include <map>

#define DEBUG_TAG_DELL L"ndd.dell.powerconnect"

/**
 * Base OID of DNOS-DHCPSERVER-PRIVATE-MIB (fastPathDHCPServerPrivate)
 */
#define DNOS_DHCP_MIB   1, 3, 6, 1, 4, 1, 674, 10895, 5000, 2, 6132, 1, 1, 12
#define DNOS_DHCP_MIB_LEN  14

/**
 * DHCP server pool (from agentDhcpServerPoolConfigTable and agentDhcpServerPoolAllocationTable)
 */
struct DnosPool
{
   MutableString name;
   InetAddress address;
   uint32_t prefixLength;
   uint32_t type;   // 0 = un-allocated, 1 = dynamic, 2 = manual

   DnosPool()
   {
      prefixLength = 0;
      type = 0;
   }
};

/**
 * Lease (from agentDhcpServerLeaseTable)
 */
struct DnosLease
{
   MacAddress macAddress;
   uint32_t remainingTime;  // in 1/100 seconds
   uint32_t type;           // 1 = automatic, 2 = manual

   DnosLease()
   {
      remainingTime = 0;
      type = 1;
   }
};

/**
 * Walk column of table indexed by single integer. Handler receives row index.
 */
static uint32_t WalkPoolColumn(SNMP_Transport *snmp, uint32_t table, uint32_t column, std::function<void (uint32_t, SNMP_Variable*)> handler)
{
   uint32_t oid[] = { DNOS_DHCP_MIB, 2, table, 1, column };
   return SnmpWalk(snmp, oid, sizeof(oid) / sizeof(uint32_t),
      [handler] (SNMP_Variable *v) -> uint32_t
      {
         const SNMP_ObjectId& name = v->getName();
         if (name.length() == DNOS_DHCP_MIB_LEN + 5)
            handler(name.getLastElement(), v);
         return SNMP_ERR_SUCCESS;
      });
}

/**
 * Walk column of lease table (indexed by IP address). Handler receives leased address.
 */
static uint32_t WalkLeaseColumn(SNMP_Transport *snmp, uint32_t column, std::function<void (uint32_t, SNMP_Variable*)> handler)
{
   uint32_t oid[] = { DNOS_DHCP_MIB, 3, 2, 1, column };
   return SnmpWalk(snmp, oid, sizeof(oid) / sizeof(uint32_t),
      [handler] (SNMP_Variable *v) -> uint32_t
      {
         const SNMP_ObjectId& name = v->getName();
         if (name.length() == DNOS_DHCP_MIB_LEN + 8)
         {
            const uint32_t *index = name.value() + DNOS_DHCP_MIB_LEN + 4;
            handler((index[0] << 24) | (index[1] << 16) | (index[2] << 8) | index[3], v);
         }
         return SNMP_ERR_SUCCESS;
      });
}

/**
 * Read DHCP server information from device implementing DNOS-DHCPSERVER-PRIVATE-MIB. Scope is created for each
 * dynamic pool. Either of output arrays can be nullptr.
 */
static DataCollectionError ReadDhcpData(SNMP_Transport *snmp, ObjectArray<DhcpScopeInfo> *scopes, ObjectArray<DhcpLeaseInfo> *leases)
{
   // agentDhcpServerAdminMode
   uint32_t adminMode = 0;
   uint32_t rc = SnmpGetEx(snmp, { DNOS_DHCP_MIB, 1, 1, 0 }, &adminMode, sizeof(uint32_t), 0);
   if (rc != SNMP_ERR_SUCCESS)
      return (rc == SNMP_ERR_TIMEOUT) ? DCE_COMM_ERROR : DCE_NOT_SUPPORTED;

   std::map<uint32_t, DnosPool> pools;
   rc = WalkPoolColumn(snmp, 2, 2, [&pools] (uint32_t index, SNMP_Variable *v) -> void
      {
         wchar_t buffer[64];
         pools[index].name = v->getValueAsString(buffer, 64);
      });
   if (rc == SNMP_ERR_SUCCESS)
      rc = WalkPoolColumn(snmp, 3, 2, [&pools] (uint32_t index, SNMP_Variable *v) -> void
         {
            wchar_t buffer[64];
            pools[index].address = InetAddress::parse(v->getValueAsIPAddr(buffer));
         });
   if (rc == SNMP_ERR_SUCCESS)
      rc = WalkPoolColumn(snmp, 3, 4, [&pools] (uint32_t index, SNMP_Variable *v) -> void
         {
            pools[index].prefixLength = v->getValueAsUInt();
         });
   if (rc == SNMP_ERR_SUCCESS)
      rc = WalkPoolColumn(snmp, 3, 5, [&pools] (uint32_t index, SNMP_Variable *v) -> void
         {
            pools[index].type = v->getValueAsUInt();
         });
   if (rc != SNMP_ERR_SUCCESS)
      return DCE_COMM_ERROR;

   // Pools without dynamic allocation are host pools for manual bindings
   ObjectArray<DhcpScopeInfo> scopeList(0, 16, Ownership::True);
   for(auto it = pools.begin(); it != pools.end(); it++)
   {
      DnosPool& pool = it->second;
      if ((pool.type != 1) || !pool.address.isValid() || (pool.prefixLength == 0) || (pool.prefixLength > 30))
         continue;
      InetAddress network(pool.address);
      network.setMaskBits(pool.prefixLength);
      network = network.getSubnetAddress();

      auto scope = new DhcpScopeInfo();
      scope->setNetwork(network);
      scope->name = pool.name;
      scope->enabled = (adminMode == 1);
      scope->totalAddresses = (static_cast<uint64_t>(1) << (32 - pool.prefixLength)) - 2;

      wchar_t first[32], last[32];
      InetAddress(network.getAddressV4() + 1).toString(first);
      InetAddress(network.getSubnetBroadcast().getAddressV4() - 1).toString(last);
      StringBuffer ranges(first);
      ranges.append(L'-');
      ranges.append(last);
      scope->ranges = ranges;
      scopeList.add(scope);
   }

   // Excluded address ranges reduce number of addresses available in pools
   std::map<uint32_t, std::pair<uint32_t, uint32_t>> excludedRanges;
   rc = WalkPoolColumn(snmp, 5, 2, [&excludedRanges] (uint32_t index, SNMP_Variable *v) -> void
      {
         wchar_t buffer[64];
         excludedRanges[index].first = InetAddress::parse(v->getValueAsIPAddr(buffer)).getAddressV4();
      });
   if (rc == SNMP_ERR_SUCCESS)
      rc = WalkPoolColumn(snmp, 5, 3, [&excludedRanges] (uint32_t index, SNMP_Variable *v) -> void
         {
            wchar_t buffer[64];
            excludedRanges[index].second = InetAddress::parse(v->getValueAsIPAddr(buffer)).getAddressV4();
         });
   if (rc != SNMP_ERR_SUCCESS)
      return DCE_COMM_ERROR;
   for(int i = 0; i < scopeList.size(); i++)
   {
      DhcpScopeInfo *scope = scopeList.get(i);
      uint64_t low = static_cast<uint64_t>(scope->network.getAddressV4()) + 1;
      uint64_t high = static_cast<uint64_t>(scope->network.getSubnetBroadcast().getAddressV4()) - 1;
      for(auto it = excludedRanges.begin(); it != excludedRanges.end(); it++)
      {
         uint64_t start = std::max(low, static_cast<uint64_t>(it->second.first));
         uint64_t end = std::min(high, static_cast<uint64_t>(it->second.second));
         if ((start <= end) && (it->second.first <= it->second.second))
         {
            uint64_t count = end - start + 1;
            scope->totalAddresses = (scope->totalAddresses > count) ? scope->totalAddresses - count : 0;
         }
      }
   }

   std::map<uint32_t, DnosLease> leaseMap;
   rc = WalkLeaseColumn(snmp, 3, [&leaseMap] (uint32_t address, SNMP_Variable *v) -> void
      {
         leaseMap[address].macAddress = v->getValueAsMACAddr();
      });
   if (rc == SNMP_ERR_SUCCESS)
      rc = WalkLeaseColumn(snmp, 4, [&leaseMap] (uint32_t address, SNMP_Variable *v) -> void
         {
            leaseMap[address].remainingTime = v->getValueAsUInt();
         });
   if (rc == SNMP_ERR_SUCCESS)
      rc = WalkLeaseColumn(snmp, 5, [&leaseMap] (uint32_t address, SNMP_Variable *v) -> void
         {
            leaseMap[address].type = v->getValueAsUInt();
         });
   if (rc != SNMP_ERR_SUCCESS)
      return DCE_COMM_ERROR;

   time_t now = time(nullptr);
   for(auto it = leaseMap.begin(); it != leaseMap.end(); it++)
   {
      InetAddress address(it->first);
      DhcpScopeInfo *scope = FindDhcpScopeForAddress(scopeList, address);
      if (scope != nullptr)
         scope->usedAddresses++;

      if (leases == nullptr)
         continue;

      auto lease = new DhcpLeaseInfo();
      lease->ipAddress = address;
      if (scope != nullptr)
         lease->scopeId = scope->id;
      lease->macAddress = it->second.macAddress;
      lease->type = (it->second.type == 2) ? DHCP_LEASE_TYPE_RESERVATION : DHCP_LEASE_TYPE_DYNAMIC;
      lease->state = DHCP_LEASE_STATE_ACTIVE;
      if ((lease->type == DHCP_LEASE_TYPE_DYNAMIC) && (it->second.remainingTime > 0))
         lease->expiration = now + static_cast<time_t>(it->second.remainingTime / 100);
      leases->add(lease);
   }

   for(int i = 0; i < scopeList.size(); i++)
   {
      DhcpScopeInfo *scope = scopeList.get(i);
      scope->freeAddresses = (scope->totalAddresses > scope->usedAddresses) ? scope->totalAddresses - scope->usedAddresses : 0;
   }

   if (scopes != nullptr)
   {
      if (scopeList.isEmpty())
         return DCE_NOT_SUPPORTED;   // DHCP server not configured
      scopeList.setOwner(Ownership::False);
      for(int i = 0; i < scopeList.size(); i++)
         scopes->add(scopeList.get(i));
   }
   return DCE_SUCCESS;
}

/**
 * Get DHCP scopes (DHCP server pools from DNOS-DHCPSERVER-PRIVATE-MIB). Number of declined addresses is not reported.
 */
DataCollectionError PowerConnectDriver::getDhcpScopes(DeviceContext *context, NObject *node, DriverData *driverData, ObjectArray<DhcpScopeInfo> *scopes)
{
   DataCollectionError rc = ReadDhcpData(context->getSNMPTransport(), scopes, nullptr);
   nxlog_debug_tag(DEBUG_TAG_DELL, 6, L"PowerConnectDriver::getDhcpScopes: %hs (%d scopes)", DataCollectionErrorName(rc), scopes->size());
   return rc;
}

/**
 * Get DHCP leases (from DNOS-DHCPSERVER-PRIVATE-MIB)
 */
DataCollectionError PowerConnectDriver::getDhcpLeases(DeviceContext *context, NObject *node, DriverData *driverData, ObjectArray<DhcpLeaseInfo> *leases)
{
   DataCollectionError rc = ReadDhcpData(context->getSNMPTransport(), nullptr, leases);
   nxlog_debug_tag(DEBUG_TAG_DELL, 6, L"PowerConnectDriver::getDhcpLeases: %hs (%d leases)", DataCollectionErrorName(rc), leases->size());
   return rc;
}
