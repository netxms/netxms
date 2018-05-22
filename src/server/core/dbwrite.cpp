/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2018 Victor Kirhenshtein
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
   UINT32 nodeId;
   UINT32 dciId;
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
   UINT32 dciId;
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
   Queue *queue;
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
 * Generic DB writer queue
 */
Queue *g_dbWriterQueue = NULL;

/**
 * Raw DCI data writer queue
 */
static DELAYED_RAW_DATA_UPDATE *s_rawDataWriterQueue = NULL;
static Mutex s_rawDataWriterLock;
static int s_batchSize = 0;

/**
 * Performance counters
 */
UINT64 g_idataWriteRequests = 0;
UINT64 g_rawDataWriteRequests = 0;
UINT64 g_otherWriteRequests = 0;

/**
 * Static data
 */
static THREAD s_writerThread = INVALID_THREAD_HANDLE;
static THREAD s_rawDataWriterThread = INVALID_THREAD_HANDLE;

/**
 * Put SQL request into queue for later execution
 */
void NXCORE_EXPORTABLE QueueSQLRequest(const TCHAR *query)
{
	DELAYED_SQL_REQUEST *rq = (DELAYED_SQL_REQUEST *)malloc(sizeof(DELAYED_SQL_REQUEST) + (_tcslen(query) + 1) * sizeof(TCHAR));
	rq->query = (TCHAR *)&rq->bindings[0];
	_tcscpy(rq->query, query);
	rq->bindCount = 0;
   g_dbWriterQueue->put(rq);
	DbgPrintf(8, _T("SQL request queued: %s"), query);
	g_otherWriteRequests++;
}

/**
 * Put parameterized SQL request into queue for later execution
 */
void NXCORE_EXPORTABLE QueueSQLRequest(const TCHAR *query, int bindCount, int *sqlTypes, const TCHAR **values)
{
	int size = sizeof(DELAYED_SQL_REQUEST) + ((int)_tcslen(query) + 1) * sizeof(TCHAR) + bindCount * sizeof(TCHAR *) + bindCount;
	for(int i = 0; i < bindCount; i++)
	{
	   if (values[i] != NULL)
	      size += ((int)_tcslen(values[i]) + 1) * sizeof(TCHAR) + sizeof(TCHAR *);
	}
	DELAYED_SQL_REQUEST *rq = (DELAYED_SQL_REQUEST *)malloc(size);

	BYTE *base = (BYTE *)&rq->bindings[bindCount];
	int pos = 0;
	int align = sizeof(TCHAR *);

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
		if (values[i] != NULL)
		{
         rq->bindings[i] = (TCHAR *)&base[pos];
         _tcscpy(rq->bindings[i], values[i]);
         pos += ((int)_tcslen(values[i]) + 1) * sizeof(TCHAR);
         if (pos % align != 0)
            pos += align - pos % align;
		}
		else
		{
		   rq->bindings[i] = NULL;
		}
	}

   g_dbWriterQueue->put(rq);
	DbgPrintf(8, _T("SQL request queued: %s"), query);
   g_otherWriteRequests++;
}

/**
 * Queue INSERT request for idata_xxx table
 */
void QueueIDataInsert(time_t timestamp, UINT32 nodeId, UINT32 dciId, const TCHAR *rawValue, const TCHAR *transformedValue)
{
	DELAYED_IDATA_INSERT *rq = (DELAYED_IDATA_INSERT *)malloc(sizeof(DELAYED_IDATA_INSERT));
	rq->timestamp = timestamp;
	rq->nodeId = nodeId;
	rq->dciId = dciId;
   _tcslcpy(rq->rawValue, rawValue, MAX_RESULT_LENGTH);
   _tcslcpy(rq->transformedValue, transformedValue, MAX_RESULT_LENGTH);
   if (s_idataWriterCount > 1)
   {
      int hash = nodeId % s_idataWriterCount;
      s_idataWriters[hash].queue->put(rq);
   }
   else
   {
      s_idataWriters[0].queue->put(rq);
   }
	g_idataWriteRequests++;
}

/**
 * Queue UPDATE request for raw_dci_values table
 */
void QueueRawDciDataUpdate(time_t timestamp, UINT32 dciId, const TCHAR *rawValue, const TCHAR *transformedValue)
{
   s_rawDataWriterLock.lock();
   DELAYED_RAW_DATA_UPDATE *rq;
   HASH_FIND_INT(s_rawDataWriterQueue, &dciId, rq);
   if (rq == NULL)
   {
      rq = (DELAYED_RAW_DATA_UPDATE *)malloc(sizeof(DELAYED_RAW_DATA_UPDATE));
      rq->dciId = dciId;
      rq->deleteFlag = false;
      HASH_ADD_INT(s_rawDataWriterQueue, dciId, rq);
   }
	rq->timestamp = timestamp;
	_tcslcpy(rq->rawValue, rawValue, MAX_RESULT_LENGTH);
	_tcslcpy(rq->transformedValue, transformedValue, MAX_RESULT_LENGTH);
   s_rawDataWriterLock.unlock();
	g_rawDataWriteRequests++;
}

/**
 * Queue DELETE request for raw_dci_values table
 */
void QueueRawDciDataDelete(UINT32 dciId)
{
   s_rawDataWriterLock.lock();
   DELAYED_RAW_DATA_UPDATE *rq;
   HASH_FIND_INT(s_rawDataWriterQueue, &dciId, rq);
   if (rq == NULL)
   {
      rq = (DELAYED_RAW_DATA_UPDATE *)malloc(sizeof(DELAYED_RAW_DATA_UPDATE));
      rq->dciId = dciId;
      HASH_ADD_INT(s_rawDataWriterQueue, dciId, rq);
   }
   rq->deleteFlag = true;
   s_rawDataWriterLock.unlock();
   g_rawDataWriteRequests++;
}

/**
 * Database "lazy" write thread
 */
static THREAD_RESULT THREAD_CALL DBWriteThread(void *arg)
{
   ThreadSetName("DBWriter");
   while(true)
   {
      DELAYED_SQL_REQUEST *rq = (DELAYED_SQL_REQUEST *)g_dbWriterQueue->getOrBlock();
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
			if (hStmt != NULL)
			{
				for(int i = 0; i < rq->bindCount; i++)
				{
					DBBind(hStmt, i + 1, (int)rq->sqlTypes[i], rq->bindings[i], DB_BIND_STATIC);
				}
				DBExecute(hStmt);
				DBFreeStatement(hStmt);
			}
		}
      free(rq);

      DBConnectionPoolReleaseConnection(hdb);
   }

   return THREAD_OK;
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
		DELAYED_IDATA_INSERT *rq = static_cast<DELAYED_IDATA_INSERT*>(writer->queue->getOrBlock());
      if (rq == INVALID_POINTER_VALUE)   // End-of-job indicator
         break;

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

				free(rq);

				count++;
				if (!success || (count > maxRecords))
					break;

				rq = static_cast<DELAYED_IDATA_INSERT*>(writer->queue->getOrBlock(500));
				if ((rq == NULL) || (rq == INVALID_POINTER_VALUE))
					break;
			}
			DBCommit(hdb);
		}
		else
		{
			free(rq);
		}
		DBConnectionPoolReleaseConnection(hdb);
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
      free(rq);
   }
   s_batchSize = 0;
}

/**
 * Database "lazy" write thread for raw_dci_values UPDATEs
 */
static THREAD_RESULT THREAD_CALL RawDataWriteThread(void *arg)
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
   return THREAD_OK;
}

/**
 * Start writer thread
 */
void StartDBWriter()
{
   s_writerThread = ThreadCreateEx(DBWriteThread, 0, NULL);
	s_rawDataWriterThread = ThreadCreateEx(RawDataWriteThread, 0, NULL);

	s_idataWriterCount = ConfigReadInt(_T("DBWriter.DataQueues"), 1);
	if (s_idataWriterCount < 1)
	   s_idataWriterCount = 1;
	else if (s_idataWriterCount > MAX_IDATA_WRITERS)
	   s_idataWriterCount = MAX_IDATA_WRITERS;
	nxlog_debug(1, _T("Using %d DCI data write queues"), s_idataWriterCount);
	for(int i = 0; i < s_idataWriterCount; i++)
	{
	   s_idataWriters[i].queue = new Queue();
	   s_idataWriters[i].thread = ThreadCreateEx(IDataWriteThread, 0, &s_idataWriters[i]);
	}
}

/**
 * Stop writer thread and wait while all queries will be executed
 */
void StopDBWriter()
{
   g_dbWriterQueue->put(INVALID_POINTER_VALUE);
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
 * Get size of IData writer queue
 */
int GetIDataWriterQueueSize()
{
   int size = 0;
   for(int i = 0; i < s_idataWriterCount; i++)
      size += s_idataWriters[i].queue->size();
   return size;
}

/**
 * Get size of raw data writer queue
 */
int GetRawDataWriterQueueSize()
{
   s_rawDataWriterLock.lock();
   int size = HASH_COUNT(s_rawDataWriterQueue);
   s_rawDataWriterLock.unlock();
   return size + s_batchSize;
}
