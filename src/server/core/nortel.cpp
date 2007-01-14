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

#include "nxcore.h"


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

static DWORD HandlerVlanList(DWORD dwVersion, const char *pszCommunity, SNMP_Variable *pVar,
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
   dwResult = SnmpGet(dwVersion, pTransport, pszCommunity, NULL, oidName, dwNameLen, 
                      pVlanList->pList[dwIndex].szName, MAX_OBJECT_NAME, FALSE, FALSE);
   if (dwResult != SNMP_ERR_SUCCESS)
      return dwResult;

   // Get VLAN interface index
   oidName[dwNameLen - 2] = 6;
   dwResult = SnmpGet(dwVersion, pTransport, pszCommunity, NULL, oidName, dwNameLen, 
                      &pVlanList->pList[dwIndex].dwIfIndex, sizeof(DWORD), FALSE, FALSE);
   if (dwResult != SNMP_ERR_SUCCESS)
      return dwResult;

   // Get VLAN MAC address
   oidName[dwNameLen - 2] = 19;
   memset(pVlanList->pList[dwIndex].bMacAddr, 0, MAC_ADDR_LENGTH);
   memset(szBuffer, 0, MAC_ADDR_LENGTH);
   dwResult = SnmpGet(dwVersion, pTransport, pszCommunity, NULL, oidName, dwNameLen, 
                      szBuffer, 256, FALSE, FALSE);
   if (dwResult == SNMP_ERR_SUCCESS)
      memcpy(pVlanList->pList[dwIndex].bMacAddr, szBuffer, MAC_ADDR_LENGTH);
   return dwResult;
}


//
// Handler for VLAN enumeration on Passport
//

static DWORD HandlerPassportIfList(DWORD dwVersion, const char *pszCommunity, SNMP_Variable *pVar,
                                  SNMP_Transport *pTransport, void *pArg)
{
   INTERFACE_LIST *pIfList = (INTERFACE_LIST *)pArg;
   VLAN_LIST *pVlanList = (VLAN_LIST *)pIfList->pArg;
   int iIndex;
   DWORD oidName[MAX_OID_LEN], dwVlanIndex, dwIfIndex, dwNameLen, dwResult;

   dwIfIndex = pVar->GetValueAsUInt();
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
      
      dwNameLen = pVar->GetName()->Length();

      // Get IP address
      memcpy(oidName, pVar->GetName()->GetValue(), dwNameLen * sizeof(DWORD));
      oidName[dwNameLen - 6] = 2;
      dwResult = SnmpGet(dwVersion, pTransport, pszCommunity, NULL, oidName, dwNameLen,
                         &pIfList->pInterfaces[iIndex].dwIpAddr, sizeof(DWORD), FALSE, FALSE);

      if (dwResult == SNMP_ERR_SUCCESS)
      {
         // Get netmask
         oidName[dwNameLen - 6] = 3;
         dwResult = SnmpGet(dwVersion, pTransport, pszCommunity, NULL, oidName, dwNameLen,
                            &pIfList->pInterfaces[iIndex].dwIpNetMask, sizeof(DWORD), FALSE, FALSE);
      }
   }
   else
   {
      dwResult = SNMP_ERR_SUCCESS;  // Ignore interfaces without matching VLANs
   }
   return dwResult;
}


//
// Get list of VLAN interfaces from Nortel Passport 8000/Accelar switch
//

void GetAccelarVLANIfList(DWORD dwVersion, SNMP_Transport *pTransport, 
                          const TCHAR *pszCommunity, INTERFACE_LIST *pIfList)
{
   VLAN_LIST vlanList;

   // Get VLAN list
   memset(&vlanList, 0, sizeof(VLAN_LIST));
   SnmpEnumerate(dwVersion, pTransport, pszCommunity, _T(".1.3.6.1.4.1.2272.1.3.2.1.1"), 
                 HandlerVlanList, &vlanList, FALSE);

   // Get interfaces
   pIfList->pArg = &vlanList;
   SnmpEnumerate(dwVersion, pTransport, pszCommunity, _T(".1.3.6.1.4.1.2272.1.8.2.1.1"), 
                 HandlerPassportIfList, pIfList, FALSE);
   safe_free(vlanList.pList);
}
