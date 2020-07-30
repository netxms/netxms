/*
** NetXMS - Network Management System
** Drivers for Cambium devices
** Copyright (C) 2020 Raden Solutions
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
** File: cnpilot.cpp
**
**/

#include "cambium.h"

/**
 * Get driver name
 */
const TCHAR *CambiumCnPilotDriver::getName()
{
   return _T("CAMBIUM-CNPILOT");
}

/**
 * Get driver version
 */
const TCHAR *CambiumCnPilotDriver::getVersion()
{
   return NETXMS_VERSION_STRING;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int CambiumCnPilotDriver::isPotentialDevice(const TCHAR *oid)
{
   return !_tcsncmp(oid, _T(".1.3.6.1.4.1.17713.22."), 22) ? 200 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool CambiumCnPilotDriver::isDeviceSupported(SNMP_Transport *snmp, const TCHAR *oid)
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
bool CambiumCnPilotDriver::getHardwareInformation(SNMP_Transport *snmp, NObject *node, DriverData *driverData, DeviceHardwareInfo *hwInfo)
{
   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.17713.22.1.1.1.8.0")));  // cambiumAPSWVersion
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.17713.22.1.1.1.4.0")));  // cambiumAPSerialNum
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.17713.22.1.1.1.5.0")));  // cambiumAPModel

   SNMP_PDU *response;
   if (snmp->doRequest(&request, &response, SnmpGetDefaultTimeout(), 3) != SNMP_ERR_SUCCESS)
      return false;

   if (response->getNumVariables() != request.getNumVariables())
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Malformed device response in CambiumCnPilotDriver::getHardwareInformation"));
      delete response;
      return false;
   }

   _tcscpy(hwInfo->vendor, _T("Cambium"));

   response->getVariable(0)->getValueAsString(hwInfo->productVersion, sizeof(hwInfo->productVersion) / sizeof(TCHAR));
   response->getVariable(1)->getValueAsString(hwInfo->serialNumber, sizeof(hwInfo->serialNumber) / sizeof(TCHAR));
   response->getVariable(2)->getValueAsString(hwInfo->productName, sizeof(hwInfo->productName) / sizeof(TCHAR));

   delete response;
   return true;
}
