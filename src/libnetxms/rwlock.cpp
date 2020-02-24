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
** File: rwlock.cpp
**
**/

#include "libnetxms.h"

#if !HAVE_PTHREAD_RWLOCK && !defined(_USE_GNU_PTH)

/**
 * Create read/write lock
 */
RWLOCK LIBNETXMS_EXPORTABLE RWLockCreate()
{
   RWLOCK hLock = MemAllocStruct<__rwlock_data>();
   if (hLock == nullptr)
   {
#ifdef _WIN32
      hLock->m_mutex = CreateMutex(NULL, FALSE, NULL);
      hLock->m_condRead = CreateEvent(NULL, TRUE, FALSE, NULL);
      hLock->m_condWrite = CreateEvent(NULL, FALSE, FALSE, NULL);
#else
      pthread_mutex_init(&hLock->m_mutex, NULL);
      pthread_cond_init(&hLock->m_condRead, NULL);
      pthread_cond_init(&hLock->m_condWrite, NULL);
#endif
   }
   return hLock;
}

/**
 * Destroy read/write lock
 */
void LIBNETXMS_EXPORTABLE RWLockDestroy(RWLOCK hLock)
{
   if ((hLock != nullptr) && (hLock->m_refCount == 0))
   {
#ifdef _WIN32
      CloseHandle(hLock->m_mutex);
      CloseHandle(hLock->m_condRead);
      CloseHandle(hLock->m_condWrite);
#else
      pthread_mutex_destroy(&hLock->m_mutex);
      pthread_cond_destroy(&hLock->m_condRead);
      pthread_cond_destroy(&hLock->m_condWrite);
#endif
      MemFree(hLock);
   }
}

/**
 * Lock read/write lock for reading
 */
bool LIBNETXMS_EXPORTABLE RWLockReadLock(RWLOCK hLock)
{
   if (hLock == nullptr)
      return false;

#ifdef _WIN32
   UINT32 dwStart, dwElapsed;
   BOOL bTimeOut = FALSE;

   // Acquire access to handle
   WaitForSingleObject(hLock->m_mutex, INFINITE);

   do
   {
      if ((hLock->m_refCount == -1) || (hLock->m_waitWriters > 0))
      {
         // Object is locked for writing or somebody wish to lock it for writing
         hLock->m_waitReaders++;
         ReleaseMutex(hLock->m_mutex);
         dwStart = GetTickCount();
         retcode = WaitForSingleObject(hLock->m_condRead, dwTimeOut);
         dwElapsed = GetTickCount() - dwStart;
         WaitForSingleObject(hLock->m_mutex, INFINITE);   // Re-acquire mutex
         hLock->m_waitReaders--;
         if (retcode == WAIT_TIMEOUT)
         {
            bTimeOut = TRUE;
         }
         else
         {
            if (dwTimeOut != INFINITE)
            {
               dwTimeOut -= std::min(dwElapsed, dwTimeOut);
            }
         }
      }
      else
      {
         hLock->m_refCount++;
         bResult = TRUE;
      }
   } while((!bResult) && (!bTimeOut));

   ReleaseMutex(hLock->m_mutex);
#else
   if (pthread_mutex_lock(&hLock->m_mutex) != 0)
      return false;     // Problem with mutex

   while ((hLock->m_refCount == -1) || (hLock->m_waitWriters > 0))
   {
      // Object is locked for writing or somebody wish to lock it for writing
      hLock->m_waitReaders++;
      pthread_cond_wait(&hLock->m_condRead, &hLock->m_mutex);
      hLock->m_waitReaders--;
   }

   hLock->m_refCount++;
   pthread_mutex_unlock(&hLock->m_mutex);
#endif

   return true;
}

/**
 * Lock read/write lock for writing
 */
bool LIBNETXMS_EXPORTABLE RWLockWriteLock(RWLOCK hLock)
{
   if (hLock == nullptr)
      return false;

#ifdef _WIN32
   UINT32 dwStart, dwElapsed;
   BOOL bTimeOut = FALSE;

   WaitForSingleObject(hLock->m_mutex, INFINITE);
   // Reset reader event because it can be set by previous Unlock() call
   ResetEvent(hLock->m_condRead);

   do
   {
      if (hLock->m_refCount != 0)
      {
         hLock->m_waitWriters++;
         ReleaseMutex(hLock->m_mutex);
         dwStart = GetTickCount();
         retcode = WaitForSingleObject(hLock->m_condWrite, dwTimeOut);
         dwElapsed = GetTickCount() - dwStart;
         WaitForSingleObject(hLock->m_mutex, INFINITE);   // Re-acquire mutex
         hLock->m_waitWriters--;
         if (retcode == WAIT_TIMEOUT)
         {
            bTimeOut = TRUE;
         }
         else
         {
            if (dwTimeOut != INFINITE)
            {
               dwTimeOut -= std::min(dwElapsed, dwTimeOut);
            }
         }
      }
      else
      {
         hLock->m_refCount--;
         bResult = TRUE;
      }
   } while((!bResult) && (!bTimeOut));

   if (bResult)
      hLock->m_writerThreadId = GetCurrentThreadId();

   ReleaseMutex(hLock->m_mutex);
#else
   if (pthread_mutex_lock(&hLock->m_mutex) != 0)
      return false;     // Problem with mutex

   while (hLock->m_refCount != 0)
   {
      // Object is locked, wait for unlock
      hLock->m_waitWriters++;
      pthread_cond_wait(&hLock->m_condWrite, &hLock->m_mutex);
      hLock->m_waitWriters--;
   }

   hLock->m_refCount--;
   hLock->m_writerThreadId = GetCurrentThreadId();

   pthread_mutex_unlock(&hLock->m_mutex);
#endif

   return true;
}

/**
 * Unlock read/write lock
 */
void LIBNETXMS_EXPORTABLE RWLockUnlock(RWLOCK hLock)
{
   if (hLock == nullptr)
      return;

   // Acquire access to handle
#ifdef _WIN32
   WaitForSingleObject(hLock->m_mutex, INFINITE);
#else
   if (pthread_mutex_lock(&hLock->m_mutex) != 0)
      return;     // Problem with mutex
#endif

   // Remove lock
   if (hLock->m_refCount > 0)
   {
      hLock->m_refCount--;
   }
   else if (hLock->m_refCount == -1)
   {
      hLock->m_refCount = 0;
      hLock->m_writerThreadId = 0;
   }

   // Notify waiting threads
   if (hLock->m_waitWriters > 0)
   {
      if (hLock->m_refCount == 0)
#ifdef _WIN32
         SetEvent(hLock->m_condWrite);
#else
         pthread_cond_signal(&hLock->m_condWrite);
#endif
   }
   else if (hLock->m_waitReaders > 0)
   {
#ifdef _WIN32
      SetEvent(hLock->m_condRead);
#else
      pthread_cond_broadcast(&hLock->m_condRead);
#endif
   }

#ifdef _WIN32
   ReleaseMutex(hLock->m_mutex);
#else
   pthread_mutex_unlock(&hLock->m_mutex);
#endif
}

#endif   /* !HAVE_PTHREAD_RWLOCK */
