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

#ifndef UNDER_CE
# include <process.h>
#endif

//
// Related datatypes and constants
//

typedef HANDLE MUTEX;
typedef HANDLE THREAD;
typedef HANDLE CONDITION;

#define INVALID_MUTEX_HANDLE        INVALID_HANDLE_VALUE
#define INVALID_CONDITION_HANDLE    INVALID_HANDLE_VALUE
#define INVALID_THREAD_HANDLE       (NULL)

#ifdef UNDER_CE
typedef DWORD THREAD_RESULT;
typedef DWORD THREAD_ID;
#else
typedef unsigned int THREAD_RESULT;
typedef unsigned int THREAD_ID;
#endif

#define THREAD_OK       0

#ifdef UNDER_CE
#define THREAD_CALL
#else
#define THREAD_CALL     __stdcall
#endif


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

inline BOOL ThreadCreate(THREAD_RESULT (THREAD_CALL *start_address )(void *), int stack_size, void *args)
{
   HANDLE hThread;
   THREAD_ID dwThreadId;

#ifdef UNDER_CE
	hThread = CreateThread(NULL, (DWORD)stack_size, start_address, args, 0, &dwThreadId);
#else
   hThread = (HANDLE)_beginthreadex(NULL, stack_size, start_address, args, 0, &dwThreadId);
#endif
   if (hThread != NULL)
      CloseHandle(hThread);
   return (hThread != NULL);
}

inline THREAD ThreadCreateEx(THREAD_RESULT (THREAD_CALL *start_address )(void *), int stack_size, void *args)
{
   THREAD_ID dwThreadId;

#ifdef UNDER_CE
	return CreateThread(NULL, (DWORD)stack_size, start_address, args, 0, &dwThreadId);
#else
   return (HANDLE)_beginthreadex(NULL, stack_size, start_address, args, 0, &dwThreadId);
#endif
}

inline void ThreadExit(void)
{
#ifdef UNDER_CE
   ExitThread(0);
#else
   _endthreadex(0);
#endif
}

inline void ThreadJoin(THREAD hThread)
{
   if (hThread != INVALID_THREAD_HANDLE)
   {
      WaitForSingleObject(hThread, INFINITE);
      CloseHandle(hThread);
   }
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

inline CONDITION ConditionCreate(BOOL bBroadcast)
{
   return CreateEvent(NULL, bBroadcast, FALSE, NULL);
}

inline void ConditionDestroy(CONDITION hCond)
{
   CloseHandle(hCond);
}

inline void ConditionSet(CONDITION hCond)
{
   PulseEvent(hCond);
}

inline BOOL ConditionWait(CONDITION hCond, DWORD dwTimeOut)
{
   return WaitForSingleObject(hCond, dwTimeOut) == WAIT_OBJECT_0;
}

#else    /* _WIN32 */

/****************************************************************************/
/* unix part                                                                */
/****************************************************************************/

#include <pthread.h>
#include <errno.h>
#include <sys/time.h>

//
// Related datatypes and constants
//

typedef pthread_t THREAD;
typedef pthread_mutex_t * MUTEX;
struct condition_t
{
	pthread_cond_t cond;
	pthread_mutex_t mutex;
	BOOL broadcast;
};
typedef struct condition_t * CONDITION;

#define INVALID_MUTEX_HANDLE        (NULL)
#define INVALID_CONDITION_HANDLE    (NULL)
#define INVALID_THREAD_HANDLE       0

#ifndef INFINITE
# define INFINITE 0
#endif

typedef void *THREAD_RESULT;

#define THREAD_OK       ((void *)0)
#define THREAD_CALL


//
// Inline functions
//

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

inline void ThreadSleepMs(DWORD dwMilliseconds)
{
	// select is a sort of overkill
   usleep(dwMilliseconds * 1000);   // Convert to microseconds
}

inline BOOL ThreadCreate(THREAD_RESULT (THREAD_CALL *start_address )(void *), int stack_size, void *args)
{
	THREAD id;

	if (pthread_create(&id, NULL, start_address, args) == 0) 
   {
      pthread_detach(id);
		return TRUE;
	} 
   else 
   {
		return FALSE;
	}
}

inline THREAD ThreadCreateEx(THREAD_RESULT (THREAD_CALL *start_address )(void *), int stack_size, void *args)
{
   THREAD id;

   if (pthread_create(&id, NULL, start_address, args) == 0)
   {
      return id;
   } 
   else 
   {
      return INVALID_THREAD_HANDLE;
   }
}

inline void ThreadExit(void)
{
   pthread_exit(NULL);
}

inline void ThreadJoin(THREAD hThread)
{
   if (hThread != INVALID_THREAD_HANDLE)
      pthread_join(hThread, NULL);
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
   if (mutex != NULL) {
      pthread_mutex_destroy(mutex);
      free(mutex);
   }
}

inline BOOL MutexLock(MUTEX mutex, DWORD dwTimeOut)
{
	int i;
	int ret = FALSE;

   if (mutex != NULL) {
		if (dwTimeOut == INFINITE)
		{
			if (pthread_mutex_lock(mutex) == 0) {
				ret = TRUE;
			}
		}
		else
		{
			for (i = (dwTimeOut / 50) + 1; i > 0; i--) {
				if (pthread_mutex_trylock(mutex) == 0) 
            {
					ret = TRUE;
					break;
				}
				ThreadSleepMs(50);
			}
		}
	}
   return ret;
}

inline void MutexUnlock(MUTEX mutex)
{
   if (mutex != NULL) 
   {
      pthread_mutex_unlock(mutex);
	}
}

inline CONDITION ConditionCreate(BOOL bBroadcast)
{
	CONDITION cond;

   cond = (CONDITION)malloc(sizeof(struct condition_t));
   if (cond != NULL) 
   {
      pthread_cond_init(&cond->cond, NULL);
      pthread_mutex_init(&cond->mutex, NULL);
		cond->broadcast = bBroadcast;
	}

   return cond;
}

inline void ConditionDestroy(CONDITION cond)
{
	if (cond != NULL)
	{
		pthread_cond_destroy(&cond->cond);
		pthread_mutex_destroy(&cond->mutex);
		free(cond);
	}
}

inline void ConditionSet(CONDITION cond)
{
	if (cond != NULL)
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
		pthread_mutex_unlock(&cond->mutex);
	}
}

inline BOOL ConditionWait(CONDITION cond, DWORD dwTimeOut)
{
	BOOL ret = FALSE;

	if (cond != NULL)
	{
		int retcode;

		pthread_mutex_lock(&cond->mutex);

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

			// FIXME there should be more accurate way
			gettimeofday(&now, NULL);
			timeout.tv_sec = now.tv_sec + (dwTimeOut / 1000);

			timeout.tv_nsec = now.tv_usec * 1000;
			timeout.tv_nsec += (dwTimeOut % 1000) * 1000;
			if (timeout.tv_nsec >= 1000000)
			{
				timeout.tv_sec += timeout.tv_nsec / 1000000;
				timeout.tv_nsec = timeout.tv_nsec % 1000000;
			}
			retcode = pthread_cond_timedwait(&cond->cond, &cond->mutex, &timeout);
#endif
		}
		else
		{
			retcode = pthread_cond_wait(&cond->cond, &cond->mutex);
		}

		pthread_mutex_unlock(&cond->mutex);

		if (retcode == 0)
		{
			ret = TRUE;
		}
	}

	return ret;
}

#endif   /* _WIN32 */

#endif   /* _nms_threads_h_ */
