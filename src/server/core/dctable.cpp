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
LONG DCTable::columnIdFromName(const TCHAR *name)
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
	m_instanceColumn[0] = 0;
	m_columns = new ObjectArray<DCTableColumn>(8, 8, true);
	m_lastValue = NULL;
}

/**
 * Copy constructor
 */
DCTable::DCTable(const DCTable *src) : DCObject(src)
{
	nx_strncpy(m_instanceColumn, src->m_instanceColumn, MAX_COLUMN_NAME);
	m_columns = new ObjectArray<DCTableColumn>(src->m_columns->size(), 8, true);
	for(int i = 0; i < src->m_columns->size(); i++)
		m_columns->add(new DCTableColumn(src->m_columns->get(i)));
	m_lastValue = NULL;
}

/**
 * Constructor for creating new DCTable from scratch
 */
DCTable::DCTable(DWORD id, const TCHAR *name, int source, int pollingInterval, int retentionTime,
	              Template *node, const TCHAR *instanceColumn, const TCHAR *description, const TCHAR *systemTag)
        : DCObject(id, name, source, pollingInterval, retentionTime, node, description, systemTag)
{
	nx_strncpy(m_instanceColumn, CHECK_NULL_EX(instanceColumn), MAX_COLUMN_NAME);
	m_columns = new ObjectArray<DCTableColumn>(8, 8, true);
	m_lastValue = NULL;
}

/**
 * Constructor for creating DCTable from database
 * Assumes that fields in SELECT query are in following order:
 *    item_id,template_id,template_item_id,name,instance_column,
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
	DBGetField(hResult, iRow, 4, m_instanceColumn, MAX_COLUMN_NAME);
   DBGetField(hResult, iRow, 5, m_szDescription, MAX_DB_STRING);
   m_flags = (WORD)DBGetFieldLong(hResult, iRow, 6);
   m_source = (BYTE)DBGetFieldLong(hResult, iRow, 7);
	m_snmpPort = (WORD)DBGetFieldLong(hResult, iRow, 8);
   m_iPollingInterval = DBGetFieldLong(hResult, iRow, 9);
   m_iRetentionTime = DBGetFieldLong(hResult, iRow, 10);
   m_status = (BYTE)DBGetFieldLong(hResult, iRow, 11);
	DBGetField(hResult, iRow, 12, m_systemTag, MAX_DB_STRING);
	m_dwResourceId = DBGetFieldULong(hResult, iRow, 13);
	m_dwProxyNode = DBGetFieldULong(hResult, iRow, 14);
	m_pszPerfTabSettings = DBGetField(hResult, iRow, 15, NULL, 0);
   TCHAR *pszTmp = DBGetField(hResult, iRow, 16, NULL, 0);
   setTransformationScript(pszTmp);
   free(pszTmp);

   m_pNode = pNode;
	m_columns = new ObjectArray<DCTableColumn>(8, 8, true);
	m_lastValue = NULL;

	DB_STATEMENT hStmt = DBPrepare(g_hCoreDB, _T("SELECT column_name,data_type,snmp_oid FROM dc_table_columns WHERE table_id=?"));
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
}

/**
 * Destructor
 */
DCTable::~DCTable()
{
	delete m_columns;
	delete m_lastValue;
}

/**
 * Clean expired data
 */
void DCTable::deleteExpiredData()
{
   TCHAR szQuery[256];
   time_t now;

   now = time(NULL);
   lock();
   _sntprintf(szQuery, 256, _T("DELETE FROM tdata_%d WHERE (item_id=%d) AND (tdata_timestamp<%ld)"),
              (int)m_pNode->Id(), (int)m_dwId, (long)(now - (time_t)m_iRetentionTime * 86400));
   unlock();
   QueueSQLRequest(szQuery);
}

/**
 * Delete all collected data
 */
bool DCTable::deleteAllData()
{
   TCHAR szQuery[256];
	bool success;

   lock();
   _sntprintf(szQuery, 256, _T("DELETE FROM tdata_%d WHERE item_id=%d"), m_pNode->Id(), (int)m_dwId);
	success = DBQuery(g_hCoreDB, szQuery) ? true : false;
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

   m_dwErrorCount = 0;
	delete m_lastValue;
	m_lastValue = (Table *)value;
	m_lastValue->setTitle(m_szDescription);
   m_lastValue->setSource(m_source);

	// Copy required fields into local variables
	DWORD tableId = m_dwId;
	DWORD nodeId = m_pNode->Id();

   unlock();

	// Save data to database
	// Object is unlocked, so only local variables can be used
	TCHAR query[256];
	_sntprintf(query, 256, _T("INSERT INTO tdata_%d (item_id,tdata_timestamp,tdata_row,tdata_column,tdata_value) VALUES (?,?,?,?,?)"), (int)nodeId);

	DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
	DB_STATEMENT hStmt = DBPrepare(hdb, query);
	if (hStmt != NULL)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, tableId);
		DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, (LONG)nTimeStamp);

		Table *data = (Table *)value;
		for(int col = 0; col < data->getNumColumns(); col++)
		{
			LONG colId = columnIdFromName(data->getColumnName(col));
			if (colId == 0)
				continue;	// cannot get column ID

			for(int row = 0; row < data->getNumRows(); row++)
			{
				DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, (LONG)row);
				DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, colId);
				DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, data->getAsString(row, col), DB_BIND_STATIC);
				DBExecute(hStmt);
			}
		}
		DBFreeStatement(hStmt);
	}

	DBConnectionPoolReleaseConnection(hdb);
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
		                       _T("instance_column=?,description=?,flags=?,source=?,snmp_port=?,polling_interval=?,")
                             _T("retention_time=?,status=?,system_tag=?,resource_id=?,proxy_node=?,")
									  _T("perftab_settings=? WHERE item_id=?"));
	}
	else
	{
		hStmt = DBPrepare(hdb, _T("INSERT INTO dc_tables (node_id,template_id,template_item_id,name,")
		                       _T("instance_column,description,flags,source,snmp_port,polling_interval,")
		                       _T("retention_time,status,system_tag,resource_id,proxy_node,perftab_settings,")
									  _T("item_id) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)"));
	}
	if (hStmt == NULL)
		return FALSE;

   lock();

	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, (m_pNode == NULL) ? (DWORD)0 : m_pNode->Id());
	DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_dwTemplateId);
	DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_dwTemplateItemId);
	DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, m_szName, DB_BIND_STATIC);
	DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, m_instanceColumn, DB_BIND_STATIC);
	DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, m_szDescription, DB_BIND_STATIC);
	DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, (DWORD)m_flags);
	DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, (LONG)m_source);
	DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, (DWORD)m_snmpPort);
	DBBind(hStmt, 10, DB_SQLTYPE_INTEGER, (LONG)m_iPollingInterval);
	DBBind(hStmt, 11, DB_SQLTYPE_INTEGER, (LONG)m_iRetentionTime);
	DBBind(hStmt, 12, DB_SQLTYPE_INTEGER, (LONG)m_status);
	DBBind(hStmt, 13, DB_SQLTYPE_VARCHAR, m_systemTag, DB_BIND_STATIC);
	DBBind(hStmt, 14, DB_SQLTYPE_INTEGER, m_dwResourceId);
	DBBind(hStmt, 15, DB_SQLTYPE_INTEGER, m_dwProxyNode);
	DBBind(hStmt, 16, DB_SQLTYPE_TEXT, m_pszPerfTabSettings, DB_BIND_STATIC);
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
			hStmt = DBPrepare(hdb, _T("INSERT INTO dc_table_columns (table_id,column_name,snmp_oid,data_type) VALUES (?,?,?,?)"));
			if (hStmt != NULL)
			{
				DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_dwId);
				for(int i = 0; i < m_columns->size(); i++)
				{
					DCTableColumn *column = m_columns->get(i);
					DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, column->getName(), DB_BIND_STATIC);
					SNMP_ObjectId *oid = column->getSnmpOid();
					DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, (oid != NULL) ? oid->getValueAsText() : NULL, DB_BIND_STATIC);
					DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, (LONG)column->getDataType());

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

   unlock();
	return result ? DCObject::saveToDB(hdb) : FALSE;
}

/**
 * Delete table object and collected data from database
 */
void DCTable::deleteFromDB()
{
   TCHAR szQuery[256];

	DCObject::deleteFromDB();

   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM dc_tables WHERE item_id=%d"), (int)m_dwId);
   QueueSQLRequest(szQuery);
   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM tdata_%d WHERE item_id=%d"), (int)m_pNode->Id(), (int)m_dwId);
   QueueSQLRequest(szQuery);
   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM dc_table_columns WHERE table_id=%d"), (int)m_dwId);
   QueueSQLRequest(szQuery);
}

/**
 * Create NXCP message with item data
 */
void DCTable::createMessage(CSCPMessage *pMsg)
{
	DCObject::createMessage(pMsg);

   lock();
   pMsg->SetVariable(VID_INSTANCE_COLUMN, m_instanceColumn);
	pMsg->SetVariable(VID_NUM_COLUMNS, (DWORD)m_columns->size());
	DWORD varId = VID_DCI_COLUMN_BASE;
	for(int i = 0; i < m_columns->size(); i++)
	{
		DCTableColumn *column = m_columns->get(i);
		pMsg->SetVariable(varId++, column->getName());
		pMsg->SetVariable(varId++, (WORD)column->getDataType());
		SNMP_ObjectId *oid = column->getSnmpOid();
		if (oid != NULL)
			pMsg->SetVariableToInt32Array(varId++, oid->getLength(), oid->getValue());
		else
			varId++;
		varId += 7;
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

	pMsg->GetVariableStr(VID_INSTANCE_COLUMN, m_instanceColumn, MAX_DB_STRING);

	m_columns->clear();
	int count = (int)pMsg->GetVariableLong(VID_NUM_COLUMNS);
	DWORD varId = VID_DCI_COLUMN_BASE;
	for(int i = 0; i < count; i++)
	{
		m_columns->add(new DCTableColumn(pMsg, varId));
		varId += 10;
	}

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
		if (m_instanceColumn[0] != 0)
			msg->SetVariable(VID_INSTANCE_COLUMN, m_instanceColumn);
	}
   unlock();
}

/**
 * Get summary of last collected value (to show along simple DCI values)
 */
void DCTable::fillLastValueSummaryMessage(CSCPMessage *pMsg, DWORD dwId)
{
	lock();
   pMsg->SetVariable(dwId++, m_dwId);
   pMsg->SetVariable(dwId++, m_szName);
   pMsg->SetVariable(dwId++, m_szDescription);
   pMsg->SetVariable(dwId++, (WORD)m_source);
   pMsg->SetVariable(dwId++, (WORD)DCI_DT_NULL);  // compatibility: data type
   pMsg->SetVariable(dwId++, _T(""));             // compatibility: value
   pMsg->SetVariable(dwId++, (DWORD)m_tLastPoll);
   pMsg->SetVariable(dwId++, (WORD)m_status);
	pMsg->SetVariable(dwId++, (WORD)getType());
	pMsg->SetVariable(dwId++, m_dwErrorCount);
	pMsg->SetVariable(dwId++, m_dwTemplateItemId);
   pMsg->SetVariable(dwId++, (WORD)0);            // compatibility: number of thresholds

	unlock();
}

/**
 * Get ID of instance column
 */
LONG DCTable::getInstanceColumnId()
{
	if (m_instanceColumn[0] != 0)
		return columnIdFromName(m_instanceColumn);
	return 0;	/* TODO: try to auto-detect instance column */
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
         dt = m_lastValue->getColumnFormat(index);
   }

   unlock();
   return dt;
}
