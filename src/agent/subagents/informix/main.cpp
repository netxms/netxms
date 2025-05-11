/*
** NetXMS subagent for Informix monitoring
** Copyright (C) 2011-2021 Raden Solutions
**/

#include "informix_subagent.h"
#include <netxms-version.h>

Condition g_shutdownCondition(true);
Mutex g_paramAccessMutex;
int g_dbCount;
DB_DRIVER g_driverHandle = nullptr;
DatabaseInfo g_dbInfo[MAX_DATABASES];
DatabaseData g_dbData[MAX_DATABASES];

static void QueryThread(int dbIndex);

DBParameterGroup g_paramGroup[] =
{
	{
		1100, _T("Informix.Session."),
		_T("select ") DB_NULLARG_MAGIC _T(" ValueName, count(*) Count from syssessions"),
		2, { NULL }, 0
	},
	{
		1100, _T("Informix.Database."),
			_T("select name ValueName, owner Owner, is_logging Logged, created Created from sysdatabases"),
			4, { NULL }, 0
	},
	{
		1100, _T("Informix.Dbspace.Pages."),
			_T("select name ValueName,sum(chksize) PageSize,sum(chksize)-sum(nfree) Used,")
			_T("sum(nfree) Free,round((sum(nfree))/(sum(chksize))*100,2) FreePerc from sysdbspaces d,syschunks c ")
			_T("where d.dbsnum=c.dbsnum	group by name order by name"),
			5, { NULL }, 0
	},
	0
};

/**
 * Handler functions
 */
LONG H_DatabaseParameter(const TCHAR *parameter, const TCHAR *argument, TCHAR *value, AbstractCommSession *session)
{
	LONG ret = SYSINFO_RC_UNSUPPORTED;
	TCHAR dbId[MAX_STR];
	TCHAR entity[MAX_STR];

	// Get id of the database requested
	if (!AgentGetParameterArg(parameter, 1, dbId, MAX_STR))
		return ret;
	if (!AgentGetParameterArg(parameter, 2, entity, MAX_STR) || entity[0] == _T('\0'))
		_tcslcpy(entity, DB_NULLARG_MAGIC, MAX_STR);

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
					g_dbInfo[i].accessMutex->lock();
					// Loop through the values
					AgentWriteDebugLog(7, _T("%s: valuecount %d"), MYNAMESTR, g_paramGroup[k].valueCount[i]);
					for (int j = 0; j < g_paramGroup[k].valueCount[i]; j++)
					{
						StringMap *map = (g_paramGroup[k].values[i])[j].attrs;
						TCHAR *name = (g_paramGroup[k].values[i])[j].name;
						if (!_tcsnicmp(name, entity, MAX_STR))	// found value which matches the parameters argument
						{
							TCHAR key[MAX_STR];
							_tcslcpy(key, parameter + _tcslen(g_paramGroup[k].prefix), MAX_STR);
							TCHAR* place = _tcschr(key, _T('('));
							if (place != NULL)
							{
								*place = _T('\0');
								const TCHAR* dbval = map->get(key);
                        if (dbval != NULL)
                        {
								   ret_string(value, dbval);
								   ret = SYSINFO_RC_SUCCESS;
                        }
                        else
                        {
                           ret = SYSINFO_RC_ERROR;
                        }
							}
							break;
						}
					}
					g_dbInfo[i].accessMutex->unlock();
					break;
				}
			}
			break;
		}
	}

	return ret;
}

/**
 * Subagent initialization
 */
static bool SubAgentInit(Config *config)
{
	bool result = true;
	static DatabaseInfo info;
	int i;
   static NX_CFG_TEMPLATE configTemplate[] =
   {
      { _T("Id"),                CT_STRING, 0, 0, MAX_STR,       0, info.id },
      { _T("DBServer"),          CT_STRING, 0, 0, MAX_STR,       0, info.server },
      { _T("DBName"),            CT_STRING, 0, 0, MAX_STR,       0, info.dsn },
      { _T("DBLogin"),           CT_STRING, 0, 0, MAX_USERNAME,  0, info.username },
      { _T("DBPassword"),        CT_STRING, 0, 0, MAX_PASSWORD,  0, info.password },
      { _T("DBPasswordEncrypted"), CT_STRING, 0, 0, MAX_PASSWORD, 0, info.password },
      { _T(""), CT_END_OF_LIST, 0, 0, 0, 0, nullptr }
   };

	// Init db driver
	g_driverHandle = DBLoadDriver(_T("informix.ddr"), nullptr, nullptr, nullptr);
	if (g_driverHandle == nullptr)
	{
		AgentWriteLog(EVENTLOG_ERROR_TYPE, _T("%s: failed to load db driver"), MYNAMESTR);
		result = false;
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
			memcpy(&g_dbInfo[++g_dbCount], &info, sizeof(DatabaseInfo));
			g_dbInfo[g_dbCount].accessMutex = new Mutex();
		}

      DecryptPassword(info.username, info.password, info.password, MAX_PASSWORD);
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
			return false;
		}
		if (info.dsn[0] == 0)
			continue;

		if (info.id[0] == 0)
			_tcscpy(info.id, info.dsn);
		memcpy(&g_dbInfo[++g_dbCount], &info, sizeof(DatabaseInfo));

		if (info.username[0] == '\0')
		{
			AgentWriteLog(EVENTLOG_ERROR_TYPE, _T("%s: error getting username for "), MYNAMESTR);
			result = false;
		}

      DecryptPassword(info.username, info.password, info.password, MAX_PASSWORD);

      if (info.password[0] == '\0')
      {
         AgentWriteLog(EVENTLOG_ERROR_TYPE, _T("%s: error getting password for "), MYNAMESTR);
         result = false;
      }
		if (result)
		{
         g_dbInfo[g_dbCount].accessMutex = new Mutex();
		}
	}

	// Exit if no usable configuration found
	if (result && g_dbCount < 0)
	{
		AgentWriteLog(EVENTLOG_ERROR_TYPE, _T("%s: no databases to monitor"), MYNAMESTR);
		result = false;
	}

	// Run query thread for each database configured
	for (i = 0; result && i <= g_dbCount; i++)
	{
		g_dbInfo[i].queryThreadHandle = ThreadCreateEx(QueryThread, i);
	}

	return result;
}

/**
 * Shutdown handler
 */
static void SubAgentShutdown()
{
	AgentWriteLog(EVENTLOG_INFORMATION_TYPE, _T("%s: shutting down"), MYNAMESTR);
	g_shutdownCondition.set();
	for (int i = 0; i <= g_dbCount; i++)
	{
		ThreadJoin(g_dbInfo[i].queryThreadHandle);
		delete g_dbInfo[i].accessMutex;
	}
}

/**
 * Figure out Informix DBMS version
 */
static int getInformixVersion(DB_HANDLE handle)
{
	return 1100;
}

/**
 * Thread for SQL queries
 */
static void QueryThread(int dbIndex)
{
	DatabaseInfo& db = g_dbInfo[dbIndex];
	const DWORD pollInterval = 60 * 1000L;	// 1 minute
	int waitTimeout;
	int64_t startTimeMs;
	TCHAR errorText[DBDRV_MAX_ERROR_TEXT];

	while(true)
	{
		db.handle = DBConnect(g_driverHandle, db.server, db.dsn, db.username, db.password, NULL, errorText);
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
			if (g_shutdownCondition.wait(waitTimeout < 0 ? 1 : waitTimeout))
				goto finish;
		}

		// Try to reconnect every 30 secs
		if (g_shutdownCondition.wait(30000))
			break;
	}

finish:
	if (db.connected && db.handle != nullptr)
	{
		DBDisconnect(db.handle);
	}
}


bool getParametersFromDB( int dbIndex )
{
	bool ret = true;
	DatabaseInfo& info = g_dbInfo[dbIndex];

	if (!info.connected)
	{
		return false;
	}

	info.accessMutex->lock();

	for (int i = 0; g_paramGroup[i].prefix; i++)
	{
		AgentWriteDebugLog(7, _T("%s: got entry for '%s'"), MYNAMESTR, g_paramGroup[i].prefix);

		if (g_paramGroup[i].version > info.version)	// this parameter group is not supported for this DB
			continue;

		// Release previously allocated array of values for this group
		for (int j = 0; j < g_paramGroup[i].valueCount[dbIndex]; j++)
			delete (g_paramGroup[i].values[dbIndex])[j].attrs;
		MemFree((void*)g_paramGroup[i].values[dbIndex]);
      g_paramGroup[i].valueCount[dbIndex] = 0;

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

	info.accessMutex->unlock();

	return ret;
}

/**
 * Parameters
 */
static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
	{ _T("Informix.Session.Count"), H_DatabaseParameter, _T("S"), DCI_DT_INT, _T("Informix/Sessions: Number of sessions opened") },
	{ _T("Informix.Session.Count(*)"), H_DatabaseParameter, _T("M"), DCI_DT_INT, _T("Informix/Sessions: Number of sessions opened") },
	{ _T("Informix.Database.Owner"), H_DatabaseParameter, _T("S"), DCI_DT_STRING, _T("Informix/Databases: Database owner") },
	{ _T("Informix.Database.Owner(*)"), H_DatabaseParameter, _T("M"), DCI_DT_STRING, _T("Informix/Databases: Database owner") },
	{ _T("Informix.Database.Created"), H_DatabaseParameter, _T("S"), DCI_DT_STRING, _T("Informix/Databases: Creation date") },
	{ _T("Informix.Database.Created(*)"), H_DatabaseParameter, _T("M"), DCI_DT_STRING, _T("Informix/Databases: Creation date") },
	{ _T("Informix.Database.Logged"), H_DatabaseParameter, _T("S"), DCI_DT_INT, _T("Informix/Databases: Is logged") },
	{ _T("Informix.Database.Logged(*)"), H_DatabaseParameter, _T("M"), DCI_DT_INT, _T("Informix/Databases: Is logged") },
	{ _T("Informix.Dbspace.Pages.PageSize"), H_DatabaseParameter, _T("S"), DCI_DT_INT, _T("Informix/Dbspaces: Page size") },
	{ _T("Informix.Dbspace.Pages.PageSize(*)"), H_DatabaseParameter, _T("M"), DCI_DT_INT, _T("Informix/Dbspaces: Page size") },
	{ _T("Informix.Dbspace.Pages.Used"), H_DatabaseParameter, _T("S"), DCI_DT_INT, _T("Informix/Dbspaces: Number of pages used in dbspace") },
	{ _T("Informix.Dbspace.Pages.Used(*)"), H_DatabaseParameter, _T("M"), DCI_DT_INT, _T("Informix/Dbspaces: Number of pages used in dbspace") },
	{ _T("Informix.Dbspace.Pages.Free"), H_DatabaseParameter, _T("S"), DCI_DT_INT, _T("Informix/Dbspaces: Number of pages free in dbspace") },
	{ _T("Informix.Dbspace.Pages.Free(*)"), H_DatabaseParameter, _T("M"), DCI_DT_INT, _T("Informix/Dbspaces: Number of pages free in dbspace") },
	{ _T("Informix.Dbspace.Pages.FreePerc"), H_DatabaseParameter, _T("S"), DCI_DT_INT, _T("Informix/Dbspaces: Percentage of free space in dbspace") },
	{ _T("Informix.Dbspace.Pages.FreePerc(*)"), H_DatabaseParameter, _T("M"), DCI_DT_INT, _T("Informix/Dbspaces: Percentage of free space in dbspace") },
};

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO m_info =
{
	NETXMS_SUBAGENT_INFO_MAGIC,
	_T("INFORMIX"), NETXMS_VERSION_STRING,
	SubAgentInit, SubAgentShutdown, nullptr, nullptr, nullptr,
	sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM), m_parameters,
	0, nullptr,
   0, nullptr,
   0, nullptr,
   0, nullptr
};

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(INFORMIX)
{
	*ppInfo = &m_info;
	return true;
}

#ifdef _WIN32

/**
 * DLL entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
		DisableThreadLibraryCalls(hInstance);
	return TRUE;
}

#endif
