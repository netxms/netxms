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

#include <nddrv.h>


//
// Static data
//

static TCHAR s_driverName[] = _T("BAYSTACK");
static TCHAR s_driverVersion[] = NETXMS_VERSION_STRING;


/**
 * Driver's class
 */
class BayStackDriver : public NetworkDeviceDriver
{
public:
	virtual const TCHAR *getName();
	virtual const TCHAR *getVersion();

	virtual bool isDeviceSupported(const TCHAR *oid);
	virtual void analyzeDevice(SNMP_Transport *snmp, const TCHAR *oid, StringMap *attributes);
	virtual InterfaceList *getInterfaces(SNMP_Transport *snmp, StringMap *attributes);
};

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
 * Check if given device is supported by driver
 *
 * @param oid Device OID
 */
bool BayStackDriver::isDeviceSupported(const TCHAR *oid)
{
	return _tcsncmp(oid, _T(".1.3.6.1.4.1.45.3"), 17) == 0;
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
	
	if (!_tcsncmp(oid, _T(".1.3.6.1.4.1.45.3.74"), 20))	// 56xx
	{
		slotSize = 128;
	}
	else if (!_tcsncmp(oid, _T(".1.3.6.1.4.1.45.3.40"), 20))	// BPS2000
	{
		slotSize = 32;
	}
	else
	{
		slotSize = 64;
	}

	attributes->set(_T(".baystack.rapidCity.vlan"), slotSize);
}

/**
 * Get list of interfaces for given node
 *
 * @param snmp SNMP transport
 * @param attributes Node's custom attributes
 */
InterfaceList *BayStackDriver::getInterfaces(SNMP_Transport *snmp, StringMap *attributes)
{
	// Get interface list from standard MIB
	InterfaceList *ifList = NetworkDeviceDriver::getInterfaces(snmp, attributes);
	if (ifList == NULL)
		return NULL;

   // Translate interface names 
	// TODO: does it really needed?
   for(int i = 0; i < ifList->getSize(); i++)
   {
		INTERFACE_INFO *iface = ifList->get(i);

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
	DWORD slotSize = attributes->getULong(_T(".baystack.rapidCity.vlan"), 64);
	for(int i = 0; i < ifList->getSize(); i++)
	{
		DWORD slot = ifList->get(i)->dwIndex / slotSize + 1;
		if ((slot > 0) && (slot <= 8))
		{
			ifList->get(i)->dwSlotNumber = slot;
			ifList->get(i)->dwPortNumber = ifList->get(i)->dwIndex % slotSize;
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
