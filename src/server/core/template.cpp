/*
** NetXMS - Network Management System
** Copyright (C) 2003-2017 Victor Kirhenshtein
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
** File: template.cpp
**
**/

#include "nxcore.h"

/**
 * Redefined status calculation for template group
 */
void TemplateGroup::calculateCompoundStatus(BOOL bForcedRecalc)
{
   m_status = STATUS_NORMAL;
}

/**
 * Called by client session handler to check if threshold summary should be shown for this object.
 */
bool TemplateGroup::showThresholdSummary()
{
	return false;
}

/**
 * Template object constructor
 */
Template::Template() : NetObj()
{
	m_dcObjects = new ObjectArray<DCObject>(8, 16, true);
   m_dciLockStatus = -1;
	m_flags = 0;
   m_dwVersion = 0x00010000;  // Initial version is 1.0
	m_applyFilter = NULL;
	m_applyFilterSource = NULL;
   m_status = STATUS_NORMAL;
   m_dciAccessLock = RWLockCreate();
   m_dciListModified = false;
}

/**
 * Constructor for new template object
 */
Template::Template(const TCHAR *pszName) : NetObj()
{
   nx_strncpy(m_name, pszName, MAX_OBJECT_NAME);
	m_dcObjects = new ObjectArray<DCObject>(8, 16, true);
   m_dciLockStatus = -1;
	m_flags = 0;
   m_dwVersion = 0x00010000;  // Initial version is 1.0
	m_applyFilter = NULL;
	m_applyFilterSource = NULL;
   m_status = STATUS_NORMAL;
   m_isHidden = true;
   m_dciAccessLock = RWLockCreate();
   m_dciListModified = false;
}

/**
 * Create template object from import file
 */
Template::Template(ConfigEntry *config) : NetObj()
{
   m_isHidden = true;
   m_dciLockStatus = -1;
   m_status = STATUS_NORMAL;
   m_dciAccessLock = RWLockCreate();
   m_dciListModified = false;

   // GUID
   uuid guid = config->getSubEntryValueAsUUID(_T("guid"));
   if (!guid.isNull())
      m_guid = guid;

	// Name and version
	nx_strncpy(m_name, config->getSubEntryValue(_T("name"), 0, _T("Unnamed Template")), MAX_OBJECT_NAME);
	m_dwVersion = config->getSubEntryValueAsUInt(_T("version"), 0, 0x00010000);
	m_flags = config->getSubEntryValueAsUInt(_T("flags"), 0, 0);

	// Auto-apply filter
	m_applyFilter = NULL;
	m_applyFilterSource = NULL;
	if (m_flags & TF_AUTO_APPLY)
		setAutoApplyFilter(config->getSubEntryValue(_T("filter")));

	// Data collection
	m_dcObjects = new ObjectArray<DCObject>(8, 16, true);
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
Template::~Template()
{
	delete m_dcObjects;
	delete m_applyFilter;
	safe_free(m_applyFilterSource);
	RWLockDestroy(m_dciAccessLock);
}

/**
 * Destroy all related data collection items.
 */
void Template::destroyItems()
{
	m_dcObjects->clear();
}

/**
 * Set auto apply filter.
 *
 * @param filter new filter script code or NULL to clear filter
 */
void Template::setAutoApplyFilter(const TCHAR *filter)
{
	lockProperties();
	free(m_applyFilterSource);
	delete m_applyFilter;
	if (filter != NULL)
	{
		TCHAR error[256];

		m_applyFilterSource = _tcsdup(filter);
		m_applyFilter = NXSLCompile(m_applyFilterSource, error, 256, NULL);
		if (m_applyFilter == NULL)
		{
         TCHAR buffer[1024];
         _sntprintf(buffer, 1024, _T("Template::%s::%d"), m_name, m_id);
         PostEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", buffer, error, m_id);
			nxlog_write(MSG_TEMPLATE_SCRIPT_COMPILATION_ERROR, NXLOG_WARNING, "dss", m_id, m_name, error);
		}
	}
	else
	{
		m_applyFilterSource = NULL;
		m_applyFilter = NULL;
	}
	setModified(MODIFY_OTHER);
	unlockProperties();
}

/**
 * Create template object from database data
 *
 * @param dwId object ID
 */
bool Template::loadFromDatabase(DB_HANDLE hdb, UINT32 dwId)
{
   TCHAR szQuery[256];
   UINT32 i, dwNumNodes, dwNodeId;
   NetObj *pObject;

   m_id = dwId;

   if (!loadCommonProperties(hdb))
      return false;

   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("SELECT version,flags,apply_filter FROM templates WHERE id=%d"), dwId);
   DB_RESULT hResult = DBSelect(hdb, szQuery);
   if (hResult == NULL)
      return false;

   if (DBGetNumRows(hResult) == 0)
   {
      // No object with given ID in database
      DBFreeResult(hResult);
      return false;
   }

   bool success = true;

   m_dwVersion = DBGetFieldULong(hResult, 0, 0);
	m_flags = DBGetFieldULong(hResult, 0, 1);
   m_applyFilterSource = DBGetField(hResult, 0, 2, NULL, 0);
   if (m_applyFilterSource != NULL)
   {
      TCHAR error[256];
      m_applyFilter = NXSLCompile(m_applyFilterSource, error, 256, NULL);
      if (m_applyFilter == NULL)
      {
         TCHAR buffer[1024];
         _sntprintf(buffer, 1024, _T("Template::%s::%d"), m_name, m_id);
         PostEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", buffer, error, m_id);
         nxlog_write(MSG_TEMPLATE_SCRIPT_COMPILATION_ERROR, EVENTLOG_WARNING_TYPE, "dss", m_id, m_name, error);
      }
   }
   DBFreeResult(hResult);

   // Load DCI and access list
   loadACLFromDB(hdb);
   loadItemsFromDB(hdb);
   for(i = 0; i < (UINT32)m_dcObjects->size(); i++)
      if (!m_dcObjects->get(i)->loadThresholdsFromDB(hdb))
         success = false;

   // Load related nodes list
   if (!m_isDeleted)
   {
      _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("SELECT node_id FROM dct_node_map WHERE template_id=%d"), m_id);
      hResult = DBSelect(hdb, szQuery);
      if (hResult != NULL)
      {
         dwNumNodes = DBGetNumRows(hResult);
         for(i = 0; i < dwNumNodes; i++)
         {
            dwNodeId = DBGetFieldULong(hResult, i, 0);
            pObject = FindObjectById(dwNodeId);
            if (pObject != NULL)
            {
               if ((pObject->getObjectClass() == OBJECT_NODE) || (pObject->getObjectClass() == OBJECT_CLUSTER) || (pObject->getObjectClass() == OBJECT_MOBILEDEVICE))
               {
                  addChild(pObject);
                  pObject->addParent(this);
               }
               else
               {
                  nxlog_write(MSG_DCT_MAP_NOT_NODE, EVENTLOG_ERROR_TYPE, "dd", m_id, dwNodeId);
               }
            }
            else
            {
               nxlog_write(MSG_INVALID_DCT_MAP, EVENTLOG_ERROR_TYPE, "dd", m_id, dwNodeId);
            }
         }
         DBFreeResult(hResult);
      }
   }

	m_status = STATUS_NORMAL;

   return success;
}

/**
 * Save object to database
 */
bool Template::saveToDatabase(DB_HANDLE hdb)
{
   lockProperties();

   bool success = saveCommonProperties(hdb);

   if (success && (m_modified & MODIFY_OTHER))
   {
      DB_STATEMENT hStmt;
      if (IsDatabaseRecordExist(hdb, _T("templates"), _T("id"), m_id))
      {
         hStmt = DBPrepare(hdb, _T("UPDATE templates SET version=?,apply_filter=?,flags=? WHERE id=?"));
      }
      else
      {
         hStmt = DBPrepare(hdb, _T("INSERT INTO templates (version,apply_filter,flags,id) VALUES (?,?,?,?)"));
      }
      if (hStmt != NULL)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_dwVersion);
         DBBind(hStmt, 2, DB_SQLTYPE_TEXT, m_applyFilterSource, DB_BIND_STATIC);
         DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_flags);
         DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, m_id);
         success = DBExecute(hStmt);
         DBFreeStatement(hStmt);
      }
      else
      {
         success = false;
      }
   }

   // Save access list
   if (success)
      success = saveACLToDB(hdb);

   unlockProperties();

	if (success && (m_modified & MODIFY_RELATIONS))
	{
		// Update members list
		success = executeQueryOnObject(hdb, _T("DELETE FROM dct_node_map WHERE template_id=?"));
      lockChildList(false);
		if (success && !m_childList->isEmpty())
		{
		   DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO dct_node_map (template_id,node_id) VALUES (?,?)"));
		   if (hStmt != NULL)
		   {
		      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
            for(int i = 0; success && (i < m_childList->size()); i++)
            {
               DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_childList->get(i)->getId());
               success = DBExecute(hStmt);
            }
            DBFreeStatement(hStmt);
		   }
		   else
		   {
		      success = false;
		   }
		}
      unlockChildList();
	}

   // Save data collection items
	if (success && (m_modified & MODIFY_DATA_COLLECTION))
	{
      lockDciAccess(false);
      for(int i = 0; success && (i < m_dcObjects->size()); i++)
         success = m_dcObjects->get(i)->saveToDatabase(hdb);
      unlockDciAccess();
	}

   // Clear modifications flag
	lockProperties();
   m_modified = 0;
	unlockProperties();

   return success;
}

/**
 * Delete object from database
 */
bool Template::deleteFromDatabase(DB_HANDLE hdb)
{
   bool success = NetObj::deleteFromDatabase(hdb);
   if (!success)
      return false;

   if (getObjectClass() == OBJECT_TEMPLATE)
   {
      success = executeQueryOnObject(hdb, _T("DELETE FROM templates WHERE id=?"));
      if (success)
         success = executeQueryOnObject(hdb, _T("DELETE FROM dct_node_map WHERE template_id=?"));
   }
   else
   {
      success = executeQueryOnObject(hdb, _T("DELETE FROM dct_node_map WHERE node_id=?"));
   }

   // Delete DCI configuration
   if (success)
   {
      String listItems, listTables, listAll, listTableThresholds;
      for(int i = 0; i < m_dcObjects->size(); i++)
      {
         DCObject *o = m_dcObjects->get(i);
         if (!listAll.isEmpty())
            listAll.append(_T(','));
         listAll.append(o->getId());
         if (o->getType() == DCO_TYPE_ITEM)
         {
            if (!listItems.isEmpty())
               listItems.append(_T(','));
            listItems.append(o->getId());
         }
         else if (o->getType() == DCO_TYPE_TABLE)
         {
            if (!listTables.isEmpty())
               listTables.append(_T(','));
            listTables.append(o->getId());

            IntegerArray<UINT32> *idList = static_cast<DCTable*>(o)->getThresholdIdList();
            for(int j = 0; j < idList->size(); j++)
            {
               if (!listTableThresholds.isEmpty())
                  listTableThresholds.append(_T(','));
               listTableThresholds.append(idList->get(j));
            }
            delete idList;
         }
      }

      TCHAR query[8192];
      if (!listItems.isEmpty())
      {
         _sntprintf(query, 8192, _T("DELETE FROM thresholds WHERE item_id IN (%s)"), (const TCHAR *)listItems);
         success = DBQuery(hdb, query);
         if (success)
         {
            _sntprintf(query, 8192, _T("DELETE FROM raw_dci_values WHERE item_id IN (%s)"), (const TCHAR *)listItems);
            success = DBQuery(hdb, query);
         }
      }

      if (!listTables.isEmpty())
      {
         _sntprintf(query, 8192, _T("DELETE FROM dc_table_columns WHERE table_id IN (%s)"), (const TCHAR *)listTables);
         success = DBQuery(hdb, query);
         if (success)
         {
            _sntprintf(query, 8192, _T("DELETE FROM dct_thresholds WHERE table_id IN (%s)"), (const TCHAR *)listTables);
            success = DBQuery(hdb, query);
         }
      }

      if (!listTableThresholds.isEmpty())
      {
         _sntprintf(query, 8192, _T("DELETE FROM dct_threshold_conditions WHERE threshold_id IN (%s)"), (const TCHAR *)listTableThresholds);
         success = DBQuery(hdb, query);
         if (success)
         {
            _sntprintf(query, 8192, _T("DELETE FROM dct_threshold_instances WHERE threshold_id IN (%s)"), (const TCHAR *)listTableThresholds);
            success = DBQuery(hdb, query);
         }
      }

      if (!listAll.isEmpty())
      {
         _sntprintf(query, 8192, _T("DELETE FROM dci_schedules WHERE item_id IN (%s)"), (const TCHAR *)listAll);
         success = DBQuery(hdb, query);
         if (success)
         {
            _sntprintf(query, 8192, _T("DELETE FROM dci_access WHERE dci_id IN (%s)"), (const TCHAR *)listAll);
            success = DBQuery(hdb, query);
         }
      }
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
void Template::loadItemsFromDB(DB_HANDLE hdb)
{
	DB_STATEMENT hStmt = DBPrepare(hdb,
	           _T("SELECT item_id,name,source,datatype,polling_interval,retention_time,")
              _T("status,delta_calculation,transformation,template_id,description,")
              _T("instance,template_item_id,flags,resource_id,")
              _T("proxy_node,base_units,unit_multiplier,custom_units_name,")
	           _T("perftab_settings,system_tag,snmp_port,snmp_raw_value_type,")
				  _T("instd_method,instd_data,instd_filter,samples,comments,guid,npe_name, ")
				  _T("instance_retention_time FROM items WHERE node_id=?"));
	if (hStmt != NULL)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
		DB_RESULT hResult = DBSelectPrepared(hStmt);
		if (hResult != NULL)
		{
			int count = DBGetNumRows(hResult);
			for(int i = 0; i < count; i++)
				m_dcObjects->add(new DCItem(hdb, hResult, i, this));
			DBFreeResult(hResult);
		}
		DBFreeStatement(hStmt);
	}

	hStmt = DBPrepare(hdb,
	           _T("SELECT item_id,template_id,template_item_id,name,")
				  _T("description,flags,source,snmp_port,polling_interval,retention_time,")
              _T("status,system_tag,resource_id,proxy_node,perftab_settings,")
              _T("transformation_script,comments,guid,instd_method,instd_data,")
              _T("instd_filter,instance,instance_retention_time FROM dc_tables WHERE node_id=?"));
	if (hStmt != NULL)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
		DB_RESULT hResult = DBSelectPrepared(hStmt);
		if (hResult != NULL)
		{
			int count = DBGetNumRows(hResult);
			for(int i = 0; i < count; i++)
				m_dcObjects->add(new DCTable(hdb, hResult, i, this));
			DBFreeResult(hResult);
		}
		DBFreeStatement(hStmt);
	}
}

/**
 * Add data collection object to node
 */
bool Template::addDCObject(DCObject *object, bool alreadyLocked)
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
	}
   return success;
}

/**
 * Delete data collection object from node
 */
bool Template::deleteDCObject(UINT32 dcObjectId, bool needLock, UINT32 userId)
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
         if(object->hasAccess(userId))
         {
            // Check if it is instance DCI
            if ((object->getType() == DCO_TYPE_ITEM) && (((DCItem *)object)->getInstanceDiscoveryMethod() != IDM_NONE))
            {
               deleteChildDCIs(dcObjectId);

               // Index may be incorrect at this point
               if (m_dcObjects->get(i) != object)
                  i = m_dcObjects->indexOf(object);
            }
            // Destroy item
            nxlog_debug_tag(_T("obj.dc"), 7, _T("Template::DeleteDCObject: deleting DCObject [%u] from object %s [%u]"), dcObjectId, m_name, m_id);
            destroyItem(object, i);
            success = true;
            nxlog_debug_tag(_T("obj.dc"), 7, _T("Template::DeleteDCObject: DCObject deleted from object %s [%u]"), m_name, m_id);
         }
         else
         {
            nxlog_debug_tag(_T("obj.dc"), 6, _T("Template::DeleteDCObject: denied access to DCObject %u for user %u"), dcObjectId, userId);
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
void Template::deleteChildDCIs(UINT32 dcObjectId)
{
   for(int i = 0; i < m_dcObjects->size(); i++)
   {
      DCObject *subObject = m_dcObjects->get(i);
      if (subObject->getTemplateItemId() == dcObjectId)
      {
         nxlog_debug(7, _T("Template::DeleteDCObject: deleting DCObject %d created by DCObject %d instance discovery from object %d"), (int)subObject->getId(), (int)dcObjectId, (int)m_id);
         destroyItem(subObject, i);
         i--;
      }
   }
}

/**
 * Delete DCI object.
 * Deletes or schedules deletion from DB and removes it from index
 * It is assumed that list is already locked
 */
void Template::destroyItem(DCObject *object, int index)
{
   if (object->prepareForDeletion())
   {
      // Physically delete DCI only if it is not busy
      // Busy DCIs will be deleted by data collector
      object->deleteFromDatabase();
      m_dcObjects->remove(index);
   }
   else
   {
      m_dcObjects->unlink(index);
      nxlog_debug_tag(_T("obj.dc"), 7, _T("Template::DeleteItem: destruction of DCO %u delayed"), object->getId());
   }
}

/**
 * Modify data collection object from NXCP message
 */
bool Template::updateDCObject(UINT32 dwItemId, NXCPMessage *pMsg, UINT32 *pdwNumMaps, UINT32 **ppdwMapIndex, UINT32 **ppdwMapId, UINT32 userId)
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
            {
               ((DCItem *)object)->updateFromMessage(pMsg, pdwNumMaps, ppdwMapIndex, ppdwMapId);
               if (((DCItem *)object)->getInstanceDiscoveryMethod() != IDM_NONE)
               {
                  updateInstanceDiscoveryItems((DCItem *)object);
               }
            }
            else
            {
               object->updateFromMessage(pMsg);
            }
            success = true;
         }
         else
         {
            nxlog_debug_tag(_T("obj.dc"), 6, _T("Template::updateDCObject: denied access to DCObject %u for user %u"), dwItemId, userId);
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
void Template::updateInstanceDiscoveryItems(DCItem *dci)
{
   for(int i = 0; i < m_dcObjects->size(); i++)
	{
		DCObject *object = m_dcObjects->get(i);
      if ((object->getType() == DCO_TYPE_ITEM) && (object->getTemplateId() == m_id) && (object->getTemplateItemId() == dci->getId()))
      {
         object->updateFromTemplate(dci);
      }
	}
}

/**
 * Set status for group of DCIs
 */
bool Template::setItemStatus(UINT32 dwNumItems, UINT32 *pdwItemList, int iStatus)
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
 * Lock data collection items list
 */
bool Template::lockDCIList(int sessionId, const TCHAR *pszNewOwner, TCHAR *pszCurrOwner)
{
   bool success;

   lockProperties();
   if (m_dciLockStatus == -1)
   {
      m_dciLockStatus = sessionId;
      m_dciListModified = false;
      nx_strncpy(m_szCurrDCIOwner, pszNewOwner, MAX_SESSION_NAME);
      success = true;
   }
   else
   {
      if (pszCurrOwner != NULL)
         _tcscpy(pszCurrOwner, m_szCurrDCIOwner);
      success = false;
   }
   unlockProperties();
   return success;
}

/**
 * Unlock data collection items list
 */
bool Template::unlockDCIList(int sessionId)
{
   bool success = false;
   bool callChangeHook = false;

   lockProperties();
   if (m_dciLockStatus == sessionId)
   {
      m_dciLockStatus = -1;
      if (m_dciListModified)
      {
         if (getObjectClass() == OBJECT_TEMPLATE)
            m_dwVersion++;
         setModified(MODIFY_OTHER | MODIFY_DATA_COLLECTION);
         callChangeHook = true;
      }
      m_dciListModified = false;
      success = true;
   }
   unlockProperties();

   if (callChangeHook)
      onDataCollectionChange();

   return success;
}

/**
 * Send DCI list to client
 */
void Template::sendItemsToClient(ClientSession *pSession, UINT32 dwRqId)
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
         nxlog_debug_tag(_T("obj.dc"), 6, _T("Template::sendItemsToClient: denied access to DCObject %u for user %u"), dco->getId(), pSession->getUserId());
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
DCObject *Template::getDCObjectById(UINT32 itemId, UINT32 userId, bool lock)
{
   DCObject *object = NULL;

   if (lock)
      lockDciAccess(false);

   for(int i = 0; i < m_dcObjects->size(); i++)
	{
		DCObject *curr = m_dcObjects->get(i);
      if (curr->getId() == itemId)
		{
         if (curr->hasAccess(userId))
            object = curr;
         else
            nxlog_debug_tag(_T("obj.dc"), 6, _T("Template::getDCObjectById: denied access to DCObject %u for user %u"), itemId, userId);
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
DCObject *Template::getDCObjectByTemplateId(UINT32 tmplItemId, UINT32 userId)
{
   DCObject *object = NULL;

   lockDciAccess(false);
   // Check if that item exists
   for(int i = 0; i < m_dcObjects->size(); i++)
	{
		DCObject *curr = m_dcObjects->get(i);
      if (curr->getTemplateItemId() == tmplItemId)
		{
         if (curr->hasAccess(userId))
            object = curr;
         else
            nxlog_debug_tag(_T("obj.dc"), 6, _T("Template::getDCObjectByTemplateId: denied access to DCObject %u for user %u"), object->getId(), userId);
         break;
		}
	}

   unlockDciAccess();
   return object;
}

/**
 * Get item by it's name (case-insensetive)
 */
DCObject *Template::getDCObjectByName(const TCHAR *name, UINT32 userId)
{
   DCObject *object = NULL;

   lockDciAccess(false);
   // Check if that item exists
   for(int i = 0; i < m_dcObjects->size(); i++)
	{
		DCObject *curr = m_dcObjects->get(i);
      if (!_tcsicmp(curr->getName(), name))
		{
         if (curr->hasAccess(userId))
            object = curr;
         else
            nxlog_debug_tag(_T("obj.dc"), 6, _T("Template::getDCObjectByName: denied access to DCObject %u for user %u"), object->getId(), userId);
         break;
		}
	}
   unlockDciAccess();
   return object;
}

/**
 * Get item by it's description (case-insensetive)
 */
DCObject *Template::getDCObjectByDescription(const TCHAR *description, UINT32 userId)
{
   DCObject *object = NULL;

   lockDciAccess(false);
   // Check if that item exists
   for(int i = 0; i < m_dcObjects->size(); i++)
	{
		DCObject *curr = m_dcObjects->get(i);
      if (!_tcsicmp(curr->getDescription(), description))
		{
         if (curr->hasAccess(userId))
            object = curr;
         else
            nxlog_debug_tag(_T("obj.dc"), 6, _T("Template::getDCObjectByDescription: denied access to DCObject %u for user %u"), object->getId(), userId);
         break;
		}
	}
	unlockDciAccess();
   return object;
}

/**
 * Get item by GUID
 */
DCObject *Template::getDCObjectByGUID(const uuid& guid, UINT32 userId, bool lock)
{
   DCObject *object = NULL;

   if (lock)
      lockDciAccess(false);

   // Check if that item exists
   for(int i = 0; i < m_dcObjects->size(); i++)
   {
      DCObject *curr = m_dcObjects->get(i);
      if (guid.equals(curr->getGuid()))
      {
         if (curr->hasAccess(userId))
            object = curr;
         else
            nxlog_debug_tag(_T("obj.dc"), 6, _T("Template::getDCObjectByGUID: denied access to DCObject %u for user %u"), object->getId(), userId);
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
NXSL_Value *Template::getAllDCObjectsForNXSL(const TCHAR *name, const TCHAR *description, UINT32 userID)
{
   NXSL_Array *list = new NXSL_Array();
   lockDciAccess(false);
   for(int i = 0; i < m_dcObjects->size(); i++)
	{
		DCObject *curr = m_dcObjects->get(i);
      if (((name == NULL) || MatchString(name, curr->getName(), false)) &&
          ((description == NULL) || MatchString(description, curr->getDescription(), false)) &&
          curr->hasAccess(userID))
		{
         list->set(list->size(), curr->createNXSLObject());
		}
	}
	unlockDciAccess();
   return new NXSL_Value(list);
}

/**
 * Redefined status calculation for template
 */
void Template::calculateCompoundStatus(BOOL bForcedRecalc)
{
   m_status = STATUS_NORMAL;
}

/**
 * Create NXCP message with object's data
 */
void Template::fillMessageInternal(NXCPMessage *pMsg, UINT32 userId)
{
   NetObj::fillMessageInternal(pMsg, userId);
   pMsg->setField(VID_TEMPLATE_VERSION, m_dwVersion);
	pMsg->setField(VID_FLAGS, m_flags);
	pMsg->setField(VID_AUTOBIND_FILTER, CHECK_NULL_EX(m_applyFilterSource));
}

/**
 * Modify object from NXCP message
 */
UINT32 Template::modifyFromMessageInternal(NXCPMessage *pRequest)
{
   // Skip modifications for subclasses
   if (getObjectClass() != OBJECT_TEMPLATE)
      return NetObj::modifyFromMessageInternal(pRequest);

   // Change template version
   if (pRequest->isFieldExist(VID_TEMPLATE_VERSION))
      m_dwVersion = pRequest->getFieldAsUInt32(VID_TEMPLATE_VERSION);

   // Change flags
   if (pRequest->isFieldExist(VID_FLAGS))
   {
      UINT32 mask = pRequest->isFieldExist(VID_FLAGS_MASK) ? pRequest->getFieldAsUInt32(VID_FLAGS_MASK) : 0xFFFFFFFF;
      m_flags &= ~mask;
      m_flags |= pRequest->getFieldAsUInt32(VID_FLAGS) & mask;
   }

   // Change apply filter
	if (pRequest->isFieldExist(VID_AUTOBIND_FILTER))
	{
		free(m_applyFilterSource);
		delete m_applyFilter;
		m_applyFilterSource = pRequest->getFieldAsString(VID_AUTOBIND_FILTER);
		if (m_applyFilterSource != NULL)
		{
			TCHAR error[256];

			m_applyFilter = NXSLCompile(m_applyFilterSource, error, 256, NULL);
			if (m_applyFilter == NULL)
			{
	         TCHAR buffer[1024];
	         _sntprintf(buffer, 1024, _T("Template::%s::%d"), m_name, m_id);
	         PostEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", buffer, error, m_id);
				nxlog_write(MSG_TEMPLATE_SCRIPT_COMPILATION_ERROR, EVENTLOG_WARNING_TYPE, "dss", m_id, m_name, error);
			}
		}
		else
		{
			m_applyFilter = NULL;
		}
	}

   return NetObj::modifyFromMessageInternal(pRequest);
}

/**
 * Apply template to data collection target
 */
BOOL Template::applyToTarget(DataCollectionTarget *target)
{
   UINT32 *pdwItemList;
   BOOL bErrors = FALSE;

   // Link node to template
   if (!isChild(target->getId()))
   {
      addChild(target);
      target->addParent(this);
   }

   pdwItemList = (UINT32 *)malloc(sizeof(UINT32) * m_dcObjects->size());
   nxlog_debug_tag(_T("obj.dc"), 2, _T("Apply %d items from template \"%s\" to target \"%s\""),
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
   free(pdwItemList);

   target->onDataCollectionChange();

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
void Template::queueUpdate()
{
   lockChildList(false);
   for(int i = 0; i < m_childList->size(); i++)
   {
      NetObj *object = m_childList->get(i);
      if (object->isDataCollectionTarget())
      {
         incRefCount();
         TEMPLATE_UPDATE_INFO *pInfo = (TEMPLATE_UPDATE_INFO *)malloc(sizeof(TEMPLATE_UPDATE_INFO));
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
void Template::queueRemoveFromTarget(UINT32 targetId, bool removeDCI)
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
IntegerArray<UINT32> *Template::getDCIEventsList()
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
StringSet *Template::getDCIScriptList()
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
            nx_strncpy(buffer, name, p - name + 1);
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
 * Create management pack record
 */
void Template::createExportRecord(String &str)
{
   TCHAR guid[48];
   str.appendFormattedString(_T("\t\t<template id=\"%d\">\n\t\t\t<guid>%s</guid>\n\t\t\t<name>%s</name>\n\t\t\t<flags>%d</flags>\n"),
                             m_id, m_guid.toString(guid), (const TCHAR *)EscapeStringForXML2(m_name), m_flags);

   // Path in groups
   StringList path;
   ObjectArray<NetObj> *list = getParentList(OBJECT_TEMPLATEGROUP);
   TemplateGroup *parent = NULL;
   while(list->size() > 0)
   {
      parent = (TemplateGroup *)list->get(0);
      path.add(parent->getName());
      delete list;
      list = parent->getParentList(OBJECT_TEMPLATEGROUP);
   }
   delete list;

   str.append(_T("\t\t\t<path>\n"));
   for(int j = path.size() - 1, id = 1; j >= 0; j--, id++)
   {
      str.append(_T("\t\t\t\t<element id=\""));
      str.append(id);
      str.append(_T("\">"));
      str.append(path.get(j));
      str.append(_T("</element>\n"));
   }
   str.append(_T("\t\t\t</path>\n\t\t\t<dataCollection>\n"));

   lockDciAccess(false);
   for(int i = 0; i < m_dcObjects->size(); i++)
      m_dcObjects->get(i)->createExportRecord(str);
   unlockDciAccess();

   str.append(_T("\t\t\t</dataCollection>\n"));
	lockProperties();
	if (m_applyFilterSource != NULL)
	{
      str.append(_T("\t\t\t<filter>"));
		str.appendPreallocated(EscapeStringForXML(m_applyFilterSource, -1));
      str.append(_T("</filter>\n"));
	}
	unlockProperties();
   str.append(_T("\t\t</template>\n"));
}

/**
 * Enumerate all DCIs
 */
bool Template::enumDCObjects(bool (* pfCallback)(DCObject *, UINT32, void *), void *pArg)
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
void Template::associateItems()
{
	lockDciAccess(false);
	for(int i = 0; i < m_dcObjects->size(); i++)
		m_dcObjects->get(i)->changeBinding(0, this, FALSE);
	unlockDciAccess();
}

/**
 * Prepare template for deletion
 */
void Template::prepareForDeletion()
{
	if (getObjectClass() == OBJECT_TEMPLATE)
	{
		lockChildList(false);
		for(int i = 0; i < m_childList->size(); i++)
		{
		   NetObj *object = m_childList->get(i);
			if (object->isDataCollectionTarget())
				queueRemoveFromTarget(object->getId(), true);
		}
		unlockChildList();
	}
	NetObj::prepareForDeletion();
}

/**
 * Check if template should be automatically applied to given data collection target
 * Returns AutoBindDecision_Bind if applicable, AutoBindDecision_Unbind if not, 
 * AutoBindDecision_Ignore if no change required (script error or no auto apply)
 */
AutoBindDecision Template::isApplicable(DataCollectionTarget *target)
{
	AutoBindDecision result = AutoBindDecision_Ignore;

	NXSL_VM *filter = NULL;
	lockProperties();
	if ((m_flags & TF_AUTO_APPLY) && (m_applyFilter != NULL))
	{
	   filter = new NXSL_VM(new NXSL_ServerEnv());
	   if (!filter->load(m_applyFilter))
	   {
	      TCHAR buffer[1024];
	      _sntprintf(buffer, 1024, _T("Template::%s::%d"), m_name, m_id);
	      PostEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", buffer, filter->getErrorText(), m_id);
	      nxlog_write(MSG_TEMPLATE_SCRIPT_EXECUTION_ERROR, EVENTLOG_WARNING_TYPE, "dss", m_id, m_name, filter->getErrorText());
	      delete_and_null(filter);
	   }
	}
   unlockProperties();

   if (filter == NULL)
      return result;

   filter->setGlobalVariable(_T("$object"), target->createNXSLObject());
   if (target->getObjectClass() == OBJECT_NODE)
      filter->setGlobalVariable(_T("$node"), target->createNXSLObject());
   if (filter->run())
   {
      NXSL_Value *value = filter->getResult();
      if (!value->isNull())
         result = ((value != NULL) && (value->getValueAsInt32() != 0)) ? AutoBindDecision_Bind : AutoBindDecision_Unbind;
   }
   else
   {
      lockProperties();
      TCHAR buffer[1024];
      _sntprintf(buffer, 1024, _T("Template::%s::%d"), m_name, m_id);
      PostEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", buffer, filter->getErrorText(), m_id);
      nxlog_write(MSG_TEMPLATE_SCRIPT_EXECUTION_ERROR, EVENTLOG_WARNING_TYPE, "dss", m_id, m_name, filter->getErrorText());
      unlockProperties();
   }
   delete filter;
	return result;
}

/**
 * Get last (current) DCI values. Moved to Template from DataCollectionTarget to allow
 * simplified creation of DCI selection dialog in management console. For classes not
 * derived from DataCollectionTarget actual values will always be empty strings
 * with data type DCI_DT_NULL.
 */
UINT32 Template::getLastValues(NXCPMessage *msg, bool objectTooltipOnly, bool overviewOnly, bool includeNoValueObjects, UINT32 userId)
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

/**
 * Called when data collection configuration changed
 */
void Template::onDataCollectionChange()
{
   // Do not queue updates for subclasses
   if (getObjectClass() == OBJECT_TEMPLATE)
      queueUpdate();
}

/**
 * Update template from import
 */
void Template::updateFromImport(ConfigEntry *config)
{
   // Name and version
   lockProperties();
   m_dwVersion = config->getSubEntryValueAsUInt(_T("version"), 0, m_dwVersion);
   m_flags = config->getSubEntryValueAsUInt(_T("flags"), 0, m_flags);
   unlockProperties();

   // Auto-apply filter
   setAutoApplyFilter(config->getSubEntryValue(_T("filter")));

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
         DCObject *curr = !guid.isNull() ? getDCObjectByGUID(guid, 0, false) : NULL;
         if ((curr != NULL) && (curr->getType() == DCO_TYPE_ITEM))
         {
            curr->updateFromImport(e);
         }
         else
         {
            m_dcObjects->add(new DCItem(e, this));
         }
         guidList.add(new uuid(guid));
      }
      delete dcis;

      ObjectArray<ConfigEntry> *dctables = dcRoot->getSubEntries(_T("dctable#*"));
      for(int i = 0; i < dctables->size(); i++)
      {
         ConfigEntry *e = dctables->get(i);
         uuid guid = e->getSubEntryValueAsUUID(_T("guid"));
         DCObject *curr = !guid.isNull() ? getDCObjectByGUID(guid, 0, false) : NULL;
         if ((curr != NULL) && (curr->getType() == DCO_TYPE_TABLE))
         {
            curr->updateFromImport(e);
         }
         else
         {
            m_dcObjects->add(new DCTable(e, this));
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
json_t *Template::toJson()
{
   json_t *root = NetObj::toJson();
   json_object_set_new(root, "dcObjects", json_object_array(m_dcObjects));
   json_object_set_new(root, "version", json_integer(m_dwVersion));
   json_object_set_new(root, "flags", json_integer(m_flags));
   json_object_set_new(root, "applyFilter", json_string_t(m_applyFilterSource));
   return root;
}
