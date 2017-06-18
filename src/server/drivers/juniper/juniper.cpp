/* 
** NetXMS - Network Management System
** Driver for Juniper Networks switches
** Copyright (C) 2003-2017 Victor Kirhenshtein
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
** File: juniper.cpp
**/

#include "juniper.h"

/**
 * Driver name
 */
static TCHAR s_driverName[] = _T("JUNIPER");

/**
 * Driver version
 */
static TCHAR s_driverVersion[] = NETXMS_VERSION_STRING;

/**
 * Get driver name
 */
const TCHAR *JuniperDriver::getName()
{
	return s_driverName;
}

/**
 * Get driver version
 */
const TCHAR *JuniperDriver::getVersion()
{
	return s_driverVersion;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int JuniperDriver::isPotentialDevice(const TCHAR *oid)
{
	return (_tcsncmp(oid, _T(".1.3.6.1.4.1.2636.1."), 20) == 0) ? 255 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool JuniperDriver::isDeviceSupported(SNMP_Transport *snmp, const TCHAR *oid)
{
	return true;
}

/**
 * Get list of interfaces for given node
 *
 * @param snmp SNMP transport
 * @param attributes Node's custom attributes
 */
InterfaceList *JuniperDriver::getInterfaces(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData, int useAliases, bool useIfXTable)
{
	// Get interface list from standard MIB
	InterfaceList *ifList = NetworkDeviceDriver::getInterfaces(snmp, attributes, driverData, useAliases, useIfXTable);
	if (ifList == NULL)
		return NULL;

	// Update physical port locations
	SNMP_Snapshot *chassisTable = SNMP_Snapshot::create(snmp, _T(".1.3.6.1.4.1.2636.3.3.2.1"));
	if (chassisTable != NULL)
	{
	   // Update slot/port for physical interfaces
      for(int i = 0; i < ifList->size(); i++)
      {
         InterfaceInfo *iface = ifList->get(i);
         if (iface->type != IFTYPE_ETHERNET_CSMACD)
            continue;

         SNMP_ObjectId oid = SNMP_ObjectId::parse(_T(".1.3.6.1.4.1.2636.3.3.2.1.1"));
         oid.extend(iface->index);
         int slot = chassisTable->getAsInt32(oid);

         oid.changeElement(oid.length() - 2, 2);
         int pic = chassisTable->getAsInt32(oid);

         oid.changeElement(oid.length() - 2, 3);
         int port = chassisTable->getAsInt32(oid);

         if ((slot == 0) || (pic != 1) || (port == 0))   // FIXME: support for multiple PICs in one slot
            continue;

         iface->isPhysicalPort = true;
         iface->slot = slot - 1;  // Juniper numbers slots from 0 but reports in SNMP as n + 1
         iface->port = port - 1;  // Juniper numbers ports from 0 but reports in SNMP as n + 1
      }

      // Attach logical interfaces to physical
      for(int i = 0; i < ifList->size(); i++)
      {
         InterfaceInfo *iface = ifList->get(i);
         if (iface->type != IFTYPE_PROP_VIRTUAL)
            continue;

         SNMP_ObjectId oid = SNMP_ObjectId::parse(_T(".1.3.6.1.4.1.2636.3.3.2.1.1"));
         oid.extend(iface->index);
         int slot = chassisTable->getAsInt32(oid);

         oid.changeElement(oid.length() - 2, 2);
         int pic = chassisTable->getAsInt32(oid);

         oid.changeElement(oid.length() - 2, 3);
         int port = chassisTable->getAsInt32(oid);

         if ((slot == 0) || (pic != 1) || (port == 0))   // FIXME: support for multiple PICs in one slot
            continue;

         InterfaceInfo *parent = ifList->findByPhyPosition(slot - 1, port - 1);
         if (parent != NULL)
            iface->parentIndex = parent->index;
      }

      delete chassisTable;
	}
	return ifList;
}

/**
 * Get VLANs
 */
VlanList *JuniperDriver::getVlans(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData)
{
   SNMP_Snapshot *vlanTable = SNMP_Snapshot::create(snmp, _T(".1.3.6.1.4.1.2636.3.40.1.5.1.5.1"));
   if (vlanTable == NULL)
      return NetworkDeviceDriver::getVlans(snmp, attributes, driverData);

   SNMP_Snapshot *portTable = SNMP_Snapshot::create(snmp, _T(".1.3.6.1.4.1.2636.3.40.1.5.1.7.1"));
   if (portTable == NULL)
   {
      delete vlanTable;
      return NetworkDeviceDriver::getVlans(snmp, attributes, driverData);
   }

   VlanList *vlans = new VlanList();
   SNMP_ObjectId oid = SNMP_ObjectId::parse(_T(".1.3.6.1.4.1.2636.3.40.1.5.1.5.1.5"));
   const SNMP_Variable *v;
   while((v = vlanTable->getNext(oid)) != NULL)
   {
      VlanInfo *vlan = new VlanInfo(v->getValueAsInt(), VLAN_PRM_IFINDEX);
      vlans->add(vlan);

      oid = v->getName();
      oid.changeElement(oid.length() - 2, 2);   // VLAN name
      const SNMP_Variable *name = vlanTable->get(oid);
      if (name != NULL)
      {
         TCHAR buffer[256];
         vlan->setName(name->getValueAsString(buffer, 256));
      }

      // VLAN ports
      UINT32 vlanId = oid.getElement(oid.length() - 1);
      SNMP_ObjectId baseOid = SNMP_ObjectId::parse(_T(".1.3.6.1.4.1.2636.3.40.1.5.1.7.1.5"));
      baseOid.extend(vlanId);
      const SNMP_Variable *p = NULL;
      while((p = portTable->getNext((p != NULL) ? p->getName() : baseOid)) != NULL)
      {
         if (p->getName().compare(baseOid) != OID_LONGER)
            break;
         vlan->add(p->getName().getElement(p->getName().length() - 1));
      }

      oid = v->getName();
   }

   delete vlanTable;
   delete portTable;
   return vlans;
}

/**
 * Get orientation of the modules in the device
 *
 * @param snmp SNMP transport
 * @param attributes Node's custom attributes
 * @param driverData driver-specific data previously created in analyzeDevice
 * @return module orientation
 */
int JuniperDriver::getModulesOrientation(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData)
{
   return NDD_ORIENTATION_HORIZONTAL;
}

/**
 * Get port layout of given module
 * @param snmp SNMP transport
 * @param attributes Node's custom attributes
 * @param driverData driver-specific data previously created in analyzeDevice
 * @param module Module number (starting from 1)
 * @param layout Layout structure to fill
 */
void JuniperDriver::getModuleLayout(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData, int module, NDD_MODULE_LAYOUT *layout)
{
   layout->numberingScheme = NDD_PN_UD_LR;
   layout->rows = 2;
}

/**
 * Driver entry point
 */
DECLARE_NDD_ENTRY_POINT(s_driverName, JuniperDriver);

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
