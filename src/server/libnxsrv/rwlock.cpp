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
** $module: rwlock.cpp
**
**/

#include "libnxsrv.h"


//
// Create read/write lock
//

RWLOCK LIBNXSRV_EXPORTABLE RWLockCreate(void)
{
   RWLOCK hLock;

   hLock = (RWLOCK)malloc(sizeof(struct __rwlock_data));
   if (hLock != NULL)
   {
      hLock->m_mutex = MutexCreate();
      hLock->m_condRead = ConditionCreate(TRUE);
      hLock->m_condWrite = ConditionCreate(FALSE);
      hLock->m_dwWaitReaders = 0;
      hLock->m_dwWaitWriters = 0;
      hLock->m_iRefCount = 0;
   }

   return hLock;
}


//
// Destroy read/write lock
//

void LIBNXSRV_EXPORTABLE RWLockDestroy(RWLOCK hLock)
{
   if (hLock != NULL)
   {
      if (hLock->m_iRefCount == 0)
      {
         MutexDestroy(hLock->m_mutex);
         ConditionDestroy(hLock->m_condRead);
         ConditionDestroy(hLock->m_condWrite);
      }
   }
}


//
// Lock read/write lock for reading
//

BOOL LIBNXSRV_EXPORTABLE RWLockReadLock(RWLOCK hLock, DWORD dwTimeOut)
{
   BOOL bResult = FALSE;

   return bResult;
}


//
// Lock read/write lock for writing
//

BOOL LIBNXSRV_EXPORTABLE RWLockWriteLock(RWLOCK hLock, DWORD dwTimeOut)
{
   BOOL bResult = FALSE;

   return bResult;
}


//
// Unlock read/write lock
//

void LIBNXSRV_EXPORTABLE RWLockUnlock(RWLOCK hLock)
{
   // Check if handle is valid
   if (hLock == NULL)
      return;

   // Acquire access to handle
   if (!MutexLock(hLock->m_mutex, INFINITE))
      return;     // Problem with mutex

   // Remove lock
   if (hLock->m_iRefCount > 0)
      hLock->m_iRefCount--;
   else if (hLock->m_iRefCount == -1)
      hLock->m_iRefCount = 0;

   // Notify waiting threads
   if (hLock->m_dwWaitWriters > 0)
   {
      if (hLock->m_iRefCount == 0)
         ConditionSet(hLock->m_condWrite);
   }
   else if (hLock->m_dwWaitReaders > 0)
   {
      ConditionSet(hLock->m_condRead);
   }

   MutexUnlock(hLock->m_mutex);
}
