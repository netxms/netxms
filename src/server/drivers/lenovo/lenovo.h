/*
** NetXMS - Network Management System
** Drivers for Lenovo devices
** Copyright (C) 2003-2026 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: lenovo.h
**
**/

#ifndef _lenovo_h_
#define _lenovo_h_

#include <nddrv.h>
#include <netxms-version.h>

/**
 * Driver for Lenovo ThinkSystem rack switches
 */
class LenovoRackDriver : public NetworkDeviceDriver
{
public:
   virtual const TCHAR *getName() override;
   virtual const TCHAR *getVersion() override;

   virtual int isPotentialDevice(const SNMP_ObjectId& oid) override;
   virtual bool isDeviceSupported(DeviceContext *context, const SNMP_ObjectId& oid) override;
   virtual InterfaceList *getInterfaces(DeviceContext *context, NObject *node, DriverData *driverData, bool useIfXTable) override;
};

/**
 * Driver for Lenovo Flex System Fabric switches
 */
class LenovoFlexDriver : public NetworkDeviceDriver
{
public:
   virtual const TCHAR *getName() override;
   virtual const TCHAR *getVersion() override;

   virtual int isPotentialDevice(const SNMP_ObjectId& oid) override;
   virtual bool isDeviceSupported(DeviceContext *context, const SNMP_ObjectId& oid) override;
   virtual InterfaceList *getInterfaces(DeviceContext *context, NObject *node, DriverData *driverData, bool useIfXTable) override;
   virtual int getModulesOrientation(DeviceContext *context, NObject *node, DriverData *driverData) override;
   virtual void getModuleLayout(DeviceContext *context, NObject *node, DriverData *driverData, int module, NDD_MODULE_LAYOUT *layout) override;
};

#endif
