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
** File: ssh.h
**
**/

#ifndef _ssh_subagent_h_
#define _ssh_subagent_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nms_agent.h>
#include <libssh/libssh.h>

#define MAX_SSH_SESSION_NAME_LEN (MAX_SSH_LOGIN_LEN + MAX_DNS_NAME + 16)

/**
 * SSH session
 */
class SSHSession
{
private:
   INT32 m_id;
   InetAddress m_addr;
   unsigned int m_port;
   TCHAR m_user[MAX_SSH_LOGIN_LEN];
   ssh_session m_session;
   time_t m_lastAccess;
   bool m_busy;
   TCHAR m_name[MAX_SSH_SESSION_NAME_LEN];

public:
   SSHSession(const InetAddress& addr, UINT16 port, INT32 id = 0);
   ~SSHSession();

   bool connect(const TCHAR *user, const TCHAR *password);
   void disconnect();
   bool isConnected() const { return (m_session != NULL) && ssh_is_connected(m_session); }

   const TCHAR *getName() const { return m_name; }
   time_t getLastAccessTime() const { return m_lastAccess; }
   bool isBusy() const { return m_busy; }

   bool match(const InetAddress& addr, UINT16 port, const TCHAR *user) const;

   bool acquire();
   void release();

   StringList *execute(const TCHAR *command);
};

/* Session pool */
void InitializeSessionPool();
void ShutdownSessionPool();
SSHSession *AcquireSession(const InetAddress& addr, UINT16 port, const TCHAR *user, const TCHAR *password);
void ReleaseSession(SSHSession *session);

/* handlers */
LONG H_SSHCommand(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_SSHCommandList(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session);

/* globals */
extern UINT32 g_sshConnectTimeout;
extern UINT32 g_sshSessionIdleTimeout;

#endif
