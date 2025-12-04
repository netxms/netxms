/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2024 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: dbwrite.cpp
**
**/

#include "nxcore.h"
#include <uthash.h>

#define DEBUG_TAG _T("db.writer")

/**
 * Delayed SQL request
 */
struct DELAYED_SQL_REQUEST
{
   TCHAR *query;
   int bindCount;
   BYTE *sqlTypes;
   TCHAR *bindings[1]; /* actual size determined by bindCount field */
};

/**
 * Delayed request for idata_ INSERT
 */
struct DELAYED_IDATA_INSERT
{
   Timestamp timestamp;
   uint32_t nodeId;
   uint32_t dciId;
   TCHAR *transformedValue;
   TCHAR rawValue[2]; // Actual size determined by text part length
};

/**
 * Delayed request for raw_dci_values UPDATE or DELETE
 */
struct DELAYED_RAW_DATA_UPDATE
{
   UT_hash_handle hh;
   Timestamp timestamp;
   Timestamp cacheTimestamp;
   uint32_t dciId;
   bool anomalyDetected;
   bool deleteFlag;
   TCHAR *transformedValue;
   TCHAR rawValue[2];  // Actual size determined by text part length
};

/**
 * IData writer
 */
struct IDataWriter
{
   THREAD thread;
   ObjectQueue<DELAYED_IDATA_INSERT> *queue;
   const TCHAR *storageClass;
   int workerCount;   // Number of additional worker threads
   VolatileCounter pendingRequests;  // Requests taken from queue but not completed yet
};

/**
 * Maximum possible number of IData writers
 */
#define MAX_IDATA_WRITERS  64

/**
 * Configured number of IData writers
 */
static int s_idataWriterCount = 1;

/**
 * IData writers
 */
static IDataWriter s_idataWriters[MAX_IDATA_WRITERS];

/**
 * Custom destructor for writer queue
 */
static void WriterQueueElementDestructor(void *element, Queue *queue)
{
   MemFree(element);
}

/**
 * Generic DB writer queue
 */
ObjectQueue<DELAYED_SQL_REQUEST> g_dbWriterQueue(1024, Ownership::True, WriterQueueElementDestructor);

/**
 * Raw DCI data writer queue
 */
static DELAYED_RAW_DATA_UPDATE *s_rawDataWriterQueue = nullptr;
static Mutex s_rawDataWriterLock;
static VolatileCounter s_batchSize = 0;

/**
 * Performance counters
 */
VolatileCounter64 g_idataWriteRequests = 0;
uint64_t g_rawDataWriteRequests = 0;
VolatileCounter64 g_otherWriteRequests = 0;

/**
 * Queue monitor data
 */
static bool s_queueMonitorDiscardFlag = false;  // true when new data should be discarded
static Mutex s_queueMonitorStateLock(MutexType::FAST);
static Condition s_queueMonitorStopCondition(true);

/**
 * Static data
 */
static THREAD s_writerThread = INVALID_THREAD_HANDLE;
static THREAD s_rawDataWriterThread = INVALID_THREAD_HANDLE;
static THREAD s_queueMonitorThread = INVALID_THREAD_HANDLE;

/**
 * IDATA tables write lock
 */
static RWLock s_idataWriteLock;
static bool s_idataWriteLockActive = false;

/**
 * Lock IDATA writes (used by housekeeper)
 */
void LockIDataWrites()
{
   if (g_flags & AF_DBWRITER_HK_INTERLOCK)
   {
      s_idataWriteLock.writeLock();
      s_idataWriteLockActive = true;
   }
}

/**
 * Unlock IDATA writes
 */
void UnlockIDataWrites()
{
   if (s_idataWriteLockActive)
   {
      s_idataWriteLockActive = false;
      s_idataWriteLock.unlock();
   }
}

/**
 * Put SQL request into queue for later execution
 */
void NXCORE_EXPORTABLE QueueSQLRequest(const TCHAR *query)
{
	DELAYED_SQL_REQUEST *rq = static_cast<DELAYED_SQL_REQUEST*>(MemAlloc(sizeof(DELAYED_SQL_REQUEST) + (_tcslen(query) + 1) * sizeof(TCHAR)));
	rq->query = (TCHAR *)&rq->bindings[0];
	_tcscpy(rq->query, query);
	rq->bindCount = 0;
   g_dbWriterQueue.put(rq);
   nxlog_debug_tag(DEBUG_TAG, 8, _T("SQL request queued: %s"), query);
	InterlockedIncrement64(&g_otherWriteRequests);
}

/**
 * Put parameterized SQL request into queue for later execution
 */
void NXCORE_EXPORTABLE QueueSQLRequest(const TCHAR *query, int bindCount, int *sqlTypes, const TCHAR **values)
{
	size_t size = sizeof(DELAYED_SQL_REQUEST) + (_tcslen(query) + 1) * sizeof(TCHAR) + bindCount * sizeof(TCHAR *) + bindCount;
	for(int i = 0; i < bindCount; i++)
	{
	   if (values[i] != nullptr)
	      size += (_tcslen(values[i]) + 1) * sizeof(TCHAR) + sizeof(TCHAR *);
	}
	DELAYED_SQL_REQUEST *rq = static_cast<DELAYED_SQL_REQUEST*>(MemAlloc(size));

	BYTE *base = (BYTE *)&rq->bindings[bindCount];
	size_t pos = 0;
	size_t align = sizeof(TCHAR *);

	rq->query = (TCHAR *)base;
	_tcscpy(rq->query, query);
	rq->bindCount = bindCount;
	pos += ((int)_tcslen(query) + 1) * sizeof(TCHAR);

	rq->sqlTypes = &base[pos];
	pos += bindCount;
	if (pos % align != 0)
		pos += align - pos % align;

	for(int i = 0; i < bindCount; i++)
	{
		rq->sqlTypes[i] = (BYTE)sqlTypes[i];
		if (values[i] != nullptr)
		{
         rq->bindings[i] = (TCHAR *)&base[pos];
         _tcscpy(rq->bindings[i], values[i]);
         pos += (_tcslen(values[i]) + 1) * sizeof(TCHAR);
         if (pos % align != 0)
            pos += align - pos % align;
		}
		else
		{
		   rq->bindings[i] = nullptr;
		}
	}

   g_dbWriterQueue.put(rq);
   nxlog_debug_tag(DEBUG_TAG, 8, _T("SQL request queued: %s"), query);
   InterlockedIncrement64(&g_otherWriteRequests);
}

/**
 * Queue INSERT request for idata_xxx table
 */
void QueueIDataInsert(Timestamp timestamp, uint32_t nodeId, uint32_t dciId, const TCHAR *rawValue, const TCHAR *transformedValue, DCObjectStorageClass storageClass)
{
   if (s_queueMonitorDiscardFlag)
      return;

   size_t rawValueLength = _tcslen(rawValue);
   size_t transformedValueLength = _tcslen(transformedValue);
	auto rq = static_cast<DELAYED_IDATA_INSERT*>(MemAlloc(sizeof(DELAYED_IDATA_INSERT) + (rawValueLength + transformedValueLength) * sizeof(TCHAR)));
	rq->timestamp = timestamp;
	rq->nodeId = nodeId;
	rq->dciId = dciId;
   rq->transformedValue = rq->rawValue + rawValueLength + 1;
   memcpy(rq->rawValue, rawValue, (rawValueLength + 1) * sizeof(TCHAR));
   memcpy(rq->transformedValue, transformedValue, (transformedValueLength + 1) * sizeof(TCHAR));
   if ((g_flags & AF_SINGLE_TABLE_PERF_DATA) && (g_dbSyntax == DB_SYNTAX_TSDB))
   {
      s_idataWriters[static_cast<int>(storageClass)].queue->put(rq);
   }
   else if (s_idataWriterCount > 1)
   {
      int hash = nodeId % s_idataWriterCount;
      s_idataWriters[hash].queue->put(rq);
   }
   else
   {
      s_idataWriters[0].queue->put(rq);
   }
	InterlockedIncrement64(&g_idataWriteRequests);
}

/**
 * Queue UPDATE request for raw_dci_values table
 */
void QueueRawDciDataUpdate(Timestamp timestamp, uint32_t dciId, const TCHAR *rawValue, const TCHAR *transformedValue, Timestamp cacheTimestamp, bool anomalyDetected)
{
   size_t rawValueLength = _tcslen(rawValue);
   size_t transformedValueLength = _tcslen(transformedValue);
   auto rq = static_cast<DELAYED_RAW_DATA_UPDATE*>(MemAlloc(sizeof(DELAYED_RAW_DATA_UPDATE) + (rawValueLength + transformedValueLength) * sizeof(TCHAR)));
   rq->dciId = dciId;
   rq->timestamp = timestamp;
   rq->cacheTimestamp = cacheTimestamp;
   rq->anomalyDetected = anomalyDetected;
   rq->deleteFlag = false;
   rq->transformedValue = rq->rawValue + rawValueLength + 1;
   memcpy(rq->rawValue, rawValue, (rawValueLength + 1) * sizeof(TCHAR));
   memcpy(rq->transformedValue, transformedValue, (transformedValueLength + 1) * sizeof(TCHAR));

   s_rawDataWriterLock.lock();
   DELAYED_RAW_DATA_UPDATE *curr;
   HASH_FIND_INT(s_rawDataWriterQueue, &dciId, curr);
   if (curr != nullptr)
   {
      HASH_DEL(s_rawDataWriterQueue, curr);
      MemFree(curr);
   }
   HASH_ADD_INT(s_rawDataWriterQueue, dciId, rq);
   g_rawDataWriteRequests++;
   s_rawDataWriterLock.unlock();
}

/**
 * Queue DELETE request for raw_dci_values table
 */
void QueueRawDciDataDelete(uint32_t dciId)
{
   s_rawDataWriterLock.lock();
   DELAYED_RAW_DATA_UPDATE *rq;
   HASH_FIND_INT(s_rawDataWriterQueue, &dciId, rq);
   if (rq == nullptr)
   {
      rq = MemAllocStruct<DELAYED_RAW_DATA_UPDATE>();
      rq->dciId = dciId;
      HASH_ADD_INT(s_rawDataWriterQueue, dciId, rq);
   }
   rq->deleteFlag = true;
   g_rawDataWriteRequests++;
   s_rawDataWriterLock.unlock();
}

/**
 * Database "lazy" write thread
 */
static void DBWriteThread()
{
   ThreadSetName("DBWriter");
   while(true)
   {
      DELAYED_SQL_REQUEST *rq = g_dbWriterQueue.getOrBlock();
      if (rq == INVALID_POINTER_VALUE)   // End-of-job indicator
         break;

      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

		if (rq->bindCount == 0)
		{
			DBQuery(hdb, rq->query);
		}
		else
		{
			DB_STATEMENT hStmt = DBPrepare(hdb, rq->query);
			if (hStmt != nullptr)
			{
				for(int i = 0; i < rq->bindCount; i++)
				{
					DBBind(hStmt, i + 1, (int)rq->sqlTypes[i], rq->bindings[i], DB_BIND_STATIC);
				}
				DBExecute(hStmt);
				DBFreeStatement(hStmt);
			}
		}
      MemFree(rq);

      DBConnectionPoolReleaseConnection(hdb);
   }
}

/**
 * Database "lazy" write thread for idata_xxx INSERTs
 */
static void IDataWriteThread(IDataWriter *writer)
{
   ThreadSetName("DBWriter/IData");
   int maxRecords = ConfigReadInt(_T("DBWriter.MaxRecordsPerTransaction"), 1000);

   StringBuffer query;
   query.setAllocationStep(1024);

   while(true)
   {
		DELAYED_IDATA_INSERT *rq = writer->queue->getOrBlock();
      if (rq == INVALID_POINTER_VALUE)   // End-of-job indicator
         break;

      bool idataLock;
      if (g_flags & AF_DBWRITER_HK_INTERLOCK)
      {
         s_idataWriteLock.readLock();
         idataLock = true;
      }
      else
      {
         idataLock = false;
      }

      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
		if (DBBegin(hdb))
		{
			int count = 0;
			while(true)
			{
				bool success;

            query.clear(false);

				// For Oracle preparing statement even for one time execution is preferred
				// For other databases it will actually slow down inserts
				if (g_dbSyntax == DB_SYNTAX_ORACLE)
				{
				   query.append(_T("INSERT INTO idata_")).append(rq->nodeId).append(_T(" (item_id,idata_timestamp,idata_value,raw_value) VALUES (?,?,?,?)"));
               DB_STATEMENT hStmt = DBPrepare(hdb, query);
               if (hStmt != nullptr)
               {
                  DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, rq->dciId);
                  DBBind(hStmt, 2, DB_SQLTYPE_BIGINT, rq->timestamp);
                  DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, rq->transformedValue, DB_BIND_STATIC);
                  DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, rq->rawValue, DB_BIND_STATIC);
                  success = DBExecute(hStmt);
                  DBFreeStatement(hStmt);
               }
               else
               {
                  success = false;
               }
				}
				else
				{
               query
                  .append(_T("INSERT INTO idata_"))
                  .append(rq->nodeId)
                  .append(_T(" (item_id,idata_timestamp,idata_value,raw_value) VALUES ("))
                  .append(rq->dciId)
                  .append(_T(','))
                  .append(rq->timestamp)
                  .append(_T(','))
                  .append(DBPrepareString(hdb, rq->transformedValue))
                  .append(_T(','))
                  .append(DBPrepareString(hdb, rq->rawValue))
                  .append(_T(')'));
               success = DBQuery(hdb, query);
				}

				MemFree(rq);

				count++;
				if (!success || (count > maxRecords))
					break;

				rq = writer->queue->getOrBlock(500);
				if ((rq == nullptr) || (rq == INVALID_POINTER_VALUE))
					break;
			}
			DBCommit(hdb);
		}
		else
		{
			MemFree(rq);
		}
		DBConnectionPoolReleaseConnection(hdb);

		if (idataLock)
		   s_idataWriteLock.unlock();

      if (rq == INVALID_POINTER_VALUE)   // End-of-job indicator
         break;
	}
}

/**
 * Database "lazy" write thread for idata INSERTs - generic version
 */
static void IDataWriteThreadSingleTable_Generic(IDataWriter *writer)
{
   ThreadSetName("DBWriter/IData");
   int maxRecords = ConfigReadInt(_T("DBWriter.MaxRecordsPerTransaction"), 1000);

   StringBuffer query;
   query.setAllocationStep(1024);

   while(true)
   {
      DELAYED_IDATA_INSERT *rq = writer->queue->getOrBlock();
      if (rq == INVALID_POINTER_VALUE)   // End-of-job indicator
         break;

      bool idataLock;
      if (g_flags & AF_DBWRITER_HK_INTERLOCK)
      {
         s_idataWriteLock.readLock();
         idataLock = true;
      }
      else
      {
         idataLock = false;
      }

      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
      if (DBBegin(hdb))
      {
         int count = 0;
         while(true)
         {
            query.clear(false);
            query.append(_T("INSERT INTO idata (item_id,idata_timestamp,idata_value,raw_value) VALUES ("));
            query.append(rq->dciId);
            query.append(_T(','));
            query.append(rq->timestamp);
            query.append(_T(','));
            query.append(DBPrepareString(hdb, rq->transformedValue));
            query.append(_T(','));
            query.append(DBPrepareString(hdb, rq->rawValue));
            query.append(_T(')'));

            bool success = DBQuery(hdb, query);

            MemFree(rq);

            count++;
            if (!success || (count > maxRecords))
               break;

            rq = writer->queue->getOrBlock(500);
            if ((rq == nullptr) || (rq == INVALID_POINTER_VALUE))
               break;
         }
         DBCommit(hdb);
      }
      else
      {
         MemFree(rq);
      }
      DBConnectionPoolReleaseConnection(hdb);

      if (idataLock)
         s_idataWriteLock.unlock();

      if (rq == INVALID_POINTER_VALUE)   // End-of-job indicator
         break;
   }
}

/**
 * Prepared PostgreSQL INSERT statement
 */
struct PreparedStatement_PostgreSQL
{
   TCHAR *statement;
   int32_t numRecords;
};

/**
 * Worker thread that prepares INSERT statements for PostgreSQL database
 */
static void QueryPrepareThread_PostgreSQL(IDataWriter *writer, ObjectQueue<PreparedStatement_PostgreSQL> *statementQueue,
      SynchronizedObjectMemoryPool<PreparedStatement_PostgreSQL> *memoryPool)
{
   ThreadSetName("DBWriter/QPrep");

   bool convertTimestamps;
   TCHAR queryBase[256];
   if (writer->storageClass != nullptr)   // TimescaleDB
   {
      _sntprintf(queryBase, 256, _T("INSERT INTO idata_sc_%s (item_id,idata_timestamp,idata_value,raw_value) VALUES"), writer->storageClass);
      convertTimestamps = true;
   }
   else
   {
      _tcscpy(queryBase, _T("INSERT INTO idata (item_id,idata_timestamp,idata_value,raw_value) VALUES"));
      convertTimestamps = false;
   }

   int maxRecordsPerStmt = ConfigReadInt(_T("DBWriter.MaxRecordsPerStatement"), 100);

   StringBuffer query;
   query.setAllocationStep(65536);

   while(true)
   {
      DELAYED_IDATA_INSERT *rq = writer->queue->getOrBlock();
      if (rq == INVALID_POINTER_VALUE)   // End-of-job indicator
         break;

      query.append(queryBase);
      int count = 0;
      while(true)
      {
         query.append((count > 0) ? _T(",(") : _T(" ("), 2);
         query.append(rq->dciId);
         if (convertTimestamps)
         {
            query.append(L",ms_to_timestamptz(", 19);
            query.append(rq->timestamp);
            query.append(L"),", 2);
         }
         else
         {
            query.append(L',');
            query.append(rq->timestamp);
            query.append(L',');
         }
         query.append(DBPrepareString(g_dbDriver, rq->transformedValue));
         query.append(L',');
         query.append(DBPrepareString(g_dbDriver, rq->rawValue));
         query.append(L')');
         MemFree(rq);

         count++;
         if (count >= maxRecordsPerStmt)
            break;

         rq = writer->queue->getOrBlock(500);
         if ((rq == nullptr) || (rq == INVALID_POINTER_VALUE))
            break;
      }

      query.append(L" ON CONFLICT DO NOTHING");
      InterlockedAdd(&writer->pendingRequests, count);
      PreparedStatement_PostgreSQL *s = memoryPool->allocate();
      s->statement = query.takeBuffer();
      s->numRecords = count;
      statementQueue->put(s);

      if (rq == INVALID_POINTER_VALUE)   // End-of-job indicator
         break;
   }

   statementQueue->put(INVALID_POINTER_VALUE);
}

/**
 * Database "lazy" write thread for idata INSERTs - PostgreSQL version
 */
static void IDataWriteThreadSingleTable_PostgreSQL(IDataWriter *writer)
{
   ThreadSetName("DBWriter/IData");

   bool idataLock;
   if (writer->storageClass == nullptr)   // Lock is not needed for TimescaleDB
      idataLock = ((g_flags & AF_DBWRITER_HK_INTERLOCK) != 0);
   else
      idataLock = false;

   int maxRecordsPerTxn = ConfigReadInt(_T("DBWriter.MaxRecordsPerTransaction"), 1000);
   int maxRecordsPerStmt = ConfigReadInt(_T("DBWriter.MaxRecordsPerStatement"), 100);
   if (maxRecordsPerTxn < maxRecordsPerStmt)
      maxRecordsPerTxn = maxRecordsPerStmt;
   else if (maxRecordsPerTxn % maxRecordsPerStmt != 0)
      maxRecordsPerTxn = (maxRecordsPerTxn / maxRecordsPerStmt + 1) * maxRecordsPerStmt;

   ObjectQueue<PreparedStatement_PostgreSQL> statementQueue(1024, Ownership::False);
   SynchronizedObjectMemoryPool<PreparedStatement_PostgreSQL> memoryPool;

   int activeWorkers = writer->workerCount;
   THREAD *workerThreads = static_cast<THREAD*>(MemAllocLocal(activeWorkers * sizeof(THREAD)));
   for(int i = 0; i < activeWorkers; i++)
      workerThreads[i] = ThreadCreateEx(QueryPrepareThread_PostgreSQL, writer, &statementQueue, &memoryPool);

   ThreadPool *writerPool = nullptr;
   int numWriters = ConfigReadInt(L"DBWriter.InsertParallelismDegree", 1);
   if (numWriters > 1)
   {
      if (!idataLock)
      {
         TCHAR poolName[64] = _T("DBWRITE");
         if (writer->storageClass != nullptr)
         {
            _tcscat(poolName, _T("/"));
            _tcscat(poolName, writer->storageClass);
            _tcsupr(poolName);
         }
         writerPool = ThreadPoolCreate(poolName, numWriters, numWriters);
         nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Using parallel write mode for idata (%d writers)"), numWriters);
      }
      else
      {
         nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Parallel write disabled because DBWriter/Housekeeper interlock is ON"));
      }
   }

   while(true)
   {
      PreparedStatement_PostgreSQL *statement = statementQueue.getOrBlock();
      if (statement == INVALID_POINTER_VALUE)   // End-of-job indicator
      {
         activeWorkers--;
         if (activeWorkers == 0) // We should get end-of-job indicator from each worker
            break;
         continue;
      }

      if (writerPool != nullptr)
      {
         ThreadPoolExecute(writerPool,
            [writer, statement, &memoryPool] ()
            {
               DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
               DBQuery(hdb, statement->statement);
               InterlockedAdd(&writer->pendingRequests, -statement->numRecords);
               MemFree(statement->statement);
               memoryPool.free(statement);
               DBConnectionPoolReleaseConnection(hdb);
            });
      }
      else
      {
         if (idataLock)
            s_idataWriteLock.readLock();

         DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
         if (DBBegin(hdb))
         {
            int count = 0;
            while(true)
            {
               bool success = DBQuery(hdb, statement->statement);
               count += statement->numRecords;
               InterlockedAdd(&writer->pendingRequests, -statement->numRecords);
               MemFree(statement->statement);
               memoryPool.free(statement);

               if (!success || (count >= maxRecordsPerTxn))
                  break;

               statement = statementQueue.getOrBlock(500);
               if ((statement == nullptr) || (statement == INVALID_POINTER_VALUE))
                  break;
            }
            DBCommit(hdb);
         }
         else
         {
            MemFree(statement->statement);
            memoryPool.free(statement);
         }
         DBConnectionPoolReleaseConnection(hdb);

         if (idataLock)
            s_idataWriteLock.unlock();

         if (statement == INVALID_POINTER_VALUE)   // End-of-job indicator
         {
            activeWorkers--;
            if (activeWorkers == 0) // We should get end-of-job indicator from each worker
               break;
         }
      }
   }

   for(int i = 0; i < writer->workerCount; i++)
      ThreadJoin(workerThreads[i]);

   if (writerPool != nullptr)
      ThreadPoolDestroy(writerPool);
}

/**
 * Database "lazy" write thread for idata INSERTs - Oracle version
 */
static void IDataWriteThreadSingleTable_Oracle(IDataWriter *writer)
{
   ThreadSetName("DBWriter/IData");
   int maxRecords = ConfigReadInt(L"DBWriter.MaxRecordsPerTransaction", 1000);
   while(true)
   {
      DELAYED_IDATA_INSERT *rq = writer->queue->getOrBlock();
      if (rq == INVALID_POINTER_VALUE)   // End-of-job indicator
         break;

      bool idataLock;
      if (g_flags & AF_DBWRITER_HK_INTERLOCK)
      {
         s_idataWriteLock.readLock();
         idataLock = true;
      }
      else
      {
         idataLock = false;
      }

      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
      if (DBBegin(hdb))
      {
         int count = 0;
         DB_STATEMENT hStmt = DBPrepare(hdb, L"INSERT INTO idata (item_id,idata_timestamp,idata_value,raw_value) VALUES (?,?,?,?)");
         if (hStmt != nullptr)
         {
            while(true)
            {
               DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, rq->dciId);
               DBBind(hStmt, 2, DB_SQLTYPE_BIGINT, rq->timestamp);
               DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, rq->transformedValue, DB_BIND_STATIC);
               DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, rq->rawValue, DB_BIND_STATIC);
               bool success = DBExecute(hStmt);

               MemFree(rq);

               count++;
               if (!success || (count > maxRecords))
                  break;

               rq = writer->queue->getOrBlock(500);
               if ((rq == nullptr) || (rq == INVALID_POINTER_VALUE))
                  break;
            }
            DBFreeStatement(hStmt);
         }
         else
         {
            MemFree(rq);
         }
         DBCommit(hdb);
      }
      else
      {
         MemFree(rq);
      }
      DBConnectionPoolReleaseConnection(hdb);

      if (idataLock)
         s_idataWriteLock.unlock();

      if (rq == INVALID_POINTER_VALUE)   // End-of-job indicator
         break;
   }
}

/**
 * Save raw data in a batch
 */
static void SaveRawDataBatch(DELAYED_RAW_DATA_UPDATE *batch, int maxRecords)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   if (DBBegin(hdb))
   {
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("UPDATE raw_dci_values SET raw_value=?,transformed_value=?,last_poll_time=?,cache_timestamp=?,anomaly_detected=? WHERE item_id=?"), true);
      if (hStmt != nullptr)
      {
         DB_STATEMENT hDeleteStmt = nullptr;
         int count = 0;

         DELAYED_RAW_DATA_UPDATE *rq, *tmp;
         HASH_ITER(hh, batch, rq, tmp)
         {
            bool success = false;
            if (rq->deleteFlag)
            {
               if (hDeleteStmt == nullptr)
                  hDeleteStmt = DBPrepare(hdb, _T("DELETE FROM raw_dci_values WHERE item_id=?"), true);
               if (hDeleteStmt != nullptr)
               {
                  DBBind(hDeleteStmt, 1, DB_SQLTYPE_INTEGER, rq->dciId);
                  success = DBExecute(hDeleteStmt);
               }
            }
            else
            {
               DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, rq->rawValue, DB_BIND_STATIC);
               DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, rq->transformedValue, DB_BIND_STATIC);
               DBBind(hStmt, 3, DB_SQLTYPE_BIGINT, rq->timestamp);
               DBBind(hStmt, 4, DB_SQLTYPE_BIGINT, rq->cacheTimestamp);
               DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, rq->anomalyDetected ? _T("1") : _T("0"), DB_BIND_STATIC);
               DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, rq->dciId);
               success = DBExecute(hStmt);
            }

            if (!success)
               break;

            HASH_DEL(batch, rq);
            MemFree(rq);
            InterlockedDecrement(&s_batchSize);

            count++;
            if (count >= maxRecords)
            {
               DBCommit(hdb);
               if (!DBBegin(hdb))
                  break;
               count = 0;
            }
         }
         DBFreeStatement(hStmt);
         if (hDeleteStmt != nullptr)
            DBFreeStatement(hDeleteStmt);
      }
      DBCommit(hdb);
   }
   DBConnectionPoolReleaseConnection(hdb);

   // Clean remaining items if any
   DELAYED_RAW_DATA_UPDATE *rq, *tmp;
   HASH_ITER(hh, batch, rq, tmp)
   {
      HASH_DEL(batch, rq);
      MemFree(rq);
      InterlockedDecrement(&s_batchSize);
   }
}

/**
 * Save raw DCI data
 */
static void SaveRawData(int maxRecords, ThreadPool *writerPool)
{
   s_rawDataWriterLock.lock();
   DELAYED_RAW_DATA_UPDATE *batch = s_rawDataWriterQueue;
   s_rawDataWriterQueue = nullptr;
   s_rawDataWriterLock.unlock();

   int32_t batchSize = HASH_COUNT(batch);
   if (batchSize == 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 7, L"Empty raw data batch, skipping write cycle");
      return;
   }

   InterlockedAdd(&s_batchSize, batchSize);
   nxlog_debug_tag(DEBUG_TAG, 7, L"%d records in raw data batch", batchSize);

   if (writerPool != nullptr)
      ThreadPoolExecute(writerPool, SaveRawDataBatch, batch, maxRecords);
   else
      SaveRawDataBatch(batch, maxRecords);
}

/**
 * Database "lazy" write thread for raw_dci_values UPDATEs
 */
static void RawDataWriteThread()
{
   ThreadSetName("DBWriter/RData");
   int maxRecords = ConfigReadInt(L"DBWriter.MaxRecordsPerTransaction", 1000);
   int flushInterval = ConfigReadInt(L"DBWriter.RawDataFlushInterval", 30);
   if (flushInterval < 1)
      flushInterval = 1;

   ThreadPool *writerPool = nullptr;
   int numWriters = ConfigReadInt(L"DBWriter.UpdateParallelismDegree", 1);
   if (numWriters > 1)
   {
      writerPool = ThreadPoolCreate(L"DBWRITE/RAW", numWriters, numWriters);
   }

   nxlog_debug_tag(DEBUG_TAG, 1, L"Raw DCI data flush interval is %d seconds (%d writer%s)", flushInterval, (writerPool != nullptr) ? numWriters : 1, (numWriters > 1) ? _T("s") : _T(""));
   while(!SleepAndCheckForShutdown(flushInterval))
   {
      SaveRawData(maxRecords, writerPool);
	}
   SaveRawData(maxRecords, writerPool);

   if (writerPool != nullptr)
      ThreadPoolDestroy(writerPool);
   nxlog_debug_tag(DEBUG_TAG, 1, L"Raw DCI data writer stopped");
}

/**
 * Queue monitor thread
 */
static void QueueMonitorThread()
{
   ThreadSetName("DBQueueMonitor");
   nxlog_debug_tag(DEBUG_TAG, 1, _T("Queue monitor started"));
   while(!s_queueMonitorStopCondition.wait(5000))
   {
      int64_t maxQueueSize = ConfigReadULong(_T("DBWriter.MaxQueueSize"), 0);
      if (maxQueueSize == 0)
      {
         s_queueMonitorDiscardFlag = false;
         break;
      }

      int64_t currentQueueSize = GetIDataWriterQueueSize();
      if (currentQueueSize > maxQueueSize)
      {
         if (!s_queueMonitorDiscardFlag)
         {
            s_queueMonitorDiscardFlag = true;
            nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Background database writer queue size for DCI data exceeds threshold (current=") INT64_FMT _T(", threshold=") INT64_FMT _T(")"),
                     currentQueueSize, maxQueueSize);
            PostSystemEvent(EVENT_DBWRITER_QUEUE_OVERFLOW, g_dwMgmtNode);
         }
      }
      else if (s_queueMonitorDiscardFlag)
      {
         s_queueMonitorDiscardFlag = false;
         nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Background database writer queue size for DCI data is below threshold (current=") INT64_FMT _T(", threshold=") INT64_FMT _T(")"),
                  currentQueueSize, maxQueueSize);
         PostSystemEvent(EVENT_DBWRITER_QUEUE_NORMAL, g_dwMgmtNode);
      }
   }
   nxlog_debug_tag(DEBUG_TAG, 1, _T("Queue monitor stopped"));
}

/**
 * Custom destructor for queued requests
 */
static void QueuedRequestDestructor(void *object, Queue *queue)
{
   MemFree(object);
}

/**
 * Start writer thread
 */
void StartDBWriter()
{
   s_writerThread = ThreadCreateEx(DBWriteThread);
	s_rawDataWriterThread = ThreadCreateEx(RawDataWriteThread);

	if (g_flags & AF_SINGLE_TABLE_PERF_DATA)
	{
	   // Always use single writer if performance data stored in single table
      switch(g_dbSyntax)
      {
         case DB_SYNTAX_ORACLE:
            s_idataWriters[0].storageClass = nullptr;
            s_idataWriters[0].queue = new ObjectQueue<DELAYED_IDATA_INSERT>(4096, Ownership::True, QueuedRequestDestructor);
            s_idataWriters[0].thread = ThreadCreateEx(IDataWriteThreadSingleTable_Oracle, &s_idataWriters[0]);
            s_idataWriters[0].workerCount = 0;
            s_idataWriters[0].pendingRequests = 0;
            break;
         case DB_SYNTAX_PGSQL:
            s_idataWriters[0].storageClass = nullptr;
            s_idataWriters[0].queue = new ObjectQueue<DELAYED_IDATA_INSERT>(4096, Ownership::True, QueuedRequestDestructor);
            s_idataWriters[0].thread = ThreadCreateEx(IDataWriteThreadSingleTable_PostgreSQL, &s_idataWriters[0]);
            s_idataWriters[0].workerCount = ConfigReadInt(_T("DBWriter.BackgroundWorkers"), 1);
            s_idataWriters[0].pendingRequests = 0;
            break;
         case DB_SYNTAX_TSDB:
            s_idataWriterCount = static_cast<int>(DCObjectStorageClass::OTHER) + 1;
            for(int i = 0; i < s_idataWriterCount; i++)
            {
               s_idataWriters[i].storageClass = DCObject::getStorageClassName(static_cast<DCObjectStorageClass>(i));
               s_idataWriters[i].queue = new ObjectQueue<DELAYED_IDATA_INSERT>(4096, Ownership::True, QueuedRequestDestructor);
               s_idataWriters[i].thread = ThreadCreateEx(IDataWriteThreadSingleTable_PostgreSQL, &s_idataWriters[i]);
               s_idataWriters[i].workerCount = ConfigReadInt(_T("DBWriter.BackgroundWorkers"), 1);
               s_idataWriters[i].pendingRequests = 0;
            }
            break;
         default:
            s_idataWriters[0].storageClass = nullptr;
            s_idataWriters[0].queue = new ObjectQueue<DELAYED_IDATA_INSERT>(4096, Ownership::True, QueuedRequestDestructor);
            s_idataWriters[0].thread = ThreadCreateEx(IDataWriteThreadSingleTable_Generic, &s_idataWriters[0]);
            s_idataWriters[0].workerCount = 0;
            s_idataWriters[0].pendingRequests = 0;
            break;
      }
	}
	else
	{
      s_idataWriterCount = ConfigReadInt(_T("DBWriter.DataQueues"), 1);
      if (s_idataWriterCount < 1)
         s_idataWriterCount = 1;
      else if (s_idataWriterCount > MAX_IDATA_WRITERS)
         s_idataWriterCount = MAX_IDATA_WRITERS;
      nxlog_debug_tag(DEBUG_TAG, 1, _T("Using %d DCI data write queues"), s_idataWriterCount);
      for(int i = 0; i < s_idataWriterCount; i++)
      {
         s_idataWriters[i].storageClass = nullptr;
         s_idataWriters[i].queue = new ObjectQueue<DELAYED_IDATA_INSERT>(4096, Ownership::True, QueuedRequestDestructor);
         s_idataWriters[i].thread = ThreadCreateEx(IDataWriteThread, &s_idataWriters[i]);
         s_idataWriters[i].workerCount = 0;
         s_idataWriters[i].pendingRequests = 0;
      }
	}

	if (ConfigReadULong(_T("DBWriter.MaxQueueSize"), 0) > 0)
	   s_queueMonitorThread = ThreadCreateEx(QueueMonitorThread);
}

/**
 * Stop writer thread and wait while all queries will be executed
 */
void StopDBWriter()
{
   if (s_queueMonitorThread != INVALID_THREAD_HANDLE)
   {
      s_queueMonitorStopCondition.set();
      ThreadJoin(s_queueMonitorThread);
   }

   g_dbWriterQueue.put(INVALID_POINTER_VALUE);
   ThreadJoin(s_writerThread);

   for(int i = 0; i < s_idataWriterCount; i++)
   {
      s_idataWriters[i].queue->put(INVALID_POINTER_VALUE);
      for(int j = 0; j < s_idataWriters[i].workerCount; j++)
         s_idataWriters[i].queue->put(INVALID_POINTER_VALUE);  // Additional stop indicator for each worker
      ThreadJoin(s_idataWriters[i].thread);
      delete s_idataWriters[i].queue;
   }
   ThreadJoin(s_rawDataWriterThread);

   nxlog_debug_tag(DEBUG_TAG, 1, _T("All background database writers stopped"));
}

/**
 * Handle changes in writer queue size threshold
 */
void OnDBWriterMaxQueueSizeChange()
{
   nxlog_debug_tag(DEBUG_TAG, 3, _T("Threshold for background database writer queue size changed"));
   s_queueMonitorStateLock.lock();
   if (ConfigReadULong(_T("DBWriter.MaxQueueSize"), 0) > 0)
   {
      if (s_queueMonitorThread == INVALID_THREAD_HANDLE)
         s_queueMonitorThread = ThreadCreateEx(QueueMonitorThread);
   }
   else if (s_queueMonitorThread != INVALID_THREAD_HANDLE)
   {
      s_queueMonitorStopCondition.set();
      ThreadJoin(s_queueMonitorThread);
      s_queueMonitorThread = INVALID_THREAD_HANDLE;
      s_queueMonitorStopCondition.reset();
      s_queueMonitorDiscardFlag = false;
   }
   s_queueMonitorStateLock.unlock();
}

/**
 * Get size of IData writer queue
 */
int64_t GetIDataWriterQueueSize()
{
   int64_t size = 0;
   for(int i = 0; i < s_idataWriterCount; i++)
      size += s_idataWriters[i].queue->size() + s_idataWriters[i].pendingRequests;
   return size;
}

/**
 * Get size of raw data writer queue
 */
int64_t GetRawDataWriterQueueSize()
{
   s_rawDataWriterLock.lock();
   int64_t size = HASH_COUNT(s_rawDataWriterQueue);
   s_rawDataWriterLock.unlock();
   return size + s_batchSize;
}

/**
 * Get memory consumption by raw DCI data write cache
 */
uint64_t GetRawDataWriterMemoryUsage()
{
   s_rawDataWriterLock.lock();
   uint64_t size = (HASH_COUNT(s_rawDataWriterQueue) + s_batchSize) * sizeof(DELAYED_RAW_DATA_UPDATE);
   s_rawDataWriterLock.unlock();
   return size;
}

/**
 * Clear DB writer data from debug console
 */
void ClearDBWriterData(ServerConsole *console, const TCHAR *component)
{
   if (!_tcsicmp(component, _T("Counters")))
   {
      g_idataWriteRequests = 0;
      g_rawDataWriteRequests = 0;
      g_otherWriteRequests = 0;
      console->print(_T("Database writer counters cleared\n"));
   }
   else if (!_tcsicmp(component, _T("DataQueue")))
   {
      for(int i = 0; i < s_idataWriterCount; i++)
      {
         s_idataWriters[i].queue->clear();
      }
      console->print(_T("Database writer data queue cleared\n"));
   }
   else
   {
      console->print(_T("Invalid DB writer component"));
   }
}
