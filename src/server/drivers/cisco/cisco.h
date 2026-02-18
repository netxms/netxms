/* 
** NetXMS - Network Management System
** Drivers Cisco devices
** Copyright (C) 2003-2026 Victor Kirhenshtein
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

#define DEBUG_TAG_CISCO _T("ndd.cisco")

/**
 * Strip diagnostic preamble from Cisco configuration output.
 * Removes lines like "Building configuration..." that appear before the actual config
 * which always starts with a comment line (!)
 */
static inline void StripCiscoConfigPreamble(ByteStream *output)
{
   const char *data = reinterpret_cast<const char*>(output->buffer());
   size_t len = output->size();

   // Find first line starting with '!' which marks the beginning of actual configuration
   size_t pos = 0;
   while (pos < len)
   {
      if (data[pos] == '!')
         break;
      // Skip to next line
      while (pos < len && data[pos] != '\n')
         pos++;
      if (pos < len)
         pos++;
   }

   if (pos > 0 && pos < len)
      output->truncateLeft(pos);
}

/**
 * Generic Cisco driver
 */
class CiscoDeviceDriver : public NetworkDeviceDriver
{
public:
   virtual const TCHAR *getVersion() override;
   virtual InterfaceList *getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable) override;
	virtual VlanList *getVlans(SNMP_Transport *snmp, NObject *node, DriverData *driverData) override;
   virtual StructArray<ForwardingDatabaseEntry> *getForwardingDatabase(SNMP_Transport *snmp, NObject *node, DriverData *driverData) override;
   virtual void getSSHDriverHints(SSHDriverHints *hints) const override;
   virtual bool isConfigBackupSupported() override;
   virtual bool getRunningConfig(DeviceBackupContext *ctx, ByteStream *output) override;
   virtual bool getStartupConfig(DeviceBackupContext *ctx, ByteStream *output) override;
};

/**
 * Generic driver
 */
class GenericCiscoDriver : public CiscoDeviceDriver
{
public:
   virtual const TCHAR *getName() override;

   virtual int isPotentialDevice(const SNMP_ObjectId& oid) override;
   virtual bool isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid) override;
   virtual InterfaceList *getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable) override;
};

/**
 * Catalyst driver
 */
class CatalystDriver : public CiscoDeviceDriver
{
public:
   virtual const TCHAR *getName() override;

   virtual int isPotentialDevice(const SNMP_ObjectId& oid) override;
   virtual bool isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid) override;
   virtual InterfaceList *getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable) override;
};

/**
 * Catalyst 2900 driver
 */
class Cat2900Driver : public CiscoDeviceDriver
{
public:
   virtual const TCHAR *getName() override;

   virtual int isPotentialDevice(const SNMP_ObjectId& oid) override;
   virtual bool isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid) override;
   virtual InterfaceList *getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable) override;
};

/**
 * Cisco ESW driver
 */
class CiscoEswDriver : public NetworkDeviceDriver
{
public:
   virtual const TCHAR *getName() override;

   virtual int isPotentialDevice(const SNMP_ObjectId& oid) override;
   virtual bool isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid) override;
   virtual InterfaceList *getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable) override;
};

/**
 * Max number of modules (stack units) for SB switch
 */
#define SB_MAX_MODULE_NUMBER        64

/**
 * Max number of interfaces per row for SB switch
 */
#define SB_MAX_INTERFACES_PER_ROW   32

/**
 * Cisco SB module (stack unit) layout information
 */
struct SB_MODULE_LAYOUT
{
   UINT32 index;
   UINT32 minIfIndex;
   UINT32 maxIfIndex;
   UINT32 rows;
   UINT32 columns;
   UINT32 interfaces[SB_MAX_INTERFACES_PER_ROW * 2];
};

/**
 * Cisco SB driver
 */
class CiscoSbDriver : public NetworkDeviceDriver
{
private:
   int getPhysicalPortLayout(SNMP_Transport *snmp, SB_MODULE_LAYOUT *layout);

public:
   virtual const TCHAR *getName() override;

   virtual int isPotentialDevice(const SNMP_ObjectId& oid) override;
   virtual bool isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid) override;
   virtual InterfaceList *getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable) override;
   virtual void getModuleLayout(SNMP_Transport *snmp, NObject *node, DriverData *driverData, int module, NDD_MODULE_LAYOUT *layout) override;
};

/**
 * Cisco Nexus driver
 */
class CiscoNexusDriver : public NetworkDeviceDriver
{
public:
   virtual const TCHAR *getName() override;

   virtual int isPotentialDevice(const SNMP_ObjectId& oid) override;
   virtual bool isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid) override;
   virtual InterfaceList *getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable) override;
   virtual VlanList *getVlans(SNMP_Transport *snmp, NObject *node, DriverData *driverData) override;
   virtual void getSSHDriverHints(SSHDriverHints *hints) const override;
   virtual bool isConfigBackupSupported() override;
   virtual bool getRunningConfig(DeviceBackupContext *ctx, ByteStream *output) override;
   virtual bool getStartupConfig(DeviceBackupContext *ctx, ByteStream *output) override;
};

/**
 * Cisco Wireless LAN Controllers driver
 */
class CiscoWirelessControllerDriver : public NetworkDeviceDriver
{
public:
   virtual const TCHAR *getName() override;
   virtual const TCHAR *getVersion() override;

   virtual int isPotentialDevice(const SNMP_ObjectId& oid) override;
   virtual bool isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid) override;
   virtual bool getHardwareInformation(SNMP_Transport *snmp, NObject *node, DriverData *driverData, DeviceHardwareInfo *hwInfo) override;
   virtual bool getVirtualizationType(SNMP_Transport *snmp, NObject *node, DriverData *driverData, VirtualizationType *vtype) override;
   virtual int getClusterMode(SNMP_Transport *snmp, NObject *node, DriverData *driverData) override;
   virtual bool isWirelessController(SNMP_Transport *snmp, NObject *node, DriverData *driverData) override;
   virtual ObjectArray<AccessPointInfo> *getAccessPoints(SNMP_Transport *snmp, NObject *node, DriverData *driverData) override;
   virtual ObjectArray<WirelessStationInfo> *getWirelessStations(SNMP_Transport *snmp, NObject *node, DriverData *driverData) override;
   virtual AccessPointState getAccessPointState(SNMP_Transport *snmp, NObject *node, DriverData *driverData,
         uint32_t apIndex, const MacAddress& macAddr, const InetAddress& ipAddr, const StructArray<RadioInterfaceInfo>& radioInterfaces) override;
};

#endif
