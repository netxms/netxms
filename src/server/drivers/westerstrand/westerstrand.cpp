/* 
** NetXMS - Network Management System
** Driver for Westerstrand clocks
** Copyright (C) 2003-2024 Raden Solutions
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
 * Get driver name
 */
const TCHAR *WesterstrandDriver::getName()
{
	return _T("WESTERSTRAND");
}

/**
 * Get driver version
 */
const TCHAR *WesterstrandDriver::getVersion()
{
	return NETXMS_VERSION_STRING;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int WesterstrandDriver::isPotentialDevice(const SNMP_ObjectId& oid)
{
	return oid.startsWith({ 1, 3, 6, 1, 4, 1, 25281 }) ? 254 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool WesterstrandDriver::isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid)
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
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.25281.130.10.1.0")));  // Product name (ANOC MIB)
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.25281.130.10.2.0")));  // Firmware version (ANOC MIB)

   SNMP_PDU *response;
   uint32_t rc;
   if ((rc = snmp->doRequest(&request, &response)) != SNMP_ERR_SUCCESS)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("WesterstrandDriver::getHardwareInformation(%s): SNMP failure (%s)"), node->getName(), SnmpGetErrorText(rc));
      return false;
   }

   TCHAR buffer[256];

   SNMP_Variable *v = response->getVariable(0);
   if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
   {
      _tcslcpy(hwInfo->productName, v->getValueAsString(buffer, 256), 128);
      nxlog_debug_tag(DEBUG_TAG, 5, _T("WesterstrandDriver::getHardwareInformation(%s): retrieved product name \"%s\""), node->getName(), buffer);
   }
   else
   {
      v = response->getVariable(2);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
      {
         _tcslcpy(hwInfo->productName, v->getValueAsString(buffer, 256), 128);
         nxlog_debug_tag(DEBUG_TAG, 5, _T("WesterstrandDriver::getHardwareInformation(%s): retrieved product name \"%s\""), node->getName(), buffer);
      }
   }

   v = response->getVariable(1);
   if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
   {
      _tcslcpy(hwInfo->productVersion, v->getValueAsString(buffer, 256), 16);
      nxlog_debug_tag(DEBUG_TAG, 5, _T("WesterstrandDriver::getHardwareInformation(%s): retrieved product version \"%s\""), node->getName(), buffer);
   }
   else
   {
      v = response->getVariable(3);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
      {
         _tcslcpy(hwInfo->productVersion, v->getValueAsString(buffer, 256), 16);
         nxlog_debug_tag(DEBUG_TAG, 5, _T("WesterstrandDriver::getHardwareInformation(%s): retrieved product version \"%s\""), node->getName(), buffer);
      }
   }

   delete response;
   return true;
}

/**
 * Get device geographical location.
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver data
 * @return device geographical location or "UNSET" type location object
 */
GeoLocation WesterstrandDriver::getGeoLocation(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.25281.130.10.8.5.0")));  // Latitude
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.25281.130.10.8.6.0")));  // Longitude

   SNMP_PDU *response;
   uint32_t rc;
   if ((rc = snmp->doRequest(&request, &response)) != SNMP_ERR_SUCCESS)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("WesterstrandDriver::getGeoLocation(%s): SNMP failure (%s)"), node->getName(), SnmpGetErrorText(rc));
      return GeoLocation();
   }

   double lat, lon;
   bool vlat = false, vlon = false;
   TCHAR buffer[256];

   SNMP_Variable *v = response->getVariable(0);
   if ((v != nullptr) && (v->getType() != ASN_NULL))
   {
      TCHAR *eptr;
      lat = _tcstod(v->getValueAsString(buffer, 256), &eptr);
      if (*eptr == 0)
         vlat = true;
      nxlog_debug_tag(DEBUG_TAG, 5, _T("WesterstrandDriver::getGeoLocation(%s): got latitude \"%s\" (%s number)"), node->getName(), buffer, vlat ? _T("valid") : _T("invalid"));
   }

   v = response->getVariable(1);
   if ((v != nullptr) && (v->getType() != ASN_NULL))
   {
      TCHAR *eptr;
      lon = _tcstod(v->getValueAsString(buffer, 256), &eptr);
      if (*eptr == 0)
         vlon = true;
      nxlog_debug_tag(DEBUG_TAG, 5, _T("WesterstrandDriver::getGeoLocation(%s): got longitude \"%s\" (%s number)"), node->getName(), buffer, vlat ? _T("valid") : _T("invalid"));
   }

   delete response;

   return (vlat && vlon) ? GeoLocation(GL_MANUAL, lat, lon) : GeoLocation();
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
