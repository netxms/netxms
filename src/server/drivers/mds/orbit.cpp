/*
** NetXMS - Network Management System
** Driver for GE MDS devices
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
** File: mds.cpp
**/

#include "mds.h"
#include <netxms-version.h>

/**
 * Get driver name
 */
const TCHAR *MdsOrbitDriver::getName()
{
   return _T("MDS-ORBIT");
}

/**
 * Get driver version
 */
const TCHAR *MdsOrbitDriver::getVersion()
{
   return NETXMS_VERSION_STRING;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int MdsOrbitDriver::isPotentialDevice(const SNMP_ObjectId& oid)
{
   return oid.startsWith({ 1, 3, 6, 1, 4, 1, 4130, 10 }) ? 255 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool MdsOrbitDriver::isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid)
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
bool MdsOrbitDriver::getHardwareInformation(SNMP_Transport *snmp, NObject *node, DriverData *driverData, DeviceHardwareInfo *hwInfo)
{
   _tcscpy(hwInfo->vendor, _T("GE MDS"));
   _tcscpy(hwInfo->productName, _T("Orbit"));

   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
   request.bindVariable(new SNMP_Variable(_T("1.3.6.1.4.1.4130.10.1.1.1.2.1.0")));  // Serial number
   request.bindVariable(new SNMP_Variable(_T("1.3.6.1.4.1.4130.10.1.1.1.2.7.1.2.1")));  // Version

   SNMP_PDU *response;
   if (snmp->doRequest(&request, &response) == SNMP_ERR_SUCCESS)
   {
      const SNMP_Variable *v = response->getVariable(0);
      if ((v != nullptr) && (v->getType() == ASN_GAUGE32))
      {
         IntegerToString(v->getValueAsUInt(), hwInfo->serialNumber);
      }

      v = response->getVariable(1);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
      {
         TCHAR buffer[256];
         _tcslcpy(hwInfo->productVersion, v->getValueAsString(buffer, 256), 16);
      }

      delete response;
   }

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
GeoLocation MdsOrbitDriver::getGeoLocation(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
   request.bindVariable(new SNMP_Variable(_T("1.3.6.1.4.1.4130.10.3.12.1.2.3.0")));  // mGpsStatusLatitude
   request.bindVariable(new SNMP_Variable(_T("1.3.6.1.4.1.4130.10.3.12.1.2.4.0")));  // mGpsStatusLongitude
   request.bindVariable(new SNMP_Variable(_T("1.3.6.1.4.1.4130.10.3.12.1.2.9.0")));  // mGpsStatusSatellitesUsed

   SNMP_PDU *response;
   if (snmp->doRequest(&request, &response) != SNMP_ERR_SUCCESS)
      return GeoLocation();

   const SNMP_Variable *v = response->getVariable(2);
   if (v->getValueAsUInt() == 0)
   {
      delete response;
      return GeoLocation();
   }

   double lat = 0, lon = 0;

   TCHAR buffer[256];
   v = response->getVariable(0);
   if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
   {
      v->getValueAsString(buffer, 256);
      lat = _tcstod(buffer, nullptr);
   }

   v = response->getVariable(1);
   if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
   {
      v->getValueAsString(buffer, 256);
      lon = _tcstod(buffer, nullptr);
   }

   return (lat != 0) || (lon != 0) ? GeoLocation(GL_GPS, lat, lon, 0, time(nullptr)) : GeoLocation();
}
