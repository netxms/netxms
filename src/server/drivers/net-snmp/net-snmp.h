/**
 * NetXMS - Network Management System
 * Driver for NetSNMP agents
 * Copyright (C) 2017-2024 Raden Solutions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * File: net-snmp.h
 */

#ifndef _net_snmp_h_
#define _net_snmp_h_

#include <nddrv.h>

/**
 * Device data
 */
class NetSnmpDriverData : public HostMibDriverData
{
public:
   NetSnmpDriverData() : HostMibDriverData() { }

   const HostMibStorageEntry *getCacheMemory(SNMP_Transport *snmp) { return getStorageEntry(snmp, _T("Cached memory"), hrStorageOther); }
   const HostMibStorageEntry *getMemoryBuffers(SNMP_Transport *snmp) { return getStorageEntry(snmp, _T("Memory buffers"), hrStorageOther); }
   const HostMibStorageEntry *getSharedMemory(SNMP_Transport *snmp) { return getStorageEntry(snmp, _T("Shared memory"), hrStorageOther); }
   const HostMibStorageEntry *getSwapSpace(SNMP_Transport *snmp) { return getStorageEntry(snmp, _T("Swap space"), hrStorageVirtualMemory); }
   const HostMibStorageEntry *getVirtualMemory(SNMP_Transport *snmp) { return getStorageEntry(snmp, _T("Virtual memory"), hrStorageVirtualMemory); }
};

/**
 * Base driver for Net-SNMP compatible devices
 */
class NetSnmpBaseDriver : public NetworkDeviceDriver
{
public:
   virtual void analyzeDevice(SNMP_Transport *snmp, const SNMP_ObjectId& oid, NObject *node, DriverData **driverData) override;
   virtual bool hasMetrics() override;
   virtual DataCollectionError getMetric(SNMP_Transport *snmp, NObject *node, DriverData *driverData, const TCHAR *name, TCHAR *value, size_t size) override;
   virtual ObjectArray<AgentParameterDefinition> *getAvailableMetrics(SNMP_Transport *snmp, NObject *node, DriverData *driverData) override;
};

/**
 * Generic driver for Net-SNMP compatible devices
 */
class NetSnmpDriver : public NetSnmpBaseDriver
{
public:
   virtual const TCHAR *getName() override;
   virtual const TCHAR *getVersion() override;

   virtual int isPotentialDevice(const SNMP_ObjectId& oid) override;
   virtual bool isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid) override;
};

/**
 * Driver for Teltonika modems
 */
class TeltonikaDriver : public NetSnmpBaseDriver
{
public:
   virtual const TCHAR *getName() override;
   virtual const TCHAR *getVersion() override;

   virtual int isPotentialDevice(const SNMP_ObjectId& oid) override;
   virtual bool isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid) override;
   virtual bool getHardwareInformation(SNMP_Transport *snmp, NObject *node, DriverData *driverData, DeviceHardwareInfo *hwInfo) override;
};

#endif
