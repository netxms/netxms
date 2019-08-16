/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2019 Victor Kirhenshtein
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
** File: nms_threads.h
**
**/

#ifndef _nms_threads_h_
#define _nms_threads_h_

#ifdef __cplusplus

#define NMS_THREADS_H_INCLUDED

#if defined(_WIN32)

#ifndef UNDER_CE
#include <process.h>
#endif

#include "winmutex.h"

//
// Related datatypes and constants
//

#define INVALID_MUTEX_HANDLE        (NULL)
#define INVALID_CONDITION_HANDLE    INVALID_HANDLE_VALUE
#define INVALID_THREAD_HANDLE       (NULL)

#ifdef UNDER_CE
typedef UINT32 THREAD_RESULT;
typedef UINT32 THREAD_ID;
#else
typedef unsigned int THREAD_RESULT;
typedef unsigned int THREAD_ID;
#endif

#define THREAD_OK       0

#ifdef UNDER_CE
#define THREAD_CALL
#else
#define THREAD_CALL     __stdcall

extern "C" typedef THREAD_RESULT (THREAD_CALL *ThreadFunction)(void *);

typedef win_mutex_t *MUTEX;
typedef HANDLE CONDITION;
struct netxms_thread_t
{
	HANDLE handle;
	THREAD_ID id;
};
typedef struct netxms_thread_t *THREAD;

typedef struct
{
	ThreadFunction start_address;
	void *args;
} THREAD_START_DATA;

THREAD_RESULT LIBNETXMS_EXPORTABLE THREAD_CALL SEHThreadStarter(void *);
int LIBNETXMS_EXPORTABLE ___ExceptionHandler(EXCEPTION_POINTERS *pInfo);

void LIBNETXMS_EXPORTABLE SetExceptionHandler(BOOL (*pfHandler)(EXCEPTION_POINTERS *),
															 void (*pfWriter)(const TCHAR *), const TCHAR *pszDumpDir,
															 const TCHAR *pszBaseProcessName,
															 BOOL writeFullDump, BOOL printToScreen);
BOOL LIBNETXMS_EXPORTABLE SEHDefaultConsoleHandler(EXCEPTION_POINTERS *pInfo);
TCHAR LIBNETXMS_EXPORTABLE *SEHExceptionName(DWORD code);
void LIBNETXMS_EXPORTABLE SEHShowCallStack(CONTEXT *pCtx);

void LIBNETXMS_EXPORTABLE SEHServiceExceptionDataWriter(const TCHAR *pszText);
BOOL LIBNETXMS_EXPORTABLE SEHServiceExceptionHandler(EXCEPTION_POINTERS *pInfo);

#define LIBNETXMS_EXCEPTION_HANDLER \
	} __except(___ExceptionHandler((EXCEPTION_POINTERS *)_exception_info())) { ExitProcess(99); }

#endif


//
// Inline functions
//

inline void InitThreadLibrary()
{
}

inline void ThreadSleep(int iSeconds)
{
   Sleep((UINT32)iSeconds * 1000);   // Convert to milliseconds
}

inline void ThreadSleepMs(UINT32 dwMilliseconds)
{
   Sleep(dwMilliseconds);
}

inline bool ThreadCreate(ThreadFunction start_address, int stack_size, void *args)
{
   HANDLE hThread;
   THREAD_ID dwThreadId;

#ifdef UNDER_CE
	hThread = CreateThread(NULL, (UINT32)stack_size, start_address, args, 0, &dwThreadId);
#else
	THREAD_START_DATA *data = (THREAD_START_DATA *)MemAlloc(sizeof(THREAD_START_DATA));
	data->start_address = start_address;
	data->args = args;
   hThread = (HANDLE)_beginthreadex(NULL, stack_size, SEHThreadStarter, data, 0, &dwThreadId);
#endif
   if (hThread != NULL)
      CloseHandle(hThread);
   return (hThread != NULL);
}

inline THREAD ThreadCreateEx(ThreadFunction start_address, int stack_size, void *args)
{
   THREAD thread;

	thread = (THREAD)MemAlloc(sizeof(struct netxms_thread_t));
#ifdef UNDER_CE
	thread->handle = CreateThread(NULL, (UINT32)stack_size, start_address, args, 0, &thread->id);
	if (thread->handle == NULL)
	{
#else
	THREAD_START_DATA *data = (THREAD_START_DATA *)MemAlloc(sizeof(THREAD_START_DATA));
	data->start_address = start_address;
	data->args = args;
   thread->handle = (HANDLE)_beginthreadex(NULL, stack_size, SEHThreadStarter, data, 0, &thread->id);
	if ((thread->handle == (HANDLE)-1) || (thread->handle == 0))
	{
		MemFree(data);
#endif
		MemFree(thread);
		thread = INVALID_THREAD_HANDLE;
	}
	return thread;
}

#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
   DWORD dwType; // Must be 0x1000.
   LPCSTR szName; // Pointer to name (in user addr space).
   DWORD dwThreadID; // Thread ID (-1=caller thread).
   DWORD dwFlags; // Reserved for future use, must be zero.
 } THREADNAME_INFO;
#pragma pack(pop)

/**
 * Set thread name. Thread can be set to INVALID_THREAD_HANDLE to change name of current thread.
 */
void LIBNETXMS_EXPORTABLE ThreadSetName(THREAD thread, const char *name);

/**
 * Set name for current thread
 */
inline void ThreadSetName(const char *name)
{
   ThreadSetName(INVALID_THREAD_HANDLE, name);
}

/**
 * Exit thread
 */
inline void ThreadExit()
{
#ifdef UNDER_CE
   ExitThread(0);
#else
   _endthreadex(0);
#endif
}

inline bool ThreadJoin(THREAD thread)
{
   if (thread == INVALID_THREAD_HANDLE)
      return false;

   bool success = (WaitForSingleObject(thread->handle, INFINITE) == WAIT_OBJECT_0);
   CloseHandle(thread->handle);
   MemFree(thread);
   return success;
}

inline void ThreadDetach(THREAD thread)
{
   if (thread != INVALID_THREAD_HANDLE)
   {
      CloseHandle(thread->handle);
      MemFree(thread);
   }
}

inline THREAD_ID ThreadId(THREAD thread)
{
   return (thread != INVALID_THREAD_HANDLE) ? thread->id : 0;
}

inline MUTEX MutexCreate()
{
	MUTEX mutex = (MUTEX)MemAlloc(sizeof(win_mutex_t));
	InitializeMutex(mutex, 4000);
   return mutex;
}

inline MUTEX MutexCreateRecursive()
{
   return MutexCreate();
}

inline void MutexDestroy(MUTEX mutex)
{
	DestroyMutex(mutex);
   MemFree(mutex);
}

inline bool MutexLock(MUTEX mutex)
{
	if (mutex == INVALID_MUTEX_HANDLE)
		return false;
	LockMutex(mutex, INFINITE);
   return true;
}

inline bool MutexTryLock(MUTEX mutex)
{
	if (mutex == INVALID_MUTEX_HANDLE)
		return false;
	return TryLockMutex(mutex);
}

inline bool MutexTimedLock(MUTEX mutex, UINT32 timeout)
{
   if (mutex == INVALID_MUTEX_HANDLE)
      return false;
   return LockMutex(mutex, timeout);
}

inline void MutexUnlock(MUTEX mutex)
{
   UnlockMutex(mutex);
}

inline CONDITION ConditionCreate(bool bBroadcast)
{
   return CreateEvent(NULL, bBroadcast, FALSE, NULL);
}

inline void ConditionDestroy(CONDITION hCond)
{
   CloseHandle(hCond);
}

inline void ConditionSet(CONDITION hCond)
{
   SetEvent(hCond);
}

inline void ConditionReset(CONDITION hCond)
{
   ResetEvent(hCond);
}

inline void ConditionPulse(CONDITION hCond)
{
   PulseEvent(hCond);
}

inline bool ConditionWait(CONDITION hCond, UINT32 dwTimeOut)
{
	if (hCond == INVALID_CONDITION_HANDLE)
		return false;
   return WaitForSingleObject(hCond, dwTimeOut) == WAIT_OBJECT_0;
}

#elif defined(_USE_GNU_PTH)

/****************************************************************************/
/* GNU Pth                                                                  */
/****************************************************************************/

//
// Related datatypes and constants
//

typedef pth_t THREAD;
typedef pth_mutex_t * MUTEX;
struct netxms_condition_t
{
	pth_cond_t cond;
	pth_mutex_t mutex;
	bool broadcast;
   bool isSet;
};
typedef struct netxms_condition_t * CONDITION;

#define INVALID_MUTEX_HANDLE        (NULL)
#define INVALID_CONDITION_HANDLE    (NULL)
#define INVALID_THREAD_HANDLE       (NULL)

#ifndef INFINITE
#define INFINITE 0xFFFFFFFF
#endif

typedef void *THREAD_RESULT;

#define THREAD_OK       ((void *)0)
#define THREAD_CALL

extern "C" typedef THREAD_RESULT (THREAD_CALL *ThreadFunction)(void *);


//
// Inline functions
//

inline void InitThreadLibrary()
{
   if (!pth_init())
   {
      perror("pth_init() failed");
      exit(200);
   }
}

inline void ThreadSleep(int nSeconds)
{
   pth_sleep(nSeconds);
}

inline void ThreadSleepMs(UINT32 dwMilliseconds)
{
	pth_usleep(dwMilliseconds * 1000);
}

inline bool ThreadCreate(ThreadFunction start_address, int stack_size, void *args)
{
	THREAD id;

	if ((id = pth_spawn(PTH_ATTR_DEFAULT, start_address, args)) != NULL) 
   {
      pth_attr_set(pth_attr_of(id), PTH_ATTR_JOINABLE, 0);
		return true;
	} 
   else 
   {
		return false;
	}
}

inline THREAD ThreadCreateEx(ThreadFunction start_address, int stack_size, void *args)
{
	THREAD id;

	if ((id = pth_spawn(PTH_ATTR_DEFAULT, start_address, args)) != NULL) 
   {
		return id;
	} 
   else 
   {
		return INVALID_THREAD_HANDLE;
	}
}

/**
 * Set thread name
 */
inline void ThreadSetName(THREAD thread, const char *name)
{
}

/**
 * Set name for current thread
 */
inline void ThreadSetName(const char *name)
{
   ThreadSetName(INVALID_THREAD_HANDLE, name);
}

/**
 * Exit thread
 */
inline void ThreadExit(void)
{
   pth_exit(NULL);
}

inline bool ThreadJoin(THREAD hThread)
{
   if (hThread == INVALID_THREAD_HANDLE)
      return false;
   return pth_join(hThread, NULL) != -1;
}

inline void ThreadDetach(THREAD hThread)
{
   if (hThread != INVALID_THREAD_HANDLE)
      pth_detach(hThread);
}

inline MUTEX MutexCreate(void)
{
   MUTEX mutex;

   mutex = (MUTEX)MemAlloc(sizeof(pth_mutex_t));
   if (mutex != NULL)
   {
      pth_mutex_init(mutex);
   }
   return mutex;
}

inline MUTEX MutexCreateRecursive()
{
   MUTEX mutex;

   // In libpth, recursive locking is explicitly supported,
   // so we just create mutex
   mutex = (MUTEX)MemAlloc(sizeof(pth_mutex_t));
   if (mutex != NULL)
   {
      pth_mutex_init(mutex);
   }
   return mutex;
}

inline void MutexDestroy(MUTEX mutex)
{
   if (mutex != NULL)
      MemFree(mutex);
}

inline bool MutexLock(MUTEX mutex)
{
   return (mutex != NULL) ? (pth_mutex_acquire(mutex, FALSE, NULL) != 0) : false;
}

inline bool MutexTryLock(MUTEX mutex)
{
   return (mutex != NULL) ? (pth_mutex_acquire(mutex, TRUE, NULL) != 0) : false;
}

inline bool MutexTimedLock(MUTEX mutex, UINT32 timeout)
{
   if (mutex == NULL)
      return false;

   if (timeout == 0)
      return pth_mutex_acquire(mutex, TRUE, NULL) != 0;

   if (timeout == INFINITE)
      return pth_mutex_acquire(mutex, FALSE, NULL) != 0;

   pth_event_t ev = pth_event(PTH_EVENT_TIME, pth_timeout(timeout / 1000, (timeout % 1000) * 1000));
   int retcode = pth_mutex_acquire(mutex, FALSE, ev);
   pth_event_free(ev, PTH_FREE_ALL);
   return retcode != 0;
}

inline void MutexUnlock(MUTEX mutex)
{
   if (mutex != NULL) 
      pth_mutex_release(mutex);
}

inline CONDITION ConditionCreate(bool bBroadcast)
{
	CONDITION cond;

	cond = (CONDITION)MemAlloc(sizeof(struct netxms_condition_t));
	if (cond != NULL) 
	{
		pth_cond_init(&cond->cond);
		pth_mutex_init(&cond->mutex);
		cond->broadcast = bBroadcast;
		cond->isSet = FALSE;
	}

	return cond;
}

inline void ConditionDestroy(CONDITION cond)
{
	if (cond != INVALID_CONDITION_HANDLE)
	{
		MemFree(cond);
	}
}

inline void ConditionSet(CONDITION cond)
{
	if (cond != INVALID_CONDITION_HANDLE)
	{
		pth_mutex_acquire(&cond->mutex, FALSE, NULL);
      cond->isSet = TRUE;
      pth_cond_notify(&cond->cond, cond->broadcast);
		pth_mutex_release(&cond->mutex);
	}
}

inline void ConditionReset(CONDITION cond)
{
	if (cond != INVALID_CONDITION_HANDLE)
	{
		pth_mutex_acquire(&cond->mutex, FALSE, NULL);
      cond->isSet = FALSE;
		pth_mutex_release(&cond->mutex);
	}
}

inline void ConditionPulse(CONDITION cond)
{
	if (cond != INVALID_CONDITION_HANDLE)
	{
		pth_mutex_acquire(&cond->mutex, FALSE, NULL);
      pth_cond_notify(&cond->cond, cond->broadcast);
      cond->isSet = FALSE;
		pth_mutex_release(&cond->mutex);
	}
}

inline bool ConditionWait(CONDITION cond, UINT32 dwTimeOut)
{
	bool ret = false;

	if (cond != NULL)
	{
		int retcode;

		pth_mutex_acquire(&cond->mutex, FALSE, NULL);
      if (cond->isSet)
      {
         ret = true;
         if (!cond->broadcast)
            cond->isSet = FALSE;
      }
      else
      {
		   if (dwTimeOut != INFINITE)
		   {
            pth_event_t ev = pth_event(PTH_EVENT_TIME, pth_timeout(dwTimeOut / 1000, (dwTimeOut % 1000) * 1000));
			   retcode = pth_cond_await(&cond->cond, &cond->mutex, ev);
            pth_event_free(ev, PTH_FREE_ALL);
		   }
		   else
		   {
			   retcode = pth_cond_await(&cond->cond, &cond->mutex, NULL);
		   }

		   if (retcode)
		   {
            if (!cond->broadcast)
               cond->isSet = FALSE;
			   ret = true;
		   }
      }

		pth_mutex_release(&cond->mutex);
	}

	return ret;
}

inline UINT32 GetCurrentProcessId()
{
   return getpid();
}

inline UINT32 GetCurrentThreadId()
{
   return (UINT32)pth_self();
}

#else    /* not _WIN32 && not _USE_GNU_PTH */

/****************************************************************************/
/* pthreads                                                                 */
/****************************************************************************/

#include <pthread.h>
#include <errno.h>
#include <sys/time.h>

#if HAVE_SYS_SYSCALL_H
#include <sys/syscall.h>
#endif

#if HAVE_PTHREAD_NP_H && !defined(_IPSO)
#include <pthread_np.h>
#endif

#if (HAVE_PTHREAD_MUTEXATTR_SETTYPE || HAVE___PTHREAD_MUTEXATTR_SETTYPE || HAVE_PTHREAD_MUTEXATTR_SETKIND_NP) && \
    HAVE_DECL_PTHREAD_MUTEXATTR_SETTYPE && \
    (HAVE_DECL_PTHREAD_MUTEX_RECURSIVE || \
	  HAVE_DECL_PTHREAD_MUTEX_RECURSIVE_NP || \
          HAVE_DECL_MUTEX_TYPE_COUNTING_FAST)

#define HAVE_RECURSIVE_MUTEXES 1

#if HAVE_DECL_PTHREAD_MUTEX_RECURSIVE
#define MUTEX_RECURSIVE_FLAG PTHREAD_MUTEX_RECURSIVE
#elif HAVE_DECL_PTHREAD_MUTEX_RECURSIVE_NP
#define MUTEX_RECURSIVE_FLAG PTHREAD_MUTEX_RECURSIVE_NP
#elif HAVE_DECL_MUTEX_TYPE_COUNTING_FAST
#define MUTEX_RECURSIVE_FLAG MUTEX_TYPE_COUNTING_FAST
#else
#error Constant used to declare recursive mutex is not known 
#endif

#if HAVE_PTHREAD_MUTEXATTR_SETTYPE || HAVE_DECL_PTHREAD_MUTEXATTR_SETTYPE
#define MUTEXATTR_SETTYPE pthread_mutexattr_settype
#elif HAVE___PTHREAD_MUTEXATTR_SETTYPE
#define MUTEXATTR_SETTYPE __pthread_mutexattr_settype
#else
#define MUTEXATTR_SETTYPE pthread_mutexattr_setkind_np
#endif

#endif

//
// Related datatypes and constants
//

typedef pthread_t THREAD;
struct netxms_mutex_t
{
   pthread_mutex_t mutex;
#ifndef HAVE_RECURSIVE_MUTEXES
   bool isRecursive;
   pthread_t owner;
#endif
};
typedef netxms_mutex_t * MUTEX;
struct netxms_condition_t
{
	pthread_cond_t cond;
	pthread_mutex_t mutex;
	bool broadcast;
   bool isSet;
};
typedef struct netxms_condition_t * CONDITION;

#define INVALID_MUTEX_HANDLE        (NULL)
#define INVALID_CONDITION_HANDLE    (NULL)
#define INVALID_THREAD_HANDLE       0

#ifndef INFINITE
#define INFINITE 0xFFFFFFFF
#endif

typedef void *THREAD_RESULT;

#define THREAD_OK       ((void *)0)
#define THREAD_CALL

extern "C" typedef THREAD_RESULT (THREAD_CALL *ThreadFunction)(void *);


//
// Inline functions
//

inline void InitThreadLibrary()
{
}

inline void ThreadSleep(int nSeconds)
{
#ifdef _NETWARE
	sleep(nSeconds);
#else
	struct timeval tv;

	tv.tv_sec = nSeconds;
	tv.tv_usec = 0;

	select(1, NULL, NULL, NULL, &tv);
#endif
}

inline void ThreadSleepMs(UINT32 dwMilliseconds)
{
#if HAVE_NANOSLEEP && HAVE_DECL_NANOSLEEP
	struct timespec interval, remainder;

	interval.tv_sec = dwMilliseconds / 1000;
	interval.tv_nsec = (dwMilliseconds % 1000) * 1000000; // milli -> nano
	nanosleep(&interval, &remainder);
#else
	usleep(dwMilliseconds * 1000);   // Convert to microseconds
#endif
}

inline THREAD ThreadCreateEx(ThreadFunction start_address, int stack_size, void *args)
{
   THREAD id;

	if (stack_size <= 0)
	{
		// TODO: Find out minimal stack size
		stack_size = 1024 * 1024; // 1MB
		// set stack size to 1mb (it's windows default - and application works,
		// we need to investigate more on this)
	}
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, stack_size);

   if (pthread_create(&id, &attr, start_address, args) != 0)
   {
		id = INVALID_THREAD_HANDLE;
   } 

	pthread_attr_destroy(&attr);

	return id;
}

inline bool ThreadCreate(ThreadFunction start_address, int stack_size, void *args)
{
	THREAD id = ThreadCreateEx(start_address, stack_size, args);
	if (id != INVALID_THREAD_HANDLE)
	{
		pthread_detach(id);
		return true;
	}
	return false;
}

/**
 * Set thread name
 */
inline void ThreadSetName(THREAD thread, const char *name)
{
#if HAVE_PTHREAD_SETNAME_NP
#if PTHREAD_SETNAME_NP_2ARGS
   pthread_setname_np((thread != INVALID_THREAD_HANDLE) ? thread : pthread_self(), name);
#else
   if ((thread == INVALID_THREAD_HANDLE) || (thread == pthread_self()))
      pthread_setname_np(name);
#endif
#endif
}

/**
 * Set name for current thread
 */
inline void ThreadSetName(const char *name)
{
   ThreadSetName(INVALID_THREAD_HANDLE, name);
}

/**
 * Exit thread
 */
inline void ThreadExit()
{
   pthread_exit(NULL);
}

inline bool ThreadJoin(THREAD hThread)
{
   if (hThread == INVALID_THREAD_HANDLE)
      return false;
   return pthread_join(hThread, NULL) == 0;
}

inline void ThreadDetach(THREAD hThread)
{
   if (hThread != INVALID_THREAD_HANDLE)
      pthread_detach(hThread);
}

inline MUTEX MutexCreate()
{
   MUTEX mutex;

   mutex = (MUTEX)MemAlloc(sizeof(netxms_mutex_t));
   if (mutex != NULL)
   {
      pthread_mutex_init(&mutex->mutex, NULL);
#ifndef HAVE_RECURSIVE_MUTEXES
      mutex->isRecursive = FALSE;
#endif
   }
   return mutex;
}

inline MUTEX MutexCreateRecursive()
{
   MUTEX mutex;

   mutex = (MUTEX)MemAlloc(sizeof(netxms_mutex_t));
   if (mutex != NULL)
   {
#ifdef HAVE_RECURSIVE_MUTEXES
      pthread_mutexattr_t a;

      pthread_mutexattr_init(&a);
      MUTEXATTR_SETTYPE(&a, MUTEX_RECURSIVE_FLAG);
      pthread_mutex_init(&mutex->mutex, &a);
      pthread_mutexattr_destroy(&a);
#else
      mutex->isRecursive = TRUE;
#error FIXME: implement recursive mutexes
#endif
   }
   return mutex;
}

inline void MutexDestroy(MUTEX mutex)
{
   if (mutex != NULL)
   {
      pthread_mutex_destroy(&mutex->mutex);
      MemFree(mutex);
   }
}

inline bool MutexLock(MUTEX mutex)
{
   return (mutex != NULL) ? (pthread_mutex_lock(&mutex->mutex) == 0) : false;
}

inline bool MutexTryLock(MUTEX mutex)
{
   return (mutex != NULL) ? (pthread_mutex_trylock(&mutex->mutex) == 0) : false;
}

inline bool MutexTimedLock(MUTEX mutex, UINT32 timeout)
{
   if (mutex == NULL)
      return false;

#if HAVE_PTHREAD_MUTEX_TIMEDLOCK
   struct timeval now;
   struct timespec absTimeout;

   gettimeofday(&now, NULL);
   absTimeout.tv_sec = now.tv_sec + (timeout / 1000);

   now.tv_usec += (timeout % 1000) * 1000;
   absTimeout.tv_sec += now.tv_usec / 1000000;
   absTimeout.tv_nsec = (now.tv_usec % 1000000) * 1000;

   return pthread_mutex_timedlock(&mutex->mutex, &absTimeout) == 0;
#else
   if (pthread_mutex_trylock(&mutex->mutex) == 0)
      return true;

   while(timeout > 0)
   {
      if (timeout >= 10)
      {
         ThreadSleepMs(10);
         timeout -= 10;
      }
      else
      {
         ThreadSleepMs(timeout);
         timeout = 0;
      }

      if (pthread_mutex_trylock(&mutex->mutex) == 0)
         return true;
   } while(timeout > 0);

   return false;
#endif
}

inline void MutexUnlock(MUTEX mutex)
{
   if (mutex != NULL) 
      pthread_mutex_unlock(&mutex->mutex);
}

inline CONDITION ConditionCreate(bool bBroadcast)
{
	CONDITION cond;

   cond = (CONDITION)MemAlloc(sizeof(struct netxms_condition_t));
   if (cond != NULL) 
   {
      pthread_cond_init(&cond->cond, NULL);
      pthread_mutex_init(&cond->mutex, NULL);
		cond->broadcast = bBroadcast;
      cond->isSet = FALSE;
	}

   return cond;
}

inline void ConditionDestroy(CONDITION cond)
{
	if (cond != INVALID_CONDITION_HANDLE)
	{
		pthread_cond_destroy(&cond->cond);
		pthread_mutex_destroy(&cond->mutex);
		MemFree(cond);
	}
}

inline void ConditionSet(CONDITION cond)
{
	if (cond != INVALID_CONDITION_HANDLE)
	{
		pthread_mutex_lock(&cond->mutex);
      cond->isSet = TRUE;
		if (cond->broadcast)
		{
			pthread_cond_broadcast(&cond->cond);
		}
		else
		{
			pthread_cond_signal(&cond->cond);
		}
		pthread_mutex_unlock(&cond->mutex);
	}
}

inline void ConditionReset(CONDITION cond)
{
	if (cond != INVALID_CONDITION_HANDLE)
	{
		pthread_mutex_lock(&cond->mutex);
      cond->isSet = FALSE;
		pthread_mutex_unlock(&cond->mutex);
	}
}

inline void ConditionPulse(CONDITION cond)
{
	if (cond != INVALID_CONDITION_HANDLE)
	{
		pthread_mutex_lock(&cond->mutex);
		if (cond->broadcast)
		{
			pthread_cond_broadcast(&cond->cond);
		}
		else
		{
			pthread_cond_signal(&cond->cond);
		}
      cond->isSet = FALSE;
		pthread_mutex_unlock(&cond->mutex);
	}
}

inline bool ConditionWait(CONDITION cond, UINT32 dwTimeOut)
{
	bool ret = FALSE;

	if (cond != NULL)
	{
		int retcode;

		pthread_mutex_lock(&cond->mutex);
      if (cond->isSet)
      {
         ret = true;
         if (!cond->broadcast)
            cond->isSet = FALSE;
      }
      else
      {
		   if (dwTimeOut != INFINITE)
		   {
#if HAVE_PTHREAD_COND_RELTIMEDWAIT_NP || defined(_NETWARE)
			   struct timespec timeout;

			   timeout.tv_sec = dwTimeOut / 1000;
			   timeout.tv_nsec = (dwTimeOut % 1000) * 1000000;
#ifdef _NETWARE
			   retcode = pthread_cond_timedwait(&cond->cond, &cond->mutex, &timeout);
#else
			   retcode = pthread_cond_reltimedwait_np(&cond->cond, &cond->mutex, &timeout);
#endif
#else
			   struct timeval now;
			   struct timespec timeout;

			   // note.
			   // mili - 10^-3
			   // micro - 10^-6
			   // nano - 10^-9

			   // FIXME there should be more accurate way
			   gettimeofday(&now, NULL);
			   timeout.tv_sec = now.tv_sec + (dwTimeOut / 1000);

			   now.tv_usec += (dwTimeOut % 1000) * 1000;
			   timeout.tv_sec += now.tv_usec / 1000000;
			   timeout.tv_nsec = (now.tv_usec % 1000000) * 1000;

			   retcode = pthread_cond_timedwait(&cond->cond, &cond->mutex, &timeout);
#endif
		   }
		   else
		   {
			   retcode = pthread_cond_wait(&cond->cond, &cond->mutex);
		   }

		   if (retcode == 0)
		   {
            if (!cond->broadcast)
               cond->isSet = FALSE;
			   ret = true;
		   }
      }

		pthread_mutex_unlock(&cond->mutex);
	}

	return ret;
}

/**
 * Get ID of current process
 */
inline UINT32 GetCurrentProcessId()
{
   return getpid();
}

/**
 *  Get ID of current thread
 */
inline UINT32 GetCurrentThreadId()
{
#if HAVE_GETTID_SYSCALL
   return (UINT32)syscall(SYS_gettid);
#elif HAVE_PTHREAD_GETTHREADID_NP
   return (UINT32)pthread_getthreadid_np();
#elif HAVE_PTHREAD_THREADID_NP
   uint64_t id;
   if (pthread_threadid_np(NULL, &id) != 0)
      return 0;
   return (UINT32)id;
#else
   return 0;
#endif
}

#endif   /* _WIN32 */

#include <rwlock.h>

/**
 * String list
 */
class StringList;

/**
 * Thread pool
 */
struct ThreadPool;

/**
 * Thread pool information
 */
struct ThreadPoolInfo
{
   const TCHAR *name;      // pool name
   int minThreads;         // min threads
   int maxThreads;         // max threads
   int curThreads;         // current threads
   int activeRequests;     // number of active requests
   int scheduledRequests;  // number of requests scheduled for execution
   int serializedRequests; // number of requests waiting in serialized execution queues
   UINT64 totalRequests;   // Total requests executed (cumulative count)
   UINT64 threadStarts;    // Number of thread starts
   UINT64 threadStops;     // Number of thread stops
   int usage;              // Pool usage in %
   int load;               // Pool current load in % (can be more than 100% if there are more requests then threads available)
   double loadAvg[3];      // Pool load average
   UINT32 averageWaitTime; // Average task wait time
};

/**
 * Worker function for thread pool
 */
typedef void (* ThreadPoolWorkerFunction)(void *);

/* Thread pool functions */
ThreadPool LIBNETXMS_EXPORTABLE *ThreadPoolCreate(const TCHAR *name, int minThreads, int maxThreads, int stackSize = 0);
void LIBNETXMS_EXPORTABLE ThreadPoolDestroy(ThreadPool *p);
void LIBNETXMS_EXPORTABLE ThreadPoolExecute(ThreadPool *p, ThreadPoolWorkerFunction f, void *arg);
void LIBNETXMS_EXPORTABLE ThreadPoolExecuteSerialized(ThreadPool *p, const TCHAR *key, ThreadPoolWorkerFunction f, void *arg);
void LIBNETXMS_EXPORTABLE ThreadPoolScheduleAbsolute(ThreadPool *p, time_t runTime, ThreadPoolWorkerFunction f, void *arg);
void LIBNETXMS_EXPORTABLE ThreadPoolScheduleAbsoluteMs(ThreadPool *p, INT64 runTime, ThreadPoolWorkerFunction f, void *arg);
void LIBNETXMS_EXPORTABLE ThreadPoolScheduleRelative(ThreadPool *p, UINT32 delay, ThreadPoolWorkerFunction f, void *arg);
void LIBNETXMS_EXPORTABLE ThreadPoolGetInfo(ThreadPool *p, ThreadPoolInfo *info);
bool LIBNETXMS_EXPORTABLE ThreadPoolGetInfo(const TCHAR *name, ThreadPoolInfo *info);
int LIBNETXMS_EXPORTABLE ThreadPoolGetSerializedRequestCount(ThreadPool *p, const TCHAR *key);
UINT32 LIBNETXMS_EXPORTABLE ThreadPoolGetSerializedRequestMaxWaitTime(ThreadPool *p, const TCHAR *key);
StringList LIBNETXMS_EXPORTABLE *ThreadPoolGetAllPools();
void LIBNETXMS_EXPORTABLE ThreadPoolSetResizeParameters(int responsiveness, UINT32 waitTimeHWM, UINT32 waitTimeLWM);

/**
 * Wrapper for ThreadPoolExecute to use pointer to given type as argument
 */
template <typename T> inline void ThreadPoolExecute(ThreadPool *p, void (*f)(T *), T *arg)
{
   ThreadPoolExecute(p, (ThreadPoolWorkerFunction)f, (void *)arg);
}

/**
 * Wrapper data for ThreadPoolExecute with shared_ptr argument
 */
template <typename T> class __ThreadPoolExecute_SharedPtr_WrapperData
{
public:
   shared_ptr<T> m_object;
   void (*m_func)(shared_ptr<T>);

   __ThreadPoolExecute_SharedPtr_WrapperData(shared_ptr<T> object, void (*func)(shared_ptr<T>))
   {
      m_object = object;
      m_func = func;
   }
};

/**
 * Wrapper for ThreadPoolExecute with shared_ptr argument
 */
template <typename T> void __ThreadPoolExecute_SharedPtr_Wrapper(void *arg)
{
   __ThreadPoolExecute_SharedPtr_WrapperData<T> *wd = static_cast<__ThreadPoolExecute_SharedPtr_WrapperData<T> *>(arg);
   wd->m_func(wd->m_object);
   delete wd;
}

/**
 * Wrapper for ThreadPoolExecute to use smart pointer to given type as argument
 */
template <typename T> inline void ThreadPoolExecute(ThreadPool *p, void (*f)(shared_ptr<T>), shared_ptr<T> arg)
{
   ThreadPoolExecute(p, __ThreadPoolExecute_SharedPtr_Wrapper<T>, new __ThreadPoolExecute_SharedPtr_WrapperData<T>(arg, f));
}

/**
 * Wrapper for ThreadPoolExecuteSerialized to use pointer to given type as argument
 */
template <typename T> inline void ThreadPoolExecuteSerialized(ThreadPool *p, const TCHAR *key, void (*f)(T *), T *arg)
{
   ThreadPoolExecuteSerialized(p, key, (ThreadPoolWorkerFunction)f, (void *)arg);
}

/**
 * Wrapper for ThreadPoolExecuteSerialized to use smart pointer to given type as argument
 */
template <typename T> inline void ThreadPoolExecuteSerialized(ThreadPool *p, const TCHAR *key, void (*f)(shared_ptr<T>), shared_ptr<T> arg)
{
   ThreadPoolExecuteSerialized(p, key, __ThreadPoolExecute_SharedPtr_Wrapper<T>, new __ThreadPoolExecute_SharedPtr_WrapperData<T>(arg, f));
}

/**
 * Wrapper for ThreadPoolScheduleAbsolute to use pointer to given type as argument
 */
template <typename T> inline void ThreadPoolScheduleAbsolute(ThreadPool *p, time_t runTime, void (*f)(T *), T *arg)
{
   ThreadPoolScheduleAbsolute(p, runTime, (ThreadPoolWorkerFunction)f, (void *)arg);
}

/**
 * Wrapper for ThreadPoolScheduleAbsoluteMs to use pointer to given type as argument
 */
template <typename T> inline void ThreadPoolScheduleAbsoluteMs(ThreadPool *p, INT64 runTime, void (*f)(T *), T *arg)
{
   ThreadPoolScheduleAbsoluteMs(p, runTime, (ThreadPoolWorkerFunction)f, (void *)arg);
}

/**
 * Wrapper for ThreadPoolScheduleRelative to use pointer to given type as argument
 */
template <typename T> inline void ThreadPoolScheduleRelative(ThreadPool *p, UINT32 delay, void (*f)(T *), T *arg)
{
   ThreadPoolScheduleRelative(p, delay, (ThreadPoolWorkerFunction)f, (void *)arg);
}

/**
 * Wrapper data for ThreadPoolExecute (no arguments)
 */
template <typename T> class __ThreadPoolExecute_WrapperData_0
{
public:
   T *m_object;
   void (T::*m_func)();

   __ThreadPoolExecute_WrapperData_0(T *object, void (T::*func)())
   {
      m_object = object;
      m_func = func;
   }
};

/**
 * Wrapper for ThreadPoolExecute (no arguments)
 */
template <typename T> void __ThreadPoolExecute_Wrapper_0(void *arg)
{
   __ThreadPoolExecute_WrapperData_0<T> *wd = static_cast<__ThreadPoolExecute_WrapperData_0<T> *>(arg);
   ((*wd->m_object).*(wd->m_func))();
   delete wd;
}

/**
 * Execute task as soon as possible (use class member without arguments)
 */
template <typename T, typename B> inline void ThreadPoolExecute(ThreadPool *p, T *object, void (B::*f)())
{
   ThreadPoolExecute(p, __ThreadPoolExecute_Wrapper_0<B>, new __ThreadPoolExecute_WrapperData_0<B>(object, f));
}

/**
 * Execute serialized task as soon as possible (use class member without arguments)
 */
template <typename T, typename B> inline void ThreadPoolExecuteSerialized(ThreadPool *p, const TCHAR *key, T *object, void (B::*f)())
{
   ThreadPoolExecuteSerialized(p, key, __ThreadPoolExecute_Wrapper_0<B>, new __ThreadPoolExecute_WrapperData_0<B>(object, f));
}

/**
 * Execute task with delay (use class member without arguments)
 */
template <typename T, typename B> inline void ThreadPoolScheduleRelative(ThreadPool *p, UINT32 delay, T *object, void (B::*f)())
{
   ThreadPoolScheduleRelative(p, delay, __ThreadPoolExecute_Wrapper_0<B>, new __ThreadPoolExecute_WrapperData_0<B>(object, f));
}

/**
 * Wrapper data for ThreadPoolExecute (one argument)
 */
template <typename T, typename R> class __ThreadPoolExecute_WrapperData_1
{
public:
   T *m_object;
   void (T::*m_func)(R);
   R m_arg;

   __ThreadPoolExecute_WrapperData_1(T *object, void (T::*func)(R), R arg)
   {
      m_object = object;
      m_func = func;
      m_arg = arg;
   }
};

/**
 * Wrapper for ThreadPoolExecute (one argument)
 */
template <typename T, typename R> void __ThreadPoolExecute_Wrapper_1(void *arg)
{
   __ThreadPoolExecute_WrapperData_1<T, R> *wd = static_cast<__ThreadPoolExecute_WrapperData_1<T, R> *>(arg);
   ((*wd->m_object).*(wd->m_func))(wd->m_arg);
   delete wd;
}

/**
 * Execute task as soon as possible (use class member with one argument)
 */
template <typename T, typename B, typename R> inline void ThreadPoolExecute(ThreadPool *p, T *object, void (B::*f)(R), R arg)
{
   ThreadPoolExecute(p, __ThreadPoolExecute_Wrapper_1<B, R>, new __ThreadPoolExecute_WrapperData_1<B, R>(object, f, arg));
}

/**
 * Execute serialized task as soon as possible (use class member with one argument)
 */
template <typename T, typename B, typename R> inline void ThreadPoolExecuteSerialized(ThreadPool *p, const TCHAR *key, T *object, void (B::*f)(R), R arg)
{
   ThreadPoolExecuteSerialized(p, key, __ThreadPoolExecute_Wrapper_1<B, R>, new __ThreadPoolExecute_WrapperData_1<B, R>(object, f, arg));
}

/**
 * Execute task with delay (use class member with one argument)
 */
template <typename T, typename B, typename R> inline void ThreadPoolScheduleRelative(ThreadPool *p, UINT32 delay, T *object, void (B::*f)(R), R arg)
{
   ThreadPoolScheduleRelative(p, delay, __ThreadPoolExecute_Wrapper_1<B, R>, new __ThreadPoolExecute_WrapperData_1<B, R>(object, f, arg));
}

/**
 * Wrapper data for ThreadPoolExecute (two arguments)
 */
template <typename T, typename R1, typename R2> class __ThreadPoolExecute_WrapperData_2
{
public:
   T *m_object;
   void (T::*m_func)(R1, R2);
   R1 m_arg1;
   R2 m_arg2;

   __ThreadPoolExecute_WrapperData_2(T *object, void (T::*func)(R1, R2), R1 arg1, R2 arg2)
   {
      m_object = object;
      m_func = func;
      m_arg1 = arg1;
      m_arg2 = arg2;
   }
};

/**
 * Wrapper for ThreadPoolExecute (two arguments)
 */
template <typename T, typename R1, typename R2> void __ThreadPoolExecute_Wrapper_2(void *arg)
{
   __ThreadPoolExecute_WrapperData_2<T, R1, R2> *wd = static_cast<__ThreadPoolExecute_WrapperData_2<T, R1, R2> *>(arg);
   ((*wd->m_object).*(wd->m_func))(wd->m_arg1, wd->m_arg2);
   delete wd;
}

/**
 * Execute task as soon as possible (use class member with two arguments)
 */
template <typename T, typename B, typename R1, typename R2> inline void ThreadPoolExecute(ThreadPool *p, T *object, void (B::*f)(R1, R2), R1 arg1, R2 arg2)
{
   ThreadPoolExecute(p, __ThreadPoolExecute_Wrapper_2<B, R1, R2>, new __ThreadPoolExecute_WrapperData_2<B, R1, R2>(object, f, arg1, arg2));
}

/**
 * Get value of given attribute protected by given mutex
 */
template<typename T> inline T GetAttributeWithLock(T* attr, MUTEX mutex)
{
   MutexLock(mutex);
   T value = *attr;
   MutexUnlock(mutex);
   return value;
}

/**
 * Set value of given attribute protected by given mutex
 */
template<typename T> inline void SetAttributeWithLock(T* attr, T value, MUTEX mutex)
{
   MutexLock(mutex);
   *attr = value;
   MutexUnlock(mutex);
}

/**
 * Wrappers for mutex
 */
class LIBNETXMS_EXPORTABLE Mutex
{
private:
   MUTEX m_mutex;
   VolatileCounter *m_refCount;

public:
   Mutex();
   Mutex(const Mutex& src);
   ~Mutex();

   Mutex& operator =(const Mutex &src);

   void lock() { MutexLock(m_mutex); }
   bool tryLock() { return MutexTryLock(m_mutex); }
   void unlock() { MutexUnlock(m_mutex); }
};

/**
 * Wrappers for read/write lock
 */
class LIBNETXMS_EXPORTABLE RWLock
{
private:
   RWLOCK m_rwlock;
   VolatileCounter *m_refCount;

public:
   RWLock();
   RWLock(const RWLock& src);
   ~RWLock();

   RWLock& operator =(const RWLock &src);

   void readLock(UINT32 timeout = INFINITE) { RWLockReadLock(m_rwlock, timeout); }
   void writeLock(UINT32 timeout = INFINITE) { RWLockWriteLock(m_rwlock, timeout); }
   void unlock() { RWLockUnlock(m_rwlock); }
};

/**
 * Wrappers for condition
 */
class LIBNETXMS_EXPORTABLE Condition
{
private:
   CONDITION m_condition;
   VolatileCounter *m_refCount;

public:
   Condition(bool broadcast);
   Condition(const Condition& src);
   ~Condition();

   Condition& operator =(const Condition &src);

   void set() { ConditionSet(m_condition); }
   void pulse() { ConditionPulse(m_condition); }
   void reset() { ConditionReset(m_condition); }
   bool wait(UINT32 timeout = INFINITE) { return ConditionWait(m_condition, timeout); }
};

#endif   /* __cplusplus */

#endif   /* _nms_threads_h_ */
