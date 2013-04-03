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
 * Get list of interfaces for given node
 *
 * @param snmp SNMP transport
 * @param attributes Node's custom attributes
 */
InterfaceList *SymbolDriver::getInterfaces(SNMP_Transport *snmp, StringMap *attributes, void *driverData, int useAliases, bool useIfXTable)
{
  // Get interface list from standard MIB
  InterfaceList *ifList = NetworkDeviceDriver::getInterfaces(snmp, attributes, driverData, useAliases, useIfXTable);
  if (ifList == NULL)
  {
    return NULL;
  }

  return ifList;
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

/*
 * Get access points
 *
 * @param snmp SNMP transport
 * @param attributes Node custom attributes
 * @param driverData optional pointer to user data
 */
ObjectArray<AccessPointInfo> *SymbolDriver::getAccessPoints(SNMP_Transport *snmp, StringMap *attributes, void *driverData)
{
   return NULL;
}

/*
 *
 * @param snmp SNMP transport
 * @param attributes Node custom attributes
 * @param driverData optional pointer to user data
 */
ObjectArray<WirelessStationInfo> *SymbolDriver::getWirelessStations(SNMP_Transport *snmp, StringMap *attributes, void *driverData)
{
   return NULL;
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
