/*
** NetXMS - Network Management System
** Copyright (C) 2020 Raden Solutions
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
** File: wineventlog.cpp
**
**/


#include "nxcore.h"

/**
 * Constructor for Windows event log structure
 */
WindowsEventLogRecord::WindowsEventLogRecord()
{
   timestamp = 0;
   nodeId = 0;
   zoneUIN = 0;
   *logName = 0;
   originTimestamp = 0;
   *eventSource = 0;
   eventSeverity = 0;
   eventCode = 0;
   message = nullptr;
   rawData = nullptr;
}

/**
 * Destructor for  Windows event log structure
 */
WindowsEventLogRecord::~WindowsEventLogRecord()
{
   MemFree(message);
   MemFree(rawData);
}

/**
 * Windows event log processing queue
 */
static ObjectQueue<WindowsEventLogRecord> s_windowsEventLogQueue;
static THREAD s_processingThread = INVALID_THREAD_HANDLE;
static uint64_t s_eventId = 1;

/**
 * Put new event log message to the queue
 */
void QueueWinEventLogMessage(WindowsEventLogRecord *event)
{
   s_windowsEventLogQueue.put(event);
}


/**
 * Windows event log processing thread
 */
static THREAD_RESULT THREAD_CALL WinEventLogProcessingThread(void *pArg)
{
   ThreadSetName("WinEventLogProcessor");
   while(true)
   {
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
      WindowsEventLogRecord *event = s_windowsEventLogQueue.getOrBlock();
      int maxRecords = ConfigReadInt(_T("DBWriter.MaxRecordsPerTransaction"), 1000);

      if (event == INVALID_POINTER_VALUE)
         break;

      DB_STATEMENT hStmt = DBPrepare(hdb,
                     (g_dbSyntax == DB_SYNTAX_TSDB) ?
                              _T("INSERT INTO win_event_log (id,timestamp,node_id,zone_uin,origin_timestamp,log_name,event_source,event_severity,event_code,message,raw_data) VALUES (?,to_timestamp(?),?,?,?,?,?,?,?,?,?)") :
                              _T("INSERT INTO win_event_log (id,timestamp,node_id,zone_uin,origin_timestamp,log_name,event_source,event_severity,event_code,message,raw_data) VALUES (?,?,?,?,?,?,?,?,?,?,?)"), true);
      if (hStmt == nullptr)
      {
         delete event;
         DBConnectionPoolReleaseConnection(hdb);
         continue;
      }

      int count = 0;
      DBBegin(hdb);
      while(true)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_BIGINT, s_eventId++);
         DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, (int32_t)event->timestamp);
         DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, event->nodeId);
         DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, event->zoneUIN);
         DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, (int32_t)event->originTimestamp);
         DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, event->logName, DB_BIND_STATIC, 63);
         DBBind(hStmt, 7, DB_SQLTYPE_VARCHAR, event->eventSource, DB_BIND_STATIC, 126);
         DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, event->eventSeverity);
         DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, event->eventCode);
         DBBind(hStmt, 10, DB_SQLTYPE_VARCHAR, event->message, DB_BIND_STATIC, MAX_DB_VARCHAR);
         DBBind(hStmt, 11, DB_SQLTYPE_TEXT, event->rawData, DB_BIND_STATIC);


         if (!DBExecute(hStmt))
         {
            delete event;
            break;
         }
         delete event;
         count++;
         if (count == maxRecords)
            break;
         event = s_windowsEventLogQueue.getOrBlock(500);
         if ((event == nullptr) || (event == INVALID_POINTER_VALUE))
            break;
      }
      DBCommit(hdb);
      DBFreeStatement(hStmt);
      DBConnectionPoolReleaseConnection(hdb);
      if (event == INVALID_POINTER_VALUE)
         break;
   }
   return THREAD_OK;
}

/**
 * Start Windows event log processing thread
 */
void StartWinEventLogWriter()
{
   // Determine first available event id
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, _T("SELECT max(id) FROM win_event_log"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         s_eventId = std::max(DBGetFieldUInt64(hResult, 0, 0) + 1, s_eventId);
      }
      DBFreeResult(hResult);
   }
   DBConnectionPoolReleaseConnection(hdb);

   s_processingThread = ThreadCreateEx(WinEventLogProcessingThread, 0, nullptr);
}

/**
 * Stop Windows event log processing thread
 */
void StopWinEventLogWriter()
{
   // Stop processing thread
   s_windowsEventLogQueue.put(INVALID_POINTER_VALUE);
   ThreadJoin(s_processingThread);
}
