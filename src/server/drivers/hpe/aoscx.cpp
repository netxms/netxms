/* 
** NetXMS - Network Management System
** Driver for HPE Aruba CX (AOS-CX) switches
** Copyright (C) 2003-2026 Raden Solutions
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
** File: aoscx.cpp
**/

#include "hpe.h"
#include <netxms-regex.h>

#define DEBUG_TAG_AOSCX _T("ndd.hpe.aoscx")

/**
 * Get driver name
 */
const TCHAR *ArubaCXDriver::getName()
{
   return _T("ARUBA-CX");
}

/**
 * Get driver version
 */
const TCHAR *ArubaCXDriver::getVersion()
{
   return NETXMS_VERSION_STRING;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int ArubaCXDriver::isPotentialDevice(const SNMP_ObjectId& oid)
{
   // hpeNetworking(4).wiredNetworkingDevices(1).arubaOS-CX(1), device identifiers are below wndDeviceIds(1)
   return oid.startsWith({ 1, 3, 6, 1, 4, 1, 47196, 4, 1, 1 }) ? 255 : 0;
}

/**
 * Get list of interfaces for given node
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver data
 * @param useIfXTable if true, usage of ifXTable is allowed
 */
InterfaceList *ArubaCXDriver::getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable)
{
   InterfaceList *ifList = NetworkDeviceDriver::getInterfaces(snmp, node, driverData, useIfXTable);
   if (ifList == nullptr)
      return nullptr;

   // Physical ports are named member/slot/port (e.g. 1/1/24). Split (breakout) ports have an
   // additional lane number (e.g. 1/1/49:1), and InterfacePhysicalLocation has no field to hold it -
   // putting the lane into "pic" would invert the chassis/module/pic/port hierarchy. Such interfaces
   // are intentionally left without physical location until location model can represent a lane.
   const char *eptr;
   int eoffset;
   PCRE *re = _pcre_compile_t(reinterpret_cast<const PCRE_TCHAR*>(_T("^([0-9]+)/([0-9]+)/([0-9]+)$")), PCRE_COMMON_FLAGS, &eptr, &eoffset, nullptr);
   if (re == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_AOSCX, 5, _T("ArubaCXDriver::getInterfaces: cannot compile port name regexp: %hs at offset %d"), eptr, eoffset);
      return ifList;
   }

   int pmatch[30];
   for(int i = 0; i < ifList->size(); i++)
   {
      InterfaceInfo *iface = ifList->get(i);
      if ((iface->type == IFTYPE_ETHERNET_CSMACD) &&
          (_pcre_exec_t(re, nullptr, reinterpret_cast<PCRE_TCHAR*>(iface->name), static_cast<int>(_tcslen(iface->name)), 0, 0, pmatch, 30) == 4))
      {
         iface->isPhysicalPort = true;
         iface->location.chassis = IntegerFromCGroup(iface->name, pmatch, 1);
         iface->location.module = IntegerFromCGroup(iface->name, pmatch, 2);
         iface->location.port = IntegerFromCGroup(iface->name, pmatch, 3);
      }
   }

   _pcre_free_t(re);
   return ifList;
}

/**
 * Get SSH driver hints for interactive CLI sessions
 */
void ArubaCXDriver::getSSHDriverHints(SSHDriverHints *hints) const
{
   // AOS-CX prompt patterns:
   // - Operator role: hostname>
   // - Administrator role: hostname#
   // - Config mode: hostname(config)#, hostname(config-if-1/1/1)#, etc.
   hints->promptPattern = "^[\\w.-]+(\\([\\w/.-]+\\))?[>#]\\s*$";
   hints->enabledPromptPattern = "^[\\w.-]+(\\([\\w/.-]+\\))?#\\s*$";

   // Access level is defined by user role, there is no privilege escalation command
   hints->enableCommand = nullptr;
   hints->enablePromptPattern = nullptr;

   // Pagination control
   hints->paginationDisableCmd = "no page";
   hints->paginationPrompt = "-- MORE --";
   hints->paginationContinue = " ";

   // Exit command
   hints->exitCommand = "exit";

   // Test command for verifying command mode support
   hints->testCommand = "show version";
   hints->testCommandPattern = "ArubaOS-CX";

   // Timeouts
   hints->commandTimeout = 30000;
   hints->connectTimeout = 15000;
}

/**
 * Check if config backup is supported
 */
bool ArubaCXDriver::isConfigBackupSupported()
{
   return true;
}

/**
 * Get running configuration via interactive SSH
 */
bool ArubaCXDriver::getRunningConfig(DeviceBackupContext *ctx, ByteStream *output)
{
   SSHInteractiveChannel *ssh = ctx->getInteractiveSSH();
   if (ssh == nullptr)
      return false;
   return ssh->executeCommand("show running-config", output);
}

/**
 * Get startup configuration via interactive SSH
 */
bool ArubaCXDriver::getStartupConfig(DeviceBackupContext *ctx, ByteStream *output)
{
   SSHInteractiveChannel *ssh = ctx->getInteractiveSSH();
   if (ssh == nullptr)
      return false;
   return ssh->executeCommand("show startup-config", output);
}
