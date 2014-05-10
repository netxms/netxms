/* 
** NetXMS - Network Management System
** Driver for Symbol WS series wireless switches
** Copyright (C) 2013 Raden Solutions
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
** File: symbol-ws.cpp
**/

#include "symbol-ws.h"

/**
 * Driver name
 */
static TCHAR s_driverName[] = _T("SYMBOL-WS");

/**
 * Driver version
 */
static TCHAR s_driverVersion[] = NETXMS_VERSION_STRING;

/**
 * Get driver name
 */
const TCHAR *SymbolDriver::getName()
{
   return s_driverName;
}

/**
 * Get driver version
 */
const TCHAR *SymbolDriver::getVersion()
{
   return s_driverVersion;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int SymbolDriver::isPotentialDevice(const TCHAR *oid)
{
   return (_tcsncmp(oid, _T(".1.3.6.1.4.1.388."), 17) == 0) ? 127 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool SymbolDriver::isDeviceSupported(SNMP_Transport *snmp, const TCHAR *oid)
{
   TCHAR buffer[1024];
   return SnmpGet(snmp->getSnmpVersion(), snmp, _T(".1.3.6.1.4.1.388.14.1.6.1.10.0"), NULL, 0, buffer, sizeof(buffer), 0) == SNMP_ERR_SUCCESS;
}

/**
 * Get cluster mode for device (standalone / active / standby)
 *
 * @param snmp SNMP transport
 * @param attributes Node custom attributes
 * @param driverData optional pointer to user data
 */
int SymbolDriver::getClusterMode(SNMP_Transport *snmp, StringMap *attributes, void *driverData)
{
   int ret = CLUSTER_MODE_UNKNOWN;

#define _GET(oid) \
   SnmpGet(snmp->getSnmpVersion(), snmp, oid, NULL, 0, &value, sizeof(UINT32), 0)

   UINT32 value = 0;
   if (_GET(_T(".1.3.6.1.4.1.388.14.1.8.1.2.0")) == SNMP_ERR_SUCCESS)
   {
      if (value == 1)
      {
         if (_GET(_T(".1.3.6.1.4.1.388.14.1.8.1.3.0")) == SNMP_ERR_SUCCESS)
         {
            if (value == 1)
            {
               ret = CLUSTER_MODE_ACTIVE;
            }
            else if (value == 2)
            {
               ret = CLUSTER_MODE_STANDBY;
            }

         }
         // enabled
      }
      else
      {
         ret = CLUSTER_MODE_STANDALONE;
      }
   }
#undef _GET

   return ret;
}


/*
 * Check switch for wireless capabilities
 *
 * @param snmp SNMP transport
 * @param attributes Node custom attributes
 * @param driverData optional pointer to user data
 */
bool SymbolDriver::isWirelessController(SNMP_Transport *snmp, StringMap *attributes, void *driverData)
{
   return true;
}


/**
 * Handler for access point enumeration - unadopted
 */
static UINT32 HandlerAccessPointListUnadopted(UINT32 version, SNMP_Variable *var, SNMP_Transport *transport, void *arg)
{
   ObjectArray<AccessPointInfo> *apList = (ObjectArray<AccessPointInfo> *)arg;

   SNMP_ObjectId *name = var->getName();
   size_t nameLen = name->getLength();
   UINT32 apIndex = name->getValue()[nameLen - 1];

   UINT32 oid[] = { 1, 3, 6, 1, 4, 1, 388, 14, 3, 2, 1, 9, 4, 1, 3, 0 };
   oid[(sizeof(oid) / sizeof(oid[0])) - 1] = apIndex;
   UINT32 type;
   if (SnmpGet(version, transport, NULL, oid, 16, &type, sizeof(UINT32), 0) != SNMP_ERR_SUCCESS)
   {
      type = -1;
   }

   const TCHAR *model;
   switch (type)
   {
      case 1:
         model = _T("AP100");
         break;
      case 2:
         model = _T("AP200");
         break;
      case 3:
         model = _T("AP300");
         break;
      case 4:
         model = _T("AP4131");
         break;
      default:
         model = _T("UNKNOWN");
         break;
   }

   AccessPointInfo *info = new AccessPointInfo((BYTE *)var->getValue(), AP_UNADOPTED, NULL, model, NULL);
   apList->add(info);

   return SNMP_ERR_SUCCESS;
}

/**
 * Handler for access point enumeration - adopted
 */
static UINT32 HandlerAccessPointListAdopted(UINT32 version, SNMP_Variable *var, SNMP_Transport *transport, void *arg)
{
   int ret = SNMP_ERR_SUCCESS;

   ObjectArray<AccessPointInfo> *apList = (ObjectArray<AccessPointInfo> *)arg;

   SNMP_ObjectId *name = var->getName();
   size_t nameLen = name->getLength();
   UINT32 apIndex = name->getValue()[nameLen - 1];

   UINT32 numberOfRadios;
   TCHAR serial[128];

   UINT32 oid[] = { 1, 3, 6, 1, 4, 1, 388, 14, 3, 2, 1, 9, 2, 1, 3, 0 }; // wsCcRfApModelNumber
   oid[(sizeof(oid) / sizeof(oid[0])) - 1] = apIndex;

   const TCHAR *model;
   //TCHAR model[128];
   // TODO: model=AP300, PN=WSAP-5100-050-WWR. Unadopted return only model, adopted return PN as model
   //       and model (AP300). Need to decide, where to put this info.
   // get AP model
   //ret = SnmpGet(version, transport, NULL, oid, sizeof(oid) / sizeof(oid[0]), &model, sizeof(model), 0);

   // get serial
   if (ret == SNMP_ERR_SUCCESS)
   {
      oid[(sizeof(oid) / sizeof(oid[0])) - 2] = 4; // 1.3.6.1.4.1.388.14.3.2.1.9.2.1.4.x, wsCcRfApSerialNumber
      ret = SnmpGet(version, transport, NULL, oid, sizeof(oid) / sizeof(oid[0]), &serial, sizeof(serial), SG_STRING_RESULT);
   }
   if (ret == SNMP_ERR_SUCCESS)
   {
      // get number of radios
      oid[(sizeof(oid) / sizeof(oid[0])) - 2] = 9; // 1.3.6.1.4.1.388.14.3.2.1.9.2.1.9.x, wsCcRfApNumRadios
      ret = SnmpGet(version, transport, NULL, oid, sizeof(oid) / sizeof(oid[0]), &numberOfRadios, sizeof(numberOfRadios), 0);
   }

   // load radios
   UINT32 radioIndex[2] = { 0, 0 };
   if (ret == SNMP_ERR_SUCCESS && numberOfRadios > 0)
   {
      oid[(sizeof(oid) / sizeof(oid[0])) - 2] = 10; // 1.3.6.1.4.1.388.14.3.2.1.9.2.1.10.x, wsCcRfApRadio1
      ret = SnmpGet(version, transport, NULL, oid, sizeof(oid) / sizeof(oid[0]),
            &(radioIndex[0]), sizeof(radioIndex[0]), 0);
   }
   if (ret == SNMP_ERR_SUCCESS && numberOfRadios > 1)
   {
      oid[(sizeof(oid) / sizeof(oid[0])) - 2] = 11; // 1.3.6.1.4.1.388.14.3.2.1.9.2.1.11.x, wsCcRfApRadio2
      ret = SnmpGet(version, transport, NULL, oid, sizeof(oid) / sizeof(oid[0]),
            &(radioIndex[1]), sizeof(radioIndex[1]), 0);
   }

   // get AP model
   UINT32 radioOid[] = { 1, 3, 6, 1, 4, 1, 388, 14, 3, 2, 1, 11,5, 1, 5, 0 }; // wsCcRfRadioApType
   UINT32 type;
   if (ret == SNMP_ERR_SUCCESS)
   {
      radioOid[(sizeof(radioOid) / sizeof(radioOid[0])) - 1] = radioIndex[0];
      ret = SnmpGet(version, transport, NULL, radioOid, sizeof(radioOid) / sizeof(radioOid[0]),
            &type, sizeof(type), 0);
   }

   if (ret == SNMP_ERR_SUCCESS)
   {
      switch (type)
      {
         case 1:
            model = _T("AP100");
            break;
         case 2:
            model = _T("AP200");
            break;
         case 3:
            model = _T("AP300");
            break;
         case 4:
            model = _T("AP4131");
            break;
         default:
            model = _T("UNKNOWN");
            break;
      }
   }

   AccessPointInfo *info;
   if (ret == SNMP_ERR_SUCCESS)
   {
      info = new AccessPointInfo((BYTE *)var->getValue(), AP_ADOPTED, NULL, model, serial);
      apList->add(info);
   }

   for (int i = 0; (i < (int)numberOfRadios) && (ret == SNMP_ERR_SUCCESS) && radioIndex[i] != 0; i++)
   {
      UINT32 descOid[] = { 1, 3, 6, 1, 4, 1, 388, 14, 3, 2, 1, 11, 5, 1, 3, 0 }; // wsCcRfRadioDescr
      UINT32 macOid[] = { 1, 3, 6, 1, 4, 1, 388, 14, 3, 2, 1, 11, 11, 1, 1, 0 }; // wsCcRfRadioStatusRadioMac
      UINT32 channelOid[] = { 1, 3, 6, 1, 4, 1, 388, 14, 3, 2, 1, 11, 11, 1, 6, 0 }; // wsCcRfRadioStatusCurrChannel
      UINT32 currentPowerDbOid[] = { 1, 3, 6, 1, 4, 1, 388, 14, 3, 2, 1, 11, 11, 1, 7, 0 }; // wsCcRfRadioStatusCurrPowerDb
      UINT32 currentPowerMwOid[] = { 1, 3, 6, 1, 4, 1, 388, 14, 3, 2, 1, 11, 11, 1, 8, 0 }; // wsCcRfRadioStatusCurrPowerMw
      descOid[(sizeof(descOid) / sizeof(UINT32)) - 1] = radioIndex[i];
      macOid[(sizeof(macOid) / sizeof(UINT32)) - 1] = radioIndex[i];
      channelOid[(sizeof(channelOid) / sizeof(UINT32)) - 1] = radioIndex[i];
      currentPowerDbOid[(sizeof(currentPowerDbOid) / sizeof(UINT32)) - 1] = radioIndex[i];
      currentPowerMwOid[(sizeof(currentPowerMwOid) / sizeof(UINT32)) - 1] = radioIndex[i];

      RadioInterfaceInfo *radioInfo = new RadioInterfaceInfo;
      radioInfo->index = radioIndex[i];
      
      // MAC
      if (ret == SNMP_ERR_SUCCESS)
      {
         ret = SnmpGet(version, transport, NULL, macOid, sizeof(macOid) / sizeof(macOid[0]),
               &radioInfo->macAddr, MAC_ADDR_LENGTH, SG_RAW_RESULT);
      }

      // Name
      if (ret == SNMP_ERR_SUCCESS)
      {
         ret = SnmpGet(version, transport, NULL, descOid, sizeof(descOid) / sizeof(descOid[0]),
               &radioInfo->name, sizeof(radioInfo->name), SG_STRING_RESULT);
      }

      // Channel
      if (ret == SNMP_ERR_SUCCESS)
      {
         ret = SnmpGet(version, transport, NULL, channelOid, sizeof(channelOid) / sizeof(channelOid[0]),
               &radioInfo->channel, sizeof(radioInfo->channel), 0);
      }

      // Transmitting power (in dBm)
      if (ret == SNMP_ERR_SUCCESS)
      {
         ret = SnmpGet(version, transport, NULL, currentPowerDbOid, sizeof(currentPowerDbOid) / sizeof(currentPowerDbOid[0]),
               &radioInfo->powerDBm, sizeof(radioInfo->powerDBm), 0);
      }

      // Transmitting power (in mW)
      if (ret == SNMP_ERR_SUCCESS)
      {
         ret = SnmpGet(version, transport, NULL, currentPowerMwOid, sizeof(currentPowerMwOid) / sizeof(currentPowerMwOid[0]),
               &radioInfo->powerMW, sizeof(radioInfo->powerMW), 0);
      }

      if (ret == SNMP_ERR_SUCCESS)
      {
         info->addRadioInterface(radioInfo);
      }
      else
      {
         delete radioInfo;
      }
   }

   return ret;
}

/**
 * Get access points
 *
 * @param snmp SNMP transport
 * @param attributes Node custom attributes
 * @param driverData optional pointer to user data
 */
ObjectArray<AccessPointInfo> *SymbolDriver::getAccessPoints(SNMP_Transport *snmp, StringMap *attributes, void *driverData)
{
   ObjectArray<AccessPointInfo> *apList = new ObjectArray<AccessPointInfo>(0, 16, true);

   // Adopted
   if (SnmpWalk(snmp->getSnmpVersion(), snmp, _T(".1.3.6.1.4.1.388.14.3.2.1.9.2.1.2"),
            HandlerAccessPointListAdopted, apList, FALSE) != SNMP_ERR_SUCCESS)
   {
      delete apList;
      return NULL;
   }

   // Unadopted
   if (SnmpWalk(snmp->getSnmpVersion(), snmp, _T(".1.3.6.1.4.1.388.14.3.2.1.9.4.1.2"),
            HandlerAccessPointListUnadopted, apList, FALSE) != SNMP_ERR_SUCCESS)
   {
      delete apList;
      return NULL;
   }

   return apList;
}

/**
 * Handler for mobile units enumeration
 */
static UINT32 HandlerWirelessStationList(UINT32 version, SNMP_Variable *var, SNMP_Transport *transport, void *arg)
{
   int ret = SNMP_ERR_SUCCESS;

   ObjectArray<WirelessStationInfo> *wsList = (ObjectArray<WirelessStationInfo> *)arg;

   SNMP_ObjectId *name = var->getName();
   size_t nameLen = name->getLength();
   const UINT32 *value = name->getValue();

   UINT32 oid[32];
   memcpy(oid, value, nameLen * sizeof(value[0]));

   UINT32 ipAddr;
   oid[14] = 6; // wsCcRfMuIpAddr
   ret = SnmpGet(version, transport, NULL, oid, sizeof(oid) / sizeof(oid[0]), &ipAddr, sizeof(ipAddr), 0);
   UINT32 vlanInfex;
   if (ret == SNMP_ERR_SUCCESS)
   {
      oid[14] = 4; // wsCcRfMuVlanIndex
      ret = SnmpGet(version, transport, NULL, oid, sizeof(oid) / sizeof(oid[0]), &vlanInfex, sizeof(vlanInfex), 0);
   }

   UINT32 wlanInfex;
   if (ret == SNMP_ERR_SUCCESS)
   {
      oid[14] = 2; // wsCcRfMuWlanIndex
      ret = SnmpGet(version, transport, NULL, oid, sizeof(oid) / sizeof(oid[0]), &wlanInfex, sizeof(vlanInfex), 0);
   }

   UINT32 rfIndex;
   if (ret == SNMP_ERR_SUCCESS)
   {
      oid[14] = 3; // wsCcRfMuRadioIndex
      ret = SnmpGet(version, transport, NULL, oid, sizeof(oid) / sizeof(oid[0]), &rfIndex, sizeof(rfIndex), 0);
   }

   TCHAR ssid[MAX_OBJECT_NAME];
   if (ret == SNMP_ERR_SUCCESS)
   {
      UINT32 wlanOid[] = { 1, 3, 6, 1, 4, 1, 388, 14, 3, 2, 1, 14, 1, 1, 4, 0 };
      wlanOid[(sizeof(wlanOid) / sizeof(wlanOid[0])) - 1] = wlanInfex;

      ret = SnmpGet(version, transport, NULL, wlanOid, sizeof(wlanOid) / sizeof(wlanOid[0]), ssid, sizeof(ssid), 0);
   }

   if (ret == SNMP_ERR_SUCCESS)
   {
      WirelessStationInfo *info = new WirelessStationInfo;

      var->getRawValue(info->macAddr, MAC_ADDR_LENGTH);
      info->ipAddr = ipAddr;
      info->vlan = vlanInfex;
      nx_strncpy(info->ssid, ssid, MAX_OBJECT_NAME);
      info->rfIndex = rfIndex;

      wsList->add(info);
   }

   return ret;
}

/**
 * Get registered wireless stations (clients)
 *
 * @param snmp SNMP transport
 * @param attributes Node custom attributes
 * @param driverData optional pointer to user data
 */
ObjectArray<WirelessStationInfo> *SymbolDriver::getWirelessStations(SNMP_Transport *snmp, StringMap *attributes, void *driverData)
{
   ObjectArray<WirelessStationInfo> *wsList = new ObjectArray<WirelessStationInfo>(0, 16, true);

   if (SnmpWalk(snmp->getSnmpVersion(), snmp, _T(".1.3.6.1.4.1.388.14.3.2.1.12.3.1.1"), // wsCcRfMuMac
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
DECLARE_NDD_ENTRY_POINT(s_driverName, SymbolDriver);

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
