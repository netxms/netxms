/*
** NetXMS - Network Management System
** Drivers for Ubiquiti Networks devices
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
** File: unifi.h
**
**/

#ifndef _unifi_h_
#define _unifi_h_

#include <nddrv.h>

#define DEBUG_TAG_UNIFI _T("ndd.unifi")

/**
 * Driver for UniFi devices
 */
class UbiquitiUniFiDriver : public NetworkDeviceDriver
{
public:
   UbiquitiUniFiDriver();

   virtual const TCHAR *getName() override;
   virtual const TCHAR *getVersion() override;

   virtual int isPotentialDevice(const SNMP_ObjectId &oid) override;
   virtual bool isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId &oid) override;
   virtual bool isWirelessAccessPoint(SNMP_Transport *snmp, NObject *node, DriverData *driverData) override;
   virtual StructArray<RadioInterfaceInfo> *getRadioInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData) override;
   virtual ObjectArray<WirelessStationInfo> *getWirelessStations(SNMP_Transport *snmp, NObject *node, DriverData *driverData) override;
   virtual bool getHardwareInformation(SNMP_Transport *snmp, NObject *node, DriverData *driverData, DeviceHardwareInfo *hwInfo) override;
};

#endif /* _unifi_h_ */
