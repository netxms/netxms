/* 
** NetXMS - Network Management System
** Driver for Hirschmann HiOS devices
** Copyright (C) 2023-2024 Raden Solutions
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
** File: hios.cpp
**/

#include "hirschmann.h"

/**
 * Get driver name
 */
const TCHAR *HirschmannHiOSDriver::getName()
{
   return _T("HIRSCHMANN-HIOS");
}

/**
 * Get driver version
 */
const TCHAR *HirschmannHiOSDriver::getVersion()
{
   return NETXMS_VERSION_STRING;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int HirschmannHiOSDriver::isPotentialDevice(const SNMP_ObjectId& oid)
{
   return oid.startsWith({ 1, 3, 6, 1, 4, 1, 248, 11 }) ? 200 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool HirschmannHiOSDriver::isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid)
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
bool HirschmannHiOSDriver::getHardwareInformation(SNMP_Transport *snmp, NObject *node, DriverData *driverData, DeviceHardwareInfo *hwInfo)
{
   _tcscpy(hwInfo->vendor, _T("Hirschmann"));

   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.248.11.101.1.3.1.7.0")));  // Product name
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.248.11.101.1.2.1.6.0")));  // Firmware version
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.248.11.10.1.1.3.0")));  // Serial number

   SNMP_PDU *response;
   if (snmp->doRequest(&request, &response) == SNMP_ERR_SUCCESS)
   {
      TCHAR buffer[256];
      const SNMP_Variable *v = response->getVariable(0);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
      {
         _tcslcpy(hwInfo->productName, v->getValueAsString(buffer, 256), 128);
      }

      v = response->getVariable(1);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
      {
         _tcslcpy(hwInfo->productVersion, v->getValueAsString(buffer, 256), 16);
      }

      v = response->getVariable(2);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
      {
         _tcslcpy(hwInfo->serialNumber, v->getValueAsString(buffer, 256), 32);
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
 */
InterfaceList *HirschmannHiOSDriver::getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable)
{
	InterfaceList *ifList = NetworkDeviceDriver::getInterfaces(snmp, node, driverData, useIfXTable);
	if (ifList == nullptr)
	   return nullptr;

	for(int i = 0; i < ifList->size(); i++)
	{
	   InterfaceInfo *iface = ifList->get(i);
	   if (iface->type != 6)
	      continue;   // Logical interface

	   TCHAR buffer[64];
	   _tcslcpy(buffer, iface->name, 64);

	   TCHAR *p = _tcschr(buffer, _T('/'));
	   if (p != nullptr)
	   {
	      *p = 0;
	      p++;

	      TCHAR *eptr;
	      int module = _tcstol(buffer, &eptr, 10);
	      if (*eptr == 0)
	      {
	         int port = _tcstol(p, &eptr, 10);
	         if (*eptr == 0)
	         {
	            iface->isPhysicalPort = true;
	            iface->location.module = module;
	            iface->location.port = port;
	         }
	      }
	   }
	}

	return ifList;
}
