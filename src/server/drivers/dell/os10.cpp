/* 
** NetXMS - Network Management System
** Driver for Dell EMC Networking OS10 switches
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
** File: os10.cpp
**/

#include "dell.h"
#include <netxms-regex.h>

#define DEBUG_TAG_OS10 _T("ndd.dell.os10")

/**
 * Get driver name
 */
const TCHAR *DellOS10Driver::getName()
{
   return _T("DELL-OS10");
}

/**
 * Get driver version
 */
const TCHAR *DellOS10Driver::getVersion()
{
   return NETXMS_VERSION_STRING;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int DellOS10Driver::isPotentialDevice(const SNMP_ObjectId& oid)
{
   // dell(674).enterpriseSW(11000).networking(5000).os10(100).os10Products(2), product families (S, M, Z, N series) are below
   return oid.startsWith({ 1, 3, 6, 1, 4, 1, 674, 11000, 5000, 100, 2 }) ? 255 : 0;
}

/**
 * Get list of interfaces for given node
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver data
 * @param useIfXTable if true, usage of ifXTable is allowed
 */
InterfaceList *DellOS10Driver::getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable)
{
   InterfaceList *ifList = NetworkDeviceDriver::getInterfaces(snmp, node, driverData, useIfXTable);
   if (ifList == nullptr)
      return nullptr;

   // Physical ports are named "ethernet node/slot/port" (e.g. ethernet1/1/24). Breakout ports carry an additional
   // lane number after a colon (e.g. ethernet1/1/49:1), and InterfacePhysicalLocation has no field to hold it -
   // putting the lane into "pic" would invert the chassis/module/pic/port hierarchy. Such interfaces are
   // intentionally left without physical location until location model can represent a lane.
   const char *eptr;
   int eoffset;
   PCREHandle re(_pcre_compile_t(reinterpret_cast<const PCRE_TCHAR*>(_T("^ethernet\\s*([0-9]+)/([0-9]+)/([0-9]+)$")), PCRE_COMMON_FLAGS | PCRE_CASELESS, &eptr, &eoffset, nullptr));
   if (re == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_OS10, 5, _T("DellOS10Driver::getInterfaces: cannot compile port name regexp: %hs at offset %d"), eptr, eoffset);
      return ifList;
   }

   int pmatch[30];
   for(int i = 0; i < ifList->size(); i++)
   {
      InterfaceInfo *iface = ifList->get(i);
      if (iface->type != IFTYPE_ETHERNET_CSMACD)
         continue;

      // Name may come from ifName or ifDescr depending on ifXTable availability, so try both
      const TCHAR *text = iface->name;
      if (_pcre_exec_t(re.get(), nullptr, reinterpret_cast<const PCRE_TCHAR*>(text), static_cast<int>(_tcslen(text)), 0, 0, pmatch, 30) != 4)
      {
         text = iface->description;
         if (_pcre_exec_t(re.get(), nullptr, reinterpret_cast<const PCRE_TCHAR*>(text), static_cast<int>(_tcslen(text)), 0, 0, pmatch, 30) != 4)
            continue;
      }

      iface->isPhysicalPort = true;
      iface->location.chassis = IntegerFromCGroup(text, pmatch, 1);
      iface->location.module = IntegerFromCGroup(text, pmatch, 2);
      iface->location.port = IntegerFromCGroup(text, pmatch, 3);
   }

   return ifList;
}

/**
 * Get SSH driver hints for interactive CLI sessions
 */
void DellOS10Driver::getSSHDriverHints(SSHDriverHints *hints) const
{
   // OS10 prompt patterns:
   // - EXEC mode: hostname#  (all roles land in EXEC mode, there is no user mode)
   // - Config mode: hostname(config)#, hostname(conf-if-eth1/1/1)#, etc.
   hints->promptPattern = "^[\\w.-]+(\\([\\w/:.-]+\\))?[>#]\\s*$";
   hints->enabledPromptPattern = "^[\\w.-]+(\\([\\w/:.-]+\\))?#\\s*$";

   // Access level is defined by user role, there is no privilege escalation command
   hints->enableCommand = nullptr;
   hints->enablePromptPattern = nullptr;

   // Pagination control
   hints->paginationDisableCmd = "terminal length 0";
   hints->paginationPrompt = "--[Mm]ore--";
   hints->paginationContinue = " ";

   // Exit command
   hints->exitCommand = "exit";

   // Test command for verifying command mode support
   hints->testCommand = "show version";
   hints->testCommandPattern = "OS10";

   // Timeouts
   hints->commandTimeout = 30000;
   hints->connectTimeout = 15000;
}

/**
 * Check if config backup is supported
 */
bool DellOS10Driver::isConfigBackupSupported()
{
   return true;
}

/**
 * Get running configuration via interactive SSH
 */
bool DellOS10Driver::getRunningConfig(DeviceBackupContext *ctx, ByteStream *output)
{
   SSHInteractiveChannel *ssh = ctx->getInteractiveSSH();
   if (ssh == nullptr)
      return false;
   return ssh->executeCommand("show running-configuration", output);
}

/**
 * Get startup configuration via interactive SSH
 */
bool DellOS10Driver::getStartupConfig(DeviceBackupContext *ctx, ByteStream *output)
{
   SSHInteractiveChannel *ssh = ctx->getInteractiveSSH();
   if (ssh == nullptr)
      return false;
   return ssh->executeCommand("show startup-configuration", output);
}
