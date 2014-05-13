/* 
** NetXMS - Network Management System
** Driver for Netscreen firewalls
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
** File: mikrotik.cpp
**/

#include "mikrotik.h"

/**
 * Driver name
 */
static TCHAR s_driverName[] = _T("MIKROTIK");

/**
 * Driver version
 */
static TCHAR s_driverVersion[] = NETXMS_VERSION_STRING;

/**
 * Get driver name
 */
const TCHAR *MikrotikDriver::getName()
{
	return s_driverName;
}

/**
 * Get driver version
 */
const TCHAR *MikrotikDriver::getVersion()
{
	return s_driverVersion;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int MikrotikDriver::isPotentialDevice(const TCHAR *oid)
{
	return !_tcscmp(oid, _T(".1.3.6.1.4.1.14988.1")) ? 254 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool MikrotikDriver::isDeviceSupported(SNMP_Transport *snmp, const TCHAR *oid)
{
	return true;
}

/**
 * Get list of interfaces for given node
 *
 * @param snmp SNMP transport
 * @param attributes Node's custom attributes
 */
InterfaceList *MikrotikDriver::getInterfaces(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData, int useAliases, bool useIfXTable)
{
	// Get interface list from standard MIB
	InterfaceList *ifList = NetworkDeviceDriver::getInterfaces(snmp, attributes, driverData, 0, false);
	if (ifList == NULL)
		return NULL;

   for(int i = 0; i < ifList->getSize(); i++)
   {
      NX_INTERFACE_INFO *iface = ifList->get(i);
      if (iface->dwType == IFTYPE_ETHERNET_CSMACD)
      {
         iface->isPhysicalPort = true;
         iface->dwSlotNumber = 1;
         iface->dwPortNumber = iface->dwIndex;
      }
   }
	return ifList;
}

/**
 * SNMP walker callback which just counts number of varbinds
 */
static UINT32 CountingSnmpWalkerCallback(UINT32 version, SNMP_Variable *var, SNMP_Transport *transport, void *arg)
{
	(*((int *)arg))++;
	return SNMP_ERR_SUCCESS;
}

/**
 * Check switch for wireless capabilities
 *
 * @param snmp SNMP transport
 * @param attributes Node custom attributes
 * @param driverData optional pointer to user data
 */
bool MikrotikDriver::isWirelessController(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData)
{
   int count = 0;
   SnmpWalk(snmp->getSnmpVersion(), snmp, _T(".1.3.6.1.4.1.14988.1.1.1.3.1.4"), CountingSnmpWalkerCallback, &count, FALSE);
   return count > 0;
}

/**
 * 2.4 GHz (802.11b/g/n) frequency to channel number map
 */
static int s_frequencyMap[14] = { 2412, 2417, 2422, 2427, 2432, 2437, 2442, 2447, 2452, 2457, 2462, 2467, 2472, 2484 };

/**
 * Handler for access point enumeration - adopted
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
   
   oid[nameLen - 2] = 5;   // mtxrWlApBssid
   request->bindVariable(new SNMP_Variable(oid, nameLen));

   oid[nameLen - 2] = 7;   // mtxrWlApFreq
   request->bindVariable(new SNMP_Variable(oid, nameLen));

   SNMPParseOID(_T(".1.3.6.1.2.1.2.2.1.2"), oid, MAX_OID_LEN); // ifDescr
   oid[10] = apIndex;
   request->bindVariable(new SNMP_Variable(oid, 11));

   oid[9] = 6; // ifPhysAddress
   request->bindVariable(new SNMP_Variable(oid, 11));

   SNMP_PDU *response;
   UINT32 rcc = snmp->doRequest(request, &response, g_dwSNMPTimeout, 3);
	delete request;
   if (rcc == SNMP_ERR_SUCCESS)
   {
      if (response->getNumVariables() == 4)
      {
         BYTE macAddr[16];
         memset(macAddr, 0, sizeof(macAddr));
         response->getVariable(0)->getRawValue(macAddr, sizeof(macAddr));

         // At least some Mikrotik devices returns empty BSSID - use radio interface MAC in that case
         if (!memcmp(macAddr, "\x00\x00\x00\x00\x00\x00", 6))
            response->getVariable(3)->getRawValue(macAddr, MAC_ADDR_LENGTH);

         TCHAR name[MAX_OBJECT_NAME];
         AccessPointInfo *ap = new AccessPointInfo(macAddr, AP_ADOPTED, var->getValueAsString(name, MAX_OBJECT_NAME), _T(""), _T(""));
      
         RadioInterfaceInfo radio;
         memset(&radio, 0, sizeof(RadioInterfaceInfo));
         response->getVariable(2)->getValueAsString(radio.name, 64);
         response->getVariable(3)->getRawValue(radio.macAddr, MAC_ADDR_LENGTH);
         radio.index = apIndex;

         int freq = response->getVariable(1)->getValueAsInt();
         for(int i = 0; i < 14; i++)
         {
            if (s_frequencyMap[i] == freq)
            {
               radio.channel = i + 1;
               break;
            }
         }

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
ObjectArray<AccessPointInfo> *MikrotikDriver::getAccessPoints(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData)
{
   ObjectArray<AccessPointInfo> *apList = new ObjectArray<AccessPointInfo>(0, 16, true);
   if (SnmpWalk(snmp->getSnmpVersion(), snmp, _T(".1.3.6.1.4.1.14988.1.1.1.3.1.4"),
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
   var->getRawValue(info->macAddr, MAC_ADDR_LENGTH);
   info->ipAddr = 0;
   info->vlan = 1;
   info->rfIndex = apIndex;

   TCHAR oid[256];
   _sntprintf(oid, 256, _T(".1.3.6.1.4.1.14988.1.1.1.3.1.4.%u"), apIndex);
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
ObjectArray<WirelessStationInfo> *MikrotikDriver::getWirelessStations(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData)
{
   ObjectArray<WirelessStationInfo> *wsList = new ObjectArray<WirelessStationInfo>(0, 16, true);
   if (SnmpWalk(snmp->getSnmpVersion(), snmp, _T(".1.3.6.1.4.1.14988.1.1.1.2.1.1"), // mtxrWlRtabAddr
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
DECLARE_NDD_ENTRY_POINT(s_driverName, MikrotikDriver);

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
