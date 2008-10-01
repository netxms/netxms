/*
** NetXMS LogWatch subagent
** Copyright (C) 2008 Victor Kirhenshtein
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
** File: eventlog.cpp
**
**/

#include "logwatch.h"


//
// Constants
//

#define BUFFER_SIZE		32768


//
// Event source class definition
//

class EventSource
{
private:
	TCHAR *m_name;
	int m_numModules;
	HMODULE *m_modules;

public:
	EventSource(const TCHAR *name);
	~EventSource();

	BOOL Load();
	BOOL FormatMessage(EVENTLOGRECORD *rec, TCHAR *msg, size_t msgSize);

	const TCHAR *GetName() { return m_name; }
};


//
// Event source constructor
//

EventSource::EventSource(const TCHAR *name)
{
	m_name = _tcsdup(name);
	m_numModules = 0;
	m_modules = NULL;
}


//
// Event source destructor
//

EventSource::~EventSource()
{
	int i;

	safe_free(m_name);
	for(i = 0; i < m_numModules; i++)
		FreeLibrary(m_modules[i]);
	safe_free(m_modules);
}


//
// Load event source
//

BOOL EventSource::Load()
{
   HKEY hKey;
   TCHAR buffer[MAX_PATH], path[MAX_PATH], *curr, *next;
   HMODULE hModule;
   DWORD size = MAX_PATH;
   BOOL isLoaded = FALSE;

   _sntprintf(buffer, MAX_PATH, _T("System\\CurrentControlSet\\Services\\EventLog\\System\\%s"), m_name);
   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, buffer, 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
   {
      if (RegQueryValueEx(hKey, _T("EventMessageFile"), NULL, NULL, (BYTE *)buffer, &size) == ERROR_SUCCESS)
      {
         ExpandEnvironmentStrings(buffer, path, MAX_PATH);
         for(curr = path; curr != NULL; curr = next)
         {
            next = _tcschr(curr, _T(';'));
            if (next != NULL)
            {
               *next = 0;
               next++;
            }
            hModule = LoadLibraryEx(curr, NULL, LOAD_LIBRARY_AS_DATAFILE);
            if (hModule != NULL)
            {
					m_numModules++;
					m_modules = (HMODULE *)realloc(m_modules, sizeof(HMODULE) * m_numModules);
					m_modules[m_numModules - 1] = hModule;
               NxWriteAgentLog(EVENTLOG_DEBUG_TYPE, _T("LogWatch: Message file \"%s\" loaded"), curr);
					isLoaded = TRUE;
            }
            else
            {
               NxWriteAgentLog(EVENTLOG_WARNING_TYPE, _T("LogWatch: Unable to load message file \"%s\": %s"), 
					                curr, GetSystemErrorText(GetLastError(), buffer, MAX_PATH));
            }
         }
      }
      RegCloseKey(hKey);
   }

	return isLoaded;
}


//
// Format message
//

BOOL EventSource::FormatMessage(EVENTLOGRECORD *rec, TCHAR *msg, size_t msgSize)
{
   TCHAR *strings[256], *str;
   int i;
	BOOL success = FALSE;

   memset(strings, 0, sizeof(TCHAR *) * 256);
   for(i = 0, str = (TCHAR *)((BYTE *)rec + rec->StringOffset); i < rec->NumStrings; i++)
   {
      strings[i] = str;
      str += _tcslen(str) + 1;
   }

   for(i = 0; i < m_numModules; i++)
   {
      if (::FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | 
                          FORMAT_MESSAGE_ARGUMENT_ARRAY | 
                          FORMAT_MESSAGE_MAX_WIDTH_MASK,
                          m_modules[i], rec->EventID, 0, msg, msgSize,
                          (va_list *)strings) > 0)
		{
			success = TRUE;
         break;
		}
   }
   if (!success)
   {
      NxWriteAgentLog(EVENTLOG_DEBUG_TYPE, _T("LogWatch: event log %s FormatMessage(%d) error: %s"),
		                m_name, rec->EventID, 
							 GetSystemErrorText(GetLastError(), msg, msgSize));
      nx_strncpy(msg, _T("**** LogWatch: cannot format message ****"), msgSize);
   }
	return success;
}


//
// Loaded event sources
//

static int m_numEventSources;
static EventSource **m_eventSourceList = NULL;
static CRITICAL_SECTION m_csEventSourceAccess;


//
// Load event source
//

EventSource *LoadEventSource(const TCHAR *name)
{
	EventSource *es = NULL;
	int i;

	EnterCriticalSection(&m_csEventSourceAccess);
	
	for(i = 0; i < m_numEventSources; i++)
	{
		if (!_tcsicmp(name, m_eventSourceList[i]->GetName()))
		{
			es = m_eventSourceList[i];
			break;
		}
	}

	if (es == NULL)
	{
		es = new EventSource(name);
		if (es->Load())
		{
			m_numEventSources++;
			m_eventSourceList = (EventSource **)realloc(m_eventSourceList, sizeof(EventSource *) * m_numEventSources);
			m_eventSourceList[m_numEventSources - 1] = es;
		}
		else
		{
			delete_and_null(es);
		}
	}

	LeaveCriticalSection(&m_csEventSourceAccess);
	return es;
}


//
// Notification thread's data
//

struct NOTIFICATION_THREAD_DATA
{
	HANDLE hLogEvent;
	HANDLE hWakeupEvent;
};


//
// Change notification thread
//

static THREAD_RESULT THREAD_CALL NotificationThread(void *arg)
{
	HANDLE handles[2];

	handles[0] = ((NOTIFICATION_THREAD_DATA *)arg)->hLogEvent;
	handles[1] = g_hCondShutdown;
   while(1)
   {
      if (WaitForMultipleObjects(2, handles, FALSE, INFINITE) == WAIT_OBJECT_0 + 1)
			break;
      SetEvent(((NOTIFICATION_THREAD_DATA *)arg)->hWakeupEvent);
   }
	return THREAD_OK;
}


//
// Parse event log record
//

static void ParseEvent(LogParser *parser, EVENTLOGRECORD *rec)
{
	TCHAR msg[8192];
	EventSource *es;

	es = LoadEventSource((TCHAR *)((BYTE *)rec + sizeof(EVENTLOGRECORD)));
	if (es != NULL)
	{
		es->FormatMessage(rec, msg, 8192);
		parser->MatchLine(msg);
	}
}


//
// Callback for matched event log records
//

static void EventLogParserCallback(DWORD event, const TCHAR *text, int paramCount,
											  TCHAR **paramList, DWORD objectId, void *userArg)
{
	NxSendTrap2(event, paramCount, paramList);
}


//
// Event log parser thread
//

THREAD_RESULT THREAD_CALL ParserThreadEventLog(void *arg)
{
	LogParser *parser = (LogParser *)arg;
   HANDLE hLog, handles[2];
   BYTE *buffer, *rec;
   DWORD bytes, bytesNeeded;
   BOOL success;
	THREAD nt;	// NOtification thread's handle
	NOTIFICATION_THREAD_DATA nd;

   buffer = (BYTE *)malloc(BUFFER_SIZE);
	parser->SetCallback(EventLogParserCallback);

   hLog = OpenEventLog(NULL, &(parser->GetFileName()[1]));
   if (hLog != NULL)
   {
      nd.hLogEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
      nd.hWakeupEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
      
      // Initial read
      while(ReadEventLog(hLog, EVENTLOG_SEQUENTIAL_READ | EVENTLOG_FORWARDS_READ, 0,
                         buffer, BUFFER_SIZE, &bytes, &bytesNeeded));

      if (GetLastError() == ERROR_HANDLE_EOF)
      {
         nt = ThreadCreateEx(NotificationThread, 0, &nd);
         NotifyChangeEventLog(hLog, nd.hLogEvent);
			handles[0] = nd.hWakeupEvent;
			handles[1] = g_hCondShutdown;
			NxWriteAgentLog(EVENTLOG_DEBUG_TYPE, _T("LogWatch: Start watching event log \"%s\""),
			                &(parser->GetFileName()[1]));

         while(1)
         {
            if (WaitForMultipleObjects(2, handles, FALSE, INFINITE) == WAIT_OBJECT_0 + 1)
					break;	// Shutdown condition raised

            do
            {
               success = ReadEventLog(hLog, EVENTLOG_SEQUENTIAL_READ | EVENTLOG_FORWARDS_READ, 0,
                                       buffer, BUFFER_SIZE, &bytes, &bytesNeeded);
               if (success)
               {
                  for(rec = buffer; rec < buffer + bytes; rec += ((EVENTLOGRECORD *)rec)->Length)
                     ParseEvent(parser, (EVENTLOGRECORD *)rec);
               }
            } while(success);

            if (GetLastError() != ERROR_HANDLE_EOF)
					NxWriteAgentLog(EVENTLOG_ERROR_TYPE, _T("LogWatch: Unable to read event log \"%s\": %s"),
										 &(parser->GetFileName()[1]), GetSystemErrorText(GetLastError(), (TCHAR *)buffer, BUFFER_SIZE / sizeof(TCHAR)));
         }

			ThreadJoin(nt);
			NxWriteAgentLog(EVENTLOG_DEBUG_TYPE, _T("LogWatch: Stop watching event log \"%s\""),
			                &(parser->GetFileName()[1]));
      }
      else
      {
			NxWriteAgentLog(EVENTLOG_ERROR_TYPE, _T("LogWatch: Unable to read event log \"%s\": %s"),
			                &(parser->GetFileName()[1]), GetSystemErrorText(GetLastError(), (TCHAR *)buffer, BUFFER_SIZE / sizeof(TCHAR)));
      }
   }
   else
   {
		NxWriteAgentLog(EVENTLOG_ERROR_TYPE, _T("LogWatch: Unable to open event log \"%s\""),
		                &(parser->GetFileName()[1]), GetSystemErrorText(GetLastError(), (TCHAR *)buffer, BUFFER_SIZE / sizeof(TCHAR)));
   }

	free(buffer);
	return THREAD_OK;
}


//
// Initialize event log parsers
//

void InitEventLogParsers()
{
	InitializeCriticalSection(&m_csEventSourceAccess);
}


//
// Cleanup event log parsers
//

void CleanupEventLogParsers()
{
	int i;

	for(i = 0; i < m_numEventSources; i++)
		delete m_eventSourceList[i];
	safe_free(m_eventSourceList);
	DeleteCriticalSection(&m_csEventSourceAccess);
}
