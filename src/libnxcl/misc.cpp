/* 
** NetXMS - Network Management System
** Client Library
** Copyright (C) 2004 Victor Kirhenshtein
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
** $module: misc.cpp
**
**/

#include "libnxcl.h"
#include <stdarg.h>


//
// Static data
//

#ifdef _WIN32
static HANDLE m_condSyncOp;
#else
static pthread_cond_t m_condSyncOp;
static pthread_mutex_t m_mutexSyncOp;
static BOOL m_bSyncFinished;
#endif
static DWORD m_dwSyncExitCode;


//
// Change library state and notify client
//

void ChangeState(DWORD dwState)
{
   g_dwState = dwState;
   CallEventHandler(NXC_EVENT_STATE_CHANGED, dwState, NULL);
}


//
// Print debug messages
//

void DebugPrintf(char *szFormat, ...)
{
   va_list args;
   char szBuffer[4096];

   if (g_pDebugCallBack == NULL)
      return;

   va_start(args, szFormat);
   vsprintf(szBuffer, szFormat, args);
   va_end(args);
   g_pDebugCallBack(szBuffer);
}


//
// Initialize synchronization objects
//

void InitSyncStuff(void)
{
#ifdef _WIN32
   m_condSyncOp = CreateEvent(NULL, FALSE, FALSE, NULL);
#else
   pthread_mutex_init(&m_mutexSyncOp, NULL);
   pthread_cond_init(&m_condSyncOp, NULL);
#endif
}


//
// Delete synchronization objects
//

void SyncCleanup(void)
{
#ifdef _WIN32
   CloseHandle(m_condSyncOp);
#else
   pthread_mutex_destroy(&m_mutexSyncOp);
   pthread_cond_destroy(&m_condSyncOp);
#endif
}


//
// Wait for synchronization operation completion
//

DWORD WaitForSync(DWORD dwTimeOut)
{
#ifdef _WIN32
   DWORD dwRetCode;

   dwRetCode = WaitForSingleObject(m_condSyncOp, dwTimeOut);
   return (dwRetCode == WAIT_TIMEOUT) ? RCC_TIMEOUT : m_dwSyncExitCode;
#else
   int iRetCode;
   DWORD dwResult;

   pthread_mutex_lock(&m_mutexSyncOp);
   if (!m_bSyncFinished)
   {
      if (dwTimeOut != INFINITE)
	   {
#if HAVE_PTHREAD_COND_RELTIMEDWAIT_NP
		   struct timespec timeout;

		   timeout.tv_sec = dwTimeOut / 1000;
		   timeout.tv_nsec = (dwTimeOut % 1000) * 1000000;
		   iRetCode = pthread_cond_reltimedwait_np(&m_condSyncOp, &m_mutexSyncOp, &timeout);
#else
		   struct timeval now;
		   struct timespec timeout;

		   gettimeofday(&now, NULL);
		   timeout.tv_sec = now.tv_sec + (dwTimeOut / 1000);
		   timeout.tv_nsec = ( now.tv_usec + ( dwTimeOut % 1000 ) * 1000) * 1000;
		   iRetCode = pthread_cond_timedwait(&m_condSyncOp, &m_mutexSyncOp, &timeout);
#endif
	   }
	   else
      {
         iRetCode = pthread_cond_wait(&m_condSyncOp, &m_mutexSyncOp);
      }
      dwResult = (iRetCode == 0) ? m_dwSyncExitCode : RCC_TIMEOUT;
   }
   else
   {
      dwResult = m_dwSyncExitCode;
   }
   pthread_mutex_unlock(&m_mutexSyncOp);
   return dwResult;
#endif
}


//
// Prepare for synchronization operation
//

void PrepareForSync(void)
{
   m_dwSyncExitCode = RCC_SYSTEM_FAILURE;
#ifdef _WIN32
   ResetEvent(m_condSyncOp);
#else
   m_bSyncFinished = FALSE;
#endif
}


//
// Complete synchronization operation
//

void CompleteSync(DWORD dwRetCode)
{
#ifdef _WIN32
   m_dwSyncExitCode = dwRetCode;
   SetEvent(m_condSyncOp);
#else
   pthread_mutex_lock(&m_mutexSyncOp);
   m_dwSyncExitCode = dwRetCode;
   m_bSyncFinished = TRUE;
   pthread_cond_signal(&m_condSyncOp);
   pthread_mutex_unlock(&m_mutexSyncOp);
#endif
}
