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
** File: rwlock.cpp
**
**/

#include "libnetxms.h"
#include <assert.h>

#if !HAVE_PTHREAD_RWLOCK && !defined(_USE_GNU_PTH)

//
// Create read/write lock
//

RWLOCK LIBNETXMS_EXPORTABLE RWLockCreate(void)
{
   RWLOCK hLock;

   hLock = (RWLOCK)malloc(sizeof(struct __rwlock_data));
   if (hLock != NULL)
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
      hLock->m_dwWaitReaders = 0;
      hLock->m_dwWaitWriters = 0;
      hLock->m_iRefCount = 0;
   }

   return hLock;
}


//
// Destroy read/write lock
//

void LIBNETXMS_EXPORTABLE RWLockDestroy(RWLOCK hLock)
{
   if (hLock != NULL)
   {
      if (hLock->m_iRefCount == 0)
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
         free(hLock);
      }
   }
}

/**
 * Lock read/write lock for reading
 */
BOOL LIBNETXMS_EXPORTABLE RWLockReadLock(RWLOCK hLock, UINT32 dwTimeOut)
{
   BOOL bResult = FALSE;
   int retcode;

   // Check if handle is valid
   if (hLock == NULL)
      return FALSE;

#ifdef _WIN32
   UINT32 dwStart, dwElapsed;
   BOOL bTimeOut = FALSE;

   // Acquire access to handle
   WaitForSingleObject(hLock->m_mutex, INFINITE);

   do
   {
      if ((hLock->m_iRefCount == -1) || (hLock->m_dwWaitWriters > 0))
      {
         // Object is locked for writing or somebody wish to lock it for writing
         hLock->m_dwWaitReaders++;
         ReleaseMutex(hLock->m_mutex);
         dwStart = GetTickCount();
         retcode = WaitForSingleObject(hLock->m_condRead, dwTimeOut);
         dwElapsed = GetTickCount() - dwStart;
         WaitForSingleObject(hLock->m_mutex, INFINITE);   // Re-acquire mutex
         hLock->m_dwWaitReaders--;
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
         hLock->m_iRefCount++;
         bResult = TRUE;
      }
   } while((!bResult) && (!bTimeOut));

   ReleaseMutex(hLock->m_mutex);
#else
   // Acquire access to handle
   if (pthread_mutex_lock(&hLock->m_mutex) != 0)
      return FALSE;     // Problem with mutex

   if ((hLock->m_iRefCount == -1) || (hLock->m_dwWaitWriters > 0))
   {
      // Object is locked for writing or somebody wish to lock it for writing
      hLock->m_dwWaitReaders++;

      if (dwTimeOut == INFINITE)
      {
         retcode = pthread_cond_wait(&hLock->m_condRead, &hLock->m_mutex);
      }
      else
      {
#if HAVE_PTHREAD_COND_RELTIMEDWAIT_NP
		   struct timespec timeout;

		   timeout.tv_sec = dwTimeOut / 1000;
		   timeout.tv_nsec = (dwTimeOut % 1000) * 1000000;
		   retcode = pthread_cond_reltimedwait_np(&hLock->m_condRead, &hLock->m_mutex, &timeout);
#else
		   struct timeval now;
		   struct timespec timeout;

		   gettimeofday(&now, NULL);
		   timeout.tv_sec = now.tv_sec + (dwTimeOut / 1000);
		   timeout.tv_nsec = ( now.tv_usec + ( dwTimeOut % 1000 ) * 1000) * 1000;
		   retcode = pthread_cond_timedwait(&hLock->m_condRead, &hLock->m_mutex, &timeout);
#endif
      }

      hLock->m_dwWaitReaders--;
      if (retcode == 0)
      {
         assert(hLock->m_iRefCount != -1);
         hLock->m_iRefCount++;
         bResult = TRUE;
      }
   }
   else
   {
      hLock->m_iRefCount++;
      bResult = TRUE;
   }

   pthread_mutex_unlock(&hLock->m_mutex);
#endif

   return bResult;
}


//
// Lock read/write lock for writing
//

BOOL LIBNETXMS_EXPORTABLE RWLockWriteLock(RWLOCK hLock, UINT32 dwTimeOut)
{
   BOOL bResult = FALSE;
   int retcode;

   // Check if handle is valid
   if (hLock == NULL)
      return FALSE;

#ifdef _WIN32
   UINT32 dwStart, dwElapsed;
   BOOL bTimeOut = FALSE;

   WaitForSingleObject(hLock->m_mutex, INFINITE);
   // Reset reader event because it can be set by previous Unlock() call
   ResetEvent(hLock->m_condRead);

   do
   {
      if (hLock->m_iRefCount != 0)
      {
         hLock->m_dwWaitWriters++;
         ReleaseMutex(hLock->m_mutex);
         dwStart = GetTickCount();
         retcode = WaitForSingleObject(hLock->m_condWrite, dwTimeOut);
         dwElapsed = GetTickCount() - dwStart;
         WaitForSingleObject(hLock->m_mutex, INFINITE);   // Re-acquire mutex
         hLock->m_dwWaitWriters--;
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
         hLock->m_iRefCount--;
         bResult = TRUE;
      }
   } while((!bResult) && (!bTimeOut));

   ReleaseMutex(hLock->m_mutex);
#else
   if (pthread_mutex_lock(&hLock->m_mutex) != 0)
      return FALSE;     // Problem with mutex

   if (hLock->m_iRefCount != 0)
   {
      // Object is locked, wait for unlock
      hLock->m_dwWaitWriters++;

      if (dwTimeOut == INFINITE)
      {
         retcode = pthread_cond_wait(&hLock->m_condWrite, &hLock->m_mutex);
      }
      else
      {
#if HAVE_PTHREAD_COND_RELTIMEDWAIT_NP
		   struct timespec timeout;

		   timeout.tv_sec = dwTimeOut / 1000;
		   timeout.tv_nsec = (dwTimeOut % 1000) * 1000000;
		   retcode = pthread_cond_reltimedwait_np(&hLock->m_condWrite, &hLock->m_mutex, &timeout);
#else
		   struct timeval now;
		   struct timespec timeout;

		   gettimeofday(&now, NULL);
		   timeout.tv_sec = now.tv_sec + (dwTimeOut / 1000);
		   timeout.tv_nsec = ( now.tv_usec + ( dwTimeOut % 1000 ) * 1000) * 1000;
		   retcode = pthread_cond_timedwait(&hLock->m_condWrite, &hLock->m_mutex, &timeout);
#endif
      }

      hLock->m_dwWaitWriters--;
      if (retcode == 0)
      {
         assert(hLock->m_iRefCount == 0);
         hLock->m_iRefCount--;
         bResult = TRUE;
      }
   }
   else
   {
      hLock->m_iRefCount--;
      bResult = TRUE;
   }

   pthread_mutex_unlock(&hLock->m_mutex);
#endif

   return bResult;
}


//
// Unlock read/write lock
//

void LIBNETXMS_EXPORTABLE RWLockUnlock(RWLOCK hLock)
{
   // Check if handle is valid
   if (hLock == NULL)
      return;

   // Acquire access to handle
#ifdef _WIN32
   WaitForSingleObject(hLock->m_mutex, INFINITE);
#else
   if (pthread_mutex_lock(&hLock->m_mutex) != 0)
      return;     // Problem with mutex
#endif

   // Remove lock
   if (hLock->m_iRefCount > 0)
      hLock->m_iRefCount--;
   else if (hLock->m_iRefCount == -1)
      hLock->m_iRefCount = 0;

   // Notify waiting threads
   if (hLock->m_dwWaitWriters > 0)
   {
      if (hLock->m_iRefCount == 0)
#ifdef _WIN32
         SetEvent(hLock->m_condWrite);
#else
         pthread_cond_signal(&hLock->m_condWrite);
#endif
   }
   else if (hLock->m_dwWaitReaders > 0)
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
