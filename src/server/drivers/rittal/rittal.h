/**
 * NetXMS - Network Management System
 * Driver for Rittal CMC and LCP devices
 * Copyright (C) 2017 Raden Solutions
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
   UINT32 index;
   TCHAR name[MAX_OBJECT_NAME];
   TCHAR description[MAX_DB_STRING];
   int dataType;
   UINT32 oid[15];
};

/**
 * Device information
 */
struct RittalDevice
{
   UINT32 index;
   UINT32 bus;
   UINT32 position;
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
      metrics = new ObjectArray<RittalMetric>(32, 32, true);
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
   MUTEX m_cacheLock;

   UINT32 deviceInfoWalkCallback(SNMP_Variable *v, SNMP_Transport *snmp);
   UINT32 metricInfoWalkCallback(SNMP_Variable *v, SNMP_Transport *snmp);
   void updateDeviceInfoInternal(SNMP_Transport *snmp);
   RittalDevice *getDevice(UINT32 index);
   RittalDevice *getDevice(UINT32 bus, UINT32 position);

public:
   RittalDriverData();
   virtual ~RittalDriverData();

   void updateDeviceInfo(SNMP_Transport *snmp);
   void registerMetrics(ObjectArray<AgentParameterDefinition> *metrics);
   bool getMetric(const TCHAR *name, SNMP_Transport *snmp, RittalMetric *metric);
};

/**
 * Driver's class
 */
class RittalDriver : public NetworkDeviceDriver
{
public:
	virtual const TCHAR *getName();
	virtual const TCHAR *getVersion();

	virtual int isPotentialDevice(const TCHAR *oid);
	virtual bool isDeviceSupported(SNMP_Transport *snmp, const TCHAR *oid);
   virtual void analyzeDevice(SNMP_Transport *snmp, const TCHAR *oid, StringMap *attributes, DriverData **driverData);
   virtual bool hasMetrics();
   virtual DataCollectionError getMetric(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData, const TCHAR *name, TCHAR *value, size_t size);
   virtual ObjectArray<AgentParameterDefinition> *getAvailableMetrics(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData);
};

#endif
