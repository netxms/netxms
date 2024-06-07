/* 
** NetXMS - Network Management System
** Driver for EtherWan switches
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
** File: etherwan.cpp
**/

#include "etherwan.h"
#include <netxms-version.h>

/**
 * Get driver name
 */
const TCHAR *EtherWanDriver::getName()
{
	return _T("ETHERWAN");
}

/**
 * Get driver version
 */
const TCHAR *EtherWanDriver::getVersion()
{
	return NETXMS_VERSION_STRING;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int EtherWanDriver::isPotentialDevice(const SNMP_ObjectId& oid)
{
   return oid.startsWith({ 1, 3, 6, 1, 4, 1, 2736 }) ? 220 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool EtherWanDriver::isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid)
{
   return true;
}

/**
 * Get hardware information from device.
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver data
 * @param hwInfo pointer to hardware information structure to fill
 * @return true if hardware information is available
 */
bool EtherWanDriver::getHardwareInformation(SNMP_Transport *snmp, NObject *node, DriverData *driverData, DeviceHardwareInfo *hwInfo)
{
   _tcscpy(hwInfo->vendor, _T("EtherWan"));

   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
   request.bindVariable(new SNMP_Variable(_T("1.3.6.1.4.1.2736.1.1.1.6.0")));  // ewnSystemProductModel
   request.bindVariable(new SNMP_Variable(_T("1.3.6.1.4.1.2736.1.1.1.14.0")));  // ewnSystemProductSeries
   request.bindVariable(new SNMP_Variable(_T("1.3.6.1.4.1.2736.1.1.1.9.0")));  // ewnSystemMacAddr
   request.bindVariable(new SNMP_Variable(_T("1.3.6.1.4.1.2736.1.1.1.1.0")));  // ewnSystemFirmwareRev

   SNMP_PDU *response;
   if (snmp->doRequest(&request, &response) == SNMP_ERR_SUCCESS)
   {
      TCHAR buffer[256];
      const SNMP_Variable *v = response->getVariable(0);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
      {
         _tcslcpy(hwInfo->productName, v->getValueAsString(buffer, 256), 128);
      }

      v = response->getVariable(1);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
      {
         _tcslcpy(hwInfo->productCode, v->getValueAsString(buffer, 256), 32);
      }

      v = response->getVariable(2);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
      {
         _tcslcpy(hwInfo->serialNumber, v->getValueAsMACAddr().toString(MacAddressNotation::FLAT_STRING), 32);
      }

      v = response->getVariable(3);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
      {
         _tcslcpy(hwInfo->productVersion, v->getValueAsString(buffer, 256), 16);
         TCHAR *sp = _tcschr(hwInfo->productVersion, _T(' '));
         if (sp != nullptr)
            *sp = 0;
      }

      delete response;
   }
   return true;
}

/**
 * Get list of interfaces for given node
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver data
 * @param useAliases policy for interface alias usage
 * @param useIfXTable if true, usage of ifXTable is allowed
 */
InterfaceList *EtherWanDriver::getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable)
{
   InterfaceList *ifList = NetworkDeviceDriver::getInterfaces(snmp, node, driverData, useIfXTable);
   if (ifList == nullptr)
      return nullptr;

   for(int i = 0; i < ifList->size(); i++)
   {
      InterfaceInfo *iface = ifList->get(i);
      if (!_tcscmp(iface->alias, _T("(null)")))
         iface->alias[0] = 0;
   }

   SnmpWalk(snmp, { 1, 3, 6, 1, 4, 1, 2736, 1, 1, 2, 2, 1, 2 },
      [ifList] (SNMP_Variable *v) -> uint32_t
      {
         TCHAR ifName[64];
         v->getValueAsString(ifName, 64);
         for(int i = 0; i < ifList->size(); i++)
         {
            InterfaceInfo *iface = ifList->get(i);
            if (!_tcscmp(iface->name, ifName))
            {
               iface->isPhysicalPort = true;
               iface->location = InterfacePhysicalLocation(0, 0, 0, v->getName().getLastElement());
               break;
            }
         }
         return SNMP_ERR_SUCCESS;
      });
   return ifList;
}

/**
 * Parse port map for VLAN
 */
static void ParsePortMap(const BYTE *ports, size_t len, bool tagged, VlanInfo *vlan)
{
   int p = 1;
   for(size_t i = 0; i < len; i++)
   {
      BYTE b = 0x80;
      for(int j = 0; j < 8; j++)
      {
         if (ports[i] & b)
         {
            vlan->add(0, 0, 0, p, tagged);
         }
         p++;
         b = b >> 1;
      }
   }
}

/**
 * Read information on given VLAN
 */
static VlanInfo *ReadVlanInfo(SNMP_Transport *snmp, int vlanId, const SNMP_ObjectId& baseOid)
{
   VlanInfo *vlan = nullptr;

   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());

   SNMP_ObjectId oid(baseOid);
   oid.changeElement(oid.length() - 2, 3); // ewnVLANName
   request.bindVariable(new SNMP_Variable(oid));

   oid.changeElement(oid.length() - 2, 8); // ewnVLANCurrentPortMap
   request.bindVariable(new SNMP_Variable(oid));

   oid.changeElement(oid.length() - 2, 9); // ewnVLANCurrentUntaggedPortMap
   request.bindVariable(new SNMP_Variable(oid));

   SNMP_PDU *response;
   if (snmp->doRequest(&request, &response) == SNMP_ERR_SUCCESS)
   {
      if (response->getNumVariables() == 3)
      {
         TCHAR name[128];
         response->getVariable(0)->getValueAsString(name, 128);

         vlan = new VlanInfo(vlanId, VLAN_PRM_PHYLOC, name);

         BYTE ports[64];
         size_t bytes = response->getVariable(1)->getRawValue(ports, sizeof(ports));
         ParsePortMap(ports, bytes, true, vlan);

         bytes = response->getVariable(2)->getRawValue(ports, sizeof(ports));
         ParsePortMap(ports, bytes, false, vlan);
      }
      delete response;
   }

   return vlan;
}

/**
 * Get list of VLANs on given node
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @return VLAN list or NULL
 */
VlanList *EtherWanDriver::getVlans(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   VlanList *vlans = new VlanList();
   if (SnmpWalk(snmp, { 1, 3, 6, 1, 4, 1, 2736, 1, 1, 8, 2, 1, 2 }, //  ewnVLANID
      [vlans, snmp] (SNMP_Variable *v) -> uint32_t
      {
         VlanInfo *vlan = ReadVlanInfo(snmp, v->getValueAsInt(), v->getName());
         if (vlan != nullptr)
            vlans->add(vlan);
         return SNMP_ERR_SUCCESS;
      }) != SNMP_ERR_SUCCESS)
   {
      delete vlans;
      return nullptr;
   }
   return vlans;
}

/**
 * Driver entry point
 */
DECLARE_NDD_ENTRY_POINT(EtherWanDriver);

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
