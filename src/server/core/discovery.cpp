/* 
** Project X - Network Management System
** Copyright (C) 2003 Victor Kirhenshtein
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
** $module: discovery.cpp
**
**/

#include "nms_core.h"

#ifdef _WIN32
#include <iphlpapi.h>
#include <iprtrmib.h>
#include <rtinfo.h>
#endif


//
// Get local ARP cache
//

ARP_CACHE *GetLocalArpCache(void)
{
   ARP_CACHE *pArpCache = NULL;

   return pArpCache;
}


//
// Get local interface list
//

INTERFACE_LIST *GetLocalInterfaceList(void)
{
   INTERFACE_LIST *pIfList;

#ifdef _WIN32
   MIB_IPADDRTABLE *ifTable;
   MIB_IFROW ifRow;
   DWORD i, dwSize, dwError, dwIfType;
   char szIfName[32], *pszIfName;

   ifTable = (MIB_IPADDRTABLE *)malloc(SIZEOF_IPADDRTABLE(256));
   if (ifTable == NULL)
      return NULL;

   dwSize = SIZEOF_IPADDRTABLE(256);
   dwError = GetIpAddrTable(ifTable, &dwSize, FALSE);
   if (dwError != NO_ERROR)
   {
      WriteLog(MSG_GETIPADDRTABLE_FAILED, EVENTLOG_ERROR_TYPE, "e", NULL);
      free(ifTable);
      return NULL;
   }

   // Allocate and initialize interface list structure
   pIfList = (INTERFACE_LIST *)malloc(sizeof(INTERFACE_LIST));
   pIfList->iNumEntries = ifTable->dwNumEntries;
   pIfList->pInterfaces = (INTERFACE_INFO *)malloc(sizeof(INTERFACE_INFO) * ifTable->dwNumEntries);
   memset(pIfList->pInterfaces, 0, sizeof(INTERFACE_INFO) * ifTable->dwNumEntries);

   // Fill in our interface list
   for(i = 0; i < ifTable->dwNumEntries; i++)
   {
      ifRow.dwIndex = ifTable->table[i].dwIndex;
      if (GetIfEntry(&ifRow) == NO_ERROR)
      {
         dwIfType = ifRow.dwType;
         pszIfName = (char *)ifRow.bDescr;
      }
      else
      {
         dwIfType = IFTYPE_OTHER;
         sprintf(szIfName, "IF-UNKNOWN-%d", ifTable->table[i].dwIndex);
         pszIfName = szIfName;
      }
      pIfList->pInterfaces[i].dwIndex = ifTable->table[i].dwIndex;
      pIfList->pInterfaces[i].dwIpAddr = ifTable->table[i].dwAddr;
      pIfList->pInterfaces[i].dwIpNetMask = ifTable->table[i].dwMask;
      pIfList->pInterfaces[i].dwType = dwIfType;
      strncpy(pIfList->pInterfaces[i].szName, pszIfName, MAX_OBJECT_NAME - 1);
   }

   free(ifTable);

   // Delete loopback interface(s) from list
   for(int j = 0; j < pIfList->iNumEntries; j++)
      if ((pIfList->pInterfaces[j].dwIpAddr & pIfList->pInterfaces[j].dwIpNetMask) == 0x0000007F)
      {
         pIfList->iNumEntries--;
         memmove(&pIfList->pInterfaces[j], &pIfList->pInterfaces[j + 1],
                 sizeof(INTERFACE_INFO) * (pIfList->iNumEntries - j));
         j--;
      }
#else
   /* TODO: Add UNIX code here */
#endif

   return pIfList;
}


//
// Check if management server node presented in node list
//

static void CheckForMgmtNode(void)
{
   INTERFACE_LIST *pIfList;
   int i;

   pIfList = GetLocalInterfaceList();
   if (pIfList != NULL)
   {
      for(i = 0; i < pIfList->iNumEntries; i++)
         if (FindNodeByIP(pIfList->pInterfaces[i].dwIpAddr) != NULL)
            break;
      if (i == pIfList->iNumEntries)   // No such node
      {
         Node *pNode = new Node(pIfList->pInterfaces[0].dwIpAddr, NF_IS_LOCAL_MGMT, DF_DEFAULT);
         if (!pNode->NewNodePoll(0))
         {
            delete pNode;     // Node poll failed, delete it
         }
         else
         {
            NetObjInsert(pNode, TRUE);
         }
      }
      DestroyInterfaceList(pIfList);
   }
}


//
// Network discovery thread
//

void DiscoveryThread(void *arg)
{
//   ARP_CACHE *pArpCache;
   DWORD dwNewNodeId = 1;

   while(1)
   {
      ThreadSleep(60);

      CheckForMgmtNode();
   }
}
