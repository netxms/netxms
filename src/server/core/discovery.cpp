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

#ifdef _WIN32
   MIB_IPNETTABLE *sysArpCache;
   DWORD i, dwError, dwSize;

   sysArpCache = (MIB_IPNETTABLE *)malloc(SIZEOF_IPNETTABLE(4096));
   if (sysArpCache == NULL)
      return NULL;

   dwSize = SIZEOF_IPNETTABLE(4096);
   dwError = GetIpNetTable(sysArpCache, &dwSize, FALSE);
   if (dwError != NO_ERROR)
   {
      WriteLog(MSG_GETIPNETTABLE_FAILED, EVENTLOG_ERROR_TYPE, "e", NULL);
      free(sysArpCache);
      return NULL;
   }

   pArpCache = (ARP_CACHE *)malloc(sizeof(ARP_CACHE));
   pArpCache->dwNumEntries = 0;
   pArpCache->pEntries = (ARP_ENTRY *)malloc(sizeof(ARP_ENTRY) * sysArpCache->dwNumEntries);

   for(i = 0; i < sysArpCache->dwNumEntries; i++)
      if ((sysArpCache->table[i].dwType == 3) ||
          (sysArpCache->table[i].dwType == 4))  // Only static and dynamic entries
      {
         pArpCache->pEntries[pArpCache->dwNumEntries].dwIndex = sysArpCache->table[i].dwIndex;
         pArpCache->pEntries[pArpCache->dwNumEntries].dwIpAddr = sysArpCache->table[i].dwAddr;
         memcpy(pArpCache->pEntries[pArpCache->dwNumEntries].bMacAddr, sysArpCache->table[i].bPhysAddr, 6);
         pArpCache->dwNumEntries++;
      }

   free(sysArpCache);
#else
   /* TODO: Add UNIX code here */
#endif

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

   CleanInterfaceList(pIfList);

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
         NetObjInsert(pNode, TRUE);
         if (!pNode->NewNodePoll(0))
         {
            NetObjDelete(pNode);
            delete pNode;     // Node poll failed, delete it
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
   DWORD dwNewNodeId = 1;
   Node *pNode;

   // Flush new nodes table
   DBQuery(g_hCoreDB, "DELETE FROM newnodes");

   while(1)
   {
      ThreadSleep(30);

printf("* Discovery thread wake up\n");
      CheckForMgmtNode();

      // Walk through nodes and poll for ARP tables
      for(DWORD i = 0; i < g_dwNodeAddrIndexSize; i++)
      {
         pNode = (Node *)g_pNodeIndexByAddr[i].pObject;
         if (pNode->ReadyForDiscoveryPoll())
         {
            ARP_CACHE *pArpCache;

printf("Discovery poll on node %s\n",pNode->Name());
            // Retrieve and analize node's ARP cache
            pArpCache = pNode->GetArpCache();
            if (pArpCache != NULL)
            {
               for(DWORD j = 0; j < pArpCache->dwNumEntries; j++)
               {
                  if (FindNodeByIP(pArpCache->pEntries[j].dwIpAddr) == NULL)
                  {
                     Interface *pInterface = pNode->FindInterface(pArpCache->pEntries[j].dwIndex,
                                                                  pArpCache->pEntries[j].dwIpAddr);
                     if (pInterface != NULL)
                        if (!IsBroadcastAddress(pArpCache->pEntries[j].dwIpAddr,
                                                pInterface->IpNetMask()))
                        {
                           char szQuery[256];

                           sprintf(szQuery, "INSERT INTO newnodes (id,ip_addr,ip_netmask,discovery_flags) VALUES (%d,%d,%d,%d)",
                                   dwNewNodeId++, pArpCache->pEntries[j].dwIpAddr,
                                   pInterface->IpNetMask(), DF_DEFAULT);
                           DBQuery(g_hCoreDB, szQuery);
                        }
                  }
               }
               DestroyArpCache(pArpCache);
            }

            pNode->SetDiscoveryPollTimeStamp();
         }
      }
printf("* Discovery thread goes to sleep\n");
   }
}
