/* 
** NetXMS - Network Management System
** Drivers for Avaya devices
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
	virtual uint32_t getSlotSize(NObject *node);

	void getVlanInterfaces(SNMP_Transport *pTransport, InterfaceList *pIfList);

public:
   virtual const TCHAR *getVersion() override;
	virtual VlanList *getVlans(SNMP_Transport *snmp, NObject *node, DriverData *driverData) override;
};

/**
 * Driver for BayStack switches
 */
class BayStackDriver : public AvayaERSDriver
{
protected:
   virtual uint32_t getSlotSize(NObject *node) override;

public:
   virtual const TCHAR *getName() override;

   virtual int isPotentialDevice(const SNMP_ObjectId& oid) override;
   virtual bool isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid) override;
   virtual void analyzeDevice(SNMP_Transport *snmp, const SNMP_ObjectId& oid, NObject *node, DriverData **driverData) override;
   virtual InterfaceList *getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable) override;
};

/**
 * Driver for Avaya ERS 8xxx switches (former Nortel/Bay Networks Passport)
 */
class PassportDriver : public AvayaERSDriver
{
public:
   virtual const TCHAR *getName() override;

   virtual int isPotentialDevice(const SNMP_ObjectId& oid) override;
   virtual bool isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid) override;
   virtual void analyzeDevice(SNMP_Transport *snmp, const SNMP_ObjectId& oid, NObject *node, DriverData **driverData) override;
   virtual InterfaceList *getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable) override;
};

/**
 * Driver for Nortel WLAN Security Switch series
 */
class NtwsDriver : public NetworkDeviceDriver
{
public:
   virtual const TCHAR *getName() override;
   virtual const TCHAR *getVersion() override;

   virtual int isPotentialDevice(const SNMP_ObjectId& oid) override;
   virtual bool isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid) override;
   virtual void analyzeDevice(SNMP_Transport *snmp, const SNMP_ObjectId& oid, NObject *node, DriverData **driverData) override;
   virtual int getClusterMode(SNMP_Transport *snmp, NObject *node, DriverData *driverData) override;
   virtual bool isWirelessController(SNMP_Transport *snmp, NObject *node, DriverData *driverData) override;
   virtual ObjectArray<AccessPointInfo> *getAccessPoints(SNMP_Transport *snmp, NObject *node, DriverData *driverData) override;
   virtual ObjectArray<WirelessStationInfo> *getWirelessStations(SNMP_Transport *snmp, NObject *node, DriverData *driverData) override;
};

#endif
