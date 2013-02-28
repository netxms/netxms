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
** File: dbquery.cpp
**
**/

#include "dbquery.h"


/**
 * Global variables
 */
CONDITION g_condShutdown;

/**
 * Subagent initialization
 */
static BOOL SubAgentInit(Config *config)
{
   int i;

	// Add database connections
	ConfigEntryList *databases = config->getSubEntries(_T("/DBQuery"), _T("Database"));
	ConfigEntry *queries = config->getEntry(_T("/DBQuery/Query"));
	if (queries != NULL)
	{
		for(i = 0; i < queries->getValueCount(); i++)
		{
			if (!AddQueryFromConfig(queries->getValue(i)))
			{
            AgentWriteLog(EVENTLOG_WARNING_TYPE,
                            _T("Unable to add ODBC query from configuration file. ")
									 _T("Original configuration record: %s"), queries->getValue(i));
			}
		}
	}

   // Create shutdown condition and start poller threads
   g_condShutdown = ConditionCreate(TRUE);
   //for(i = 0; i < (int)m_dwNumQueries; i++)
   //   m_pQueryList[i].hThread = ThreadCreateEx(PollerThread, 0, &m_pQueryList[i]);

   return TRUE;
}


/**
 * Called by master agent at unload
 */
static void SubAgentShutdown()
{
}

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
   { _T("DB.Query(*)"), H_DirectQuery, NULL, DCI_DT_STRING, _T("Direct database query result") },
   { _T("DB.QueryResult(*)"), H_PollResult, _T("R"), DCI_DT_STRING, _T("Database query result") },
   { _T("DB.QueryStatus(*)"), H_PollResult, _T("S"), DCI_DT_UINT, _T("Database query status") },
   { _T("DB.QueryStatusText(*)"), H_PollResult, _T("T"), DCI_DT_STRING, _T("Database query status as text") }
};

static NETXMS_SUBAGENT_TABLE m_tables[] =
{
   { _T("DB.Query(*)"), H_DirectQueryTable, NULL, NULL, _T("Direct database query result") },
};

static NETXMS_SUBAGENT_INFO m_info =
{
   NETXMS_SUBAGENT_INFO_MAGIC,
	_T("DBQUERY"), NETXMS_VERSION_STRING,
   SubAgentInit, SubAgentShutdown, NULL,
	sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
	m_parameters,
	0, NULL,	// lists
	sizeof(m_tables) / sizeof(NETXMS_SUBAGENT_TABLE),
	m_tables,
   0, NULL,	// actions
	0, NULL	// push parameters
};

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(DBQUERY)
{
   *ppInfo = &m_info;
   return TRUE;
}

/**
 * DLL entry point
 */
#ifdef _WIN32

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
      DisableThreadLibraryCalls(hInstance);
   return TRUE;
}

#endif
