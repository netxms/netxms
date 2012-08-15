/*
** NetXMS ODBCQUERY subagent
** Copyright (C) 2006 Alex Kalimulin
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
** File: odbcquery.cpp
**
**/

#include "odbcquery.h"

#ifdef _WIN32
#define ODBCQUERY_EXPORTABLE __declspec(dllexport) __cdecl
#else
#define ODBCQUERY_EXPORTABLE
#endif

#define EL_WARN		EVENTLOG_WARNING_TYPE
#define EL_INFO		EVENTLOG_INFORMATION_TYPE
#define EL_ERROR		EVENTLOG_ERROR_TYPE
#define EL_DEBUG		EVENTLOG_DEBUG_TYPE


//
// Static data
//

static CONDITION m_hCondShutdown = INVALID_CONDITION_HANDLE;
static BOOL m_bShutdown = FALSE;
static DWORD m_dwNumQueries = 0;
static ODBC_QUERY *m_pQueryList = NULL;


//
// Poller thread
//

static THREAD_RESULT THREAD_CALL PollerThread(void *pArg)
{
   QWORD qwTime, qwPrevTime;
	ODBC_QUERY* pQuery;

	if ((pQuery = (ODBC_QUERY *)pArg) == NULL)
	{
		AgentWriteLog(EVENTLOG_ERROR_TYPE, _T("ODBC Internal error: NULL passed to thread"));
		goto thread_finish;
	}

	if ((pQuery->pSqlCtx = OdbcCtxAlloc()) == NULL)
	{
		AgentWriteLog(EVENTLOG_ERROR_TYPE, _T("Failed to allocate ODBC context"));
		goto thread_finish;
	}

	qwPrevTime = 0;
   while(!m_bShutdown)
   {
		qwTime = GetCurrentTimeMs();
		if (qwTime - qwPrevTime >= ((QWORD)pQuery->dwPollInterval) * 1000)
		{
			if (OdbcConnect(pQuery->pSqlCtx, pQuery->szOdbcSrc) < 0)
			{
				pQuery->dwCompletionCode = OdbcGetSqlErrorNumber(pQuery->pSqlCtx);
				AgentWriteDebugLog(2, _T("ODBC connect error: %s (%s)"), 
									    OdbcGetInfo(pQuery->pSqlCtx),
									    OdbcGetSqlError(pQuery->pSqlCtx));
			}
			else
			{
				if (OdbcQuerySelect(pQuery->pSqlCtx, pQuery->szSqlQuery, 
										  pQuery->szQueryResult, (size_t)MAX_DB_STRING) < 0)
				{
					pQuery->dwCompletionCode = OdbcGetSqlErrorNumber(pQuery->pSqlCtx);
					AgentWriteDebugLog(2, _T("ODBC query error: %s (%s)"), 
										    OdbcGetInfo(pQuery->pSqlCtx),
										    OdbcGetSqlError(pQuery->pSqlCtx));
				}
				else
				{
					pQuery->dwCompletionCode = 0;
				}
				OdbcDisconnect(pQuery->pSqlCtx);
			}
			qwPrevTime = GetCurrentTimeMs();
		}
      if (ConditionWait(m_hCondShutdown, 1000))
         break;
   }

thread_finish:
	if (pQuery->pSqlCtx)
	{
		OdbcCtxFree(pQuery->pSqlCtx);
	}

   return THREAD_OK;
}


//
// Handler for poller information
//

static LONG H_PollResult(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue)
{
   TCHAR szName[MAX_DB_STRING];
   DWORD i;

   if (!AgentGetParameterArg(pszParam, 1, szName, MAX_DB_STRING))
      return SYSINFO_RC_UNSUPPORTED;
   StrStrip(szName);

   for(i = 0; i < m_dwNumQueries; i++)
   {
		if (!_tcsicmp(m_pQueryList[i].szName, szName))
			break;
   }

   if (i == m_dwNumQueries)
      return SYSINFO_RC_UNSUPPORTED;   // No such target

	switch(*pArg)
	{
		case 'R':
			if (m_pQueryList[i].dwCompletionCode != 0)
				return SYSINFO_RC_ERROR;
			ret_string(pValue, m_pQueryList[i].szQueryResult);
			break;
		case 'S':
			ret_uint(pValue, m_pQueryList[i].dwCompletionCode);
			break;
		case 'T':
			ret_string(pValue, OdbcGetInfo(m_pQueryList[i].pSqlCtx));
			break;
		default:
	      return SYSINFO_RC_UNSUPPORTED;
	}

   return SYSINFO_RC_SUCCESS;
}


//
// Add query string from configuration file parameter
// Parameter value should be <query_name>:<sql_source>:<sql>:<poll_interval>
//

static BOOL AddQueryFromConfig(const TCHAR *pszCfg)
{
   TCHAR *ptr, *pszLine, *pszName = NULL;
	TCHAR *pszSrc = NULL, *pszQuery = NULL, *pszPollInterval = NULL;
	DWORD dwPollInterval = 0;
   BOOL bResult = FALSE;

	if (pszCfg == NULL)
	{
		AgentWriteLog(EVENTLOG_WARNING_TYPE, _T("ODBC: Null config entry, ignored"));
		bResult = TRUE;
		goto finish_add_query;
	}

   if ((pszLine = _tcsdup(pszCfg)) == NULL)
	{
		AgentWriteLog(EVENTLOG_ERROR_TYPE, _T("ODBC: String allocation failed"));
		goto finish_add_query;
	}
	StrStrip(pszLine);

	// Get query name
	pszName = pszLine;

	// Get source name
   ptr = _tcschr(pszLine, _T(':'));
   if (ptr != NULL)
   {
      *ptr = _T('\0');
      ptr++;
      StrStrip(ptr);
      pszSrc = ptr;
   }
	else
	{
		AgentWriteLog(EVENTLOG_WARNING_TYPE, _T("ODBC: Wrong query string format, ODBC source missing"));
		goto finish_add_query;
	}

	// Get SQL query
	ptr = _tcschr(pszSrc, _T(':'));
   if (ptr != NULL)
   {
      *ptr = _T('\0');
      ptr++;
      StrStrip(ptr);
      pszQuery = ptr;
   }
	else
	{
		AgentWriteLog(EVENTLOG_WARNING_TYPE, _T("ODBC: Wrong query string format, SQL query missing"));
		goto finish_add_query;
	}

	// Get poll interval
	ptr = _tcschr(pszQuery, _T(':'));
   if (ptr != NULL)
   {
      *ptr = _T('\0');
      ptr++;
      StrStrip(ptr);
      pszPollInterval = ptr;
   }
	else
	{
		AgentWriteLog(EVENTLOG_WARNING_TYPE, _T("ODBC: Wrong query string format, poll interval missing"));
		goto finish_add_query;
	}

	dwPollInterval = (DWORD)_tcstol(pszPollInterval, NULL, 0);
	if (dwPollInterval == (DWORD)LONG_MIN || 
		 dwPollInterval == (DWORD)LONG_MAX ||
		 dwPollInterval == 0)
	{
		AgentWriteLog(EVENTLOG_WARNING_TYPE, _T("ODBC: Wrong query string format, invalid poll interval"));
		goto finish_add_query;
	}

   m_pQueryList = (ODBC_QUERY*)realloc(m_pQueryList, 
						sizeof(ODBC_QUERY) * (m_dwNumQueries + 1));
	if (m_pQueryList == NULL)
	{
		AgentWriteLog(EVENTLOG_ERROR_TYPE, _T("ODBC: QueryList allocation failed"));
		goto finish_add_query;
	}

	nx_strncpy(m_pQueryList[m_dwNumQueries].szName, pszName, MAX_DB_STRING);
	nx_strncpy(m_pQueryList[m_dwNumQueries].szOdbcSrc, pszSrc, MAX_DB_STRING);
	nx_strncpy(m_pQueryList[m_dwNumQueries].szSqlQuery, pszQuery,MAX_SQL_QUERY_LEN);
	m_pQueryList[m_dwNumQueries].dwPollInterval = dwPollInterval;
	m_pQueryList[m_dwNumQueries].pSqlCtx = NULL;
	m_dwNumQueries++;
	bResult = TRUE;
	AgentWriteDebugLog(1, _T("ODBC: query \"%s\" successfully registered"), pszQuery);

finish_add_query:
   free(pszLine);
   return bResult;
}


//
// Subagent initialization
//

static BOOL SubAgentInit(Config *config)
{
   int i;
	ConfigEntry *ql;

	// Add queries from config
	ql = config->getEntry(_T("/ODBC/Query"));
	if (ql != NULL)
	{
		for(i = 0; i < ql->getValueCount(); i++)
		{
			if (!AddQueryFromConfig(ql->getValue(i)))
			{
            AgentWriteLog(EVENTLOG_WARNING_TYPE,
                            _T("Unable to add ODBC query from configuration file. ")
									 _T("Original configuration record: %s"), ql->getValue(i));
			}
		}
	}

   // Create shutdown condition and start poller threads
   m_hCondShutdown = ConditionCreate(TRUE);
   for(i = 0; i < (int)m_dwNumQueries; i++)
      m_pQueryList[i].hThread = ThreadCreateEx(PollerThread, 0, &m_pQueryList[i]);

   return TRUE;
}


//
// Called by master agent at unload
//

static void SubAgentShutdown()
{
   DWORD i;

   m_bShutdown = TRUE;
   if (m_hCondShutdown != INVALID_CONDITION_HANDLE)
      ConditionSet(m_hCondShutdown);

   for(i = 0; i < m_dwNumQueries; i++)
      ThreadJoin(m_pQueryList[i].hThread);
   safe_free(m_pQueryList);
}


//
// Subagent information
//

static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
   { _T("ODBC.QueryResult(*)"), H_PollResult, _T("R"), DCI_DT_STRING, _T("ODBC query result") },
   { _T("ODBC.QueryStatus(*)"), H_PollResult, _T("S"), DCI_DT_UINT, _T("ODBC query status") },
   { _T("ODBC.QueryStatusText(*)"), H_PollResult, _T("T"), DCI_DT_STRING, _T("ODBC query status as text") }
};

static NETXMS_SUBAGENT_INFO m_info =
{
   NETXMS_SUBAGENT_INFO_MAGIC,
	_T("ODBCQUERY"), NETXMS_VERSION_STRING,
   SubAgentInit, SubAgentShutdown, NULL,
	sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
	m_parameters,
	0, NULL,	// lists
	0, NULL,	// tables
   0, NULL,	// actions
	0, NULL	// push parameters
};


//
// Entry point for NetXMS agent
//

DECLARE_SUBAGENT_ENTRY_POINT(ODBCQUERY)
{
   *ppInfo = &m_info;
   return TRUE;
}

//
// DLL entry point
//

#ifdef _WIN32

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
      DisableThreadLibraryCalls(hInstance);
   return TRUE;
}

#endif
