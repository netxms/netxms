/*
** NetXMS - Network Management System
** Log Parsing Library
** Copyright (C) 2003-2014 Victor Kirhenshtein
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

#include "libnxlp.h"

/**
 * Event source class definition
 */
class EventSource
{
private:
	TCHAR *m_name;
	TCHAR *m_logName;
	int m_numModules;
	HMODULE *m_modules;

public:
	EventSource(const TCHAR *logName, const TCHAR *name);
	~EventSource();

	BOOL load();
	BOOL formatMessage(EVENTLOGRECORD *rec, TCHAR *msg, size_t msgSize);

	const TCHAR *getName() { return m_name; }
	const TCHAR *getLogName() { return m_logName; }
};

/**
 * Event source constructor
 */
EventSource::EventSource(const TCHAR *logName, const TCHAR *name)
{
	m_logName = _tcsdup(logName);
	m_name = _tcsdup(name);
	m_numModules = 0;
	m_modules = NULL;
}

/**
 * Event source destructor
 */
EventSource::~EventSource()
{
	int i;

	safe_free(m_logName);
	safe_free(m_name);
	for(i = 0; i < m_numModules; i++)
		FreeLibrary(m_modules[i]);
	safe_free(m_modules);
}

/**
 * Load event source
 */
BOOL EventSource::load()
{
   HKEY hKey;
   TCHAR buffer[MAX_PATH], path[MAX_PATH], *curr, *next;
   HMODULE hModule;
   DWORD size = MAX_PATH;
   BOOL isLoaded = FALSE;

   _sntprintf(buffer, MAX_PATH, _T("System\\CurrentControlSet\\Services\\EventLog\\%s\\%s"), m_logName, m_name);
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
               LogParserTrace(4, _T("LogWatch: Message file \"%s\" loaded"), curr);
					isLoaded = TRUE;
            }
            else
            {
               LogParserTrace(0, _T("LogWatch: Unable to load message file \"%s\": %s"), 
					                curr, GetSystemErrorText(GetLastError(), buffer, MAX_PATH));
            }
         }
      }
      RegCloseKey(hKey);
   }

	return isLoaded;
}

/**
 * Format message
 */
BOOL EventSource::formatMessage(EVENTLOGRECORD *rec, TCHAR *msg, size_t msgSize)
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
                          m_modules[i], rec->EventID, 0, msg, (DWORD)msgSize,
                          (va_list *)strings) > 0)
		{
			success = TRUE;
         break;
		}
   }
   if (!success)
   {
      LogParserTrace(4, _T("LogWatch: event log %s source %s FormatMessage(%d) error: %s"),
		                m_logName, m_name, rec->EventID, 
							 GetSystemErrorText(GetLastError(), msg, msgSize));
      nx_strncpy(msg, _T("**** LogWatch: cannot format message ****"), msgSize);
   }
	return success;
}

/**
 * Loaded event sources
 */
static int m_numEventSources;
static EventSource **m_eventSourceList = NULL;
static CRITICAL_SECTION m_csEventSourceAccess;

/**
 * Load event source
 */
EventSource *LoadEventSource(const TCHAR *log, const TCHAR *name)
{
	EventSource *es = NULL;
	int i;

	EnterCriticalSection(&m_csEventSourceAccess);
	
	for(i = 0; i < m_numEventSources; i++)
	{
		if (!_tcsicmp(name, m_eventSourceList[i]->getName()) &&
			 !_tcsicmp(log, m_eventSourceList[i]->getLogName()))
		{
			es = m_eventSourceList[i];
			break;
		}
	}

	if (es == NULL)
	{
		es = new EventSource(log, name);
		if (es->load())
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

/**
 * Notification thread's data
 */
struct NOTIFICATION_THREAD_DATA
{
	HANDLE hLogEvent;
	HANDLE hWakeupEvent;
	HANDLE hStopEvent;
};

/**
 * Change notification thread
 */
static THREAD_RESULT THREAD_CALL NotificationThread(void *arg)
{
	HANDLE handles[2];

	handles[0] = ((NOTIFICATION_THREAD_DATA *)arg)->hLogEvent;
	handles[1] = ((NOTIFICATION_THREAD_DATA *)arg)->hStopEvent;
   while(1)
   {
      if (WaitForMultipleObjects(2, handles, FALSE, INFINITE) == WAIT_OBJECT_0 + 1)
			break;
      SetEvent(((NOTIFICATION_THREAD_DATA *)arg)->hWakeupEvent);
   }
	return THREAD_OK;
}

/**
 * Parse event log record
 */
void LogParser::parseEvent(EVENTLOGRECORD *rec)
{
	TCHAR msg[8192], *eventSourceName;
	EventSource *es;

	eventSourceName = (TCHAR *)((BYTE *)rec + sizeof(EVENTLOGRECORD));
	es = LoadEventSource(&m_fileName[1], eventSourceName);
	if (es != NULL)
	{
		es->formatMessage(rec, msg, 8192);
		matchEvent(eventSourceName, rec->EventID & 0x0000FFFF, rec->EventType, msg);
	}
	else
	{
		LogParserTrace(4, _T("LogWatch: unable to load event source \"%s\" for log \"%s\""), eventSourceName, &m_fileName[1]);
	}
}

/**
 * Event log parser thread
 */
bool LogParser::monitorEventLogV4(CONDITION stopCondition)
{
   HANDLE hLog, handles[2];
   BYTE *buffer, *rec;
   DWORD bytes, bytesNeeded, bufferSize = 32768, error = 0;
   BOOL success, reopen = FALSE;
	THREAD nt;	// Notification thread's handle
	NOTIFICATION_THREAD_DATA nd;
   bool result;

   buffer = (BYTE *)malloc(bufferSize);

reopen_log:
   hLog = OpenEventLog(NULL, &m_fileName[1]);
   if (hLog != NULL)
   {
      nd.hLogEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
      nd.hWakeupEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
      nd.hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
      
      // Initial read (skip on reopen)
		if (!reopen)
		{
			LogParserTrace(7, _T("LogWatch: Initial read of event log \"%s\""), &m_fileName[1]);
			do
			{
				while((success = ReadEventLog(hLog, EVENTLOG_SEQUENTIAL_READ | EVENTLOG_FORWARDS_READ, 0,
														buffer, bufferSize, &bytes, &bytesNeeded)));
				if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
				{
					bufferSize = bytesNeeded;
					buffer = (BYTE *)realloc(buffer, bufferSize);
					success = TRUE;
					LogParserTrace(9, _T("LogWatch: Increasing buffer for event log \"%s\" to %u bytes on initial read"), &m_fileName[1], bufferSize);
				}
			} while(success);
		}

      if (reopen || (GetLastError() == ERROR_HANDLE_EOF))
      {
         nt = ThreadCreateEx(NotificationThread, 0, &nd);
         NotifyChangeEventLog(hLog, nd.hLogEvent);
			handles[0] = nd.hWakeupEvent;
			handles[1] = stopCondition;
			LogParserTrace(1, _T("LogWatch: Start watching event log \"%s\""), &m_fileName[1]);
			LogParserTrace(7, _T("LogWatch: Process RSS is ") INT64_FMT _T(" bytes"), GetProcessRSS());
			setStatus(LPS_RUNNING);

         while(1)
         {
            if (WaitForMultipleObjects(2, handles, FALSE, 5000) == WAIT_OBJECT_0 + 1)
					break;	// Shutdown condition raised

            do
            {
retry_read:
               success = ReadEventLog(hLog, EVENTLOG_SEQUENTIAL_READ | EVENTLOG_FORWARDS_READ, 0,
                                       buffer, bufferSize, &bytes, &bytesNeeded);
					error = GetLastError();
					if (!success && (error == ERROR_INSUFFICIENT_BUFFER))
					{
						bufferSize = bytesNeeded;
						buffer = (BYTE *)realloc(buffer, bufferSize);
						LogParserTrace(9, _T("LogWatch: Increasing buffer for event log \"%s\" to %u bytes"), &m_fileName[1], bufferSize);
						goto retry_read;
					}
               if (success)
               {
                  for(rec = buffer; rec < buffer + bytes; rec += ((EVENTLOGRECORD *)rec)->Length)
                     parseEvent((EVENTLOGRECORD *)rec);
               }
            } while(success);

				if (error == ERROR_EVENTLOG_FILE_CHANGED)
				{
					LogParserTrace(4, _T("LogWatch: Got ERROR_EVENTLOG_FILE_CHANGED, reopen event log \"%s\""), &m_fileName[1]);
					break;
				}

            if (error != ERROR_HANDLE_EOF)
				{
					LogParserTrace(0, _T("LogWatch: Unable to read event log \"%s\": %s"),
										&m_fileName[1], GetSystemErrorText(GetLastError(), (TCHAR *)buffer, bufferSize / sizeof(TCHAR)));
					setStatus(_T("EVENT LOG READ ERROR"));
				}
         }

			SetEvent(nd.hStopEvent);
			ThreadJoin(nt);
			LogParserTrace(1, _T("LogWatch: Stop watching event log \"%s\""), &m_fileName[1]);
      }
      else
      {
			LogParserTrace(0, _T("LogWatch: Unable to read event log (initial read) \"%s\": %s"),
			               &m_fileName[1], GetSystemErrorText(GetLastError(), (TCHAR *)buffer, bufferSize / sizeof(TCHAR)));
      }

		CloseEventLog(hLog);
		CloseHandle(nd.hLogEvent);
		CloseHandle(nd.hWakeupEvent);
		CloseHandle(nd.hStopEvent);
		
		if (error == ERROR_EVENTLOG_FILE_CHANGED)
		{
			error = 0;
			reopen = TRUE;
			goto reopen_log;
		}
      result = true;
   }
   else
   {
		LogParserTrace(0, _T("LogWatch: Unable to open event log \"%s\": %s"),
		               &m_fileName[1], GetSystemErrorText(GetLastError(), (TCHAR *)buffer, bufferSize / sizeof(TCHAR)));
		setStatus(_T("EVENT LOG OPEN ERROR"));
      result = false;
   }

	free(buffer);
	return result;
}

/**
 * Initialize event log parsers
 */
void InitEventLogParsers()
{
	InitializeCriticalSection(&m_csEventSourceAccess);
}

/**
 * Cleanup event log parsers
 */
void CleanupEventLogParsers()
{
	int i;

	for(i = 0; i < m_numEventSources; i++)
		delete m_eventSourceList[i];
	safe_free(m_eventSourceList);
	DeleteCriticalSection(&m_csEventSourceAccess);
}
