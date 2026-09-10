/* 
** NetXMS - Network Management System
** Drivers for Dell network devices
** Copyright (C) 2003-2026 Raden Solutions
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
** File: dell.h
**
**/

#ifndef _dell_h_
#define _dell_h_

#include <nddrv.h>
#include <netxms-version.h>

/**
 * Driver for Dell Networking OS9 (Force10 / FTOS) switches
 */
class DellFTOSDriver : public NetworkDeviceDriver
{
public:
   virtual const TCHAR *getName() override;
   virtual const TCHAR *getVersion() override;

   virtual int isPotentialDevice(const SNMP_ObjectId& oid) override;
   virtual bool getHardwareInformation(SNMP_Transport *snmp, NObject *node, DriverData *driverData, DeviceHardwareInfo *hwInfo) override;
   virtual InterfaceList *getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable) override;
   virtual void getSSHDriverHints(SSHDriverHints *hints) const override;
   virtual bool isConfigBackupSupported() override;
   virtual bool getRunningConfig(DeviceBackupContext *ctx, ByteStream *output) override;
   virtual bool getStartupConfig(DeviceBackupContext *ctx, ByteStream *output) override;
};

/**
 * Driver for Dell EMC Networking OS10 switches
 */
class DellOS10Driver : public NetworkDeviceDriver
{
public:
   virtual const TCHAR *getName() override;
   virtual const TCHAR *getVersion() override;

   virtual int isPotentialDevice(const SNMP_ObjectId& oid) override;
   virtual InterfaceList *getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable) override;
   virtual void getSSHDriverHints(SSHDriverHints *hints) const override;
   virtual bool isConfigBackupSupported() override;
   virtual bool getRunningConfig(DeviceBackupContext *ctx, ByteStream *output) override;
   virtual bool getStartupConfig(DeviceBackupContext *ctx, ByteStream *output) override;
};

/**
 * Driver for Dell PowerConnect switches
 */
class PowerConnectDriver : public NetworkDeviceDriver
{
public:
   virtual const TCHAR *getName() override;
   virtual const TCHAR *getVersion() override;

   virtual int isPotentialDevice(const SNMP_ObjectId& oid) override;
   virtual bool isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid) override;
   virtual void analyzeDevice(SNMP_Transport *snmp, const SNMP_ObjectId& oid, NObject *node, DriverData **driverData) override;
   virtual bool getHardwareInformation(SNMP_Transport *snmp, NObject *node, DriverData *driverData, DeviceHardwareInfo *hwInfo) override;
   virtual InterfaceList *getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable) override;
   virtual void getSSHDriverHints(SSHDriverHints *hints) const override;
   virtual bool isConfigBackupSupported() override;
   virtual bool getRunningConfig(DeviceBackupContext *ctx, ByteStream *output) override;
   virtual bool getStartupConfig(DeviceBackupContext *ctx, ByteStream *output) override;
};

#endif
