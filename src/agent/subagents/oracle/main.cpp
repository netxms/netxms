/*
** NetXMS subagent for Oracle monitoring
** Copyright (C) 2009-2012 Raden Solutions
**/

#include "oracle_subagent.h"

CONDITION g_shutdownCondition;
MUTEX g_paramAccessMutex;
int g_dbCount;
DB_DRIVER g_driverHandle = NULL;
DatabaseInfo g_dbInfo[MAX_DATABASES];
DatabaseData g_dbData[MAX_DATABASES];

THREAD_RESULT THREAD_CALL queryThread(void *arg);

DBParameterGroup g_paramGroup[] = {
	{
		700, _T("Oracle.Sessions."),
		_T("select ") DB_NULLARG_MAGIC _T(" ValueName, count(*) Count from v$session"),
		2, { NULL }, 0
	},
	{
		700, _T("Oracle.Cursors."),
		_T("select ") DB_NULLARG_MAGIC _T(" ValueName, sum(a.value) Count from v$sesstat a, v$statname b, v$session s where a.statistic# = b.statistic#  and s.sid=a.sid and b.name = 'opened cursors current'"),
		2, { NULL }, 0
	},
	{
		700, _T("Oracle.Objects."),
			_T("select ") DB_NULLARG_MAGIC _T(" ValueName, count(*) InvalidCount from dba_objects where status!='VALID'"),
			2, { NULL }, 0
	},
	{
		700, _T("Oracle.DBInfo."), 
		_T("select ") DB_NULLARG_MAGIC _T(" ValueName, name Name, to_char(created) CreateDate, log_mode LogMode, open_mode OpenMode from v$database"),
		5, { NULL }, 0
	},
	{
		700, _T("Oracle.Instance."),
		_T("select ") DB_NULLARG_MAGIC _T(" ValueName, version Version, status Status, archiver ArchiverStatus, shutdown_pending ShutdownPending from v$instance"),
		5, { NULL }, 0
	},
	{
		1000, _T("Oracle.TableSpaces."),
		_T("select d.tablespace_name ValueName, d.status Status, d.contents Type, to_char(round(used_percent,2)) UsedPct from dba_tablespaces d, dba_tablespace_usage_metrics m where d.tablespace_name=m.tablespace_name"),
		3, { NULL }, 0
	},
	{
		700, _T("Oracle.Dual."),
		_T("select ") DB_NULLARG_MAGIC _T(" ValueName, decode(count(*),1,0,1) ExcessRows from dual"),
		1, { NULL }, 0
	},
	{
		700, _T("Oracle.Performance."),
		_T("select ") DB_NULLARG_MAGIC _T(" ValueName, (select s.value PhysReads from v$sysstat s, v$statname n where n.name='physical reads' and n.statistic#=s.statistic#) PhysReads, ")
		_T("(select s.value LogicReads from v$sysstat s, v$statname n where n.name='session logical reads' and n.statistic#=s.statistic#) LogicReads, ")
		_T("(select round((sum(decode(name,'consistent gets',value,0))+sum(decode(name,'db block gets',value,0))-sum(decode(name,'physical reads',value, 0)))/(sum(decode(name,'consistent gets',value,0))+sum(decode(name,'db block gets',value,0)))*100,2) from v$sysstat) CacheHitRatio, ")
		_T("(select round(sum(waits)*100/sum(gets),2) from v$rollstat) RollbackWaitRatio, ")
		_T("(select round((1-(sum(getmisses)/sum(gets)))*100,2) from v$rowcache) DictCacheHitRatio, ")
		_T("(select round(sum(pins)/(sum(pins)+sum(reloads))*100,2) from v$librarycache) LibCacheHitRatio, ")
		_T("(select round((100*b.value)/decode((a.value+b.value),0,1,(a.value+b.value)),2) from v$sysstat a,v$sysstat b where a.name='sorts (disk)' and b.name='sorts (memory)') MemorySortRatio, ")
		_T("(select round(nvl((sum(busy)/(sum(busy)+sum(idle)))*100,0),2) from v$dispatcher) DispatcherWorkload, ")
		_T("(select bytes	from v$sgastat where name='free memory' and pool='shared pool') FreeSharedPool ")
		_T("from DUAL "),
		10, { NULL }, 0
	},
	{
		700, _T("Oracle.CriticalStats."), 
		_T("select ") DB_NULLARG_MAGIC _T(" ValueName, (select count(*) TSOFF from dba_tablespaces where status <> 'ONLINE') TSOffCount, ")
		_T("(select count(*) DFOFF from V$DATAFILE where status not in ('ONLINE','SYSTEM')) DFOffCount, ")
		_T("(select count(*) from dba_segments where max_extents = extents) FullSegmentsCount, ")
		_T("(select count(*) from dba_rollback_segs where status <> 'ONLINE') RBSegsNotOnlineCount, ")
		_T("decode(sign(decode((select upper(log_mode) from v$database),'ARCHIVELOG',1,0)-")
		_T("decode((select upper(value) from v$parameter where upper(name)='LOG_ARCHIVE_START'),'TRUE',1,0)),1, 1, 0) AutoArchivingOff, ")
		_T("(select count(file#) from v$datafile_header where recover ='YES') DatafilesNeedMediaRecovery, ")
		_T("(select count(*) FROM dba_jobs where NVL(failures,0) <> 0) FailedJobs ")
		_T("from DUAL"),		
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

	AgentWriteDebugLog(7, _T("%s: got request for params: dbid='%s', param='%s'"), MYNAMESTR, dbId, parameter);

	// Loop through databases and find an entry in g_dbInfo[] for this id
	for (int i = 0; i <= g_dbCount; i++)
	{
		if (!_tcsnicmp(g_dbInfo[i].id, dbId, MAX_STR))	// found DB
		{
			if (argument[0] == _T('R'))
			{
				ret_string(value, g_dbInfo[i].connected ? _T("YES") : _T("NO"));
				ret = SYSINFO_RC_SUCCESS;
			}
			// Loop through parameter groups and check whose prefix matches the parameter requested
			for (int k = 0; argument[0] == _T('X') && g_paramGroup[k].prefix; k++)
			{
				if (!_tcsnicmp(g_paramGroup[k].prefix, parameter, _tcslen(g_paramGroup[k].prefix))) // found prefix
				{
					MutexLock(g_dbInfo[i].accessMutex);
					// Loop through the values
					AgentWriteDebugLog(9, _T("%s: valuecount %d"), MYNAMESTR, g_paramGroup[k].valueCount[i]);
					for (int j = 0; j < g_paramGroup[k].valueCount[i]; j++)
					{
						StringMap* map = (g_paramGroup[k].values[i])[j].attrs;
						TCHAR* name = (g_paramGroup[k].values[i])[j].name;
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
		{ _T("Name"),				CT_STRING,	0, 0, MAX_STR, 0,	info.name },	
		{ _T("TnsName"),			CT_STRING,	0, 0, MAX_STR, 0,	info.name },	
		{ _T("UserName"),			CT_STRING,	0, 0, MAX_USERNAME, 0,	info.username },	
		{ _T("Password"),			CT_STRING,	0, 0, MAX_PASSWORD, 0,	info.password },
		{ _T(""), CT_END_OF_LIST, 0, 0, 0, 0, NULL }
	};

	// Init db driver
#ifdef _WIN32
	g_driverHandle = DBLoadDriver(_T("oracle.ddr"), NULL, TRUE, NULL, NULL);
#else
	g_driverHandle = DBLoadDriver(LIBDIR "/libnxddr_oracle.so", NULL, TRUE, NULL, NULL);
#endif
	if (g_driverHandle == NULL)
	{
		AgentWriteLog(EVENTLOG_ERROR_TYPE, _T("%s: failed to load db driver"), MYNAMESTR);
		result = FALSE;
	}

	if (result)
	{
		g_shutdownCondition = ConditionCreate(TRUE);
	}

	// Load configuration from "oracle" section to allow simple configuration
	// of one database without XML includes
	memset(&info, 0, sizeof(info));
	g_dbCount = -1;
	if (config->parseTemplate(_T("ORACLE"), configTemplate))
	{
		if (info.name[0] != 0)
		{
			if (info.id[0] == 0)
				_tcscpy(info.id, info.name);
			memcpy(&g_dbInfo[++g_dbCount], &info, sizeof(DatabaseInfo));
			g_dbInfo[g_dbCount].accessMutex = MutexCreate();
		}
	}

	// Load full-featured XML configuration
	if (g_dbCount == -1) // Didn't load anything from the .conf file
	{
		for (i = 1; result && i <= MAX_DATABASES; i++)
		{
			TCHAR section[MAX_STR];
			memset((void*)&info, 0, sizeof(info));
			_sntprintf(section, MAX_STR, _T("oracle/databases/database#%d"), i);
			if ((result = config->parseTemplate(section, configTemplate)) != TRUE)
			{
				AgentWriteLog(EVENTLOG_ERROR_TYPE, _T("%s: error parsing configuration template"), MYNAMESTR);
				return FALSE;
			}
			if (info.name[0] != _T('\0'))
				memcpy((void*)&g_dbInfo[++g_dbCount], (void*)&info, sizeof(info));
			else
				continue;
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

static void SubAgentShutdown(void)
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
// Figure out Oracle DBMS version
//

static int getOracleVersion(DB_HANDLE handle) 
{
	TCHAR versionString[32];

	DB_RESULT result = DBSelect(handle,_T("select version from v$instance"));
	if (result == NULL)	
	{
		AgentWriteLog(EVENTLOG_WARNING_TYPE, _T("%s: query from v$instance failed"), MYNAMESTR);
		return 700;		// assume Oracle 7.0 by default
	}

	DBGetField(result, 0, 0, versionString, 32);
	int major = 0, minor = 0;
	_stscanf(versionString, _T("%d.%d"), &major, &minor);
	DBFreeResult(result);

	return major * 100 + minor * 10;
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
		db.handle = DBConnect(g_driverHandle, db.name, NULL /* db.server */, db.username, db.password, NULL, errorText);
		DBEnableReconnect(db.handle, false);
		if (db.handle != NULL)
		{
			AgentWriteLog(EVENTLOG_INFORMATION_TYPE, _T("%s: connected to DB '%s'"), MYNAMESTR, db.name);
			db.connected = true;
			db.version = getOracleVersion(db.handle);
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

	MutexLock(info.accessMutex);

	for (int i = 0; g_paramGroup[i].prefix; i++)
	{
		AgentWriteDebugLog(7, _T("%s: got entry for '%s'"), MYNAMESTR, g_paramGroup[i].prefix);

		if (g_paramGroup[i].version > info.version)	// this parameter group is not supported for this DB
			continue; 

		// Release previously allocated array of values for this group
		for (int j = 0; g_paramGroup[i].values[dbIndex] && j < g_paramGroup[i].valueCount[dbIndex]; j++)
			delete (g_paramGroup[i].values[dbIndex])[j].attrs;
		safe_free_and_null(g_paramGroup[i].values[dbIndex]);

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
	{ _T("Oracle.Sessions.Count(*)"), getParameters, _T("X"), DCI_DT_INT, _T("Oracle/Sessions: Number of sessions opened") },
	{ _T("Oracle.Cursors.Count(*)"), getParameters, _T("X"), DCI_DT_INT, _T("Oracle/Cursors: Current number of opened cursors systemwide") },
	{ _T("Oracle.DBInfo.IsReachable(*)"), getParameters, _T("R"), DCI_DT_STRING, _T("Oracle/Info: Database is reachable") },
	{ _T("Oracle.DBInfo.Name(*)"), getParameters, _T("X"), DCI_DT_STRING, _T("Oracle/Info: Database name") },
	{ _T("Oracle.DBInfo.CreateDate(*)"), getParameters, _T("X"), DCI_DT_STRING, _T("Oracle/Info: Database creation date") },
	{ _T("Oracle.DBInfo.LogMode(*)"), getParameters, _T("X"), DCI_DT_STRING, _T("Oracle/Info: Database log mode") },
	{ _T("Oracle.DBInfo.OpenMode(*)"), getParameters, _T("X"), DCI_DT_STRING, _T("Oracle/Info: Database open mode") },
	{ _T("Oracle.TableSpaces.Status(*)"), getParameters, _T("X"), DCI_DT_STRING, _T("Oracle/Tablespaces: Status") },
	{ _T("Oracle.TableSpaces.Type(*)"), getParameters, _T("X"), DCI_DT_STRING, _T("Oracle/Tablespaces: Type") },
	{ _T("Oracle.TableSpaces.UsedPct(*)"), getParameters, _T("X"), DCI_DT_INT, _T("Oracle/Tablespaces: Percentage used") },
	{ _T("Oracle.Instance.Version(*)"), getParameters, _T("X"), DCI_DT_STRING, _T("Oracle/Instance: DBMS Version") },
	{ _T("Oracle.Instance.Status(*)"), getParameters, _T("X"), DCI_DT_STRING, _T("Oracle/Instance: Status") },
	{ _T("Oracle.Instance.ArchiverStatus(*)"), getParameters, _T("X"), DCI_DT_STRING, _T("Oracle/Instance: Archiver status") },
	{ _T("Oracle.Instance.ShutdownPending(*)"), getParameters, _T("X"), DCI_DT_STRING, _T("Oracle/Instance: Is shutdown pending") },
	{ _T("Oracle.CriticalStats.TSOffCount(*)"), getParameters, _T("X"), DCI_DT_INT, _T("Oracle/CriticalStats: Number of offline tablespaces") },
	{ _T("Oracle.CriticalStats.DFOffCount(*)"), getParameters, _T("X"), DCI_DT_INT, _T("Oracle/CriticalStats: Number of offline datafiles") },
	{ _T("Oracle.CriticalStats.FullSegmentsCount(*)"), getParameters, _T("X"), DCI_DT_INT64, _T("Oracle/CriticalStats: Number of segments that cannot extend") },
	{ _T("Oracle.CriticalStats.RBSegsNotOnlineCount(*)"), getParameters, _T("X"), DCI_DT_INT64, _T("Oracle/CriticalStats: Number of rollback segments not online") },
	{ _T("Oracle.CriticalStats.AutoArchivingOff(*)"), getParameters, _T("X"), DCI_DT_STRING, _T("Oracle/CriticalStats: Archive logs enabled but auto archiving off ") },
	{ _T("Oracle.CriticalStats.DatafilesNeedMediaRecovery(*)"), getParameters, _T("X"), DCI_DT_INT64, _T("Oracle/CriticalStats: Number of datafiles that need media recovery") },
	{ _T("Oracle.CriticalStats.FailedJobs(*)"), getParameters, _T("X"), DCI_DT_INT64, _T("Oracle/CriticalStats: Number of failed jobs") },
	{ _T("Oracle.Dual.ExcessRows(*)"), getParameters, _T("X"), DCI_DT_INT64, _T("Oracle/Dual: Excessive rows") },
	{ _T("Oracle.Performance.PhysReads(*)"),  getParameters, _T("X"), DCI_DT_INT64, _T("Oracle/Performance: Number of physical reads") },
	{ _T("Oracle.Performance.LogicReads(*)"), getParameters, _T("X"), DCI_DT_INT64, _T("Oracle/Performance: Number of logical reads") },
	{ _T("Oracle.Performance.CacheHitRatio(*)"), getParameters, _T("X"), DCI_DT_STRING, _T("Oracle/Performance: Data buffer cache hit ratio") },
	{ _T("Oracle.Performance.LibCacheHitRatio(*)"), getParameters, _T("X"), DCI_DT_STRING, _T("Oracle/Performance: Library cache hit ratio") },
	{ _T("Oracle.Performance.DictCacheHitRatio(*)"), getParameters, _T("X"), DCI_DT_STRING, _T("Oracle/Performance: Dictionary cache hit ratio") },
	{ _T("Oracle.Performance.RollbackWaitRatio(*)"), getParameters, _T("X"), DCI_DT_STRING, _T("Oracle/Performance: Ratio of waits for requests to rollback segments") },
	{ _T("Oracle.Performance.MemorySortRatio(*)"), getParameters, _T("X"), DCI_DT_STRING, _T("Oracle/Performance: PGA memory sort ratio") },
	{ _T("Oracle.Performance.DispatcherWorkload(*)"), getParameters, _T("X"), DCI_DT_STRING, _T("Oracle/Performance: Dispatcher workload (percentage)") },
	{ _T("Oracle.Performance.FreeSharedPool(*)"), getParameters, _T("X"), DCI_DT_INT64, _T("Oracle/Performance: Free space in shared pool (bytes)") },
	{ _T("Oracle.Objects.InvalidCount(*)"), getParameters, _T("X"), DCI_DT_INT64, _T("Oracle/Objects: Number of invalid objects in DB") }
};

/*
static NETXMS_SUBAGENT_ENUM m_enums[] =
{
};
*/

static NETXMS_SUBAGENT_INFO m_info =
{
	NETXMS_SUBAGENT_INFO_MAGIC,
	_T("ORACLE"), NETXMS_VERSION_STRING,
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

DECLARE_SUBAGENT_ENTRY_POINT(ORACLE)
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
