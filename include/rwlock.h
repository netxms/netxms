/* 
** NetXMS - Network Management System
** NetXMS Foundation Library
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
** File: rwlock.h
**
**/

#ifndef _rwlock_h_
#define _rwlock_h_

#if HAVE_PTHREAD_RWLOCK

typedef pthread_rwlock_t * RWLOCK;

inline RWLOCK RWLockCreate(void)
{
   RWLOCK hLock = MemAllocStruct<pthread_rwlock_t>();
   if (pthread_rwlock_init(hLock, NULL) != 0)
   {
      MemFreeAndNull(hLock);
   }
   return hLock;
}

inline void RWLockDestroy(RWLOCK hLock)
{
   if (hLock != NULL)
   {
      pthread_rwlock_destroy(hLock);
      MemFree(hLock);
   }
}

inline bool RWLockReadLock(RWLOCK hLock)
{
   return (hLock != nullptr) ? (pthread_rwlock_rdlock(hLock) == 0) : false;
}

inline bool RWLockWriteLock(RWLOCK hLock)
{
   return (hLock != nullptr) ? (pthread_rwlock_wrlock(hLock) == 0) : false;
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
   RWLOCK hLock = MemAllocStruct<pth_rwlock_t>();
   if (!pth_rwlock_init(hLock))
   {
      MemFreeAndNull(hLock);
   }
   return hLock;
}

inline void RWLockDestroy(RWLOCK hLock)
{
   MemFree(hLock);
}

inline bool RWLockReadLock(RWLOCK hLock)
{
   return (hLock != nullptr) ? (pth_rwlock_acquire(hLock, PTH_RWLOCK_RD, FALSE, NULL) != 0) : false;
}

inline BOOL RWLockWriteLock(RWLOCK hLock)
{
   return (hLock != nullptr) ? (pth_rwlock_acquire(hLock, PTH_RWLOCK_RW, FALSE, NULL) != 0) : false;
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
   uint32_t m_waitReaders;
   uint32_t m_waitWriters;
   int m_refCount;  // -1 for write lock, otherwise number of read locks
   uint32_t m_writerThreadId;
};

typedef struct __rwlock_data * RWLOCK;

RWLOCK LIBNETXMS_EXPORTABLE RWLockCreate();
void LIBNETXMS_EXPORTABLE RWLockDestroy(RWLOCK hLock);
bool LIBNETXMS_EXPORTABLE RWLockReadLock(RWLOCK hLock);
bool LIBNETXMS_EXPORTABLE RWLockWriteLock(RWLOCK hLock);
void LIBNETXMS_EXPORTABLE RWLockUnlock(RWLOCK hLock);

#endif

#endif
