/* 
** NetXMS - Network Management System
** Driver for Ping3 devices
** Copyright (C) 2003-2013 Victor Kirhenshtein
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
** File: ping3.cpp
**/

#include "ping3.h"


/**
 * Driver name
 */
static TCHAR s_driverName[] = _T("PING3");

/**
 * Driver version
 */
static TCHAR s_driverVersion[] = NETXMS_VERSION_STRING;

/**
 * Get driver name
 */
const TCHAR *Ping3Driver::getName()
{
	return s_driverName;
}

/**
 * Get driver version
 */
const TCHAR *Ping3Driver::getVersion()
{
	return s_driverVersion;
}

/**
 * Get custom OID that should be used to test SNMP connectivity. Default
 * implementation always returns NULL.
 *
 * @return OID that should be used to test SNMP connectivity or NULL.
 */
const TCHAR *Ping3Driver::getCustomTestOID()
{
	return _T(".1.3.6.1.4.1.35160.1.1.0");
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int Ping3Driver::isPotentialDevice(const TCHAR *oid)
{
	return 127;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool Ping3Driver::isDeviceSupported(SNMP_Transport *snmp, const TCHAR *oid)
{
   TCHAR buffer[256];
   return SnmpGet(snmp->getSnmpVersion(), snmp, _T(".1.3.6.1.4.1.35160.1.1.0"), NULL, 0, buffer, 256, 0) == SNMP_ERR_SUCCESS;
}

/**
 * Get list of interfaces for given node
 *
 * @param snmp SNMP transport
 * @param attributes Node's custom attributes
 */
InterfaceList *Ping3Driver::getInterfaces(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData, int useAliases, bool useIfXTable)
{
   InterfaceList *ifList = NULL;

   SNMP_PDU *request = new SNMP_PDU(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
   request->bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.35160.1.3.0")));  // IP address
   request->bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.35160.1.4.0")));  // IP netmask
   request->bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.35160.1.6.0")));  // MAC address

   SNMP_PDU *response;
   UINT32 rcc = snmp->doRequest(request, &response, SnmpGetDefaultTimeout(), 3);
	delete request;
   if (rcc == SNMP_ERR_SUCCESS)
   {
      if (response->getNumVariables() == 3)
      {
         ifList = new InterfaceList(1);

         InterfaceInfo *iface = new InterfaceInfo(1);
         _tcscpy(iface->name, _T("eth0"));
         iface->type = IFTYPE_ETHERNET_CSMACD;
         iface->isPhysicalPort = true;

         UINT32 ipAddr, ipNetMask;
         response->getVariable(0)->getRawValue((BYTE *)&ipAddr, sizeof(UINT32));
         response->getVariable(1)->getRawValue((BYTE *)&ipNetMask, sizeof(UINT32));
         response->getVariable(0)->getRawValue(iface->macAddr, MAC_ADDR_LENGTH);

         iface->ipAddrList.add(InetAddress(ntohl(ipAddr), ntohl(ipNetMask)));
         ifList->add(iface);
      }
      delete response;
   }
	return ifList;
}

/**
 * Driver entry point
 */
DECLARE_NDD_ENTRY_POINT(s_driverName, Ping3Driver);

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
