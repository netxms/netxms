/* 
** NetXMS - Network Management System
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
** $module: nms_threads.h
**
**/

#ifndef _nms_threads_h_
#define _nms_threads_h_

#ifdef _WIN32

#include <process.h>

//
// Related datatypes and constants
//

#define MUTEX  HANDLE
#define THREAD HANDLE
#define CONDITION HANDLE

#define INVALID_MUTEX_HANDLE  INVALID_HANDLE_VALUE


//
// Inline functions
//

inline void ThreadSleep(int iSeconds)
{
   Sleep((DWORD)iSeconds * 1000);   // Convert to milliseconds
}

inline void ThreadSleepMs(DWORD dwMilliseconds)
{
   Sleep(dwMilliseconds);
}

inline THREAD ThreadCreate(void (__cdecl *start_address )(void *), int stack_size, void *args)
{
   return (THREAD)_beginthread(start_address, stack_size, args);
}

inline void ThreadExit(void)
{
   _endthread();
}

inline MUTEX MutexCreate(void)
{
   return CreateMutex(NULL, FALSE, NULL);
}

inline void MutexDestroy(MUTEX mutex)
{
   CloseHandle(mutex);
}

inline BOOL MutexLock(MUTEX mutex, DWORD dwTimeOut)
{
   return WaitForSingleObject(mutex, dwTimeOut) == WAIT_OBJECT_0;
}

inline void MutexUnlock(MUTEX mutex)
{
   ReleaseMutex(mutex);
}

inline CONDITION ConditionCreate(void)
{
   return CreateEvent(NULL, FALSE, FALSE, NULL);
}

inline void ConditionDestroy(CONDITION hCond)
{
   CloseHandle(hCond);
}

inline void ConditionSet(CONDITION hCond)
{
   SetEvent(hCond);
}

inline BOOL ConditionWait(CONDITION hCond, DWORD dwTimeOut)
{
   return WaitForSingleObject(hCond, dwTimeOut) == WAIT_OBJECT_0;
}

#else    /* _WIN32 */

#include <pthread.h>

//
// Related datatypes and constants
//

typedef MUTEX * pthread_mutex_t;
#define THREAD HANDLE

#define INVALID_MUTEX_HANDLE  NULL


//
// Inline functions
//

inline void ThreadSleep(int iSeconds)
{
   sleep(iSeconds);
}

inline void ThreadSleepMs(DWORD dwMilliseconds)
{
   usleep(dwMilliseconds * 1000);   // Convert to microseconds
}

inline MUTEX MutexCreate(void)
{
   MUTEX mutex;

   mutex = (MUTEX)malloc(sizeof(pthread_mutex_t));
   if (mutex != NULL)
      pthread_mutex_init(mutex, NULL);
   return mutex;
}

inline void MutexDestroy(MUTEX mutex)
{
   if (mutex != NULL)
   {
      pthread_mutex_destroy(mutex);
      free(mutex);
   }
}

inline void MutexLock(MUTEX mutex, DWORD dwTimeOut)
{
}

inline void MutexUnlock(MUTEX mutex)
{
   if (mutex != NULL)
      pthread_mutex_unlock(mutex);
}

#endif   /* _WIN32 */

#endif   /* _nms_threads_h_ */
