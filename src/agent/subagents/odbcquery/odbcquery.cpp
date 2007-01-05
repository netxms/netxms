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
		NxWriteAgentLog(EL_ERROR, _T("Internal error: null passed to thread"));
		goto thread_finish;
	}

	if ((pQuery->pSqlCtx = OdbcCtxAlloc()) == NULL)
	{
		NxWriteAgentLog(EL_ERROR, _T("Failed to allocate ODBC context"));
		goto thread_finish;
	}

	if (OdbcConnect(pQuery->pSqlCtx, pQuery->szOdbcSrc) < 0)
	{
		NxWriteAgentLog(EL_ERROR, _T("Failed to connect to src '%s'"), 
							 pQuery->szOdbcSrc);
		goto thread_finish;
	}

	NxWriteAgentLog(EL_INFO, _T("ODBC: Connected to src '%s'"), pQuery->szOdbcSrc);

	qwPrevTime = 0;

   while(!m_bShutdown)
   {
		qwTime = GetCurrentTimeMs();
		if (qwTime - qwPrevTime >= ((QWORD)pQuery->dwPollInterval)*1000)
		{
			if (OdbcQuerySelect1(pQuery->pSqlCtx, pQuery->szSqlQuery, 
										pQuery->szQueryResult, (size_t)MAX_DB_STRING) < 0)
			{
				pQuery->dwCompletionCode = 0x00FFFFFF;		// Generic error
				NxWriteAgentLog(EL_WARN, "ODBC Error: %s (%s)", 
									 OdbcGetInfo(pQuery->pSqlCtx),
									 OdbcGetSqlError(pQuery->pSqlCtx));
			}
			else
			{
				pQuery->dwCompletionCode = 0;
				//NxWriteAgentLog(EL_INFO, _T("Result: '%s'"), pQuery->szQueryResult);
			}
			qwPrevTime = GetCurrentTimeMs();
		}
      if (ConditionWait(m_hCondShutdown, 1000))
         break;
   }

thread_finish:
	if (pQuery->pSqlCtx)
	{
		OdbcDisconnect(pQuery->pSqlCtx);
		OdbcCtxFree(pQuery->pSqlCtx);
	}

   return THREAD_OK;
}


//
// Handler for poller information
//

static LONG H_PollResult(TCHAR *pszParam, char *pArg, TCHAR *pValue)
{
   TCHAR szName[MAX_DB_STRING];
   DWORD i;

   if (!NxGetParameterArg(pszParam, 1, szName, MAX_DB_STRING))
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
			ret_string(pValue, m_pQueryList[i].szQueryResult);
			break;
		case 'S':
			ret_uint(pValue, m_pQueryList[i].dwCompletionCode);
			break;
		default:
	      return SYSINFO_RC_UNSUPPORTED;
	}

   return SYSINFO_RC_SUCCESS;
}


//
// Called by master agent at unload
//

static void UnloadHandler(void)
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
// Add query string from configuration file parameter
// Parameter value should be <query_name>:<sql_source>:<sql>:<poll_interval>
//

static BOOL AddQueryFromConfig(TCHAR *pszCfg)
{
   TCHAR *ptr, *pszLine, *pszName = NULL;
	TCHAR *pszSrc = NULL, *pszQuery = NULL, *pszPollInterval = NULL;
	DWORD dwPollInterval = 0;
   BOOL bResult = FALSE;

	if (pszCfg == NULL)
	{
		NxWriteAgentLog(EVENTLOG_WARNING_TYPE, _T("Null config entry, ignored"));
		bResult = TRUE;
		goto finish_add_query;
	}

   if ((pszLine = _tcsdup(pszCfg)) == NULL)
	{
		NxWriteAgentLog(EVENTLOG_ERROR_TYPE, _T("String allocation failed"));
		goto finish_add_query;
	}

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
		NxWriteAgentLog(EVENTLOG_WARNING_TYPE, _T("Wrong query string format, ODBC source missing"));
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
		NxWriteAgentLog(EVENTLOG_WARNING_TYPE, _T("Wrong query string format, SQL query missing"));
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
		NxWriteAgentLog(EVENTLOG_WARNING_TYPE, _T("Wrong query string format, "
							 "poll interval missing"));
		goto finish_add_query;
	}

	dwPollInterval = (DWORD)_tcstol(pszPollInterval, NULL, 0);
	if (dwPollInterval == (DWORD)LONG_MIN || 
		 dwPollInterval == (DWORD)LONG_MAX ||
		 dwPollInterval == 0)
	{
		NxWriteAgentLog(EVENTLOG_WARNING_TYPE, _T("Wrong query string format, "
							 "invalid poll interval"));
		goto finish_add_query;
	}

   m_pQueryList = (ODBC_QUERY*)realloc(m_pQueryList, 
						sizeof(ODBC_QUERY) * (m_dwNumQueries + 1));
	if (m_pQueryList == NULL)
	{
		NxWriteAgentLog(EVENTLOG_ERROR_TYPE, _T("QueryList allocation failed"));
		goto finish_add_query;
	}

	nx_strncpy(m_pQueryList[m_dwNumQueries].szName, pszName, MAX_DB_STRING);
	nx_strncpy(m_pQueryList[m_dwNumQueries].szOdbcSrc, pszSrc, MAX_DB_STRING);
	nx_strncpy(m_pQueryList[m_dwNumQueries].szSqlQuery, pszQuery,MAX_DB_STRING);
	m_pQueryList[m_dwNumQueries].dwPollInterval = dwPollInterval;
	m_pQueryList[m_dwNumQueries].pSqlCtx = NULL;
	m_dwNumQueries++;
	bResult = TRUE;
	NxWriteAgentLog(EVENTLOG_INFORMATION_TYPE, _T("Query: %s"), pszQuery);

finish_add_query:
   free(pszLine);
   return bResult;
}


//
// Subagent information
//

static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
   { _T("ODBC.QueryResult(*)"), H_PollResult, "R", DCI_DT_STRING, _T("ODBC query result") },
   { _T("ODBC.QueryStatus(*)"), H_PollResult, "S", DCI_DT_UINT, _T("ODBC query status") }
};

static NETXMS_SUBAGENT_INFO m_info =
{
   NETXMS_SUBAGENT_INFO_MAGIC,
	_T("ODBCQUERY"), _T(NETXMS_VERSION_STRING),
   UnloadHandler, NULL,
	sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
	m_parameters,
	0, NULL,	// enums
   0, NULL	// actions
};


//
// Configuration file template
//

static TCHAR *m_pszQueryList = NULL;
static NX_CFG_TEMPLATE cfgTemplate[] =
{
   { _T("Query"), CT_STRING_LIST, _T('\n'), 0, 0, 0, &m_pszQueryList },
   { _T(""), CT_END_OF_LIST, 0, 0, 0, 0, NULL }
};


//
// Entry point for NetXMS agent
//

DECLARE_SUBAGENT_INIT(ODBCQUERY)
{
   DWORD i, dwResult;

   // Load configuration
   dwResult = NxLoadConfig(pszConfigFile, _T("ODBC"), cfgTemplate, FALSE);
   if (dwResult == NXCFG_ERR_OK)
   {
      TCHAR *pItem, *pEnd;

      // Parse target list
      if (m_pszQueryList != NULL)
      {
         for(pItem = m_pszQueryList; *pItem != 0; pItem = pEnd + 1)
         {
            pEnd = _tcschr(pItem, _T('\n'));
            if (pEnd != NULL)
               *pEnd = 0;
            StrStrip(pItem);
            if (!AddQueryFromConfig(pItem))
               NxWriteAgentLog(EVENTLOG_WARNING_TYPE,
                               _T("Unable to add ODBC query from configuration file. ")
                               _T("Original configuration record: %s"), pItem);
         }
         free(m_pszQueryList);
      }

      // Create shutdown condition and start poller threads
      m_hCondShutdown = ConditionCreate(TRUE);
      for(i = 0; i < m_dwNumQueries; i++)
         m_pQueryList[i].hThread = ThreadCreateEx(PollerThread, 0, &m_pQueryList[i]);
   }
   else
   {
      safe_free(m_pszQueryList);
   }

   *ppInfo = &m_info;
   return dwResult == NXCFG_ERR_OK;
}

//
// DLL entry point
//

#ifdef _WIN32

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   return TRUE;
}

#endif
