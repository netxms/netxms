/* 
** Project X - Network Management System
** Copyright (C) 2003 Victor Kirhenshtein
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
** $module: debug.cpp
**
**/

#include "nxcore.h"


//
// Test mutex state and print to stdout
//

void DbgTestMutex(MUTEX hMutex, TCHAR *szName)
{
   _tprintf(_T("  %s: "), szName);
   if (MutexLock(hMutex, 100))
   {
      _tprintf(_T("unlocked\n"));
      MutexUnlock(hMutex);
   }
   else
   {
      _tprintf(_T("locked\n"));
   }
}


//
// Test read/write lock state and print to stdout
//

void DbgTestRWLock(RWLOCK hLock, TCHAR *szName)
{
   _tprintf(_T("  %s: "), szName);
   if (RWLockWriteLock(hLock, 100))
   {
      _tprintf(_T("unlocked\n"));
      RWLockUnlock(hLock);
   }
   else
   {
      if (RWLockReadLock(hLock, 100))
      {
         _tprintf(_T("locked for reading\n"));
         RWLockUnlock(hLock);
      }
      else
      {
         _tprintf(_T("locked for writing\n"));
      }
   }
}


//
// Debug printf - write text to stdout if in standalone mode
// and specific application flag(s) is set
//

void DbgPrintf(DWORD dwFlags, TCHAR *szFormat, ...)
{
   va_list args;
   TCHAR szBuffer[1024];

   if (!(g_dwFlags & dwFlags))
      return;     // Required application flag(s) not set

   va_start(args, szFormat);
   _vsntprintf(szBuffer, 1024, szFormat, args);
   va_end(args);
   WriteLog(MSG_DEBUG, EVENTLOG_INFORMATION_TYPE, _T("s"), szBuffer);
}
