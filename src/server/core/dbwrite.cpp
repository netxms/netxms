/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2020 Victor Kirhenshtein
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
   time_t timestamp;
   uint32_t nodeId;
   uint32_t dciId;
   TCHAR rawValue[MAX_RESULT_LENGTH];
   TCHAR transformedValue[MAX_RESULT_LENGTH];
};

/**
 * Delayed request for raw_dci_values UPDATE or DELETE
 */
struct DELAYED_RAW_DATA_UPDATE
{
   UT_hash_handle hh;
   time_t timestamp;
   uint32_t dciId;
   bool deleteFlag;
   TCHAR rawValue[MAX_RESULT_LENGTH];
   TCHAR transformedValue[MAX_RESULT_LENGTH];
};

/**
 * IData writer
 */
struct IDataWriter
{
   THREAD thread;
   ObjectQueue<DELAYED_IDATA_INSERT> *queue;
   const TCHAR *storageClass;
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
static int s_batchSize = 0;

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
static Mutex s_queueMonitorStateLock(true);
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
static RWLOCK s_idataWriteLock = RWLockCreate();
static bool s_idataWriteLockActive = false;

/**
 * Lock IDATA writes (used by housekeeper)
 */
void LockIDataWrites()
{
   if (g_flags & AF_DBWRITER_HK_INTERLOCK)
   {
      RWLockWriteLock(s_idataWriteLock);
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
      RWLockUnlock(s_idataWriteLock);
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
void QueueIDataInsert(time_t timestamp, uint32_t nodeId, uint32_t dciId, const TCHAR *rawValue, const TCHAR *transformedValue, DCObjectStorageClass storageClass)
{
   if (s_queueMonitorDiscardFlag)
      return;

	DELAYED_IDATA_INSERT *rq = MemAllocStruct<DELAYED_IDATA_INSERT>();
	rq->timestamp = timestamp;
	rq->nodeId = nodeId;
	rq->dciId = dciId;
   _tcslcpy(rq->rawValue, rawValue, MAX_RESULT_LENGTH);
   _tcslcpy(rq->transformedValue, transformedValue, MAX_RESULT_LENGTH);
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
void QueueRawDciDataUpdate(time_t timestamp, uint32_t dciId, const TCHAR *rawValue, const TCHAR *transformedValue)
{
   s_rawDataWriterLock.lock();
   DELAYED_RAW_DATA_UPDATE *rq;
   HASH_FIND_INT(s_rawDataWriterQueue, &dciId, rq);
   if (rq == nullptr)
   {
      rq = MemAllocStruct<DELAYED_RAW_DATA_UPDATE>();
      rq->dciId = dciId;
      rq->deleteFlag = false;
      HASH_ADD_INT(s_rawDataWriterQueue, dciId, rq);
   }
	rq->timestamp = timestamp;
	_tcslcpy(rq->rawValue, rawValue, MAX_RESULT_LENGTH);
	_tcslcpy(rq->transformedValue, transformedValue, MAX_RESULT_LENGTH);
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
static THREAD_RESULT THREAD_CALL IDataWriteThread(void *arg)
{
   ThreadSetName("DBWriter/IData");
   IDataWriter *writer = static_cast<IDataWriter*>(arg);
   int maxRecords = ConfigReadInt(_T("DBWriter.MaxRecordsPerTransaction"), 1000);
   while(true)
   {
		DELAYED_IDATA_INSERT *rq = writer->queue->getOrBlock();
      if (rq == INVALID_POINTER_VALUE)   // End-of-job indicator
         break;

      bool idataLock;
      if (g_flags & AF_DBWRITER_HK_INTERLOCK)
      {
         RWLockReadLock(s_idataWriteLock);
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

				// For Oracle preparing statement even for one time execution is preferred
				// For other databases it will actually slow down inserts
				if (g_dbSyntax == DB_SYNTAX_ORACLE)
				{
	            TCHAR query[256];
               _sntprintf(query, 256, _T("INSERT INTO idata_%d (item_id,idata_timestamp,idata_value,raw_value) VALUES (?,?,?,?)"), (int)rq->nodeId);
               DB_STATEMENT hStmt = DBPrepare(hdb, query);
               if (hStmt != NULL)
               {
                  DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, rq->dciId);
                  DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, (INT64)rq->timestamp);
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
               TCHAR query[1024];
               _sntprintf(query, 1024, _T("INSERT INTO idata_%d (item_id,idata_timestamp,idata_value,raw_value) VALUES (%d,%d,%s,%s)"),
                          (int)rq->nodeId, (int)rq->dciId, (int)rq->timestamp,
                          (const TCHAR *)DBPrepareString(hdb, rq->transformedValue),
                          (const TCHAR *)DBPrepareString(hdb, rq->rawValue));
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
		   RWLockUnlock(s_idataWriteLock);

      if (rq == INVALID_POINTER_VALUE)   // End-of-job indicator
         break;
	}

   return THREAD_OK;
}

/**
 * Database "lazy" write thread for idata INSERTs - generic version
 */
static THREAD_RESULT THREAD_CALL IDataWriteThreadSingleTable_Generic(void *arg)
{
   ThreadSetName("DBWriter/IData");
   IDataWriter *writer = static_cast<IDataWriter*>(arg);
   int maxRecords = ConfigReadInt(_T("DBWriter.MaxRecordsPerTransaction"), 1000);

   TCHAR query[1024];
   while(true)
   {
      DELAYED_IDATA_INSERT *rq = writer->queue->getOrBlock();
      if (rq == INVALID_POINTER_VALUE)   // End-of-job indicator
         break;

      bool idataLock;
      if (g_flags & AF_DBWRITER_HK_INTERLOCK)
      {
         RWLockReadLock(s_idataWriteLock);
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
            _sntprintf(query, 1024, _T("INSERT INTO idata (item_id,idata_timestamp,idata_value,raw_value) VALUES (%d,%d,%s,%s)"),
                       (int)rq->dciId, (int)rq->timestamp,
                       (const TCHAR *)DBPrepareString(hdb, rq->transformedValue),
                       (const TCHAR *)DBPrepareString(hdb, rq->rawValue));
            bool success = DBQuery(hdb, query);

            MemFree(rq);

            count++;
            if (!success || (count > maxRecords))
               break;

            rq = writer->queue->getOrBlock(500);
            if ((rq == NULL) || (rq == INVALID_POINTER_VALUE))
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
         RWLockUnlock(s_idataWriteLock);

      if (rq == INVALID_POINTER_VALUE)   // End-of-job indicator
         break;
   }

   return THREAD_OK;
}

/**
 * Database "lazy" write thread for idata INSERTs - PostgreSQL version
 */
static THREAD_RESULT THREAD_CALL IDataWriteThreadSingleTable_PostgreSQL(void *arg)
{
   ThreadSetName("DBWriter/IData");
   IDataWriter *writer = static_cast<IDataWriter*>(arg);

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

   int maxRecordsPerTxn = ConfigReadInt(_T("DBWriter.MaxRecordsPerTransaction"), 1000);
   int maxRecordsPerStmt = ConfigReadInt(_T("DBWriter.MaxRecordsPerStatement"), 100);

   StringBuffer query;
   query.setAllocationStep(65536);

   TCHAR data[1024];

   while(true)
   {
      DELAYED_IDATA_INSERT *rq = writer->queue->getOrBlock();
      if (rq == INVALID_POINTER_VALUE)   // End-of-job indicator
         break;

      bool idataLock;
      if (writer->storageClass == nullptr)   // Lock is not needed for TimescaleDB
      {
         if (g_flags & AF_DBWRITER_HK_INTERLOCK)
         {
            RWLockReadLock(s_idataWriteLock);
            idataLock = true;
         }
         else
         {
            idataLock = false;
         }
      }
      else
      {
         idataLock = false;
      }

      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
      if (DBBegin(hdb))
      {
         query = queryBase;
         int countTxn = 0, countStmt = 0;
         while(true)
         {
            _sntprintf(data, 1024, convertTimestamps ? _T("%c(%u,to_timestamp(%u),%s,%s)") : _T("%c(%u,%u,%s,%s)"),
                       (countStmt > 0) ? _T(',') : _T(' '),
                       rq->dciId, (unsigned int)rq->timestamp,
                       (const TCHAR *)DBPrepareString(hdb, rq->transformedValue),
                       (const TCHAR *)DBPrepareString(hdb, rq->rawValue));
            query.append(data);
            MemFree(rq);

            countTxn++;
            countStmt++;

            if (countStmt >= maxRecordsPerStmt)
            {
               countStmt = 0;
               query.append(_T(" ON CONFLICT DO NOTHING"));
               if (!DBQuery(hdb, query))
                  break;
               query = queryBase;
            }

            if (countTxn >= maxRecordsPerTxn)
               break;

            rq = writer->queue->getOrBlock(500);
            if ((rq == NULL) || (rq == INVALID_POINTER_VALUE))
               break;
         }
         if (countStmt > 0)
         {
            query.append(_T(" ON CONFLICT DO NOTHING"));
            DBQuery(hdb, query);
         }
         DBCommit(hdb);
      }
      else
      {
         MemFree(rq);
      }
      DBConnectionPoolReleaseConnection(hdb);

      if (idataLock)
         RWLockUnlock(s_idataWriteLock);

      if (rq == INVALID_POINTER_VALUE)   // End-of-job indicator
         break;
   }

   return THREAD_OK;
}

/**
 * Database "lazy" write thread for idata INSERTs - Oracle version
 */
static THREAD_RESULT THREAD_CALL IDataWriteThreadSingleTable_Oracle(void *arg)
{
   ThreadSetName("DBWriter/IData");
   IDataWriter *writer = static_cast<IDataWriter*>(arg);
   int maxRecords = ConfigReadInt(_T("DBWriter.MaxRecordsPerTransaction"), 1000);
   while(true)
   {
      DELAYED_IDATA_INSERT *rq = writer->queue->getOrBlock();
      if (rq == INVALID_POINTER_VALUE)   // End-of-job indicator
         break;

      bool idataLock;
      if (g_flags & AF_DBWRITER_HK_INTERLOCK)
      {
         RWLockReadLock(s_idataWriteLock);
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
         DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO idata (item_id,idata_timestamp,idata_value,raw_value) VALUES (?,?,?,?)"));
         if (hStmt != NULL)
         {
            while(true)
            {
               DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, rq->dciId);
               DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, (INT64)rq->timestamp);
               DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, rq->transformedValue, DB_BIND_STATIC);
               DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, rq->rawValue, DB_BIND_STATIC);
               bool success = DBExecute(hStmt);

               MemFree(rq);

               count++;
               if (!success || (count > maxRecords))
                  break;

               rq = writer->queue->getOrBlock(500);
               if ((rq == NULL) || (rq == INVALID_POINTER_VALUE))
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
         RWLockUnlock(s_idataWriteLock);

      if (rq == INVALID_POINTER_VALUE)   // End-of-job indicator
         break;
   }

   return THREAD_OK;
}

/**
 * Save raw DCI data
 */
static void SaveRawData(int maxRecords)
{
   s_rawDataWriterLock.lock();
   DELAYED_RAW_DATA_UPDATE *batch = s_rawDataWriterQueue;
   s_rawDataWriterQueue = NULL;
   s_rawDataWriterLock.unlock();

   s_batchSize = HASH_COUNT(batch);
   if (s_batchSize == 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 7, _T("Empty raw data batch, skipping write cycle"));
      return;
   }

   nxlog_debug_tag(DEBUG_TAG, 7, _T("%d records in raw data batch"), s_batchSize);
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   if (DBBegin(hdb))
   {
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("UPDATE raw_dci_values SET raw_value=?,transformed_value=?,last_poll_time=? WHERE item_id=?"), true);
      if (hStmt != NULL)
      {
         DB_STATEMENT hDeleteStmt = NULL;
         int count = 0;

         DELAYED_RAW_DATA_UPDATE *rq, *tmp;
         HASH_ITER(hh, batch, rq, tmp)
         {
            bool success = false;
            if (rq->deleteFlag)
            {
               if (hDeleteStmt == NULL)
                  hDeleteStmt = DBPrepare(hdb, _T("DELETE FROM raw_dci_values WHERE item_id=?"), true);
               if (hDeleteStmt != NULL)
               {
                  DBBind(hDeleteStmt, 1, DB_SQLTYPE_INTEGER, rq->dciId);
                  success = DBExecute(hDeleteStmt);
               }
            }
            else
            {
               DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, rq->rawValue, DB_BIND_STATIC);
               DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, rq->transformedValue, DB_BIND_STATIC);
               DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, (INT64)rq->timestamp);
               DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, rq->dciId);
               success = DBExecute(hStmt);
            }

            if (!success)
               break;

            HASH_DEL(batch, rq);
            free(rq);
            s_batchSize--;

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
         if (hDeleteStmt != NULL)
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
   }
   s_batchSize = 0;
}

/**
 * Database "lazy" write thread for raw_dci_values UPDATEs
 */
static void RawDataWriteThread()
{
   ThreadSetName("DBWriter/RData");
   int maxRecords = ConfigReadInt(_T("DBWriter.MaxRecordsPerTransaction"), 1000);
   int flushInterval = ConfigReadInt(_T("DBWriter.RawDataFlushInterval"), 30);
   nxlog_debug_tag(DEBUG_TAG, 1, _T("Raw DCI data flush interval is %d seconds"), flushInterval);
   while(!SleepAndCheckForShutdown(flushInterval))
   {
      SaveRawData(maxRecords);
	}
   SaveRawData(maxRecords);
   nxlog_debug_tag(DEBUG_TAG, 1, _T("Raw DCI data writer stopped"));
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
            PostSystemEvent(EVENT_DBWRITER_QUEUE_OVERFLOW, g_dwMgmtNode, nullptr);
         }
      }
      else if (s_queueMonitorDiscardFlag)
      {
         s_queueMonitorDiscardFlag = false;
         nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Background database writer queue size for DCI data is below threshold (current=") INT64_FMT _T(", threshold=") INT64_FMT _T(")"),
                  currentQueueSize, maxQueueSize);
         PostSystemEvent(EVENT_DBWRITER_QUEUE_NORMAL, g_dwMgmtNode, nullptr);
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
            s_idataWriters[0].thread = ThreadCreateEx(IDataWriteThreadSingleTable_Oracle, 0, &s_idataWriters[0]);
            break;
         case DB_SYNTAX_PGSQL:
            s_idataWriters[0].storageClass = nullptr;
            s_idataWriters[0].queue = new ObjectQueue<DELAYED_IDATA_INSERT>(4096, Ownership::True, QueuedRequestDestructor);
            s_idataWriters[0].thread = ThreadCreateEx(IDataWriteThreadSingleTable_PostgreSQL, 0, &s_idataWriters[0]);
            break;
         case DB_SYNTAX_TSDB:
            s_idataWriterCount = static_cast<int>(DCObjectStorageClass::OTHER) + 1;
            for(int i = 0; i < s_idataWriterCount; i++)
            {
               s_idataWriters[i].storageClass = DCObject::getStorageClassName(static_cast<DCObjectStorageClass>(i));
               s_idataWriters[i].queue = new ObjectQueue<DELAYED_IDATA_INSERT>(4096, Ownership::True, QueuedRequestDestructor);
               s_idataWriters[i].thread = ThreadCreateEx(IDataWriteThreadSingleTable_PostgreSQL, 0, &s_idataWriters[i]);
            }
            break;
         default:
            s_idataWriters[0].storageClass = nullptr;
            s_idataWriters[0].queue = new ObjectQueue<DELAYED_IDATA_INSERT>(4096, Ownership::True, QueuedRequestDestructor);
            s_idataWriters[0].thread = ThreadCreateEx(IDataWriteThreadSingleTable_Generic, 0, &s_idataWriters[0]);
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
         s_idataWriters[i].thread = ThreadCreateEx(IDataWriteThread, 0, &s_idataWriters[i]);
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
      size += s_idataWriters[i].queue->size();
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
