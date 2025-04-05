/*
** NetXMS - Network Management System
** Drivers for Cambium devices
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
** File: cnpilot.cpp
**
**/

#include "cambium.h"
#include <math.h>

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
int CambiumCnPilotDriver::isPotentialDevice(const SNMP_ObjectId& oid)
{
   return oid.startsWith({ 1, 3, 6, 1, 4, 1, 17713, 22 }) ? 200 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool CambiumCnPilotDriver::isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid)
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
   request.bindVariable(new SNMP_Variable({ 1, 3, 6, 1, 4, 1, 17713, 22, 1, 1, 1, 8, 0 }));  // cambiumAPSWVersion
   request.bindVariable(new SNMP_Variable({ 1, 3, 6, 1, 4, 1, 17713, 22, 1, 1, 1, 4, 0 }));  // cambiumAPSerialNum
   request.bindVariable(new SNMP_Variable({ 1, 3, 6, 1, 4, 1, 17713, 22, 1, 1, 1, 5, 0 }));  // cambiumAPModel

   SNMP_PDU *response;
   if (snmp->doRequest(&request, &response) != SNMP_ERR_SUCCESS)
      return false;

   if (response->getNumVariables() != request.getNumVariables())
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"Malformed device response in CambiumCnPilotDriver::getHardwareInformation");
      delete response;
      return false;
   }

   wcscpy(hwInfo->vendor, L"Cambium");
   response->getVariable(0)->getValueAsString(hwInfo->productVersion, sizeof(hwInfo->productVersion) / sizeof(wchar_t));
   response->getVariable(1)->getValueAsString(hwInfo->serialNumber, sizeof(hwInfo->serialNumber) / sizeof(wchar_t));
   response->getVariable(2)->getValueAsString(hwInfo->productName, sizeof(hwInfo->productName) / sizeof(wchar_t));

   delete response;
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
bool CambiumCnPilotDriver::isWirelessAccessPoint(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   return true;
}

/**
 * Handler for radio interfaces enumeration
 */
static uint32_t HandlerRadioInterfaces(SNMP_Variable *var, SNMP_Transport *snmp, StructArray<RadioInterfaceInfo> *radios)
{
   SNMP_ObjectId oid = var->getName();
   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());

   uint32_t radioIndex = oid.getLastElement();

   oid.changeElement(11, 6);  // cambiumRadioChannel
   request.bindVariable(new SNMP_Variable(oid));

   oid.changeElement(11, 8);  // cambiumRadioTransmitPower
   request.bindVariable(new SNMP_Variable(oid));

   oid.changeElement(11, 2);  // cambiumRadioMACAddress
   request.bindVariable(new SNMP_Variable(oid));

   oid.changeElement(11, 3);  // cambiumRadioBandType
   request.bindVariable(new SNMP_Variable(oid));

   oid.changeElement(9, 4);  // cambiumWlanSsid
   oid.changeElement(10, 1);
   oid.changeElement(11, 2);
   // Not sure about this - but old code seemed to work with index in cambiumWlanTable (cambiumWlanIndex) being cambiumRadioWlan - 1
   // (so cambiumWlanTable is 0-based while cambiumRadioWlan is 1-based)
   oid.changeElement(12, var->getValueAsUInt() - 1);
   request.bindVariable(new SNMP_Variable(oid));

   SNMP_PDU *response;
   uint32_t rcc = snmp->doRequest(&request, &response);
   if (rcc != SNMP_ERR_SUCCESS)
      return rcc;

   if (response->getNumVariables() != request.getNumVariables())
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"Malformed device response in CambiumCnPilotDriver::getRadioInterfaces");
      delete response;
      return SNMP_ERR_SUCCESS;
   }

   RadioInterfaceInfo *ri = radios->addPlaceholder();
   memset(ri, 0, sizeof(RadioInterfaceInfo));
   ri->index = radioIndex;
   wchar_t macAddrText[64] = L"";
   memcpy(ri->bssid, MacAddress::parse(response->getVariable(2)->getValueAsString(macAddrText, 64)).value(), MAC_ADDR_LENGTH);
   _sntprintf(ri->name, MAX_OBJECT_NAME, L"radio%u", radioIndex);
   response->getVariable(4)->getValueAsString(ri->ssid, MAX_SSID_LENGTH);

   wchar_t band[32] = L"";
   response->getVariable(3)->getValueAsString(band, 32);
   if (!wcsicmp(band, L"2.4GHz"))
      ri->band = RADIO_BAND_2_4_GHZ;
   if (!wcsicmp(band, L"5GHz"))
      ri->band = RADIO_BAND_2_4_GHZ;
   else
      ri->band = RADIO_BAND_UNKNOWN;
   ri->channel = static_cast<uint16_t>(wcstol(response->getVariable(0)->getValueAsString(macAddrText, 64), nullptr, 10));
   ri->frequency = WirelessChannelToFrequency(ri->band, ri->channel);
   ri->powerDBm = response->getVariable(1)->getValueAsInt();
   ri->powerMW = (int)pow(10.0, (double)ri->powerDBm / 10.0);

   delete response;
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
StructArray<RadioInterfaceInfo> *CambiumCnPilotDriver::getRadioInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   auto radios = new StructArray<RadioInterfaceInfo>(0, 8);
   if (SnmpWalk(snmp, { 1, 3, 6, 1, 4, 1, 17713, 22, 1, 2, 1, 4 }, HandlerRadioInterfaces, radios) != SNMP_ERR_SUCCESS)  // cambiumRadioWlan
   {
      delete radios;
      return nullptr;
   }
   return radios;
}

/**
 * Handler for wireless station enumeration
 */
static uint32_t HandlerWirelessStationList(SNMP_Variable *var, SNMP_Transport *snmp, ObjectArray<WirelessStationInfo> *wsList)
{
   SNMP_ObjectId oid = var->getName();
   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());

   oid.changeElement(11, 3);  // cambiumClientIPAddress
   request.bindVariable(new SNMP_Variable(oid));

   oid.changeElement(11, 8);  // cambiumClientRadioIndex
   request.bindVariable(new SNMP_Variable(oid));

   SNMP_PDU *response;
   uint32_t rcc = snmp->doRequest(&request, &response);
   if (rcc != SNMP_ERR_SUCCESS)
      return rcc;

   if (response->getNumVariables() != request.getNumVariables())
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"Malformed device response in CambiumEPMPDriver::getWirelessStations");
      delete response;
      return SNMP_ERR_SUCCESS;
   }

   auto info = new WirelessStationInfo;
   memset(info, 0, sizeof(WirelessStationInfo));

   wchar_t addrText[64];
   memcpy(info->macAddr, MacAddress::parse(var->getValueAsString(addrText, 64)).value(), MAC_ADDR_LENGTH);
   info->ipAddr = InetAddress::parse(response->getVariable(0)->getValueAsString(addrText, 64)).getAddressV4();
   info->rfIndex = response->getVariable(1)->getValueAsInt();
   info->apMatchPolicy = AP_MATCH_BY_RFINDEX;
   wsList->add(info);

   delete response;
   return SNMP_ERR_SUCCESS;
}

/**
 * Get list of associated wireless stations. Default implementation always return NULL.
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @return list of associated wireless stations
 */
ObjectArray<WirelessStationInfo> *CambiumCnPilotDriver::getWirelessStations(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   ObjectArray<WirelessStationInfo> *wsList = new ObjectArray<WirelessStationInfo>(0, 16, Ownership::True);
   if (SnmpWalk(snmp, { 1, 3, 6, 1, 4, 1, 17713, 22, 1, 3, 1, 2 }, HandlerWirelessStationList, wsList) != SNMP_ERR_SUCCESS)  // cambiumClientMACAddress
   {
      delete wsList;
      wsList = nullptr;
   }
   return wsList;
}
