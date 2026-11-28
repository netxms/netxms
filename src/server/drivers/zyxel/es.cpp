/*
** NetXMS - Network Management System
** Driver for Zyxel ES (Enterprise Switch) product line
** Copyright (C) 2003-2026 Victor Kirhenshtein
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
** File: es.cpp
**/

#include "zyxel.h"
#include <netxms-version.h>

/**
 * Get driver name
 */
const TCHAR *ZyxelESSwitchDriver::getName()
{
   return _T("ZYXEL-ES");
}

/**
 * Get driver version
 */
const TCHAR *ZyxelESSwitchDriver::getVersion()
{
   return NETXMS_VERSION_STRING;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int ZyxelESSwitchDriver::isPotentialDevice(const SNMP_ObjectId& oid)
{
   return oid.startsWith({ 1, 3, 6, 1, 4, 1, 890, 1, 15 }) ? 127 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param context device context
 * @param oid Device OID
 */
bool ZyxelESSwitchDriver::isDeviceSupported(DeviceContext *context, const SNMP_ObjectId& oid)
{
   return true;
}

/**
 * Get hardware information from device.
 *
 * @param context device context
 * @param node Node
 * @param driverData driver data
 * @param hwInfo pointer to hardware information structure to fill
 * @return true if hardware information is available
 */
bool ZyxelESSwitchDriver::getHardwareInformation(DeviceContext *context, NObject *node, DriverData *driverData, DeviceHardwareInfo *hwInfo)
{
   SNMP_Transport *snmp = context->getSNMPTransport();

   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
   request.bindVariable(new SNMP_Variable({ 1, 3, 6, 1, 4, 1, 890, 1, 15, 3, 1, 11, 0 }));  // sysProductModel
   request.bindVariable(new SNMP_Variable({ 1, 3, 6, 1, 4, 1, 890, 1, 15, 3, 1, 12, 0 }));  // sysProductSerialNumber
   request.bindVariable(new SNMP_Variable({ 1, 3, 6, 1, 4, 1, 890, 1, 15, 3, 1, 15, 0 }));  // sysHwVersionString

   SNMP_PDU *response;
   if (snmp->doRequest(&request, &response) != SNMP_ERR_SUCCESS)
      return false;

   wcscpy(hwInfo->vendor, L"Zyxel");

   const SNMP_Variable *v = response->getVariable(0);
   if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
   {
      v->getValueAsString(hwInfo->productName, sizeof(hwInfo->productName) / sizeof(wchar_t));
      TrimW(hwInfo->productName);
   }

   v = response->getVariable(1);
   if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
   {
      v->getValueAsString(hwInfo->serialNumber, sizeof(hwInfo->serialNumber) / sizeof(wchar_t));
      TrimW(hwInfo->serialNumber);
   }

   v = response->getVariable(2);
   if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
   {
      v->getValueAsString(hwInfo->productVersion, sizeof(hwInfo->productVersion) / sizeof(wchar_t));
      TrimW(hwInfo->productVersion);
   }

   delete response;
   return true;
}

/**
 * Get firmware information from device
 */
bool ZyxelESSwitchDriver::getFirmwareInformation(DeviceContext *context, NObject *node, DriverData *driverData, StructArray<FirmwareInfo> *firmware)
{
   wchar_t raw[256];
   if (!ReadFirmwareVersion(context->getSNMPTransport(), { 1, 3, 6, 1, 4, 1, 890, 1, 15, 3, 1, 6, 0 }, raw, 256))  // sysSwVersionString
      return false;
   if (raw[0] == 0)
      return NetworkDeviceDriver::getFirmwareInformation(context, node, driverData, firmware);

   // Version is the first word, followed by build date ("V2.70(ABTO.0) | 10/22/2021")
   wchar_t version[64];
   wcslcpy(version, raw, 64);
   wchar_t *sp = wcschr(version, L' ');
   if (sp != nullptr)
      *sp = 0;

   AddFirmwareInfo(firmware, FirmwareRole::MAIN, L"", L"", version, raw);
   return true;
}

/**
 * Get port layout of given module. Default implementation always set NDD_PN_UNKNOWN as layout.
 *
 * @param context device context
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @param module Module number (starting from 1)
 * @param layout Layout structure to fill
 */
void ZyxelESSwitchDriver::getModuleLayout(DeviceContext *context, NObject *node, DriverData *driverData, int module, NDD_MODULE_LAYOUT *layout)
{
   layout->numberingScheme = NDD_PN_DU_LR;
   layout->rows = 2;
}

/**
 * Get list of interfaces for given node
 *
 * @param context device context
 * @param node Node
 * @param driverData driver data
 * @param useIfXTable if true, usage of ifXTable is allowed
 */
InterfaceList *ZyxelESSwitchDriver::getInterfaces(DeviceContext *context, NObject *node, DriverData *driverData, bool useIfXTable)
{
   InterfaceList *ifList = NetworkDeviceDriver::getInterfaces(context, node, driverData, useIfXTable);
   if (ifList == nullptr)
      return nullptr;

   for (int i = 0; i < ifList->size(); i++)
   {
      InterfaceInfo *iface = ifList->get(i);
      if (iface->type == IFTYPE_ETHERNET_CSMACD)
      {
         iface->setPhysicalLocation(0, 1, 0, iface->index);
      }
   }

   return ifList;
}
