/*
** NetXMS - Network Management System
** Copyright (C) 2003-2023 Victor Kirhenshtein
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
** File: winrwlock.h
**
**/

#ifndef _winrwlock_h_
#define _winrwlock_h_

/**
 * R/W lock data structure
 */
struct win_rwlock_t
{
   CRITICAL_SECTION mutex;
   CONDITION_VARIABLE condRead;
   CONDITION_VARIABLE condWrite;
   int32_t refCount;  // -1 for write lock, otherwise number of read locks
   uint32_t waitReaders;
   uint32_t waitWriters;
   uint32_t writerThreadId;
};

/**
 * Initialize r/w lock
 */
static inline void InitializeRWLock(win_rwlock_t *rwlock)
{
   InitializeCriticalSectionAndSpinCount(&rwlock->mutex, 4000);
   InitializeConditionVariable(&rwlock->condRead);
   InitializeConditionVariable(&rwlock->condWrite);
   rwlock->refCount = 0;
   rwlock->waitReaders = 0;
   rwlock->waitWriters = 0;
   rwlock->writerThreadId = 0;
}

/**
 * Destroy r/w lock
 */
static inline void DestroyRWLock(win_rwlock_t *rwlock)
{
   DeleteCriticalSection(&rwlock->mutex);
}

/**
 * Read lock
 */
static inline void ReadLockRWLock(win_rwlock_t *rwlock)
{
   EnterCriticalSection(&rwlock->mutex);
   while ((rwlock->refCount == -1) || (rwlock->waitWriters > 0))
   {
      // Object is locked for writing or somebody wish to lock it for writing
      rwlock->waitReaders++;
      SleepConditionVariableCS(&rwlock->condRead, &rwlock->mutex, INFINITE);
      rwlock->waitReaders--;
   }
   rwlock->refCount++;
   LeaveCriticalSection(&rwlock->mutex);
}

/**
 * Write lock
 */
static inline void WriteLockRWLock(win_rwlock_t *rwlock)
{
   EnterCriticalSection(&rwlock->mutex);
   while (rwlock->refCount != 0)
   {
      // Object is locked, wait for unlock
      rwlock->waitWriters++;
      SleepConditionVariableCS(&rwlock->condWrite, &rwlock->mutex, INFINITE);
      rwlock->waitWriters--;
   }
   rwlock->refCount--;
   rwlock->writerThreadId = GetCurrentThreadId();
   LeaveCriticalSection(&rwlock->mutex);
}

/**
 * Unlock
 */
static inline void UnlockRWLock(win_rwlock_t *rwlock)
{
   EnterCriticalSection(&rwlock->mutex);

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
         WakeConditionVariable(&rwlock->condWrite);
      }
   }
   else if (rwlock->waitReaders > 0)
   {
      WakeAllConditionVariable(&rwlock->condRead);
   }

   LeaveCriticalSection(&rwlock->mutex);
}

#endif
