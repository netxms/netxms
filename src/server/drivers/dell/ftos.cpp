/* 
** NetXMS - Network Management System
** Driver for Dell Networking OS9 (Force10 / FTOS) switches
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
** File: ftos.cpp
**/

#include "dell.h"
#include <netxms-regex.h>

#define DEBUG_TAG_FTOS _T("ndd.dell.ftos")

/**
 * Get driver name
 */
const TCHAR *DellFTOSDriver::getName()
{
   return _T("DELL-FTOS");
}

/**
 * Get driver version
 */
const TCHAR *DellFTOSDriver::getVersion()
{
   return NETXMS_VERSION_STRING;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int DellFTOSDriver::isPotentialDevice(const SNMP_ObjectId& oid)
{
   // dellNet(6027).dellNetProducts(1), product families (E, C, S, M, Z series) are below
   return oid.startsWith({ 1, 3, 6, 1, 4, 1, 6027, 1 }) ? 255 : 0;
}

/**
 * Model names for DellNetChassisType enumeration (DELL-NETWORKING-TC), indexed by enumeration value
 */
static const TCHAR *s_modelNames[] =
{
   nullptr,
   _T("E1200"), _T("E600"), _T("E300"), _T("E150"), _T("E610"), _T("C150"), _T("C300"), _T("E1200i"), _T("S2410CP"), _T("S2410P"),
   _T("S50"), _T("S50E"), _T("S50V"), _T("S50NAC"), _T("S50NDC"), _T("S25PDC"), _T("S25PAC"), _T("S25V"), _T("S25N"), _T("S60"),
   _T("S55"), _T("S4810"), _T("S6410"), _T("Z9000"), _T("MXL"), _T("PowerEdge M I/O Aggregator"), _T("S4820"), _T("S6000"), _T("S5000"), _T("PowerEdge FN410S"),
   _T("PowerEdge FN410T"), _T("PowerEdge FN2210S"), _T("Z9500"), _T("C9010"), _T("C1048P"), _T("S4048-ON"), _T("S4810-ON"), _T("S6000-ON"), _T("S3048-ON"), _T("Z9100-ON"),
   _T("S6100-ON"), _T("S3148P"), _T("S3124P"), _T("S3124F"), _T("S3124"), _T("S3148"), _T("S4048T-ON"), _T("S6010-ON")
};

/**
 * Find management unit in stack unit table. Returns index of management unit, or index of first unit if
 * management unit is not marked, or 0 if table is empty.
 *
 * @param snmp SNMP transport
 * @param mgmtStatusColumn OID of management status column of stack unit table
 */
static uint32_t FindManagementUnit(SNMP_Transport *snmp, const TCHAR *mgmtStatusColumn)
{
   uint32_t unitIndex = 0;
   SnmpWalk(snmp, mgmtStatusColumn,
      [&unitIndex] (SNMP_Variable *v) -> uint32_t
      {
         uint32_t index = v->getName().getLastElement();
         if (v->getValueAsInt() == 1)   // mgmtUnit(1)
         {
            unitIndex = index;
            return SNMP_ERR_ABORTED;
         }
         if (unitIndex == 0)
            unitIndex = index;
         return SNMP_ERR_SUCCESS;
      });
   return unitIndex;
}

/**
 * Get hardware information from device. Devices running Dell Networking OS9 do not implement ENTITY-MIB, so
 * information is taken from stack unit table of DELL-NETWORKING-CHASSIS-MIB, with fallback to the legacy
 * F10-S-SERIES-CHASSIS-MIB for devices running older FTOS versions.
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver data
 * @param hwInfo pointer to hardware information structure to fill
 * @return true if hardware information is available
 */
bool DellFTOSDriver::getHardwareInformation(SNMP_Transport *snmp, NObject *node, DriverData *driverData, DeviceHardwareInfo *hwInfo)
{
   _tcscpy(hwInfo->vendor, _T("Dell"));

   // DELL-NETWORKING-CHASSIS-MIB::dellNetStackUnitTable
   uint32_t unitIndex = FindManagementUnit(snmp, _T(".1.3.6.1.4.1.6027.3.26.1.3.4.1.4"));
   if (unitIndex != 0)
   {
      SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
      TCHAR oid[128];
      _sntprintf(oid, 128, _T(".1.3.6.1.4.1.6027.3.26.1.3.4.1.7.%u"), unitIndex);   // dellNetStackUnitModelId
      request.bindVariable(new SNMP_Variable(oid));
      _sntprintf(oid, 128, _T(".1.3.6.1.4.1.6027.3.26.1.3.4.1.9.%u"), unitIndex);   // dellNetStackUnitDescription
      request.bindVariable(new SNMP_Variable(oid));
      _sntprintf(oid, 128, _T(".1.3.6.1.4.1.6027.3.26.1.3.4.1.10.%u"), unitIndex);  // dellNetStackUnitCodeVersion
      request.bindVariable(new SNMP_Variable(oid));
      _sntprintf(oid, 128, _T(".1.3.6.1.4.1.6027.3.26.1.3.4.1.11.%u"), unitIndex);  // dellNetStackUnitSerialNumber
      request.bindVariable(new SNMP_Variable(oid));
      _sntprintf(oid, 128, _T(".1.3.6.1.4.1.6027.3.26.1.3.4.1.17.%u"), unitIndex);  // dellNetStackUnitPartNum
      request.bindVariable(new SNMP_Variable(oid));
      _sntprintf(oid, 128, _T(".1.3.6.1.4.1.6027.3.26.1.3.4.1.23.%u"), unitIndex);  // dellNetStackUnitServiceTag
      request.bindVariable(new SNMP_Variable(oid));

      SNMP_PDU *response;
      if (snmp->doRequest(&request, &response) != SNMP_ERR_SUCCESS)
      {
         nxlog_debug_tag(DEBUG_TAG_FTOS, 5, _T("DellFTOSDriver::getHardwareInformation(%s): request to dellNetStackUnitTable failed"), node->getName());
         return true;   // Vendor is known, the rest can be filled from ENTITY-MIB by the caller if available
      }

      TCHAR buffer[256];
      const SNMP_Variable *v = response->getVariable(0);
      if ((v != nullptr) && (v->getType() == ASN_INTEGER))
      {
         uint32_t model = v->getValueAsUInt();
         if ((model > 0) && (model < sizeof(s_modelNames) / sizeof(s_modelNames[0])))
            _tcslcpy(hwInfo->productName, s_modelNames[model], 128);
      }

      v = response->getVariable(1);
      if ((hwInfo->productName[0] == 0) && (v != nullptr) && (v->getType() == ASN_OCTET_STRING))
         _tcslcpy(hwInfo->productName, v->getValueAsString(buffer, 256), 128);

      v = response->getVariable(2);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
         _tcslcpy(hwInfo->productVersion, v->getValueAsString(buffer, 256), 16);

      v = response->getVariable(3);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
         _tcslcpy(hwInfo->serialNumber, v->getValueAsString(buffer, 256), 128);

      v = response->getVariable(4);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
         _tcslcpy(hwInfo->productCode, v->getValueAsString(buffer, 256), 32);

      v = response->getVariable(5);
      if ((hwInfo->serialNumber[0] == 0) && (v != nullptr) && (v->getType() == ASN_OCTET_STRING))
         _tcslcpy(hwInfo->serialNumber, v->getValueAsString(buffer, 256), 128);

      delete response;
      return true;
   }

   // F10-S-SERIES-CHASSIS-MIB::chStackUnitTable (legacy FTOS releases)
   unitIndex = FindManagementUnit(snmp, _T(".1.3.6.1.4.1.6027.3.10.1.2.2.1.4"));
   if (unitIndex != 0)
   {
      SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
      TCHAR oid[128];
      _sntprintf(oid, 128, _T(".1.3.6.1.4.1.6027.3.10.1.2.2.1.7.%u"), unitIndex);   // chStackUnitModelID
      request.bindVariable(new SNMP_Variable(oid));
      _sntprintf(oid, 128, _T(".1.3.6.1.4.1.6027.3.10.1.2.2.1.10.%u"), unitIndex);  // chStackUnitCodeVersion
      request.bindVariable(new SNMP_Variable(oid));
      _sntprintf(oid, 128, _T(".1.3.6.1.4.1.6027.3.10.1.2.2.1.12.%u"), unitIndex);  // chStackUnitSerialNumber
      request.bindVariable(new SNMP_Variable(oid));
      _sntprintf(oid, 128, _T(".1.3.6.1.4.1.6027.3.10.1.2.2.1.20.%u"), unitIndex);  // chStackUnitPartNum
      request.bindVariable(new SNMP_Variable(oid));

      SNMP_PDU *response;
      if (snmp->doRequest(&request, &response) != SNMP_ERR_SUCCESS)
      {
         nxlog_debug_tag(DEBUG_TAG_FTOS, 5, _T("DellFTOSDriver::getHardwareInformation(%s): request to chStackUnitTable failed"), node->getName());
         return true;
      }

      TCHAR buffer[256];
      const SNMP_Variable *v = response->getVariable(0);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
         _tcslcpy(hwInfo->productName, v->getValueAsString(buffer, 256), 128);

      v = response->getVariable(1);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
         _tcslcpy(hwInfo->productVersion, v->getValueAsString(buffer, 256), 16);

      v = response->getVariable(2);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
         _tcslcpy(hwInfo->serialNumber, v->getValueAsString(buffer, 256), 128);

      v = response->getVariable(3);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
         _tcslcpy(hwInfo->productCode, v->getValueAsString(buffer, 256), 32);

      delete response;
      return true;
   }

   nxlog_debug_tag(DEBUG_TAG_FTOS, 5, _T("DellFTOSDriver::getHardwareInformation(%s): stack unit table not available"), node->getName());
   return true;
}

/**
 * Get list of interfaces for given node
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver data
 * @param useIfXTable if true, usage of ifXTable is allowed
 */
InterfaceList *DellFTOSDriver::getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable)
{
   InterfaceList *ifList = NetworkDeviceDriver::getInterfaces(snmp, node, driverData, useIfXTable);
   if (ifList == nullptr)
      return nullptr;

   // Physical ports are named "<Type> unit/port" (e.g. "TenGigabitEthernet 1/24", "fortyGigE 1/49"), where unit
   // is stack unit number for stackable switches or slot number for chassis-based systems. Breakout ports carry
   // an additional lane number (e.g. "TenGigabitEthernet 1/49/1"), and InterfacePhysicalLocation has no field to
   // hold it - such interfaces are intentionally left without physical location. Out-of-band management port
   // ("ManagementEthernet 1/1") is excluded by the explicit list of front panel port types.
   const char *eptr;
   int eoffset;
   PCREHandle re(_pcre_compile_t(
         reinterpret_cast<const PCRE_TCHAR*>(_T("^(FastEthernet|GigabitEthernet|TenGigabitEthernet|TwentyFiveGigE|FortyGigE|HundredGigE)\\s*([0-9]+)/([0-9]+)$")),
         PCRE_COMMON_FLAGS | PCRE_CASELESS, &eptr, &eoffset, nullptr));
   if (re == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_FTOS, 5, _T("DellFTOSDriver::getInterfaces: cannot compile port name regexp: %hs at offset %d"), eptr, eoffset);
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
      iface->location.chassis = 1;
      iface->location.module = IntegerFromCGroup(text, pmatch, 2);
      iface->location.port = IntegerFromCGroup(text, pmatch, 3);
   }

   return ifList;
}

/**
 * Get SSH driver hints for interactive CLI sessions
 */
void DellFTOSDriver::getSSHDriverHints(SSHDriverHints *hints) const
{
   // FTOS prompt patterns (IOS-like):
   // - EXEC mode: hostname>
   // - EXEC Privilege mode: hostname#
   // - Config mode: hostname(conf)#, hostname(conf-if-te-1/1)#, etc.
   hints->promptPattern = "^[\\w.-]+(\\([\\w/.-]+\\))?[>#]\\s*$";
   hints->enabledPromptPattern = "^[\\w.-]+(\\([\\w/.-]+\\))?#\\s*$";

   // Privilege escalation
   hints->enableCommand = "enable";
   hints->enablePromptPattern = "[Pp]assword:\\s*$";

   // Pagination control
   hints->paginationDisableCmd = "terminal length 0";
   hints->paginationPrompt = "--[Mm]ore--";
   hints->paginationContinue = " ";

   // Exit command
   hints->exitCommand = "exit";

   // Test command for verifying command mode support
   hints->testCommand = "show version";
   hints->testCommandPattern = "Real Time Operating System";

   // Timeouts
   hints->commandTimeout = 30000;
   hints->connectTimeout = 15000;
}

/**
 * Check if config backup is supported
 */
bool DellFTOSDriver::isConfigBackupSupported()
{
   return true;
}

/**
 * Get running configuration via interactive SSH
 */
bool DellFTOSDriver::getRunningConfig(DeviceBackupContext *ctx, ByteStream *output)
{
   SSHInteractiveChannel *ssh = ctx->getInteractiveSSH();
   if (ssh == nullptr)
      return false;
   return ssh->executeCommand("show running-config", output);
}

/**
 * Get startup configuration via interactive SSH
 */
bool DellFTOSDriver::getStartupConfig(DeviceBackupContext *ctx, ByteStream *output)
{
   SSHInteractiveChannel *ssh = ctx->getInteractiveSSH();
   if (ssh == nullptr)
      return false;
   return ssh->executeCommand("show startup-config", output);
}
