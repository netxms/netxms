/*
** NetXMS - Network Management System
** Copyright (C) 2003-2019 Victor Kirhenshtein
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
** File: dctarget.cpp
**
**/

#include "nxcore.h"

/**
 * Data collector thread pool
 */
extern ThreadPool *g_dataCollectorThreadPool;

/**
 * Data collector worker
 */
void DataCollector(void *arg);

/**
 * Throttle housekeeper if needed. Returns false if shutdown time has arrived and housekeeper process should be aborted.
 */
bool ThrottleHousekeeper();

/**
 * Default constructor
 */
DataCollectionTarget::DataCollectionTarget() : super()
{
   m_deletedItems = new IntegerArray<UINT32>(32, 32);
   m_deletedTables = new IntegerArray<UINT32>(32, 32);
   m_scriptErrorReports = new StringMap();
   m_pingLastTimeStamp = 0;
   m_pingTime = PING_TIME_TIMEOUT;
   m_lastConfigurationPoll = 0;
   m_lastStatusPoll = 0;
   m_lastInstancePoll = 0;
   m_hPollerMutex = MutexCreate();
}

/**
 * Constructor for creating new data collection capable objects
 */
DataCollectionTarget::DataCollectionTarget(const TCHAR *name) : super(name)
{
   m_deletedItems = new IntegerArray<UINT32>(32, 32);
   m_deletedTables = new IntegerArray<UINT32>(32, 32);
   m_scriptErrorReports = new StringMap();
   m_pingLastTimeStamp = 0;
   m_pingTime = PING_TIME_TIMEOUT;
   m_lastConfigurationPoll = 0;
   m_lastStatusPoll = 0;
   m_lastInstancePoll = 0;
   m_hPollerMutex = MutexCreate();
}

/**
 * Destructor
 */
DataCollectionTarget::~DataCollectionTarget()
{
   delete m_deletedItems;
   delete m_deletedTables;
   delete m_scriptErrorReports;
   MutexDestroy(m_hPollerMutex);
}

/**
 * Delete object from database
 */
bool DataCollectionTarget::deleteFromDatabase(DB_HANDLE hdb)
{
   bool success = super::deleteFromDatabase(hdb);
   if(success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM dct_node_map WHERE node_id=?"));
   if (success)
   {
      TCHAR query[256];
      _sntprintf(query, 256, (g_flags & AF_SINGLE_TABLE_PERF_DATA) ? _T("DELETE FROM idata WHERE node_id=%u") : _T("DROP TABLE idata_%u"), m_id);
      QueueSQLRequest(query);

      _sntprintf(query, 256, (g_flags & AF_SINGLE_TABLE_PERF_DATA) ? _T("DELETE FROM tdata WHERE node_id=%u") : _T("DROP TABLE tdata_%u"), m_id);
      QueueSQLRequest(query);
   }
   return success;
}

/**
 * Create NXCP message with object's data
 */
void DataCollectionTarget::fillMessageInternal(NXCPMessage *msg, UINT32 userId)
{
   super::fillMessageInternal(msg, userId);
}

/**
 * Create NXCP message with object's data - stage 2
 */
void DataCollectionTarget::fillMessageInternalStage2(NXCPMessage *msg, UINT32 userId)
{
   super::fillMessageInternalStage2(msg, userId);

   // Sent all DCIs marked for display on overview page or in tooltips
   UINT32 fieldIdOverview = VID_OVERVIEW_DCI_LIST_BASE;
   UINT32 countOverview = 0;
   UINT32 fieldIdTooltip = VID_TOOLTIP_DCI_LIST_BASE;
   UINT32 countTooltip = 0;
   lockDciAccess(false);
   for(int i = 0; i < m_dcObjects->size(); i++)
	{
      DCObject *dci = m_dcObjects->get(i);
      if ((dci->getType() == DCO_TYPE_ITEM) &&
          (dci->getStatus() == ITEM_STATUS_ACTIVE) &&
          (((DCItem *)dci)->getInstanceDiscoveryMethod() == IDM_NONE) &&
          dci->hasAccess(userId))
		{
         if  (dci->isShowInObjectOverview())
         {
            countOverview++;
            ((DCItem *)dci)->fillLastValueMessage(msg, fieldIdOverview);
            fieldIdOverview += 50;
         }
         if  (dci->isShowOnObjectTooltip())
         {
            countTooltip++;
            ((DCItem *)dci)->fillLastValueMessage(msg, fieldIdTooltip);
            fieldIdTooltip += 50;
         }
		}
	}
   unlockDciAccess();
   msg->setField(VID_OVERVIEW_DCI_COUNT, countOverview);
   msg->setField(VID_TOOLTIP_DCI_COUNT, countTooltip);
}

/**
 * Modify object from message
 */
UINT32 DataCollectionTarget::modifyFromMessageInternal(NXCPMessage *pRequest)
{
   return super::modifyFromMessageInternal(pRequest);
}

/**
 * Update cache for all DCI's
 */
void DataCollectionTarget::updateDciCache()
{
	lockDciAccess(false);
   for(int i = 0; i < m_dcObjects->size(); i++)
	{
		if (m_dcObjects->get(i)->getType() == DCO_TYPE_ITEM)
		{
			((DCItem *)m_dcObjects->get(i))->updateCacheSize();
		}
	}
	unlockDciAccess();
}

/**
 * Clean expired DCI data
 */
void DataCollectionTarget::cleanDCIData(DB_HANDLE hdb)
{
   String queryItems = _T("DELETE FROM idata");
   if (g_flags & AF_SINGLE_TABLE_PERF_DATA)
   {
      queryItems.append(_T(" WHERE node_id="));
      queryItems.append(m_id);
      queryItems.append(_T(" AND "));
   }
   else
   {
      queryItems.append(_T('_'));
      queryItems.append(m_id);
      queryItems.append(_T(" WHERE "));
   }

   String queryTables = _T("DELETE FROM tdata");
   if (g_flags & AF_SINGLE_TABLE_PERF_DATA)
   {
      queryTables.append(_T(" WHERE node_id="));
      queryTables.append(m_id);
      queryTables.append(_T(" AND "));
   }
   else
   {
      queryTables.append(_T('_'));
      queryTables.append(m_id);
      queryTables.append(_T(" WHERE "));
   }

   int itemCount = 0;
   int tableCount = 0;
   time_t now = time(NULL);

   lockDciAccess(false);
   for(int i = 0; i < m_dcObjects->size(); i++)
   {
      DCObject *o = m_dcObjects->get(i);
      if (o->getType() == DCO_TYPE_ITEM)
      {
         if (itemCount > 0)
            queryItems.append(_T(" OR "));
         queryItems.append(_T("(item_id="));
         queryItems.append(o->getId());
         queryItems.append(_T(" AND idata_timestamp<"));
         queryItems.append((INT64)(now - o->getEffectiveRetentionTime() * 86400));
         queryItems.append(_T(')'));
         itemCount++;
      }
      else if (o->getType() == DCO_TYPE_TABLE)
      {
         if (tableCount > 0)
            queryTables.append(_T(" OR "));
         queryTables.append(_T("(item_id="));
         queryTables.append(o->getId());
         queryTables.append(_T(" AND tdata_timestamp<"));
         queryTables.append((INT64)(now - o->getEffectiveRetentionTime() * 86400));
         queryTables.append(_T(')'));
         tableCount++;
      }
   }
   unlockDciAccess();

   lockProperties();
   for(int i = 0; i < m_deletedItems->size(); i++)
   {
      if (itemCount > 0)
         queryItems.append(_T(" OR "));
      queryItems.append(_T("item_id="));
      queryItems.append(m_deletedItems->get(i));
      itemCount++;
   }
   m_deletedItems->clear();

   for(int i = 0; i < m_deletedTables->size(); i++)
   {
      if (tableCount > 0)
         queryTables.append(_T(" OR "));
      queryTables.append(_T("item_id="));
      queryTables.append(m_deletedItems->get(i));
      tableCount++;
   }
   m_deletedTables->clear();
   unlockProperties();

   if (itemCount > 0)
   {
      nxlog_debug_tag(_T("housekeeper"), 6, _T("DataCollectionTarget::cleanDCIData(%s [%d]): running query \"%s\""), m_name, m_id, (const TCHAR *)queryItems);
      DBQuery(hdb, queryItems);
      if (!ThrottleHousekeeper())
         return;
   }

   if (tableCount > 0)
   {
      nxlog_debug_tag(_T("housekeeper"), 6, _T("DataCollectionTarget::cleanDCIData(%s [%d]): running query \"%s\""), m_name, m_id, (const TCHAR *)queryTables);
      DBQuery(hdb, queryTables);
   }
}

/**
 * Queue prediction engine training when necessary
 */
void DataCollectionTarget::queuePredictionEngineTraining()
{
   lockDciAccess(false);
   for(int i = 0; i < m_dcObjects->size(); i++)
   {
      DCObject *o = m_dcObjects->get(i);
      if (o->getType() == DCO_TYPE_ITEM)
      {
         DCItem *dci = static_cast<DCItem*>(o);
         if (dci->getPredictionEngine()[0] != 0)
         {
            PredictionEngine *e = FindPredictionEngine(dci->getPredictionEngine());
            if ((e != NULL) && e->requiresTraining())
            {
               QueuePredictionEngineTraining(e, dci->getOwner()->getId(), dci->getId());
            }
         }
      }
   }
   unlockDciAccess();
}

/**
 * Schedule cleanup of DCI data after DCI deletion
 */
void DataCollectionTarget::scheduleItemDataCleanup(UINT32 dciId)
{
   lockProperties();
   m_deletedItems->add(dciId);
   unlockProperties();
}

/**
 * Schedule cleanup of table DCI data after DCI deletion
 */
void DataCollectionTarget::scheduleTableDataCleanup(UINT32 dciId)
{
   lockProperties();
   m_deletedTables->add(dciId);
   unlockProperties();
}

/**
 * Get last collected values of given table
 */
UINT32 DataCollectionTarget::getTableLastValues(UINT32 dciId, NXCPMessage *msg)
{
	UINT32 rcc = RCC_INVALID_DCI_ID;

   lockDciAccess(false);

   for(int i = 0; i < m_dcObjects->size(); i++)
	{
		DCObject *object = m_dcObjects->get(i);
		if ((object->getId() == dciId) && (object->getType() == DCO_TYPE_TABLE))
		{
			((DCTable *)object)->fillLastValueMessage(msg);
			rcc = RCC_SUCCESS;
			break;
		}
	}

   unlockDciAccess();
   return rcc;
}

/**
 * Apply DCI from template
 * dcObject passed to this method should be a template's DCI
 */
bool DataCollectionTarget::applyTemplateItem(UINT32 dwTemplateId, DCObject *dcObject)
{
   bool bResult = true;

   lockDciAccess(true);	// write lock

   nxlog_debug(5, _T("Applying DCO \"%s\" to target \"%s\""), dcObject->getName(), m_name);

   // Check if that template item exists
	int i;
   for(i = 0; i < m_dcObjects->size(); i++)
      if ((m_dcObjects->get(i)->getTemplateId() == dwTemplateId) &&
          (m_dcObjects->get(i)->getTemplateItemId() == dcObject->getId()))
         break;   // Item with specified id already exist

   if (i == m_dcObjects->size())
   {
      // New item from template, just add it
		DCObject *newObject = dcObject->clone();
      newObject->setTemplateId(dwTemplateId, dcObject->getId());
      newObject->changeBinding(CreateUniqueId(IDG_ITEM), this, TRUE);
      bResult = addDCObject(newObject, true);
   }
   else
   {
      // Update existing item unless it is disabled
      DCObject *curr = m_dcObjects->get(i);
      curr->updateFromTemplate(dcObject);
      if (curr->getInstanceDiscoveryMethod() != IDM_NONE)
      {
         updateInstanceDiscoveryItems(curr);
      }
   }

   unlockDciAccess();

	if (bResult)
	{
		lockProperties();
		setModified(MODIFY_DATA_COLLECTION, false);
		unlockProperties();
	}
   return bResult;
}

/**
 * Clean deleted template items from target's DCI list
 * Arguments is template id and list of valid template item ids.
 * all items related to given template and not presented in list should be deleted.
 */
void DataCollectionTarget::cleanDeletedTemplateItems(UINT32 dwTemplateId, UINT32 dwNumItems, UINT32 *pdwItemList)
{
   UINT32 i, j, dwNumDeleted, *pdwDeleteList;

   lockDciAccess(true);  // write lock

   pdwDeleteList = (UINT32 *)malloc(sizeof(UINT32) * m_dcObjects->size());
   dwNumDeleted = 0;

   for(i = 0; i < (UINT32)m_dcObjects->size(); i++)
      if (m_dcObjects->get(i)->getTemplateId() == dwTemplateId)
      {
         for(j = 0; j < dwNumItems; j++)
            if (m_dcObjects->get(i)->getTemplateItemId() == pdwItemList[j])
               break;

         // Delete DCI if it's not in list
         if (j == dwNumItems)
            pdwDeleteList[dwNumDeleted++] = m_dcObjects->get(i)->getId();
      }

   for(i = 0; i < dwNumDeleted; i++)
      deleteDCObject(pdwDeleteList[i], false, 0);

   unlockDciAccess();
   free(pdwDeleteList);
}

/**
 * Unbind data collection target from template, i.e either remove DCI
 * association with template or remove these DCIs at all
 */
void DataCollectionTarget::unbindFromTemplate(UINT32 dwTemplateId, bool removeDCI)
{
   if (removeDCI)
   {
      lockDciAccess(true);  // write lock

		UINT32 *deleteList = (UINT32 *)malloc(sizeof(UINT32) * m_dcObjects->size());
		int numDeleted = 0;

		int i;
      for(i = 0; i < m_dcObjects->size(); i++)
         if (m_dcObjects->get(i)->getTemplateId() == dwTemplateId)
         {
            deleteList[numDeleted++] = m_dcObjects->get(i)->getId();
         }

		for(i = 0; i < numDeleted; i++)
			deleteDCObject(deleteList[i], false, 0);

      unlockDciAccess();
		free(deleteList);
   }
   else
   {
      lockDciAccess(false);

      for(int i = 0; i < m_dcObjects->size(); i++)
         if (m_dcObjects->get(i)->getTemplateId() == dwTemplateId)
         {
            m_dcObjects->get(i)->setTemplateId(0, 0);
         }

      unlockDciAccess();
   }
}

/**
 * Get list of DCIs to be shown on performance tab
 */
UINT32 DataCollectionTarget::getPerfTabDCIList(NXCPMessage *pMsg, UINT32 userId)
{
	lockDciAccess(false);

	UINT32 dwId = VID_SYSDCI_LIST_BASE, dwCount = 0;
   for(int i = 0; i < m_dcObjects->size(); i++)
	{
		DCObject *object = m_dcObjects->get(i);
      if ((object->getPerfTabSettings() != NULL) &&
          object->hasValue() &&
          (object->getStatus() == ITEM_STATUS_ACTIVE) &&
          object->matchClusterResource() &&
          object->hasAccess(userId))
		{
			pMsg->setField(dwId++, object->getId());
			pMsg->setField(dwId++, object->getDescription());
			pMsg->setField(dwId++, (WORD)object->getStatus());
			pMsg->setField(dwId++, object->getPerfTabSettings());
			pMsg->setField(dwId++, (WORD)object->getType());
			pMsg->setField(dwId++, object->getTemplateItemId());
			if (object->getType() == DCO_TYPE_ITEM)
			{
				pMsg->setField(dwId++, ((DCItem *)object)->getInstance());
            if ((object->getTemplateItemId() != 0) && (object->getTemplateId() == m_id))
            {
               // DCI created via instance discovery - send ID of root template item
               // to allow UI to resolve double template case
               // (template -> instance discovery item on node -> actual item on node)
               DCObject *src = getDCObjectById(object->getTemplateItemId(), userId, false);
               pMsg->setField(dwId++, (src != NULL) ? src->getTemplateItemId() : 0);
               dwId += 2;
            }
            else
            {
				   dwId += 3;
            }
			}
			else
			{
				dwId += 4;
			}
			dwCount++;
		}
	}
   pMsg->setField(VID_NUM_ITEMS, dwCount);

	unlockDciAccess();
   return RCC_SUCCESS;
}

/**
 * Get threshold violation summary into NXCP message
 */
UINT32 DataCollectionTarget::getThresholdSummary(NXCPMessage *msg, UINT32 baseId, UINT32 userId)
{
	UINT32 varId = baseId;

	msg->setField(varId++, m_id);
	UINT32 countId = varId++;
	UINT32 count = 0;

	lockDciAccess(false);
   for(int i = 0; i < m_dcObjects->size(); i++)
	{
		DCObject *object = m_dcObjects->get(i);
		if (object->hasValue() && (object->getType() == DCO_TYPE_ITEM) && (object->getStatus() == ITEM_STATUS_ACTIVE) && object->hasAccess(userId))
		{
			if (((DCItem *)object)->hasActiveThreshold())
			{
				((DCItem *)object)->fillLastValueMessage(msg, varId);
				varId += 50;
				count++;
			}
		}
	}
	unlockDciAccess();
	msg->setField(countId, count);
   return varId;
}

/**
 * Process new DCI value
 */
bool DataCollectionTarget::processNewDCValue(DCObject *dco, time_t currTime, const void *value)
{
   bool updateStatus;
	bool result = dco->processNewValue(currTime, value, &updateStatus);
	if (updateStatus)
	{
      calculateCompoundStatus(FALSE);
   }
   return result;
}

/**
 * Check if data collection is disabled
 */
bool DataCollectionTarget::isDataCollectionDisabled()
{
	return false;
}

/**
 * Put items which requires polling into the queue
 */
void DataCollectionTarget::queueItemsForPolling()
{
   if ((m_status == STATUS_UNMANAGED) || isDataCollectionDisabled() || m_isDeleted)
      return;  // Do not collect data for unmanaged objects or if data collection is disabled

   time_t currTime = time(NULL);

   lockDciAccess(false);
   for(int i = 0; i < m_dcObjects->size(); i++)
   {
		DCObject *object = m_dcObjects->get(i);
      if (object->isReadyForPolling(currTime))
      {
         object->setBusyFlag();
         incRefCount();   // Increment reference count for each queued DCI

         if ((object->getDataSource() == DS_NATIVE_AGENT) ||
             (object->getDataSource() == DS_WINPERF) ||
             (object->getDataSource() == DS_SSH) ||
             (object->getDataSource() == DS_SMCLP))
         {
            TCHAR key[32];
            _sntprintf(key, 32, _T("%08X/%s"),
                     m_id, (object->getDataSource() == DS_SSH) ? _T("ssh") :
                              (object->getDataSource() == DS_SMCLP) ? _T("smclp") : _T("agent"));
            ThreadPoolExecuteSerialized(g_dataCollectorThreadPool, key, DataCollector, object);
         }
         else
         {
            ThreadPoolExecute(g_dataCollectorThreadPool, DataCollector, object);
         }
			nxlog_debug_tag(_T("obj.dc.queue"), 8, _T("DataCollectionTarget(%s)->QueueItemsForPolling(): item %d \"%s\" added to queue"), m_name, object->getId(), object->getName());
      }
   }
   unlockDciAccess();
}

/**
 * Get object from parameter
 */
NetObj *DataCollectionTarget::objectFromParameter(const TCHAR *param)
{
   TCHAR *eptr, arg[256];
   AgentGetParameterArg(param, 1, arg, 256);
   UINT32 objectId = _tcstoul(arg, &eptr, 0);
   if (*eptr != 0)
   {
      // Argument is object's name
      objectId = 0;
   }

   // Find child object with requested ID or name
   NetObj *object = NULL;
   lockChildList(false);
   for(int i = 0; i < m_childList->size(); i++)
   {
      NetObj *curr = m_childList->get(i);
      if (((objectId == 0) && (!_tcsicmp(curr->getName(), arg))) ||
          (objectId == curr->getId()))
      {
         object = curr;
         break;
      }
   }
   unlockChildList();
   return object;
}

/**
 * Get value for server's internal parameter
 */
DataCollectionError DataCollectionTarget::getInternalItem(const TCHAR *param, size_t bufSize, TCHAR *buffer)
{
   DataCollectionError error = DCE_SUCCESS;

   if (!_tcsicmp(param, _T("Status")))
   {
      _sntprintf(buffer, bufSize, _T("%d"), m_status);
   }
   else if (!_tcsicmp(param, _T("Dummy")) || MatchString(_T("Dummy(*)"), param, FALSE))
   {
      _tcscpy(buffer, _T("0"));
   }
   else if (MatchString(_T("ChildStatus(*)"), param, FALSE))
   {
      NetObj *object = objectFromParameter(param);
      if (object != NULL)
      {
         _sntprintf(buffer, bufSize, _T("%d"), object->getStatus());
      }
      else
      {
         error = DCE_NOT_SUPPORTED;
      }
   }
   else if (MatchString(_T("ConditionStatus(*)"), param, FALSE))
   {
      TCHAR *pEnd, szArg[256];
      UINT32 dwId;
      NetObj *pObject = NULL;

      AgentGetParameterArg(param, 1, szArg, 256);
      dwId = _tcstoul(szArg, &pEnd, 0);
      if (*pEnd == 0)
		{
			pObject = FindObjectById(dwId);
			if (pObject != NULL)
				if (pObject->getObjectClass() != OBJECT_CONDITION)
					pObject = NULL;
		}
		else
      {
         // Argument is object's name
			pObject = FindObjectByName(szArg, OBJECT_CONDITION);
      }

      if (pObject != NULL)
      {
			if (pObject->isTrustedNode(m_id))
			{
				_sntprintf(buffer, bufSize, _T("%d"), pObject->getStatus());
			}
			else
			{
	         error = DCE_NOT_SUPPORTED;
			}
      }
      else
      {
         error = DCE_NOT_SUPPORTED;
      }
   }
   else
   {
      error = DCE_NOT_SUPPORTED;
   }

   return error;
}

/**
 * Run data collection script. Returns pointer to NXSL VM after successful run and NULL on failure.
 */
NXSL_VM *DataCollectionTarget::runDataCollectionScript(const TCHAR *param, DataCollectionTarget *targetObject)
{
   TCHAR name[256];
   nx_strncpy(name, param, 256);
   Trim(name);

   NXSL_VM *vm = CreateServerScriptVM(name, this);
   if (vm != NULL)
   {
      ObjectRefArray<NXSL_Value> args(16, 16);

      // Can be in form parameter(arg1, arg2, ... argN)
      TCHAR *p = _tcschr(name, _T('('));
      if (p != NULL)
      {
         if (name[_tcslen(name) - 1] != _T(')'))
            return NULL;
         name[_tcslen(name) - 1] = 0;

         if (!ParseValueList(vm, &p, args))
         {
            // argument parsing error
            delete vm;
            return NULL;
         }
      }

      if (targetObject != NULL)
      {
         vm->setGlobalVariable("$targetObject", targetObject->createNXSLObject(vm));
      }
      if (!vm->run(&args))
      {
         DbgPrintf(6, _T("DataCollectionTarget(%s)->runDataCollectionScript(%s): Script execution error: %s"), m_name, param, vm->getErrorText());
         time_t now = time(NULL);
         time_t lastReport = static_cast<time_t>(m_scriptErrorReports->getInt64(param, 0));
         if (lastReport + ConfigReadInt(_T("DataCollection.ScriptErrorReportInterval"), 86400) < now)
         {
            PostEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", name, vm->getErrorText(), m_id);
            m_scriptErrorReports->set(param, static_cast<UINT64>(now));
         }
         delete_and_null(vm);
      }
   }
   nxlog_debug(7, _T("DataCollectionTarget(%s)->runDataCollectionScript(%s): %s"), m_name, param, (vm != NULL) ? _T("success") : _T("failure"));
   return vm;
}

/**
 * Get parameter value from NXSL script
 */
DataCollectionError DataCollectionTarget::getScriptItem(const TCHAR *param, size_t bufSize, TCHAR *buffer, DataCollectionTarget *targetObject)
{
   DataCollectionError rc = DCE_NOT_SUPPORTED;
   NXSL_VM *vm = runDataCollectionScript(param, targetObject);
   if (vm != NULL)
   {
      NXSL_Value *value = vm->getResult();
      if (value->isNull())
      {
         // NULL value is an error indicator
         rc = DCE_COLLECTION_ERROR;
      }
      else
      {
         const TCHAR *dciValue = value->getValueAsCString();
         nx_strncpy(buffer, CHECK_NULL_EX(dciValue), bufSize);
         rc = DCE_SUCCESS;
      }
      delete vm;
   }
   nxlog_debug(7, _T("DataCollectionTarget(%s)->getScriptItem(%s): rc=%d"), m_name, param, rc);
   return rc;
}

/**
 * Get list from library script
 */
UINT32 DataCollectionTarget::getListFromScript(const TCHAR *param, StringList **list, DataCollectionTarget *targetObject)
{
   UINT32 rc = DCE_NOT_SUPPORTED;
   NXSL_VM *vm = runDataCollectionScript(param, targetObject);
   if (vm != NULL)
   {
      rc = DCE_SUCCESS;
      NXSL_Value *value = vm->getResult();
      if (value->isArray())
      {
         *list = value->getValueAsArray()->toStringList();
      }
      else if (value->isString())
      {
         *list = new StringList;
         (*list)->add(value->getValueAsCString());
      }
      else if (value->isNull())
      {
         rc = DCE_COLLECTION_ERROR;
      }
      else
      {
         *list = new StringList;
      }
      delete vm;
   }
   nxlog_debug(7, _T("DataCollectionTarget(%s)->getListFromScript(%s): rc=%d"), m_name, param, rc);
   return rc;
}

/**
 * Get table from NXSL script
 */
DataCollectionError DataCollectionTarget::getScriptTable(const TCHAR *param, Table **result, DataCollectionTarget *targetObject)
{
   DataCollectionError rc = DCE_NOT_SUPPORTED;
   NXSL_VM *vm = runDataCollectionScript(param, targetObject);
   if (vm != NULL)
   {
      NXSL_Value *value = vm->getResult();
      if (value->isObject(_T("Table")))
      {
         *result = (Table *)value->getValueAsObject()->getData();
         (*result)->incRefCount();
         rc = DCE_SUCCESS;
      }
      else
      {
         rc = DCE_COLLECTION_ERROR;
      }
      delete vm;
   }
   nxlog_debug(7, _T("DataCollectionTarget(%s)->getScriptTable(%s): rc=%d"), m_name, param, rc);
   return rc;
}

/**
 * Get string map from library script
 */
UINT32 DataCollectionTarget::getStringMapFromScript(const TCHAR *param, StringMap **map, DataCollectionTarget *targetObject)
{
   TCHAR name[256];
   nx_strncpy(name, param, 256);
   Trim(name);

   UINT32 rc = DCE_NOT_SUPPORTED;
   NXSL_VM *vm = CreateServerScriptVM(name, this);
   if (vm != NULL)
   {
      ObjectRefArray<NXSL_Value> args(16, 16);

      // Can be in form parameter(arg1, arg2, ... argN)
      TCHAR *p = _tcschr(name, _T('('));
      if (p != NULL)
      {
         if (name[_tcslen(name) - 1] != _T(')'))
            return DCE_NOT_SUPPORTED;
         name[_tcslen(name) - 1] = 0;

         if (!ParseValueList(vm, &p, args))
         {
            // argument parsing error
            delete vm;
            return DCE_NOT_SUPPORTED;
         }
      }

      if (targetObject != NULL)
      {
         vm->setGlobalVariable("$targetObject", targetObject->createNXSLObject(vm));
      }
      if (vm->run(&args))
      {
         rc = DCE_SUCCESS;
         NXSL_Value *value = vm->getResult();
         if (value->isHashMap())
         {
            *map = value->getValueAsHashMap()->toStringMap();
         }
         else if (value->isArray())
         {
            *map = new StringMap();
            NXSL_Array *a = value->getValueAsArray();
            for(int i = 0; i < a->size(); i++)
            {
               NXSL_Value *v = a->getByPosition(i);
               if (v->isString())
               {
                  (*map)->set(v->getValueAsCString(), v->getValueAsCString());
               }
            }
         }
         else if (value->isString())
         {
            *map = new StringMap();
            (*map)->set(value->getValueAsCString(), value->getValueAsCString());
         }
         else if (value->isNull())
         {
            rc = DCE_COLLECTION_ERROR;
         }
         else
         {
            *map = new StringMap();
         }
      }
      else
      {
         DbgPrintf(4, _T("DataCollectionTarget(%s)->getListFromScript(%s): Script execution error: %s"), m_name, param, vm->getErrorText());
         PostEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", name, vm->getErrorText(), m_id);
         rc = DCE_COLLECTION_ERROR;
      }
      delete vm;
   }
   else
   {
      DbgPrintf(4, _T("DataCollectionTarget(%s)->getListFromScript(%s): script \"%s\" not found"), m_name, param, name);
   }
   DbgPrintf(7, _T("DataCollectionTarget(%s)->getListFromScript(%s): rc=%d"), m_name, param, rc);
   return rc;
}

/**
 * Get last (current) DCI values for summary table.
 */
void DataCollectionTarget::getDciValuesSummary(SummaryTable *tableDefinition, Table *tableData, UINT32 userId)
{
   if (tableDefinition->isTableDciSource())
      getTableDciValuesSummary(tableDefinition, tableData, userId);
   else
      getItemDciValuesSummary(tableDefinition, tableData, userId);
}

/**
 * Get last (current) DCI values for summary table using single-value DCIs
 */
void DataCollectionTarget::getItemDciValuesSummary(SummaryTable *tableDefinition, Table *tableData, UINT32 userId)
{
   int offset = tableDefinition->isMultiInstance() ? 2 : 1;
   int baseRow = tableData->getNumRows();
   bool rowAdded = false;
   lockDciAccess(false);
   for(int i = 0; i < tableDefinition->getNumColumns(); i++)
   {
      SummaryTableColumn *tc = tableDefinition->getColumn(i);
      for(int j = 0; j < m_dcObjects->size(); j++)
	   {
		   DCObject *object = m_dcObjects->get(j);
         if ((object->getType() == DCO_TYPE_ITEM) && object->hasValue() &&
             (object->getStatus() == ITEM_STATUS_ACTIVE) &&
             ((tc->m_flags & COLUMN_DEFINITION_REGEXP_MATCH) ?
               RegexpMatch(object->getName(), tc->m_dciName, FALSE) :
               !_tcsicmp(object->getName(), tc->m_dciName)
             ) && object->hasAccess(userId))
         {
            int row;
            if (tableDefinition->isMultiInstance())
            {
               // Find instance
               const TCHAR *instance = ((DCItem *)object)->getInstance();
               for(row = baseRow; row < tableData->getNumRows(); row++)
               {
                  const TCHAR *v = tableData->getAsString(row, 1);
                  if (!_tcscmp(CHECK_NULL_EX(v), instance))
                     break;
               }
               if (row == tableData->getNumRows())
               {
                  tableData->addRow();
                  tableData->set(0, m_name);
                  tableData->set(1, instance);
                  tableData->setObjectId(m_id);
               }
            }
            else
            {
               if (!rowAdded)
               {
                  tableData->addRow();
                  tableData->set(0, m_name);
                  tableData->setObjectId(m_id);
                  rowAdded = true;
               }
               row = tableData->getNumRows() - 1;
            }
            tableData->setStatusAt(row, i + offset, ((DCItem *)object)->getThresholdSeverity());
            tableData->setCellObjectIdAt(row, i + offset, object->getId());
            tableData->getColumnDefinitions()->get(i + offset)->setDataType(((DCItem *)object)->getDataType());
            if (tableDefinition->getAggregationFunction() == F_LAST)
            {
               if (tc->m_flags & COLUMN_DEFINITION_MULTIVALUED)
               {
                  StringList *values = String(((DCItem *)object)->getLastValue()).split(tc->m_separator);
                  tableData->setAt(row, i + offset, values->get(0));
                  for(int r = 1; r < values->size(); r++)
                  {
                     if (row + r >= tableData->getNumRows())
                     {
                        tableData->addRow();
                        tableData->setObjectId(m_id);
                        tableData->setBaseRow(row);
                     }
                     tableData->setAt(row + r, i + offset, values->get(r));
                     tableData->setStatusAt(row + r, i + offset, ((DCItem *)object)->getThresholdSeverity());
                     tableData->setCellObjectIdAt(row + r, i + offset, object->getId());
                  }
               }
               else
               {
                  tableData->setAt(row, i + offset, ((DCItem *)object)->getLastValue());
               }
            }
            else
            {
               tableData->setAt(row, i + offset,
                  ((DCItem *)object)->getAggregateValue(
                     tableDefinition->getAggregationFunction(),
                     tableDefinition->getPeriodStart(),
                     tableDefinition->getPeriodEnd()));
            }

            if (!tableDefinition->isMultiInstance())
               break;
         }
      }
   }
   unlockDciAccess();
}

/**
 * Get last (current) DCI values for summary table using table DCIs
 */
void DataCollectionTarget::getTableDciValuesSummary(SummaryTable *tableDefinition, Table *tableData, UINT32 userId)
{
   lockDciAccess(false);
   for(int i = 0; i < m_dcObjects->size(); i++)
   {
      DCObject *o = m_dcObjects->get(i);
      if ((o->getType() == DCO_TYPE_TABLE) && o->hasValue() &&
           (o->getStatus() == ITEM_STATUS_ACTIVE) &&
           !_tcsicmp(o->getName(), tableDefinition->getTableDciName()) &&
           o->hasAccess(userId))
      {
         Table *lastValue = ((DCTable*)o)->getLastValue();
         if (lastValue == NULL)
            continue;

         for(int j = 0; j < lastValue->getNumRows(); j++)
         {
            tableData->addRow();
            tableData->setObjectId(m_id);
            tableData->set(0, m_name);
            for(int k = 0; k < lastValue->getNumColumns(); k++)
            {
               int columnIndex = tableData->getColumnIndex(lastValue->getColumnName(k));
               if (columnIndex == -1)
                  columnIndex = tableData->addColumn(lastValue->getColumnDefinition(k));
               tableData->set(columnIndex, lastValue->getAsString(j, k));
            }
         }
      }
   }
   unlockDciAccess();
}

/**
 * Must return true if object is a possible event source
 */
bool DataCollectionTarget::isEventSource()
{
   return true;
}

/**
 * Returns most critical status of DCI used for
 * status calculation
 */
int DataCollectionTarget::getMostCriticalDCIStatus()
{
   int status = -1;
   lockDciAccess(false);
   for(int i = 0; i < m_dcObjects->size(); i++)
	{
		DCObject *curr = m_dcObjects->get(i);
      if (curr->isStatusDCO() && (curr->getType() == DCO_TYPE_ITEM) &&
          curr->hasValue() && (curr->getStatus() == ITEM_STATUS_ACTIVE))
      {
         if (getObjectClass() == OBJECT_CLUSTER && !curr->isAggregateOnCluster())
            continue; // Calculated only on those that are aggregated on cluster

         ItemValue *value = static_cast<DCItem*>(curr)->getInternalLastValue();
         if (value != NULL && (INT32)*value >= STATUS_NORMAL && (INT32)*value <= STATUS_CRITICAL)
            status = std::max(status, (INT32)*value);
         delete value;
      }
	}
   unlockDciAccess();
   return (status == -1) ? STATUS_UNKNOWN : status;
}

/**
 * Calculate compound status
 */
void DataCollectionTarget::calculateCompoundStatus(BOOL bForcedRecalc)
{
   NetObj::calculateCompoundStatus(bForcedRecalc);
}

/**
 * Returns last ping time
 */
UINT32 DataCollectionTarget::getPingTime()
{
   if ((time(NULL) - m_pingLastTimeStamp) > g_dwStatusPollingInterval)
   {
      updatePingData();
      DbgPrintf(7, _T("DataCollectionTarget::getPingTime: update ping time is required! Last ping time %d."), m_pingLastTimeStamp);
   }
   return m_pingTime;
}

/**
 * Update ping data
 */
void DataCollectionTarget::updatePingData()
{
   m_pingLastTimeStamp = 0;
   m_pingTime = PING_TIME_TIMEOUT;
}

/**
 * Enter maintenance mode
 */
void DataCollectionTarget::enterMaintenanceMode()
{
   DbgPrintf(4, _T("Entering maintenance mode for %s [%d]"), m_name, m_id);
   UINT64 eventId = PostEvent2(EVENT_MAINTENANCE_MODE_ENTERED, m_id, NULL);

   lockDciAccess(false);
   for(int i = 0; i < m_dcObjects->size(); i++)
   {
      DCObject *dco = m_dcObjects->get(i);
      if (dco->getStatus() == ITEM_STATUS_DISABLED)
         continue;

      dco->updateThresholdsBeforeMaintenanceState();
   }
   unlockDciAccess();

   lockProperties();
   m_maintenanceEventId = eventId;
   m_stateBeforeMaintenance = m_state;
   setModified(MODIFY_COMMON_PROPERTIES | MODIFY_DATA_COLLECTION);
   unlockProperties();
}

/**
 * Leave maintenance mode
 */
void DataCollectionTarget::leaveMaintenanceMode()
{
   DbgPrintf(4, _T("Leaving maintenance mode for %s [%d]"), m_name, m_id);
   PostEvent(EVENT_MAINTENANCE_MODE_LEFT, m_id, NULL);

   lockDciAccess(false);
   for(int i = 0; i < m_dcObjects->size(); i++)
   {
      DCObject *dco = m_dcObjects->get(i);
      if (dco->getStatus() == ITEM_STATUS_DISABLED)
      {
         continue;
      }

      dco->generateEventsBasedOnThrDiff();
   }
   unlockDciAccess();

   lockProperties();
   m_maintenanceEventId = 0;
   bool forcePoll = m_state != m_stateBeforeMaintenance;
   m_state = m_stateBeforeMaintenance;
   setModified(MODIFY_COMMON_PROPERTIES);
   unlockProperties();

   if(forcePoll)
      statusPollWorkerEntry(RegisterPoller(POLLER_TYPE_STATUS, this), NULL, 0);
}

/**
 * Update cache size for given data collection item
 */
void DataCollectionTarget::updateDCItemCacheSize(UINT32 dciId, UINT32 conditionId)
{
   lockDciAccess(false);
   DCObject *dci = getDCObjectById(dciId, 0, false);
   if ((dci != NULL) && (dci->getType() == DCO_TYPE_ITEM))
   {
      static_cast<DCItem*>(dci)->updateCacheSize(conditionId);
   }
   unlockDciAccess();
}

/**
 * Reload DCI cache
 */
void DataCollectionTarget::reloadDCItemCache(UINT32 dciId)
{
   lockDciAccess(false);
   DCObject *dci = getDCObjectById(dciId, 0, false);
   if ((dci != NULL) && (dci->getType() == DCO_TYPE_ITEM))
   {
      nxlog_debug_tag(_T("obj.dc.cache"), 6, _T("Reload DCI cache for \"%s\" [%d] on %s [%d]"),
               dci->getName(), dci->getId(), m_name, m_id);
      static_cast<DCItem*>(dci)->reloadCache();
   }
   unlockDciAccess();
}

/**
 * Returns true if object is data collection target
 */
bool DataCollectionTarget::isDataCollectionTarget()
{
   return true;
}

/**
 * Add data collection element to proxy info structure
 */
void DataCollectionTarget::addProxyDataCollectionElement(ProxyInfo *info, const DCObject *dco)
{
   info->msg->setField(info->fieldId++, dco->getId());
   info->msg->setField(info->fieldId++, (INT16)dco->getType());
   info->msg->setField(info->fieldId++, (INT16)dco->getDataSource());
   info->msg->setField(info->fieldId++, dco->getName());
   info->msg->setField(info->fieldId++, (INT32)dco->getEffectivePollingInterval());
   info->msg->setFieldFromTime(info->fieldId++, dco->getLastPollTime());
   info->msg->setField(info->fieldId++, m_guid);
   info->msg->setField(info->fieldId++, dco->getSnmpPort());
   if (dco->getType() == DCO_TYPE_ITEM)
      info->msg->setField(info->fieldId++, ((DCItem *)dco)->getSnmpRawValueType());
   else
      info->msg->setField(info->fieldId++, (INT16)0);
   info->fieldId += 1;
   info->count++;
}

/**
 * Add SNMP target to proxy info structure
 */
void DataCollectionTarget::addProxySnmpTarget(ProxyInfo *info, const Node *node)
{
   info->msg->setField(info->nodeInfoFieldId++, m_guid);
   info->msg->setField(info->nodeInfoFieldId++, node->getIpAddress());
   info->msg->setField(info->nodeInfoFieldId++, node->getSNMPVersion());
   info->msg->setField(info->nodeInfoFieldId++, node->getSNMPPort());
   SNMP_SecurityContext *snmpSecurity = node->getSnmpSecurityContext();
   info->msg->setField(info->nodeInfoFieldId++, (INT16)snmpSecurity->getAuthMethod());
   info->msg->setField(info->nodeInfoFieldId++, (INT16)snmpSecurity->getPrivMethod());
   info->msg->setFieldFromMBString(info->nodeInfoFieldId++, snmpSecurity->getUser());
   info->msg->setFieldFromMBString(info->nodeInfoFieldId++, snmpSecurity->getAuthPassword());
   info->msg->setFieldFromMBString(info->nodeInfoFieldId++, snmpSecurity->getPrivPassword());
   delete snmpSecurity;
   info->nodeInfoFieldId += 41;
   info->nodeInfoCount++;
}

/**
 * Collect info for SNMP proxy and DCI source (proxy) nodes
 * Default implementation adds only agent based DCIs with source node set to requesting node
 */
void DataCollectionTarget::collectProxyInfo(ProxyInfo *info)
{
   lockDciAccess(false);
   for(int i = 0; i < m_dcObjects->size(); i++)
   {
      DCObject *dco = m_dcObjects->get(i);
      if (dco->getStatus() == ITEM_STATUS_DISABLED)
         continue;

      if ((dco->getDataSource() == DS_NATIVE_AGENT) && (dco->getSourceNode() == info->proxyId) &&
          dco->hasValue() && (dco->getAgentCacheMode() == AGENT_CACHE_ON))
      {
         addProxyDataCollectionElement(info, dco);
      }
   }
   unlockDciAccess();
}

/**
 * Callback for colecting proxied SNMP DCIs
 */
void DataCollectionTarget::collectProxyInfoCallback(NetObj *object, void *data)
{
   ((DataCollectionTarget *)object)->collectProxyInfo((ProxyInfo *)data);
}

/**
 * Get effective source node for given data collection object
 */
UINT32 DataCollectionTarget::getEffectiveSourceNode(DCObject *dco)
{
   return dco->getSourceNode();
}

/**
 * Filter for selecting templates from objects
 */
static bool TemplateSelectionFilter(NetObj *object, void *userData)
{
   return (object->getObjectClass() == OBJECT_TEMPLATE) && !object->isDeleted() && static_cast<Template*>(object)->isAutoBindEnabled();
}

/**
 * Apply user templates
 */
void DataCollectionTarget::applyUserTemplates()
{
   if (IsShutdownInProgress())
      return;

   ObjectArray<NetObj> *templates = g_idxObjectById.getObjects(true, TemplateSelectionFilter);
   for(int i = 0; i < templates->size(); i++)
   {
      Template *pTemplate = (Template *)templates->get(i);
      AutoBindDecision decision = pTemplate->isApplicable(this);
      if (decision == AutoBindDecision_Bind)
      {
         if (!pTemplate->isDirectChild(m_id))
         {
            DbgPrintf(4, _T("DataCollectionTarget::applyUserTemplates(): applying template %d \"%s\" to object %d \"%s\""),
                      pTemplate->getId(), pTemplate->getName(), m_id, m_name);
            pTemplate->applyToTarget(this);
            PostEvent(EVENT_TEMPLATE_AUTOAPPLY, g_dwMgmtNode, "isis", m_id, m_name, pTemplate->getId(), pTemplate->getName());
         }
      }
      else if (decision == AutoBindDecision_Unbind)
      {
         if (pTemplate->isAutoUnbindEnabled() && pTemplate->isDirectChild(m_id))
         {
            DbgPrintf(4, _T("DataCollectionTarget::applyUserTemplates(): removing template %d \"%s\" from object %d \"%s\""),
                      pTemplate->getId(), pTemplate->getName(), m_id, m_name);
            pTemplate->deleteChild(this);
            deleteParent(pTemplate);
            pTemplate->queueRemoveFromTarget(m_id, true);
            PostEvent(EVENT_TEMPLATE_AUTOREMOVE, g_dwMgmtNode, "isis", m_id, m_name, pTemplate->getId(), pTemplate->getName());
         }
      }
      pTemplate->decRefCount();
   }
   delete templates;
}

/**
 * Filter for selecting containers from objects
 */
static bool ContainerSelectionFilter(NetObj *object, void *userData)
{
   return (object->getObjectClass() == OBJECT_CONTAINER) && !object->isDeleted() && ((Container *)object)->isAutoBindEnabled();
}

/**
 * Update container membership
 */
void DataCollectionTarget::updateContainerMembership()
{
   if (IsShutdownInProgress())
      return;

   ObjectArray<NetObj> *containers = g_idxObjectById.getObjects(true, ContainerSelectionFilter);
   for(int i = 0; i < containers->size(); i++)
   {
      Container *pContainer = (Container *)containers->get(i);
      AutoBindDecision decision = pContainer->isApplicable(this);
      if (decision == AutoBindDecision_Bind)
      {
         if (!pContainer->isDirectChild(m_id))
         {
            DbgPrintf(4, _T("DataCollectionTarget::updateContainerMembership(): binding object %d \"%s\" to container %d \"%s\""),
                      m_id, m_name, pContainer->getId(), pContainer->getName());
            pContainer->addChild(this);
            addParent(pContainer);
            PostEvent(EVENT_CONTAINER_AUTOBIND, g_dwMgmtNode, "isis", m_id, m_name, pContainer->getId(), pContainer->getName());
            pContainer->calculateCompoundStatus();
         }
      }
      else if (decision == AutoBindDecision_Unbind)
      {
         if (pContainer->isAutoUnbindEnabled() && pContainer->isDirectChild(m_id))
         {
            DbgPrintf(4, _T("DataCollectionTarget::updateContainerMembership(): removing object %d \"%s\" from container %d \"%s\""),
                      m_id, m_name, pContainer->getId(), pContainer->getName());
            pContainer->deleteChild(this);
            deleteParent(pContainer);
            PostEvent(EVENT_CONTAINER_AUTOUNBIND, g_dwMgmtNode, "isis", m_id, m_name, pContainer->getId(), pContainer->getName());
            pContainer->calculateCompoundStatus();
         }
      }
      pContainer->decRefCount();
   }
   delete containers;
}

/**
 * Serialize object to JSON
 */
json_t *DataCollectionTarget::toJson()
{
   json_t *root = super::toJson();
   json_object_set_new(root, "pingTime", json_integer(m_pingTime));
   json_object_set_new(root, "pingLastTimeStamp", json_integer(m_pingLastTimeStamp));
   return root;
}

/**
 * Entry point for status poll worker thread
 */
void DataCollectionTarget::statusPollWorkerEntry(PollerInfo *poller)
{
   statusPollWorkerEntry(poller, NULL, 0);
}

/**
 * Entry point for status poll worker thread
 */
void DataCollectionTarget::statusPollWorkerEntry(PollerInfo *poller, ClientSession *session, UINT32 rqId)
{
   poller->startExecution();
   statusPoll(poller, session, rqId);
   delete poller;
}

/**
 * Entry point for second level status poll (called by parent object)
 */
void DataCollectionTarget::statusPollPollerEntry(PollerInfo *poller, ClientSession *session, UINT32 rqId)
{
   poller->setStatus(_T("child poll"));
   statusPoll(poller, session, rqId);
}

/**
 * Perform status poll on this data collection target. Default implementation do nothing.
 */
void DataCollectionTarget::statusPoll(PollerInfo *poller, ClientSession *session, UINT32 rqId)
{
}

/**
 * Entry point for configuration poll worker thread
 */
void DataCollectionTarget::configurationPollWorkerEntry(PollerInfo *poller)
{
   configurationPollWorkerEntry(poller, NULL, 0);
}

/**
 * Entry point for configuration poll worker thread
 */
void DataCollectionTarget::configurationPollWorkerEntry(PollerInfo *poller, ClientSession *session, UINT32 rqId)
{
   poller->startExecution();
   ObjectTransactionStart();
   configurationPoll(poller, session, rqId);
   ObjectTransactionEnd();
   delete poller;
}

/**
 * Perform configuration poll on this data collection target. Default implementation do nothing.
 */
void DataCollectionTarget::configurationPoll(PollerInfo *poller, ClientSession *session, UINT32 rqId)
{
}

/**
 * Entry point for instance discovery poll worker thread
 */
void DataCollectionTarget::instanceDiscoveryPollWorkerEntry(PollerInfo *poller)
{
   instanceDiscoveryPollWorkerEntry(poller, NULL, 0);
}

/**
 * Entry point for instance discovery poll worker thread
 */
void DataCollectionTarget::instanceDiscoveryPollWorkerEntry(PollerInfo *poller, ClientSession *session, UINT32 requestId)
{
   poller->startExecution();
   ObjectTransactionStart();
   instanceDiscoveryPoll(poller, session, requestId);
   ObjectTransactionEnd();
   delete poller;
}

/**
 * Perform instance discovery poll on data collection target
 */
void DataCollectionTarget::instanceDiscoveryPoll(PollerInfo *poller, ClientSession *session, UINT32 requestId)
{
   if (m_runtimeFlags & DCDF_DELETE_IN_PROGRESS)
   {
      if (requestId == 0)
         m_runtimeFlags &= ~DCDF_QUEUED_FOR_INSTANCE_POLL;
      return;
   }

   if (IsShutdownInProgress())
      return;

   poller->setStatus(_T("wait for lock"));
   pollerLock();

   if (IsShutdownInProgress())
   {
      pollerUnlock();
      return;
   }

   m_pollRequestor = session;
   sendPollerMsg(requestId, _T("Starting instance discovery poll for %s %s\r\n"), getObjectClassName(), m_name);
   DbgPrintf(4, _T("Starting instance discovery poll for %s %s (ID: %d)"), getObjectClassName(), m_name, m_id);

   // Check if DataCollectionTarget is marked as unreachable
   if (!(m_state & DCSF_UNREACHABLE))
   {
      poller->setStatus(_T("instance discovery"));
      doInstanceDiscovery(requestId);

      // Execute hook script
      poller->setStatus(_T("hook"));
      executeHookScript(_T("InstancePoll"));
   }
   else
   {
      sendPollerMsg(requestId, POLLER_WARNING _T("%s is marked as unreachable, instance discovery poll aborted\r\n"), getObjectClassName());
      DbgPrintf(4, _T("%s is marked as unreachable, instance discovery poll aborted"), getObjectClassName());
   }

   m_lastInstancePoll = time(NULL);

   // Finish instance discovery poll
   poller->setStatus(_T("cleanup"));
   if (requestId == 0)
      m_runtimeFlags &= ~DCDF_QUEUED_FOR_INSTANCE_POLL;
   pollerUnlock();
   DbgPrintf(4, _T("Finished instance discovery poll for %s %s (ID: %d)"), getObjectClassName(), m_name, m_id);
}

/**
 * Get list of instances for given data collection object. Default implementation always returns NULL.
 */
StringMap *DataCollectionTarget::getInstanceList(DCObject *dco)
{
   return NULL;
}

/**
 * Do instance discovery
 */
void DataCollectionTarget::doInstanceDiscovery(UINT32 requestId)
{
   sendPollerMsg(requestId, _T("Running DCI instance discovery\r\n"));

   // collect instance discovery DCIs
   ObjectArray<DCObject> rootObjects;
   lockDciAccess(false);
   for(int i = 0; i < m_dcObjects->size(); i++)
   {
      DCObject *object = m_dcObjects->get(i);
      if (object->getInstanceDiscoveryMethod() != IDM_NONE)
      {
         object->setBusyFlag();
         rootObjects.add(object);
      }
   }
   unlockDciAccess();

   // process instance discovery DCIs
   // it should be done that way to prevent DCI list lock for long time
   bool changed = false;
   for(int i = 0; i < rootObjects.size(); i++)
   {
      DCObject *object = rootObjects.get(i);
      DbgPrintf(5, _T("DataCollectionTarget::doInstanceDiscovery(%s [%u]): Updating instances for instance discovery DCO %s [%d]"),
                m_name, m_id, object->getName(), object->getId());
      sendPollerMsg(requestId, _T("   Updating instances for %s [%d]\r\n"), object->getName(), object->getId());
      StringMap *instances = getInstanceList(object);
      if (instances != NULL)
      {
         DbgPrintf(5, _T("DataCollectionTarget::doInstanceDiscovery(%s [%u]): read %d values"), m_name, m_id, instances->size());
         object->filterInstanceList(instances);
         if (updateInstances(object, instances, requestId))
            changed = true;
         delete instances;
      }
      else
      {
         DbgPrintf(5, _T("DataCollectionTarget::doInstanceDiscovery(%s [%u]): failed to get instance list for DCO %s [%d]"),
                   m_name, m_id, object->getName(), object->getId());
         sendPollerMsg(requestId, POLLER_ERROR _T("      Failed to get instance list\r\n"));
      }
      object->clearBusyFlag();
   }

   if (changed)
   {
      onDataCollectionChange();

      lockProperties();
      setModified(MODIFY_DATA_COLLECTION);
      unlockProperties();
   }
}

/**
 * Callback for finding instance
 */
static EnumerationCallbackResult FindInstanceCallback(const TCHAR *key, const void *value, void *data)
{
   return !_tcscmp((const TCHAR *)data, key) ? _STOP : _CONTINUE;
}

/**
 * Data for CreateInstanceDCI
 */
struct CreateInstanceDCOData
{
   DCObject *root;
   DataCollectionTarget *object;
   UINT32 requestId;
};

/**
 * Callback for creating instance DCIs
 */
static EnumerationCallbackResult CreateInstanceDCI(const TCHAR *key, const void *value, void *data)
{
   DataCollectionTarget *object = ((CreateInstanceDCOData *)data)->object;
   DCObject *root = ((CreateInstanceDCOData *)data)->root;

   DbgPrintf(5, _T("DataCollectionTarget::updateInstances(%s [%u], %s [%u]): creating new DCO for instance \"%s\""),
             object->getName(), object->getId(), root->getName(), root->getId(), key);
   object->sendPollerMsg(((CreateInstanceDCOData *)data)->requestId, _T("      Creating new DCO for instance \"%s\"\r\n"), key);

   DCObject *dco = root->clone();

   dco->setTemplateId(object->getId(), root->getId());
   dco->setInstance((const TCHAR *)value);
   dco->setInstanceDiscoveryMethod(IDM_NONE);
   dco->setInstanceDiscoveryData(key);
   dco->setInstanceFilter(NULL);
   dco->expandInstance();
   dco->changeBinding(CreateUniqueId(IDG_ITEM), object, FALSE);
   object->addDCObject(dco, true);
   return _CONTINUE;
}

/**
 * Update instance DCIs created from instance discovery DCI
 */
bool DataCollectionTarget::updateInstances(DCObject *root, StringMap *instances, UINT32 requestId)
{
   bool changed = false;

   lockDciAccess(true);

   // Delete DCIs for missing instances and update existing
   IntegerArray<UINT32> deleteList;
   for(int i = 0; i < m_dcObjects->size(); i++)
   {
      DCObject *object = m_dcObjects->get(i);
      if ((object->getTemplateId() != m_id) || (object->getTemplateItemId() != root->getId()))
         continue;

      const TCHAR *dcoInstance = object->getInstanceDiscoveryData();
      if (instances->forEach(FindInstanceCallback, (void *)dcoInstance) == _STOP)
      {
         // found, remove value from instances
         nxlog_debug(5, _T("DataCollectionTarget::updateInstances(%s [%u], %s [%u]): instance \"%s\" found"),
                   m_name, m_id, root->getName(), root->getId(), dcoInstance);
         const TCHAR *name = instances->get(dcoInstance);
         if (_tcscmp(name, object->getInstance()))
         {
            object->setInstance(name);
            object->updateFromTemplate(root);
            changed = true;
         }
         if (object->getInstanceGracePeriodStart() > 0)
         {
            object->setInstanceGracePeriodStart(0);
            object->setStatus(ITEM_STATUS_ACTIVE, false);
         }
         instances->remove(dcoInstance);
      }
      else
      {
         time_t retentionTime = ((object->getInstanceRetentionTime() != -1) ? object->getInstanceRetentionTime() : g_instanceRetentionTime) * 86400;

         if ((object->getInstanceGracePeriodStart() == 0) && (retentionTime > 0))
         {
            object->setInstanceGracePeriodStart(time(NULL));
            object->setStatus(ITEM_STATUS_DISABLED, false);
            nxlog_debug(5, _T("DataCollectionTarget::updateInstances(%s [%u], %s [%u]): instance \"%s\" not found, grace period started"),
                      m_name, m_id, root->getName(), root->getId(), dcoInstance);
            sendPollerMsg(requestId, _T("      Existing instance \"%s\" not found, grace period started\r\n"), dcoInstance);
            changed = true;
         }

         if ((retentionTime == 0) || ((time(NULL) - object->getInstanceGracePeriodStart()) > retentionTime))
         {
            // not found, delete DCO
            nxlog_debug(5, _T("DataCollectionTarget::updateInstances(%s [%u], %s [%u]): instance \"%s\" not found, instance DCO will be deleted"),
                      m_name, m_id, root->getName(), root->getId(), dcoInstance);
            sendPollerMsg(requestId, _T("      Existing instance \"%s\" not found and will be deleted\r\n"), dcoInstance);
            deleteList.add(object->getId());
            changed = true;
         }
      }
   }

   for(int i = 0; i < deleteList.size(); i++)
      deleteDCObject(deleteList.get(i), false, 0);

   // Create new instances
   if (instances->size() > 0)
   {
      CreateInstanceDCOData data;
      data.root = root;
      data.object = this;
      data.requestId = requestId;
      instances->forEach(CreateInstanceDCI, &data);
      changed = true;
   }

   unlockDciAccess();
   return changed;
}

/**
 * Get last (current) DCI values.
 */
UINT32 DataCollectionTarget::getLastValues(NXCPMessage *msg, bool objectTooltipOnly, bool overviewOnly, bool includeNoValueObjects, UINT32 userId)
{
   lockDciAccess(false);

   UINT32 dwId = VID_DCI_VALUES_BASE, dwCount = 0;
   for(int i = 0; i < m_dcObjects->size(); i++)
   {
      DCObject *object = m_dcObjects->get(i);
      if ((object->hasValue() || includeNoValueObjects) &&
          (!objectTooltipOnly || object->isShowOnObjectTooltip()) &&
          (!overviewOnly || object->isShowInObjectOverview()) &&
          object->hasAccess(userId))
      {
         if (object->getType() == DCO_TYPE_ITEM)
         {
            ((DCItem *)object)->fillLastValueMessage(msg, dwId);
            dwId += 50;
            dwCount++;
         }
         else if (object->getType() == DCO_TYPE_TABLE)
         {
            ((DCTable *)object)->fillLastValueSummaryMessage(msg, dwId);
            dwId += 50;
            dwCount++;
         }
      }
   }
   msg->setField(VID_NUM_ITEMS, dwCount);

   unlockDciAccess();
   return RCC_SUCCESS;
}
