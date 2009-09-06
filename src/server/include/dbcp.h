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
** File: dbcp.h
**
**/

#ifndef __dbConnectionPool_h__
#define __dbConnectionPool_h__

class LIBNXSRV_EXPORTABLE ConnectionPool
{
private:
	int basePoolSize;
	int maxPoolSize;
	int currentPoolSize;
	int cooldownTime;

	MUTEX poolAccessMutex;

	DB_HANDLE *dbHandles;
	bool *dbHandlesInUseMarker;
	time_t *dbHandleLastAccessTime;

	void populate();
	void shrink();

public:
	ConnectionPool(int basePoolSize, int maxPoolSize);
	~ConnectionPool();
	void setCooldownTime(int cooldownTime)
	{
		this->cooldownTime = cooldownTime;
	}

	DB_HANDLE acquireConnection();
	void releaseConnection(DB_HANDLE connection);
};

#endif // __dbConnectionPool_h__