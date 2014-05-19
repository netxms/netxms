/*
** NetXMS - Network Management System
** Copyright (C) 2003-2012 Victor Kirhenshtein
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
** File: dbquery.h
**
**/

#ifndef _dbquery_h_
#define _dbquery_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nms_agent.h>
#include <nxdbapi.h>

#define MAX_DBID_LEN       64
#define MAX_QUERY_NAME_LEN 64

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
   TCHAR *m_description;
   int m_interval;
   time_t m_lastPoll;
   int m_status;
   TCHAR m_statusText[MAX_RESULT_LENGTH];
   DB_RESULT m_result;
   bool m_pollRequired;

   Query();

   void setError(const TCHAR *msg);

public:
   static Query *createFromConfig(const TCHAR *src);
   static Query *createConfigurableFromConfig(const TCHAR *src);

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
   const TCHAR *getDBid() { return m_dbid; }
   const TCHAR *getQuery() { return m_query; }
   const TCHAR *getDescription() { return m_description; }
   bool isPollRequired() { return m_pollRequired; }
};

/**
 * Query status codes
 */
enum QueryStatusCode
{
   QUERY_STATUS_UNKNOWN = -1,
   QUERY_STATUS_OK = 0,
   QUERY_STATUS_ERROR = 1
};

/**
 * database connection
 */
class DBConnection
{
private:
   TCHAR *m_id;
	TCHAR *m_driver;
	TCHAR *m_server;
	TCHAR *m_dbName;
	TCHAR *m_login;
	TCHAR *m_password;
	DB_DRIVER m_hDriver;
	DB_HANDLE m_hdb;

   DBConnection();

public:
   static DBConnection *createFromConfig(const TCHAR *config);

   ~DBConnection();

   bool connect();

   const TCHAR *getId() { return m_id; }
   DB_HANDLE getHandle() { return m_hdb; }
};

/**
 * handlers
 */
LONG H_DirectQuery(const TCHAR *param, const TCHAR *arg, TCHAR *value);
LONG H_DirectQueryTable(const TCHAR *param, const TCHAR *arg, Table *value);
LONG H_DirectQueryConfigurable(const TCHAR *param, const TCHAR *arg, TCHAR *value);
LONG H_DirectQueryConfigurableTable(const TCHAR *param, const TCHAR *arg, Table *value);
LONG H_PollResult(const TCHAR *param, const TCHAR *arg, TCHAR *value);
LONG H_PollResultTable(const TCHAR *param, const TCHAR *arg, Table *value);

/**
 * Functions
 */
bool AddDatabaseFromConfig(const TCHAR *db);
bool AddQueryFromConfig(const TCHAR *config, Query** createdQuery);
bool AddConfigurableQueryFromConfig(const TCHAR *config, Query** createdQuery);
void ShutdownConnections();
DB_HANDLE GetConnectionHandle(const TCHAR *dbid);
void StartPollingThreads();
void StopPollingThreads();
void DBResultToTable(DB_RESULT hResult, Table *table);
Query *AcquireQueryObject(const TCHAR *name);

/**
 * Shutdown condition
 */
extern CONDITION g_condShutdown;

#endif
