/* 
** NetXMS - Network Management System
** Generic driver for Avaya ERS switches (former Nortel)
** Copyright (C) 2003-2024 Victor Kirhenshtein
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
** File: vlan-if.cpp
**
**/

#include "avaya.h"

/**
 * VLAN information structure
 */
struct VLAN_INFO
{
   TCHAR name[MAX_OBJECT_NAME];
   uint32_t vlanId;
   uint32_t ifIndex;
   BYTE macAddress[MAC_ADDR_LENGTH];
};

/**
 * Handler for VLAN enumeration on Avaya ERS
 */
static uint32_t HandlerVlanIfList(SNMP_Variable *var, SNMP_Transport *snmp, StructArray<VLAN_INFO> *vlanList)
{
   // Extend VLAN list and set ID of new VLAN
   VLAN_INFO *vlan = vlanList->addPlaceholder();
   vlan->vlanId = var->getValueAsUInt();

   // Get VLAN name
   uint32_t oidName[MAX_OID_LEN];
   size_t nameLen = var->getName().length();
   memcpy(oidName, var->getName().value(), nameLen * sizeof(uint32_t));
   oidName[nameLen - 2] = 2;
   uint32_t rc = SnmpGetEx(snmp, nullptr, oidName, nameLen, vlan->name, MAX_OBJECT_NAME * sizeof(TCHAR), 0, nullptr);
   if (rc != SNMP_ERR_SUCCESS)
      return rc;

   // Get VLAN interface index
   oidName[nameLen - 2] = 6;
   rc = SnmpGetEx(snmp, nullptr, oidName, nameLen, &vlan->ifIndex, sizeof(uint32_t), 0, nullptr);
   if (rc != SNMP_ERR_SUCCESS)
      return rc;

   // Get VLAN MAC address
   oidName[nameLen - 2] = 19;
   memset(vlan->macAddress, 0, MAC_ADDR_LENGTH);
   BYTE szBuffer[256];
   memset(szBuffer, 0, MAC_ADDR_LENGTH);
   rc = SnmpGetEx(snmp, nullptr, oidName, nameLen, szBuffer, 256, SG_RAW_RESULT, nullptr);
   if (rc == SNMP_ERR_SUCCESS)
      memcpy(vlan->macAddress, szBuffer, MAC_ADDR_LENGTH);
   return rc;
}

/**
 * Handler for VLAN enumeration
 */
static uint32_t HandlerRapidCityIfList(SNMP_Variable *pVar, SNMP_Transport *pTransport, InterfaceList *pIfList)
{
   StructArray<VLAN_INFO> *pVlanList = static_cast<StructArray<VLAN_INFO>*>(pIfList->getData());
   uint32_t oidName[MAX_OID_LEN];

   uint32_t ifIndex = pVar->getValueAsUInt();
   int dwVlanIndex;
   for(dwVlanIndex = 0; dwVlanIndex < pVlanList->size(); dwVlanIndex++)
      if (pVlanList->get(dwVlanIndex)->ifIndex == ifIndex)
         break;

   // Create new interface only if we have VLAN with same interface index
   if (dwVlanIndex >= pVlanList->size())
      return SNMP_ERR_SUCCESS;  // Ignore interfaces without matching VLANs

   InterfaceInfo *iface = new InterfaceInfo(ifIndex);
   VLAN_INFO *vlan = pVlanList->get(dwVlanIndex);
   _tcscpy(iface->name, vlan->name);
   iface->type = IFTYPE_L2VLAN;
   memcpy(iface->macAddr, vlan->macAddress, MAC_ADDR_LENGTH);

   size_t nameLen = pVar->getName().length();

   // Get IP address
   uint32_t ipAddr, ipNetMask;

   memcpy(oidName, pVar->getName().value(), nameLen * sizeof(uint32_t));
   oidName[nameLen - 6] = 2;
   uint32_t rc = SnmpGetEx(pTransport, nullptr, oidName, nameLen, &ipAddr, sizeof(uint32_t), 0, nullptr);

   if (rc == SNMP_ERR_SUCCESS)
   {
      // Get netmask
      oidName[nameLen - 6] = 3;
      rc = SnmpGetEx(pTransport, nullptr, oidName, nameLen, &ipNetMask, sizeof(uint32_t), 0, nullptr);
   }

   if (rc == SNMP_ERR_SUCCESS)
   {
      iface->ipAddrList.add(InetAddress(ipAddr, ipNetMask));
   }

   pIfList->add(iface);
   return rc;
}

/**
 * Get list of VLAN interfaces from Avaya ERS switch
 *
 * @param pTransport SNMP transport
 * @param pIfList interface list to be populated
 */
void AvayaERSDriver::getVlanInterfaces(SNMP_Transport *pTransport, InterfaceList *pIfList)
{
   // Get VLAN list
   StructArray<VLAN_INFO> vlanList(0, 32);
   SnmpWalk(pTransport, _T(".1.3.6.1.4.1.2272.1.3.2.1.1"), HandlerVlanIfList, &vlanList);

   // Get interfaces
   pIfList->setData(&vlanList);
   SnmpWalk(pTransport, _T(".1.3.6.1.4.1.2272.1.8.2.1.1"), HandlerRapidCityIfList, pIfList);
}
