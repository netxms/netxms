/*
** NetXMS - Network Management System
** Drivers for UNIX-like systems accessible via SSH
** Copyright (C) 2026 Raden Solutions
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
** File: unix.h
**
**/

#ifndef _unix_h_
#define _unix_h_

#include <nddrv.h>

#define DEBUG_TAG_UNIX  L"ndd.unix"

/**
 * Base class for drivers of UNIX-like systems accessible via SSH command channel only.
 * Device is identified by kernel name reported by "uname -s".
 */
class UnixDriver : public NetworkDeviceDriver
{
protected:
   virtual const char *getKernelName() const = 0;

   static bool executeCommand(DeviceContext *context, const char *command, ByteStream *output);

public:
   virtual const wchar_t *getVersion() override;

   virtual uint32_t getSupportedTransports() const override;
   virtual uint32_t getRequiredTransports() const override;

   virtual int isPotentialDevice(const SNMP_ObjectId& oid) override;
   virtual bool isDeviceSupported(DeviceContext *context, const SNMP_ObjectId& oid) override;
};

/**
 * State of Linux network interface
 */
struct LinuxInterfaceState
{
   uint32_t ifIndex;
   char name[64];
   InterfaceAdminState adminState;
   InterfaceOperState operState;
   uint64_t speed;   // bits per second, 0 if unknown
};

/**
 * Linux driver data. Interface states are read for all interfaces at once and cached for short time,
 * so status poll of all node's interfaces requires single command execution.
 */
class LinuxDriverData : public DriverData
{
public:
   Mutex mutex;
   StructArray<LinuxInterfaceState> interfaceStates;
   int64_t interfaceStateTimestamp;

   LinuxDriverData() : mutex(MutexType::FAST), interfaceStates(0, 16)
   {
      interfaceStateTimestamp = 0;
   }
};

/**
 * Linux
 */
class LinuxDriver : public UnixDriver
{
protected:
   virtual const char *getKernelName() const override;

   static bool readInterfaceStates(DeviceContext *context, StructArray<LinuxInterfaceState> *states);

public:
   virtual const wchar_t *getName() override;

   virtual void analyzeDevice(DeviceContext *context, const SNMP_ObjectId& oid, NObject *node, DriverData **driverData) override;
   virtual void getInterfaceState(DeviceContext *context, NObject *node, DriverData *driverData, uint32_t ifIndex, const wchar_t *ifName,
         uint32_t ifType, int ifTableSuffixLen, const uint32_t *ifTableSuffix, InterfaceAdminState *adminState, InterfaceOperState *operState, uint64_t *speed) override;

   virtual bool getHardwareInformation(DeviceContext *context, NObject *node, DriverData *driverData, DeviceHardwareInfo *hwInfo) override;
   virtual bool getVirtualizationType(DeviceContext *context, NObject *node, DriverData *driverData, VirtualizationType *vtype) override;
   virtual InterfaceList *getInterfaces(DeviceContext *context, NObject *node, DriverData *driverData, bool useIfXTable) override;
   virtual shared_ptr<ArpCache> getArpCache(DeviceContext *context, DriverData *driverData) override;
};

/**
 * FreeBSD
 */
class FreeBSDDriver : public UnixDriver
{
protected:
   virtual const char *getKernelName() const override;

public:
   virtual const wchar_t *getName() override;
};

/**
 * NetBSD
 */
class NetBSDDriver : public UnixDriver
{
protected:
   virtual const char *getKernelName() const override;

public:
   virtual const wchar_t *getName() override;
};

/**
 * OpenBSD
 */
class OpenBSDDriver : public UnixDriver
{
protected:
   virtual const char *getKernelName() const override;

public:
   virtual const wchar_t *getName() override;
};

#endif
