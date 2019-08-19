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
** File: dcowner.cpp
**
**/

#include "nxcore.h"
#include <netxms-regex.h>

/**
 * Data collection owner object constructor
 */
DataCollectionOwner::DataCollectionOwner() : super()
{
	m_dcObjects = new SharedObjectArray<DCObject>(0, 128);
   m_status = STATUS_NORMAL;
   m_dciAccessLock = RWLockCreate();
   m_dciListModified = false;
}

/**
 * Constructor for new data collection owner object
 */
DataCollectionOwner::DataCollectionOwner(const TCHAR *pszName) : super()
{
   nx_strncpy(m_name, pszName, MAX_OBJECT_NAME);
	m_dcObjects = new SharedObjectArray<DCObject>(0, 128);
   m_status = STATUS_NORMAL;
   m_isHidden = true;
   m_dciAccessLock = RWLockCreate();
   m_dciListModified = false;
}

/**
 * Create data collection owner object from import file
 */
DataCollectionOwner::DataCollectionOwner(ConfigEntry *config)
{
   m_isHidden = true;
   m_status = STATUS_NORMAL;
   m_dciAccessLock = RWLockCreate();
   m_dciListModified = false;

   // GUID
   uuid guid = config->getSubEntryValueAsUUID(_T("guid"));
   if (!guid.isNull())
      m_guid = guid;

   // Name
   _tcslcpy(m_name, config->getSubEntryValue(_T("name"), 0, _T("Unnamed Object")), MAX_OBJECT_NAME);
   m_flags = config->getSubEntryValueAsUInt(_T("flags"), 0, 0);

   //Comment
   m_comments = MemCopyString(config->getSubEntryValue(_T("comments")));

   // Data collection
   m_dcObjects = new SharedObjectArray<DCObject>(0, 128);
   ConfigEntry *dcRoot = config->findEntry(_T("dataCollection"));
   if (dcRoot != NULL)
   {
      ObjectArray<ConfigEntry> *dcis = dcRoot->getSubEntries(_T("dci#*"));
      for(int i = 0; i < dcis->size(); i++)
      {
         m_dcObjects->add(new DCItem(dcis->get(i), this));
      }
      delete dcis;

      ObjectArray<ConfigEntry> *dctables = dcRoot->getSubEntries(_T("dctable#*"));
      for(int i = 0; i < dctables->size(); i++)
      {
         m_dcObjects->add(new DCTable(dctables->get(i), this));
      }
      delete dctables;
   }
}

/**
 * Destructor
 */
DataCollectionOwner::~DataCollectionOwner()
{
	delete m_dcObjects;
	RWLockDestroy(m_dciAccessLock);
}

/**
 * Destroy all related data collection items.
 */
void DataCollectionOwner::destroyItems()
{
	m_dcObjects->clear();
}

/**
 * Create data collection owner object from database data
 *
 * @param dwId object ID
 */
bool DataCollectionOwner::loadFromDatabase(DB_HANDLE hdb, UINT32 id)
{
   bool success = true;

   m_id = id;

   if (!loadCommonProperties(hdb))
      return false;

   // Load DCI and access list
   loadACLFromDB(hdb);
   loadItemsFromDB(hdb);
   for(int i = 0; i < m_dcObjects->size(); i++)
      if (!m_dcObjects->get(i)->loadThresholdsFromDB(hdb))
         success = false;

	m_status = STATUS_NORMAL;

   return success;
}

/**
 * Save object to database
 */
bool DataCollectionOwner::saveToDatabase(DB_HANDLE hdb)
{
   lockProperties();

   bool success = saveCommonProperties(hdb);

   // Save access list
   if (success)
      success = saveACLToDB(hdb);

   unlockProperties();

   // Save data collection items
	if (success && (m_modified & MODIFY_DATA_COLLECTION))
	{
      lockDciAccess(false);
      for(int i = 0; success && (i < m_dcObjects->size()); i++)
         success = m_dcObjects->get(i)->saveToDatabase(hdb);
      unlockDciAccess();
	}

   return success;
}

/**
 * Process list of deleted items
 */
static bool ProcessDeletedItems(DB_HANDLE hdb, const String& list)
{
   String query = _T("DELETE FROM thresholds WHERE item_id IN (");
   query.append(list);
   query.append(_T(')'));
   return DBQuery(hdb, query);
}

/**
 * Process list of deleted tables
 */
static bool ProcessDeletedTables(DB_HANDLE hdb, const String& list)
{
   String query = _T("DELETE FROM dc_table_columns WHERE table_id IN (");
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
   String query = _T("DELETE FROM dct_threshold_conditions WHERE threshold_id IN (");
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
   String query = _T("DELETE FROM dci_schedules WHERE item_id IN (");
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
      String listItems, listTables, listTableThresholds, listAll;
      int countItems = 0, countTables = 0, countTableThresholds = 0, countAll = 0;
      for(int i = 0; (i < m_dcObjects->size()) && success; i++)
      {
         DCObject *o = m_dcObjects->get(i);
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

            IntegerArray<UINT32> *idList = static_cast<DCTable*>(o)->getThresholdIdList();
            for(int j = 0; j < idList->size(); j++)
            {
               if (!listTableThresholds.isEmpty())
                  listTableThresholds.append(_T(','));
               listTableThresholds.append(idList->get(j));
               countTableThresholds++;
            }
            delete idList;
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
void DataCollectionOwner::loadItemsFromDB(DB_HANDLE hdb)
{
   bool useStartupDelay = ConfigReadBoolean(_T("DataCollection.StartupDelay"), false);

	DB_STATEMENT hStmt = DBPrepare(hdb,
	           _T("SELECT item_id,name,source,datatype,polling_interval,retention_time,")
              _T("status,delta_calculation,transformation,template_id,description,")
              _T("instance,template_item_id,flags,resource_id,")
              _T("proxy_node,base_units,unit_multiplier,custom_units_name,")
	           _T("perftab_settings,system_tag,snmp_port,snmp_raw_value_type,")
				  _T("instd_method,instd_data,instd_filter,samples,comments,guid,npe_name, ")
				  _T("instance_retention_time,grace_period_start FROM items WHERE node_id=?"));
	if (hStmt != NULL)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
		DB_RESULT hResult = DBSelectPrepared(hStmt);
		if (hResult != NULL)
		{
			int count = DBGetNumRows(hResult);
			for(int i = 0; i < count; i++)
				m_dcObjects->add(new DCItem(hdb, hResult, i, this, useStartupDelay));
			DBFreeResult(hResult);
		}
		DBFreeStatement(hStmt);
	}

	hStmt = DBPrepare(hdb,
	           _T("SELECT item_id,template_id,template_item_id,name,")
				  _T("description,flags,source,snmp_port,polling_interval,retention_time,")
              _T("status,system_tag,resource_id,proxy_node,perftab_settings,")
              _T("transformation_script,comments,guid,instd_method,instd_data,")
              _T("instd_filter,instance,instance_retention_time,grace_period_start ")
              _T("FROM dc_tables WHERE node_id=?"));
	if (hStmt != NULL)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
		DB_RESULT hResult = DBSelectPrepared(hStmt);
		if (hResult != NULL)
		{
			int count = DBGetNumRows(hResult);
			for(int i = 0; i < count; i++)
				m_dcObjects->add(new DCTable(hdb, hResult, i, this, useStartupDelay));
			DBFreeResult(hResult);
		}
		DBFreeStatement(hStmt);
	}

   onDataCollectionLoad();
}

/**
 * Add data collection object to node
 */
bool DataCollectionOwner::addDCObject(DCObject *object, bool alreadyLocked, bool notify)
{
   int i;
   bool success = false;
   if (!alreadyLocked)
      lockDciAccess(true); // write lock
   // Check if that object exists
   for(i = 0; i < m_dcObjects->size(); i++)
      if (m_dcObjects->get(i)->getId() == object->getId())
         break;   // Object with specified id already exist

   if (i == m_dcObjects->size())     // Add new item
   {
		m_dcObjects->add(object);
      object->setLastPollTime(0);    // Cause item to be polled immediately
      if (object->getStatus() != ITEM_STATUS_DISABLED)
         object->setStatus(ITEM_STATUS_ACTIVE, false);
      object->clearBusyFlag();
      success = true;
   }

   if (!alreadyLocked)
      unlockDciAccess();

	if (success)
	{
		lockProperties();
      setModified(MODIFY_DATA_COLLECTION);
		unlockProperties();
		if(notify)
		   NotifyClientsOnDCIUpdate(this, object);
	}
   return success;
}

/**
 * Delete data collection object from node
 */
bool DataCollectionOwner::deleteDCObject(UINT32 dcObjectId, bool needLock, UINT32 userId)
{
   bool success = false;

	if (needLock)
		lockDciAccess(true);  // write lock

   // Check if that item exists
   for(int i = 0; i < m_dcObjects->size(); i++)
	{
		DCObject *object = m_dcObjects->get(i);
      if (object->getId() == dcObjectId)
      {
         if (object->hasAccess(userId))
         {
            shared_ptr<DCObject> ref = m_dcObjects->getShared(i);  // Prevent destruction by call to remove
            m_dcObjects->remove(i);

            // Check if it is instance DCI
            if ((object->getType() == DCO_TYPE_ITEM) && (((DCItem *)object)->getInstanceDiscoveryMethod() != IDM_NONE))
            {
               deleteChildDCIs(dcObjectId);
            }
            // Destroy item
            nxlog_debug_tag(_T("obj.dc"), 7, _T("DataCollectionOwner::DeleteDCObject: deleting DCObject [%u] from object %s [%u]"), dcObjectId, m_name, m_id);
            deleteDCObject(object);
            success = true;
            NotifyClientsOnDCIDelete(this, dcObjectId);
            nxlog_debug_tag(_T("obj.dc"), 7, _T("DataCollectionOwner::DeleteDCObject: DCObject deleted from object %s [%u]"), m_name, m_id);
         }
         else
         {
            nxlog_debug_tag(_T("obj.dc"), 6, _T("DataCollectionOwner::DeleteDCObject: denied access to DCObject %u for user %u"), dcObjectId, userId);
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
	}
   return success;
}

/**
 * Deletes child DCI objects of instance discovery DCI.
 * It is assumed that list is already locked
 */
void DataCollectionOwner::deleteChildDCIs(UINT32 dcObjectId)
{
   for(int i = 0; i < m_dcObjects->size(); i++)
   {
      DCObject *subObject = m_dcObjects->get(i);
      if (subObject->getTemplateItemId() == dcObjectId)
      {
         nxlog_debug_tag(_T("obj.dc"), 7, _T("DataCollectionOwner::DeleteDCObject: deleting DCObject %d created by DCObject %d instance discovery from object %d"), (int)subObject->getId(), (int)dcObjectId, (int)m_id);
         deleteDCObject(subObject);
         m_dcObjects->remove(i);
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
      nxlog_debug_tag(_T("obj.dc"), 7, _T("DataCollectionOwner::DeleteItem: destruction of DCO %u delayed"), object->getId());
   }
}

/**
 * Modify data collection object from NXCP message
 */
bool DataCollectionOwner::updateDCObject(UINT32 dwItemId, NXCPMessage *pMsg, UINT32 *pdwNumMaps, UINT32 **ppdwMapIndex, UINT32 **ppdwMapId, UINT32 userId)
{
   bool success = false;

   lockDciAccess(false);

   // Check if that item exists
   for(int i = 0; i < m_dcObjects->size(); i++)
	{
		DCObject *object = m_dcObjects->get(i);
      if (object->getId() == dwItemId)
      {
         if (object->hasAccess(userId))
         {
            if (object->getType() == DCO_TYPE_ITEM)
               static_cast<DCItem*>(object)->updateFromMessage(pMsg, pdwNumMaps, ppdwMapIndex, ppdwMapId);
            else
               object->updateFromMessage(pMsg);

            if (object->getInstanceDiscoveryMethod() != IDM_NONE)
               updateInstanceDiscoveryItems(object);

            success = true;
         }
         else
         {
            nxlog_debug_tag(_T("obj.dc"), 6, _T("DataCollectionOwner::updateDCObject: denied access to DCObject %u for user %u"), dwItemId, userId);
         }
         break;
      }
	}

   unlockDciAccess();

   if (success)
   {
      lockProperties();
      setModified(MODIFY_DATA_COLLECTION);
      unlockProperties();
   }

   return success;
}

/**
 * Update DCIs created by instance dicovery.
 * This method expects DCI access already locked.
 *
 * @param dci instance discovery template DCI
 */
void DataCollectionOwner::updateInstanceDiscoveryItems(DCObject *dci)
{
   for(int i = 0; i < m_dcObjects->size(); i++)
	{
		DCObject *object = m_dcObjects->get(i);
      if ((object->getTemplateId() == m_id) && (object->getTemplateItemId() == dci->getId()))
      {
         object->updateFromTemplate(dci);
      }
	}
}

/**
 * Set status for group of DCIs
 */
bool DataCollectionOwner::setItemStatus(UINT32 dwNumItems, UINT32 *pdwItemList, int iStatus)
{
   bool success = true;

   lockDciAccess(false);
   for(UINT32 i = 0; i < dwNumItems; i++)
   {
		int j;
      for(j = 0; j < m_dcObjects->size(); j++)
      {
         if (m_dcObjects->get(j)->getId() == pdwItemList[i])
         {
            m_dcObjects->get(j)->setStatus(iStatus, true);
            break;
         }
      }
      if (j == m_dcObjects->size())
         success = false;     // Invalid DCI ID provided
   }
   unlockDciAccess();
   return success;
}

/**
 * Unlock data collection items list
 */
void DataCollectionOwner::applyDCIChanges()
{
   bool callChangeHook = false;

   lockProperties();
   if (m_dciListModified)
   {
      setModified(MODIFY_DATA_COLLECTION);
      callChangeHook = true;
   }
   m_dciListModified = false;
   unlockProperties();

   if (callChangeHook)
      onDataCollectionChange();
}

/**
 * Send DCI list to client
 */
void DataCollectionOwner::sendItemsToClient(ClientSession *pSession, UINT32 dwRqId)
{
   NXCPMessage msg;

   // Prepare message
   msg.setId(dwRqId);
   msg.setCode(CMD_NODE_DCI);

   lockDciAccess(false);

   // Walk through items list
   for(int i = 0; i < m_dcObjects->size(); i++)
   {
      DCObject *dco = m_dcObjects->get(i);
      if (dco->hasAccess(pSession->getUserId()))
      {
         dco->createMessage(&msg);
         pSession->sendMessage(&msg);
         msg.deleteAllFields();
      }
      else
      {
         nxlog_debug_tag(_T("obj.dc"), 6, _T("DataCollectionOwner::sendItemsToClient: denied access to DCObject %u for user %u"), dco->getId(), pSession->getUserId());
      }
   }

   unlockDciAccess();

   // Send end-of-list indicator
	msg.setEndOfSequence();
   pSession->sendMessage(&msg);
}

/**
 * Get item by it's id
 */
shared_ptr<DCObject> DataCollectionOwner::getDCObjectById(UINT32 itemId, UINT32 userId, bool lock)
{
   shared_ptr<DCObject> object;

   if (lock)
      lockDciAccess(false);

   for(int i = 0; i < m_dcObjects->size(); i++)
	{
      DCObject *curr = m_dcObjects->get(i);
      if (curr->getId() == itemId)
		{
         if (curr->hasAccess(userId))
            object = m_dcObjects->getShared(i);
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
shared_ptr<DCObject> DataCollectionOwner::getDCObjectByTemplateId(UINT32 tmplItemId, UINT32 userId)
{
   shared_ptr<DCObject> object;

   lockDciAccess(false);
   // Check if that item exists
   for(int i = 0; i < m_dcObjects->size(); i++)
	{
      DCObject *curr = m_dcObjects->get(i);
      if (curr->getTemplateItemId() == tmplItemId)
		{
         if (curr->hasAccess(userId))
            object = m_dcObjects->getShared(i);
         else
            nxlog_debug_tag(_T("obj.dc"), 6, _T("DataCollectionOwner::getDCObjectByTemplateId: denied access to DCObject %u for user %u"), curr->getId(), userId);
         break;
		}
	}

   unlockDciAccess();
   return object;
}

/**
 * Get item by it's name (case-insensitive)
 */
shared_ptr<DCObject> DataCollectionOwner::getDCObjectByName(const TCHAR *name, UINT32 userId)
{
   shared_ptr<DCObject> object;

   lockDciAccess(false);
   // Check if that item exists
   for(int i = 0; i < m_dcObjects->size(); i++)
	{
      DCObject *curr = m_dcObjects->get(i);
      if (!_tcsicmp(curr->getName(), name))
		{
         if (curr->hasAccess(userId))
            object = m_dcObjects->getShared(i);
         else
            nxlog_debug_tag(_T("obj.dc"), 6, _T("DataCollectionOwner::getDCObjectByName: denied access to DCObject %u for user %u"), curr->getId(), userId);
         break;
		}
	}
   unlockDciAccess();
   return object;
}

/**
 * Get item by it's description (case-insensitive)
 */
shared_ptr<DCObject> DataCollectionOwner::getDCObjectByDescription(const TCHAR *description, UINT32 userId)
{
   shared_ptr<DCObject> object;

   lockDciAccess(false);
   // Check if that item exists
   for(int i = 0; i < m_dcObjects->size(); i++)
	{
      DCObject *curr = m_dcObjects->get(i);
      if (!_tcsicmp(curr->getDescription(), description))
		{
         if (curr->hasAccess(userId))
            object = m_dcObjects->getShared(i);
         else
            nxlog_debug_tag(_T("obj.dc"), 6, _T("DataCollectionOwner::getDCObjectByDescription: denied access to DCObject %u for user %u"), curr->getId(), userId);
         break;
		}
	}
	unlockDciAccess();
   return object;
}

/**
 * Get item by GUID
 */
shared_ptr<DCObject> DataCollectionOwner::getDCObjectByGUID(const uuid& guid, UINT32 userId, bool lock)
{
   shared_ptr<DCObject> object;

   if (lock)
      lockDciAccess(false);

   // Check if that item exists
   for(int i = 0; i < m_dcObjects->size(); i++)
   {
      DCObject *curr = m_dcObjects->get(i);
      if (guid.equals(curr->getGuid()))
      {
         if (curr->hasAccess(userId))
            object = m_dcObjects->getShared(i);
         else
            nxlog_debug_tag(_T("obj.dc"), 6, _T("DataCollectionOwner::getDCObjectByGUID: denied access to DCObject %u for user %u"), curr->getId(), userId);
         break;
      }
   }

   if (lock)
      unlockDciAccess();
   return object;
}

/**
 * Get all DC objects with matching name and description
 */
NXSL_Value *DataCollectionOwner::getAllDCObjectsForNXSL(NXSL_VM *vm, const TCHAR *name, const TCHAR *description, UINT32 userID)
{
   NXSL_Array *list = new NXSL_Array(vm);
   lockDciAccess(false);
   for(int i = 0; i < m_dcObjects->size(); i++)
	{
		DCObject *curr = m_dcObjects->get(i);
      if (((name == NULL) || MatchString(name, curr->getName(), false)) &&
          ((description == NULL) || MatchString(description, curr->getDescription(), false)) &&
          curr->hasAccess(userID))
		{
         list->set(list->size(), curr->createNXSLObject(vm));
		}
	}
	unlockDciAccess();
   return vm->createValue(list);
}

/**
 * Redefined status calculation for template
 */
void DataCollectionOwner::calculateCompoundStatus(BOOL bForcedRecalc)
{
   m_status = STATUS_NORMAL;
}

/**
 * Create NXCP message with object's data
 */
void DataCollectionOwner::fillMessageInternal(NXCPMessage *pMsg, UINT32 userId)
{
   super::fillMessageInternal(pMsg, userId);
}

/**
 * Modify object from NXCP message
 */
UINT32 DataCollectionOwner::modifyFromMessageInternal(NXCPMessage *pRequest)
{
   return super::modifyFromMessageInternal(pRequest);
}

/**
 * Apply template to data collection target
 */
BOOL DataCollectionOwner::applyToTarget(DataCollectionTarget *target)
{
   UINT32 *pdwItemList;
   BOOL bErrors = FALSE;

   // Link node to template
   if (!isDirectChild(target->getId()))
   {
      addChild(target);
      target->addParent(this);
   }

   pdwItemList = MemAllocArray<UINT32>(m_dcObjects->size());
   nxlog_debug_tag(_T("obj.dc"), 2, _T("Apply %d metric items from template \"%s\" to target \"%s\""),
                   m_dcObjects->size(), m_name, target->getName());

   // Copy items
   for(int i = 0; i < m_dcObjects->size(); i++)
   {
		DCObject *object = m_dcObjects->get(i);
      pdwItemList[i] = object->getId();
      if (!target->applyTemplateItem(m_id, object))
      {
         bErrors = TRUE;
      }
   }

   // Clean items deleted from template
   target->cleanDeletedTemplateItems(m_id, m_dcObjects->size(), pdwItemList);

   // Cleanup
   MemFree(pdwItemList);

   static_cast<DataCollectionOwner*>(target)->onDataCollectionChange();

   // Queue update if target is a cluster
   if (target->getObjectClass() == OBJECT_CLUSTER)
   {
      target->queueUpdate();
   }

   return bErrors;
}

/**
 * Queue template update
 */
void DataCollectionOwner::queueUpdate()
{
   lockChildList(false);
   for(int i = 0; i < m_childList->size(); i++)
   {
      NetObj *object = m_childList->get(i);
      if (object->isDataCollectionTarget())
      {
         incRefCount();
         TEMPLATE_UPDATE_INFO *pInfo = MemAllocStruct<TEMPLATE_UPDATE_INFO>();
         pInfo->updateType = APPLY_TEMPLATE;
         pInfo->pTemplate = this;
         pInfo->targetId = object->getId();
         pInfo->removeDCI = false;
         g_templateUpdateQueue.put(pInfo);
      }
   }
   unlockChildList();
}

/**
 * Queue template remove from node
 */
void DataCollectionOwner::queueRemoveFromTarget(UINT32 targetId, bool removeDCI)
{
   lockProperties();
   incRefCount();
   TEMPLATE_UPDATE_INFO *pInfo = (TEMPLATE_UPDATE_INFO *)malloc(sizeof(TEMPLATE_UPDATE_INFO));
   pInfo->updateType = REMOVE_TEMPLATE;
   pInfo->pTemplate = this;
   pInfo->targetId = targetId;
   pInfo->removeDCI = removeDCI;
   g_templateUpdateQueue.put(pInfo);
   unlockProperties();
}

/**
 * Get list of events used by DCIs
 */
IntegerArray<UINT32> *DataCollectionOwner::getDCIEventsList()
{
   IntegerArray<UINT32> *eventList = new IntegerArray<UINT32>(64);
   lockDciAccess(false);
   for(int i = 0; i < m_dcObjects->size(); i++)
   {
      m_dcObjects->get(i)->getEventList(eventList);
   }
   unlockDciAccess();

   // Clean list from duplicates
   for(int i = 0; i < eventList->size(); i++)
   {
      for(int j = i + 1; j < eventList->size(); j++)
      {
         if (eventList->get(i) == eventList->get(j))
         {
            eventList->remove(j);
            j--;
         }
      }
   }

   return eventList;
}

/**
 * Get list of scripts used by DCIs
 */
StringSet *DataCollectionOwner::getDCIScriptList()
{
   StringSet *list = new StringSet;

   lockDciAccess(false);
   for(int i = 0; i < m_dcObjects->size(); i++)
   {
      DCObject *o = m_dcObjects->get(i);
      if (o->getDataSource() == DS_SCRIPT)
      {
         const TCHAR *name = o->getName();
         const TCHAR *p = _tcschr(name, _T('('));
         if (p != NULL)
         {
            TCHAR buffer[256];
            _tcslcpy(buffer, name, p - name + 1);
            list->add(buffer);
         }
         else
         {
            list->add(name);
         }
      }
   }
   unlockDciAccess();
   return list;
}

/**
 * Enumerate all DCIs
 */
bool DataCollectionOwner::enumDCObjects(bool (* pfCallback)(DCObject *, UINT32, void *), void *pArg)
{
	bool success = true;

	lockDciAccess(false);
	for(int i = 0; i < m_dcObjects->size(); i++)
	{
		if (!pfCallback(m_dcObjects->get(i), i, pArg))
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
	lockDciAccess(false);
	for(int i = 0; i < m_dcObjects->size(); i++)
		m_dcObjects->get(i)->changeBinding(0, this, FALSE);
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
void DataCollectionOwner::updateFromImport(ConfigEntry *config)
{
   lockProperties();
   m_flags = config->getSubEntryValueAsUInt(_T("flags"), 0, m_flags);
   unlockProperties();

   // Data collection
   ObjectArray<uuid> guidList(32, 32, true);

   lockDciAccess(true);
   ConfigEntry *dcRoot = config->findEntry(_T("dataCollection"));
   if (dcRoot != NULL)
   {
      ObjectArray<ConfigEntry> *dcis = dcRoot->getSubEntries(_T("dci#*"));
      for(int i = 0; i < dcis->size(); i++)
      {
         ConfigEntry *e = dcis->get(i);
         uuid guid = e->getSubEntryValueAsUUID(_T("guid"));
         shared_ptr<DCObject> curr = !guid.isNull() ? getDCObjectByGUID(guid, 0, false) : shared_ptr<DCObject>();
         if ((curr != NULL) && (curr->getType() == DCO_TYPE_ITEM))
         {
            curr->updateFromImport(e);
         }
         else
         {
            m_dcObjects->add(make_shared<DCItem>(e, this));
         }
         guidList.add(new uuid(guid));
      }
      delete dcis;

      ObjectArray<ConfigEntry> *dctables = dcRoot->getSubEntries(_T("dctable#*"));
      for(int i = 0; i < dctables->size(); i++)
      {
         ConfigEntry *e = dctables->get(i);
         uuid guid = e->getSubEntryValueAsUUID(_T("guid"));
         shared_ptr<DCObject> curr = !guid.isNull() ? getDCObjectByGUID(guid, 0, false) : shared_ptr<DCObject>();
         if ((curr != NULL) && (curr->getType() == DCO_TYPE_TABLE))
         {
            curr->updateFromImport(e);
         }
         else
         {
            m_dcObjects->add(make_shared<DCTable>(e, this));
         }
         guidList.add(new uuid(guid));
      }
      delete dctables;
   }

   // Delete DCIs missing in import
   IntegerArray<UINT32> deleteList;
   for(int i = 0; i < m_dcObjects->size(); i++)
   {
      bool found = false;
      for(int j = 0; j < guidList.size(); j++)
      {
         if (guidList.get(j)->equals(m_dcObjects->get(i)->getGuid()))
         {
            found = true;
            break;
         }
      }

      if (!found)
      {
         deleteList.add(m_dcObjects->get(i)->getId());
      }
   }

   for(int i = 0; i < deleteList.size(); i++)
      deleteDCObject(deleteList.get(i), false, 0);

   unlockDciAccess();

   queueUpdate();
}

/**
 * Serialize object to JSON
 */
json_t *DataCollectionOwner::toJson()
{
   json_t *root = super::toJson();

   lockDciAccess(false);
   json_object_set_new(root, "dcObjects", json_object_array(m_dcObjects));
   unlockDciAccess();

   lockProperties();
   json_object_set_new(root, "flags", json_integer(m_flags));
   unlockProperties();

   return root;
}

/**
 * Check if given node is data collection source for at least one DCI
 */
bool DataCollectionOwner::isDataCollectionSource(UINT32 nodeId)
{
   bool result = false;
   lockDciAccess(false);
   for(int i = 0; i < m_dcObjects->size(); i++)
   {
      if (m_dcObjects->get(i)->getSourceNode() == nodeId)
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
SharedObjectArray<DCObject> *DataCollectionOwner::getDCObjectsByRegex(const TCHAR *regex, bool searchName, UINT32 userId)
{
   const char *eptr;
   int eoffset;
   PCRE *preg = _pcre_compile_t(reinterpret_cast<const PCRE_TCHAR*>(regex), PCRE_COMMON_FLAGS | PCRE_CASELESS, &eptr, &eoffset, NULL);
   if (preg == NULL)
      return NULL;

   lockDciAccess(false);

   SharedObjectArray<DCObject> *result = new SharedObjectArray<DCObject>(m_dcObjects->size());

   for(int i = 0; i < m_dcObjects->size(); i++)
   {
      DCObject *o = m_dcObjects->get(i);
      if (!o->hasAccess(userId))
         continue;

      const TCHAR *subj = searchName ? o->getName() : o->getDescription();
      int ovector[30];
      if (_pcre_exec_t(preg, NULL, reinterpret_cast<const PCRE_TCHAR*>(subj), _tcslen(subj), 0, 0, ovector, 30) >= 0)
      {
         result->add(m_dcObjects->getShared(i));
      }
   }

   unlockDciAccess();

   _pcre_free_t(preg);
   return result;
}
