/* 
** NetXMS - Network Management System
** Driver for Netscreen firewalls
** Copyright (C) 2003-2024 Victor Kirhenshtein
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

#include "juniper.h"
#include <netxms-version.h>

/**
 * Get driver name
 */
const TCHAR *NetscreenDriver::getName()
{
   return _T("NETSCREEN");
}

/**
 * Get driver version
 */
const TCHAR *NetscreenDriver::getVersion()
{
   return NETXMS_VERSION_STRING;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int NetscreenDriver::isPotentialDevice(const SNMP_ObjectId& oid)
{
   return oid.startsWith({ 1, 3, 6, 1, 4, 1, 3224, 1 }) ? 254 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool NetscreenDriver::isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid)
{
   return true;
}

/**
 * Handler for interface enumeration
 */
static uint32_t HandlerIfList(SNMP_Variable *varbind, SNMP_Transport *transport, InterfaceList *ifList)
{
   size_t nameLen = varbind->getName().length();
   uint32_t oidName[MAX_OID_LEN];
   memcpy(oidName, varbind->getName().value(), nameLen * sizeof(uint32_t));

   InterfaceInfo *iface = new InterfaceInfo(varbind->getValueAsUInt());

   oidName[10] = 2;	// nsIfName
   uint32_t rc = SnmpGetEx(transport, NULL, oidName, nameLen, iface->name, MAX_DB_STRING * sizeof(TCHAR), SG_STRING_RESULT, NULL);
   if (rc != SNMP_ERR_SUCCESS)
   {
      delete iface;
      return rc;
   }
   _tcslcpy(iface->description, iface->name, MAX_DB_STRING);

   oidName[10] = 11;	// nsIfMAC
   rc = SnmpGetEx(transport, NULL, oidName, nameLen, iface->macAddr, 6, SG_RAW_RESULT, NULL);
   if (rc != SNMP_ERR_SUCCESS)
   {
      delete iface;
      return rc;
   }

   oidName[10] = 6;	// nsIfIp
   uint32_t ipAddr;
   rc = SnmpGetEx(transport, NULL, oidName, nameLen, &ipAddr, sizeof(UINT32), 0, NULL);
   if (rc != SNMP_ERR_SUCCESS)
   {
      delete iface;
      return rc;
   }

   oidName[10] = 7;	// nsIfNetmask
   uint32_t ipNetMask;
   rc = SnmpGetEx(transport, NULL, oidName, nameLen, &ipNetMask, sizeof(UINT32), 0, NULL);
   if (rc != SNMP_ERR_SUCCESS)
   {
      delete iface;
      return rc;
   }

   iface->ipAddrList.add(InetAddress(ipAddr, ipNetMask));
	ifList->add(iface);
	return SNMP_ERR_SUCCESS;
}

/**
 * Get list of interfaces for given node
 *
 * @param snmp SNMP transport
 * @param node Node
 */
InterfaceList *NetscreenDriver::getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable)
{
	// Get interface list from standard MIB
	InterfaceList *stdIfList = NetworkDeviceDriver::getInterfaces(snmp, node, driverData, false);
	if (stdIfList == nullptr)
		return nullptr;

	InterfaceList *ifList = new InterfaceList;
	if (SnmpWalk(snmp, _T(".1.3.6.1.4.1.3224.9.1.1.1"), HandlerIfList, ifList) == SNMP_ERR_SUCCESS)
	{
		// Fix interface indexes
		for(int i = 0; i < ifList->size(); i++)
		{
			InterfaceInfo *iface = ifList->get(i);
			int j;
			for(j = 0; j < stdIfList->size(); j++)
			{
				if (!_tcscmp(iface->name, stdIfList->get(j)->name))
				{
					iface->index = stdIfList->get(j)->index;
					break;
				}
			}
			if (j == stdIfList->size())
			{
				// Interface not found in standard interface list (usually tunnel interface)
				iface->index += 32768;
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
