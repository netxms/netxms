/* 
** NetXMS - Network Management System
** Driver for Ubiquity Networks access points
** Copyright (C) 2003-2014 Victor Kirhenshtein
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
** File: ubnt.cpp
**/

#include "ubnt.h"

/**
 * Driver name
 */
static TCHAR s_driverName[] = _T("UBNT");

/**
 * Driver version
 */
static TCHAR s_driverVersion[] = NETXMS_VERSION_STRING;

/**
 * Get driver name
 */
const TCHAR *UbiquityNetworksDriver::getName()
{
	return s_driverName;
}

/**
 * Get driver version
 */
const TCHAR *UbiquityNetworksDriver::getVersion()
{
	return s_driverVersion;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int UbiquityNetworksDriver::isPotentialDevice(const TCHAR *oid)
{
	return !_tcscmp(oid, _T(".1.3.6.1.4.1.10002.1")) ? 254 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool UbiquityNetworksDriver::isDeviceSupported(SNMP_Transport *snmp, const TCHAR *oid)
{
	return true;
}

/**
 * Check switch for wireless capabilities
 *
 * @param snmp SNMP transport
 * @param attributes Node custom attributes
 * @param driverData optional pointer to user data
 */
bool UbiquityNetworksDriver::isWirelessController(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData)
{
   return true;
}

/**
 * Parse MAC address. Could be without separators or with any separator char.
 */
static bool ParseMACAddress(const TCHAR *text, BYTE *mac)
{
   bool withSeparator = false;
   char separator = 0;
   int p = 0;
   bool hi = true;
   int length = (int)_tcslen(text);
   for(int i = 0; (i < length) && (p < MAC_ADDR_LENGTH); i++)
   {
      char c = toupper((char)text[i]); // use (char) here because we expect only ASCII characters
      if ((i % 3 == 2) && withSeparator)
      {
         if (c != separator)
            return false;
         continue;
      }
      if (!isdigit(c) && ((c < 'A') || (c > 'F')))
      {
         if (i == 2)
         {
            withSeparator = true;
            separator = c;
            continue;
         }
         return false;
      }
      if (hi)
      {
         mac[p] = (isdigit(c) ? (c - '0') : (c - 'A' + 10)) << 4;
         hi = false;
      }
      else
      {
         mac[p] |= (isdigit(c) ? (c - '0') : (c - 'A' + 10));
         p++;
         hi = true;
      }
   }
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
   UINT32 apIndex = oid[nameLen - 1];

   SNMP_PDU *request = new SNMP_PDU(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
   
   oid[nameLen - 2] = 9;   // dot11DesiredSSID
   request->bindVariable(new SNMP_Variable(oid, nameLen));

   SNMPParseOID(_T(".1.3.6.1.2.1.2.2.1.2"), oid, MAX_OID_LEN); // ifDescr
   oid[10] = apIndex;
   request->bindVariable(new SNMP_Variable(oid, 11));

   oid[9] = 6; // ifPhysAddress
   request->bindVariable(new SNMP_Variable(oid, 11));

   SNMP_PDU *response;
   UINT32 rcc = snmp->doRequest(request, &response, g_snmpTimeout, 3);
	delete request;
   if (rcc == SNMP_ERR_SUCCESS)
   {
      if (response->getNumVariables() == 3)
      {
         TCHAR macAddrText[32];
         var->getValueAsString(macAddrText, 32);
         
         BYTE macAddr[16];
         memset(macAddr, 0, sizeof(macAddr));
         ParseMACAddress(macAddrText, macAddr);

         TCHAR name[MAX_OBJECT_NAME];
         AccessPointInfo *ap = new AccessPointInfo(apIndex, macAddr, InetAddress::INVALID, AP_ADOPTED, response->getVariable(0)->getValueAsString(name, MAX_OBJECT_NAME), NULL, NULL, NULL);
      
         RadioInterfaceInfo radio;
         memset(&radio, 0, sizeof(RadioInterfaceInfo));
         response->getVariable(1)->getValueAsString(radio.name, 64);
         response->getVariable(2)->getRawValue(radio.macAddr, MAC_ADDR_LENGTH);
         radio.index = apIndex;

         ap->addRadioInterface(&radio);
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
ObjectArray<AccessPointInfo> *UbiquityNetworksDriver::getAccessPoints(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData)
{
   ObjectArray<AccessPointInfo> *apList = new ObjectArray<AccessPointInfo>(0, 16, true);
   if (SnmpWalk(snmp->getSnmpVersion(), snmp, _T(".1.2.840.10036.1.1.1.1"),   // dot11StationID
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
   UINT32 apIndex = name->getValue()[name->getLength() - 1];

   WirelessStationInfo *info = new WirelessStationInfo;
   memset(info, 0, sizeof(WirelessStationInfo));
   for(int i = 0; i < MAC_ADDR_LENGTH; i++)
      info->macAddr[i] = name->getValue()[i + 13];
   info->ipAddr = 0;
   info->vlan = 1;
   info->signalStrength = var->getValueAsInt();
   info->rfIndex = apIndex;
   info->apMatchPolicy = AP_MATCH_BY_RFINDEX;

   TCHAR oid[256];
   _sntprintf(oid, 256, _T(".1.2.840.10036.1.1.1.9.%u"), apIndex);
   if (SnmpGet(snmp->getSnmpVersion(), snmp, oid, NULL, 0, info->ssid, sizeof(info->ssid), SG_STRING_RESULT) != SNMP_ERR_SUCCESS)
   {
      info->ssid[0] = 0;
   }

   wsList->add(info);
   return SNMP_ERR_SUCCESS;
}

/**
 * Get registered wireless stations (clients)
 *
 * @param snmp SNMP transport
 * @param attributes Node custom attributes
 * @param driverData optional pointer to user data
 */
ObjectArray<WirelessStationInfo> *UbiquityNetworksDriver::getWirelessStations(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData)
{
   ObjectArray<WirelessStationInfo> *wsList = new ObjectArray<WirelessStationInfo>(0, 16, true);
   if (SnmpWalk(snmp->getSnmpVersion(), snmp, _T(".1.3.6.1.4.1.14988.1.1.1.2.1.3"), // mtxrWlRtabStrength
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
DECLARE_NDD_ENTRY_POINT(s_driverName, UbiquityNetworksDriver);

/**
 * DLL entry point
 */
#ifdef _WIN32

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
		DisableThreadLibraryCalls(hInstance);
	return TRUE;
}

#endif
