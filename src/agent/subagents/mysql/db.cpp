/*
 ** NetXMS - Network Management System
 ** Subagent for MySQL monitoring
 ** Copyright (C) 2016-2026 Raden Solutions
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
#include <netxms-regex.h>

#define DEBUG_TAG _T("mysql")

/**
 * Data to query
 */
struct GlobalData
{
   const TCHAR *tag;
   const TCHAR *globalStatusVar;
   const TCHAR *globalVariablesVar;
   const TCHAR *query;
   bool (*calculator)(const StringMap*, TCHAR*);
};

/**
 * Attribute calculator: innodbBufferPoolDataPerc
 */
static bool A_innodbBufferPoolDataPerc(const StringMap *attributes, TCHAR *value)
{
   int64_t innodbBufferPoolSize = attributes->getInt64(_T("innodbBufferPoolSize"), -1);
   int64_t innodbBufferPoolData = attributes->getInt64(_T("innodbBufferPoolData"), -1);
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
   int64_t innodbBufferPoolSize = attributes->getInt64(_T("innodbBufferPoolSize"), -1);
   int64_t innodbBufferPoolDirty = attributes->getInt64(_T("innodbBufferPoolDirty"), -1);
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
   int64_t innodbBufferPoolSize = attributes->getInt64(_T("innodbBufferPoolSize"), -1);
   int64_t innodbBufferPoolData = attributes->getInt64(_T("innodbBufferPoolData"), -1);
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
   int64_t innodbBufferPoolSize = attributes->getInt64(_T("innodbBufferPoolSize"), -1);
   int64_t innodbBufferPoolFree = attributes->getInt64(_T("innodbBufferPoolFree"), -1);
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
   int64_t innodbDiskReads = attributes->getInt64(_T("innodbDiskReads"), -1);
   int64_t innodbReadRequests = attributes->getInt64(_T("innodbReadRequests"), -1);
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
   int64_t connectionsLimit = attributes->getInt64(_T("connectionsLimit"), -1);
   int64_t maxUsedConnections = attributes->getInt64(_T("maxUsedConnections"), -1);
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
   int64_t myISAMKeyCacheBlockSize = attributes->getInt64(_T("myISAMKeyCacheBlockSize"), -1);
   int64_t myISAMKeyCacheBlocksUnused = attributes->getInt64(_T("myISAMKeyCacheBlocksUnused"), -1);
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
   int64_t myISAMKeyCacheSize = attributes->getInt64(_T("myISAMKeyCacheSize"), -1);
   int64_t myISAMKeyCacheFree = attributes->getInt64(_T("myISAMKeyCacheFree"), -1);
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
   int64_t myISAMKeyDiskReads = attributes->getInt64(_T("myISAMKeyDiskReads"), -1);
   int64_t myISAMKeyReadRequests = attributes->getInt64(_T("myISAMKeyReadRequests"), -1);
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
   int64_t myISAMKeyCacheSize = attributes->getInt64(_T("myISAMKeyCacheSize"), -1);
   int64_t myISAMKeyCacheFree = attributes->getInt64(_T("myISAMKeyCacheFree"), -1);
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
   int64_t myISAMKeyCacheSize = attributes->getInt64(_T("myISAMKeyCacheSize"), -1);
   int64_t myISAMKeyCacheUsed = attributes->getInt64(_T("myISAMKeyCacheUsed"), -1);
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
   int64_t myISAMKeyDiskWrites = attributes->getInt64(_T("myISAMKeyDiskWrites"), -1);
   int64_t myISAMKeyWriteRequests = attributes->getInt64(_T("myISAMKeyWriteRequests"), -1);
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
   int64_t maxOpenFiles = attributes->getInt64(_T("maxOpenFiles"), -1);
   int64_t openFiles = attributes->getInt64(_T("openFiles"), -1);
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
   int64_t openTablesLimit = attributes->getInt64(_T("openTablesLimit"), -1);
   int64_t openTables = attributes->getInt64(_T("openTables"), -1);
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
   int64_t qcacheHits = attributes->getInt64(_T("qcacheHits"), -1);
   int64_t queriesSelectNoCache = attributes->getInt64(_T("queriesSelectNoCache"), -1);
   if ((queriesSelectNoCache < 0) || (qcacheHits < 0))
      return false;
   int64_t totalSelects = qcacheHits + queriesSelectNoCache;
   _sntprintf(value, MAX_RESULT_LENGTH, _T("%1.2f"), (totalSelects > 0) ? (double)qcacheHits / (double)totalSelects * 100.0 : (double)0);
   return true;
}

/**
 * Attribute calculator: queriesSelect
 */
static bool A_queriesSelect(const StringMap *attributes, TCHAR *value)
{
   int64_t qcacheHits = attributes->getInt64(_T("qcacheHits"), -1);
   int64_t queriesSelectNoCache = attributes->getInt64(_T("queriesSelectNoCache"), -1);
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
   int64_t slowQueriesPerc = attributes->getInt64(_T("slowQueries"), -1);
   int64_t queriesTotal = attributes->getInt64(_T("queriesTotal"), -1);
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
   int64_t sortMergePasses = attributes->getInt64(_T("sortMergePasses"), -1);
   int64_t sortRange = attributes->getInt64(_T("sortRange"), -1);
   int64_t sortScan = attributes->getInt64(_T("sortScan"), -1);
   if ((sortMergePasses < 0) || (sortRange < 0) || (sortScan < 0))
      return false;
   int64_t sorts = sortRange + sortScan;
   _sntprintf(value, MAX_RESULT_LENGTH, _T("%1.2f"), (sorts > 0) ? (double)sortMergePasses / (double)sorts * 100.0 : (double)0);
   return true;
}

/**
 * Attribute calculator: tempTablesCreatedOnDiskPerc
 */
static bool A_tempTablesCreatedOnDiskPerc(const StringMap *attributes, TCHAR *value)
{
   int64_t tempTablesCreated = attributes->getInt64(_T("tempTablesCreated"), -1);
   int64_t tempTablesCreatedOnDisk = attributes->getInt64(_T("tempTablesCreatedOnDisk"), -1);
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
   int64_t connectionsTotal = attributes->getInt64(_T("connectionsTotal"), -1);
   int64_t threadsCreated = attributes->getInt64(_T("threadsCreated"), -1);
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
   int64_t connectionsLimit = attributes->getInt64(_T("connectionsLimit"), -1);
   int64_t threadsConnected = attributes->getInt64(_T("threadsConnected"), -1);
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
 * Per-database (per-schema) statistics query. The first result column is the schema name
 * (used as instance); every remaining column is stored in the cached data map under the key
 * "<columnAlias>@<schema>". Tier 2 queries require performance schema instrumentation and are
 * skipped on MariaDB and MySQL < 5.7.
 */
struct PerDatabaseQuery
{
   bool requiresPerformanceSchema;
   const TCHAR *query;
};

/**
 * Per-database statistics queries
 */
static PerDatabaseQuery s_perDatabaseQueries[] =
{
   // Tier 1: storage statistics from information_schema (always available)
   { false,
     _T("SELECT TABLE_SCHEMA AS instance, COUNT(*) AS tableCount, ")
     _T("COALESCE(SUM(DATA_LENGTH),0) AS dataSize, COALESCE(SUM(INDEX_LENGTH),0) AS indexSize, ")
     _T("COALESCE(SUM(DATA_LENGTH+INDEX_LENGTH),0) AS totalSize, COALESCE(SUM(TABLE_ROWS),0) AS rowCount, ")
     _T("COALESCE(SUM(DATA_FREE),0) AS freeSpace, ")
     _T("SUM(CASE WHEN DATA_FREE > 0 AND ENGINE <> 'MEMORY' THEN 1 ELSE 0 END) AS fragmentedTables ")
     _T("FROM information_schema.TABLES WHERE TABLE_TYPE='BASE TABLE' ")
     _T("AND TABLE_SCHEMA NOT IN ('information_schema','performance_schema') GROUP BY TABLE_SCHEMA")
   },
   // Tier 2: activity statistics from performance schema (MySQL >= 5.7)
   { true,
     _T("SELECT OBJECT_SCHEMA AS instance, COALESCE(SUM(COUNT_READ),0) AS rowsRead, ")
     _T("COALESCE(SUM(COUNT_FETCH),0) AS rowsFetched, COALESCE(SUM(COUNT_INSERT),0) AS rowsInserted, ")
     _T("COALESCE(SUM(COUNT_UPDATE),0) AS rowsUpdated, COALESCE(SUM(COUNT_DELETE),0) AS rowsDeleted, ")
     _T("COALESCE(ROUND(SUM(SUM_TIMER_WAIT)/1000000000),0) AS ioWaitTime ")
     _T("FROM performance_schema.table_io_waits_summary_by_table ")
     _T("WHERE OBJECT_SCHEMA IS NOT NULL AND OBJECT_SCHEMA NOT IN ('information_schema','mysql','performance_schema','sys') ")
     _T("GROUP BY OBJECT_SCHEMA")
   },
   { false, nullptr }
};

/**
 * Collect per-database (per-schema) statistics into the cached data map. Each metric is stored
 * under the key "<columnAlias>@<schema>". Best-effort: a failed query is logged but does not mark
 * the connection as broken, so an absent or disabled performance schema does not disrupt polling.
 */
static void ReadPerDatabaseStats(DB_HANDLE hdb, bool usePerformanceSchema, const TCHAR *connId, StringMap *data)
{
   for(int i = 0; s_perDatabaseQueries[i].query != nullptr; i++)
   {
      if (s_perDatabaseQueries[i].requiresPerformanceSchema && !usePerformanceSchema)
         continue;

      DB_RESULT hResult = DBSelect(hdb, s_perDatabaseQueries[i].query);
      if (hResult == nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("MYSQL: per-database statistics query failed for connection %s"), connId);
         continue;
      }

      int numColumns = DBGetColumnCount(hResult);
      int numRows = DBGetNumRows(hResult);
      for(int row = 0; row < numRows; row++)
      {
         TCHAR schema[128];
         DBGetField(hResult, row, 0, schema, 128);
         if (schema[0] == 0)
            continue;

         for(int col = 1; col < numColumns; col++)
         {
            TCHAR column[128], tag[256];
            DBGetColumnName(hResult, col, column, 128);
            _sntprintf(tag, 256, _T("%s@%s"), column, schema);
            data->setPreallocated(MemCopyString(tag), DBGetField(hResult, row, col, nullptr, 0));
         }
      }
      DBFreeResult(hResult);
   }
}

/**
 * Create new database instance object
 */
DatabaseConnection::DatabaseConnection(ConnectionInfo *info) : m_dataLock(MutexType::FAST), m_sessionLock(MutexType::NORMAL), m_stopCondition(true)
{
   memcpy(&m_info, info, sizeof(ConnectionInfo));
	m_pollerThread = INVALID_THREAD_HANDLE;
	m_session = nullptr;
	m_connected = false;
   m_data = nullptr;
   m_usePerformanceSchema = true;
}

/**
 * Destructor
 */
DatabaseConnection::~DatabaseConnection()
{
   stop();
   delete m_data;
}

/**
 * Run
 */
void DatabaseConnection::run()
{
   m_pollerThread = ThreadCreateEx(DatabaseConnection::pollerThreadStarter, 0, this);
}

/**
 * Stop
 */
void DatabaseConnection::stop()
{
   m_stopCondition.set();
   ThreadJoin(m_pollerThread);
   m_pollerThread = INVALID_THREAD_HANDLE;
   if (m_session != nullptr)
   {
      DBDisconnect(m_session);
      m_session = nullptr;
   }
}

/**
 * Checks if database server is mysql and version is > 5.7.
 * Sets m_usePerformanceSchema flag.
 */
void DatabaseConnection::checkMySQLVersion()
{
   m_usePerformanceSchema = true;

   DB_RESULT hResult = DBSelect(m_session, _T("SELECT VERSION()"));

   if (hResult != nullptr)
   {
      TCHAR version[256];
      DBGetField(hResult, 0, 0, version, 256);

      if (_tcsstr(_tcslwr(version), _T("mariadb")) == nullptr) // MySQL
      {
         if (version[1] == _T('.')) // Version is X.*
         {
            if (version[0] < _T('5') || (version[0] == _T('5') && version[2] < _T('7'))) // Version is < 5.7
            {
               m_usePerformanceSchema = false;
            }
         }
      }
      else // MariaDB
      {
         m_usePerformanceSchema = false;
      }
      DBFreeResult(hResult);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("MYSQL: failed to check version for database %s"), m_info.id);
   }
}

/**
 * Poller thread starter
 */
THREAD_RESULT THREAD_CALL DatabaseConnection::pollerThreadStarter(void *arg)
{
   ((DatabaseConnection *)arg)->pollerThread();
   return THREAD_OK;
}

/**
 * Poller thread
 */
void DatabaseConnection::pollerThread()
{
   nxlog_debug_tag(DEBUG_TAG, 3, _T("MYSQL: poller thread for database %s started"), m_info.id);
   INT64 connectionTTL = (INT64)m_info.connectionTTL * _LL(1000);
   do
   {
reconnect:
      m_sessionLock.lock();

      TCHAR errorText[DBDRV_MAX_ERROR_TEXT];
      m_session = DBConnect(g_mysqlDriver, m_info.endpoint, m_info.database, m_info.login, m_info.password, nullptr, errorText);
      if (m_session == nullptr)
      {
         m_sessionLock.unlock();
         nxlog_debug_tag(DEBUG_TAG, 6, _T("MYSQL: cannot connect to database %s: %s"), m_info.id, errorText);
         continue;
      }

      m_connected = true;
		DBEnableReconnect(m_session, false);
      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("MYSQL: connection with database %s restored (connection TTL %d)"), m_info.id, m_info.connectionTTL);

      m_sessionLock.unlock();

      checkMySQLVersion();

      int64_t pollerLoopStartTime = GetCurrentTimeMs();
      uint32_t sleepTime;
      do
      {
         INT64 startTime = GetCurrentTimeMs();
         if (!poll())
         {
            nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("MYSQL: connection with database %s lost"), m_info.id);
            break;
         }
         INT64 currTime = GetCurrentTimeMs();
         if (currTime - pollerLoopStartTime > connectionTTL)
         {
            nxlog_debug_tag(DEBUG_TAG, 4, _T("MYSQL: planned connection reset"));
            m_sessionLock.lock();
            m_connected = false;
            DBDisconnect(m_session);
            m_session = nullptr;
            m_sessionLock.unlock();
            goto reconnect;
         }
         int64_t elapsedTime = currTime - startTime;
         sleepTime = (uint32_t)((elapsedTime >= 60000) ? 60000 : (60000 - elapsedTime));
      }
      while(!m_stopCondition.wait(sleepTime));

      m_sessionLock.lock();
      m_connected = false;
      DBDisconnect(m_session);
      m_session = nullptr;
      m_sessionLock.unlock();
   }
   while(!m_stopCondition.wait(60000));   // reconnect every 60 seconds
   nxlog_debug_tag(DEBUG_TAG, 3, _T("MYSQL: poller thread for database %s stopped"), m_info.id);
}

/**
 * Read statement counts from performance schema events_statements_summary table
 * Returns a StringMap with Com_% style keys, or nullptr on failure
 */
static StringMap *ReadStatementCounts(DB_HANDLE hdb)
{
   static const struct
   {
      const TCHAR *eventName;
      const TCHAR *comName;
   } statementMapping[] =
   {
      { _T("statement/sql/select"), _T("Com_select") },
      { _T("statement/sql/insert"), _T("Com_insert") },
      { _T("statement/sql/update"), _T("Com_update") },
      { _T("statement/sql/delete"), _T("Com_delete") },
      { _T("statement/sql/update_multi"), _T("Com_update_multi") },
      { _T("statement/sql/delete_multi"), _T("Com_delete_multi") },
      { nullptr, nullptr }
   };

   DB_RESULT hResult = DBSelect(hdb,
      _T("SELECT EVENT_NAME, COUNT_STAR FROM performance_schema.events_statements_summary_global_by_event_name ")
      _T("WHERE EVENT_NAME LIKE 'statement/sql/%'"));

   if (hResult == nullptr)
      return nullptr;

   StringMap *data = new StringMap();
   int count = DBGetNumRows(hResult);
   for (int i = 0; i < count; i++)
   {
      TCHAR eventName[256];
      DBGetField(hResult, i, 0, eventName, 256);

      // Map event name to Com_% style name
      for (int j = 0; statementMapping[j].eventName != nullptr; j++)
      {
         if (_tcscmp(eventName, statementMapping[j].eventName) == 0)
         {
            data->setPreallocated(MemCopyString(statementMapping[j].comName), DBGetField(hResult, i, 1, nullptr, 0));
            break;
         }
      }
   }
   DBFreeResult(hResult);

   return data;
}

/**
 * Read COM_% status variables using SHOW GLOBAL STATUS (fallback method)
 */
static StringMap *ReadComStatusViaShow(DB_HANDLE hdb)
{
   DB_RESULT hResult = DBSelect(hdb, _T("SHOW GLOBAL STATUS LIKE 'Com_%'"));
   if (hResult == nullptr)
      return nullptr;

   StringMap *data = new StringMap();
   int count = DBGetNumRows(hResult);
   for (int i = 0; i < count; i++)
   {
      data->setPreallocated(DBGetField(hResult, i, 0, nullptr, 0), DBGetField(hResult, i, 1, nullptr, 0));
   }
   DBFreeResult(hResult);
   return data;
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
bool DatabaseConnection::poll()
{
   // Query global stat tables
   StringMap *globalStatus;
   StringMap *globalVariables;

   if (m_usePerformanceSchema)
   {
      nxlog_debug_tag(DEBUG_TAG, 7, _T("MYSQL: using performance schema for %s database during polling"), m_info.id);
      globalStatus = ReadGlobalStatsTable(m_session, _T("performance_schema.global_status"));
      globalVariables = ReadGlobalStatsTable(m_session, _T("performance_schema.global_variables"));

      // Get statement counts from events_statements_summary (COM_% variables are not in performance_schema.global_status)
      StringMap *statementCounts = ReadStatementCounts(m_session);
      if (statementCounts == nullptr || statementCounts->size() == 0)
      {
         delete statementCounts;
         nxlog_debug_tag(DEBUG_TAG, 5, _T("MYSQL: events_statements_summary not available for %s, falling back to SHOW STATUS"), m_info.id);
         statementCounts = ReadComStatusViaShow(m_session);
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 7, _T("MYSQL: using events_statements_summary for statement counts for %s"), m_info.id);
      }

      // Merge statement counts into globalStatus
      if (statementCounts != nullptr && globalStatus != nullptr)
      {
         globalStatus->addAll(statementCounts);
         delete statementCounts;
      }
      else
      {
         delete statementCounts;
      }
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 7, _T("MYSQL: using information schema for %s database during polling"), m_info.id);
      globalStatus = ReadGlobalStatsTable(m_session, _T("information_schema.global_status"));
      globalVariables = ReadGlobalStatsTable(m_session, _T("information_schema.global_variables"));
   }

   if ((globalStatus == nullptr) || (globalVariables == nullptr))
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
               data->setPreallocated(MemCopyString(s_globalData[i].tag), DBGetField(hResult, 0, 0, NULL, 0));
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

   // Collect per-database (per-schema) statistics into the same data map (best-effort)
   ReadPerDatabaseStats(m_session, m_usePerformanceSchema, m_info.id, data);

   // update cached data
   m_dataLock.lock();
   delete m_data;
   m_data = data;
   m_dataLock.unlock();

   return failures < count;
}

/**
 * Get collected data
 */
bool DatabaseConnection::getData(const TCHAR *tag, TCHAR *value)
{
   bool success = false;
   m_dataLock.lock();
   if (m_data != nullptr)
   {
      const TCHAR *v = m_data->get(tag);
      if (v != nullptr)
      {
         ret_string(value, v);
         success = true;
      }
   }
   m_dataLock.unlock();
   return success;
}

/**
 * Tag list callback data
 */
struct TagListCallbackData
{
   PCRE *preg;
   StringList *list;
};

/**
 * Tag list callback
 */
static EnumerationCallbackResult TagListCallback(const TCHAR *key, const TCHAR *value, TagListCallbackData *data)
{
   int pmatch[9];
   if (_pcre_exec_t(data->preg, nullptr, reinterpret_cast<const PCRE_TCHAR*>(key), static_cast<int>(_tcslen(key)), 0, 0, pmatch, 9) >= 2)
   {
      size_t slen = pmatch[3] - pmatch[2];
      Buffer<TCHAR, 256> s(slen + 1);
      memcpy(s.buffer(), &key[pmatch[2]], slen * sizeof(TCHAR));
      s[slen] = 0;
      data->list->add(s);
   }
   return _CONTINUE;
}

/**
 * Get list of instances captured by group 1 of given pattern from collected data
 */
bool DatabaseConnection::getTagList(const TCHAR *pattern, StringList *value)
{
   bool success = false;
   m_dataLock.lock();
   if (m_data != nullptr)
   {
      const char *eptr;
      int eoffset;
      TagListCallbackData data;
      data.list = value;
      data.preg = _pcre_compile_t(reinterpret_cast<const PCRE_TCHAR*>(pattern), PCRE_COMMON_FLAGS | PCRE_CASELESS, &eptr, &eoffset, nullptr);
      if (data.preg != nullptr)
      {
         m_data->forEach(TagListCallback, &data);
         _pcre_free_t(data.preg);
         success = true;
      }
   }
   m_dataLock.unlock();
   return success;
}
