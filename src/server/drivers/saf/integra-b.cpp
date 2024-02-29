/*
** NetXMS - Network Management System
** Drivers for SAF Tehnika devices
** Copyright (C) 2020-2024 Raden Solutions
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
** File: integra-b.cpp
**
**/

#include "saf.h"

/**
 * Get driver name
 */
const TCHAR *SafIntegraBDriver::getName()
{
   return _T("SAF-INTEGRA-B");
}

/**
 * Get driver version
 */
const TCHAR *SafIntegraBDriver::getVersion()
{
   return NETXMS_VERSION_STRING;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int SafIntegraBDriver::isPotentialDevice(const SNMP_ObjectId& oid)
{
   return oid.equals({ 1, 3, 6, 1, 4, 1, 7571, 100, 1, 1, 7, 1 }) ? 200 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool SafIntegraBDriver::isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid)
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
bool SafIntegraBDriver::getHardwareInformation(SNMP_Transport *snmp, NObject *node, DriverData *driverData, DeviceHardwareInfo *hwInfo)
{
   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.7571.100.1.1.7.1.4.13.0")));  // integraBsysDeviceProductModel
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.7571.100.1.1.7.1.4.12.0")));  // integraBsysDeviceSerial
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.7571.100.1.1.7.1.4.11.0")));  // integraBsysDeviceType
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.2.1.1.1.0")));                    // sysDescr

   SNMP_PDU *response;
   if (snmp->doRequest(&request, &response) != SNMP_ERR_SUCCESS)
      return false;

   if (response->getNumVariables() != request.getNumVariables())
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Malformed device response in SafIntegraBDriver::getHardwareInformation"));
      delete response;
      return false;
   }

   _tcscpy(hwInfo->vendor, _T("SAF Tehnika"));

   response->getVariable(0)->getValueAsString(hwInfo->productCode, sizeof(hwInfo->productCode) / sizeof(TCHAR));
   response->getVariable(1)->getValueAsString(hwInfo->serialNumber, sizeof(hwInfo->serialNumber) / sizeof(TCHAR));
   response->getVariable(2)->getValueAsString(hwInfo->productName, sizeof(hwInfo->productName) / sizeof(TCHAR));

   TCHAR sysDescription[1024];
   response->getVariable(3)->getValueAsString(sysDescription, 1024);
   TCHAR *version = _tcsstr(sysDescription, _T(";Vers:"));
   if (version != nullptr)
   {
      version += 6;
      TCHAR *eptr = _tcschr(version, _T(';'));
      if (eptr != nullptr)
         *eptr = 0;
      Trim(version);
      _tcslcpy(hwInfo->productVersion, version, sizeof(hwInfo->productVersion) / sizeof(TCHAR));
   }

   delete response;
   return true;
}
