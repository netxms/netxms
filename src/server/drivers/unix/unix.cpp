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
** File: unix.cpp
**
**/

#include "unix.h"
#include <netxms-version.h>

/**
 * Get driver version
 */
const wchar_t *UnixDriver::getVersion()
{
   return NETXMS_VERSION_STRING;
}

/**
 * Get transports that this driver can use
 */
uint32_t UnixDriver::getSupportedTransports() const
{
   return NDD_TRANSPORT_SSH_COMMAND;
}

/**
 * Get transports that must be available for regular driver methods to be called
 */
uint32_t UnixDriver::getRequiredTransports() const
{
   return NDD_TRANSPORT_SSH_COMMAND;
}

/**
 * Execute command via SSH command channel. On success output contains command's standard output
 * terminated by zero byte, so it can be used as C string.
 */
bool UnixDriver::executeCommand(DeviceContext *context, const char *command, ByteStream *output)
{
   if (!context->executeSSHCommand(command, output))
   {
      nxlog_debug_tag(DEBUG_TAG_UNIX, 6, L"UnixDriver::executeCommand: execution of command \"%hs\" failed", command);
      return false;
   }
   output->write('\0');
   return true;
}

/**
 * Check if given device can be potentially supported by driver. Systems with SNMP are handled
 * by SNMP drivers (NET-SNMP or vendor specific), so these drivers are only considered for
 * devices without SNMP, where they get baseline priority.
 */
int UnixDriver::isPotentialDevice(const SNMP_ObjectId& oid)
{
   return 0;
}

/**
 * Check if given device is supported by driver - device should report expected kernel name
 */
bool UnixDriver::isDeviceSupported(DeviceContext *context, const SNMP_ObjectId& oid)
{
   ByteStream output;
   if (!executeCommand(context, "uname -s", &output))
      return false;

   // Compare first line of output with expected kernel name
   char kernelName[64];
   strlcpy(kernelName, reinterpret_cast<const char*>(output.buffer()), sizeof(kernelName));
   char *eol = strpbrk(kernelName, "\r\n");
   if (eol != nullptr)
      *eol = 0;
   TrimA(kernelName);

   bool supported = !strcmp(kernelName, getKernelName());
   nxlog_debug_tag(DEBUG_TAG_UNIX, 6, L"%s::isDeviceSupported: kernel name \"%hs\", device %s", getName(), kernelName, supported ? L"supported" : L"not supported");
   return supported;
}

/**
 * Driver entry point (Linux is first because it is most common and drivers with same priority are probed in registration order)
 */
NDD_BEGIN_DRIVER_LIST
NDD_DRIVER(LinuxDriver)
NDD_DRIVER(FreeBSDDriver)
NDD_DRIVER(NetBSDDriver)
NDD_DRIVER(OpenBSDDriver)
NDD_END_DRIVER_LIST
DECLARE_NDD_MODULE_ENTRY_POINT

/**
 * DLL entry point
 */
#ifdef _WIN32

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
      DisableThreadLibraryCalls(hInstance);
   return TRUE;
}

#endif
