/* 
** NetXMS - Network Management System
** Driver for Allied Telesis switches
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
** File: at.cpp
**/

#include "at.h"


/**
 * Driver name
 */
static TCHAR s_driverName[] = _T("AT");

/**
 * Driver version
 */
static TCHAR s_driverVersion[] = NETXMS_VERSION_STRING;

/**
 * Get driver name
 */
const TCHAR *AlliedTelesisDriver::getName()
{
	return s_driverName;
}

/**
 * Get driver version
 */
const TCHAR *AlliedTelesisDriver::getVersion()
{
	return s_driverVersion;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int AlliedTelesisDriver::isPotentialDevice(const TCHAR *oid)
{
	return (_tcsncmp(oid, _T(".1.3.6.1.4.1.207.1"), 18) == 0) ? 127 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool AlliedTelesisDriver::isDeviceSupported(SNMP_Transport *snmp, const TCHAR *oid)
{
   TCHAR buffer[256];
   return SnmpGet(snmp->getSnmpVersion(), snmp, _T(".1.3.6.1.4.1.207.8.33.1.1.0"), NULL, 0, buffer, 256, 0) == SNMP_ERR_SUCCESS;
}

/**
 * Get list of interfaces for given node
 *
 * @param snmp SNMP transport
 * @param attributes Node's custom attributes
 */
InterfaceList *AlliedTelesisDriver::getInterfaces(SNMP_Transport *snmp, StringMap *attributes, void *driverData, int useAliases, bool useIfXTable)
{
   InterfaceList *ifList = NetworkDeviceDriver::getInterfaces(snmp, attributes, driverData, useAliases, useIfXTable);
   if (ifList != NULL)
   {
      TCHAR oid[256];

      // Find physical ports
      for(int i = 0; i < ifList->getSize(); i++)
      {
         SNMP_PDU *request = new SNMP_PDU(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());

         NX_INTERFACE_INFO *iface = ifList->get(i);
         _sntprintf(oid, 256, _T(".1.3.6.1.4.1.207.8.33.9.1.1.1.2.%d"), iface->dwIndex);  // module
         request->bindVariable(new SNMP_Variable(oid));
         _sntprintf(oid, 256, _T(".1.3.6.1.4.1.207.8.33.9.1.1.1.3.%d"), iface->dwIndex);  // port
         request->bindVariable(new SNMP_Variable(oid));

         SNMP_PDU *response;
         UINT32 rcc = snmp->doRequest(request, &response, g_dwSNMPTimeout, 3);
	      delete request;
         if (rcc == SNMP_ERR_SUCCESS)
         {
            if (response->getNumVariables() == 2)
            {
               iface->dwSlotNumber = response->getVariable(0)->GetValueAsInt();
               iface->dwPortNumber = response->getVariable(1)->GetValueAsInt();
               iface->isPhysicalPort = true;
            }
            delete response;
         }
      }
   }
	return ifList;
}

/**
 * Handler for VLAN enumeration
 */
static UINT32 HandlerVlanList(UINT32 version, SNMP_Variable *var, SNMP_Transport *snmp, void *arg)
{
   VlanList *vlanList = (VlanList *)arg;

	VlanInfo *vlan = new VlanInfo(var->GetName()->getValue()[var->GetName()->getLength() - 1], VLAN_PRM_SLOTPORT);

	TCHAR buffer[256];
	vlan->setName(var->GetValueAsString(buffer, 256));
	vlanList->add(vlan);
   return SNMP_ERR_SUCCESS;
}

/**
 * Handler for VLAN port enumeration
 */
static UINT32 HandlerVlanPortList(UINT32 version, SNMP_Variable *var, SNMP_Transport *snmp, void *arg)
{
   VlanList *vlanList = (VlanList *)arg;

   TCHAR vlanName[256];
   var->GetValueAsString(vlanName, 256);
   VlanInfo *vlan = vlanList->findByName(vlanName);
   if (vlan != NULL)
   {
      int slot = var->GetName()->getValue()[var->GetName()->getLength() - 2];
      int port = var->GetName()->getValue()[var->GetName()->getLength() - 1];
      vlan->add(slot, port);
   }

   return SNMP_ERR_SUCCESS;
}

/**
 * Get VLANs 
 */
VlanList *AlliedTelesisDriver::getVlans(SNMP_Transport *snmp, StringMap *attributes, void *driverData)
{
   VlanList *list = NetworkDeviceDriver::getVlans(snmp, attributes, driverData);
   if (list != NULL)
      return list;   // retrieved from standard MIBs

   list = new VlanList();

   if (SnmpWalk(snmp->getSnmpVersion(), snmp, _T(".1.3.6.1.4.1.207.8.33.8.1.1.2"), HandlerVlanList, list, FALSE) != SNMP_ERR_SUCCESS)
		goto failure;

   if (SnmpWalk(snmp->getSnmpVersion(), snmp, _T(".1.3.6.1.4.1.207.8.33.8.2.1.3"), HandlerVlanPortList, list, FALSE) != SNMP_ERR_SUCCESS)
		goto failure;

   return list;

failure:
	delete list;
	return NULL;
}

/**
 * Driver entry point
 */
DECLARE_NDD_ENTRY_POINT(s_driverName, AlliedTelesisDriver);

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
