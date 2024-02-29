/* 
** NetXMS - Network Management System
** Drivers for Ubiquiti Networks devices
** Copyright (C) 2003-2024 Victor Kirhenshtein
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
** File: edgeswitch.cpp
**/

#include "ubnt.h"
#include <netxms-regex.h>
#include <netxms-version.h>

/**
 * Get driver name
 */
const TCHAR *UbiquitiEdgeSwitchDriver::getName()
{
	return _T("UBNT-EDGESW");
}

/**
 * Get driver version
 */
const TCHAR *UbiquitiEdgeSwitchDriver::getVersion()
{
	return NETXMS_VERSION_STRING;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int UbiquitiEdgeSwitchDriver::isPotentialDevice(const SNMP_ObjectId& oid)
{
	return oid.equals({ 1, 3, 6, 1, 4, 1, 4413 }) ? 254 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool UbiquitiEdgeSwitchDriver::isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid)
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
bool UbiquitiEdgeSwitchDriver::getHardwareInformation(SNMP_Transport *snmp, NObject *node, DriverData *driverData, DeviceHardwareInfo *hwInfo)
{
   _tcscpy(hwInfo->vendor, _T("Ubiquiti"));

   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.4413.1.1.1.1.1.3.0")));  // Product code
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.4413.1.1.1.1.1.2.0")));  // Product name
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.4413.1.1.1.1.1.13.0"))); // Firmware version
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.4413.1.1.1.1.1.4.0")));  // Serial number

   SNMP_PDU *response;
   if (snmp->doRequest(&request, &response) == SNMP_ERR_SUCCESS)
   {
      TCHAR buffer[256];

      SNMP_Variable *v = response->getVariable(0);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
      {
         _tcslcpy(hwInfo->productCode, v->getValueAsString(buffer, 256), 32);
      }

      v = response->getVariable(1);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
      {
         _tcslcpy(hwInfo->productName, v->getValueAsString(buffer, 256), 128);
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
 * Get list of interfaces for given node
 *
 * @param snmp SNMP transport
 * @param node Node
 */
InterfaceList *UbiquitiEdgeSwitchDriver::getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable)
{
   // Get interface list from standard MIB
   InterfaceList *ifList = NetworkDeviceDriver::getInterfaces(snmp, node, driverData, useIfXTable);
   if (ifList == nullptr)
      return nullptr;

   const char *eptr;
   int eoffset;
   PCRE *re = _pcre_compile_t(reinterpret_cast<const PCRE_TCHAR*>(_T("^([0-9]+)/([0-9]+)$")), PCRE_COMMON_FLAGS | PCRE_CASELESS, &eptr, &eoffset, nullptr);
   if (re == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_UBNT, 5, _T("UbiquitiEdgeSwitchDriver::getInterfaces: cannot compile regexp: %hs at offset %d"), eptr, eoffset);
      return ifList;
   }

   int pmatch[30];
   for(int i = 0; i < ifList->size(); i++)
   {
      InterfaceInfo *iface = ifList->get(i);
      if ((iface->type == IFTYPE_ETHERNET_CSMACD) &&
          (_pcre_exec_t(re, nullptr, reinterpret_cast<PCRE_TCHAR*>(iface->name), static_cast<int>(_tcslen(iface->name)), 0, 0, pmatch, 30) == 3))
      {
         iface->isPhysicalPort = true;
         iface->location.chassis = 1;
         iface->location.module = IntegerFromCGroup(iface->name, pmatch, 1);
         iface->location.port = IntegerFromCGroup(iface->name, pmatch, 2);
      }
   }
   _pcre_free_t(re);

   return ifList;
}
