/*
** NetXMS - Network Management System
** Driver for other Cisco devices
** Copyright (C) 2003-2024 Victor Kirhenshtein
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
** File: generic.cpp
**/

#include "cisco.h"
#include <netxms-regex.h>

/**
 * Get driver name
 */
const TCHAR *GenericCiscoDriver::getName()
{
   return _T("CISCO-GENERIC");
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int GenericCiscoDriver::isPotentialDevice(const SNMP_ObjectId& oid)
{
   return oid.startsWith({ 1, 3, 6, 1, 4, 1, 9, 1 }) ? 127 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool GenericCiscoDriver::isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid)
{
   return SnmpWalkCount(snmp, _T(".1.3.6.1.4.1.9.9.46.1.3.1.1.4")) > 0;
}

/**
 * Get list of interfaces for given node
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver data
 * @param useAliases policy for interface alias usage
 * @param useIfXTable if true, usage of ifXTable is allowed
 */
InterfaceList *GenericCiscoDriver::getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable)
{
   // Get interface list from standard MIB
   InterfaceList *ifList = CiscoDeviceDriver::getInterfaces(snmp, node, driverData, useIfXTable);
   if (ifList == nullptr)
      return nullptr;

   const char *eptr;
   int eoffset;
   PCRE *reBase = _pcre_compile_t(
         reinterpret_cast<const PCRE_TCHAR*>(_T("^(Se|Serial|Et|Ethernet|Fa|FastEthernet|Gi|GigabitEthernet|Te|TenGigabitEthernet|Fo|FortyGigabitEthernet)([0-9]+)/([0-9]+)$")),
         PCRE_COMMON_FLAGS | PCRE_CASELESS, &eptr, &eoffset, nullptr);
   if (reBase == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_CISCO, 5, _T("GenericCiscoDriver::getInterfaces: cannot compile base regexp: %hs at offset %d"), eptr, eoffset);
      return ifList;
   }
   PCRE *reFex = _pcre_compile_t(
         reinterpret_cast<const PCRE_TCHAR*>(_T("^(Se|Serial|Et|Ethernet|Fa|FastEthernet|Gi|GigabitEthernet|Te|TenGigabitEthernet|Fo|FortyGigabitEthernet)([0-9]+)/([0-9]+)/([0-9]+)$")),
         PCRE_COMMON_FLAGS | PCRE_CASELESS, &eptr, &eoffset, nullptr);
   if (reFex == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_CISCO, 5, _T("GenericCiscoDriver::getInterfaces: cannot compile FEX regexp: %hs at offset %d"), eptr, eoffset);
      _pcre_free_t(reBase);
      return ifList;
   }

   int pmatch[30];
   for(int i = 0; i < ifList->size(); i++)
   {
      InterfaceInfo *iface = ifList->get(i);
      if (_pcre_exec_t(reBase, nullptr, reinterpret_cast<PCRE_TCHAR*>(iface->name), static_cast<int>(_tcslen(iface->name)), 0, 0, pmatch, 30) == 4)
      {
         iface->isPhysicalPort = true;
         iface->location.chassis = 1;
         iface->location.module = IntegerFromCGroup(iface->name, pmatch, 2);
         iface->location.port = IntegerFromCGroup(iface->name, pmatch, 3);
      }
      else if (_pcre_exec_t(reFex, nullptr, reinterpret_cast<PCRE_TCHAR*>(iface->name), static_cast<int>(_tcslen(iface->name)), 0, 0, pmatch, 30) == 5)
      {
         iface->isPhysicalPort = true;
         iface->location.chassis = IntegerFromCGroup(iface->name, pmatch, 2);
         iface->location.module = IntegerFromCGroup(iface->name, pmatch, 3);
         iface->location.port = IntegerFromCGroup(iface->name, pmatch, 4);
      }
   }

   _pcre_free_t(reBase);
   _pcre_free_t(reFex);

   return ifList;
}
