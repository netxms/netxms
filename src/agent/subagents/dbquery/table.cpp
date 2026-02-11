/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Raden Solutions
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: table.cpp
**
**/

#include "dbquery.h"

/**
 * DBQueryTable objects
 */
static ObjectArray<DBQueryTable> s_dbQueryTables(8, 8, Ownership::False);

/**
 * Add DBQueryTable to the list
 */
void AddDBQueryTable(DBQueryTable *t)
{
   s_dbQueryTables.add(t);
}

/**
 * Acquire DBQueryTable object by name. Caller must call unlock() to release.
 */
DBQueryTable *AcquireDBQueryTable(const TCHAR *name)
{
   for(int i = 0; i < s_dbQueryTables.size(); i++)
   {
      DBQueryTable *t = s_dbQueryTables.get(i);
      if (!_tcsicmp(t->getName(), name))
      {
         t->lock();
         return t;
      }
   }
   return nullptr;
}

/**
 * DBQueryTable constructor
 */
DBQueryTable::DBQueryTable(const TCHAR *name, ConfigEntry *config)
{
   m_name = MemCopyString(name);
   m_dbid = MemCopyString(config->getSubEntryValue(_T("Database")));
   m_query = MemCopyString(config->getSubEntryValue(_T("Query")));
   m_description = MemCopyString(config->getSubEntryValue(_T("Description")));
   m_pollingInterval = config->getSubEntryValueAsInt(_T("PollingInterval"), 0, 0);
   m_defaultColumnDataType = DCI_DT_STRING;
   m_cachedResult = nullptr;
   m_pollerThread = INVALID_THREAD_HANDLE;
   m_status = QUERY_STATUS_UNKNOWN;
   _tcscpy(m_statusText, _T("UNKNOWN"));

   // Parse instance columns
   const TCHAR *instanceColumnsStr = config->getSubEntryValue(_T("InstanceColumns"), 0, _T(""));
   m_instanceColumns = MemCopyString(instanceColumnsStr);
   m_instanceColumnList = SplitString(instanceColumnsStr, _T(','), &m_instanceColumnCount);
   for(int i = 0; i < m_instanceColumnCount; i++)
      Trim(m_instanceColumnList[i]);

   // Parse column data types from ColumnType sub-section
   unique_ptr<ObjectArray<ConfigEntry>> columnTypes = config->getSubEntries(_T("ColumnType"));
   if ((columnTypes != nullptr) && !columnTypes->isEmpty())
   {
      ConfigEntry *configEntry = columnTypes->get(0);
      unique_ptr<ObjectArray<ConfigEntry>> entries = configEntry->getSubEntries(nullptr);
      if (entries != nullptr)
      {
         for(int i = 0; i < entries->size(); i++)
         {
            ConfigEntry *col = entries->get(i);
            int dataType = TextToDataType(col->getValue());
            m_columnDataTypes.set(col->getName(), (dataType == -1) ? DCI_DT_STRING : dataType);
         }
      }
   }

   if (m_dbid == nullptr || m_query == nullptr)
   {
      nxlog_write_tag(NXLOG_WARNING, DBQUERY_DEBUG_TAG,
         _T("Table \"%s\": missing required Database or Query parameter"), m_name);
   }
}

/**
 * DBQueryTable destructor
 */
DBQueryTable::~DBQueryTable()
{
   MemFree(m_name);
   MemFree(m_dbid);
   MemFree(m_query);
   MemFree(m_description);
   MemFree(m_instanceColumns);
   for(int i = 0; i < m_instanceColumnCount; i++)
      MemFree(m_instanceColumnList[i]);
   MemFree(m_instanceColumnList);
   if (m_cachedResult != nullptr)
      DBFreeResult(m_cachedResult);
}

/**
 * Check if column is an instance column
 */
bool DBQueryTable::isInstanceColumn(const TCHAR *name) const
{
   for(int i = 0; i < m_instanceColumnCount; i++)
      if (!_tcsicmp(m_instanceColumnList[i], name))
         return true;
   return false;
}

/**
 * Get column data type
 */
int DBQueryTable::getColumnDataType(const TCHAR *name) const
{
   return m_columnDataTypes.getInt32(name, m_defaultColumnDataType);
}

/**
 * Fill table columns and rows from DB result with metadata
 */
void DBQueryTable::fillTableColumns(DB_RESULT hResult, Table *table)
{
   int numColumns = DBGetColumnCount(hResult);
   for(int c = 0; c < numColumns; c++)
   {
      TCHAR name[64];
      if (!DBGetColumnName(hResult, c, name, 64))
         _sntprintf(name, 64, _T("COL_%d"), c + 1);

      bool instanceFlag = isInstanceColumn(name);
      int dataType = getColumnDataType(name);
      table->addColumn(name, dataType, name, instanceFlag);
   }

   int numRows = DBGetNumRows(hResult);
   for(int r = 0; r < numRows; r++)
   {
      table->addRow();
      for(int c = 0; c < numColumns; c++)
      {
         table->setPreallocated(c, DBGetField(hResult, r, c, nullptr, 0));
      }
   }
}

/**
 * Poll - execute query and cache result
 */
void DBQueryTable::poll()
{
   DB_HANDLE hdb = GetConnectionHandle(m_dbid);
   if (hdb == nullptr)
   {
      nxlog_debug_tag(DBQUERY_DEBUG_TAG, 4, _T("DBQueryTable::poll(%s): no connection handle for database %s"), m_name, m_dbid);
      lock();
      m_status = QUERY_STATUS_ERROR;
      _tcslcpy(m_statusText, _T("DB connection not available"), MAX_RESULT_LENGTH);
      if (m_cachedResult != nullptr)
      {
         DBFreeResult(m_cachedResult);
         m_cachedResult = nullptr;
      }
      unlock();
      return;
   }

   nxlog_debug_tag(DBQUERY_DEBUG_TAG, 7, _T("DBQueryTable::poll(%s): executing query \"%s\" in database \"%s\""), m_name, m_query, m_dbid);

   TCHAR errorText[DBDRV_MAX_ERROR_TEXT];
   DB_RESULT hResult = DBSelectEx(hdb, m_query, errorText);
   if (hResult == nullptr)
   {
      nxlog_debug_tag(DBQUERY_DEBUG_TAG, 4, _T("DBQueryTable::poll(%s): query failed (%s)"), m_name, errorText);
      lock();
      m_status = QUERY_STATUS_ERROR;
      _tcslcpy(m_statusText, errorText, MAX_RESULT_LENGTH);
      if (m_cachedResult != nullptr)
      {
         DBFreeResult(m_cachedResult);
         m_cachedResult = nullptr;
      }
      unlock();
      return;
   }

   lock();
   m_status = QUERY_STATUS_OK;
   _tcscpy(m_statusText, _T("OK"));
   if (m_cachedResult != nullptr)
      DBFreeResult(m_cachedResult);
   m_cachedResult = hResult;
   unlock();
}

/**
 * Fill table from cached result (polled mode)
 */
LONG DBQueryTable::fillTable(Table *table)
{
   if (m_cachedResult == nullptr)
      return SYSINFO_RC_ERROR;
   fillTableColumns(m_cachedResult, table);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Fill table with on-demand query execution
 */
LONG DBQueryTable::fillTableOnDemand(Table *table)
{
   DB_HANDLE hdb = GetConnectionHandle(m_dbid);
   if (hdb == nullptr)
   {
      nxlog_debug_tag(DBQUERY_DEBUG_TAG, 4, _T("DBQueryTable::fillTableOnDemand(%s): no connection handle for database %s"), m_name, m_dbid);
      return SYSINFO_RC_ERROR;
   }

   nxlog_debug_tag(DBQUERY_DEBUG_TAG, 7, _T("DBQueryTable::fillTableOnDemand(%s): executing query \"%s\" in database \"%s\""), m_name, m_query, m_dbid);

   TCHAR errorText[DBDRV_MAX_ERROR_TEXT];
   DB_RESULT hResult = DBSelectEx(hdb, m_query, errorText);
   if (hResult == nullptr)
   {
      nxlog_debug_tag(DBQUERY_DEBUG_TAG, 4, _T("DBQueryTable::fillTableOnDemand(%s): query failed (%s)"), m_name, errorText);
      return SYSINFO_RC_ERROR;
   }

   fillTableColumns(hResult, table);
   DBFreeResult(hResult);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Polling thread for DBQueryTable
 */
static void DBQueryTablePollerThread(DBQueryTable *table)
{
   nxlog_debug_tag(DBQUERY_DEBUG_TAG, 3, _T("Polling thread for table \"%s\" started"), table->getName());

   table->poll();
   while(!g_condShutdown.wait(table->getPollingInterval() * 1000))
   {
      table->poll();
   }

   nxlog_debug_tag(DBQUERY_DEBUG_TAG, 3, _T("Polling thread for table \"%s\" stopped"), table->getName());
}

/**
 * Start poller thread
 */
void DBQueryTable::startPollerThread()
{
   m_pollerThread = ThreadCreateEx(DBQueryTablePollerThread, this);
}

/**
 * Start polling threads for all polled DBQueryTable objects
 */
void StartDBQueryTablePolling()
{
   for(int i = 0; i < s_dbQueryTables.size(); i++)
   {
      if (s_dbQueryTables.get(i)->isPolled())
         s_dbQueryTables.get(i)->startPollerThread();
   }
}

/**
 * Stop polling threads and clean up DBQueryTable objects
 */
void StopDBQueryTablePolling()
{
   for(int i = 0; i < s_dbQueryTables.size(); i++)
   {
      s_dbQueryTables.get(i)->joinPollerThread();
      delete s_dbQueryTables.get(i);
   }
   nxlog_debug_tag(DBQUERY_DEBUG_TAG, 3, _T("All table polling threads stopped"));
}

/**
 * Handler for DBQueryTable
 */
LONG H_DBQueryTable(const TCHAR *param, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   DBQueryTable *t = AcquireDBQueryTable(arg);
   if (t == nullptr)
      return SYSINFO_RC_UNSUPPORTED;

   LONG rc;
   if (t->isPolled())
      rc = t->fillTable(value);
   else
      rc = t->fillTableOnDemand(value);

   t->unlock();
   return rc;
}
