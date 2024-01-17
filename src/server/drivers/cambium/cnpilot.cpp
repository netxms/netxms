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
int CambiumCnPilotDriver::isPotentialDevice(const TCHAR *oid)
{
   return !_tcsncmp(oid, _T(".1.3.6.1.4.1.17713.22."), 22) ? 200 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool CambiumCnPilotDriver::isDeviceSupported(SNMP_Transport *snmp, const TCHAR *oid)
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
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.17713.22.1.1.1.8.0")));  // cambiumAPSWVersion
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.17713.22.1.1.1.4.0")));  // cambiumAPSerialNum
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.17713.22.1.1.1.5.0")));  // cambiumAPModel

   SNMP_PDU *response;
   if (snmp->doRequest(&request, &response) != SNMP_ERR_SUCCESS)
      return false;

   if (response->getNumVariables() != request.getNumVariables())
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Malformed device response in CambiumCnPilotDriver::getHardwareInformation"));
      delete response;
      return false;
   }

   _tcscpy(hwInfo->vendor, _T("Cambium"));

   response->getVariable(0)->getValueAsString(hwInfo->productVersion, sizeof(hwInfo->productVersion) / sizeof(TCHAR));
   response->getVariable(1)->getValueAsString(hwInfo->serialNumber, sizeof(hwInfo->serialNumber) / sizeof(TCHAR));
   response->getVariable(2)->getValueAsString(hwInfo->productName, sizeof(hwInfo->productName) / sizeof(TCHAR));

   delete response;
   return true;
}

/**
 * Returns true if device is a wireless controller.
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @return true if device is a wireless controller
 */
bool CambiumCnPilotDriver::isWirelessController(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   return true;
}

/**
 * Handler for SSID enumeration
 */
static UINT32 HandlerSSIDList(SNMP_Variable *var, SNMP_Transport *snmp, void *arg)
{
   TCHAR ssid[128];
   var->getValueAsString(ssid, 128);
   if (ssid[0] != 0)
   {
      static_cast<ObjectArray<AccessPointInfo>*>(arg)->add(new AccessPointInfo(var->getName().getLastElement() + 1, MacAddress::ZERO, InetAddress::INVALID, AP_ADOPTED, ssid, nullptr, nullptr, nullptr));
   }
   return SNMP_ERR_SUCCESS;
}

/**
 * Handler for radio interfaces enumeration
 */
static UINT32 HandlerRadioInterfaces(SNMP_Variable *var, SNMP_Transport *snmp, void *arg)
{
   SNMP_ObjectId oid = var->getName();
   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());

   oid.changeElement(11, 6);  // cambiumRadioChannel
   request.bindVariable(new SNMP_Variable(oid));

   oid.changeElement(11, 8);  // cambiumRadioTransmitPower
   request.bindVariable(new SNMP_Variable(oid));

   oid.changeElement(11, 4);  // cambiumRadioWlan
   request.bindVariable(new SNMP_Variable(oid));

   SNMP_PDU *response;
   uint32_t rcc = snmp->doRequest(&request, &response);
   if (rcc != SNMP_ERR_SUCCESS)
      return rcc;

   if (response->getNumVariables() != request.getNumVariables())
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Malformed device response in CambiumCnPilotDriver::getAccessPoints"));
      delete response;
      return SNMP_ERR_SUCCESS;
   }

   RadioInterfaceInfo ri;
   memset(&ri, 0, sizeof(ri));
   ri.index = oid.getLastElement() + 1;
   TCHAR macAddrText[64] = _T("");
   memcpy(ri.macAddr, MacAddress::parse(var->getValueAsString(macAddrText, 64)).value(), MAC_ADDR_LENGTH);
   _sntprintf(ri.name, sizeof(ri.name) / sizeof(TCHAR), _T("radio%u"), ri.index - 1);
   ri.channel = _tcstol(response->getVariable(0)->getValueAsString(macAddrText, 64), nullptr, 10);
   ri.powerDBm = response->getVariable(1)->getValueAsInt();
   ri.powerMW = (int)pow(10.0, (double)ri.powerDBm / 10.0);

   int32_t wlanIndex = response->getVariable(2)->getValueAsInt();
   auto apList = static_cast<ObjectArray<AccessPointInfo>*>(arg);
   for(int i = 0; i < apList->size(); i++)
   {
      AccessPointInfo *ap = apList->get(i);
      if (ap->getIndex() == wlanIndex)
      {
         ap->addRadioInterface(ri);
         if (ap->getMacAddr().isNull())
            ap->setMacAddr(MacAddress(ri.macAddr, MAC_ADDR_LENGTH));
         break;
      }
   }

   delete response;
   return SNMP_ERR_SUCCESS;
}

/**
 * Get list of wireless access points managed by this controller. Default implementation always return NULL.
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @return list of access points
 */
ObjectArray<AccessPointInfo> *CambiumCnPilotDriver::getAccessPoints(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   auto apList = new ObjectArray<AccessPointInfo>(0, 16, Ownership::True);
   if (SnmpWalk(snmp, _T(".1.3.6.1.4.1.17713.22.1.4.1.2"), HandlerSSIDList, apList) != SNMP_ERR_SUCCESS) // cambiumWlanSsid
   {
      delete apList;
      return nullptr;
   }

   if (SnmpWalk(snmp, _T(".1.3.6.1.4.1.17713.22.1.2.1.2"), HandlerRadioInterfaces, apList) != SNMP_ERR_SUCCESS)  // cambiumRadioMACAddress
   {
      delete apList;
      return nullptr;
   }

   return apList;
}

/**
 * Handler for wireless station enumeration
 */
static UINT32 HandlerWirelessStationList(SNMP_Variable *var, SNMP_Transport *snmp, void *arg)
{
   auto wsList = static_cast<ObjectArray<WirelessStationInfo>*>(arg);

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
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Malformed device response in CambiumEPMPDriver::getWirelessStations"));
      delete response;
      return SNMP_ERR_SUCCESS;
   }

   auto info = new WirelessStationInfo;
   memset(info, 0, sizeof(WirelessStationInfo));

   TCHAR addrText[64];
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
   if (SnmpWalk(snmp, _T(".1.3.6.1.4.1.17713.22.1.3.1.2"), HandlerWirelessStationList, wsList) != SNMP_ERR_SUCCESS)  // cambiumClientMACAddress
   {
      delete wsList;
      wsList = nullptr;
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
 * @param macAddr access point MAC address
 * @param ipAddr access point IP address
 * @param radioInterfaces list of radio interfaces for this AP
 * @return state of access point or AP_UNKNOWN if it cannot be determined
 */
AccessPointState CambiumCnPilotDriver::getAccessPointState(SNMP_Transport *snmp, NObject *node, DriverData *driverData, uint32_t apIndex,
         const MacAddress& macAddr, const InetAddress& ipAddr, const StructArray<RadioInterfaceInfo>& radioInterfaces)
{
   return AP_ADOPTED;
}
