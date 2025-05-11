/*
 ** NetXMS - Network Management System
 ** Subagent for PostgreSQL monitoring
 ** Copyright (C) 2009-2025 Raden Solutions
 ** Copyright (C) 2020 Petr Votava
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

#include "pgsql_subagent.h"
#include <netxms-version.h>

/**
 * Driver handle
 */
DB_DRIVER g_pgsqlDriver = nullptr;

/**
 * Polling queries
 */
DatabaseQuery g_queries[] =
{
	{ _T("DB_STAT"), MAKE_PGSQL_VERSION(12, 0, 0), 0, 1,
		_T("SELECT d.datname, CASE WHEN has_database_privilege(current_user, d.datname, 'CONNECT') THEN pg_catalog.pg_database_size(d.datname) ELSE NULL END AS size, s.numbackends, s.deadlocks, ")
			_T("round(s.blks_hit::numeric*100/(s.blks_hit+s.blks_read), 2) CacheHitRatio, s.xact_commit, s.xact_rollback, ")
			_T("s.tup_inserted, s.tup_updated, s.tup_deleted, s.tup_fetched, s.tup_returned, s.blks_read, s.blks_hit, ")
			_T("s.conflicts, s.temp_files, s.temp_bytes, s.blk_read_time, s.blk_write_time, ")
			_T("s.checksum_failures ")
			_T("FROM  pg_catalog.pg_database d LEFT JOIN pg_catalog.pg_stat_database s ON d.oid = s.datid ")
			_T("WHERE NOT d.datistemplate AND d.datallowconn AND s.blks_read >0")
	},
	{ _T("DB_STAT"), 0, MAKE_PGSQL_VERSION(12, 0, 0), 1,
		_T("SELECT d.datname, CASE WHEN has_database_privilege(current_user, d.datname, 'CONNECT') THEN pg_catalog.pg_database_size(d.datname) ELSE NULL END AS size, s.numbackends, s.deadlocks, ")
			_T("round(s.blks_hit::numeric*100/(s.blks_hit+s.blks_read), 2) CacheHitRatio, s.xact_commit, s.xact_rollback, ")
			_T("s.tup_inserted, s.tup_updated, s.tup_deleted, s.tup_fetched, s.tup_returned, s.blks_read, s.blks_hit, ")
			_T("s.conflicts, s.temp_files, s.temp_bytes, s.blk_read_time, s.blk_write_time ")
			_T("FROM  pg_catalog.pg_database d LEFT JOIN pg_catalog.pg_stat_database s ON d.oid = s.datid ")
			_T("WHERE NOT d.datistemplate AND d.datallowconn AND s.blks_read >0")
	},
	{ _T("DB_CONNECTIONS"), MAKE_PGSQL_VERSION(9, 6, 0), 0, 1,
		_T("SELECT datname, count(*) AS total, count(*) FILTER (WHERE state = 'active') AS active, count(*) FILTER (WHERE state = 'active') AS idle, ")
			_T("count(*) FILTER (WHERE state = 'idle in transaction') AS idle_in_transaction, count(*) FILTER (WHERE state = 'idle in transaction (aborted)') AS idle_in_transaction_aborted, ")
			_T("count(*) FILTER (WHERE state = 'fastpath function call') AS fastpath_function_call, greatest(max(age(backend_xmin)), max(age(backend_xid))) AS oldestxid, ")
			_T("count(*) FILTER (WHERE state <> 'idle' AND query like '%autovacuum%') AS autovacuum, ")
			_T("count(*) FILTER (WHERE wait_event IS NOT NULL ) AS waiting ") // change in v9.6
			_T("FROM pg_catalog.pg_stat_activity WHERE datid is not NULL GROUP BY datname")
	},
	{ _T("DB_CONNECTIONS"), 0, MAKE_PGSQL_VERSION(9, 6, 0), 1,
		_T("SELECT datname, count(*) AS total, count(*) FILTER (WHERE state = 'active') AS active, count(*) FILTER (WHERE state = 'active') AS idle, ")
			_T("count(*) FILTER (WHERE state = 'idle in transaction') AS idle_in_transaction, count(*) FILTER (WHERE state = 'idle in transaction (aborted)') AS idle_in_transaction_aborted, ")
			_T("count(*) FILTER (WHERE state = 'fastpath function call') AS fastpath_function_call, greatest(max(age(backend_xmin)), max(age(backend_xid))) AS oldestxid, ")
			_T("count(*) FILTER (WHERE state <> 'idle' AND query like '%autovacuum%') AS autovacuum, ")
			_T("count(*) FILTER (WHERE waiting ) AS waiting ")
			_T("FROM pg_catalog.pg_stat_activity WHERE datid is not NULL GROUP BY datname")
	},
	{ _T("CONNECTIONS"), 0, 0, 0,
		_T("SELECT count(*) AS total, pg_catalog.current_setting('max_connections')::int AS max, count(*)*100/(SELECT pg_catalog.current_setting('max_connections')::int) AS total_pct, ")
			_T("pg_catalog.current_setting('autovacuum_max_workers')::int AS autovacuum_max ")
			_T("FROM pg_catalog.pg_stat_activity WHERE datid is not NULL")
	},
	{ _T("DB_TRANSACTIONS"), 0, 0, 1,
		_T("SELECT database, count(*) AS prepared FROM pg_catalog.pg_prepared_xacts GROUP BY database")
	},
	{ _T("DB_LOCKS"), 0, 0, 1,
		_T("SELECT d.datname, ")
			_T("count(*) FILTER (WHERE MODE IS NOT NULL) AS total, ")
			_T("count(*) FILTER (WHERE MODE = 'AccessExclusiveLock') AS accessexclusive, ")
			_T("count(*) FILTER (WHERE MODE = 'AccessShareLock') AS accessshare, ")
			_T("count(*) FILTER (WHERE MODE = 'ExclusiveLock') AS exclusive, ")
			_T("count(*) FILTER (WHERE MODE = 'RowExclusiveLock') AS rowexclusive, ")
			_T("count(*) FILTER (WHERE MODE = 'RowShareLock') AS rowshare, ")
			_T("count(*) FILTER (WHERE MODE = 'ShareLock') AS share, ")
			_T("count(*) FILTER (WHERE MODE = 'ShareRowExclusiveLock') AS sharerowexclusive, ")
			_T("count(*) FILTER (WHERE MODE = 'ShareUpdateExclusiveLock') AS shareupdateexclusive ")
			_T("FROM pg_catalog.pg_locks l RIGHT JOIN pg_catalog.pg_database d ON d.oid = l.database WHERE NOT datistemplate AND datallowconn GROUP BY d.datname")
	},
	{ _T("BGWRITER"), MAKE_PGSQL_VERSION(17, 0, 0), 0, 0,
		_T("SELECT c.num_timed AS checkpoints_timed, c.num_requested AS checkpoints_req, ")
			_T("c.write_time AS checkpoint_write_time, c.sync_time AS checkpoint_sync_time, ")
			_T("c.buffers_written AS buffers_checkpoint, b.buffers_clean, b.maxwritten_clean, ")
			_T("(SELECT buffers_written FROM pg_stat_io WHERE context = 'backend') AS buffers_backend, ")
			_T("(SELECT fsyncs FROM pg_stat_io WHERE context = 'backend') AS buffers_backend_fsync, ")
			_T("b.buffers_alloc FROM pg_stat_checkpointer c, pg_stat_bgwriter b")
	},
	{ _T("BGWRITER"), 0, MAKE_PGSQL_VERSION(17, 0, 0), 0,
		_T("SELECT checkpoints_timed, checkpoints_req, checkpoint_write_time, checkpoint_sync_time, buffers_checkpoint, buffers_clean, maxwritten_clean, ")
			_T("buffers_backend, buffers_backend_fsync, buffers_alloc FROM pg_catalog.pg_stat_bgwriter")
	},
	{ _T("ARCHIVER"), 0, 0, 0,
		_T("SELECT archived_count, last_archived_wal, extract(epoch from current_timestamp - last_archived_time)::int as last_archived_age, ")
			_T("failed_count, last_failed_wal, ")
			_T("CASE WHEN (last_failed_wal IS NULL OR last_failed_wal <= last_archived_wal) THEN NULL ELSE extract(epoch from current_timestamp - last_failed_time)::int END AS last_failed_age, ")
			_T("CASE WHEN (pg_catalog.current_setting('archive_mode')::boolean AND ( last_failed_wal IS NULL OR last_failed_wal <= last_archived_wal)) THEN 'YES' ELSE 'NO' END AS is_archiving ")
			_T("FROM pg_catalog.pg_stat_archiver")
	},
	{ _T("REPLICATION"), MAKE_PGSQL_VERSION(9, 6, 0), 0, 0,
		_T("SELECT CASE WHEN pg_is_in_recovery() THEN 'YES' ELSE 'NO' END AS in_recovery, CASE WHEN count(*) > 0 THEN 'YES' ELSE 'NO' END AS is_receiver FROM pg_catalog.pg_stat_wal_receiver")
	},
	{ _T("REPLICATION"), 0, 0, 0,
		_T("SELECT count(*) wal_senders FROM pg_catalog.pg_stat_replication")
	},
	{ _T("REPLICATION"), MAKE_PGSQL_VERSION(10, 0, 0), 0, 0,
		_T("SELECT CASE WHEN pg_catalog.pg_last_wal_receive_lsn() = pg_catalog.pg_last_wal_replay_lsn() THEN 0 ELSE COALESCE(EXTRACT(EPOCH FROM now() - pg_catalog.pg_last_xact_replay_timestamp())::integer, 0) END AS lag")
	},
	{ _T("REPLICATION"), MAKE_PGSQL_VERSION(13, 0, 0), 0, 0,
		_T("SELECT pg_catalog.pg_wal_lsn_diff (flushed_lsn, pg_catalog.pg_last_wal_replay_lsn()) AS lag_bytes FROM pg_catalog.pg_stat_wal_receiver")
	},
	{ _T("REPLICATION"), MAKE_PGSQL_VERSION(10, 0, 0), MAKE_PGSQL_VERSION(13, 0, 0), 0,
		_T("SELECT pg_catalog.pg_wal_lsn_diff (received_lsn, pg_catalog.pg_last_wal_replay_lsn()) AS lag_bytes FROM pg_catalog.pg_stat_wal_receiver")
	},
	{ _T("REPLICATION"), MAKE_PGSQL_VERSION(10, 0, 0), 0, 0,
		_T("SELECT pg_catalog.pg_wal_lsn_diff(pg_current_wal_lsn(),'0/00000000') AS xlog_size, count(*) AS xlog_files FROM pg_catalog.pg_ls_waldir()")
	},
	{ nullptr, 0, 0, 0, nullptr }
};

/**
 * Table query definitions: Backends activity
 */
static TableDescriptor s_tqBackends[] =
{
	{
		MAKE_PGSQL_VERSION(10, 0, 0),
		_T("SELECT pid, datname, usename, application_name, client_addr, ")
			_T("to_char(backend_start, 'YYYY-MM-DD HH24:MI:SS TZ') AS backend_start, ")
			_T("state, ")
			_T("wait_event_type || ': ' || wait_event AS wait_event, ")
			_T("array_to_string(pg_blocking_pids(pid), ',') AS blocking_pids, ")
			_T("backend_type, ")  // new in v10
			_T("to_char(query_start, 'YYYY-MM-DD HH24:MI:SS TZ') AS query_start, ")
			_T("to_char(state_change, 'YYYY-MM-DD HH24:MI:SS TZ') AS state_change, ")
			_T("query ")
			_T("FROM pg_catalog.pg_stat_activity WHERE pid != pg_catalog.pg_backend_pid() ORDER BY pid"),
		{
			{ DCI_DT_INT64, _T("PID") },
			{ DCI_DT_STRING, _T("Database") },
			{ DCI_DT_STRING, _T("User") },
			{ DCI_DT_STRING, _T("Application") },
			{ DCI_DT_STRING, _T("Client") },
			{ DCI_DT_STRING, _T("Backend start") },
			{ DCI_DT_STRING, _T("State") },
			{ DCI_DT_STRING, _T("Wait event") },
			{ DCI_DT_STRING, _T("Blocking PIDs") },
			{ DCI_DT_STRING, _T("Backend type") },
			{ DCI_DT_STRING, _T("Query start") },
			{ DCI_DT_STRING, _T("Last state change") },
			{ DCI_DT_STRING, _T("SQL") }
		}
	},
	{
		MAKE_PGSQL_VERSION(9, 6, 0),
		_T("SELECT pid, datname, usename, application_name, client_addr, ")
			_T("to_char(backend_start, 'YYYY-MM-DD HH24:MI:SS TZ') AS backend_start, ")
			_T("state, ")
			_T("wait_event_type || ': ' || wait_event AS wait_event, ") // change in v9.6
			_T("array_to_string(pg_blocking_pids(pid), ',') AS blocking_pids, ") // new in v9.6
			_T("to_char(query_start, 'YYYY-MM-DD HH24:MI:SS TZ') AS query_start, ")
			_T("to_char(state_change, 'YYYY-MM-DD HH24:MI:SS TZ') AS state_change, ")
			_T("query ")
			_T("FROM pg_catalog.pg_stat_activity WHERE pid != pg_catalog.pg_backend_pid() ORDER BY pid"),
		{
			{ DCI_DT_INT64, _T("PID") },
			{ DCI_DT_STRING, _T("Database") },
			{ DCI_DT_STRING, _T("User") },
			{ DCI_DT_STRING, _T("Application") },
			{ DCI_DT_STRING, _T("Client") },
			{ DCI_DT_STRING, _T("Backend start") },
			{ DCI_DT_STRING, _T("State") },
			{ DCI_DT_STRING, _T("Wait event") },
			{ DCI_DT_STRING, _T("Blocking PIDs") },
			{ DCI_DT_STRING, _T("Query start") },
			{ DCI_DT_STRING, _T("Last state change") },
			{ DCI_DT_STRING, _T("SQL") }
		}
	},
	{
		0,
		_T("SELECT pid, datname, usename, application_name, client_addr, ")
			_T("to_char(backend_start, 'YYYY-MM-DD HH24:MI:SS TZ') AS backend_start, ")
			_T("state, ")
			_T("CASE WHEN waiting THEN 'YES' ELSE 'NO' END AS waiting, ")
			_T("to_char(query_start, 'YYYY-MM-DD HH24:MI:SS TZ') AS query_start, ")
			_T("to_char(state_change, 'YYYY-MM-DD HH24:MI:SS TZ') AS state_change, ")
			_T("query ")
			_T("FROM pg_catalog.pg_stat_activity WHERE pid != pg_catalog.pg_backend_pid() ORDER BY pid"),
		{
			{ DCI_DT_INT64, _T("PID") },
			{ DCI_DT_STRING, _T("Database") },
			{ DCI_DT_STRING, _T("User") },
			{ DCI_DT_STRING, _T("Application") },
			{ DCI_DT_STRING, _T("Client") },
			{ DCI_DT_STRING, _T("Backend start") },
			{ DCI_DT_STRING, _T("State") },
			{ DCI_DT_STRING, _T("Waiting?") },
			{ DCI_DT_STRING, _T("Query start") },
			{ DCI_DT_STRING, _T("Last state change") },
			{ DCI_DT_STRING, _T("SQL") }
		}
	}
};

/**
 * Table query definition: Locks
 */
static TableDescriptor s_tqLocks[] =
{
	{
		0,
		_T("SELECT pid, datname, locktype, relation::regclass, page, tuple, virtualxid, transactionid, ")
			_T("classid::regclass, objid, virtualtransaction, mode, ")
			_T("CASE WHEN granted THEN 'YES' ELSE 'NO' END AS granted, ")
			_T("CASE WHEN fastpath THEN 'YES' ELSE 'NO' END AS fastpath ")
			_T("FROM pg_catalog.pg_locks l LEFT OUTER JOIN pg_catalog.pg_database d ON (l.database = d.oid) ")
			_T("WHERE pid != pg_backend_pid() ORDER BY pid, locktype"),
		{
			{ DCI_DT_INT64, _T("PID") },
			{ DCI_DT_STRING, _T("Database") },
			{ DCI_DT_STRING, _T("Lock type") },
			{ DCI_DT_STRING, _T("Target relation") },
			{ DCI_DT_STRING, _T("Page") },
			{ DCI_DT_STRING, _T("Tuple") },
			{ DCI_DT_STRING, _T("vXID (target)") },
			{ DCI_DT_STRING, _T("XID (target)") },
			{ DCI_DT_STRING, _T("Class") },
			{ DCI_DT_STRING, _T("Object ID") },
			{ DCI_DT_STRING, _T("vXID (owner)") },
			{ DCI_DT_STRING, _T("Mode") },
			{ DCI_DT_STRING, _T("Granted?") },
			{ DCI_DT_STRING, _T("Fastpath?") }
		}
	}
};

/**
 * Table query definition: Prepared Transactions
 */
static TableDescriptor s_tqPrepared[] =
{
	{
		0,
		_T("SELECT gid, database, owner, transaction, ")
			_T("to_char(prepared, 'YYYY-MM-DD HH24:MI:SS TZ') AS prepared ")
			_T("FROM pg_catalog.pg_prepared_xacts ORDER BY gid, database, owner"),
		{
			{ DCI_DT_INT64, _T("GID") },
			{ DCI_DT_STRING, _T("Name") },
			{ DCI_DT_STRING, _T("Owner") },
			{ DCI_DT_INT64, _T("XID") },
			{ DCI_DT_STRING, _T("Prepared at") }
		}
	}
};

/**
 * Database instances
 */
static ObjectArray<DatabaseInstance> s_instances(8, 8, Ownership::True);

/**
 * Find instance by ID
 */
static DatabaseInstance *FindInstance(const TCHAR *id)
{
	for(int i = 0; i < s_instances.size(); i++)
	{
		DatabaseInstance *db = s_instances.get(i);
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
		return SYSINFO_RC_NO_SUCH_INSTANCE;

	if (arg[0] == _T('?'))  // interpret missing data as 0
	{
		if (db->getData(&arg[1], value))
			return SYSINFO_RC_SUCCESS;

		if (!db->isConnected())
		   return SYSINFO_RC_ERROR;   // Ignore "interpret missing as 0" option if DB is not connected

		ret_int(value, 0);
		return SYSINFO_RC_SUCCESS;
	}

	return db->getData(arg, value) ? SYSINFO_RC_SUCCESS : SYSINFO_RC_ERROR;
}

/**
 * Handler for parameters with instance
 */
static LONG H_InstanceParameter(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
	TCHAR id[MAX_DB_STRING];
	if (!AgentGetParameterArg(param, 1, id, MAX_DB_STRING))
		return SYSINFO_RC_UNSUPPORTED;

   TCHAR *c;
	TCHAR instance[MAX_DB_STRING];
	if ((c = _tcschr(id, _T('@'))) != nullptr)  // is the first parameter in format database@connection
   {
		*(c++) = 0;
		_tcscpy(instance, id);
		_tcscpy(id, c);
	}

   DatabaseInstance *db = FindInstance(id);
   if (db == nullptr)
      return SYSINFO_RC_NO_SUCH_INSTANCE;

   if (c == nullptr)
   {
      if (!AgentGetParameterArg(param, 2, instance, MAX_DB_STRING))
         return SYSINFO_RC_UNSUPPORTED;

      if (instance[0] == 0)
         _tcscpy(instance, db->getName()); // use connection's maintenance database
   }

   nxlog_debug_tag(DEBUG_TAG, 7, _T("H_InstanceParameter: querying %s for instance %s"), arg, instance);

	TCHAR tag[MAX_DB_STRING];
	bool missingAsZero;
	if (arg[0] == _T('?'))  // interpret missing instance as 0
	{
		_sntprintf(tag, MAX_DB_STRING, _T("%s@%s"), &arg[1], instance);
		missingAsZero = true;
	}
	else
	{
		_sntprintf(tag, MAX_DB_STRING, _T("%s@%s"), arg, instance);
		missingAsZero = false;
	}

	if (db->getData(tag, value))
		return SYSINFO_RC_SUCCESS;

	if (missingAsZero)
	{
      if (!db->isConnected())
         return SYSINFO_RC_ERROR;   // Ignore "interpret missing as 0" option if DB is not connected

		ret_int(value, 0);
		return SYSINFO_RC_SUCCESS;
	}

	return SYSINFO_RC_NO_SUCH_INSTANCE;
}

/**
 * Handler for PostgreSQL.IsReachable parameter
 */
static LONG H_DatabaseServerConnectionStatus(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
	TCHAR id[MAX_DB_STRING];
	if (!AgentGetParameterArg(param, 1, id, MAX_DB_STRING))
		return SYSINFO_RC_UNSUPPORTED;

	DatabaseInstance *db = FindInstance(id);
	if (db == nullptr)
		return SYSINFO_RC_NO_SUCH_INSTANCE;

	ret_string(value, db->isConnected() ? _T("YES") : _T("NO"));
	return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for Oracle.DBInfo.Version parameter
 */
static LONG H_DatabaseVersion(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
	TCHAR id[MAX_DB_STRING];
	if (!AgentGetParameterArg(param, 1, id, MAX_DB_STRING))
		return SYSINFO_RC_UNSUPPORTED;

	DatabaseInstance *db = FindInstance(id);
	if (db == nullptr)
		return SYSINFO_RC_NO_SUCH_INSTANCE;

   int version = db->getVersion();
   if ((version == 0) && !db->isConnected())
      return SYSINFO_RC_ERROR;   // Return error if version is not known and DB is not connected

	if ((version & 0xFF) == 0)
		_sntprintf(value, MAX_RESULT_LENGTH, _T("%d.%d"), version >> 16, (version >> 8) & 0xFF);
	else
		_sntprintf(value, MAX_RESULT_LENGTH, _T("%d.%d.%d"), version >> 16, (version >> 8) & 0xFF, version & 0xFF);
	return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for PostgreSQL.DBServersList list
 */
static LONG H_DBServersList(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
	for(int i = 0; i < s_instances.size(); i++)
		value->add(s_instances.get(i)->getId());
	return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for PostgreSQL.AllDatabases list
 */
static LONG H_AllDatabasesList(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
	for(int i = 0; i < s_instances.size(); i++)
   {
		DatabaseInstance *db = s_instances.get(i);

		StringList list;
		if (!db->getTagList(arg, &list))
			return SYSINFO_RC_ERROR;

		for(int j = 0; j < list.size(); j++)
      {
			TCHAR s[MAX_RESULT_LENGTH];
			_sntprintf(s, MAX_RESULT_LENGTH, _T("%s@%s"), list.get(j), db->getId());
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
		return SYSINFO_RC_NO_SUCH_INSTANCE;

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
		return SYSINFO_RC_NO_SUCH_INSTANCE;

	TableDescriptor *td = (TableDescriptor *)arg;
	while (td->minVersion > db->getVersion())
		td++;

	return db->queryTable(td, value) ? SYSINFO_RC_SUCCESS : SYSINFO_RC_ERROR;
}

/**
 * Config template
 */
static DatabaseInfo s_dbInfo;
static NX_CFG_TEMPLATE s_configTemplate[] =
{
	{ _T("ConnectionTTL"),	CT_LONG, 0, 0, 0, 0, &s_dbInfo.connectionTTL },
	{ _T("Database"),		CT_STRING, 0, 0, MAX_DB_STRING,	0, s_dbInfo.name },
	{ _T("Id"),				CT_STRING, 0, 0, MAX_DB_STRING,	0, s_dbInfo.id },
	{ _T("Login"),			CT_STRING, 0, 0, MAX_DB_LOGIN,	 0, s_dbInfo.login },
	{ _T("Password"),		CT_STRING, 0, 0, MAX_DB_PASSWORD, 0, s_dbInfo.password },
	{ _T("Server"),			CT_STRING, 0, 0, MAX_DB_STRING,	0, s_dbInfo.server },
	{ _T(""), CT_END_OF_LIST, 0, 0, 0, 0, nullptr }
};

/*
 * Subagent initialization
 */
static bool SubAgentInit(Config *config)
{
	// Init db driver
	g_pgsqlDriver = DBLoadDriver(_T("pgsql.ddr"), nullptr, nullptr, nullptr);
	if (g_pgsqlDriver == nullptr)
	{
		nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Failed to load PostgreSQL database driver"));
		return false;
	}

	// Load configuration from "pgsql" section to allow simple configuration
	// of one connection without XML includes
	memset(&s_dbInfo, 0, sizeof(s_dbInfo));
	s_dbInfo.connectionTTL = 3600;
	_tcscpy(s_dbInfo.id, _T("localdb"));
	_tcscpy(s_dbInfo.server, _T("127.0.0.1"));
	_tcscpy(s_dbInfo.name, _T("postgres"));
	_tcscpy(s_dbInfo.login, _T("netxms"));
	if(config->getEntry(_T("/pgsql/id")) != NULL || config->getEntry(_T("/pgsql/name")) != NULL ||
			config->getEntry(_T("pgsql/server")) != NULL || config->getEntry(_T("/pgsql/login")) != NULL  ||
			config->getEntry(_T("/pgsql/password")) != NULL)
	{
		if (config->parseTemplate(_T("PGSQL"), s_configTemplate))
		{
			if (s_dbInfo.name[0] != 0)
			{
				if (s_dbInfo.id[0] == 0)
					_tcscpy(s_dbInfo.id, s_dbInfo.name);

				DecryptPassword(s_dbInfo.login, s_dbInfo.password, s_dbInfo.password, MAX_DB_PASSWORD);
				s_instances.add(new DatabaseInstance(&s_dbInfo));
			}
		}
	}

	// Load full-featured XML configuration
	ConfigEntry *metricRoot = config->getEntry(_T("/pgsql/servers"));
	if (metricRoot != NULL)
	{
      unique_ptr<ObjectArray<ConfigEntry>> metrics = metricRoot->getSubEntries(_T("*"));
      for (int i = 0; i < metrics->size(); i++)
      {
			TCHAR section[MAX_DB_STRING];
			ConfigEntry *e = metrics->get(i);
			s_dbInfo.connectionTTL = 3600;
			_tcscpy(s_dbInfo.id, e->getName());
			_tcscpy(s_dbInfo.server, _T("127.0.0.1"));
			_tcscpy(s_dbInfo.name, _T("postgres"));
			_tcscpy(s_dbInfo.login, _T("netxms"));

			_sntprintf(section, MAX_DB_STRING, _T("pgsql/servers/%s"), e->getName());
			if (!config->parseTemplate(section, s_configTemplate))
			{
				nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Error parsing PostgreSQL subagent configuration template %s"), e->getName());
				continue;
			}

			if (s_dbInfo.id[0] == 0)
				continue;

			DecryptPassword(s_dbInfo.login, s_dbInfo.password, s_dbInfo.password, MAX_DB_PASSWORD);

			s_instances.add(new DatabaseInstance(&s_dbInfo));
      }
   }

	// Exit if no usable configuration found
	if (s_instances.isEmpty())
	{
	   nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("No databases to monitor, exiting"));
		return false;
	}

	// Run query thread for each configured database
	for(int i = 0; i < s_instances.size(); i++)
		s_instances.get(i)->run();

	return true;
}

/**
 * Shutdown handler
 */
static void SubAgentShutdown()
{
	nxlog_debug_tag(DEBUG_TAG, 1, _T("Stopping PostgreSQL database pollers"));
	for(int i = 0; i < s_instances.size(); i++)
		s_instances.get(i)->stop();
	nxlog_debug_tag(DEBUG_TAG, 1, _T("PostgreSQL subagent stopped"));
}

/**
 * Supported parameters
 */
static NETXMS_SUBAGENT_PARAM s_parameters[] =
{
	{ _T("PostgreSQL.IsReachable(*)"), H_DatabaseServerConnectionStatus, nullptr, DCI_DT_STRING, _T("PostgreSQL: database server instance reachable") },
	{ _T("PostgreSQL.Version(*)"), H_DatabaseVersion, nullptr, DCI_DT_STRING, _T("PostgreSQL: database server version") },
	{ _T("PostgreSQL.Archiver.ArchivedCount(*)"), H_GlobalParameter, _T("ARCHIVER/archived_count"), DCI_DT_INT64, _T("PostgreSQL/Archiver: archived") },
	{ _T("PostgreSQL.Archiver.FailedCount(*)"), H_GlobalParameter, _T("ARCHIVER/failed_count"), DCI_DT_INT64, _T("PostgreSQL/Archiver: failed") },
	{ _T("PostgreSQL.Archiver.IsArchiving(*)"), H_GlobalParameter, _T("ARCHIVER/is_archiving"), DCI_DT_STRING, _T("PostgreSQL/Archiver: is running") },
	{ _T("PostgreSQL.Archiver.LastArchivedAge(*)"), H_GlobalParameter, _T("ARCHIVER/last_archived_age"), DCI_DT_INT, _T("PostgreSQL/Archiver: last archived age") },
	{ _T("PostgreSQL.Archiver.LastArchivedWAL(*)"), H_GlobalParameter, _T("ARCHIVER/last_archived_wal"), DCI_DT_STRING, _T("PostgreSQL/Archiver: last archived") },
	{ _T("PostgreSQL.Archiver.LastFailedAge(*)"), H_GlobalParameter, _T("ARCHIVER/last_failed_age"), DCI_DT_INT, _T("PostgreSQL/Archiver: last failed age") },
	{ _T("PostgreSQL.Archiver.LastFailedWAL(*)"), H_GlobalParameter, _T("ARCHIVER/last_failed_wal"), DCI_DT_STRING, _T("PostgreSQL/Archiver: last failed") },
	{ _T("PostgreSQL.BGWriter.BuffersAlloc(*)"), H_GlobalParameter, _T("BGWRITER/buffers_alloc"), DCI_DT_INT64, _T("PostgreSQL/BGWriter: allocated buffers") },
	{ _T("PostgreSQL.BGWriter.BuffersBackend(*)"), H_GlobalParameter, _T("BGWRITER/buffers_backend"), DCI_DT_INT64, _T("PostgreSQL/BGWriter: directly written buffers") },
	{ _T("PostgreSQL.BGWriter.BuffersBackendFsync(*)"), H_GlobalParameter, _T("BGWRITER/buffers_backend_fsync"), DCI_DT_INT64, _T("PostgreSQL/BGWriter: fsync written buffers") },
	{ _T("PostgreSQL.BGWriter.BuffersClean(*)"), H_GlobalParameter, _T("BGWRITER/buffers_clean"), DCI_DT_INT64, _T("PostgreSQL/BGWriter: buffers written by background writer") },
	{ _T("PostgreSQL.BGWriter.BuffersCheckpoint(*)"), H_GlobalParameter, _T("BGWRITER/buffers_checkpoint"), DCI_DT_INT64, _T("PostgreSQL/BGWriter: buffers written during checkpoints") },
	{ _T("PostgreSQL.BGWriter.CheckpointsReq(*)"), H_GlobalParameter, _T("BGWRITER/checkpoints_req"), DCI_DT_INT64, _T("PostgreSQL/BGWriter: requested checkpoints") },
	{ _T("PostgreSQL.BGWriter.CheckpointsTimed(*)"), H_GlobalParameter, _T("BGWRITER/checkpoints_timed"), DCI_DT_INT64, _T("PostgreSQL/BGWriter: scheduled checkpoints") },
	{ _T("PostgreSQL.BGWriter.CheckpointSyncTime(*)"), H_GlobalParameter, _T("BGWRITER/checkpoint_sync_time"), DCI_DT_FLOAT, _T("PostgreSQL/BGWriter: checkpoint sync time") },
	{ _T("PostgreSQL.BGWriter.CheckpointWriteTime(*)"), H_GlobalParameter, _T("BGWRITER/checkpoint_write_time"), DCI_DT_FLOAT, _T("PostgreSQL/BGWriter: checkpoint write time") },
	{ _T("PostgreSQL.BGWriter.MaxWrittenClean(*)"), H_GlobalParameter, _T("BGWRITER/maxwritten_clean"), DCI_DT_INT64, _T("PostgreSQL/BGWriter: background writer stops") },
	{ _T("PostgreSQL.GlobalConnections.AutovacuumMax(*)"), H_GlobalParameter, _T("CONNECTIONS/autovacuum_max"), DCI_DT_INT, _T("PostgreSQL/GlobalConnections: max autovacuum backends") },
	{ _T("PostgreSQL.GlobalConnections.Total(*)"), H_GlobalParameter, _T("CONNECTIONS/total"), DCI_DT_INT, _T("PostgreSQL/GlobalConnections: number of all backends") },
	{ _T("PostgreSQL.GlobalConnections.TotalMax(*)"), H_GlobalParameter, _T("CONNECTIONS/max"), DCI_DT_INT, _T("PostgreSQL/GlobalConnections: max connections") },
	{ _T("PostgreSQL.GlobalConnections.TotalPct(*)"), H_GlobalParameter, _T("CONNECTIONS/total_pct"), DCI_DT_INT, _T("PostgreSQL/GlobalConnections: used connections (%)") },
	{ _T("PostgreSQL.Replication.InRecovery(*)"), H_GlobalParameter, _T("REPLICATION/in_recovery"), DCI_DT_STRING, _T("PostgreSQL/Replication: in recovery") },
	{ _T("PostgreSQL.Replication.IsReceiver(*)"), H_GlobalParameter, _T("REPLICATION/is_receiver"), DCI_DT_STRING, _T("PostgreSQL/Replication: is receiver") },
	{ _T("PostgreSQL.Replication.Lag(*)"), H_GlobalParameter, _T("REPLICATION/lag"), DCI_DT_INT, _T("PostgreSQL/Replication: lag in seconds") },
	{ _T("PostgreSQL.Replication.LagBytes(*)"), H_GlobalParameter, _T("?REPLICATION/lag_bytes"), DCI_DT_INT64, _T("PostgreSQL/Replication: lag in bytes ") },
	{ _T("PostgreSQL.Replication.WALFiles(*)"), H_GlobalParameter, _T("REPLICATION/xlog_files"), DCI_DT_INT, _T("PostgreSQL/Replication: WAL files") },
	{ _T("PostgreSQL.Replication.WALSenders(*)"), H_GlobalParameter, _T("REPLICATION/wal_senders"), DCI_DT_INT, _T("PostgreSQL/Replication: WAL senders") },
	{ _T("PostgreSQL.Replication.WALSize(*)"), H_GlobalParameter, _T("REPLICATION/xlog_size"), DCI_DT_INT64, _T("PostgreSQL/Replication: WAL size") },
	{ _T("PostgreSQL.DBConnections.Active(*)"), H_InstanceParameter, _T("DB_CONNECTIONS/active"), DCI_DT_INT, _T("PostgreSQL/DBConnections: {instance-name} number of active backends") },
	{ _T("PostgreSQL.DBConnections.Autovacuum(*)"), H_InstanceParameter, _T("DB_CONNECTIONS/autovacuum"), DCI_DT_INT, _T("PostgreSQL/DBConnections: {instance-name} autovacuum backends") },
	{ _T("PostgreSQL.DBConnections.FastpathFunctionCall(*)"), H_InstanceParameter, _T("DB_CONNECTIONS/fastpath_function_call"), DCI_DT_INT, _T("PostgreSQL/DBConnections: {instance-name} number of fastpath function call backends") },
	{ _T("PostgreSQL.DBConnections.Idle(*)"), H_InstanceParameter, _T("DB_CONNECTIONS/idle"), DCI_DT_INT, _T("PostgreSQL/DBConnections: {instance-name} number of idle backends") },
	{ _T("PostgreSQL.DBConnections.IdleInTransaction(*)"), H_InstanceParameter, _T("DB_CONNECTIONS/idle_in_transaction"), DCI_DT_INT, _T("PostgreSQL/DBConnections: {instance-name} number of idle in a transaction backends") },
	{ _T("PostgreSQL.DBConnections.IdleInTransactionAborted(*)"), H_InstanceParameter, _T("DB_CONNECTIONS/idle_in_transaction_aborted"), DCI_DT_INT, _T("PostgreSQL/DBConnections: {instance-name} number of idle in transaction (aborted) backends") },
	{ _T("PostgreSQL.DBConnections.OldestXID(*)"), H_InstanceParameter, _T("DB_CONNECTIONS/oldestxid"), DCI_DT_INT, _T("PostgreSQL/DBConnections: {instance-name} oldest XID age") },
	{ _T("PostgreSQL.DBConnections.Total(*)"), H_InstanceParameter, _T("DB_CONNECTIONS/total"), DCI_DT_INT, _T("PostgreSQL/DBConnections: {instance-name} number of all backends") },
	{ _T("PostgreSQL.DBConnections.Waiting(*)"), H_InstanceParameter, _T("DB_CONNECTIONS/waiting"), DCI_DT_INT, _T("PostgreSQL/DBConnections: {instance-name} waiting backends") },
	{ _T("PostgreSQL.Locks.AccessExclusive(*)"), H_InstanceParameter, _T("DB_LOCKS/accessexclusive"), DCI_DT_INT64, _T("PostgreSQL/Locks: {instance-name} AccessExclusive locks") },
	{ _T("PostgreSQL.Locks.AccessShare(*)"), H_InstanceParameter, _T("DB_LOCKS/accessshare"), DCI_DT_INT64, _T("PostgreSQL/Locks: {instance-name} AccessShare locks") },
	{ _T("PostgreSQL.Locks.Exclusive(*)"), H_InstanceParameter, _T("DB_LOCKS/exclusive"), DCI_DT_INT64, _T("PostgreSQL/Locks: {instance-name} Exclusive locks") },
	{ _T("PostgreSQL.Locks.RowExclusive(*)"), H_InstanceParameter, _T("DB_LOCKS/rowexclusive"), DCI_DT_INT64, _T("PostgreSQL/Locks: {instance-name} RowExclusive locks") },
	{ _T("PostgreSQL.Locks.RowShare(*)"), H_InstanceParameter, _T("DB_LOCKS/rowshare"), DCI_DT_INT64, _T("PostgreSQL/Locks: {instance-name} RowShare locks") },
	{ _T("PostgreSQL.Locks.Share(*)"), H_InstanceParameter, _T("DB_LOCKS/share"), DCI_DT_INT64, _T("PostgreSQL/Locks: {instance-name} Share locks") },
	{ _T("PostgreSQL.Locks.ShareRowExclusive(*)"), H_InstanceParameter, _T("DB_LOCKS/sharerowexclusive"), DCI_DT_INT64, _T("PostgreSQL/Locks: {instance-name} ShareRowExclusive locks") },
	{ _T("PostgreSQL.Locks.ShareUpdateExclusive(*)"), H_InstanceParameter, _T("DB_LOCKS/shareupdateexclusive"), DCI_DT_INT64, _T("PostgreSQL/Locks: {instance-name} ShareUpdateExclusive locks") },
	{ _T("PostgreSQL.Locks.Total(*)"), H_InstanceParameter, _T("DB_LOCKS/total"), DCI_DT_INT64, _T("PostgreSQL/Locks: {instance-name} all locks") },
	{ _T("PostgreSQL.Stats.BlkWriteTime(*)"), H_InstanceParameter, _T("DB_STAT/blk_write_time"), DCI_DT_FLOAT, _T("PostgreSQL/Stats: {instance-name} writing data time") },
	{ _T("PostgreSQL.Stats.BlockReadTime(*)"), H_InstanceParameter, _T("DB_STAT/blk_read_time"), DCI_DT_FLOAT, _T("PostgreSQL/Stats: {instance-name} reading data time") },
	{ _T("PostgreSQL.Stats.BlocksRead(*)"), H_InstanceParameter, _T("DB_STAT/blks_read"), DCI_DT_INT64, _T("PostgreSQL/Stats: {instance-name} read blocks") },
	{ _T("PostgreSQL.Stats.BlocksHit(*)"), H_InstanceParameter, _T("DB_STAT/blks_hit"), DCI_DT_INT64, _T("PostgreSQL/Stats: {instance-name} blocks found in cache") },
	{ _T("PostgreSQL.Stats.CacheHitRatio(*)"), H_InstanceParameter, _T("DB_STAT/cachehitratio"), DCI_DT_FLOAT, _T("PostgreSQL/Stats: {instance-name} cache hit ratio (%)") },
	{ _T("PostgreSQL.Stats.Conflicts(*)"), H_InstanceParameter, _T("DB_STAT/conflicts"), DCI_DT_INT64, _T("PostgreSQL/Stats: {instance-name} conflicts") },
	{ _T("PostgreSQL.Stats.DatabaseSize(*)"), H_InstanceParameter, _T("DB_STAT/size"), DCI_DT_INT64, _T("PostgreSQL/Stats: {instance-name} database size") },
	{ _T("PostgreSQL.Stats.Deadlocks(*)"), H_InstanceParameter, _T("DB_STAT/deadlocks"), DCI_DT_INT64, _T("PostgreSQL/Stats: {instance-name} number of deadlocks") },
	{ _T("PostgreSQL.Stats.ChecksumFailures(*)"), H_InstanceParameter, _T("DB_STAT/checksum_failures"), DCI_DT_INT64, _T("PostgreSQL/Stats: {instance-name} page checksum failures") },
	{ _T("PostgreSQL.Stats.NumBackends(*)"), H_InstanceParameter, _T("DB_STAT/numbackends"), DCI_DT_INT, _T("PostgreSQL/Stats: {instance-name} number of backends") },
	{ _T("PostgreSQL.Stats.RowsDeleted(*)"), H_InstanceParameter, _T("DB_STAT/tup_deleted"), DCI_DT_INT64, _T("PostgreSQL/Stats: {instance-name} deleted rows") },
	{ _T("PostgreSQL.Stats.RowsFetched(*)"), H_InstanceParameter, _T("DB_STAT/tup_fetched"), DCI_DT_INT64, _T("PostgreSQL/Stats: {instance-name} fetched rows") },
	{ _T("PostgreSQL.Stats.RowsInserted(*)"), H_InstanceParameter, _T("DB_STAT/tup_inserted"), DCI_DT_INT64, _T("PostgreSQL/Stats: {instance-name} inserted rows") },
	{ _T("PostgreSQL.Stats.RowsReturned(*)"), H_InstanceParameter, _T("DB_STAT/tup_returned"), DCI_DT_INT64, _T("PostgreSQL/Stats: {instance-name} returned rows") },
	{ _T("PostgreSQL.Stats.RowsUpdated(*)"), H_InstanceParameter, _T("DB_STAT/tup_updated"), DCI_DT_INT64, _T("PostgreSQL/Stats: {instance-name} updated rows") },
	{ _T("PostgreSQL.Stats.TempBytes(*)"), H_InstanceParameter, _T("DB_STAT/temp_bytes"), DCI_DT_INT64, _T("PostgreSQL/Stats: {instance-name} temporary data size") },
	{ _T("PostgreSQL.Stats.TempFiles(*)"), H_InstanceParameter, _T("DB_STAT/temp_files"), DCI_DT_INT64, _T("PostgreSQL/Stats: {instance-name} temporary files") },
	{ _T("PostgreSQL.Stats.TransactionCommits(*)"), H_InstanceParameter, _T("DB_STAT/xact_commit"), DCI_DT_INT64, _T("PostgreSQL/Stats: {instance-name} commited tranactions") },
	{ _T("PostgreSQL.Stats.TransactionRollbacks(*)"), H_InstanceParameter, _T("DB_STAT/xact_rollback"), DCI_DT_INT64, _T("PostgreSQL/Stats: {instance-name} rolled back transactions") },
	{ _T("PostgreSQL.Transactions.Prepared(*)"), H_InstanceParameter, _T("?DB_TRANSACTIONS/prepared"), DCI_DT_INT64, _T("PostgreSQL/Transactions: {instance-name} prepared transactions") }
};

/**
 * Supported lists
 */
static NETXMS_SUBAGENT_LIST s_lists[] =
{
	{ _T("PostgreSQL.DBServers"), H_DBServersList, nullptr, _T("PostgreSQL: database servers being monitored") },
	{ _T("PostgreSQL.Databases(*)"), H_TagList, _T("^DB_STAT/size@(.*)$"), _T("PostgreSQL: databases on the specific server") },
	{ _T("PostgreSQL.AllDatabases"), H_AllDatabasesList, _T("^DB_STAT/size@(.*)$"), _T("PostgreSQL: all databases on all monitored servers") },
	{ _T("PostgreSQL.DataTags(*)"), H_TagList, _T("^(.*)$") }
};

/**
 * Supported tables
 */
static NETXMS_SUBAGENT_TABLE s_tables[] =
{
	{ _T("PostgreSQL.Backends(*)"), H_TableQuery, (const TCHAR *)&s_tqBackends, _T("PID"), _T("PostgreSQL: backend activity") },
	{ _T("PostgreSQL.Locks(*)"), H_TableQuery, (const TCHAR *)&s_tqLocks, _T("PID"), _T("PostgreSQL: locks") },
	{ _T("PostgreSQL.PreparedTransactions(*)"), H_TableQuery, (const TCHAR *)&s_tqPrepared, _T("GID"), _T("PostgreSQL: prepared transactions") }
};

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO s_info =
{
	NETXMS_SUBAGENT_INFO_MAGIC,
	_T("PGSQL"), NETXMS_VERSION_STRING,
	SubAgentInit, SubAgentShutdown, nullptr, nullptr, nullptr,
	sizeof(s_parameters) / sizeof(NETXMS_SUBAGENT_PARAM), s_parameters,
	sizeof(s_lists) / sizeof(NETXMS_SUBAGENT_LIST), s_lists,
	sizeof(s_tables) / sizeof(NETXMS_SUBAGENT_TABLE), s_tables,
	0,	nullptr,	 // actions
	0,	nullptr	  // push parameters
};

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(PGSQL)
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
