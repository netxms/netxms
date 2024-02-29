/* 
** NetXMS - Network Management System
** Driver for Teltonika modems
** Copyright (C) 2022-2024 Raden Solutions
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
** File: teltonika.cpp
**/

#include "net-snmp.h"
#include <netxms-version.h>

#define DEBUG_TAG _T("ndd.teltonika")

/**
 * Get driver name
 */
const TCHAR *TeltonikaDriver::getName()
{
	return _T("TELTONIKA");
}

/**
 * Get driver version
 */
const TCHAR *TeltonikaDriver::getVersion()
{
	return NETXMS_VERSION_STRING;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int TeltonikaDriver::isPotentialDevice(const SNMP_ObjectId& oid)
{
	return oid.equals({ 1, 3, 6, 1, 4, 1, 8072, 3, 2, 10 }) ? 254 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool TeltonikaDriver::isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid)
{
   char buffer[256];
	return SnmpGetEx(snmp, _T(".1.3.6.1.4.1.48690.1.1.0"), nullptr, 0, buffer, sizeof(buffer), SG_RAW_RESULT) == SNMP_ERR_SUCCESS;
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
bool TeltonikaDriver::getHardwareInformation(SNMP_Transport *snmp, NObject *node, DriverData *driverData, DeviceHardwareInfo *hwInfo)
{
   _tcscpy(hwInfo->vendor, _T("Teltonika"));

   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.48690.1.2.0")));  // Product name
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.48690.1.3.0")));  // Product code
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.48690.1.6.0")));  // Firmware version
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.48690.1.1.0")));  // Serial number

   SNMP_PDU *response;
   uint32_t rc;
   if ((rc = snmp->doRequest(&request, &response)) != SNMP_ERR_SUCCESS)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("TeltonikaDriver::getHardwareInformation(%s): SNMP failure (%s)"), node->getName(), SnmpGetErrorText(rc));
      return false;
   }

   TCHAR buffer[256];

   SNMP_Variable *v = response->getVariable(0);
   if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
   {
      _tcslcpy(hwInfo->productName, v->getValueAsString(buffer, 256), 128);
      nxlog_debug_tag(DEBUG_TAG, 5, _T("TeltonikaDriver::getHardwareInformation(%s): retrieved product name \"%s\""), node->getName(), buffer);
   }

   v = response->getVariable(1);
   if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
   {
      _tcslcpy(hwInfo->productCode, v->getValueAsString(buffer, 256), 32);
      nxlog_debug_tag(DEBUG_TAG, 5, _T("TeltonikaDriver::getHardwareInformation(%s): retrieved product code \"%s\""), node->getName(), buffer);
   }

   v = response->getVariable(2);
   if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
   {
      _tcslcpy(hwInfo->productVersion, v->getValueAsString(buffer, 256), 16);
      nxlog_debug_tag(DEBUG_TAG, 5, _T("TeltonikaDriver::getHardwareInformation(%s): retrieved product version \"%s\""), node->getName(), buffer);
   }

   v = response->getVariable(3);
   if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
   {
      _tcslcpy(hwInfo->serialNumber, v->getValueAsString(buffer, 256), 32);
      nxlog_debug_tag(DEBUG_TAG, 5, _T("TeltonikaDriver::getHardwareInformation(%s): retrieved product serial number \"%s\""), node->getName(), buffer);
   }

   delete response;
   return true;
}
