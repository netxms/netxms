/* 
** NetXMS - Network Management System
** Log Parsing Library
** Copyright (C) 2003-2016 Victor Kirhenshtein
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
 * Init log parser library
 */
void LIBNXLP_EXPORTABLE InitLogParserLibrary()
{
   if (InterlockedIncrement(&s_referenceCount) > 1)
      return;  // already initialized

#ifdef _WIN32
   InitVSSWrapper();
#endif
}

/**
 * Cleanup event log parsig library
 */
void LIBNXLP_EXPORTABLE CleanupLogParserLibrary()
{
   if (InterlockedDecrement(&s_referenceCount) > 0)
      return;  // still referenced
}

#ifdef _WIN32

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
   else
   {
      TCHAR buffer[1024];
      nxlog_debug_tag(DEBUG_TAG, 5, _T("LogParser::saveLastProcessedRecordTimestamp: cannot create registry key (%s)"),
         GetSystemErrorText(GetLastError(), buffer, 1024));
   }
}

/**
 * Read timestamp of last processed record
 */
time_t LogParser::readLastProcessedRecordTimestamp()
{
   if (m_marker == NULL)
      return time(NULL);

   time_t now = time(NULL);
   time_t result;
   HKEY hKey;
   if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\NetXMS\\LogParserLibrary"), 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
   {
      DWORD t;
      DWORD size = sizeof(DWORD);
      if (RegQueryValueEx(hKey, m_marker, NULL, NULL, (BYTE *)&t, &size) == ERROR_SUCCESS)
      {
         result = (time_t)t;
         if (result < now - 86400) // do not read records older than 24 hours
            result = now - 86400;
      }
      else
      {
         result = now;
      }
      RegCloseKey(hKey);
   }
   else
   {
      result = now;
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
