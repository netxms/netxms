/* 
** NetXMS - Network Management System
** NetXMS Foundation Library
** Copyright (C) 2003-2010 Victor Kirhenshtein
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
** File: rwlock.h
**
**/

#ifndef _rwlock_h_
#define _rwlock_h_

#if HAVE_PTHREAD_RWLOCK

typedef pthread_rwlock_t * RWLOCK;

inline RWLOCK RWLockCreate(void)
{
   RWLOCK hLock;

   hLock = (RWLOCK)malloc(sizeof(pthread_rwlock_t));
   if (pthread_rwlock_init(hLock, NULL) != 0)
   {
      free(hLock);
      hLock = NULL;
   }
   return hLock;
}

inline void RWLockDestroy(RWLOCK hLock)
{
   if (hLock != NULL)
   {
      pthread_rwlock_destroy(hLock);
      free(hLock);
   }
}

inline BOOL RWLockReadLock(RWLOCK hLock, UINT32 dwTimeOut)
{
	BOOL ret = FALSE;

   if (hLock != NULL) 
   {
		if (dwTimeOut == INFINITE)
		{
			if (pthread_rwlock_rdlock(hLock) == 0) 
         {
				ret = TRUE;
			}
		}
		else
		{
#if HAVE_PTHREAD_RWLOCK_TIMEDRDLOCK
			struct timeval now;
			struct timespec timeout;

			// FIXME: there should be more accurate way
			gettimeofday(&now, NULL);
			timeout.tv_sec = now.tv_sec + (dwTimeOut / 1000);

			now.tv_usec += (dwTimeOut % 1000) * 1000;
			timeout.tv_sec += now.tv_usec / 1000000;
			timeout.tv_nsec = (now.tv_usec % 1000000) * 1000;

			ret = (pthread_rwlock_timedrdlock(hLock, &timeout) == 0);
#else
			for(int i = (dwTimeOut / 50) + 1; i > 0; i--) 
         {
				if (pthread_rwlock_tryrdlock(hLock) == 0) 
            {
					ret = TRUE;
					break;
				}
				ThreadSleepMs(50);
			}
#endif
		}
	}
   return ret;
}

inline BOOL RWLockWriteLock(RWLOCK hLock, UINT32 dwTimeOut)
{
	BOOL ret = FALSE;

   if (hLock != NULL) 
   {
		if (dwTimeOut == INFINITE)
		{
			if (pthread_rwlock_wrlock(hLock) == 0) 
         {
				ret = TRUE;
			}
		}
		else
		{
#if HAVE_PTHREAD_RWLOCK_TIMEDWRLOCK
			struct timeval now;
			struct timespec timeout;

			// FIXME: there should be more accurate way
			gettimeofday(&now, NULL);
			timeout.tv_sec = now.tv_sec + (dwTimeOut / 1000);

			now.tv_usec += (dwTimeOut % 1000) * 1000;
			timeout.tv_sec += now.tv_usec / 1000000;
			timeout.tv_nsec = (now.tv_usec % 1000000) * 1000;

			ret = (pthread_rwlock_timedwrlock(hLock, &timeout) == 0);
#else
			for(int i = (dwTimeOut / 50) + 1; i > 0; i--) 
         {
				if (pthread_rwlock_trywrlock(hLock) == 0) 
            {
					ret = TRUE;
					break;
				}
				ThreadSleepMs(50);
			}
#endif
		}
	}
   return ret;
}

inline void RWLockUnlock(RWLOCK hLock)
{
   if (hLock != NULL)
      pthread_rwlock_unlock(hLock);
}

#elif defined(_USE_GNU_PTH)

typedef pth_rwlock_t * RWLOCK;

inline RWLOCK RWLockCreate(void)
{
   RWLOCK hLock;

   hLock = (RWLOCK)malloc(sizeof(pth_rwlock_t));
   if (!pth_rwlock_init(hLock))
   {
      free(hLock);
      hLock = NULL;
   }
   return hLock;
}

inline void RWLockDestroy(RWLOCK hLock)
{
   if (hLock != NULL)
   {
      free(hLock);
   }
}

inline BOOL RWLockReadLock(RWLOCK hLock, UINT32 dwTimeOut)
{
	BOOL ret = FALSE;

   if (hLock != NULL) 
   {
		if (dwTimeOut == INFINITE)
		{
			if (pth_rwlock_acquire(hLock, PTH_RWLOCK_RD, FALSE, NULL)) 
         {
				ret = TRUE;
			}
		}
		else
		{
         pth_event_t ev;

         ev = pth_event(PTH_EVENT_TIME, pth_timeout(dwTimeOut / 1000, (dwTimeOut % 1000) * 1000));
			if (pth_rwlock_acquire(hLock, PTH_RWLOCK_RD, FALSE, ev)) 
         {
				ret = TRUE;
			}
         pth_event_free(ev, PTH_FREE_ALL);
		}
	}
   return ret;
}

inline BOOL RWLockWriteLock(RWLOCK hLock, UINT32 dwTimeOut)
{
	BOOL ret = FALSE;

   if (hLock != NULL) 
   {
		if (dwTimeOut == INFINITE)
		{
			if (pth_rwlock_acquire(hLock, PTH_RWLOCK_RW, FALSE, NULL)) 
         {
				ret = TRUE;
			}
		}
		else
		{
         pth_event_t ev;

         ev = pth_event(PTH_EVENT_TIME, pth_timeout(dwTimeOut / 1000, (dwTimeOut % 1000) * 1000));
			if (pth_rwlock_acquire(hLock, PTH_RWLOCK_RW, FALSE, ev)) 
         {
				ret = TRUE;
			}
         pth_event_free(ev, PTH_FREE_ALL);
		}
	}
   return ret;
}

inline void RWLockUnlock(RWLOCK hLock)
{
   if (hLock != NULL)
      pth_rwlock_release(hLock);
}

#else    /* not HAVE_PTHREAD_RWLOCK and not defined _USE_GNU_PTH */

struct __rwlock_data
{
#ifdef _WIN32
   HANDLE m_mutex;
   HANDLE m_condRead;
   HANDLE m_condWrite;
#else
   pthread_mutex_t m_mutex;
   pthread_cond_t m_condRead;
   pthread_cond_t m_condWrite;
#endif
   UINT32 m_dwWaitReaders;
   UINT32 m_dwWaitWriters;
   int m_iRefCount;  // -1 for write lock, otherwise number of read locks
};

typedef struct __rwlock_data * RWLOCK;

RWLOCK LIBNETXMS_EXPORTABLE RWLockCreate();
void LIBNETXMS_EXPORTABLE RWLockDestroy(RWLOCK hLock);
BOOL LIBNETXMS_EXPORTABLE RWLockReadLock(RWLOCK hLock, UINT32 dwTimeOut);
BOOL LIBNETXMS_EXPORTABLE RWLockWriteLock(RWLOCK hLock, UINT32 dwTimeOut);
void LIBNETXMS_EXPORTABLE RWLockUnlock(RWLOCK hLock);

#endif

inline void RWLockPreemptiveWriteLock(RWLOCK hLock, UINT32 dwTimeout, UINT32 dwSleep)
{
	while(!RWLockWriteLock(hLock, dwTimeout))
		ThreadSleepMs(dwSleep);
}

#endif
