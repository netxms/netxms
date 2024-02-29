/* 
** NetXMS - Network Management System
** Drivers for Moxa devices
** Copyright (C) 2020-2024 Raden Solutions
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
** File: moxa.h
**
**/

#ifndef _moxa_h_
#define _moxa_h_

#include <nddrv.h>
#include <netxms-version.h>

/**
 * Driver for EDR devices
 */
class MoxaEDRDriver : public NetworkDeviceDriver
{
public:
   virtual const TCHAR *getName() override;
   virtual const TCHAR *getVersion() override;

   virtual int isPotentialDevice(const SNMP_ObjectId& oid) override;
   virtual bool isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid) override;
   virtual bool getHardwareInformation(SNMP_Transport *snmp, NObject *node, DriverData *driverData, DeviceHardwareInfo *hwInfo) override;
   virtual InterfaceList *getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable) override;
   virtual bool lldpNameToInterfaceId(SNMP_Transport *snmp, NObject *node, DriverData *driverData, const TCHAR *lldpName, InterfaceId *id) override;
   virtual VlanList *getVlans(SNMP_Transport *snmp, NObject *node, DriverData *driverData) override;
};

#endif
