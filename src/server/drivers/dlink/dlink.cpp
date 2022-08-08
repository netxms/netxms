/* 
** NetXMS - Network Management System
** Driver for D-Link switches
** Copyright (C) 2003-2022 Victor Kirhenshtein
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
** File: dlink.cpp
**/

#include "dlink.h"
#include <netxms-version.h>

#define DEBUG_TAG _T("ndd.dlink")

/**
 * Get driver name
 */
const TCHAR *DLinkDriver::getName()
{
	return _T("DLINK");
}

/**
 * Get driver version
 */
const TCHAR *DLinkDriver::getVersion()
{
	return NETXMS_VERSION_STRING;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int DLinkDriver::isPotentialDevice(const TCHAR *oid)
{
	return (_tcsncmp(oid, _T(".1.3.6.1.4.1.171.10."), 20) == 0) ? 127 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool DLinkDriver::isDeviceSupported(SNMP_Transport *snmp, const TCHAR *oid)
{
	return true;
}

/**
 * Do additional checks on the device required by driver.
 * Driver can set device's custom attributes from within
 * this function.
 *
 * @param snmp SNMP transport
 * @param node Node
 */
void DLinkDriver::analyzeDevice(SNMP_Transport *snmp, const TCHAR *oid, NObject *node, DriverData **driverData)
{
	node->setCustomAttribute(_T(".dlink.slotSize"), 48);

	// Check if device returns incorrect values for ifHighSpeed
	bool ifHighSpeedInBps = false;
	SnmpWalk(snmp, _T(".1.3.6.1.2.1.31.1.1.1.15"), [snmp, node, &ifHighSpeedInBps](SNMP_Variable *v) {
	   uint32_t ifHighSpeed = v->getValueAsUInt();
	   if (ifHighSpeed != 0)
      {
	      uint32_t ifIndex = v->getName().getLastElement();

	      TCHAR oid[256];
	      _sntprintf(oid, 256, _T(".1.3.6.1.2.1.2.2.1.5.%u"), ifIndex);

	      uint32_t ifSpeed;
	      if (SnmpGetEx(snmp, oid, nullptr, 0, &ifSpeed, sizeof(uint32_t), 0) == SNMP_ERR_SUCCESS)
         {
	         if (ifSpeed == ifHighSpeed)
            {
	            ifHighSpeedInBps = true;
	            nxlog_debug_tag(DEBUG_TAG, 5, _T("DLinkDriver::analyzeDevice(%s): ifSpeed == ifHighSpeed for interface %u, assuming that ifHighSpeed reports values in bps"), node->getName(), ifIndex);
	            return SNMP_ERR_ABORTED;
            }
         }
      }
	   return SNMP_ERR_SUCCESS;
	});
   node->setCustomAttribute(_T(".dlink.ifHighSpeedInBps"), ifHighSpeedInBps ? _T("true") : _T("false"), StateChange::CLEAR);
}

/**
 * Get list of interfaces for given node
 *
 * @param snmp SNMP transport
 * @param node Node
 */
InterfaceList *DLinkDriver::getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, int useAliases, bool useIfXTable)
{
	// Get interface list from standard MIB
	InterfaceList *ifList = NetworkDeviceDriver::getInterfaces(snmp, node, driverData, useAliases, useIfXTable);
	if (ifList == nullptr)
		return nullptr;

	uint32_t slotSize = node->getCustomAttributeAsUInt32(_T(".dlink.slotSize"), 48);

	// Find physical ports
	for(int i = 0; i < ifList->size(); i++)
	{
		InterfaceInfo *iface = ifList->get(i);
		if (iface->index < 1024)
		{
			iface->isPhysicalPort = true;
			iface->location.module = (iface->index / slotSize) + 1;
			iface->location.port = iface->index % slotSize;
		}
	}

	if (node->getCustomAttributeAsBoolean(_T(".dlink.ifHighSpeedInBps"), false))
	{
	   uint32_t ifHighSpeed;
      uint32_t oid[] = { 1, 3, 6, 1, 2, 1, 31, 1, 1, 1, 15, 0 };
	   for(int i = 0; i < ifList->size(); i++)
	   {
	      InterfaceInfo *iface = ifList->get(i);
	      if (iface->speed != 0)
	      {
	         oid[11] = iface->index;
	         if (SnmpGetEx(snmp, nullptr, oid, 12, &ifHighSpeed, sizeof(uint32_t), 0) == SNMP_ERR_SUCCESS)
	         {
	            iface->speed = ifHighSpeed;
	         }
	      }
	   }
	}

	return ifList;
}

/**
 * Driver entry point
 */
DECLARE_NDD_ENTRY_POINT(DLinkDriver);

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
