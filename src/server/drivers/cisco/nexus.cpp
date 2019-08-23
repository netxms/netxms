/*
** NetXMS - Network Management System
** Driver for Cisco Nexus switches
** Copyright (C) 2003-2019 Victor Kirhenshtein
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
** File: nexus.cpp
**/

#include "cisco.h"
#include <entity_mib.h>
#include <netxms-regex.h>

/**
 * Get driver name
 */
const TCHAR *CiscoNexusDriver::getName()
{
   return _T("CISCO-NEXUS");
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int CiscoNexusDriver::isPotentialDevice(const TCHAR *oid)
{
   static int supportedDevices[] = { 1308, -1 };   //FIXME: populate list

   if (_tcsncmp(oid, _T(".1.3.6.1.4.1.9.12.3.1.3."), 24))
      return 0;

   int id = _tcstol(&oid[24], NULL, 10);
   for(int i = 0; supportedDevices[i] != -1; i++)
      if (supportedDevices[i] == id)
         return 200;
   return 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool CiscoNexusDriver::isDeviceSupported(SNMP_Transport *snmp, const TCHAR *oid)
{
   return true;
}

/**
 * Extract integer from capture group
 */
static UINT32 IntegerFromCGroup(const TCHAR *text, int *cgroups, int cgindex)
{
   TCHAR buffer[32];
   int len = cgroups[cgindex * 2 + 1] - cgroups[cgindex * 2];
   if (len > 31)
      len = 31;
   memcpy(buffer, &text[cgroups[cgindex * 2]], len * sizeof(TCHAR));
   buffer[len] = 0;
   return _tcstoul(buffer, NULL, 10);
}

/**
 * Get list of interfaces for given node
 *
 * @param snmp SNMP transport
 * @param attributes Node's custom attributes
 */
InterfaceList *CiscoNexusDriver::getInterfaces(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData, int useAliases, bool useIfXTable)
{
   // Get interface list from standard MIB
   InterfaceList *ifList = NetworkDeviceDriver::getInterfaces(snmp, attributes, driverData, useAliases, useIfXTable);
   if (ifList == NULL)
      return NULL;

   const char *eptr;
   int eoffset;
   PCRE *reBase = _pcre_compile_t(reinterpret_cast<const PCRE_TCHAR*>(_T("^Ethernet([0-9]+)/([0-9]+)$")), PCRE_COMMON_FLAGS | PCRE_CASELESS, &eptr, &eoffset, NULL);
   if (reBase == NULL)
   {
      nxlog_debug_tag(DEBUG_TAG_CISCO, 5, _T("CiscoNexusDriver::getInterfaces: cannot compile base regexp: %hs at offset %d"), eptr, eoffset);
      return ifList;
   }
   PCRE *reFex = _pcre_compile_t(reinterpret_cast<const PCRE_TCHAR*>(_T("^Ethernet([0-9]+)/([0-9]+)/([0-9]+)$")), PCRE_COMMON_FLAGS | PCRE_CASELESS, &eptr, &eoffset, NULL);
   if (reFex == NULL)
   {
      nxlog_debug_tag(DEBUG_TAG_CISCO, 5, _T("CiscoNexusDriver::getInterfaces: cannot compile FEX regexp: %hs at offset %d"), eptr, eoffset);
      _pcre_free_t(reBase);
      return ifList;
   }

   int pmatch[30];
   for(int i = 0; i < ifList->size(); i++)
   {
      InterfaceInfo *iface = ifList->get(i);
      if (_pcre_exec_t(reBase, NULL, reinterpret_cast<PCRE_TCHAR*>(iface->name), static_cast<int>(_tcslen(iface->name)), 0, 0, pmatch, 30) == 3)
      {
         iface->isPhysicalPort = true;
         iface->location.chassis = 1;
         iface->location.module = IntegerFromCGroup(iface->name, pmatch, 1);
         iface->location.port = IntegerFromCGroup(iface->name, pmatch, 2);
      }
      else if (_pcre_exec_t(reFex, NULL, reinterpret_cast<PCRE_TCHAR*>(iface->name), static_cast<int>(_tcslen(iface->name)), 0, 0, pmatch, 30) == 4)
      {
         iface->isPhysicalPort = true;
         iface->location.chassis = IntegerFromCGroup(iface->name, pmatch, 1);
         iface->location.module = IntegerFromCGroup(iface->name, pmatch, 2);
         iface->location.port = IntegerFromCGroup(iface->name, pmatch, 3);
      }
   }

   _pcre_free_t(reBase);
   _pcre_free_t(reFex);
   return ifList;
}
