/*
** NetXMS SSH subagent
** Copyright (C) 2004-2025 Victor Kirhenshtein
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
static ObjectArray<SSHSession> s_sessions(16, 16, Ownership::True);
static Mutex s_lock(MutexType::FAST);
static VolatileCounter s_sessionId = 0;
static Condition s_shutdownCondition(true);
static THREAD s_housekeeperThread = INVALID_THREAD_HANDLE;

/**
 * Acquire SSH session
 */
SSHSession *AcquireSession(const InetAddress& addr, uint16_t port, const TCHAR *user, const TCHAR *password, const shared_ptr<KeyPair>& keys)
{
   s_lock.lock();
   for(int i = 0; i < s_sessions.size(); i++)
   {
      SSHSession *s = s_sessions.get(i);
      if (s->match(addr, port, user) && s->acquire())
      {
         nxlog_debug_tag(DEBUG_TAG, 7, _T("AcquireSession: acquired existing session %s"), s->getName());
         s_lock.unlock();
         return s;
      }
   }
   s_lock.unlock();

   // No matching sessions, create new one
   SSHSession *session = new SSHSession(addr, port, InterlockedIncrement(&s_sessionId));
   if (!session->connect(user, password, keys))
   {
      delete session;
      return nullptr;
   }
   nxlog_debug_tag(DEBUG_TAG, 7, _T("AcquireSession: created new session %s"), session->getName());

   session->acquire();
   s_lock.lock();
   s_sessions.add(session);
   s_lock.unlock();
   return session;
}

/**
 * Release SSH session
 */
void ReleaseSession(SSHSession *session)
{
   s_lock.lock();
   session->release();
   if (!session->isConnected())
   {
      nxlog_debug_tag(DEBUG_TAG, 7, _T("ReleaseSession: disconnected session %s removed"), session->getName());
      s_sessions.remove(session);
   }
   s_lock.unlock();
}

/**
 * Housekeeping thread
 */
static void HousekeeperThread()
{
   ObjectArray<SSHSession> deleteList(16, 16, Ownership::True);
   while(!s_shutdownCondition.wait(30000))
   {
      s_lock.lock();
      time_t now = time(nullptr);
      for(int i = 0; i < s_sessions.size(); i++)
      {
         SSHSession *s = s_sessions.get(i);
         if (!s->isBusy() && (!s->isConnected() || (now - s->getLastAccessTime() > g_sshSessionIdleTimeout)))
         {
            nxlog_debug_tag(DEBUG_TAG, 7, _T("HousekeeperThread: session %s removed by housekeeper"), s->getName());
            s_sessions.unlink(i);
            i--;
            deleteList.add(s);
         }
      }
      s_lock.unlock();
      deleteList.clear();
   }
}

/**
 * Initialize SSH session pool
 */
void InitializeSessionPool()
{
   s_housekeeperThread = ThreadCreateEx(HousekeeperThread);
   nxlog_debug_tag(DEBUG_TAG, 5, _T("InitializeSessionPool: connection pool initialized"));
}

/**
 * Shutdown SSH session pool
 */
void ShutdownSessionPool()
{
   s_shutdownCondition.set();
   ThreadJoin(s_housekeeperThread);

   s_lock.lock();
   s_sessions.clear();
   s_lock.unlock();

   nxlog_debug_tag(DEBUG_TAG, 5, _T("ShutdownSessionPool: connection pool closed"));
}
