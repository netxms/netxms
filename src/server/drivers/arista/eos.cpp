/*
** NetXMS - Network Management System
** Driver for Arista switches running EOS
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
** File: eos.cpp
**
**/

#include "arista.h"
#include <netxms-regex.h>

/**
 * Get driver name
 */
const TCHAR *AristaEOSDriver::getName()
{
   return _T("ARISTA-EOS");
}

/**
 * Get driver version
 */
const TCHAR *AristaEOSDriver::getVersion()
{
   return NETXMS_VERSION_STRING;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int AristaEOSDriver::isPotentialDevice(const SNMP_ObjectId& oid)
{
   return oid.startsWith({ 1, 3, 6, 1, 4, 1, 30065, 1 }) ? 200 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param context device context
 * @param oid Device OID
 */
bool AristaEOSDriver::isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid)
{
   return true;
}

/**
 * Get port layout of given module. Arista front panels use two rows of ports with
 * odd-numbered ports on top and even-numbered ports directly below them.
 *
 * @param context device context
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @param module Module number (starting from 1)
 * @param layout Layout structure to fill
 */
void AristaEOSDriver::getModuleLayout(SNMP_Transport *snmp, NObject *node, DriverData *driverData, int module, NDD_MODULE_LAYOUT *layout)
{
   layout->numberingScheme = NDD_PN_UD_LR;
   layout->rows = 2;
}

/**
 * Maximum number of breakout lanes per front panel port. A second name component above
 * this value cannot be a lane number and therefore indicates a modular chassis.
 */
#define MAX_LANES_PER_PORT 8

/**
 * Get list of interfaces for given node
 *
 * @param context device context
 * @param node Node
 * @param driverData driver data
 * @param useIfXTable if true, usage of ifXTable is allowed
 */
InterfaceList *AristaEOSDriver::getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable)
{
   InterfaceList *ifList = NetworkDeviceDriver::getInterfaces(snmp, node, driverData, useIfXTable);
   if (ifList == nullptr)
      return nullptr;

   // Front panel ports are named Ethernet<port> on fixed switches, with breakout lanes as
   // Ethernet<port>/<lane>. Modular chassis use Ethernet<slot>/<port>, with breakout lanes as
   // Ethernet<slot>/<port>/<lane>. Two-component names are therefore ambiguous, and chassis type
   // is inferred from the whole interface list: a three-component name or a second component
   // above the maximum lane count can only come from a modular chassis.
   // Management, Port-Channel, Vlan, Loopback and similar interfaces never match the pattern.
   const char *eptr;
   int eoffset;
   PCRE *re = _pcre_compile_t(reinterpret_cast<const PCRE_TCHAR*>(_T("^Ethernet([0-9]+)(?:/([0-9]+))?(?:/([0-9]+))?$")), PCRE_COMMON_FLAGS, &eptr, &eoffset, nullptr);
   if (re == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_ARISTA, 5, _T("AristaEOSDriver::getInterfaces: cannot compile port name regexp: %hs at offset %d"), eptr, eoffset);
      return ifList;
   }

   int pmatch[30];
   bool modular = false;
   for(int i = 0; (i < ifList->size()) && !modular; i++)
   {
      InterfaceInfo *iface = ifList->get(i);
      if (iface->type != IFTYPE_ETHERNET_CSMACD)
         continue;

      int rc = _pcre_exec_t(re, nullptr, reinterpret_cast<PCRE_TCHAR*>(iface->name), static_cast<int>(_tcslen(iface->name)), 0, 0, pmatch, 30);
      if ((rc == 4) || ((rc == 3) && (IntegerFromCGroup(iface->name, pmatch, 2) > MAX_LANES_PER_PORT)))
         modular = true;
   }

   for(int i = 0; i < ifList->size(); i++)
   {
      InterfaceInfo *iface = ifList->get(i);
      if (iface->type != IFTYPE_ETHERNET_CSMACD)
         continue;

      int rc = _pcre_exec_t(re, nullptr, reinterpret_cast<PCRE_TCHAR*>(iface->name), static_cast<int>(_tcslen(iface->name)), 0, 0, pmatch, 30);
      if (rc < 2)
         continue;

      uint32_t module, port, lane;
      if (modular)
      {
         if (rc < 3)
            continue;   // Ethernet<n> without slot number is not expected on modular chassis
         module = IntegerFromCGroup(iface->name, pmatch, 1);
         port = IntegerFromCGroup(iface->name, pmatch, 2);
         lane = (rc == 4) ? IntegerFromCGroup(iface->name, pmatch, 3) : 0;
      }
      else
      {
         module = 1;
         port = IntegerFromCGroup(iface->name, pmatch, 1);
         lane = (rc >= 3) ? IntegerFromCGroup(iface->name, pmatch, 2) : 0;
      }

      // InterfacePhysicalLocation has no field for a breakout lane, so only the first lane
      // represents the physical port; remaining lanes are left without physical location.
      if (lane > 1)
         continue;

      iface->isPhysicalPort = true;
      iface->location.module = module;
      iface->location.port = port;
   }

   _pcre_free_t(re);
   return ifList;
}

/**
 * Get SSH driver hints for interactive CLI sessions
 */
void AristaEOSDriver::getSSHDriverHints(SSHDriverHints *hints) const
{
   // EOS prompt patterns follow IOS conventions:
   // - User mode: hostname>
   // - Privileged mode: hostname#
   // - Config mode: hostname(config)#, hostname(config-if-Et1)#, etc.
   hints->promptPattern = "^[\\w.-]+(\\([\\w/.-]+\\))?[>#]\\s*$";
   hints->enabledPromptPattern = "^[\\w.-]+(\\([\\w/.-]+\\))?#\\s*$";

   // Enable command and password prompt
   hints->enableCommand = "enable";
   hints->enablePromptPattern = "[Pp]assword:\\s*$";

   // Pagination control
   hints->paginationDisableCmd = "terminal length 0";
   hints->paginationPrompt = " --[Mm]ore-- ";
   hints->paginationContinue = " ";

   // Exit command
   hints->exitCommand = "exit";

   // Test command for verifying command mode support
   hints->testCommand = "show version | include Arista";
   hints->testCommandPattern = "Arista";
}
