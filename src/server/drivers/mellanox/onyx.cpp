/*
** NetXMS - Network Management System
** Driver for Mellanox / NVIDIA switches running Onyx (MLNX-OS)
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
** File: onyx.cpp
**
**/

#include "mellanox.h"
#include <netxms-regex.h>

/**
 * Get driver name
 */
const TCHAR *MellanoxOnyxDriver::getName()
{
   return _T("MELLANOX-ONYX");
}

/**
 * Get driver version
 */
const TCHAR *MellanoxOnyxDriver::getVersion()
{
   return NETXMS_VERSION_STRING;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int MellanoxOnyxDriver::isPotentialDevice(const SNMP_ObjectId& oid)
{
   return oid.startsWith({ 1, 3, 6, 1, 4, 1, 33049 }) ? 200 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param context device context
 * @param oid Device OID
 */
bool MellanoxOnyxDriver::isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid)
{
   return true;
}

/**
 * Get port layout of given module. Spectrum-based switches use two rows of ports with
 * odd-numbered ports on top and even-numbered ports directly below them.
 *
 * @param context device context
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @param module Module number (starting from 1)
 * @param layout Layout structure to fill
 */
void MellanoxOnyxDriver::getModuleLayout(SNMP_Transport *snmp, NObject *node, DriverData *driverData, int module, NDD_MODULE_LAYOUT *layout)
{
   layout->numberingScheme = NDD_PN_UD_LR;
   layout->rows = 2;
}

/**
 * Get list of interfaces for given node
 *
 * @param context device context
 * @param node Node
 * @param driverData driver data
 * @param useIfXTable if true, usage of ifXTable is allowed
 */
InterfaceList *MellanoxOnyxDriver::getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable)
{
   InterfaceList *ifList = NetworkDeviceDriver::getInterfaces(snmp, node, driverData, useIfXTable);
   if (ifList == nullptr)
      return nullptr;

   // Front panel ports are named Eth<module>/<port>, with split ports as Eth<module>/<port>/<lane>.
   // ifIndex values do not follow port numbers, so the location is always taken from the name.
   // Management (mgmt0), LAG (Po), MLAG (Mpo) and VLAN interfaces never match the pattern.
   const char *eptr;
   int eoffset;
   PCRE *re = _pcre_compile_t(reinterpret_cast<const PCRE_TCHAR*>(_T("^Eth([0-9]+)/([0-9]+)(?:/([0-9]+))?$")), PCRE_COMMON_FLAGS, &eptr, &eoffset, nullptr);
   if (re == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_MELLANOX, 5, _T("MellanoxOnyxDriver::getInterfaces: cannot compile port name regexp: %hs at offset %d"), eptr, eoffset);
      return ifList;
   }

   int pmatch[30];
   for(int i = 0; i < ifList->size(); i++)
   {
      InterfaceInfo *iface = ifList->get(i);
      if (iface->type != IFTYPE_ETHERNET_CSMACD)
         continue;

      int rc = _pcre_exec_t(re, nullptr, reinterpret_cast<PCRE_TCHAR*>(iface->name), static_cast<int>(_tcslen(iface->name)), 0, 0, pmatch, 30);
      if (rc < 3)
         continue;

      // InterfacePhysicalLocation has no field for a split port lane, so only the first lane
      // represents the physical port; remaining lanes are left without physical location.
      if ((rc == 4) && (IntegerFromCGroup(iface->name, pmatch, 3) > 1))
         continue;

      iface->isPhysicalPort = true;
      iface->location.module = IntegerFromCGroup(iface->name, pmatch, 1);
      iface->location.port = IntegerFromCGroup(iface->name, pmatch, 2);
   }

   _pcre_free_t(re);
   return ifList;
}
