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
 * @param context device context
 * @param oid Device OID
 */
bool ProCurveDriver::isDeviceSupported(DeviceContext *context, const SNMP_ObjectId& oid)
{
	return true;
}

/**
 * Do additional checks on the device required by driver.
 * Driver can set device's custom attributes from within
 * this function.
 *
 * @param context device context
 * @param node Node
 */
void ProCurveDriver::analyzeDevice(DeviceContext *context, const SNMP_ObjectId& oid, NObject *node, DriverData **driverData)
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
 * Copy word that follows given marker in the text (up to whitespace or comma). Returns false if marker is not found.
 */
static bool ExtractWord(const wchar_t *text, const wchar_t *marker, wchar_t *word, size_t size)
{
   const wchar_t *start = wcsstr(text, marker);
   if (start == nullptr)
      return false;
   start += wcslen(marker);
   size_t length = wcscspn(start, L" ,\t\r\n");
   if ((length == 0) || (length >= size))
      return false;
   memcpy(word, start, length * sizeof(wchar_t));
   word[length] = 0;
   return true;
}

/**
 * Get firmware information from device. Software and boot ROM versions are taken from sysDescr
 * ("HP J9729A 2920-48G-POE+ Switch, revision WB.16.10.0015, ROM WB.16.03 (/ws/swbuildm/...)").
 * Generic information from ENTITY MIB is used if software version cannot be found there.
 *
 * @param context device context
 * @param node Node
 * @param driverData driver data
 * @param firmware list of firmware to fill
 * @return true if firmware information was read from device
 */
bool ProCurveDriver::getFirmwareInformation(DeviceContext *context, NObject *node, DriverData *driverData, StructArray<FirmwareInfo> *firmware)
{
   SNMP_Transport *snmp = context->getSNMPTransport();
   if (snmp == nullptr)
      return false;

   wchar_t sysDescr[1024];
   if (SnmpGetEx(snmp, { 1, 3, 6, 1, 2, 1, 1, 1, 0 }, sysDescr, sizeof(sysDescr), SG_STRING_RESULT) != SNMP_ERR_SUCCESS)
      return false;

   wchar_t raw[64], version[64];
   if (!ExtractWord(sysDescr, L"revision ", raw, 64) || !NormalizeHPEImageVersion(raw, version, 64))
      return NetworkDeviceDriver::getFirmwareInformation(context, node, driverData, firmware);

   AddFirmwareInfo(firmware, FirmwareRole::MAIN, L"ArubaOS-Switch", L"aos-s", version, raw);
   if (ExtractWord(sysDescr, L"ROM ", raw, 64))
      AddFirmwareInfo(firmware, FirmwareRole::BOOTLOADER, L"Boot ROM", L"", L"", raw);
   return true;
}

/**
 * Get list of interfaces for given node
 *
 * @param context device context
 * @param node Node
 */
InterfaceList *ProCurveDriver::getInterfaces(DeviceContext *context, NObject *node, DriverData *driverData, bool useIfXTable)
{
	// Get interface list from standard MIB
	InterfaceList *ifList = NetworkDeviceDriver::getInterfaces(context, node, driverData, useIfXTable);
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
			if (isModular)
			{
			   uint32_t p = iface->index % slotSize;
			   if (p != 0)
               iface->setPhysicalLocation(0, (iface->index / slotSize) + 1, 0, p);
			   else
               iface->setPhysicalLocation(0, iface->index / slotSize, 0, slotSize);   // Last port in slot
			}
			else
			{
				iface->setPhysicalLocation(0, 1, 0, iface->index);
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

   // Error response detection (used by configuration restore)
   hints->errorPattern = "^(Invalid input|Incomplete input|Ambiguous input|Unable to|Error)";

   // Timeouts
   hints->commandTimeout = 30000;
   hints->connectTimeout = 15000;
}

/**
 * Check if config backup is supported
 */
bool ProCurveDriver::isConfigBackupSupported(DeviceContext *ctx)
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
bool ProCurveDriver::getRunningConfig(DeviceContext *ctx, ByteStream *output)
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
bool ProCurveDriver::getStartupConfig(DeviceContext *ctx, ByteStream *output)
{
   SSHInteractiveChannel *ssh = ctx->getInteractiveSSH();
   if (ssh == nullptr)
      return false;
   if (!ssh->executeCommand("show config", output))
      return false;
   StripProCurveConfigPreamble(output);
   return true;
}

/**
 * Check if config restore is supported
 */
bool ProCurveDriver::isConfigRestoreSupported()
{
   return true;
}

/**
 * Restore configuration via interactive SSH (merge semantics)
 */
bool ProCurveDriver::restoreConfig(DeviceContext *ctx, const ByteStream& config, StringBuffer *errorLog,
      const std::function<void (int, int)>& progressCallback)
{
   SSHInteractiveChannel *ssh = ctx->getInteractiveSSH();
   if (ssh == nullptr)
   {
      errorLog->append(L"Cannot open interactive SSH channel");
      return false;
   }

   StringList commands;
   PrepareConfigCommands(config, ';', &commands);
   return ssh->applyConfiguration(commands, "configure", "end", "write memory", errorLog, progressCallback);
}
