/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Victor Kirhenshtein
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
** File: dcowner.cpp
**
**/

#include "nxcore.h"
#include <netxms-regex.h>

/**
 * Data collection owner object constructor
 */
DataCollectionOwner::DataCollectionOwner() : super(), m_dcObjects(0, 128)
{
   m_status = STATUS_NORMAL;
   m_dciListModified = false;
   m_instanceDiscoveryChanges = false;
}

/**
 * Constructor for new data collection owner object
 */
DataCollectionOwner::DataCollectionOwner(const TCHAR *name, const uuid& guid) : super(), m_dcObjects(0, 128)
{
   _tcslcpy(m_name, name, MAX_OBJECT_NAME);
   m_status = STATUS_NORMAL;
   m_isHidden = true;
   m_dciListModified = false;
   m_instanceDiscoveryChanges = false;
   setCreationTime();

   if (!guid.isNull())
      m_guid = guid;
}

/**
 * Destructor
 */
DataCollectionOwner::~DataCollectionOwner()
{
}

/**
 * Destroy all related data collection items.
 */
void DataCollectionOwner::destroyItems()
{
	m_dcObjects.clear();
}

/**
 * Create data collection owner object from database data
 *
 * @param dwId object ID
 */
bool DataCollectionOwner::loadFromDatabase(DB_HANDLE hdb, uint32_t id, DB_STATEMENT *preparedStatements)
{
   bool success = true;

   m_id = id;

   if (!loadCommonProperties(hdb, preparedStatements))
      return false;

   // Load DCI and access list
   loadACLFromDB(hdb, preparedStatements);
   loadItemsFromDB(hdb, preparedStatements);
   for(int i = 0; i < m_dcObjects.size(); i++)
      if (!m_dcObjects.get(i)->loadThresholdsFromDB(hdb, preparedStatements))
         success = false;

	m_status = STATUS_NORMAL;

   return success;
}

/**
 * Save object to database
 */
bool DataCollectionOwner::saveToDatabase(DB_HANDLE hdb)
{
   bool success = super::saveToDatabase(hdb);

   // Save data collection items
	if (success && (m_modified & MODIFY_DATA_COLLECTION))
	{
      readLockDciAccess();
      for(int i = 0; success && (i < m_dcObjects.size()); i++)
         success = m_dcObjects.get(i)->saveToDatabase(hdb);
      unlockDciAccess();
	}

   return success;
}

/**
 * Process list of deleted items
 */
static bool ProcessDeletedItems(DB_HANDLE hdb, const String& list)
{
   StringBuffer query = _T("DELETE FROM thresholds WHERE item_id IN (");
   query.append(list);
   query.append(_T(')'));
   return DBQuery(hdb, query);
}

/**
 * Process list of deleted tables
 */
static bool ProcessDeletedTables(DB_HANDLE hdb, const String& list)
{
   StringBuffer query = _T("DELETE FROM dc_table_columns WHERE table_id IN (");
   query.append(list);
   query.append(_T(')'));
   if (!DBQuery(hdb, query))
      return false;

   query = _T("DELETE FROM dct_thresholds WHERE table_id IN (");
   query.append(list);
   query.append(_T(')'));
   return DBQuery(hdb, query);
}

/**
 * Process list of deleted table thresholds
 */
static bool ProcessDeletedTableThresholds(DB_HANDLE hdb, const String& list)
{
   StringBuffer query = _T("DELETE FROM dct_threshold_conditions WHERE threshold_id IN (");
   query.append(list);
   query.append(_T(')'));
   if (!DBQuery(hdb, query))
      return false;

   query = _T("DELETE FROM dct_threshold_instances WHERE threshold_id IN (");
   query.append(list);
   query.append(_T(')'));
   return DBQuery(hdb, query);
}

/**
 * Process list of deleted data collection objects
 */
static bool ProcessDeletedDCObjects(DB_HANDLE hdb, const String& list)
{
   StringBuffer query = _T("DELETE FROM dci_schedules WHERE item_id IN (");
   query.append(list);
   query.append(_T(')'));
   if (!DBQuery(hdb, query))
      return false;

   query = _T("DELETE FROM dci_access WHERE dci_id IN (");
   query.append(list);
   query.append(_T(')'));
   return DBQuery(hdb, query);
}

/**
 * Delete object from database
 */
bool DataCollectionOwner::deleteFromDatabase(DB_HANDLE hdb)
{
   bool success = super::deleteFromDatabase(hdb);
   if (!success)
      return false;

   // Delete DCI configuration
   if (success)
   {
      StringBuffer listItems, listTables, listTableThresholds, listAll;
      int countItems = 0, countTables = 0, countTableThresholds = 0, countAll = 0;
      for(int i = 0; (i < m_dcObjects.size()) && success; i++)
      {
         DCObject *o = m_dcObjects.get(i);
         if (!listAll.isEmpty())
            listAll.append(_T(','));
         listAll.append(o->getId());
         countAll++;

         if (o->getType() == DCO_TYPE_ITEM)
         {
            if (!listItems.isEmpty())
               listItems.append(_T(','));
            listItems.append(o->getId());
            QueueRawDciDataDelete(o->getId());
            countItems++;
         }
         else if (o->getType() == DCO_TYPE_TABLE)
         {
            if (!listTables.isEmpty())
               listTables.append(_T(','));
            listTables.append(o->getId());
            countTables++;

            IntegerArray<uint32_t> idList;
            static_cast<DCTable*>(o)->getThresholdIdList(&idList);
            for(int j = 0; j < idList.size(); j++)
            {
               if (!listTableThresholds.isEmpty())
                  listTableThresholds.append(_T(','));
               listTableThresholds.append(idList.get(j));
               countTableThresholds++;
            }
         }

         if (countItems >= 500)
         {
            success = ProcessDeletedItems(hdb, listItems);
            listItems.clear();
            countItems = 0;
         }

         if ((countTables >= 500) && success)
         {
            success = ProcessDeletedTables(hdb, listTables);
            listTables.clear();
            countTables = 0;
         }

         if ((countTableThresholds >= 500) && success)
         {
            success = ProcessDeletedTableThresholds(hdb, listTableThresholds);
            listTableThresholds.clear();
            countTableThresholds = 0;
         }

         if ((countAll >= 500) && success)
         {
            success = ProcessDeletedDCObjects(hdb, listAll);
            listAll.clear();
            countAll = 0;
         }
      }

      if ((countItems > 0) && success)
         success = ProcessDeletedItems(hdb, listItems);

      if ((countTables > 0) && success)
         success = ProcessDeletedTables(hdb, listTables);

      if ((countTableThresholds > 0) && success)
         success = ProcessDeletedTableThresholds(hdb, listTableThresholds);

      if ((countAll > 0) && success)
         success = ProcessDeletedDCObjects(hdb, listAll);
   }

   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM items WHERE node_id=?"));
   if (success)
      success = executeQueryOnObject(hdb, _T("UPDATE items SET template_id=0,template_item_id=0 WHERE template_id=?"));
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM dc_tables WHERE node_id=?"));
   if (success)
      success = executeQueryOnObject(hdb, _T("UPDATE dc_tables SET template_id=0,template_item_id=0 WHERE template_id=?"));
   return success;
}

/**
 * Load data collection items from database
 */
void DataCollectionOwner::loadItemsFromDB(DB_HANDLE hdb, DB_STATEMENT *preparedStatements)
{
   bool useStartupDelay = ConfigReadBoolean(_T("DataCollection.StartupDelay"), false);

	DB_STATEMENT hStmt = PrepareObjectLoadStatement(hdb, preparedStatements, LSI_DC_ITEMS,
        L"SELECT item_id,name,source,datatype,polling_interval,retention_time,"
        L"status,delta_calculation,transformation,template_id,description,"
        L"instance,template_item_id,flags,resource_id,proxy_node,multiplier,units_name,"
        L"perftab_settings,system_tag,snmp_port,snmp_raw_value_type,instd_method,instd_data,"
        L"instd_filter,samples,comments,guid,npe_name,instance_retention_time,"
        L"grace_period_start,related_object,polling_schedule_type,retention_type,"
        L"polling_interval_src,retention_time_src,snmp_version,state_flags,all_rearmed_event,"
        L"transformed_datatype,user_tag,thresholds_disable_end_time "
        L"FROM items WHERE node_id=?");
	if (hStmt != nullptr)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
		DB_RESULT hResult = DBSelectPrepared(hStmt);
		if (hResult != nullptr)
		{
			int count = DBGetNumRows(hResult);
			for(int i = 0; i < count; i++)
				m_dcObjects.add(make_shared<DCItem>(hdb, preparedStatements, hResult, i, self(), useStartupDelay));
			DBFreeResult(hResult);
		}
	}

	hStmt = PrepareObjectLoadStatement(hdb, preparedStatements, LSI_DC_TABLES,
        L"SELECT item_id,template_id,template_item_id,name,"
        L"description,flags,source,snmp_port,polling_interval,retention_time,"
        L"status,system_tag,resource_id,proxy_node,perftab_settings,"
        L"transformation_script,comments,guid,instd_method,instd_data,"
        L"instd_filter,instance,instance_retention_time,grace_period_start,"
        L"related_object,polling_schedule_type,retention_type,polling_interval_src,"
        L"retention_time_src,snmp_version,state_flags,user_tag,thresholds_disable_end_time "
        L"FROM dc_tables WHERE node_id=?");
	if (hStmt != nullptr)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
		DB_RESULT hResult = DBSelectPrepared(hStmt);
		if (hResult != nullptr)
		{
			int count = DBGetNumRows(hResult);
			for(int i = 0; i < count; i++)
				m_dcObjects.add(make_shared<DCTable>(hdb, preparedStatements, hResult, i, self(), useStartupDelay));
			DBFreeResult(hResult);
		}
	}

   onDataCollectionLoad();
}

/**
 * Add data collection object to node
 */
bool DataCollectionOwner::addDCObject(DCObject *object, bool alreadyLocked, bool notify)
{
   bool success = false;

   if (!alreadyLocked)
      writeLockDciAccess();

   // Check if that object exists
   int i;
   for(i = 0; i < m_dcObjects.size(); i++)
      if (m_dcObjects.get(i)->getId() == object->getId())
         break;   // Object with specified id already exist

   if (i == m_dcObjects.size())     // Add new item
   {
		m_dcObjects.add(object);
      object->setLastPollTime(Timestamp::fromMilliseconds(0));    // Cause item to be polled immediately
      if (object->getStatus() != ITEM_STATUS_DISABLED)
         object->setStatus(ITEM_STATUS_ACTIVE, false);
      object->clearBusyFlag();
      if (object->getInstanceDiscoveryMethod() != IDM_NONE)
         m_instanceDiscoveryChanges = true;
      success = true;
   }

   if (!alreadyLocked)
      unlockDciAccess();

	if (success)
	{
      setModified(MODIFY_DATA_COLLECTION);
		if (notify)
		   NotifyClientsOnDCIUpdate(*this, object);
	}
   return success;
}

/**
 * Delete data collection object from node
 */
bool DataCollectionOwner::deleteDCObject(uint32_t dcObjectId, bool needLock, uint32_t userId, uint32_t *rcc, json_t **json)
{
   bool success = false;

   if (rcc != nullptr)
      *rcc = RCC_INVALID_DCI_ID;

	if (needLock)
		writeLockDciAccess();  // write lock

   // Check if that item exists
   for(int i = 0; i < m_dcObjects.size(); i++)
	{
		DCObject *object = m_dcObjects.get(i);
      if (object->getId() == dcObjectId)
      {
         if (object->hasAccess(userId))
         {
            shared_ptr<DCObject> ref = m_dcObjects.getShared(i);  // Prevent destruction by call to remove
            m_dcObjects.remove(i);

            // Check if it is instance DCI
            if (object->getInstanceDiscoveryMethod() != IDM_NONE)
            {
               deleteChildDCIs(dcObjectId);
            }

            if (json != nullptr)
               *json = object->toJson();

            // Destroy item
            nxlog_debug_tag(_T("obj.dc"), 7, _T("DataCollectionOwner::DeleteDCObject: deleting DCObject [%u] from object %s [%u]"), dcObjectId, m_name, m_id);
            deleteDCObject(object);
            success = true;
            NotifyClientsOnDCIDelete(*this, dcObjectId);
            nxlog_debug_tag(_T("obj.dc"), 7, _T("DataCollectionOwner::DeleteDCObject: DCObject deleted from object %s [%u]"), m_name, m_id);
         }
         else
         {
            nxlog_debug_tag(_T("obj.dc"), 6, _T("DataCollectionOwner::DeleteDCObject: denied access to DCObject %u for user %u"), dcObjectId, userId);
            if (rcc != nullptr)
               *rcc = RCC_ACCESS_DENIED;
         }
         break;
      }
	}

	if (needLock)
	   unlockDciAccess();

	if (success)
	{
	   lockProperties();
	   setModified(MODIFY_DATA_COLLECTION, false);
	   unlockProperties();
      if (rcc != nullptr)
         *rcc = RCC_SUCCESS;
	}
   return success;
}

/**
 * Deletes child DCI objects of instance discovery DCI.
 * It is assumed that list is already locked
 */
void DataCollectionOwner::deleteChildDCIs(uint32_t dcObjectId)
{
   for(int i = 0; i < m_dcObjects.size(); i++)
   {
      DCObject *subObject = m_dcObjects.get(i);
      if (subObject->getTemplateItemId() == dcObjectId)
      {
         nxlog_debug_tag(_T("obj.dc"), 7, _T("DataCollectionOwner::DeleteDCObject: deleting DCObject %d created by DCObject %d instance discovery from object %d"), (int)subObject->getId(), (int)dcObjectId, (int)m_id);
         deleteDCObject(subObject);
         NotifyClientsOnDCIDelete(*this, subObject->getId());
         m_dcObjects.remove(i);
         i--;
      }
   }
}

/**
 * Delete DCI object.
 * Deletes or schedules deletion from DB.
 * It is assumed that list is already locked.
 */
void DataCollectionOwner::deleteDCObject(DCObject *object)
{
   if (object->prepareForDeletion())
   {
      // Delete DCI from database only if it is not busy
      // Busy DCIs will be deleted by data collector
      object->deleteFromDatabase();
   }
   else
   {
      nxlog_debug_tag(_T("obj.dc"), 7, _T("DataCollectionOwner::deleteDCObject: destruction of DCO %u delayed"), object->getId());
   }
}

/**
 * Modify data collection object from NXCP message
 */
uint32_t DataCollectionOwner::updateDCObject(uint32_t dcObjectId, const NXCPMessage& msg, uint32_t *numMaps, uint32_t **mapIndex, uint32_t **mapId, uint32_t userId)
{
   uint32_t result = RCC_INVALID_DCI_ID;

   readLockDciAccess();

   // Check if that item exists
   for(int i = 0; i < m_dcObjects.size(); i++)
	{
		DCObject *object = m_dcObjects.get(i);
      if (object->getId() == dcObjectId)
      {
         if (object->hasAccess(userId))
         {
            if (object->getType() == DCO_TYPE_ITEM)
               static_cast<DCItem*>(object)->updateFromMessage(msg, numMaps, mapIndex, mapId);
            else
               object->updateFromMessage(msg);

            if (object->getInstanceDiscoveryMethod() != IDM_NONE)
            {
               updateInstanceDiscoveryItems(object);
               m_instanceDiscoveryChanges = true;
            }

            result = RCC_SUCCESS;
         }
         else
         {
            result = RCC_ACCESS_DENIED;
            nxlog_debug_tag(_T("obj.dc"), 6, _T("DataCollectionOwner::updateDCObject: denied access to DCObject %u for user %u"), dcObjectId, userId);
         }
         break;
      }
	}

   unlockDciAccess();

   if (result == RCC_SUCCESS)
   {
      lockProperties();
      setModified(MODIFY_DATA_COLLECTION);
      unlockProperties();
   }

   return result;
}

/**
 * Update DCIs created by instance dicovery.
 * This method expects DCI access already locked.
 *
 * @param dci instance discovery template DCI
 */
void DataCollectionOwner::updateInstanceDiscoveryItems(DCObject *dci)
{
   for(int i = 0; i < m_dcObjects.size(); i++)
	{
		DCObject *object = m_dcObjects.get(i);
      if ((object->getTemplateId() == m_id) && (object->getTemplateItemId() == dci->getId()))
      {
         object->updateFromTemplate(dci);
         NotifyClientsOnDCIUpdate(*this, object);
      }
	}
}

/**
 * Set status for group of DCIs from UI
 */
unique_ptr<IntegerArray<uint32_t>> DataCollectionOwner::setItemStatus(const IntegerArray<uint32_t>& dciList, int status, uint32_t userId, bool userChange)
{
   auto result = make_unique<IntegerArray<uint32_t>>();
   readLockDciAccess();
   for(int i = 0; i < dciList.size(); i++)
   {
		int j;
      for(j = 0; j < m_dcObjects.size(); j++)
      {
         if (m_dcObjects.get(j)->getId() == dciList.get(i))
         {
            if (m_dcObjects.get(j)->hasAccess(userId))
            {
               m_dcObjects.get(j)->setStatus(status, true, userChange);
               result->set(j, RCC_SUCCESS);
               break;
            }
            else
            {
               result->set(j, RCC_ACCESS_DENIED);
            }
         }
      }
      if (j == m_dcObjects.size())
         result->set(j, RCC_INVALID_DCI_ID);
   }
   unlockDciAccess();
   return result;
}

/**
 * Update multiple data collection objects. Returns number of updated DCIs.
 */
int DataCollectionOwner::updateMultipleDCObjects(const NXCPMessage& request, uint32_t userId)
{
   IntegerArray<uint32_t> idList;
   request.getFieldAsInt32Array(VID_ITEM_LIST, &idList);
   if (idList.isEmpty())
      return 0;

   int32_t pollingScheduleType = -1;
   if (request.isFieldExist(VID_POLLING_SCHEDULE_TYPE))
      pollingScheduleType = request.getFieldAsInt32(VID_POLLING_SCHEDULE_TYPE);

   int32_t retentionType = -1;
   if (request.isFieldExist(VID_RETENTION_TYPE))
      retentionType = request.getFieldAsInt32(VID_RETENTION_TYPE);

   SharedString pollingIntervalSrc = request.getFieldAsSharedString(VID_POLLING_INTERVAL);
   SharedString retentionTimeSrc = request.getFieldAsSharedString(VID_RETENTION_TIME);
   SharedString unitName = request.getFieldAsSharedString(VID_UNITS_NAME);
   bool thresholdEnableTimeUpdated = request.isFieldExist(VID_THRESHOLD_ENABLE_TIME);
   time_t thresholdEnableTime = request.getFieldAsTime(VID_THRESHOLD_ENABLE_TIME);

   int count = 0;
   readLockDciAccess();
   for(int i = 0; i < idList.size(); i++)
   {
      int j;
      for(j = 0; j < m_dcObjects.size(); j++)
      {
         DCObject *dci = m_dcObjects.get(j);
         if (dci->getId() == idList.get(i))
         {
            if (dci->hasAccess(userId))
            {
               if (!pollingIntervalSrc.isNull())
                  dci->setPollingInterval(pollingIntervalSrc);
               if (!retentionTimeSrc.isNull())
                  dci->setRetention(retentionTimeSrc);
               if (pollingScheduleType != -1)
                  dci->setPollingIntervalType(static_cast<BYTE>(pollingScheduleType));
               if (retentionType != -1)
                  dci->setRetentionType(static_cast<BYTE>(retentionType));
               if (!unitName.isNull() && (dci->getType() == DCO_TYPE_ITEM))
                  static_cast<DCItem*>(dci)->setUnitName(unitName);
               if (thresholdEnableTimeUpdated)
                  dci->setThresholdDisableEndTime(thresholdEnableTime);
               NotifyClientsOnDCIUpdate(*this, dci);
               count++;
            }
            break;
         }
      }
   }
   unlockDciAccess();

   if (count > 0)
      setModified(MODIFY_DATA_COLLECTION);

   return count;
}

/**
 * Unlock data collection items list
 */
void DataCollectionOwner::applyDCIChanges(bool forcedChange)
{
   bool callChangeHook = false;

   lockProperties();
   if (m_dciListModified || forcedChange)
   {
      setModified(MODIFY_DATA_COLLECTION);
      callChangeHook = true;
   }
   m_dciListModified = false;

   bool callInstanceChangeHook = m_instanceDiscoveryChanges;
   m_instanceDiscoveryChanges = false;
   unlockProperties();

   if (callChangeHook)
      onDataCollectionChange();

   if (callInstanceChangeHook)
      onInstanceDiscoveryChange();
}

/**
 * Send DCI list to client
 */
void DataCollectionOwner::sendItemsToClient(ClientSession *session, uint32_t requestId) const
{
   NXCPMessage msg(CMD_NODE_DCI, requestId);

   readLockDciAccess();

   // Walk through items list
   for(int i = 0; i < m_dcObjects.size(); i++)
   {
      DCObject *dco = m_dcObjects.get(i);
      if (dco->hasAccess(session->getUserId()))
      {
         dco->createMessage(&msg);
         session->sendMessage(&msg);
         msg.deleteAllFields();
      }
      else
      {
         nxlog_debug_tag(_T("obj.dc"), 6, _T("DataCollectionOwner::sendItemsToClient: denied access to DCObject %u for user %u"), dco->getId(), session->getUserId());
      }
   }

   unlockDciAccess();

   // Send end-of-list indicator
	msg.setEndOfSequence();
   session->sendMessage(&msg);
}

/**
 * Get data collection object that match given filter
 */
shared_ptr<DCObject> DataCollectionOwner::getDCObjectByFilter(std::function<bool (DCObject*)> filter, uint32_t userId, bool lock) const
{
   shared_ptr<DCObject> object;

   if (lock)
      readLockDciAccess();

   for(int i = 0; i < m_dcObjects.size(); i++)
   {
      DCObject *curr = m_dcObjects.get(i);
      if (filter(curr))
      {
         if (curr->hasAccess(userId))
            object = m_dcObjects.getShared(i);
         else
            nxlog_debug_tag(_T("obj.dc"), 6, _T("DataCollectionOwner::getDCObjectByFilter: denied access to DCObject %u for user %u"), curr->getId(), userId);
         break;
      }
   }

   if (lock)
      unlockDciAccess();
   return object;
}

/**
 * Get item by its id
 */
shared_ptr<DCObject> DataCollectionOwner::getDCObjectById(uint32_t itemId, uint32_t userId, bool lock) const
{
   shared_ptr<DCObject> object;

   if (lock)
      readLockDciAccess();

   for(int i = 0; i < m_dcObjects.size(); i++)
	{
      DCObject *curr = m_dcObjects.get(i);
      if (curr->getId() == itemId)
		{
         if (curr->hasAccess(userId))
            object = m_dcObjects.getShared(i);
         else
            nxlog_debug_tag(_T("obj.dc"), 6, _T("DataCollectionOwner::getDCObjectById: denied access to DCObject %u for user %u"), itemId, userId);
         break;
		}
	}

   if (lock)
      unlockDciAccess();
   return object;
}

/**
 * Get item by template item id
 */
shared_ptr<DCObject> DataCollectionOwner::getDCObjectByTemplateId(uint32_t tmplItemId, uint32_t userId) const
{
   return getDCObjectByFilter(
      [tmplItemId] (DCObject *dci) -> bool
      {
         return dci->getTemplateItemId() == tmplItemId;
      }, userId);
}

/**
 * Get data collection object by its name (case-insensitive)
 */
shared_ptr<DCObject> DataCollectionOwner::getDCObjectByName(const WCHAR *name, uint32_t userId) const
{
   return getDCObjectByFilter(
      [name] (DCObject *dci) -> bool
      {
         return wcsicmp(dci->getName(), name) == 0;
      }, userId);
}

/**
 * Get data collection object by its description (case-insensitive)
 */
shared_ptr<DCObject> DataCollectionOwner::getDCObjectByDescription(const WCHAR *description, uint32_t userId) const
{
   return getDCObjectByFilter(
      [description] (DCObject *dci) -> bool
      {
         return wcsicmp(dci->getDescription(), description) == 0;
      }, userId);
}

/**
 * Get data collection object by its tag (case-insensitive)
 */
shared_ptr<DCObject> DataCollectionOwner::getDCObjectByTag(const WCHAR *tag, uint32_t userId) const
{
   return getDCObjectByFilter(
      [tag] (DCObject *dci) -> bool
      {
         return wcsicmp(dci->getUserTag(), tag) == 0;
      }, userId);
}

/**
 * Get data collection object by tag pattern (case-insensitive)
 */
shared_ptr<DCObject> DataCollectionOwner::getDCObjectByTagPattern(const WCHAR *tagPattern, uint32_t userId) const
{
   return getDCObjectByFilter(
      [tagPattern] (DCObject *dci) -> bool
      {
         return MatchStringW(tagPattern, dci->getUserTag(), false);
      }, userId);
}

/**
 * Get item by GUID
 */
shared_ptr<DCObject> DataCollectionOwner::getDCObjectByGUID(uuid guid, uint32_t userId, bool lock) const
{
   return getDCObjectByFilter(
      [guid] (DCObject *dci) -> bool
      {
         return guid.equals(dci->getGuid());
      }, userId, lock);
}

/**
 * Get all DC objects with matching name, description, and tag
 */
NXSL_Value *DataCollectionOwner::getAllDCObjectsForNXSL(NXSL_VM *vm, const wchar_t *name, const wchar_t *description, const wchar_t *tag, uint32_t relatedObjectId, uint32_t userID) const
{
   NXSL_Array *list = new NXSL_Array(vm);
   readLockDciAccess();
   for(int i = 0; i < m_dcObjects.size(); i++)
	{
		DCObject *curr = m_dcObjects.get(i);
      if (((name == nullptr) || MatchString(name, curr->getName(), false)) &&
          ((description == nullptr) || MatchString(description, curr->getDescription(), false)) &&
          ((tag == nullptr) || MatchString(tag, curr->getUserTag(), false)) &&
          ((relatedObjectId == 0) || (curr->getRelatedObject() == relatedObjectId)) &&
          curr->hasAccess(userID))
		{
         list->set(list->size(), curr->createNXSLObject(vm));
		}
	}
	unlockDciAccess();
   return vm->createValue(list);
}

/**
 * Fill NXCP message with object's data - stage 2
 * Object's properties are not locked when this method is called. Should be
 * used only to fill data where properties lock is not enough (like data
 * collection configuration).
 */
void DataCollectionOwner::fillMessageUnlocked(NXCPMessage *msg, uint32_t userId)
{
   super::fillMessageUnlocked(msg, userId);
   readLockDciAccess();
   msg->setField(VID_NUM_ITEMS, m_dcObjects.size());
   unlockDciAccess();
}

/**
 * Apply template to data collection target
 */
bool DataCollectionOwner::applyToTarget(const shared_ptr<DataCollectionTarget>& target)
{
   bool success = true;

   // Link node to template
   if (!isDirectChild(target->getId()))
   {
      linkObjects(self(), target);
   }

   // Copy items
   readLockDciAccess();

   IntegerArray<uint32_t> dciList(m_dcObjects.size());
   nxlog_debug_tag(DEBUG_TAG_DC_TEMPLATES, 4, _T("Apply %d metric items from template \"%s\" to target \"%s\""),
                   m_dcObjects.size(), m_name, target->getName());

   for(int i = 0; i < m_dcObjects.size(); i++)
   {
		DCObject *object = m_dcObjects.get(i);
		dciList.add(object->getId());
      if (!target->applyTemplateItem(m_id, object))
      {
         success = false;
      }
   }
   unlockDciAccess();

   // Clean items deleted from template
   target->cleanDeletedTemplateItems(m_id, dciList);

   static_cast<DataCollectionOwner*>(target.get())->onDataCollectionChange();

   // Queue update if target is a cluster
   if (target->getObjectClass() == OBJECT_CLUSTER)
   {
      target->queueUpdate();
   }

   return success;
}

/**
 * Queue template update
 */
void DataCollectionOwner::queueUpdate()
{
   readLockChildList();
   for(int i = 0; i < getChildList().size(); i++)
   {
      NetObj *object = getChildList().get(i);
      if (object->isDataCollectionTarget())
      {
         g_templateUpdateQueue.put(new TemplateUpdateTask(self(), object->getId(), APPLY_TEMPLATE, false));
      }
   }
   unlockChildList();
}

/**
 * Queue template remove from node
 */
void DataCollectionOwner::queueRemoveFromTarget(uint32_t targetId, bool removeDCI)
{
   g_templateUpdateQueue.put(new TemplateUpdateTask(self(), targetId, REMOVE_TEMPLATE, removeDCI));
}

/**
 * Get list of events used by DCIs
 */
HashSet<uint32_t> *DataCollectionOwner::getRelatedEventsList() const
{
   HashSet<uint32_t> *eventList = new HashSet<uint32_t>();
   readLockDciAccess();
   for(int i = 0; i < m_dcObjects.size(); i++)
   {
      m_dcObjects.get(i)->getEventList(eventList);
   }
   unlockDciAccess();

   return eventList;
}

/**
 * Get list of scripts used by DCIs
 */
unique_ptr<StringSet> DataCollectionOwner::getDCIScriptList() const
{
   StringSet *scripts = new StringSet();
   readLockDciAccess();
   for(int i = 0; i < m_dcObjects.size(); i++)
   {
      m_dcObjects.get(i)->getScriptDependencies(scripts);
   }
   unlockDciAccess();
   return unique_ptr<StringSet>(scripts);
}

/**
 * Enumerate all DCIs
 */
bool DataCollectionOwner::enumDCObjects(bool (*callback)(const shared_ptr<DCObject>&, uint32_t, void *), void *context) const
{
	bool success = true;

	readLockDciAccess();
	for(int i = 0; i < m_dcObjects.size(); i++)
	{
		if (!callback(m_dcObjects.getShared(i), i, context))
		{
			success = false;
			break;
		}
	}
	unlockDciAccess();
	return success;
}

/**
 * (Re)associate all DCIs
 */
void DataCollectionOwner::associateItems()
{
	readLockDciAccess();
	for(int i = 0; i < m_dcObjects.size(); i++)
		m_dcObjects.get(i)->changeBinding(0, self(), false);
	unlockDciAccess();
}

/**
 * Prepare data collection owner for deletion
 */
void DataCollectionOwner::prepareForDeletion()
{
   super::prepareForDeletion();
}

/**
 * Update data collection owner from import
 */
void DataCollectionOwner::updateFromImport(ConfigEntry *config, ImportContext *context, bool nxslV5)
{
   lockProperties();
   wcslcpy(m_name, config->getSubEntryValue(L"name", 0, L"Unnamed Object"), MAX_OBJECT_NAME);
   m_flags = config->getSubEntryValueAsUInt(L"flags", 0, m_flags);
   unlockProperties();
   setComments(config->getSubEntryValue(L"comments", 0, nullptr));

   // Data collection
   ObjectArray<uuid> guidList(32, 32, Ownership::True);

   writeLockDciAccess();
   ConfigEntry *dcRoot = config->findEntry(L"dataCollection");
   if (dcRoot != nullptr)
   {
      unique_ptr<ObjectArray<ConfigEntry>> dcis = dcRoot->getSubEntries(L"dci#*");
      for(int i = 0; i < dcis->size(); i++)
      {
         ConfigEntry *e = dcis->get(i);
         uuid guid = e->getSubEntryValueAsUUID(L"guid");
         shared_ptr<DCObject> curr = !guid.isNull() ? getDCObjectByGUID(guid, 0, false) : shared_ptr<DCObject>();
         if ((curr != nullptr) && (curr->getType() == DCO_TYPE_ITEM))
         {
            curr->updateFromImport(e, nxslV5);
         }
         else
         {
            auto dci = make_shared<DCItem>(e, self(), nxslV5);
            m_dcObjects.add(dci);
            guid = dci->getGuid();  // For case when export file does not contain valid GUID
         }
         guidList.add(new uuid(guid));
      }

      unique_ptr<ObjectArray<ConfigEntry>> dctables = dcRoot->getSubEntries(L"dctable#*");
      for(int i = 0; i < dctables->size(); i++)
      {
         ConfigEntry *e = dctables->get(i);
         uuid guid = e->getSubEntryValueAsUUID(_T("guid"));
         shared_ptr<DCObject> curr = !guid.isNull() ? getDCObjectByGUID(guid, 0, false) : shared_ptr<DCObject>();
         if ((curr != nullptr) && (curr->getType() == DCO_TYPE_TABLE))
         {
            curr->updateFromImport(e, nxslV5);
         }
         else
         {
            auto dci = make_shared<DCTable>(e, self(), nxslV5);
            m_dcObjects.add(dci);
            guid = dci->getGuid();  // For case when export file does not contain valid GUID
         }
         guidList.add(new uuid(guid));
      }
   }

   // Delete DCIs missing in import
   IntegerArray<uint32_t> deleteList;
   for(int i = 0; i < m_dcObjects.size(); i++)
   {
      bool found = false;
      for(int j = 0; j < guidList.size(); j++)
      {
         if (guidList.get(j)->equals(m_dcObjects.get(i)->getGuid()))
         {
            found = true;
            break;
         }
      }

      if (!found)
      {
         deleteList.add(m_dcObjects.get(i)->getId());
      }
   }

   for(int i = 0; i < deleteList.size(); i++)
      deleteDCObject(deleteList.get(i), false);

   unlockDciAccess();

   queueUpdate();
}

/**
 * Serialize object to JSON
 */
json_t *DataCollectionOwner::toJson()
{
   json_t *root = super::toJson();

   lockProperties();
   json_object_set_new(root, "flags", json_integer(m_flags));
   unlockProperties();

   return root;
}

/**
 * Check if given node is data collection source for at least one DCI
 */
bool DataCollectionOwner::isDataCollectionSource(UINT32 nodeId) const
{
   bool result = false;
   readLockDciAccess();
   for(int i = 0; i < m_dcObjects.size(); i++)
   {
      if (m_dcObjects.get(i)->getSourceNode() == nodeId)
      {
         result = true;
         break;
      }
   }
   unlockDciAccess();
   return result;
}

/**
 * Find DCObject by regex
 *
 * @param regex
 * @param searchName set to true if search by DCO name required
 * @param userId to check user access
 * @return list of matching DCOs
 */
unique_ptr<SharedObjectArray<DCObject>> DataCollectionOwner::getDCObjectsByRegex(const TCHAR *regex, bool searchName, uint32_t userId) const
{
   const char *eptr;
   int eoffset;
   PCRE *preg = _pcre_compile_t(reinterpret_cast<const PCRE_TCHAR*>(regex), PCRE_COMMON_FLAGS | PCRE_CASELESS, &eptr, &eoffset, nullptr);
   if (preg == nullptr)
      return unique_ptr<SharedObjectArray<DCObject>>();

   readLockDciAccess();

   auto result = make_unique<SharedObjectArray<DCObject>>(m_dcObjects.size());

   for(int i = 0; i < m_dcObjects.size(); i++)
   {
      DCObject *o = m_dcObjects.get(i);
      if (!o->hasAccess(userId))
         continue;

      const TCHAR *subj = searchName ? o->getName() : o->getDescription();
      int ovector[30];
      if (_pcre_exec_t(preg, nullptr, reinterpret_cast<const PCRE_TCHAR*>(subj), static_cast<int>(_tcslen(subj)), 0, 0, ovector, 30) >= 0)
      {
         result->add(m_dcObjects.getShared(i));
      }
   }

   unlockDciAccess();

   _pcre_free_t(preg);
   return result;
}


SharedObjectArray<DCObject> DataCollectionOwner::getDCObjectsByFilter(std::function<bool (DCObject*)> filter, uint32_t userId) const
{
   auto result = SharedObjectArray<DCObject>(m_dcObjects.size());
   readLockDciAccess();

   for(int i = 0; i < m_dcObjects.size(); i++)
   {
      DCObject *curr = m_dcObjects.get(i);
      if (filter(curr))
      {
         if (curr->hasAccess(userId))
            result.add(m_dcObjects.getShared(i));
         else
            nxlog_debug_tag(_T("obj.dc"), 6, _T("DataCollectionOwner::getDCObjectByFilter: denied access to DCObject %u for user %u"), curr->getId(), userId);
      }
   }

   unlockDciAccess();
   return result;

}

/**
 * Get all data collection objects
 */
unique_ptr<SharedObjectArray<DCObject>> DataCollectionOwner::getAllDCObjects(uint32_t userId) const
{
   readLockDciAccess();
   auto result = make_unique<SharedObjectArray<DCObject>>(m_dcObjects.size());
   for(int i = 0; i < m_dcObjects.size(); i++)
   {
      shared_ptr<DCObject> dci = m_dcObjects.getShared(i);
      if (userId == 0 || dci->hasAccess(userId))
      {
         result->add(dci);
      }
   }
   unlockDciAccess();
   return result;
}

/**
 * Collects information about all DCI that are using specified event
 */
void DataCollectionOwner::getEventReferences(uint32_t eventCode, ObjectArray<EventReference>* eventReferences) const
{
   readLockDciAccess();
   for (int i = 0; i < m_dcObjects.size(); i++)
   {
      DCObject *dci = m_dcObjects.get(i);
      if (dci->isUsingEvent(eventCode))
      {
         eventReferences->add(new EventReference(
            EventReferenceType::DCI,
            dci->getId(),
            dci->getGuid(),
            dci->getName(),
            m_id,
            m_guid,
            m_name,
            dci->getDescription()));
      }
   }
   unlockDciAccess();
}

/**
 * Get last (current) DCI values.
 */
uint32_t DataCollectionOwner::getDataCollectionSummary(NXCPMessage *msg, bool objectTooltipOnly, bool overviewOnly, bool includeNoValueObjects, uint32_t userId)
{
   if (!includeNoValueObjects)
      return RCC_INCOMPATIBLE_OPERATION;

   readLockDciAccess();

   uint32_t fieldId = VID_DCI_VALUES_BASE, count = 0;
   for(int i = 0; i < m_dcObjects.size(); i++)
   {
      DCObject *object = m_dcObjects.get(i);
      if ((!objectTooltipOnly || object->isShowOnObjectTooltip()) &&
          (!overviewOnly || object->isShowInObjectOverview()) &&
          object->hasAccess(userId))
      {
         static_cast<DCItem*>(object)->fillLastValueSummaryMessage(msg, fieldId);
         fieldId += 50;
         count++;
      }
   }
   msg->setField(VID_NUM_ITEMS, count);

   unlockDciAccess();
   return RCC_SUCCESS;
}

/**
 * Get last (current) DCI values.
 */
uint32_t DataCollectionOwner::getDataCollectionSummary(json_t *values, bool objectTooltipOnly, bool overviewOnly, bool includeNoValueObjects, uint32_t userId, std::function<bool(DCObject*)> filter)
{
   if (!includeNoValueObjects)
      return RCC_INCOMPATIBLE_OPERATION;

   readLockDciAccess();

   for(int i = 0; i < m_dcObjects.size(); i++)
   {
      DCObject *object = m_dcObjects.get(i);
      if ((!objectTooltipOnly || object->isShowOnObjectTooltip()) &&
          (!overviewOnly || object->isShowInObjectOverview()) &&
          object->hasAccess(userId) &&
          (filter == nullptr || filter(object)))
      {
         json_array_append_new(values, static_cast<DCItem*>(object)->lastValueToJSON());
      }
   }

   unlockDciAccess();
   return RCC_SUCCESS;
}
