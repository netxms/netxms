/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2013 Victor Kirhenshtein
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

/**
 * Interval between DCI polling
 */
#define ITEM_POLLING_INTERVAL    1	

/**
 * Externals
 */
extern Queue g_statusPollQueue;
extern Queue g_configPollQueue;

/**
 * Global data
 */
double g_dAvgPollerQueueSize = 0;
double g_dAvgDBWriterQueueSize = 0;
double g_dAvgIDataWriterQueueSize = 0;
double g_dAvgDBAndIDataWriterQueueSize = 0;
double g_dAvgStatusPollerQueueSize = 0;
double g_dAvgConfigPollerQueueSize = 0;
UINT32 g_dwAvgDCIQueuingTime = 0;
Queue *g_pItemQueue = NULL;

/**
 * Collect data for DCI
 */
static void *GetItemData(DataCollectionTarget *dcTarget, DCItem *pItem, TCHAR *pBuffer, UINT32 *error)
{
   if (dcTarget->Type() == OBJECT_CLUSTER)
   {
      if (pItem->isAggregateOnCluster())
      {
         *error = ((Cluster *)dcTarget)->collectAggregatedData(pItem, pBuffer);
      }
      else
      {
         *error = DCE_NOT_SUPPORTED;
      }
   }
   else
   {
      switch(pItem->getDataSource())
      {
         case DS_INTERNAL:    // Server internal parameters (like status)
            *error = dcTarget->getInternalItem(pItem->getName(), MAX_LINE_SIZE, pBuffer);
            break;
         case DS_SNMP_AGENT:
			   if (dcTarget->Type() == OBJECT_NODE)
				   *error = ((Node *)dcTarget)->getItemFromSNMP(pItem->getSnmpPort(), pItem->getName(), MAX_LINE_SIZE, 
					   pBuffer, pItem->isInterpretSnmpRawValue() ? (int)pItem->getSnmpRawValueType() : SNMP_RAWTYPE_NONE);
			   else
				   *error = DCE_NOT_SUPPORTED;
            break;
         case DS_CHECKPOINT_AGENT:
			   if (dcTarget->Type() == OBJECT_NODE)
	            *error = ((Node *)dcTarget)->getItemFromCheckPointSNMP(pItem->getName(), MAX_LINE_SIZE, pBuffer);
			   else
				   *error = DCE_NOT_SUPPORTED;
            break;
         case DS_NATIVE_AGENT:
			   if (dcTarget->Type() == OBJECT_NODE)
	            *error = ((Node *)dcTarget)->getItemFromAgent(pItem->getName(), MAX_LINE_SIZE, pBuffer);
			   else
				   *error = DCE_NOT_SUPPORTED;
            break;
         case DS_WINPERF:
			   if (dcTarget->Type() == OBJECT_NODE)
			   {
				   TCHAR name[MAX_PARAM_NAME];
				   _sntprintf(name, MAX_PARAM_NAME, _T("PDH.CounterValue(\"%s\",%d)"), pItem->getName(), pItem->getSampleCount());
	            *error = ((Node *)dcTarget)->getItemFromAgent(name, MAX_LINE_SIZE, pBuffer);
			   }
			   else
			   {
				   *error = DCE_NOT_SUPPORTED;
			   }
            break;
         case DS_SMCLP:
            if (dcTarget->Type() == OBJECT_NODE)
            {
	            *error = ((Node *)dcTarget)->getItemFromSMCLP(pItem->getName(), MAX_LINE_SIZE, pBuffer);
            }
            else
            {
               *error = DCE_NOT_SUPPORTED;
            }
            break;
		   default:
			   *error = DCE_NOT_SUPPORTED;
			   break;
      }
   }
	return pBuffer;
}

/**
 * Collect data for table
 */
static void *GetTableData(DataCollectionTarget *dcTarget, DCTable *table, UINT32 *error)
{
	Table *result = NULL;
   if (dcTarget->Type() == OBJECT_CLUSTER)
   {
      if (table->isAggregateOnCluster())
      {
         *error = ((Cluster *)dcTarget)->collectAggregatedData(table, &result);
      }
      else
      {
         *error = DCE_NOT_SUPPORTED;
      }
   }
   else
   {
      switch(table->getDataSource())
      {
		   case DS_NATIVE_AGENT:
			   if (dcTarget->Type() == OBJECT_NODE)
            {
				   *error = ((Node *)dcTarget)->getTableFromAgent(table->getName(), &result);
               if ((*error == DCE_SUCCESS) && (result != NULL))
                  table->updateResultColumns(result);
            }
			   else
            {
				   *error = DCE_NOT_SUPPORTED;
            }
			   break;
         case DS_SNMP_AGENT:
			   if (dcTarget->Type() == OBJECT_NODE)
            {
               *error = ((Node *)dcTarget)->getTableFromSNMP(table->getSnmpPort(), table->getName(), table->getColumns(), &result);
               if ((*error == DCE_SUCCESS) && (result != NULL))
                  table->updateResultColumns(result);
            }
			   else
            {
				   *error = DCE_NOT_SUPPORTED;
            }
            break;
		   default:
			   *error = DCE_NOT_SUPPORTED;
			   break;
	   }
   }
	return result;
}

/**
 * Data collector
 */
static THREAD_RESULT THREAD_CALL DataCollector(void *pArg)
{
   UINT32 dwError;

   TCHAR *pBuffer = (TCHAR *)malloc(MAX_LINE_SIZE * sizeof(TCHAR));
   while(!IsShutdownInProgress())
   {
      DCObject *pItem = (DCObject *)g_pItemQueue->GetOrBlock();
		DataCollectionTarget *target = (DataCollectionTarget *)pItem->getTarget();

		if (pItem->isScheduledForDeletion())
		{
	      DbgPrintf(7, _T("DataCollector(): about to destroy DC object %d \"%s\" owner=%d"),
			          pItem->getId(), pItem->getName(), (target != NULL) ? (int)target->Id() : -1);
			pItem->deleteFromDB();
			delete pItem;
			continue;
		}

      DbgPrintf(8, _T("DataCollector(): processing DC object %d \"%s\" owner=%d proxy=%d"),
		          pItem->getId(), pItem->getName(), (target != NULL) ? (int)target->Id() : -1, pItem->getProxyNode());
		if (pItem->getProxyNode() != 0)
		{
			NetObj *object = FindObjectById(pItem->getProxyNode(), OBJECT_NODE);
			if (object != NULL)
			{
				if (object->isTrustedNode((target != NULL) ? target->Id() : 0))
				{
					target = (Node *)object;
					target->incRefCount();
				}
				else
				{
               // Change item's status to _T("not supported")
               pItem->setStatus(ITEM_STATUS_NOT_SUPPORTED, true);

					if (target != NULL)
					{
						target->decRefCount();
						target = NULL;
					}
				}
			}
			else
			{
				if (target != NULL)
				{
					target->decRefCount();
					target = NULL;
				}
			}
		}

      time_t currTime = time(NULL);
      if (target != NULL)
      {
			void *data;

			switch(pItem->getType())
			{
				case DCO_TYPE_ITEM:
					data = GetItemData(target, (DCItem *)pItem, pBuffer, &dwError);
					break;
				case DCO_TYPE_TABLE:
					data = GetTableData(target, (DCTable *)pItem, &dwError);
					break;
				default:
					data = NULL;
					dwError = DCE_NOT_SUPPORTED;
					break;
			}

         // Transform and store received value into database or handle error
         switch(dwError)
         {
            case DCE_SUCCESS:
					if (pItem->getStatus() == ITEM_STATUS_NOT_SUPPORTED)
	               pItem->setStatus(ITEM_STATUS_ACTIVE, true);
					((DataCollectionTarget *)pItem->getTarget())->processNewDCValue(pItem, currTime, data);
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
         target->decRefCount();
			if ((pItem->getProxyNode() != 0) && (pItem->getTarget() != NULL))
			{
				pItem->getTarget()->decRefCount();
			}
      }
      else     /* target == NULL */
      {
			Template *n = pItem->getTarget();
         DbgPrintf(3, _T("*** DataCollector: Attempt to collect information for non-existing node (DCI=%d \"%s\" target=%d proxy=%d)"),
			          pItem->getId(), pItem->getName(), (n != NULL) ? (int)n->Id() : -1, pItem->getProxyNode());
      }

		// Update item's last poll time and clear busy flag so item can be polled again
      pItem->setLastPollTime(currTime);
      pItem->setBusyFlag(FALSE);
   }

   free(pBuffer);
   DbgPrintf(1, _T("Data collector thread terminated"));
   return THREAD_OK;
}

/**
 * Callback for queueing DCIs
 */
static void QueueItems(NetObj *object, void *data)
{
	DbgPrintf(8, _T("ItemPoller: calling DataCollectionTarget::queueItemsForPolling for object %s [%d]"),
				 object->Name(), object->Id());
	((DataCollectionTarget *)object)->queueItemsForPolling(g_pItemQueue);
}

/**
 * Item poller thread: check nodes' items and put into the 
 * data collector queue when data polling required
 */
static THREAD_RESULT THREAD_CALL ItemPoller(void *pArg)
{
   UINT32 dwSum, dwWatchdogId, dwCurrPos = 0;
   UINT32 dwTimingHistory[60 / ITEM_POLLING_INTERVAL];
   INT64 qwStart;

   dwWatchdogId = WatchdogAddThread(_T("Item Poller"), 20);
   memset(dwTimingHistory, 0, sizeof(UINT32) * (60 / ITEM_POLLING_INTERVAL));

   while(!IsShutdownInProgress())
   {
      if (SleepAndCheckForShutdown(ITEM_POLLING_INTERVAL))
         break;      // Shutdown has arrived
      WatchdogNotify(dwWatchdogId);
		DbgPrintf(8, _T("ItemPoller: wakeup"));

      qwStart = GetCurrentTimeMs();
		g_idxNodeById.forEach(QueueItems, NULL);
		g_idxClusterById.forEach(QueueItems, NULL);
		g_idxMobileDeviceById.forEach(QueueItems, NULL);

      // Save last poll time
      dwTimingHistory[dwCurrPos] = (UINT32)(GetCurrentTimeMs() - qwStart);
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

/**
 * Statistics collection thread
 */
static THREAD_RESULT THREAD_CALL StatCollector(void *pArg)
{
   UINT32 i, dwCurrPos = 0;
   UINT32 dwPollerQS[12], dwDBWriterQS[12];
   UINT32 dwIDataWriterQS[12], dwDBAndIDataWriterQS[12];
   UINT32 dwStatusPollerQS[12], dwConfigPollerQS[12];
   double dSum1, dSum2, dSum3, dSum4, dSum5, dSum6;

   memset(dwPollerQS, 0, sizeof(UINT32) * 12);
   memset(dwDBWriterQS, 0, sizeof(UINT32) * 12);
   memset(dwIDataWriterQS, 0, sizeof(UINT32) * 12);
   memset(dwDBAndIDataWriterQS, 0, sizeof(UINT32) * 12);
   memset(dwStatusPollerQS, 0, sizeof(UINT32) * 12);
   memset(dwConfigPollerQS, 0, sizeof(UINT32) * 12);
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

/**
 * Initialize data collection subsystem
 */
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

static void UpdateParamList(NetObj *object, void *data)
{
	ObjectArray<AgentParameterDefinition> *fullList = (ObjectArray<AgentParameterDefinition> *)data;

	ObjectArray<AgentParameterDefinition> *paramList;
	((Node *)object)->openParamList(&paramList);
	if ((paramList != NULL) && (paramList->size() > 0))
	{
		for(int i = 0; i < paramList->size(); i++)
		{
			int j;
			for(j = 0; j < fullList->size(); j++)
			{
				if (!_tcsicmp(paramList->get(i)->getName(), fullList->get(j)->getName()))
					break;
			}

			if (j == fullList->size())
			{
            fullList->add(new AgentParameterDefinition(paramList->get(i)));
			}
		}
	}
	((Node *)object)->closeParamList();
}

static void UpdateTableList(NetObj *object, void *data)
{
	ObjectArray<AgentTableDefinition> *fullList = (ObjectArray<AgentTableDefinition> *)data;

   ObjectArray<AgentTableDefinition> *tableList;
	((Node *)object)->openTableList(&tableList);
	if ((tableList != NULL) && (tableList->size() > 0))
	{
		for(int i = 0; i < tableList->size(); i++)
		{
			int j;
			for(j = 0; j < fullList->size(); j++)
			{
				if (!_tcsicmp(tableList->get(i)->getName(), fullList->get(j)->getName()))
					break;
			}

			if (j == fullList->size())
			{
            fullList->add(new AgentTableDefinition(tableList->get(i)));
			}
		}
	}
	((Node *)object)->closeTableList();
}

void WriteFullParamListToMessage(CSCPMessage *pMsg, WORD flags)
{
   // Gather full parameter list
	if (flags & 0x01)
	{
		ObjectArray<AgentParameterDefinition> fullList(64, 64, true);
		g_idxNodeById.forEach(UpdateParamList, &fullList);

		// Put list into the message
		pMsg->SetVariable(VID_NUM_PARAMETERS, (UINT32)fullList.size());
      UINT32 varId = VID_PARAM_LIST_BASE;
		for(int i = 0; i < fullList.size(); i++)
		{
         varId += fullList.get(i)->fillMessage(pMsg, varId);
		}
	}

   // Gather full table list
	if (flags & 0x02)
	{
		ObjectArray<AgentTableDefinition> fullList(64, 64, true);
		g_idxNodeById.forEach(UpdateTableList, &fullList);

		// Put list into the message
		pMsg->SetVariable(VID_NUM_TABLES, (UINT32)fullList.size());
      UINT32 varId = VID_TABLE_LIST_BASE;
		for(int i = 0; i < fullList.size(); i++)
		{
         varId += fullList.get(i)->fillMessage(pMsg, varId);
		}
	}
}
