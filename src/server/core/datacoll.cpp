/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Victor Kirhenshtein
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

void ScheduleThresholdRepeatEventsOnStartup();

/**
 * Interval between DCI polling
 */
#define ITEM_POLLING_INTERVAL             1

/**
 * Thread pool for data collectors
 */
ThreadPool *g_dataCollectorThreadPool = nullptr;

/**
 * Thread pool for threshold repeat events
 */
ThreadPool *g_thresholdRepeatPool = nullptr;

/**
 * DCI cache loader queue
 */
SharedObjectQueue<DCObjectInfo> g_dciCacheLoaderQueue;

/**
 * Average time to queue DCI
 */
uint32_t g_averageDCIQueuingTime = 0;

/**
 * GUIDs for NXSL script exit codes
 */
uuid g_nxslExitDCError = uuid::parseA("8c640b20-ee7d-4a5b-9e34-eebbf868302a");
uuid g_nxslExitDCNotSupported = uuid::parseA("6e3282cc-84c5-4f5b-b354-b08691d8404f");
uuid g_nxslExitDCNoSuchInstance = uuid::parseA("0043db2a-17f0-464c-83ee-bf0ac6e8cfb4");

/**
 * Collect data for DCI
 */
static void GetItemData(DataCollectionTarget *dcTarget, const DCItem& dci, wchar_t *buffer, uint32_t *error)
{
   if (dcTarget->getObjectClass() == OBJECT_CLUSTER)
   {
      if (dci.isAggregateOnCluster())
      {
         *error = static_cast<Cluster*>(dcTarget)->collectAggregatedData(dci, buffer);
      }
      else
      {
         *error = DCE_IGNORE;
      }
   }
   else
   {
      switch(dci.getDataSource())
      {
         case DS_INTERNAL:    // Server internal parameters (like status)
            *error = dcTarget->getInternalMetric(dci.getName(), buffer, MAX_RESULT_LENGTH);
            break;
         case DS_SNMP_AGENT:
			   if (dcTarget->getObjectClass() == OBJECT_NODE)
			   {
				   *error = static_cast<Node*>(dcTarget)->getMetricFromSNMP(dci.getSnmpPort(), dci.getSnmpVersion(), dci.getName(),
                     buffer, MAX_RESULT_LENGTH, dci.isInterpretSnmpRawValue() ? (int)dci.getSnmpRawValueType() : SNMP_RAWTYPE_NONE, dci.getSnmpContext());
			   }
			   else
			   {
				   *error = DCE_NOT_SUPPORTED;
			   }
            break;
         case DS_DEVICE_DRIVER:
            if (dcTarget->getObjectClass() == OBJECT_NODE)
               *error = static_cast<Node*>(dcTarget)->getMetricFromDeviceDriver(dci.getName(), buffer, MAX_RESULT_LENGTH);
            else
               *error = DCE_NOT_SUPPORTED;
            break;
         case DS_NATIVE_AGENT:
			   if (dcTarget->getObjectClass() == OBJECT_NODE)
	            *error = static_cast<Node*>(dcTarget)->getMetricFromAgent(dci.getName(), buffer, MAX_RESULT_LENGTH);
			   else
				   *error = DCE_NOT_SUPPORTED;
            break;
         case DS_WINPERF:
			   if (dcTarget->getObjectClass() == OBJECT_NODE)
			   {
				   wchar_t name[MAX_PARAM_NAME];
				   _sntprintf(name, MAX_PARAM_NAME, L"PDH.CounterValue(\"%s\",%d)", EscapeStringForAgent(dci.getName()).cstr(), dci.getSampleCount());
	            *error = static_cast<Node*>(dcTarget)->getMetricFromAgent(name, buffer, MAX_RESULT_LENGTH);
			   }
			   else
			   {
				   *error = DCE_NOT_SUPPORTED;
			   }
            break;
         case DS_SSH:
            if (dcTarget->getObjectClass() == OBJECT_NODE)
            {
               Node *node = static_cast<Node*>(dcTarget);
               if (node->isPortBlocked(node->getSshPort(), true))
               {
                  nxlog_debug_tag(DEBUG_TAG_DC_SSH, 5, _T("DataCollector(%s [%u]): SSH port %u blocked by port stop list"), node->getName(), node->getId(), node->getSshPort());
                  *error = DCE_COMM_ERROR;
                  break;
               }
               shared_ptr<Node> proxy = static_pointer_cast<Node>(FindObjectById(node->getEffectiveSshProxy(), OBJECT_NODE));
               if (proxy != nullptr)
               {
                  wchar_t name[MAX_PARAM_NAME], ipAddr[64];
                  nx_swprintf(name, MAX_PARAM_NAME, _T("SSH.Command(%s:%u,\"%s\",\"%s\",\"%s\",\"\",%d)"),
                             node->getIpAddress().toString(ipAddr),
                             static_cast<uint32_t>(node->getSshPort()),
                             EscapeStringForAgent(node->getSshLogin()).cstr(),
                             EscapeStringForAgent(node->getSshPassword()).cstr(),
                             EscapeStringForAgent(dci.getName()).cstr(),
                             node->getSshKeyId());
                  *error = proxy->getMetricFromAgent(name, buffer, MAX_RESULT_LENGTH);
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
	            *error = static_cast<Node*>(dcTarget)->getMetricFromSmclp(dci.getName(), buffer, MAX_RESULT_LENGTH);
            }
            else
            {
               *error = DCE_NOT_SUPPORTED;
            }
            break;
         case DS_SCRIPT:
            *error = dcTarget->getMetricFromScript(dci.getName(), buffer, MAX_RESULT_LENGTH, static_cast<DataCollectionTarget*>(dci.getOwner().get()), dci.createDescriptor());
            break;
         case DS_WEB_SERVICE:
            *error = dcTarget->getMetricFromWebService(dci.getName(), buffer, MAX_RESULT_LENGTH);
            break;
         case DS_MODBUS:
            if (dcTarget->getObjectClass() == OBJECT_NODE)
            {
               *error = static_cast<Node*>(dcTarget)->getMetricFromModbus(dci.getName(), buffer, MAX_RESULT_LENGTH);
            }
            else if (dcTarget->getObjectClass() == OBJECT_SENSOR)
            {
               *error = static_cast<Sensor*>(dcTarget)->getMetricFromModbus(dci.getName(), buffer, MAX_RESULT_LENGTH);
            }
            else
            {
               *error = DCE_NOT_SUPPORTED;
            }
            break;
         case DS_ETHERNET_IP:
            if (dcTarget->getObjectClass() == OBJECT_NODE)
            {
               *error = static_cast<Node*>(dcTarget)->getMetricFromEtherNetIP(dci.getName(), buffer, MAX_RESULT_LENGTH);
            }
            else
            {
               *error = DCE_NOT_SUPPORTED;
            }
            break;
         case DS_MQTT:
            if (dcTarget->getObjectClass() == OBJECT_NODE)
            {
               shared_ptr<Node> proxy = static_pointer_cast<Node>(FindObjectById(static_cast<Node*>(dcTarget)->getEffectiveMqttProxy(), OBJECT_NODE));
               if (proxy != nullptr)
               {
                  wchar_t name[MAX_PARAM_NAME];
                  nx_swprintf(name, MAX_PARAM_NAME, L"MQTT.TopicData(\"%s\")", EscapeStringForAgent(dci.getName()).cstr());
                  *error = proxy->getMetricFromAgent(name, buffer, MAX_RESULT_LENGTH);
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
         case DS_CLOUD_CONNECTOR:
            if (dcTarget->getObjectClass() == OBJECT_RESOURCE)
            {
               *error = static_cast<Resource*>(dcTarget)->getMetricFromCloudConnector(dci.getName(), buffer, MAX_RESULT_LENGTH);
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
}

/**
 * Collect data for table
 */
static shared_ptr<Table> GetTableData(DataCollectionTarget *dcTarget, const DCTable& table, uint32_t *error)
{
	shared_ptr<Table> result;
   if (dcTarget->getObjectClass() == OBJECT_CLUSTER)
   {
      if (table.isAggregateOnCluster())
      {
         *error = static_cast<Cluster*>(dcTarget)->collectAggregatedData(table, &result);
      }
      else
      {
         *error = DCE_IGNORE;
      }
   }
   else
   {
      switch(table.getDataSource())
      {
         case DS_INTERNAL:    // Server internal parameter tables
            *error = dcTarget->getInternalTable(table.getName(), &result);
            break;
		   case DS_NATIVE_AGENT:
			   if (dcTarget->getObjectClass() == OBJECT_NODE)
            {
				   *error = static_cast<Node*>(dcTarget)->getTableFromAgent(table.getName(), &result);
            }
			   else
            {
				   *error = DCE_NOT_SUPPORTED;
            }
			   break;
         case DS_SNMP_AGENT:
			   if (dcTarget->getObjectClass() == OBJECT_NODE)
            {
               *error = static_cast<Node*>(dcTarget)->getTableFromSNMP(table.getSnmpPort(), table.getSnmpVersion(), table.getName(), table.getColumns(), &result, table.getSnmpContext(),
                  (table.getFlags() & DCF_ADD_INSTANCE_OID_COLUMN) != 0);
            }
			   else
            {
				   *error = DCE_NOT_SUPPORTED;
            }
            break;
         case DS_SCRIPT:
            *error = dcTarget->getTableFromScript(table.getName(), &result, static_cast<DataCollectionTarget*>(table.getOwner().get()), table.createDescriptor());
            break;
		   default:
			   *error = DCE_NOT_SUPPORTED;
			   break;
	   }
      if ((*error == DCE_SUCCESS) && (result != nullptr))
         table.updateResultColumns(result);
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
      nxlog_debug_tag(DEBUG_TAG_DC_COLLECTOR, 7, _T("DataCollector(): about to destroy DC object [%u] \"%s\" owner=[%u]"),
            dcObject->getId(), dcObjectName.cstr(), (target != nullptr) ? target->getId() : 0);
      dcObject->deleteFromDatabase();
      return;
   }

   if (target == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_DC_COLLECTOR, 3, _T("DataCollector: attempt to collect data for non-existing node (DCI=[%u] \"%s\")"),
            dcObject->getId(), dcObjectName.cstr());

      // Update item's last poll time and clear busy flag so item can be polled again
      dcObject->setLastPollTime(Timestamp::now());
      dcObject->clearBusyFlag();
      return;
   }

   if (IsShutdownInProgress())
   {
      dcObject->clearBusyFlag();
      return;
   }

   nxlog_debug_tag(DEBUG_TAG_DC_COLLECTOR, 8, _T("DataCollector(): processing DC object %u \"%s\" owner=%u sourceNode=%u"),
         dcObject->getId(), dcObjectName.cstr(), target->getId(), dcObject->getSourceNode());
   uint32_t sourceNodeId = target->getEffectiveSourceNode(dcObject.get());
   if (sourceNodeId != 0)
   {
      shared_ptr<Node> sourceNode = static_pointer_cast<Node>(FindObjectById(sourceNodeId, OBJECT_NODE));
      if (sourceNode != nullptr)
      {
         if (((target->getObjectClass() == OBJECT_CHASSIS) && (static_cast<Chassis*>(target.get())->getControllerId() == sourceNodeId)) || sourceNode->isTrustedObject(target->getId()))
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
         // Change item's status to "not supported"
         dcObject->setStatus(ITEM_STATUS_NOT_SUPPORTED, true);
         target.reset();
      }
   }

   Timestamp currTime = Timestamp::now();
   if (target != nullptr)
   {
      if (!IsShutdownInProgress())
      {
         wchar_t value[MAX_RESULT_LENGTH];
         shared_ptr<Table> table;
         uint32_t error;
         switch(dcObject->getType())
         {
            case DCO_TYPE_ITEM:
               GetItemData(target.get(), static_cast<DCItem&>(*dcObject), value, &error);
               break;
            case DCO_TYPE_TABLE:
               table = GetTableData(target.get(), static_cast<DCTable&>(*dcObject), &error);
               break;
            default:
               error = DCE_NOT_SUPPORTED;
               break;
         }

         if ((error == DCE_NOT_SUPPORTED) && dcObject->isUnsupportedAsError())
            error = DCE_COLLECTION_ERROR;

         // Transform and store received value into database or handle error
         switch(error)
         {
            case DCE_SUCCESS:
               if (dcObject->getStatus() == ITEM_STATUS_NOT_SUPPORTED)
                  dcObject->setStatus(ITEM_STATUS_ACTIVE, true);
               static_cast<DataCollectionTarget*>(dcObject->getOwner().get())->processNewDCValue(dcObject, currTime, value, table, false);
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
            session_id_t sessionId = dcObject->processForcePoll();
            if (sessionId != -1)
            {
               NotifyClientSession(sessionId, NX_NOTIFY_FORCE_DCI_POLL, dcObject->getOwnerId());
            }
         }
      }
   }
   else     /* target == nullptr */
   {
      shared_ptr<DataCollectionOwner> n = dcObject->getOwner();
      nxlog_debug_tag(DEBUG_TAG_DC_COLLECTOR, 5, _T("Attempt to collect data from non-existing or inaccessible node (DCI=[%u] \"%s\" target=[%u] sourceNode=[%u])"),
                  dcObject->getId(), dcObjectName.cstr(), (n != nullptr) ? n->getId() : 0, sourceNodeId);
   }

   // Update item's last poll time and clear busy flag so item can be polled again
   dcObject->setLastPollTime(currTime);
   dcObject->clearBusyFlag();
}

/**
 * Callback for queueing DCIs
 */
static EnumerationCallbackResult QueueItems(NetObj *object, uint32_t *watchdogId)
{
   if (IsShutdownInProgress())
      return _STOP;

   WatchdogNotify(*watchdogId);
	nxlog_debug_tag(DEBUG_TAG_DC_POLLER, 8, _T("ItemPoller: calling DataCollectionTarget::queueItemsForPolling for object %s [%u]"), object->getName(), object->getId());
	static_cast<DataCollectionTarget*>(object)->queueItemsForPolling();
	return _CONTINUE;
}

/**
 * Item poller thread: check nodes' items and put into the
 * data collector queue when data polling required
 */
static void ItemPoller()
{
   ThreadSetName("ItemPoller");

   nxlog_debug_tag(DEBUG_TAG_DC_POLLER, 1, _T("DCI poller thread started"));

   uint32_t watchdogId = WatchdogAddThread(_T("Item Poller"), 10);
   GaugeData<uint32_t> queuingTime(ITEM_POLLING_INTERVAL, 300);

   while(!IsShutdownInProgress())
   {
      if (SleepAndCheckForShutdown(ITEM_POLLING_INTERVAL))
         break;      // Shutdown has arrived
      WatchdogNotify(watchdogId);
      nxlog_debug_tag(DEBUG_TAG_DC_POLLER, 8, _T("ItemPoller: wakeup"));

      int64_t startTime = GetCurrentTimeMs();
      g_idxAccessPointById.forEach(QueueItems, &watchdogId);
      g_idxChassisById.forEach(QueueItems, &watchdogId);
      g_idxCircuitById.forEach(QueueItems, &watchdogId);
      g_idxCloudDomainById.forEach(QueueItems, &watchdogId);
      g_idxClusterById.forEach(QueueItems, &watchdogId);
      g_idxCollectorById.forEach(QueueItems, &watchdogId);
		g_idxMobileDeviceById.forEach(QueueItems, &watchdogId);
      g_idxNodeById.forEach(QueueItems, &watchdogId);
		g_idxResourceById.forEach(QueueItems, &watchdogId);
      g_idxSensorById.forEach(QueueItems, &watchdogId);

		queuingTime.update(static_cast<uint32_t>(GetCurrentTimeMs() - startTime));
		g_averageDCIQueuingTime = static_cast<uint32_t>(queuingTime.getAverage());
   }
   nxlog_debug_tag(DEBUG_TAG_DC_POLLER, 1, _T("DCI poller thread stopped"));
}

/**
 * DCI cache loader
 */
static void CacheLoader()
{
   ThreadSetName("CacheLoader");
   nxlog_debug_tag(DEBUG_TAG_DC_CACHE, 2, _T("DCI cache loader thread started"));
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
            nxlog_debug_tag(DEBUG_TAG_DC_CACHE, 6, _T("Loading cache for DCI %s [%d] on %s [%d]"),
                     ref->getName(), ref->getId(), object->getName(), object->getId());
            static_cast<DCItem*>(dci.get())->reloadCache(false);
         }
      }
   }
   nxlog_debug_tag(DEBUG_TAG_DC_CACHE, 2, _T("DCI cache loader thread stopped"));
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
   g_dataCollectorThreadPool = ThreadPoolCreate(L"DATACOLL",
            ConfigReadInt(L"ThreadPool.DataCollector.BaseSize", 10),
            ConfigReadInt(L"ThreadPool.DataCollector.MaxSize", 250),
            256 * 1024);

   g_thresholdRepeatPool = ThreadPoolCreate(L"THREVT", 2, 4);

   s_itemPollerThread = ThreadCreateEx(ItemPoller);
   s_cacheLoaderThread = ThreadCreateEx(CacheLoader);

   ScheduleThresholdRepeatEventsOnStartup();
}

/**
 * Stop data collection
 */
void StopDataCollection()
{
   ThreadJoin(s_itemPollerThread);
   ThreadJoin(s_cacheLoaderThread);
   ThreadPoolDestroy(g_dataCollectorThreadPool);
   ThreadPoolDestroy(g_thresholdRepeatPool);
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
static EnumerationCallbackResult UpdateParamList(NetObj *object, void *data)
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

	return _CONTINUE;
}

/**
 * Update table list from node
 */
static EnumerationCallbackResult UpdateTableList(NetObj *object, void *data)
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

	return _CONTINUE;
}


/**
 * Write full (from all nodes) agent parameters list to NXCP message
 */
void WriteFullParamListToMessage(NXCPMessage *msg, int origin, uint16_t flags)
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
		msg->setField(VID_NUM_PARAMETERS, (UINT32)fullList.size());
      UINT32 varId = VID_PARAM_LIST_BASE;
		for(int i = 0; i < fullList.size(); i++)
		{
         varId += fullList.get(i)->fillMessage(msg, varId);
		}
	}

   // Gather full table list
	if (flags & 0x02)
	{
		ObjectArray<AgentTableDefinition> fullList(64, 64, Ownership::True);
		g_idxNodeById.forEach(UpdateTableList, &fullList);

		// Put list into the message
		msg->setField(VID_NUM_TABLES, (UINT32)fullList.size());
		uint32_t fieldId = VID_TABLE_LIST_BASE;
		for(int i = 0; i < fullList.size(); i++)
		{
         fieldId += fullList.get(i)->fillMessage(msg, fieldId);
		}
	}
}

/**
 * Get type of data collection object
 */
int GetDCObjectType(uint32_t nodeId, uint32_t dciId)
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

/**
 * Parse Modbus metric
 * Expected metric format is
 *    [unit:][source:]address[|conversion]
 * If unit is not set in metric, unitId passed to the function will remain unchanged
 */
bool ParseModbusMetric(const TCHAR *metric, uint16_t *unitId, const TCHAR **source, int *address, const TCHAR **conversion)
{
   const TCHAR *addressPart = metric;
   *source = _T("hold:");
   const TCHAR *p = _tcschr(metric, _T(':'));
   if (p != nullptr)
   {
      // At least one : is present, check for second one
      p++;
      const TCHAR *n = _tcschr(p, _T(':'));
      if (n != nullptr)
      {
         TCHAR *eptr;
         uint16_t uid = static_cast<uint16_t>(_tcstoul(metric, &eptr, 10));
         if ((uid > 255) || (*eptr != _T(':')))
         {
            nxlog_debug_tag(DEBUG_TAG_DC_MODBUS, 7, L"ParseModbusMetric(%s): invalid unit ID", metric);
            return false;
         }
         *unitId = uid;
         *source = p;
         addressPart = n + 1;
      }
      else
      {
         // Only source and address
         *source = metric;
         addressPart = p;
      }
   }

   TCHAR *eptr;
   int a = _tcstol(addressPart, &eptr, 0);
   if ((a < 0) || (a > 65535) || ((*eptr != 0) && (*eptr != _T('|'))))
   {
      nxlog_debug_tag(DEBUG_TAG_DC_MODBUS, 7, L"ParseModbusMetric(%s): invalid register address", metric);
      return false;
   }
   *address = a;

   *conversion = (*eptr == _T('|')) ? eptr + 1 : _T("");
   return true;
}

#define DEBUG_TAG_DC_V5MIGRATE  L"dc.v5migrate"

/**
 * Throttle V5 data migration if needed. Returns false if shutdown time has arrived and migration process should be aborted.
 */
static bool ThrottleV5DataMigration()
{
   size_t throttlingHighWatermark = ConfigReadInt(_T("Housekeeper.Throttle.HighWatermark"), 250000);
   size_t throttlingLowWatermark = ConfigReadInt(_T("Housekeeper.Throttle.LowWatermark"), 50000);

   size_t qsize = g_dbWriterQueue.size() + static_cast<size_t>(GetIDataWriterQueueSize());
   if (qsize < throttlingHighWatermark)
      return true;

   nxlog_debug_tag(DEBUG_TAG_DC_V5MIGRATE, 1, _T("V5 data migration paused (queue size %d, high watermark %d, low watermark %d)"),
      static_cast<int>(qsize), static_cast<int>(throttlingHighWatermark), static_cast<int>(throttlingLowWatermark));
   while((qsize >= throttlingLowWatermark) && !IsShutdownInProgress())
   {
      if (SleepAndCheckForShutdown(30))
         break;
      qsize = g_dbWriterQueue.size() + static_cast<size_t>(GetIDataWriterQueueSize());
   }
   nxlog_debug_tag(DEBUG_TAG_DC_V5MIGRATE, 1, _T("V5 data migration resumed (queue size %d)"), static_cast<int>(qsize));
   return !IsShutdownInProgress();
}

/**
 * Target number of rows to copy per v5 migration chunk. Chunking is done on item_id boundaries
 * (using the item_id-leading index that v5 per-object tables always carry), so a chunk always
 * contains complete DCIs and may slightly exceed this value when a single DCI holds more rows than
 * the limit.
 */
#define V5_MIGRATION_CHUNK_SIZE   50000

/**
 * Migrate one v5 data table (idata or tdata) of given data collection target into the corresponding
 * v6 table, then drop the v5 table. Rows are copied (never deleted) in chunks bounded by item_id
 * ranges, advancing a cursor over item_id. Each chunk is a single auto-committed INSERT, which bounds
 * transaction/WAL size while avoiding the I/O of deleting rows from a table that is dropped at the end.
 * The copy is idempotent (rows already present in the target are skipped), so a migration interrupted
 * by a server restart is simply restarted from the beginning on next start. Because every chunk
 * boundary is an item_id, each (item_id, timestamp) key is fully contained in a single chunk, so
 * within-chunk deduplication of the source is sufficient.
 * Returns true if the table was fully migrated and dropped, false on error or shutdown.
 */
static bool MigrateV5DataTable(DataCollectionTarget *target, DB_HANDLE hdb, bool tdata)
{
   uint32_t id = target->getId();
   const wchar_t *prefix = tdata ? L"tdata" : L"idata";
   const wchar_t *valueColumns = tdata ? L"tdata_value" : L"idata_value,raw_value";
   const wchar_t *tsConv = V5TimestampToMs(tdata);
   wchar_t query[2048];

   uint32_t lastItem = 0;
   while(!IsShutdownInProgress())
   {
      // Find the item_id at the chunk boundary: the V5_MIGRATION_CHUNK_SIZE-th not-yet-processed row
      // in item_id order. The item_id>lastItem predicate lets the item_id index seek straight to the
      // cursor position, so the probe stays O(chunk size) regardless of table size. If fewer rows
      // remain, there is no boundary and the rest of the table is the final chunk.
      if ((g_dbSyntax == DB_SYNTAX_MYSQL) || (g_dbSyntax == DB_SYNTAX_PGSQL) || (g_dbSyntax == DB_SYNTAX_SQLITE))
         nx_swprintf(query, 2048, L"SELECT item_id FROM %s_v5_%u WHERE item_id>%u ORDER BY item_id LIMIT 1 OFFSET %d",
            prefix, id, lastItem, V5_MIGRATION_CHUNK_SIZE - 1);
      else
         nx_swprintf(query, 2048, L"SELECT item_id FROM %s_v5_%u WHERE item_id>%u ORDER BY item_id OFFSET %d ROWS FETCH NEXT 1 ROWS ONLY",
            prefix, id, lastItem, V5_MIGRATION_CHUNK_SIZE - 1);
      DB_RESULT hResult = DBSelect(hdb, query);
      if (hResult == nullptr)
         return false;
      bool hasBoundary = (DBGetNumRows(hResult) > 0);
      uint32_t boundaryItem = hasBoundary ? DBGetFieldULong(hResult, 0, 0) : 0;
      DBFreeResult(hResult);

      wchar_t rangeClause[96];
      if (hasBoundary)
         nx_swprintf(rangeClause, 96, L" WHERE item_id>%u AND item_id<=%u", lastItem, boundaryItem);
      else
         nx_swprintf(rangeClause, 96, L" WHERE item_id>%u", lastItem);

      switch(g_dbSyntax)
      {
         case DB_SYNTAX_MYSQL:
            nx_swprintf(query, 2048,
               L"INSERT IGNORE INTO %s_%u (item_id,%s_timestamp,%s) SELECT item_id,%s,%s FROM %s_v5_%u%s",
               prefix, id, prefix, valueColumns, tsConv, valueColumns, prefix, id, rangeClause);
            break;
         case DB_SYNTAX_PGSQL:
         case DB_SYNTAX_SQLITE:
            nx_swprintf(query, 2048,
               L"INSERT INTO %s_%u (item_id,%s_timestamp,%s) SELECT item_id,%s,%s FROM %s_v5_%u%s ON CONFLICT DO NOTHING",
               prefix, id, prefix, valueColumns, tsConv, valueColumns, prefix, id, rangeClause);
            break;
         default:
            // Databases without an upsert/ignore clause: deduplicate the source within the chunk with
            // ROW_NUMBER and skip rows already present in the target (restart-safe) with NOT EXISTS.
            nx_swprintf(query, 2048,
               L"INSERT INTO %s_%u (item_id,%s_timestamp,%s) SELECT item_id,%s_timestamp,%s FROM ("
               L"SELECT item_id,%s AS %s_timestamp,%s,"
               L"ROW_NUMBER() OVER (PARTITION BY item_id,%s ORDER BY %s_timestamp) AS rn"
               L" FROM %s_v5_%u%s) sub WHERE rn=1 AND NOT EXISTS ("
               L"SELECT 1 FROM %s_%u d WHERE d.item_id=sub.item_id AND d.%s_timestamp=sub.%s_timestamp)",
               prefix, id, prefix, valueColumns, prefix, valueColumns,
               tsConv, prefix, valueColumns, tsConv, prefix, prefix, id, rangeClause,
               prefix, id, prefix, prefix);
            break;
      }

      if (!DBQuery(hdb, query))
         return false;

      if (!hasBoundary)
      {
         // Whole table consumed in this final chunk
         target->deleteV5DataTable(hdb, tdata, L"migration complete");
         return true;
      }

      if (!ThrottleV5DataMigration())
         break;

      lastItem = boundaryItem;
   }
   return false;
}

/**
 * Migrate V5 data for given data collection target
 */
static void MigrateV5Data(DataCollectionTarget *target)
{
   if (IsShutdownInProgress())
      return;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   if (target->hasV5IdataTable())
      MigrateV5DataTable(target, hdb, false);

   if (!IsShutdownInProgress() && target->hasV5TdataTable())
      MigrateV5DataTable(target, hdb, true);

   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Background thread handle for v5 data migration
 */
static THREAD s_v5DataMigrationThread = INVALID_THREAD_HANDLE;

/**
 * Background thread for managing migration of v5 data.
 * Each eligible object is submitted to a pool of worker connections that fully drains its v5 data
 * tables (copying rows in item_id-bounded chunks) and drops them when done. Objects are processed in
 * parallel; a sweep that finds no remaining v5 tables ends the migration. An object whose migration
 * failed (e.g. transient DB error) keeps its v5 flag and is retried on the next sweep.
 */
static void V5DataMigrationManager()
{
   nxlog_debug_tag(DEBUG_TAG_DC_V5MIGRATE, 1, L"V5 data migration manager started");

   auto filter =
      [] (NetObj *object) -> bool
      {
         DataCollectionTarget *dct = static_cast<DataCollectionTarget*>(object);
         return dct->hasV5IdataTable() || dct->hasV5TdataTable();
      };

   int workers = ConfigReadInt(L"DBConnectionPool.MaxSize", 30) / 3;
   if (workers < 2)
      workers = 2;
   else if (workers > 8)
      workers = 8;
   ThreadPool *migrationPool = ThreadPoolCreate(L"V5MIGRATE", workers, workers);

   while(!SleepAndCheckForShutdown(5))
   {
      SharedObjectArray<NetObj> objects(1024, 1024);
      g_idxAccessPointById.getObjects(&objects, filter);
      g_idxChassisById.getObjects(&objects, filter);
      g_idxCircuitById.getObjects(&objects, filter);
      g_idxCloudDomainById.getObjects(&objects, filter);
      g_idxClusterById.getObjects(&objects, filter);
      g_idxCollectorById.getObjects(&objects, filter);
      g_idxMobileDeviceById.getObjects(&objects, filter);
      g_idxNodeById.getObjects(&objects, filter);
      g_idxResourceById.getObjects(&objects, filter);
      g_idxSensorById.getObjects(&objects, filter);

      if (objects.isEmpty())
      {
         nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_DC_V5MIGRATE, L"All v5 data migration completed");
         break;
      }

      nxlog_debug_tag(DEBUG_TAG_DC_V5MIGRATE, 5, L"Starting data migration batch (%d objects)", objects.size());
      for(int i = 0; i < objects.size(); i++)
      {
         ThreadPoolExecute(migrationPool, MigrateV5Data, static_cast<DataCollectionTarget*>(objects.get(i)));
      }

      while(!SleepAndCheckForShutdown(1))
      {
         ThreadPoolInfo tpi;
         ThreadPoolGetInfo(migrationPool, &tpi);
         if (tpi.activeRequests == 0)
         {
            nxlog_debug_tag(DEBUG_TAG_DC_V5MIGRATE, 5, L"Batch completed");
            break;
         }
      }
   }

   ThreadPoolDestroy(migrationPool);
   nxlog_debug_tag(DEBUG_TAG_DC_V5MIGRATE, 1, _T("V5 data migration manager stopped"));
}

/**
 * Start background v5 data migration
 */
void StartV5DataMigration()
{
   if (g_flags & AF_SINGLE_TABLE_PERF_DATA)
      return;

   // Check if any v5 tables exist before starting thread
   bool hasV5Tables = false;
   auto callback = [&hasV5Tables] (NetObj *object) -> EnumerationCallbackResult
   {
      DataCollectionTarget *dct = static_cast<DataCollectionTarget*>(object);
      if (dct->hasV5IdataTable() || dct->hasV5TdataTable())
      {
         hasV5Tables = true;
         return _STOP;
      }
      return _CONTINUE;
   };
   g_idxAccessPointById.forEach(callback);
   if (!hasV5Tables)
      g_idxChassisById.forEach(callback);
   if (!hasV5Tables)
      g_idxCircuitById.forEach(callback);
   if (!hasV5Tables)
      g_idxCloudDomainById.forEach(callback);
   if (!hasV5Tables)
      g_idxClusterById.forEach(callback);
   if (!hasV5Tables)
      g_idxCollectorById.forEach(callback);
   if (!hasV5Tables)
      g_idxMobileDeviceById.forEach(callback);
   if (!hasV5Tables)
      g_idxNodeById.forEach(callback);
   if (!hasV5Tables)
      g_idxResourceById.forEach(callback);
   if (!hasV5Tables)
      g_idxSensorById.forEach(callback);

   if (hasV5Tables)
      s_v5DataMigrationThread = ThreadCreateEx(V5DataMigrationManager);
   else
      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_DC_V5MIGRATE, L"No v5 data tables found, skipping migration");
}

/**
 * Stop background v5 data migration
 */
void StopV5DataMigration()
{
   ThreadJoin(s_v5DataMigrationThread);
}
