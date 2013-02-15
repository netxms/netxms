/* 
** NetXMS - Network Management System
** Generic driver for Cisco devices
** Copyright (C) 2003-2012 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: cisco.cpp
**
**/

#include "cisco.h"


/**
 * Handler for VLAN enumeration on Cisco device
 */
static DWORD HandlerVlanList(DWORD version, SNMP_Variable *var, SNMP_Transport *transport, void *arg)
{
   VlanList *vlanList = (VlanList *)arg;

	VlanInfo *vlan = new VlanInfo(var->GetName()->getValue()[var->GetName()->getLength() - 1], VLAN_PRM_IFINDEX);

	TCHAR buffer[256];
	vlan->setName(var->GetValueAsString(buffer, 256));

	vlanList->add(vlan);
   return SNMP_ERR_SUCCESS;
}

/**
 * Parse VLAN membership bit map
 *
 * @param vlanList VLAN list
 * @param ifIndex interface index for current interface
 * @param map VLAN membership map
 * @param offset VLAN ID offset from 0
 */
static void ParseVlanMap(VlanList *vlanList, DWORD ifIndex, BYTE *map, int offset)
{
	// VLAN map description from Cisco MIB:
	// ======================================
	// A string of octets containing one bit per VLAN in the
	// management domain on this trunk port.  The first octet
	// corresponds to VLANs with VlanIndex values of 0 through 7;
	// the second octet to VLANs 8 through 15; etc.  The most
	// significant bit of each octet corresponds to the lowest
	// value VlanIndex in that octet.  If the bit corresponding to
	// a VLAN is set to '1', then the local system is enabled for
	// sending and receiving frames on that VLAN; if the bit is set
	// to '0', then the system is disabled from sending and
	// receiving frames on that VLAN.

	int vlanId = offset;
	for(int i = 0; i < 128; i++)
	{
		BYTE mask = 0x80;
		while(mask > 0)
		{
			if (map[i] & mask)
			{
				vlanList->addMemberPort(vlanId, ifIndex);
			}
			mask >>= 1;
			vlanId++;
		}
	}
}

/**
 * Handler for trunk port enumeration on Cisco device
 */
static DWORD HandlerTrunkPorts(DWORD version, SNMP_Variable *var, SNMP_Transport *transport, void *arg)
{
   VlanList *vlanList = (VlanList *)arg;
   DWORD nameLen = var->GetName()->getLength();
	DWORD ifIndex = var->GetName()->getValue()[nameLen - 1];

	// Check if port is acting as trunk
	DWORD oidName[256], value;
   memcpy(oidName, var->GetName()->getValue(), nameLen * sizeof(DWORD));
   oidName[nameLen - 2] = 14;	// .1.3.6.1.4.1.9.9.46.1.6.1.1.14
	if (SnmpGet(version, transport, NULL, oidName, nameLen, &value, sizeof(DWORD), 0) != SNMP_ERR_SUCCESS)
	   return SNMP_ERR_SUCCESS;	// Cannot get trunk state, ignore port
	if (value != 1)
	   return SNMP_ERR_SUCCESS;	// Not a trunk port, ignore

	// Native VLAN
	int vlanId = var->GetValueAsInt();
	if (vlanId != 0)
		vlanList->addMemberPort(vlanId, ifIndex);

	// VLAN map for VLAN IDs 0..1023
   oidName[nameLen - 2] = 4;	// .1.3.6.1.4.1.9.9.46.1.6.1.1.4
	BYTE map[128];
	memset(map, 0, 128);
	if (SnmpGet(version, transport, NULL, oidName, nameLen, map, 128, SG_RAW_RESULT) == SNMP_ERR_SUCCESS)
		ParseVlanMap(vlanList, ifIndex, map, 0);

	// VLAN map for VLAN IDs 1024..2047
   oidName[nameLen - 2] = 17;	// .1.3.6.1.4.1.9.9.46.1.6.1.1.17
	memset(map, 0, 128);
	if (SnmpGet(version, transport, NULL, oidName, nameLen, map, 128, SG_RAW_RESULT) == SNMP_ERR_SUCCESS)
		ParseVlanMap(vlanList, ifIndex, map, 1024);

	// VLAN map for VLAN IDs 2048..3071
   oidName[nameLen - 2] = 18;	// .1.3.6.1.4.1.9.9.46.1.6.1.1.18
	memset(map, 0, 128);
	if (SnmpGet(version, transport, NULL, oidName, nameLen, map, 128, SG_RAW_RESULT) == SNMP_ERR_SUCCESS)
		ParseVlanMap(vlanList, ifIndex, map, 2048);

	// VLAN map for VLAN IDs 3072..4095
   oidName[nameLen - 2] = 19;	// .1.3.6.1.4.1.9.9.46.1.6.1.1.19
	memset(map, 0, 128);
	if (SnmpGet(version, transport, NULL, oidName, nameLen, map, 128, SG_RAW_RESULT) == SNMP_ERR_SUCCESS)
		ParseVlanMap(vlanList, ifIndex, map, 3072);

   return SNMP_ERR_SUCCESS;
}


/**
 * Handler for access port enumeration on Cisco device
 */
static DWORD HandlerAccessPorts(DWORD version, SNMP_Variable *var, SNMP_Transport *transport, void *arg)
{
   VlanList *vlanList = (VlanList *)arg;
   DWORD nameLen = var->GetName()->getLength();
	DWORD ifIndex = var->GetName()->getValue()[nameLen - 1];

	DWORD oidName[256];
   memcpy(oidName, var->GetName()->getValue(), nameLen * sizeof(DWORD));

	// Entry type: 3=multi-vlan
	if (var->GetValueAsInt() == 3)
	{
		BYTE map[128];

		oidName[nameLen - 2] = 4;	// .1.3.6.1.4.1.9.9.68.1.2.2.1.4
		memset(map, 0, 128);
		if (SnmpGet(version, transport, NULL, oidName, nameLen, map, 128, SG_RAW_RESULT) == SNMP_ERR_SUCCESS)
			ParseVlanMap(vlanList, ifIndex, map, 0);

		// VLAN map for VLAN IDs 1024..2047
		oidName[nameLen - 2] = 5;	// .1.3.6.1.4.1.9.9.68.1.2.2.1.5
		memset(map, 0, 128);
		if (SnmpGet(version, transport, NULL, oidName, nameLen, map, 128, SG_RAW_RESULT) == SNMP_ERR_SUCCESS)
			ParseVlanMap(vlanList, ifIndex, map, 1024);

		// VLAN map for VLAN IDs 2048..3071
		oidName[nameLen - 2] = 6;	// .1.3.6.1.4.1.9.9.68.1.2.2.1.6
		memset(map, 0, 128);
		if (SnmpGet(version, transport, NULL, oidName, nameLen, map, 128, SG_RAW_RESULT) == SNMP_ERR_SUCCESS)
			ParseVlanMap(vlanList, ifIndex, map, 2048);

		// VLAN map for VLAN IDs 3072..4095
		oidName[nameLen - 2] = 7;	// .1.3.6.1.4.1.9.9.68.1.2.2.1.7
		memset(map, 0, 128);
		if (SnmpGet(version, transport, NULL, oidName, nameLen, map, 128, SG_RAW_RESULT) == SNMP_ERR_SUCCESS)
			ParseVlanMap(vlanList, ifIndex, map, 3072);
	}
	else
	{
		// Port is in just one VLAN, it's ID must be in vmVlan
	   oidName[nameLen - 2] = 2;	// .1.3.6.1.4.1.9.9.68.1.2.2.1.2
		DWORD vlanId = 0;
		if (SnmpGet(version, transport, NULL, oidName, nameLen, &vlanId, sizeof(DWORD), 0) == SNMP_ERR_SUCCESS)
		{
			if (vlanId != 0)
				vlanList->addMemberPort((int)vlanId, ifIndex);
		}
	}

	return SNMP_ERR_SUCCESS;
}

/**
 * Get VLANs 
 */
VlanList *CiscoDeviceDriver::getVlans(SNMP_Transport *snmp, StringMap *attributes, void *driverData)
{
	VlanList *list = new VlanList();
	
	// Vlan list
	if (SnmpWalk(snmp->getSnmpVersion(), snmp, _T(".1.3.6.1.4.1.9.9.46.1.3.1.1.4"), HandlerVlanList, list, FALSE) != SNMP_ERR_SUCCESS)
		goto failure;

	// Trunk ports
	if (SnmpWalk(snmp->getSnmpVersion(), snmp, _T(".1.3.6.1.4.1.9.9.46.1.6.1.1.5"), HandlerTrunkPorts, list, FALSE) != SNMP_ERR_SUCCESS)
		goto failure;

	// Access ports
	if (SnmpWalk(snmp->getSnmpVersion(), snmp, _T(".1.3.6.1.4.1.9.9.68.1.2.2.1.1"), HandlerAccessPorts, list, FALSE) != SNMP_ERR_SUCCESS)
		goto failure;

	return list;

failure:
	delete list;
	return NULL;
}

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
