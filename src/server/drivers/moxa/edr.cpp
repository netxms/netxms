/* 
** NetXMS - Network Management System
** Driver for Moxa EDR devices
** Copyright (C) 2020-2024 Raden Solutions
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
** File: edr.cpp
**/

#include "moxa.h"

#define DEBUG_TAG _T("ndd.moxa.edr")

/**
 * Get driver name
 */
const TCHAR *MoxaEDRDriver::getName()
{
   return _T("MOXA-EDR");
}

/**
 * Get driver version
 */
const TCHAR *MoxaEDRDriver::getVersion()
{
   return NETXMS_VERSION_STRING;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int MoxaEDRDriver::isPotentialDevice(const SNMP_ObjectId& oid)
{
   return oid.startsWith({ 1, 3, 6, 1, 4, 1, 8691, 6 }) ? 200 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool MoxaEDRDriver::isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid)
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
bool MoxaEDRDriver::getHardwareInformation(SNMP_Transport *snmp, NObject *node, DriverData *driverData, DeviceHardwareInfo *hwInfo)
{
   TCHAR objectId[128];
   if (SnmpGetEx(snmp, _T(".1.3.6.1.2.1.1.2.0"), nullptr, 0, objectId, sizeof(objectId), 0, nullptr) != SNMP_ERR_SUCCESS)
      return false;

   _tcscpy(hwInfo->vendor, _T("Moxa"));

   // Model name
   TCHAR requestOID[128];
   _sntprintf(requestOID, 128, _T("%s.1.22.2.0"), objectId);
   if (SnmpGetEx(snmp, requestOID, nullptr, 0, hwInfo->productName, sizeof(hwInfo->productName), 0, nullptr) != SNMP_ERR_SUCCESS)
   {
      // Guess model from product OID
      if (!_tcscmp(objectId, _T(".1.3.6.1.4.1.8691.6.13")))
         _tcscpy(hwInfo->productName, _T("EDR-810"));
      else if (!_tcscmp(objectId, _T(".1.3.6.1.4.1.8691.6.12")))
         _tcscpy(hwInfo->productName, _T("EDR-G902"));
      else if (!_tcscmp(objectId, _T(".1.3.6.1.4.1.8691.6.11")))
         _tcscpy(hwInfo->productName, _T("EDR-G903"));
   }

   // Firmware version
   _sntprintf(requestOID, 128, _T("%s.1.22.4.0"), objectId);
   TCHAR version[128];
   SnmpGetEx(snmp, requestOID, nullptr, 0, version, sizeof(version), 0, nullptr);
   if (_tcslen(version) < sizeof(hwInfo->productVersion) / sizeof(TCHAR))
   {
      _tcscpy(hwInfo->productVersion, version);
   }
   else
   {
      TCHAR *p = _tcschr(version, _T(' '));
      if (p != nullptr)
         *p = 0;
      _tcslcpy(hwInfo->productVersion, version, sizeof(hwInfo->productVersion) / sizeof(TCHAR));
   }
   return true;
}

/**
 * Get list of interfaces for given node
 *
 * @param snmp SNMP transport
 * @param node Node
 */
InterfaceList *MoxaEDRDriver::getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable)
{
	InterfaceList *ifList = NetworkDeviceDriver::getInterfaces(snmp, node, driverData, useIfXTable);
	if (ifList == nullptr)
	   return nullptr;

	// Moxa EDR devices seems to report physical ports with indexes from 1 to 127
	// and logical ports with indexes 128 + index (as if part of interface MIB is proxied from
	// another SNMP agent on embedded Linux machine). IP addresses reported using incorrect
	// interface index (without adding 128) so they has to be moved.
	for(int i = 0; i < ifList->size(); i++)
	{
	   InterfaceInfo *iface = ifList->get(i);
	   if (iface->index >= 128)
	      continue;   // Logical interface

      iface->isPhysicalPort = true;
      iface->bridgePort = iface->index;
      iface->location.module = 1;
      iface->location.port = iface->index;
      if (!iface->ipAddrList.isEmpty())
      {
         // Find correct interface
         InterfaceInfo *logicalInterface = ifList->findByIfIndex(iface->index + 128);
         if (logicalInterface != nullptr)
            logicalInterface->ipAddrList.add(iface->ipAddrList);
         iface->ipAddrList.clear();
      }
	}

	return ifList;
}

/**
 * Translate LLDP port name (port ID subtype 5) to local interface id.
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver's data
 * @param lldpName port name received from LLDP MIB
 * @param id interface ID structure to be filled at success
 * @return true if interface identification provided
 */
bool MoxaEDRDriver::lldpNameToInterfaceId(SNMP_Transport *snmp, NObject *node, DriverData *driverData, const TCHAR *lldpName, InterfaceId *id)
{
   // Device could present string like "PORTn" or just "n" where n is port number (same as interface index)
   TCHAR *eptr;
   uint32_t portId = !_tcsncmp(lldpName, _T("PORT"), 4) ? _tcstoul(&lldpName[4], &eptr, 10) : _tcstoul(lldpName, &eptr, 10);
   if ((portId != 0) && (*eptr == 0))
   {
      id->type = InterfaceIdType::INDEX;
      id->value.ifIndex = portId;
      return true;
   }
   return false;
}

/**
 * Handler for VLAN enumeration on Avaya ERS
 */
static UINT32 HandlerVlanPorts(SNMP_Variable *var, SNMP_Transport *transport, void *context)
{
   uint32_t vlanId = var->getName().getLastElement();
   VlanList *vlanList = static_cast<VlanList*>(context);
   VlanInfo *vlan = vlanList->findById(vlanId);
   if (vlan == nullptr)
   {
      vlan = new VlanInfo(vlanId, VLAN_PRM_IFINDEX);
      vlanList->add(vlan);
   }

   // Get member ports
   // From MOXA MIB:
   // Each octet within this value specifies a set of eight
   // ports, with the first octet specifying ports 1 through
   // 8, the second octet specifying ports 9 through 16, etc.
   // Within each octet, the most significant bit represents
   // the lowest numbered port, and the least significant bit
   // represents the highest numbered port.  Thus, each port
   // of the bridge is represented by a single bit within the
   // value of this object.  If that bit has a value of '1'
   // then that port is included in the set of ports; the port
   // is not included if its bit has a value of '0'.
   BYTE portMask[256];
   size_t bytes = var->getRawValue(portMask, 256);
   uint32_t ifIndex = 1;
   for(size_t i = 0; i < bytes; i++)
   {
      BYTE mask = 0x80;
      while(mask > 0)
      {
         if (portMask[i] & mask)
         {
            vlan->add(ifIndex);
         }
         mask >>= 1;
         ifIndex++;
      }
   }

   return SNMP_ERR_SUCCESS;
}

/**
 * Get list of VLANs on given node
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @return VLAN list or NULL
 */
VlanList *MoxaEDRDriver::getVlans(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   VlanList *list = new VlanList();
   if ((SnmpWalk(snmp, _T(".1.3.6.1.4.1.8691.6.13.1.21.2.1.2"), HandlerVlanPorts, list) != SNMP_ERR_SUCCESS) &&
       (SnmpWalk(snmp, _T(".1.3.6.1.4.1.8691.6.13.1.21.2.1.3"), HandlerVlanPorts, list) != SNMP_ERR_SUCCESS) &&
       (SnmpWalk(snmp, _T(".1.3.6.1.4.1.8691.6.13.1.21.2.1.4"), HandlerVlanPorts, list) != SNMP_ERR_SUCCESS))
   {
      delete list;
      return nullptr;
   }
   return list;
}
