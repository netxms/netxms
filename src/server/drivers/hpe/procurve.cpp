/* 
** NetXMS - Network Management System
** Driver for HP ProCurve switches
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
** File: procurve.cpp
**/

#include "hpe.h"

/**
 * Get driver name
 */
const TCHAR *ProCurveDriver::getName()
{
	return _T("PROCURVE");
}

/**
 * Get driver version
 */
const TCHAR *ProCurveDriver::getVersion()
{
	return NETXMS_BUILD_TAG;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int ProCurveDriver::isPotentialDevice(const SNMP_ObjectId& oid)
{
	return oid.startsWith({ 1, 3, 6, 1, 4, 1, 11, 2, 3, 7, 11 }) ? 127 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool ProCurveDriver::isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid)
{
	return true;
}

/**
 * Do additional checks on the device required by driver.
 * Driver can set device's custom attributes from within
 * this function.
 *
 * @param snmp SNMP transport
 * @param node Node
 */
void ProCurveDriver::analyzeDevice(SNMP_Transport *snmp, const SNMP_ObjectId& oid, NObject *node, DriverData **driverData)
{
	uint32_t model = oid.getElement(11);

	// modular switches
	if ((model == 7) || (model == 9) || (model == 13) || (model == 14) || (model == 23) || 
	    (model == 27) || (model == 46) || (model == 47) || (model == 50) || (model == 51))
	{
		node->setCustomAttribute(_T(".procurve.isModular"), _T("1"), StateChange::IGNORE);
		node->setCustomAttribute(_T(".procurve.slotSize"), _T("24"), StateChange::IGNORE);
	}
}

/**
 * Get list of interfaces for given node
 *
 * @param snmp SNMP transport
 * @param node Node
 */
InterfaceList *ProCurveDriver::getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable)
{
	// Get interface list from standard MIB
	InterfaceList *ifList = NetworkDeviceDriver::getInterfaces(snmp, node, driverData, useIfXTable);
	if (ifList == nullptr)
		return nullptr;

	bool isModular = node->getCustomAttributeAsBoolean(_T(".procurve.isModular"), false);
	uint32_t slotSize = node->getCustomAttributeAsUInt32(_T(".procurve.slotSize"), 24);

	// Find physical ports
	for(int i = 0; i < ifList->size(); i++)
	{
		InterfaceInfo *iface = ifList->get(i);
		if (iface->type == IFTYPE_ETHERNET_CSMACD)
		{
			iface->isPhysicalPort = true;
			if (isModular)
			{
			   int p = iface->index % slotSize;
			   if (p != 0)
			   {
               iface->location.module = (iface->index / slotSize) + 1;
               iface->location.port = p;
			   }
			   else
			   {
			      // Last port in slot
               iface->location.module = iface->index / slotSize;
               iface->location.port = slotSize;
			   }
			}
			else
			{
				iface->location.module = 1;
				iface->location.port = iface->index;
			}
		}
	}

	return ifList;
}

/**
 * Get SSH driver hints for interactive CLI sessions
 */
void ProCurveDriver::getSSHDriverHints(SSHDriverHints *hints) const
{
   // ProVision / ArubaOS-S prompt patterns:
   // - Operator mode: hostname>
   // - Manager mode: hostname#
   // - Config mode: hostname(config)#, hostname(vlan-10)#, etc.
   hints->promptPattern = "^[\\w.-]+(\\([\\w-]+\\))?[>#]\\s*$";
   hints->enabledPromptPattern = "^[\\w.-]+(\\([\\w-]+\\))?#\\s*$";

   // Enable command for switching from operator to manager mode
   hints->enableCommand = "enable";
   hints->enablePromptPattern = "[Pp]assword:\\s*$";

   // Pagination control
   hints->paginationDisableCmd = "no page";
   hints->paginationPrompt = "-- MORE --|--More--";
   hints->paginationContinue = " ";

   // Exit command
   hints->exitCommand = "exit";

   // Test command for verifying command mode support
   hints->testCommand = "show system";
   hints->testCommandPattern = "System";

   // Timeouts
   hints->commandTimeout = 30000;
   hints->connectTimeout = 15000;
}

/**
 * Check if config backup is supported
 */
bool ProCurveDriver::isConfigBackupSupported()
{
   return true;
}

/**
 * Strip diagnostic preamble from ProCurve configuration output.
 * Removes lines like "Running configuration:" that appear before the actual config
 * which always starts with a comment line (;)
 */
static void StripProCurveConfigPreamble(ByteStream *output)
{
   const char *data = reinterpret_cast<const char*>(output->buffer());
   size_t len = output->size();

   // Find first line starting with ';' which marks the beginning of actual configuration
   size_t pos = 0;
   while (pos < len)
   {
      if (data[pos] == ';')
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
 * Get running configuration via interactive SSH
 */
bool ProCurveDriver::getRunningConfig(DeviceBackupContext *ctx, ByteStream *output)
{
   SSHInteractiveChannel *ssh = ctx->getInteractiveSSH();
   if (ssh == nullptr)
      return false;
   if (!ssh->executeCommand("show running-config", output))
      return false;
   StripProCurveConfigPreamble(output);
   return true;
}

/**
 * Get startup configuration via interactive SSH
 */
bool ProCurveDriver::getStartupConfig(DeviceBackupContext *ctx, ByteStream *output)
{
   SSHInteractiveChannel *ssh = ctx->getInteractiveSSH();
   if (ssh == nullptr)
      return false;
   if (!ssh->executeCommand("show config", output))
      return false;
   StripProCurveConfigPreamble(output);
   return true;
}
