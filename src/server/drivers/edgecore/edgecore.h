/* 
** NetXMS - Network Management System
** Drivers for Edgecore devices
** Copyright (C) 2024 Raden Solutions
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
** File: edgecore.h
**
**/

#ifndef _edgecore_h_
#define _edgecore_h_

#include <nddrv.h>
#include <netxms-version.h>

#define DEBUG_TAG_EDGECORE _T("ndd.edgecore")

/**
 * Driver data for EdgecoreEnterpriseSwitchDriver
 */
class ESWDriverData : public DriverData
{
private:
   uint32_t m_subtree;  // MIB subtree selector

public:
   ESWDriverData(uint32_t subtree) : DriverData()
   {
      m_subtree = subtree;
   }
   virtual ~ESWDriverData() = default;

   uint32_t getSubtree() const { return m_subtree; }
};

/**
 * Driver for Edgecore enterprise switches
 */
class EdgecoreEnterpriseSwitchDriver : public NetworkDeviceDriver
{
public:
   virtual const TCHAR *getName() override;
   virtual const TCHAR *getVersion() override;

   virtual int isPotentialDevice(const SNMP_ObjectId& oid) override;
   virtual bool isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid) override;
   virtual void analyzeDevice(SNMP_Transport *snmp, const SNMP_ObjectId& oid, NObject *node, DriverData **driverData) override;
   virtual bool getHardwareInformation(SNMP_Transport *snmp, NObject *node, DriverData *driverData, DeviceHardwareInfo *hwInfo) override;
   virtual InterfaceList *getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable) override;
};

#endif
