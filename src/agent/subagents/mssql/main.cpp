/*
 ** NetXMS - Network Management System
 ** Subagent for Microsoft SQL Server monitoring
 ** Copyright (C) 2026 Raden Solutions
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

#include "mssql_subagent.h"
#include <netxms-version.h>

/**
 * Driver handle
 */
DB_DRIVER g_mssqlDriver = nullptr;

/**
 * Polling queries
 */
DatabaseQuery g_queries[] =
{
   { _T("SERVERINFO"), 0,
      _T("SELECT CONVERT(varchar(64), SERVERPROPERTY('ProductVersion')) AS VERSION,")
         _T("CONVERT(varchar(64), SERVERPROPERTY('Edition')) AS EDITION,")
         _T("CONVERT(varchar(64), SERVERPROPERTY('ProductLevel')) AS PRODUCTLEVEL,")
         _T("CONVERT(varchar(128), SERVERPROPERTY('MachineName')) AS MACHINENAME,")
         _T("CONVERT(varchar(128), SERVERPROPERTY('InstanceName')) AS INSTANCENAME,")
         _T("DATEDIFF(SECOND, sqlserver_start_time, GETDATE()) AS UPTIME ")
         _T("FROM sys.dm_os_sys_info")
   },
   { _T("GLOBALSTATS"), 0,
      _T("SELECT ")
         _T("(SELECT COUNT(*) FROM sys.dm_exec_connections) AS CONNECTIONS,")
         _T("(SELECT COUNT(*) FROM sys.dm_exec_sessions WHERE is_user_process = 1) AS USERSESSIONS,")
         _T("(SELECT COUNT(*) FROM sys.dm_exec_sessions WHERE is_user_process = 1 AND status = 'running') AS ACTIVESESSIONS,")
         _T("(SELECT COUNT(*) FROM sys.dm_exec_requests WHERE blocking_session_id <> 0) AS BLOCKEDPROCESSES,")
         _T("(SELECT COUNT(*) FROM sys.dm_tran_locks WHERE request_status = 'WAIT') AS LOCKWAITS")
   },
   { _T("PERFCOUNTERS"), 0,
      _T("SELECT ")
         _T("(SELECT TOP 1 cntr_value FROM sys.dm_os_performance_counters WHERE counter_name = 'Page life expectancy' AND object_name LIKE '%:Buffer Manager%') AS PAGELIFEEXPECTANCY,")
         _T("(SELECT TOP 1 cntr_value FROM sys.dm_os_performance_counters WHERE counter_name = 'Total Server Memory (KB)') * 1024 AS TOTALSERVERMEMORY,")
         _T("(SELECT TOP 1 cntr_value FROM sys.dm_os_performance_counters WHERE counter_name = 'Target Server Memory (KB)') * 1024 AS TARGETSERVERMEMORY,")
         _T("(SELECT TOP 1 cntr_value FROM sys.dm_os_performance_counters WHERE counter_name = 'Batch Requests/sec') AS BATCHREQUESTS,")
         _T("(SELECT TOP 1 cntr_value FROM sys.dm_os_performance_counters WHERE counter_name = 'SQL Compilations/sec') AS SQLCOMPILATIONS,")
         _T("(SELECT TOP 1 cntr_value FROM sys.dm_os_performance_counters WHERE counter_name = 'SQL Re-Compilations/sec') AS SQLRECOMPILATIONS,")
         _T("(SELECT TOP 1 cntr_value FROM sys.dm_os_performance_counters WHERE counter_name = 'Number of Deadlocks/sec' AND instance_name = '_Total') AS DEADLOCKS,")
         _T("(SELECT TOP 1 cntr_value FROM sys.dm_os_performance_counters WHERE counter_name = 'User Connections') AS USERCONNECTIONS,")
         _T("CASE WHEN (SELECT TOP 1 cntr_value FROM sys.dm_os_performance_counters WHERE counter_name = 'Buffer cache hit ratio base' AND object_name LIKE '%:Buffer Manager%') = 0 THEN 0 ")
         _T("ELSE CONVERT(DECIMAL(5,2), 100.0 * (SELECT TOP 1 cntr_value FROM sys.dm_os_performance_counters WHERE counter_name = 'Buffer cache hit ratio' AND object_name LIKE '%:Buffer Manager%') / ")
         _T("(SELECT TOP 1 cntr_value FROM sys.dm_os_performance_counters WHERE counter_name = 'Buffer cache hit ratio base' AND object_name LIKE '%:Buffer Manager%')) END AS BUFFERCACHEHITRATIO")
   },
   { _T("MEMORY"), 0,
      _T("SELECT physical_memory_in_use_kb * 1024 AS PHYSICALMEMORYINUSE,")
         _T("memory_utilization_percentage AS MEMORYUTILIZATIONPCT,")
         _T("(SELECT total_physical_memory_kb * 1024 FROM sys.dm_os_sys_memory) AS TOTALPHYSICALMEMORY,")
         _T("(SELECT available_physical_memory_kb * 1024 FROM sys.dm_os_sys_memory) AS AVAILABLEPHYSICALMEMORY ")
         _T("FROM sys.dm_os_process_memory")
   },
   { _T("DATABASES"), 1,
      _T("SELECT name AS DBNAME, state_desc AS STATE, recovery_model_desc AS RECOVERYMODEL,")
         _T("user_access_desc AS USERACCESS, CONVERT(int, is_read_only) AS READONLY,")
         _T("compatibility_level AS COMPATLEVEL, log_reuse_wait_desc AS LOGREUSEWAIT,")
         _T("collation_name AS COLLATION, CONVERT(int, is_encrypted) AS ENCRYPTED ")
         _T("FROM sys.databases")
   },
   { _T("DBSPACE"), 1,
      _T("SELECT RTRIM(DB_NAME(database_id)) AS DBNAME,")
         _T("SUM(CASE WHEN type = 0 THEN CONVERT(bigint, size) * 8192 ELSE 0 END) AS DATASIZE,")
         _T("SUM(CASE WHEN type = 1 THEN CONVERT(bigint, size) * 8192 ELSE 0 END) AS LOGSIZE,")
         _T("SUM(CONVERT(bigint, size) * 8192) AS TOTALSIZE ")
         _T("FROM sys.master_files WHERE database_id < 32767 GROUP BY database_id")
   },
   { _T("LOGSPACE"), 1,
      _T("SELECT RTRIM(instance_name) AS DBNAME,")
         _T("MAX(CASE WHEN counter_name = 'Percent Log Used' THEN cntr_value END) AS PERCENTLOGUSED,")
         _T("MAX(CASE WHEN counter_name = 'Log File(s) Used Size (KB)' THEN cntr_value * 1024 END) AS LOGUSEDSIZE,")
         _T("MAX(CASE WHEN counter_name = 'Log File(s) Size (KB)' THEN cntr_value * 1024 END) AS LOGTOTALSIZE ")
         _T("FROM sys.dm_os_performance_counters ")
         _T("WHERE object_name LIKE '%:Databases%' AND instance_name <> '_Total' ")
         _T("AND counter_name IN ('Percent Log Used', 'Log File(s) Used Size (KB)', 'Log File(s) Size (KB)') ")
         _T("GROUP BY instance_name")
   },
   { _T("BACKUP"), 1,
      _T("SELECT d.name AS DBNAME,")
         _T("DATEDIFF(SECOND, MAX(CASE WHEN b.type = 'D' THEN b.backup_finish_date END), GETDATE()) AS FULLBACKUPAGE,")
         _T("DATEDIFF(SECOND, MAX(CASE WHEN b.type = 'I' THEN b.backup_finish_date END), GETDATE()) AS DIFFBACKUPAGE,")
         _T("DATEDIFF(SECOND, MAX(CASE WHEN b.type = 'L' THEN b.backup_finish_date END), GETDATE()) AS LOGBACKUPAGE ")
         _T("FROM sys.databases d LEFT JOIN msdb.dbo.backupset b ON b.database_name = d.name ")
         _T("GROUP BY d.name")
   },
   { _T("WAITSTATS"), 1,
      _T("SELECT wait_type AS WAITTYPE, wait_time_ms AS WAITTIME, waiting_tasks_count AS WAITINGTASKS,")
         _T("signal_wait_time_ms AS SIGNALWAITTIME FROM sys.dm_os_wait_stats ")
         _T("WHERE wait_time_ms > 0 AND wait_type NOT IN ")
         _T("('CLR_SEMAPHORE','LAZYWRITER_SLEEP','RESOURCE_QUEUE','SLEEP_TASK','SLEEP_SYSTEMTASK',")
         _T("'SQLTRACE_BUFFER_FLUSH','WAITFOR','LOGMGR_QUEUE','CHECKPOINT_QUEUE','REQUEST_FOR_DEADLOCK_SEARCH',")
         _T("'XE_TIMER_EVENT','BROKER_TO_FLUSH','BROKER_TASK_STOP','CLR_MANUAL_EVENT','CLR_AUTO_EVENT',")
         _T("'DISPATCHER_QUEUE_SEMAPHORE','FT_IFTS_SCHEDULER_IDLE_WAIT','XE_DISPATCHER_WAIT','XE_DISPATCHER_JOIN',")
         _T("'BROKER_EVENTHANDLER','TRACEWRITE','FT_IFTSHC_MUTEX','SQLTRACE_INCREMENTAL_FLUSH_SLEEP',")
         _T("'BROKER_RECEIVE_WAITFOR','ONDEMAND_TASK_QUEUE','DBMIRROR_EVENTS_QUEUE','DBMIRRORING_CMD',")
         _T("'BROKER_TRANSMITTER','SQLTRACE_WAIT_ENTRIES','SLEEP_BPOOL_FLUSH','SQLTRACE_LOCK',")
         _T("'HADR_FILESTREAM_IOMGR_IOCOMPLETION','DIRTY_PAGE_POLL','SP_SERVER_DIAGNOSTICS_SLEEP')")
   },
   { _T("PERFRATES"), 0,
      // Server-wide performance counters not covered by PERFCOUNTERS. The /sec counters are
      // cumulative (PERF_COUNTER_BULK_COUNT) and exposed as raw values so the server-side DCI
      // computes the per-second rate via delta. Single-row pivot over a single table scan.
      _T("SELECT ")
         _T("MAX(CASE WHEN counter_name = 'Logins/sec' THEN cntr_value END) AS LOGINS,")
         _T("MAX(CASE WHEN counter_name = 'Logouts/sec' THEN cntr_value END) AS LOGOUTS,")
         _T("MAX(CASE WHEN counter_name = 'Page reads/sec' THEN cntr_value END) AS PAGEREADS,")
         _T("MAX(CASE WHEN counter_name = 'Page writes/sec' THEN cntr_value END) AS PAGEWRITES,")
         _T("MAX(CASE WHEN counter_name = 'Checkpoint pages/sec' THEN cntr_value END) AS CHECKPOINTPAGES,")
         _T("MAX(CASE WHEN counter_name = 'Free list stalls/sec' THEN cntr_value END) AS FREELISTSTALLS,")
         _T("MAX(CASE WHEN counter_name = 'Page lookups/sec' THEN cntr_value END) AS PAGELOOKUPS,")
         _T("MAX(CASE WHEN counter_name = 'Lock Waits/sec' AND instance_name = '_Total' THEN cntr_value END) AS LOCKWAITSPERSEC,")
         _T("MAX(CASE WHEN counter_name = 'Lock Timeouts/sec' AND instance_name = '_Total' THEN cntr_value END) AS LOCKTIMEOUTS,")
         _T("MAX(CASE WHEN counter_name = 'Full Scans/sec' THEN cntr_value END) AS FULLSCANS,")
         _T("MAX(CASE WHEN counter_name = 'Page Splits/sec' THEN cntr_value END) AS PAGESPLITS,")
         _T("MAX(CASE WHEN counter_name = 'Forwarded Records/sec' THEN cntr_value END) AS FORWARDEDRECORDS,")
         _T("MAX(CASE WHEN counter_name = 'Workfiles Created/sec' THEN cntr_value END) AS WORKFILESCREATED,")
         _T("MAX(CASE WHEN counter_name = 'Worktables Created/sec' THEN cntr_value END) AS WORKTABLESCREATED,")
         _T("MAX(CASE WHEN counter_name = 'Errors/sec' AND instance_name = '_Total' THEN cntr_value END) AS SQLERRORS,")
         _T("MAX(CASE WHEN counter_name = 'Errors/sec' AND instance_name = 'User Errors' THEN cntr_value END) AS USERERRORS,")
         _T("MAX(CASE WHEN counter_name = 'Memory Grants Pending' THEN cntr_value END) AS MEMORYGRANTSPENDING,")
         _T("MAX(CASE WHEN counter_name = 'Memory Grants Outstanding' THEN cntr_value END) AS MEMORYGRANTSOUTSTANDING,")
         _T("MAX(CASE WHEN counter_name = 'Cache Pages' AND instance_name = '_Total' AND object_name LIKE '%:Plan Cache%' THEN cntr_value END) AS PLANCACHEPAGES,")
         _T("MAX(CASE WHEN counter_name = 'Free Space in tempdb (KB)' THEN cntr_value END) * 1024 AS TEMPDBFREESPACE,")
         _T("MAX(CASE WHEN counter_name = 'Version Store Size (KB)' THEN cntr_value END) * 1024 AS VERSIONSTORESIZE,")
         _T("MAX(CASE WHEN counter_name = 'Transactions/sec' AND instance_name = '_Total' THEN cntr_value END) AS TOTALTRANSACTIONS,")
         _T("MAX(CASE WHEN counter_name = 'Active Transactions' AND instance_name = '_Total' THEN cntr_value END) AS TOTALACTIVETRANSACTIONS,")
         _T("MAX(CASE WHEN counter_name = 'Log Flushes/sec' AND instance_name = '_Total' THEN cntr_value END) AS TOTALLOGFLUSHES,")
         _T("MAX(CASE WHEN counter_name = 'Log Bytes Flushed/sec' AND instance_name = '_Total' THEN cntr_value END) AS TOTALLOGBYTESFLUSHED,")
         // Average Wait Time (ms) is a PERF_AVERAGE_BULK pair; value/base yields the lifetime
         // average wait time per lock wait (same approximation used for buffer cache hit ratio).
         _T("CASE WHEN MAX(CASE WHEN counter_name = 'Average Wait Time Base' AND instance_name = '_Total' THEN cntr_value END) = 0 THEN 0 ")
         _T("ELSE CONVERT(DECIMAL(10,2), 1.0 * MAX(CASE WHEN counter_name = 'Average Wait Time (ms)' AND instance_name = '_Total' THEN cntr_value END) / ")
         _T("MAX(CASE WHEN counter_name = 'Average Wait Time Base' AND instance_name = '_Total' THEN cntr_value END)) END AS AVERAGELOCKWAITTIME ")
         _T("FROM sys.dm_os_performance_counters")
   },
   { _T("DBSTATS"), 1,
      // Per-database transaction/log activity counters from the Databases performance object.
      // The /sec counters are cumulative and the DCI computes the rate.
      _T("SELECT RTRIM(instance_name) AS DBNAME,")
         _T("MAX(CASE WHEN counter_name = 'Active Transactions' THEN cntr_value END) AS ACTIVETRANSACTIONS,")
         _T("MAX(CASE WHEN counter_name = 'Transactions/sec' THEN cntr_value END) AS TRANSACTIONS,")
         _T("MAX(CASE WHEN counter_name = 'Write Transactions/sec' THEN cntr_value END) AS WRITETRANSACTIONS,")
         _T("MAX(CASE WHEN counter_name = 'Log Flushes/sec' THEN cntr_value END) AS LOGFLUSHES,")
         _T("MAX(CASE WHEN counter_name = 'Log Growths' THEN cntr_value END) AS LOGGROWTHS,")
         _T("CASE WHEN MAX(CASE WHEN counter_name = 'Log Cache Hit Ratio Base' THEN cntr_value END) = 0 THEN 0 ")
         _T("ELSE CONVERT(DECIMAL(5,2), 100.0 * MAX(CASE WHEN counter_name = 'Log Cache Hit Ratio' THEN cntr_value END) / ")
         _T("MAX(CASE WHEN counter_name = 'Log Cache Hit Ratio Base' THEN cntr_value END)) END AS LOGCACHEHITRATIO ")
         _T("FROM sys.dm_os_performance_counters ")
         _T("WHERE object_name LIKE '%:Databases%' AND instance_name <> '_Total' ")
         _T("GROUP BY instance_name")
   },
   { _T("RESOURCEPOOL"), 1,
      // Per resource-governor pool CPU usage. CPU usage % is a PERF_AVERAGE_BULK pair, so
      // value/base yields the lifetime average CPU usage for the pool.
      _T("SELECT RTRIM(instance_name) AS POOLNAME,")
         _T("CASE WHEN MAX(CASE WHEN counter_name = 'CPU usage % base' THEN cntr_value END) = 0 THEN 0 ")
         _T("ELSE CONVERT(DECIMAL(5,2), 100.0 * MAX(CASE WHEN counter_name = 'CPU usage %' THEN cntr_value END) / ")
         _T("MAX(CASE WHEN counter_name = 'CPU usage % base' THEN cntr_value END)) END AS CPUUSAGE ")
         _T("FROM sys.dm_os_performance_counters ")
         _T("WHERE object_name LIKE '%:Resource Pool Stats%' ")
         _T("GROUP BY instance_name")
   },
   { _T("WAITCATEGORIES"), 0,
      // Curated rollups from the Wait Statistics performance object. The 'Cumulative wait time
      // (ms) per second' instance holds cumulative wait time in ms per category; the DCI computes
      // the ms/sec rate via delta.
      _T("SELECT ")
         _T("MAX(CASE WHEN counter_name = 'Page IO latch waits' THEN cntr_value END) AS PAGEIOLATCH,")
         _T("MAX(CASE WHEN counter_name = 'Page latch waits' THEN cntr_value END) AS PAGELATCH,")
         _T("MAX(CASE WHEN counter_name = 'Log write waits' THEN cntr_value END) AS LOGWRITE,")
         _T("MAX(CASE WHEN counter_name = 'Log buffer waits' THEN cntr_value END) AS LOGBUFFER,")
         _T("MAX(CASE WHEN counter_name = 'Memory grant queue waits' THEN cntr_value END) AS MEMORYGRANTQUEUE,")
         _T("MAX(CASE WHEN counter_name = 'Network IO waits' THEN cntr_value END) AS NETWORKIO ")
         _T("FROM sys.dm_os_performance_counters ")
         _T("WHERE object_name LIKE '%:Wait Statistics%' AND instance_name = 'Cumulative wait time (ms) per second'")
   },
   { nullptr, 0, nullptr }
};

/**
 * Table query definition: Databases
 */
static TableDescriptor s_tqDatabases =
{
   _T("SELECT d.name AS name, d.state_desc AS state, d.recovery_model_desc AS recovery_model,")
      _T("d.user_access_desc AS user_access, d.compatibility_level AS compat_level,")
      _T("d.log_reuse_wait_desc AS log_reuse_wait, s.data_size AS data_size, s.log_size AS log_size ")
      _T("FROM sys.databases d ")
      _T("LEFT JOIN (SELECT database_id, SUM(CASE WHEN type = 0 THEN CONVERT(bigint, size) * 8192 ELSE 0 END) AS data_size,")
      _T("SUM(CASE WHEN type = 1 THEN CONVERT(bigint, size) * 8192 ELSE 0 END) AS log_size ")
      _T("FROM sys.master_files GROUP BY database_id) s ON s.database_id = d.database_id"),
   {
      { DCI_DT_STRING, _T("Name") },
      { DCI_DT_STRING, _T("State") },
      { DCI_DT_STRING, _T("Recovery model") },
      { DCI_DT_STRING, _T("User access") },
      { DCI_DT_INT, _T("Compatibility level") },
      { DCI_DT_STRING, _T("Log reuse wait") },
      { DCI_DT_INT64, _T("Data size") },
      { DCI_DT_INT64, _T("Log size") }
   }
};

/**
 * Table query definition: Sessions
 */
static TableDescriptor s_tqSessions =
{
   _T("SELECT session_id AS session_id, login_name AS login_name, DB_NAME(database_id) AS database_name,")
      _T("host_name AS host_name, program_name AS program_name, status AS status,")
      _T("cpu_time AS cpu_time, memory_usage * 8192 AS memory_usage,")
      _T("CONVERT(varchar(32), last_request_start_time, 120) AS last_request ")
      _T("FROM sys.dm_exec_sessions WHERE is_user_process = 1"),
   {
      { DCI_DT_INT, _T("Session ID") },
      { DCI_DT_STRING, _T("Login") },
      { DCI_DT_STRING, _T("Database") },
      { DCI_DT_STRING, _T("Host") },
      { DCI_DT_STRING, _T("Program") },
      { DCI_DT_STRING, _T("Status") },
      { DCI_DT_INT64, _T("CPU time") },
      { DCI_DT_INT64, _T("Memory usage") },
      { DCI_DT_STRING, _T("Last request") }
   }
};

/**
 * Table query definition: Wait statistics
 */
static TableDescriptor s_tqWaitStats =
{
   _T("SELECT wait_type AS wait_type, waiting_tasks_count AS waiting_tasks, wait_time_ms AS wait_time,")
      _T("max_wait_time_ms AS max_wait_time, signal_wait_time_ms AS signal_wait_time ")
      _T("FROM sys.dm_os_wait_stats WHERE wait_time_ms > 0 ORDER BY wait_time_ms DESC"),
   {
      { DCI_DT_STRING, _T("Wait type") },
      { DCI_DT_INT64, _T("Waiting tasks") },
      { DCI_DT_INT64, _T("Wait time") },
      { DCI_DT_INT64, _T("Max wait time") },
      { DCI_DT_INT64, _T("Signal wait time") }
   }
};

/**
 * Database instances
 */
static ObjectArray<DatabaseInstance> *s_connections = nullptr;

/**
 * Find instance by ID
 */
static DatabaseInstance *FindInstance(const TCHAR *id)
{
   for(int i = 0; i < s_connections->size(); i++)
   {
      DatabaseInstance *db = s_connections->get(i);
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
   TCHAR id[MAX_DB_STRING];
   if (!AgentGetParameterArg(param, 1, id, MAX_DB_STRING))
      return SYSINFO_RC_UNSUPPORTED;

   DatabaseInstance *db = FindInstance(id);
   if (db == nullptr)
      return SYSINFO_RC_UNSUPPORTED;

   return db->getData(arg, value) ? SYSINFO_RC_SUCCESS : SYSINFO_RC_ERROR;
}

/**
 * Handler for parameters with instance (per database, per wait type)
 */
static LONG H_InstanceParameter(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR id[MAX_DB_STRING];
   if (!AgentGetParameterArg(param, 1, id, MAX_DB_STRING))
      return SYSINFO_RC_UNSUPPORTED;

   DatabaseInstance *db = FindInstance(id);
   if (db == nullptr)
      return SYSINFO_RC_UNSUPPORTED;

   TCHAR instance[MAX_DB_STRING];
   if (!AgentGetParameterArg(param, 2, instance, MAX_DB_STRING))
      return SYSINFO_RC_UNSUPPORTED;

   TCHAR tag[MAX_DB_STRING];
   _sntprintf(tag, MAX_DB_STRING, _T("%s@%s"), arg, instance);
   return db->getData(tag, value) ? SYSINFO_RC_SUCCESS : SYSINFO_RC_ERROR;
}

/**
 * Handler for MSSQL.Server.Version parameter
 */
static LONG H_ServerVersion(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR id[MAX_DB_STRING];
   if (!AgentGetParameterArg(param, 1, id, MAX_DB_STRING))
      return SYSINFO_RC_UNSUPPORTED;

   DatabaseInstance *db = FindInstance(id);
   if (db == nullptr)
      return SYSINFO_RC_UNSUPPORTED;

   db->getVersion(value, MAX_RESULT_LENGTH);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for MSSQL.IsReachable parameter
 */
static LONG H_DatabaseConnectionStatus(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR id[MAX_DB_STRING];
   if (!AgentGetParameterArg(param, 1, id, MAX_DB_STRING))
      return SYSINFO_RC_UNSUPPORTED;

   DatabaseInstance *db = FindInstance(id);
   if (db == nullptr)
      return SYSINFO_RC_UNSUPPORTED;

   ret_string(value, db->isConnected() ? _T("YES") : _T("NO"));
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for MSSQL.Connections list
 */
static LONG H_ConnectionList(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   for(int i = 0; i < s_connections->size(); i++)
      value->add(s_connections->get(i)->getId());
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for MSSQL.AllDatabases list. Rows are emitted in canonical positional
 * form "connectionId,databaseName" so they can be fed directly into the
 * (connectionId, database) metric signature (e.g. MSSQL.Database.DataSize) via
 * instance discovery.
 */
static LONG H_AllDatabasesList(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   for(int i = 0; i < s_connections->size(); i++)
   {
      DatabaseInstance *db = s_connections->get(i);

      StringList list;
      if (!db->getTagList(arg, &list))
         return SYSINFO_RC_ERROR;

      for(int j = 0; j < list.size(); j++)
      {
         TCHAR s[MAX_RESULT_LENGTH];
         _sntprintf(s, MAX_RESULT_LENGTH, _T("%s,%s"), db->getId(), list.get(j));
         value->add(s);
      }
   }
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for generic list parameter
 */
static LONG H_TagList(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   TCHAR id[MAX_DB_STRING];
   if (!AgentGetParameterArg(param, 1, id, MAX_DB_STRING))
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
   TCHAR id[MAX_DB_STRING];
   if (!AgentGetParameterArg(param, 1, id, MAX_DB_STRING))
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
   { _T("ConnectionTTL"),     CT_LONG,   0, 0, 0,               0, &s_dbInfo.connectionTTL },
   { _T("Database"),          CT_STRING, 0, 0, MAX_DB_STRING,   0, s_dbInfo.name },
   { _T("EncryptedPassword"), CT_STRING, 0, 0, MAX_DB_PASSWORD, 0, s_dbInfo.password },
   { _T("Id"),                CT_STRING, 0, 0, MAX_DB_STRING,   0, s_dbInfo.id },
   { _T("Login"),             CT_STRING, 0, 0, MAX_DB_LOGIN,    0, s_dbInfo.login },
   { _T("Password"),          CT_STRING, 0, 0, MAX_DB_PASSWORD, 0, s_dbInfo.password },
   { _T("Server"),            CT_STRING, 0, 0, MAX_DB_STRING,   0, s_dbInfo.server },
   { _T(""), CT_END_OF_LIST, 0, 0, 0, 0, nullptr }
};

/**
 * Reset configuration template to default values
 */
static void ResetDatabaseInfo()
{
   memset(&s_dbInfo, 0, sizeof(s_dbInfo));
   s_dbInfo.connectionTTL = 3600;
   _tcscpy(s_dbInfo.server, _T("127.0.0.1"));
   _tcscpy(s_dbInfo.name, _T("master"));
}

/*
 * Subagent initialization
 */
static bool SubAgentInit(Config *config)
{
   g_mssqlDriver = DBLoadDriver(config->getValue(_T("/MSSQL/Driver"), _T("mssql.ddr")),
         config->getValue(_T("/MSSQL/DriverOptions")), nullptr, nullptr);
   if (g_mssqlDriver == nullptr)
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Cannot load database driver"));
      return false;
   }

   s_connections = new ObjectArray<DatabaseInstance>(8, 8, Ownership::True);

   // Load configuration from "mssql" section to allow simple configuration
   // of one server without XML includes
   ResetDatabaseInfo();
   if (config->getEntry(_T("/mssql/id")) != nullptr || config->getEntry(_T("/mssql/server")) != nullptr ||
       config->getEntry(_T("/mssql/login")) != nullptr || config->getEntry(_T("/mssql/password")) != nullptr)
   {
      if (config->parseTemplate(_T("MSSQL"), s_configTemplate))
      {
         if (s_dbInfo.id[0] == 0)
            _tcscpy(s_dbInfo.id, s_dbInfo.server);
         DecryptPassword(s_dbInfo.login, s_dbInfo.password, s_dbInfo.password, MAX_DB_PASSWORD);
         s_connections->add(new DatabaseInstance(&s_dbInfo));
      }
   }

   // Load full-featured configuration from [mssql/connections/<id>] subsections.
   // Only sections that actually exist are processed, so absent slots do not
   // create phantom instances connecting to the default server address.
   ConfigEntry *connectionsRoot = config->getEntry(_T("/mssql/connections"));
   if (connectionsRoot != nullptr)
   {
      unique_ptr<ObjectArray<ConfigEntry>> connections = connectionsRoot->getSubEntries(_T("*"));
      for(int i = 0; i < connections->size(); i++)
      {
         ConfigEntry *e = connections->get(i);
         ResetDatabaseInfo();
         _tcslcpy(s_dbInfo.id, e->getName(), MAX_DB_STRING);   // Id defaults to subsection name

         TCHAR section[MAX_DB_STRING];
         _sntprintf(section, MAX_DB_STRING, _T("mssql/connections/%s"), e->getName());
         if (!config->parseTemplate(section, s_configTemplate))
         {
            nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Error parsing configuration for SQL Server connection %s"), e->getName());
            continue;
         }

         if (s_dbInfo.id[0] == 0)
            continue;

         DecryptPassword(s_dbInfo.login, s_dbInfo.password, s_dbInfo.password, MAX_DB_PASSWORD);
         s_connections->add(new DatabaseInstance(&s_dbInfo));
      }
   }

   // Exit if no usable configuration found
   if (s_connections->isEmpty())
   {
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("No SQL Server instances to monitor"));
      delete s_connections;
      DBUnloadDriver(g_mssqlDriver);
      return false;
   }

   // Run query thread for each configured server
   for(int i = 0; i < s_connections->size(); i++)
      s_connections->get(i)->run();

   return true;
}

/**
 * Shutdown handler
 */
static void SubAgentShutdown()
{
   nxlog_debug_tag(DEBUG_TAG, 1, _T("Stopping SQL Server pollers"));
   for(int i = 0; i < s_connections->size(); i++)
      s_connections->get(i)->stop();
   delete s_connections;
   DBUnloadDriver(g_mssqlDriver);
   nxlog_debug_tag(DEBUG_TAG, 1, _T("MSSQL subagent stopped"));
}

/**
 * Supported parameters
 */
static NETXMS_SUBAGENT_PARAM s_parameters[] =
{
   { _T("MSSQL.Database.ActiveTransactions(*)"), H_InstanceParameter, _T("DBSTATS/ACTIVETRANSACTIONS"), DCI_DT_INT, _T("MSSQL/Database: {instance} active transactions") },
   { _T("MSSQL.Database.Collation(*)"), H_InstanceParameter, _T("DATABASES/COLLATION"), DCI_DT_STRING, _T("MSSQL/Database: {instance} collation") },
   { _T("MSSQL.Database.CompatibilityLevel(*)"), H_InstanceParameter, _T("DATABASES/COMPATLEVEL"), DCI_DT_INT, _T("MSSQL/Database: {instance} compatibility level") },
   { _T("MSSQL.Database.DataSize(*)"), H_InstanceParameter, _T("DBSPACE/DATASIZE"), DCI_DT_INT64, _T("MSSQL/Database: {instance} data file size") },
   { _T("MSSQL.Database.IsEncrypted(*)"), H_InstanceParameter, _T("DATABASES/ENCRYPTED"), DCI_DT_INT, _T("MSSQL/Database: {instance} is encrypted") },
   { _T("MSSQL.Database.IsReadOnly(*)"), H_InstanceParameter, _T("DATABASES/READONLY"), DCI_DT_INT, _T("MSSQL/Database: {instance} is read only") },
   { _T("MSSQL.Database.LastDiffBackupAge(*)"), H_InstanceParameter, _T("BACKUP/DIFFBACKUPAGE"), DCI_DT_INT64, _T("MSSQL/Database: {instance} time since last differential backup") },
   { _T("MSSQL.Database.LastFullBackupAge(*)"), H_InstanceParameter, _T("BACKUP/FULLBACKUPAGE"), DCI_DT_INT64, _T("MSSQL/Database: {instance} time since last full backup") },
   { _T("MSSQL.Database.LastLogBackupAge(*)"), H_InstanceParameter, _T("BACKUP/LOGBACKUPAGE"), DCI_DT_INT64, _T("MSSQL/Database: {instance} time since last transaction log backup") },
   { _T("MSSQL.Database.LogCacheHitRatio(*)"), H_InstanceParameter, _T("DBSTATS/LOGCACHEHITRATIO"), DCI_DT_FLOAT, _T("MSSQL/Database: {instance} log cache hit ratio (%)") },
   { _T("MSSQL.Database.LogFlushes(*)"), H_InstanceParameter, _T("DBSTATS/LOGFLUSHES"), DCI_DT_COUNTER64, _T("MSSQL/Database: {instance} total transaction log flushes") },
   { _T("MSSQL.Database.LogGrowths(*)"), H_InstanceParameter, _T("DBSTATS/LOGGROWTHS"), DCI_DT_COUNTER64, _T("MSSQL/Database: {instance} transaction log growths") },
   { _T("MSSQL.Database.LogReuseWait(*)"), H_InstanceParameter, _T("DATABASES/LOGREUSEWAIT"), DCI_DT_STRING, _T("MSSQL/Database: {instance} log reuse wait reason") },
   { _T("MSSQL.Database.LogSize(*)"), H_InstanceParameter, _T("DBSPACE/LOGSIZE"), DCI_DT_INT64, _T("MSSQL/Database: {instance} transaction log file size") },
   { _T("MSSQL.Database.LogUsedSize(*)"), H_InstanceParameter, _T("LOGSPACE/LOGUSEDSIZE"), DCI_DT_INT64, _T("MSSQL/Database: {instance} used transaction log size") },
   { _T("MSSQL.Database.PercentLogUsed(*)"), H_InstanceParameter, _T("LOGSPACE/PERCENTLOGUSED"), DCI_DT_INT, _T("MSSQL/Database: {instance} transaction log used (%)") },
   { _T("MSSQL.Database.RecoveryModel(*)"), H_InstanceParameter, _T("DATABASES/RECOVERYMODEL"), DCI_DT_STRING, _T("MSSQL/Database: {instance} recovery model") },
   { _T("MSSQL.Database.Status(*)"), H_InstanceParameter, _T("DATABASES/STATE"), DCI_DT_STRING, _T("MSSQL/Database: {instance} status") },
   { _T("MSSQL.Database.TotalSize(*)"), H_InstanceParameter, _T("DBSPACE/TOTALSIZE"), DCI_DT_INT64, _T("MSSQL/Database: {instance} total file size") },
   { _T("MSSQL.Database.Transactions(*)"), H_InstanceParameter, _T("DBSTATS/TRANSACTIONS"), DCI_DT_COUNTER64, _T("MSSQL/Database: {instance} total transactions") },
   { _T("MSSQL.Database.UserAccess(*)"), H_InstanceParameter, _T("DATABASES/USERACCESS"), DCI_DT_STRING, _T("MSSQL/Database: {instance} user access mode") },
   { _T("MSSQL.Database.WriteTransactions(*)"), H_InstanceParameter, _T("DBSTATS/WRITETRANSACTIONS"), DCI_DT_COUNTER64, _T("MSSQL/Database: {instance} total write transactions") },
   { _T("MSSQL.IsReachable(*)"), H_DatabaseConnectionStatus, nullptr, DCI_DT_STRING, _T("MSSQL: is server reachable") },
   { _T("MSSQL.Memory.AvailablePhysicalMemory(*)"), H_GlobalParameter, _T("MEMORY/AVAILABLEPHYSICALMEMORY"), DCI_DT_INT64, _T("MSSQL/Memory: available physical memory") },
   { _T("MSSQL.Memory.BufferCacheHitRatio(*)"), H_GlobalParameter, _T("PERFCOUNTERS/BUFFERCACHEHITRATIO"), DCI_DT_FLOAT, _T("MSSQL/Memory: buffer cache hit ratio (%)") },
   { _T("MSSQL.Memory.MemoryGrantsOutstanding(*)"), H_GlobalParameter, _T("PERFRATES/MEMORYGRANTSOUTSTANDING"), DCI_DT_INT, _T("MSSQL/Memory: outstanding memory grants") },
   { _T("MSSQL.Memory.MemoryGrantsPending(*)"), H_GlobalParameter, _T("PERFRATES/MEMORYGRANTSPENDING"), DCI_DT_INT, _T("MSSQL/Memory: pending memory grants") },
   { _T("MSSQL.Memory.MemoryUtilization(*)"), H_GlobalParameter, _T("MEMORY/MEMORYUTILIZATIONPCT"), DCI_DT_INT, _T("MSSQL/Memory: memory utilization (%)") },
   { _T("MSSQL.Memory.PageLifeExpectancy(*)"), H_GlobalParameter, _T("PERFCOUNTERS/PAGELIFEEXPECTANCY"), DCI_DT_INT64, _T("MSSQL/Memory: page life expectancy (seconds)") },
   { _T("MSSQL.Memory.PhysicalMemoryInUse(*)"), H_GlobalParameter, _T("MEMORY/PHYSICALMEMORYINUSE"), DCI_DT_INT64, _T("MSSQL/Memory: physical memory in use") },
   { _T("MSSQL.Memory.PlanCachePages(*)"), H_GlobalParameter, _T("PERFRATES/PLANCACHEPAGES"), DCI_DT_INT64, _T("MSSQL/Memory: plan cache pages") },
   { _T("MSSQL.Memory.TargetServerMemory(*)"), H_GlobalParameter, _T("PERFCOUNTERS/TARGETSERVERMEMORY"), DCI_DT_INT64, _T("MSSQL/Memory: target server memory") },
   { _T("MSSQL.Memory.TotalServerMemory(*)"), H_GlobalParameter, _T("PERFCOUNTERS/TOTALSERVERMEMORY"), DCI_DT_INT64, _T("MSSQL/Memory: total server memory") },
   { _T("MSSQL.ResourcePool.CPUUsage(*)"), H_InstanceParameter, _T("RESOURCEPOOL/CPUUSAGE"), DCI_DT_FLOAT, _T("MSSQL/ResourcePool: {instance} CPU usage (%)") },
   { _T("MSSQL.Server.ActiveSessions(*)"), H_GlobalParameter, _T("GLOBALSTATS/ACTIVESESSIONS"), DCI_DT_INT, _T("MSSQL/Server: number of active user sessions") },
   { _T("MSSQL.Server.ActiveTransactions(*)"), H_GlobalParameter, _T("PERFRATES/TOTALACTIVETRANSACTIONS"), DCI_DT_INT, _T("MSSQL/Server: total active transactions") },
   { _T("MSSQL.Server.AverageLockWaitTime(*)"), H_GlobalParameter, _T("PERFRATES/AVERAGELOCKWAITTIME"), DCI_DT_FLOAT, _T("MSSQL/Server: average lock wait time (ms)") },
   { _T("MSSQL.Server.BatchRequests(*)"), H_GlobalParameter, _T("PERFCOUNTERS/BATCHREQUESTS"), DCI_DT_COUNTER64, _T("MSSQL/Server: total batch requests") },
   { _T("MSSQL.Server.BlockedProcesses(*)"), H_GlobalParameter, _T("GLOBALSTATS/BLOCKEDPROCESSES"), DCI_DT_INT, _T("MSSQL/Server: number of blocked processes") },
   { _T("MSSQL.Server.CheckpointPages(*)"), H_GlobalParameter, _T("PERFRATES/CHECKPOINTPAGES"), DCI_DT_COUNTER64, _T("MSSQL/Server: total checkpoint pages") },
   { _T("MSSQL.Server.Connections(*)"), H_GlobalParameter, _T("GLOBALSTATS/CONNECTIONS"), DCI_DT_INT, _T("MSSQL/Server: number of connections") },
   { _T("MSSQL.Server.Deadlocks(*)"), H_GlobalParameter, _T("PERFCOUNTERS/DEADLOCKS"), DCI_DT_COUNTER64, _T("MSSQL/Server: total number of deadlocks") },
   { _T("MSSQL.Server.Edition(*)"), H_GlobalParameter, _T("SERVERINFO/EDITION"), DCI_DT_STRING, _T("MSSQL/Server: edition") },
   { _T("MSSQL.Server.ForwardedRecords(*)"), H_GlobalParameter, _T("PERFRATES/FORWARDEDRECORDS"), DCI_DT_COUNTER64, _T("MSSQL/Server: total forwarded records") },
   { _T("MSSQL.Server.FreeListStalls(*)"), H_GlobalParameter, _T("PERFRATES/FREELISTSTALLS"), DCI_DT_COUNTER64, _T("MSSQL/Server: total free list stalls") },
   { _T("MSSQL.Server.FullScans(*)"), H_GlobalParameter, _T("PERFRATES/FULLSCANS"), DCI_DT_COUNTER64, _T("MSSQL/Server: total full scans") },
   { _T("MSSQL.Server.LockTimeouts(*)"), H_GlobalParameter, _T("PERFRATES/LOCKTIMEOUTS"), DCI_DT_COUNTER64, _T("MSSQL/Server: total lock timeouts") },
   { _T("MSSQL.Server.LockWaits(*)"), H_GlobalParameter, _T("GLOBALSTATS/LOCKWAITS"), DCI_DT_INT, _T("MSSQL/Server: number of lock waits") },
   { _T("MSSQL.Server.LockWaitsPerSec(*)"), H_GlobalParameter, _T("PERFRATES/LOCKWAITSPERSEC"), DCI_DT_COUNTER64, _T("MSSQL/Server: total lock waits") },
   { _T("MSSQL.Server.LogBufferWaitTime(*)"), H_GlobalParameter, _T("WAITCATEGORIES/LOGBUFFER"), DCI_DT_COUNTER64, _T("MSSQL/Server: cumulative log buffer wait time (ms)") },
   { _T("MSSQL.Server.LogBytesFlushed(*)"), H_GlobalParameter, _T("PERFRATES/TOTALLOGBYTESFLUSHED"), DCI_DT_COUNTER64, _T("MSSQL/Server: total log bytes flushed") },
   { _T("MSSQL.Server.LogFlushes(*)"), H_GlobalParameter, _T("PERFRATES/TOTALLOGFLUSHES"), DCI_DT_COUNTER64, _T("MSSQL/Server: total transaction log flushes") },
   { _T("MSSQL.Server.Logins(*)"), H_GlobalParameter, _T("PERFRATES/LOGINS"), DCI_DT_COUNTER64, _T("MSSQL/Server: total logins") },
   { _T("MSSQL.Server.Logouts(*)"), H_GlobalParameter, _T("PERFRATES/LOGOUTS"), DCI_DT_COUNTER64, _T("MSSQL/Server: total logouts") },
   { _T("MSSQL.Server.LogWriteWaitTime(*)"), H_GlobalParameter, _T("WAITCATEGORIES/LOGWRITE"), DCI_DT_COUNTER64, _T("MSSQL/Server: cumulative log write wait time (ms)") },
   { _T("MSSQL.Server.MemoryGrantQueueWaitTime(*)"), H_GlobalParameter, _T("WAITCATEGORIES/MEMORYGRANTQUEUE"), DCI_DT_COUNTER64, _T("MSSQL/Server: cumulative memory grant queue wait time (ms)") },
   { _T("MSSQL.Server.NetworkIOWaitTime(*)"), H_GlobalParameter, _T("WAITCATEGORIES/NETWORKIO"), DCI_DT_COUNTER64, _T("MSSQL/Server: cumulative network IO wait time (ms)") },
   { _T("MSSQL.Server.PageIOLatchWaitTime(*)"), H_GlobalParameter, _T("WAITCATEGORIES/PAGEIOLATCH"), DCI_DT_COUNTER64, _T("MSSQL/Server: cumulative page IO latch wait time (ms)") },
   { _T("MSSQL.Server.PageLatchWaitTime(*)"), H_GlobalParameter, _T("WAITCATEGORIES/PAGELATCH"), DCI_DT_COUNTER64, _T("MSSQL/Server: cumulative page latch wait time (ms)") },
   { _T("MSSQL.Server.PageLookups(*)"), H_GlobalParameter, _T("PERFRATES/PAGELOOKUPS"), DCI_DT_COUNTER64, _T("MSSQL/Server: total page lookups") },
   { _T("MSSQL.Server.PageReads(*)"), H_GlobalParameter, _T("PERFRATES/PAGEREADS"), DCI_DT_COUNTER64, _T("MSSQL/Server: total page reads") },
   { _T("MSSQL.Server.PageSplits(*)"), H_GlobalParameter, _T("PERFRATES/PAGESPLITS"), DCI_DT_COUNTER64, _T("MSSQL/Server: total page splits") },
   { _T("MSSQL.Server.PageWrites(*)"), H_GlobalParameter, _T("PERFRATES/PAGEWRITES"), DCI_DT_COUNTER64, _T("MSSQL/Server: total page writes") },
   { _T("MSSQL.Server.ProductLevel(*)"), H_GlobalParameter, _T("SERVERINFO/PRODUCTLEVEL"), DCI_DT_STRING, _T("MSSQL/Server: product level") },
   { _T("MSSQL.Server.SQLCompilations(*)"), H_GlobalParameter, _T("PERFCOUNTERS/SQLCOMPILATIONS"), DCI_DT_COUNTER64, _T("MSSQL/Server: total SQL compilations") },
   { _T("MSSQL.Server.SQLErrors(*)"), H_GlobalParameter, _T("PERFRATES/SQLERRORS"), DCI_DT_COUNTER64, _T("MSSQL/Server: total SQL errors") },
   { _T("MSSQL.Server.SQLRecompilations(*)"), H_GlobalParameter, _T("PERFCOUNTERS/SQLRECOMPILATIONS"), DCI_DT_COUNTER64, _T("MSSQL/Server: total SQL re-compilations") },
   { _T("MSSQL.Server.TempdbFreeSpace(*)"), H_GlobalParameter, _T("PERFRATES/TEMPDBFREESPACE"), DCI_DT_INT64, _T("MSSQL/Server: free space in tempdb") },
   { _T("MSSQL.Server.Transactions(*)"), H_GlobalParameter, _T("PERFRATES/TOTALTRANSACTIONS"), DCI_DT_COUNTER64, _T("MSSQL/Server: total transactions") },
   { _T("MSSQL.Server.Uptime(*)"), H_GlobalParameter, _T("SERVERINFO/UPTIME"), DCI_DT_INT64, _T("MSSQL/Server: uptime in seconds") },
   { _T("MSSQL.Server.UserConnections(*)"), H_GlobalParameter, _T("PERFCOUNTERS/USERCONNECTIONS"), DCI_DT_INT, _T("MSSQL/Server: number of user connections") },
   { _T("MSSQL.Server.UserErrors(*)"), H_GlobalParameter, _T("PERFRATES/USERERRORS"), DCI_DT_COUNTER64, _T("MSSQL/Server: total user errors") },
   { _T("MSSQL.Server.UserSessions(*)"), H_GlobalParameter, _T("GLOBALSTATS/USERSESSIONS"), DCI_DT_INT, _T("MSSQL/Server: number of user sessions") },
   { _T("MSSQL.Server.Version(*)"), H_ServerVersion, nullptr, DCI_DT_STRING, _T("MSSQL/Server: product version") },
   { _T("MSSQL.Server.VersionStoreSize(*)"), H_GlobalParameter, _T("PERFRATES/VERSIONSTORESIZE"), DCI_DT_INT64, _T("MSSQL/Server: version store size in tempdb") },
   { _T("MSSQL.Server.WorkfilesCreated(*)"), H_GlobalParameter, _T("PERFRATES/WORKFILESCREATED"), DCI_DT_COUNTER64, _T("MSSQL/Server: total workfiles created") },
   { _T("MSSQL.Server.WorktablesCreated(*)"), H_GlobalParameter, _T("PERFRATES/WORKTABLESCREATED"), DCI_DT_COUNTER64, _T("MSSQL/Server: total worktables created") },
   { _T("MSSQL.WaitStats.SignalWaitTime(*)"), H_InstanceParameter, _T("WAITSTATS/SIGNALWAITTIME"), DCI_DT_COUNTER64, _T("MSSQL/WaitStats: {instance} signal wait time") },
   { _T("MSSQL.WaitStats.WaitingTasks(*)"), H_InstanceParameter, _T("WAITSTATS/WAITINGTASKS"), DCI_DT_COUNTER64, _T("MSSQL/WaitStats: {instance} waiting tasks count") },
   { _T("MSSQL.WaitStats.WaitTime(*)"), H_InstanceParameter, _T("WAITSTATS/WAITTIME"), DCI_DT_COUNTER64, _T("MSSQL/WaitStats: {instance} wait time") }
};

/**
 * Supported lists
 */
static NETXMS_SUBAGENT_LIST s_lists[] =
{
   { _T("MSSQL.AllDatabases"), H_AllDatabasesList, _T("^DATABASES/STATE@(.*)$"), _T("MSSQL: all databases on all monitored connections") },
   { _T("MSSQL.Connections"), H_ConnectionList, nullptr, _T("MSSQL: configured server connections") },
   { _T("MSSQL.Databases(*)"), H_TagList, _T("^DATABASES/STATE@(.*)$"), _T("MSSQL: databases on monitored server") },
   { _T("MSSQL.DataTags(*)"), H_TagList, _T("^(.*)$"), _T("MSSQL: data collection tags") },
   { _T("MSSQL.ResourcePools(*)"), H_TagList, _T("^RESOURCEPOOL/CPUUSAGE@(.*)$"), _T("MSSQL: resource governor pools on monitored server") },
   { _T("MSSQL.WaitTypes(*)"), H_TagList, _T("^WAITSTATS/WAITTIME@(.*)$"), _T("MSSQL: active wait types on monitored server") }
};

/**
 * Supported tables
 */
static NETXMS_SUBAGENT_TABLE s_tables[] =
{
   { _T("MSSQL.Databases(*)"), H_TableQuery, (const TCHAR *)&s_tqDatabases, _T("NAME"), _T("MSSQL: databases") },
   { _T("MSSQL.Sessions(*)"), H_TableQuery, (const TCHAR *)&s_tqSessions, _T("SESSION_ID"), _T("MSSQL: user sessions") },
   { _T("MSSQL.WaitStats(*)"), H_TableQuery, (const TCHAR *)&s_tqWaitStats, _T("WAIT_TYPE"), _T("MSSQL: wait statistics") }
};

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO s_info =
{
   NETXMS_SUBAGENT_INFO_MAGIC,
   _T("MSSQL"), NETXMS_VERSION_STRING,
   SubAgentInit, SubAgentShutdown, nullptr, nullptr, nullptr,
   sizeof(s_parameters) / sizeof(NETXMS_SUBAGENT_PARAM), s_parameters,
   sizeof(s_lists) / sizeof(NETXMS_SUBAGENT_LIST), s_lists,
   sizeof(s_tables) / sizeof(NETXMS_SUBAGENT_TABLE), s_tables,
   0, nullptr,    // actions
   0, nullptr     // push parameters
};

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(MSSQL)
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
