/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2017 Victor Kirhenshtein
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

/**
 * Generic DB writer queue
 */
Queue *g_dbWriterQueue = NULL;

/**
 * DCI data (idata_* tables) writer queue
 */
Queue *g_dciDataWriterQueue = NULL;

/**
 * Raw DCI data writer queue
 */
Queue *g_dciRawDataWriterQueue = NULL;

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
static THREAD s_iDataWriterThread = INVALID_THREAD_HANDLE;
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
void QueueIDataInsert(time_t timestamp, UINT32 nodeId, UINT32 dciId, const TCHAR *value)
{
	DELAYED_IDATA_INSERT *rq = (DELAYED_IDATA_INSERT *)malloc(sizeof(DELAYED_IDATA_INSERT));
	rq->timestamp = timestamp;
	rq->nodeId = nodeId;
	rq->dciId = dciId;
	nx_strncpy(rq->value, value, MAX_RESULT_LENGTH);
	g_dciDataWriterQueue->put(rq);
	g_idataWriteRequests++;
}

/**
 * Queue UPDATE request for raw_dci_values table
 */
void QueueRawDciDataUpdate(time_t timestamp, UINT32 dciId, const TCHAR *rawValue, const TCHAR *transformedValue)
{
	DELAYED_RAW_DATA_UPDATE *rq = (DELAYED_RAW_DATA_UPDATE *)malloc(sizeof(DELAYED_RAW_DATA_UPDATE));
	rq->timestamp = timestamp;
	rq->dciId = dciId;
	nx_strncpy(rq->rawValue, rawValue, MAX_RESULT_LENGTH);
	nx_strncpy(rq->transformedValue, transformedValue, MAX_RESULT_LENGTH);
	g_dciRawDataWriterQueue->put(rq);
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
   int maxRecords = ConfigReadInt(_T("DBWriter.MaxRecordsPerTransaction"), 1000);
   while(true)
   {
		DELAYED_IDATA_INSERT *rq = (DELAYED_IDATA_INSERT *)g_dciDataWriterQueue->getOrBlock();
      if (rq == INVALID_POINTER_VALUE)   // End-of-job indicator
         break;

      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
		if (DBBegin(hdb))
		{
			int count = 0;
			while(1)
			{
				bool success;

				// For Oracle preparing statement even for one time execution is preferred
				// For other databases it will actually slow down inserts
				if (g_dbSyntax == DB_SYNTAX_ORACLE)
				{
	            TCHAR query[256];
               _sntprintf(query, 256, _T("INSERT INTO idata_%d (item_id,idata_timestamp,idata_value) VALUES (?,?,?)"), (int)rq->nodeId);
               DB_STATEMENT hStmt = DBPrepare(hdb, query);
               if (hStmt != NULL)
               {
                  DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, rq->dciId);
                  DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, (INT64)rq->timestamp);
                  DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, rq->value, DB_BIND_STATIC);
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
               _sntprintf(query, 1024, _T("INSERT INTO idata_%d (item_id,idata_timestamp,idata_value) VALUES (%d,%d,%s)"),
                          (int)rq->nodeId, (int)rq->dciId, (int)rq->timestamp, (const TCHAR *)DBPrepareString(hdb, rq->value));
               success = DBQuery(hdb, query);
				}

				free(rq);

				count++;
				if (!success || (count > maxRecords))
					break;

				rq = (DELAYED_IDATA_INSERT *)g_dciDataWriterQueue->get();
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
 * Database "lazy" write thread for raw_dci_values UPDATEs
 */
static THREAD_RESULT THREAD_CALL RawDataWriteThread(void *arg)
{
   ThreadSetName("DBWriter/RData");
   int maxRecords = ConfigReadInt(_T("DBWriter.MaxRecordsPerTransaction"), 1000);
   while(true)
   {
		DELAYED_RAW_DATA_UPDATE *rq = (DELAYED_RAW_DATA_UPDATE *)g_dciRawDataWriterQueue->getOrBlock();
      if (rq == INVALID_POINTER_VALUE)   // End-of-job indicator
         break;

      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
		if (DBBegin(hdb))
		{
         DB_STATEMENT hStmt = DBPrepare(hdb, _T("UPDATE raw_dci_values SET raw_value=?,transformed_value=?,last_poll_time=? WHERE item_id=?"));
         if (hStmt != NULL)
         {
            int count = 0;
            while(true)
            {
               DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, rq->rawValue, DB_BIND_STATIC);
               DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, rq->transformedValue, DB_BIND_STATIC);
               DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, (INT64)rq->timestamp);
               DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, rq->dciId);
               bool success = DBExecute(hStmt);

               free(rq);

               count++;
               if (!success || (maxRecords > 1000))
                  break;

               rq = (DELAYED_RAW_DATA_UPDATE *)g_dciRawDataWriterQueue->get();
               if ((rq == NULL) || (rq == INVALID_POINTER_VALUE))
                  break;
            }
            DBFreeStatement(hStmt);
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
 * Start writer thread
 */
void StartDBWriter()
{
   s_writerThread = ThreadCreateEx(DBWriteThread, 0, NULL);
	s_iDataWriterThread = ThreadCreateEx(IDataWriteThread, 0, NULL);
	s_rawDataWriterThread = ThreadCreateEx(RawDataWriteThread, 0, NULL);
}

/**
 * Stop writer thread and wait while all queries will be executed
 */
void StopDBWriter()
{
   g_dbWriterQueue->put(INVALID_POINTER_VALUE);
	g_dciDataWriterQueue->put(INVALID_POINTER_VALUE);
	g_dciRawDataWriterQueue->put(INVALID_POINTER_VALUE);
   ThreadJoin(s_writerThread);
	ThreadJoin(s_iDataWriterThread);
	ThreadJoin(s_rawDataWriterThread);
}
