/* 
** NetXMS - Network Management System
** Server Library
** Copyright (C) 2003, 2004 Victor Kirhenshtein
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
** $module: rwlock.h
**
**/

#ifndef _rwlock_h_
#define _rwlock_h_

#include <nms_threads.h>

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

inline BOOL RWLockReadLock(RWLOCK hLock, DWORD dwTimeOut)
{
	int i;
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
			for (i = (dwTimeOut / 50) + 1; i > 0; i--) 
         {
				if (pthread_rwlock_tryrdlock(hLock) == 0) 
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

inline BOOL RWLockWriteLock(RWLOCK hLock, DWORD dwTimeOut)
{
	int i;
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
			for (i = (dwTimeOut / 50) + 1; i > 0; i--) 
         {
				if (pthread_rwlock_trywrlock(hLock) == 0) 
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

inline void RWLockUnlock(RWLOCK hLock)
{
   if (hLock != NULL)
      pthread_rwlock_unlock(hLock);
}

#else

struct __rwlock_data
{
#ifdef _WIN32
   HANDLE m_mutex;
   HANDLE m_condRead;
   HANDLE m_condWrite;
#else
   pthread_mutex_t mutex;
   pthread_cond_t m_condRead;
   pthread_cond_t m_condWrite;
#endif
   DWORD m_dwWaitReaders;
   DWORD m_dwWaitWriters;
   int m_iRefCount;  // -1 for write lock, otherwise number of read locks
};

typedef struct __rwlock_data * RWLOCK;

RWLOCK LIBNXSRV_EXPORTABLE RWLockCreate(void);
void LIBNXSRV_EXPORTABLE RWLockDestroy(RWLOCK hLock);
BOOL LIBNXSRV_EXPORTABLE RWLockReadLock(RWLOCK hLock, DWORD dwTimeOut);
BOOL LIBNXSRV_EXPORTABLE RWLockWriteLock(RWLOCK hLock, DWORD dwTimeOut);
void LIBNXSRV_EXPORTABLE RWLockUnlock(RWLOCK hLock);

#endif

#endif
