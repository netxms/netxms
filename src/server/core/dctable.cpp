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
** File: dctable.cpp
**
**/

#include "nxcore.h"

/**
 * Column ID cache
 */
TC_ID_MAP_ENTRY *DCTable::m_cache = NULL;
int DCTable::m_cacheSize = 0;
int DCTable::m_cacheAllocated = 0;
MUTEX DCTable::m_cacheMutex = MutexCreate();

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
 * Default constructor
 */
DCTable::DCTable() : DCObject()
{
	m_columns = new ObjectArray<DCTableColumn>(8, 8, true);
   m_thresholds = new ObjectArray<DCTableThreshold>(0, 4, true);
	m_lastValue = NULL;
}

/**
 * Copy constructor
 */
DCTable::DCTable(const DCTable *src) : DCObject(src)
{
	m_columns = new ObjectArray<DCTableColumn>(src->m_columns->size(), 8, true);
	for(int i = 0; i < src->m_columns->size(); i++)
		m_columns->add(new DCTableColumn(src->m_columns->get(i)));
   m_thresholds = new ObjectArray<DCTableThreshold>(src->m_thresholds->size(), 4, true);
	for(int i = 0; i < src->m_thresholds->size(); i++)
		m_thresholds->add(new DCTableThreshold(src->m_thresholds->get(i)));
	m_lastValue = NULL;
}

/**
 * Constructor for creating new DCTable from scratch
 */
DCTable::DCTable(UINT32 id, const TCHAR *name, int source, int pollingInterval, int retentionTime,
	              Template *node, const TCHAR *description, const TCHAR *systemTag)
        : DCObject(id, name, source, pollingInterval, retentionTime, node, description, systemTag)
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
 *    instd_filter,instance
 */
DCTable::DCTable(DB_HANDLE hdb, DB_RESULT hResult, int iRow, Template *pNode) : DCObject()
{
   m_id = DBGetFieldULong(hResult, iRow, 0);
   m_dwTemplateId = DBGetFieldULong(hResult, iRow, 1);
   m_dwTemplateItemId = DBGetFieldULong(hResult, iRow, 2);
	DBGetField(hResult, iRow, 3, m_name, MAX_ITEM_NAME);
   DBGetField(hResult, iRow, 4, m_description, MAX_DB_STRING);
   m_flags = (WORD)DBGetFieldLong(hResult, iRow, 5);
   m_source = (BYTE)DBGetFieldLong(hResult, iRow, 6);
	m_snmpPort = (WORD)DBGetFieldLong(hResult, iRow, 7);
   m_iPollingInterval = DBGetFieldLong(hResult, iRow, 8);
   m_iRetentionTime = DBGetFieldLong(hResult, iRow, 9);
   m_status = (BYTE)DBGetFieldLong(hResult, iRow, 10);
	DBGetField(hResult, iRow, 11, m_systemTag, MAX_DB_STRING);
	m_dwResourceId = DBGetFieldULong(hResult, iRow, 12);
	m_sourceNode = DBGetFieldULong(hResult, iRow, 13);
	m_pszPerfTabSettings = DBGetField(hResult, iRow, 14, NULL, 0);
   TCHAR *pszTmp = DBGetField(hResult, iRow, 15, NULL, 0);
   m_comments = DBGetField(hResult, iRow, 16, NULL, 0);
   m_guid = DBGetFieldGUID(hResult, iRow, 17);
   setTransformationScript(pszTmp);
   free(pszTmp);
   m_instanceDiscoveryMethod = (WORD)DBGetFieldLong(hResult, iRow, 18);
   m_instanceDiscoveryData = DBGetField(hResult, iRow, 19, NULL, 0);
   m_instanceFilterSource = NULL;
   m_instanceFilter = NULL;
   pszTmp = DBGetField(hResult, iRow, 20, NULL, 0);
   setInstanceFilter(pszTmp);
   free(pszTmp);
   DBGetField(hResult, iRow, 21, m_instance, MAX_DB_STRING);
   m_instanceRetentionTime = DBGetFieldLong(hResult, iRow, 22);

   m_owner = pNode;
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
}

/**
 * Create DCTable from import file
 */
DCTable::DCTable(ConfigEntry *config, Template *owner) : DCObject(config, owner)
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
   TCHAR szQuery[256];
	bool success;

   lock();
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   _sntprintf(szQuery, 256, _T("DELETE FROM tdata_%d WHERE item_id=%d"), m_owner->getId(), (int)m_id);
	success = DBQuery(hdb, szQuery) ? true : false;
   DBConnectionPoolReleaseConnection(hdb);
   unlock();
	return success;
}

/**
 * Process new collected value. Should return true on success.
 * If returns false, current poll result will be converted into data collection error.
 *
 * @return true on success
 */
bool DCTable::processNewValue(time_t timestamp, const void *value, bool *updateStatus)
{
   *updateStatus = false;
   lock();

   // Normally m_owner shouldn't be NULL for polled items, but who knows...
   if (m_owner == NULL)
   {
      unlock();
      ((Table *)value)->decRefCount();
      return false;
   }

   // Transform input value
   // Cluster can have only aggregated data, and transformation
   // should not be used on aggregation
   if ((m_owner->getObjectClass() != OBJECT_CLUSTER) || (m_flags & DCF_TRANSFORM_AGGREGATED))
   {
      if (!transform((Table *)value))
      {
         unlock();
         ((Table *)value)->decRefCount();
         return false;
      }
   }

   m_dwErrorCount = 0;
   if (m_lastValue != NULL)
      m_lastValue->decRefCount();
	m_lastValue = (Table *)value;
	m_lastValue->setTitle(m_description);
   m_lastValue->setSource(m_source);

	// Copy required fields into local variables
	UINT32 tableId = m_id;
	UINT32 nodeId = m_owner->getId();
   bool save = (m_flags & DCF_NO_STORAGE) == 0;

   ((Table *)value)->incRefCount();

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
	   Table *data = (Table *)value;

	   TCHAR query[256];
	   _sntprintf(query, 256, _T("INSERT INTO tdata_%d (item_id,tdata_timestamp,tdata_value) VALUES (?,?,?)"), (int)nodeId);
	   DB_STATEMENT hStmt = DBPrepare(hdb, query);
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
      checkThresholds((Table *)value);

   if (g_flags & AF_PERFDATA_STORAGE_DRIVER_LOADED)
      PerfDataStorageRequest(this, timestamp, (Table *)value);

   ((Table *)value)->decRefCount();
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
   NXSL_VM *vm = new NXSL_VM(new NXSL_ServerEnv());
   if (vm->load(m_transformationScript))
   {
      NXSL_Value *nxslValue = new NXSL_Value(new NXSL_Object(&g_nxslStaticTableClass, value));
      vm->setGlobalVariable(_T("$object"), m_owner->createNXSLObject());
      if (m_owner->getObjectClass() == OBJECT_NODE)
      {
         vm->setGlobalVariable(_T("$node"), m_owner->createNXSLObject());
      }
      vm->setGlobalVariable(_T("$dci"), createNXSLObject());
      vm->setGlobalVariable(_T("$isCluster"), new NXSL_Value((m_owner->getObjectClass() == OBJECT_CLUSTER) ? 1 : 0));

      // remove lock from DCI for script execution to avoid deadlocks
      unlock();
      success = vm->run(1, &nxslValue);
      lock();
      if (!success)
      {
         if (vm->getErrorCode() == NXSL_ERR_EXECUTION_ABORTED)
         {
            DbgPrintf(6, _T("Transformation script for DCI \"%s\" [%d] on node %s [%d] aborted"),
                            m_description, m_id, getOwnerName(), getOwnerId());
         }
         else
         {
            TCHAR buffer[1024];
            _sntprintf(buffer, 1024, _T("DCI::%s::%d::TransformationScript"), getOwnerName(), m_id);
            PostDciEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, m_id, "ssd", buffer, vm->getErrorText(), m_id);
            nxlog_write(MSG_TRANSFORMATION_SCRIPT_EXECUTION_ERROR, NXLOG_WARNING, "dsdss",
                        getOwnerId(), getOwnerName(), m_id, m_name, vm->getErrorText());
         }
      }
   }
   else
   {
      TCHAR buffer[1024];
      _sntprintf(buffer, 1024, _T("DCI::%s::%d::TransformationScript"), getOwnerName(), m_id);
      PostDciEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, m_id, "ssd", buffer, vm->getErrorText(), m_id);
      nxlog_write(MSG_TRANSFORMATION_SCRIPT_EXECUTION_ERROR, NXLOG_WARNING, "dsdss",
                  getOwnerId(), getOwnerName(), m_id, m_name, vm->getErrorText());
   }
   delete vm;
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
            case ACTIVATED:
               PostDciEventWithNames(t->getActivationEvent(), m_owner->getId(), m_id, "ssids", paramNames, m_name, m_description, m_id, row, instance);
               if (!(m_flags & DCF_ALL_THRESHOLDS))
                  i = m_thresholds->size();  // Stop processing (for current row)
               break;
            case DEACTIVATED:
               PostDciEventWithNames(t->getDeactivationEvent(), m_owner->getId(), m_id, "ssids", paramNames, m_name, m_description, m_id, row, instance);
               break;
            case ALREADY_ACTIVE:
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
 * Save to database
 */
bool DCTable::saveToDatabase(DB_HANDLE hdb)
{
	DB_STATEMENT hStmt;
	if (IsDatabaseRecordExist(hdb, _T("dc_tables"), _T("item_id"), m_id))
	{
		hStmt = DBPrepare(hdb, _T("UPDATE dc_tables SET node_id=?,template_id=?,template_item_id=?,name=?,")
		                       _T("description=?,flags=?,source=?,snmp_port=?,polling_interval=?,")
                             _T("retention_time=?,status=?,system_tag=?,resource_id=?,proxy_node=?,")
									  _T("perftab_settings=?,transformation_script=?,comments=?,guid=?,")
									  _T("instd_method=?,instd_data=?,instd_filter=?,instance=?,instance_retention_time=? WHERE item_id=?"));
	}
	else
	{
		hStmt = DBPrepare(hdb, _T("INSERT INTO dc_tables (node_id,template_id,template_item_id,name,")
		                       _T("description,flags,source,snmp_port,polling_interval,")
		                       _T("retention_time,status,system_tag,resource_id,proxy_node,perftab_settings,")
									  _T("transformation_script,comments,guid,")
									  _T("instd_method,instd_data,instd_filter,instance,instance_retention_time,item_id) ")
									  _T("VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)"));
	}
	if (hStmt == NULL)
		return FALSE;

   lock();

	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, (m_owner == NULL) ? (UINT32)0 : m_owner->getId());
	DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_dwTemplateId);
	DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_dwTemplateItemId);
	DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, m_name, DB_BIND_STATIC);
	DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, m_description, DB_BIND_STATIC);
	DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, (UINT32)m_flags);
	DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, (INT32)m_source);
	DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, (UINT32)m_snmpPort);
	DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, (INT32)m_iPollingInterval);
	DBBind(hStmt, 10, DB_SQLTYPE_INTEGER, (INT32)m_iRetentionTime);
	DBBind(hStmt, 11, DB_SQLTYPE_INTEGER, (INT32)m_status);
	DBBind(hStmt, 12, DB_SQLTYPE_VARCHAR, m_systemTag, DB_BIND_STATIC);
	DBBind(hStmt, 13, DB_SQLTYPE_INTEGER, m_dwResourceId);
	DBBind(hStmt, 14, DB_SQLTYPE_INTEGER, m_sourceNode);
	DBBind(hStmt, 15, DB_SQLTYPE_TEXT, m_pszPerfTabSettings, DB_BIND_STATIC);
   DBBind(hStmt, 16, DB_SQLTYPE_TEXT, m_transformationScriptSource, DB_BIND_STATIC);
   DBBind(hStmt, 17, DB_SQLTYPE_TEXT, m_comments, DB_BIND_STATIC);
   DBBind(hStmt, 18, DB_SQLTYPE_VARCHAR, m_guid);
   DBBind(hStmt, 19, DB_SQLTYPE_INTEGER, (INT32)m_instanceDiscoveryMethod);
   DBBind(hStmt, 20, DB_SQLTYPE_VARCHAR, m_instanceDiscoveryData, DB_BIND_STATIC);
   DBBind(hStmt, 21, DB_SQLTYPE_TEXT, m_instanceFilterSource, DB_BIND_STATIC);
   DBBind(hStmt, 22, DB_SQLTYPE_VARCHAR, m_instance, DB_BIND_STATIC);
   DBBind(hStmt, 23, DB_SQLTYPE_INTEGER, m_instanceRetentionTime);
   DBBind(hStmt, 24, DB_SQLTYPE_INTEGER, m_id);

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
					SNMP_ObjectId *oid = column->getSnmpOid();
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
		SNMP_ObjectId *oid = column->getSnmpOid();
		if (oid != NULL)
			pMsg->setFieldFromInt32Array(varId++, (UINT32)oid->length(), oid->value());
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
   pMsg->setField(dwId++, m_description);
   pMsg->setField(dwId++, (WORD)m_source);
   pMsg->setField(dwId++, (WORD)DCI_DT_NULL);  // compatibility: data type
   pMsg->setField(dwId++, _T(""));             // compatibility: value
   pMsg->setField(dwId++, (UINT32)m_tLastPoll);
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

   m_thresholds->clear();
	for(int i = 0; i < table->m_thresholds->size(); i++)
		m_thresholds->add(new DCTableThreshold(table->m_thresholds->get(i)));

   unlock();
}

/**
 * Create management pack record
 */
void DCTable::createExportRecord(String &str)
{
   lock();

   str.appendFormattedString(_T("\t\t\t\t<dctable id=\"%d\">\n")
                          _T("\t\t\t\t\t<guid>%s</guid>\n")
                          _T("\t\t\t\t\t<name>%s</name>\n")
                          _T("\t\t\t\t\t<description>%s</description>\n")
                          _T("\t\t\t\t\t<origin>%d</origin>\n")
                          _T("\t\t\t\t\t<interval>%d</interval>\n")
                          _T("\t\t\t\t\t<retention>%d</retention>\n")
                          _T("\t\t\t\t\t<systemTag>%s</systemTag>\n")
                          _T("\t\t\t\t\t<flags>%d</flags>\n")
                          _T("\t\t\t\t\t<snmpPort>%d</snmpPort>\n")
                          _T("\t\t\t\t\t<instanceDiscoveryMethod>%d</instanceDiscoveryMethod>\n")
                          _T("\t\t\t\t\t<instance>%s</instance>\n"),
                          _T("\t\t\t\t\t<instanceRetentionTime>%d</instanceRetentionTime>\n"),
								  (int)m_id, (const TCHAR *)m_guid.toString(),
								  (const TCHAR *)EscapeStringForXML2(m_name),
                          (const TCHAR *)EscapeStringForXML2(m_description),
                          (int)m_source, m_iPollingInterval, m_iRetentionTime,
                          (const TCHAR *)EscapeStringForXML2(m_systemTag),
								  (int)m_flags, (int)m_snmpPort, (int)m_instanceDiscoveryMethod,
								  (const TCHAR *)EscapeStringForXML2(m_instance), m_instanceRetentionTime);

	if (m_transformationScriptSource != NULL)
	{
		str.append(_T("\t\t\t\t\t<transformation>"));
		str.appendPreallocated(EscapeStringForXML(m_transformationScriptSource, -1));
		str.append(_T("</transformation>\n"));
	}

   if ((m_schedules != NULL) && (m_schedules->size() > 0))
   {
      str.append(_T("\t\t\t\t\t<schedules>\n"));
      for(int i = 0; i < m_schedules->size(); i++)
      {
         str.append(_T("\t\t\t\t\t\t<schedule>"));
         str.append(EscapeStringForXML2(m_schedules->get(i)));
         str.append(_T("</schedule>\n"));
      }
      str.append(_T("\t\t\t\t\t</schedules>\n"));
   }

	if (m_columns != NULL)
	{
	   str += _T("\t\t\t\t\t<columns>\n");
		for(int i = 0; i < m_columns->size(); i++)
		{
			m_columns->get(i)->createNXMPRecord(str, i + 1);
		}
	   str += _T("\t\t\t\t\t</columns>\n");
	}

	if (m_thresholds != NULL)
	{
	   str += _T("\t\t\t\t\t<thresholds>\n");
		for(int i = 0; i < m_thresholds->size(); i++)
		{
			m_thresholds->get(i)->createNXMPRecord(str, i + 1);
		}
	   str += _T("\t\t\t\t\t</thresholds>\n");
	}

	if (m_pszPerfTabSettings != NULL)
	{
		str.append(_T("\t\t\t\t\t<perfTabSettings>"));
		str.appendPreallocated(EscapeStringForXML(m_pszPerfTabSettings, -1));
		str.append(_T("</perfTabSettings>\n"));
	}

	if (m_instanceDiscoveryData != NULL)
   {
      str += _T("\t\t\t\t\t<instanceDiscoveryData>");
      str.appendPreallocated(EscapeStringForXML(m_instanceDiscoveryData, -1));
      str += _T("</instanceDiscoveryData>\n");
   }

   if (m_instanceFilterSource != NULL)
   {
      str += _T("\t\t\t\t\t<instanceFilter>");
      str.appendPreallocated(EscapeStringForXML(m_instanceFilterSource, -1));
      str += _T("</instanceFilter>\n");
   }

   unlock();
   str.append(_T("\t\t\t\t</dctable>\n"));
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
DCObject *DCTable::clone()
{
   return new DCTable(this);
}

/**
 * Serialize object to JSON
 */
json_t *DCTable::toJson()
{
   json_t *root = DCObject::toJson();
   json_object_set_new(root, "columns", json_object_array(m_columns));
   json_object_set_new(root, "thresholds", json_object_array(m_thresholds));
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
