/*
** NetXMS - Network Management System
** Driver for Qtech OLT switches
** Copyright (C) 2003-2014 Procyshin Dmitriy
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
** File: qtech-olt.cpp
**/

#include "qtech-olt.h"

/**
 * Driver name
 */
static TCHAR s_driverName[] = _T("QTECH-OLT");

/**
 * Driver version
 */
static TCHAR s_driverVersion[] = NETXMS_VERSION_STRING;

/**
 * Get driver name
 */
const TCHAR *QtechOLTDriver::getName()
{
	return s_driverName;
}

/**
 * Get driver version
 */
const TCHAR *QtechOLTDriver::getVersion()
{
	return s_driverVersion;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int QtechOLTDriver::isPotentialDevice(const TCHAR *oid)
{
	return !_tcscmp(oid, _T(".1.3.6.1.4.1.27514.1.10.4.1")) ? 254 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool QtechOLTDriver::isDeviceSupported(SNMP_Transport *snmp, const TCHAR *oid)
{
	return true;
}

/**
 * Handler for enumerating indexes
 */
static UINT32 HandlerIndex(UINT32 dwVersion, SNMP_Variable *pVar, SNMP_Transport *pTransport, void *pArg)
{
    UINT32 oid[128];
    size_t oidLen = pVar->getName()->getLength();
    memcpy(oid, pVar->getName()->getValue(), oidLen * sizeof(UINT32));

    InterfaceInfo *info = new InterfaceInfo(pVar->getValueAsUInt() + oid[14] * 1000);
    info->slot = oid[14];
    info->port = info->index;
    info->isPhysicalPort = true;
    ((InterfaceList *)pArg)->add(info);
    return SNMP_ERR_SUCCESS;
}

/**
 * Get list of interfaces for given node
 *
 * @param snmp SNMP transport
 * @param attributes Node's custom attributes
 */
InterfaceList *QtechOLTDriver::getInterfaces(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData, int useAliases, bool useIfXTable)
{
   InterfaceList *pIfList = NULL;
   TCHAR szOid[128];
   UINT32 oid[MAX_OID_LEN];
   pIfList = new InterfaceList(128*8);

   pIfList = NetworkDeviceDriver::getInterfaces(snmp, attributes, driverData, 0, false);

   if (SnmpWalk(snmp->getSnmpVersion(), snmp, _T(".1.3.6.1.4.1.27514.1.11.4.1.1.1"), HandlerIndex, pIfList, FALSE) == SNMP_ERR_SUCCESS)
   {
      for(int i = 0; i < pIfList->size(); i++)
      {
         InterfaceInfo *iface = pIfList->get(i);
         if (iface->index > 1000)
         {
            SNMPParseOID(_T(".1.3.6.1.4.1.27514.1.11.4.1.1.0.0.0.0"), oid, MAX_OID_LEN);
            oid[12] = 22;
            oid[14] = iface->slot;
            oid[15] = iface->index - iface->slot * 1000;
            iface->type = IFTYPE_GPON;
            SNMPConvertOIDToText(16, oid, szOid, 128);
            if (SnmpGet(snmp->getSnmpVersion(), snmp, szOid, NULL, 0, iface->alias, MAX_DB_STRING * sizeof(TCHAR), 0) != SNMP_ERR_SUCCESS)
            {
               StrStrip(iface->alias);
            }
            oid[12] = 13;
            SNMPConvertOIDToText(16, oid, szOid, 128);
            if (SnmpGet(snmp->getSnmpVersion(), snmp, szOid, NULL, 0, iface->description, MAX_DB_STRING * sizeof(TCHAR), 0) != SNMP_ERR_SUCCESS)
            {
               StrStrip(iface->description);
            }
            oid[12] = 2;
            SNMPConvertOIDToText(16, oid, szOid, 128);
            if (SnmpGet(snmp->getSnmpVersion(), snmp, szOid, NULL, 0, iface->name, MAX_DB_STRING * sizeof(TCHAR), 0) != SNMP_ERR_SUCCESS)
            {
               iface->type = IFTYPE_OTHER;
            }
         }
      }
   }
   return pIfList;
}

/**
 * Get interface status
 *
 * @param snmp SNMP transport
 * @param attributes Node's custom attributes
 * @param driverData
 * @param ifIndex
 * @param *adminState
 * @param *operState
 */
void QtechOLTDriver::getInterfaceState(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData, UINT32 ifIndex, InterfaceAdminState *adminState, InterfaceOperState *operState)
{
   UINT32 dwOperStatus = 0;
   TCHAR szOid[256];
   UINT32 oid[MAX_OID_LEN];

   *adminState = IF_ADMIN_STATE_UP;

   if (ifIndex > 1000)
   {
      SNMPParseOID(_T(".1.3.6.1.4.1.27514.1.11.4.1.1.3.0.0.0"), oid, MAX_OID_LEN);
      oid[14] = ifIndex / 1000;
      oid[15] = ifIndex - oid[14] * 1000;
      SNMPConvertOIDToText(16, oid, szOid, 128);
      SnmpGet(snmp->getSnmpVersion(), snmp, szOid, NULL, 0, &dwOperStatus, sizeof(UINT32), 0);
      switch(dwOperStatus)
      {
         case 1:
            *operState = IF_OPER_STATE_UP;
            break;
         case 0:
            *operState = IF_OPER_STATE_DOWN;
            break;
         default:
            *operState = IF_OPER_STATE_UNKNOWN;
            break;
      }
   }
   else
   {
      NetworkDeviceDriver::getInterfaceState(snmp, attributes, driverData, ifIndex, adminState, operState);
   }
}

/**
 * Driver entry point
 */
DECLARE_NDD_ENTRY_POINT(s_driverName, QtechOLTDriver);

#ifdef _WIN32

/**
 * DLL entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
      DisableThreadLibraryCalls(hInstance);
   return TRUE;
}

#endif
