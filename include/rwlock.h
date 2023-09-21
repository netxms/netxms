/*
** NetXMS - Network Management System
** Copyright (C) 2003-2022 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
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

/**
 * R/W lock data structure
 */
struct rwlock_t
{
   pthread_mutex_t mutex;
   pthread_cond_t condRead;
   pthread_cond_t condWrite;
   int32_t refCount;  // -1 for write lock, otherwise number of read locks
   uint32_t waitReaders;
   uint32_t waitWriters;
   uint32_t writerThreadId;
};

/**
 * Initialize r/w lock
 */
static inline void InitializeRWLock(rwlock_t *rwlock)
{
#if HAVE_DECL_PTHREAD_MUTEX_ADAPTIVE_NP
   pthread_mutexattr_t a;
   pthread_mutexattr_init(&a);
   MUTEXATTR_SETTYPE(&a, PTHREAD_MUTEX_ADAPTIVE_NP);
   pthread_mutex_init(&rwlock->mutex, &a);
   pthread_mutexattr_destroy(&a);
#else
   pthread_mutex_init(&rwlock->mutex, nullptr);
#endif
   pthread_cond_init(&rwlock->condRead, nullptr);
   pthread_cond_init(&rwlock->condWrite, nullptr);
   rwlock->refCount = 0;
   rwlock->waitReaders = 0;
   rwlock->waitWriters = 0;
   rwlock->writerThreadId = 0;
}

/**
 * Destroy r/w lock
 */
static inline void DestroyRWLock(rwlock_t *rwlock)
{
   pthread_mutex_destroy(&rwlock->mutex);
   pthread_cond_destroy(&rwlock->condRead);
   pthread_cond_destroy(&rwlock->condWrite);
}

/**
 * Read lock
 */
static inline void ReadLockRWLock(rwlock_t *rwlock)
{
   pthread_mutex_lock(&rwlock->mutex);
   while ((rwlock->refCount == -1) || (rwlock->waitWriters > 0))
   {
      // Object is locked for writing or somebody wish to lock it for writing
      rwlock->waitReaders++;
      pthread_cond_wait(&rwlock->condRead, &rwlock->mutex);
      rwlock->waitReaders--;
   }
   rwlock->refCount++;
   pthread_mutex_unlock(&rwlock->mutex);
}

/**
 * Write lock
 */
static inline void WriteLockRWLock(rwlock_t *rwlock)
{
   pthread_mutex_lock(&rwlock->mutex);
   while (rwlock->refCount != 0)
   {
      // Object is locked, wait for unlock
      rwlock->waitWriters++;
      pthread_cond_wait(&rwlock->condWrite, &rwlock->mutex);
      rwlock->waitWriters--;
   }
   rwlock->refCount--;
   rwlock->writerThreadId = GetCurrentThreadId();
   pthread_mutex_unlock(&rwlock->mutex);
}

/**
 * Unlock
 */
static inline void UnlockRWLock(rwlock_t *rwlock)
{
   // Acquire access to handle
   pthread_mutex_lock(&rwlock->mutex);

   // Remove lock
   if (rwlock->refCount > 0)
   {
      rwlock->refCount--;
   }
   else if (rwlock->refCount == -1)
   {
      rwlock->refCount = 0;
      rwlock->writerThreadId = 0;
   }

   // Notify waiting threads
   if (rwlock->waitWriters > 0)
   {
      if (rwlock->refCount == 0)
      {
         pthread_cond_signal(&rwlock->condWrite);
      }
   }
   else if (rwlock->waitReaders > 0)
   {
      pthread_cond_broadcast(&rwlock->condRead);
   }

   pthread_mutex_unlock(&rwlock->mutex);
}

#endif
