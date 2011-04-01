/* 
** NetXMS - Network Management System
** Driver for Avaya ERS switches (except ERS 8xxx) (former Nortel/Bay Networks BayStack)
** Copyright (C) 2003-2011 Victor Kirhenshtein
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
** File: vlan.cpp
**
**/

#include "baystack.h"


//
// VLAN information structure
//

struct VLAN_INFO
{
   TCHAR szName[MAX_OBJECT_NAME];
   DWORD dwVlanId;
   DWORD dwIfIndex;
   BYTE bMacAddr[MAC_ADDR_LENGTH];
};

struct VLAN_LIST
{
   DWORD dwNumVlans;
   VLAN_INFO *pList;
};


/**
 * Handler for VLAN enumeration on Avaya ERS
 */
static DWORD HandlerVlanIfList(DWORD dwVersion, SNMP_Variable *pVar,
                             SNMP_Transport *pTransport, void *pArg)
{
   DWORD dwIndex, oidName[MAX_OID_LEN], dwNameLen, dwResult;
   VLAN_LIST *pVlanList = (VLAN_LIST *)pArg;
   BYTE szBuffer[256];

   dwNameLen = pVar->GetName()->Length();

   // Extend VLAN list and set ID of new VLAN
   dwIndex = pVlanList->dwNumVlans;
   pVlanList->dwNumVlans++;
   pVlanList->pList = (VLAN_INFO *)realloc(pVlanList->pList, sizeof(VLAN_INFO) * pVlanList->dwNumVlans);
   pVlanList->pList[dwIndex].dwVlanId = pVar->GetValueAsUInt();

   // Get VLAN name
   memcpy(oidName, pVar->GetName()->GetValue(), dwNameLen * sizeof(DWORD));
   oidName[dwNameLen - 2] = 2;
   dwResult = SnmpGet(dwVersion, pTransport, NULL, oidName, dwNameLen, 
                      pVlanList->pList[dwIndex].szName, MAX_OBJECT_NAME, 0);
   if (dwResult != SNMP_ERR_SUCCESS)
      return dwResult;

   // Get VLAN interface index
   oidName[dwNameLen - 2] = 6;
   dwResult = SnmpGet(dwVersion, pTransport, NULL, oidName, dwNameLen, 
                      &pVlanList->pList[dwIndex].dwIfIndex, sizeof(DWORD), 0);
   if (dwResult != SNMP_ERR_SUCCESS)
      return dwResult;

   // Get VLAN MAC address
   oidName[dwNameLen - 2] = 19;
   memset(pVlanList->pList[dwIndex].bMacAddr, 0, MAC_ADDR_LENGTH);
   memset(szBuffer, 0, MAC_ADDR_LENGTH);
   dwResult = SnmpGet(dwVersion, pTransport, NULL, oidName, dwNameLen, szBuffer, 256, SG_RAW_RESULT);
   if (dwResult == SNMP_ERR_SUCCESS)
      memcpy(pVlanList->pList[dwIndex].bMacAddr, szBuffer, MAC_ADDR_LENGTH);
   return dwResult;
}

/**
 * Handler for VLAN enumeration on Passport
 */
static DWORD HandlerPassportIfList(DWORD dwVersion, SNMP_Variable *pVar,
                                  SNMP_Transport *pTransport, void *pArg)
{
   InterfaceList *pIfList = (InterfaceList *)pArg;
   VLAN_LIST *pVlanList = (VLAN_LIST *)pIfList->getData();
   DWORD oidName[MAX_OID_LEN], dwVlanIndex, dwIfIndex, dwNameLen, dwResult;

   dwIfIndex = pVar->GetValueAsUInt();
   for(dwVlanIndex = 0; dwVlanIndex < pVlanList->dwNumVlans; dwVlanIndex++)
      if (pVlanList->pList[dwVlanIndex].dwIfIndex == dwIfIndex)
         break;

   // Create new interface only if we have VLAN with same interface index
   if (dwVlanIndex < pVlanList->dwNumVlans)
   {
		NX_INTERFACE_INFO iface;

		memset(&iface, 0, sizeof(NX_INTERFACE_INFO));
      iface.dwIndex = dwIfIndex;
      _tcscpy(iface.szName, pVlanList->pList[dwVlanIndex].szName);
      iface.dwType = IFTYPE_OTHER;
      memcpy(iface.bMacAddr, pVlanList->pList[dwVlanIndex].bMacAddr, MAC_ADDR_LENGTH);
      
      dwNameLen = pVar->GetName()->Length();

      // Get IP address
      memcpy(oidName, pVar->GetName()->GetValue(), dwNameLen * sizeof(DWORD));
      oidName[dwNameLen - 6] = 2;
      dwResult = SnmpGet(dwVersion, pTransport, NULL, oidName, dwNameLen,
                         &iface.dwIpAddr, sizeof(DWORD), 0);

      if (dwResult == SNMP_ERR_SUCCESS)
      {
         // Get netmask
         oidName[dwNameLen - 6] = 3;
         dwResult = SnmpGet(dwVersion, pTransport, NULL, oidName, dwNameLen,
                            &iface.dwIpNetMask, sizeof(DWORD), 0);
      }

		pIfList->add(&iface);
   }
   else
   {
      dwResult = SNMP_ERR_SUCCESS;  // Ignore interfaces without matching VLANs
   }
   return dwResult;
}

/**
 * Get list of VLAN interfaces from Avaya ERS switch
 *
 * @param pTransport SNMP transport
 * @param pIfList interface list to be populated
 */
void GetVLANInterfaces(SNMP_Transport *pTransport, InterfaceList *pIfList)
{
   VLAN_LIST vlanList;

   // Get VLAN list
   memset(&vlanList, 0, sizeof(VLAN_LIST));
   SnmpEnumerate(pTransport->getSnmpVersion(), pTransport, _T(".1.3.6.1.4.1.2272.1.3.2.1.1"), HandlerVlanIfList, &vlanList, FALSE);

   // Get interfaces
   pIfList->setData(&vlanList);
   SnmpEnumerate(pTransport->getSnmpVersion(), pTransport, _T(".1.3.6.1.4.1.2272.1.8.2.1.1"), 
                 HandlerPassportIfList, pIfList, FALSE);
   safe_free(vlanList.pList);
}

/**
 * Handler for VLAN enumeration on Avaya ERS
 */
static DWORD HandlerVlanList(DWORD dwVersion, SNMP_Variable *pVar, SNMP_Transport *pTransport, void *pArg)
{
   DWORD oidName[MAX_OID_LEN], dwResult;
   VlanList *vlanList = (VlanList *)pArg;
	StringMap *attributes = (StringMap *)vlanList->getData();

   DWORD dwNameLen = pVar->GetName()->Length();
	VlanInfo *vlan = new VlanInfo(pVar->GetValueAsInt());

   // Get VLAN name
   memcpy(oidName, pVar->GetName()->GetValue(), dwNameLen * sizeof(DWORD));
   oidName[dwNameLen - 2] = 2;
   TCHAR buffer[256];
	dwResult = SnmpGet(dwVersion, pTransport, NULL, oidName, dwNameLen, buffer, 256, SG_STRING_RESULT);
   if (dwResult != SNMP_ERR_SUCCESS)
	{
		delete vlan;
      return dwResult;
	}
	vlan->setName(buffer);

   // Get member ports
	// From RapidCity MIB:
	//   The string is 32 octets long, for a total of 256 bits. Each bit 
	//   corresponds to a port, as represented by its ifIndex value . When a 
	//   bit has the value one(1), the corresponding port is a member of the 
	//   set. When a bit has the value zero(0), the corresponding port is not 
	//   a member of the set. The encoding is such that the most significant 
	//   bit of octet #1 corresponds to ifIndex 0, while the least significant 
	//   bit of octet #32 corresponds to ifIndex 255." 
	// Note: on newer devices port list can be longer
   oidName[dwNameLen - 2] = 12;
	BYTE portMask[256];
	memset(portMask, 0, sizeof(portMask));
   dwResult = SnmpGet(dwVersion, pTransport, NULL, oidName, dwNameLen, portMask, 256, SG_RAW_RESULT);
   if (dwResult != SNMP_ERR_SUCCESS)
	{
		delete vlan;
      return dwResult;
	}

	DWORD slotSize = attributes->getULong(_T(".baystack.slotSize"), 64);
	DWORD ifIndex = 0;

	for(int i = 0; i < 256; i++)
	{
		BYTE mask = 0x80;
		while(mask > 0)
		{
			if (portMask[i] & mask)
			{
				DWORD slot = ifIndex / slotSize + 1;
				DWORD port = ifIndex % slotSize;
				vlan->add(slot, port);
			}
			mask >>= 1;
			ifIndex++;
		}
	}

	vlanList->add(vlan);
   return SNMP_ERR_SUCCESS;
}

/**
 * Get VLANs 
 */
VlanList *BayStackDriver::getVlans(SNMP_Transport *snmp, StringMap *attributes)
{
	VlanList *list = new VlanList();
	list->setData(attributes);
	if (SnmpEnumerate(snmp->getSnmpVersion(), snmp, _T(".1.3.6.1.4.1.2272.1.3.2.1.1"), HandlerVlanList, list, FALSE) != SNMP_ERR_SUCCESS)
	{
		delete_and_null(list);
	}
	return list;
}
