/* 
** NetXMS - Network Management System
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
** $module: datacoll.cpp
**
**/

#include "nms_core.h"


//
// Static data
//

static Queue *m_pItemQueue = NULL;


//
// Data collector
//

static THREAD_RESULT THREAD_CALL DataCollector(void *pArg)
{
   DCItem *pItem;
   Node *pNode;
   DWORD dwError;
   time_t currTime;
   char *pBuffer;

   pBuffer = (char *)malloc(MAX_LINE_SIZE);

   while(!ShutdownInProgress())
   {
      pItem = (DCItem *)m_pItemQueue->GetOrBlock();
      pNode = pItem->RelatedNode();
      if (pNode != NULL)
      {
         switch(pItem->DataSource())
         {
            case DS_INTERNAL:    // Server internal parameters (like status)
               dwError = pNode->GetInternalItem(pItem->Name(), MAX_LINE_SIZE, pBuffer);
               break;
            case DS_SNMP_AGENT:
               dwError = pNode->GetItemFromSNMP(pItem->Name(), MAX_LINE_SIZE, pBuffer);
               break;
            case DS_NATIVE_AGENT:
               dwError = pNode->GetItemFromAgent(pItem->Name(), MAX_LINE_SIZE, pBuffer);
               break;
         }

         // Update item's last poll time
         currTime = time(NULL);
         pItem->SetLastPollTime(currTime);

         // Transform and store received value into database or handle error
         switch(dwError)
         {
            case DCE_SUCCESS:
               pItem->NewValue(currTime, pBuffer);
               break;
            case DCE_COMM_ERROR:
               break;
            case DCE_NOT_SUPPORTED:
               // Change item's status
               pItem->SetStatus(ITEM_STATUS_NOT_SUPPORTED);
               break;
         }

         // Clear busy flag so item can be polled again
         pItem->SetBusyFlag(FALSE);
      }
      else     /* pNode == NULL */
      {
         DbgPrintf(AF_DEBUG_DC, "*** DataCollector: Attempt to collect information for non-existing node.\n");
      }
   }

   free(pBuffer);
   DbgPrintf(AF_DEBUG_DC, "Data collector thread terminated\n");
   return THREAD_OK;
}


//
// Item poller thread: check nodes' items and put into the 
// data collector queue when data polling required
//

static THREAD_RESULT THREAD_CALL ItemPoller(void *pArg)
{
   DWORD i, dwElapsed, dwWatchdogId;
   INT64 qwStart;

   dwWatchdogId = WatchdogAddThread("Item Poller", 20);

   while(!ShutdownInProgress())
   {
      if (SleepAndCheckForShutdown(2))
         break;      // Shutdown has arrived
      WatchdogNotify(dwWatchdogId);

      MutexLock(g_hMutexNodeIndex, INFINITE);
      qwStart = GetCurrentTimeMs();
      for(i = 0; i < g_dwNodeAddrIndexSize; i++)
         ((Node *)g_pNodeIndexByAddr[i].pObject)->QueueItemsForPolling(m_pItemQueue);
      MutexUnlock(g_hMutexNodeIndex);

      dwElapsed = (DWORD)(GetCurrentTimeMs() - qwStart);
   }
   DbgPrintf(AF_DEBUG_DC, "Item poller thread terminated\n");
   return THREAD_OK;
}


//
// Statistics collection thread
//

static THREAD_RESULT THREAD_CALL StatCollector(void *pArg)
{
   while(!ShutdownInProgress())
   {
      if (SleepAndCheckForShutdown(10))
         break;      // Shutdown has arrived

      if (g_dwFlags & AF_DEBUG_DC)
      {
//         printf("*** Poller Queue size: %d ***\n", m_pItemQueue->Size());
//         printf("*** DB Writer Queue size: %d ***\n", g_pLazyRequestQueue->Size());
      }
   }
   return THREAD_OK;
}


//
// Initialize data collection subsystem
//

BOOL InitDataCollector(void)
{
   int i, iNumCollectors;

   // Create collection requests queue
   m_pItemQueue = new Queue(4096, 256);

   // Start data collection threads
   iNumCollectors = ConfigReadInt("NumberOfDataCollectors", 10);
   for(i = 0; i < iNumCollectors; i++)
      ThreadCreate(DataCollector, 0, NULL);

   // Start item poller thread
   ThreadCreate(ItemPoller, 0, NULL);

   // Start statistics collection thread
   ThreadCreate(StatCollector, 0, NULL);

   return TRUE;
}
