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
 * Static data
 */
static TCHAR s_driverName[] = _T("SYMBOL-WS");
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
   return (_tcsncmp(oid, _T(".1.3.6.1.4.1.388.14."), 20) == 0) ? 127 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool SymbolDriver::isDeviceSupported(SNMP_Transport *snmp, const TCHAR *oid)
{
   return true;
}

/**
 * Do additional checks on the device required by driver.
 * Driver can set device's custom attributes from within
 * this function.
 *
 * @param snmp SNMP transport
 * @param attributes Node's custom attributes
 */
void SymbolDriver::analyzeDevice(SNMP_Transport *snmp, const TCHAR *oid, StringMap *attributes, void **driverData)
{
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
   SnmpGet(snmp->getSnmpVersion(), snmp, oid, NULL, 0, &value, sizeof(DWORD), 0)

   DWORD value = 0;
   if (_GET(_T(".1.3.6.1.4.1.388.14.1.8.1.2.0")) == SNMP_ERR_SUCCESS)
   {
      if (value == 1)
      {
         if (_GET(_T(".1.3.6.1.4.1.388.14.1.8.1.2.0")) == SNMP_ERR_SUCCESS)
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
 * Handler for access point enumeration
 */
static DWORD HandlerAccessPointList(DWORD version, SNMP_Variable *var, SNMP_Transport *transport, void *arg)
{
   ObjectArray<AccessPointInfo> *apList = (ObjectArray<AccessPointInfo> *)arg;

   SNMP_ObjectId *name = var->GetName();
   DWORD nameLen = name->getLength();
   DWORD apIndex = name->getValue()[nameLen - 1];
   bool adopted = name->getValue()[12] == 2; // 2 == adopted, 4 == unadopted

   if (!adopted)
   {
      DWORD oid[] = { 1, 3, 6, 1, 4, 1, 388, 14, 3, 2, 1, 9, 4, 1, 3, 0 };
      oid[15] = apIndex;
      DWORD type;
      if (SnmpGet(version, transport, NULL, oid, 16, &type, sizeof(DWORD), 0) != SNMP_ERR_SUCCESS)
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

      AccessPointInfo *info = new AccessPointInfo((BYTE *)var->GetValue(), AP_UNADOPTED, model, NULL);
      apList->add(info);
   }
   else
   {
   }

   return SNMP_ERR_SUCCESS;
}


/*
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
   if (SnmpWalk(snmp->getSnmpVersion(), snmp, _T(".1.3.6.1.4.1.388.14.3.2.1.9.2.1.2"), HandlerAccessPointList, apList, FALSE) != SNMP_ERR_SUCCESS)
   {
      delete apList;
      apList = NULL;
   }

   // Unadopted
   if (SnmpWalk(snmp->getSnmpVersion(), snmp, _T(".1.3.6.1.4.1.388.14.3.2.1.9.4.1.2"), HandlerAccessPointList, apList, FALSE) != SNMP_ERR_SUCCESS)
   {
      delete apList;
      apList = NULL;
   }

   return apList;
}

/*
 *
 * @param snmp SNMP transport
 * @param attributes Node custom attributes
 * @param driverData optional pointer to user data
 */
ObjectArray<WirelessStationInfo> *SymbolDriver::getWirelessStations(SNMP_Transport *snmp, StringMap *attributes, void *driverData)
{
   ObjectArray<WirelessStationInfo> *ret = new ObjectArray<WirelessStationInfo>(0, 16, true);

   return ret;
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
