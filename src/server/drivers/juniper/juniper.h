/* 
** NetXMS - Network Management System
** Driver for Juniper Networks devices
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
** File: juniper.h
**
**/

#ifndef _juniper_h_
#define _juniper_h_

#include <nddrv.h>

#define JUNIPER_DEBUG_TAG   _T("ndd.juniper")

/**
 * Driver's class
 */
class JuniperDriver : public NetworkDeviceDriver
{
private:
   VlanList *getVlansDot1q(SNMP_Transport *snmp, NObject *node, DriverData *driverData);

public:
	virtual const TCHAR *getName() override;
	virtual const TCHAR *getVersion() override;

	virtual int isPotentialDevice(const SNMP_ObjectId& oid) override;
	virtual bool isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid) override;
   virtual bool getHardwareInformation(SNMP_Transport *snmp, NObject *node, DriverData *driverData, DeviceHardwareInfo *hwInfo) override;
	virtual InterfaceList *getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable) override;
   virtual VlanList *getVlans(SNMP_Transport *snmp, NObject *node, DriverData *driverData) override;
   virtual int getModulesOrientation(SNMP_Transport *snmp, NObject *node, DriverData *driverData) override;
   virtual void getModuleLayout(SNMP_Transport *snmp, NObject *node, DriverData *driverData, int module, NDD_MODULE_LAYOUT *layout) override;
   virtual bool isLldpRemTableUsingIfIndex(const NObject *node, DriverData *driverData) override;
};

/**
 * Driver for Juniper ScreenOS devices (former Netscreen)
 */
class NetscreenDriver : public NetworkDeviceDriver
{
public:
   virtual const TCHAR *getName() override;
   virtual const TCHAR *getVersion() override;

   virtual int isPotentialDevice(const SNMP_ObjectId& oid) override;
   virtual bool isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid) override;
   virtual InterfaceList *getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable) override;
};

#endif
