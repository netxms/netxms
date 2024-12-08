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
** File: epmp.cpp
**
**/

#include "cambium.h"
#include <math.h>

/**
 * Get driver name
 */
const TCHAR *CambiumEPMPDriver::getName()
{
   return _T("CAMBIUM-EPMP");
}

/**
 * Get driver version
 */
const TCHAR *CambiumEPMPDriver::getVersion()
{
   return NETXMS_VERSION_STRING;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int CambiumEPMPDriver::isPotentialDevice(const SNMP_ObjectId& oid)
{
   return oid.startsWith({ 1, 3, 6, 1, 4, 1, 17713, 21 }) ? 200 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool CambiumEPMPDriver::isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid)
{
   return true;
}

/**
 * Device code to name mapping
 */
static CodeLookupElement s_deviceNames[] =
{
   { 0, _T("5 GHz Connectorized Radio with Sync") },
   { 1, _T("5 GHz Connectorized Radio") },
   { 2, _T("5 GHz Integrated Radio") },
   { 3, _T("2.4 GHz Connectorized Radio with Sync") },
   { 4, _T("2.4 GHz Connectorized Radio") },
   { 5, _T("2.4 GHz Integrated Radio") },
   { 6, _T("5 GHz Force 200 (ROW)") },
   { 8, _T("5 GHz Force 200 (FCC)") },
   { 9, _T("2.4 GHz Force 200") },
   { 10, _T("ePMP 2000") },
   { 11, _T("5 GHz Force 180 (ROW)") },
   { 12, _T("5 GHz Force 180 (FCC)") },
   { 13, _T("5 GHz Force 190 Radio (ROW/ETSI)") },
   { 14, _T("5 GHz Force 190 Radio (FCC)") },
   { 16, _T("6 GHz Force 180 Radio") },
   { 17, _T("6 GHz Connectorized Radio with Sync") },
   { 18, _T("6 GHz Connectorized Radio") },
   { 19, _T("2.5 GHz Connectorized Radio with Sync") },
   { 20, _T("2.5 GHz Connectorized Radio") },
   { 22, _T("5 GHz Force 130 Radio") },
   { 23, _T("2.4 GHz Force 130 Radio") },
   { 24, _T("5 GHz Force 200L Radio") },
   { 25, _T("5 GHz Force 200L Radio V2") },
   { 33, _T("5 GHz PTP550 Integrated Radio") },
   { 34, _T("5 GHz PTP550 Connectorized Radio") },
   { 35, _T("5 GHz Force 300-25 Radio (FCC)") },
   { 36, _T("5 GHz Force 300-25 Radio (ROW/ETSI)") },
   { 37, _T("ePMP3000 (FCC)") },
   { 38, _T("5 GHz Force 300-16 Radio (FCC)") },
   { 39, _T("5 GHz Force 300-16 Radio (ROW/ETSI)") },
   { 40, _T("ePMP3000 (ROW/ETSI)") },
   { 41, _T("5 GHz PTP 550E Integrated Radio") },
   { 42, _T("5 GHz PTP 550E Connectorized Radio") },
   { 43, _T("5 GHz ePMP3000L (FCC)") },
   { 44, _T("5 GHz ePMP3000L (ROW/ETSI)") },
   { 45, _T("5 GHz Force 300 Connectorized Radio without GPS (FCC)") },
   { 46, _T("5 GHz Force 300 Connectorized Radio without GPS (ROW/ETSI)") },
   { 47, _T("5 GHz Force 300-13 Radio (FCC)") },
   { 48, _T("5 GHz Force 300-13 Radio (ROW/ETSI)") },
   { 49, _T("5 GHz Force 300-19 Radio (FCC)") },
   { 50, _T("5 GHz Force 300-19 Radio (ROW/ETSI)") },
   { 51, _T("5 GHz Force 300-19R IP67 Radio (ROW/ETSI)") },
   { 52, _T("5 GHz Force 300-19R IP67 Radio (FCC)") },
   { 53, _T("5 GHz ePMP Client MAXrP IP67 Radio (FCC)") },
   { 54, _T("5 GHz ePMP Client MAXrP IP67 Radio (ROW/ETSI)") },
   { 55, _T("5 GHz Force 300-25 Radio V2 (FCC)") },
   { 58, _T("5 GHz Force 300-25L Radio") },
   { 59, _T("5 GHz Force 300 CSML Connectorized Radio") },
   { 60, _T("5 GHz Force 300-22L Radio") },
   { 61, _T("5 GHz Force 300-13L Radio") },
   { 62, _T("5 GHz ePMP MP 3000") },
   { 100, _T("ePMP Elevate NSM5-XW") },
   { 110, _T("ePMP Elevate NSlocoM5-XW") },
   { 111, _T("ePMP Elevate NSlocoM2-XW") },
   { 112, _T("ePMP Elevate NSlocoM2-V2-XW") },
   { 113, _T("ePMP Elevate NSlocoM2-V3-XW") },
   { 120, _T("ePMP Elevate RM5-XW-V1") },
   { 121, _T("ePMP Elevate RM5-XW-V2") },
   { 130, _T("ePMP Elevate NBE-M5-16-XW") },
   { 131, _T("ePMP Elevate NBE-M5-19-XW") },
   { 132, _T("ePMP Elevate NBE-M2-13-XW") },
   { 140, _T("ePMP Elevate SXTLITE5 BOARD") },
   { 141, _T("ePMP Elevate INTELBRAS BOARD") },
   { 142, _T("ePMP Elevate LHG5 BOARD") },
   { 143, _T("ePMP Elevate Disc Lite BOARD") },
   { 144, _T("ePMP Elevate 911L BOARD") },
   { 145, _T("ePMP Elevate Sextant BOARD") },
   { 150, _T("ePMP Elevate PBE-M5-300-XW") },
   { 151, _T("ePMP Elevate PBE-M5-400-XW") },
   { 152, _T("ePMP Elevate PBE-M5-620-XW") },
   { 153, _T("ePMP Elevate PBE-M2-400-XW") },
   { 154, _T("ePMP Elevate PBE-M5-400-ISO-XW") },
   { 155, _T("ePMP Elevate PBE-M5-300-ISO-XW") },
   { 156, _T("ePMP Elevate PBE-M5-600-ISO-XW") },
   { 160, _T("ePMP Elevate AG-HP-5G-XW") },
   { 161, _T("ePMP Elevate AG-HP-2G-XW") },
   { 162, _T("ePMP Elevate LB-M5-23-XW") },
   { 170, _T("ePMP Elevate NSM5-XM-V1") },
   { 171, _T("ePMP Elevate NSM5-XM-V2") },
   { 173, _T("ePMP Elevate NS-M2-V1") },
   { 176, _T("ePMP Elevate NS-M2-V2") },
   { 180, _T("ePMP Elevate NSlocoM5-XM-V1") },
   { 181, _T("ePMP Elevate NSlocoM5-XM-V2") },
   { 183, _T("ePMP Elavate NSloco-M2") },
   { 193, _T("ePMP Elevate NB-5G22-XM") },
   { 194, _T("ePMP Elevate NB-5G25-XM") },
   { 195, _T("ePMP Elevate NB-XM") },
   { 196, _T("ePMP Elevate NB-M2-V1-XM") },
   { 197, _T("ePMP Elevate NB-M2-V2-XM") },
   { 200, _T("ePMP Elevate INTELBRAS WOM-5A-MiMo BOARD") },
   { 201, _T("ePMP Elevate INTELBRAS WOM-5A-23 BOARD") },
   { 220, _T("ePMP Elevate NS-M6") },
   { 230, _T("ePMP Elevate AG-M5-23-XM") },
   { 231, _T("ePMP Elevate AG-M5-28-XM") },
   { 232, _T("ePMP Elevate AG-HP-5G-XM") },
   { 233, _T("ePMP Elevate AG-M2-16-XM") },
   { 234, _T("ePMP Elevate AG-M2-20-XM") },
   { 235, _T("ePMP Elevate AG-M2-HP-XM") },
   { 236, _T("ePMP Elevate BM-M2-HP-XM") },
   { 237, _T("ePMP Elevate BM-M5-HP-XM") },
   { 241, _T("ePMP Elevate RM5-V1-XM") },
   { 242, _T("ePMP Elevate RM5-V2-XM") },
   { 243, _T("ePMP Elevate RM5-V3-XM") },
   { 244, _T("ePMP Elevate RM5-V4-XM") },
   { 245, _T("ePMP Elevate RM2-V1-XM") },
   { 246, _T("ePMP Elevate RM2-V2-XM") },
   { 247, _T("ePMP Elevate RM2-V3-XM") },
   { 248, _T("ePMP Elevate RM2-V4-XM") },
   { 53280, _T("5 GHz ePMP4000 8x8 (ROW)") },
   { 53288, _T("5 GHz ePMP4000 8x8 (FCC)") },
   { 53344, _T("5 GHz Force 400 Radio with GPS (ROW)") },
   { 53352, _T("5 GHz Force 400 Radio with GPS (FCC)") },
   { 53504, _T("5 GHz Force 425 Radio (ROW)") },
   { 53505, _T("5 GHz Force 400 CSM Radio (ROW)") },
   { 53512, _T("5 GHz Force 425 Radio (FCC)") },
   { 53513, _T("5 GHz Force 400 CSM Radio (FCC)") },
   { 0, nullptr }
};

/**
 * Get hardware information from device.
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver data
 * @param hwInfo pointer to hardware information structure to fill
 * @return true if hardware information is available
 */
bool CambiumEPMPDriver::getHardwareInformation(SNMP_Transport *snmp, NObject *node, DriverData *driverData, DeviceHardwareInfo *hwInfo)
{
   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.17713.21.1.1.1.0")));   // cambiumCurrentSWInfo
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.17713.21.1.1.31.0")));  // cambiumEPMPMSN
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.17713.21.1.1.2.0")));   // cambiumHWInfo

   SNMP_PDU *response;
   if (snmp->doRequest(&request, &response) != SNMP_ERR_SUCCESS)
      return false;

   if (response->getNumVariables() != request.getNumVariables())
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Malformed device response in CambiumEPMPDriver::getHardwareInformation"));
      delete response;
      return false;
   }

   _tcscpy(hwInfo->vendor, _T("Cambium"));

   response->getVariable(0)->getValueAsString(hwInfo->productVersion, sizeof(hwInfo->productVersion) / sizeof(TCHAR));
   response->getVariable(1)->getValueAsString(hwInfo->serialNumber, sizeof(hwInfo->serialNumber) / sizeof(TCHAR));

   SNMP_Variable *v = response->getVariable(2);
   if (v->getType() == ASN_INTEGER)
   {
      const TCHAR *modelName = CodeToText(v->getValueAsInt(), s_deviceNames, nullptr);
      if (modelName != nullptr)
         _tcslcpy(hwInfo->productName, modelName, sizeof(hwInfo->productName) / sizeof(TCHAR));
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
GeoLocation CambiumEPMPDriver::getGeoLocation(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.17713.21.1.1.18.0")));   // cambiumDeviceLatitude
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.17713.21.1.1.19.0")));  // cambiumDeviceLongitude

   SNMP_PDU *response;
   if (snmp->doRequest(&request, &response) != SNMP_ERR_SUCCESS)
      return GeoLocation();

   if (response->getNumVariables() != request.getNumVariables())
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Malformed device response in CambiumEPMPDriver::getGeoLocation"));
      delete response;
      return GeoLocation();
   }

   TCHAR latitudeString[64], longitudeString[64], *eptrLat, *eptrLon;
   response->getVariable(0)->getValueAsString(latitudeString, 64);
   response->getVariable(1)->getValueAsString(longitudeString, 64);
   double latitude = _tcstod(latitudeString, &eptrLat);
   double longitude = _tcstod(longitudeString, &eptrLon);

   if ((*eptrLat != 0) || (*eptrLon != 0) || ((latitude == 0) && (longitude == 0)))
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("CambiumEPMPDriver::getGeoLocation: invalid coordinates %s %s"), latitudeString, longitudeString);
      return GeoLocation();
   }

   return GeoLocation(GL_GPS, latitude, longitude);
}

/**
 * Get list of interfaces for given node
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver data
 * @param useIfXTable if true, usage of ifXTable is allowed
 */
InterfaceList *CambiumEPMPDriver::getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable)
{
   InterfaceList *ifList = NetworkDeviceDriver::getInterfaces(snmp, node, driverData, useIfXTable);
   if (ifList == nullptr)
      return nullptr;

   // ePMP devices report incorrect interface speed through standard MIBs.
   // Try to get correct values from vendor MIBs or calculate it.
   for(int i = 0; i < ifList->size(); i++)
   {
      InterfaceInfo *iface = ifList->get(i);
      if (!_tcsnicmp(iface->name, _T("LAN interface "), 14))
      {
         int32_t speed;
         if (SnmpGetEx(snmp, (iface->name[14] == '1') ? _T(".1.3.6.1.4.1.17713.21.3.4.2.10.0") : _T(".1.3.6.1.4.1.17713.21.3.4.2.22.0"), // use networkLanSpeed or networkLan2Speed
                  nullptr, 0, &speed, sizeof(speed), 0, nullptr) == SNMP_ERR_SUCCESS)
         {
            // Some devices reported to return -1, also check for other unrealistic values
            if ((speed > 0) && (speed <= 1000))
               iface->speed = static_cast<uint64_t>(speed) * _ULL(1000000);   // Returned speed is in Mbps
            else if (iface->speed > _ULL(1000000000))
               iface->speed = 0;
         }

         if (iface->name[14] == '1')
         {
            // Get device mode and then read IP address/mask from appropriate OIDs
            int32_t mode;
            if (SnmpGetEx(snmp, _T(".1.3.6.1.4.1.17713.21.3.4.1.0"), nullptr, 0, &mode, sizeof(mode), 0, nullptr) == SNMP_ERR_SUCCESS) // networkMode
            {
               if ((mode == 1) || (mode == 2))  // NAT or bridge mode
               {
                  SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
                  request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.17713.21.1.4.3.0")));   // cambiumEffectiveDeviceIPAddress
                  request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.17713.21.1.4.5.0")));   // cambiumEffectiveDeviceLANNetMask

                  bool addressRetrieved = false;
                  SNMP_PDU *response;
                  if (snmp->doRequest(&request, &response) == SNMP_ERR_SUCCESS)
                  {
                     if (response->getNumVariables() == 2)
                     {
                        TCHAR buffer[256];
                        InetAddress addr = InetAddress::parse(response->getVariable(0)->getValueAsString(buffer, 256));
                        if (addr.isValid())
                        {
                           uint32_t mask = InetAddress::parse(response->getVariable(1)->getValueAsString(buffer, 256)).getAddressV4();
                           if (mask != 0)
                           {
                              addr.setMaskBits(BitsInMask(mask));
                              iface->ipAddrList.add(addr);
                              addressRetrieved = true;
                           }
                        }
                     }
                     delete response;
                  }

                  if (!addressRetrieved)
                  {
                     SNMP_PDU request2(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
                     request2.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.17713.21.3.4.2.2.0")));   // networkLanIPAddr
                     request2.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.17713.21.3.4.2.3.0")));   // networkLanNetmask

                     if (snmp->doRequest(&request2, &response) == SNMP_ERR_SUCCESS)
                     {
                        if (response->getNumVariables() == 2)
                        {
                           TCHAR buffer[256];
                           InetAddress addr = InetAddress::parse(response->getVariable(0)->getValueAsString(buffer, 256));
                           if (addr.isValid())
                           {
                              uint32_t mask = InetAddress::parse(response->getVariable(1)->getValueAsString(buffer, 256)).getAddressV4();
                              if (mask != 0)
                              {
                                 addr.setMaskBits(BitsInMask(mask));
                                 iface->ipAddrList.add(addr);
                                 addressRetrieved = true;
                              }
                           }
                        }
                        delete response;
                     }
                  }

                  if (!addressRetrieved && (mode == 2))
                  {
                     SNMP_PDU request2(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
                     request2.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.17713.21.3.4.7.2.0")));   // networkBridgeIPAddr
                     request2.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.17713.21.3.4.7.3.0")));   // networkBridgeNetmask
                     if (snmp->doRequest(&request2, &response) == SNMP_ERR_SUCCESS)
                     {
                        if (response->getNumVariables() == 2)
                        {
                           TCHAR buffer[256];
                           InetAddress addr = InetAddress::parse(response->getVariable(0)->getValueAsString(buffer, 256));
                           if (addr.isValid())
                           {
                              uint32_t mask = InetAddress::parse(response->getVariable(1)->getValueAsString(buffer, 256)).getAddressV4();
                              if (mask != 0)
                              {
                                 addr.setMaskBits(BitsInMask(mask));
                                 iface->ipAddrList.add(addr);
                              }
                           }
                        }
                        delete response;
                     }
                  }
               }
            }
         }
      }
      else if ((iface->type == 6) || (iface->type == 71))
      {
         // Speed reported through ifHighSpeed seems to be in bps instead of Mbps as required by specifications
         // If it is > 10G assume that it was incorrectly reported in bps
         if (iface->speed > _ULL(10000000000))
            iface->speed /= _ULL(1000000);
      }
   }

   return ifList;
}

/**
 * Returns true if device is a standalone wireless access point (not managed by a controller). Default implementation always return false.
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @return true if device is a standalone wireless access point
 */
bool CambiumEPMPDriver::isWirelessAccessPoint(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   // Read wirelessInterfaceMode - it should be 1 for AP
   int32_t mode;
   if (SnmpGetEx(snmp, _T(".1.3.6.1.4.1.17713.21.3.8.2.1.0"), nullptr, 0, &mode, sizeof(mode), 0, nullptr) != SNMP_ERR_SUCCESS)
      return false;
   return mode == 1;
}

/**
 * Get list of radio interfaces for standalone access point. Default implementation always return NULL.
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @return list of radio interfaces for standalone access point
 */
StructArray<RadioInterfaceInfo> *CambiumEPMPDriver::getRadioInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());

   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.17713.21.3.8.2.2.0")));    // wirelessInterfaceSSID
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.17713.21.3.8.2.6.0")));    // wirelessInterfaceTXPower
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.17713.21.3.8.2.18.0")));   // centerFrequency
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.17713.21.3.8.12.6.0")));   // wirelessInterface2TXPower
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.17713.21.3.8.12.18.0")));  // centerFrequency2
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.17713.21.1.1.5.0")));      // cambiumWirelessMACAddress
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.17713.21.1.1.51.0")));     // cambiumWireless2MACAddress

   SNMP_PDU *response;
   if (snmp->doRequest(&request, &response) != SNMP_ERR_SUCCESS)
      return nullptr;

   if (response->getNumVariables() != request.getNumVariables())
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Malformed device response in CambiumEPMPDriver::getRadioInterfaces"));
      delete response;
      return nullptr;
   }

   auto radios = new StructArray<RadioInterfaceInfo>(2, 16);

   TCHAR macAddrText[64] = _T("");
   MacAddress radioMAC = MacAddress::parse(response->getVariable(5)->getValueAsString(macAddrText, 64));

   RadioInterfaceInfo *ri = radios->addPlaceholder();
   memset(ri, 0, sizeof(RadioInterfaceInfo));
   ri->index = 1;
   memcpy(ri->bssid, radioMAC.value(), MAC_ADDR_LENGTH);
   response->getVariable(0)->getValueAsString(ri->ssid, MAX_SSID_LENGTH);
   _tcscpy(ri->name, _T("radio1"));
   ri->frequency = static_cast<uint16_t>(response->getVariable(2)->getValueAsUInt());
   ri->band = WirelessFrequencyToBand(ri->frequency);
   ri->channel = WirelessFrequencyToChannel(ri->frequency);
   ri->powerDBm = response->getVariable(1)->getValueAsInt();
   ri->powerMW = (int)pow(10.0, (double)ri->powerDBm / 10.0);

   macAddrText[0] = 0;
   radioMAC = MacAddress::parse(response->getVariable(6)->getValueAsString(macAddrText, 64));
   if (radioMAC.isValid())
   {
      ri = radios->addPlaceholder();
      memset(ri, 0, sizeof(RadioInterfaceInfo));
      ri->index = 2;
      memcpy(ri->bssid, radioMAC.value(), MAC_ADDR_LENGTH);
      _tcscpy(ri->name, _T("radio2"));
      response->getVariable(0)->getValueAsString(ri->ssid, MAX_SSID_LENGTH);
      ri->frequency = static_cast<uint16_t>(response->getVariable(4)->getValueAsUInt());
      ri->band = WirelessFrequencyToBand(ri->frequency);
      ri->channel = WirelessFrequencyToChannel(ri->frequency);
      ri->powerDBm = response->getVariable(3)->getValueAsInt();
      ri->powerMW = (int)pow(10.0, (double)ri->powerDBm / 10.0);
   }

   delete response;
   return radios;
}

/**
 * Handler for wireless station enumeration
 */
static uint32_t HandlerWirelessStationList(SNMP_Variable *var, SNMP_Transport *snmp, ObjectArray<WirelessStationInfo> *wsList)
{
   SNMP_ObjectId oid = var->getName();
   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());

   oid.changeElement(12, 5);  // connectedSTADLRSSI
   request.bindVariable(new SNMP_Variable(oid));

   oid.changeElement(12, 10);  // connectedSTAIP
   request.bindVariable(new SNMP_Variable(oid));

   oid.changeElement(12, 36);  // connectedSTAIsViaInterface
   request.bindVariable(new SNMP_Variable(oid));

   SNMP_PDU *response;
   uint32_t rcc = snmp->doRequest(&request, &response);
   if (rcc != SNMP_ERR_SUCCESS)
      return rcc;

   if (response->getNumVariables() != request.getNumVariables())
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Malformed device response in CambiumEPMPDriver::getWirelessStations"));
      delete response;
      return SNMP_ERR_SUCCESS;
   }

   auto info = new WirelessStationInfo;
   memset(info, 0, sizeof(WirelessStationInfo));

   TCHAR macAddrText[64];
   memcpy(info->macAddr, MacAddress::parse(var->getValueAsString(macAddrText, 64)).value(), MAC_ADDR_LENGTH);
   info->ipAddr = ntohl(response->getVariable(1)->getValueAsUInt());
   info->rfIndex = response->getVariable(2)->getValueAsInt() + 1;
   if ((info->rfIndex < 1) || (info->rfIndex > 2))
      info->rfIndex = 1;
   info->apMatchPolicy = AP_MATCH_BY_RFINDEX;
   info->rssi = response->getVariable(0)->getValueAsInt();
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
ObjectArray<WirelessStationInfo> *CambiumEPMPDriver::getWirelessStations(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   auto wsList = new ObjectArray<WirelessStationInfo>(0, 16, Ownership::True);
   if (SnmpWalk(snmp, _T(".1.3.6.1.4.1.17713.21.1.2.30.1.1"), HandlerWirelessStationList, wsList) != SNMP_ERR_SUCCESS) // connectedSTAMAC
   {
      delete wsList;
      wsList = nullptr;
   }
   return wsList;
}
