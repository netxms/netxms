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
   return oid.equals({1, 3, 6, 1, 4, 1, 10002, 1}) || oid.startsWith({1, 3, 6, 1, 4, 1, 41112}) ? 254 : 0;
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
   if (SnmpGet(snmp->getSnmpVersion(), snmp, _T(".1.3.6.1.4.1.41112.1.6.3.3.0"), nullptr, 0, tmp, sizeof(tmp), 0) == SNMP_ERR_SUCCESS && tmp[0] != 0)
      return true;
   if (SnmpGet(snmp->getSnmpVersion(), snmp, _T(".1.3.6.1.4.1.41112.1.1.1.3.1"), nullptr, 0, tmp, sizeof(tmp), 0) == SNMP_ERR_SUCCESS && (_tcslen(tmp) > 0))
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
   uint32_t radioIndex = name.getElement(name.length() - 1);

   RadioInterfaceInfo *radio = radios->addPlaceholder();
   memset(radio, 0, sizeof(RadioInterfaceInfo));
   radio->index = radioIndex;
   radio->ifIndex = radioIndex;

   // Name (ifDescr)
   TCHAR ifDescr[MAX_OBJECT_NAME];
   if (SnmpGet(snmp->getSnmpVersion(), snmp, {1, 3, 6, 1, 2, 1, 2, 2, 1, 2, radioIndex}, ifDescr, sizeof(ifDescr), SG_STRING_RESULT) == SNMP_ERR_SUCCESS)
      _tcslcpy(radio->name, ifDescr, MAX_OBJECT_NAME);

   // BSSID (ifPhysAddress)
   BYTE macAddr[MAC_ADDR_LENGTH];
   if (SnmpGet(snmp->getSnmpVersion(), snmp, {1, 3, 6, 1, 2, 1, 2, 2, 1, 6, radioIndex}, macAddr, sizeof(macAddr), SG_RAW_RESULT) == SNMP_ERR_SUCCESS)
      memcpy(radio->bssid, macAddr, MAC_ADDR_LENGTH);

   // SSID (from walked variable)
   var->getValueAsString(radio->ssid, MAX_SSID_LENGTH);

   // Channel (per radio index)
   UINT32 channel = 0;
   if (SnmpGetEx(snmp, {1, 3, 6, 1, 4, 1, 41112, 1, 4, 1, 1, 3, radioIndex}, &channel, sizeof(channel), 0) == SNMP_ERR_SUCCESS)
      radio->channel = channel;

   // Frequency (per radio index)
   UINT32 frequency = 0;
   if (SnmpGetEx(snmp, {1, 3, 6, 1, 4, 1, 41112, 1, 4, 1, 1, 4, radioIndex}, &frequency, sizeof(frequency), 0) == SNMP_ERR_SUCCESS)
      radio->frequency = static_cast<uint16_t>(frequency);

   // TX Power (dBm, per radio index)
   UINT32 txPower = 0;
   if (SnmpGetEx(snmp, {1, 3, 6, 1, 4, 1, 41112, 1, 4, 1, 1, 6, radioIndex}, &txPower, sizeof(txPower), 0) == SNMP_ERR_SUCCESS)
   {
      radio->powerDBm = (int32_t)txPower;
      radio->powerMW = (uint32_t)(pow(10.0, (double)radio->powerDBm / 10.0));
   }

   // Band detection from frequency
   if (radio->frequency >= 2400 && radio->frequency < 2500)
      radio->band = RADIO_BAND_2_4_GHZ;
   else if (radio->frequency >= 3600 && radio->frequency < 3700)
      radio->band = RADIO_BAND_3_65_GHZ;
   else if (radio->frequency >= 4900 && radio->frequency < 6000)
      radio->band = RADIO_BAND_5_GHZ;
   else if (radio->frequency >= 5925 && radio->frequency < 7125)
      radio->band = RADIO_BAND_6_GHZ;
   else
      radio->band = RADIO_BAND_UNKNOWN;

   nxlog_debug_tag(DEBUG_TAG_UBNT, 7, _T("[SNMP] Radio index=%u name=\"%s\" SSID=\"%s\" MAC=%02X:%02X:%02X:%02X:%02X:%02X channel=%u freq=%u MHz power=%d dBm (%u mW) band=%d"),
                   radioIndex, radio->name, radio->ssid, radio->bssid[0], radio->bssid[1], radio->bssid[2], radio->bssid[3], radio->bssid[4], radio->bssid[5],
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
   if (SnmpWalk(snmp, _T(".1.2.840.10036.1.1.1.9"), HandlerRadioInterfaceList, radios) != SNMP_ERR_SUCCESS) // dot11DesiredSSID
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
   const SNMP_ObjectId &name = var->getName();

   // MAC starts at element 13
   BYTE mac[MAC_ADDR_LENGTH];
   for (int i = 0; i < MAC_ADDR_LENGTH; i++)
      mac[i] = name.getElement(i + 13);

   WirelessStationInfo *info = new WirelessStationInfo;
   memset(info, 0, sizeof(WirelessStationInfo));
   memcpy(info->macAddr, mac, MAC_ADDR_LENGTH);
   info->apMatchPolicy = AP_MATCH_BY_RFINDEX;

   uint32_t apIndex = name.getElement(name.length() - 7); // element just before MAC
   info->rfIndex = apIndex;
   info->vlan = 1;

   // SSID
   SnmpGet(snmp->getSnmpVersion(), snmp, {1, 3, 6, 1, 4, 1, 41112, 1, 4, 7, 1, 2, apIndex, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]}, info->ssid, sizeof(info->ssid), SG_STRING_RESULT);

   // RSSI
   SnmpGet(snmp->getSnmpVersion(), snmp, {1, 3, 6, 1, 4, 1, 41112, 1, 4, 7, 1, 3, apIndex, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]}, &info->rssi, sizeof(info->rssi), 0);

   // TX rate (kbps)
   uint32_t txRate = 0;
   if (SnmpGet(snmp->getSnmpVersion(), snmp, {1, 3, 6, 1, 4, 1, 41112, 1, 4, 7, 1, 5, apIndex, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]}, &txRate, sizeof(txRate), 0) == SNMP_ERR_SUCCESS)
      info->txRate = txRate;

   // RX rate (kbps)
   uint32_t rxRate = 0;
   if (SnmpGet(snmp->getSnmpVersion(), snmp, {1, 3, 6, 1, 4, 1, 41112, 1, 4, 7, 1, 12, apIndex, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]}, &rxRate, sizeof(rxRate), 0) == SNMP_ERR_SUCCESS)
      info->rxRate = rxRate;

   // IP address
   InetAddress ip;
   if (SnmpGet(snmp->getSnmpVersion(), snmp, {1, 3, 6, 1, 4, 1, 41112, 1, 4, 7, 1, 10, apIndex, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]}, &ip, sizeof(ip), 0) == SNMP_ERR_SUCCESS)
      info->ipAddr = ip;

   wsList->add(info);

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
   if (SnmpWalk(snmp, _T(".1.3.6.1.4.1.41112.1.4.7.1.1"), HandlerWirelessStationList, wsList) != SNMP_ERR_SUCCESS)
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
   TCHAR buf[256];

   // Product name / model
   if (SnmpGet(snmp->getSnmpVersion(), snmp, _T(".1.2.840.10036.3.1.2.1.3.5"), nullptr, 0, buf, sizeof(buf), SG_STRING_RESULT) == SNMP_ERR_SUCCESS && buf[0] != 0)
   {
      _tcslcpy(hwInfo->productCode, buf, 32);
      _tcslcpy(hwInfo->productName, buf, 128);
   }

   // Firmware version
   if (SnmpGet(snmp->getSnmpVersion(), snmp, _T(".1.2.840.10036.3.1.2.1.4.5"), nullptr, 0, buf, sizeof(buf), SG_STRING_RESULT) == SNMP_ERR_SUCCESS && buf[0] != 0)
      _tcslcpy(hwInfo->productVersion, buf, 16);

   // Vendor
   if (SnmpGet(snmp->getSnmpVersion(), snmp, _T(".1.2.840.10036.3.1.2.1.2.5"), nullptr, 0, buf, sizeof(buf), SG_STRING_RESULT) == SNMP_ERR_SUCCESS && buf[0] != 0)
      _tcslcpy(hwInfo->vendor, buf, 128);
   else
      _tcscpy(hwInfo->vendor, _T("Ubiquiti, Inc."));

   // Serial number (base MAC without colons)
   BYTE mac[MAC_ADDR_LENGTH];
   if (SnmpGet(snmp->getSnmpVersion(), snmp, _T(".1.2.840.10036.1.1.1.1.5"), nullptr, 0, mac, sizeof(mac), SG_RAW_RESULT) == SNMP_ERR_SUCCESS)
      BinToStr(mac, MAC_ADDR_LENGTH, hwInfo->serialNumber); // TCHAR-safe
   else
      hwInfo->serialNumber[0] = 0;

   return true;
}
