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

#include "nxcore.h"


//
// Check if management server node presented in node list
//

void CheckForMgmtNode(void)
{
   INTERFACE_LIST *pIfList;
   Node *pNode;
   int i;

   pIfList = GetLocalInterfaceList();
   if (pIfList != NULL)
   {
      for(i = 0; i < pIfList->iNumEntries; i++)
         if ((pNode = FindNodeByIP(pIfList->pInterfaces[i].dwIpAddr)) != NULL)
         {
            g_dwMgmtNode = pNode->Id();   // Set local management node ID
            break;
         }
      if (i == pIfList->iNumEntries)   // No such node
      {
         // Find interface with IP address
         for(i = 0; i < pIfList->iNumEntries; i++)
            if (pIfList->pInterfaces[i].dwIpAddr != 0)
            {
               pNode = new Node(pIfList->pInterfaces[i].dwIpAddr, NF_IS_LOCAL_MGMT, DF_DEFAULT);
               NetObjInsert(pNode, TRUE);
               pNode->NewNodePoll(0);
               g_dwMgmtNode = pNode->Id();   // Set local management node ID
               
               // Add default data collection items
               pNode->AddItem(new DCItem(CreateUniqueId(IDG_ITEM), "Status", 
                                         DS_INTERNAL, DCI_DT_INT, 60, 30, pNode));
               pNode->AddItem(new DCItem(CreateUniqueId(IDG_ITEM), 
                                         "Server.AverageDCPollerQueueSize", 
                                         DS_INTERNAL, DCI_DT_FLOAT, 60, 30, pNode,
                                         "Average length of data collection poller's request queue for last minute"));
               pNode->AddItem(new DCItem(CreateUniqueId(IDG_ITEM), 
                                         "Server.AverageDBWriterQueueSize", 
                                         DS_INTERNAL, DCI_DT_FLOAT, 60, 30, pNode,
                                         "Average length of database writer's request queue for last minute"));
               break;
            }
      }
      DestroyInterfaceList(pIfList);
   }
}


//
// Network discovery thread
//

THREAD_RESULT THREAD_CALL DiscoveryThread(void *arg)
{
   DWORD dwNewNodeId = 1, dwWatchdogId;
   Node *pNode;
   char szIpAddr[16], szNetMask[16];

   dwWatchdogId = WatchdogAddThread("Network Discovery Thread", 90);

   // Flush new nodes table
   DBQuery(g_hCoreDB, "DELETE FROM new_nodes");

   while(!ShutdownInProgress())
   {
      if (SleepAndCheckForShutdown(30))
         break;      // Shutdown has arrived
      WatchdogNotify(dwWatchdogId);

      CheckForMgmtNode();

      // Walk through nodes and poll for ARP tables
      RWLockReadLock(g_rwlockNodeIndex, INFINITE);
      for(DWORD i = 0; i < g_dwNodeAddrIndexSize; i++)
      {
         pNode = (Node *)g_pNodeIndexByAddr[i].pObject;
         if (pNode->ReadyForDiscoveryPoll())
         {
            ARP_CACHE *pArpCache;

            DbgPrintf(AF_DEBUG_DISCOVERY, _T("Starting discovery poll for node %s (%s)"),
                      pNode->Name(), IpToStr(pNode->IpAddr(), szIpAddr));
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

                           sprintf(szQuery, "INSERT INTO new_nodes (id,ip_addr,ip_netmask,discovery_flags) VALUES (%d,'%s','%s',%d)",
                                   dwNewNodeId++, IpToStr(pArpCache->pEntries[j].dwIpAddr, szIpAddr),
                                   IpToStr(pInterface->IpNetMask(), szNetMask), DF_DEFAULT);
                           DBQuery(g_hCoreDB, szQuery);
                        }
                  }
               }
               DestroyArpCache(pArpCache);
            }

            DbgPrintf(AF_DEBUG_DISCOVERY, _T("Finished discovery poll for node %s (%s)"),
                      pNode->Name(), IpToStr(pNode->IpAddr(), szIpAddr));
            pNode->SetDiscoveryPollTimeStamp();
         }
      }
      RWLockUnlock(g_rwlockNodeIndex);
   }

   DbgPrintf(AF_DEBUG_DISCOVERY, "Discovery thread terminated");
   return THREAD_OK;
}
