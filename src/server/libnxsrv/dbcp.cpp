/* 
** NetXMS - Network Management System
** Server Library
** Copyright (C) 2008 Alex Kirhenshtein
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
** File: dbcp.cpp
**
**/

#include "libnxsrv.h"
#include "dbcp.h"

ConnectionPool::ConnectionPool(int basePoolSize, int maxPoolSize)
{
	this->basePoolSize = basePoolSize;
	this->maxPoolSize = maxPoolSize;
	this->currentPoolSize = basePoolSize;
	this->cooldownTime = 300; // 5 min
	
	poolAccessMutex = MutexCreate();

	dbHandles = new DB_HANDLE[maxPoolSize];
	dbHandlesInUseMarker = new bool[maxPoolSize];
	dbHandleLastAccessTime = new time_t[maxPoolSize];

	for (int i = 0; i < maxPoolSize; i++)
	{
		dbHandles[i] = NULL;
		dbHandlesInUseMarker[i] = false;
		dbHandleLastAccessTime[i] = 0;
	}

	populate();
}

ConnectionPool::~ConnectionPool()
{
	MutexDestroy(poolAccessMutex);

	for (int i = 0; i < maxPoolSize; i++)
	{
		if (dbHandles[i] != NULL)
		{
			DBDisconnect(dbHandles[i]);
		}
	}

	delete dbHandles;
	delete dbHandlesInUseMarker;
	delete dbHandleLastAccessTime;
}

void ConnectionPool::populate()
{
	MutexLock(poolAccessMutex, INFINITE);

	for (int i = 0; i < basePoolSize; i++)
	{
		dbHandles[i] = DBConnect();
	}

	MutexUnlock(poolAccessMutex);
}

void ConnectionPool::shrink()
{
	MutexLock(poolAccessMutex, INFINITE);

	for (int i = 0; i < maxPoolSize; i++)
	{
		if (currentPoolSize <= basePoolSize)
		{
			break;
		}

		if (!dbHandlesInUseMarker[i] && dbHandles[i] != NULL)
		{
			if ((time(NULL) - dbHandleLastAccessTime[i]) > cooldownTime)
			{
				DBDisconnect(dbHandles[i]);
				dbHandles[i] = NULL;
				currentPoolSize--;
			}
		}
	}

	MutexUnlock(poolAccessMutex);
}

DB_HANDLE ConnectionPool::acquireConnection()
{
	MutexLock(poolAccessMutex, INFINITE);

	DB_HANDLE handle = NULL;
	for (int i = 0; i < maxPoolSize; i++)
	{
		if (dbHandles[i] != NULL && !dbHandlesInUseMarker[i])
		{
			handle = dbHandles[i];
			dbHandlesInUseMarker[i] = true;
			break;
		}
	}

	if (handle == NULL && currentPoolSize < maxPoolSize)
	{
		for (int i = 0; i < maxPoolSize; i++)
		{
			if (dbHandles[i] == NULL)
			{
				dbHandles[i] = DBConnect();
				currentPoolSize++;

				handle = dbHandles[i];
				dbHandlesInUseMarker[i] = true;
				break;
			}
		}
	}

	MutexUnlock(poolAccessMutex);

	shrink();

	return handle;
}

void ConnectionPool::releaseConnection(DB_HANDLE connection)
{
	MutexLock(poolAccessMutex, INFINITE);

	DB_HANDLE handle = NULL;
	for (int i = 0; i < maxPoolSize; i++)
	{
		if (dbHandles[i] == connection)
		{
			dbHandlesInUseMarker[i] = false;
			dbHandleLastAccessTime[i] = time(NULL);
			break;
		}
	}

	MutexUnlock(poolAccessMutex);

	shrink();

}
