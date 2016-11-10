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
 * Data to query
 */
struct GlobalData
{
   const TCHAR *tag;
   const TCHAR *globalStatusVar;
   const TCHAR *globalVariablesVar;
   const TCHAR *query;
   bool (*calculator)(const StringMap *, TCHAR *);
};

/**
 * Attribute calculator: innodbBufferPoolDataPerc
 */
static bool A_innodbBufferPoolDataPerc(const StringMap *attributes, TCHAR *value)
{
   INT64 innodbBufferPoolSize = attributes->getInt64(_T("innodbBufferPoolSize"), -1);
   INT64 innodbBufferPoolData = attributes->getInt64(_T("innodbBufferPoolData"), -1);
   if ((innodbBufferPoolSize <= 0) || (innodbBufferPoolData < 0))
      return false;
   _sntprintf(value, MAX_RESULT_LENGTH, _T("%1.2f"), (double)innodbBufferPoolData / (double)innodbBufferPoolSize * 100.0);
   return true;
}

/**
 * Attribute calculator: innodbBufferPoolDirtyPerc
 */
static bool A_innodbBufferPoolDirtyPerc(const StringMap *attributes, TCHAR *value)
{
   INT64 innodbBufferPoolSize = attributes->getInt64(_T("innodbBufferPoolSize"), -1);
   INT64 innodbBufferPoolDirty = attributes->getInt64(_T("innodbBufferPoolDirty"), -1);
   if ((innodbBufferPoolSize <= 0) || (innodbBufferPoolDirty < 0))
      return false;
   _sntprintf(value, MAX_RESULT_LENGTH, _T("%1.2f"), (double)innodbBufferPoolDirty / (double)innodbBufferPoolSize * 100.0);
   return true;
}

/**
 * Attribute calculator: innodbBufferPoolFree
 */
static bool A_innodbBufferPoolFree(const StringMap *attributes, TCHAR *value)
{
   INT64 innodbBufferPoolSize = attributes->getInt64(_T("innodbBufferPoolSize"), -1);
   INT64 innodbBufferPoolData = attributes->getInt64(_T("innodbBufferPoolData"), -1);
   if ((innodbBufferPoolSize <= 0) || (innodbBufferPoolData < 0))
      return false;
   _sntprintf(value, MAX_RESULT_LENGTH, INT64_FMT, innodbBufferPoolSize - innodbBufferPoolData);
   return true;
}

/**
 * Attribute calculator: innodbBufferPoolFreePerc
 */
static bool A_innodbBufferPoolFreePerc(const StringMap *attributes, TCHAR *value)
{
   INT64 innodbBufferPoolSize = attributes->getInt64(_T("innodbBufferPoolSize"), -1);
   INT64 innodbBufferPoolFree = attributes->getInt64(_T("innodbBufferPoolFree"), -1);
   if ((innodbBufferPoolSize <= 0) || (innodbBufferPoolFree < 0))
      return false;
   _sntprintf(value, MAX_RESULT_LENGTH, _T("%1.2f"), (double)innodbBufferPoolFree / (double)innodbBufferPoolSize * 100.0);
   return true;
}

/**
 * Attribute calculator: innodbReadCacheHitRatio
 */
static bool A_innodbReadCacheHitRatio(const StringMap *attributes, TCHAR *value)
{
   INT64 innodbDiskReads = attributes->getInt64(_T("innodbDiskReads"), -1);
   INT64 innodbReadRequests = attributes->getInt64(_T("innodbReadRequests"), -1);
   if ((innodbDiskReads < 0) || (innodbReadRequests < 0))
      return false;
   _sntprintf(value, MAX_RESULT_LENGTH, _T("%1.2f"), 100.0 - ((innodbReadRequests > 0) ? (double)innodbDiskReads / (double)innodbReadRequests * 100.0 : (double)0));
   return true;
}

/**
 * Attribute calculator: maxUsedConnectionsPerc
 */
static bool A_maxUsedConnectionsPerc(const StringMap *attributes, TCHAR *value)
{
   INT64 connectionsLimit = attributes->getInt64(_T("connectionsLimit"), -1);
   INT64 maxUsedConnections = attributes->getInt64(_T("maxUsedConnections"), -1);
   if ((connectionsLimit <= 0) || (maxUsedConnections < 0))
      return false;
   _sntprintf(value, MAX_RESULT_LENGTH, _T("%1.2f"), (double)maxUsedConnections / (double)connectionsLimit * 100.0);
   return true;
}

/**
 * Attribute calculator: myISAMKeyCacheFree
 */
static bool A_myISAMKeyCacheFree(const StringMap *attributes, TCHAR *value)
{
   INT64 myISAMKeyCacheBlockSize = attributes->getInt64(_T("myISAMKeyCacheBlockSize"), -1);
   INT64 myISAMKeyCacheBlocksUnused = attributes->getInt64(_T("myISAMKeyCacheBlocksUnused"), -1);
   if ((myISAMKeyCacheBlockSize < 0) || (myISAMKeyCacheBlocksUnused < 0))
      return false;
   _sntprintf(value, MAX_RESULT_LENGTH, INT64_FMT, myISAMKeyCacheBlocksUnused * myISAMKeyCacheBlockSize);
   return true;
}

/**
 * Attribute calculator: myISAMKeyCacheFreePerc
 */
static bool A_myISAMKeyCacheFreePerc(const StringMap *attributes, TCHAR *value)
{
   INT64 myISAMKeyCacheSize = attributes->getInt64(_T("myISAMKeyCacheSize"), -1);
   INT64 myISAMKeyCacheFree = attributes->getInt64(_T("myISAMKeyCacheFree"), -1);
   if ((myISAMKeyCacheSize < 0) || (myISAMKeyCacheFree < 0))
      return false;
   _sntprintf(value, MAX_RESULT_LENGTH, _T("%1.2f"), (myISAMKeyCacheSize > 0) ? (double)myISAMKeyCacheFree / (double)myISAMKeyCacheSize * 100.0 : (double)0);
   return true;
}

/**
 * Attribute calculator: myISAMKeyCacheReadHitRatio
 */
static bool A_myISAMKeyCacheReadHitRatio(const StringMap *attributes, TCHAR *value)
{
   INT64 myISAMKeyDiskReads = attributes->getInt64(_T("myISAMKeyDiskReads"), -1);
   INT64 myISAMKeyReadRequests = attributes->getInt64(_T("myISAMKeyReadRequests"), -1);
   if ((myISAMKeyDiskReads < 0) || (myISAMKeyReadRequests < 0))
      return false;
   _sntprintf(value, MAX_RESULT_LENGTH, _T("%1.2f"), 100.0 - ((myISAMKeyReadRequests > 0) ? (double)myISAMKeyDiskReads / (double)myISAMKeyReadRequests * 100.0 : (double)0));
   return true;
}

/**
 * Attribute calculator: myISAMKeyCacheUsed
 */
static bool A_myISAMKeyCacheUsed(const StringMap *attributes, TCHAR *value)
{
   INT64 myISAMKeyCacheSize = attributes->getInt64(_T("myISAMKeyCacheSize"), -1);
   INT64 myISAMKeyCacheFree = attributes->getInt64(_T("myISAMKeyCacheFree"), -1);
   if ((myISAMKeyCacheSize < 0) || (myISAMKeyCacheFree < 0))
      return false;
   _sntprintf(value, MAX_RESULT_LENGTH, INT64_FMT, myISAMKeyCacheSize - myISAMKeyCacheFree);
   return true;
}

/**
 * Attribute calculator: myISAMKeyCacheUsedPerc
 */
static bool A_myISAMKeyCacheUsedPerc(const StringMap *attributes, TCHAR *value)
{
   INT64 myISAMKeyCacheSize = attributes->getInt64(_T("myISAMKeyCacheSize"), -1);
   INT64 myISAMKeyCacheUsed = attributes->getInt64(_T("myISAMKeyCacheUsed"), -1);
   if ((myISAMKeyCacheSize < 0) || (myISAMKeyCacheUsed < 0))
      return false;
   _sntprintf(value, MAX_RESULT_LENGTH, _T("%1.2f"), (myISAMKeyCacheSize > 0) ? (double)myISAMKeyCacheUsed / (double)myISAMKeyCacheSize * 100.0 : (double)0);
   return true;
}

/**
 * Attribute calculator: myISAMKeyCacheWriteHitRatio
 */
static bool A_myISAMKeyCacheWriteHitRatio(const StringMap *attributes, TCHAR *value)
{
   INT64 myISAMKeyDiskWrites = attributes->getInt64(_T("myISAMKeyDiskWrites"), -1);
   INT64 myISAMKeyWriteRequests = attributes->getInt64(_T("myISAMKeyWriteRequests"), -1);
   if ((myISAMKeyDiskWrites < 0) || (myISAMKeyWriteRequests < 0))
      return false;
   _sntprintf(value, MAX_RESULT_LENGTH, _T("%1.2f"), (myISAMKeyWriteRequests > 0) ? (double)myISAMKeyDiskWrites / (double)myISAMKeyWriteRequests * 100.0 : (double)0);
   return true;
}

/**
 * Attribute calculator: openFilesPerc
 */
static bool A_openFilesPerc(const StringMap *attributes, TCHAR *value)
{
   INT64 maxOpenFiles = attributes->getInt64(_T("maxOpenFiles"), -1);
   INT64 openFiles = attributes->getInt64(_T("openFiles"), -1);
   if ((maxOpenFiles <= 0) || (openFiles < 0))
      return false;
   _sntprintf(value, MAX_RESULT_LENGTH, _T("%1.2f"), (double)openFiles / (double)maxOpenFiles * 100.0);
   return true;
}

/**
 * Attribute calculator: openTablesPerc
 */
static bool A_openTablesPerc(const StringMap *attributes, TCHAR *value)
{
   INT64 openTablesLimit = attributes->getInt64(_T("openTablesLimit"), -1);
   INT64 openTables = attributes->getInt64(_T("openTables"), -1);
   if ((openTablesLimit <= 0) || (openTables < 0))
      return false;
   _sntprintf(value, MAX_RESULT_LENGTH, _T("%1.2f"), (double)openTables / (double)openTablesLimit * 100.0);
   return true;
}

/**
 * Attribute calculator: qcacheHitRatio
 */
static bool A_qcacheHitRatio(const StringMap *attributes, TCHAR *value)
{
   INT64 qcacheHits = attributes->getInt64(_T("qcacheHits"), -1);
   INT64 queriesSelectNoCache = attributes->getInt64(_T("queriesSelectNoCache"), -1);
   if ((queriesSelectNoCache < 0) || (qcacheHits < 0))
      return false;
   INT64 totalSelects = qcacheHits + queriesSelectNoCache;
   _sntprintf(value, MAX_RESULT_LENGTH, _T("%1.2f"), (totalSelects > 0) ? (double)qcacheHits / (double)totalSelects * 100.0 : (double)0);
   return true;
}

/**
 * Attribute calculator: queriesSelect
 */
static bool A_queriesSelect(const StringMap *attributes, TCHAR *value)
{
   INT64 qcacheHits = attributes->getInt64(_T("qcacheHits"), -1);
   INT64 queriesSelectNoCache = attributes->getInt64(_T("queriesSelectNoCache"), -1);
   if ((queriesSelectNoCache < 0) || (qcacheHits < 0))
      return false;
   _sntprintf(value, MAX_RESULT_LENGTH, INT64_FMT, qcacheHits + queriesSelectNoCache);
   return true;
}

/**
 * Attribute calculator: slowQueriesPerc
 */
static bool A_slowQueriesPerc(const StringMap *attributes, TCHAR *value)
{
   INT64 slowQueriesPerc = attributes->getInt64(_T("slowQueries"), -1);
   INT64 queriesTotal = attributes->getInt64(_T("queriesTotal"), -1);
   if ((slowQueriesPerc < 0) || (queriesTotal < 0))
      return false;
   _sntprintf(value, MAX_RESULT_LENGTH, _T("%1.2f"), (queriesTotal > 0) ? (double)slowQueriesPerc / (double)queriesTotal * 100.0 : (double)0);
   return true;
}

/**
 * Attribute calculator: sortMergeRatio
 */
static bool A_sortMergeRatio(const StringMap *attributes, TCHAR *value)
{
   INT64 sortMergePasses = attributes->getInt64(_T("sortMergePasses"), -1);
   INT64 sortRange = attributes->getInt64(_T("sortRange"), -1);
   INT64 sortScan = attributes->getInt64(_T("sortScan"), -1);
   if ((sortMergePasses < 0) || (sortRange < 0) || (sortScan < 0))
      return false;
   INT64 sorts = sortRange + sortScan;
   _sntprintf(value, MAX_RESULT_LENGTH, _T("%1.2f"), (sorts > 0) ? (double)sortMergePasses / (double)sorts * 100.0 : (double)0);
   return true;
}

/**
 * Attribute calculator: tempTablesCreatedOnDiskPerc
 */
static bool A_tempTablesCreatedOnDiskPerc(const StringMap *attributes, TCHAR *value)
{
   INT64 tempTablesCreated = attributes->getInt64(_T("tempTablesCreated"), -1);
   INT64 tempTablesCreatedOnDisk = attributes->getInt64(_T("tempTablesCreatedOnDisk"), -1);
   if ((tempTablesCreated < 0) || (tempTablesCreatedOnDisk < 0))
      return false;
   _sntprintf(value, MAX_RESULT_LENGTH, _T("%1.2f"), (tempTablesCreated > 0) ? (double)tempTablesCreatedOnDisk / (double)tempTablesCreated * 100.0 : (double)0);
   return true;
}

/**
 * Attribute calculator: threadCacheHitRatio
 */
static bool A_threadCacheHitRatio(const StringMap *attributes, TCHAR *value)
{
   INT64 connectionsTotal = attributes->getInt64(_T("connectionsTotal"), -1);
   INT64 threadsCreated = attributes->getInt64(_T("threadsCreated"), -1);
   if ((connectionsTotal < 0) || (threadsCreated < 0))
      return false;
   _sntprintf(value, MAX_RESULT_LENGTH, _T("%1.2f"), 100.0 - ((connectionsTotal > 0) ? (double)threadsCreated / (double)connectionsTotal * 100.0 : (double)0));
   return true;
}

/**
 * Attribute calculator: threadsConnectedPerc
 */
static bool A_threadsConnectedPerc(const StringMap *attributes, TCHAR *value)
{
   INT64 connectionsLimit = attributes->getInt64(_T("connectionsLimit"), -1);
   INT64 threadsConnected = attributes->getInt64(_T("threadsConnected"), -1);
   if ((connectionsLimit <= 0) || (threadsConnected < 0))
      return false;
   _sntprintf(value, MAX_RESULT_LENGTH, _T("%1.2f"), (double)threadsConnected / (double)connectionsLimit * 100.0);
   return true;
}

/**
 * Data to query
 */
static GlobalData s_globalData[] =
{
   /* raw attributes */
   { _T("abortedClients"), _T("Aborted_clients"), NULL, NULL, NULL },
   { _T("abortedConnections"), _T("Aborted_connects"), NULL, NULL, NULL },
   { _T("bytesReceived"), _T("Bytes_received"), NULL, NULL, NULL },
   { _T("bytesSent"), _T("Bytes_sent"), NULL, NULL, NULL },
   { _T("connectionsLimit"), NULL, _T("MAX_CONNECTIONS"), NULL, NULL },
   { _T("connectionsTotal"), _T("Connections"), NULL, NULL, NULL },
   { _T("fragmentedTables"), NULL, NULL, _T("SELECT count(*) FROM information_schema.tables WHERE table_schema NOT IN ('information_schema','mysql') AND `Data_free` > 0 AND NOT `ENGINE`='MEMORY'"), NULL },
   { _T("innodbBufferPoolData"), _T("Innodb_buffer_pool_bytes_data"), NULL, NULL, NULL },
   { _T("innodbBufferPoolDirty"), _T("Innodb_buffer_pool_bytes_dirty"), NULL, NULL, NULL },
   { _T("innodbBufferPoolSize"), NULL, _T("innodb_buffer_pool_size"), NULL, NULL },
   { _T("innodbDiskReads"), _T("Innodb_buffer_pool_reads"), NULL, NULL, NULL },
   { _T("innodbReadRequests"), _T("Innodb_buffer_pool_read_requests"), NULL, NULL, NULL },
   { _T("innodbWriteRequests"), _T("Innodb_buffer_pool_write_requests"), NULL, NULL, NULL },
   { _T("maxOpenFiles"), NULL, _T("OPEN_FILES_LIMIT"), NULL, NULL },
   { _T("maxUsedConnections"), _T("Max_used_connections"), NULL, NULL, NULL },
   { _T("myISAMKeyCacheSize"), NULL, _T("key_buffer_size"), NULL, NULL },
   { _T("myISAMKeyCacheBlockSize"), NULL, _T("key_cache_block_size"), NULL, NULL },
   { _T("myISAMKeyCacheBlocksUnused"), _T("Key_blocks_unused"), NULL, NULL, NULL },
   { _T("myISAMKeyDiskReads"), _T("Key_reads"), NULL, NULL, NULL },
   { _T("myISAMKeyDiskWrites"), _T("Key_writes"), NULL, NULL, NULL },
   { _T("myISAMKeyReadRequests"), _T("Key_read_requests"), NULL, NULL, NULL },
   { _T("myISAMKeyWriteRequests"), _T("Key_write_requests"), NULL, NULL, NULL },
   { _T("openFiles"), _T("OPEN_FILES"), NULL, NULL, NULL },
   { _T("openedTables"), _T("Opened_tables"), NULL, NULL, NULL },
   { _T("openTables"), _T("Open_tables"), NULL, NULL, NULL },
   { _T("openTablesLimit"), NULL, _T("table_open_cache"), NULL, NULL },
   { _T("qcacheHits"), _T("Qcache_hits"), NULL, NULL, NULL },
   { _T("qcacheSize"), NULL, _T("query_cache_size"), NULL, NULL },
   { _T("queriesClientsTotal"), _T("Questions"), NULL, NULL, NULL },
   { _T("queriesDelete"), _T("Com_delete"), NULL, NULL, NULL },
   { _T("queriesDeleteMultiTable"), _T("Com_delete_multi"), NULL, NULL, NULL },
   { _T("queriesInsert"), _T("Com_insert"), NULL, NULL, NULL },
   { _T("queriesSelectNoCache"), _T("Com_select"), NULL, NULL, NULL },
   { _T("queriesTotal"), _T("Queries"), NULL, NULL, NULL },
   { _T("queriesUpdate"), _T("Com_update"), NULL, NULL, NULL },
   { _T("queriesUpdateMultiTable"), _T("Com_update_multi"), NULL, NULL, NULL },
   { _T("slowQueries"), _T("Slow_queries"), NULL, NULL, NULL },
   { _T("sortMergePasses"), _T("Sort_merge_passes"), NULL, NULL, NULL },
   { _T("sortRange"), _T("Sort_range"), NULL, NULL, NULL },
   { _T("sortScan"), _T("Sort_scan"), NULL, NULL, NULL },
   { _T("tempTablesCreated"), _T("Created_tmp_tables"), NULL, NULL, NULL },
   { _T("tempTablesCreatedOnDisk"), _T("Created_tmp_disk_tables"), NULL, NULL, NULL },
   { _T("threadCacheSize"), _T("thread_cache_size"), NULL, NULL, NULL },
   { _T("threadsConnected"), _T("Threads_connected"), NULL, NULL, NULL },
   { _T("threadsCreated"), _T("Threads_created"), NULL, NULL, NULL },
   { _T("threadsRunning"), _T("Threads_running"), NULL, NULL, NULL },
   { _T("uptime"), _T("Uptime"), NULL, NULL, NULL },
   /* calculated attributes */
   { _T("innodbBufferPoolDataPerc"), NULL, NULL, NULL, A_innodbBufferPoolDataPerc },
   { _T("innodbBufferPoolDirtyPerc"), NULL, NULL, NULL, A_innodbBufferPoolDirtyPerc },
   { _T("innodbBufferPoolFree"), NULL, NULL, NULL, A_innodbBufferPoolFree },
   { _T("innodbBufferPoolFreePerc"), NULL, NULL, NULL, A_innodbBufferPoolFreePerc },
   { _T("innodbReadCacheHitRatio"), NULL, NULL, NULL, A_innodbReadCacheHitRatio },
   { _T("maxUsedConnectionsPerc"), NULL, NULL, NULL, A_maxUsedConnectionsPerc },
   { _T("myISAMKeyCacheFree"), NULL, NULL, NULL, A_myISAMKeyCacheFree },
   { _T("myISAMKeyCacheFreePerc"), NULL, NULL, NULL, A_myISAMKeyCacheFreePerc },
   { _T("myISAMKeyCacheReadHitRatio"), NULL, NULL, NULL, A_myISAMKeyCacheReadHitRatio },
   { _T("myISAMKeyCacheUsed"), NULL, NULL, NULL, A_myISAMKeyCacheUsed },
   { _T("myISAMKeyCacheUsedPerc"), NULL, NULL, NULL, A_myISAMKeyCacheUsedPerc },
   { _T("myISAMKeyCacheWriteHitRatio"), NULL, NULL, NULL, A_myISAMKeyCacheWriteHitRatio },
   { _T("openFilesPerc"), NULL, NULL, NULL, A_openFilesPerc },
   { _T("openTablesPerc"), NULL, NULL, NULL, A_openTablesPerc },
   { _T("qcacheHitRatio"), NULL, NULL, NULL, A_qcacheHitRatio },
   { _T("queriesSelect"), NULL, NULL, NULL, A_queriesSelect },
   { _T("slowQueriesPerc"), NULL, NULL, NULL, A_slowQueriesPerc },
   { _T("sortMergeRatio"), NULL, NULL, NULL, A_sortMergeRatio },
   { _T("tempTablesCreatedOnDiskPerc"), NULL, NULL, NULL, A_tempTablesCreatedOnDiskPerc },
   { _T("threadCacheHitRatio"), NULL, NULL, NULL, A_threadCacheHitRatio },
   { _T("threadsConnectedPerc"), NULL, NULL, NULL, A_threadsConnectedPerc },
   { NULL, NULL, NULL, NULL, NULL }
};

/**
 * Create new database instance object
 */
DatabaseInstance::DatabaseInstance(DatabaseInfo *info)
{
   memcpy(&m_info, info, sizeof(DatabaseInfo));
	m_pollerThread = INVALID_THREAD_HANDLE;
	m_session = NULL;
	m_connected = false;
   m_data = NULL;
	m_dataLock = MutexCreate();
	m_sessionLock = MutexCreate();
   m_stopCondition = ConditionCreate(TRUE);
}

/**
 * Destructor
 */
DatabaseInstance::~DatabaseInstance()
{
   stop();
   MutexDestroy(m_dataLock);
   MutexDestroy(m_sessionLock);
   ConditionDestroy(m_stopCondition);
   delete m_data;
}

/**
 * Run
 */
void DatabaseInstance::run()
{
   m_pollerThread = ThreadCreateEx(DatabaseInstance::pollerThreadStarter, 0, this);
}

/**
 * Stop
 */
void DatabaseInstance::stop()
{
   ConditionSet(m_stopCondition);
   ThreadJoin(m_pollerThread);
   m_pollerThread = INVALID_THREAD_HANDLE;
   if (m_session != NULL)
   {
      DBDisconnect(m_session);
      m_session = NULL;
   }
}

/**
 * Poller thread starter
 */
THREAD_RESULT THREAD_CALL DatabaseInstance::pollerThreadStarter(void *arg)
{
   ((DatabaseInstance *)arg)->pollerThread();
   return THREAD_OK;
}

/**
 * Poller thread
 */
void DatabaseInstance::pollerThread()
{
   AgentWriteDebugLog(3, _T("MYSQL: poller thread for database %s started"), m_info.id);
   INT64 connectionTTL = (INT64)m_info.connectionTTL * _LL(1000);
   do
   {
reconnect:
      MutexLock(m_sessionLock);

      TCHAR errorText[DBDRV_MAX_ERROR_TEXT];
      m_session = DBConnect(g_mysqlDriver, m_info.server, m_info.name, m_info.login, m_info.password, NULL, errorText);
      if (m_session == NULL)
      {
         MutexUnlock(m_sessionLock);
         AgentWriteDebugLog(6, _T("MYSQL: cannot connect to database %s: %s"), m_info.id, errorText);
         continue;
      }

      m_connected = true;
		DBEnableReconnect(m_session, false);
      AgentWriteLog(NXLOG_INFO, _T("MYSQL: connection with database %s restored (connection TTL %d)"), m_info.id, m_info.connectionTTL);

      MutexUnlock(m_sessionLock);

      INT64 pollerLoopStartTime = GetCurrentTimeMs();
      UINT32 sleepTime;
      do
      {
         INT64 startTime = GetCurrentTimeMs();
         if (!poll())
         {
            AgentWriteLog(NXLOG_WARNING, _T("MYSQL: connection with database %s lost"), m_info.id);
            break;
         }
         INT64 currTime = GetCurrentTimeMs();
         if (currTime - pollerLoopStartTime > connectionTTL)
         {
            AgentWriteDebugLog(4, _T("MYSQL: planned connection reset"));
            MutexLock(m_sessionLock);
            m_connected = false;
            DBDisconnect(m_session);
            m_session = NULL;
            MutexUnlock(m_sessionLock);
            goto reconnect;
         }
         INT64 elapsedTime = currTime - startTime;
         sleepTime = (UINT32)((elapsedTime >= 60000) ? 60000 : (60000 - elapsedTime));
      }
      while(!ConditionWait(m_stopCondition, sleepTime));

      MutexLock(m_sessionLock);
      m_connected = false;
      DBDisconnect(m_session);
      m_session = NULL;
      MutexUnlock(m_sessionLock);
   }
   while(!ConditionWait(m_stopCondition, 60000));   // reconnect every 60 seconds
   AgentWriteDebugLog(3, _T("MYSQL: poller thread for database %s stopped"), m_info.id);
}

/**
 * Read global stats table into memory
 */
static StringMap *ReadGlobalStatsTable(DB_HANDLE hdb, const TCHAR *table)
{
   TCHAR query[128];
   _sntprintf(query, 128, _T("SELECT variable_name,variable_value FROM %s"), table);
   DB_RESULT hResult = DBSelect(hdb, query);
   if (hResult == NULL)
      return NULL;

   StringMap *data = new StringMap();
   int count = DBGetNumRows(hResult);
   for(int i = 0; i < count; i++)
      data->setPreallocated(DBGetField(hResult, i, 0, NULL, 0), DBGetField(hResult, i, 1, NULL, 0));
   DBFreeResult(hResult);
   return data;
}

/**
 * Do actual database polling. Should return false if connection is broken.
 */
bool DatabaseInstance::poll()
{
   // Query global stat tables
   StringMap *globalStatus = ReadGlobalStatsTable(m_session, _T("information_schema.global_status"));
   StringMap *globalVariables = ReadGlobalStatsTable(m_session, _T("information_schema.global_variables"));
   if ((globalStatus == NULL) || (globalVariables == NULL))
   {
      delete globalStatus;
      delete globalVariables;
      return false;
   }

   StringMap *data = new StringMap();

   int count = 0;
   int failures = 0;
   for(int i = 0; s_globalData[i].tag != NULL; i++)
   {
      count++;

      if (s_globalData[i].globalStatusVar != NULL)
      {
         const TCHAR *value = globalStatus->get(s_globalData[i].globalStatusVar);
         if (value != NULL)
            data->set(s_globalData[i].tag, value);
         else
            failures++;
      }
      else if (s_globalData[i].globalVariablesVar != NULL)
      {
         const TCHAR *value = globalVariables->get(s_globalData[i].globalVariablesVar);
         if (value != NULL)
            data->set(s_globalData[i].tag, value);
         else
            failures++;
      }
      else if (s_globalData[i].query != NULL)
      {
         DB_RESULT hResult = DBSelect(m_session, s_globalData[i].query);
         if (hResult != NULL)
         {
            if (DBGetNumRows(hResult) > 0)
            {
               data->setPreallocated(_tcsdup(s_globalData[i].tag), DBGetField(hResult, 0, 0, NULL, 0));
            }
            else
            {
               failures++;
            }
            DBFreeResult(hResult);
         }
         else
         {
            failures++;
         }
      }
      else if (s_globalData[i].calculator != NULL)
      {
         TCHAR buffer[256];
         if (s_globalData[i].calculator(data, buffer))
         {
            data->set(s_globalData[i].tag, buffer);
         }
         else
         {
            failures++;
         }
      }
   }

   delete globalStatus;
   delete globalVariables;

   // update cached data
   MutexLock(m_dataLock);
   delete m_data;
   m_data = data;
   MutexUnlock(m_dataLock);

   return failures < count;
}

/**
 * Get collected data
 */
bool DatabaseInstance::getData(const TCHAR *tag, TCHAR *value)
{
   bool success = false;
   MutexLock(m_dataLock);
   if (m_data != NULL)
   {
      const TCHAR *v = m_data->get(tag);
      if (v != NULL)
      {
         ret_string(value, v);
         success = true;
      }
   }
   MutexUnlock(m_dataLock);
   return success;
}
