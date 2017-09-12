/*
** NetXMS - Network Management System
** NetXMS Foundation Library
** Copyright (C) 2003-2017 Victor Kirhenshtein
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
** File: nxproc_unix.cpp
**
**/

#include "libnetxms.h"
#include <nxproc.h>

/**
 * Create listener end for named pipe
 */
NamedPipe *NamedPipe::createListener(const TCHAR *name, NamedPipeRequestHandler reqHandler, void *userArg)
{
   mode_t prevMask = 0;

   int s = socket(AF_UNIX, SOCK_STREAM, 0);
   if (s == INVALID_SOCKET)
   {
      nxlog_debug(2, _T("NamedPipe(%s): socket() call failed (%s)"), name, _tcserror(errno));
      return NULL;
   }

   struct sockaddr_un addrLocal;
   addrLocal.sun_family = AF_UNIX;
#ifdef UNICODE
   sprintf(addrLocal.sun_path, "/tmp/.%S", name);
#else
   sprintf(addrLocal.sun_path, "/tmp/.%s", name);
#endif
   unlink(addrLocal.sun_path);
   prevMask = umask(S_IWGRP | S_IWOTH);
   if (bind(s, (struct sockaddr *)&addrLocal, SUN_LEN(&addrLocal)) == -1)
   {
      nxlog_debug(2, _T("NamedPipe(%s): bind failed (%s)"), name, _tcserror(errno));
      umask(prevMask);
      goto failure;
   }
   umask(prevMask);

   if (listen(s, 5) == -1)
   {
      nxlog_debug(2, _T("NamedPipe(%s): listen() call failed (%s)"), name, _tcserror(errno));
      goto failure;
   }

   return new NamedPipe(name, s, true, reqHandler, userArg);

failure:
   close(s);
   unlink(addrLocal.sun_path);
   return NULL;
}

/**
 * Create client end for named pipe
 */
NamedPipe *NamedPipe::connectTo(const TCHAR *name)
{
   int s = socket(AF_UNIX, SOCK_STREAM, 0);
   if (s == INVALID_SOCKET)
   {
      nxlog_debug(2, _T("NamedPipe(%s): socket() call failed (%s)"), name, _tcserror(errno));
      return NULL;
   }

   struct sockaddr_un remote;
   remote.sun_family = AF_UNIX;
#ifdef UNICODE
   sprintf(remote.sun_path, "/tmp/.%S", name);
#else
   sprintf(remote.sun_path, "/tmp/.%s", name);
#endif
   if (connect(s, (struct sockaddr *)&remote, SUN_LEN(&remote)) == -1)
   {
      close(s);
      return NULL;
   }

   return new NamedPipe(name, s, false);
}

/**
 * Pipe destructor
 */
NamedPipe::~NamedPipe()
{
   close(m_handle);
   if (m_isListener)
   {
      stopServer();
      char path[MAX_PATH];
#ifdef UNICODE
      sprintf(path, "/tmp/.%S", m_name);
#else
      sprintf(path, "/tmp/.%s", m_name);
#endif
      unlink(path);
   }
}

/**
 * Named pipe server thread
 */
void NamedPipe::serverThread()
{
   nxlog_debug(2, _T("NamedPipe(%s): waiting for connection"), m_name);
   while(!m_stop)
   {
      struct sockaddr_un addrRemote;
      socklen_t size = sizeof(struct sockaddr_un);
      SOCKET cs = accept(m_handle, (struct sockaddr *)&addrRemote, &size);
      if (cs > 0)
      {
         NamedPipe *cp = new NamedPipe(m_name, cs, false);
         m_reqHandler(cp, m_userArg);
         delete cp;
      }
      else
      {
         nxlog_debug(2, _T("NamedPipe(%s): accept failed (%s)"), m_name, _tcserror(errno));
      }
   }
}
