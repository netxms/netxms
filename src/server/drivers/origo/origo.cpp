/*
** NetXMS - Network Management System
** Driver for Origo Networks switches
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
** File: origo.cpp
**
**/

#include "origo.h"

/**
 * Get driver name
 */
const TCHAR *OrigoDriver::getName()
{
   return _T("ORIGO");
}

/**
 * Get driver version
 */
const TCHAR *OrigoDriver::getVersion()
{
   return NETXMS_VERSION_STRING;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int OrigoDriver::isPotentialDevice(const SNMP_ObjectId& oid)
{
   return oid.startsWith({ 1, 3, 6, 1, 4, 1, 52208, 1, 1, 1 }) ? 127 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param context device context
 * @param oid Device OID
 */
bool OrigoDriver::isDeviceSupported(DeviceContext *context, const SNMP_ObjectId& oid)
{
   return true;
}

/**
 * Get port layout of given module
 *
 * @param context device context
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @param module Module number (starting from 1)
 * @param layout Layout structure to fill
 */
void OrigoDriver::getModuleLayout(DeviceContext *context, NObject *node, DriverData *driverData, int module, NDD_MODULE_LAYOUT *layout)
{
   layout->numberingScheme = NDD_PN_UD_LR;
   layout->rows = 2;
}

/**
 * Get list of interfaces for given node. Origo devices number physical Ethernet
 * ports directly via ifIndex, so each Ethernet (ifType=6) interface is mapped
 * to a single chassis/module.
 *
 * @param context device context
 * @param node Node
 * @param driverData driver-specific data
 * @param useIfXTable if true, usage of ifXTable is allowed
 */
InterfaceList *OrigoDriver::getInterfaces(DeviceContext *context, NObject *node, DriverData *driverData, bool useIfXTable)
{
   InterfaceList *ifList = NetworkDeviceDriver::getInterfaces(context, node, driverData, true);
   if (ifList == nullptr)
      return nullptr;

   for(int i = 0; i < ifList->size(); i++)
   {
      InterfaceInfo *iface = ifList->get(i);
      if (iface->type == IFTYPE_ETHERNET_CSMACD)
      {
         iface->isPhysicalPort = true;
         iface->location.chassis = 1;
         iface->location.module = 0;
         iface->location.port = iface->index;
      }
   }

   return ifList;
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
bool OrigoDriver::getHardwareInformation(DeviceContext *context, NObject *node, DriverData *driverData, DeviceHardwareInfo *hwInfo)
{
   SNMP_Transport *snmp = context->getSNMPTransport();
   wcscpy(hwInfo->vendor, L"Origo Networks");

   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
   request.bindVariable(new SNMP_Variable({ 1, 3, 6, 1, 4, 1, 52208, 100, 1, 2, 0 }));    // Product code
   request.bindVariable(new SNMP_Variable({ 1, 3, 6, 1, 2, 1, 1, 1, 0 }));              // Product name (sysDescr)
   request.bindVariable(new SNMP_Variable({ 1, 3, 6, 1, 4, 1, 52208, 100, 1, 3, 0 }));    // Firmware version
   request.bindVariable(new SNMP_Variable({ 1, 3, 6, 1, 4, 1, 52208, 100, 1, 101, 0 }));  // Serial number

   SNMP_PDU *response;
   if (snmp->doRequest(&request, &response) == SNMP_ERR_SUCCESS)
   {
      TCHAR buffer[256];

      const SNMP_Variable *v = response->getVariable(0);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
      {
         _tcslcpy(hwInfo->productCode, v->getValueAsString(buffer, 256), 32);
      }

      // sysDescr typically looks like "Origo OE-5108GE5 Gigabit Ethernet Switch, ..."
      // Extract the model token (second word of the segment before the first comma).
      v = response->getVariable(1);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
      {
         v->getValueAsString(buffer, 256);
         TCHAR *comma = _tcschr(buffer, _T(','));
         if (comma != nullptr)
            *comma = 0;
         StringList tokens = String::split(buffer, _T(" "), true);
         if (tokens.size() > 1)
            _tcslcpy(hwInfo->productName, tokens.get(1), 128);
         else
            _tcslcpy(hwInfo->productName, buffer, 128);
      }

      v = response->getVariable(2);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
      {
         _tcslcpy(hwInfo->productVersion, v->getValueAsString(buffer, 256), 16);
      }

      v = response->getVariable(3);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
      {
         _tcslcpy(hwInfo->serialNumber, v->getValueAsString(buffer, 256), 32);
      }

      delete response;
   }
   return true;
}

/**
 * Driver entry point
 */
DECLARE_NDD_ENTRY_POINT(OrigoDriver);

#ifdef _WIN32

/**
 * DLL entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
      DisableThreadLibraryCalls(hInstance);
   return TRUE;
}

#endif
