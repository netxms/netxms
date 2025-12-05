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
** File: dctarget.cpp
**
**/

#include "nxcore.h"
#include <npe.h>
#include <nxcore_websvc.h>
#include <netxms_maps.h>
#include <nms_users.h>
#include <netxms-regex.h>

/**
 * Data collector thread pool
 */
extern ThreadPool *g_dataCollectorThreadPool;

/**
 * Data collector worker
 */
void DataCollector(const shared_ptr<DCObject>& dcObject);

/**
 * Throttle housekeeper if needed. Returns false if shutdown time has arrived and housekeeper process should be aborted.
 */
bool ThrottleHousekeeper();

/**
 * Lock IDATA writes
 */
void LockIDataWrites();

/**
 * Unlock IDATA writes
 */
void UnlockIDataWrites();

/**
 * Poller thread pool
 */
extern ThreadPool *g_pollerThreadPool;

/**
 * Default constructor
 */
DataCollectionTarget::DataCollectionTarget(uint32_t pollableFlags) : super(), Pollable(this, pollableFlags | Pollable::INSTANCE_DISCOVERY),
      m_deletedItems(0, 32), m_deletedTables(0, 32), m_geoAreas(0, 16), m_proxyLoadFactor(0)
{
   m_geoLocationControlMode = GEOLOCATION_NO_CONTROL;
   m_geoLocationRestrictionsViolated = false;
   m_instanceDiscoveryPending = false;
   m_webServiceProxy = 0;
}

/**
 * Constructor for creating new data collection capable objects
 */
DataCollectionTarget::DataCollectionTarget(const TCHAR *name, uint32_t pollableFlags) : super(name), Pollable(this, pollableFlags | Pollable::INSTANCE_DISCOVERY),
      m_deletedItems(0, 32), m_deletedTables(0, 32), m_geoAreas(0, 16), m_proxyLoadFactor(0)
{
   m_geoLocationControlMode = GEOLOCATION_NO_CONTROL;
   m_geoLocationRestrictionsViolated = false;
   m_instanceDiscoveryPending = false;
   m_webServiceProxy = 0;
}

/**
 * Delete object from database
 */
bool DataCollectionTarget::deleteFromDatabase(DB_HANDLE hdb)
{
   bool success = executeQueryOnObject(hdb, _T("DELETE FROM dct_node_map WHERE node_id=?"));

   // TSDB: to avoid heavy query on idata tables let collected data expire instead of deleting it immediately
   if (success && ((g_dbSyntax != DB_SYNTAX_TSDB) || !(g_flags & AF_SINGLE_TABLE_PERF_DATA)))
   {
      TCHAR query[256];
      _sntprintf(query, 256, (g_flags & AF_SINGLE_TABLE_PERF_DATA) ? _T("DELETE FROM idata WHERE item_id IN (SELECT item_id FROM items WHERE node_id=%u)") : _T("DROP TABLE idata_%u"), m_id);
      QueueSQLRequest(query);

      _sntprintf(query, 256, (g_flags & AF_SINGLE_TABLE_PERF_DATA) ? _T("DELETE FROM tdata WHERE item_id IN (SELECT item_id FROM dc_tables WHERE node_id=%u)") : _T("DROP TABLE tdata_%u"), m_id);
      QueueSQLRequest(query);
   }

   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM dc_targets WHERE id=?"));

   if (success)
      success = super::deleteFromDatabase(hdb);

   return success;
}

/**
 * Create NXCP message with object's data
 */
void DataCollectionTarget::fillMessageLocked(NXCPMessage *msg, uint32_t userId)
{
   super::fillMessageLocked(msg, userId);
   msg->setField(VID_GEOLOCATION_CTRL_MODE, static_cast<int16_t>(m_geoLocationControlMode));
   msg->setFieldFromInt32Array(VID_GEO_AREAS, m_geoAreas);
   msg->setField(VID_WEB_SERVICE_PROXY, m_webServiceProxy);
}

/**
 * Create NXCP message with object's data - stage 2
 */
void DataCollectionTarget::fillMessageUnlockedEssential(NXCPMessage *msg, uint32_t userId)
{
   super::fillMessageUnlockedEssential(msg, userId);

   // Sent all DCIs marked for display on overview page or in tooltips
   uint32_t fieldIdOverview = VID_OVERVIEW_DCI_LIST_BASE;
   uint32_t countOverview = 0;
   uint32_t fieldIdTooltip = VID_TOOLTIP_DCI_LIST_BASE;
   uint32_t countTooltip = 0;
   readLockDciAccess();
   for(int i = 0; i < m_dcObjects.size(); i++)
	{
      DCObject *dci = m_dcObjects.get(i);
      if ((dci->getType() == DCO_TYPE_ITEM) &&
          (dci->getStatus() == ITEM_STATUS_ACTIVE) &&
          (dci->getInstanceDiscoveryMethod() == IDM_NONE) &&
          dci->hasAccess(userId))
		{
         if  (dci->isShowInObjectOverview())
         {
            countOverview++;
            static_cast<DCItem*>(dci)->fillLastValueSummaryMessage(msg, fieldIdOverview);
            fieldIdOverview += 50;
         }
         if  (dci->isShowOnObjectTooltip())
         {
            countTooltip++;
            static_cast<DCItem*>(dci)->fillLastValueSummaryMessage(msg, fieldIdTooltip);
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
uint32_t DataCollectionTarget::modifyFromMessageInternal(const NXCPMessage& msg, ClientSession *session)
{
   if (msg.isFieldExist(VID_GEOLOCATION_CTRL_MODE))
      m_geoLocationControlMode = static_cast<GeoLocationControlMode>(msg.getFieldAsInt16(VID_GEOLOCATION_CTRL_MODE));

   if (msg.isFieldExist(VID_GEO_AREAS))
      msg.getFieldAsInt32Array(VID_GEO_AREAS, &m_geoAreas);

   if (msg.isFieldExist(VID_WEB_SERVICE_PROXY))
      m_webServiceProxy = msg.getFieldAsUInt32(VID_WEB_SERVICE_PROXY);

   return super::modifyFromMessageInternal(msg, session);
}

/**
 * Create object from database data
 */
bool DataCollectionTarget::loadFromDatabase(DB_HANDLE hdb, uint32_t id, DB_STATEMENT *preparedStatements)
{
   DB_STATEMENT hStmt = PrepareObjectLoadStatement(hdb, preparedStatements, LSI_DC_TARGET,
      _T("SELECT geolocation_ctrl_mode,geo_areas,web_service_proxy FROM dc_targets WHERE id=?"));
   if (hStmt == nullptr)
      return false;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
   DB_RESULT hResult = DBSelectPrepared(hStmt);
   if (hResult == nullptr)
      return false;

   if (DBGetNumRows(hResult) > 0)
   {
      m_geoLocationControlMode = static_cast<GeoLocationControlMode>(DBGetFieldLong(hResult, 0, 0));

      TCHAR areas[2000];
      DBGetField(hResult, 0, 1, areas, 2000);
      Trim(areas);
      if (*areas != 0)
      {
         TCHAR *curr = areas;
         while(true)
         {
            TCHAR *next = _tcschr(curr, _T(','));
            if (next != nullptr)
               *next = 0;

            TCHAR *eptr;
            uint32_t id = _tcstoul(curr, &eptr, 10);
            if ((id != 0) && (*eptr == 0))
               m_geoAreas.add(id);

            if (next == nullptr)
               break;
            curr = next + 1;
         }
      }

      m_webServiceProxy = DBGetFieldULong(hResult, 0, 2);
   }
   DBFreeResult(hResult);

   return true;
}

/**
 * Save object to database
 */
bool DataCollectionTarget::saveToDatabase(DB_HANDLE hdb)
{
   bool success = super::saveToDatabase(hdb);

   if (success && (m_modified & MODIFY_DC_TARGET))
   {
      static const TCHAR *columns[] = {_T("geolocation_ctrl_mode"), _T("geo_areas"), _T("web_service_proxy"), nullptr };
      DB_STATEMENT hStmt = DBPrepareMerge(hdb, _T("dc_targets"), _T("id"), m_id, columns);
      if (hStmt != nullptr)
      {
         StringBuffer areas;
         lockProperties();
         for(int i = 0; i < m_geoAreas.size(); i++)
         {
            areas.append(m_geoAreas.get(i));
            areas.append(_T(','));
         }
         unlockProperties();
         areas.shrink();

         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, static_cast<int32_t>(m_geoLocationControlMode));
         DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, areas, DB_BIND_STATIC);
         DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_webServiceProxy);
         DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, m_id);
         success = DBExecute(hStmt);
         DBFreeStatement(hStmt);
      }
      else
      {
         success = false;
      }
   }

   if ((success) && (m_modified & MODIFY_DATA_COLLECTION))
      success = saveDCIListForCleanup(hdb);

   return success;
}

/**
 * Handler for object deletion notification
 */
void DataCollectionTarget::onObjectDelete(const NetObj& object)
{
   uint32_t objectId = object.getId();
   lockProperties();
   if (m_webServiceProxy == objectId)
   {
      m_webServiceProxy = 0;
      nxlog_debug_tag(DEBUG_TAG_OBJECT_RELATIONS, 3, _T("DataCollectionTarget::onObjectDelete(%s [%u]): Web Service proxy node %s [%u] deleted"), m_name, m_id, object.getName(), objectId);
      setModified(MODIFY_DC_TARGET);
   }
   unlockProperties();
   super::onObjectDelete(object);
}

/**
 * Update cache for all DCI's
 */
void DataCollectionTarget::updateDciCache()
{
   readLockDciAccess();
   for(int i = 0; i < m_dcObjects.size(); i++)
   {
      m_dcObjects.get(i)->loadCache();
   }
   unlockDciAccess();
}

/**
 * Calculate DCI cutoff time for cleaning expired DCI data using TSDB drop_chunks() function
 */
void DataCollectionTarget::calculateDciCutoffTimes(time_t *cutoffTimeIData, time_t *cutoffTimeTData)
{
   time_t now = time(nullptr);

   readLockDciAccess();
   for(int i = 0; i < m_dcObjects.size(); i++)
   {
      DCObject *o = m_dcObjects.get(i);
      DCObjectStorageClass sclass = o->getStorageClass();
      if (sclass == DCObjectStorageClass::DEFAULT)
         continue;

      time_t *cutoffTime = (o->getType() == DCO_TYPE_ITEM) ? &cutoffTimeIData[static_cast<int>(sclass) - 1] : &cutoffTimeTData[static_cast<int>(sclass) - 1];
      if ((*cutoffTime == 0) || (*cutoffTime > now - o->getEffectiveRetentionTime() * 86400))
         *cutoffTime = now - o->getEffectiveRetentionTime() * 86400;
   }
   unlockDciAccess();
}

/**
 * Clean expired DCI data
 */
void DataCollectionTarget::cleanDCIData(DB_HANDLE hdb)
{
   StringBuffer queryItems = _T("DELETE FROM idata");
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

   StringBuffer queryTables = _T("DELETE FROM tdata");
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
   time_t now = time(nullptr);

   readLockDciAccess();

   // Check if all DCIs has same retention time
   bool sameRetentionTimeItems = true;
   bool sameRetentionTimeTables = true;
   int retentionTimeItems = -1;
   int retentionTimeTables = -1;
   for(int i = 0; (i < m_dcObjects.size()) && (sameRetentionTimeItems || sameRetentionTimeTables); i++)
   {
      DCObject *o = m_dcObjects.get(i);
      if (!o->isDataStorageEnabled())
         continue;   // Ignore "do not store" objects

      if (o->getType() == DCO_TYPE_ITEM)
      {
         if (retentionTimeItems == -1)
         {
            retentionTimeItems = o->getEffectiveRetentionTime();
         }
         else if (retentionTimeItems != o->getEffectiveRetentionTime())
         {
            sameRetentionTimeItems = false;
         }
      }
      else if (o->getType() == DCO_TYPE_TABLE)
      {
         if (retentionTimeTables == -1)
         {
            retentionTimeTables = o->getEffectiveRetentionTime();
         }
         else if (retentionTimeTables != o->getEffectiveRetentionTime())
         {
            sameRetentionTimeTables = false;
         }
      }
   }

   if (!sameRetentionTimeItems || !sameRetentionTimeTables)
   {
      if ((g_dbSyntax == DB_SYNTAX_PGSQL) || (g_dbSyntax == DB_SYNTAX_MYSQL) || (g_dbSyntax == DB_SYNTAX_SQLITE))
      {
         HashMap<int, StringBuffer> itemGroups(Ownership::True);
         HashMap<int, StringBuffer> tableGroups(Ownership::True);
         for(int i = 0; i < m_dcObjects.size(); i++)
         {
            DCObject *o = m_dcObjects.get(i);
            if (!o->isDataStorageEnabled())
               continue;   // Ignore "do not store" objects

            if ((o->getType() == DCO_TYPE_ITEM) && !sameRetentionTimeItems)
            {
               StringBuffer *group = itemGroups.get(o->getEffectiveRetentionTime());
               if (group == nullptr)
               {
                  group = new StringBuffer();
                  group->append(o->getId());
                  itemGroups.set(o->getEffectiveRetentionTime(), group);
               }
               else
               {
                  group->append(_T(','));
                  group->append(o->getId());
               }
            }
            else if ((o->getType() == DCO_TYPE_TABLE) && !sameRetentionTimeTables)
            {
               StringBuffer *group = tableGroups.get(o->getEffectiveRetentionTime());
               if (group == nullptr)
               {
                  group = new StringBuffer();
                  group->append(o->getId());
                  tableGroups.set(o->getEffectiveRetentionTime(), group);
               }
               else
               {
                  group->append(_T(','));
                  group->append(o->getId());
               }
            }
         }

         itemGroups.forEach(
            [&queryItems, &itemCount, now] (const int retentionTime, StringBuffer *idList) -> EnumerationCallbackResult
            {
               if (itemCount > 0)
                  queryItems.append(_T(" OR "));
               queryItems.append(_T("(idata_timestamp<"));
               queryItems.append(static_cast<int64_t>(now - retentionTime * 86400));
               queryItems.append(_T(" AND item_id IN ("));
               queryItems.append(*idList);
               queryItems.append(_T("))"));
               itemCount++;
               return _CONTINUE;
            });

         tableGroups.forEach(
            [&queryTables, &tableCount, now] (const int retentionTime, StringBuffer *idList) -> EnumerationCallbackResult
            {
               if (tableCount > 0)
                  queryTables.append(_T(" OR "));
               queryTables.append(_T("(tdata_timestamp<"));
               queryTables.append(static_cast<int64_t>(now - retentionTime * 86400));
               queryTables.append(_T(" AND item_id IN ("));
               queryTables.append(*idList);
               queryTables.append(_T("))"));
               tableCount++;
               return _CONTINUE;
            });
      }
      else
      {
         for(int i = 0; i < m_dcObjects.size(); i++)
         {
            DCObject *o = m_dcObjects.get(i);
            if (!o->isDataStorageEnabled())
               continue;   // Ignore "do not store" objects

            if ((o->getType() == DCO_TYPE_ITEM) && !sameRetentionTimeItems)
            {
               if (itemCount > 0)
                  queryItems.append(_T(" OR "));
               queryItems.append(_T("(item_id="));
               queryItems.append(o->getId());
               queryItems.append(_T(" AND idata_timestamp<"));
               queryItems.append(static_cast<int64_t>(now - o->getEffectiveRetentionTime() * 86400));
               queryItems.append(_T(')'));
               itemCount++;
            }
            else if ((o->getType() == DCO_TYPE_TABLE) && !sameRetentionTimeTables)
            {
               if (tableCount > 0)
                  queryTables.append(_T(" OR "));
               queryTables.append(_T("(item_id="));
               queryTables.append(o->getId());
               queryTables.append(_T(" AND tdata_timestamp<"));
               queryTables.append(static_cast<int64_t>(now - o->getEffectiveRetentionTime() * 86400));
               queryTables.append(_T(')'));
               tableCount++;
            }
         }
      }
   }
   unlockDciAccess();

   if (sameRetentionTimeItems && (retentionTimeItems != -1))
   {
      queryItems.append(_T("idata_timestamp<"));
      queryItems.append(static_cast<int64_t>(now - retentionTimeItems * 86400));
      itemCount++;   // Indicate that query should be run
   }

   if (sameRetentionTimeTables && (retentionTimeTables != -1))
   {
      queryTables.append(_T("tdata_timestamp<"));
      queryTables.append(static_cast<int64_t>(now - retentionTimeTables * 86400));
      tableCount++;   // Indicate that query should be run
   }

   lockProperties();
   for(int i = 0; i < m_deletedItems.size(); i++)
   {
      if (itemCount > 0)
         queryItems.append(_T(" OR "));
      queryItems.append(_T("item_id="));
      queryItems.append(m_deletedItems.get(i));
      itemCount++;
   }
   m_deletedItems.clear();

   for(int i = 0; i < m_deletedTables.size(); i++)
   {
      if (tableCount > 0)
         queryTables.append(_T(" OR "));
      queryTables.append(_T("item_id="));
      queryTables.append(m_deletedTables.get(i));
      tableCount++;
   }
   m_deletedTables.clear();
   setModified(MODIFY_DATA_COLLECTION, false);  //To update cleanup lists in database
   unlockProperties();

   if (itemCount > 0)
   {
      LockIDataWrites();
      nxlog_debug_tag(_T("housekeeper"), 6, _T("DataCollectionTarget::cleanDCIData(%s [%d]): running query \"%s\""), m_name, m_id, queryItems.cstr());
      DBQuery(hdb, queryItems);
      UnlockIDataWrites();
      if (!ThrottleHousekeeper())
         return;
   }

   if (tableCount > 0)
   {
      nxlog_debug_tag(_T("housekeeper"), 6, _T("DataCollectionTarget::cleanDCIData(%s [%d]): running query \"%s\""), m_name, m_id, queryTables.cstr());
      DBQuery(hdb, queryTables);
   }
}

/**
 * Queue prediction engine training when necessary
 */
void DataCollectionTarget::queuePredictionEngineTraining()
{
   readLockDciAccess();
   for(int i = 0; i < m_dcObjects.size(); i++)
   {
      DCObject *o = m_dcObjects.get(i);
      if (o->getType() == DCO_TYPE_ITEM)
      {
         DCItem *dci = static_cast<DCItem*>(o);
         if (dci->getPredictionEngine()[0] != 0)
         {
            PredictionEngine *e = FindPredictionEngine(dci->getPredictionEngine());
            if ((e != nullptr) && e->requiresTraining())
            {
               QueuePredictionEngineTraining(e, dci);
            }
         }
      }
   }
   unlockDciAccess();
}

/**
 * Schedule cleanup of DCI data after DCI deletion
 */
void DataCollectionTarget::scheduleItemDataCleanup(uint32_t dciId)
{
   lockProperties();
   if (!m_deletedItems.contains(dciId))
   {
      m_deletedItems.add(dciId);
   }
   else
   {
      nxlog_write_tag(NXLOG_WARNING, _T("obj.dc"), _T("Internal structure inconsistency - attempt to add DCI [%u] to delete list for object %s [%u] but it is already added"), dciId, m_name, m_id);
   }
   unlockProperties();
}

/**
 * Schedule cleanup of table DCI data after DCI deletion
 */
void DataCollectionTarget::scheduleTableDataCleanup(uint32_t dciId)
{
   lockProperties();
   if (!m_deletedTables.contains(dciId))
   {
      m_deletedTables.add(dciId);
   }
   else
   {
      nxlog_write_tag(NXLOG_WARNING, _T("obj.dc"), _T("Internal structure inconsistency - attempt to add table DCI [%u] to delete list for object %s [%u] but it is already added"), dciId, m_name, m_id);
   }
   unlockProperties();
}

/**
 * Save DCI list that should be cleared
 */
bool DataCollectionTarget::saveDCIListForCleanup(DB_HANDLE hdb)
{
   bool success = executeQueryOnObject(hdb, _T("DELETE FROM dci_delete_list WHERE node_id=?"));

   lockProperties();
   if (success && (!m_deletedItems.isEmpty() || !m_deletedTables.isEmpty()))
   {
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO dci_delete_list (node_id,dci_id,type) VALUES (?,?,?)"), (m_deletedItems.size() + m_deletedTables.size()) > 1);
      if (hStmt == nullptr)
         return false;

      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
      DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, _T("i"), 1);
      for(int i = 0; i < m_deletedItems.size() && success; i++)
      {
         DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_deletedItems.get(i));
         success = DBExecute(hStmt);
      }

      DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, _T("t"), 1);
      for(int i = 0; i < m_deletedTables.size() && success; i++)
      {
         DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_deletedTables.get(i));
         success = DBExecute(hStmt);
      }
      DBFreeStatement(hStmt);
   }
   unlockProperties();

   return success;
}

/**
 * Load DCI list that should be cleared
 */
void DataCollectionTarget::loadDCIListForCleanup(DB_HANDLE hdb)
{
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT dci_id FROM dci_delete_list WHERE type='i' AND node_id=?"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != nullptr)
      {
         int count = DBGetNumRows(hResult);
         for(int i = 0; i < count; i++)
         {
            m_deletedItems.add(DBGetFieldULong(hResult, i, 0));
         }
         DBFreeResult(hResult);
      }
      DBFreeStatement(hStmt);
   }

   hStmt = DBPrepare(hdb, _T("SELECT dci_id FROM dci_delete_list WHERE type='t' AND node_id=?"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != nullptr)
      {
         int count = DBGetNumRows(hResult);
         for(int i = 0; i < count; i++)
            m_deletedTables.add(DBGetFieldULong(hResult, i, 0));
         DBFreeResult(hResult);
      }
      DBFreeStatement(hStmt);
   }
}

/**
 * Get last value of DCI (either table or single value)
 */
uint32_t DataCollectionTarget::getDciLastValue(uint32_t dciId, NXCPMessage *msg)
{
   uint32_t rcc = RCC_INVALID_DCI_ID;

   readLockDciAccess();

   for(int i = 0; i < m_dcObjects.size(); i++)
   {
      DCObject *object = m_dcObjects.get(i);
      if (object->getId() == dciId)
      {
         msg->setField(VID_DCOBJECT_TYPE, static_cast<int16_t>(object->getType()));
         object->fillLastValueMessage(msg);
         rcc = RCC_SUCCESS;
         break;
      }
   }

   unlockDciAccess();
   return rcc;
}

/**
 * Get active threshold severity for given DCI
 */
int DataCollectionTarget::getDciThresholdSeverity(uint32_t dciId)
{
   int severity = 0;

   readLockDciAccess();

   for(int i = 0; i < m_dcObjects.size(); i++)
   {
      DCObject *object = m_dcObjects.get(i);
      if (object->getId() == dciId)
      {
         if (object->getType() == DCO_TYPE_TABLE)
         {
            //TODO: add DCTable support
         }
         else
         {
            severity = static_cast<DCItem*>(object)->getThresholdSeverity();
         }
         break;
      }
   }

   unlockDciAccess();
   return severity;
}

/**
 * Get last collected values of given table
 */
uint32_t DataCollectionTarget::getTableLastValue(uint32_t dciId, NXCPMessage *msg)
{
   uint32_t rcc = RCC_INVALID_DCI_ID;

   readLockDciAccess();

   for(int i = 0; i < m_dcObjects.size(); i++)
	{
		DCObject *object = m_dcObjects.get(i);
		if (object->getId() == dciId)
		{
		   if (object->getType() == DCO_TYPE_TABLE)
		   {
		      static_cast<DCTable*>(object)->fillLastValueMessage(msg);
		      rcc = RCC_SUCCESS;
		   }
		   else
		   {
		      rcc = RCC_INCOMPATIBLE_OPERATION;
		   }
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
bool DataCollectionTarget::applyTemplateItem(uint32_t templateId, DCObject *dcObject)
{
   bool bResult = true;

   writeLockDciAccess();	// write lock

   nxlog_debug_tag(_T("obj.dc"), 5, _T("Applying data collection object \"%s\" to target \"%s\""), dcObject->getName().cstr(), m_name);

   // Check if that template item exists
	int i;
   for(i = 0; i < m_dcObjects.size(); i++)
      if ((m_dcObjects.get(i)->getTemplateId() == templateId) &&
          (m_dcObjects.get(i)->getTemplateItemId() == dcObject->getId()))
         break;   // Item with specified id already exist

   if (i == m_dcObjects.size())
   {
      // New item from template, just add it
		DCObject *newObject = dcObject->clone();
      newObject->setTemplateId(templateId, dcObject->getId());
      newObject->changeBinding(CreateUniqueId(IDG_ITEM), self(), TRUE);
      bResult = addDCObject(newObject, true);
   }
   else
   {
      // Update existing item unless it is disabled
      DCObject *curr = m_dcObjects.get(i);
      curr->updateFromTemplate(dcObject);
      NotifyClientsOnDCIUpdate(*this, curr);
      if (curr->getInstanceDiscoveryMethod() != IDM_NONE)
      {
         updateInstanceDiscoveryItems(curr);
      }
   }

   unlockDciAccess();

	if (bResult)
		setModified(MODIFY_DATA_COLLECTION, false);
   return bResult;
}

/**
 * Clean deleted template items from target's DCI list
 * Arguments is template id and list of valid template item ids.
 * all items related to given template and not presented in list should be deleted.
 */
void DataCollectionTarget::cleanDeletedTemplateItems(uint32_t templateId, const IntegerArray<uint32_t>& dciList)
{
   writeLockDciAccess();  // write lock

   IntegerArray<uint32_t> deleteList;
   for(int i = 0; i < m_dcObjects.size(); i++)
      if (m_dcObjects.get(i)->getTemplateId() == templateId)
      {
         int j;
         for(j = 0; j < dciList.size(); j++)
            if (m_dcObjects.get(i)->getTemplateItemId() == dciList.get(j))
               break;

         // Delete DCI if it's not in list
         if (j == dciList.size())
            deleteList.add(m_dcObjects.get(i)->getId());
      }

   for(int i = 0; i < deleteList.size(); i++)
      deleteDCObject(deleteList.get(i), false);

   unlockDciAccess();
}

/**
 * Unbind data collection target from template, i.e either remove DCI
 * association with template or remove these DCIs at all
 */
void DataCollectionTarget::onTemplateRemove(const shared_ptr<DataCollectionOwner>& templateObject, bool removeDCI)
{
   if ((getObjectClass() == OBJECT_NODE) && (templateObject->getObjectClass() == OBJECT_TEMPLATE))
   {
      static_cast<Template&>(*templateObject).removeAllPolicies(static_cast<Node*>(this));
   }

   uint32_t templateId = templateObject->getId();
   if (removeDCI)
   {
      writeLockDciAccess();  // write lock

      uint32_t *deleteList = MemAllocArray<uint32_t>(m_dcObjects.size());
		int numDeleted = 0;

		int i;
      for(i = 0; i < m_dcObjects.size(); i++)
         if (m_dcObjects.get(i)->getTemplateId() == templateId)
         {
            deleteList[numDeleted++] = m_dcObjects.get(i)->getId();
         }

		for(i = 0; i < numDeleted; i++)
			deleteDCObject(deleteList[i], false);

      unlockDciAccess();
		MemFree(deleteList);
   }
   else
   {
      readLockDciAccess();

      for(int i = 0; i < m_dcObjects.size(); i++)
         if (m_dcObjects.get(i)->getTemplateId() == templateId)
         {
            m_dcObjects.get(i)->setTemplateId(0, 0);
         }

      unlockDciAccess();
   }
}

/**
 * Get list of DCIs to be shown on performance tab
 */
uint32_t DataCollectionTarget::getPerfTabDCIList(NXCPMessage *msg, uint32_t userId)
{
	readLockDciAccess();

	uint32_t fieldId = VID_SYSDCI_LIST_BASE, count = 0;
   for (int i = 0; i < m_dcObjects.size(); i++)
	{
		DCObject *object = m_dcObjects.get(i);
      if ((object->getPerfTabSettings() != nullptr) &&
          object->hasValue() &&
          (object->getStatus() == ITEM_STATUS_ACTIVE) &&
          object->matchClusterResource() &&
          object->hasAccess(userId))
		{
			msg->setField(fieldId, object->getId());
			msg->setField(fieldId+1, object->getDescription());
			msg->setField(fieldId+2, static_cast<uint16_t>(object->getStatus()));
			msg->setField(fieldId+3, object->getPerfTabSettings());
			msg->setField(fieldId+4, static_cast<uint16_t>(object->getType()));
			msg->setField(fieldId+5, object->getTemplateItemId());
         msg->setField(fieldId+6, object->getInstanceDiscoveryData());
         msg->setField(fieldId+7, object->getInstanceName());
         shared_ptr<DCObject> src = getDCObjectById(object->getTemplateItemId(), userId, false);
         msg->setField(fieldId+8, (src != nullptr) ? src->getTemplateItemId() : 0);

         fieldId+=50;
			count++;
		}
	}
   msg->setField(VID_NUM_ITEMS, count);

	unlockDciAccess();
   return RCC_SUCCESS;
}

/**
 * Get threshold violation summary into NXCP message
 */
uint32_t DataCollectionTarget::getThresholdSummary(NXCPMessage *msg, uint32_t baseId, uint32_t userId)
{
   uint32_t fieldId = baseId;

	msg->setField(fieldId++, m_id);
	uint32_t countId = fieldId++;
	uint32_t count = 0;

	readLockDciAccess();
   for(int i = 0; i < m_dcObjects.size(); i++)
	{
		DCObject *object = m_dcObjects.get(i);
		if ((object->getType() == DCO_TYPE_ITEM) && (object->getStatus() == ITEM_STATUS_ACTIVE) && object->hasValue() && object->hasAccess(userId) && static_cast<DCItem*>(object)->hasActiveThreshold())
      {
         static_cast<DCItem*>(object)->fillLastValueSummaryMessage(msg, fieldId);
         fieldId += 50;
         count++;
      }
	}
	unlockDciAccess();
	msg->setField(countId, count);
   return fieldId;
}

/**
 * Process new DCI value
 */
bool DataCollectionTarget::processNewDCValue(const shared_ptr<DCObject>& dcObject, Timestamp timestamp, const wchar_t *itemValue, const shared_ptr<Table>& tableValue, bool allowPastDataPoints)
{
   if (dcObject->getLastValueTimestamp() == timestamp)
      return true;  // duplicate timestamp and/or value, silently ignore it

   bool updateStatus;
	bool success = (dcObject->getType() == DCO_TYPE_ITEM) ?
	         static_cast<DCItem&>(*dcObject).processNewValue(timestamp, itemValue, &updateStatus, allowPastDataPoints) :
	         static_cast<DCTable&>(*dcObject).processNewValue(timestamp, tableValue, &updateStatus, allowPastDataPoints);
	if (!success)
	{
      // value processing failed, convert to data collection error
      dcObject->processNewError(false);
	}
	if (updateStatus)
	{
      calculateCompoundStatus(false);
   }
   return success;
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

   time_t currTime = time(nullptr);

   bool requireConnectivity = getCustomAttributeAsBoolean(L"SysConfig:DataCollection.Scheduler.RequireConnectivity", (g_flags & AF_DC_SCHEDULER_REQUIRES_CONNECTIVITY) != 0);

   readLockDciAccess();
   for(int i = 0; i < m_dcObjects.size(); i++)
   {
		DCObject *object = m_dcObjects.get(i);
      if (m_dcObjects.get(i)->isReadyForPolling(currTime))
      {
         object->setBusyFlag();

         if ((object->getDataSource() == DS_NATIVE_AGENT) ||
             (object->getDataSource() == DS_WINPERF) ||
             (object->getDataSource() == DS_SNMP_AGENT) ||
             (object->getDataSource() == DS_SSH) ||
             (object->getDataSource() == DS_MODBUS) ||
             (object->getDataSource() == DS_SMCLP))
         {
            uint32_t sourceNodeId = getEffectiveSourceNode(object);
            if (requireConnectivity)
            {
               Node *target;
               shared_ptr<NetObj> sourceNode;
               if (sourceNodeId != 0)
               {
                  sourceNode = FindObjectById(sourceNodeId, OBJECT_NODE);
                  target = static_cast<Node*>(sourceNode.get());
               }
               else if (getObjectClass() == OBJECT_NODE)
               {
                  target = static_cast<Node*>(this);
               }
               else
               {
                  // skip connectivity check if target is not a node
                  target = nullptr;
               }

               if (target != nullptr)
               {
                  if (((object->getDataSource() == DS_NATIVE_AGENT) || (object->getDataSource() == DS_WINPERF)) && (!target->isNativeAgent() || (target->getState() & NSF_AGENT_UNREACHABLE)))
                  {
                     nxlog_debug_tag(_T("obj.dc.queue"), 8, _T("DataCollectionTarget(%s)->QueueItemsForPolling(): item %d \"%s\" skipped because agent is unreachable"),
                              m_name, object->getId(), object->getName().cstr());
                     // Set next poll time to be at least half of status polling interval later to avoid re-checking too often
                     object->setNextPollTime(currTime + g_statusPollingInterval / 2);
                     object->clearBusyFlag();
                     continue;  // Skip polling if agent is unreachable
                  }
                  if ((object->getDataSource() == DS_SNMP_AGENT) && (!target->isSNMPSupported() || (target->getState() & NSF_SNMP_UNREACHABLE)))
                  {
                     nxlog_debug_tag(_T("obj.dc.queue"), 8, _T("DataCollectionTarget(%s)->QueueItemsForPolling(): item %d \"%s\" skipped because SNMP is unreachable"),
                              m_name, object->getId(), object->getName().cstr());
                     // Set next poll time to be at least half of status polling interval later to avoid re-checking too often
                     object->setNextPollTime(currTime + g_statusPollingInterval / 2);
                     object->clearBusyFlag();
                     continue;  // Skip polling if SNMP is unreachable
                  }
                  if (((object->getDataSource() == DS_SSH) || (object->getDataSource() == DS_SMCLP)) && (!target->isSSHSupported() || (target->getState() & NSF_SSH_UNREACHABLE)))
                  {
                     nxlog_debug_tag(_T("obj.dc.queue"), 8, _T("DataCollectionTarget(%s)->QueueItemsForPolling(): item %d \"%s\" skipped because SSH is unreachable"),
                              m_name, object->getId(), object->getName().cstr());
                     // Set next poll time to be at least half of status polling interval later to avoid re-checking too often
                     object->setNextPollTime(currTime + g_statusPollingInterval / 2);
                     object->clearBusyFlag();
                     continue;  // Skip polling if SSH is unreachable
                  }
                  if ((object->getDataSource() == DS_MODBUS) && (!target->isModbusTCPSupported() || (target->getState() & NSF_MODBUS_UNREACHABLE)))
                  {
                     nxlog_debug_tag(_T("obj.dc.queue"), 8, _T("DataCollectionTarget(%s)->QueueItemsForPolling(): item %d \"%s\" skipped because MODBUS is unreachable"),
                              m_name, object->getId(), object->getName().cstr());
                     // Set next poll time to be at least half of status polling interval later to avoid re-checking too often
                     object->setNextPollTime(currTime + g_statusPollingInterval / 2);
                     object->clearBusyFlag();
                     continue;  // Skip polling if MODBUS is unreachable
                  }
               }
            }

            wchar_t key[32];
            _sntprintf(key, 32, L"%08X/%s", (sourceNodeId != 0) ? sourceNodeId : m_id, object->getDataProviderName());
            ThreadPoolExecuteSerialized(g_dataCollectorThreadPool, key, DataCollector, m_dcObjects.getShared(i));
         }
         else
         {
            ThreadPoolExecute(g_dataCollectorThreadPool, DataCollector, m_dcObjects.getShared(i));
         }
			nxlog_debug_tag(_T("obj.dc.queue"), 8, _T("DataCollectionTarget(%s)->QueueItemsForPolling(): item %d \"%s\" added to queue"),
			         m_name, object->getId(), object->getName().cstr());
      }
   }
   unlockDciAccess();
}

/**
 * Update time intervals in data collection objects
 */
void DataCollectionTarget::updateDataCollectionTimeIntervals()
{
   readLockDciAccess();
   for(int i = 0; i < m_dcObjects.size(); i++)
   {
      m_dcObjects.get(i)->updateTimeIntervals();
   }
   unlockDciAccess();
}

/**
 * Get metric value for client
 */
uint32_t DataCollectionTarget::getMetricForClient(int origin, uint32_t userId, const TCHAR *name, TCHAR *buffer, size_t size)
{
   DataCollectionError rc = DCE_ACCESS_DENIED;
   switch(origin)
   {
      case DS_INTERNAL:
         if (checkAccessRights(userId, OBJECT_ACCESS_READ))
            rc = getInternalMetric(name, buffer, size);
         break;
      case DS_SCRIPT:
         if (checkAccessRights(userId, OBJECT_ACCESS_MODIFY))
            rc = getMetricFromScript(name, buffer, size, this, shared_ptr<DCObjectInfo>());
         break;
      default:
         return RCC_INCOMPATIBLE_OPERATION;
   }
   return RCCFromDCIError(rc);
}

/**
 * Get table for client
 */
uint32_t DataCollectionTarget::getTableForClient(int origin, uint32_t userId, const TCHAR *name, shared_ptr<Table> *table)
{
   DataCollectionError rc = DCE_ACCESS_DENIED;
   switch(origin)
   {
      case DS_INTERNAL:
         if (checkAccessRights(userId, OBJECT_ACCESS_READ))
            rc = getInternalTable(name, table);
         break;
      case DS_SCRIPT:
         if (checkAccessRights(userId, OBJECT_ACCESS_MODIFY))
            rc = getTableFromScript(name, table, this, shared_ptr<DCObjectInfo>());
         break;
      default:
         return RCC_INCOMPATIBLE_OPERATION;
   }
   return RCCFromDCIError(rc);
}

/**
 * Get object from parameter
 */
shared_ptr<NetObj> DataCollectionTarget::objectFromParameter(const TCHAR *param) const
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
   shared_ptr<NetObj> object;
   readLockChildList();
   for(int i = 0; i < getChildList().size(); i++)
   {
      NetObj *curr = getChildList().get(i);
      if (((objectId == 0) && (!_tcsicmp(curr->getName(), arg))) ||
          (objectId == curr->getId()))
      {
         object = getChildList().getShared(i);
         break;
      }
   }
   unlockChildList();
   return object;
}

/**
 * Get value for server's internal table parameter
 */
DataCollectionError DataCollectionTarget::getInternalTable(const TCHAR *name, shared_ptr<Table> *result)
{
   ENUMERATE_MODULES(pfGetInternalTable)
   {
      nxlog_debug_tag(DEBUG_TAG_DC_COLLECTOR, 5, _T("Calling GetInternalTable(\"%s\") in module %s"), name, CURRENT_MODULE.name);
      DataCollectionError rc = CURRENT_MODULE.pfGetInternalTable(this, name, result);
      if (rc != DCE_NOT_SUPPORTED)
         return rc;
   }

   return DCE_NOT_SUPPORTED;
}

/**
 * Get value for server's internal parameter
 */
DataCollectionError DataCollectionTarget::getInternalMetric(const TCHAR *name, TCHAR *buffer, size_t size)
{
   if (size < 64)
      return DCE_COLLECTION_ERROR;  // Result buffer is too small

   ENUMERATE_MODULES(pfGetInternalMetric)
   {
      nxlog_debug_tag(DEBUG_TAG_DC_COLLECTOR, 5, _T("Calling GetInternalMetric(\"%s\") in module %s"), name, CURRENT_MODULE.name);
      DataCollectionError rc = CURRENT_MODULE.pfGetInternalMetric(this, name, buffer, size);
      if (rc != DCE_NOT_SUPPORTED)
         return rc;
   }

   if (isPollable())
   {
      DataCollectionError rc = getAsPollable()->getInternalMetric(name, buffer, size);
      if (rc != DCE_NOT_SUPPORTED)
		   return rc;
   }

   DataCollectionError error = DCE_SUCCESS;
   if (!_tcsicmp(name, _T("Status")))
   {
      IntegerToString(m_status, buffer);
   }
   else if (!_tcsicmp(name, _T("Dummy")) || MatchString(_T("Dummy(*)"), name, FALSE))
   {
      _tcscpy(buffer, _T("0"));
   }
   else if (MatchString(_T("ChildStatus(*)"), name, FALSE))
   {
      shared_ptr<NetObj> object = objectFromParameter(name);
      if (object != nullptr)
      {
         IntegerToString(object->getStatus(), buffer);
      }
      else
      {
         error = DCE_NOT_SUPPORTED;
      }
   }
   else if (MatchString(_T("ConditionStatus(*)"), name, FALSE))
   {
      TCHAR *pEnd, szArg[256];
      shared_ptr<NetObj> object;

      AgentGetParameterArg(name, 1, szArg, 256);
      uint32_t dwId = _tcstoul(szArg, &pEnd, 0);
      if (*pEnd == 0)
		{
			object = FindObjectById(dwId);
         if ((object != nullptr) && (object->getObjectClass() != OBJECT_CONDITION))
            object.reset();
		}
		else
      {
         // Argument is object's name
			object = FindObjectByName(szArg, OBJECT_CONDITION);
      }

      if (object != nullptr)
      {
			if (object->isTrustedObject(m_id))
			{
	         IntegerToString(object->getStatus(), buffer);
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
 * Run data collection script. Result will contain pointer to NXSL VM after successful run and nullptr on failure (with attribute "loaded" indicating if VM script was loaded).
 */
DataCollectionTarget::ScriptExecutionResult DataCollectionTarget::runDataCollectionScript(const TCHAR *param, DataCollectionTarget *targetObject, const shared_ptr<DCObjectInfo>& dciInfo)
{
   wchar_t name[256];
   wcslcpy(name, param, 256);
   TrimW(name);

   // Can be in form parameter(arg1, arg2, ... argN)
   wchar_t *p = wcschr(name, L'(');
   if (p != nullptr)
   {
      size_t l = wcslen(name) - 1;
      if (name[l] != L')')
         return { nullptr, false };  // Interpret argument parsing error as load failure
      name[l] = 0;
      *p = 0;
   }

   // Entry point can be given in form script.entry_point or script/entry_point
   char entryPoint[MAX_IDENTIFIER_LENGTH];
   ExtractScriptEntryPoint(name, entryPoint);

   NXSL_VM *vm = CreateServerScriptVM(name, self(), dciInfo);
   bool loaded = (vm != nullptr);
   if (loaded)
   {
      ObjectRefArray<NXSL_Value> args(16, 16);
      if ((p != nullptr) && !ParseValueList(vm, &p, args, true))
      {
         // argument parsing error
         nxlog_debug(6, _T("DataCollectionTarget(%s)->runDataCollectionScript(%s): Argument parsing error"), m_name, param);
         delete_and_null(vm);
         return { nullptr, false };  // Interpret argument parsing error as load failure
      }

      if (targetObject != nullptr)
      {
         vm->setGlobalVariable("$targetObject", targetObject->createNXSLObject(vm));
      }
      if (!vm->run(args, (entryPoint[0] != 0) ? entryPoint : nullptr))
      {
         if (vm->getErrorCode() == NXSL_ERR_EXECUTION_ABORTED)
         {
            nxlog_debug(6, _T("DataCollectionTarget(%s)::runDataCollectionScript(%s): Script execution aborted"), m_name, param);
         }
         else
         {
            nxlog_debug(6, _T("DataCollectionTarget(%s)::runDataCollectionScript(%s): Script execution error: %s"), m_name, param, vm->getErrorText());
            time_t now = time(nullptr);
            time_t lastReport = static_cast<time_t>(m_scriptErrorReports.getInt64(param, 0));
            if (lastReport + ConfigReadInt(_T("DataCollection.ScriptErrorReportInterval"), 86400) < now)
            {
               ReportScriptError(SCRIPT_CONTEXT_DCI, this, 0, vm->getErrorText(), _T("%s"), name);
               m_scriptErrorReports.set(param, static_cast<uint64_t>(now));
            }
         }
         delete_and_null(vm);
      }
   }
   else
   {
      nxlog_debug(6, _T("DataCollectionTarget(%s)::runDataCollectionScript(%s): VM load error"), m_name, param);
   }
   nxlog_debug(7, _T("DataCollectionTarget(%s)::runDataCollectionScript(%s): %s"), m_name, param, (vm != nullptr) ? _T("success") : _T("failure"));
   return { vm, loaded };
}

/**
 * Parse list of service call arguments
 */
static bool ParseCallArgumensList(TCHAR *input, StringList *args)
{
   TCHAR *p = input;

   TCHAR *s = p;
   int state = 1; // normal text
   for(; state > 0; p++)
   {
      switch(*p)
      {
         case '"':
            if (state == 1)
            {
               state = 2;
               s = p + 1;
            }
            else
            {
               state = 3;
               *p = 0;
               args->add(s);
            }
            break;
         case ',':
            if (state == 1)
            {
               *p = 0;
               Trim(s);
               args->add(s);
               s = p + 1;
            }
            else if (state == 3)
            {
               state = 1;
               s = p + 1;
            }
            break;
         case 0:
            if (state == 1)
            {
               Trim(s);
               args->add(s);
               state = 0;
            }
            else if (state == 3)
            {
               state = 0;
            }
            else
            {
               state = -1; // error
            }
            break;
         case ' ':
            break;
         case ')':
            if (state == 1)
            {
               *p = 0;
               Trim(s);
               args->add(s);
               state = 0;
            }
            else if (state == 3)
            {
               state = 0;
            }
            break;
         case '\\':
            if (state == 2)
            {
               memmove(p, p + 1, _tcslen(p) * sizeof(TCHAR));
               switch(*p)
               {
                  case 'r':
                     *p = '\r';
                     break;
                  case 'n':
                     *p = '\n';
                     break;
                  case 't':
                     *p = '\t';
                     break;
                  default:
                     break;
               }
            }
            else if (state == 3)
            {
               state = -1;
            }
            break;
         default:
            if (state == 3)
               state = -1;
            break;
      }
   }
   return (state != -1);
}

/**
 * Get item or list from web service
 * Parameter is expected in form service:path or service(arguments):path
 */
DataCollectionError DataCollectionTarget::queryWebService(const TCHAR *param, WebServiceRequestType queryType, TCHAR *buffer,
         size_t bufSize, StringList *list)
{
   uint32_t proxyId = getEffectiveWebServiceProxy();
   shared_ptr<Node> proxyNode = static_pointer_cast<Node>(FindObjectById(proxyId, OBJECT_NODE));
   if (proxyNode == nullptr)
   {
      nxlog_debug(7, _T("DataCollectionTarget(%s)->queryWebService(%s): cannot find proxy node [%u]"), m_name, param, proxyId);
      return DCE_COMM_ERROR;
   }

   TCHAR name[1024];
   _tcslcpy(name, param, 1024);
   Trim(name);

   TCHAR *path = nullptr;
   bool openedBraceFound = false;
   for (const TCHAR *curr = name; *curr != 0 && path == nullptr; curr++)
   {
      switch(*curr)
      {
         case ')':
            openedBraceFound = false;
            break;
         case '(':
            openedBraceFound = true;
            break;
         case '\'':
            curr++;
            while (*curr != '\'' && *curr != 0)
            {
               curr++;
            }
            break;
         case '"':
            curr++;
            while (*curr != '"' && *curr != 0)
            {
               curr++;
            }
            break;
         case ':':
            if (!openedBraceFound)
               path = const_cast<TCHAR *>(curr);
            break;
      }
      if (*curr == 0)
         curr--;
   }
   if (path == nullptr)
   {
      nxlog_debug(7, _T("DataCollectionTarget(%s)->queryWebService(%s): missing parameter path"), m_name, param);
      return DCE_NOT_SUPPORTED;
   }
   *path = 0;
   path++;

   // Can be in form service(arg1, arg2, ... argN)
   StringList args;
   TCHAR *p = _tcschr(name, _T('('));
   if (p != nullptr)
   {
      size_t l = _tcslen(name) - 1;
      if (name[l] != _T(')'))
      {
         nxlog_debug(7, _T("DataCollectionTarget(%s)->queryWebService(%s): error parsing argument list"), m_name, param);
         return DCE_NOT_SUPPORTED;
      }
      name[l] = 0;
      *p = 0;
      p++;
      if (!ParseCallArgumensList(p, &args))
      {
         nxlog_debug(7, _T("DataCollectionTarget(%s)->queryWebService(%s): error parsing argument list"), m_name, param);
         return DCE_NOT_SUPPORTED;
      }
      Trim(name);
   }

   shared_ptr<WebServiceDefinition> d = FindWebServiceDefinition(name);
   if (d == nullptr)
   {
      nxlog_debug(7, _T("DataCollectionTarget(%s)->queryWebService(%s): cannot find web service definition"), m_name, param);
      return DCE_NOT_SUPPORTED;
   }

   shared_ptr<AgentConnectionEx> conn = proxyNode->acquireProxyConnection(WEB_SERVICE_PROXY);
   if (conn == nullptr)
   {
      nxlog_debug(7, _T("DataCollectionTarget(%s)->queryWebService(%s): cannot acquire proxy connection"), m_name, param);
      return DCE_COMM_ERROR;
   }

   StringBuffer url = expandText(d->getUrl(), nullptr, nullptr, shared_ptr<DCObjectInfo>(), nullptr, nullptr, nullptr, nullptr, &args);

   StringMap headers;
   auto it = d->getHeaders().begin();
   while(it.hasNext())
   {
      auto h = it.next();
      StringBuffer value = expandText(h->value, nullptr, nullptr, shared_ptr<DCObjectInfo>(), nullptr, nullptr, nullptr, nullptr, &args);
      headers.set(h->key, value);
   }

   StringList pathList;
   pathList.add(path);
   StringMap results;
   uint32_t agentStatus = conn->queryWebService(queryType, url, d->getHttpRequestMethod(), d->getRequestData(), d->getRequestTimeout(),
         d->getCacheRetentionTime(), d->getLogin(), d->getPassword(), d->getAuthType(), headers, pathList, d->isVerifyCertificate(),
         d->isVerifyHost(), d->isFollowLocation(), d->isForcePlainTextParser(),
         (queryType == WebServiceRequestType::PARAMETER) ? static_cast<void*>(&results) : static_cast<void*>(list));

   DataCollectionError rc;
   if (agentStatus == ERR_SUCCESS)
   {
      if (queryType == WebServiceRequestType::PARAMETER)
      {
         const TCHAR *value = results.get(pathList.get(0));
         if (value != nullptr)
         {
            _tcslcpy(buffer, value, bufSize);
            rc = DCE_SUCCESS;
         }
         else
         {
            rc = DCE_NO_SUCH_INSTANCE;
         }
      }
      else
      {
         rc = DCE_SUCCESS;
      }
   }
   else
   {
      rc = DCE_COMM_ERROR;
   }
   nxlog_debug(7, _T("DataCollectionTarget(%s)->queryWebService(%s): rc=%d"), m_name, param, rc);
   return rc;
}

/**
 * Get item from web service
 * Parameter is expected in form service:path or service(arguments):path
 */
DataCollectionError DataCollectionTarget::getMetricFromWebService(const TCHAR *param, TCHAR *buffer, size_t bufSize)
{
   return queryWebService(param, WebServiceRequestType::PARAMETER, buffer, bufSize, nullptr);
}

/**
 * Get list from library script
 * Parameter is expected in form service:path or service(arguments):path
 */
DataCollectionError DataCollectionTarget::getListFromWebService(const TCHAR *param, StringList **list)
{
   *list = new StringList();
   DataCollectionError rc = queryWebService(param, WebServiceRequestType::LIST, nullptr, 0, *list);
   if (rc != DCE_SUCCESS)
   {
      delete *list;
      *list = nullptr;
   }
   return rc;
}

/**
 * Get parameter value from NXSL script
 */
DataCollectionError DataCollectionTarget::getMetricFromScript(const TCHAR *param, TCHAR *buffer, size_t bufSize, DataCollectionTarget *targetObject, const shared_ptr<DCObjectInfo>& dciInfo)
{
   DataCollectionError rc = DCE_NOT_SUPPORTED;
   ScriptExecutionResult result = runDataCollectionScript(param, targetObject, dciInfo);
   if (result.vm != nullptr)
   {
      NXSL_Value *value = result.vm->getResult();
      if (value->isNull())
      {
         // nullptr value is an error indicator
         rc = DCE_COLLECTION_ERROR;
      }
      else
      {
         rc = value->isGuid() ? NXSLExitCodeToDCE(value->getValueAsGuid()) : DCE_SUCCESS;
         if (rc == DCE_SUCCESS)
         {
            const TCHAR *dciValue = value->getValueAsCString();
            _tcslcpy(buffer, CHECK_NULL_EX(dciValue), bufSize);
         }
      }
      delete result.vm;
   }
   else if (result.loaded)
   {
      rc = DCE_COLLECTION_ERROR;
   }
   nxlog_debug(7, _T("DataCollectionTarget(%s)::getMetricFromScript(%s): rc=%d"), m_name, param, rc);
   return rc;
}

/**
 * Get table from NXSL script
 */
DataCollectionError DataCollectionTarget::getTableFromScript(const TCHAR *param, shared_ptr<Table> *table, DataCollectionTarget *targetObject, const shared_ptr<DCObjectInfo>& dciInfo)
{
   DataCollectionError rc = DCE_NOT_SUPPORTED;
   ScriptExecutionResult result = runDataCollectionScript(param, targetObject, dciInfo);
   if (result.vm != nullptr)
   {
      NXSL_Value *value = result.vm->getResult();
      if (value->isObject(_T("Table")))
      {
         *table = *static_cast<shared_ptr<Table>*>(value->getValueAsObject()->getData());
         rc = DCE_SUCCESS;
      }
      else if (value->isGuid())
      {
         rc = NXSLExitCodeToDCE(value->getValueAsGuid(), DCE_COLLECTION_ERROR);
      }
      else
      {
         rc = DCE_COLLECTION_ERROR;
      }
      delete result.vm;
   }
   else if (result.loaded)
   {
      rc = DCE_COLLECTION_ERROR;
   }
   nxlog_debug(7, _T("DataCollectionTarget(%s)->getScriptTable(%s): rc=%d"), m_name, param, rc);
   return rc;
}

/**
 * Get string map from library script
 */
DataCollectionError DataCollectionTarget::getStringMapFromScript(const TCHAR *param, StringMap **map, DataCollectionTarget *targetObject)
{
   DataCollectionError rc = DCE_NOT_SUPPORTED;
   ScriptExecutionResult result = runDataCollectionScript(param, targetObject, nullptr);
   if (result.vm != nullptr)
   {
      rc = DCE_SUCCESS;
      NXSL_Value *value = result.vm->getResult();
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
      else if (value->isGuid())
      {
         rc = NXSLExitCodeToDCE(value->getValueAsGuid(), DCE_COLLECTION_ERROR);
      }
      else if (value->isNull())
      {
         rc = DCE_COLLECTION_ERROR;
      }
      else
      {
         *map = new StringMap();
      }
      delete result.vm;
   }
   else if (result.loaded)
   {
      rc = DCE_COLLECTION_ERROR;
   }
   nxlog_debug(7, _T("DataCollectionTarget(%s)->getListFromScript(%s): rc=%d"), m_name, param, rc);
   return rc;
}

/**
 * Get last (current) DCI values for summary table.
 */
void DataCollectionTarget::getDciValuesSummary(SummaryTable *tableDefinition, Table *tableData, uint32_t userId)
{
   if (tableDefinition->isTableDciSource())
      getTableDciValuesSummary(tableDefinition, tableData, userId);
   else
      getItemDciValuesSummary(tableDefinition, tableData, userId);
}

/**
 * Match data collection object to column definition
 */
static inline bool MatchDCItem(SummaryTableColumn *tc, DCObject *dci)
{
   const TCHAR *text = (tc->m_flags & COLUMN_DEFINITION_BY_DESCRIPTION) ? dci->getDescription() : dci->getName();
   return (tc->m_flags & COLUMN_DEFINITION_REGEXP_MATCH) ? RegexpMatch(text, tc->m_dciName, false) : (_tcsicmp(text, tc->m_dciName) == 0);
}

/**
 * Get last (current) DCI values for summary table using single-value DCIs
 */
void DataCollectionTarget::getItemDciValuesSummary(SummaryTable *tableDefinition, Table *tableData, uint32_t userId)
{
   int offset = tableDefinition->isMultiInstance() ? 2 : 1;
   int baseRow = tableData->getNumRows();
   bool rowAdded = false;
   readLockDciAccess();
   for(int i = 0; i < tableDefinition->getNumColumns(); i++)
   {
      SummaryTableColumn *tc = tableDefinition->getColumn(i);
      for(int j = 0; j < m_dcObjects.size(); j++)
	   {
		   DCObject *object = m_dcObjects.get(j);
         if ((object->getType() == DCO_TYPE_ITEM) && object->hasValue() && object->hasAccess(userId) &&
             (object->getStatus() == ITEM_STATUS_ACTIVE) && MatchDCItem(tc, object))
         {
            int row;
            if (tableDefinition->isMultiInstance())
            {
               // Find instance
               const TCHAR *instance = object->getInstanceName();
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
            tableData->setStatusAt(row, i + offset, static_cast<DCItem*>(object)->getThresholdSeverity());
            tableData->setCellObjectIdAt(row, i + offset, object->getId());
            tableData->getColumnDefinitions().get(i + offset)->setDataType(static_cast<DCItem*>(object)->getTransformedDataType());
            tableData->getColumnDefinitions().get(i + offset)->setUnitName(static_cast<DCItem*>(object)->getUnitName());
            tableData->getColumnDefinitions().get(i + offset)->setMultiplier(static_cast<DCItem*>(object)->getMultiplier());
            tableData->getColumnDefinitions().get(i + offset)->setUseMultiplier(static_cast<DCItem*>(object)->getUseMultiplier());

            if (tableDefinition->getAggregationFunction() == DCI_AGG_LAST)
            {
               if (tc->m_flags & COLUMN_DEFINITION_MULTIVALUED)
               {
                  StringList values = String(static_cast<DCItem*>(object)->getLastValue()).split(tc->m_separator);
                  tableData->setAt(row, i + offset, values.get(0));
                  for(int r = 1; r < values.size(); r++)
                  {
                     if (row + r >= tableData->getNumRows())
                     {
                        tableData->addRow();
                        tableData->setObjectId(m_id);
                        tableData->setBaseRow(row);
                     }
                     tableData->setAt(row + r, i + offset, values.get(r));
                     tableData->setStatusAt(row + r, i + offset, static_cast<DCItem*>(object)->getThresholdSeverity());
                     tableData->setCellObjectIdAt(row + r, i + offset, object->getId());
                  }
               }
               else
               {
                  tableData->setAt(row, i + offset, static_cast<DCItem*>(object)->getLastValue());
               }
            }
            else
            {
               tableData->setPreallocatedAt(row, i + offset,
                  static_cast<DCItem*>(object)->getAggregateValue(
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
void DataCollectionTarget::getTableDciValuesSummary(SummaryTable *tableDefinition, Table *tableData, uint32_t userId)
{
   readLockDciAccess();
   for(int i = 0; i < m_dcObjects.size(); i++)
   {
      DCObject *o = m_dcObjects.get(i);
      if ((o->getType() == DCO_TYPE_TABLE) && o->hasValue() &&
           (o->getStatus() == ITEM_STATUS_ACTIVE) &&
           !_tcsicmp(o->getName(), tableDefinition->getTableDciName()) &&
           o->hasAccess(userId))
      {
         shared_ptr<Table> lastValue = static_cast<DCTable*>(o)->getLastValue();
         if (lastValue == nullptr)
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
                  columnIndex = tableData->addColumn(*lastValue->getColumnDefinition(k));
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
bool DataCollectionTarget::isEventSource() const
{
   return true;
}

/**
 * Returns most critical status of DCI used for status calculation
 */
int DataCollectionTarget::getAdditionalMostCriticalStatus()
{
   int status = -1;
   readLockDciAccess();
   for(int i = 0; i < m_dcObjects.size(); i++)
	{
		DCObject *curr = m_dcObjects.get(i);
      if (curr->isStatusDCO() && (curr->getType() == DCO_TYPE_ITEM) &&
          curr->hasValue() && (curr->getStatus() == ITEM_STATUS_ACTIVE))
      {
         if (getObjectClass() == OBJECT_CLUSTER && !curr->isAggregateOnCluster())
            continue; // Calculated only on those that are aggregated on cluster

         ItemValue *value = static_cast<DCItem*>(curr)->getInternalLastValue();
         if (value != nullptr && value->getInt32() >= STATUS_NORMAL && value->getInt32() <= STATUS_CRITICAL)
            status = std::max(status, value->getInt32());
         delete value;
      }
	}
   unlockDciAccess();
   return (status == -1) ? STATUS_UNKNOWN : status;
}

/**
 * Enter maintenance mode
 */
void DataCollectionTarget::enterMaintenanceMode(uint32_t userId, const TCHAR *comments)
{
   TCHAR userName[MAX_USER_NAME];
   ResolveUserId(userId, userName, true);

   nxlog_debug_tag(DEBUG_TAG_MAINTENANCE, 4, _T("Entering maintenance mode for %s [%d] (initiated by %s)"), m_name, m_id, userName);
   uint64_t eventId = 0;
   EventBuilder(EVENT_MAINTENANCE_MODE_ENTERED, m_id)
      .param(_T("comments"), CHECK_NULL_EX(comments))
      .param(_T("userId"), userId, EventBuilder::OBJECT_ID_FORMAT)
      .param(_T("userName"), userName)
      .storeEventId(&eventId)
      .post();

   readLockDciAccess();
   for(int i = 0; i < m_dcObjects.size(); i++)
   {
      DCObject *dco = m_dcObjects.get(i);
      if (dco->getStatus() == ITEM_STATUS_DISABLED)
         continue;

      dco->saveStateBeforeMaintenance();
   }
   unlockDciAccess();

   readLockChildList();
   for(int i = 0; i < getChildList().size(); i++)
   {
      NetObj *object = getChildList().get(i);
      if (object->getObjectClass() == OBJECT_INTERFACE)
         static_cast<Interface*>(object)->saveStateBeforeMaintenance(m_id);
      if (object->getObjectClass() == OBJECT_NETWORKSERVICE)
         static_cast<NetworkService*>(object)->saveStateBeforeMaintenance();
   }
   unlockChildList();

   lockProperties();
   m_maintenanceEventId = eventId;
   m_maintenanceInitiator = userId;
   m_stateBeforeMaintenance = m_state;
   setModified(MODIFY_COMMON_PROPERTIES | MODIFY_DATA_COLLECTION);
   unlockProperties();
}

/**
 * Leave maintenance mode
 */
void DataCollectionTarget::leaveMaintenanceMode(uint32_t userId)
{
   TCHAR userName[MAX_USER_NAME];
   ResolveUserId(userId, userName, true);

   nxlog_debug_tag(DEBUG_TAG_MAINTENANCE, 4, _T("Leaving maintenance mode for %s [%d] (initiated by %s)"), m_name, m_id, userName);
   EventBuilder(EVENT_MAINTENANCE_MODE_LEFT, m_id)
      .param(_T("userId"), userId, EventBuilder::OBJECT_ID_FORMAT)
      .param(_T("userName"), userName)
      .post();


   readLockDciAccess();
   for(int i = 0; i < m_dcObjects.size(); i++)
   {
      DCObject *dco = m_dcObjects.get(i);
      if (dco->getStatus() == ITEM_STATUS_DISABLED)
      {
         continue;
      }

      dco->generateEventsAfterMaintenance();
   }
   unlockDciAccess();

   lockProperties();
   m_maintenanceEventId = 0;
   m_maintenanceInitiator = 0;
   bool forcePoll = m_state != m_stateBeforeMaintenance;
   m_state = m_stateBeforeMaintenance;
   setModified(MODIFY_COMMON_PROPERTIES);
   unlockProperties();

   if (forcePoll)
   {
      TCHAR threadKey[32];
      _sntprintf(threadKey, 32, _T("POLL_%u"), getId());
      ThreadPoolExecuteSerialized(g_pollerThreadPool, threadKey, static_cast<Pollable*>(this), &Pollable::doForcedStatusPoll, RegisterPoller(PollerType::STATUS, self()));
   }

   readLockChildList();
   for(int i = 0; i < getChildList().size(); i++)
   {
      NetObj *object = getChildList().get(i);
      if (object->getObjectClass() == OBJECT_INTERFACE)
         static_cast<Interface*>(object)->generateEventsAfterMaintenace(m_id);
      else if (object->getObjectClass() == OBJECT_NETWORKSERVICE)
         static_cast<NetworkService*>(object)->generateEventsAfterMaintenace();
   }
   unlockChildList();
}

/**
 * Update cache size for given data collection item
 */
void DataCollectionTarget::updateDCItemCacheSize(uint32_t dciId)
{
   readLockDciAccess();
   shared_ptr<DCObject> dci = getDCObjectById(dciId, 0, false);
   if ((dci != nullptr) && (dci->getType() == DCO_TYPE_ITEM))
   {
      static_cast<DCItem*>(dci.get())->updateCacheSize();
   }
   unlockDciAccess();
}

/**
 * Reload DCI cache
 */
void DataCollectionTarget::reloadDCItemCache(uint32_t dciId)
{
   readLockDciAccess();
   shared_ptr<DCObject> dci = getDCObjectById(dciId, 0, false);
   if ((dci != nullptr) && (dci->getType() == DCO_TYPE_ITEM))
   {
      nxlog_debug_tag(DEBUG_TAG_DC_CACHE, 6, _T("Reload DCI cache for \"%s\" [%u] on %s [%u]"), dci->getName().cstr(), dci->getId(), m_name, m_id);
      static_cast<DCItem&>(*dci).reloadCache(true);
   }
   unlockDciAccess();
}

/**
 * Returns true if object is data collection target
 */
bool DataCollectionTarget::isDataCollectionTarget() const
{
   return true;
}

/**
 * Add data collection element to proxy info structure
 */
void DataCollectionTarget::addProxyDataCollectionElement(ProxyInfo *info, const DCObject *dco, uint32_t primaryProxyId)
{
   info->msg->setField(info->baseInfoFieldId++, dco->getId());
   info->msg->setField(info->baseInfoFieldId++, static_cast<int16_t>(dco->getType()));
   info->msg->setField(info->baseInfoFieldId++, static_cast<int16_t>(dco->getDataSource()));
   if (dco->getDataSource() == DS_MODBUS)
   {
      uint16_t unitId = getModbusUnitId();
      int address;
      const TCHAR *source;
      const TCHAR *conversion;
      if (ParseModbusMetric(dco->getName(), &unitId, &source, &address, &conversion))
      {
         StringBuffer metric(_T("Modbus."));
         if (!_tcsnicmp(source, _T("hold:"), 5))
         {
            metric.append(_T("HoldingRegister("));
         }
         else if (!_tcsnicmp(source, _T("input:"), 6))
         {
            metric.append(_T("InputRegister("));
         }
         else if (!_tcsnicmp(source, _T("coil:"), 5))
         {
            metric.append(_T("Coil("));
         }
         else if (!_tcsnicmp(source, _T("discrete:"), 9))
         {
            metric.append(_T("DiscreteInput("));
         }
         metric.append(getPrimaryIpAddress().toString());
         metric.append(_T(","));
         metric.append(getModbusTcpPort());
         metric.append(_T(","));
         metric.append(unitId);
         metric.append(_T(","));
         metric.append(address);
         if (*conversion != 0)
         {
            metric.append(_T(","));
            metric.append(conversion);
         }
         metric.append(_T(")"));
         info->msg->setField(info->baseInfoFieldId++, metric);
      }
      else
      {
         info->msg->setField(info->baseInfoFieldId++, _T("invalid_metric"));
      }
   }
   else
   {
      info->msg->setField(info->baseInfoFieldId++, dco->getName());
   }
   info->msg->setField(info->baseInfoFieldId++, dco->getEffectivePollingInterval());
   info->msg->setFieldFromTime(info->baseInfoFieldId++, dco->getLastPollTime().asTime()); // convert to seconds for compatibility with older agents
   info->msg->setField(info->baseInfoFieldId++, m_guid);
   info->msg->setField(info->baseInfoFieldId++, dco->getSnmpPort());
   if (dco->getType() == DCO_TYPE_ITEM)
      info->msg->setField(info->baseInfoFieldId++, static_cast<const DCItem*>(dco)->getSnmpRawValueType());
   else
      info->msg->setField(info->baseInfoFieldId++, static_cast<int16_t>(0));
   info->msg->setField(info->baseInfoFieldId++, primaryProxyId);

   info->msg->setField(info->extraInfoFieldId + 1, dco->getLastPollTime()); // in milliseconds

   if (dco->getDataSource() == DS_SNMP_AGENT)
   {
      info->msg->setField(info->extraInfoFieldId++, static_cast<int16_t>(dco->getSnmpVersion()));
      if (dco->getType() == DCO_TYPE_TABLE)
      {
         info->extraInfoFieldId += 8;
         const ObjectArray<DCTableColumn> &columns = static_cast<const DCTable*>(dco)->getColumns();
         int count = std::min(columns.size(), 99);
         info->msg->setField(info->extraInfoFieldId++, count);
         for(int i = 0; i < count; i++)
         {
            columns.get(i)->fillMessage(info->msg, info->extraInfoFieldId);
            info->extraInfoFieldId += 10;
         }
         info->extraInfoFieldId += (99 - count) * 10;
      }
      else
      {
         info->extraInfoFieldId += 999;
      }
   }
   else
   {
      info->extraInfoFieldId += 1000;
   }

   if (dco->isAdvancedSchedule())
   {
      dco->fillSchedulingDataMessage(info->msg, info->extraInfoFieldId);
   }
   info->extraInfoFieldId += 100;

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
   info->msg->setField(info->nodeInfoFieldId++, static_cast<int16_t>(snmpSecurity->getAuthMethod()));
   info->msg->setField(info->nodeInfoFieldId++, static_cast<int16_t>(snmpSecurity->getPrivMethod()));
   info->msg->setFieldFromMBString(info->nodeInfoFieldId++, snmpSecurity->getAuthName());
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
   if ((m_status == STATUS_UNMANAGED) || (m_state & DCSF_UNREACHABLE))
      return;

   readLockDciAccess();
   for(int i = 0; i < m_dcObjects.size(); i++)
   {
      DCObject *dco = m_dcObjects.get(i);
      if (dco->getStatus() == ITEM_STATUS_DISABLED)
         continue;

      if ((dco->getDataSource() == DS_NATIVE_AGENT) && (dco->getSourceNode() == info->proxyId) &&
          dco->hasValue() && (dco->getAgentCacheMode() == AGENT_CACHE_ON))
      {
         addProxyDataCollectionElement(info, dco, 0);
      }
   }
   unlockDciAccess();
}

/**
 * Callback for collecting proxied SNMP DCIs
 */
EnumerationCallbackResult DataCollectionTarget::collectProxyInfoCallback(NetObj *object, void *data)
{
   static_cast<DataCollectionTarget*>(object)->collectProxyInfo(static_cast<ProxyInfo*>(data));
   return _CONTINUE;
}

/**
 * Get effective source node for given data collection object
 */
uint32_t DataCollectionTarget::getEffectiveSourceNode(DCObject *dco)
{
   return dco->getSourceNode();
}

/**
 * Remove template from this data collection target. Intended to be called only by template automatic removal code.
 */
void DataCollectionTarget::removeTemplate(Template *templateObject, NetObj *pollTarget)
{
   pollTarget->sendPollerMsg(_T("   Removing template %s from %s\r\n"), templateObject->getName(), m_name);
   nxlog_debug_tag(DEBUG_TAG_AUTOBIND_POLL, 4, _T("DataCollectionTarget::removeTemplate(): removing template %d \"%s\" from object %d \"%s\""),
               templateObject->getId(), templateObject->getName(), m_id, m_name);
   unlinkObjects(templateObject, this);
   templateObject->queueRemoveFromTarget(m_id, true);
   EventBuilder(EVENT_TEMPLATE_AUTOREMOVE, g_dwMgmtNode)
      .param(_T("nodeId"), templateObject->getId(), EventBuilder::OBJECT_ID_FORMAT)
      .param(_T("nodeName"), templateObject->getName())
      .param(_T("templateId"), m_id, EventBuilder::OBJECT_ID_FORMAT)
      .param(_T("templateName"), m_name)
      .post();
}

/**
 * Scheduled task executor - remove template from data collection target
 */
void DataCollectionTarget::removeTemplate(const shared_ptr<ScheduledTaskParameters>& parameters)
{
   shared_ptr<NetObj> target = FindObjectById(parameters->m_objectId);
   if ((target != nullptr) && target->isDataCollectionTarget())
   {
      uuid guid = uuid::parse(parameters->m_persistentData);
      shared_ptr<Template> templateObject = static_pointer_cast<Template>(FindObjectByGUID(guid, OBJECT_TEMPLATE));
      if (templateObject != nullptr)
      {
         static_cast<DataCollectionTarget&>(*target).removeTemplate(templateObject.get(), target.get());
      }
   }
}

/**
 * Schedule template removal after grace period (will execute removeTemplate immediately if grace period is zero)
 */
void DataCollectionTarget::scheduleTemplateRemoval(Template *templateObject, NetObj *pollTarget)
{
   int gracePeriod = ConfigReadInt(_T("DataCollection.TemplateRemovalGracePeriod"), 0);
   if (gracePeriod > 0)
   {
      TCHAR key[64];
      _sntprintf(key, 64, _T("Delete.Template.%u.NetObj.%u"), templateObject->getId(), m_id);
      if (CountScheduledTasksByKey(key) == 0)
      {
         AddOneTimeScheduledTask(_T("DataCollection.RemoveTemplate"), time(nullptr) + static_cast<time_t>(gracePeriod * 86400),
               m_guid.toString(), nullptr, 0, m_id, SYSTEM_ACCESS_FULL, _T(""), key, true);
         nxlog_debug_tag(DEBUG_TAG_AUTOBIND_POLL, 4, _T("DataCollectionTarget::scheduleTemplateRemoval(\"%s\" [%u]): scheduled removal of template \"%s\" [%u]"),
               m_name, m_id, templateObject->getName(), templateObject->getId());
         pollTarget->sendPollerMsg(_T("   Scheduled removal of template %s from %s\r\n"), templateObject->getName(), m_name);
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_AUTOBIND_POLL, 4, _T("DataCollectionTarget::scheduleTemplateRemoval(\"%s\" [%u]): removal of template \"%s\" [%u] already scheduled"),
               m_name, m_id, templateObject->getName(), templateObject->getId());
         pollTarget->sendPollerMsg(_T("   Removal of template %s from %s already scheduled\r\n"), templateObject->getName(), m_name);
      }
   }
   else
   {
      removeTemplate(templateObject, pollTarget);
   }
}

/**
 * Filter for selecting templates from objects
 */
static bool TemplateSelectionFilter(NetObj *object, void *userData)
{
   return (object->getObjectClass() == OBJECT_TEMPLATE) && !object->isDeleted() && static_cast<Template*>(object)->isAutoBindEnabled();
}

/**
 * Apply templates
 */
void DataCollectionTarget::applyTemplates()
{
   if (IsShutdownInProgress() || !(g_flags & AF_AUTOBIND_ON_CONF_POLL))
      return;

   sendPollerMsg(_T("Processing template automatic apply rules\r\n"));
   unique_ptr<SharedObjectArray<NetObj>> templates = g_idxObjectById.getObjects(TemplateSelectionFilter);
   for(int i = 0; i < templates->size(); i++)
   {
      Template *templateObject = static_cast<Template*>(templates->get(i));
      NXSL_VM *cachedFilterVM = nullptr;
      AutoBindDecision decision = templateObject->isApplicable(&cachedFilterVM, self());
      if (decision == AutoBindDecision_Bind)
      {
         TCHAR key[64];
         _sntprintf(key, 64, _T("Delete.Template.%u.NetObj.%u"), templateObject->getId(), m_id);
         if (DeleteScheduledTasksByKey(key) > 0)
         {
            sendPollerMsg(_T("   Pending removal from template \"%s\" cancelled\r\n"), templateObject->getName());
            nxlog_debug_tag(_T("obj.bind"), 4, _T("DataCollectionTarget::applyTemplates(): pending template \"%s\" [%u] removal from object \"%s\" [%u] cancelled"),
                  templateObject->getName(), templateObject->getId(), m_name, m_id);
         }

         if (!templateObject->isDirectChild(m_id))
         {
            sendPollerMsg(_T("   Applying template \"%s\"\r\n"), templateObject->getName());
            nxlog_debug_tag(_T("obj.bind"), 4, _T("DataCollectionTarget::applyTemplates(): applying template \"%s\" [%u] to object \"%s\" [%u]"),
                  templateObject->getName(), templateObject->getId(), m_name, m_id);
            templateObject->applyToTarget(self());
            EventBuilder(EVENT_TEMPLATE_AUTOAPPLY, g_dwMgmtNode)
               .param(_T("nodeId"), m_id, EventBuilder::OBJECT_ID_FORMAT)
               .param(_T("nodeName"), m_name)
               .param(_T("templateId"), templateObject->getId(), EventBuilder::OBJECT_ID_FORMAT)
               .param(_T("templateName"), templateObject->getName())
               .post();
         }
      }
      else if ((decision == AutoBindDecision_Unbind) && templateObject->isAutoUnbindEnabled() && templateObject->isDirectChild(m_id))
      {
         scheduleTemplateRemoval(templateObject, this);
      }
      delete cachedFilterVM;
   }
}

/**
 * Filter for selecting containers from objects
 */
static bool ContainerSelectionFilter(NetObj *object, void *context)
{
   if (object->isDeleted())
      return false;
   if (object->getObjectClass() == OBJECT_CONTAINER)
      return static_cast<Container*>(object)->isAutoBindEnabled();
   if (object->getObjectClass() == OBJECT_COLLECTOR)
      return static_cast<Collector*>(object)->isAutoBindEnabled();
   return false;
}

/**
 * Update container membership
 */
void DataCollectionTarget::updateContainerMembership()
{
   if (IsShutdownInProgress() || !(g_flags & AF_AUTOBIND_ON_CONF_POLL))
      return;

   sendPollerMsg(_T("Processing container autobind rules\r\n"));
   unique_ptr<SharedObjectArray<NetObj>> containers = g_idxObjectById.getObjects(ContainerSelectionFilter);
   for(int i = 0; i < containers->size(); i++)
   {
      NetObj *container = containers->get(i);

      AutoBindTarget *abtInterface;
      const wchar_t *className;
      if (container->getObjectClass() == OBJECT_CONTAINER)
      {
         abtInterface = static_cast<Container*>(container);
         className = L"container";
      }
      else if (container->getObjectClass() == OBJECT_COLLECTOR)
      {
         abtInterface = static_cast<Collector*>(container);
         className = L"collector";
      }
      else
      {
         continue;   // Should not happen
      }

      NXSL_VM *cachedFilterVM = nullptr;
      AutoBindDecision decision = abtInterface->isApplicable(&cachedFilterVM, self());
      if (decision == AutoBindDecision_Bind)
      {
         if (!container->isDirectChild(m_id))
         {
            sendPollerMsg(_T("   Binding to %s %s\r\n"), className, container->getName());
            nxlog_debug_tag(_T("obj.bind"), 4, _T("DataCollectionTarget::updateContainerMembership(): binding object \"%s\" [%u] to %s \"%s\" [%u]"),
                      m_name, m_id, className, container->getName(), container->getId());
            linkObjects(container->self(), self());
            EventBuilder(EVENT_CONTAINER_AUTOBIND, g_dwMgmtNode)
               .param(_T("nodeId"), m_id, EventBuilder::OBJECT_ID_FORMAT)
               .param(_T("nodeName"), m_name)
               .param(_T("containerId"), container->getId(), EventBuilder::OBJECT_ID_FORMAT)
               .param(_T("containerName"), container->getName())
               .post();

            container->calculateCompoundStatus();
         }
      }
      else if (decision == AutoBindDecision_Unbind)
      {
         if (abtInterface->isAutoUnbindEnabled() && container->isDirectChild(m_id))
         {
            sendPollerMsg(_T("   Removing from %s %s\r\n"), className, container->getName());
            nxlog_debug_tag(_T("obj.bind"), 4, _T("DataCollectionTarget::updateContainerMembership(): removing object \"%s\" [%u] from %s \"%s\" [%u]"),
                      m_name, m_id, className, container->getName(), container->getId());
            unlinkObjects(container, this);
            EventBuilder(EVENT_CONTAINER_AUTOUNBIND, g_dwMgmtNode)
               .param(_T("nodeId"), m_id, EventBuilder::OBJECT_ID_FORMAT)
               .param(_T("nodeName"), m_name)
               .param(_T("containerId"), container->getId(), EventBuilder::OBJECT_ID_FORMAT)
               .param(_T("containerName"), container->getName())
               .post();
            container->calculateCompoundStatus();
         }
      }
      delete cachedFilterVM;
   }
}

/**
 * Lock object for instance discovery poll
 */
bool DataCollectionTarget::lockForInstanceDiscoveryPoll()
{
   bool success = false;
   lockProperties();
   if (!m_isDeleted && !m_isDeleteInitiated &&
       (m_status != STATUS_UNMANAGED) &&
       (!(m_flags & DCF_DISABLE_CONF_POLL)) &&
       (m_runtimeFlags & ODF_CONFIGURATION_POLL_PASSED) &&
       (m_instanceDiscoveryPending || (static_cast<uint32_t>(time(nullptr) - m_instancePollState.getLastCompleted()) > getCustomAttributeAsUInt32(_T("SysConfig:DataCollection.InstancePollingInterval"), g_instancePollingInterval))))
   {
      success = m_instancePollState.schedule();
      m_instanceDiscoveryPending = false;
   }
   unlockProperties();
   return success;
}

/**
 * Perform instance discovery poll on data collection target
 */
void DataCollectionTarget::instanceDiscoveryPoll(PollerInfo *poller, ClientSession *session, uint32_t requestId)
{
   lockProperties();
   if (m_isDeleteInitiated || IsShutdownInProgress())
   {
      m_instancePollState.complete(0);
      unlockProperties();
      return;
   }
   unlockProperties();

   poller->setStatus(_T("wait for lock"));
   pollerLock(instance);

   if (IsShutdownInProgress())
   {
      pollerUnlock();
      return;
   }

   m_pollRequestor = session;
   m_pollRequestId = requestId;

   if (m_status == STATUS_UNMANAGED)
   {
      sendPollerMsg(POLLER_WARNING _T("%s %s is unmanaged, instance discovery  poll aborted\r\n"), getObjectClassName(), m_name);
      nxlog_debug_tag(DEBUG_TAG_INSTANCE_POLL, 5, _T("%s %s [%u] is unmanaged, instance discovery poll aborted"), getObjectClassName(), m_name, m_id);
      pollerUnlock();
      return;
   }

   sendPollerMsg(_T("Starting instance discovery poll of %s %s\r\n"), getObjectClassName(), m_name);
   nxlog_debug_tag(DEBUG_TAG_INSTANCE_POLL, 4, _T("Starting instance discovery poll of %s \"%s\" [%u]"), getObjectClassName(), m_name, m_id);

   // Check if DataCollectionTarget is marked as unreachable
   if (!(m_state & DCSF_UNREACHABLE))
   {
      poller->setStatus(_T("instance discovery"));
      doInstanceDiscovery(requestId);

      // Update time intervals in data collection objects
      updateDataCollectionTimeIntervals();

      // Execute hook script
      poller->setStatus(_T("hook"));
      executeHookScript(_T("InstancePoll"));
   }
   else
   {
      sendPollerMsg(POLLER_WARNING _T("%s is marked as unreachable, instance discovery poll aborted\r\n"), getObjectClassName());
      nxlog_debug_tag(DEBUG_TAG_INSTANCE_POLL, 4, _T("%s is marked as unreachable, instance discovery poll aborted"), getObjectClassName());
   }

   // Finish instance discovery poll
   poller->setStatus(_T("cleanup"));
   pollerUnlock();
   nxlog_debug_tag(DEBUG_TAG_INSTANCE_POLL, 4, _T("Finished instance discovery poll of %s %s (ID: %d)"), getObjectClassName(), m_name, m_id);
}

/**
 * Get list of instances for given data collection object. Default implementation always returns nullptr.
 */
StringMap *DataCollectionTarget::getInstanceList(DCObject *dco)
{
   return nullptr;
}

/**
 * Parse instance discovery data into table name and name of the column that provides instance name.
 */
void DataCollectionTarget::parseInstanceDiscoveryTableName(const wchar_t *definition, wchar_t *tableName, wchar_t *nameColumn)
{
   const wchar_t *d = wcsrchr(definition, _T(')'));
   if (d != nullptr)
      d++;
   else
      d = definition;

   const wchar_t *c = wcsrchr(d, L':');
   if (c != nullptr)
   {
      memcpy(tableName, d, (c - d) * sizeof(wchar_t));
      tableName[c - d] = 0;
      wcslcpy(nameColumn, c + 1, MAX_DB_STRING);
   }
   else
   {
      wcslcpy(tableName, d, MAX_DB_STRING);
      nameColumn[0] = 0;
   }
}

/**
 * Cancellation checkpoint for instance discovery loop
 */
#define INSTANCE_DISCOVERY_CANCELLATION_CHECKPOINT \
if (g_flags & AF_SHUTDOWN) \
{ \
   object->clearBusyFlag(); \
   for(i++; i < rootObjects.size(); i++) \
      rootObjects.get(i)->clearBusyFlag(); \
   delete instances; \
   changed = false; \
   break; \
}

/**
 * Do instance discovery
 */
void DataCollectionTarget::doInstanceDiscovery(uint32_t requestId)
{
   sendPollerMsg(_T("Running DCI instance discovery\r\n"));

   // collect instance discovery DCIs
   SharedObjectArray<DCObject> rootObjects;
   readLockDciAccess();
   for(int i = 0; i < m_dcObjects.size(); i++)
   {
      shared_ptr<DCObject> object = m_dcObjects.getShared(i);
      if ((object->getInstanceDiscoveryMethod() != IDM_NONE) && (object->getInstanceDiscoveryMethod() != IDM_PUSH) &&(object->getStatus() != ITEM_STATUS_DISABLED))
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
      nxlog_debug_tag(DEBUG_TAG_INSTANCE_POLL, 5, _T("DataCollectionTarget::doInstanceDiscovery(%s [%u]): Updating instances for instance discovery DCO %s [%d]"),
                m_name, m_id, object->getName().cstr(), object->getId());
      sendPollerMsg(_T("   Updating instances for %s [%d]\r\n"), object->getName().cstr(), object->getId());
      StringMap *instances = (object->getInstanceDiscoveryData() != nullptr) ? getInstanceList(object) : nullptr;
      INSTANCE_DISCOVERY_CANCELLATION_CHECKPOINT;
      if (instances != nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG_INSTANCE_POLL, 5, _T("DataCollectionTarget::doInstanceDiscovery(%s [%u]): read %d values"), m_name, m_id, instances->size());
         StringObjectMap<InstanceDiscoveryData> *filteredInstances = object->filterInstanceList(instances);
         if (filteredInstances != nullptr)
         {
            INSTANCE_DISCOVERY_CANCELLATION_CHECKPOINT;
            if (updateInstances(object, filteredInstances, requestId))
               changed = true;
            delete filteredInstances;
         }
         else
         {
            sendPollerMsg(POLLER_ERROR _T("      Error while filtering instance list\r\n"));
         }
         delete instances;
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_INSTANCE_POLL, 5, _T("DataCollectionTarget::doInstanceDiscovery(%s [%u]): failed to get instance list for DCO %s [%u]"),
                   m_name, m_id, object->getName().cstr(), object->getId());
         sendPollerMsg(POLLER_ERROR _T("      Failed to get instance list\r\n"));
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
static EnumerationCallbackResult FindInstanceCallback(const TCHAR *key, const InstanceDiscoveryData *value, const TCHAR *data)
{
   return !_tcscmp(data, key) ? _STOP : _CONTINUE;
}

/**
 * Data for CreateInstanceDCI
 */
struct CreateInstanceDCOData
{
   DCObject *root;
   shared_ptr<DataCollectionTarget> object;

   CreateInstanceDCOData(DCObject *_root, const shared_ptr<DataCollectionTarget>& _object) : object(_object)
   {
      root = _root;
   }
};

/**
 * Callback for creating instance DCIs
 */
static EnumerationCallbackResult CreateInstanceDCI(const TCHAR *key, const InstanceDiscoveryData *value, CreateInstanceDCOData *data)
{
   auto object = data->object;
   auto root = data->root;

   nxlog_debug_tag(DEBUG_TAG_INSTANCE_POLL, 5, _T("DataCollectionTarget::updateInstances(%s [%u], %s [%u]): creating new DCO for instance \"%s\""),
             object->getName(), object->getId(), root->getName().cstr(), root->getId(), key);
   object->sendPollerMsg(_T("      Creating new DCO for instance \"%s\"\r\n"), key);

   DCObject *dco = root->clone();

   dco->setTemplateId(object->getId(), root->getId());
   dco->setInstanceName(value->getInstanceName());
   dco->setInstanceDiscoveryMethod(IDM_NONE);
   dco->setInstanceDiscoveryData(key);
   dco->setInstanceFilter(nullptr);
   dco->expandInstance();
   dco->setRelatedObject(value->getRelatedObject());
   dco->changeBinding(CreateUniqueId(IDG_ITEM), object, false);
   object->addDCObject(dco, true);
   return _CONTINUE;
}

/**
 * Update instance DCIs created from instance discovery DCI
 */
bool DataCollectionTarget::updateInstances(DCObject *root, StringObjectMap<InstanceDiscoveryData> *instances, uint32_t requestId)
{
   bool changed = false;

   writeLockDciAccess();

   // Delete DCIs for missing instances and update existing
   IntegerArray<uint32_t> deleteList;
   for(int i = 0; i < m_dcObjects.size(); i++)
   {
      DCObject *object = m_dcObjects.get(i);
      if ((object->getTemplateId() != m_id) || (object->getTemplateItemId() != root->getId()))
         continue;

      SharedString dcoInstance = object->getInstanceDiscoveryData();
      if (instances->forEach(FindInstanceCallback, dcoInstance.cstr()) == _STOP)
      {
         // found, remove value from instances
         nxlog_debug_tag(DEBUG_TAG_INSTANCE_POLL, 5, _T("DataCollectionTarget::updateInstances(%s [%u], %s [%u]): instance \"%s\" found"),
                   m_name, m_id, root->getName().cstr(), root->getId(), dcoInstance.cstr());
         InstanceDiscoveryData *instanceObject = instances->get(dcoInstance);
         const wchar_t *name = instanceObject->getInstanceName();
         bool notify = false;
         if (wcscmp(name, object->getInstanceName()))
         {
            object->setInstanceName(name);
            object->updateFromTemplate(root);
            changed = true;
            notify = true;
         }
         if (object->getInstanceGracePeriodStart() > 0)
         {
            object->setInstanceGracePeriodStart(0);
            object->setStatus(ITEM_STATUS_ACTIVE, false);
         }
         if (instanceObject->getRelatedObject() != object->getRelatedObject())
         {
            object->setRelatedObject(instanceObject->getRelatedObject());
            changed = true;
            notify = true;
         }
         instances->remove(dcoInstance);
         if (notify)
            NotifyClientsOnDCIUpdate(*this, object);
      }
      else
      {
         time_t retentionTime = ((object->getInstanceRetentionTime() != -1) ? object->getInstanceRetentionTime() : g_instanceRetentionTime) * 86400;

         if ((object->getInstanceGracePeriodStart() == 0) && (retentionTime > 0))
         {
            object->setInstanceGracePeriodStart(time(nullptr));
            object->setStatus(ITEM_STATUS_DISABLED, false);
            nxlog_debug_tag(DEBUG_TAG_INSTANCE_POLL, 5, _T("DataCollectionTarget::updateInstances(%s [%u], %s [%u]): instance \"%s\" not found, grace period started"),
                      m_name, m_id, root->getName().cstr(), root->getId(), dcoInstance.cstr());
            sendPollerMsg(_T("      Existing instance \"%s\" not found, grace period started\r\n"), dcoInstance.cstr());
            changed = true;
         }

         if ((retentionTime == 0) || ((time(nullptr) - object->getInstanceGracePeriodStart()) > retentionTime))
         {
            // not found, delete DCO
            nxlog_debug_tag(DEBUG_TAG_INSTANCE_POLL, 5, _T("DataCollectionTarget::updateInstances(%s [%u], %s [%u]): instance \"%s\" not found, instance DCO will be deleted"),
                      m_name, m_id, root->getName().cstr(), root->getId(), dcoInstance.cstr());
            sendPollerMsg(_T("      Existing instance \"%s\" not found and will be deleted\r\n"), dcoInstance.cstr());
            deleteList.add(object->getId());
            changed = true;
         }
      }
   }

   for(int i = 0; i < deleteList.size(); i++)
      deleteDCObject(deleteList.get(i), false);

   // Create new instances
   if (instances->size() > 0)
   {
      CreateInstanceDCOData data(root, self());
      instances->forEach(CreateInstanceDCI, &data);
      changed = true;
   }

   unlockDciAccess();
   return changed;
}

/**
 * Create new push DCI instance. Returns shared_ptr to created DCI on success or nullptr when no matching instance discovery DCI is found.
 */
shared_ptr<DCObject> DataCollectionTarget::createPushDciInstance(const wchar_t *name)
{
   bool created = false;

   // collect instance discovery DCIs
   SharedObjectArray<DCObject> rootObjects;
   readLockDciAccess();
   for(int i = 0; i < m_dcObjects.size(); i++)
   {
      shared_ptr<DCObject> object = m_dcObjects.getShared(i);
      if ((object->getInstanceDiscoveryMethod() == IDM_PUSH) && (object->getStatus() != ITEM_STATUS_DISABLED))
      {
         object->setBusyFlag();
         rootObjects.add(object);
      }
   }
   unlockDciAccess();

   for(int i = 0; i < rootObjects.size(); i++)
   {
      shared_ptr<DCObject> object = rootObjects.getShared(i);
      SharedString instanceData = object->getInstanceDiscoveryData();
      if (!instanceData.isEmpty())
      {
         const char *eptr;
         int eoffset;
         PCRE *preg = _pcre_compile_t(reinterpret_cast<const PCRE_TCHAR*>(instanceData.cstr()), PCRE_COMMON_FLAGS | PCRE_CASELESS, &eptr, &eoffset, nullptr);
         if (preg != nullptr)
         {
            int ovector[30];
            int rc = _pcre_exec_t(preg, nullptr, reinterpret_cast<const PCRE_TCHAR*>(name), static_cast<int>(wcslen(name)), 0, 0, ovector, 30);
            if (rc >= 2)
            {
               int len = ovector[3] - ovector[2];
               wchar_t instanceName[MAX_DB_STRING];
               wcsncpy(instanceName, &name[ovector[2]], len);
               instanceName[len] = 0;

               StringMap instances;
               instances.set(instanceName, instanceName);
               StringObjectMap<InstanceDiscoveryData> *filteredInstances = object->filterInstanceList(&instances);
               if ((filteredInstances != nullptr) && !filteredInstances->isEmpty())
               {
                  nxlog_debug_tag(DEBUG_TAG_INSTANCE_POLL, 5, _T("DataCollectionTarget::createPushDciInstance(%s [%u]): creating new DCO \"%s\" (instance=\"%s\")"), m_name, m_id, name, instanceName);
                  CreateInstanceDCOData data(object.get(), self());
                  filteredInstances->forEach(CreateInstanceDCI, &data);
                  created = true;
               }
               else
               {
                  nxlog_debug_tag(DEBUG_TAG_INSTANCE_POLL, 5, _T("DataCollectionTarget::createPushDciInstance(%s [%u]): instance \"%s\" filtered out for IDM_PUSH DCO \"%s\""),
                            m_name, m_id, instanceName, object->getName().cstr());
               }
               delete filteredInstances;
            }
            _pcre_free_t(preg);
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG_INSTANCE_POLL, 5, _T("DataCollectionTarget::createPushDciInstance(%s [%u]): cannot compile regex \"%s\" for IDM_PUSH DCO \"%s\""),
                      m_name, m_id, instanceData.cstr(), object->getName().cstr());
         }
      }
      object->clearBusyFlag();
   }

   return created ? getDCObjectByName(name, 0) : nullptr;
}

/**
 * Get last (current) DCI values.
 */
uint32_t DataCollectionTarget::getDataCollectionSummary(NXCPMessage *msg, bool objectTooltipOnly, bool overviewOnly, bool includeNoValueObjects, uint32_t userId)
{
   readLockDciAccess();

   uint32_t fieldId = VID_DCI_VALUES_BASE, count = 0;
   for(int i = 0; i < m_dcObjects.size(); i++)
   {
      DCObject *object = m_dcObjects.get(i);
      if ((object->hasValue() || includeNoValueObjects) &&
          (!objectTooltipOnly || object->isShowOnObjectTooltip()) &&
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
uint32_t DataCollectionTarget::getDataCollectionSummary(json_t *values, bool objectTooltipOnly, bool overviewOnly, bool includeNoValueObjects, uint32_t userId, std::function<bool(DCObject*)> filter)
{
   readLockDciAccess();

   for(int i = 0; i < m_dcObjects.size(); i++)
   {
      DCObject *object = m_dcObjects.get(i);
      if ((object->hasValue() || includeNoValueObjects) &&
          (!objectTooltipOnly || object->isShowOnObjectTooltip()) &&
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

/**
 * Get last (current) DCI values that should be shown on tooltip
 */
void DataCollectionTarget::getTooltipLastValues(NXCPMessage *msg, uint32_t userId, uint32_t *index)
{
   readLockDciAccess();

   for(int i = 0; i < m_dcObjects.size(); i++)
   {
      DCObject *object = m_dcObjects.get(i);
      if (object->hasValue() && object->isShowOnObjectTooltip() &&
          object->hasAccess(userId))
      {
         if (object->getType() == DCO_TYPE_ITEM)
         {
            object->fillLastValueSummaryMessage(msg, *index);
            *index += 50;
         }
      }
   }
   unlockDciAccess();
}

/**
 * Hook for data collection load
 */
void DataCollectionTarget::onDataCollectionLoad()
{
   super::onDataCollectionLoad();
   calculateProxyLoad();
}

/**
 * Hook for data collection change
 */
void DataCollectionTarget::onDataCollectionChange()
{
   super::onDataCollectionChange();
   calculateProxyLoad();
}

/**
 * Hook for instance discovery configuration change
 */
void DataCollectionTarget::onInstanceDiscoveryChange()
{
   super::onInstanceDiscoveryChange();
   nxlog_debug_tag(_T("obj.dc"), 5, _T("Instance discovery configuration change for %s [%u]"), m_name, m_id);
   lockProperties();
   m_instanceDiscoveryPending = true;
   unlockProperties();
}

/**
 * Calculate proxy load factor
 */
void DataCollectionTarget::calculateProxyLoad()
{
   double loadFactor = 0;
   readLockDciAccess();
   for(int i = 0; i < m_dcObjects.size(); i++)
   {
      DCObject *object = m_dcObjects.get(i);
      if ((object->getDataSource() == DS_SNMP_AGENT) && (object->getStatus() == ITEM_STATUS_ACTIVE))
      {
         if (object->isAdvancedSchedule())
            loadFactor += 12;  // assume 5 minutes interval for custom schedule
         else
            loadFactor += 3600.0 / static_cast<double>(object->getEffectivePollingInterval());
      }
   }
   unlockDciAccess();

   m_proxyLoadFactor.store(loadFactor);
}

/**
 * Get list of template type parent objects for NXSL script
 */
NXSL_Array *DataCollectionTarget::getTemplatesForNXSL(NXSL_VM *vm)
{
   NXSL_Array *templates = new NXSL_Array(vm);
   int index = 0;

   readLockParentList();
   for(int i = 0; i < getParentList().size(); i++)
   {
      NetObj *object = getParentList().get(i);
      if ((object->getObjectClass() == OBJECT_TEMPLATE) && object->isTrustedObject(m_id))
      {
         templates->set(index++, object->createNXSLObject(vm));
      }
   }
   unlockParentList();

   return templates;
}

/**
 * Get cache memory usage
 */
uint64_t DataCollectionTarget::getCacheMemoryUsage()
{
   uint64_t cacheSize = 0;
   readLockDciAccess();
   for(int i = 0; i < m_dcObjects.size(); i++)
   {
      DCObject *object = m_dcObjects.get(i);
      if (object->getType() == DCO_TYPE_ITEM)
      {
         cacheSize += static_cast<DCItem*>(object)->getCacheMemoryUsage();
      }
   }
   unlockDciAccess();
   return cacheSize;
}

/**
 * Get effective web service proxy for this node
 */
uint32_t DataCollectionTarget::getEffectiveWebServiceProxy()
{
   uint32_t webServiceProxy = m_webServiceProxy;
   int32_t zoneUIN = getZoneUIN();
   if (IsZoningEnabled() && (webServiceProxy == 0) && (zoneUIN != 0))
   {
      // Use zone default proxy if set
      shared_ptr<Zone> zone = FindZoneByUIN(zoneUIN);
      if (zone != nullptr)
      {
         webServiceProxy = zone->isProxyNode(m_id) ? m_id : zone->getAvailableProxyNodeId(this);
      }
   }
   return (webServiceProxy != 0) ? webServiceProxy : g_dwMgmtNode;
}

/**
 * Update geolocation for device. Object properties assumed to be locked when this method is called.
 */
void DataCollectionTarget::updateGeoLocation(const GeoLocation& geoLocation)
{
   if (geoLocation.getTimestamp() <= m_geoLocation.getTimestamp())
   {
      nxlog_debug_tag(DEBUG_TAG_GEOLOCATION, 4, _T("Location update for device %s [%u] ignored because it is older than last known location"), m_name, m_id);
      return;
   }

   if ((m_flags & DCF_LOCATION_CHANGE_EVENT) && m_geoLocation.isValid() && geoLocation.isValid() && !m_geoLocation.equals(geoLocation))
   {
      EventBuilder(EVENT_GEOLOCATION_CHANGED, m_id)
         .param(_T("newLatitude"), geoLocation.getLatitude())
         .param(_T("newLongitude"), geoLocation.getLongitude())
         .param(_T("newLatitudeAsString"), geoLocation.getLatitudeAsString())
         .param(_T("newLongitudeAsString"), geoLocation.getLongitudeAsString())
         .param(_T("previousLatitude"), m_geoLocation.getLatitude())
         .param(_T("previousLongitude"), m_geoLocation.getLongitude())
         .param(_T("previousLatitudeAsString"), m_geoLocation.getLatitudeAsString())
         .param(_T("previoudLongitudeAsString"), m_geoLocation.getLongitudeAsString())
         .post();
   }
   m_geoLocation = geoLocation;
   setModified(MODIFY_COMMON_PROPERTIES);

   TCHAR key[32];
   _sntprintf(key, 32, _T("GLupd-%u"), m_id);
   ThreadPoolExecuteSerialized(g_mainThreadPool, key, self(), &NetObj::updateGeoLocationHistory, m_geoLocation);

   nxlog_debug_tag(DEBUG_TAG_GEOLOCATION, 4, _T("Location for device %s [%u] set to %s %s)"), m_name, m_id, geoLocation.getLatitudeAsString(), geoLocation.getLatitudeAsString());

   if (m_geoLocationControlMode == GEOLOCATION_RESTRICTED_AREAS)
   {
      bool insideArea = false;
      for(int i = 0; i < m_geoAreas.size(); i++)
      {
         shared_ptr<GeoArea> area = GetGeoArea(m_geoAreas.get(i));
         if ((area != nullptr) && area->containsLocation(geoLocation))
         {
            if (!m_geoLocationRestrictionsViolated)
            {
               nxlog_debug_tag(DEBUG_TAG_GEOLOCATION, 4, _T("Device %s [%u] is within restricted area %s [%u] (current coordinates %s %s)"),
                        m_name, m_id, area->getName(), area->getId(), geoLocation.getLatitudeAsString(), geoLocation.getLatitudeAsString());
               EventBuilder(EVENT_GEOLOCATION_INSIDE_RESTRICTED_AREA, m_id)
                  .param(_T("newLatitude"), geoLocation.getLatitude())
                  .param(_T("newLongitude"), geoLocation.getLongitude())
                  .param(_T("newLatitudeAsString"), geoLocation.getLatitudeAsString())
                  .param(_T("newLongitudeAsString"), geoLocation.getLongitudeAsString())
                  .param(_T("areaName"), area->getName())
                  .param(_T("areaId"), area->getId())
                  .post();

               m_geoLocationRestrictionsViolated = true;
            }
            break;
         }
      }
      if (!insideArea && m_geoLocationRestrictionsViolated)
      {
         nxlog_debug_tag(DEBUG_TAG_GEOLOCATION, 4, _T("Device %s [%u] is outside restricted area (current coordinates %s %s)"),
                  m_name, m_id, geoLocation.getLatitudeAsString(), geoLocation.getLatitudeAsString());
         m_geoLocationRestrictionsViolated = false;
      }
   }
   else if (m_geoLocationControlMode == GEOLOCATION_ALLOWED_AREAS)
   {
      bool insideArea = false;
      for(int i = 0; i < m_geoAreas.size(); i++)
      {
         shared_ptr<GeoArea> area = GetGeoArea(m_geoAreas.get(i));
         if ((area != nullptr) && area->containsLocation(geoLocation))
         {
            insideArea = true;
            break;
         }
      }
      if (!insideArea && !m_geoLocationRestrictionsViolated)
      {
         nxlog_debug_tag(DEBUG_TAG_GEOLOCATION, 4, _T("Device %s [%u] is outside allowed area (current coordinates %s %s)"),
                  m_name, m_id, geoLocation.getLatitudeAsString(), geoLocation.getLatitudeAsString());
         EventBuilder(EVENT_GEOLOCATION_OUTSIDE_ALLOWED_AREA, m_id)
            .param(_T("newLatitude"), geoLocation.getLatitude())
            .param(_T("newLongitude"), geoLocation.getLongitude())
            .param(_T("newLatitudeAsString"), geoLocation.getLatitudeAsString())
            .param(_T("newLongitudeAsString"), geoLocation.getLongitudeAsString())
            .post();

         m_geoLocationRestrictionsViolated = true;
      }
      else if (insideArea && m_geoLocationRestrictionsViolated)
      {
         nxlog_debug_tag(DEBUG_TAG_GEOLOCATION, 4, _T("Device %s [%u] is inside allowed area (current coordinates %s %s)"),
                  m_name, m_id, geoLocation.getLatitudeAsString(), geoLocation.getLatitudeAsString());
         m_geoLocationRestrictionsViolated = false;
      }
   }
}

/**
 * Find matching DCIs using search query object
 */
void DataCollectionTarget::findDcis(const SearchQuery &query, uint32_t userId, SharedObjectArray<DCObject> *result)
{
   readLockDciAccess();
   for(int i = 0; i < m_dcObjects.size(); i++)
   {
      shared_ptr<DCObject> dci = m_dcObjects.getShared(i);
      if (dci->hasAccess(userId) && query.match(*dci))
      {
         result->add(dci);
      }
   }
   unlockDciAccess();
}

/**
 * Build internal communication topology
 */
shared_ptr<NetworkMapObjectList> DataCollectionTarget::buildInternalCommunicationTopology()
{
   auto topology = make_shared<NetworkMapObjectList>();
   topology->setAllowDuplicateLinks(true);
   populateInternalCommunicationTopologyMap(topology.get(), m_id, false, false);
   return topology;
}

/**
 * Build internal connection topology - internal function
 */
void DataCollectionTarget::populateInternalCommunicationTopologyMap(NetworkMapObjectList *map, uint32_t currentObjectId, bool agentConnectionOnly, bool checkAllProxies)
{
   if ((map->getNumObjects() == 0) && (currentObjectId == m_id))
      map->addObject(m_id);
}

/**
 * Check if geo area is referenced
 */
bool DataCollectionTarget::isGeoAreaReferenced(uint32_t id) const
{
   bool result = false;
   lockProperties();
   for(int i = 0; i < m_geoAreas.size(); i++)
      if (m_geoAreas.get(i) == id)
      {
         result = true;
         break;
      }
   unlockProperties();
   return result;
}

/**
 * Remove geo area from object
 */
void DataCollectionTarget::removeGeoArea(uint32_t id)
{
   lockProperties();
   for(int i = 0; i < m_geoAreas.size(); i++)
      if (m_geoAreas.get(i) == id)
      {
         m_geoAreas.remove(i);
         setModified(MODIFY_DC_TARGET);
         break;
      }
   unlockProperties();
}
