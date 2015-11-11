/* 
** NetXMS - Network Management System
** Database Abstraction Library
** Copyright (C) 2008-2015 Raden Solutions
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

static bool s_initialized = false;
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
static THREAD m_maintThread = INVALID_THREAD_HANDLE;
static CONDITION m_condShutdown = INVALID_CONDITION_HANDLE;
static CONDITION m_condRelease = INVALID_CONDITION_HANDLE;

/**
 * Create connections on pool initialization
 */
static bool DBConnectionPoolPopulate()
{
	TCHAR errorText[DBDRV_MAX_ERROR_TEXT];
	bool success = false;

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
         conn->usageCount = 0;
         conn->srcFile[0] = 0;
         conn->srcLine = 0;
         m_connections.add(conn);
         success = true;
      }
      else
      {
         __DBDbgPrintf(3, _T("Database Connection Pool: cannot create DB connection %d (%s)"), i, errorText);
         delete conn;
      }
	}
	MutexUnlock(m_poolAccessMutex);
	return success;
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

/*
 * Reset connection
 */
static bool ResetConnection(PoolConnectionInfo *conn)
{
	time_t now = time(NULL);
	DBDisconnect(conn->handle);

	TCHAR errorText[DBDRV_MAX_ERROR_TEXT];
	conn->handle = DBConnect(m_driver, m_server, m_dbName, m_login, m_password, m_schema, errorText);
	if (conn->handle != NULL)
   {
		conn->connectTime = now;
		conn->lastAccessTime = now;
		conn->usageCount = 0;

		__DBDbgPrintf(3, _T("Database Connection Pool: connection %p reconnected"), conn->handle);
		return true;
	}
   else
   {
		__DBDbgPrintf(3, _T("Database Connection Pool: connection %p reconnect failure (%s)"), conn->handle, errorText);
		return false;
	}
}

/**
 * Callback for sorting reset list
 */
static int ResetListSortCallback(const void *e1, const void *e2)
{
   return ((PoolConnectionInfo *)e1)->usageCount > ((PoolConnectionInfo *)e2)->usageCount ? -1 :
		(((PoolConnectionInfo *)e1)->usageCount == ((PoolConnectionInfo *)e2)->usageCount ? 0 : 1);
}

/**
 * Reset expired connections
 */
static void ResetExpiredConnections()
{
   time_t now = time(NULL);

   MutexLock(m_poolAccessMutex);

	int i, availCount = 0;
   ObjectArray<PoolConnectionInfo> reconnList(m_connections.size(), 16, false);
	for(i = 0; i < m_connections.size(); i++)
	{
		PoolConnectionInfo *conn = m_connections.get(i);
		if (!conn->inUse)
      {
         availCount++;
         if (now - conn->connectTime > m_connectionTTL)
         {
            reconnList.add(conn);
         }
      }
	}
	
   int count = min(availCount / 2 + 1, reconnList.size()); // reset no more than 50% of available connections
   if (count < reconnList.size())
   {
      reconnList.sort(ResetListSortCallback);
      while(reconnList.size() > count)
         reconnList.remove(count);
   }

   for(i = 0; i < count; i++)
      reconnList.get(i)->inUse = true;
   MutexUnlock(m_poolAccessMutex);

   // do reconnects
   for(i = 0; i < count; i++)
	{
   	PoolConnectionInfo *conn = reconnList.get(i);
   	bool success = ResetConnection(conn);
   	MutexLock(m_poolAccessMutex);
		if (success)
		{
			conn->inUse = false;
		}
		else
		{
			m_connections.remove(conn);
		}
		MutexUnlock(m_poolAccessMutex);
	}
}

/**
 * Pool maintenance thread
 */
static THREAD_RESULT THREAD_CALL MaintenanceThread(void *arg)
{
	__DBDbgPrintf(1, _T("Database Connection Pool maintenance thread started"));

   while(!ConditionWait(m_condShutdown, (m_connectionTTL > 0) ? m_connectionTTL * 750 : 300000))
   {
      DBConnectionPoolShrink();
      if (m_connectionTTL > 0)
      {
         ResetExpiredConnections();
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
																int connTTL)
{
   if (s_initialized)
      return true;   // already initialized

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

	m_poolAccessMutex = MutexCreate();
   m_connections.setOwner(true);
   m_condShutdown = ConditionCreate(TRUE);
   m_condRelease = ConditionCreate(FALSE);

	if (!DBConnectionPoolPopulate())
	{
	   // cannot open at least one connection
	   ConditionDestroy(m_condShutdown);
	   ConditionDestroy(m_condRelease);
	   MutexDestroy(m_poolAccessMutex);
	   return false;
	}

   m_maintThread = ThreadCreateEx(MaintenanceThread, 0, NULL);

   s_initialized = true;
	__DBDbgPrintf(1, _T("Database Connection Pool initialized"));

	return true;
}

/**
 * Shutdown connection pool
 */
void LIBNXDB_EXPORTABLE DBConnectionPoolShutdown()
{
   if (!s_initialized)
      return;

   ConditionSet(m_condShutdown);
   ThreadJoin(m_maintThread);

   ConditionDestroy(m_condShutdown);
   ConditionDestroy(m_condRelease);
	MutexDestroy(m_poolAccessMutex);

   for(int i = 0; i < m_connections.size(); i++)
	{
      DBDisconnect(m_connections.get(i)->handle);
	}

   m_connections.clear();

   s_initialized = false;
	__DBDbgPrintf(1, _T("Database Connection Pool terminated"));

}

/**
 * Acquire connection from pool. This function never fails - if it's impossible to acquire
 * pooled connection, calling thread will be suspended until there will be connection available.
 */
DB_HANDLE LIBNXDB_EXPORTABLE __DBConnectionPoolAcquireConnection(const char *srcFile, int srcLine)
{
retry:
	MutexLock(m_poolAccessMutex);

	DB_HANDLE handle = NULL;

	// find less used connection
   UINT32 count = 0xFFFFFFFF;
	int index = -1;
   for(int i = 0; (i < m_connections.size()) && (count > 0); i++)
	{
      PoolConnectionInfo *conn = m_connections.get(i);
      if (!conn->inUse && (conn->usageCount < count))
      {
         count = conn->usageCount;
         index = i;
		}
	}

	if (index > -1)
	{
		PoolConnectionInfo *conn = m_connections.get(index);
		handle = conn->handle;
		conn->inUse = true;
		conn->lastAccessTime = time(NULL);
		conn->usageCount++;
		strncpy(conn->srcFile, srcFile, 128);
		conn->srcLine = srcLine;
	}
   else if (m_connections.size() < m_maxPoolSize)
	{
	   TCHAR errorText[DBDRV_MAX_ERROR_TEXT];
      PoolConnectionInfo *conn = new PoolConnectionInfo;
      conn->handle = DBConnect(m_driver, m_server, m_dbName, m_login, m_password, m_schema, errorText);
      if (conn->handle != NULL)
      {
         conn->inUse = true;
         conn->connectTime = time(NULL);
         conn->lastAccessTime = conn->connectTime;
         conn->usageCount = 0;
         strncpy(conn->srcFile, srcFile, 128);
         conn->srcLine = srcLine;
         m_connections.add(conn);
         handle = conn->handle;
      }
      else
      {
         __DBDbgPrintf(3, _T("Database Connection Pool: cannot create additional DB connection (%s)"), errorText);
         delete conn;
      }
	}

	MutexUnlock(m_poolAccessMutex);

	if (handle == NULL)
	{
   	__DBDbgPrintf(1, _T("Database Connection Pool exhausted (call from %hs:%d)"), srcFile, srcLine);
      ConditionWait(m_condRelease, 10000);
      __DBDbgPrintf(5, _T("Database Connection Pool: retry acquire connection (call from %hs:%d)"), srcFile, srcLine);
      goto retry;
	}

   __DBDbgPrintf(7, _T("Database Connection Pool: handle %p acquired (call from %hs:%d)"), handle, srcFile, srcLine);
	return handle;
}

/**
 * Release acquired connection
 */
void LIBNXDB_EXPORTABLE DBConnectionPoolReleaseConnection(DB_HANDLE handle)
{
	MutexLock(m_poolAccessMutex);

   for(int i = 0; i < m_connections.size(); i++)
	{
      PoolConnectionInfo *conn = m_connections.get(i);
      if (conn->handle == handle)
		{
         conn->inUse = false;
         conn->lastAccessTime = time(NULL);
         conn->srcFile[0] = 0;
         conn->srcLine = 0;
			break;
		}
	}

	MutexUnlock(m_poolAccessMutex);

   __DBDbgPrintf(7, _T("Database Connection Pool: handle %p released"), handle);
   ConditionPulse(m_condRelease);
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

/**
 * Get copy of active DB connections.
 * Returned list must be deleted by the caller.
 */
ObjectArray<PoolConnectionInfo> LIBNXDB_EXPORTABLE *DBConnectionPoolGetConnectionList()
{
   ObjectArray<PoolConnectionInfo> *list = new ObjectArray<PoolConnectionInfo>(32, 32, true);
	MutexLock(m_poolAccessMutex);
   for(int i = 0; i < m_connections.size(); i++)
      if (m_connections.get(i)->inUse)
      {
         list->add((PoolConnectionInfo *)nx_memdup(m_connections.get(i), sizeof(PoolConnectionInfo)));
      }
	MutexUnlock(m_poolAccessMutex);
   return list;
}
