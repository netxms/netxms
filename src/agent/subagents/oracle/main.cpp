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
   { _T("DBINFO"), MAKE_ORACLE_VERSION(7, 0), 0, _T("SELECT name, to_char(created) CreateDate, log_mode, open_mode FROM v$database") },
   { _T("DUAL"), MAKE_ORACLE_VERSION(7, 0), 0, _T("SELECT decode(count(*),1,0,1) ExcessRows FROM dual") },
   { _T("INSTANCE"), MAKE_ORACLE_VERSION(7, 0), 0, _T("SELECT version,status,archiver,shutdown_pending FROM v$instance") },
   { _T("GLOBALSTATS"), MAKE_ORACLE_VERSION(7, 0), 0,
         _T("SELECT ")
            _T("(SELECT s.value PhysReads FROM v$sysstat s, v$statname n WHERE n.name='physical reads' AND n.statistic#=s.statistic#) PhysReads, ")
            _T("(SELECT s.value LogicReads FROM v$sysstat s, v$statname n WHERE n.name='session logical reads' AND n.statistic#=s.statistic#) LogicReads, ")
            _T("(SELECT round((sum(decode(name,'consistent gets',value,0))+sum(decode(name,'db block gets',value,0))-sum(decode(name,'physical reads',value, 0)))/(sum(decode(name,'consistent gets',value,0))+sum(decode(name,'db block gets',value,0)))*100,2) FROM v$sysstat) CacheHitRatio, ")
            _T("(SELECT round(sum(waits)*100/sum(gets),2) FROM v$rollstat) RollbackWaitRatio, ")
            _T("(SELECT round((1-(sum(getmisses)/sum(gets)))*100,2) FROM v$rowcache) DictCacheHitRatio, ")
            _T("(SELECT round(sum(pins)/(sum(pins)+sum(reloads))*100,2) FROM v$librarycache) LibCacheHitRatio, ")
            _T("(SELECT round((100*b.value)/decode((a.value+b.value),0,1,(a.value+b.value)),2) FROM v$sysstat a,v$sysstat b WHERE a.name='sorts (disk)' AND b.name='sorts (memory)') MemorySortRatio, ")
            _T("(SELECT round(nvl((sum(busy)/(sum(busy)+sum(idle)))*100,0),2) FROM v$dispatcher) DispatcherWorkload, ")
            _T("(SELECT bytes	FROM v$sgastat WHERE name='free memory' AND pool='shared pool') FreeSharedPool, ")
            _T("(SELECT count(*) FROM dba_tablespaces WHERE status <> 'ONLINE') TSOffCount, ")
            _T("(SELECT count(*) FROM v$datafile WHERE status NOT IN ('ONLINE','SYSTEM')) DFOffCount, ")
            _T("(SELECT count(*) FROM dba_segments WHERE max_extents = extents) FullSegCnt, ")
            _T("(SELECT count(*) FROM dba_rollback_segs WHERE status <> 'ONLINE') RBSNotOnline,")
            _T("decode(sign(decode((SELECT upper(log_mode) FROM v$database),'ARCHIVELOG',1,0)-")
            _T("   decode((SELECT upper(value) FROM v$parameter WHERE upper(name)='LOG_ARCHIVE_START'),'TRUE',1,0)),1, 1, 0) AAOff,")
            _T("(SELECT count(file#) FROM v$datafile_header WHERE recover ='YES') DFNeedRec,")
            _T("(SELECT count(*) FROM dba_jobs WHERE nvl(failures,0) <> 0) FailedJobs,")
      		_T("(SELECT sum(a.value) FROM v$sesstat a, v$statname b, v$session s WHERE a.statistic#=b.statistic# AND s.sid=a.sid AND b.name='opened cursors current') OpenCursors,")
            _T("(SELECT count(*) FROM dba_objects WHERE status!='VALID') InvalidObjects,")
            _T("(SELECT count(*) FROM v$lock) Locks,")
            _T("(SELECT count(*) FROM v$session) SessionCount ")
         _T("FROM dual")
   },
   { _T("TABLESPACE"), MAKE_ORACLE_VERSION(10, 0), 1, 
      _T("SELECT d.tablespace_name, d.status Status, d.contents Type, to_char(round(used_percent,2)) UsedPct FROM dba_tablespaces d, dba_tablespace_usage_metrics m WHERE d.tablespace_name=m.tablespace_name")
   },
   { NULL, 0, 0, NULL }
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
 * Handler for parameters with instance
 */
static LONG H_InstanceParameter(const TCHAR *param, const TCHAR *arg, TCHAR *value)
{
   TCHAR id[MAX_STR];
   if (!AgentGetParameterArg(param, 1, id, MAX_STR))
      return SYSINFO_RC_UNSUPPORTED;

   DatabaseInstance *db = FindInstance(id);
   if (db == NULL)
      return SYSINFO_RC_UNSUPPORTED;

   TCHAR instance[MAX_STR];
   if (!AgentGetParameterArg(param, 2, instance, MAX_STR))
      return SYSINFO_RC_UNSUPPORTED;

   TCHAR tag[MAX_STR];
   _sntprintf(tag, MAX_STR, _T("%s@%s"), arg, instance);
   return db->getData(tag, value) ? SYSINFO_RC_SUCCESS : SYSINFO_RC_ERROR;
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
 * Handler for generic list parameter
 */
static LONG H_TagList(const TCHAR *param, const TCHAR *arg, StringList *value)
{
   TCHAR id[MAX_STR];
   if (!AgentGetParameterArg(param, 1, id, MAX_STR))
      return SYSINFO_RC_UNSUPPORTED;

   DatabaseInstance *db = FindInstance(id);
   if (db == NULL)
      return SYSINFO_RC_UNSUPPORTED;

   return db->getTagList(arg, value) ? SYSINFO_RC_SUCCESS : SYSINFO_RC_ERROR;
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
	{ _T("Oracle.CriticalStats.AutoArchivingOff(*)"), H_GlobalParameter, _T("GLOBALSTATS/AAOFF"), DCI_DT_STRING, _T("Oracle/CriticalStats: Archive logs enabled but auto archiving off ") },
	{ _T("Oracle.CriticalStats.DatafilesNeedMediaRecovery(*)"), H_GlobalParameter, _T("GLOBALSTATS/DFNEEDREC"), DCI_DT_INT64, _T("Oracle/CriticalStats: Number of datafiles that need media recovery") },
	{ _T("Oracle.CriticalStats.DFOffCount(*)"), H_GlobalParameter, _T("GLOBALSTATS/DFOFFCOUNT"), DCI_DT_INT, _T("Oracle/CriticalStats: Number of offline datafiles") },
	{ _T("Oracle.CriticalStats.FailedJobs(*)"), H_GlobalParameter, _T("GLOBALSTATS/FAILEDJOBS"), DCI_DT_INT64, _T("Oracle/CriticalStats: Number of failed jobs") },
	{ _T("Oracle.CriticalStats.FullSegmentsCount(*)"), H_GlobalParameter, _T("GLOBALSTATS/FULLSEGCNT"), DCI_DT_INT64, _T("Oracle/CriticalStats: Number of segments that cannot extend") },
	{ _T("Oracle.CriticalStats.RBSegsNotOnlineCount(*)"), H_GlobalParameter, _T("GLOBALSTATS/RBSNOTONLINE"), DCI_DT_INT64, _T("Oracle/CriticalStats: Number of rollback segments not online") },
	{ _T("Oracle.CriticalStats.TSOffCount(*)"), H_GlobalParameter, _T("GLOBALSTATS/TSOFFCOUNT"), DCI_DT_INT, _T("Oracle/CriticalStats: Number of offline tablespaces") },
	{ _T("Oracle.Cursors.Count(*)"), H_GlobalParameter, _T("GLOBALSTATS/OPENCURSORS"), DCI_DT_INT, _T("Oracle/Cursors: Current number of opened cursors systemwide") },
	{ _T("Oracle.DBInfo.CreateDate(*)"), H_GlobalParameter, _T("DBINFO/CREATEDATE"), DCI_DT_STRING, _T("Oracle/Info: Database creation date") },
	{ _T("Oracle.DBInfo.IsReachable(*)"), H_DatabaseConnectionStatus, NULL, DCI_DT_STRING, _T("Oracle/Info: Database is reachable") },
	{ _T("Oracle.DBInfo.LogMode(*)"), H_GlobalParameter, _T("DBINFO/LOG_MODE"), DCI_DT_STRING, _T("Oracle/Info: Database log mode") },
	{ _T("Oracle.DBInfo.Name(*)"), H_GlobalParameter, _T("DBINFO/NAME"), DCI_DT_STRING, _T("Oracle/Info: Database name") },
	{ _T("Oracle.DBInfo.OpenMode(*)"), H_GlobalParameter, _T("DBINFO/OPEN_MODE"), DCI_DT_STRING, _T("Oracle/Info: Database open mode") },
	{ _T("Oracle.DBInfo.Version(*)"), H_DatabaseVersion, NULL, DCI_DT_STRING, _T("Oracle/Info: Database version") },
	{ _T("Oracle.Dual.ExcessRows(*)"), H_GlobalParameter, _T("DUAL/EXCESSROWS"), DCI_DT_INT64, _T("Oracle/Dual: Excessive rows") },
	{ _T("Oracle.Instance.ArchiverStatus(*)"), H_GlobalParameter, _T("INSTANCE/ARCHIVER"), DCI_DT_STRING, _T("Oracle/Instance: Archiver status") },
	{ _T("Oracle.Instance.Status(*)"), H_GlobalParameter, _T("INSTANCE/STATUS"), DCI_DT_STRING, _T("Oracle/Instance: Status") },
	{ _T("Oracle.Instance.ShutdownPending(*)"), H_GlobalParameter, _T("INSTANCE/SHUTDOWN_PENDING"), DCI_DT_STRING, _T("Oracle/Instance: Is shutdown pending") },
	{ _T("Oracle.Instance.Version(*)"), H_GlobalParameter, _T("INSTANCE/VERSION"), DCI_DT_STRING, _T("Oracle/Instance: DBMS Version") },
	{ _T("Oracle.Objects.InvalidCount(*)"), H_GlobalParameter, _T("GLOBALSTATS/INVALIDOBJECTS"), DCI_DT_INT64, _T("Oracle/Objects: Number of invalid objects in DB") },
	{ _T("Oracle.Performance.CacheHitRatio(*)"), H_GlobalParameter, _T("GLOBALSTATS/CACHEHITRATIO"), DCI_DT_STRING, _T("Oracle/Performance: Data buffer cache hit ratio") },
	{ _T("Oracle.Performance.DictCacheHitRatio(*)"), H_GlobalParameter, _T("GLOBALSTATS/DICTCACHEHITRATIO"), DCI_DT_STRING, _T("Oracle/Performance: Dictionary cache hit ratio") },
	{ _T("Oracle.Performance.DispatcherWorkload(*)"), H_GlobalParameter, _T("GLOBALSTATS/DISPATCHERWORKLOAD"), DCI_DT_STRING, _T("Oracle/Performance: Dispatcher workload (percentage)") },
	{ _T("Oracle.Performance.FreeSharedPool(*)"), H_GlobalParameter, _T("GLOBALSTATS/FREESHAREDPOOL"), DCI_DT_INT64, _T("Oracle/Performance: Free space in shared pool (bytes)") },
	{ _T("Oracle.Performance.Locks(*)"), H_GlobalParameter, _T("GLOBALSTATS/LOCKS"), DCI_DT_INT64, _T("Oracle/Performance: Number of locks") },
	{ _T("Oracle.Performance.LogicalReads(*)"), H_GlobalParameter, _T("GLOBALSTATS/LOGICREADS"), DCI_DT_INT64, _T("Oracle/Performance: Number of logical reads") },
	{ _T("Oracle.Performance.LibCacheHitRatio(*)"), H_GlobalParameter, _T("GLOBALSTATS/LIBCACHEHITRATIO"), DCI_DT_STRING, _T("Oracle/Performance: Library cache hit ratio") },
	{ _T("Oracle.Performance.MemorySortRatio(*)"), H_GlobalParameter, _T("GLOBALSTATS/MEMORYSORTRATIO"), DCI_DT_STRING, _T("Oracle/Performance: PGA memory sort ratio") },
	{ _T("Oracle.Performance.PhysicalReads(*)"),  H_GlobalParameter, _T("GLOBALSTATS/PHYSREADS"), DCI_DT_INT64, _T("Oracle/Performance: Number of physical reads") },
	{ _T("Oracle.Performance.RollbackWaitRatio(*)"), H_GlobalParameter, _T("GLOBALSTATS/ROLLBACKWAITRATIO"), DCI_DT_STRING, _T("Oracle/Performance: Ratio of waits for requests to rollback segments") },
	{ _T("Oracle.Sessions.Count(*)"), H_GlobalParameter, _T("GLOBALSTATS/SESSIONCOUNT"), DCI_DT_INT, _T("Oracle/Sessions: Number of sessions opened") },
	{ _T("Oracle.TableSpaces.Status(*)"), H_InstanceParameter, _T("TABLESPACE/STATUS"), DCI_DT_STRING, _T("Oracle/Tablespaces: Status") },
	{ _T("Oracle.TableSpaces.Type(*)"), H_InstanceParameter, _T("TABLESPACE/TYPE"), DCI_DT_STRING, _T("Oracle/Tablespaces: Type") },
	{ _T("Oracle.TableSpaces.UsedPct(*)"), H_InstanceParameter, _T("TABLESPACE/USEDPCT"), DCI_DT_INT, _T("Oracle/Tablespaces: Percentage used") }
};

/**
 * Supported lists
 */
static NETXMS_SUBAGENT_LIST s_lists[] =
{
   { _T("Oracle.DataTags(*)"), H_TagList, _T("^(.*)$") },
   { _T("Oracle.TableSpaces(*)"), H_TagList, _T("^TABLESPACE/STATUS@(.*)$") }
};

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO m_info =
{
	NETXMS_SUBAGENT_INFO_MAGIC,
	_T("ORACLE"), NETXMS_VERSION_STRING,
	SubAgentInit, SubAgentShutdown, NULL,
	sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM), m_parameters,
	sizeof(s_lists) / sizeof(NETXMS_SUBAGENT_LIST), s_lists,
	0,	NULL,    // tables
	0,	NULL,    // actions
	0,	NULL     // push parameters
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
