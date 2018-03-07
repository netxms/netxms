/* 
** NetXMS - Network Management System
** Drivers for Avaya devices
** Copyright (C) 2003-2018 Victor Kirhenshtein
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
** File: avaya.h
**
**/

#ifndef _avaya_h_
#define _avaya_h_

#include <nddrv.h>

/**
 * Generic driver for Avaya ERS switches (former Nortel)
 */
class AvayaERSDriver : public NetworkDeviceDriver
{
protected:
	virtual UINT32 getSlotSize(StringMap *attributes);

	void getVlanInterfaces(SNMP_Transport *pTransport, InterfaceList *pIfList);

public:
   virtual const TCHAR *getVersion();
	virtual VlanList *getVlans(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData);
};

/**
 * Driver for BayStack switches
 */
class BayStackDriver : public AvayaERSDriver
{
protected:
   virtual UINT32 getSlotSize(StringMap *attributes);

public:
   virtual const TCHAR *getName();

   virtual int isPotentialDevice(const TCHAR *oid);
   virtual bool isDeviceSupported(SNMP_Transport *snmp, const TCHAR *oid);
   virtual void analyzeDevice(SNMP_Transport *snmp, const TCHAR *oid, StringMap *attributes, DriverData **driverData);
   virtual InterfaceList *getInterfaces(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData, int useAliases, bool useIfXTable);
};

/**
 * Driver for Avaya ERS 8xxx switches (former Nortel/Bay Networks Passport)
 */
class PassportDriver : public AvayaERSDriver
{
public:
   virtual const TCHAR *getName();

   virtual int isPotentialDevice(const TCHAR *oid);
   virtual bool isDeviceSupported(SNMP_Transport *snmp, const TCHAR *oid);
   virtual void analyzeDevice(SNMP_Transport *snmp, const TCHAR *oid, StringMap *attributes, DriverData **driverData);
   virtual InterfaceList *getInterfaces(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData, int useAliases, bool useIfXTable);
};

/**
 * Driver for Nortel WLAN Security Switch series
 */
class NtwsDriver : public NetworkDeviceDriver
{
public:
   virtual const TCHAR *getName();
   virtual const TCHAR *getVersion();

   virtual int isPotentialDevice(const TCHAR *oid);
   virtual bool isDeviceSupported(SNMP_Transport *snmp, const TCHAR *oid);
   virtual void analyzeDevice(SNMP_Transport *snmp, const TCHAR *oid, StringMap *attributes, DriverData **driverData);
   virtual int getClusterMode(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData);
   virtual bool isWirelessController(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData);
   virtual ObjectArray<AccessPointInfo> *getAccessPoints(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData);
   virtual ObjectArray<WirelessStationInfo> *getWirelessStations(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData);
};

#endif
