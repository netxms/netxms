/*
** NetXMS - Network Management System
** Driver for H3C (now HPE A-series) switches
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
** File: aruba-sw.cpp
**/

#include "hpe.h"
#include <math.h>

#define DEBUG_TAG_ARUBA_SW _T("ndd.arubasw")

/**
 * Get driver name
 */
const TCHAR *ArubaSwitchDriver::getName()
{
   return _T("ARUBA-SW");
}

/**
 * Get driver version
 */
const TCHAR *ArubaSwitchDriver::getVersion()
{
   return NETXMS_VERSION_STRING;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int ArubaSwitchDriver::isPotentialDevice(const SNMP_ObjectId& oid)
{
   return oid.startsWith({ 1, 3, 6, 1, 4, 1, 14823, 1, 1 }) ? 255 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool ArubaSwitchDriver::isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid)
{
   TCHAR model[256];
   if (SnmpGetEx(snmp, { 1, 3, 6, 1, 4, 1, 14823, 2, 2, 1, 2, 1, 3, 0 }, model, sizeof(model), SG_STRING_RESULT) != SNMP_ERR_SUCCESS)
      return false;
   nxlog_debug_tag(DEBUG_TAG_ARUBA_SW, 5, _T("ArubaSwitchDriver::isDeviceSupported(): model name = %s"), model);
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
bool ArubaSwitchDriver::getHardwareInformation(SNMP_Transport *snmp, NObject *node, DriverData *driverData, DeviceHardwareInfo *hwInfo)
{
   _tcscpy(hwInfo->vendor, _T("HPE Aruba Networking"));

   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.14823.2.2.1.2.1.3.0"))); // wlsxSysExtModelName
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.14823.2.2.1.2.1.27.0"))); // wlsxSysExtHwVersion
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.14823.2.2.1.2.1.29.0"))); // wlsxSysExtSerialNumber

   SNMP_PDU *response;
   if (snmp->doRequest(&request, &response) == SNMP_ERR_SUCCESS)
   {
      SNMP_Variable *v = response->getVariable(0);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
      {
         v->getValueAsString(hwInfo->productName, 128);
      }

      v = response->getVariable(1);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
      {
         v->getValueAsString(hwInfo->productVersion, 16);
      }

      v = response->getVariable(2);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
      {
         v->getValueAsString(hwInfo->serialNumber, 32);
      }

      delete response;
   }

   return true;
}

/*
 * Check switch for wireless capabilities
 *
 * @param snmp SNMP transport
 * @param attributes Node custom attributes
 * @param driverData optional pointer to user data
 */
bool ArubaSwitchDriver::isWirelessController(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   BYTE buffer[256];
   return SnmpGetEx(snmp, { 1, 3, 6, 1, 4, 1, 14823, 2, 2, 1, 5, 2, 1, 1, 0 }, buffer, sizeof(buffer), SG_RAW_RESULT) == SNMP_ERR_SUCCESS;
}

/**
 * Handler for access point enumeration
 */
static uint32_t HandlerAccessPointList(SNMP_Variable *var, SNMP_Transport *snmp, ObjectArray<AccessPointInfo> *apList)
{
   const SNMP_ObjectId& name = var->getName();
   size_t nameLen = name.length();
   uint32_t oid[MAX_OID_LEN];
   memcpy(oid, name.value(), nameLen * sizeof(UINT32));

   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());

   oid[15] = 2;   // wlanAPIpAddress
   request.bindVariable(new SNMP_Variable(oid, nameLen));

   oid[15] = 13;  // wlanAPModelName
   request.bindVariable(new SNMP_Variable(oid, nameLen));

   oid[15] = 6;  // wlanAPSerialNumber
   request.bindVariable(new SNMP_Variable(oid, nameLen));

   oid[15] = 19;  // wlanAPStatus (up(1), down(2))
   request.bindVariable(new SNMP_Variable(oid, nameLen));

   oid[15] = 9;  // wlanAPNumRadios
   request.bindVariable(new SNMP_Variable(oid, nameLen));

   oid[15] = 33;  // wlanAPHwVersion
   request.bindVariable(new SNMP_Variable(oid, nameLen));

   oid[15] = 34;  // wlanAPSwVersion
   request.bindVariable(new SNMP_Variable(oid, nameLen));

   SNMP_PDU *response;
   uint32_t rcc = snmp->doRequest(&request, &response);
   if (rcc == SNMP_ERR_SUCCESS)
   {
      AccessPointInfo *ap = nullptr;
      if (response->getNumVariables() == 7)
      {
         BYTE macAddress[MAC_ADDR_LENGTH];
         for(int i = 0; i < MAC_ADDR_LENGTH; i++)
            macAddress[i] = static_cast<BYTE>(oid[i + 16]);

         TCHAR ipAddr[32], name[MAX_OBJECT_NAME], model[64], serial[64];
         ap = new AccessPointInfo(
            0,
            MacAddress(macAddress, MAC_ADDR_LENGTH),
            InetAddress::parse(response->getVariable(0)->getValueAsString(ipAddr, 32)),
            (response->getVariable(3)->getValueAsInt() == 1) ? AP_UP : AP_DOWN,
            var->getValueAsString(name, MAX_OBJECT_NAME),
            _T("HPE Aruba Networking"),   // vendor
            response->getVariable(1)->getValueAsString(model, 64), // model
            response->getVariable(2)->getValueAsString(serial, 64)); // serial
         apList->add(ap);
      }

      int numRadios = response->getVariable(4)->getValueAsInt();
      delete response;

      if ((ap != nullptr) && (numRadios > 0))
      {
         uint32_t radioOID[] = { 1, 3, 6, 1, 4, 1, 14823, 2, 2, 1, 5, 2, 1, 5, 1, 3, 0, 0, 0, 0, 0, 0, 0 }; // wlanAPRadioChannel
         memcpy(&radioOID[16], &oid[16], 6 * sizeof(uint32_t));   // AP MAC address

         RadioInterfaceInfo radio;
         for(int i = 1; i <= numRadios; i++)
         {
            SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());

            radioOID[22] = i; // Radio number
            request.bindVariable(new SNMP_Variable(radioOID, sizeof(radioOID) / sizeof(uint32_t)));

            radioOID[15] = 17; // wlanAPRadioTransmitPower10x
            request.bindVariable(new SNMP_Variable(radioOID, sizeof(radioOID) / sizeof(uint32_t)));

            radioOID[15] = 2; // wlanAPRadioType
            request.bindVariable(new SNMP_Variable(radioOID, sizeof(radioOID) / sizeof(uint32_t)));

            rcc = snmp->doRequest(&request, &response);
            if (rcc == SNMP_ERR_SUCCESS)
            {
               if (response->getNumVariables() == 3)
               {
                  memset(&radio, 0, sizeof(RadioInterfaceInfo));
                  memcpy(radio.name, _T("radio"), 5 * sizeof(TCHAR));
                  IntegerToString(i, &radio.name[5]);
                  radio.index = i;
                  radio.channel = response->getVariable(0)->getValueAsInt();
                  double dBm = static_cast<double>(response->getVariable(1)->getValueAsInt()) / 10.0;
                  radio.powerDBm = static_cast<int32_t>(dBm);
                  radio.powerMW = (dBm > 0) ? static_cast<int32_t>(pow(10.0, dBm / 10.0)) : 0;

                  switch(response->getVariable(2)->getValueAsInt())
                  {
                     case 1:  // 802.11a
                        radio.band = RADIO_BAND_5_GHZ;
                        break;
                     case 2:  // 802.11b
                        radio.band = RADIO_BAND_2_4_GHZ;
                        break;
                     case 3:  // 802.11g
                        radio.band = RADIO_BAND_2_4_GHZ;
                        break;
                     case 4:  // 802.11ag (802.11a + 802.11g)
                        radio.band = (radio.channel < 15) ? RADIO_BAND_2_4_GHZ : RADIO_BAND_5_GHZ;
                        break;
                     case 6:  // 802.11ax 6GHz
                        radio.band = RADIO_BAND_6_GHZ;
                        break;
                  }
                  radio.frequency = WirelessChannelToFrequency(radio.band, radio.channel);

                  uint32_t ssidOID[] = { 1, 3, 6, 1, 4, 1, 14823, 2, 2, 1, 5, 2, 1, 7, 1, 2, 0, 0, 0, 0, 0, 0, 0 }; // wlanAPESSID
                  memcpy(&ssidOID[16], &oid[16], 6 * sizeof(uint32_t));   // AP MAC address
                  ssidOID[22] = i; // Radio number
                  int count = 0;
                  size_t nl = _tcslen(radio.name);
                  SnmpWalk(snmp, ssidOID, sizeof(ssidOID) / sizeof(uint32_t),
                     [&count, &radio, nl, ap] (SNMP_Variable *v) -> uint32_t
                     {
                        const SNMP_ObjectId& n = v->getName();
                        for(int i = 0; i < MAC_ADDR_LENGTH; i++)
                           radio.bssid[i] = static_cast<BYTE>(n.getElement(i + 23));
                        v->getValueAsString(radio.ssid, MAX_SSID_LENGTH);
                        radio.name[nl] = '.';
                        IntegerToString(++count, &radio.name[nl + 1]);
                        ap->addRadioInterface(radio);
                        return SNMP_ERR_SUCCESS;
                     });

                  if (count == 0)
                     ap->addRadioInterface(radio); // No BSSID found for this radio interface
               }
               delete response;
            }
         }
      }
   }

   return SNMP_ERR_SUCCESS;
}

/**
 * Get access points
 *
 * @param snmp SNMP transport
 * @param attributes Node custom attributes
 * @param driverData optional pointer to user data
 */
ObjectArray<AccessPointInfo> *ArubaSwitchDriver::getAccessPoints(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   auto apList = new ObjectArray<AccessPointInfo>(0, 16, Ownership::True);

   if (SnmpWalk(snmp, _T(".1.3.6.1.4.1.14823.2.2.1.5.2.1.4.1.3"), HandlerAccessPointList, apList) != SNMP_ERR_SUCCESS) // wlanAPName
   {
      delete apList;
      return nullptr;
   }

   return apList;
}

/**
 * Get access point state
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @param apIndex access point index
 * @param macAdddr access point MAC address
 * @param ipAddr access point IP address
 * @param radioInterfaces radio interfaces of this AP
 * @return state of access point or AP_UNKNOWN if it cannot be determined
 */
AccessPointState ArubaSwitchDriver::getAccessPointState(SNMP_Transport *snmp, NObject *node, DriverData *driverData,
      uint32_t apIndex, const MacAddress &macAddr, const InetAddress &ipAddr, const StructArray<RadioInterfaceInfo>& radioInterfaces)
{
   uint32_t oid[] = { 1, 3, 6, 1, 4, 1, 14823, 2, 2, 1, 5, 2, 1, 4, 1, 19, 0, 0, 0, 0, 0, 0 }; // wlanAPStatus
   for(int i = 0; i < MAC_ADDR_LENGTH; i++)
      oid[i + 16] = macAddr.value()[i];

   uint32_t state;
   if (SnmpGetEx(snmp, nullptr, oid, sizeof(oid) / sizeof(uint32_t), &state, sizeof(uint32_t), 0) != SNMP_ERR_SUCCESS)
      return AP_UNKNOWN;

   return (state == 1) ? AP_UP : ((state == 2) ? AP_DOWN : AP_UNKNOWN);
}

/**
 * Handler for mobile units enumeration
 */
static uint32_t HandlerWirelessStationList(SNMP_Variable *var, SNMP_Transport *snmp, ObjectArray<WirelessStationInfo> *wsList)
{
   if (var->getValueAsInt() == 5)   // wired user
      return SNMP_ERR_SUCCESS;

   SNMP_ObjectId oid(var->getName());
   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());

   oid.changeElement(14, 11); // nUserApBSSID
   request.bindVariable(new SNMP_Variable(oid));

   oid.changeElement(14, 22); // nUserCurrentVlan
   request.bindVariable(new SNMP_Variable(oid));

   SNMP_PDU *response;
   uint32_t rcc = snmp->doRequest(&request, &response);
   if (rcc == SNMP_ERR_SUCCESS)
   {
      if (response->getNumVariables() == 3)
      {
         auto ws = new WirelessStationInfo;
         memset(ws, 0, sizeof(WirelessStationInfo));
         response->getVariable(0)->getRawValue(ws->bssid, MAC_ADDR_LENGTH);
         for(int i = 0; i < MAC_ADDR_LENGTH; i++)
            ws->macAddr[i] = oid.getElement(i + 15);
         ws->ipAddr = InetAddress((oid.getElement(21) << 24) | (oid.getElement(22) << 16) | (oid.getElement(23) << 8) | oid.getElement(24));
         ws->vlan = response->getVariable(1)->getValueAsInt();
         ws->apMatchPolicy = AP_MATCH_BY_BSSID;
         wsList->add(ws);
      }
      delete response;
   }

   return SNMP_ERR_SUCCESS;
}

/**
 * Get registered wireless stations (clients)
 *
 * @param snmp SNMP transport
 * @param attributes Node custom attributes
 * @param driverData optional pointer to user data
 */
ObjectArray<WirelessStationInfo> *ArubaSwitchDriver::getWirelessStations(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   auto wsList = new ObjectArray<WirelessStationInfo>(0, 16, Ownership::True);
   if (SnmpWalk(snmp, _T(".1.3.6.1.4.1.14823.2.2.1.4.1.2.1.26"),// nUserPhyType
                HandlerWirelessStationList, wsList) != SNMP_ERR_SUCCESS)
   {
      delete wsList;
      wsList = nullptr;
   }
   return wsList;
}
