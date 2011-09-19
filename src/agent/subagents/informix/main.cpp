/*
** NetXMS subagent for Informix monitoring
** Copyright (C) 2011 Raden Solutions
**/

#include "informix_subagent.h"

CONDITION g_shutdownCondition;
MUTEX g_paramAccessMutex;
int g_dbCount;
DB_DRIVER g_driverHandle = NULL;
DatabaseInfo g_dbInfo[MAX_DATABASES];
DatabaseData g_dbData[MAX_DATABASES];

THREAD_RESULT THREAD_CALL queryThread(void *arg);

DBParameterGroup g_paramGroup[] = {
	{
		1100, _T("Informix.Sessions."),
		_T("select ") DB_NULLARG_MAGIC _T(" ValueName, count(*) Count from syssessions"),
		2, { NULL }, 0
	}, 
	{
		1100, _T("Informix.Databases."),
			_T("select name ValueName, owner Owner, is_logging Logged, created Created from sysdatabases"),
			4, { NULL }, 0
	}, 
	{
		1100, _T("Informix.Dbspaces."),
			_T("select name ValueName,sum(chksize) PageSize,sum(chksize)-sum(nfree) PagesUsed,")
			_T("sum(nfree) PagesFree,round((sum(nfree))/(sum(chksize))*100,2) PctFree from sysdbspaces d,syschunks c ")
			_T("where d.dbsnum=c.dbsnum	group by name order by name"),
			5, { NULL }, 0
	}, 
	0
};

//
// Handler functions
//

LONG getParameters(const TCHAR *parameter, const TCHAR *argument, TCHAR *value)
{
	LONG ret = SYSINFO_RC_UNSUPPORTED;
	TCHAR dbId[MAX_STR];
	TCHAR entity[MAX_STR];

	// Get id of the database requested
	if (!AgentGetParameterArg(parameter, 1, dbId, MAX_STR))
		return ret;
	if (!AgentGetParameterArg(parameter, 2, entity, MAX_STR) || entity[0] == _T('\0'))
		nx_strncpy(entity, DB_NULLARG_MAGIC, MAX_STR);

	AgentWriteDebugLog(5, _T("%s: ***** got request for params: dbid='%s', param='%s'"), MYNAMESTR, dbId, parameter);

	// Loop through databases and find an entry in g_dbInfo[] for this id
	for (int i = 0; i <= g_dbCount; i++)
	{
		if (!_tcsnicmp(g_dbInfo[i].id, dbId, MAX_STR))	// found DB
		{
			// Loop through parameter groups and check whose prefix matches the parameter requested
			for (int k = 0; g_paramGroup[k].prefix; k++)
			{
				if (!_tcsnicmp(g_paramGroup[k].prefix, parameter, _tcslen(g_paramGroup[k].prefix))) // found prefix
				{
					MutexLock(g_dbInfo[i].accessMutex, INFINITE);
					// Loop through the values
					AgentWriteDebugLog(7, _T("%s: valuecount %d"), MYNAMESTR, g_paramGroup[k].valueCount[i]);
					for (int j = 0; j < g_paramGroup[k].valueCount[i]; j++)
					{
						StringMap* map = (g_paramGroup[k].values[i])[j].attrs;
						TCHAR* name = (g_paramGroup[k].values[i])[j].name;
						Trim(name);
						if (!_tcsnicmp(name, entity, MAX_STR))	// found value which matches the parameters argument
						{
							TCHAR key[MAX_STR];
							nx_strncpy(key, parameter + _tcslen(g_paramGroup[k].prefix), MAX_STR);
							TCHAR* place = _tcschr(key, _T('('));
							if (place != NULL)
							{
								*place = _T('\0');
								const TCHAR* dbval = map->get(key);
								ret_string(value, dbval);
								ret = SYSINFO_RC_SUCCESS;
							}
							break;
						}
					}
					MutexUnlock(g_dbInfo[i].accessMutex);

					break;
				}
			}
			break;
		}
	}

	return ret;
}


//
// Subagent initialization
//

static BOOL SubAgentInit(Config *config)
{
	BOOL result = TRUE;
	static DatabaseInfo info;
	int i;
	static NX_CFG_TEMPLATE configTemplate[] = 
	{
		{ _T("Id"),					CT_STRING,	0, 0, MAX_STR, 0,	info.id },	
		{ _T("DBName"),				CT_STRING,	0, 0, MAX_STR, 0,	info.dsn },	
		{ _T("DBLogin"),			CT_STRING,	0, 0, MAX_USERNAME, 0,	info.username },	
		{ _T("DBPassword"),			CT_STRING,	0, 0, MAX_PASSWORD, 0,	info.password },
		{ _T(""), CT_END_OF_LIST, 0, 0, 0, 0, NULL }
	};

	// Init db driver
	g_driverHandle = DBLoadDriver(_T("informix.ddr"), NULL, TRUE, NULL, NULL);
	if (g_driverHandle == NULL)
	{
		AgentWriteLog(EVENTLOG_ERROR_TYPE, _T("%s: failed to load db driver"), MYNAMESTR);
		result = FALSE;
	}

	if (result)
	{
		g_shutdownCondition = ConditionCreate(TRUE);
	}

	// Load configuration from "informix" section to allow simple configuration
	// of one database without XML includes
	memset(&info, 0, sizeof(info));
	g_dbCount = -1;
	if (config->parseTemplate(_T("INFORMIX"), configTemplate))
	{
		if (info.dsn[0] != 0)
		{
			if (info.id[0] == 0)
				_tcscpy(info.id, info.dsn);
			memcpy(&g_dbInfo[++g_dbCount], &info, sizeof(info));
			g_dbInfo[g_dbCount].accessMutex = MutexCreate();
		}
	}

	// Load configuration
	for(i = 1; result && i <= MAX_DATABASES; i++)
	{
		TCHAR section[MAX_STR];
		memset(&info, 0, sizeof(info));
		_sntprintf(section, MAX_STR, _T("informix/databases/database#%d"), i);
		if ((result = config->parseTemplate(section, configTemplate)) != TRUE)
		{
			AgentWriteLog(EVENTLOG_ERROR_TYPE, _T("%s: error parsing configuration template"), MYNAMESTR);
			return FALSE;
		}
		if (info.dsn[0] == 0)
			continue;
		
		if (info.id[0] == 0)
			_tcscpy(info.id, info.dsn);
		memcpy(&g_dbInfo[++g_dbCount], &info, sizeof(info));

		if (info.username[0] == '\0' || info.password[0] == '\0')
		{
			AgentWriteLog(EVENTLOG_ERROR_TYPE, _T("%s: error getting username and/or password for "), MYNAMESTR);
			result = FALSE;
		}
		if (result && (g_dbInfo[g_dbCount].accessMutex = MutexCreate()) == NULL)
		{
			AgentWriteLog(EVENTLOG_ERROR_TYPE, _T("%s: failed to create mutex (%d)"), MYNAMESTR, i);
			result = FALSE;
		}
	}

	// Exit if no usable configuration found
	if (result && g_dbCount < 0)
	{
		AgentWriteLog(EVENTLOG_ERROR_TYPE, _T("%s: no databases to monitor"), MYNAMESTR);
		result = FALSE;
	}

	// Run query thread for each database configured
	for (i = 0; result && i <= g_dbCount; i++)
	{
		g_dbInfo[i].queryThreadHandle = ThreadCreateEx(queryThread, 0, CAST_TO_POINTER(i, void *));
	}

	return result;
}


//
// Shutdown handler
//

static void SubAgentShutdown()
{
	AgentWriteLog(EVENTLOG_INFORMATION_TYPE, _T("%s: shutting down"), MYNAMESTR);
	ConditionSet(g_shutdownCondition);
	for (int i = 0; i <= g_dbCount; i++)
	{
		ThreadJoin(g_dbInfo[i].queryThreadHandle);
		MutexDestroy(g_dbInfo[i].accessMutex);
	}
	ConditionDestroy(g_shutdownCondition);
}

//
// Figure out Informix DBMS version
//

static int getInformixVersion(DB_HANDLE handle) 
{
	return 1100;
}

//
// Thread for SQL queries
//

THREAD_RESULT THREAD_CALL queryThread(void* arg)
{
	int dbIndex = CAST_FROM_POINTER(arg, int);
	DatabaseInfo& db = g_dbInfo[dbIndex];
	const DWORD pollInterval = 60 * 1000L;	// 1 minute
	int waitTimeout;
	QWORD startTimeMs;
	TCHAR errorText[DBDRV_MAX_ERROR_TEXT];

	while (true)
	{
		db.handle = DBConnect(g_driverHandle, NULL, db.dsn, db.username, db.password, NULL, errorText);
		if (db.handle != NULL)
		{
			AgentWriteLog(EVENTLOG_INFORMATION_TYPE, _T("%s: connected to DB"), MYNAMESTR);
			db.connected = true;
			db.version = getInformixVersion(db.handle);
		}
		else
		{
			AgentWriteLog(EVENTLOG_ERROR_TYPE, _T("%s: failed to connect, %s"), MYNAMESTR, errorText);
		}

		while (db.connected)
		{
			startTimeMs = GetCurrentTimeMs();

			// Do queries
			if (!(db.connected = getParametersFromDB(dbIndex)))
			{
				break;
			}

			waitTimeout = pollInterval - DWORD(GetCurrentTimeMs() - startTimeMs);
			if (ConditionWait(g_shutdownCondition, waitTimeout < 0 ? 1 : waitTimeout))
				goto finish;
		}

		// Try to reconnect every 30 secs
		if (ConditionWait(g_shutdownCondition, DWORD(30 * 1000)))
			break;
	}

finish:
	if (db.connected && db.handle != NULL)
	{
		DBDisconnect(db.handle);
	}

	return THREAD_OK;
}


bool getParametersFromDB( int dbIndex )
{
	bool ret = true;
	DatabaseInfo& info = g_dbInfo[dbIndex];

	if (!info.connected)
	{
		return false;
	}

	MutexLock(info.accessMutex, INFINITE);

	for (int i = 0; g_paramGroup[i].prefix; i++)
	{
		AgentWriteDebugLog(7, _T("%s: got entry for '%s'"), MYNAMESTR, g_paramGroup[i].prefix);

		if (g_paramGroup[i].version > info.version)	// this parameter group is not supported for this DB
			continue; 

		// Release previously allocated array of values for this group
		for (int j = 0; j < g_paramGroup[i].valueCount[dbIndex]; j++)
			delete (g_paramGroup[i].values[dbIndex])[j].attrs;
		safe_free((void*)g_paramGroup[i].values[dbIndex]);

		DB_RESULT queryResult = DBSelect(info.handle, g_paramGroup[i].query);
		if (queryResult == NULL)
		{
			ret = false;
			break;
		}

		int rows = DBGetNumRows(queryResult);
		g_paramGroup[i].values[dbIndex] = (DBParameter*)malloc(sizeof(DBParameter) * rows);
		g_paramGroup[i].valueCount[dbIndex]		= rows;
		for (int j = 0; j < rows; j++)
		{
			TCHAR colname[MAX_STR];
			DBGetField(queryResult, j, 0, (g_paramGroup[i].values[dbIndex])[j].name, MAX_STR);
			(g_paramGroup[i].values[dbIndex])[j].attrs = new StringMap;
			for (int k = 1; DBGetColumnName(queryResult, k, colname, MAX_STR); k++) 
			{
				TCHAR colval[MAX_STR];
				DBGetField(queryResult, j, k, colval, MAX_STR);
				// AgentWriteDebugLog(9, _T("%s: getParamsFromDB: colname '%s' ::: colval '%s'"), MYNAMESTR, colname, colval);
				(g_paramGroup[i].values[dbIndex])[j].attrs->set(colname, colval);
			}
		}

		DBFreeResult(queryResult);
	}

	MutexUnlock(info.accessMutex);

	return ret;
}

//
// Subagent information
//


static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
	{ _T("Informix.Session.Count(*)"), getParameters, "X", DCI_DT_INT, _T("Informix/Sessions: Number of sessions opened") },
	{ _T("Informix.Database.Owner(*)"), getParameters, "X", DCI_DT_STRING, _T("Informix/Databases: Database owner") },
	{ _T("Informix.Database.Created(*)"), getParameters, "X", DCI_DT_STRING, _T("Informix/Databases: Creation date") },
	{ _T("Informix.Database.Logged(*)"), getParameters, "X", DCI_DT_INT, _T("Informix/Databases: Is logged") },
	{ _T("Informix.Dbspace.PageSize(*)"), getParameters, "X", DCI_DT_INT, _T("Informix/Dbspaces: Page size") },
	{ _T("Informix.Dbspace.Pages.Used(*)"), getParameters, "X", DCI_DT_INT, _T("Informix/Dbspaces: Number of pages used in dbspace") },
	{ _T("Informix.Dbspace.Pages.Free(*)"), getParameters, "X", DCI_DT_INT, _T("Informix/Dbspaces: Number of pages free in dbspace") },
	{ _T("Informix.Dbspace.Pages.FreePerc(*)"), getParameters, "X", DCI_DT_INT, _T("Informix/Dbspaces: Percentage of free space in dbspace") },
};

/*
static NETXMS_SUBAGENT_ENUM m_enums[] =
{
};
*/

static NETXMS_SUBAGENT_INFO m_info =
{
	NETXMS_SUBAGENT_INFO_MAGIC,
	_T("INFORMIX"), NETXMS_VERSION_STRING,
	SubAgentInit, SubAgentShutdown, NULL,
	sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM), m_parameters,
	0,	NULL,
	/*sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
	m_parameters,
	sizeof(m_enums) / sizeof(NETXMS_SUBAGENT_ENUM),
	m_enums,*/
	0,	NULL
};


//
// Entry point for NetXMS agent
//

DECLARE_SUBAGENT_ENTRY_POINT(INFORMIX)
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
