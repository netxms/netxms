/* 
** NetXMS - Network Management System
** Drivers Cisco devices
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
** File: cisco.h
**
**/

#ifndef _cisco_h_
#define _cisco_h_

#include <nddrv.h>

/**
 * Generic Cisco driver
 */
class CiscoDeviceDriver : public NetworkDeviceDriver
{
public:
   virtual const TCHAR *getVersion();
	virtual VlanList *getVlans(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData);
	virtual bool isPerVlanFdbSupported();
};

/**
 * Catalyst driver
 */
class CatalystDriver : public CiscoDeviceDriver
{
public:
   virtual const TCHAR *getName();

   virtual int isPotentialDevice(const TCHAR *oid);
   virtual bool isDeviceSupported(SNMP_Transport *snmp, const TCHAR *oid);
   virtual InterfaceList *getInterfaces(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData, int useAliases, bool useIfXTable);
};

/**
 * Catalyst 2900 driver
 */
class Cat2900Driver : public CiscoDeviceDriver
{
public:
   virtual const TCHAR *getName();

   virtual int isPotentialDevice(const TCHAR *oid);
   virtual bool isDeviceSupported(SNMP_Transport *snmp, const TCHAR *oid);
   virtual InterfaceList *getInterfaces(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData, int useAliases, bool useIfXTable);
};

/**
 * Cisco ESW driver
 */
class CiscoEswDriver : public NetworkDeviceDriver
{
public:
   virtual const TCHAR *getName();
   virtual const TCHAR *getVersion();

   virtual int isPotentialDevice(const TCHAR *oid);
   virtual bool isDeviceSupported(SNMP_Transport *snmp, const TCHAR *oid);
   virtual InterfaceList *getInterfaces(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData, int useAliases, bool useIfXTable);
};

/**
 * Cisco SB driver
 */
class CiscoSbDriver : public NetworkDeviceDriver
{
public:
   virtual const TCHAR *getName();
   virtual const TCHAR *getVersion();

   virtual int isPotentialDevice(const TCHAR *oid);
   virtual bool isDeviceSupported(SNMP_Transport *snmp, const TCHAR *oid);
   virtual InterfaceList *getInterfaces(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData, int useAliases, bool useIfXTable);
};

#endif
