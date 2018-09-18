/* 
** libnetxms - Common NetXMS utility library
** Copyright (C) 2003-2016 Victor Kirhenshtein
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
 * Mutex class constructor
 */
Mutex::Mutex()
{
   m_mutex = MutexCreate();
   m_refCount = new VolatileCounter(1);
}

/**
 * Mutex class copy constructor
 */
Mutex::Mutex(const Mutex& src)
{
   InterlockedIncrement(src.m_refCount);
   m_mutex = src.m_mutex;
   m_refCount = src.m_refCount;
}

/**
 * Mutex destructor
 */
Mutex::~Mutex()
{
   if (InterlockedDecrement(m_refCount) == 0)
   {
      MutexDestroy(m_mutex);
      delete m_refCount;
   }
}

/**
 * Mutex assignment operator
 */
Mutex& Mutex::operator =(const Mutex& src)
{
   if (&src == this)
      return *this;
   if (InterlockedDecrement(m_refCount))
   {
      MutexDestroy(m_mutex);
      delete m_refCount;
   }
   InterlockedIncrement(src.m_refCount);
   m_mutex = src.m_mutex;
   m_refCount = src.m_refCount;
   return *this;
}

/**
 * R/W Lock class constructor
 */
RWLock::RWLock()
{
   m_rwlock = RWLockCreate();
   m_refCount = new VolatileCounter(1);
}

/**
 * R/W Lock class copy constructor
 */
RWLock::RWLock(const RWLock& src)
{
   InterlockedIncrement(src.m_refCount);
   m_rwlock = src.m_rwlock;
   m_refCount = src.m_refCount;
}

/**
 * R/W Lock destructor
 */
RWLock::~RWLock()
{
   if (InterlockedDecrement(m_refCount) == 0)
   {
      RWLockDestroy(m_rwlock);
      delete m_refCount;
   }
}

/**
 * R/W Lock assignment operator
 */
RWLock& RWLock::operator =(const RWLock& src)
{
   if (&src == this)
      return *this;
   if (InterlockedDecrement(m_refCount))
   {
      RWLockDestroy(m_rwlock);
      delete m_refCount;
   }
   InterlockedIncrement(src.m_refCount);
   m_rwlock = src.m_rwlock;
   m_refCount = src.m_refCount;
   return *this;
}

/**
 * Condition class constructor
 */
Condition::Condition(bool broadcast)
{
   m_condition = ConditionCreate(broadcast);
   m_refCount = new VolatileCounter(1);
}

/**
 * Condition class copy constructor
 */
Condition::Condition(const Condition& src)
{
   InterlockedIncrement(src.m_refCount);
   m_condition = src.m_condition;
   m_refCount = src.m_refCount;
}

/**
 * Condition destructor
 */
Condition::~Condition()
{
   if (InterlockedDecrement(m_refCount) == 0)
   {
      ConditionDestroy(m_condition);
      delete m_refCount;
   }
}

/**
 * Condition assignment operator
 */
Condition& Condition::operator =(const Condition& src)
{
   if (&src == this)
      return *this;
   if (InterlockedDecrement(m_refCount))
   {
      ConditionDestroy(m_condition);
      delete m_refCount;
   }
   InterlockedIncrement(src.m_refCount);
   m_condition = src.m_condition;
   m_refCount = src.m_refCount;
   return *this;
}

#if !defined(_WIN32) && !defined(_NETWARE)

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
      sigprocmask(SIG_BLOCK, &signals, NULL);
   else
      pthread_sigmask(SIG_BLOCK, &signals, NULL);
}

/**
 * Start main loop and signal handler for daemon
 */
void LIBNETXMS_EXPORTABLE StartMainLoop(ThreadFunction pfSignalHandler, ThreadFunction pfMain)
{
   THREAD hThread;
   struct utsname un;
   int nModel = 0;

   if (uname(&un) != -1)
   {
      char *ptr;

      ptr = strchr(un.release, '.');
      if (ptr != NULL)
         *ptr = 0;
      if (!stricmp(un.sysname, "FreeBSD") && (atoi(un.release) >= 5))
         nModel = 1;
   }

   if (pfMain != NULL)
   {
      if (nModel == 0)
      {
         hThread = ThreadCreateEx(pfMain, 0, NULL);
         pfSignalHandler(NULL);
         ThreadJoin(hThread);
      }
      else
      {
         hThread = ThreadCreateEx(pfSignalHandler, 0, NULL);
         pfMain(NULL);
         ThreadJoin(hThread);
      }
   }
   else
   {
      if (nModel == 0)
      {
         pfSignalHandler(NULL);
      }
      else
      {
         hThread = ThreadCreateEx(pfSignalHandler, 0, NULL);
         ThreadJoin(hThread);
      }
   }
}

#endif   /* _WIN32 && _NETWARE*/

#ifdef _WIN32
extern HRESULT (*imp_SetThreadDescription)(HANDLE, PCWSTR);

/**
* Set thread name. Thread can be set to INVALID_THREAD_HANDLE to change name of current thread.
*/
void LIBNETXMS_EXPORTABLE ThreadSetName(THREAD thread, const char *name)
{
   if (imp_SetThreadDescription != NULL)
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
