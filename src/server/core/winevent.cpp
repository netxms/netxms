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

#define DEBUG_TAG _T("winevt")

/**
 * Constructor for Windows event structure
 */
WindowsEvent::WindowsEvent(uint32_t _nodeId, int32_t _zoneUIN, const NXCPMessage& msg)
{
   timestamp = time(nullptr);
   nodeId = _nodeId;
   zoneUIN = _zoneUIN;
   msg.getFieldAsString(VID_LOG_NAME, logName, 64);
   originTimestamp = msg.getFieldAsTime(VID_TIMESTAMP);
   msg.getFieldAsString(VID_EVENT_SOURCE, eventSource, 128);
   eventSeverity = msg.getFieldAsInt32(VID_EVENT_SEVERITY);
   eventCode = msg.getFieldAsInt32(VID_EVENT_CODE);
   message = msg.getFieldAsString(VID_MESSAGE);
   rawData = msg.getFieldAsString(VID_RAW_DATA);
}

/**
 * Destructor for Windows event structure
 */
WindowsEvent::~WindowsEvent()
{
   MemFree(message);
   MemFree(rawData);
}

/**
 * Windows event log processing queue
 */
static ObjectQueue<WindowsEvent> s_windowsEventQueue;
static THREAD s_writerThread = INVALID_THREAD_HANDLE;
static uint64_t s_eventId = 1;

/**
 * Put new event log message to the queue
 */
void QueueWindowsEvent(WindowsEvent *event)
{
   s_windowsEventQueue.put(event);
}

/**
 * Windows event writer thread
 */
static void WindowsEventWriterThread()
{
   ThreadSetName("WinEvtWriter");
   int maxRecords = ConfigReadInt(_T("DBWriter.MaxRecordsPerTransaction"), 1000);
   nxlog_debug_tag(DEBUG_TAG, 2, _T("Windows event writer started"));

   while(true)
   {
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
      WindowsEvent *event = s_windowsEventQueue.getOrBlock();
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
         DBBind(hStmt, 7, DB_SQLTYPE_VARCHAR, event->eventSource, DB_BIND_STATIC, 127);
         DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, event->eventSeverity);
         DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, event->eventCode);
         DBBind(hStmt, 10, DB_SQLTYPE_VARCHAR, event->message, DB_BIND_STATIC, MAX_EVENT_MSG_LENGTH);
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

         event = s_windowsEventQueue.getOrBlock(500);
         if ((event == nullptr) || (event == INVALID_POINTER_VALUE))
            break;
      }
      DBCommit(hdb);
      DBFreeStatement(hStmt);
      DBConnectionPoolReleaseConnection(hdb);
      if (event == INVALID_POINTER_VALUE)
         break;
   }

   nxlog_debug_tag(DEBUG_TAG, 2, _T("Windows event writer stopped"));
}

/**
 * Get next windows event log id
 */
uint64_t GetNextWinEventId()
{
   return s_eventId++;
}

/**
 * Start Windows event log writer thread
 */
void StartWindowsEventWriter()
{
   // Determine first available event id
   uint64_t id = ConfigReadUInt64(_T("FirstFreeWineventId"), s_eventId);
   if (id > s_eventId)
      s_eventId = id;
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

   s_writerThread = ThreadCreateEx(WindowsEventWriterThread);
}

/**
 * Stop Windows event log writer thread
 */
void StopWindowsEventWriter()
{
   s_windowsEventQueue.put(INVALID_POINTER_VALUE);
   nxlog_debug_tag(DEBUG_TAG, 3, _T("Waiting for Windows event writer to stop"));
   ThreadJoin(s_writerThread);
}
