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

#ifndef _ssh_subagent_h
#define _ssh_subagent_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nms_agent.h>
#include <libssh/libssh.h>

/**
 * SSH session
 */
class SSHSession
{
private:
   InetAddress m_addr;
   unsigned int m_port;
   ssh_session m_session;

public:
   SSHSession(const InetAddress& addr, UINT16 port);
   ~SSHSession();

   bool connect(const TCHAR *user, const TCHAR *password);
   void disconnect();
   StringList *execute(const TCHAR *command);
};

/* handlers */
LONG H_SSHCommand(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_SSHCommandList(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session);

#endif
