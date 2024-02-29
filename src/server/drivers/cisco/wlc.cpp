/* 
** NetXMS - Network Management System
** Driver for Cisco (former Airespace) wireless switches
** Copyright (C) 2013-2024 Raden Solutions
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
** File: wlc.cpp
**/

#include "cisco.h"
#include <netxms-version.h>

#define DEBUG_TAG DEBUG_TAG_CISCO _T(".wlc")

/**
 * Get driver name
 */
const TCHAR *CiscoWirelessControllerDriver::getName()
{
   return _T("CISCO-WLC");
}

/**
 * Get driver version
 */
const TCHAR *CiscoWirelessControllerDriver::getVersion()
{
   return NETXMS_VERSION_STRING;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int CiscoWirelessControllerDriver::isPotentialDevice(const SNMP_ObjectId& oid)
{
   return oid.startsWith({ 1, 3, 6, 1, 4, 1, 14179 }) || oid.startsWith({ 1, 3, 6, 1, 4, 1, 9, 1 }) ? 128 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool CiscoWirelessControllerDriver::isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid)
{
   // read agentInventoryMachineModel
   TCHAR buffer[1024];
   if (SnmpGetEx(snmp, _T(".1.3.6.1.4.1.14179.1.1.1.3.0"), nullptr, 0, buffer, sizeof(buffer), SG_STRING_RESULT, nullptr) != SNMP_ERR_SUCCESS)
      return false;

   nxlog_debug_tag(DEBUG_TAG, 5, _T("agentInventoryMachineModel=\"%s\""), buffer);

   // model must start with AIR-WLC44 (like AIR-WLC4402-50-K9) or AIR-CT55 (like AIR-CT5508-K90)
   return (_tcsncmp(buffer, _T("AIR-WLC44"), 9) == 0) || (_tcsncmp(buffer, _T("AIR-CT55"), 8) == 0) || (_tcsncmp(buffer, _T("AIR-CT85"), 8) == 0) || (_tcsncmp(buffer, _T("AIR-CTVM"), 8) == 0);
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
bool CiscoWirelessControllerDriver::getHardwareInformation(SNMP_Transport *snmp, NObject *node, DriverData *driverData, DeviceHardwareInfo *hwInfo)
{
   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.14179.1.1.1.3.0")));  // agentInventoryMachineModel (product code)
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.14179.1.1.1.13.0"))); // agentInventoryProductName (product name)
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.14179.1.1.1.14.0"))); // agentInventoryProductVersion (firmware version)
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.14179.1.1.1.4.0")));  // agentInventorySerialNumber (serial number)
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.14179.1.1.1.12.0"))); // agentInventoryManufacturerName (vendor)

   SNMP_PDU *response;
   if (snmp->doRequest(&request, &response) == SNMP_ERR_SUCCESS)
   {
      TCHAR buffer[256];

      const SNMP_Variable *v = response->getVariable(0);
      if ((v != NULL) && (v->getType() == ASN_OCTET_STRING))
      {
         _tcslcpy(hwInfo->productCode, v->getValueAsString(buffer, 256), 32);
      }

      v = response->getVariable(1);
      if ((v != NULL) && (v->getType() == ASN_OCTET_STRING))
      {
         _tcslcpy(hwInfo->productName, v->getValueAsString(buffer, 256), 128);
      }
      else
      {
         // Use product code as product name if product name OID is not supported
         _tcscpy(hwInfo->productName, hwInfo->productCode);
      }

      v = response->getVariable(2);
      if ((v != NULL) && (v->getType() == ASN_OCTET_STRING))
      {
         _tcslcpy(hwInfo->productVersion, v->getValueAsString(buffer, 256), 16);
      }

      v = response->getVariable(3);
      if ((v != NULL) && (v->getType() == ASN_OCTET_STRING))
      {
         _tcslcpy(hwInfo->serialNumber, v->getValueAsString(buffer, 256), 32);
      }

      v = response->getVariable(4);
      if ((v != NULL) && (v->getType() == ASN_OCTET_STRING))
      {
         _tcslcpy(hwInfo->vendor, v->getValueAsString(buffer, 256), 128);
      }
      else
      {
         _tcscpy(hwInfo->vendor, _T("Cisco Systems Inc."));
      }

      delete response;
   }
   return true;
}

/**
 * Get device virtualization type.
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver data
 * @param vtype pointer to virtualization type enum to fill
 * @return true if virtualization type is known
 */
bool CiscoWirelessControllerDriver::getVirtualizationType(SNMP_Transport *snmp, NObject *node, DriverData *driverData, VirtualizationType *vtype)
{
   TCHAR buffer[1024];
   if (SnmpGetEx(snmp, _T(".1.3.6.1.4.1.14179.1.1.1.3.0"), nullptr, 0, buffer, sizeof(buffer), SG_STRING_RESULT, nullptr) != SNMP_ERR_SUCCESS)
      return false;
   if (_tcsncmp(buffer, _T("AIR-CTVM"), 8) == 0)
      *vtype = VTYPE_FULL;
   else
      *vtype = VTYPE_NONE;
   return true;
}

/**
 * Get cluster mode for device (standalone / active / standby)
 *
 * @param snmp SNMP transport
 * @param attributes Node custom attributes
 * @param driverData optional pointer to user data
 */
int CiscoWirelessControllerDriver::getClusterMode(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   /* TODO: check if other cluster modes possible */
   return CLUSTER_MODE_STANDALONE;
}

/*
 * Check switch for wireless capabilities
 *
 * @param snmp SNMP transport
 * @param attributes Node custom attributes
 * @param driverData optional pointer to user data
 */
bool CiscoWirelessControllerDriver::isWirelessController(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   return true;
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

   SNMP_PDU *request = new SNMP_PDU(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
   
   oid[11] = 1;   // bsnAPDot3MacAddress
   request->bindVariable(new SNMP_Variable(oid, nameLen));

   oid[11] = 19;  // bsnApIpAddress
   request->bindVariable(new SNMP_Variable(oid, nameLen));

   oid[11] = 6;   // bsnAPOperationStatus
   request->bindVariable(new SNMP_Variable(oid, nameLen));

   oid[11] = 3;   // bsnAPName
   request->bindVariable(new SNMP_Variable(oid, nameLen));

   oid[11] = 16;  // bsnAPModel
   request->bindVariable(new SNMP_Variable(oid, nameLen));

   oid[11] = 17;  // bsnAPSerialNumber
   request->bindVariable(new SNMP_Variable(oid, nameLen));

   oid[9] = 2;    // bsnAPIfTable
   oid[10] = 1;   // bsnAPIfEntry
   oid[11] = 4;   // bsnAPIfPhyChannelNumber
   
   nameLen++;
   oid[nameLen - 1] = 0;   // first radio
   request->bindVariable(new SNMP_Variable(oid, nameLen));

   oid[nameLen - 1] = 1;   // second radio
   request->bindVariable(new SNMP_Variable(oid, nameLen));

   oid[11] = 2;   // bsnAPIfType
   oid[nameLen - 1] = 0;   // first radio
   request->bindVariable(new SNMP_Variable(oid, nameLen));

   oid[nameLen - 1] = 1;   // second radio
   request->bindVariable(new SNMP_Variable(oid, nameLen));

   SNMP_PDU *response;
   uint32_t rcc = snmp->doRequest(request, &response);
	delete request;
   if (rcc == SNMP_ERR_SUCCESS)
   {
      if (response->getNumVariables() == 10)
      {
         TCHAR ipAddr[32], name[MAX_OBJECT_NAME], model[MAX_OBJECT_NAME], serial[MAX_OBJECT_NAME];
         AccessPointInfo *ap = 
            new AccessPointInfo(
               0,
               var->getValueAsMACAddr(),
               InetAddress::parse(response->getVariable(1)->getValueAsString(ipAddr, 32)),
               (response->getVariable(2)->getValueAsInt() == 1) ? AP_UP : AP_UNPROVISIONED,
               response->getVariable(3)->getValueAsString(name, MAX_OBJECT_NAME),
               _T("Cisco Systems Inc."),   // vendor
               response->getVariable(4)->getValueAsString(model, MAX_OBJECT_NAME),
               response->getVariable(5)->getValueAsString(serial, MAX_OBJECT_NAME));

         RadioInterfaceInfo radio;
         memset(&radio, 0, sizeof(RadioInterfaceInfo));
         _tcscpy(radio.name, _T("slot0"));
         radio.index = 0;
         response->getVariable(0)->getRawValue(radio.bssid, MAC_ADDR_LENGTH);
         radio.channel = response->getVariable(6)->getValueAsInt();
         switch(response->getVariable(8)->getValueAsInt())  // bsnAPIfType
         {
            case 1:  // dot11b
               radio.band = RADIO_BAND_2_4_GHZ;
               break;
            case 2:  // dot11a
               radio.band = RADIO_BAND_5_GHZ;
               break;
         }
         radio.frequency = WirelessChannelToFrequency(radio.band, radio.channel);
         ap->addRadioInterface(radio);

         if ((response->getVariable(7)->getType() != ASN_NO_SUCH_INSTANCE) && (response->getVariable(7)->getType() != ASN_NO_SUCH_OBJECT))
         {
            _tcscpy(radio.name, _T("slot1"));
            radio.index = 1;
            radio.channel = response->getVariable(7)->getValueAsInt();
            switch(response->getVariable(9)->getValueAsInt())  // bsnAPIfType
            {
               case 1:  // dot11b
                  radio.band = RADIO_BAND_2_4_GHZ;
                  break;
               case 2:  // dot11a
                  radio.band = RADIO_BAND_5_GHZ;
                  break;
            }
            radio.frequency = WirelessChannelToFrequency(radio.band, radio.channel);
            ap->addRadioInterface(radio);
         }

         apList->add(ap);
      }
      delete response;
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
ObjectArray<AccessPointInfo> *CiscoWirelessControllerDriver::getAccessPoints(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   ObjectArray<AccessPointInfo> *apList = new ObjectArray<AccessPointInfo>(0, 16, Ownership::True);

   if (SnmpWalk(snmp, _T(".1.3.6.1.4.1.14179.2.2.1.1.33"),  // bsnAPEthernetMacAddress
                HandlerAccessPointList, apList) != SNMP_ERR_SUCCESS)
   {
      delete apList;
      return nullptr;
   }

   return apList;
}

/**
 * Handler for mobile units enumeration
 */
static uint32_t HandlerWirelessStationList(SNMP_Variable *var, SNMP_Transport *snmp, ObjectArray<WirelessStationInfo> *wsList)
{
   const SNMP_ObjectId& name = var->getName();
   size_t nameLen = name.length();
   uint32_t oid[MAX_OID_LEN];
   memcpy(oid, name.value(), nameLen * sizeof(UINT32));

   SNMP_PDU *request = new SNMP_PDU(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());

   oid[11] = 2; // bsnMobileStationIpAddress
   request->bindVariable(new SNMP_Variable(oid, nameLen));

   oid[11] = 4; // bsnMobileStationAPMacAddr
   request->bindVariable(new SNMP_Variable(oid, nameLen));

   oid[11] = 5; // bsnMobileStationAPIfSlotId
   request->bindVariable(new SNMP_Variable(oid, nameLen));

   oid[11] = 7; // bsnMobileStationSsid
   request->bindVariable(new SNMP_Variable(oid, nameLen));

   oid[11] = 29; // bsnMobileStationVlanId
   request->bindVariable(new SNMP_Variable(oid, nameLen));

   SNMP_PDU *response;
   uint32_t rcc = snmp->doRequest(request, &response);
	delete request;
   if (rcc == SNMP_ERR_SUCCESS)
   {
      if (response->getNumVariables() == 5)
      {
         WirelessStationInfo *info = new WirelessStationInfo;
         memset(info, 0, sizeof(WirelessStationInfo));

         var->getRawValue(info->macAddr, MAC_ADDR_LENGTH);
         TCHAR ipAddr[32];
         info->ipAddr = InetAddress::parse(response->getVariable(0)->getValueAsString(ipAddr, 32));
         response->getVariable(3)->getValueAsString(info->ssid, MAX_OBJECT_NAME);
         info->vlan = response->getVariable(4)->getValueAsInt();
         response->getVariable(1)->getRawValue(info->bssid, MAC_ADDR_LENGTH);
         info->rfIndex = response->getVariable(2)->getValueAsInt();
         info->apMatchPolicy = AP_MATCH_BY_BSSID;

         wsList->add(info);
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
ObjectArray<WirelessStationInfo> *CiscoWirelessControllerDriver::getWirelessStations(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   auto wsList = new ObjectArray<WirelessStationInfo>(0, 16, Ownership::True);

   if (SnmpWalk(snmp, _T(".1.3.6.1.4.1.14179.2.1.4.1.1"), // bsnMobileStationMacAddress
                HandlerWirelessStationList, wsList) != SNMP_ERR_SUCCESS)
   {
      delete wsList;
      return nullptr;
   }

   return wsList;
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
 * @param radioInterfaces list of radio interfaces for this AP
 * @return state of access point or AP_UNKNOWN if it cannot be determined
 */
AccessPointState CiscoWirelessControllerDriver::getAccessPointState(SNMP_Transport *snmp, NObject *node, DriverData *driverData,
      uint32_t apIndex, const MacAddress& macAddr, const InetAddress& ipAddr, const StructArray<RadioInterfaceInfo>& radioInterfaces)
{
   if (radioInterfaces.isEmpty())
      return AP_UNKNOWN;

   TCHAR oid[256], macAddrText[64];
   _sntprintf(oid, 256, _T(".1.3.6.1.4.1.14179.2.2.1.1.6.%s"),
            MacAddress(radioInterfaces.get(0)->bssid, MAC_ADDR_LENGTH).toString(macAddrText, MacAddressNotation::DECIMAL_DOT_SEPARATED));

   TCHAR buffer[32];
   uint32_t value;
   if (SnmpGet(snmp->getSnmpVersion(), snmp, oid, nullptr, 0, &value, sizeof(uint32_t), 0) != SNMP_ERR_SUCCESS)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("Cannot get access point [index=%u, mac=%s] status from OID %s"), apIndex, macAddr.toString(buffer), oid);
      return AP_UNKNOWN;
   }

   nxlog_debug_tag(DEBUG_TAG, 6, _T("Retrieved access point [index=%u, mac=%s] status %d"), apIndex, macAddr.toString(buffer), value);
   switch(value)
   {
      case 1:
         return AP_UP;
      case 2:
         return AP_UNPROVISIONED;
      default:
         return AP_UNKNOWN;
   }
}
