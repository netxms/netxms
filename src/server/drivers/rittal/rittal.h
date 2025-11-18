/**
 * NetXMS - Network Management System
 * Driver for Rittal CMC and LCP devices
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
 * File: rittal.h
 */

#ifndef _rittal_h_
#define _rittal_h_

#include <nddrv.h>

#define RITTAL_DEBUG_TAG   _T("ndd.rittal")

/**
 * Metric information
 */
struct RittalMetric
{
   uint32_t index;
   TCHAR name[MAX_OBJECT_NAME];
   TCHAR description[MAX_DB_STRING];
   int dataType;
   uint32_t oid[15];
};

/**
 * Device information
 */
struct RittalDevice
{
   uint32_t index;
   uint32_t bus;
   uint32_t position;
   TCHAR name[MAX_OBJECT_NAME];
   TCHAR alias[MAX_OBJECT_NAME];
   ObjectArray<RittalMetric> *metrics;

   RittalDevice(UINT32 _index, UINT32 _bus, UINT32 _position, const SNMP_Variable *_name, const SNMP_Variable *_alias)
   {
      index = _index;
      bus = _bus;
      position = _position;
      _name->getValueAsString(name, MAX_OBJECT_NAME);
      _alias->getValueAsString(alias, MAX_OBJECT_NAME);
      metrics = new ObjectArray<RittalMetric>(32, 32, Ownership::True);
   }

   ~RittalDevice()
   {
      delete metrics;
   }
};

/**
 * Device data
 */
class RittalDriverData : public HostMibDriverData
{
private:
   ObjectArray<RittalDevice> m_devices;
   time_t m_cacheTimestamp;
   Mutex m_cacheLock;

   uint32_t deviceInfoWalkCallback(SNMP_Variable *v, SNMP_Transport *snmp);
   uint32_t metricInfoWalkCallback(SNMP_Variable *v, SNMP_Transport *snmp);
   void updateDeviceInfoInternal(SNMP_Transport *snmp);
   RittalDevice *getDevice(uint32_t index);
   RittalDevice *getDevice(uint32_t bus, uint32_t position);

public:
   RittalDriverData();
   virtual ~RittalDriverData();

   void updateDeviceInfo(SNMP_Transport *snmp);
   void registerMetrics(ObjectArray<AgentParameterDefinition> *metrics);
   bool getMetric(const wchar_t *name, SNMP_Transport *snmp, RittalMetric *metric);
};

/**
 * Driver's class
 */
class RittalDriver : public NetworkDeviceDriver
{
public:
	virtual const wchar_t *getName() override;
	virtual const wchar_t *getVersion() override;

	virtual int isPotentialDevice(const SNMP_ObjectId& oid) override;
	virtual bool isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid) override;
   virtual void analyzeDevice(SNMP_Transport *snmp, const SNMP_ObjectId& oid, NObject *node, DriverData **driverData) override;
   virtual bool hasMetrics() override;
   virtual DataCollectionError getMetric(SNMP_Transport *snmp, NObject *node, DriverData *driverData, const wchar_t *name, wchar_t *value, size_t size) override;
   virtual ObjectArray<AgentParameterDefinition> *getAvailableMetrics(SNMP_Transport *snmp, NObject *node, DriverData *driverData) override;
};

#endif
