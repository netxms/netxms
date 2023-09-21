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
** File: winmutex.h
**
**/

#ifndef _winmutex_h_
#define _winmutex_h_

#include <intrin.h>
#pragma intrinsic(_WriteBarrier)
#pragma intrinsic(_ReadWriteBarrier)

/**
 * Mutex data structure
 */
struct win_mutex_t
{
   volatile DWORD owner;
   volatile DWORD lockCount;
   volatile DWORD spinCount;
   volatile HANDLE condition;
};

/**
 * Initialize mutex
 */
static inline void InitializeMutex(win_mutex_t *m, DWORD spinCount)
{
   memset(m, 0, sizeof(win_mutex_t));
   m->spinCount = spinCount;
}

/**
 * Destroy mutex
 */
static inline void DestroyMutex(win_mutex_t *m)
{
   if (m->condition != nullptr)
      CloseHandle(m->condition);
}

/**
 * Try to lock mutex
 */
static inline bool TryLockMutex(win_mutex_t *m)
{
   DWORD thisThread = GetCurrentThreadId();

   DWORD owner = InterlockedCompareExchange(&m->owner, thisThread, 0);
   if ((owner == 0) || (owner == thisThread))
   {
      m->lockCount++;
      return true;  // Locked on first attempt
   }

   DWORD spinCount = m->spinCount;
   while (spinCount-- > 0)
   {
      if (InterlockedCompareExchange(&m->owner, thisThread, 0) == 0)
      {
         m->lockCount++;
         return true;
      }
      YieldProcessor();
   }

   return false;
}

/**
 * Lock mutex
 */
static inline bool LockMutex(win_mutex_t *m, DWORD timeout)
{
   if (TryLockMutex(m))
      return true;

   if (m->condition == nullptr)
   {
      HANDLE condition = CreateEvent(nullptr, FALSE, FALSE, nullptr);
      if (InterlockedCompareExchangePointer(&m->condition, condition, nullptr) != nullptr)
         CloseHandle(condition);    // condition already created by other thread
      if (TryLockMutex(m))
         return true;
   }

   while (true)
   {
      if (WaitForSingleObject(m->condition, timeout) != WAIT_OBJECT_0)
         return false;
      if (TryLockMutex(m))
         return true;
   }
}

/**
 * Unlock mutex
 */
static inline void UnlockMutex(win_mutex_t *m)
{
   m->lockCount--;
   if (m->lockCount == 0)
   {
      _WriteBarrier();
      m->owner = 0;
      _ReadWriteBarrier();
      if (m->condition != nullptr)
         SetEvent(m->condition);
   }
}

#endif
