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
int ArubaSwitchDriver::isPotentialDevice(const TCHAR *oid)
{
   return (_tcsncmp(oid, _T(".1.3.6.1.4.1.14823.1.1."), 23) == 0) ? 255 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool ArubaSwitchDriver::isDeviceSupported(SNMP_Transport *snmp, const TCHAR *oid)
{
   TCHAR model[256];
   if (SnmpGetEx(snmp, _T(".1.3.6.1.4.1.14823.2.2.1.2.1.3.0"), nullptr, 0, model, sizeof(model), SG_STRING_RESULT) != SNMP_ERR_SUCCESS)
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
   return SnmpGetEx(snmp, _T(".1.3.6.1.4.1.14823.2.2.1.1.3.1.0"), nullptr, 0, buffer, sizeof(buffer), SG_RAW_RESULT) == SNMP_ERR_SUCCESS;
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

   oid[14] = 5;   // apIpAddress
   request.bindVariable(new SNMP_Variable(oid, nameLen));

   oid[11] = 8;  // apCurrentChannel
   request.bindVariable(new SNMP_Variable(oid, nameLen));

   SNMP_PDU *response;
   uint32_t rcc = snmp->doRequest(&request, &response);
   if (rcc == SNMP_ERR_SUCCESS)
   {
      if (response->getNumVariables() == 2)
      {
         BYTE bssid[MAC_ADDR_LENGTH];
         for(int i = 0; i < MAC_ADDR_LENGTH; i++)
            bssid[i] = static_cast<BYTE>(oid[i + 15]);

         TCHAR ipAddr[32], name[32];
         AccessPointInfo *ap =
            new AccessPointInfo(
               0,
               MacAddress(bssid, MAC_ADDR_LENGTH),
               InetAddress::parse(response->getVariable(0)->getValueAsString(ipAddr, 32)),
               AP_ADOPTED,
               BinToStrEx(bssid, MAC_ADDR_LENGTH, name, '-', 0),
               _T("HPE Aruba Networking"),   // vendor
               _T(""), // model
               _T("")); // serial

         RadioInterfaceInfo radio;
         memset(&radio, 0, sizeof(RadioInterfaceInfo));
         var->getValueAsString(radio.name, 64);
         radio.index = 0;
         memcpy(radio.bssid, bssid, MAC_ADDR_LENGTH);
         radio.channel = response->getVariable(1)->getValueAsInt();
         ap->addRadioInterface(radio);

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
ObjectArray<AccessPointInfo> *ArubaSwitchDriver::getAccessPoints(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   auto apList = new ObjectArray<AccessPointInfo>(0, 16, Ownership::True);

   if (SnmpWalk(snmp, _T(".1.3.6.1.4.1.14823.2.2.1.1.3.3.1.2"), HandlerAccessPointList, apList) != SNMP_ERR_SUCCESS) // apESSID
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
   return AP_ADOPTED;
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
