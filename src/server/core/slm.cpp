/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2011 NetXMS Team
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
** File: slm.cpp
**
**/

#include "nxcore.h"

static bool SlmLoadChecks(void)
{
	DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

	DBConnectionPoolReleaseConnection(hdb);

	return true;
}

static bool SlmInit(void)
{
	// Load SLM checks

	if (!SlmLoadChecks())
	{
		DbgPrintf(1, _T("Cannot load SLM checks"));
	}

	return true;
}

THREAD_RESULT THREAD_CALL ServiceLevelMonitoring(void *pArg)
{
	const DWORD dwInterval = 60;
	time_t currTime;

	DbgPrintf(1, _T("SLM thread started"));

	if (!SlmInit())
		goto finish;

	while(!IsShutdownInProgress())
	{
		currTime = time(NULL);
		if (SleepAndCheckForShutdown(dwInterval - (DWORD)(currTime % dwInterval)))
			break;      // Shutdown has arrived

		DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

		NetObj* curObject;

		for (int i = 0; i < g_pBizServiceRoot->m_dwChildCount; i++)
		{
			curObject = g_pBizServiceRoot->m_pChildList[i];
			curObject->calculateCompoundStatus();
		}

		DBConnectionPoolReleaseConnection(hdb);
	}

finish:
	DbgPrintf(1, _T("SLM thread terminated"));
	return THREAD_OK;
}
