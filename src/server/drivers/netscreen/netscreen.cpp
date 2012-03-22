/* 
** NetXMS - Network Management System
** Driver for Netscreen firewalls
** Copyright (C) 2003-2011 Victor Kirhenshtein
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
** File: netscreen.cpp
**/

#include "netscreen.h"


//
// Static data
//

static TCHAR s_driverName[] = _T("NETSCREEN");
static TCHAR s_driverVersion[] = NETXMS_VERSION_STRING;


/**
 * Get driver name
 */
const TCHAR *NetscreenDriver::getName()
{
	return s_driverName;
}

/**
 * Get driver version
 */
const TCHAR *NetscreenDriver::getVersion()
{
	return s_driverVersion;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int NetscreenDriver::isPotentialDevice(const TCHAR *oid)
{
	return (_tcsncmp(oid, _T(".1.3.6.1.4.1.3224.1"), 19) == 0) ? 254 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool NetscreenDriver::isDeviceSupported(SNMP_Transport *snmp, const TCHAR *oid)
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
void NetscreenDriver::analyzeDevice(SNMP_Transport *snmp, const TCHAR *oid, StringMap *attributes)
{
}

/**
 * Handler for interface enumeration
 */
static DWORD HandlerIfList(DWORD snmpVersion, SNMP_Variable *varbind, SNMP_Transport *transport, void *arg)
{
	InterfaceList *ifList = (InterfaceList *)arg;

   DWORD nameLen = varbind->GetName()->getLength();
	DWORD oidName[MAX_OID_LEN];
	memcpy(oidName, varbind->GetName()->getValue(), nameLen * sizeof(DWORD));

	NX_INTERFACE_INFO iface;
	memset(&iface, 0, sizeof(NX_INTERFACE_INFO));
	iface.dwIndex = varbind->GetValueAsUInt();

	oidName[10] = 2;	// nsIfName
	DWORD rc = SnmpGet(snmpVersion, transport, NULL, oidName, nameLen, iface.szName, MAX_DB_STRING, SG_STRING_RESULT);
	if (rc != SNMP_ERR_SUCCESS)
		return rc;
	nx_strncpy(iface.szDescription, iface.szName, MAX_DB_STRING);

	oidName[10] = 11;	// nsIfMAC
	rc = SnmpGet(snmpVersion, transport, NULL, oidName, nameLen, iface.bMacAddr, 6, SG_RAW_RESULT);
	if (rc != SNMP_ERR_SUCCESS)
		return rc;

	oidName[10] = 6;	// nsIfIp
	rc = SnmpGet(snmpVersion, transport, NULL, oidName, nameLen, &iface.dwIpAddr, sizeof(DWORD), 0);
	if (rc != SNMP_ERR_SUCCESS)
		return rc;

	oidName[10] = 7;	// nsIfNetmask
	rc = SnmpGet(snmpVersion, transport, NULL, oidName, nameLen, &iface.dwIpNetMask, sizeof(DWORD), 0);
	if (rc != SNMP_ERR_SUCCESS)
		return rc;

	ifList->add(&iface);
	return SNMP_ERR_SUCCESS;
}

/**
 * Get list of interfaces for given node
 *
 * @param snmp SNMP transport
 * @param attributes Node's custom attributes
 */
InterfaceList *NetscreenDriver::getInterfaces(SNMP_Transport *snmp, StringMap *attributes, int useAliases, bool useIfXTable)
{
	// Get interface list from standard MIB
	InterfaceList *stdIfList = NetworkDeviceDriver::getInterfaces(snmp, attributes, 0, false);
	if (stdIfList == NULL)
		return NULL;

	InterfaceList *ifList = new InterfaceList;
	if (SnmpEnumerate(snmp->getSnmpVersion(), snmp, _T(".1.3.6.1.4.1.3224.9.1.1.1"), HandlerIfList, ifList, FALSE) == SNMP_ERR_SUCCESS)
	{
		// Fix interface indexes
		for(int i = 0; i < ifList->getSize(); i++)
		{
			NX_INTERFACE_INFO *iface = ifList->get(i);
			int j;
			for(j = 0; j < stdIfList->getSize(); j++)
			{
				if (!_tcscmp(iface->szName, stdIfList->get(j)->szName))
				{
					iface->dwIndex = stdIfList->get(j)->dwIndex;
					break;
				}
			}
			if (j == stdIfList->getSize())
			{
				// Interface nt found in standard interface list (usually tunnel interface)
				iface->dwIndex += 32768;
			}
		}
	}
	else
	{
		delete_and_null(ifList);
	}
	delete stdIfList;
	return ifList;
}

/**
 * Driver entry point
 */
DECLARE_NDD_ENTRY_POINT(s_driverName, NetscreenDriver);

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
