/*
 ** NetXMS - Network Management System
 ** Subagent for Oracle monitoring
 ** Copyright (C) 2009-2025 Raden Solutions
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
#include <netxms-version.h>

/**
 * Driver handle
 */
DB_DRIVER g_oracleDriver = nullptr;

/**
 * Polling queries
 */
DatabaseQuery g_queries[] =
{
   { _T("ASM_DISKGROUP"), MAKE_ORACLE_VERSION(11, 1), 1,
       _T("SELECT name, state, type, (total_mb * 1024 * 1024) AS total, (free_mb * 1024 * 1024) AS free,")
         _T("round(100 * free_mb / total_mb, 2) AS freepct, round(100 * (total_mb - free_mb) / total_mb, 2) AS usedpct,")
         _T("((total_mb - free_mb) * 1024 * 1024) AS used, offline_disks FROM v$asm_diskgroup")
   },
   { _T("DATAFILE"), MAKE_ORACLE_VERSION(10, 2), 1,
      _T("SELECT regexp_substr(regexp_substr(f.name, '[/\\][^/\\]+$'), '[^/\\]+') AS name,")
         _T("f.name AS full_name, (SELECT name FROM v$tablespace ts WHERE ts.ts#=d.ts#) AS tablespace,")
         _T("d.status AS status, d.bytes AS bytes, d.blocks AS blocks, d.block_size AS block_size,")
         _T("s.phyrds AS phy_reads, s.phywrts AS phy_writes, s.readtim * 100 AS read_time,")
         _T("s.writetim * 100 AS write_time, s.avgiotim * 100 AS avg_io_time, s.miniotim * 100 AS min_io_time,")
         _T("s.maxiortm * 100 AS max_io_rtime, s.maxiowtm * 100 AS max_io_wtime ")
         _T("FROM sys.v_$dbfile f ")
         _T("INNER JOIN sys.v_$filestat s ON s.file#=f.file# ")
         _T("INNER JOIN v$datafile d ON d.file#=f.file#")
   },
   { _T("DBINFO"), MAKE_ORACLE_VERSION(7, 0), 0, _T("SELECT name, to_char(created) CreateDate, log_mode, open_mode FROM v$database") },
   { _T("DUAL"), MAKE_ORACLE_VERSION(7, 0), 0, _T("SELECT decode(count(*),1,0,1) ExcessRows FROM dual") },
   { _T("INSTANCE"), MAKE_ORACLE_VERSION(7, 0), 0, _T("SELECT version,status,archiver,shutdown_pending FROM v$instance") },
   { _T("GLOBALSTATS"), MAKE_ORACLE_VERSION(7, 0), 0,
         _T("SELECT ")
            _T("(SELECT s.value FROM v$sysstat s, v$statname n WHERE n.name='enqueue deadlocks' AND n.statistic#=s.statistic#) Deadlocks, ")
            _T("(SELECT s.value FROM v$sysstat s, v$statname n WHERE n.name='physical reads' AND n.statistic#=s.statistic#) PhysReads, ")
            _T("(SELECT s.value FROM v$sysstat s, v$statname n WHERE n.name='physical writes' AND n.statistic#=s.statistic#) PhysWrites, ")
            _T("(SELECT s.value FROM v$sysstat s, v$statname n WHERE n.name='session logical reads' AND n.statistic#=s.statistic#) LogicReads, ")
            _T("(SELECT round((sum(decode(name, 'consistent gets', value, 0)) + sum(decode(name, 'db block gets', value, 0)) - sum(decode(name, 'physical reads', value, 0))) / (sum(decode(name, 'consistent gets', value, 0)) + sum(decode(name, 'db block gets', value, 0))) * 100, 2) FROM v$sysstat) CacheHitRatio, ")
            _T("(SELECT CASE WHEN sum(gets) = 0 THEN 0 ELSE round(sum(waits) * 100 / sum(gets), 2) END FROM v$rollstat) RollbackWaitRatio, ")
            _T("(SELECT CASE WHEN sum(gets) = 0 THEN 0 ELSE round((1 - (sum(getmisses) / sum(gets))) * 100, 2) END FROM v$rowcache) DictCacheHitRatio, ")
            _T("(SELECT CASE WHEN sum(pins) + sum(reloads) = 0 THEN 0 ELSE round(sum(pins) / (sum(pins) + sum(reloads)) * 100, 2) END FROM v$librarycache) LibCacheHitRatio, ")
            _T("(SELECT round((100 * b.value) / decode((a.value + b.value), 0, 1, (a.value + b.value)), 2) FROM v$sysstat a,v$sysstat b WHERE a.name = 'sorts (disk)' AND b.name = 'sorts (memory)') MemorySortRatio, ")
            _T("(SELECT CASE WHEN sum(busy) + sum(idle) = 0 THEN 0 ELSE round(nvl((sum(busy) / (sum(busy) + sum(idle))) * 100, 0), 2) END FROM v$dispatcher) DispatcherWorkload, ")
            _T("(SELECT bytes	FROM v$sgastat WHERE name = 'free memory' AND pool = 'shared pool') FreeSharedPool, ")
            _T("(SELECT count(*) FROM dba_tablespaces WHERE status <> 'ONLINE') TSOffCount, ")
            _T("(SELECT count(*) FROM v$datafile WHERE status NOT IN ('ONLINE', 'SYSTEM')) DFOffCount, ")
            _T("(SELECT count(*) FROM dba_segments WHERE max_extents = extents) FullSegCnt, ")
            _T("(SELECT count(*) FROM dba_rollback_segs WHERE status <> 'ONLINE') RBSNotOnline,")
            _T("decode(sign(decode((SELECT upper(log_mode) FROM v$database), 'ARCHIVELOG', 1, 0) -")
            _T("   decode((SELECT upper(value) FROM v$parameter WHERE upper(name) = 'LOG_ARCHIVE_START'), 'TRUE', 1, 0)), 1, 1, 0) AAOff,")
            _T("(SELECT count(file#) FROM v$datafile_header WHERE recover = 'YES') DFNeedRec,")
            _T("(SELECT count(*) FROM dba_jobs WHERE nvl(failures, 0) <> 0) FailedJobs,")
            _T("(SELECT sum(a.value) FROM v$sesstat a, v$statname b, v$session s WHERE a.statistic# = b.statistic# AND s.sid = a.sid AND b.name = 'opened cursors current') OpenCursors,")
            _T("(SELECT max(a.value) FROM v$sesstat a, v$statname b, v$session s WHERE a.statistic# = b.statistic# AND s.sid = a.sid AND b.name = 'opened cursors current') MaxOpenCursorsPerSession,")
            _T("(SELECT count(*) FROM dba_objects WHERE status!='VALID') InvalidObjects,")
            _T("(SELECT coalesce(sum(bytes), 0) FROM v$log) RedoTotal,")
            _T("(SELECT coalesce(sum(blocks * block_size), 0) FROM v$archived_log WHERE deleted='NO') ArchivedTotal,")
            _T("(SELECT /*+ cardinality(l.s 3000) cardinality(l.r 13000) */ count(*) FROM v$lock l) Locks,")
            _T("(SELECT count(*) FROM v$session) SessionCount ")
         _T("FROM dual")
   },
   { _T("SESSIONS_BY_PROGRAM"), MAKE_ORACLE_VERSION(7, 0), 1, _T("SELECT program, count(*) AS count FROM v$session GROUP BY program") },
   { _T("SESSIONS_BY_SCHEMA"), MAKE_ORACLE_VERSION(7, 0), 1, _T("SELECT schemaname, count(*) AS count FROM v$session GROUP BY schemaname") },
   { _T("SESSIONS_BY_USER"), MAKE_ORACLE_VERSION(7, 0), 1, _T("SELECT username, count(*) AS count FROM v$session WHERE username IS NOT NULL GROUP BY username") },
   { _T("TABLESPACE"), MAKE_ORACLE_VERSION(10, 0), 1,
      _T("SELECT d.tablespace_name, d.status AS Status, d.contents AS Type, d.block_size AS BlockSize, d.logging AS Logging,")
         _T("m.tablespace_size * d.block_size AS TotalSpace,")
         _T("m.used_space * d.block_size AS UsedSpace,")
         _T("trim(to_char(round(m.used_percent, 2), '990.99')) AS UsedSpacePct,")
         _T("(m.tablespace_size - m.used_space) * d.block_size AS FreeSpace,")
         _T("trim(to_char(round((100 - m.used_percent), 2), '990.99')) AS FreeSpacePct,")
         _T("(SELECT count(*) FROM v$datafile df WHERE df.ts#=v.ts#) AS DataFiles ")
         _T("FROM dba_tablespaces d ")
         _T("INNER JOIN dba_tablespace_usage_metrics m ON m.tablespace_name = d.tablespace_name ")
         _T("INNER JOIN v$tablespace v ON v.name = d.tablespace_name")
   },
   { nullptr, 0, 0, nullptr }
};

/**
 * Table query definition: Sessions
 */
static TableDescriptor s_tqSessions =
{
   _T("SELECT sid,username,schemaname AS schema,osuser AS os_user,machine,status,process,program,type,state,service_name AS service FROM v$session"),
   {
      { DCI_DT_INT, _T("SID") },
      { DCI_DT_STRING, _T("User") },
      { DCI_DT_STRING, _T("Schema") },
      { DCI_DT_STRING, _T("OS User") },
      { DCI_DT_STRING, _T("Machine") },
      { DCI_DT_STRING, _T("Status") },
      { DCI_DT_STRING, _T("Process") },
      { DCI_DT_STRING, _T("Program") },
      { DCI_DT_STRING, _T("Type") },
      { DCI_DT_STRING, _T("State") },
      { DCI_DT_STRING, _T("Service") }
   }
};

/**
 * Table query definition: TableSpaces
 */
static TableDescriptor s_tqTableSpaces =
{
   _T("SELECT d.tablespace_name AS name, d.status AS status, d.contents AS type, d.block_size AS block_size, d.logging AS logging,")
      _T("m.tablespace_size * d.block_size AS total,")
      _T("m.used_space * d.block_size AS used,")
      _T("trim(to_char(round(m.used_percent,2), '990.99')) AS used_pct,")
      _T("(m.tablespace_size - m.used_space) * d.block_size AS free,")
      _T("trim(to_char(round((100 - m.used_percent),2), '990.99')) AS free_pct,")
      _T("(SELECT count(*) FROM v$datafile df WHERE df.ts#=v.ts#) AS data_files ")
      _T("FROM dba_tablespaces d ")
      _T("INNER JOIN dba_tablespace_usage_metrics m ON m.tablespace_name=d.tablespace_name ")
      _T("INNER JOIN v$tablespace v ON v.name=d.tablespace_name"),
   {
      { DCI_DT_STRING, _T("Name") },
      { DCI_DT_STRING, _T("Status") },
      { DCI_DT_STRING, _T("Type") },
      { DCI_DT_INT, _T("Block size") },
      { DCI_DT_STRING, _T("Logging") },
      { DCI_DT_INT64, _T("Total") },
      { DCI_DT_INT64, _T("Used") },
      { DCI_DT_FLOAT, _T("Used %") },
      { DCI_DT_INT64, _T("Free") },
      { DCI_DT_FLOAT, _T("Free %") },
      { DCI_DT_INT, _T("Data Files") }
   }
};

/**
 * Table query definition: DataFiles
 */
static TableDescriptor s_tqDataFiles =
{
   _T("SELECT regexp_substr(regexp_substr(f.name, '[/\\][^/\\]+$'), '[^/\\]+') AS name, f.file# AS id,")
      _T("f.name AS full_name, (SELECT name FROM v$tablespace ts WHERE ts.ts#=d.ts#) AS tablespace,")
      _T("d.status AS status, d.bytes AS bytes,d.blocks AS blocks, d.block_size AS block_size,")
      _T("s.phyrds AS phy_reads, s.phywrts AS phy_writes, s.readtim * 100 AS read_time,")
      _T("s.writetim * 100 AS write_time, s.avgiotim * 100 AS avg_io_time, s.miniotim * 100 AS min_io_time,")
      _T("s.maxiortm * 100 AS max_io_rtime, s.maxiowtm * 100 AS max_io_wtime ")
      _T("FROM sys.v_$dbfile f ")
      _T("INNER JOIN sys.v_$filestat s ON s.file#=f.file# ")
      _T("INNER JOIN v$datafile d ON d.file#=f.file#"),
   {
      { DCI_DT_STRING, _T("Name") },
      { DCI_DT_INT, _T("ID") },
      { DCI_DT_STRING, _T("Full name") },
      { DCI_DT_STRING, _T("Tablespace") },
      { DCI_DT_STRING, _T("Status") },
      { DCI_DT_INT64, _T("Bytes") },
      { DCI_DT_INT64, _T("Blocks") },
      { DCI_DT_INT, _T("Block Size") },
      { DCI_DT_INT64, _T("Physical Reads") },
      { DCI_DT_INT64, _T("Physical Writes") },
      { DCI_DT_INT64, _T("Read Time") },
      { DCI_DT_INT64, _T("Write Time") },
      { DCI_DT_INT, _T("Avg I/O Time") },
      { DCI_DT_INT, _T("Min I/O Time") },
      { DCI_DT_INT, _T("Max Read Time") },
      { DCI_DT_INT, _T("Max Write Time") }
   }
};

/**
 * Database instances
 */
static ObjectArray<DatabaseInstance> *s_instances = nullptr;

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
   return nullptr;
}

/**
 * Handler for parameters without instance
 */
static LONG H_GlobalParameter(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR id[MAX_STR];
   if (!AgentGetParameterArg(param, 1, id, MAX_STR))
      return SYSINFO_RC_UNSUPPORTED;

   DatabaseInstance *db = FindInstance(id);
   if (db == nullptr)
      return SYSINFO_RC_UNSUPPORTED;

   return db->getData(arg, value) ? SYSINFO_RC_SUCCESS : SYSINFO_RC_ERROR;
}

/**
 * Handler for parameters with instance
 */
static LONG H_InstanceParameter(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR id[MAX_STR];
   if (!AgentGetParameterArg(param, 1, id, MAX_STR))
      return SYSINFO_RC_UNSUPPORTED;

   DatabaseInstance *db = FindInstance(id);
   if (db == nullptr)
      return SYSINFO_RC_UNSUPPORTED;

   TCHAR instance[MAX_STR];
   if (!AgentGetParameterArg(param, 2, instance, MAX_STR))
      return SYSINFO_RC_UNSUPPORTED;

   TCHAR tag[MAX_STR];
   bool missingAsZero;
   if (arg[0] == _T('?'))  // count missing instance as 0
   {
      _sntprintf(tag, MAX_STR, _T("%s@%s"), &arg[1], instance);
      missingAsZero = true;
   }
   else
   {
      _sntprintf(tag, MAX_STR, _T("%s@%s"), arg, instance);
      missingAsZero = false;
   }
   if (db->getData(tag, value))
      return SYSINFO_RC_SUCCESS;
   if (missingAsZero)
   {
      ret_int(value, 0);
      return SYSINFO_RC_SUCCESS;
   }
   return SYSINFO_RC_ERROR;
}

/**
 * Handler for Oracle.DBInfo.Version parameter
 */
static LONG H_DatabaseVersion(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR id[MAX_STR];
   if (!AgentGetParameterArg(param, 1, id, MAX_STR))
      return SYSINFO_RC_UNSUPPORTED;

   DatabaseInstance *db = FindInstance(id);
   if (db == nullptr)
      return SYSINFO_RC_UNSUPPORTED;

   int version = db->getVersion();
   _sntprintf(value, MAX_RESULT_LENGTH, _T("%d.%d"), version >> 8, version & 0xFF);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for Oracle.DBInfo.IsReachable parameter
 */
static LONG H_DatabaseConnectionStatus(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR id[MAX_STR];
   if (!AgentGetParameterArg(param, 1, id, MAX_STR))
      return SYSINFO_RC_UNSUPPORTED;

   DatabaseInstance *db = FindInstance(id);
   if (db == nullptr)
      return SYSINFO_RC_UNSUPPORTED;

   ret_string(value, db->isConnected() ? _T("YES") : _T("NO"));
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for generic list parameter
 */
static LONG H_TagList(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   TCHAR id[MAX_STR];
   if (!AgentGetParameterArg(param, 1, id, MAX_STR))
      return SYSINFO_RC_UNSUPPORTED;

   DatabaseInstance *db = FindInstance(id);
   if (db == nullptr)
      return SYSINFO_RC_UNSUPPORTED;

   return db->getTagList(arg, value) ? SYSINFO_RC_SUCCESS : SYSINFO_RC_ERROR;
}

/**
 * Handler for generic table queries
 */
static LONG H_TableQuery(const TCHAR *param, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   TCHAR id[MAX_STR];
   if (!AgentGetParameterArg(param, 1, id, MAX_STR))
      return SYSINFO_RC_UNSUPPORTED;

   DatabaseInstance *db = FindInstance(id);
   if (db == nullptr)
      return SYSINFO_RC_UNSUPPORTED;

   return db->queryTable((TableDescriptor *)arg, value) ? SYSINFO_RC_SUCCESS : SYSINFO_RC_ERROR;
}

/**
 * Config template
 */
static DatabaseInfo s_dbInfo;
static NX_CFG_TEMPLATE s_configTemplate[] =
{
   { _T("ConnectionTTL"),     CT_LONG,   0, 0, 0,             0, &s_dbInfo.connectionTTL },
   { _T("Id"),                CT_STRING, 0, 0, MAX_STR,       0, s_dbInfo.id },
	{ _T("Name"),				   CT_STRING, 0, 0, MAX_STR,       0, s_dbInfo.name },
	{ _T("TnsName"),			   CT_STRING, 0, 0, MAX_STR,       0, s_dbInfo.name },
	{ _T("UserName"),			   CT_STRING, 0, 0, MAX_USERNAME,  0, s_dbInfo.username },
	{ _T("Password"),			   CT_STRING, 0, 0, MAX_PASSWORD,  0, s_dbInfo.password },
   { _T("EncryptedPassword"), CT_STRING, 0, 0, MAX_PASSWORD,  0, s_dbInfo.password },
	{ _T(""), CT_END_OF_LIST, 0, 0, 0, 0, nullptr }
};

/*
 * Subagent initialization
 */
static bool SubAgentInit(Config *config)
{
	// Init db driver
   g_oracleDriver = DBLoadDriver(_T("oracle.ddr"), config->getValue(_T("/ORACLE/DriverOptions")), nullptr, nullptr);
	if (g_oracleDriver == nullptr)
	{
		nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_ORACLE, _T("Cannot load Oracle database driver"));
		return false;
	}

   s_instances = new ObjectArray<DatabaseInstance>(8, 8, Ownership::True);

	// Load configuration from "oracle" section to allow simple configuration
	// of one database without XML includes
	memset(&s_dbInfo, 0, sizeof(s_dbInfo));
	s_dbInfo.connectionTTL = 3600;
	if (config->parseTemplate(_T("ORACLE"), s_configTemplate))
	{
		if (s_dbInfo.name[0] != 0)
		{
			if (s_dbInfo.id[0] == 0)
				_tcscpy(s_dbInfo.id, s_dbInfo.name);

         DecryptPassword(s_dbInfo.username, s_dbInfo.password, s_dbInfo.password, MAX_PASSWORD);
         s_instances->add(new DatabaseInstance(&s_dbInfo));
		}
	}

	// Load full-featured XML configuration
	for(int i = 1; i <= 64; i++)
	{
		TCHAR section[MAX_STR];
		memset(&s_dbInfo, 0, sizeof(s_dbInfo));
		s_dbInfo.connectionTTL = 3600;
		_sntprintf(section, MAX_STR, _T("oracle/databases/database#%d"), i);

		if (!config->parseTemplate(section, s_configTemplate))
		{
			nxlog_write(NXLOG_WARNING, DEBUG_TAG_ORACLE, _T("Error parsing Oracle subagent configuration template #%d"), i);
         continue;
		}

		if (s_dbInfo.name[0] == 0)
			continue;

      DecryptPassword(s_dbInfo.username, s_dbInfo.password, s_dbInfo.password, MAX_PASSWORD);

      s_instances->add(new DatabaseInstance(&s_dbInfo));
	}

	// Exit if no usable configuration found
   if (s_instances->isEmpty())
	{
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG_ORACLE, _T("No Oracle databases to monitor"));
      delete s_instances;
      return false;
	}

	// Run query thread for each configured database
   for(int i = 0; i < s_instances->size(); i++)
      s_instances->get(i)->run();

	return true;
}

/**
 * Shutdown handler
 */
static void SubAgentShutdown()
{
	nxlog_debug_tag(DEBUG_TAG_ORACLE, 1, _T("Stopping Oracle database pollers"));
   for(int i = 0; i < s_instances->size(); i++)
      s_instances->get(i)->stop();
   delete s_instances;
   DBUnloadDriver(g_oracleDriver);
	nxlog_debug_tag(DEBUG_TAG_ORACLE, 1, _T("Oracle subagent stopped"));
}

/**
 * Supported parameters
 */
static NETXMS_SUBAGENT_PARAM s_parameters[] =
{
   { _T("Oracle.ASM.DiskGroup.Free(*)"), H_InstanceParameter, _T("ASM_DISKGROUP/FREE"), DCI_DT_STRING, _T("Oracle/ASM/Disk group: {instance} free space") },
   { _T("Oracle.ASM.DiskGroup.FreePerc(*)"), H_InstanceParameter, _T("ASM_DISKGROUP/FREEPCT"), DCI_DT_STRING, _T("Oracle/ASM/Disk group: {instance} free space (%)") },
   { _T("Oracle.ASM.DiskGroup.OfflineDisks(*)"), H_InstanceParameter, _T("ASM_DISKGROUP/OFFLINE_DISKS"), DCI_DT_STRING, _T("Oracle/ASM/Disk group: {instance} offline disks") },
   { _T("Oracle.ASM.DiskGroup.State(*)"), H_InstanceParameter, _T("ASM_DISKGROUP/STATE"), DCI_DT_STRING, _T("Oracle/ASM/Disk group: {instance} state") },
   { _T("Oracle.ASM.DiskGroup.Total(*)"), H_InstanceParameter, _T("ASM_DISKGROUP/TOTAL"), DCI_DT_STRING, _T("Oracle/ASM/Disk group: {instance} total space") },
   { _T("Oracle.ASM.DiskGroup.Type(*)"), H_InstanceParameter, _T("ASM_DISKGROUP/TYPE"), DCI_DT_STRING, _T("Oracle/ASM/Disk group: {instance} type") },
   { _T("Oracle.ASM.DiskGroup.Used(*)"), H_InstanceParameter, _T("ASM_DISKGROUP/USED"), DCI_DT_STRING, _T("Oracle/ASM/Disk group: {instance} used space") },
   { _T("Oracle.ASM.DiskGroup.UsedPerc(*)"), H_InstanceParameter, _T("ASM_DISKGROUP/USEDPCT"), DCI_DT_STRING, _T("Oracle/ASM/Disk group: {instance} used space (%)") },
	{ _T("Oracle.CriticalStats.AutoArchivingOff(*)"), H_GlobalParameter, _T("GLOBALSTATS/AAOFF"), DCI_DT_STRING, _T("Oracle/CriticalStats: Archive logs enabled but auto archiving off ") },
	{ _T("Oracle.CriticalStats.DatafilesNeedMediaRecovery(*)"), H_GlobalParameter, _T("GLOBALSTATS/DFNEEDREC"), DCI_DT_INT64, _T("Oracle/CriticalStats: Number of datafiles that need media recovery") },
	{ _T("Oracle.CriticalStats.Deadlocks(*)"), H_GlobalParameter, _T("GLOBALSTATS/DEADLOCKS"), DCI_DT_INT64, _T("Oracle/CriticalStats: Cumulative number of deadlocks") },
	{ _T("Oracle.CriticalStats.DFOffCount(*)"), H_GlobalParameter, _T("GLOBALSTATS/DFOFFCOUNT"), DCI_DT_INT, _T("Oracle/CriticalStats: Number of offline datafiles") },
	{ _T("Oracle.CriticalStats.FailedJobs(*)"), H_GlobalParameter, _T("GLOBALSTATS/FAILEDJOBS"), DCI_DT_INT64, _T("Oracle/CriticalStats: Number of failed jobs") },
	{ _T("Oracle.CriticalStats.FullSegmentsCount(*)"), H_GlobalParameter, _T("GLOBALSTATS/FULLSEGCNT"), DCI_DT_INT64, _T("Oracle/CriticalStats: Number of segments that cannot extend") },
	{ _T("Oracle.CriticalStats.RBSegsNotOnlineCount(*)"), H_GlobalParameter, _T("GLOBALSTATS/RBSNOTONLINE"), DCI_DT_INT64, _T("Oracle/CriticalStats: Number of rollback segments not online") },
	{ _T("Oracle.CriticalStats.TSOffCount(*)"), H_GlobalParameter, _T("GLOBALSTATS/TSOFFCOUNT"), DCI_DT_INT, _T("Oracle/CriticalStats: Number of offline tablespaces") },
	{ _T("Oracle.Cursors.Count(*)"), H_GlobalParameter, _T("GLOBALSTATS/OPENCURSORS"), DCI_DT_INT, _T("Oracle/Cursors: Current number of opened cursors systemwide") },
   { _T("Oracle.Cursors.MaxPerSession(*)"), H_GlobalParameter, _T("GLOBALSTATS/MAXOPENCURSORSPERSESSION"), DCI_DT_INT, _T("Oracle/Cursors: Current maximum number of opened cursors per session") },
   { _T("Oracle.DataFile.AvgIoTime(*)"), H_InstanceParameter, _T("DATAFILE/AVG_IO_TIME"), DCI_DT_INT, _T("Oracle/Datafile: {instance} average I/O time") },
   { _T("Oracle.DataFile.Blocks(*)"), H_InstanceParameter, _T("DATAFILE/BLOCKS"), DCI_DT_INT64, _T("Oracle/Datafile: {instance} size in blocks") },
   { _T("Oracle.DataFile.BlockSize(*)"), H_InstanceParameter, _T("DATAFILE/BLOCK_SIZE"), DCI_DT_INT, _T("Oracle/Datafile: {instance} block size") },
   { _T("Oracle.DataFile.Bytes(*)"), H_InstanceParameter, _T("DATAFILE/BYTES"), DCI_DT_INT64, _T("Oracle/Datafile: {instance} size in bytes") },
   { _T("Oracle.DataFile.FullName(*)"), H_InstanceParameter, _T("DATAFILE/FULL_NAME"), DCI_DT_STRING, _T("Oracle/Datafile: {instance} full name") },
   { _T("Oracle.DataFile.MaxIoReadTime(*)"), H_InstanceParameter, _T("DATAFILE/MAX_IO_RTIME"), DCI_DT_INT, _T("Oracle/Datafile: {instance} maximum read time") },
   { _T("Oracle.DataFile.MaxIoWriteTime(*)"), H_InstanceParameter, _T("DATAFILE/MAX_IO_WTIME"), DCI_DT_INT, _T("Oracle/Datafile: {instance} maximum write time") },
   { _T("Oracle.DataFile.MinIoTime(*)"), H_InstanceParameter, _T("DATAFILE/MIN_IO_TIME"), DCI_DT_INT, _T("Oracle/Datafile: {instance} minimum I/O time") },
   { _T("Oracle.DataFile.PhysicalReads(*)"), H_InstanceParameter, _T("DATAFILE/PHY_READS"), DCI_DT_INT64, _T("Oracle/Datafile: {instance} physical reads") },
   { _T("Oracle.DataFile.PhysicalWrites(*)"), H_InstanceParameter, _T("DATAFILE/PHY_WRITES"), DCI_DT_INT64, _T("Oracle/Datafile: {instance} physical writes") },
   { _T("Oracle.DataFile.ReadTime(*)"), H_InstanceParameter, _T("DATAFILE/READ_TIME"), DCI_DT_INT64, _T("Oracle/Datafile: {instance} read time") },
   { _T("Oracle.DataFile.Status(*)"), H_InstanceParameter, _T("DATAFILE/STATUS"), DCI_DT_STRING, _T("Oracle/Datafile: {instance} status") },
   { _T("Oracle.DataFile.Tablespace(*)"), H_InstanceParameter, _T("DATAFILE/TABLESPACE"), DCI_DT_STRING, _T("Oracle/Datafile: {instance} tablespace") },
   { _T("Oracle.DataFile.WriteTime(*)"), H_InstanceParameter, _T("DATAFILE/WRITE_TIME"), DCI_DT_INT64, _T("Oracle/Datafile: {instance} write time") },
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
   { _T("Oracle.Logs.ArchivedSize(*)"), H_GlobalParameter, _T("GLOBALSTATS/ARCHIVEDTOTAL"), DCI_DT_INT64, _T("Oracle/Logs: Total size of archived logs") },
   { _T("Oracle.Logs.RedoSize(*)"), H_GlobalParameter, _T("GLOBALSTATS/REDOTOTAL"), DCI_DT_INT64, _T("Oracle/Logs: Total size of redo logs") },
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
	{ _T("Oracle.Performance.PhysicalWrites(*)"),  H_GlobalParameter, _T("GLOBALSTATS/PHYSWRITES"), DCI_DT_INT64, _T("Oracle/Performance: Number of physical writes") },
	{ _T("Oracle.Performance.RollbackWaitRatio(*)"), H_GlobalParameter, _T("GLOBALSTATS/ROLLBACKWAITRATIO"), DCI_DT_STRING, _T("Oracle/Performance: Ratio of waits for requests to rollback segments") },
	{ _T("Oracle.Sessions.Count(*)"), H_GlobalParameter, _T("GLOBALSTATS/SESSIONCOUNT"), DCI_DT_INT, _T("Oracle/Sessions: Number of sessions opened") },
   { _T("Oracle.Sessions.CountByProgram(*)"), H_InstanceParameter, _T("?SESSIONS_BY_PROGRAM/COUNT"), DCI_DT_INT, _T("Oracle/Sessions: Number of sessions created by program {instance}") },
   { _T("Oracle.Sessions.CountBySchema(*)"), H_InstanceParameter, _T("?SESSIONS_BY_SCHEMA/COUNT"), DCI_DT_INT, _T("Oracle/Sessions: Number of sessions with schema {instance}") },
   { _T("Oracle.Sessions.CountByUser(*)"), H_InstanceParameter, _T("?SESSIONS_BY_USER/COUNT"), DCI_DT_INT, _T("Oracle/Sessions: Number of sessions with user name {instance}") },
   { _T("Oracle.TableSpace.BlockSize(*)"), H_InstanceParameter, _T("TABLESPACE/BLOCKSIZE"), DCI_DT_INT, _T("Oracle/Tablespace: {instance} block size") },
   { _T("Oracle.TableSpace.DataFiles(*)"), H_InstanceParameter, _T("TABLESPACE/DATAFILES"), DCI_DT_INT, _T("Oracle/Tablespace: {instance} number of datafiles") },
	{ _T("Oracle.TableSpace.FreeBytes(*)"), H_InstanceParameter, _T("TABLESPACE/FREESPACE"), DCI_DT_INT64, _T("Oracle/Tablespace: {instance} bytes free") },
	{ _T("Oracle.TableSpace.FreePct(*)"), H_InstanceParameter, _T("TABLESPACE/FREESPACEPCT"), DCI_DT_INT, _T("Oracle/Tablespace: {instance} percentage free") },
	{ _T("Oracle.TableSpace.Logging(*)"), H_InstanceParameter, _T("TABLESPACE/LOGGING"), DCI_DT_STRING, _T("Oracle/Tablespace: {instance} logging mode") },
	{ _T("Oracle.TableSpace.Status(*)"), H_InstanceParameter, _T("TABLESPACE/STATUS"), DCI_DT_STRING, _T("Oracle/Tablespace: {instance} status") },
	{ _T("Oracle.TableSpace.TotalBytes(*)"), H_InstanceParameter, _T("TABLESPACE/TOTALSPACE"), DCI_DT_INT64, _T("Oracle/Tablespace: {instance} bytes total") },
	{ _T("Oracle.TableSpace.Type(*)"), H_InstanceParameter, _T("TABLESPACE/TYPE"), DCI_DT_STRING, _T("Oracle/Tablespace: {instance} type") },
	{ _T("Oracle.TableSpace.UsedBytes(*)"), H_InstanceParameter, _T("TABLESPACE/USEDSPACE"), DCI_DT_INT64, _T("Oracle/Tablespace: {instance} bytes used") },
	{ _T("Oracle.TableSpace.UsedPct(*)"), H_InstanceParameter, _T("TABLESPACE/USEDSPACEPCT"), DCI_DT_INT, _T("Oracle/Tablespace: {instance} percentage used") }
};

/**
 * Supported lists
 */
static NETXMS_SUBAGENT_LIST s_lists[] =
{
   { _T("Oracle.ASM.DiskGroups(*)"), H_TagList, _T("^ASM_DISKGROUP/STATUS@(.*)$") },
   { _T("Oracle.DataFiles(*)"), H_TagList, _T("^DATAFILE/STATUS@(.*)$") },
   { _T("Oracle.DataTags(*)"), H_TagList, _T("^(.*)$") },
   { _T("Oracle.TableSpaces(*)"), H_TagList, _T("^TABLESPACE/STATUS@(.*)$") }
};

/**
 * Supported tables
 */
static NETXMS_SUBAGENT_TABLE s_tables[] =
{
   { _T("Oracle.DataFiles(*)"), H_TableQuery, (const TCHAR *)&s_tqDataFiles, _T("SID"), _T("Oracle: data files") },
   { _T("Oracle.Sessions(*)"), H_TableQuery, (const TCHAR *)&s_tqSessions, _T("SID"), _T("Oracle: open sessions") },
   { _T("Oracle.TableSpaces(*)"), H_TableQuery, (const TCHAR *)&s_tqTableSpaces, _T("NAME"), _T("Oracle: table spaces") }
};

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO s_info =
{
	NETXMS_SUBAGENT_INFO_MAGIC,
	_T("ORACLE"), NETXMS_VERSION_STRING,
	SubAgentInit, SubAgentShutdown, nullptr, nullptr, nullptr,
	sizeof(s_parameters) / sizeof(NETXMS_SUBAGENT_PARAM), s_parameters,
	sizeof(s_lists) / sizeof(NETXMS_SUBAGENT_LIST), s_lists,
	sizeof(s_tables) / sizeof(NETXMS_SUBAGENT_TABLE), s_tables,
	0,	nullptr,    // actions
	0,	nullptr     // push parameters
};

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(ORACLE)
{
	*ppInfo = &s_info;
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
