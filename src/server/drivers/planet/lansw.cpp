/* 
** NetXMS - Network Management System
** Driver for Planet Technology Corp. LAN switch devices
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
** File: lansw.cpp
**/

#include "planet.h"

#define DEBUG_TAG _T("ndd.planet.lansw")

/**
 * Get driver name
 */
const wchar_t *PlanetLanSwDriver::getName()
{
   return L"PLANET-LANSW";
}

/**
 * Get driver version
 */
const wchar_t *PlanetLanSwDriver::getVersion()
{
   return NETXMS_VERSION_STRING;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int PlanetLanSwDriver::isPotentialDevice(const SNMP_ObjectId& oid)
{
	return oid.startsWith({ 1, 3, 6, 1, 4, 1, 10456, 1 }) ? 255 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool PlanetLanSwDriver::isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid)
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
bool PlanetLanSwDriver::getHardwareInformation(SNMP_Transport *snmp, NObject *node, DriverData *driverData, DeviceHardwareInfo *hwInfo)
{
   wcscpy(hwInfo->vendor, L"Planet Technology Corp.");

   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
   request.bindVariable(new SNMP_Variable({ 1, 3, 6, 1, 4, 1, 10456, 3, 6, 21, 0 }));  // nmsSwitchName
   request.bindVariable(new SNMP_Variable({ 1, 3, 6, 1, 4, 1, 10456, 3, 6, 3, 0 }));  // nmschassisId (serial number)
   request.bindVariable(new SNMP_Variable({ 1, 3, 6, 1, 4, 1, 10456, 3, 6, 10, 1, 6, 0 }));  // nmscardSwVersion (only for non-modular switches)

   SNMP_PDU *response;
   if (snmp->doRequest(&request, &response) == SNMP_ERR_SUCCESS)
   {
      wchar_t buffer[256];

      SNMP_Variable *v = response->getVariable(0);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
      {
         wcslcpy(hwInfo->productName, v->getValueAsString(buffer, 256), 128);
      }

      v = response->getVariable(1);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
      {
         wcslcpy(hwInfo->serialNumber, v->getValueAsString(buffer, 256), 32);
      }

      v = response->getVariable(2);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
      {
         wcslcpy(hwInfo->productVersion, v->getValueAsString(buffer, 256), 16);
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
InterfaceList *PlanetLanSwDriver::getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable)
{
   InterfaceList *ifList = NetworkDeviceDriver::getInterfaces(snmp, node, driverData, true);
   if (ifList == nullptr)
      return nullptr;

   SnmpWalk(snmp, { 1, 3, 6, 1, 4, 1, 10456, 3, 6, 12, 1, 3 }, // nmscardIfSlotNumber
      [ifList] (SNMP_Variable *var) -> uint32_t
      {
         const SNMP_ObjectId& name = var->getName();
         uint32_t ifIndex = name.getLastElement();
         InterfaceInfo *iface = ifList->findByIfIndex(ifIndex);
         if (iface != nullptr)
         {
            iface->location.module = var->getValueAsInt();
            iface->isPhysicalPort = true;
         }
         return SNMP_ERR_SUCCESS;
      });

   SnmpWalk(snmp, { 1, 3, 6, 1, 4, 1, 10456, 3, 6, 12, 1, 2 }, // nmscardIfPortNumber
      [ifList] (SNMP_Variable *var) -> uint32_t
      {
         const SNMP_ObjectId& name = var->getName();
         uint32_t ifIndex = name.getLastElement();
         InterfaceInfo *iface = ifList->findByIfIndex(ifIndex);
         if (iface != nullptr)
         {
            uint32_t n = var->getValueAsUInt();
            iface->location.pic = (n >> 16) & 0x0F;
            iface->location.port = n & 0xFFFF;
            iface->isPhysicalPort = true;
         }
         return SNMP_ERR_SUCCESS;
      });

   for (int i = 0; i < ifList->size(); i++)
   {
      InterfaceInfo *iface = ifList->get(i);
      if (iface->type == IFTYPE_ETHERNET_CSMACD)
         iface->isPhysicalPort = true;
   }

   return ifList;
}
