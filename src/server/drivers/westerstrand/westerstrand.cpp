/* 
** NetXMS - Network Management System
** Driver for Westerstrand clocks
** Copyright (C) 2003-2020 Raden Solutions
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
** File: westerstrand.cpp
**/

#include "westerstrand.h"
#include <netxms-version.h>

#define DEBUG_TAG _T("ndd.westerstrand")

/**
 * Driver name
 */
static TCHAR s_driverName[] = _T("WESTERSTRAND");

/**
 * Driver version
 */
static TCHAR s_driverVersion[] = NETXMS_VERSION_STRING;

/**
 * Get driver name
 */
const TCHAR *WesterstrandDriver::getName()
{
	return s_driverName;
}

/**
 * Get driver version
 */
const TCHAR *WesterstrandDriver::getVersion()
{
	return s_driverVersion;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int WesterstrandDriver::isPotentialDevice(const TCHAR *oid)
{
	return !_tcsncmp(oid, _T(".1.3.6.1.4.1.25281."), 19) ? 254 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool WesterstrandDriver::isDeviceSupported(SNMP_Transport *snmp, const TCHAR *oid)
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
bool WesterstrandDriver::getHardwareInformation(SNMP_Transport *snmp, NObject *node, DriverData *driverData, DeviceHardwareInfo *hwInfo)
{
   _tcscpy(hwInfo->vendor, _T("Westerstrand"));

   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.25281.100.1.1.0")));  // Product name
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.25281.100.1.2.0")));  // Firmware version

   SNMP_PDU *response;
   if (snmp->doRequest(&request, &response, SnmpGetDefaultTimeout(), 3) == SNMP_ERR_SUCCESS)
   {
      TCHAR buffer[256];

      SNMP_Variable *v = response->getVariable(0);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
      {
         _tcslcpy(hwInfo->productName, v->getValueAsString(buffer, 256), 128);
      }

      v = response->getVariable(1);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
      {
         _tcslcpy(hwInfo->productVersion, v->getValueAsString(buffer, 256), 16);
      }

      delete response;
   }
   return true;
}

/**
 * Driver entry point
 */
DECLARE_NDD_ENTRY_POINT(WesterstrandDriver);

/**
 * DLL entry point
 */
#ifdef _WIN32

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
		DisableThreadLibraryCalls(hInstance);
	return TRUE;
}

#endif
