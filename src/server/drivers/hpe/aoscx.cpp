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
 * Get firmware information from device. AOS-CX version is the software image version in sysDescr
 * ("Aruba JL659A 6300M 48SR5 CL6 PoE 4SFP56 Swch FL.10.04.3031"), or software revision reported
 * by ENTITY MIB if it cannot be found there.
 *
 * @param context device context
 * @param node Node
 * @param driverData driver data
 * @param firmware list of firmware to fill
 * @return true if firmware information was read from device
 */
bool ArubaCXDriver::getFirmwareInformation(DeviceContext *context, NObject *node, DriverData *driverData, StructArray<FirmwareInfo> *firmware)
{
   SNMP_Transport *snmp = context->getSNMPTransport();
   if (snmp == nullptr)
      return false;

   wchar_t sysDescr[1024];
   if (SnmpGetEx(snmp, { 1, 3, 6, 1, 2, 1, 1, 1, 0 }, sysDescr, sizeof(sysDescr), SG_STRING_RESULT) != SNMP_ERR_SUCCESS)
      return false;

   wchar_t version[64];
   wchar_t *state;
   for(wchar_t *word = wcstok(sysDescr, L" ,\t\r\n", &state); word != nullptr; word = wcstok(nullptr, L" ,\t\r\n", &state))
   {
      if (NormalizeHPEImageVersion(word, version, 64))
      {
         AddFirmwareInfo(firmware, FirmwareRole::MAIN, L"AOS-CX", L"aoscx", version, word);
         return true;
      }
   }

   // Use generic information, with platform set if software revision is software image version
   StructArray<FirmwareInfo> generic;
   if (!NetworkDeviceDriver::getFirmwareInformation(context, node, driverData, &generic))
      return false;
   for(int i = 0; i < generic.size(); i++)
   {
      FirmwareInfo *fw = generic.get(i);
      if ((fw->role == FirmwareRole::MAIN) && NormalizeHPEImageVersion(fw->raw, version, 64))
      {
         wcscpy(fw->name, L"AOS-CX");
         wcscpy(fw->platform, L"aoscx");
         wcscpy(fw->version, version);
      }
      firmware->add(fw);
   }
   return true;
}

/**
 * Get list of interfaces for given node
 *
 * @param context device context
 * @param node Node
 * @param driverData driver data
 * @param useIfXTable if true, usage of ifXTable is allowed
 */
InterfaceList *ArubaCXDriver::getInterfaces(DeviceContext *context, NObject *node, DriverData *driverData, bool useIfXTable)
{
   InterfaceList *ifList = NetworkDeviceDriver::getInterfaces(context, node, driverData, useIfXTable);
   if (ifList == nullptr)
      return nullptr;

   // Physical ports are named member/slot/port (e.g. 1/1/24). Split (breakout) ports have an
   // additional lane number (e.g. 1/1/49:1).
   const char *eptr;
   int eoffset;
   PCRE *re = _pcre_compile_t(reinterpret_cast<const PCRE_TCHAR*>(_T("^([0-9]+)/([0-9]+)/([0-9]+)(?::([0-9]+))?$")), PCRE_COMMON_FLAGS, &eptr, &eoffset, nullptr);
   if (re == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_AOSCX, 5, _T("ArubaCXDriver::getInterfaces: cannot compile port name regexp: %hs at offset %d"), eptr, eoffset);
      return ifList;
   }

   int pmatch[30];
   for(int i = 0; i < ifList->size(); i++)
   {
      InterfaceInfo *iface = ifList->get(i);
      if (iface->type != IFTYPE_ETHERNET_CSMACD)
         continue;

      int rc = _pcre_exec_t(re, nullptr, reinterpret_cast<PCRE_TCHAR*>(iface->name), static_cast<int>(_tcslen(iface->name)), 0, 0, pmatch, 30);
      if (rc < 4)
         continue;

      uint32_t lane = (rc == 5) ? IntegerFromCGroup(iface->name, pmatch, 4) : 0;
      if (lane > 127)
         continue;
      iface->setPhysicalLocation(IntegerFromCGroup(iface->name, pmatch, 1), IntegerFromCGroup(iface->name, pmatch, 2), 0, IntegerFromCGroup(iface->name, pmatch, 3),
         (rc == 5) ? static_cast<int8_t>(lane) : INTERFACE_LANE_NONE);
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

   // Error response detection (used by configuration restore)
   hints->errorPattern = "^(Invalid input|Incomplete input|Ambiguous input|Unknown command|Command failed|Error)";

   // Timeouts
   hints->commandTimeout = 30000;
   hints->connectTimeout = 15000;
}

/**
 * Check if config backup is supported
 */
bool ArubaCXDriver::isConfigBackupSupported(DeviceContext *ctx)
{
   return true;
}

/**
 * Get running configuration via interactive SSH
 */
bool ArubaCXDriver::getRunningConfig(DeviceContext *ctx, ByteStream *output)
{
   SSHInteractiveChannel *ssh = ctx->getInteractiveSSH();
   if (ssh == nullptr)
      return false;
   return ssh->executeCommand("show running-config", output);
}

/**
 * Get startup configuration via interactive SSH
 */
bool ArubaCXDriver::getStartupConfig(DeviceContext *ctx, ByteStream *output)
{
   SSHInteractiveChannel *ssh = ctx->getInteractiveSSH();
   if (ssh == nullptr)
      return false;
   return ssh->executeCommand("show startup-config", output);
}

/**
 * Check if config restore is supported
 */
bool ArubaCXDriver::isConfigRestoreSupported()
{
   return true;
}

/**
 * Restore configuration via interactive SSH (merge semantics)
 */
bool ArubaCXDriver::restoreConfig(DeviceContext *ctx, const ByteStream& config, StringBuffer *errorLog,
      const std::function<void (int, int)>& progressCallback)
{
   SSHInteractiveChannel *ssh = ctx->getInteractiveSSH();
   if (ssh == nullptr)
   {
      errorLog->append(L"Cannot open interactive SSH channel");
      return false;
   }

   StringList commands;
   PrepareConfigCommands(config, '!', &commands);
   return ssh->applyConfiguration(commands, "configure terminal", "end", "write memory", errorLog, progressCallback);
}
