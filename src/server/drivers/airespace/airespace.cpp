/* 
** NetXMS - Network Management System
** Driver for Cisco 4400 (former Airespace) wireless switches
** Copyright (C) 2013-2014 Raden Solutions
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
** File: airespace.cpp
**/

#include "airespace.h"

/**
 * Driver name
 */
static TCHAR s_driverName[] = _T("AIRESPACE");

/**
 * Driver version
 */
static TCHAR s_driverVersion[] = NETXMS_VERSION_STRING;

/**
 * Get driver name
 */
const TCHAR *AirespaceDriver::getName()
{
   return s_driverName;
}

/**
 * Get driver version
 */
const TCHAR *AirespaceDriver::getVersion()
{
   return s_driverVersion;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int AirespaceDriver::isPotentialDevice(const TCHAR *oid)
{
   return (_tcsncmp(oid, _T(".1.3.6.1.4.1.14179."), 19) == 0) ? 127 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool AirespaceDriver::isDeviceSupported(SNMP_Transport *snmp, const TCHAR *oid)
{
   // read agentInventoryMachineModel
   TCHAR buffer[1024];
   if (SnmpGet(snmp->getSnmpVersion(), snmp, _T(".1.3.6.1.4.1.14179.1.1.1.3.0"), NULL, 0, buffer, sizeof(buffer), 0) != SNMP_ERR_SUCCESS)
      return false;

   // model must start with AIR-WLC44, like AIR-WLC4402-50-K9
   return _tcsncmp(buffer, _T("AIR-WLC44"), 9) == 0;
}

/**
 * Get cluster mode for device (standalone / active / standby)
 *
 * @param snmp SNMP transport
 * @param attributes Node custom attributes
 * @param driverData optional pointer to user data
 */
int AirespaceDriver::getClusterMode(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData)
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
bool AirespaceDriver::isWirelessController(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData)
{
   return true;
}

/**
 * Handler for access point enumeration
 */
static UINT32 HandlerAccessPointList(UINT32 version, SNMP_Variable *var, SNMP_Transport *snmp, void *arg)
{
   ObjectArray<AccessPointInfo> *apList = (ObjectArray<AccessPointInfo> *)arg;

   SNMP_ObjectId *name = var->getName();
   size_t nameLen = name->getLength();
   UINT32 oid[MAX_OID_LEN];
   memcpy(oid, name->getValue(), nameLen * sizeof(UINT32));

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

   SNMP_PDU *response;
   UINT32 rcc = snmp->doRequest(request, &response, g_dwSNMPTimeout, 3);
	delete request;
   if (rcc == SNMP_ERR_SUCCESS)
   {
      if (response->getNumVariables() == 8)
      {
         BYTE macAddr[16];
         memset(macAddr, 0, sizeof(macAddr));
         var->getRawValue(macAddr, sizeof(macAddr));

         TCHAR ipAddr[32], name[MAX_OBJECT_NAME], model[MAX_OBJECT_NAME], serial[MAX_OBJECT_NAME];
         AccessPointInfo *ap = 
            new AccessPointInfo(
               macAddr,
               ntohl(_t_inet_addr(response->getVariable(1)->getValueAsString(ipAddr, 32))),
               (response->getVariable(2)->getValueAsInt() == 1) ? AP_ADOPTED : AP_UNADOPTED,
               response->getVariable(3)->getValueAsString(name, MAX_OBJECT_NAME),
               _T("Cisco"),   // vendor
               response->getVariable(4)->getValueAsString(model, MAX_OBJECT_NAME),
               response->getVariable(5)->getValueAsString(serial, MAX_OBJECT_NAME));

         RadioInterfaceInfo radio;
         memset(&radio, 0, sizeof(RadioInterfaceInfo));
         _tcscpy(radio.name, _T("slot0"));
         radio.index = 0;
         response->getVariable(0)->getRawValue(radio.macAddr, MAC_ADDR_LENGTH);
         radio.channel = response->getVariable(6)->getValueAsInt();
         ap->addRadioInterface(&radio);

         if ((response->getVariable(7)->getType() != ASN_NO_SUCH_INSTANCE) && (response->getVariable(7)->getType() != ASN_NO_SUCH_OBJECT))
         {
            _tcscpy(radio.name, _T("slot1"));
            radio.index = 1;
            radio.channel = response->getVariable(7)->getValueAsInt();
            ap->addRadioInterface(&radio);
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
ObjectArray<AccessPointInfo> *AirespaceDriver::getAccessPoints(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData)
{
   ObjectArray<AccessPointInfo> *apList = new ObjectArray<AccessPointInfo>(0, 16, true);

   if (SnmpWalk(snmp->getSnmpVersion(), snmp, _T(".1.3.6.1.4.1.14179.2.2.1.1.33"),  // bsnAPEthernetMacAddress
            HandlerAccessPointList, apList, FALSE) != SNMP_ERR_SUCCESS)
   {
      delete apList;
      return NULL;
   }

   return apList;
}

/**
 * Handler for mobile units enumeration
 */
static UINT32 HandlerWirelessStationList(UINT32 version, SNMP_Variable *var, SNMP_Transport *snmp, void *arg)
{
   ObjectArray<WirelessStationInfo> *wsList = (ObjectArray<WirelessStationInfo> *)arg;

   SNMP_ObjectId *name = var->getName();
   size_t nameLen = name->getLength();
   UINT32 oid[MAX_OID_LEN];
   memcpy(oid, name->getValue(), nameLen * sizeof(UINT32));

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
   UINT32 rcc = snmp->doRequest(request, &response, g_dwSNMPTimeout, 3);
	delete request;
   if (rcc == SNMP_ERR_SUCCESS)
   {
      if (response->getNumVariables() == 5)
      {
         WirelessStationInfo *info = new WirelessStationInfo;
         memset(info, 0, sizeof(WirelessStationInfo));

         var->getRawValue(info->macAddr, MAC_ADDR_LENGTH);
         TCHAR ipAddr[32];
         info->ipAddr = ntohl(_t_inet_addr(response->getVariable(0)->getValueAsString(ipAddr, 32)));
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
ObjectArray<WirelessStationInfo> *AirespaceDriver::getWirelessStations(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData)
{
   ObjectArray<WirelessStationInfo> *wsList = new ObjectArray<WirelessStationInfo>(0, 16, true);

   if (SnmpWalk(snmp->getSnmpVersion(), snmp, _T(".1.3.6.1.4.1.14179.2.1.4.1.1"), // bsnMobileStationMacAddress
            HandlerWirelessStationList, wsList, FALSE) != SNMP_ERR_SUCCESS)
   {
      delete wsList;
      wsList = NULL;
   }

   return wsList;
}

/**
 * Driver entry point
 */
DECLARE_NDD_ENTRY_POINT(s_driverName, AirespaceDriver);

/**
 * DLL entry point
 */
#ifdef _WIN32

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
   {
      DisableThreadLibraryCalls(hInstance);
   }

   return TRUE;
}

#endif
