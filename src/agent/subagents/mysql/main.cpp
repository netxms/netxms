/*
 ** NetXMS - Network Management System
 ** Subagent for MySQL monitoring
 ** Copyright (C) 2016 Raden Solutions
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

#include "mysql_subagent.h"

/**
 * Driver handle
 */
DB_DRIVER g_mysqlDriver = NULL;

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
static LONG H_GlobalParameter(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR id[MAX_DB_STRING];
   if (!AgentGetParameterArg(param, 1, id, MAX_DB_STRING))
      return SYSINFO_RC_UNSUPPORTED;

   DatabaseInstance *db = FindInstance(id);
   if (db == NULL)
      return SYSINFO_RC_UNSUPPORTED;

   return db->getData(arg, value) ? SYSINFO_RC_SUCCESS : SYSINFO_RC_ERROR;
}

/**
 * Handler for MySQL.IsReachable parameter
 */
static LONG H_DatabaseConnectionStatus(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR id[MAX_DB_STRING];
   if (!AgentGetParameterArg(param, 1, id, MAX_DB_STRING))
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
static DatabaseInfo s_dbInfo;
static NX_CFG_TEMPLATE s_configTemplate[] =
{
   { _T("ConnectionTTL"),     CT_LONG,   0, 0, 0,               0, &s_dbInfo.connectionTTL },
	{ _T("Database"),		      CT_STRING, 0, 0, MAX_DB_STRING,   0, s_dbInfo.name },
   { _T("EncryptedPassword"), CT_STRING, 0, 0, MAX_DB_PASSWORD, 0, s_dbInfo.password },
   { _T("Id"),                CT_STRING, 0, 0, MAX_DB_STRING,   0, s_dbInfo.id },
	{ _T("Login"),             CT_STRING, 0, 0, MAX_DB_LOGIN,    0, s_dbInfo.login },
	{ _T("Password"),			   CT_STRING, 0, 0, MAX_DB_PASSWORD, 0, s_dbInfo.password },
   { _T("Server"),            CT_STRING, 0, 0, MAX_DB_STRING,   0, s_dbInfo.server },
	{ _T(""), CT_END_OF_LIST, 0, 0, 0, 0, NULL }
};

/*
 * Subagent initialization
 */
static BOOL SubAgentInit(Config *config)
{
   int i;

	g_mysqlDriver = DBLoadDriver(_T("mysql.ddr"), NULL, TRUE, NULL, NULL);
	if (g_mysqlDriver == NULL)
	{
		AgentWriteLog(EVENTLOG_ERROR_TYPE, _T("MYSQL: failed to load database driver"));
		return FALSE;
	}

   s_instances = new ObjectArray<DatabaseInstance>(8, 8, true);

	// Load configuration from "mysql" section to allow simple configuration
	// of one database without XML includes
	memset(&s_dbInfo, 0, sizeof(s_dbInfo));
	s_dbInfo.connectionTTL = 3600;
	_tcscpy(s_dbInfo.id, _T("localdb"));
   _tcscpy(s_dbInfo.server, _T("127.0.0.1"));
   _tcscpy(s_dbInfo.name, _T("information_schema"));
	if (config->parseTemplate(_T("MYSQL"), s_configTemplate))
	{
		if (s_dbInfo.name[0] != 0)
		{
			if (s_dbInfo.id[0] == 0)
				_tcscpy(s_dbInfo.id, s_dbInfo.name);

         DecryptPassword(s_dbInfo.login, s_dbInfo.password, s_dbInfo.password, MAX_DB_PASSWORD);
         s_instances->add(new DatabaseInstance(&s_dbInfo));
		}
	}

	// Load full-featured XML configuration
	for(i = 1; i <= 64; i++)
	{
		TCHAR section[MAX_DB_STRING];
		memset(&s_dbInfo, 0, sizeof(s_dbInfo));
		s_dbInfo.connectionTTL = 3600;
	   _tcscpy(s_dbInfo.name, _T("information_schema"));
		_sntprintf(section, MAX_DB_STRING, _T("mysql/databases/database#%d"), i);
		if (!config->parseTemplate(section, s_configTemplate))
		{
			AgentWriteLog(NXLOG_WARNING, _T("MYSQL: error parsing configuration template %d"), i);
         continue;
		}

		if (s_dbInfo.id[0] == 0)
			continue;

      DecryptPassword(s_dbInfo.login, s_dbInfo.password, s_dbInfo.password, MAX_DB_PASSWORD);

      s_instances->add(new DatabaseInstance(&s_dbInfo));
	}

	// Exit if no usable configuration found
   if (s_instances->size() == 0)
	{
      AgentWriteLog(NXLOG_WARNING, _T("MYSQL: no databases to monitor, exiting"));
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
	AgentWriteDebugLog(1, _T("MYSQL: stopping pollers"));
   for(int i = 0; i < s_instances->size(); i++)
      s_instances->get(i)->stop();
   delete s_instances;
	AgentWriteDebugLog(1, _T("MYSQL: stopped"));
}

/**
 * Supported parameters
 */
static NETXMS_SUBAGENT_PARAM s_parameters[] =
{
   { _T("MySQL.Connections.Aborted(*)"), H_GlobalParameter, _T("abortedClients"), DCI_DT_UINT64, _T("MySQL: aborted connections") },
   { _T("MySQL.Connections.BytesReceived(*)"), H_GlobalParameter, _T("bytesReceived"), DCI_DT_UINT64, _T("MySQL: bytes received from all clients") },
   { _T("MySQL.Connections.BytesSent(*)"), H_GlobalParameter, _T("bytesSent"), DCI_DT_UINT64, _T("MySQL: bytes sent to all clients") },
   { _T("MySQL.Connections.Current(*)"), H_GlobalParameter, _T("threadsConnected"), DCI_DT_UINT64, _T("MySQL: number of active connections") },
   { _T("MySQL.Connections.CurrentPerc(*)"), H_GlobalParameter, _T("threadsConnectedPerc"), DCI_DT_FLOAT, _T("MySQL: connection pool usage (%)") },
   { _T("MySQL.Connections.Failed(*)"), H_GlobalParameter, _T("abortedConnections"), DCI_DT_UINT64, _T("MySQL: failed connection attempts") },
   { _T("MySQL.Connections.Limit(*)"), H_GlobalParameter, _T("connectionsLimit"), DCI_DT_UINT64, _T("MySQL: maximum possible number of simultaneous connections") },
   { _T("MySQL.Connections.Max(*)"), H_GlobalParameter, _T("maxUsedConnections"), DCI_DT_UINT64, _T("MySQL: maximum number of simultaneous connections") },
   { _T("MySQL.Connections.MaxPerc(*)"), H_GlobalParameter, _T("maxUsedConnectionsPerc"), DCI_DT_FLOAT, _T("MySQL: maximum connection pool usage  (%)") },
   { _T("MySQL.Connections.Total(*)"), H_GlobalParameter, _T("connectionsTotal"), DCI_DT_UINT64, _T("MySQL: cumulative connection count") },
   { _T("MySQL.InnoDB.BufferPool.Dirty(*)"), H_GlobalParameter, _T("innodbBufferPoolDirty"), DCI_DT_UINT64, _T("MySQL: InnoDB used buffer pool space in dirty pages") },
   { _T("MySQL.InnoDB.BufferPool.DirtyPerc(*)"), H_GlobalParameter, _T("innodbBufferPoolDirtyPerc"), DCI_DT_FLOAT, _T("MySQL: InnoDB used buffer pool space in dirty pages (%)") },
   { _T("MySQL.InnoDB.BufferPool.Free(*)"), H_GlobalParameter, _T("innodbBufferPoolFree"), DCI_DT_UINT64, _T("MySQL: InnoDB free buffer pool space") },
   { _T("MySQL.InnoDB.BufferPool.FreePerc(*)"), H_GlobalParameter, _T("innodbBufferPoolFreePerc"), DCI_DT_FLOAT, _T("MySQL: InnoDB free buffer pool space (%)") },
   { _T("MySQL.InnoDB.BufferPool.Size(*)"), H_GlobalParameter, _T("innodbBufferPoolSize"), DCI_DT_UINT64, _T("MySQL: InnoDB buffer pool size") },
   { _T("MySQL.InnoDB.BufferPool.Used(*)"), H_GlobalParameter, _T("innodbBufferPoolData"), DCI_DT_UINT64, _T("MySQL: InnoDB used buffer pool space") },
   { _T("MySQL.InnoDB.BufferPool.UsedPerc(*)"), H_GlobalParameter, _T("innodbBufferPoolDataPerc"), DCI_DT_FLOAT, _T("MySQL: InnoDB used buffer pool space (%)") },
   { _T("MySQL.InnoDB.DiskReads(*)"), H_GlobalParameter, _T("innodbDiskReads"), DCI_DT_UINT64, _T("MySQL: InnoDB disk reads") },
   { _T("MySQL.InnoDB.ReadCacheHitRatio(*)"), H_GlobalParameter, _T("innodbReadCacheHitRatio"), DCI_DT_FLOAT, _T("MySQL: InnoDB read cache hit ratio (%)") },
   { _T("MySQL.InnoDB.ReadRequest(*)"), H_GlobalParameter, _T("innodbReadRequests"), DCI_DT_UINT64, _T("MySQL: InnoDB read requests") },
   { _T("MySQL.InnoDB.WriteRequest(*)"), H_GlobalParameter, _T("innodbWriteRequests"), DCI_DT_UINT64, _T("MySQL: InnoDB write requests") },
   { _T("MySQL.IsReachable(*)"), H_DatabaseConnectionStatus, NULL, DCI_DT_STRING, _T("MySQL: is database reachable") },
   { _T("MySQL.MyISAM.KeyCacheFree(*)"), H_GlobalParameter, _T("myISAMKeyCacheFree"), DCI_DT_UINT64, _T("MySQL: MyISAM key cache free space") },
   { _T("MySQL.MyISAM.KeyCacheFreePerc(*)"), H_GlobalParameter, _T("myISAMKeyCacheFreePerc"), DCI_DT_FLOAT, _T("MySQL: MyISAM key cache free space (%)") },
   { _T("MySQL.MyISAM.KeyCacheReadHitRatio(*)"), H_GlobalParameter, _T("myISAMKeyCacheReadHitRatio"), DCI_DT_UINT64, _T("MySQL: MyISAM key cache read hit ratio (%)") },
   { _T("MySQL.MyISAM.KeyCacheSize(*)"), H_GlobalParameter, _T("myISAMKeyCacheSize"), DCI_DT_UINT64, _T("MySQL: MyISAM key cache size") },
   { _T("MySQL.MyISAM.KeyCacheUsed(*)"), H_GlobalParameter, _T("myISAMKeyCacheUsed"), DCI_DT_UINT64, _T("MySQL: MyISAM key cache used space") },
   { _T("MySQL.MyISAM.KeyCacheUsedPerc(*)"), H_GlobalParameter, _T("myISAMKeyCacheUsedPerc"), DCI_DT_FLOAT, _T("MySQL: MyISAM key cache used space (%)") },
   { _T("MySQL.MyISAM.KeyCacheWriteHitRatio(*)"), H_GlobalParameter, _T("myISAMKeyCacheWriteHitRatio"), DCI_DT_UINT64, _T("MySQL: MyISAM key cache write hit ratio (%)") },
   { _T("MySQL.MyISAM.KeyDiskReads(*)"), H_GlobalParameter, _T("myISAMKeyDiskReads"), DCI_DT_UINT64, _T("MySQL: MyISAM key cache disk reads") },
   { _T("MySQL.MyISAM.KeyDiskWrites(*)"), H_GlobalParameter, _T("myISAMKeyDiskWrites"), DCI_DT_UINT64, _T("MySQL: MyISAM key cache disk writes") },
   { _T("MySQL.MyISAM.KeyReadRequests(*)"), H_GlobalParameter, _T("myISAMKeyReadRequests"), DCI_DT_UINT64, _T("MySQL: MyISAM key cache read requests") },
   { _T("MySQL.MyISAM.KeyWriteRequests(*)"), H_GlobalParameter, _T("myISAMKeyWriteRequests"), DCI_DT_UINT64, _T("MySQL: MyISAM key cache write requests") },
   { _T("MySQL.OpenFiles.Current(*)"), H_GlobalParameter, _T("openFiles"), DCI_DT_UINT64, _T("MySQL: open files") },
   { _T("MySQL.OpenFiles.CurrentPerc(*)"), H_GlobalParameter, _T("openFilesPerc"), DCI_DT_FLOAT, _T("MySQL: open file pool usage (%)") },
   { _T("MySQL.OpenFiles.Limit(*)"), H_GlobalParameter, _T("maxOpenFiles"), DCI_DT_UINT64, _T("MySQL: maximum possible number of open files") },
   { _T("MySQL.Queries.Cache.HitRatio(*)"), H_GlobalParameter, _T("qcacheHitRatio"), DCI_DT_FLOAT, _T("MySQL: query cache hit ratio (%)") },
   { _T("MySQL.Queries.Cache.Hits(*)"), H_GlobalParameter, _T("qcacheHits"), DCI_DT_UINT64, _T("MySQL: query cache hits") },
   { _T("MySQL.Queries.Cache.Size(*)"), H_GlobalParameter, _T("qcacheSize"), DCI_DT_UINT64, _T("MySQL: query cache size") },
   { _T("MySQL.Queries.ClientsTotal(*)"), H_GlobalParameter, _T("queriesClientsTotal"), DCI_DT_UINT64, _T("MySQL: number of queries executed by clients") },
   { _T("MySQL.Queries.Delete(*)"), H_GlobalParameter, _T("queriesDelete"), DCI_DT_UINT64, _T("MySQL: number of DELETE queries") },
   { _T("MySQL.Queries.DeleteMultiTable(*)"), H_GlobalParameter, _T("queriesDeleteMultiTable"), DCI_DT_UINT64, _T("MySQL: number of multitable DELETE queries") },
   { _T("MySQL.Queries.Insert(*)"), H_GlobalParameter, _T("queriesInsert"), DCI_DT_UINT64, _T("MySQL: number of INSERT queries") },
	{ _T("MySQL.Queries.Select(*)"), H_GlobalParameter, _T("queriesSelect"), DCI_DT_UINT64, _T("MySQL: number of SELECT queries") },
   { _T("MySQL.Queries.Slow(*)"), H_GlobalParameter, _T("slowQueries"), DCI_DT_UINT64, _T("MySQL: slow queries") },
   { _T("MySQL.Queries.SlowPerc(*)"), H_GlobalParameter, _T("slowQueriesPerc"), DCI_DT_FLOAT, _T("MySQL: slow queries (%)") },
   { _T("MySQL.Queries.Total(*)"), H_GlobalParameter, _T("queriesTotal"), DCI_DT_UINT64, _T("MySQL: number of queries") },
   { _T("MySQL.Queries.Update(*)"), H_GlobalParameter, _T("queriesUpdate"), DCI_DT_UINT64, _T("MySQL: number of UPDATE queries") },
   { _T("MySQL.Queries.UpdateMultiTable(*)"), H_GlobalParameter, _T("queriesUpdateMultiTable"), DCI_DT_UINT64, _T("MySQL: number of multitable UPDATE queries") },
   { _T("MySQL.Server.Uptime(*)"), H_GlobalParameter, _T("uptime"), DCI_DT_UINT64, _T("MySQL: server uptime") },
   { _T("MySQL.Sort.MergePasses(*)"), H_GlobalParameter, _T("sortMergePasses"), DCI_DT_UINT64, _T("MySQL: sort merge passes") },
   { _T("MySQL.Sort.MergeRatio(*)"), H_GlobalParameter, _T("sortMergeRatio"), DCI_DT_FLOAT, _T("MySQL: sort merge ratio (%)") },
   { _T("MySQL.Sort.Range(*)"), H_GlobalParameter, _T("sortRange"), DCI_DT_UINT64, _T("MySQL: number of sorts using ranges") },
   { _T("MySQL.Sort.Scan(*)"), H_GlobalParameter, _T("sortScan"), DCI_DT_UINT64, _T("MySQL: number of sorts using table scans") },
   { _T("MySQL.Tables.Fragmented(*)"), H_GlobalParameter, _T("fragmentedTables"), DCI_DT_UINT64, _T("MySQL: fragmented tables") },
   { _T("MySQL.Tables.Open(*)"), H_GlobalParameter, _T("openTables"), DCI_DT_UINT64, _T("MySQL: open tables") },
   { _T("MySQL.Tables.OpenLimit(*)"), H_GlobalParameter, _T("openTablesLimit"), DCI_DT_UINT64, _T("MySQL: maximum possible number of open tables") },
   { _T("MySQL.Tables.OpenPerc(*)"), H_GlobalParameter, _T("openTablesPerc"), DCI_DT_FLOAT, _T("MySQL: table open cache usage (%)") },
   { _T("MySQL.Tables.Opened(*)"), H_GlobalParameter, _T("openedTables"), DCI_DT_UINT64, _T("MySQL: tables that have been opened") },
   { _T("MySQL.TempTables.Created(*)"), H_GlobalParameter, _T("tempTablesCreated"), DCI_DT_UINT64, _T("MySQL: temporary tables created") },
   { _T("MySQL.TempTables.CreatedOnDisk(*)"), H_GlobalParameter, _T("tempTablesCreatedOnDisk"), DCI_DT_UINT64, _T("MySQL: temporary tables created on disk") },
   { _T("MySQL.TempTables.CreatedOnDiskPerc(*)"), H_GlobalParameter, _T("tempTablesCreatedOnDiskPerc"), DCI_DT_FLOAT, _T("MySQL: temporary tables created on disk (%)") },
   { _T("MySQL.Threads.CacheHitRatio(*)"), H_GlobalParameter, _T("threadCacheHitRatio"), DCI_DT_FLOAT, _T("MySQL: thread cache hit ratio (%)") },
   { _T("MySQL.Threads.CacheSize(*)"), H_GlobalParameter, _T("threadCacheSize"), DCI_DT_UINT64, _T("MySQL: thread cache size") },
   { _T("MySQL.Threads.Created(*)"), H_GlobalParameter, _T("threadsCreated"), DCI_DT_UINT64, _T("MySQL: threads created") },
   { _T("MySQL.Threads.Running(*)"), H_GlobalParameter, _T("threadsRunning"), DCI_DT_UINT64, _T("MySQL: threads running") }
};

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO s_info =
{
	NETXMS_SUBAGENT_INFO_MAGIC,
	_T("MYSQL"), NETXMS_BUILD_TAG,
	SubAgentInit, SubAgentShutdown, NULL,
	sizeof(s_parameters) / sizeof(NETXMS_SUBAGENT_PARAM), s_parameters,
	0, NULL,    // lists
	0, NULL,    // tables
	0,	NULL,    // actions
	0,	NULL     // push parameters
};

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(MYSQL)
{
	*ppInfo = &s_info;
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
