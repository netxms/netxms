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

static DB_DRIVER m_driver;
static TCHAR m_server[256];
static TCHAR m_login[256];
static TCHAR m_password[256];
static TCHAR m_dbName[256];
static TCHAR m_schema[256];

static int m_basePoolSize;
static int m_maxPoolSize;
static int m_currentPoolSize;
static int m_cooldownTime;

static MUTEX m_poolAccessMutex;

static DB_HANDLE *m_dbHandles;
static bool *m_dbHandlesInUseMarker;
static time_t *m_dbHandleLastAccessTime;

static DB_HANDLE m_hFallback;

/**
 * Create connections on pool initialization
 */
static void DBConnectionPoolPopulate()
{
	TCHAR errorText[DBDRV_MAX_ERROR_TEXT];

	MutexLock(m_poolAccessMutex);
	for (int i = 0; i < m_basePoolSize; i++)
	{
		m_dbHandles[i] = DBConnect(m_driver, m_server, m_dbName, m_login, m_password, m_schema, errorText);
	}
	MutexUnlock(m_poolAccessMutex);
}

/**
 * Shrink connection pool up to base size when possible
 */
static void DBConnectionPoolShrink()
{
	MutexLock(m_poolAccessMutex);

	for (int i = 0; i < m_maxPoolSize; i++)
	{
		if (m_currentPoolSize <= m_basePoolSize)
		{
			break;
		}

		if (!m_dbHandlesInUseMarker[i] && m_dbHandles[i] != NULL)
		{
			if ((time(NULL) - m_dbHandleLastAccessTime[i]) > m_cooldownTime)
			{
				DBDisconnect(m_dbHandles[i]);
				m_dbHandles[i] = NULL;
				m_currentPoolSize--;
			}
		}
	}

	MutexUnlock(m_poolAccessMutex);
}

/**
 * Start connection pool
 */
bool LIBNXDB_EXPORTABLE DBConnectionPoolStartup(DB_DRIVER driver, const TCHAR *server, const TCHAR *dbName,
																const TCHAR *login, const TCHAR *password, const TCHAR *schema,
																int basePoolSize, int maxPoolSize, int cooldownTime,
																DB_HANDLE fallback)
{
	m_driver = driver;
	nx_strncpy(m_server, CHECK_NULL_EX(server), 256);
	nx_strncpy(m_dbName, CHECK_NULL_EX(dbName), 256);
	nx_strncpy(m_login, CHECK_NULL_EX(login), 256);
	nx_strncpy(m_password, CHECK_NULL_EX(password), 256);
	nx_strncpy(m_schema, CHECK_NULL_EX(schema), 256);

	m_basePoolSize = basePoolSize;
	m_currentPoolSize = basePoolSize;
	m_maxPoolSize = maxPoolSize;
	m_cooldownTime = cooldownTime;
	m_hFallback = fallback;

	m_poolAccessMutex = MutexCreate();

	m_dbHandles = new DB_HANDLE[maxPoolSize];
	m_dbHandlesInUseMarker = new bool[maxPoolSize];
	m_dbHandleLastAccessTime = new time_t[maxPoolSize];

	for (int i = 0; i < m_maxPoolSize; i++)
	{
		m_dbHandles[i] = NULL;
		m_dbHandlesInUseMarker[i] = false;
		m_dbHandleLastAccessTime[i] = 0;
	}

	DBConnectionPoolPopulate();

	__DBDbgPrintf(1, _T("Database Connection Pool initialized"));

	return true;
}

/**
 * Shutdown connection poo
 */
void LIBNXDB_EXPORTABLE DBConnectionPoolShutdown()
{
	MutexDestroy(m_poolAccessMutex);

	for (int i = 0; i < m_maxPoolSize; i++)
	{
		if (m_dbHandles[i] != NULL)
		{
			DBDisconnect(m_dbHandles[i]);
		}
	}

	delete[] m_dbHandles;
	delete[] m_dbHandlesInUseMarker;
	delete[] m_dbHandleLastAccessTime;
	
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
	for (int i = 0; i < m_maxPoolSize; i++)
	{
		if (m_dbHandles[i] != NULL && !m_dbHandlesInUseMarker[i])
		{
			handle = m_dbHandles[i];
			m_dbHandlesInUseMarker[i] = true;
			break;
		}
	}

	if (handle == NULL && m_currentPoolSize < m_maxPoolSize)
	{
		for (int i = 0; i < m_maxPoolSize; i++)
		{
			if (m_dbHandles[i] == NULL)
			{
				TCHAR errorText[DBDRV_MAX_ERROR_TEXT];
				m_dbHandles[i] = DBConnect(m_driver, m_server, m_dbName, m_login, m_password, m_schema, errorText);
				m_currentPoolSize++;

				handle = m_dbHandles[i];
				m_dbHandlesInUseMarker[i] = true;
				break;
			}
		}
	}

	MutexUnlock(m_poolAccessMutex);

	if (handle == NULL)
	{
		handle = m_hFallback;
	}

	return handle;
}

/**
 * Release acquired connection
 */
void LIBNXDB_EXPORTABLE DBConnectionPoolReleaseConnection(DB_HANDLE connection)
{
	if (connection == m_hFallback)
		return;

	MutexLock(m_poolAccessMutex);

	DB_HANDLE handle = NULL;
	for (int i = 0; i < m_maxPoolSize; i++)
	{
		if (m_dbHandles[i] == connection)
		{
			m_dbHandlesInUseMarker[i] = false;
			m_dbHandleLastAccessTime[i] = time(NULL);
			break;
		}
	}

	MutexUnlock(m_poolAccessMutex);

	DBConnectionPoolShrink();
}
