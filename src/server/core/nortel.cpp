/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004 Victor Kirhenshtein
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
** $module: nortel.cpp
**
**/

#include "nms_core.h"


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


//
// Handler for VLAN enumeration on Passport
//

static void HandlerVlanList(DWORD dwAddr, const char *pszCommunity, variable_list *pVar, void *pArg)
{
   oid oidName[MAX_OID_LEN];
   VLAN_LIST *pVlanList = (VLAN_LIST *)pArg;
   DWORD dwIndex;
   BYTE szBuffer[256];

   // Extend VLAN list and set ID of new VLAN
   dwIndex = pVlanList->dwNumVlans;
   pVlanList->dwNumVlans++;
   pVlanList->pList = (VLAN_INFO *)realloc(pVlanList->pList, sizeof(VLAN_INFO) * pVlanList->dwNumVlans);
   pVlanList->pList[dwIndex].dwVlanId = *pVar->val.integer;

   // Get VLAN name
   memcpy(oidName, pVar->name, pVar->name_length * sizeof(oid));
   oidName[pVar->name_length - 2] = 2;
   SnmpGet(dwAddr, pszCommunity, NULL, oidName, pVar->name_length, 
           pVlanList->pList[dwIndex].szName, MAX_OBJECT_NAME, FALSE, FALSE);

   // Get VLAN interface index
   oidName[pVar->name_length - 2] = 6;
   SnmpGet(dwAddr, pszCommunity, NULL, oidName, pVar->name_length, 
           &pVlanList->pList[dwIndex].dwIfIndex, sizeof(DWORD), FALSE, FALSE);

   // Get VLAN MAC address
   oidName[pVar->name_length - 2] = 19;
   memset(pVlanList->pList[dwIndex].bMacAddr, 0, MAC_ADDR_LENGTH);
   memset(szBuffer, 0, MAC_ADDR_LENGTH);
   SnmpGet(dwAddr, pszCommunity, NULL, oidName, pVar->name_length, 
           szBuffer, 256, FALSE, FALSE);
   memcpy(pVlanList->pList[dwIndex].bMacAddr, szBuffer, MAC_ADDR_LENGTH);
}


//
// Handler for VLAN enumeration on Passport
//

static void HandlerPassportIfList(DWORD dwAddr, const char *pszCommunity, variable_list *pVar, void *pArg)
{
   oid oidName[MAX_OID_LEN];
   INTERFACE_LIST *pIfList = (INTERFACE_LIST *)pArg;
   VLAN_LIST *pVlanList = (VLAN_LIST *)pIfList->pArg;
   int iIndex;
   DWORD dwVlanIndex, dwIfIndex;

   dwIfIndex = *pVar->val.integer;
   for(dwVlanIndex = 0; dwVlanIndex < pVlanList->dwNumVlans; dwVlanIndex++)
      if (pVlanList->pList[dwVlanIndex].dwIfIndex == dwIfIndex)
         break;

   // Create new interface only if we have VLAN with same interface index
   if (dwVlanIndex < pVlanList->dwNumVlans)
   {
      iIndex = pIfList->iNumEntries;
      pIfList->iNumEntries++;
      pIfList->pInterfaces = (INTERFACE_INFO *)realloc(pIfList->pInterfaces, 
                                             sizeof(INTERFACE_INFO) * pIfList->iNumEntries);
      memset(&pIfList->pInterfaces[iIndex], 0, sizeof(INTERFACE_INFO));
      pIfList->pInterfaces[iIndex].dwIndex = dwIfIndex;
      _tcscpy(pIfList->pInterfaces[iIndex].szName, pVlanList->pList[dwVlanIndex].szName);
      pIfList->pInterfaces[iIndex].dwType = IFTYPE_OTHER;
      memcpy(pIfList->pInterfaces[iIndex].bMacAddr, pVlanList->pList[dwVlanIndex].bMacAddr, MAC_ADDR_LENGTH);
      
      // Get IP address
      memcpy(oidName, pVar->name, pVar->name_length * sizeof(oid));
      oidName[pVar->name_length - 6] = 2;
      SnmpGet(dwAddr, pszCommunity, NULL, oidName, pVar->name_length,
              &pIfList->pInterfaces[iIndex].dwIpAddr, sizeof(DWORD), FALSE, FALSE);

      // Get netmask
      oidName[pVar->name_length - 6] = 3;
      SnmpGet(dwAddr, pszCommunity, NULL, oidName, pVar->name_length,
              &pIfList->pInterfaces[iIndex].dwIpNetMask, sizeof(DWORD), FALSE, FALSE);
   }
}


//
// Get list of VLAN interfaces from Nortel Passport 8000/Accelar switch
//

void GetAccelarVLANIfList(DWORD dwIpAddr, const TCHAR *pszCommunity, INTERFACE_LIST *pIfList)
{
   VLAN_LIST vlanList;

   // Get VLAN list
   memset(&vlanList, 0, sizeof(VLAN_LIST));
   SnmpEnumerate(dwIpAddr, pszCommunity, _T(".1.3.6.1.4.1.2272.1.3.2.1.1"), 
                 HandlerVlanList, &vlanList, FALSE);

   // Get interfaces
   pIfList->pArg = &vlanList;
   SnmpEnumerate(dwIpAddr, pszCommunity, _T(".1.3.6.1.4.1.2272.1.8.2.1.1"), 
                 HandlerPassportIfList, pIfList, FALSE);
   safe_free(vlanList.pList);
}
