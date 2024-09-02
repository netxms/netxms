/*
** NetXMS - Network Management System
** Copyright (C) 2003-2023 Raden Solutions
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
** File: poll.cpp
**
**/

#include "dbquery.h"

/**
 * Query object constructor
 */
Query::Query()
{
   m_name = nullptr;
   m_dbid = nullptr;
   m_query = nullptr;
   m_interval = 60;
   m_lastPoll = 0;
   m_lastExecutionTime = 0;
   m_status = QUERY_STATUS_UNKNOWN;
   _tcscpy(m_statusText, _T("UNKNOWN"));
   m_result = nullptr;
   m_pollerThread = INVALID_THREAD_HANDLE;
   m_pollRequired = false;
   m_description = nullptr;
}

/**
 * Query object destructor
 */
Query::~Query()
{
   MemFree(m_name);
   MemFree(m_dbid);
   MemFree(m_query);
   MemFree(m_description);
   DBFreeResult(m_result);
}

/**
 * Get result from cell 0,0
 */
LONG Query::getResult(TCHAR *buffer)
{
   if (m_result == nullptr)
      return SYSINFO_RC_ERROR;

   if (DBGetNumRows(m_result) == 0)
   {
      if (g_allowEmptyResultSet)
      {
         buffer[0] = 0;
         return SYSINFO_RC_SUCCESS;
      }
      return SYSINFO_RC_ERROR;
   }
   DBGetField(m_result, 0, 0, buffer, MAX_RESULT_LENGTH);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Fill result table
 */
LONG Query::fillResultTable(Table *table)
{
   if (m_result == nullptr)
      return SYSINFO_RC_ERROR;
   DBResultToTable(m_result, table);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Set error status
 */
void Query::setError(const TCHAR *msg, uint32_t elapsedTime)
{
   lock();
   m_status = QUERY_STATUS_ERROR;
   _tcslcpy(m_statusText, msg, MAX_RESULT_LENGTH);
   m_lastExecutionTime = elapsedTime;
   if (m_result != nullptr)
   {
      DBFreeResult(m_result);
      m_result = nullptr;
   }
   unlock();
}

/**
 * Poll
 */
void Query::poll()
{
   m_lastPoll = time(nullptr);

   DB_HANDLE hdb = GetConnectionHandle(m_dbid);
   if (hdb == nullptr)
   {
      nxlog_debug_tag(DBQUERY_DEBUG_TAG, 4, _T("DBQUERY: Query::poll(%s): no connection handle for database %s"), m_name, m_dbid);
      setError(_T("DB connection not available"), 0);
      return;
   }

   nxlog_debug_tag(DBQUERY_DEBUG_TAG, 7, _T("DBQUERY: Query::poll(%s): Executing query \"%s\" in database \"%s\""), m_name, m_query, m_dbid);

   int64_t startTime = GetMonotonicClockTime();

   TCHAR errorText[DBDRV_MAX_ERROR_TEXT];
   DB_RESULT hResult = DBSelectEx(hdb, m_query, errorText);
   uint32_t elapsedTime = static_cast<uint32_t>(GetMonotonicClockTime() - startTime);
   if (hResult == nullptr)
   {
      nxlog_debug_tag(DBQUERY_DEBUG_TAG, 4, _T("DBQUERY: Query::poll(%s): query failed (%s)"), m_name, errorText);
      setError(errorText, elapsedTime);
      return;
   }

   lock();
   m_status = QUERY_STATUS_OK;
   _tcscpy(m_statusText, _T("OK"));
   m_lastExecutionTime = elapsedTime;
   DBFreeResult(m_result);
   m_result = hResult;
   unlock();
}

/**
 * Create new query object from config
 * Format is following:
 *    name:dbid:interval:query
 */
Query *Query::createFromConfig(const TCHAR *src)
{
   TCHAR *config = MemCopyString(src);
   TCHAR *curr = config;
   Query *query = new Query;

   // Name
   TCHAR *s = _tcschr(config, _T(':'));
   if (s == nullptr)
      goto fail;
   *s = 0;
   query->m_name = MemCopyString(config);
   curr = s + 1;

   // DB ID
   s = _tcschr(curr, _T(':'));
   if (s == nullptr)
      goto fail;
   *s = 0;
   query->m_dbid = MemCopyString(curr);
   curr = s + 1;

   // interval
   s = _tcschr(curr, _T(':'));
   if (s == nullptr)
      goto fail;
   *s = 0;
   query->m_interval = _tcstol(curr, nullptr, 0);
   if ((query->m_interval < 1) || (query->m_interval > 86400))
   {
      nxlog_debug_tag(DBQUERY_DEBUG_TAG, 1, _T("Invalid interval %s for query \"%s\""), curr, query->m_name);
      goto fail;
   }
   curr = s + 1;

   // Rest is SQL query
   query->m_query = MemCopyString(curr);
   query->m_pollRequired = true;
   MemFree(config);
   return query;

fail:
   MemFree(config);
   delete query;
   return nullptr;
}

/**
 * Create new query object from config
 * Created query is marked as not polled
 * Format is following:
 *    name:dbid:description:query
 */
Query *Query::createConfigurableFromConfig(const TCHAR *src)
{
   TCHAR *config = MemCopyString(src);
   TCHAR *curr = config;
   Query *query = new Query;

   // Name
   TCHAR *s = _tcschr(config, _T(':'));
   if (s == nullptr)
      goto fail;
   *s = 0;
   query->m_name = MemCopyString(config);
   curr = s + 1;

   // DB ID
   s = _tcschr(curr, _T(':'));
   if (s == nullptr)
      goto fail;
   *s = 0;
   query->m_dbid = MemCopyString(curr);
   curr = s + 1;

   // description
   s = _tcschr(curr, _T(':'));
   if (s == nullptr)
      goto fail;
   *s = 0;
   query->m_description = MemCopyString(curr);
   curr = s + 1;

   // Rest is SQL query
   query->m_query = MemCopyString(curr);
   MemFree(config);
   query->m_pollRequired = false;
   return query;

fail:
   MemFree(config);
   delete query;
   return nullptr;
}

/**
 * Queries
 */
static ObjectArray<Query> s_queries;

/**
 * Acquire query object. Caller must call Query::unlock() to release query object.
 */
Query *AcquireQueryObject(const TCHAR *name)
{
   // It is safe to scan query list without locks because
   // list itself and query names not changing after subagent initialization
   for(int i = 0; i < s_queries.size(); i++)
   {
      Query *q = s_queries.get(i);
      if (!_tcsicmp(q->getName(), name))
      {
         q->lock();
         return q;
      }
   }
   return nullptr;
}

/**
 * Polling thread
 */
static void PollerThread(Query *query)
{
   nxlog_debug_tag(DBQUERY_DEBUG_TAG, 3, _T("Polling thread for query \"%s\" started"), query->getName());

   int sleepTime = (int)(query->getNextPoll() - time(nullptr));
   if (sleepTime <= 0)
      sleepTime = 1;
   while(!g_condShutdown.wait(sleepTime * 1000))
   {
      query->poll();
      sleepTime = (int)(query->getNextPoll() - time(nullptr));
      if (sleepTime <= 0)
         sleepTime = 1;
   }

   nxlog_debug_tag(DBQUERY_DEBUG_TAG, 3, _T("Polling thread for query \"%s\" stopped"), query->getName());
}

/**
 * Start poller thread for query
 */
void Query::startPollerThread()
{
   m_pollerThread = ThreadCreateEx(PollerThread, this);
}

/**
 * Start polling threads
 */
void StartPollingThreads()
{
   for(int i = 0; i < s_queries.size(); i++)
   {
      if (s_queries.get(i)->isPollRequired())
         s_queries.get(i)->startPollerThread();
   }
}

/**
 * Stop polling threads
 */
void StopPollingThreads()
{
   for(int i = 0; i < s_queries.size(); i++)
   {
      s_queries.get(i)->joinPollerThread();
      delete s_queries.get(i);
   }
   nxlog_debug_tag(DBQUERY_DEBUG_TAG, 3, _T("All polling threads stopped"));
}

/**
 * Add query to the list from config
 * Format is following:
 *    name:dbid:interval:query
 */
bool AddQueryFromConfig(const TCHAR *config, Query **createdQuery)
{
   Query *query = Query::createFromConfig(config);
   if (query != nullptr)
   {
      s_queries.add(query);
      *createdQuery = query;
      nxlog_debug_tag(DBQUERY_DEBUG_TAG, 1, _T("Query \"%s\" added for polling"), query->getName());
      return true;
   }
   return false;
}

/**
 * Add configurable query to the list from config
 * Format is following:
 *    name:dbid:description:query
 */
bool AddConfigurableQueryFromConfig(const TCHAR *config, Query **createdQuery)
{
   Query *query = Query::createConfigurableFromConfig(config);
   if (query != nullptr)
   {
      s_queries.add(query);
      *createdQuery = query;
      nxlog_debug_tag(DBQUERY_DEBUG_TAG, 1, _T("Query \"%s\" added to query list"), query->getName());
      return true;
   }
   return false;
}

/**
 * Get poll results - single value
 */
LONG H_PollResult(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR name[MAX_QUERY_NAME_LEN];
   AgentGetParameterArg(param, 1, name, MAX_QUERY_NAME_LEN);
   Query *query = AcquireQueryObject(name);
   if (query == nullptr)
      return SYSINFO_RC_UNSUPPORTED;

   LONG rc = SYSINFO_RC_ERROR;
   switch(*arg)
   {
      case _T('E'):  // Execution time
         ret_uint(value, query->getLastExecutionTime());
         rc = SYSINFO_RC_SUCCESS;
         break;
      case _T('R'):  // Result
         rc = query->getResult(value);
         break;
      case _T('S'):  // Result
         ret_int(value, query->getStatus());
         rc = SYSINFO_RC_SUCCESS;
         break;
      case _T('T'):  // Result
         ret_string(value, query->getStatusText());
         rc = SYSINFO_RC_SUCCESS;
         break;
   }

   query->unlock();
   return rc;
}

/**
 * Get poll results - table
 */
LONG H_PollResultTable(const TCHAR *param, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   TCHAR name[MAX_QUERY_NAME_LEN];
   AgentGetParameterArg(param, 1, name, MAX_QUERY_NAME_LEN);
   Query *query = AcquireQueryObject(name);
   if (query == NULL)
      return SYSINFO_RC_UNSUPPORTED;

   LONG rc = query->fillResultTable(value);
   query->unlock();
   return rc;
}
