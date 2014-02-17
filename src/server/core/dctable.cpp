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
 *    transformation_script
 */
DCTable::DCTable(DB_RESULT hResult, int iRow, Template *pNode) : DCObject()
{
   m_dwId = DBGetFieldULong(hResult, iRow, 0);
   m_dwTemplateId = DBGetFieldULong(hResult, iRow, 1);
   m_dwTemplateItemId = DBGetFieldULong(hResult, iRow, 2);
	DBGetField(hResult, iRow, 3, m_szName, MAX_ITEM_NAME);
   DBGetField(hResult, iRow, 4, m_szDescription, MAX_DB_STRING);
   m_flags = (WORD)DBGetFieldLong(hResult, iRow, 5);
   m_source = (BYTE)DBGetFieldLong(hResult, iRow, 6);
	m_snmpPort = (WORD)DBGetFieldLong(hResult, iRow, 7);
   m_iPollingInterval = DBGetFieldLong(hResult, iRow, 8);
   m_iRetentionTime = DBGetFieldLong(hResult, iRow, 9);
   m_status = (BYTE)DBGetFieldLong(hResult, iRow, 10);
	DBGetField(hResult, iRow, 11, m_systemTag, MAX_DB_STRING);
	m_dwResourceId = DBGetFieldULong(hResult, iRow, 12);
	m_dwProxyNode = DBGetFieldULong(hResult, iRow, 13);
	m_pszPerfTabSettings = DBGetField(hResult, iRow, 14, NULL, 0);
   TCHAR *pszTmp = DBGetField(hResult, iRow, 15, NULL, 0);
   setTransformationScript(pszTmp);
   free(pszTmp);

   m_pNode = pNode;
	m_columns = new ObjectArray<DCTableColumn>(8, 8, true);
	m_lastValue = NULL;

	DB_STATEMENT hStmt = DBPrepare(g_hCoreDB, _T("SELECT column_name,flags,snmp_oid,display_name FROM dc_table_columns WHERE table_id=? ORDER BY sequence_number"));
	if (hStmt != NULL)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_dwId);
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

	loadCustomSchedules();

   m_thresholds = new ObjectArray<DCTableThreshold>(0, 4, true);
   loadThresholds();
}

/**
 * Create DCTable from import file
 */
DCTable::DCTable(ConfigEntry *config, Template *owner) : DCObject(config, owner)
{
	ConfigEntry *columnsRoot = config->findEntry(_T("columns"));
	if (columnsRoot != NULL)
	{
		ConfigEntryList *columns = columnsRoot->getSubEntries(_T("column#*"));
		m_columns = new ObjectArray<DCTableColumn>(columns->getSize(), 8, true);
		for(int i = 0; i < columns->getSize(); i++)
		{
			m_columns->add(new DCTableColumn(columns->getEntry(i)));
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
		ConfigEntryList *thresholds = thresholdsRoot->getSubEntries(_T("threshold#*"));
		m_thresholds = new ObjectArray<DCTableThreshold>(thresholds->getSize(), 8, true);
		for(int i = 0; i < thresholds->getSize(); i++)
		{
			m_thresholds->add(new DCTableThreshold(thresholds->getEntry(i)));
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
 * Clean expired data
 */
void DCTable::deleteExpiredData()
{
   TCHAR query[256];
   time_t now;

   now = time(NULL);

   lock();
   _sntprintf(query, 256, _T("DELETE FROM tdata_%d WHERE (item_id=%d) AND (tdata_timestamp<%ld)"),
              (int)m_pNode->Id(), (int)m_dwId, (long)(now - (time_t)m_iRetentionTime * 86400));
   unlock();

   QueueSQLRequest(query);
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
   _sntprintf(szQuery, 256, _T("DELETE FROM tdata_%d WHERE item_id=%d"), m_pNode->Id(), (int)m_dwId);
	success = DBQuery(hdb, szQuery) ? true : false;
   DBConnectionPoolReleaseConnection(hdb);
   unlock();
	return success;
}

/**
 * Process new collected value
 */
void DCTable::processNewValue(time_t nTimeStamp, void *value)
{
   lock();

   // Normally m_pNode shouldn't be NULL for polled items, but who knows...
   if (m_pNode == NULL)
   {
      unlock();
      return;
   }

   // Transform input value
   // Cluster can have only aggregated data, and transformation
   // should not be used on aggregation
   if ((m_pNode->Type() != OBJECT_CLUSTER) || (m_flags & DCF_TRANSFORM_AGGREGATED))
      transform((Table *)value);

   m_dwErrorCount = 0;
   if (m_lastValue != NULL)
      m_lastValue->decRefCount();
	m_lastValue = (Table *)value;
	m_lastValue->setTitle(m_szDescription);
   m_lastValue->setSource(m_source);

	// Copy required fields into local variables
	UINT32 tableId = m_dwId;
	UINT32 nodeId = m_pNode->Id();
   bool save = (m_flags & DCF_NO_STORAGE) == 0;

   unlock();

	// Save data to database
	// Object is unlocked, so only local variables can be used
   if (save)
   {
	   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
      if (!DBBegin(hdb))
      {
   	   DBConnectionPoolReleaseConnection(hdb);
         return;
      }

      INT64 recordId = ((INT64)time(NULL) << 30) | (((INT64)tableId & 0xFFFF) << 14);
      BOOL success = FALSE;
	   Table *data = (Table *)value;

	   TCHAR query[256];
	   _sntprintf(query, 256, _T("INSERT INTO tdata_%d (item_id,tdata_timestamp,record_id) VALUES (?,?,?)"), (int)nodeId);
	   DB_STATEMENT hStmt = DBPrepare(hdb, query);
	   if (hStmt != NULL)
	   {
		   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, tableId);
		   DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, (INT32)nTimeStamp);
		   DBBind(hStmt, 3, DB_SQLTYPE_BIGINT, recordId);
	      success = DBExecute(hStmt);
		   DBFreeStatement(hStmt);
	   }

      if (success)
      {
	      _sntprintf(query, 256, _T("INSERT INTO tdata_records_%d (record_id,row_id,instance) VALUES (?,?,?)"), (int)nodeId);
	      DB_STATEMENT hStmt = DBPrepare(hdb, query);
	      if (hStmt != NULL)
	      {
            DBBind(hStmt, 1, DB_SQLTYPE_BIGINT, recordId);
            for(int row = 0; row < data->getNumRows(); row++)
            {
               TCHAR instance[MAX_RESULT_LENGTH];
               data->buildInstanceString(row, instance, MAX_RESULT_LENGTH);
               DBBind(hStmt, 2, DB_SQLTYPE_BIGINT, recordId | (INT64)row);
		         DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, instance, DB_BIND_STATIC);
	            success = DBExecute(hStmt);
               if (!success)
                  break;
            }
	         DBFreeStatement(hStmt);
	      }
      }

      if (success)
      {
	      _sntprintf(query, 256, _T("INSERT INTO tdata_rows_%d (row_id,column_id,value) VALUES (?,?,?)"), (int)nodeId);
	      DB_STATEMENT hStmt = DBPrepare(hdb, query);
	      if (hStmt != NULL)
	      {
		      for(int col = 0; col < data->getNumColumns(); col++)
		      {
			      INT32 colId = columnIdFromName(data->getColumnName(col));
			      if (colId == 0)
				      continue;	// cannot get column ID

			      for(int row = 0; row < data->getNumRows(); row++)
			      {
                  DBBind(hStmt, 1, DB_SQLTYPE_BIGINT, recordId | (INT64)row);
				      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, colId);
                  const TCHAR *s = data->getAsString(row, col);
                  if ((s == NULL) || (_tcslen(s) < MAX_DB_STRING))
                  {
				         DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, s, DB_BIND_STATIC);
                  }
                  else
                  {
                     TCHAR *sp = (TCHAR *)nx_memdup(s, MAX_DB_STRING * sizeof(TCHAR));
                     sp[MAX_DB_STRING - 1] = 0;
				         DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, sp, DB_BIND_DYNAMIC);
                  }
				      success = DBExecute(hStmt);
                  if (!success)
                     break;
			      }
		      }
	         DBFreeStatement(hStmt);
         }
      }

      if (success)
         DBCommit(hdb);
      else
         DBRollback(hdb);

	   DBConnectionPoolReleaseConnection(hdb);
   }
   checkThresholds((Table *)value);
}

/**
 * Transform received value
 */
void DCTable::transform(Table *value)
{
   if (m_transformationScript == NULL)
      return;

   NXSL_Value *nxslValue;
   NXSL_ServerEnv *pEnv;

   nxslValue = new NXSL_Value(new NXSL_Object(&g_nxslStaticTableClass, value));
   pEnv = new NXSL_ServerEnv;
   m_transformationScript->setGlobalVariable(_T("$object"), new NXSL_Value(new NXSL_Object(&g_nxslNetObjClass, m_pNode)));
   if (m_pNode->Type() == OBJECT_NODE)
   {
      m_transformationScript->setGlobalVariable(_T("$node"), new NXSL_Value(new NXSL_Object(&g_nxslNodeClass, m_pNode)));
   }
   m_transformationScript->setGlobalVariable(_T("$dci"), new NXSL_Value(new NXSL_Object(&g_nxslDciClass, this)));
   m_transformationScript->setGlobalVariable(_T("$isCluster"), new NXSL_Value((m_pNode->Type() == OBJECT_CLUSTER) ? 1 : 0));

   if (m_transformationScript->run(pEnv, 1, &nxslValue) != 0)
   {
      TCHAR szBuffer[1024];

		_sntprintf(szBuffer, 1024, _T("DCI::%s::%d::TransformationScript"),
                 (m_pNode != NULL) ? m_pNode->Name() : _T("(null)"), m_dwId);
      PostEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", szBuffer, m_transformationScript->getErrorText(), m_dwId);
   }
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
               PostEventWithNames(t->getActivationEvent(), m_pNode->Id(), "ssids", paramNames, m_szName, m_szDescription, m_dwId, row, instance);
               if (!(m_flags & DCF_ALL_THRESHOLDS))
                  i = m_thresholds->size();  // Stop processing (for current row)
               break;
            case DEACTIVATED:
               PostEventWithNames(t->getDeactivationEvent(), m_pNode->Id(), "ssids", paramNames, m_szName, m_szDescription, m_dwId, row, instance);
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
void DCTable::processNewError()
{
	m_dwErrorCount++;
}

/**
 * Save to database
 */
BOOL DCTable::saveToDB(DB_HANDLE hdb)
{
	DB_STATEMENT hStmt;
	if (IsDatabaseRecordExist(hdb, _T("dc_tables"), _T("item_id"), m_dwId))
	{
		hStmt = DBPrepare(hdb, _T("UPDATE dc_tables SET node_id=?,template_id=?,template_item_id=?,name=?,")
		                       _T("description=?,flags=?,source=?,snmp_port=?,polling_interval=?,")
                             _T("retention_time=?,status=?,system_tag=?,resource_id=?,proxy_node=?,")
									  _T("perftab_settings=?,transformation_script=? WHERE item_id=?"));
	}
	else
	{
		hStmt = DBPrepare(hdb, _T("INSERT INTO dc_tables (node_id,template_id,template_item_id,name,")
		                       _T("description,flags,source,snmp_port,polling_interval,")
		                       _T("retention_time,status,system_tag,resource_id,proxy_node,perftab_settings,")
									  _T("transformation_script,item_id) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)"));
	}
	if (hStmt == NULL)
		return FALSE;

   lock();

	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, (m_pNode == NULL) ? (UINT32)0 : m_pNode->Id());
	DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_dwTemplateId);
	DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_dwTemplateItemId);
	DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, m_szName, DB_BIND_STATIC);
	DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, m_szDescription, DB_BIND_STATIC);
	DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, (UINT32)m_flags);
	DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, (INT32)m_source);
	DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, (UINT32)m_snmpPort);
	DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, (INT32)m_iPollingInterval);
	DBBind(hStmt, 10, DB_SQLTYPE_INTEGER, (INT32)m_iRetentionTime);
	DBBind(hStmt, 11, DB_SQLTYPE_INTEGER, (INT32)m_status);
	DBBind(hStmt, 12, DB_SQLTYPE_VARCHAR, m_systemTag, DB_BIND_STATIC);
	DBBind(hStmt, 13, DB_SQLTYPE_INTEGER, m_dwResourceId);
	DBBind(hStmt, 14, DB_SQLTYPE_INTEGER, m_dwProxyNode);
	DBBind(hStmt, 15, DB_SQLTYPE_TEXT, m_pszPerfTabSettings, DB_BIND_STATIC);
   DBBind(hStmt, 16, DB_SQLTYPE_TEXT, m_transformationScriptSource, DB_BIND_STATIC);
	DBBind(hStmt, 17, DB_SQLTYPE_INTEGER, m_dwId);

	BOOL result = DBExecute(hStmt);
	DBFreeStatement(hStmt);

	if (result)
	{
		// Save column configuration
		hStmt = DBPrepare(hdb, _T("DELETE FROM dc_table_columns WHERE table_id=?"));
		if (hStmt != NULL)
		{
			DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_dwId);
			result = DBExecute(hStmt);
			DBFreeStatement(hStmt);
		}
		else
		{
			result = FALSE;
		}

		if (result && (m_columns->size() > 0))
		{
			hStmt = DBPrepare(hdb, _T("INSERT INTO dc_table_columns (table_id,sequence_number,column_name,snmp_oid,flags,display_name) VALUES (?,?,?,?,?,?)"));
			if (hStmt != NULL)
			{
				DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_dwId);
				for(int i = 0; i < m_columns->size(); i++)
				{
					DCTableColumn *column = m_columns->get(i);
               DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, (INT32)(i + 1));
					DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, column->getName(), DB_BIND_STATIC);
					SNMP_ObjectId *oid = column->getSnmpOid();
					DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, (oid != NULL) ? oid->getValueAsText() : NULL, DB_BIND_STATIC);
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
				result = FALSE;
			}
		}
	}

   saveThresholds(hdb);

   unlock();
	return result ? DCObject::saveToDB(hdb) : FALSE;
}

/**
 * Load thresholds from database
 */
bool DCTable::loadThresholds()
{
   DB_STATEMENT hStmt = DBPrepare(g_hCoreDB, _T("SELECT id,activation_event,deactivation_event FROM dct_thresholds WHERE table_id=? ORDER BY sequence_number"));
   if (hStmt == NULL)
      return false;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_dwId);
   DB_RESULT hResult = DBSelectPrepared(hStmt);
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         DCTableThreshold *t = new DCTableThreshold(hResult, i);
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

   hStmt = DBPrepare(hdb, _T("DELETE FROM dct_thresholds WHERE table_id=?"));
   if (hStmt == NULL)
      return false;
   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_dwId);
   DBExecute(hStmt);
   DBFreeStatement(hStmt);

   for(int i = 0; i < m_thresholds->size(); i++)
      m_thresholds->get(i)->saveToDatabase(hdb, m_dwId, i);
   return true;
}

/**
 * Delete table object and collected data from database
 */
void DCTable::deleteFromDB()
{
   TCHAR szQuery[256];

	DCObject::deleteFromDB();

   deleteAllData();

   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM dc_tables WHERE item_id=%d"), (int)m_dwId);
   QueueSQLRequest(szQuery);
   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM dc_table_columns WHERE table_id=%d"), (int)m_dwId);
   QueueSQLRequest(szQuery);

   for(int i = 0; i < m_thresholds->size(); i++)
   {
      _sntprintf(szQuery, 256, _T("DELETE FROM dct_threshold_conditions WHERE threshold_id=%d"), (int)m_thresholds->get(i)->getId());
      QueueSQLRequest(szQuery);
   }

   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM dct_thresholds WHERE table_id=%d"), (int)m_dwId);
   QueueSQLRequest(szQuery);
}

/**
 * Create NXCP message with item data
 */
void DCTable::createMessage(CSCPMessage *pMsg)
{
	DCObject::createMessage(pMsg);

   lock();
	pMsg->SetVariable(VID_NUM_COLUMNS, (UINT32)m_columns->size());
	UINT32 varId = VID_DCI_COLUMN_BASE;
	for(int i = 0; i < m_columns->size(); i++)
	{
		DCTableColumn *column = m_columns->get(i);
		pMsg->SetVariable(varId++, column->getName());
		pMsg->SetVariable(varId++, column->getFlags());
		SNMP_ObjectId *oid = column->getSnmpOid();
		if (oid != NULL)
			pMsg->SetVariableToInt32Array(varId++, oid->getLength(), oid->getValue());
		else
			varId++;
		pMsg->SetVariable(varId++, column->getDisplayName());
		varId += 6;
	}

	pMsg->SetVariable(VID_NUM_THRESHOLDS, (UINT32)m_thresholds->size());
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
void DCTable::updateFromMessage(CSCPMessage *pMsg)
{
	DCObject::updateFromMessage(pMsg);

   lock();

	m_columns->clear();
	int count = (int)pMsg->GetVariableLong(VID_NUM_COLUMNS);
	UINT32 varId = VID_DCI_COLUMN_BASE;
	for(int i = 0; i < count; i++)
	{
		m_columns->add(new DCTableColumn(pMsg, varId));
		varId += 10;
	}

	count = (int)pMsg->GetVariableLong(VID_NUM_THRESHOLDS);
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
void DCTable::fillLastValueMessage(CSCPMessage *msg)
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
void DCTable::fillLastValueSummaryMessage(CSCPMessage *pMsg, UINT32 dwId)
{
	lock();
   pMsg->SetVariable(dwId++, m_dwId);
   pMsg->SetVariable(dwId++, m_szName);
   pMsg->SetVariable(dwId++, m_szDescription);
   pMsg->SetVariable(dwId++, (WORD)m_source);
   pMsg->SetVariable(dwId++, (WORD)DCI_DT_NULL);  // compatibility: data type
   pMsg->SetVariable(dwId++, _T(""));             // compatibility: value
   pMsg->SetVariable(dwId++, (UINT32)m_tLastPoll);
   pMsg->SetVariable(dwId++, (WORD)(matchClusterResource() ? m_status : ITEM_STATUS_DISABLED)); // show resource-bound DCIs as inactive if cluster resource is not on this node
	pMsg->SetVariable(dwId++, (WORD)getType());
	pMsg->SetVariable(dwId++, m_dwErrorCount);
	pMsg->SetVariable(dwId++, m_dwTemplateItemId);
   pMsg->SetVariable(dwId++, (WORD)0);            // compatibility: number of thresholds

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
		DbgPrintf(2, _T("INTERNAL ERROR: DCTable::updateFromTemplate(%d, %d): source type is %d"), (int)m_dwId, (int)src->getId(), src->getType());
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
void DCTable::createNXMPRecord(String &str)
{
	UINT32 i;

   lock();
   
   str.addFormattedString(_T("\t\t\t\t<dctable id=\"%d\">\n")
                          _T("\t\t\t\t\t<name>%s</name>\n")
                          _T("\t\t\t\t\t<description>%s</description>\n")
                          _T("\t\t\t\t\t<origin>%d</origin>\n")
                          _T("\t\t\t\t\t<interval>%d</interval>\n")
                          _T("\t\t\t\t\t<retention>%d</retention>\n")
                          _T("\t\t\t\t\t<systemTag>%s</systemTag>\n")
                          _T("\t\t\t\t\t<advancedSchedule>%d</advancedSchedule>\n")
                          _T("\t\t\t\t\t<rawValueInOctetString>%d</rawValueInOctetString>\n")
                          _T("\t\t\t\t\t<snmpPort>%d</snmpPort>\n"),
								  (int)m_dwId, (const TCHAR *)EscapeStringForXML2(m_szName),
                          (const TCHAR *)EscapeStringForXML2(m_szDescription),
                          (int)m_source, m_iPollingInterval, m_iRetentionTime,
                          (const TCHAR *)EscapeStringForXML2(m_systemTag),
								  (m_flags & DCF_ADVANCED_SCHEDULE) ? 1 : 0,
								  (m_flags & DCF_RAW_VALUE_OCTET_STRING) ? 1 : 0, 
								  (int)m_snmpPort);

	if (m_transformationScriptSource != NULL)
	{
		str += _T("\t\t\t\t\t<transformation>");
		str.addDynamicString(EscapeStringForXML(m_transformationScriptSource, -1));
		str += _T("</transformation>\n");
	}

	if (m_dwNumSchedules > 0)
   {
      str += _T("\t\t\t\t\t<schedules>\n");
      for(i = 0; i < m_dwNumSchedules; i++)
         str.addFormattedString(_T("\t\t\t\t\t\t<schedule>%s</schedule>\n"), (const TCHAR *)EscapeStringForXML2(m_ppScheduleList[i]));
      str += _T("\t\t\t\t\t</schedules>\n");
   }

	if (m_columns != NULL)
	{
	   str += _T("\t\t\t\t\t<columns>\n");
		for(i = 0; i < (UINT32)m_columns->size(); i++)
		{
			m_columns->get(i)->createNXMPRecord(str, i + 1);
		}
	   str += _T("\t\t\t\t\t</columns>\n");
	}

	if (m_thresholds != NULL)
	{
	   str += _T("\t\t\t\t\t<thresholds>\n");
		for(i = 0; i < (UINT32)m_thresholds->size(); i++)
		{
			m_thresholds->get(i)->createNXMPRecord(str, i + 1);
		}
	   str += _T("\t\t\t\t\t</thresholds>\n");
	}

	if (m_pszPerfTabSettings != NULL)
	{
		str += _T("\t\t\t\t\t<perfTabSettings>");
		str.addDynamicString(EscapeStringForXML(m_pszPerfTabSettings, -1));
		str += _T("</perfTabSettings>\n");
	}

   unlock();
   str += _T("\t\t\t\t</dctable>\n");
}
