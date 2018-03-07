/* 
** NetXMS - Network Management System
** Drivers for HPE (Hewlett Packard Enterprise) devices
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
** File: hpe.h
**
**/

#ifndef _hpe_h_
#define _hpe_h_

#include <nddrv.h>

/**
 * Driver for H3C (now HP A-series) switches
 */
class H3CDriver : public NetworkDeviceDriver
{
public:
	virtual const TCHAR *getName();
	virtual const TCHAR *getVersion();

	virtual int isPotentialDevice(const TCHAR *oid);
	virtual bool isDeviceSupported(SNMP_Transport *snmp, const TCHAR *oid);
	virtual void analyzeDevice(SNMP_Transport *snmp, const TCHAR *oid, StringMap *attributes, DriverData **driverData);
	virtual InterfaceList *getInterfaces(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData, int useAliases, bool useIfXTable);
   virtual int getModulesOrientation(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData);
   virtual void getModuleLayout(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData, int module, NDD_MODULE_LAYOUT *layout);
};

/**
 * Driver for HP switches with HH3C MIB support
 */
class HPSwitchDriver : public NetworkDeviceDriver
{
public:
   virtual const TCHAR *getName();
   virtual const TCHAR *getVersion();

   virtual int isPotentialDevice(const TCHAR *oid);
   virtual bool isDeviceSupported(SNMP_Transport *snmp, const TCHAR *oid);
   virtual void analyzeDevice(SNMP_Transport *snmp, const TCHAR *oid, StringMap *attributes, DriverData **driverData);
   virtual InterfaceList *getInterfaces(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData, int useAliases, bool useIfXTable);
};

/**
 * Driver for HP ProCurve switches
 */
class ProCurveDriver : public NetworkDeviceDriver
{
public:
   virtual const TCHAR *getName();
   virtual const TCHAR *getVersion();

   virtual int isPotentialDevice(const TCHAR *oid);
   virtual bool isDeviceSupported(SNMP_Transport *snmp, const TCHAR *oid);
   virtual void analyzeDevice(SNMP_Transport *snmp, const TCHAR *oid, StringMap *attributes, DriverData **driverData);
   virtual InterfaceList *getInterfaces(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData, int useAliases, bool useIfXTable);
};

#endif
