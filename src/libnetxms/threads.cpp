/* 
** libnetxms - Common NetXMS utility library
** Copyright (C) 2003-2024 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation; either version 3 of the License, or
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
** File: threads.cpp
**
**/

#include "libnetxms.h"

/**
 * Default thread stack size
 */
LIBNETXMS_EXPORTABLE_VAR(int g_defaultThreadStackSize) = 1024 * 1024;  // 1MB by default

/**
 * Set default thread stack size
 */
void LIBNETXMS_EXPORTABLE ThreadSetDefaultStackSize(int size)
{
   g_defaultThreadStackSize = size;
}

#ifndef _WIN32

#include <signal.h>
#include <sys/wait.h>

#if HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif

/**
 * Block all signals on a thread
 */
void LIBNETXMS_EXPORTABLE BlockAllSignals(bool processWide, bool allowInterrupt)
{
   sigset_t signals;
   sigemptyset(&signals);
   sigaddset(&signals, SIGTERM);
   if (!allowInterrupt)
      sigaddset(&signals, SIGINT);
   sigaddset(&signals, SIGSEGV);
   sigaddset(&signals, SIGCHLD);
   sigaddset(&signals, SIGHUP);
   sigaddset(&signals, SIGUSR1);
   sigaddset(&signals, SIGUSR2);
#if !defined(__sun) && !defined(_AIX) && !defined(__hpux)
   sigaddset(&signals, SIGPIPE);
#endif
   if (processWide)
   {
      sigprocmask(SIG_BLOCK, &signals, nullptr);
   }
   else
   {
#ifdef _USE_GNU_PTH
      pth_sigmask(SIG_BLOCK, &signals, nullptr);
#else
      pthread_sigmask(SIG_BLOCK, &signals, nullptr);
#endif
   }
}

/**
 * Start main loop and signal handler for daemon
 */
void LIBNETXMS_EXPORTABLE StartMainLoop(ThreadFunction pfSignalHandler, ThreadFunction pfMain)
{
   int model = 0;

   struct utsname un;
   if (uname(&un) != -1)
   {
      char *ptr = strchr(un.release, '.');
      if (ptr != nullptr)
         *ptr = 0;
      if (!stricmp(un.sysname, "FreeBSD") && (atoi(un.release) >= 5))
         model = 1;
   }

   if (pfMain != nullptr)
   {
      if (model == 0)
      {
         THREAD hThread = ThreadCreateEx(pfMain, 0, nullptr);
         pfSignalHandler(nullptr);
         ThreadJoin(hThread);
      }
      else
      {
         THREAD hThread = ThreadCreateEx(pfSignalHandler, 0, nullptr);
         pfMain(nullptr);
         ThreadJoin(hThread);
      }
   }
   else
   {
      if (model == 0)
      {
         pfSignalHandler(nullptr);
      }
      else
      {
         THREAD hThread = ThreadCreateEx(pfSignalHandler, 0, nullptr);
         ThreadJoin(hThread);
      }
   }
}

#endif   /* _WIN32 */

#ifdef _WIN32
extern HRESULT (WINAPI *imp_SetThreadDescription)(HANDLE, PCWSTR);

/**
* Set thread name. Thread can be set to INVALID_THREAD_HANDLE to change name of current thread.
*/
void LIBNETXMS_EXPORTABLE ThreadSetName(THREAD thread, const char *name)
{
   if (imp_SetThreadDescription != nullptr)
   {
      WCHAR wname[256];
      MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, name, -1, wname, 256);
      imp_SetThreadDescription((thread != INVALID_THREAD_HANDLE) ? thread->handle : GetCurrentThread(), wname);
   }
   else
   {
      THREADNAME_INFO info;
      info.dwType = 0x1000;
      info.szName = name;
      info.dwThreadID = (thread != INVALID_THREAD_HANDLE) ? thread->id : (DWORD)-1;
      info.dwFlags = 0;
#pragma warning(push)
#pragma warning(disable: 6320 6322)
      __try
      {
         RaiseException(0x406D1388, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
      }
      __except (EXCEPTION_EXECUTE_HANDLER)
      {
      }
#pragma warning(pop)
   }
}

#endif   /* _WIN32 */
