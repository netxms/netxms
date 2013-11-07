/* 
** NetXMS - Network Management System
** Database Abstraction Library
** Copyright (C) 2008, 2009 Alex Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
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
** File: dbcp.cpp
**
**/

#include "libnxdb.h"

/**
 * Pool connection information
 */
struct PoolConnectionInfo
{
   DB_HANDLE handle;
   bool inUse;
   time_t lastAccessTime;
   time_t connectTime;
};

static DB_DRIVER m_driver;
static TCHAR m_server[256];
static TCHAR m_login[256];
static TCHAR m_password[256];
static TCHAR m_dbName[256];
static TCHAR m_schema[256];

static int m_basePoolSize;
static int m_maxPoolSize;
static int m_cooldownTime;
static int m_connectionTTL;

static MUTEX m_poolAccessMutex = INVALID_MUTEX_HANDLE;
static ObjectArray<PoolConnectionInfo> m_connections;
static DB_HANDLE m_hFallback;
static THREAD m_maintThread = INVALID_THREAD_HANDLE;
static CONDITION m_condShutdown = INVALID_CONDITION_HANDLE;

/**
 * Create connections on pool initialization
 */
static void DBConnectionPoolPopulate()
{
	TCHAR errorText[DBDRV_MAX_ERROR_TEXT];

	MutexLock(m_poolAccessMutex);
	for(int i = 0; i < m_basePoolSize; i++)
	{
      PoolConnectionInfo *conn = new PoolConnectionInfo;
      conn->handle = DBConnect(m_driver, m_server, m_dbName, m_login, m_password, m_schema, errorText);
      if (conn->handle != NULL)
      {
         conn->inUse = false;
         conn->connectTime = time(NULL);
         conn->lastAccessTime = conn->connectTime;
         m_connections.add(conn);
      }
      else
      {
         __DBDbgPrintf(3, _T("Database Connection Pool: cannot create DB connection %d (%s)"), i, errorText);
      }
	}
	MutexUnlock(m_poolAccessMutex);
}

/**
 * Shrink connection pool up to base size when possible
 */
static void DBConnectionPoolShrink()
{
	MutexLock(m_poolAccessMutex);

   time_t now = time(NULL);
   for(int i = m_basePoolSize; i < m_connections.size(); i++)
	{
      PoolConnectionInfo *conn = m_connections.get(i);
		if (!conn->inUse && (now - conn->lastAccessTime > m_cooldownTime))
		{
			DBDisconnect(conn->handle);
         m_connections.remove(i);
         i--;
		}
	}

	MutexUnlock(m_poolAccessMutex);
}

/**
 * Reconnect old connections
 */
static void DBConnectionPoolReconnect()
{
   time_t now = time(NULL);
   MutexLock(m_poolAccessMutex);
   for(int i = 0; i < m_connections.size(); i++)
   {
      PoolConnectionInfo *conn = m_connections.get(i);
      if (!conn->inUse && (now - conn->connectTime > m_connectionTTL))
      {
	      DBDisconnect(conn->handle);

	      TCHAR errorText[DBDRV_MAX_ERROR_TEXT];
         conn->handle = DBConnect(m_driver, m_server, m_dbName, m_login, m_password, m_schema, errorText);
         if (conn->handle != NULL)
         {
            conn->connectTime = now;
            conn->lastAccessTime = now;

            __DBDbgPrintf(3, _T("Database Connection Pool: connection %d reconnected"), i);
         }
         else
         {
            __DBDbgPrintf(3, _T("Database Connection Pool: connection %d reconnect failure (%s)"), i, errorText);
            m_connections.remove(i);
         }
         break;   // no more then one reconnect at a time to prevent locking pool for long time
      }
   }
   MutexUnlock(m_poolAccessMutex);
}

/**
 * Pool maintenance thread
 */
static THREAD_RESULT THREAD_CALL MaintenanceThread(void *arg)
{
	__DBDbgPrintf(1, _T("Database Connection Pool maintenance thread started"));

   while(!ConditionWait(m_condShutdown, 300000))
   {
      DBConnectionPoolShrink();
      if (m_connectionTTL > 0)
      {
         DBConnectionPoolReconnect();
      }
   }

	__DBDbgPrintf(1, _T("Database Connection Pool maintenance thread stopped"));
   return THREAD_OK;
}

/**
 * Start connection pool
 */
bool LIBNXDB_EXPORTABLE DBConnectionPoolStartup(DB_DRIVER driver, const TCHAR *server, const TCHAR *dbName,
																const TCHAR *login, const TCHAR *password, const TCHAR *schema,
																int basePoolSize, int maxPoolSize, int cooldownTime,
																int connTTL, DB_HANDLE fallback)
{
	m_driver = driver;
	nx_strncpy(m_server, CHECK_NULL_EX(server), 256);
	nx_strncpy(m_dbName, CHECK_NULL_EX(dbName), 256);
	nx_strncpy(m_login, CHECK_NULL_EX(login), 256);
	nx_strncpy(m_password, CHECK_NULL_EX(password), 256);
	nx_strncpy(m_schema, CHECK_NULL_EX(schema), 256);

	m_basePoolSize = basePoolSize;
	m_maxPoolSize = maxPoolSize;
	m_cooldownTime = cooldownTime;
   m_connectionTTL = connTTL;
	m_hFallback = fallback;

	m_poolAccessMutex = MutexCreate();
   m_connections.setOwner(true);
   m_condShutdown = ConditionCreate(TRUE);

	DBConnectionPoolPopulate();

   m_maintThread = ThreadCreateEx(MaintenanceThread, 0, NULL);

	__DBDbgPrintf(1, _T("Database Connection Pool initialized"));

	return true;
}

/**
 * Shutdown connection pool
 */
void LIBNXDB_EXPORTABLE DBConnectionPoolShutdown()
{
   ConditionSet(m_condShutdown);
   ThreadJoin(m_maintThread);

   ConditionDestroy(m_condShutdown);
	MutexDestroy(m_poolAccessMutex);

   for(int i = 0; i < m_connections.size(); i++)
	{
      DBDisconnect(m_connections.get(i)->handle);
	}

   m_connections.clear();

	__DBDbgPrintf(1, _T("Database Connection Pool terminated"));

}

/**
 * Acquire connection from pool. This function never fails - if it's impossible to acquire
 * pooled connection, fallback connection will be returned.
 */
DB_HANDLE LIBNXDB_EXPORTABLE DBConnectionPoolAcquireConnection()
{
	MutexLock(m_poolAccessMutex);

	DB_HANDLE handle = NULL;
   for(int i = 0; i < m_connections.size(); i++)
	{
      PoolConnectionInfo *conn = m_connections.get(i);
		if (!conn->inUse)
		{
			handle = conn->handle;
			conn->inUse = true;
			break;
		}
	}

   if (handle == NULL && (m_connections.size() < m_maxPoolSize))
	{
	   TCHAR errorText[DBDRV_MAX_ERROR_TEXT];
      PoolConnectionInfo *conn = new PoolConnectionInfo;
      conn->handle = DBConnect(m_driver, m_server, m_dbName, m_login, m_password, m_schema, errorText);
      conn->inUse = true;
      conn->connectTime = time(NULL);
      conn->lastAccessTime = conn->connectTime;
      m_connections.add(conn);
      handle = conn->handle;
	}

	MutexUnlock(m_poolAccessMutex);

	if (handle == NULL)
	{
		handle = m_hFallback;
   	__DBDbgPrintf(1, _T("Database Connection Pool exhausted, fallback connection used"));
	}

	return handle;
}

/**
 * Release acquired connection
 */
void LIBNXDB_EXPORTABLE DBConnectionPoolReleaseConnection(DB_HANDLE handle)
{
	if (handle == m_hFallback)
		return;

	MutexLock(m_poolAccessMutex);

   for(int i = 0; i < m_connections.size(); i++)
	{
      PoolConnectionInfo *conn = m_connections.get(i);
      if (conn->handle == handle)
		{
         conn->inUse = false;
         conn->lastAccessTime = time(NULL);
			break;
		}
	}

	MutexUnlock(m_poolAccessMutex);
}

/**
 * Get current size of DB connection pool
 */
int LIBNXDB_EXPORTABLE DBConnectionPoolGetSize()
{
	MutexLock(m_poolAccessMutex);
   int size = m_connections.size();
	MutexUnlock(m_poolAccessMutex);
   return size;
}

/**
 * Get number of acquired connections in DB connection pool
 */
int LIBNXDB_EXPORTABLE DBConnectionPoolGetAcquiredCount()
{
   int count = 0;
	MutexLock(m_poolAccessMutex);
   for(int i = 0; i < m_connections.size(); i++)
      if (m_connections.get(i)->inUse)
         count++;
	MutexUnlock(m_poolAccessMutex);
   return count;
}
