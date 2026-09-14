/*
** NetXMS - Network Management System
** Drivers for Mellanox devices
** Copyright (C) 2026 Raden Solutions
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
** File: mellanox.h
**
**/

#ifndef _mellanox_h_
#define _mellanox_h_

#include <nddrv.h>
#include <netxms-version.h>

#define DEBUG_TAG_MELLANOX _T("ndd.mellanox")

/**
 * Driver for Mellanox / NVIDIA switches running Onyx (MLNX-OS)
 */
class MellanoxOnyxDriver : public NetworkDeviceDriver
{
public:
   virtual const TCHAR *getName() override;
   virtual const TCHAR *getVersion() override;

   virtual int isPotentialDevice(const SNMP_ObjectId& oid) override;
   virtual bool isDeviceSupported(DeviceContext *context, const SNMP_ObjectId& oid) override;
   virtual void getModuleLayout(DeviceContext *context, NObject *node, DriverData *driverData, int module, NDD_MODULE_LAYOUT *layout) override;
   virtual InterfaceList *getInterfaces(DeviceContext *context, NObject *node, DriverData *driverData, bool useIfXTable) override;
};

#endif
