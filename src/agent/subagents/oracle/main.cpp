/*
 ** NetXMS - Network Management System
 ** Subagent for Oracle monitoring
 ** Copyright (C) 2009-2014 Raden Solutions
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
 **/

#include "oracle_subagent.h"

/**
 * Driver handle
 */
DB_DRIVER g_driverHandle = NULL;

/**
 * Polling queries
 */
DatabaseQuery g_queries[] = 
{
   { _T("DUAL"), MAKE_ORACLE_VERSION(7, 0), 0, _T("SELECT decode(count(*),1,0,1) ExcessRows FROM dual") },
   { _T("OBJECTS"), MAKE_ORACLE_VERSION(7, 0), 0, _T("SELECT count(*) InvalidCount FROM dba_objects WHERE status!='VALID'") },
   { _T("SESSIONS"), MAKE_ORACLE_VERSION(7, 0), 0, _T("SELECT count(*) Count from v$session") },
   { NULL, 0, NULL }
};

/**
 * Database instances
 */
static ObjectArray<DatabaseInstance> *s_instances = NULL;

/**
 * Find instance by ID
 */
static DatabaseInstance *FindInstance(const TCHAR *id)
{
   for(int i = 0; i < s_instances->size(); i++)
   {
      DatabaseInstance *db = s_instances->get(i);
      if (!_tcsicmp(db->getId(), id))
         return db;
   }
   return NULL;
}

/*
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
*/

/**
 * Handler for parameters without instance
 */
static LONG H_GlobalParameter(const TCHAR *param, const TCHAR *arg, TCHAR *value)
{
   TCHAR id[MAX_STR];
   if (!AgentGetParameterArg(param, 1, id, MAX_STR))
      return SYSINFO_RC_UNSUPPORTED;

   DatabaseInstance *db = FindInstance(id);
   if (db == NULL)
      return SYSINFO_RC_UNSUPPORTED;

   return db->getData(arg, value) ? SYSINFO_RC_SUCCESS : SYSINFO_RC_ERROR;
}

/**
 * Handler for Oracle.DBInfo.Version parameter
 */
static LONG H_DatabaseVersion(const TCHAR *param, const TCHAR *arg, TCHAR *value)
{
   TCHAR id[MAX_STR];
   if (!AgentGetParameterArg(param, 1, id, MAX_STR))
      return SYSINFO_RC_UNSUPPORTED;

   DatabaseInstance *db = FindInstance(id);
   if (db == NULL)
      return SYSINFO_RC_UNSUPPORTED;

   int version = db->getVersion();
   _sntprintf(value, MAX_RESULT_LENGTH, _T("%d.%d"), version >> 8, version & 0xFF);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for Oracle.DBInfo.IsReachable parameter
 */
static LONG H_DatabaseConnectionStatus(const TCHAR *param, const TCHAR *arg, TCHAR *value)
{
   TCHAR id[MAX_STR];
   if (!AgentGetParameterArg(param, 1, id, MAX_STR))
      return SYSINFO_RC_UNSUPPORTED;

   DatabaseInstance *db = FindInstance(id);
   if (db == NULL)
      return SYSINFO_RC_UNSUPPORTED;

   ret_string(value, db->isConnected() ? _T("YES") : _T("NO"));
   return SYSINFO_RC_SUCCESS;
}

/**
 * Config template
 */
static DatabaseInfo s_info;
static TCHAR s_dbPassEncrypted[MAX_DB_STRING] = _T("");
static NX_CFG_TEMPLATE s_configTemplate[] = 
{
	{ _T("Id"),					   CT_STRING, 0, 0, MAX_STR,       0, s_info.id },
	{ _T("Name"),				   CT_STRING, 0, 0, MAX_STR,       0, s_info.name },
	{ _T("TnsName"),			   CT_STRING, 0, 0, MAX_STR,       0, s_info.name },
	{ _T("UserName"),			   CT_STRING, 0, 0, MAX_USERNAME,  0, s_info.username },
	{ _T("Password"),			   CT_STRING, 0, 0, MAX_PASSWORD,  0, s_info.password },
   { _T("EncryptedPassword"), CT_STRING, 0, 0, MAX_DB_STRING, 0, s_dbPassEncrypted },
	{ _T(""), CT_END_OF_LIST, 0, 0, 0, 0, NULL }
};

/*
 * Subagent initialization
 */
static BOOL SubAgentInit(Config *config)
{
   int i;

	// Init db driver
	g_driverHandle = DBLoadDriver(_T("oracle.ddr"), NULL, TRUE, NULL, NULL);
	if (g_driverHandle == NULL)
	{
		AgentWriteLog(EVENTLOG_ERROR_TYPE, _T("%s: failed to load database driver"), MYNAMESTR);
		return FALSE;
	}

   s_instances = new ObjectArray<DatabaseInstance>(8, 8, true);

	// Load configuration from "oracle" section to allow simple configuration
	// of one database without XML includes
	memset(&s_info, 0, sizeof(s_info));
	if (config->parseTemplate(_T("ORACLE"), s_configTemplate))
	{
		if (s_info.name[0] != 0)
		{
			if (s_info.id[0] == 0)
				_tcscpy(s_info.id, s_info.name);
         if (*s_dbPassEncrypted != 0)
         {
            DecryptPassword(s_info.username, s_dbPassEncrypted, s_info.password);
         }
         s_instances->add(new DatabaseInstance(&s_info));
		}
	}

	// Load full-featured XML configuration
	for(i = 1; i <= 64; i++)
	{
		TCHAR section[MAX_STR];
		memset((void*)&s_info, 0, sizeof(s_info));
		_sntprintf(section, MAX_STR, _T("oracle/databases/database#%d"), i);
		s_dbPassEncrypted[0] = 0;

		if (!config->parseTemplate(section, s_configTemplate))
		{
			AgentWriteLog(NXLOG_WARNING, _T("ORACLE: error parsing configuration template %d"), i);
         continue;
		}

		if (s_info.name[0] == 0)
			continue;

      if (*s_dbPassEncrypted != 0)
      {
         DecryptPassword(s_info.username, s_dbPassEncrypted, s_info.password);
      }

      s_instances->add(new DatabaseInstance(&s_info));
	}

	// Exit if no usable configuration found
   if (s_instances->size() == 0)
	{
      AgentWriteLog(NXLOG_WARNING, _T("ORACLE: no databases to monitor, exiting"));
      delete s_instances;
      return FALSE;
	}

	// Run query thread for each configured database
   for(i = 0; i < s_instances->size(); i++)
      s_instances->get(i)->run();

	return TRUE;
}

/**
 * Shutdown handler
 */
static void SubAgentShutdown()
{
	AgentWriteDebugLog(1, _T("ORACLE: stopping pollers"));
   for(int i = 0; i < s_instances->size(); i++)
      s_instances->get(i)->stop();
   delete s_instances;
	AgentWriteDebugLog(1, _T("ORACLE: stopped"));
}

/**
 * Supported parameters
 */
static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
	{ _T("Oracle.Sessions.Count(*)"), H_GlobalParameter, _T("SESSIONS/COUNT"), DCI_DT_INT, _T("Oracle/Sessions: Number of sessions opened") },
	{ _T("Oracle.Cursors.Count(*)"), H_GlobalParameter, _T("X"), DCI_DT_INT, _T("Oracle/Cursors: Current number of opened cursors systemwide") },
	{ _T("Oracle.DBInfo.IsReachable(*)"), H_DatabaseConnectionStatus, NULL, DCI_DT_STRING, _T("Oracle/Info: Database is reachable") },
	{ _T("Oracle.DBInfo.Name(*)"), H_GlobalParameter, _T("X"), DCI_DT_STRING, _T("Oracle/Info: Database name") },
	{ _T("Oracle.DBInfo.CreateDate(*)"), H_GlobalParameter, _T("X"), DCI_DT_STRING, _T("Oracle/Info: Database creation date") },
	{ _T("Oracle.DBInfo.LogMode(*)"), H_GlobalParameter, _T("X"), DCI_DT_STRING, _T("Oracle/Info: Database log mode") },
	{ _T("Oracle.DBInfo.OpenMode(*)"), H_GlobalParameter, _T("X"), DCI_DT_STRING, _T("Oracle/Info: Database open mode") },
	{ _T("Oracle.DBInfo.Version(*)"), H_DatabaseVersion, NULL, DCI_DT_STRING, _T("Oracle/Info: Database version") },
	{ _T("Oracle.TableSpaces.Status(*)"), H_GlobalParameter, _T("X"), DCI_DT_STRING, _T("Oracle/Tablespaces: Status") },
	{ _T("Oracle.TableSpaces.Type(*)"), H_GlobalParameter, _T("X"), DCI_DT_STRING, _T("Oracle/Tablespaces: Type") },
	{ _T("Oracle.TableSpaces.UsedPct(*)"), H_GlobalParameter, _T("X"), DCI_DT_INT, _T("Oracle/Tablespaces: Percentage used") },
	{ _T("Oracle.Instance.Version(*)"), H_GlobalParameter, _T("X"), DCI_DT_STRING, _T("Oracle/Instance: DBMS Version") },
	{ _T("Oracle.Instance.Status(*)"), H_GlobalParameter, _T("X"), DCI_DT_STRING, _T("Oracle/Instance: Status") },
	{ _T("Oracle.Instance.ArchiverStatus(*)"), H_GlobalParameter, _T("X"), DCI_DT_STRING, _T("Oracle/Instance: Archiver status") },
	{ _T("Oracle.Instance.ShutdownPending(*)"), H_GlobalParameter, _T("X"), DCI_DT_STRING, _T("Oracle/Instance: Is shutdown pending") },
	{ _T("Oracle.CriticalStats.TSOffCount(*)"), H_GlobalParameter, _T("X"), DCI_DT_INT, _T("Oracle/CriticalStats: Number of offline tablespaces") },
	{ _T("Oracle.CriticalStats.DFOffCount(*)"), H_GlobalParameter, _T("X"), DCI_DT_INT, _T("Oracle/CriticalStats: Number of offline datafiles") },
	{ _T("Oracle.CriticalStats.FullSegmentsCount(*)"), H_GlobalParameter, _T("X"), DCI_DT_INT64, _T("Oracle/CriticalStats: Number of segments that cannot extend") },
	{ _T("Oracle.CriticalStats.RBSegsNotOnlineCount(*)"), H_GlobalParameter, _T("X"), DCI_DT_INT64, _T("Oracle/CriticalStats: Number of rollback segments not online") },
	{ _T("Oracle.CriticalStats.AutoArchivingOff(*)"), H_GlobalParameter, _T("X"), DCI_DT_STRING, _T("Oracle/CriticalStats: Archive logs enabled but auto archiving off ") },
	{ _T("Oracle.CriticalStats.DatafilesNeedMediaRecovery(*)"), H_GlobalParameter, _T("X"), DCI_DT_INT64, _T("Oracle/CriticalStats: Number of datafiles that need media recovery") },
	{ _T("Oracle.CriticalStats.FailedJobs(*)"), H_GlobalParameter, _T("X"), DCI_DT_INT64, _T("Oracle/CriticalStats: Number of failed jobs") },
	{ _T("Oracle.Dual.ExcessRows(*)"), H_GlobalParameter, _T("DUAL/EXCESSROWS"), DCI_DT_INT64, _T("Oracle/Dual: Excessive rows") },
	{ _T("Oracle.Performance.PhysReads(*)"),  H_GlobalParameter, _T("X"), DCI_DT_INT64, _T("Oracle/Performance: Number of physical reads") },
	{ _T("Oracle.Performance.LogicReads(*)"), H_GlobalParameter, _T("X"), DCI_DT_INT64, _T("Oracle/Performance: Number of logical reads") },
	{ _T("Oracle.Performance.CacheHitRatio(*)"), H_GlobalParameter, _T("X"), DCI_DT_STRING, _T("Oracle/Performance: Data buffer cache hit ratio") },
	{ _T("Oracle.Performance.LibCacheHitRatio(*)"), H_GlobalParameter, _T("X"), DCI_DT_STRING, _T("Oracle/Performance: Library cache hit ratio") },
	{ _T("Oracle.Performance.DictCacheHitRatio(*)"), H_GlobalParameter, _T("X"), DCI_DT_STRING, _T("Oracle/Performance: Dictionary cache hit ratio") },
	{ _T("Oracle.Performance.RollbackWaitRatio(*)"), H_GlobalParameter, _T("X"), DCI_DT_STRING, _T("Oracle/Performance: Ratio of waits for requests to rollback segments") },
	{ _T("Oracle.Performance.MemorySortRatio(*)"), H_GlobalParameter, _T("X"), DCI_DT_STRING, _T("Oracle/Performance: PGA memory sort ratio") },
	{ _T("Oracle.Performance.DispatcherWorkload(*)"), H_GlobalParameter, _T("X"), DCI_DT_STRING, _T("Oracle/Performance: Dispatcher workload (percentage)") },
	{ _T("Oracle.Performance.FreeSharedPool(*)"), H_GlobalParameter, _T("X"), DCI_DT_INT64, _T("Oracle/Performance: Free space in shared pool (bytes)") },
	{ _T("Oracle.Objects.InvalidCount(*)"), H_GlobalParameter, _T("OBJECTS/INVALIDCOUNT"), DCI_DT_INT64, _T("Oracle/Objects: Number of invalid objects in DB") }
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

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(ORACLE)
{
	*ppInfo = &m_info;
	return TRUE;
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
