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
   SetEvent(hCond);
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

#define INVALID_MUTEX_HANDLE  NULL

#ifndef INFINITE
# define INFINITE 0
#endif

//
// Inline functions
//

inline void ThreadSleep(int nSeconds)
{
	struct timeval tv;

	tv.tv_sec = nSeconds;
	tv.tv_usec = 0;

	select(1, NULL, NULL, NULL, &tv);
}

inline void ThreadSleepMs(DWORD dwMilliseconds)
{
	// select is a sort of overkill
   usleep(dwMilliseconds * 1000);   // Convert to microseconds
}

inline THREAD ThreadCreate(void (*start_address )(void *), int stack_size, void *args)
{
	THREAD id;

	if (pthread_create(&id, NULL, (void *(*)(void *))start_address, args) == 0) {
		return id;
	} else {
		return 0;
	}
}

inline void ThreadExit(void)
{
   pthread_kill(pthread_self(), 15); // 9?
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
		// FIXME: hack, always waiting a bit longer
		for (i = (dwTimeOut / 50) + 1; i > 0; i--) {
	      if (pthread_mutex_trylock(mutex) != EBUSY) {
				ret = TRUE;
				break;
			}
			ThreadSleep(50);
		}
	}
   return ret;
}

inline void MutexUnlock(MUTEX mutex)
{
   if (mutex != NULL) {
      pthread_mutex_unlock(mutex);
	}
}

inline CONDITION ConditionCreate(BOOL bBroadcast)
{
	CONDITION cond;

   cond = (CONDITION)malloc(sizeof(struct condition_t));
   if (cond != NULL) {
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
		if (cond->broadcast == TRUE)
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
			struct timeval now;
			struct timespec timeout;

			gettimeofday(&now, NULL);
			timeout.tv_sec = now.tv_sec + (dwTimeOut / 1000);
			timeout.tv_nsec = ( now.tv_usec + ( dwTimeOut % 1000 ) ) * 1000;
			while (retcode != ETIMEDOUT) {
				retcode = pthread_cond_timedwait(&cond->cond, &cond->mutex, &timeout);
			}
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
