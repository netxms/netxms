/*
** NetXMS - Network Management System
** Copyright (C) 2003-2021 Victor Kirhenshtein
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
#include <nxcore_websvc.h>
#include <gauge_helpers.h>

/**
 * Interval between DCI polling
 */
#define ITEM_POLLING_INTERVAL             1

/**
 * Thread pool for data collectors
 */
ThreadPool *g_dataCollectorThreadPool = nullptr;

/**
 * DCI cache loader queue
 */
SharedObjectQueue<DCObjectInfo> g_dciCacheLoaderQueue;

/**
 * Average time to queue DCI
 */
uint32_t g_averageDCIQueuingTime = 0;

/**
 * Collect data for DCI
 */
static void *GetItemData(DataCollectionTarget *dcTarget, DCItem *pItem, TCHAR *pBuffer, UINT32 *error)
{
   if (dcTarget->getObjectClass() == OBJECT_CLUSTER)
   {
      if (pItem->isAggregateOnCluster())
      {
         *error = static_cast<Cluster*>(dcTarget)->collectAggregatedData(pItem, pBuffer);
      }
      else
      {
         *error = DCE_IGNORE;
      }
   }
   else
   {
      switch(pItem->getDataSource())
      {
         case DS_INTERNAL:    // Server internal parameters (like status)
            *error = dcTarget->getInternalMetric(pItem->getName(), pBuffer, MAX_LINE_SIZE);
            break;
         case DS_SNMP_AGENT:
			   if (dcTarget->getObjectClass() == OBJECT_NODE)
			   {
				   *error = static_cast<Node*>(dcTarget)->getMetricFromSNMP(pItem->getSnmpPort(), pItem->getSnmpVersion(), pItem->getName(), 
                     pBuffer, MAX_LINE_SIZE, pItem->isInterpretSnmpRawValue() ? (int)pItem->getSnmpRawValueType() : SNMP_RAWTYPE_NONE);
			   }
			   else
			   {
				   *error = DCE_NOT_SUPPORTED;
			   }
            break;
         case DS_DEVICE_DRIVER:
            if (dcTarget->getObjectClass() == OBJECT_NODE)
               *error = ((Node *)dcTarget)->getMetricFromDeviceDriver(pItem->getName(), pBuffer, MAX_LINE_SIZE);
            else
               *error = DCE_NOT_SUPPORTED;
            break;
         case DS_NATIVE_AGENT:
			   if (dcTarget->getObjectClass() == OBJECT_NODE)
	            *error = ((Node *)dcTarget)->getMetricFromAgent(pItem->getName(), pBuffer, MAX_LINE_SIZE);
			   else if (dcTarget->getObjectClass() == OBJECT_SENSOR)
               *error = ((Sensor *)dcTarget)->getMetricFromAgent(pItem->getName(), pBuffer, MAX_LINE_SIZE);
			   else
				   *error = DCE_NOT_SUPPORTED;
            break;
         case DS_WINPERF:
			   if (dcTarget->getObjectClass() == OBJECT_NODE)
			   {
				   TCHAR name[MAX_PARAM_NAME];
				   _sntprintf(name, MAX_PARAM_NAME, _T("PDH.CounterValue(\"%s\",%d)"), (const TCHAR *)EscapeStringForAgent(pItem->getName()), pItem->getSampleCount());
	            *error = ((Node *)dcTarget)->getMetricFromAgent(name, pBuffer, MAX_LINE_SIZE);
			   }
			   else
			   {
				   *error = DCE_NOT_SUPPORTED;
			   }
            break;
         case DS_SSH:
            if (dcTarget->getObjectClass() == OBJECT_NODE)
            {
               shared_ptr<Node> proxy = static_pointer_cast<Node>(FindObjectById(static_cast<Node*>(dcTarget)->getEffectiveSshProxy(), OBJECT_NODE));
               if (proxy != nullptr)
               {
                  TCHAR name[MAX_PARAM_NAME], ipAddr[64];
                  _sntprintf(name, MAX_PARAM_NAME, _T("SSH.Command(%s:%u,\"%s\",\"%s\",\"%s\",\"\",%d)"),
                             static_cast<Node*>(dcTarget)->getIpAddress().toString(ipAddr),
                             static_cast<uint32_t>(static_cast<Node*>(dcTarget)->getSshPort()),
                             EscapeStringForAgent(static_cast<Node*>(dcTarget)->getSshLogin()).cstr(),
                             EscapeStringForAgent(static_cast<Node*>(dcTarget)->getSshPassword()).cstr(),
                             EscapeStringForAgent(pItem->getName()).cstr(),
                             static_cast<Node*>(dcTarget)->getSshKeyId());
                  *error = proxy->getMetricFromAgent(name, pBuffer, MAX_LINE_SIZE);
               }
               else
               {
                  *error = DCE_COMM_ERROR;
               }
            }
            else
            {
               *error = DCE_NOT_SUPPORTED;
            }
            break;
         case DS_SMCLP:
            if (dcTarget->getObjectClass() == OBJECT_NODE)
            {
	            *error = static_cast<Node*>(dcTarget)->getMetricFromSMCLP(pItem->getName(), pBuffer, MAX_LINE_SIZE);
            }
            else
            {
               *error = DCE_NOT_SUPPORTED;
            }
            break;
         case DS_SCRIPT:
            *error = dcTarget->getMetricFromScript(pItem->getName(), pBuffer, MAX_LINE_SIZE, static_cast<DataCollectionTarget*>(pItem->getOwner().get()));
            break;
         case DS_WEB_SERVICE:
            *error = dcTarget->getMetricFromWebService(pItem->getName(), pBuffer, MAX_LINE_SIZE);
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
	Table *result = nullptr;
   if (dcTarget->getObjectClass() == OBJECT_CLUSTER)
   {
      if (table->isAggregateOnCluster())
      {
         *error = ((Cluster *)dcTarget)->collectAggregatedData(table, &result);
      }
      else
      {
         *error = DCE_IGNORE;
      }
   }
   else
   {
      switch(table->getDataSource())
      {
         case DS_INTERNAL:    // Server internal parameter tables
            *error = dcTarget->getInternalTable(table->getName(), &result);
            break;
		   case DS_NATIVE_AGENT:
			   if (dcTarget->getObjectClass() == OBJECT_NODE)
            {
				   *error = static_cast<Node*>(dcTarget)->getTableFromAgent(table->getName(), &result);
            }
			   else
            {
				   *error = DCE_NOT_SUPPORTED;
            }
			   break;
         case DS_SNMP_AGENT:
			   if (dcTarget->getObjectClass() == OBJECT_NODE)
            {
               *error = static_cast<Node*>(dcTarget)->getTableFromSNMP(table->getSnmpPort(), table->getSnmpVersion(), table->getName(), table->getColumns(), &result);
            }
			   else
            {
				   *error = DCE_NOT_SUPPORTED;
            }
            break;
         case DS_SCRIPT:
            *error = dcTarget->getTableFromScript(table->getName(), &result, static_cast<DataCollectionTarget*>(table->getOwner().get()));
            break;
		   default:
			   *error = DCE_NOT_SUPPORTED;
			   break;
	   }
      if ((*error == DCE_SUCCESS) && (result != nullptr))
         table->updateResultColumns(result);
   }
	return result;
}

/**
 * Data collector
 */
void DataCollector(const shared_ptr<DCObject>& dcObject)
{
   shared_ptr<DataCollectionTarget> target = static_pointer_cast<DataCollectionTarget>(dcObject->getOwner());

   SharedString dcObjectName = dcObject->getName();
   if (dcObject->isScheduledForDeletion())
   {
      nxlog_debug(7, _T("DataCollector(): about to destroy DC object [%u] \"%s\" owner=[%u]"),
                  dcObject->getId(), dcObjectName.cstr(), (target != nullptr) ? target->getId() : 0);
      dcObject->deleteFromDatabase();
      return;
   }

   if (target == nullptr)
   {
      nxlog_debug(3, _T("DataCollector: attempt to collect information for non-existing node (DCI=[%u] \"%s\")"),
                  dcObject->getId(), dcObjectName.cstr());

      // Update item's last poll time and clear busy flag so item can be polled again
      dcObject->setLastPollTime(time(nullptr));
      dcObject->clearBusyFlag();
      return;
   }

   if (IsShutdownInProgress())
   {
      dcObject->clearBusyFlag();
      return;
   }

   DbgPrintf(8, _T("DataCollector(): processing DC object %d \"%s\" owner=%d sourceNode=%d"),
             dcObject->getId(), dcObjectName.cstr(), (target != nullptr) ? (int)target->getId() : -1, dcObject->getSourceNode());
   uint32_t sourceNodeId = target->getEffectiveSourceNode(dcObject.get());
   if (sourceNodeId != 0)
   {
      shared_ptr<Node> sourceNode = static_pointer_cast<Node>(FindObjectById(sourceNodeId, OBJECT_NODE));
      if (sourceNode != nullptr)
      {
         if (((target->getObjectClass() == OBJECT_CHASSIS) && (static_cast<Chassis*>(target.get())->getControllerId() == sourceNodeId)) ||
             sourceNode->isTrustedNode(target->getId()))
         {
            target = sourceNode;
         }
         else
         {
            // Change item's status to "not supported"
            dcObject->setStatus(ITEM_STATUS_NOT_SUPPORTED, true);
            target.reset();
         }
      }
      else
      {
         target.reset();
      }
   }

   time_t currTime = time(nullptr);
   if (target != nullptr)
   {
      if (!IsShutdownInProgress())
      {
         void *data;
         TCHAR buffer[MAX_LINE_SIZE];
         UINT32 error;
         switch(dcObject->getType())
         {
            case DCO_TYPE_ITEM:
               data = GetItemData(target.get(), static_cast<DCItem*>(dcObject.get()), buffer, &error);
               break;
            case DCO_TYPE_TABLE:
               data = GetTableData(target.get(), static_cast<DCTable*>(dcObject.get()), &error);
               break;
            default:
               data = nullptr;
               error = DCE_NOT_SUPPORTED;
               break;
         }

         // Transform and store received value into database or handle error
         switch(error)
         {
            case DCE_SUCCESS:
               if (dcObject->getStatus() == ITEM_STATUS_NOT_SUPPORTED)
                  dcObject->setStatus(ITEM_STATUS_ACTIVE, true);
               if (!static_cast<DataCollectionTarget*>(dcObject->getOwner().get())->processNewDCValue(dcObject, currTime, data))
               {
                  // value processing failed, convert to data collection error
                  dcObject->processNewError(false);
               }
               break;
            case DCE_COLLECTION_ERROR:
               if (dcObject->getStatus() == ITEM_STATUS_NOT_SUPPORTED)
                  dcObject->setStatus(ITEM_STATUS_ACTIVE, true);
               dcObject->processNewError(false);
               break;
            case DCE_NO_SUCH_INSTANCE:
               if (dcObject->getStatus() == ITEM_STATUS_NOT_SUPPORTED)
                  dcObject->setStatus(ITEM_STATUS_ACTIVE, true);
               dcObject->processNewError(true);
               break;
            case DCE_COMM_ERROR:
               dcObject->processNewError(false);
               break;
            case DCE_NOT_SUPPORTED:
               // Change item's status
               dcObject->setStatus(ITEM_STATUS_NOT_SUPPORTED, true);
               break;
         }

         // Send session notification when force poll is performed
         if (dcObject->isForcePollRequested())
         {
            ClientSession *session = dcObject->processForcePoll();
            if (session != nullptr)
            {
               session->notify(NX_NOTIFY_FORCE_DCI_POLL, dcObject->getOwnerId());
               session->decRefCount();
            }
         }
      }
   }
   else     /* target == nullptr */
   {
      shared_ptr<DataCollectionOwner> n = dcObject->getOwner();
      nxlog_debug(5, _T("DataCollector: attempt to collect information for non-existing or inaccessible node (DCI=[%u] \"%s\" target=[%u] sourceNode=[%u])"),
                  dcObject->getId(), dcObjectName.cstr(), (n != nullptr) ? n->getId() : 0, sourceNodeId);
   }

   // Update item's last poll time and clear busy flag so item can be polled again
   dcObject->setLastPollTime(currTime);
   dcObject->clearBusyFlag();
}

/**
 * Callback for queueing DCIs
 */
static void QueueItems(NetObj *object, uint32_t *watchdogId)
{
   if (IsShutdownInProgress())
      return;

   WatchdogNotify(*watchdogId);
	nxlog_debug(8, _T("ItemPoller: calling DataCollectionTarget::queueItemsForPolling for object %s [%d]"),
				   object->getName(), object->getId());
	static_cast<DataCollectionTarget*>(object)->queueItemsForPolling();
}

/**
 * Item poller thread: check nodes' items and put into the
 * data collector queue when data polling required
 */
static void ItemPoller()
{
   ThreadSetName("ItemPoller");

   uint32_t watchdogId = WatchdogAddThread(_T("Item Poller"), 10);
   GaugeData<uint32_t> queuingTime(ITEM_POLLING_INTERVAL, 300);

   while(!IsShutdownInProgress())
   {
      if (SleepAndCheckForShutdown(ITEM_POLLING_INTERVAL))
         break;      // Shutdown has arrived
      WatchdogNotify(watchdogId);
      nxlog_debug_tag(_T("obj.dc.poller"), 8, _T("ItemPoller: wakeup"));

      INT64 startTime = GetCurrentTimeMs();
		g_idxNodeById.forEach(QueueItems, &watchdogId);
		g_idxClusterById.forEach(QueueItems, &watchdogId);
		g_idxMobileDeviceById.forEach(QueueItems, &watchdogId);
      g_idxChassisById.forEach(QueueItems, &watchdogId);
		g_idxSensorById.forEach(QueueItems, &watchdogId);

		queuingTime.update(static_cast<uint32_t>(GetCurrentTimeMs() - startTime));
		g_averageDCIQueuingTime = static_cast<uint32_t>(queuingTime.getAverage());
   }
   nxlog_debug_tag(_T("obj.dc.poller"), 1, _T("Item poller thread terminated"));
}

/**
 * DCI cache loader
 */
static void CacheLoader()
{
   ThreadSetName("CacheLoader");
   nxlog_debug_tag(_T("obj.dc.cache"), 2, _T("DCI cache loader thread started"));
   while(!IsShutdownInProgress())
   {
      shared_ptr<DCObjectInfo> ref = g_dciCacheLoaderQueue.getOrBlock();
      if (ref == nullptr)
         break;

      shared_ptr<NetObj> object = FindObjectById(ref->getOwnerId());
      if ((object != nullptr) && object->isDataCollectionTarget())
      {
         shared_ptr<DCObject> dci = static_cast<DataCollectionTarget*>(object.get())->getDCObjectById(ref->getId(), 0, true);
         if ((dci != nullptr) && (dci->getType() == DCO_TYPE_ITEM))
         {
            nxlog_debug_tag(_T("obj.dc.cache"), 6, _T("Loading cache for DCI %s [%d] on %s [%d]"),
                     ref->getName(), ref->getId(), object->getName(), object->getId());
            static_cast<DCItem*>(dci.get())->reloadCache(false);
         }
      }
   }
   nxlog_debug_tag(_T("obj.dc.cache"), 2, _T("DCI cache loader thread stopped"));
}

/**
 * Threads
 */
static THREAD s_itemPollerThread = INVALID_THREAD_HANDLE;
static THREAD s_cacheLoaderThread = INVALID_THREAD_HANDLE;

/**
 * Initialize data collection subsystem
 */
void InitDataCollector()
{
   g_dataCollectorThreadPool = ThreadPoolCreate(_T("DATACOLL"),
            ConfigReadInt(_T("ThreadPool.DataCollector.BaseSize"), 10),
            ConfigReadInt(_T("ThreadPool.DataCollector.MaxSize"), 250),
            256 * 1024);

   s_itemPollerThread = ThreadCreateEx(ItemPoller);
   s_cacheLoaderThread = ThreadCreateEx(CacheLoader);
}

/**
 * Stop data collection
 */
void StopDataCollection()
{
   ThreadJoin(s_itemPollerThread);
   ThreadJoin(s_cacheLoaderThread);
   ThreadPoolDestroy(g_dataCollectorThreadPool);
}

/**
 * Callback data for WriteFullParamListToMessage
 */
struct WriteFullParamListToMessage_CallbackData
{
   int origin;
   ObjectArray<AgentParameterDefinition> *parameters;
};

/**
 * Update parameter list from node
 */
static void UpdateParamList(NetObj *object, void *data)
{
   WriteFullParamListToMessage_CallbackData *cd = static_cast<WriteFullParamListToMessage_CallbackData*>(data);

	ObjectArray<AgentParameterDefinition> *paramList = static_cast<Node*>(object)->openParamList(cd->origin);
	if ((paramList != nullptr) && (paramList->size() > 0))
	{
		for(int i = 0; i < paramList->size(); i++)
		{
			int j;
			for(j = 0; j < cd->parameters->size(); j++)
			{
				if (!_tcsicmp(paramList->get(i)->getName(), cd->parameters->get(j)->getName()))
					break;
			}

			if (j == cd->parameters->size())
			{
			   cd->parameters->add(new AgentParameterDefinition(paramList->get(i)));
			}
		}
	}
	static_cast<Node*>(object)->closeParamList();
}

/**
 * Update table list from node
 */
static void UpdateTableList(NetObj *object, void *data)
{
	ObjectArray<AgentTableDefinition> *fullList = (ObjectArray<AgentTableDefinition> *)data;

   ObjectArray<AgentTableDefinition> *tableList = static_cast<Node*>(object)->openTableList();
	if ((tableList != nullptr) && (tableList->size() > 0))
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
	static_cast<Node*>(object)->closeTableList();
}

/**
 * Write full (from all nodes) agent parameters list to NXCP message
 */
void WriteFullParamListToMessage(NXCPMessage *pMsg, int origin, WORD flags)
{
   // Gather full parameter list
	if (flags & 0x01)
	{
		ObjectArray<AgentParameterDefinition> fullList(64, 64, Ownership::True);
		WriteFullParamListToMessage_CallbackData data;
		data.origin = origin;
		data.parameters = &fullList;
		g_idxNodeById.forEach(UpdateParamList, &data);

		// Put list into the message
		pMsg->setField(VID_NUM_PARAMETERS, (UINT32)fullList.size());
      UINT32 varId = VID_PARAM_LIST_BASE;
		for(int i = 0; i < fullList.size(); i++)
		{
         varId += fullList.get(i)->fillMessage(pMsg, varId);
		}
	}

   // Gather full table list
	if (flags & 0x02)
	{
		ObjectArray<AgentTableDefinition> fullList(64, 64, Ownership::True);
		g_idxNodeById.forEach(UpdateTableList, &fullList);

		// Put list into the message
		pMsg->setField(VID_NUM_TABLES, (UINT32)fullList.size());
      UINT32 varId = VID_TABLE_LIST_BASE;
		for(int i = 0; i < fullList.size(); i++)
		{
         varId += fullList.get(i)->fillMessage(pMsg, varId);
		}
	}
}

/**
 * Get type of data collection object
 */
int GetDCObjectType(UINT32 nodeId, UINT32 dciId)
{
   shared_ptr<Node> node = static_pointer_cast<Node>(FindObjectById(nodeId, OBJECT_NODE));
   if (node != nullptr)
   {
      shared_ptr<DCObject> dcObject = node->getDCObjectById(dciId, 0);
      if (dcObject != nullptr)
      {
         return dcObject->getType();
      }
   }
   return DCO_TYPE_ITEM;   // default
}
