/*
** NetXMS - Network Management System
** Drivers for Fortinet devices
** Copyright (C) 2023-2026 Raden Solutions
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
** File: fortiswitch.cpp
**
**/

#include "fortinet.h"

/**
 * Get driver name
 */
const TCHAR *FortiSwitchDriver::getName()
{
   return _T("FORTISWITCH");
}

/**
 * Get driver version
 */
const TCHAR *FortiSwitchDriver::getVersion()
{
   return NETXMS_VERSION_STRING;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int FortiSwitchDriver::isPotentialDevice(const SNMP_ObjectId& oid)
{
   return oid.startsWith({ 1, 3, 6, 1, 4, 1, 12356, 106 }) ? 250 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool FortiSwitchDriver::isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid)
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
bool FortiSwitchDriver::getHardwareInformation(SNMP_Transport *snmp, NObject *node, DriverData *driverData, DeviceHardwareInfo *hwInfo)
{
   _tcscpy(hwInfo->vendor, _T("Fortinet"));

   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.2.1.47.1.1.1.1.13.1")));  // first entry in entPhysicalModelName
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.12356.106.1.1.1.0")));  // fsSysSerial
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.12356.106.4.1.1.0")));  // fsSysVersion

   SNMP_PDU *response;
   if (snmp->doRequest(&request, &response) == SNMP_ERR_SUCCESS)
   {
      TCHAR buffer[256];

      const SNMP_Variable *v = response->getVariable(0);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
      {
         _tcslcpy(hwInfo->productName, v->getValueAsString(buffer, 256), 128);
         for(int i = 0; hwInfo->productName[i] != 0; i++)
         {
            if (hwInfo->productName[i] == '_')
               hwInfo->productName[i] = '-';
         }
      }

      v = response->getVariable(1);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
      {
         _tcslcpy(hwInfo->serialNumber, v->getValueAsString(buffer, 256), 128);
      }

      v = response->getVariable(2);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
      {
         _tcslcpy(hwInfo->productVersion, v->getValueAsString(buffer, 256), 16);
      }

      delete response;
   }
   return true;
}

/**
 * Returns true if FDB uses ifIndex instead of bridge port number for referencing interfaces.
 * FortiSwitch reports interface indexes in dot1qTpFdbPort / dot1dTpFdbPort instead of dot1dBasePort numbers.
 *
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @return true if FDB uses ifIndex instead of bridge port number for referencing interfaces
 */
bool FortiSwitchDriver::isFdbUsingIfIndex(const NObject *node, DriverData *driverData)
{
   return true;
}

/**
 * Returns true if lldpRemLocalPortNum in LLDP remote table uses ifIndex instead of bridge port number.
 * FortiSwitch reports interface indexes in lldpRemLocalPortNum.
 *
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @return true if lldpRemLocalPortNum uses ifIndex instead of bridge port number
 */
bool FortiSwitchDriver::isLldpRemTableUsingIfIndex(const NObject *node, DriverData *driverData)
{
   return true;
}
