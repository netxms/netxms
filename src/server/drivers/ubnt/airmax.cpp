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
** File: ubnt.cpp
**/

#include "ubnt.h"
#include <netxms-version.h>

/**
 * Get driver name
 */
const TCHAR *UbiquitiAirMaxDriver::getName()
{
	return _T("UBNT-AIRMAX");
}

/**
 * Get driver version
 */
const TCHAR *UbiquitiAirMaxDriver::getVersion()
{
	return NETXMS_VERSION_STRING;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int UbiquitiAirMaxDriver::isPotentialDevice(const TCHAR *oid)
{
	return (!_tcscmp(oid, _T(".1.3.6.1.4.1.10002.1")) || !_tcsncmp(oid, _T(".1.3.6.1.4.1.41112."), 19)) ? 254 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool UbiquitiAirMaxDriver::isDeviceSupported(SNMP_Transport *snmp, const TCHAR *oid)
{
	return true;
}

/**
 * Returns true if device is a standalone wireless access point (not managed by a controller). Default implementation always return false.
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @return true if device is a standalone wireless access point
 */
bool UbiquitiAirMaxDriver::isWirelessAccessPoint(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   return true;
}

/**
 * Handler for access point enumeration
 */
static uint32_t HandlerRadioInterfaceList(SNMP_Variable *var, SNMP_Transport *snmp, StructArray<RadioInterfaceInfo> *radios)
{
   const SNMP_ObjectId& name = var->getName();
   size_t nameLen = name.length();
   uint32_t oid[MAX_OID_LEN];
   memcpy(oid, name.value(), nameLen * sizeof(UINT32));
   uint32_t radioIndex = oid[nameLen - 1];

   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());

   SnmpParseOID(_T(".1.3.6.1.2.1.2.2.1.2"), oid, MAX_OID_LEN); // ifDescr
   oid[10] = radioIndex;
   request.bindVariable(new SNMP_Variable(oid, 11));

   oid[9] = 6; // ifPhysAddress
   request.bindVariable(new SNMP_Variable(oid, 11));

   SNMP_PDU *response;
   uint32_t rcc = snmp->doRequest(&request, &response);
   if (rcc == SNMP_ERR_SUCCESS)
   {
      if (response->getNumVariables() == 3)
      {
         RadioInterfaceInfo *radio = radios->addPlaceholder();
         memset(radio, 0, sizeof(RadioInterfaceInfo));
         radio->index = radioIndex;
         radio->ifIndex = radioIndex;
         response->getVariable(0)->getValueAsString(radio->name, MAX_OBJECT_NAME);
         response->getVariable(1)->getRawValue(radio->bssid, MAC_ADDR_LENGTH);
         var->getValueAsString(radio->ssid, MAX_SSID_LENGTH);
      }
      delete response;
   }

   return SNMP_ERR_SUCCESS;
}

/**
 * Get list of radio interfaces for standalone access point. Default implementation always return NULL.
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @return list of radio interfaces for standalone access point
 */
StructArray<RadioInterfaceInfo> *UbiquitiAirMaxDriver::getRadioInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   auto radios = new StructArray<RadioInterfaceInfo>(0, 4);
   if (SnmpWalk(snmp, _T(".1.2.840.10036.1.1.1.9"), HandlerRadioInterfaceList, radios) != SNMP_ERR_SUCCESS)  // dot11DesiredSSID
   {
      delete radios;
      return nullptr;
   }
   return radios;
}

/**
 * Handler for mobile units enumeration
 */
static uint32_t HandlerWirelessStationList(SNMP_Variable *var, SNMP_Transport *snmp, ObjectArray<WirelessStationInfo> *wsList)
{
   const SNMP_ObjectId& name = var->getName();
   uint32_t apIndex = name.getElement(name.length() - 1);

   WirelessStationInfo *info = new WirelessStationInfo;
   memset(info, 0, sizeof(WirelessStationInfo));
   for(int i = 0; i < MAC_ADDR_LENGTH; i++)
      info->macAddr[i] = name.getElement(i + 13);
   info->ipAddr = InetAddress();
   info->vlan = 1;
   info->signalStrength = var->getValueAsInt();
   info->rfIndex = apIndex;
   info->apMatchPolicy = AP_MATCH_BY_RFINDEX;

   TCHAR oid[256];
   _sntprintf(oid, 256, _T(".1.2.840.10036.1.1.1.9.%u"), apIndex);
   if (SnmpGet(snmp->getSnmpVersion(), snmp, oid, NULL, 0, info->ssid, sizeof(info->ssid), SG_STRING_RESULT) != SNMP_ERR_SUCCESS)
   {
      info->ssid[0] = 0;
   }

   wsList->add(info);
   return SNMP_ERR_SUCCESS;
}

/**
 * Get registered wireless stations (clients)
 *
 * @param snmp SNMP transport
 * @param attributes Node custom attributes
 * @param driverData optional pointer to user data
 */
ObjectArray<WirelessStationInfo> *UbiquitiAirMaxDriver::getWirelessStations(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   ObjectArray<WirelessStationInfo> *wsList = new ObjectArray<WirelessStationInfo>(0, 16, Ownership::True);
   if (SnmpWalk(snmp, _T(".1.3.6.1.4.1.14988.1.1.1.2.1.3"), // mtxrWlRtabStrength
                HandlerWirelessStationList, wsList) != SNMP_ERR_SUCCESS)
   {
      delete wsList;
      wsList = nullptr;
   }
   return wsList;
}
