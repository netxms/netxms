/*
** NetXMS - Network Management System
** Driver for Planet Technology Corp. industrial switch devices
** Copyright (C) 2003-2025 Raden Solutions
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
** File: isw.cpp
**/

#include "planet.h"

#define DEBUG_TAG _T("ndd.planet.isw")

/**
 * Get driver name
 */
const wchar_t *PlanetIndustrialSwDriver::getName()
{
   return L"PLANET-ISW";
}

/**
 * Get driver version
 */
const wchar_t *PlanetIndustrialSwDriver::getVersion()
{
   return NETXMS_VERSION_STRING;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int PlanetIndustrialSwDriver::isPotentialDevice(const SNMP_ObjectId& oid)
{
   return oid.startsWith({ 1, 3, 6, 1, 4, 1, 10456, 9 }) ? 250 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool PlanetIndustrialSwDriver::isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid)
{
   BYTE buffer[256];
   return SnmpGet(snmp->getSnmpVersion(), snmp, { 1, 3, 6, 1, 4, 1, 6603, 1, 24, 1, 3, 5, 2, 0 }, buffer, sizeof(buffer), SG_RAW_RESULT) == SNMP_ERR_SUCCESS;
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
bool PlanetIndustrialSwDriver::getHardwareInformation(SNMP_Transport *snmp, NObject *node, DriverData *driverData, DeviceHardwareInfo *hwInfo)
{
   wcscpy(hwInfo->vendor, L"Planet Technology Corp.");

   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
   request.bindVariable(new SNMP_Variable({ 1, 3, 6, 1, 4, 1, 6603, 1, 28, 1, 3, 3, 1, 5, 1 }));  // vtssFirmwareStatusSwitchProduct
   request.bindVariable(new SNMP_Variable({ 1, 3, 6, 1, 4, 1, 6603, 1, 28, 1, 3, 3, 1, 6, 1 }));  // vtssFirmwareStatusSwitchVersion
   request.bindVariable(new SNMP_Variable({ 1, 3, 6, 1, 4, 1, 6603, 1, 24, 1, 3, 5, 3, 0 }));  // vtssSysutilStatusBoardInfoBoardSerial

   SNMP_PDU *response;
   if (snmp->doRequest(&request, &response) == SNMP_ERR_SUCCESS)
   {
      wchar_t buffer[256];

      SNMP_Variable *v = response->getVariable(0);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
      {
         v->getValueAsString(buffer, 256);
         if (!wcsncmp(buffer, L"PLANET,", 7))
         {
            wchar_t *p = wcschr(&buffer[7], L',');
            if (p != nullptr)
               *p = 0;
            wcslcpy(hwInfo->productName, &buffer[7], 128);
         }
         else
         {
            wcslcpy(hwInfo->productName, buffer, 128);
         }
         TrimW(hwInfo->productName);
      }

      v = response->getVariable(1);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
      {
         wcslcpy(hwInfo->productVersion, v->getValueAsString(buffer, 256), 16);
         TrimW(hwInfo->productVersion);
      }

      v = response->getVariable(2);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
      {
         wcslcpy(hwInfo->serialNumber, v->getValueAsString(buffer, 256), 32);
         TrimW(hwInfo->serialNumber);
      }

      delete response;
   }
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
InterfaceList *PlanetIndustrialSwDriver::getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable)
{
   InterfaceList *ifList = NetworkDeviceDriver::getInterfaces(snmp, node, driverData, true);
   if (ifList == nullptr)
      return nullptr;

   for(int i = 0; i < ifList->size(); i++)
   {
      InterfaceInfo *iface = ifList->get(i);
      if (iface->type == IFTYPE_ETHERNET_CSMACD)
      {
         iface->isPhysicalPort = true;
         if (!wcsncmp(iface->description, L"Port ", 5))
         {
            wchar_t *eptr;
            uint32_t portNumber = wcstoul(&iface->description[5], &eptr, 10);
            if ((*eptr == 0) && (portNumber > 0))
            {
               iface->location.module = 1;
               iface->location.port = portNumber;
               nxlog_debug_tag(DEBUG_TAG, 6, L"PlanetIndustrialSwDriver::getInterfaces(%s [%u]): Interface %u (%s): parsed location %d.%d.%d",
                  node->getName(), node->getId(), iface->index, iface->name, iface->location.module, iface->location.pic, iface->location.port);
            }
         }
      }
   }

   return ifList;
}
