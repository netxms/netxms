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
static DCI_ENVELOPE *m_pItems = NULL;
static DWORD m_dwNumItems = 0;
static MUTEX m_hMutexListAccess;


//
// Data collector
//

static void DataCollector(void *pArg)
{
   DCI_ENVELOPE *pEnvelope;
   Node *pNode;
   DWORD dwError;
   time_t currTime;
   char *pBuffer, szQuery[MAX_LINE_SIZE + 128];

   pBuffer = (char *)malloc(MAX_LINE_SIZE);

   while(1)
   {
      pEnvelope = (DCI_ENVELOPE *)m_pItemQueue->GetOrBlock();
      pNode = (Node *)FindObjectById(pEnvelope->dwNodeId);
      if (pNode != NULL)
      {
         switch(pEnvelope->iDataSource)
         {
            case DS_INTERNAL:    // Server internal parameters (like status)
               dwError = pNode->GetInternalItem(pEnvelope->szItemName, MAX_LINE_SIZE, pBuffer);
               break;
            case DS_SNMP_AGENT:
               dwError = pNode->GetItemFromSNMP(pEnvelope->szItemName, MAX_LINE_SIZE, pBuffer);
               break;
            case DS_NATIVE_AGENT:
               dwError = pNode->GetItemFromAgent(pEnvelope->szItemName, MAX_LINE_SIZE, pBuffer);
               break;
         }

         // Update item's last poll time
         currTime = time(NULL);
         pNode->SetItemLastPollTime(pEnvelope->dwItemId, currTime);

         // Store received value into database or handle error
         switch(dwError)
         {
            case DCE_SUCCESS:
               sprintf(szQuery, "INSERT INTO idata_%d (item_id,timestamp,value)"
                                " VALUES (%d,%d,'%s')", pEnvelope->dwNodeId,
                       pEnvelope->dwItemId, currTime, pBuffer);
               DBQuery(g_hCoreDB, szQuery);
               break;
            case DCE_COMM_ERROR:
               break;
            case DCE_NOT_SUPPORTED:
               // Change item's status
               pNode->SetItemStatus(pEnvelope->dwItemId, ITEM_STATUS_NOT_SUPPORTED);
               break;
         }
      }
      else     /* pNode == NULL */
      {
printf("Attempt to collect information for non-existing node.\n");
      }
   }
}


//
// Initialize data collection subsystem
//

BOOL InitDataCollector(void)
{
   int i, iNumCollectors;

   // Create mutex for synchronizing access to item list
   m_hMutexListAccess = MutexCreate();

   // Create collection requests queue
   m_pItemQueue = new Queue;

   // Start data collection threads
   iNumCollectors = ConfigReadInt("NumberOfDataCollectors", 10);
   for(i = 0; i < iNumCollectors; i++)
      ThreadCreate(DataCollector, 0, NULL);

   return TRUE;
}


/*
//
// Delete all items for specific node from polls
//

void DeleteAllItemsForNode(DWORD dwNodeId)
{
   DWORD i;

   MutexLock(m_hMutexListAccess, INFINITE);

   for(i = 0; i < m_dwNumItems; i++)
      if (m_pItems[i].pNode->Id() == dwNodeId)
         m_pItems[i].bDeleted = TRUE;

   MutexUnlock(m_hMutexListAccess);
}
*/
