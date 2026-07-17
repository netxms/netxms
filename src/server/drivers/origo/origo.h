/*
** NetXMS - Network Management System
** Driver for Origo Networks switches
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
** File: origo.h
**
**/

#ifndef _origo_h_
#define _origo_h_

#include <nddrv.h>
#include <netxms-version.h>

/**
 * Driver for Origo Networks switches
 */
class OrigoDriver : public NetworkDeviceDriver
{
public:
   virtual const TCHAR *getName() override;
   virtual const TCHAR *getVersion() override;

   virtual int isPotentialDevice(const SNMP_ObjectId& oid) override;
   virtual bool isDeviceSupported(DeviceContext *context, const SNMP_ObjectId& oid) override;
   virtual InterfaceList *getInterfaces(DeviceContext *context, NObject *node, DriverData *driverData, bool useIfXTable) override;
   virtual bool getHardwareInformation(DeviceContext *context, NObject *node, DriverData *driverData, DeviceHardwareInfo *hwInfo) override;
   virtual void getModuleLayout(DeviceContext *context, NObject *node, DriverData *driverData, int module, NDD_MODULE_LAYOUT *layout) override;
};

#endif
