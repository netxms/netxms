/* 
** NetXMS - Network Management System
** Driver for IgniteNet devices
** Copyright (C) 2003-2017 Raden Solutions
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
** File: ignitenet.cpp
**/

#include "ignitenet.h"

/**
 * Driver name
 */
static TCHAR s_driverName[] = _T("IGNITENET");

/**
 * Driver version
 */
static TCHAR s_driverVersion[] = NETXMS_VERSION_STRING;

/**
 * Get driver name
 */
const TCHAR *IgniteNetDriver::getName()
{
	return s_driverName;
}

/**
 * Get driver version
 */
const TCHAR *IgniteNetDriver::getVersion()
{
	return s_driverVersion;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int IgniteNetDriver::isPotentialDevice(const TCHAR *oid)
{
	return !_tcscmp(oid, _T(".1.3.6.1.4.1.47307")) ? 254 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool IgniteNetDriver::isDeviceSupported(SNMP_Transport *snmp, const TCHAR *oid)
{
	return true;
}

/**
 * Get interface state. Both states must be set to UNKNOWN if cannot be read from device.
 *
 * @param snmp SNMP transport
 * @param attributes node's custom attributes
 * @param driverData driver's data
 * @param ifIndex interface index
 * @param adminState OUT: interface administrative state
 * @param operState OUT: interface operational state
 */
void IgniteNetDriver::getInterfaceState(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData, UINT32 ifIndex,
                                        int ifTableSuffixLen, UINT32 *ifTableSuffix, InterfaceAdminState *adminState, InterfaceOperState *operState)
{
   UINT32 state = 0;
   TCHAR oid[256], suffix[128];
   if (ifTableSuffixLen > 0)
      _sntprintf(oid, 256, _T(".1.3.6.1.2.1.2.2.1.7%s"), SNMPConvertOIDToText(ifTableSuffixLen, ifTableSuffix, suffix, 128)); // Interface administrative state
   else
      _sntprintf(oid, 256, _T(".1.3.6.1.2.1.2.2.1.7.%d"), (int)ifIndex); // Interface administrative state
   SnmpGet(snmp->getSnmpVersion(), snmp, oid, NULL, 0, &state, sizeof(UINT32), 0);

   // IgniteNet devices may not support interface administrative state reading through SNMP
   // Assume interface administratively UP
   if (state == 0)
      state = 1;

   switch(state)
   {
      case 2:
         *adminState = IF_ADMIN_STATE_DOWN;
         *operState = IF_OPER_STATE_DOWN;
         break;
      case 1:
      case 3:
         *adminState = (InterfaceAdminState)state;
         // Get interface operational state
         state = 0;
         if (ifTableSuffixLen > 0)
            _sntprintf(oid, 256, _T(".1.3.6.1.2.1.2.2.1.8%s"), SNMPConvertOIDToText(ifTableSuffixLen, ifTableSuffix, suffix, 128));
         else
            _sntprintf(oid, 256, _T(".1.3.6.1.2.1.2.2.1.8.%d"), (int)ifIndex);
         SnmpGet(snmp->getSnmpVersion(), snmp, oid, NULL, 0, &state, sizeof(UINT32), 0);
         switch(state)
         {
            case 3:
               *operState = IF_OPER_STATE_TESTING;
               break;
            case 2:  // down: interface is down
            case 7:  // lowerLayerDown: down due to state of lower-layer interface(s)
               *operState = IF_OPER_STATE_DOWN;
               break;
            case 1:
               *operState = IF_OPER_STATE_UP;
               break;
            default:
               *operState = IF_OPER_STATE_UNKNOWN;
               break;
         }
         break;
      default:
         *adminState = IF_ADMIN_STATE_UNKNOWN;
         *operState = IF_OPER_STATE_UNKNOWN;
         break;
   }
}

/**
 * Driver entry point
 */
DECLARE_NDD_ENTRY_POINT(s_driverName, IgniteNetDriver);

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
