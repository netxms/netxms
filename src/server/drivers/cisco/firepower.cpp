/*
** NetXMS - Network Management System
** Driver for Cisco Firepower NGFW devices
** Copyright (C) 2003-2026 Victor Kirhenshtein
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
** File: firepower.cpp
**/

#include "cisco.h"
#include <netxms-version.h>

#define DEBUG_TAG DEBUG_TAG_CISCO _T(".firepower")

/**
 * Get driver name
 */
const TCHAR *CiscoFirepowerDriver::getName()
{
   return _T("CISCO-FIREPOWER");
}

/**
 * Get driver version
 */
const TCHAR *CiscoFirepowerDriver::getVersion()
{
   return NETXMS_VERSION_STRING;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int CiscoFirepowerDriver::isPotentialDevice(const SNMP_ObjectId& oid)
{
   return oid.startsWith({ 1, 3, 6, 1, 4, 1, 9, 1 }) ? 200 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param context device context
 * @param oid Device OID
 */
bool CiscoFirepowerDriver::isDeviceSupported(DeviceContext *context, const SNMP_ObjectId& oid)
{
   SNMP_Transport *snmp = context->getSNMPTransport();
   wchar_t buffer[1024];
   if (SnmpGetEx(snmp, L".1.3.6.1.2.1.1.1.0", nullptr, 0, buffer, sizeof(buffer), SG_STRING_RESULT, nullptr) != SNMP_ERR_SUCCESS)
      return false;

   nxlog_debug_tag(DEBUG_TAG, 5, L"sysDescr=\"%s\"", buffer);
   return _tcsnicmp(buffer, L"Cisco Firepower", 15) == 0;
}

/**
 * Get hardware information from device.
 *
 * @param context device context
 * @param node Node
 * @param driverData driver data
 * @param hwInfo pointer to hardware information structure to fill
 * @return true if hardware information is available
 */
bool CiscoFirepowerDriver::getHardwareInformation(DeviceContext *context, NObject *node, DriverData *driverData, DeviceHardwareInfo *hwInfo)
{
   SNMP_Transport *snmp = context->getSNMPTransport();
   wcscpy(hwInfo->vendor, L"Cisco");

   wchar_t sysDescr[1024];
   if (SnmpGetEx(snmp, L".1.3.6.1.2.1.1.1.0", nullptr, 0, sysDescr, sizeof(sysDescr), SG_STRING_RESULT, nullptr) == SNMP_ERR_SUCCESS)
   {
      // Typical sysDescr values:
      // "Cisco Firepower Threat Defense, Version 7.2.5, Build 208"
      // "Cisco FirePOWER FPR-2140 Threat Defense, Version 7.0.1"
      // Extract product name - everything before ", Version"
      wchar_t *versionStart = wcsistr(sysDescr, L", Version");
      if (versionStart != nullptr)
      {
         *versionStart = 0;
         wcslcpy(hwInfo->productName, sysDescr, 128);

         // Extract version string
         const wchar_t *version = versionStart + 10; // skip ", Version "
         const wchar_t *versionEnd = wcschr(version, L',');
         if (versionEnd != nullptr)
         {
            size_t len = std::min(static_cast<size_t>(versionEnd - version), static_cast<size_t>(15));
            wcsncpy(hwInfo->productVersion, version, len);
            hwInfo->productVersion[len] = 0;
         }
         else
         {
            wcslcpy(hwInfo->productVersion, version, 16);
         }
      }
      else
      {
         wcslcpy(hwInfo->productName, sysDescr, 128);
      }
   }

   // Try ENTITY-MIB entPhysicalSerialNum.1 for serial number
   SnmpGetEx(snmp, L".1.3.6.1.2.1.47.1.1.1.1.11.1", nullptr, 0, hwInfo->serialNumber, sizeof(hwInfo->serialNumber), SG_STRING_RESULT, nullptr);
   return true;
}
