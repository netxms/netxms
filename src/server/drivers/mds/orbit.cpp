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
#include <math.h>
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
   request.bindVariable(new SNMP_Variable(_T("1.3.6.1.4.1.4130.10.1.1.1.2.2.0")));  // Serial number
   request.bindVariable(new SNMP_Variable(_T("1.3.6.1.4.1.4130.10.1.1.1.2.3.0")));  // mSysProductConfiguration
   request.bindVariable(new SNMP_Variable(_T("1.3.6.1.4.1.4130.10.1.1.1.2.7.1.3.1")));  // mSysActive.1
   request.bindVariable(new SNMP_Variable(_T("1.3.6.1.4.1.4130.10.1.1.1.2.7.1.2.1")));  // mSysVersion.1
   request.bindVariable(new SNMP_Variable(_T("1.3.6.1.4.1.4130.10.1.1.1.2.7.1.2.2")));  // mSysVersion.2

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
         _tcslcpy(hwInfo->productCode, v->getValueAsString(buffer, 256), 32);
      }

      int activeSw = 0;
      v = response->getVariable(2);
      if (v != nullptr)
      {
         activeSw = (v->getValueAsInt() == 1) ? 3 : ((v->getValueAsInt() == 2) ? 4 : 0);
      }

      if (activeSw != 0)
      {
         v = response->getVariable(activeSw);
         if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
         {
            TCHAR buffer[256];
            _tcslcpy(hwInfo->productVersion, v->getValueAsString(buffer, 256), 16);
         }
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

/**
 * Returns true if device is a standalone wireless access point (not managed by a controller). Default implementation always return false.
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @return true if device is a standalone wireless access point
 */
bool MdsOrbitDriver::isWirelessAccessPoint(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   bool isAP = false;
   if (SnmpWalk(snmp, { 1, 3, 6, 1, 4, 1, 4130, 10, 2, 5, 1, 2, 1, 1, 3 },
      [&isAP] (SNMP_Variable *v) -> uint32_t
      {
         if (v->getValueAsInt() == 1)
            isAP = true;
         return SNMP_ERR_SUCCESS;
      }) != SNMP_ERR_SUCCESS)
   {
      return false;
   }

   if (!isAP)
      isAP = SnmpWalkCount(snmp, { 1, 3, 6, 1, 4, 1, 4130, 10, 2, 2, 1, 2, 2, 1, 1 }) > 0;

   return isAP;
}

/**
 * Add radio interface
 */
static void AddLnRadioInterface(StructArray<RadioInterfaceInfo> *rifList, SNMP_Transport *snmp, uint32_t ifIndex)
{
   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
   request.bindVariable(new SNMP_Variable({ 1, 3, 6, 1, 2, 1, 31, 1, 1, 1, 1, ifIndex }));  // ifName
   request.bindVariable(new SNMP_Variable({ 1, 3, 6, 1, 2, 1, 2, 2, 1, 6, ifIndex }));  // ifPhysAddress
   request.bindVariable(new SNMP_Variable({ 1, 3, 6, 1, 4, 1, 4130, 10, 2, 5, 1, 2, 1, 1, 25, ifIndex }));  // mIfLnActiveTxFrequency

   SNMP_PDU *response;
   if (snmp->doRequest(&request, &response) != SNMP_ERR_SUCCESS)
      return;

   if (response->getNumVariables() == 3)
   {
      MacAddress bssid = response->getVariable(1)->getValueAsMACAddr();

      TCHAR frequency[64];
      response->getVariable(2)->getValueAsString(frequency, 64);

      RadioInterfaceInfo *radio = rifList->addPlaceholder();
      memset(radio, 0, sizeof(RadioInterfaceInfo));
      response->getVariable(0)->getValueAsString(radio->name, 64);
      radio->index = ifIndex;
      radio->ifIndex = ifIndex;
      memcpy(radio->bssid, bssid.value(), MAC_ADDR_LENGTH);
      radio->frequency = static_cast<uint16_t>(_tcstod(frequency, nullptr));
      radio->band = WirelessFrequencyToBand(radio->frequency);
      radio->channel = WirelessFrequencyToChannel(radio->frequency);

      TCHAR bssidText[64];
      nxlog_debug_tag(MDS_DEBUG_TAG, 6, _T("LnRadio: index=%u name=%s bssid=%s"), ifIndex, radio->name, bssid.toString(bssidText));
   }
   delete response;
}

/**
 * Add WiFi radio interface
 */
static void AddWiFiRadioInterface(StructArray<RadioInterfaceInfo> *rifList, SNMP_Transport *snmp, uint32_t ifIndex)
{
   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
   request.bindVariable(new SNMP_Variable({ 1, 3, 6, 1, 2, 1, 31, 1, 1, 1, 1, ifIndex }));  // ifName
   request.bindVariable(new SNMP_Variable({ 1, 3, 6, 1, 4, 1, 4130, 10, 2, 2, 1, 2, 1, 1, 6, ifIndex }));  // mIfDot11StationBssid
   request.bindVariable(new SNMP_Variable({ 1, 3, 6, 1, 4, 1, 4130, 10, 2, 2, 1, 2, 1, 1, 5, ifIndex }));  // mIfDot11StationSsid
   request.bindVariable(new SNMP_Variable({ 1, 3, 6, 1, 4, 1, 4130, 10, 2, 2, 1, 2, 1, 1, 4, ifIndex }));  // mIfDot11Channel
   request.bindVariable(new SNMP_Variable({ 1, 3, 6, 1, 4, 1, 4130, 10, 2, 2, 1, 2, 1, 1, 3, ifIndex }));  // mIfDot11TxPower

   SNMP_PDU *response;
   if (snmp->doRequest(&request, &response) != SNMP_ERR_SUCCESS)
      return;

   if (response->getNumVariables() == 3)
   {
      MacAddress bssid = response->getVariable(1)->getValueAsMACAddr();

      RadioInterfaceInfo *radio = rifList->addPlaceholder();
      memset(radio, 0, sizeof(RadioInterfaceInfo));
      response->getVariable(0)->getValueAsString(radio->name, 64);
      radio->index = ifIndex;
      radio->ifIndex = ifIndex;
      memcpy(radio->bssid, bssid.value(), MAC_ADDR_LENGTH);
      radio->channel = static_cast<uint16_t>(response->getVariable(3)->getValueAsUInt());
      radio->band = RADIO_BAND_2_4_GHZ;
      radio->frequency = WirelessChannelToFrequency(radio->band, radio->channel);
      radio->powerDBm = response->getVariable(4)->getValueAsInt();
      radio->powerMW = static_cast<int32_t>(pow(10.0, (double)radio->powerDBm / 10.0));

      TCHAR bssidText[64];
      nxlog_debug_tag(MDS_DEBUG_TAG, 6, _T("LnRadio: index=%u name=%s bssid=%s"), ifIndex, radio->name, bssid.toString(bssidText));
   }
   delete response;
}

/**
 * Get list of radio interfaces for standalone access point. Default implementation always return NULL.
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @return list of radio interfaces for standalone access point
 */
StructArray<RadioInterfaceInfo> *MdsOrbitDriver::getRadioInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   auto rifList = new StructArray<RadioInterfaceInfo>(0, 4);
   if (SnmpWalk(snmp, { 1, 3, 6, 1, 4, 1, 4130, 10, 2, 5, 1, 2, 1, 1, 3 }, // mIfLnCurrentDeviceMode
      [rifList, snmp] (SNMP_Variable *v) -> uint32_t
      {
         if (v->getValueAsInt() == 1)
            AddLnRadioInterface(rifList, snmp, v->getName().getLastElement());
         return SNMP_ERR_SUCCESS;
      }) != SNMP_ERR_SUCCESS)
   {
      delete rifList;
      return nullptr;
   }
   if (SnmpWalk(snmp, { 1, 3, 6, 1, 4, 1, 4130, 10, 2, 2, 1, 2, 1, 1, 2 }, // mIfDot11Mode
      [rifList, snmp] (SNMP_Variable *v) -> uint32_t
      {
         int mode = v->getValueAsInt();
         if ((mode == 2) || (mode == 3))  // accessPoint(2) or accessPointStation(3)
            AddWiFiRadioInterface(rifList, snmp, v->getName().getLastElement());
         return SNMP_ERR_SUCCESS;
      }) != SNMP_ERR_SUCCESS)
   {
      delete rifList;
      return nullptr;
   }
   return rifList;
}

/**
 * Add radio client
 */
static void AddLnRadioClient(SNMP_Transport *snmp, SNMP_Variable *v, ObjectArray<WirelessStationInfo> *wsList)
{
   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());

   SNMP_ObjectId oid(v->getName());
   oid.changeElement(14, 2);  // mIfLnStatusConnRemIpAddress
   request.bindVariable(new SNMP_Variable(oid));

   SNMP_PDU *response;
   if (snmp->doRequest(&request, &response) != SNMP_ERR_SUCCESS)
      return;

   WirelessStationInfo *info = new WirelessStationInfo;
   memset(info, 0, sizeof(WirelessStationInfo));
   info->apMatchPolicy = AP_MATCH_BY_RFINDEX;
   info->rfIndex = oid.getElement(15);
   info->rssi = v->getValueAsInt();

   for(size_t i = 0; i < 6; i++)
      info->macAddr[i] = oid.getElement(i + 16);

   uint32_t ipAddress;
   if (response->getVariable(0)->getRawValue((BYTE*)&ipAddress, 4) == 4)
      info->ipAddr = InetAddress(ntohl(ipAddress));

   wsList->add(info);
   delete response;
}

/**
 * Add radio client
 */
static void AddWiFiRadioClient(SNMP_Transport *snmp, SNMP_Variable *v, ObjectArray<WirelessStationInfo> *wsList)
{
   SNMP_ObjectId oid(v->getName());

   WirelessStationInfo *info = new WirelessStationInfo;
   memset(info, 0, sizeof(WirelessStationInfo));
   info->apMatchPolicy = AP_MATCH_BY_RFINDEX;
   info->rfIndex = oid.getElement(15);
   info->rssi = v->getValueAsInt();

   size_t base = oid.length() - 6;
   for(size_t i = 0; i < 6; i++)
      info->macAddr[i] = oid.getElement(i + base);

   wsList->add(info);
}

/**
 * Get list of associated wireless stations. Default implementation always return NULL.
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @return list of associated wireless stations
 */
ObjectArray<WirelessStationInfo> *MdsOrbitDriver::getWirelessStations(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   auto wsList = new ObjectArray<WirelessStationInfo>(0, 16, Ownership::True);

   if (SnmpWalk(snmp, { 1, 3, 6, 1, 4, 1, 4130, 10, 2, 5, 1, 2, 2, 1, 6 }, // mIfLnStatusConnRemRssi
      [wsList, snmp] (SNMP_Variable *v) -> uint32_t
      {
         AddLnRadioClient(snmp, v, wsList);
         return SNMP_ERR_SUCCESS;
      }) != SNMP_ERR_SUCCESS)
   {
      delete wsList;
      return nullptr;
   }

   if (SnmpWalk(snmp, { 1, 3, 6, 1, 4, 1, 4130, 10, 2, 2, 1, 2, 3, 1, 2 }, // mIfDot11ApClientRssi
      [wsList, snmp] (SNMP_Variable *v) -> uint32_t
      {
         AddWiFiRadioClient(snmp, v, wsList);
         return SNMP_ERR_SUCCESS;
      }) != SNMP_ERR_SUCCESS)
   {
      delete wsList;
      return nullptr;
   }

   return wsList;
}
