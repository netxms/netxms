/*
** NetXMS - Network Management System
** Drivers for Ubiquiti Networks devices
** Copyright (C) 2003-2025 Victor Kirhenshtein
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
** File: airmax.cpp
**/

#include "ubnt.h"
#include <netxms-version.h>
#include <math.h>

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
int UbiquitiAirMaxDriver::isPotentialDevice(const SNMP_ObjectId &oid)
{
   return oid.equals({ 1, 3, 6, 1, 4, 1, 10002, 1 }) || oid.startsWith({ 1, 3, 6, 1, 4, 1, 41112, 1, 4 }) ? 254 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool UbiquitiAirMaxDriver::isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId &oid)
{
   TCHAR tmp[256];
   if ((SnmpGet(snmp->getSnmpVersion(), snmp, { 1, 3, 6, 1, 4, 1, 41112, 1, 6, 3, 3, 0 }, tmp, sizeof(tmp), SG_STRING_RESULT) == SNMP_ERR_SUCCESS) && (tmp[0] != 0))
      return true;
   if ((SnmpGet(snmp->getSnmpVersion(), snmp, { 1, 3, 6, 1, 4, 1, 41112, 1, 1, 1, 3, 1 }, tmp, sizeof(tmp), SG_STRING_RESULT) == SNMP_ERR_SUCCESS) && (tmp[0] != 0))
      return true;
   return false;
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
   const SNMP_ObjectId &name = var->getName();

   RadioInterfaceInfo *radio = radios->addPlaceholder();
   memset(radio, 0, sizeof(RadioInterfaceInfo));
   radio->index = name.getElement(name.length() - 1);
   radio->ifIndex = var->getValueAsUInt();

   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
   SNMP_ObjectId oid({ 1, 3, 6, 1, 2, 1, 2, 2, 1, 2 });

   oid.extend(radio->ifIndex);
   request.bindVariable(new SNMP_Variable(oid)); // Name (ifDescr)

   oid.changeElement(9, 6);
   request.bindVariable(new SNMP_Variable(oid)); // BSSID (ifPhysAddress)

   oid = { 1, 2, 840, 10036, 1, 1, 1, 9 }; // dot11DesiredSSID
   oid.extend(radio->ifIndex);
   request.bindVariable(new SNMP_Variable(oid));

   oid = { 1, 3, 6, 1, 4, 1, 41112, 1, 4, 1, 1, 4 }; // ubntRadioFreq
   oid.extend(radio->index);
   request.bindVariable(new SNMP_Variable(oid));

   oid.changeElement(11, 6); // ubntRadioTxPower
   request.bindVariable(new SNMP_Variable(oid));

   SNMP_PDU *response = nullptr;
   if (snmp->doRequest(&request, &response) == SNMP_ERR_SUCCESS)
   {
      if (request.getNumVariables() == response->getNumVariables())
      {
         // Name (ifDescr)
         SNMP_Variable *var = response->getVariable(0);
         if (var->getType() == ASN_OCTET_STRING)
            var->getValueAsString(radio->name, MAX_OBJECT_NAME);

         // BSSID (ifPhysAddress)
         var = response->getVariable(1);
         if (var->getType() == ASN_OCTET_STRING && var->getValueLength() >= MAC_ADDR_LENGTH)
            var->getRawValue(radio->bssid, MAC_ADDR_LENGTH);

         // SSID
         var = response->getVariable(2);
         var->getValueAsString(radio->ssid, MAX_SSID_LENGTH);

         // Frequency, Channel and Band (per radio index)
         var = response->getVariable(3);
         radio->frequency = static_cast<uint16_t>(var->getValueAsUInt());
         radio->channel = WirelessFrequencyToChannel(radio->frequency);
         radio->band = WirelessFrequencyToBand(radio->frequency);

         // TX Power (dBm)
         var = response->getVariable(4);
         radio->powerDBm = var->getValueAsInt();
         radio->powerMW = static_cast<int32_t>(pow(10.0, static_cast<double>(radio->powerDBm) / 10.0));
      }
      delete response;
   }

   nxlog_debug_tag(DEBUG_TAG_UBNT, 7, _T("[SNMP] radioIndex=%u ifIndex=%u name=\"%s\" SSID=\"%s\" BSSID=%02X:%02X:%02X:%02X:%02X:%02X channel=%u freq=%u MHz power=%d dBm (%u mW) band=%d"),
                   radio->index, radio->ifIndex, radio->name, radio->ssid, radio->bssid[0], radio->bssid[1], radio->bssid[2], radio->bssid[3], radio->bssid[4], radio->bssid[5],
                   radio->channel, radio->frequency, radio->powerDBm, radio->powerMW, radio->band);

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
   if (SnmpWalk(snmp, { 1, 3, 6, 1, 4, 1, 41112, 1, 4, 1, 1, 1 }, HandlerRadioInterfaceList, radios) != SNMP_ERR_SUCCESS) // ubntRadioIndex
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
   SNMP_ObjectId oid = var->getName();

   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());

   oid.changeElement(11, 3); // RSSI
   request.bindVariable(new SNMP_Variable(oid));

   oid.changeElement(11, 11); // TX rate
   request.bindVariable(new SNMP_Variable(oid));

   oid.changeElement(11, 12); // RX rate
   request.bindVariable(new SNMP_Variable(oid));

   oid.changeElement(11, 10); // IP address
   request.bindVariable(new SNMP_Variable(oid));

   SNMP_PDU *response = nullptr;
   if (snmp->doRequest(&request, &response) != SNMP_ERR_SUCCESS)
      return SNMP_ERR_AGENT;

   if (request.getNumVariables() != response->getNumVariables())
   {
      nxlog_debug_tag(DEBUG_TAG_UBNT, 5, _T("[SNMP] Invalid response from Ubiquiti AirMax device - number of variables in request (%d) does not match number of variables in response (%d)"),
                      request.getNumVariables(), response->getNumVariables());
      delete response;
      return SNMP_ERR_AGENT;
   }

   // MAC starts at element 13
   BYTE mac[MAC_ADDR_LENGTH];
   for (int i = 0; i < MAC_ADDR_LENGTH; i++)
      mac[i] = oid.getElement(i + 13);

   WirelessStationInfo *info = new WirelessStationInfo;
   memset(info, 0, sizeof(WirelessStationInfo));
   memcpy(info->macAddr, mac, MAC_ADDR_LENGTH);
   info->apMatchPolicy = AP_MATCH_BY_RFINDEX;
   info->vlan = 1;

   // Radio index is at element 12 just before MAC
   uint32_t radioIndex = oid.getElement(oid.length() - 7);
   info->rfIndex = radioIndex;

   info->rssi = response->getVariable(0)->getValueAsInt();    // RSSI
   info->txRate = response->getVariable(1)->getValueAsUInt(); // TX rate (kbps)
   info->rxRate = response->getVariable(2)->getValueAsUInt(); // RX rate (kbps)

   wchar_t ipAddr[64];
   info->ipAddr = InetAddress::parse(response->getVariable(3)->getValueAsIPAddr(ipAddr));

   wsList->add(info);
   delete response;

   nxlog_debug_tag(DEBUG_TAG_UBNT, 7, _T("[SNMP] Station MAC=%02X:%02X:%02X:%02X:%02X:%02X SSID=\"%s\" RSSI=%d dBm TX=%u RX=%u IP=%s (rfIndex=%u)"),
                   mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], info->ssid, info->rssi, info->txRate, info->rxRate, (const TCHAR *)info->ipAddr.toString(), info->rfIndex);

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
   if (SnmpWalk(snmp, { 1, 3, 6, 1, 4, 1, 41112, 1, 4, 7, 1, 1 }, HandlerWirelessStationList, wsList) != SNMP_ERR_SUCCESS)
   {
      delete wsList;
      wsList = nullptr;
   }
   return wsList;
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
bool UbiquitiAirMaxDriver::getHardwareInformation(SNMP_Transport *snmp, NObject *, DriverData *, DeviceHardwareInfo *hwInfo)
{
   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());

   // Product name / model
   request.bindVariable(new SNMP_Variable({ 1, 2, 840, 10036, 3, 1, 2, 1, 3, 5 }));

   // Firmware version  
   request.bindVariable(new SNMP_Variable({ 1, 2, 840, 10036, 3, 1, 2, 1, 4, 5 }));

   // Vendor
   request.bindVariable(new SNMP_Variable({ 1, 2, 840, 10036, 3, 1, 2, 1, 2, 5 }));

   // Serial number (base MAC without colons)
   request.bindVariable(new SNMP_Variable({ 1, 2, 840, 10036, 1, 1, 1, 1, 5 }));

   SNMP_PDU *response = nullptr;
   if (snmp->doRequest(&request, &response) != SNMP_ERR_SUCCESS)
      return false;

   if (request.getNumVariables() != response->getNumVariables())
   {
      delete response;
      return false;
   }

   SNMP_Variable *var = response->getVariable(0);
   if (var->getType() == ASN_OCTET_STRING)
   {
      wchar_t buf[256];
      var->getValueAsString(buf, sizeof(buf));
      if (buf[0] != 0)
      {
         wcslcpy(hwInfo->productCode, buf, 32);
         wcslcpy(hwInfo->productName, buf, 128);
      }
   }

   // Firmware version
   var = response->getVariable(1);
   var->getValueAsString(hwInfo->productVersion, 16);

   // Vendor
   var = response->getVariable(2);
   var->getValueAsString(hwInfo->vendor, 128);
   if (hwInfo->vendor[0] == 0)
      _tcscpy(hwInfo->vendor, _T("Ubiquiti, Inc."));

   // Serial number (base MAC without colons)
   var = response->getVariable(3);
   if (var->getType() == ASN_OCTET_STRING && var->getValueLength() >= MAC_ADDR_LENGTH)
   {
      BYTE mac[MAC_ADDR_LENGTH];
      var->getRawValue(mac, MAC_ADDR_LENGTH);
      BinToStr(mac, MAC_ADDR_LENGTH, hwInfo->serialNumber); // TCHAR-safe
   }
   else
   {
      hwInfo->serialNumber[0] = 0;
   }

   delete response;
   return true;
}
