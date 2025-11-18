/* 
** NetXMS - Network Management System
** Drivers for Planet Technology Corp. devices
** Copyright (C) 2003-2025 Raden Solutions
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
** File: planet.h
**
**/

#ifndef _planet_h_
#define _planet_h_

#include <nddrv.h>
#include <netxms-version.h>

/**
 * Device data
 */
class LanSwDriverData : public DriverData
{
private:
   SNMP_Snapshot *m_cardTable;
   time_t m_cacheTimestamp;
   Mutex m_cacheLock;

public:
   LanSwDriverData() : m_cacheLock(MutexType::FAST)
   {
      m_cardTable = nullptr;
      m_cacheTimestamp = 0;
   }
   virtual ~LanSwDriverData();

   DataCollectionError getMetric(SNMP_Transport *transport, uint32_t cardIndex, uint32_t metric, wchar_t *value, size_t size);
};

/**
 * Driver for LAN switches
 */
class PlanetLanSwDriver : public NetworkDeviceDriver
{
public:
   virtual const wchar_t *getName() override;
   virtual const wchar_t *getVersion() override;

   virtual int isPotentialDevice(const SNMP_ObjectId& oid) override;
   virtual bool isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid) override;
   virtual void analyzeDevice(SNMP_Transport *snmp, const SNMP_ObjectId& oid, NObject *node, DriverData **driverData) override;
   virtual bool getHardwareInformation(SNMP_Transport *snmp, NObject *node, DriverData *driverData, DeviceHardwareInfo *hwInfo) override;
   virtual InterfaceList *getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable) override;
   virtual bool hasMetrics() override;
   virtual DataCollectionError getMetric(SNMP_Transport *snmp, NObject *node, DriverData *driverData, const wchar_t *name, wchar_t *value, size_t size) override;
   virtual ObjectArray<AgentParameterDefinition> *getAvailableMetrics(SNMP_Transport *snmp, NObject *node, DriverData *driverData) override;
};

/**
 * Driver for industrial switches
 */
class PlanetIndustrialSwDriver : public NetworkDeviceDriver
{
public:
   virtual const wchar_t *getName() override;
   virtual const wchar_t *getVersion() override;

   virtual int isPotentialDevice(const SNMP_ObjectId& oid) override;
   virtual bool isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid) override;
   virtual bool getHardwareInformation(SNMP_Transport *snmp, NObject *node, DriverData *driverData, DeviceHardwareInfo *hwInfo) override;
   virtual InterfaceList *getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable) override;
};

#endif
