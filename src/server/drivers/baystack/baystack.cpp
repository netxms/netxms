/* 
** NetXMS - Network Management System
** Driver for Avaya ERS switches (except ERS 8xxx) (former Nortel/Bay Networks BayStack)
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
** File: baystack.cpp
**/

#include "baystack.h"


//
// Static data
//

static TCHAR s_driverName[] = _T("BAYSTACK");
static TCHAR s_driverVersion[] = NETXMS_VERSION_STRING;


/**
 * Get driver name
 */
const TCHAR *BayStackDriver::getName()
{
	return s_driverName;
}

/**
 * Get driver version
 */
const TCHAR *BayStackDriver::getVersion()
{
	return s_driverVersion;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int BayStackDriver::isPotentialDevice(const TCHAR *oid)
{
	return (_tcsncmp(oid, _T(".1.3.6.1.4.1.45.3"), 17) == 0) ? 127 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool BayStackDriver::isDeviceSupported(SNMP_Transport *snmp, const TCHAR *oid)
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
void BayStackDriver::analyzeDevice(SNMP_Transport *snmp, const TCHAR *oid, StringMap *attributes)
{
	DWORD slotSize;
	
	if (!_tcsncmp(oid, _T(".1.3.6.1.4.1.45.3.74"), 20) ||	// 56xx
	    !_tcsncmp(oid, _T(".1.3.6.1.4.1.45.3.65"), 20))	// 5530-24TFD
	{
		slotSize = 128;
	}
	/* TODO: should OPtera Metro 1200 be there? */
	else if ((!_tcsncmp(oid, _T(".1.3.6.1.4.1.45.3.35"), 20)) ||	// BayStack 450
	         (!_tcsncmp(oid, _T(".1.3.6.1.4.1.45.3.40"), 20)) ||	// BPS2000
	         (!_tcsncmp(oid, _T(".1.3.6.1.4.1.45.3.43"), 20)))		// BayStack 420
	{
		slotSize = 32;
	}
	else
	{
		slotSize = 64;
	}

	attributes->set(_T(".baystack.slotSize"), slotSize);

	DWORD numVlans;
	if (SnmpGet(snmp->getSnmpVersion(), snmp, _T(".1.3.6.1.4.1.2272.1.3.1.0"), NULL, 0, &numVlans, sizeof(DWORD), 0) == SNMP_ERR_SUCCESS)
		attributes->set(_T(".baystack.rapidCity.vlan"), numVlans);
	else
		attributes->set(_T(".baystack.rapidCity.vlan"), (DWORD)0);
}

/**
 * Get slot size from object's custom attributes. Default implementation always return constant value 64.
 *
 * @param attributes object's custom attributes
 * @return slot size
 */
DWORD BayStackDriver::getSlotSize(StringMap *attributes)
{
	return attributes->getULong(_T(".baystack.slotSize"), 64);
}

/**
 * Get list of interfaces for given node
 *
 * @param snmp SNMP transport
 * @param attributes Node's custom attributes
 */
InterfaceList *BayStackDriver::getInterfaces(SNMP_Transport *snmp, StringMap *attributes, int useAliases, bool useIfXTable)
{
	// Get interface list from standard MIB
	InterfaceList *ifList = NetworkDeviceDriver::getInterfaces(snmp, attributes, useAliases, useIfXTable);
	if (ifList == NULL)
		return NULL;

   // Translate interface names 
	// TODO: does it really needed?
   for(int i = 0; i < ifList->getSize(); i++)
   {
		NX_INTERFACE_INFO *iface = ifList->get(i);

		const TCHAR *ptr;
      if ((ptr = _tcsstr(iface->szName, _T("- Port"))) != NULL)
		{
			ptr += 2;
         memmove(iface->szName, ptr, _tcslen(ptr) + 1);
		}
      else if ((ptr = _tcsstr(iface->szName, _T("- Unit"))) != NULL)
		{
			ptr += 2;
         memmove(iface->szName, ptr, _tcslen(ptr) + 1);
		}
      else if ((_tcsstr(iface->szName, _T("BayStack")) != NULL) ||
               (_tcsstr(iface->szName, _T("Nortel Ethernet Switch")) != NULL))
      {
         ptr = _tcsrchr(iface->szName, _T('-'));
         if (ptr != NULL)
         {
            ptr++;
            while(*ptr == _T(' '))
               ptr++;
            memmove(iface->szName, ptr, _tcslen(ptr) + 1);
         }
      }
		StrStrip(iface->szName);
   }
	
	// Calculate slot/port pair from ifIndex
	DWORD slotSize = attributes->getULong(_T(".baystack.slotSize"), 64);
	for(int i = 0; i < ifList->getSize(); i++)
	{
		DWORD slot = ifList->get(i)->dwIndex / slotSize + 1;
		if ((slot > 0) && (slot <= 8))
		{
			ifList->get(i)->dwSlotNumber = slot;
			ifList->get(i)->dwPortNumber = ifList->get(i)->dwIndex % slotSize;
			ifList->get(i)->isPhysicalPort = true;
		}
	}

	if (attributes->getULong(_T(".baystack.rapidCity.vlan"), 0) > 0)
		getVlanInterfaces(snmp, ifList);

	DWORD mgmtIpAddr, mgmtNetMask;
	if ((SnmpGet(snmp->getSnmpVersion(), snmp, _T(".1.3.6.1.4.1.45.1.6.4.2.2.1.2.1"), NULL, 0, &mgmtIpAddr, sizeof(DWORD), 0) == SNMP_ERR_SUCCESS) &&
	    (SnmpGet(snmp->getSnmpVersion(), snmp, _T(".1.3.6.1.4.1.45.1.6.4.2.2.1.3.1"), NULL, 0, &mgmtNetMask, sizeof(DWORD), 0) == SNMP_ERR_SUCCESS))
	{
		// Get switch base MAC address
		BYTE baseMacAddr[MAC_ADDR_LENGTH];
		SnmpGet(snmp->getSnmpVersion(), snmp, _T(".1.3.6.1.4.1.45.1.6.4.2.2.1.10.1"), NULL, 0, baseMacAddr, MAC_ADDR_LENGTH, SG_RAW_RESULT);

		if (mgmtIpAddr != 0)
		{
			int i;

			// Add management virtual interface if management IP is missing in interface list
			for(i = 0; i < ifList->getSize(); i++)
				if (ifList->get(i)->dwIpAddr == mgmtIpAddr)
					break;
			if (i == ifList->getSize())
			{
				NX_INTERFACE_INFO iface;

				memset(&iface, 0, sizeof(NX_INTERFACE_INFO));
				iface.dwIpAddr = mgmtIpAddr;
				iface.dwIpNetMask = mgmtNetMask;
				iface.dwType = IFTYPE_OTHER;
				_tcscpy(iface.szName, _T("MGMT"));
				memcpy(iface.bMacAddr, baseMacAddr, MAC_ADDR_LENGTH);
				ifList->add(&iface);
			}
		}

		// Update wrongly reported MAC addresses
		for(int i = 0; i < ifList->getSize(); i++)
		{
			NX_INTERFACE_INFO *curr = ifList->get(i);
			if ((curr->dwSlotNumber != 0) &&
				 (!memcmp(curr->bMacAddr, "\x00\x00\x00\x00\x00\x00", MAC_ADDR_LENGTH) ||
			     !memcmp(curr->bMacAddr, baseMacAddr, MAC_ADDR_LENGTH)))
			{
				memcpy(curr->bMacAddr, baseMacAddr, MAC_ADDR_LENGTH);
				curr->bMacAddr[5] += (BYTE)curr->dwPortNumber;
			}
		}
	}

	return ifList;
}

/**
 * Driver entry point
 */
DECLARE_NDD_ENTRY_POINT(s_driverName, BayStackDriver);


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
