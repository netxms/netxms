/*
** NetXMS - Network Management System
** Drivers for Siemens RuggedCom switches
** Copyright (C) 2024-2025 Raden Solutions
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
** File: ruggedcom.cpp
**
**/

#include "siemens.h"
#include <interface_types.h>

/**
 * Get driver name
 */
const TCHAR *RuggedComDriver::getName()
{
   return _T("RUGGEDCOM");
}

/**
 * Get driver version
 */
const TCHAR *RuggedComDriver::getVersion()
{
   return NETXMS_VERSION_STRING;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int RuggedComDriver::isPotentialDevice(const SNMP_ObjectId& oid)
{
   return oid.equals({ 1, 3, 6, 1, 4, 1, 15004, 2, 1 }) ? 250 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool RuggedComDriver::isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid)
{
   return true;
}

/**
 * Get hardware information from device.
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver data
 * @param hwInfo pointer to hardware information structure to fill
 * @return true if hardware information is available
 */
bool RuggedComDriver::getHardwareInformation(SNMP_Transport *snmp, NObject *node, DriverData *driverData, DeviceHardwareInfo *hwInfo)
{
   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
   request.bindVariable(new SNMP_Variable({ 1, 3, 6, 1, 4, 1, 15004, 4, 2, 3, 3, 0 }));  // rcDeviceInfoMainSwVersion
   request.bindVariable(new SNMP_Variable({ 1, 3, 6, 1, 4, 1, 15004, 4, 2, 3, 1, 0 }));  // rcDeviceInfoSerialNumber
   request.bindVariable(new SNMP_Variable({ 1, 3, 6, 1, 2, 1, 1, 1, 0 }));           // sysDescr

   SNMP_PDU *response;
   if (snmp->doRequest(&request, &response) != SNMP_ERR_SUCCESS)
      return false;

   if (response->getNumVariables() != request.getNumVariables())
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Malformed device response in RuggedComDriver::getHardwareInformation"));
      delete response;
      return false;
   }

   wcscpy(hwInfo->vendor, L"Siemens AG");

   response->getVariable(0)->getValueAsString(hwInfo->productVersion, sizeof(hwInfo->productVersion) / sizeof(wchar_t));
   response->getVariable(1)->getValueAsString(hwInfo->serialNumber, sizeof(hwInfo->serialNumber) / sizeof(wchar_t));

   wchar_t *sp = wcschr(hwInfo->productVersion, L' ');
   if (sp != nullptr)
      *sp = 0;
   if (hwInfo->productVersion[0] == 'v')
      memmove(hwInfo->productVersion, &hwInfo->productVersion[1], wcslen(hwInfo->productVersion) * sizeof(wchar_t));

   // Sample sysDescription:
   // Siemens, RUGGEDCOM, RS900G, RS900G-HI-D-2SC25, HW: Version RS900G, FW: Version Main v4.3.7 Boot v4.3.0, S900G-1109-06613
   wchar_t sysDescription[1024];
   response->getVariable(2)->getValueAsString(sysDescription, 1024);
   StringList fields = String::split(sysDescription, L",", true);
   if ((fields.size() > 2) && !wcsicmp(fields.get(0), L"Siemens") && !wcsicmp(fields.get(1), L"RUGGEDCOM"))
   {
      wcslcpy(hwInfo->productName, fields.get(2), sizeof(hwInfo->productName) / sizeof(wchar_t));
      if (fields.size() > 3)
      {
         wcslcpy(hwInfo->productCode, fields.get(3), sizeof(hwInfo->productCode) / sizeof(wchar_t));
      }
   }

   delete response;
   return true;
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
InterfaceList *RuggedComDriver::getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable)
{
   InterfaceList *ifList = NetworkDeviceDriver::getInterfaces(snmp, node, driverData, useIfXTable);
   if (ifList == nullptr)
      return nullptr;

   // Mark physical ports
   for(int i = 0; i < ifList->size(); i++)
   {
      InterfaceInfo *iface = ifList->get(i);
      if (iface->type == IFTYPE_ETHERNET_CSMACD)
      {
         TCHAR *eptr;
         int n = _tcstol(iface->name, &eptr, 10);
         if (*eptr == 0)
         {
            iface->isPhysicalPort = true;
            iface->location = InterfacePhysicalLocation(0, 0, 0, n);
         }
      }
   }

   return ifList;
}
