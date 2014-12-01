/* 
** NetXMS - Network Management System
** Log Parsing Library
** Copyright (C) 2003-2014 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
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
**
** File: main.cpp
**
**/

#include "libnxlp.h"

/**
 * Reference count
 */
static VolatileCounter s_referenceCount = 0;

/**
 * Trace callback
 */
void (* s_traceCallback)(int, const TCHAR *, va_list) = NULL;

/**
 * Event log reading mode
 */
#ifdef _WIN32
bool s_eventLogV6 = false;
#endif

/**
 * Set trace callback for log parser library
 */
void LIBNXLP_EXPORTABLE SetLogParserTraceCallback(void (* traceCallback)(int, const TCHAR *, va_list))
{
   s_traceCallback = traceCallback;
}

/**
 * Trace messages for log parser
 */
void LogParserTrace(int level, const TCHAR *format, ...)
{
   if (s_traceCallback == NULL)
      return;

   va_list args;
   va_start(args, format);
   s_traceCallback(level, format, args);
   va_end(args);
}

/**
 * Init log parser library
 */
void LIBNXLP_EXPORTABLE InitLogParserLibrary()
{
   if (InterlockedIncrement(&s_referenceCount) > 1)
      return;  // already initialized

#ifdef _WIN32
	if (InitEventLogParsersV6())
	{
		s_eventLogV6 = true;
	}
	else
	{
		s_eventLogV6 = false;
		InitEventLogParsers();
	}
#endif
}

/**
 * Cleanup event log parsig library
 */
void LIBNXLP_EXPORTABLE CleanupLogParserLibrary()
{
   if (InterlockedDecrement(&s_referenceCount) > 0)
      return;  // still referenced

#ifdef _WIN32
   if (!s_eventLogV6)
   {
      CleanupEventLogParsers();
   }
#endif
}

#ifdef _WIN32

/**
 * Event log parsing wrapper. Calls appropriate implementation.
 */
bool LogParser::monitorEventLog(CONDITION stopCondition, const TCHAR *markerPrefix)
{
   if (markerPrefix != NULL)
   {
      size_t len = _tcslen(markerPrefix) + _tcslen(m_fileName) + 2;
      m_marker = (TCHAR *)malloc(len * sizeof(TCHAR));
      _sntprintf(m_marker, len, _T("%s.%s"), markerPrefix, &m_fileName[1]);
   }
   return s_eventLogV6 ? monitorEventLogV6(stopCondition) : monitorEventLogV4(stopCondition);
}

/**
 * Save timestamp of last processed record
 */
void LogParser::saveLastProcessedRecordTimestamp(time_t timestamp)
{
   if (m_marker == NULL)
      return;

   HKEY hKey;
   if (RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\NetXMS\\LogParserLibrary"), 0, NULL, 
                      REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL) == ERROR_SUCCESS)
   {
      DWORD t = (DWORD)timestamp;
      RegSetValueEx(hKey, m_marker, 0, REG_DWORD, (BYTE *)&t, sizeof(DWORD));
      RegCloseKey(hKey);
   }
}

/**
 * Read timestamp of last processed record
 */
time_t LogParser::readLastProcessedRecordTimestamp()
{
   if (m_marker == NULL)
      return time(NULL);

   time_t result;
   HKEY hKey;
   if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\NetXMS\\LogParserLibrary"), 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
   {
      DWORD t;
      DWORD size = sizeof(DWORD);
      RegQueryValueEx(hKey, m_marker, NULL, NULL, (BYTE *)&t, &size);
      RegCloseKey(hKey);
      result = (time_t)t;
   }
   else
   {
      result = time(NULL);
   }
   return result;
}

/**
 * DLL entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
      DisableThreadLibraryCalls(hInstance);
   return TRUE;
}

#endif   /* _WIN32 */
