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
#include <nxlpapi.h>

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
   eventSeverity = msg.getFieldAsInt32(VID_SEVERITY);
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
 * Processing queues
 */
ObjectQueue<WindowsEvent> g_windowsEventProcessingQueue;
ObjectQueue<WindowsEvent> g_windowsEventWriterQueue;

/**
 * Static data
 */
static uint64_t s_eventId = 1;  // Next available event ID
static LogParser *s_parser = nullptr;
static MUTEX s_parserLock = INVALID_MUTEX_HANDLE;
static THREAD s_writerThread = INVALID_THREAD_HANDLE;
static THREAD s_processingThread = INVALID_THREAD_HANDLE;

/**
 * Put new event log message to the queue
 */
void QueueWindowsEvent(WindowsEvent *event)
{
   g_windowsEventProcessingQueue.put(event);
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
      WindowsEvent *event = g_windowsEventWriterQueue.getOrBlock();
      if (event == INVALID_POINTER_VALUE)
         break;

      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
      DB_STATEMENT hStmt = DBPrepare(hdb,
            (g_dbSyntax == DB_SYNTAX_TSDB) ?
                  _T("INSERT INTO win_event_log (id,event_timestamp,node_id,zone_uin,origin_timestamp,log_name,event_source,event_severity,event_code,message,raw_data) VALUES (?,to_timestamp(?),?,?,?,?,?,?,?,?,?)") :
                  _T("INSERT INTO win_event_log (id,event_timestamp,node_id,zone_uin,origin_timestamp,log_name,event_source,event_severity,event_code,message,raw_data) VALUES (?,?,?,?,?,?,?,?,?,?,?)"), true);
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

         event = g_windowsEventWriterQueue.getOrBlock(500);
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
 * Windows event writer thread - PostgreSQL version
 */
static void WindowsEventWriterThread_PGSQL()
{
   ThreadSetName("WinEvtWriter");
   nxlog_debug_tag(DEBUG_TAG, 2, _T("Windows event writer started"));

   int maxRecordsPerTxn = ConfigReadInt(_T("DBWriter.MaxRecordsPerTransaction"), 1000);
   int maxRecordsPerStmt = ConfigReadInt(_T("DBWriter.MaxRecordsPerStatement"), 100);
   if (maxRecordsPerTxn < maxRecordsPerStmt)
      maxRecordsPerTxn = maxRecordsPerStmt;
   else if (maxRecordsPerTxn % maxRecordsPerStmt != 0)
      maxRecordsPerTxn = (maxRecordsPerTxn / maxRecordsPerStmt + 1) * maxRecordsPerStmt;
   bool convertTimestamp = (g_dbSyntax == DB_SYNTAX_TSDB);

   StringBuffer query;
   query.setAllocationStep(65536);
   const TCHAR *queryBase = _T("INSERT INTO win_event_log (id,event_timestamp,node_id,zone_uin,origin_timestamp,log_name,event_source,event_severity,event_code,message,raw_data) VALUES ");

   while(true)
   {
      WindowsEvent *event = g_windowsEventWriterQueue.getOrBlock();
      if (event == INVALID_POINTER_VALUE)
         break;

      query = queryBase;
      int countTxn = 0, countStmt = 0;

      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
      if (!DBBegin(hdb))
      {
         delete event;
         DBConnectionPoolReleaseConnection(hdb);
         continue;
      }

      while(true)
      {
         query.append(_T('('));
         query.append(s_eventId++);
         if (convertTimestamp)
         {
            query.append(_T(",to_timestamp("));
            query.append(static_cast<int64_t>(event->timestamp));
            query.append(_T("),"));
         }
         else
         {
            query.append(_T(','));
            query.append(static_cast<int64_t>(event->timestamp));
            query.append(_T(','));
         }
         query.append(event->nodeId);
         query.append(_T(','));
         query.append(event->zoneUIN);
         query.append(_T(','));
         query.append(static_cast<int64_t>(event->originTimestamp));
         query.append(_T(','));
         query.append(DBPrepareString(hdb, event->logName, 63));
         query.append(_T(','));
         query.append(DBPrepareString(hdb, event->eventSource, 127));
         query.append(_T(','));
         query.append(event->eventSeverity);
         query.append(_T(','));
         query.append(event->eventCode);
         query.append(_T(','));
         query.append(DBPrepareString(hdb, event->message, MAX_EVENT_MSG_LENGTH));
         query.append(_T(','));
         query.append(DBPrepareString(hdb, event->rawData));
         query.append(_T("),"));
         delete event;

         countTxn++;
         countStmt++;

         if (countStmt >= maxRecordsPerStmt)
         {
            countStmt = 0;
            query.shrink();   // Remove trailing , from last block
            query.append(_T(" ON CONFLICT DO NOTHING"));
            if (!DBQuery(hdb, query))
               break;
            query = queryBase;
         }

         if (countTxn >= maxRecordsPerTxn)
            break;

         event = g_windowsEventWriterQueue.getOrBlock(500);
         if ((event == nullptr) || (event == INVALID_POINTER_VALUE))
            break;
      }
      if (countStmt > 0)
      {
         query.shrink();   // Remove trailing , from last block
         query.append(_T(" ON CONFLICT DO NOTHING"));
         DBQuery(hdb, query);
      }
      DBCommit(hdb);
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
   return s_eventId;
}

/**
 * Callback for Windows event parser
 */
static void WindwsEventParserCallback(UINT32 eventCode, const TCHAR *eventName, const TCHAR *eventTag,
         const TCHAR *line, const TCHAR *source, UINT32 facility, UINT32 severity, const StringList *captureGroups,
         const StringList *variables, UINT64 recordId, UINT32 objectId, int repeatCount, time_t timestamp,
         const TCHAR *agentAction, const StringList *agentActionArgs, void *context)
{
   nxlog_debug_tag(DEBUG_TAG, 7, _T("Windows event message matched, capture group count = %d, repeat count = %d"), captureGroups->size(), repeatCount);

   shared_ptr<Node> node = static_pointer_cast<Node>(FindObjectById(objectId, OBJECT_NODE));
   if (node == nullptr || (node->getStatus() != STATUS_UNMANAGED) || (g_flags & AF_TRAPS_FROM_UNMANAGED_NODES))
   {
      StringMap pmap;
      for(int i = 0; i < captureGroups->size(); i++)
      {
         TCHAR name[32];
         _sntprintf(name, 32, _T("cg%d"), i + 1);
         pmap.set(name, captureGroups->get(i));
      }
      pmap.set(_T("repeatCount"), repeatCount);

      PostEventWithTagAndNames(eventCode, EventOrigin::WIN_EVENT, timestamp, objectId, eventTag, &pmap);
   }
}

/**
 * Initialize parser on start on config change
 */
void InitializeWindowsEventParser()
{
   MutexLock(s_parserLock);
   LogParser *prev = s_parser;
   s_parser = nullptr;
#ifdef UNICODE
   char *xml;
   WCHAR *wxml = ConfigReadCLOB(_T("WindowsEventParser"), _T("<parser></parser>"));
   if (wxml != nullptr)
   {
      xml = UTF8StringFromWideString(wxml);
      MemFree(wxml);
   }
   else
   {
      xml = nullptr;
   }
#else
   char *xml = ConfigReadCLOB("WindowsEventParser", "<parser></parser>");
#endif
   if (xml != nullptr)
   {
      TCHAR parseError[256];
      ObjectArray<LogParser> *parsers = LogParser::createFromXml(xml, -1, parseError, 256, EventNameResolver);
      if ((parsers != nullptr) && (parsers->size() > 0))
      {
         s_parser = parsers->get(0);
         s_parser->setCallback(WindwsEventParserCallback);
         if (prev != nullptr)
            s_parser->restoreCounters(prev);
         nxlog_debug_tag(DEBUG_TAG, 3, _T("Windows evnet parser successfully created from config"));
      }
      else
      {
         nxlog_write(NXLOG_ERROR, _T("Cannot initialize Windows event parser (%s)"), parseError);
      }
      MemFree(xml);
      delete parsers;
   }
   MutexUnlock(s_parserLock);
   delete prev;
}

/**
 * Windows event processing thread
 */
static void WindowsEventProcessingThread()
{
   ThreadSetName("WinEvtProc");

   while(true)
   {
      WindowsEvent *event = g_windowsEventProcessingQueue.getOrBlock();
      if (event == INVALID_POINTER_VALUE)
         break;

      bool write = true;

      MutexLock(s_parserLock);
      if (s_parser != nullptr)
      {
         s_parser->matchEvent(event->eventSource, event->eventCode, event->eventSeverity, event->message, nullptr, 0, event->nodeId, event->originTimestamp, event->logName, &write);
      }
      MutexUnlock(s_parserLock);

      if (write)
         g_windowsEventWriterQueue.put(event);
      else
         delete event;
   }

}

/**
 * Start Windows event log writer and processing threads
 */
void StartWindowsEventProcessing()
{
   // Determine first available event id
   uint64_t id = ConfigReadUInt64(_T("FirstFreeWinEventId"), s_eventId);
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

   InitLogParserLibrary();

   // Create message parser
   s_parserLock = MutexCreate();
   InitializeWindowsEventParser();

   s_writerThread = ThreadCreateEx((g_dbSyntax == DB_SYNTAX_PGSQL) || (g_dbSyntax == DB_SYNTAX_TSDB) ? WindowsEventWriterThread_PGSQL :  WindowsEventWriterThread);
   s_processingThread = ThreadCreateEx(WindowsEventProcessingThread);
}

/**
 * Stop Windows event log writer and processing threads
 */
void StopWindowsEventProcessing()
{
   g_windowsEventProcessingQueue.put(INVALID_POINTER_VALUE);
   nxlog_debug_tag(DEBUG_TAG, 3, _T("Waiting for Windows event processing thread to stop"));
   ThreadJoin(s_processingThread);

   g_windowsEventWriterQueue.put(INVALID_POINTER_VALUE);
   nxlog_debug_tag(DEBUG_TAG, 3, _T("Waiting for Windows event writer to stop"));
   ThreadJoin(s_writerThread);

   CleanupLogParserLibrary();
}
