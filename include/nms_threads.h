/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2020 Victor Kirhenshtein
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

extern LIBNETXMS_EXPORTABLE_VAR(int g_defaultThreadStackSize);

#if defined(_WIN32)

#ifndef UNDER_CE
#include <process.h>
#endif

#include "winmutex.h"

//
// Related datatypes and constants
//

#define INVALID_MUTEX_HANDLE        (NULL)
#define INVALID_CONDITION_HANDLE    (NULL)
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

struct netxms_cond_t
{
   CONDITION_VARIABLE v;
   CRITICAL_SECTION lock;
   bool broadcast;
   bool isSet;
};
typedef netxms_cond_t *CONDITION;

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

	THREAD_START_DATA *data = (THREAD_START_DATA *)MemAlloc(sizeof(THREAD_START_DATA));
	data->start_address = start_address;
	data->args = args;
   hThread = (HANDLE)_beginthreadex(NULL, (stack_size != 0) ? stack_size : g_defaultThreadStackSize, SEHThreadStarter, data, 0, &dwThreadId);
   if (hThread != NULL)
      CloseHandle(hThread);
   return (hThread != NULL);
}

inline THREAD ThreadCreateEx(ThreadFunction start_address, int stack_size, void *args)
{
   THREAD thread;

	thread = (THREAD)MemAlloc(sizeof(struct netxms_thread_t));
	THREAD_START_DATA *data = (THREAD_START_DATA *)MemAlloc(sizeof(THREAD_START_DATA));
	data->start_address = start_address;
	data->args = args;
   thread->handle = (HANDLE)_beginthreadex(NULL, (stack_size != 0) ? stack_size : g_defaultThreadStackSize, SEHThreadStarter, data, 0, &thread->id);
	if ((thread->handle == (HANDLE)-1) || (thread->handle == 0))
	{
		MemFree(data);
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

inline MUTEX MutexCreateFast()
{
   // Under Windows we always use mutex with spin
   return MutexCreate();
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
   if (mutex != INVALID_MUTEX_HANDLE)
      UnlockMutex(mutex);
}

inline CONDITION ConditionCreate(bool broadcast)
{
   CONDITION c = MemAllocStruct<netxms_cond_t>();
   InitializeCriticalSectionAndSpinCount(&c->lock, 4000);
   InitializeConditionVariable(&c->v);
   c->broadcast = broadcast;
   c->isSet = false;
   return c;
}

inline void ConditionDestroy(CONDITION c)
{
   if (c != INVALID_CONDITION_HANDLE)
   {
      DeleteCriticalSection(&c->lock);
      MemFree(c);
   }
}

inline void ConditionSet(CONDITION cond)
{
   if (cond != INVALID_CONDITION_HANDLE)
   {
      EnterCriticalSection(&cond->lock);
      cond->isSet = true;
      if (cond->broadcast)
      {
         WakeAllConditionVariable(&cond->v);
      }
      else
      {
         WakeConditionVariable(&cond->v);
      }
      LeaveCriticalSection(&cond->lock);
   }
}

inline void ConditionReset(CONDITION cond)
{
   if (cond != INVALID_CONDITION_HANDLE)
   {
      EnterCriticalSection(&cond->lock);
      cond->isSet = FALSE;
      LeaveCriticalSection(&cond->lock);
   }
}

inline void ConditionPulse(CONDITION cond)
{
   if (cond != INVALID_CONDITION_HANDLE)
   {
      EnterCriticalSection(&cond->lock);
      if (cond->broadcast)
      {
         WakeAllConditionVariable(&cond->v);
      }
      else
      {
         WakeConditionVariable(&cond->v);
      }
      cond->isSet = FALSE;
      LeaveCriticalSection(&cond->lock);
   }
}

inline bool ConditionWait(CONDITION cond, UINT32 timeout)
{
	if (cond == INVALID_CONDITION_HANDLE)
		return false;

   bool success;

   EnterCriticalSection(&cond->lock);

   if (cond->isSet)
   {
      success = true;
      if (!cond->broadcast)
         cond->isSet = false;
   }
   else if (timeout == INFINITE)
   {
      do
      {
         SleepConditionVariableCS(&cond->v, &cond->lock, INFINITE);
      } while (!cond->isSet);
      success = true;
      if (!cond->broadcast)
         cond->isSet = false;
   }
   else
   {
      do
      {
         UINT64 start = GetTickCount64();
         SleepConditionVariableCS(&cond->v, &cond->lock, timeout);
         UINT32 elapsed = static_cast<UINT32>(GetTickCount64() - start);
         timeout -= std::min(elapsed, timeout);
      } while (!cond->isSet && (timeout > 0));
      success = cond->isSet;
      if (!cond->broadcast)
         cond->isSet = false;
   }

   LeaveCriticalSection(&cond->lock);
   return success;
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
   MUTEX mutex = (MUTEX)MemAlloc(sizeof(pth_mutex_t));
   if (mutex != NULL)
   {
      pth_mutex_init(mutex);
   }
   return mutex;
}

inline MUTEX MutexCreateFast(void)
{
   return MutexCreate();
}

inline MUTEX MutexCreateRecursive()
{
   // In libpth, recursive locking is explicitly supported,
   // so we just create mutex
   return MutexCreate();
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

static inline uint32_t GetCurrentProcessId()
{
   return getpid();
}

static inline uint32_t GetCurrentThreadId()
{
   return (uint32_t)pth_self();
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
	struct timeval tv;
	tv.tv_sec = nSeconds;
	tv.tv_usec = 0;
	select(1, NULL, NULL, NULL, &tv);
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
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, (stack_size != 0) ? stack_size : g_defaultThreadStackSize);

   THREAD id;
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

/**
 * Create normal mutex
 */
inline MUTEX MutexCreate()
{
   MUTEX mutex = (MUTEX)MemAlloc(sizeof(netxms_mutex_t));
   if (mutex != NULL)
   {
      pthread_mutex_init(&mutex->mutex, NULL);
#ifndef HAVE_RECURSIVE_MUTEXES
      mutex->isRecursive = FALSE;
#endif
   }
   return mutex;
}

/**
 * Create fast mutex if supported, otherwise normal mutex
 * Such mutex should not be used in MutexTimedLock
 */
inline MUTEX MutexCreateFast()
{
   MUTEX mutex = (MUTEX)MemAlloc(sizeof(netxms_mutex_t));
   if (mutex != NULL)
   {
#ifndef HAVE_RECURSIVE_MUTEXES
      mutex->isRecursive = FALSE;
#endif
#if HAVE_DECL_PTHREAD_MUTEX_ADAPTIVE_NP
      pthread_mutexattr_t a;
      pthread_mutexattr_init(&a);
      MUTEXATTR_SETTYPE(&a, PTHREAD_MUTEX_ADAPTIVE_NP);
      pthread_mutex_init(&mutex->mutex, &a);
      pthread_mutexattr_destroy(&a);
#else
      pthread_mutex_init(&mutex->mutex, NULL);
#endif
   }
   return mutex;
}

/**
 * Create recursive mutex
 */
inline MUTEX MutexCreateRecursive()
{
   MUTEX mutex = (MUTEX)MemAlloc(sizeof(netxms_mutex_t));
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
#if HAVE_PTHREAD_COND_RELTIMEDWAIT_NP
			   struct timespec timeout;
			   timeout.tv_sec = dwTimeOut / 1000;
			   timeout.tv_nsec = (dwTimeOut % 1000) * 1000000;
			   retcode = pthread_cond_reltimedwait_np(&cond->cond, &cond->mutex, &timeout);
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
static inline uint32_t GetCurrentProcessId()
{
   return getpid();
}

/**
 *  Get ID of current thread
 */
static inline uint32_t GetCurrentThreadId()
{
#if HAVE_GETTID_SYSCALL
   return (uint32_t)syscall(SYS_gettid);
#elif HAVE_PTHREAD_GETTHREADID_NP
   return (uint32_t)pthread_getthreadid_np();
#elif HAVE_PTHREAD_THREADID_NP
   uint64_t id;
   if (pthread_threadid_np(NULL, &id) != 0)
      return 0;
   return (uint32_t)id;
#elif HAVE_PTHREAD_GETLWPID_NP
   lwpid_t id;
   if (pthread_getlwpid_np(pthread_self(), &id) != 0)
      return 0;
   return (uint32_t)id;
#elif defined(_AIX) || defined(__sun)
   return (uint32_t)pthread_self();
#else
   return 0;
#endif
}

#endif   /* _WIN32 */

/**
 * Template wrapper for ThreadCreate/ThreadCreateEx thread functions (no arguments)
 */
static inline THREAD_RESULT THREAD_CALL ThreadCreate_Wrapper_0(void *function)
{
   ((void (*)())function)();
   return THREAD_OK;
}

/**
 * Template wrapper for ThreadCreate (no arguments)
 */
static inline bool ThreadCreate(void (*function)(), int stackSize = 0)
{
   return ThreadCreate(ThreadCreate_Wrapper_0, stackSize, (void*)function);
}

/**
 * Template wrapper for ThreadCreateEx (no arguments)
 */
static inline THREAD ThreadCreateEx(void (*function)(), int stackSize = 0)
{
   return ThreadCreateEx(ThreadCreate_Wrapper_0, stackSize, (void*)function);
}

/**
 * Template wrapper data for ThreadCreate/ThreadCreateEx thread functions (1 argument)
 */
template<typename R> class ThreadCreate_WrapperData_1
{
public:
   void (*function)(R);
   R arg;

   ThreadCreate_WrapperData_1(void (*_function)(R), R _arg) : arg(_arg)
   {
      function = _function;
   }
};

/**
 * Template wrapper for ThreadCreate/ThreadCreateEx thread functions (1 argument)
 */
template<typename R> THREAD_RESULT THREAD_CALL ThreadCreate_Wrapper_1(void *context)
{
   auto wd = static_cast<ThreadCreate_WrapperData_1<R>*>(context);
   wd->function(wd->arg);
   delete wd;
   return THREAD_OK;
}

/**
 * Template wrapper for ThreadCreate (1 argument)
 */
template<typename R> bool ThreadCreate(void (*function)(R), R arg, int stackSize = 0)
{
   auto wd = new ThreadCreate_WrapperData_1<R>(function, arg);
   bool success = ThreadCreate(ThreadCreate_Wrapper_1<R>, stackSize, wd);
   if (!success)
      delete wd;
   return success;
}

/**
 * Template wrapper for ThreadCreateEx (1 argument)
 */
template<typename R> THREAD ThreadCreateEx(void (*function)(R), R arg, int stackSize = 0)
{
   auto wd = new ThreadCreate_WrapperData_1<R>(function, arg);
   THREAD thread = ThreadCreateEx(ThreadCreate_Wrapper_1<R>, stackSize, wd);
   if (thread == INVALID_THREAD_HANDLE)
      delete wd;
   return thread;
}

/**
 * Template wrapper data for ThreadCreate/ThreadCreateEx thread functions (2 arguments)
 */
template<typename R1, typename R2> class ThreadCreate_WrapperData_2
{
public:
   void (*function)(R1, R2);
   R1 arg1;
   R2 arg2;

   ThreadCreate_WrapperData_2(void (*_function)(R1, R2), R1 _arg1, R2 _arg2) : arg1(_arg1), arg2(_arg2)
   {
      function = _function;
   }
};

/**
 * Template wrapper for ThreadCreate/ThreadCreateEx thread functions (2 arguments)
 */
template<typename R1, typename R2> THREAD_RESULT THREAD_CALL ThreadCreate_Wrapper_2(void *context)
{
   auto wd = static_cast<ThreadCreate_WrapperData_2<R1, R2>*>(context);
   wd->function(wd->arg1, wd->arg2);
   delete wd;
   return THREAD_OK;
}

/**
 * Template wrapper for ThreadCreate (2 arguments)
 */
template<typename R1, typename R2> bool ThreadCreate(void (*function)(R1, R2), R1 arg1, R2 arg2, int stackSize = 0)
{
   auto wd = new ThreadCreate_WrapperData_2<R1, R2>(function, arg1, arg2);
   bool success = ThreadCreate(ThreadCreate_Wrapper_2<R1, R2>, stackSize, wd);
   if (!success)
      delete wd;
   return success;
}

/**
 * Template wrapper for ThreadCreateEx (2 arguments)
 */
template<typename R1, typename R2> THREAD ThreadCreateEx(void (*function)(R1, R2), R1 arg1, R2 arg2, int stackSize = 0)
{
   auto wd = new ThreadCreate_WrapperData_2<R1, R2>(function, arg1, arg2);
   THREAD thread = ThreadCreateEx(ThreadCreate_Wrapper_2<R1, R2>, stackSize, wd);
   if (thread == INVALID_THREAD_HANDLE)
      delete wd;
   return thread;
}

/**
 * Wrapper data for ThreadCreate/ThreadCreateEx on object method using pointer to object (no arguments)
 */
template <typename T> class ThreadCreate_ObjectPtr_WrapperData_0
{
public:
   T *m_object;
   void (T::*m_func)();

   ThreadCreate_ObjectPtr_WrapperData_0(T *object, void (T::*func)())
   {
      m_object = object;
      m_func = func;
   }
};

/**
 * Wrapper for ThreadCreate/ThreadCreateEx on object method using pointer to object (no arguments)
 */
template <typename T> THREAD_RESULT THREAD_CALL ThreadCreate_ObjectPtr_Wrapper_0(void *arg)
{
   auto wd = static_cast<ThreadCreate_ObjectPtr_WrapperData_0<T>*>(arg);
   ((*wd->m_object).*(wd->m_func))();
   delete wd;
   return THREAD_OK;
}

/**
 * Template wrapper for ThreadCreate on object method using pointer to object (no arguments)
 */
template<typename T, typename B> bool ThreadCreate(T *object, void (B::*method)(), int stackSize = 0)
{
   auto wd = new ThreadCreate_ObjectPtr_WrapperData_0<B>(object, method);
   bool success = ThreadCreate(ThreadCreate_ObjectPtr_Wrapper_0<B>, stackSize, wd);
   if (!success)
      delete wd;
   return success;
}

/**
 * Template wrapper for ThreadCreateEx on object method using pointer to object (no arguments)
 */
template<typename T, typename B> THREAD ThreadCreateEx(T *object, void (B::*method)(), int stackSize = 0)
{
   auto wd = new ThreadCreate_ObjectPtr_WrapperData_0<B>(object, method);
   THREAD thread = ThreadCreateEx(ThreadCreate_ObjectPtr_Wrapper_0<B>, stackSize, wd);
   if (thread == INVALID_THREAD_HANDLE)
      delete wd;
   return thread;
}

/**
 * Wrapper data for ThreadCreate/ThreadCreateEx on object method using pointer to object (no arguments)
 */
template <typename T, typename R> class ThreadCreate_ObjectPtr_WrapperData_1
{
public:
   T *m_object;
   void (T::*m_func)(R);
   R m_arg;

   ThreadCreate_ObjectPtr_WrapperData_1(T *object, void (T::*func)(R), const R& arg) : m_arg(arg)
   {
      m_object = object;
      m_func = func;
   }
};

/**
 * Wrapper for ThreadCreate/ThreadCreateEx on object method using pointer to object (one argument)
 */
template <typename T, typename R> THREAD_RESULT THREAD_CALL ThreadCreate_ObjectPtr_Wrapper_1(void *arg)
{
   auto wd = static_cast<ThreadCreate_ObjectPtr_WrapperData_1<T, R>*>(arg);
   ((*wd->m_object).*(wd->m_func))(wd->m_arg);
   delete wd;
   return THREAD_OK;
}

/**
 * Template wrapper for ThreadCreate on object method using pointer to object (one argument)
 */
template<typename T, typename B, typename R> bool ThreadCreate(T *object, void (B::*method)(R), const R& arg, int stackSize = 0)
{
   auto wd = new ThreadCreate_ObjectPtr_WrapperData_1<B, R>(object, method, arg);
   bool success = ThreadCreate(ThreadCreate_ObjectPtr_Wrapper_1<B, R>, stackSize, wd);
   if (!success)
      delete wd;
   return success;
}

/**
 * Template wrapper for ThreadCreateEx on object method using pointer to object (one argument)
 */
template<typename T, typename B, typename R> THREAD ThreadCreateEx(T *object, void (B::*method)(R), const R& arg, int stackSize = 0)
{
   auto wd = new ThreadCreate_ObjectPtr_WrapperData_1<B, R>(object, method, arg);
   THREAD thread = ThreadCreateEx(ThreadCreate_ObjectPtr_Wrapper_1<B, R>, stackSize, wd);
   if (thread == INVALID_THREAD_HANDLE)
      delete wd;
   return thread;
}

/**
 * Wrapper data for ThreadCreate/ThreadCreateEx on object method using smart pointer to object (no arguments)
 */
template <typename T> class ThreadCreate_SharedPtr_WrapperData_0
{
public:
   shared_ptr<T> m_object;
   void (T::*m_func)();

   ThreadCreate_SharedPtr_WrapperData_0(const shared_ptr<T>& object, void (T::*func)()) : m_object(object)
   {
      m_func = func;
   }
};

/**
 * Wrapper for ThreadCreate/ThreadCreateEx on object method using smart pointer to object (no arguments)
 */
template <typename T> THREAD_RESULT THREAD_CALL ThreadCreate_SharedPtr_Wrapper_0(void *arg)
{
   auto wd = static_cast<ThreadCreate_SharedPtr_WrapperData_0<T>*>(arg);
   ((*wd->m_object.get()).*(wd->m_func))();
   delete wd;
   return THREAD_OK;
}

/**
 * Template wrapper for ThreadCreate on object method using smart pointer to object (no arguments)
 */
template<typename T, typename B> bool ThreadCreate(const shared_ptr<T>& object, void (B::*method)(), int stackSize = 0)
{
   auto wd = new ThreadCreate_SharedPtr_WrapperData_0<B>(object, method);
   bool success = ThreadCreate(ThreadCreate_SharedPtr_Wrapper_0<B>, stackSize, wd);
   if (!success)
      delete wd;
   return success;
}

/**
 * Template wrapper for ThreadCreateEx on object method using smart pointer to object (no arguments)
 */
template<typename T, typename B> THREAD ThreadCreateEx(const shared_ptr<T>& object, void (B::*method)(), int stackSize = 0)
{
   auto wd = new ThreadCreate_SharedPtr_WrapperData_0<B>(object, method);
   THREAD thread = ThreadCreateEx(ThreadCreate_SharedPtr_Wrapper_0<B>, stackSize, wd);
   if (thread == INVALID_THREAD_HANDLE)
      delete wd;
   return thread;
}

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
   const TCHAR *name;          // pool name
   int32_t minThreads;         // min threads
   int32_t maxThreads;         // max threads
   int32_t curThreads;         // current threads
   int32_t activeRequests;     // number of active requests
   int32_t scheduledRequests;  // number of requests scheduled for execution
   int32_t serializedRequests; // number of requests waiting in serialized execution queues
   uint64_t totalRequests;     // Total requests executed (cumulative count)
   uint64_t threadStarts;      // Number of thread starts
   uint64_t threadStops;       // Number of thread stops
   int32_t usage;              // Pool usage in %
   int32_t load;               // Pool current load in % (can be more than 100% if there are more requests then threads available)
   double loadAvg[3];          // Pool load average
   uint32_t averageWaitTime;   // Average task wait time
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
void LIBNETXMS_EXPORTABLE ThreadPoolScheduleAbsoluteMs(ThreadPool *p, int64_t runTime, ThreadPoolWorkerFunction f, void *arg);
void LIBNETXMS_EXPORTABLE ThreadPoolScheduleRelative(ThreadPool *p, uint32_t delay, ThreadPoolWorkerFunction f, void *arg);
void LIBNETXMS_EXPORTABLE ThreadPoolGetInfo(ThreadPool *p, ThreadPoolInfo *info);
bool LIBNETXMS_EXPORTABLE ThreadPoolGetInfo(const TCHAR *name, ThreadPoolInfo *info);
int LIBNETXMS_EXPORTABLE ThreadPoolGetSerializedRequestCount(ThreadPool *p, const TCHAR *key);
uint32_t LIBNETXMS_EXPORTABLE ThreadPoolGetSerializedRequestMaxWaitTime(ThreadPool *p, const TCHAR *key);
StringList LIBNETXMS_EXPORTABLE *ThreadPoolGetAllPools();
void LIBNETXMS_EXPORTABLE ThreadPoolSetResizeParameters(int responsiveness, uint32_t waitTimeHWM, uint32_t waitTimeLWM);

/**
 * Wrapper for ThreadPoolExecute for function without arguments
 */
static inline void ThreadPoolExecute_NoArg_Wrapper(void *arg)
{
   ((void (*)())arg)();
}

/**
 * Wrapper for ThreadPoolExecute for function without arguments
 */
static inline void ThreadPoolExecute(ThreadPool *p, void (*f)())
{
   ThreadPoolExecute(p, ThreadPoolExecute_NoArg_Wrapper, (void *)f);
}

/**
 * Wrapper for ThreadPoolExecuteSerialized for function without arguments
 */
static inline void ThreadPoolExecuteSerialized(ThreadPool *p, const TCHAR *key, void (*f)())
{
   ThreadPoolExecuteSerialized(p, key, ThreadPoolExecute_NoArg_Wrapper, (void *)f);
}

/**
 * Wrapper for ThreadPoolScheduleAbsolute for function without arguments
 */
static inline void ThreadPoolScheduleAbsolute(ThreadPool *p, time_t runTime, void (*f)())
{
   ThreadPoolScheduleAbsolute(p, runTime, ThreadPoolExecute_NoArg_Wrapper, (void *)f);
}

/**
 * Wrapper for ThreadPoolScheduleAbsoluteMs for function without arguments
 */
static inline void ThreadPoolScheduleAbsoluteMs(ThreadPool *p, int64_t runTime, void (*f)())
{
   ThreadPoolScheduleAbsoluteMs(p, runTime, ThreadPoolExecute_NoArg_Wrapper, (void *)f);
}

/**
 * Wrapper for ThreadPoolScheduleRelative for function without arguments
 */
static inline void ThreadPoolScheduleRelative(ThreadPool *p, uint32_t delay, void (*f)())
{
   ThreadPoolScheduleRelative(p, delay, ThreadPoolExecute_NoArg_Wrapper, (void *)f);
}

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
   void (*m_func)(const shared_ptr<T>&);

   __ThreadPoolExecute_SharedPtr_WrapperData(const shared_ptr<T>& object, void (*func)(const shared_ptr<T>&)) : m_object(object)
   {
      m_func = func;
   }
};

/**
 * Wrapper for ThreadPoolExecute with shared_ptr argument
 */
template <typename T> void __ThreadPoolExecute_SharedPtr_Wrapper(void *arg)
{
   auto wd = static_cast<__ThreadPoolExecute_SharedPtr_WrapperData<T> *>(arg);
   wd->m_func(wd->m_object);
   delete wd;
}

/**
 * Wrapper for ThreadPoolExecute to use smart pointer to given type as argument
 */
template <typename T> inline void ThreadPoolExecute(ThreadPool *p, void (*f)(const shared_ptr<T>&), const shared_ptr<T>& arg)
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
template <typename T> inline void ThreadPoolExecuteSerialized(ThreadPool *p, const TCHAR *key, void (*f)(const shared_ptr<T>&), const shared_ptr<T>& arg)
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
 * Wrapper for ThreadPoolScheduleAbsolute to use pointer to given type as argument
 */
template <typename T> inline void ThreadPoolScheduleAbsolute(ThreadPool *p, time_t runTime, void (*f)(const shared_ptr<T>&), const shared_ptr<T>& arg)
{
   ThreadPoolScheduleAbsolute(p, runTime, __ThreadPoolExecute_SharedPtr_Wrapper<T>, new __ThreadPoolExecute_SharedPtr_WrapperData<T>(arg, f));
}

/**
 * Wrapper for ThreadPoolScheduleAbsoluteMs to use pointer to given type as argument
 */
template <typename T> inline void ThreadPoolScheduleAbsoluteMs(ThreadPool *p, int64_t runTime, void (*f)(T *), T *arg)
{
   ThreadPoolScheduleAbsoluteMs(p, runTime, (ThreadPoolWorkerFunction)f, (void *)arg);
}

/**
 * Wrapper for ThreadPoolScheduleAbsoluteMs to use pointer to given type as argument
 */
template <typename T> inline void ThreadPoolScheduleAbsoluteMs(ThreadPool *p, int64_t runTime, void (*f)(const shared_ptr<T>&), const shared_ptr<T>& arg)
{
   ThreadPoolScheduleAbsoluteMs(p, runTime, __ThreadPoolExecute_SharedPtr_Wrapper<T>, new __ThreadPoolExecute_SharedPtr_WrapperData<T>(arg, f));
}

/**
 * Wrapper for ThreadPoolScheduleRelative to use pointer to given type as argument
 */
template <typename T> inline void ThreadPoolScheduleRelative(ThreadPool *p, uint32_t delay, void (*f)(T *), T *arg)
{
   ThreadPoolScheduleRelative(p, delay, (ThreadPoolWorkerFunction)f, (void *)arg);
}

/**
 * Wrapper for ThreadPoolScheduleRelative to use pointer to given type as argument
 */
template <typename T> inline void ThreadPoolScheduleRelative(ThreadPool *p, uint32_t delay, void (*f)(const shared_ptr<T>&), const shared_ptr<T>& arg)
{
   ThreadPoolScheduleRelative(p, delay, __ThreadPoolExecute_SharedPtr_Wrapper<T>, new __ThreadPoolExecute_SharedPtr_WrapperData<T>(arg, f));
}

/**
 * Wrapper data for ThreadPoolExecute (class method without arguments)
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
   auto wd = static_cast<__ThreadPoolExecute_WrapperData_0<T> *>(arg);
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
template <typename T, typename B> inline void ThreadPoolScheduleRelative(ThreadPool *p, uint32_t delay, T *object, void (B::*f)())
{
   ThreadPoolScheduleRelative(p, delay, __ThreadPoolExecute_Wrapper_0<B>, new __ThreadPoolExecute_WrapperData_0<B>(object, f));
}

/**
 * Wrapper data for ThreadPoolExecute (no arguments) using smart pointer to object
 */
template <typename T> class __ThreadPoolExecute_SharedPtr_WrapperData_0
{
public:
   shared_ptr<T> m_object;
   void (T::*m_func)();

   __ThreadPoolExecute_SharedPtr_WrapperData_0(const shared_ptr<T>& object, void (T::*func)()) : m_object(object)
   {
      m_func = func;
   }
};

/**
 * Wrapper for ThreadPoolExecute (no arguments) using smart pointer to object
 */
template <typename T> void __ThreadPoolExecute_SharedPtr_Wrapper_0(void *arg)
{
   auto wd = static_cast<__ThreadPoolExecute_SharedPtr_WrapperData_0<T> *>(arg);
   ((*wd->m_object.get()).*(wd->m_func))();
   delete wd;
}

/**
 * Execute task as soon as possible (use class member without arguments) using smart pointer to object
 */
template <typename T, typename B> inline void ThreadPoolExecute(ThreadPool *p, const shared_ptr<T>& object, void (B::*f)())
{
   ThreadPoolExecute(p, __ThreadPoolExecute_SharedPtr_Wrapper_0<B>, new __ThreadPoolExecute_SharedPtr_WrapperData_0<B>(object, f));
}

/**
 * Execute serialized task as soon as possible (use class member without arguments) using smart pointer to object
 */
template <typename T, typename B> inline void ThreadPoolExecuteSerialized(ThreadPool *p, const TCHAR *key, const shared_ptr<T>& object, void (B::*f)())
{
   ThreadPoolExecuteSerialized(p, key, __ThreadPoolExecute_SharedPtr_Wrapper_0<B>, new __ThreadPoolExecute_SharedPtr_WrapperData_0<B>(object, f));
}

/**
 * Execute task with delay (use class member without arguments) using smart pointer to object
 */
template <typename T, typename B> inline void ThreadPoolScheduleRelative(ThreadPool *p, uint32_t delay, const shared_ptr<T>& object, void (B::*f)())
{
   ThreadPoolScheduleRelative(p, delay, __ThreadPoolExecute_SharedPtr_Wrapper_0<B>, new __ThreadPoolExecute_SharedPtr_WrapperData_0<B>(object, f));
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

   __ThreadPoolExecute_WrapperData_1(T *object, void (T::*func)(R), R arg) : m_arg(arg)
   {
      m_object = object;
      m_func = func;
   }
};

/**
 * Wrapper for ThreadPoolExecute (one argument)
 */
template <typename T, typename R> void __ThreadPoolExecute_Wrapper_1(void *arg)
{
   auto wd = static_cast<__ThreadPoolExecute_WrapperData_1<T, R> *>(arg);
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
template <typename T, typename B, typename R> inline void ThreadPoolScheduleRelative(ThreadPool *p, uint32_t delay, T *object, void (B::*f)(R), R arg)
{
   ThreadPoolScheduleRelative(p, delay, __ThreadPoolExecute_Wrapper_1<B, R>, new __ThreadPoolExecute_WrapperData_1<B, R>(object, f, arg));
}

/**
 * Wrapper data for ThreadPoolExecute (one argument) using smart pointer to object
 */
template <typename T, typename R> class __ThreadPoolExecute_SharedPtr_WrapperData_1
{
public:
   shared_ptr<T> m_object;
   void (T::*m_func)(R);
   R m_arg;

   __ThreadPoolExecute_SharedPtr_WrapperData_1(const shared_ptr<T>& object, void (T::*func)(R), R arg) : m_object(object), m_arg(arg)
   {
      m_func = func;
   }
};

/**
 * Wrapper for ThreadPoolExecute (one argument) using smart pointer to object
 */
template <typename T, typename R> void __ThreadPoolExecute_SharedPtr_Wrapper_1(void *arg)
{
   auto wd = static_cast<__ThreadPoolExecute_SharedPtr_WrapperData_1<T, R> *>(arg);
   ((*wd->m_object.get()).*(wd->m_func))(wd->m_arg);
   delete wd;
}

/**
 * Execute task as soon as possible (use class member with one argument) using smart pointer to object
 */
template <typename T, typename B, typename R> inline void ThreadPoolExecute(ThreadPool *p, const shared_ptr<T>& object, void (B::*f)(R), R arg)
{
   ThreadPoolExecute(p, __ThreadPoolExecute_SharedPtr_Wrapper_1<B, R>, new __ThreadPoolExecute_SharedPtr_WrapperData_1<B, R>(object, f, arg));
}

/**
 * Execute serialized task as soon as possible (use class member with one argument) using smart pointer to object
 */
template <typename T, typename B, typename R> inline void ThreadPoolExecuteSerialized(ThreadPool *p, const TCHAR *key, const shared_ptr<T>& object, void (B::*f)(R), R arg)
{
   ThreadPoolExecuteSerialized(p, key, __ThreadPoolExecute_SharedPtr_Wrapper_1<B, R>, new __ThreadPoolExecute_SharedPtr_WrapperData_1<B, R>(object, f, arg));
}

/**
 * Execute task with delay (use class member with one argument) using smart pointer to object
 */
template <typename T, typename B, typename R> inline void ThreadPoolScheduleRelative(ThreadPool *p, uint32_t delay, const shared_ptr<T>& object, void (B::*f)(R), R arg)
{
   ThreadPoolScheduleRelative(p, delay, __ThreadPoolExecute_SharedPtr_Wrapper_1<B, R>, new __ThreadPoolExecute_SharedPtr_WrapperData_1<B, R>(object, f, arg));
}

/**
 * Wrapper data for ThreadPoolExecute (function with two arguments)
 */
template <typename R1, typename R2> class __ThreadPoolExecute_WrapperData_2F
{
public:
   void (*m_func)(R1, R2);
   R1 m_arg1;
   R2 m_arg2;

   __ThreadPoolExecute_WrapperData_2F(void (*func)(R1, R2), R1 arg1, R2 arg2) : m_arg1(arg1), m_arg2(arg2)
   {
      m_func = func;
   }
};

/**
 * Wrapper for ThreadPoolExecute (function with two arguments)
 */
template <typename R1, typename R2> void __ThreadPoolExecute_Wrapper_2F(void *arg)
{
   auto wd = static_cast<__ThreadPoolExecute_WrapperData_2F<R1, R2>*>(arg);
   wd->m_func(wd->m_arg1, wd->m_arg2);
   delete wd;
}

/**
 * Execute task as soon as possible (function with two arguments)
 */
template <typename R1, typename R2> inline void ThreadPoolExecute(ThreadPool *p, void (*f)(R1, R2), R1 arg1, R2 arg2)
{
   ThreadPoolExecute(p, __ThreadPoolExecute_Wrapper_2F<R1, R2>, new __ThreadPoolExecute_WrapperData_2F<R1, R2>(f, arg1, arg2));
}

/**
 * Execute serialized task as soon as possible (function with two arguments)
 */
template <typename R1, typename R2> inline void ThreadPoolExecuteSerialized(ThreadPool *p, const TCHAR *key, void (*f)(R1, R2), R1 arg1, R2 arg2)
{
   ThreadPoolExecuteSerialized(p, key, __ThreadPoolExecute_Wrapper_2F<R1, R2>, new __ThreadPoolExecute_WrapperData_2F<R1, R2>(f, arg1, arg2));
}

/**
 * Wrapper data for ThreadPoolExecute (class method with two arguments)
 */
template <typename T, typename R1, typename R2> class __ThreadPoolExecute_WrapperData_2
{
public:
   T *m_object;
   void (T::*m_func)(R1, R2);
   R1 m_arg1;
   R2 m_arg2;

   __ThreadPoolExecute_WrapperData_2(T *object, void (T::*func)(R1, R2), R1 arg1, R2 arg2) : m_arg1(arg1), m_arg2(arg2)
   {
      m_object = object;
      m_func = func;
   }
};

/**
 * Wrapper for ThreadPoolExecute (class method with two arguments)
 */
template <typename T, typename R1, typename R2> void __ThreadPoolExecute_Wrapper_2(void *arg)
{
   auto wd = static_cast<__ThreadPoolExecute_WrapperData_2<T, R1, R2>*>(arg);
   ((*wd->m_object).*(wd->m_func))(wd->m_arg1, wd->m_arg2);
   delete wd;
}

/**
 * Execute task as soon as possible (class method with two arguments)
 */
template <typename T, typename B, typename R1, typename R2> inline void ThreadPoolExecute(ThreadPool *p, T *object, void (B::*f)(R1, R2), R1 arg1, R2 arg2)
{
   ThreadPoolExecute(p, __ThreadPoolExecute_Wrapper_2<B, R1, R2>, new __ThreadPoolExecute_WrapperData_2<B, R1, R2>(object, f, arg1, arg2));
}

/**
 * Wrapper data for ThreadPoolExecute (three arguments)
 */
template <typename T, typename R1, typename R2, typename R3> class __ThreadPoolExecute_WrapperData_3
{
public:
   T *m_object;
   void (T::*m_func)(R1, R2, R3);
   R1 m_arg1;
   R2 m_arg2;
   R3 m_arg3;

   __ThreadPoolExecute_WrapperData_3(T *object, void (T::*func)(R1, R2, R3), R1 arg1, R2 arg2, R3 arg3) : m_arg1(arg1), m_arg2(arg2), m_arg3(arg3)
   {
      m_object = object;
      m_func = func;
   }
};

/**
 * Wrapper for ThreadPoolExecute (three arguments)
 */
template <typename T, typename R1, typename R2, typename R3> void __ThreadPoolExecute_Wrapper_3(void *arg)
{
   __ThreadPoolExecute_WrapperData_3<T, R1, R2, R3> *wd = static_cast<__ThreadPoolExecute_WrapperData_3<T, R1, R2, R3> *>(arg);
   ((*wd->m_object).*(wd->m_func))(wd->m_arg1, wd->m_arg2, wd->m_arg3);
   delete wd;
}

/**
 * Execute task as soon as possible (use class member with three arguments)
 */
template <typename T, typename B, typename R1, typename R2, typename R3> inline void ThreadPoolExecute(ThreadPool *p, T *object, void (B::*f)(R1, R2, R3), R1 arg1, R2 arg2, R3 arg3)
{
   ThreadPoolExecute(p, __ThreadPoolExecute_Wrapper_3<B, R1, R2, R3>, new __ThreadPoolExecute_WrapperData_3<B, R1, R2, R3>(object, f, arg1, arg2, arg3));
}

/**
 * Wrapper data for ThreadPoolExecute (three arguments)
 */
template <typename R1, typename R2, typename R3> class __ThreadPoolExecute_WrapperData_3F
{
public:
   void (*m_func)(R1, R2, R3);
   R1 m_arg1;
   R2 m_arg2;
   R3 m_arg3;

   __ThreadPoolExecute_WrapperData_3F(void (*func)(R1, R2, R3), R1 arg1, R2 arg2, R3 arg3) : m_arg1(arg1), m_arg2(arg2), m_arg3(arg3)
   {
      m_func = func;
   }
};

/**
 * Wrapper for ThreadPoolExecute (three arguments)
 */
template <typename R1, typename R2, typename R3> void __ThreadPoolExecute_Wrapper_3F(void *arg)
{
   auto wd = static_cast<__ThreadPoolExecute_WrapperData_3F<R1, R2, R3>*>(arg);
   wd->m_func(wd->m_arg1, wd->m_arg2, wd->m_arg3);
   delete wd;
}

/**
 * Execute task as soon as possible (use function with three arguments)
 */
template <typename R1, typename R2, typename R3> inline void ThreadPoolExecute(ThreadPool *p, void (*f)(R1, R2, R3), R1 arg1, R2 arg2, R3 arg3)
{
   ThreadPoolExecute(p, __ThreadPoolExecute_Wrapper_3F<R1, R2, R3>, new __ThreadPoolExecute_WrapperData_3F<R1, R2, R3>(f, arg1, arg2, arg3));
}

/**
 * Set default thread stack size
 */
void LIBNETXMS_EXPORTABLE ThreadSetDefaultStackSize(int size);

/**
 * Wrappers for mutex
 */
class LIBNETXMS_EXPORTABLE Mutex
{
private:
   MUTEX m_mutex;
   VolatileCounter *m_refCount;

public:
   Mutex(bool fast = false);
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

   void readLock() { RWLockReadLock(m_rwlock); }
   void writeLock() { RWLockWriteLock(m_rwlock); }
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
   bool wait(uint32_t timeout = INFINITE) { return ConditionWait(m_condition, timeout); }
};

#endif   /* __cplusplus */

#endif   /* _nms_threads_h_ */
