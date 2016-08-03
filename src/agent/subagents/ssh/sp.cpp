/*
** NetXMS SSH subagent
** Copyright (C) 2004-2016 Victor Kirhenshtein
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
** File: sp.cpp
**/

#include "ssh_subagent.h"

/**
 * Session pool
 */
static ObjectArray<SSHSession> s_sessions(16, 16, true);
static MUTEX s_lock = MutexCreate();
static VolatileCounter s_sessionId = 0;
static CONDITION s_shutdownCondition = ConditionCreate(TRUE);
static THREAD s_housekeeperThread = INVALID_THREAD_HANDLE;

/**
 * Acquire SSH session
 */
SSHSession *AcquireSession(const InetAddress& addr, UINT16 port, const TCHAR *user, const TCHAR *password)
{
   MutexLock(s_lock);
   for(int i = 0; i < s_sessions.size(); i++)
   {
      SSHSession *s = s_sessions.get(i);
      if (s->match(addr, port, user) && s->acquire())
      {
         nxlog_debug(7, _T("SSH: acquired existing session %s"), s->getName());
         MutexUnlock(s_lock);
         return s;
      }
   }
   MutexUnlock(s_lock);

   // No matching sessions, create new one
   SSHSession *session = new SSHSession(addr, port, InterlockedIncrement(&s_sessionId));
   if (!session->connect(user, password))
   {
      delete session;
      return NULL;
   }
   nxlog_debug(7, _T("SSH: created new session %s"), session->getName());

   session->acquire();
   MutexLock(s_lock);
   s_sessions.add(session);
   MutexUnlock(s_lock);
   return session;
}

/**
 * Release SSH session
 */
void ReleaseSession(SSHSession *session)
{
   MutexLock(s_lock);
   session->release();
   if (!session->isConnected())
   {
      nxlog_debug(7, _T("SSH: disconnected session %s removed"), session->getName());
      s_sessions.remove(session);
   }
   MutexUnlock(s_lock);
}

/**
 * Housekeeping thread
 */
static THREAD_RESULT THREAD_CALL HousekeeperThread(void *arg)
{
   ObjectArray<SSHSession> deleteList(16, 16, true);
   while(!ConditionWait(s_shutdownCondition, 30000))
   {
      MutexLock(s_lock);
      time_t now = time(NULL);
      for(int i = 0; i < s_sessions.size(); i++)
      {
         SSHSession *s = s_sessions.get(i);
         if (!s->isBusy() && (!s->isConnected() || (now - s->getLastAccessTime() > g_sshSessionIdleTimeout)))
         {
            nxlog_debug(7, _T("SSH: session %s removed by housekeeper"), s->getName());
            s_sessions.unlink(i);
            i--;
            deleteList.add(s);
         }
      }
      MutexUnlock(s_lock);
      deleteList.clear();
   }
   return THREAD_OK;
}

/**
 * Initialize SSH session pool
 */
void InitializeSessionPool()
{
   s_housekeeperThread = ThreadCreateEx(HousekeeperThread, 0, NULL);
   nxlog_debug(5, _T("SSH: connection pool initialized"));
}

/**
 * Shutdown SSH session pool
 */
void ShutdownSessionPool()
{
   ConditionSet(s_shutdownCondition);
   ThreadJoin(s_housekeeperThread);

   MutexLock(s_lock);
   s_sessions.clear();
   MutexUnlock(s_lock);

   nxlog_debug(5, _T("SSH: connection pool closed"));
}
