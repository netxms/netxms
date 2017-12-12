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

/**
 * Device data
 */
class RittalDriverData : public DriverData
{
private:
   StringMap m_oids;
   time_t m_cacheTimestamp;

   void updateOidCache(SNMP_Transport *snmp);
   UINT32 cacheUpdateWalkCallback(SNMP_Variable *v, SNMP_Transport *snmp);

public:
   RittalDriverData();
   virtual ~RittalDriverData();

   const TCHAR *getMetricOid(const TCHAR *name, SNMP_Transport *snmp);
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
