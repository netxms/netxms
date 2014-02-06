/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2013 Victor Kirhenshtein
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
 * Query definition
 */
class Query
{
private:
   MUTEX m_mutex;
   THREAD m_pollingThread;
   TCHAR *m_name;
   TCHAR *m_dbid;
   TCHAR *m_query;
   int m_interval;
   time_t m_lastPoll;
   int m_status;
   TCHAR m_statusText[MAX_RESULT_LENGTH];
   DB_RESULT m_result;

   Query();

   void setError(const TCHAR *msg);

public:
   static Query *createFromConfig(const TCHAR *src);

   ~Query();

   void lock() { MutexLock(m_mutex); }
   void unlock() { MutexUnlock(m_mutex); }

   time_t getNextPoll() { return m_lastPoll + m_interval; }
   void poll();
   void joinPollingThread() { ThreadJoin(m_pollingThread); }

   LONG getResult(TCHAR *buffer);
   LONG fillResultTable(Table *table);

   const TCHAR *getName() { return m_name; }
   int getStatus() { return m_status; }
   const TCHAR *getStatusText() { return m_statusText; }
};

/**
 * Query object constructor
 */
Query::Query()
{
   m_name = NULL;
   m_dbid = NULL;
   m_query = NULL;
   m_lastPoll = 0;
   m_status = QUERY_STATUS_UNKNOWN;
   _tcscpy(m_statusText, _T("UNKNOWN"));
   m_result = NULL;
   m_pollingThread = INVALID_THREAD_HANDLE;
   m_mutex = MutexCreate();
}

/**
 * Query object destructor
 */
Query::~Query()
{
   safe_free(m_name);
   safe_free(m_dbid);
   safe_free(m_query);
   if (m_result != NULL)
      DBFreeResult(m_result);
   MutexDestroy(m_mutex);
}

/**
 * Get result from cell 0,0
 */
LONG Query::getResult(TCHAR *buffer)
{
   if (m_result == NULL)
      return SYSINFO_RC_ERROR;
   if (DBGetNumRows(m_result) == 0)
      return SYSINFO_RC_ERROR;
   DBGetField(m_result, 0, 0, buffer, MAX_RESULT_LENGTH);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Fill result table
 */
LONG Query::fillResultTable(Table *table)
{
   if (m_result == NULL)
      return SYSINFO_RC_ERROR;
   DBResultToTable(m_result, table);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Set error status
 */
void Query::setError(const TCHAR *msg)
{
   lock();
   m_status = QUERY_STATUS_ERROR;
   nx_strncpy(m_statusText, msg, MAX_RESULT_LENGTH);
   if (m_result != NULL)
   {
      DBFreeResult(m_result);
      m_result = NULL;
   }
   unlock();
}

/**
 * Poll
 */
void Query::poll()
{
   m_lastPoll = time(NULL);

   DB_HANDLE hdb = GetConnectionHandle(m_dbid);
   if (hdb == NULL)
   {
      AgentWriteDebugLog(4, _T("DBQUERY: Query::poll(%s): no connection handle for database %s"), m_name, m_dbid);
      setError(_T("DB connection not available"));
      return;
   }

   TCHAR errorText[DBDRV_MAX_ERROR_TEXT];
   DB_RESULT hResult = DBSelectEx(hdb, m_query, errorText);
   if (hResult == NULL)
   {
      AgentWriteDebugLog(4, _T("DBQUERY: Query::poll(%s): query failed (%s)"), m_name, errorText);
      setError(errorText);
      return;
   }

   lock();
   m_status = QUERY_STATUS_OK;
   _tcscpy(m_statusText, _T("OK"));
   if (m_result != NULL)
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
   TCHAR *config = _tcsdup(src);
   TCHAR *curr = config;
   Query *query = new Query;

   // Name
   TCHAR *s = _tcschr(config, _T(':'));
   if (s == NULL)
      goto fail;
   *s = 0;
   query->m_name = _tcsdup(config);
   curr = s + 1;

   // DB ID
   s = _tcschr(curr, _T(':'));
   if (s == NULL)
      goto fail;
   *s = 0;
   query->m_dbid = _tcsdup(curr);
   curr = s + 1;

   // interval
   s = _tcschr(curr, _T(':'));
   if (s == NULL)
      goto fail;
   *s = 0;
   query->m_interval = _tcstol(curr, NULL, 0);
   if ((query->m_interval < 1) || (query->m_interval > 86400))
   {
      AgentWriteDebugLog(1, _T("DBQuery: invalid interval %s for query %s"), curr, query->m_name);
      goto fail;
   }
   curr = s + 1;

   // Rest is SQL query
   query->m_query = _tcsdup(curr);
   free(config);
   return query;

fail:
   free(config);
   delete query;
   return NULL;
}

/**
 * Queries
 */
static ObjectArray<Query> s_queries;

/**
 * Acquire query object. Caller must call Query::unlock() to release query object.
 */
static Query *AcquireQueryObject(const TCHAR *name)
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
   return NULL;
}

/**
 * Polling thread
 */
static THREAD_RESULT THREAD_CALL PollingThread(void *arg)
{
   Query *query = (Query *)arg;
   AgentWriteDebugLog(3, _T("DBQuery: Polling thread for query %s started"), query->getName());

   int sleepTime = (int)(query->getNextPoll() - time(NULL));
   if (sleepTime <= 0)
      sleepTime = 1;
   while(!ConditionWait(g_condShutdown, sleepTime * 1000))
   {
      query->poll();
      sleepTime = (int)(query->getNextPoll() - time(NULL));
      if (sleepTime <= 0)
         sleepTime = 1;
   }

   AgentWriteDebugLog(3, _T("DBQuery: Polling thread for query %s stopped"), query->getName());
   return THREAD_OK;
}

/**
 * Start polling threads
 */
void StartPollingThreads()
{
   for(int i = 0; i < s_queries.size(); i++)
      ThreadCreate(PollingThread, 0, s_queries.get(i));
}

/**
 * Stop polling threads
 */
void StopPollingThreads()
{
   for(int i = 0; i < s_queries.size(); i++)
      s_queries.get(i)->joinPollingThread();
}

/**
 * Add query to the list from config
 * Format is following:
 *    name:dbid:interval:query
 */
bool AddQueryFromConfig(const TCHAR *config)
{
   Query *query = Query::createFromConfig(config);
   if (query != NULL)
   {
      s_queries.add(query);
      AgentWriteDebugLog(1, _T("DBQuery: query %s added for polling"), query->getName());
      return true;
   }
   return false;
}

/**
 * Get poll results - single value
 */
LONG H_PollResult(const TCHAR *param, const TCHAR *arg, TCHAR *value)
{
   TCHAR name[MAX_QUERY_NAME_LEN];
   AgentGetParameterArg(param, 1, name, MAX_QUERY_NAME_LEN);
   Query *query = AcquireQueryObject(name);
   if (query == NULL)
      return SYSINFO_RC_UNSUPPORTED;

   LONG rc = SYSINFO_RC_ERROR;
   switch(*arg)
   {
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
LONG H_PollResultTable(const TCHAR *param, const TCHAR *arg, Table *value)
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
