/* 
** NetXMS - Network Management System
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
** File: datacoll.cpp
**
**/

#include "nxcore.h"


//
// Constants
//

#define ITEM_POLLING_INTERVAL    2


//
// Externals
//

extern Queue g_statusPollQueue;
extern Queue g_configPollQueue;


//
// Global data
//

double g_dAvgPollerQueueSize = 0;
double g_dAvgDBWriterQueueSize = 0;
double g_dAvgIDataWriterQueueSize = 0;
double g_dAvgDBAndIDataWriterQueueSize = 0;
double g_dAvgStatusPollerQueueSize = 0;
double g_dAvgConfigPollerQueueSize = 0;
DWORD g_dwAvgDCIQueuingTime = 0;
Queue *g_pItemQueue = NULL;


//
// Data collector
//

static THREAD_RESULT THREAD_CALL DataCollector(void *pArg)
{
   DCItem *pItem;
   Node *pNode;
   DWORD dwError;
   time_t currTime;
   TCHAR *pBuffer;

   pBuffer = (TCHAR *)malloc(MAX_LINE_SIZE * sizeof(TCHAR));

   while(!IsShutdownInProgress())
   {
      pItem = (DCItem *)g_pItemQueue->GetOrBlock();
		pNode = (Node *)pItem->getRelatedNode();

		if (pItem->isScheduledForDeletion())
		{
	      DbgPrintf(7, _T("DataCollector(): about to destroy DCI %d \"%s\" node=%d"),
			          pItem->getId(), pItem->getName(), (pNode != NULL) ? pNode->Id() : -1);
			pItem->deleteFromDB();
			delete pItem;
			continue;
		}

      DbgPrintf(8, _T("DataCollector(): processing DCI %d \"%s\" node=%d proxy=%d"),
		          pItem->getId(), pItem->getName(), (pNode != NULL) ? pNode->Id() : -1, pItem->getProxyNode());
		if (pItem->getProxyNode() != 0)
		{
			NetObj *object;

			object = FindObjectById(pItem->getProxyNode());
			if (object != NULL)
			{
				if ((object->Type() == OBJECT_NODE) &&
					(object->IsTrustedNode((pNode != NULL) ? pNode->Id() : 0)))
				{
					pNode = (Node *)object;
					pNode->IncRefCount();
				}
				else
				{
               // Change item's status to _T("not supported")
               pItem->setStatus(ITEM_STATUS_NOT_SUPPORTED, true);

					if (pNode != NULL)
					{
						pNode->DecRefCount();
						pNode = NULL;
					}
				}
			}
			else
			{
				if (pNode != NULL)
				{
					pNode->DecRefCount();
					pNode = NULL;
				}
			}
		}

      if (pNode != NULL)
      {
         switch(pItem->getDataSource())
         {
            case DS_INTERNAL:    // Server internal parameters (like status)
               dwError = pNode->GetInternalItem(pItem->getName(), MAX_LINE_SIZE, pBuffer);
               break;
            case DS_SNMP_AGENT:
					dwError = pNode->getItemFromSNMP(pItem->getSnmpPort(), pItem->getName(), MAX_LINE_SIZE, 
						pBuffer, pItem->isInterpretSnmpRawValue() ? (int)pItem->getSnmpRawValueType() : SNMP_RAWTYPE_NONE);
               break;
            case DS_CHECKPOINT_AGENT:
               dwError = pNode->GetItemFromCheckPointSNMP(pItem->getName(), MAX_LINE_SIZE, pBuffer);
               break;
            case DS_NATIVE_AGENT:
               dwError = pNode->GetItemFromAgent(pItem->getName(), MAX_LINE_SIZE, pBuffer);
               break;
         }

         // Get polling time
         currTime = time(NULL);

         // Transform and store received value into database or handle error
         switch(dwError)
         {
            case DCE_SUCCESS:
					if (pItem->getStatus() == ITEM_STATUS_NOT_SUPPORTED)
	               pItem->setStatus(ITEM_STATUS_ACTIVE, true);
					((Node *)pItem->getRelatedNode())->processNewDciValue(pItem, currTime, pBuffer);
               break;
            case DCE_COMM_ERROR:
               pItem->processNewError();
               break;
            case DCE_NOT_SUPPORTED:
               // Change item's status
               pItem->setStatus(ITEM_STATUS_NOT_SUPPORTED, true);
               break;
         }

         // Decrement node's usage counter
         pNode->DecRefCount();
			if ((pItem->getProxyNode() != 0) && (pItem->getRelatedNode() != NULL))
			{
				pItem->getRelatedNode()->DecRefCount();
			}
      }
      else     /* pNode == NULL */
      {
			Template *n = pItem->getRelatedNode();
         DbgPrintf(3, _T("*** DataCollector: Attempt to collect information for non-existing node (DCI=%d \"%s\" node=%d proxy=%d)"),
			          pItem->getId(), pItem->getName(), (n != NULL) ? n->Id() : -1, pItem->getProxyNode());
      }

		// Update item's last poll time and clear busy flag so item can be polled again
      pItem->setLastPollTime(currTime);
      pItem->setBusyFlag(FALSE);
   }

   free(pBuffer);
   DbgPrintf(1, _T("Data collector thread terminated"));
   return THREAD_OK;
}


//
// Callback for queueing DCIs
//

static void QueueItems(NetObj *object, void *data)
{
	DbgPrintf(8, _T("ItemPoller: calling Node::queueItemsForPolling for node %s [%d]"),
				 object->Name(), object->Id());
	((Node *)object)->queueItemsForPolling(g_pItemQueue);
}


//
// Item poller thread: check nodes' items and put into the 
// data collector queue when data polling required
//

static THREAD_RESULT THREAD_CALL ItemPoller(void *pArg)
{
   DWORD dwSum, dwWatchdogId, dwCurrPos = 0;
   DWORD dwTimingHistory[60 / ITEM_POLLING_INTERVAL];
   INT64 qwStart;

   dwWatchdogId = WatchdogAddThread(_T("Item Poller"), 20);
   memset(dwTimingHistory, 0, sizeof(DWORD) * (60 / ITEM_POLLING_INTERVAL));

   while(!IsShutdownInProgress())
   {
      if (SleepAndCheckForShutdown(ITEM_POLLING_INTERVAL))
         break;      // Shutdown has arrived
      WatchdogNotify(dwWatchdogId);
		DbgPrintf(8, _T("ItemPoller: wakeup"));

      qwStart = GetCurrentTimeMs();
		g_idxNodeById.forEach(QueueItems, NULL);

      // Save last poll time
      dwTimingHistory[dwCurrPos] = (DWORD)(GetCurrentTimeMs() - qwStart);
      dwCurrPos++;
      if (dwCurrPos == (60 / ITEM_POLLING_INTERVAL))
         dwCurrPos = 0;

      // Calculate new average for last minute
		dwSum = 0;
      for(int i = 0; i < (60 / ITEM_POLLING_INTERVAL); i++)
         dwSum += dwTimingHistory[i];
      g_dwAvgDCIQueuingTime = dwSum / (60 / ITEM_POLLING_INTERVAL);
   }
   DbgPrintf(1, _T("Item poller thread terminated"));
   return THREAD_OK;
}


//
// Statistics collection thread
//

static THREAD_RESULT THREAD_CALL StatCollector(void *pArg)
{
   DWORD i, dwCurrPos = 0;
   DWORD dwPollerQS[12], dwDBWriterQS[12];
   DWORD dwIDataWriterQS[12], dwDBAndIDataWriterQS[12];
   DWORD dwStatusPollerQS[12], dwConfigPollerQS[12];
   double dSum1, dSum2, dSum3, dSum4, dSum5, dSum6;

   memset(dwPollerQS, 0, sizeof(DWORD) * 12);
   memset(dwDBWriterQS, 0, sizeof(DWORD) * 12);
   memset(dwIDataWriterQS, 0, sizeof(DWORD) * 12);
   memset(dwDBAndIDataWriterQS, 0, sizeof(DWORD) * 12);
   memset(dwStatusPollerQS, 0, sizeof(DWORD) * 12);
   memset(dwConfigPollerQS, 0, sizeof(DWORD) * 12);
   g_dAvgPollerQueueSize = 0;
   g_dAvgDBWriterQueueSize = 0;
   g_dAvgIDataWriterQueueSize = 0;
   g_dAvgDBAndIDataWriterQueueSize = 0;
   g_dAvgStatusPollerQueueSize = 0;
   g_dAvgConfigPollerQueueSize = 0;
   while(!IsShutdownInProgress())
   {
      if (SleepAndCheckForShutdown(5))
         break;      // Shutdown has arrived

      // Get current values
      dwPollerQS[dwCurrPos] = g_pItemQueue->Size();
      dwDBWriterQS[dwCurrPos] = g_pLazyRequestQueue->Size();
      dwIDataWriterQS[dwCurrPos] = g_pIDataInsertQueue->Size();
      dwDBAndIDataWriterQS[dwCurrPos] = g_pLazyRequestQueue->Size() + g_pIDataInsertQueue->Size();
      dwStatusPollerQS[dwCurrPos] = g_statusPollQueue.Size();
      dwConfigPollerQS[dwCurrPos] = g_configPollQueue.Size();
      dwCurrPos++;
      if (dwCurrPos == 12)
         dwCurrPos = 0;

      // Calculate new averages
      for(i = 0, dSum1 = 0, dSum2 = 0, dSum3 = 0, dSum4 = 0, dSum5 = 0, dSum6 = 0; i < 12; i++)
      {
         dSum1 += dwPollerQS[i];
         dSum2 += dwDBWriterQS[i];
         dSum3 += dwIDataWriterQS[i];
         dSum4 += dwDBAndIDataWriterQS[i];
         dSum5 += dwStatusPollerQS[i];
         dSum6 += dwConfigPollerQS[i];
      }
      g_dAvgPollerQueueSize = dSum1 / 12;
      g_dAvgDBWriterQueueSize = dSum2 / 12;
      g_dAvgIDataWriterQueueSize = dSum3 / 12;
      g_dAvgDBAndIDataWriterQueueSize = dSum4 / 12;
      g_dAvgStatusPollerQueueSize = dSum5 / 12;
      g_dAvgConfigPollerQueueSize = dSum6 / 12;
   }
   return THREAD_OK;
}


//
// Initialize data collection subsystem
//

BOOL InitDataCollector()
{
   int i, iNumCollectors;

   // Create collection requests queue
   g_pItemQueue = new Queue(4096, 256);

   // Start data collection threads
   iNumCollectors = ConfigReadInt(_T("NumberOfDataCollectors"), 10);
   for(i = 0; i < iNumCollectors; i++)
      ThreadCreate(DataCollector, 0, NULL);

   // Start item poller thread
   ThreadCreate(ItemPoller, 0, NULL);

   // Start statistics collection thread
   ThreadCreate(StatCollector, 0, NULL);

   return TRUE;
}


//
// Write full list of supported parameters (from all nodes) to message
//

struct __param_list
{
	DWORD size;
	NXC_AGENT_PARAM *data;
};

static void UpdateParamList(NetObj *object, void *data)
{
	struct __param_list *fullList = (struct __param_list *)data;

	NXC_AGENT_PARAM *paramList;
	DWORD numParams;
	((Node *)object)->openParamList(&numParams, &paramList);
	if ((numParams > 0) && (paramList != NULL))
	{
		fullList->data = (NXC_AGENT_PARAM *)realloc(fullList->data, sizeof(NXC_AGENT_PARAM) * (fullList->size + numParams));
		for(DWORD i = 0; i < numParams; i++)
		{
			DWORD j;
			for(j = 0; j < fullList->size; j++)
			{
				if (!_tcsicmp(paramList[i].szName, fullList->data[j].szName))
					break;
			}

			if (j == fullList->size)
			{
				memcpy(&fullList->data[j], &paramList[i], sizeof(NXC_AGENT_PARAM));
				fullList->size++;
			}
		}
	}
	((Node *)object)->closeParamList();
}

void WriteFullParamListToMessage(CSCPMessage *pMsg)
{
   // Gather full parameter list
	struct __param_list fullList;
	fullList.size = 0;
	fullList.data = NULL;
	g_idxNodeById.forEach(UpdateParamList, &fullList);

   // Put list into the message
   pMsg->SetVariable(VID_NUM_PARAMETERS, fullList.size);
   for(DWORD i = 0, varId = VID_PARAM_LIST_BASE; i < fullList.size; i++)
   {
		pMsg->SetVariable(varId++, fullList.data[i].szName);
		pMsg->SetVariable(varId++, fullList.data[i].szDescription);
		pMsg->SetVariable(varId++, (WORD)fullList.data[i].iDataType);
   }

	safe_free(fullList.data);
}
