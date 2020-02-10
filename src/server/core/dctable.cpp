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
** File: dctable.cpp
**
**/

#include "nxcore.h"

/**
 * Column ID cache
 */
TC_ID_MAP_ENTRY __EXPORT *DCTable::m_cache = NULL;
int __EXPORT DCTable::m_cacheSize = 0;
int __EXPORT DCTable::m_cacheAllocated = 0;
MUTEX __EXPORT DCTable::m_cacheMutex = MutexCreate();

/**
 * Compare cache element's name to string key
 */
static int CompareCacheElements(const void *key, const void *element)
{
	return _tcsicmp((const TCHAR *)key, ((TC_ID_MAP_ENTRY *)element)->name);
}

/**
 * Compare names of two cache elements
 */
static int CompareCacheElements2(const void *e1, const void *e2)
{
	return _tcsicmp(((TC_ID_MAP_ENTRY *)e1)->name, ((TC_ID_MAP_ENTRY *)e2)->name);
}

/**
 * Get column ID from column name
 */
INT32 DCTable::columnIdFromName(const TCHAR *name)
{
	TC_ID_MAP_ENTRY buffer;

	// check that column name is valid
	if ((name == NULL) || (*name == 0))
		return 0;

	MutexLock(m_cacheMutex);

	TC_ID_MAP_ENTRY *entry = (TC_ID_MAP_ENTRY *)bsearch(name, m_cache, m_cacheSize, sizeof(TC_ID_MAP_ENTRY), CompareCacheElements);
	if (entry == NULL)
	{
		// Not in cache, go to database
		DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

		DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT column_id FROM dct_column_names WHERE column_name=?"));
		if (hStmt != NULL)
		{
			DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, name, DB_BIND_STATIC);
			DB_RESULT hResult = DBSelectPrepared(hStmt);
			if (hResult != NULL)
			{
				entry = &buffer;
				nx_strncpy(entry->name, name, MAX_COLUMN_NAME);
				if (DBGetNumRows(hResult) > 0)
				{
					// found in database
					entry->id = DBGetFieldLong(hResult, 0, 0);
				}
				else
				{
					// no such column name in database
					entry->id = CreateUniqueId(IDG_DCT_COLUMN);

					// update database
					DB_STATEMENT hStmt2 = DBPrepare(hdb, _T("INSERT INTO dct_column_names (column_id,column_name) VALUES (?,?)"));
					if (hStmt2 != NULL)
					{
						DBBind(hStmt2, 1, DB_SQLTYPE_INTEGER, entry->id);
						DBBind(hStmt2, 2, DB_SQLTYPE_VARCHAR, name, DB_BIND_STATIC);
						DBExecute(hStmt2);
						DBFreeStatement(hStmt2);
					}
				}

				DBFreeResult(hResult);

				// Add to cache
				if (m_cacheSize == m_cacheAllocated)
				{
					m_cacheAllocated += 16;
					m_cache = (TC_ID_MAP_ENTRY *)realloc(m_cache, sizeof(TC_ID_MAP_ENTRY) * m_cacheAllocated);
				}
				memcpy(&m_cache[m_cacheSize++], entry, sizeof(TC_ID_MAP_ENTRY));
				qsort(m_cache, m_cacheSize, sizeof(TC_ID_MAP_ENTRY), CompareCacheElements2);

				DbgPrintf(6, _T("DCTable::columnIdFromName(): column name %s added to cache, ID=%d"), name, (int)entry->id);
			}
			DBFreeStatement(hStmt);
		}

		DBConnectionPoolReleaseConnection(hdb);
	}

	MutexUnlock(m_cacheMutex);
	return (entry != NULL) ? entry->id : 0;
}

/**
 * Copy constructor
 */
DCTable::DCTable(const DCTable *src, bool shadowCopy) : DCObject(src, shadowCopy)
{
	m_columns = new ObjectArray<DCTableColumn>(src->m_columns->size(), 8, true);
	for(int i = 0; i < src->m_columns->size(); i++)
		m_columns->add(new DCTableColumn(src->m_columns->get(i)));
   m_thresholds = new ObjectArray<DCTableThreshold>(src->m_thresholds->size(), 4, true);
	for(int i = 0; i < src->m_thresholds->size(); i++)
		m_thresholds->add(new DCTableThreshold(src->m_thresholds->get(i), shadowCopy));
	m_lastValue = (shadowCopy && (src->m_lastValue != NULL)) ? new Table(src->m_lastValue) : NULL;
}

/**
 * Constructor for creating new DCTable from scratch
 */
DCTable::DCTable(UINT32 id, const TCHAR *name, int source, const TCHAR *pollingInterval, const TCHAR *retentionTime,
                 DataCollectionOwner *owner, const TCHAR *description, const TCHAR *systemTag)
        : DCObject(id, name, source, pollingInterval, retentionTime, owner, description, systemTag)
{
	m_columns = new ObjectArray<DCTableColumn>(8, 8, true);
   m_thresholds = new ObjectArray<DCTableThreshold>(0, 4, true);
	m_lastValue = NULL;
}

/**
 * Constructor for creating DCTable from database
 * Assumes that fields in SELECT query are in following order:
 *    item_id,template_id,template_item_id,name,
 *    description,flags,source,snmp_port,polling_interval,retention_time,
 *    status,system_tag,resource_id,proxy_node,perftab_settings,
 *    transformation_script,comments,guid,instd_method,instd_data,
 *    instd_filter,instance,instance_retention_time,grace_period_start,
 *    related_object,polling_schedule_type,retention_type,polling_interval_src,
 *    retention_time_src
 */
DCTable::DCTable(DB_HANDLE hdb, DB_RESULT hResult, int row, DataCollectionOwner *owner, bool useStartupDelay) : DCObject()
{
   TCHAR readBuffer[4096];

   m_owner = owner;
   m_id = DBGetFieldULong(hResult, row, 0);
   m_dwTemplateId = DBGetFieldULong(hResult, row, 1);
   m_dwTemplateItemId = DBGetFieldULong(hResult, row, 2);
	m_name = DBGetField(hResult, row, 3, readBuffer, 4096);
   m_description = DBGetField(hResult, row, 4, readBuffer, 4096);
   m_flags = (WORD)DBGetFieldLong(hResult, row, 5);
   m_source = (BYTE)DBGetFieldLong(hResult, row, 6);
	m_snmpPort = (WORD)DBGetFieldLong(hResult, row, 7);
   m_pollingInterval = DBGetFieldLong(hResult, row, 8);
   m_retentionTime = DBGetFieldLong(hResult, row, 9);
   m_status = (BYTE)DBGetFieldLong(hResult, row, 10);
	m_systemTag = DBGetField(hResult, row, 11, readBuffer, 4096);
	m_dwResourceId = DBGetFieldULong(hResult, row, 12);
	m_sourceNode = DBGetFieldULong(hResult, row, 13);
	m_pszPerfTabSettings = DBGetField(hResult, row, 14, NULL, 0);
   TCHAR *pszTmp = DBGetField(hResult, row, 15, NULL, 0);
   m_comments = DBGetField(hResult, row, 16, NULL, 0);
   m_guid = DBGetFieldGUID(hResult, row, 17);
   setTransformationScript(pszTmp);
   MemFree(pszTmp);
   m_instanceDiscoveryMethod = (WORD)DBGetFieldLong(hResult, row, 18);
   m_instanceDiscoveryData = DBGetField(hResult, row, 19, readBuffer, 4096);
   m_instanceFilterSource = NULL;
   m_instanceFilter = NULL;
   pszTmp = DBGetField(hResult, row, 20, NULL, 0);
   setInstanceFilter(pszTmp);
   MemFree(pszTmp);
   m_instance = DBGetField(hResult, row, 21, readBuffer, 4096);
   m_instanceRetentionTime = DBGetFieldLong(hResult, row, 22);
   m_instanceGracePeriodStart = DBGetFieldLong(hResult, row, 23);
   m_relatedObject = DBGetFieldLong(hResult, row, 24);
   m_pollingScheduleType = static_cast<BYTE>(DBGetFieldULong(hResult, row, 25));
   m_retentionType = static_cast<BYTE>(DBGetFieldULong(hResult, row, 26));
   m_pollingIntervalSrc = (m_pollingScheduleType == DC_POLLING_SCHEDULE_CUSTOM) ? DBGetField(hResult, row, 27, NULL, 0) : NULL;
   m_retentionTimeSrc = (m_retentionType == DC_RETENTION_CUSTOM) ? DBGetField(hResult, row, 28, NULL, 0) : NULL;

   int effectivePollingInterval = getEffectivePollingInterval();
   m_startTime = (useStartupDelay && (effectivePollingInterval > 0)) ? time(NULL) + rand() % (effectivePollingInterval / 2) : 0;

	m_columns = new ObjectArray<DCTableColumn>(8, 8, true);
	m_lastValue = NULL;

	DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT column_name,flags,snmp_oid,display_name FROM dc_table_columns WHERE table_id=? ORDER BY sequence_number"));
	if (hStmt != NULL)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
		DB_RESULT hColumnList = DBSelectPrepared(hStmt);
		if (hColumnList != NULL)
		{
			int count = DBGetNumRows(hColumnList);
			for(int i = 0; i < count; i++)
				m_columns->add(new DCTableColumn(hColumnList, i));
			DBFreeResult(hColumnList);
		}
		DBFreeStatement(hStmt);
	}

   loadAccessList(hdb);
   loadCustomSchedules(hdb);

   m_thresholds = new ObjectArray<DCTableThreshold>(0, 4, true);
   loadThresholds(hdb);

   updateTimeIntervalsInternal();
}

/**
 * Create DCTable from import file
 */
DCTable::DCTable(ConfigEntry *config, DataCollectionOwner *owner) : DCObject(config, owner)
{
	ConfigEntry *columnsRoot = config->findEntry(_T("columns"));
	if (columnsRoot != NULL)
	{
		ObjectArray<ConfigEntry> *columns = columnsRoot->getSubEntries(_T("column#*"));
		m_columns = new ObjectArray<DCTableColumn>(columns->size(), 8, true);
		for(int i = 0; i < columns->size(); i++)
		{
			m_columns->add(new DCTableColumn(columns->get(i)));
		}
		delete columns;
	}
	else
	{
   	m_columns = new ObjectArray<DCTableColumn>(8, 8, true);
	}

	ConfigEntry *thresholdsRoot = config->findEntry(_T("thresholds"));
	if (thresholdsRoot != NULL)
	{
		ObjectArray<ConfigEntry> *thresholds = thresholdsRoot->getSubEntries(_T("threshold#*"));
		m_thresholds = new ObjectArray<DCTableThreshold>(thresholds->size(), 8, true);
		for(int i = 0; i < thresholds->size(); i++)
		{
			m_thresholds->add(new DCTableThreshold(thresholds->get(i)));
		}
		delete thresholds;
	}
	else
	{
      m_thresholds = new ObjectArray<DCTableThreshold>(0, 4, true);
	}

   m_lastValue = NULL;
}

/**
 * Destructor
 */
DCTable::~DCTable()
{
	delete m_columns;
   delete m_thresholds;
   if (m_lastValue != NULL)
      m_lastValue->decRefCount();
}

/**
 * Delete all collected data
 */
bool DCTable::deleteAllData()
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   lock();
   TCHAR query[256];
   if (g_flags & AF_SINGLE_TABLE_PERF_DATA)
   {
      if (g_dbSyntax == DB_SYNTAX_TSDB)
         _sntprintf(query, 256, _T("DELETE FROM tdata_sc_%s WHERE item_id=%u"), getStorageClassName(getStorageClass()), m_id);
      else
         _sntprintf(query, 256, _T("DELETE FROM tdata WHERE item_id=%u"), m_id);
   }
   else
   {
      _sntprintf(query, 256, _T("DELETE FROM tdata_%u WHERE item_id=%u"), m_owner->getId(), m_id);
   }
	bool success = DBQuery(hdb, query);
   unlock();

   DBConnectionPoolReleaseConnection(hdb);
   return success;
}

/**
 * Delete single DCI entry
 */
bool DCTable::deleteEntry(time_t timestamp)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   lock();
   TCHAR query[256];
   if (g_flags & AF_SINGLE_TABLE_PERF_DATA)
   {
      if (g_dbSyntax == DB_SYNTAX_TSDB)
      {
         _sntprintf(query, 256, _T("DELETE FROM tdata_sc_%s WHERE item_id=%u AND tdata_timestamp=%u"),
                  getStorageClassName(getStorageClass()), m_id, (UINT32)timestamp);
      }
      else
      {
         _sntprintf(query, 256, _T("DELETE FROM tdata WHERE item_id=%u AND tdata_timestamp=%u"), m_id, (UINT32)timestamp);
      }
   }
   else
   {
      _sntprintf(query, 256, _T("DELETE FROM tdata_%u WHERE item_id=%u AND tdata_timestamp=%u"), m_owner->getId(), m_id, (UINT32)timestamp);
   }
   bool success = DBQuery(hdb, query);
   unlock();

   DBConnectionPoolReleaseConnection(hdb);
   return success;
}

/**
 * Process new collected value. Should return true on success.
 * If returns false, current poll result will be converted into data collection error.
 *
 * @return true on success
 */
bool DCTable::processNewValue(time_t timestamp, void *value, bool *updateStatus)
{
   *updateStatus = false;
   lock();

   // Normally m_owner shouldn't be NULL for polled items, but who knows...
   if (m_owner == NULL)
   {
      unlock();
      static_cast<Table*>(value)->decRefCount();
      return false;
   }

   // Transform input value
   // Cluster can have only aggregated data, and transformation
   // should not be used on aggregation
   if ((m_owner->getObjectClass() != OBJECT_CLUSTER) || (m_flags & DCF_TRANSFORM_AGGREGATED))
   {
      if (!transform(static_cast<Table*>(value)))
      {
         unlock();
         static_cast<Table*>(value)->decRefCount();
         return false;
      }
   }

   m_dwErrorCount = 0;
   if (m_lastValue != NULL)
      m_lastValue->decRefCount();
	m_lastValue = static_cast<Table*>(value);
	m_lastValue->setTitle(m_description);
   m_lastValue->setSource(m_source);

	// Copy required fields into local variables
	UINT32 tableId = m_id;
	UINT32 nodeId = m_owner->getId();
   bool save = (m_retentionType != DC_RETENTION_NONE);

   static_cast<Table*>(value)->incRefCount();

   unlock();

	// Save data to database
	// Object is unlocked, so only local variables can be used
   if (save)
   {
	   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
      if (!DBBegin(hdb))
      {
   	   DBConnectionPoolReleaseConnection(hdb);
         return true;
      }

      bool success = false;
	   Table *data = static_cast<Table*>(value);

	   DB_STATEMENT hStmt;
	   if (g_flags & AF_SINGLE_TABLE_PERF_DATA)
	   {
	      if (g_dbSyntax == DB_SYNTAX_TSDB)
	      {
	         TCHAR query[256];
	         _sntprintf(query, 256, _T("INSERT INTO tdata_sc_%s (item_id,tdata_timestamp,tdata_value) VALUES (?,?,?)"),
	                  getStorageClassName(getStorageClass()));
            hStmt = DBPrepare(hdb, query);
	      }
	      else
	      {
	         hStmt = DBPrepare(hdb, _T("INSERT INTO tdata (item_id,tdata_timestamp,tdata_value) VALUES (?,?,?)"));
	      }
	   }
	   else
	   {
	      TCHAR query[256];
	      _sntprintf(query, 256, _T("INSERT INTO tdata_%u (item_id,tdata_timestamp,tdata_value) VALUES (?,?,?)"), nodeId);
	      hStmt = DBPrepare(hdb, query);
	   }
	   if (hStmt != NULL)
	   {
		   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, tableId);
		   DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, (INT32)timestamp);
		   DBBind(hStmt, 3, DB_SQLTYPE_TEXT, DB_CTYPE_UTF8_STRING, data->createPackedXML(), DB_BIND_DYNAMIC);
	      success = DBExecute(hStmt);
		   DBFreeStatement(hStmt);
	   }

      if (success)
         DBCommit(hdb);
      else
         DBRollback(hdb);

	   DBConnectionPoolReleaseConnection(hdb);
   }
   if ((g_offlineDataRelevanceTime <= 0) || (timestamp > (time(NULL) - g_offlineDataRelevanceTime)))
      checkThresholds(static_cast<Table*>(value));

   if (g_flags & AF_PERFDATA_STORAGE_DRIVER_LOADED)
      PerfDataStorageRequest(this, timestamp, static_cast<Table*>(value));

   static_cast<Table*>(value)->decRefCount();
   return true;
}

/**
 * Transform received value
 */
bool DCTable::transform(Table *value)
{
   if (m_transformationScript == NULL)
      return true;

   bool success = false;
   ScriptVMHandle vm = CreateServerScriptVM(m_transformationScript, m_owner, this);
   if (vm.isValid())
   {
      NXSL_Value *nxslValue = vm->createValue(new NXSL_Object(vm, &g_nxslStaticTableClass, value));

      // remove lock from DCI for script execution to avoid deadlocks
      unlock();
      success = vm->run(1, &nxslValue);
      lock();
      if (!success)
      {
         if (vm->getErrorCode() == NXSL_ERR_EXECUTION_ABORTED)
         {
            DbgPrintf(6, _T("Transformation script for DCI \"%s\" [%d] on node %s [%d] aborted"),
                            m_description.cstr(), m_id, getOwnerName(), getOwnerId());
         }
         else
         {
            time_t now = time(NULL);
            if (m_lastScriptErrorReport + ConfigReadInt(_T("DataCollection.ScriptErrorReportInterval"), 86400) < now)
            {
               TCHAR buffer[1024];
               _sntprintf(buffer, 1024, _T("DCI::%s::%d::TransformationScript"), getOwnerName(), m_id);
               PostDciEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, m_id, "ssd", buffer, vm->getErrorText(), m_id);
               nxlog_write(NXLOG_WARNING, _T("Failed to execute transformation script for object %s [%u] DCI %s [%u] (%s)"),
                        getOwnerName(), getOwnerId(), m_name.cstr(), m_id, vm->getErrorText());
               m_lastScriptErrorReport = now;
            }
         }
      }
      vm.destroy();
   }
   else if (vm.failureReason() != ScriptVMFailureReason::SCRIPT_IS_EMPTY)
   {
      time_t now = time(NULL);
      if (m_lastScriptErrorReport + ConfigReadInt(_T("DataCollection.ScriptErrorReportInterval"), 86400) < now)
      {
         TCHAR buffer[1024];
         _sntprintf(buffer, 1024, _T("DCI::%s::%d::TransformationScript"), getOwnerName(), m_id);
         PostDciEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, m_id, "ssd", buffer, _T("Script load error"), m_id);
         nxlog_write(NXLOG_WARNING, _T("Failed to load transformation script for object %s [%u] DCI %s [%u]"),
                  getOwnerName(), getOwnerId(), m_name.cstr(), m_id);
         m_lastScriptErrorReport = now;
      }
   }
   return success;
}

/**
 * Check thresholds
 */
void DCTable::checkThresholds(Table *value)
{
	static const TCHAR *paramNames[] = { _T("dciName"), _T("dciDescription"), _T("dciId"), _T("row"), _T("instance") };

   lock();
   for(int row = 0; row < value->getNumRows(); row++)
   {
      TCHAR instance[MAX_RESULT_LENGTH];
      value->buildInstanceString(row, instance, MAX_RESULT_LENGTH);
      for(int i = 0; i < m_thresholds->size(); i++)
      {
		   DCTableThreshold *t = m_thresholds->get(i);
         ThresholdCheckResult result = t->check(value, row, instance);
         switch(result)
         {
            case ThresholdCheckResult::ACTIVATED:
               PostDciEventWithNames(t->getActivationEvent(), m_owner->getId(), m_id, "ssids", paramNames, m_name.cstr(), m_description.cstr(), m_id, row, instance);
               if (!(m_flags & DCF_ALL_THRESHOLDS))
                  i = m_thresholds->size();  // Stop processing (for current row)
               NotifyClientsOnThresholdChange(m_owner->getId(), m_id, t->getId(), instance, result);
               break;
            case ThresholdCheckResult::DEACTIVATED:
               PostDciEventWithNames(t->getDeactivationEvent(), m_owner->getId(), m_id, "ssids", paramNames, m_name.cstr(), m_description.cstr(), m_id, row, instance);
               NotifyClientsOnThresholdChange(m_owner->getId(), m_id, t->getId(), instance, result);
               break;
            case ThresholdCheckResult::ALREADY_ACTIVE:
				   i = m_thresholds->size();  // Threshold condition still true, stop processing
               break;
            default:
               break;
         }
      }
   }
   unlock();
}

/**
 * Process new data collection error
 */
void DCTable::processNewError(bool noInstance, time_t now)
{
	m_dwErrorCount++;
}

/**
 * Save information about threshold state before maintenance
 */
void DCTable::updateThresholdsBeforeMaintenanceState()
{
   lock();
   for(int i = 0; i < m_thresholds->size(); i++)
   {
      m_thresholds->get(i)->updateBeforeMaintenanceState();
   }
   unlock();
}

/**
 * Generate events based on saved state before maintenance
 */
void DCTable::generateEventsBasedOnThrDiff()
{
   lock();
   TableThresholdCbData data;
   for(int i = 0; i < m_thresholds->size(); i++)
   {
      data.threshold = m_thresholds->get(i);
      data.table = this;
      m_thresholds->get(i)->generateEventsBasedOnThrDiff(&data);
   }
   unlock();
}

/**
 * Save to database
 */
bool DCTable::saveToDatabase(DB_HANDLE hdb)
{
   static const TCHAR *columns[] = {
      _T("node_id"), _T("template_id"), _T("template_item_id"), _T("name"), _T("description"), _T("flags"), _T("source"),
      _T("snmp_port"), _T("polling_interval"), _T("retention_time"), _T("status"), _T("system_tag"), _T("resource_id"),
      _T("proxy_node"), _T("perftab_settings"), _T("transformation_script"), _T("comments"), _T("guid"),
      _T("instd_method"), _T("instd_data"), _T("instd_filter"), _T("instance"), _T("instance_retention_time"),
      _T("grace_period_start"), _T("related_object"), _T("polling_interval_src"), _T("retention_time_src"),
      _T("polling_schedule_type"), _T("retention_type"),
      NULL
   };

	DB_STATEMENT hStmt = DBPrepareMerge(hdb, _T("dc_tables"), _T("item_id"), m_id, columns);
	if (hStmt == NULL)
		return FALSE;

   lock();

	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, (m_owner == NULL) ? (UINT32)0 : m_owner->getId());
	DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_dwTemplateId);
	DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_dwTemplateItemId);
	DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, m_name, DB_BIND_STATIC, MAX_ITEM_NAME - 1);
	DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, m_description, DB_BIND_STATIC, MAX_DB_STRING - 1);
	DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, (UINT32)m_flags);
	DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, (INT32)m_source);
	DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, (UINT32)m_snmpPort);
	DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, (INT32)m_pollingInterval);
	DBBind(hStmt, 10, DB_SQLTYPE_INTEGER, (INT32)m_retentionTime);
	DBBind(hStmt, 11, DB_SQLTYPE_INTEGER, (INT32)m_status);
	DBBind(hStmt, 12, DB_SQLTYPE_VARCHAR, m_systemTag, DB_BIND_STATIC, MAX_DB_STRING);
	DBBind(hStmt, 13, DB_SQLTYPE_INTEGER, m_dwResourceId);
	DBBind(hStmt, 14, DB_SQLTYPE_INTEGER, m_sourceNode);
	DBBind(hStmt, 15, DB_SQLTYPE_TEXT, m_pszPerfTabSettings, DB_BIND_STATIC);
   DBBind(hStmt, 16, DB_SQLTYPE_TEXT, m_transformationScriptSource, DB_BIND_STATIC);
   DBBind(hStmt, 17, DB_SQLTYPE_TEXT, m_comments, DB_BIND_STATIC);
   DBBind(hStmt, 18, DB_SQLTYPE_VARCHAR, m_guid);
   DBBind(hStmt, 19, DB_SQLTYPE_INTEGER, (INT32)m_instanceDiscoveryMethod);
   DBBind(hStmt, 20, DB_SQLTYPE_VARCHAR, m_instanceDiscoveryData, DB_BIND_STATIC, MAX_INSTANCE_LEN - 1);
   DBBind(hStmt, 21, DB_SQLTYPE_TEXT, m_instanceFilterSource, DB_BIND_STATIC);
   DBBind(hStmt, 22, DB_SQLTYPE_VARCHAR, m_instance, DB_BIND_STATIC, MAX_INSTANCE_LEN - 1);
   DBBind(hStmt, 23, DB_SQLTYPE_INTEGER, m_instanceRetentionTime);
   DBBind(hStmt, 24, DB_SQLTYPE_INTEGER, (INT32)m_instanceGracePeriodStart);
   DBBind(hStmt, 25, DB_SQLTYPE_INTEGER, m_relatedObject);
   DBBind(hStmt, 26, DB_SQLTYPE_VARCHAR, m_pollingIntervalSrc, DB_BIND_STATIC, MAX_DB_STRING - 1);
   DBBind(hStmt, 27, DB_SQLTYPE_VARCHAR, m_retentionTimeSrc, DB_BIND_STATIC, MAX_DB_STRING - 1);
   TCHAR pt[2], rt[2];
   pt[0] = m_pollingScheduleType + '0';
   pt[1] = 0;
   rt[0] = m_retentionType + '0';
   rt[1] = 0;
   DBBind(hStmt, 28, DB_SQLTYPE_VARCHAR, pt, DB_BIND_STATIC);
   DBBind(hStmt, 29, DB_SQLTYPE_VARCHAR, rt, DB_BIND_STATIC);
   DBBind(hStmt, 30, DB_SQLTYPE_INTEGER, m_id);

	bool result = DBExecute(hStmt);
	DBFreeStatement(hStmt);

	if (result)
	{
		// Save column configuration
		hStmt = DBPrepare(hdb, _T("DELETE FROM dc_table_columns WHERE table_id=?"));
		if (hStmt != NULL)
		{
			DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
			result = DBExecute(hStmt);
			DBFreeStatement(hStmt);
		}
		else
		{
			result = false;
		}

		if (result && (m_columns->size() > 0))
		{
			hStmt = DBPrepare(hdb, _T("INSERT INTO dc_table_columns (table_id,sequence_number,column_name,snmp_oid,flags,display_name) VALUES (?,?,?,?,?,?)"));
			if (hStmt != NULL)
			{
				DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
				for(int i = 0; i < m_columns->size(); i++)
				{
					DCTableColumn *column = m_columns->get(i);
               DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, (INT32)(i + 1));
					DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, column->getName(), DB_BIND_STATIC);
					const SNMP_ObjectId *oid = column->getSnmpOid();
					DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, (oid != NULL) ? (const TCHAR *)oid->toString() : NULL, DB_BIND_TRANSIENT);
					DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, (INT32)column->getFlags());
					DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, column->getDisplayName(), DB_BIND_STATIC);

					result = DBExecute(hStmt);
					if (!result)
						break;
				}
				DBFreeStatement(hStmt);
			}
			else
			{
				result = false;
			}
		}
	}

   saveThresholds(hdb);

   unlock();
	return result ? DCObject::saveToDatabase(hdb) : false;
}

/**
 * Load thresholds from database
 */
bool DCTable::loadThresholds(DB_HANDLE hdb)
{
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT id,activation_event,deactivation_event,sample_count FROM dct_thresholds WHERE table_id=? ORDER BY sequence_number"));
   if (hStmt == NULL)
      return false;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
   DB_RESULT hResult = DBSelectPrepared(hStmt);
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         DCTableThreshold *t = new DCTableThreshold(hdb, hResult, i);
         m_thresholds->add(t);
      }
      DBFreeResult(hResult);
   }
   DBFreeStatement(hStmt);
   return true;
}

/**
 * Save thresholds to database
 */
bool DCTable::saveThresholds(DB_HANDLE hdb)
{
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("DELETE FROM dct_threshold_conditions WHERE threshold_id=?"));
   if (hStmt == NULL)
      return false;

   for(int i = 0; i < m_thresholds->size(); i++)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_thresholds->get(i)->getId());
      DBExecute(hStmt);
   }
   DBFreeStatement(hStmt);

   hStmt = DBPrepare(hdb, _T("DELETE FROM dct_threshold_instances WHERE threshold_id=?"));
   if (hStmt == NULL)
      return false;

   for(int i = 0; i < m_thresholds->size(); i++)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_thresholds->get(i)->getId());
      DBExecute(hStmt);
   }
   DBFreeStatement(hStmt);

   hStmt = DBPrepare(hdb, _T("DELETE FROM dct_thresholds WHERE table_id=?"));
   if (hStmt == NULL)
      return false;
   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
   DBExecute(hStmt);
   DBFreeStatement(hStmt);

   for(int i = 0; i < m_thresholds->size(); i++)
      m_thresholds->get(i)->saveToDatabase(hdb, m_id, i);
   return true;
}

/**
 * Delete table object and collected data from database
 */
void DCTable::deleteFromDatabase()
{
	DCObject::deleteFromDatabase();

   TCHAR szQuery[256];
   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM dc_tables WHERE item_id=%d"), (int)m_id);
   QueueSQLRequest(szQuery);
   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM dc_table_columns WHERE table_id=%d"), (int)m_id);
   QueueSQLRequest(szQuery);

   for(int i = 0; i < m_thresholds->size(); i++)
   {
      _sntprintf(szQuery, 256, _T("DELETE FROM dct_threshold_conditions WHERE threshold_id=%d"), (int)m_thresholds->get(i)->getId());
      QueueSQLRequest(szQuery);
   }

   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM dct_thresholds WHERE table_id=%d"), (int)m_id);
   QueueSQLRequest(szQuery);

   if (m_owner->isDataCollectionTarget())
      static_cast<DataCollectionTarget*>(m_owner)->scheduleItemDataCleanup(m_id);
}

/**
 * Create NXCP message with item data
 */
void DCTable::createMessage(NXCPMessage *pMsg)
{
	DCObject::createMessage(pMsg);

   lock();
	pMsg->setField(VID_NUM_COLUMNS, (UINT32)m_columns->size());
	UINT32 varId = VID_DCI_COLUMN_BASE;
	for(int i = 0; i < m_columns->size(); i++)
	{
		DCTableColumn *column = m_columns->get(i);
		pMsg->setField(varId++, column->getName());
		pMsg->setField(varId++, column->getFlags());
		const SNMP_ObjectId *oid = column->getSnmpOid();
		if (oid != NULL)
			pMsg->setFieldFromInt32Array(varId++, oid->length(), oid->value());
		else
			varId++;
		pMsg->setField(varId++, column->getDisplayName());
		varId += 6;
	}

	pMsg->setField(VID_NUM_THRESHOLDS, (UINT32)m_thresholds->size());
   varId = VID_DCI_THRESHOLD_BASE;
   for(int i = 0; i < m_thresholds->size(); i++)
   {
      varId = m_thresholds->get(i)->fillMessage(pMsg, varId);
   }

   unlock();
}

/**
 * Update data collection object from NXCP message
 */
void DCTable::updateFromMessage(NXCPMessage *pMsg)
{
	DCObject::updateFromMessage(pMsg);

   lock();

	m_columns->clear();
	int count = (int)pMsg->getFieldAsUInt32(VID_NUM_COLUMNS);
	UINT32 varId = VID_DCI_COLUMN_BASE;
	for(int i = 0; i < count; i++)
	{
		m_columns->add(new DCTableColumn(pMsg, varId));
		varId += 10;
	}

	count = (int)pMsg->getFieldAsUInt32(VID_NUM_THRESHOLDS);
   ObjectArray<DCTableThreshold> *newThresholds = new ObjectArray<DCTableThreshold>(count, 8, true);
	varId = VID_DCI_THRESHOLD_BASE;
	for(int i = 0; i < count; i++)
	{
      DCTableThreshold *t = new DCTableThreshold(pMsg, &varId);
		newThresholds->add(t);
      for(int j = 0; j < m_thresholds->size(); j++)
      {
         DCTableThreshold *old = m_thresholds->get(j);
         if (old->getId() == t->getId())
         {
            t->copyState(old);
            break;
         }
      }
	}
   delete m_thresholds;
   m_thresholds = newThresholds;

	unlock();
}

/**
 * Get last collected value
 */
void DCTable::fillLastValueMessage(NXCPMessage *msg)
{
   lock();
	if (m_lastValue != NULL)
	{
		m_lastValue->fillMessage(*msg, 0, -1);
	}
   unlock();
}

/**
 * Get summary of last collected value (to show along simple DCI values)
 */
void DCTable::fillLastValueSummaryMessage(NXCPMessage *pMsg, UINT32 dwId)
{
	lock();
   pMsg->setField(dwId++, m_id);
   pMsg->setField(dwId++, m_name);
   pMsg->setField(dwId++, m_flags);
   pMsg->setField(dwId++, m_description);
   pMsg->setField(dwId++, (WORD)m_source);
   pMsg->setField(dwId++, (WORD)DCI_DT_NULL);  // compatibility: data type
   pMsg->setField(dwId++, _T(""));             // compatibility: value
   pMsg->setField(dwId++, (UINT32)m_lastPoll);
   pMsg->setField(dwId++, (WORD)(matchClusterResource() ? m_status : ITEM_STATUS_DISABLED)); // show resource-bound DCIs as inactive if cluster resource is not on this node
	pMsg->setField(dwId++, (WORD)getType());
	pMsg->setField(dwId++, m_dwErrorCount);
	pMsg->setField(dwId++, m_dwTemplateItemId);
   pMsg->setField(dwId++, (WORD)0);            // compatibility: number of thresholds

	unlock();
}

/**
 * Get data type of given column
 */
int DCTable::getColumnDataType(const TCHAR *name)
{
   int dt = DCI_DT_STRING;
   bool found = false;

   lock();

   // look in column definition first
	for(int i = 0; i < m_columns->size(); i++)
	{
		DCTableColumn *column = m_columns->get(i);
      if (!_tcsicmp(column->getName(), name))
      {
         dt = column->getDataType();
         break;
      }
   }

   // use last values if not found in definitions
   if (!found && (m_lastValue != NULL))
   {
      int index = m_lastValue->getColumnIndex(name);
      if (index != -1)
         dt = m_lastValue->getColumnDataType(index);
   }

   unlock();
   return dt;
}

/**
 * Get last collected value
 */
Table *DCTable::getLastValue()
{
   lock();
   Table *value;
   if (m_lastValue != NULL)
   {
      value = m_lastValue;
      value->incRefCount();
   }
   else
   {
      value = NULL;
   }
   unlock();
   return value;
}

/**
 * Update destination value from source value
 */
#define RECALCULATE_VALUE(dst, src, func, count) \
   { \
      switch(func) \
      { \
         case DCF_FUNCTION_MIN: \
            if (src < dst) dst = src; \
            break; \
         case DCF_FUNCTION_MAX: \
            if (src > dst) dst = src; \
            break; \
         case DCF_FUNCTION_SUM: \
            dst += src; \
            break; \
         case DCF_FUNCTION_AVG: \
            dst = (dst * count + src) / (count + 1); \
            break; \
      } \
   }

/**
 * Merge values
 */
void DCTable::mergeValues(Table *dest, Table *src, int count)
{
   for(int sRow = 0; sRow < src->getNumRows(); sRow++)
   {
      TCHAR instance[MAX_RESULT_LENGTH];

      src->buildInstanceString(sRow, instance, MAX_RESULT_LENGTH);
      int dRow = dest->findRowByInstance(instance);
      if (dRow >= 0)
      {
         for(int j = 0; j < m_columns->size(); j++)
         {
            DCTableColumn *cd = m_columns->get(j);
            if ((cd == NULL) || cd->isInstanceColumn() || (cd->getDataType() == DCI_DT_STRING))
               continue;
            int column = dest->getColumnIndex(cd->getName());
            if (column == -1)
               continue;

            if (cd->getDataType() == DCI_DT_FLOAT)
            {
               double sval = src->getAsDouble(sRow, column);
               double dval = dest->getAsDouble(dRow, column);

               RECALCULATE_VALUE(dval, sval, cd->getAggregationFunction(), count);

               dest->setAt(dRow, column, dval);
            }
            else if ((cd->getDataType() == DCI_DT_UINT) || (cd->getDataType() == DCI_DT_UINT64))
            {
               UINT64 sval = src->getAsUInt64(sRow, column);
               UINT64 dval = dest->getAsUInt64(dRow, column);

               RECALCULATE_VALUE(dval, sval, cd->getAggregationFunction(), count);

               dest->setAt(dRow, column, dval);
            }
            else
            {
               INT64 sval = src->getAsInt64(sRow, column);
               INT64 dval = dest->getAsInt64(dRow, column);

               RECALCULATE_VALUE(dval, sval, cd->getAggregationFunction(), count);

               dest->setAt(dRow, column, dval);
            }
         }
      }
      else
      {
         // no such instance
         dest->copyRow(src, sRow);
      }
   }
}

/**
 * Update columns in resulting table according to definition
 */
void DCTable::updateResultColumns(Table *t)
{
   lock();
   for(int i = 0; i < m_columns->size(); i++)
   {
      DCTableColumn *col = m_columns->get(i);
      int index = t->getColumnIndex(col->getName());
      if (index != -1)
      {
         TableColumnDefinition *cd = t->getColumnDefinitions()->get(index);
         if (cd != NULL)
         {
            cd->setDataType(col->getDataType());
            cd->setInstanceColumn(col->isInstanceColumn());
            cd->setDisplayName(col->getDisplayName());
         }
      }
   }
   unlock();
}

/**
 * Update from template item
 */
void DCTable::updateFromTemplate(DCObject *src)
{
	DCObject::updateFromTemplate(src);

	if (src->getType() != DCO_TYPE_TABLE)
	{
		DbgPrintf(2, _T("INTERNAL ERROR: DCTable::updateFromTemplate(%d, %d): source type is %d"), (int)m_id, (int)src->getId(), src->getType());
		return;
	}

   lock();
	DCTable *table = (DCTable *)src;

   m_columns->clear();
	for(int i = 0; i < table->m_columns->size(); i++)
		m_columns->add(new DCTableColumn(table->m_columns->get(i)));

   // Copy thresholds
   // ***************************
   // First, skip matching thresholds
   int count = std::min(m_thresholds->size(), table->m_thresholds->size());
   int i;
   for(i = 0; i < count; i++)
      if (!m_thresholds->get(i)->equals(table->m_thresholds->get(i)))
         break;
   count = i;   // First unmatched threshold's position

   // Delete all original thresholds starting from first unmatched
   while(count < m_thresholds->size())
      m_thresholds->remove(count);

   // (Re)create thresholds starting from first unmatched
   for(i = count; i < table->m_thresholds->size(); i++)
      m_thresholds->add(new DCTableThreshold(table->m_thresholds->get(i), false));

   unlock();
}

/**
 * Create management pack record
 */
void DCTable::createExportRecord(StringBuffer &xml)
{
   lock();

   xml.append(_T("\t\t\t\t<dctable id=\""));
   xml.append(m_id);
   xml.append(_T("\">\n\t\t\t\t\t<guid>"));
   xml.append(m_guid);
   xml.append(_T("</guid>\n\t\t\t\t\t<name>"));
   xml.append(EscapeStringForXML2(m_name));
   xml.append(_T("</name>\n\t\t\t\t\t<description>"));
   xml.append(EscapeStringForXML2(m_description));
   xml.append(_T("</description>\n\t\t\t\t\t<origin>"));
   xml.append(static_cast<INT32>(m_source));
   xml.append(_T("</origin>\n\t\t\t\t\t<scheduleType>"));
   xml.append(static_cast<INT32>(m_pollingScheduleType));
   xml.append(_T("</scheduleType>\n\t\t\t\t\t<interval>"));
   xml.append(EscapeStringForXML2(m_pollingIntervalSrc));
   xml.append(_T("</interval>\n\t\t\t\t\t<retentionType>"));
   xml.append(static_cast<INT32>(m_retentionType));
   xml.append(_T("</retentionType>\n\t\t\t\t\t<retention>"));
   xml.append(EscapeStringForXML2(m_retentionTimeSrc));
   xml.append(_T("</retention>\n\t\t\t\t\t<systemTag>"));
   xml.append(EscapeStringForXML2(m_systemTag));
   xml.append(_T("</systemTag>\n\t\t\t\t\t<flags>"));
   xml.append(m_flags);
   xml.append(_T("</flags>\n\t\t\t\t\t<snmpPort>"));
   xml.append(m_snmpPort);
   xml.append(_T("</snmpPort>\n\t\t\t\t\t<instanceDiscoveryMethod>"));
   xml.append(m_instanceDiscoveryMethod);
   xml.append(_T("</instanceDiscoveryMethod>\n\t\t\t\t\t<instance>"));
   xml.append(EscapeStringForXML2(m_instance));
   xml.append(_T("</instance>\n\t\t\t\t\t<instanceRetentionTime>"));
   xml.append(m_instanceRetentionTime);
   xml.append(_T("</instanceRetentionTime>\n\t\t\t\t\t<comments>"));
   xml.append(EscapeStringForXML2(m_comments));
   xml.append(_T("</comments>\n"));

	if (m_transformationScriptSource != NULL)
	{
		xml.append(_T("\t\t\t\t\t<transformation>"));
		xml.appendPreallocated(EscapeStringForXML(m_transformationScriptSource, -1));
		xml.append(_T("</transformation>\n"));
	}

   if ((m_schedules != NULL) && (m_schedules->size() > 0))
   {
      xml.append(_T("\t\t\t\t\t<schedules>\n"));
      for(int i = 0; i < m_schedules->size(); i++)
      {
         xml.append(_T("\t\t\t\t\t\t<schedule>"));
         xml.append(EscapeStringForXML2(m_schedules->get(i)));
         xml.append(_T("</schedule>\n"));
      }
      xml.append(_T("\t\t\t\t\t</schedules>\n"));
   }

	if (m_columns != NULL)
	{
	   xml.append(_T("\t\t\t\t\t<columns>\n"));
		for(int i = 0; i < m_columns->size(); i++)
		{
			m_columns->get(i)->createExportRecord(xml, i + 1);
		}
	   xml.append(_T("\t\t\t\t\t</columns>\n"));
	}

	if (m_thresholds != NULL)
	{
	   xml.append(_T("\t\t\t\t\t<thresholds>\n"));
		for(int i = 0; i < m_thresholds->size(); i++)
		{
			m_thresholds->get(i)->createExportRecord(xml, i + 1);
		}
	   xml.append(_T("\t\t\t\t\t</thresholds>\n"));
	}

	if (m_pszPerfTabSettings != NULL)
	{
		xml.append(_T("\t\t\t\t\t<perfTabSettings>"));
		xml.appendPreallocated(EscapeStringForXML(m_pszPerfTabSettings, -1));
		xml.append(_T("</perfTabSettings>\n"));
	}

	if (m_instanceDiscoveryData != NULL)
   {
      xml += _T("\t\t\t\t\t<instanceDiscoveryData>");
      xml.appendPreallocated(EscapeStringForXML(m_instanceDiscoveryData, -1));
      xml += _T("</instanceDiscoveryData>\n");
   }

   if (m_instanceFilterSource != NULL)
   {
      xml.append(_T("\t\t\t\t\t<instanceFilter>"));
      xml.appendPreallocated(EscapeStringForXML(m_instanceFilterSource, -1));
      xml.append(_T("</instanceFilter>\n"));
   }

   unlock();
   xml.append(_T("\t\t\t\t</dctable>\n"));
}

/**
 * Create DCObject from import file
 */
void DCTable::updateFromImport(ConfigEntry *config)
{
   DCObject::updateFromImport(config);

   lock();
   m_columns->clear();
   ConfigEntry *columnsRoot = config->findEntry(_T("columns"));
   if (columnsRoot != NULL)
   {
      ObjectArray<ConfigEntry> *columns = columnsRoot->getSubEntries(_T("column#*"));
      for(int i = 0; i < columns->size(); i++)
      {
         m_columns->add(new DCTableColumn(columns->get(i)));
      }
      delete columns;
   }

   m_thresholds->clear();
   ConfigEntry *thresholdsRoot = config->findEntry(_T("thresholds"));
   if (thresholdsRoot != NULL)
   {
      ObjectArray<ConfigEntry> *thresholds = thresholdsRoot->getSubEntries(_T("threshold#*"));
      for(int i = 0; i < thresholds->size(); i++)
      {
         m_thresholds->add(new DCTableThreshold(thresholds->get(i)));
      }
      delete thresholds;
   }
   unlock();
}

/**
 * Get list of used events
 */
void DCTable::getEventList(IntegerArray<UINT32> *eventList)
{
   lock();
   if (m_thresholds != NULL)
   {
      for(int i = 0; i < m_thresholds->size(); i++)
      {
         eventList->add(m_thresholds->get(i)->getActivationEvent());
         eventList->add(m_thresholds->get(i)->getDeactivationEvent());
      }
   }
   unlock();
}

/*
 * Clone DCTable
 */
DCObject *DCTable::clone() const
{
   return new DCTable(this, false);
}

/**
 * Serialize object to JSON
 */
json_t *DCTable::toJson()
{
   json_t *root = DCObject::toJson();
   lock();
   json_object_set_new(root, "columns", json_object_array(m_columns));
   json_object_set_new(root, "thresholds", json_object_array(m_thresholds));
   unlock();
   return root;
}

/**
 * Get list of all threshold IDs
 */
IntegerArray<UINT32> *DCTable::getThresholdIdList()
{
   IntegerArray<UINT32> *list = new IntegerArray<UINT32>(16, 16);
   lock();
   if (m_thresholds != NULL)
   {
      for(int i = 0; i < m_thresholds->size(); i++)
      {
         list->add(m_thresholds->get(i)->getId());
      }
   }
   unlock();
   return list;
}
