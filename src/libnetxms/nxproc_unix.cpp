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
#include <pwd.h>

#ifdef __sun
#include <ucred.h>
#endif

/**
 * Create listener end for named pipe
 */
NamedPipeListener *NamedPipeListener::create(const TCHAR *name, NamedPipeRequestHandler reqHandler, void *userArg, const TCHAR *user)
{
   mode_t prevMask = 0;

   int s = socket(AF_UNIX, SOCK_STREAM, 0);
   if (s == INVALID_SOCKET)
   {
      nxlog_debug(2, _T("NamedPipeListener(%s): socket() call failed (%s)"), name, _tcserror(errno));
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
   prevMask = umask(0);
   if (bind(s, (struct sockaddr *)&addrLocal, SUN_LEN(&addrLocal)) == -1)
   {
      nxlog_debug(2, _T("NamedPipeListener(%s): bind failed (%s)"), name, _tcserror(errno));
      umask(prevMask);
      goto failure;
   }
   umask(prevMask);

   if (listen(s, 5) == -1)
   {
      nxlog_debug(2, _T("NamedPipeListener(%s): listen() call failed (%s)"), name, _tcserror(errno));
      goto failure;
   }

   return new NamedPipeListener(name, s, reqHandler, userArg, user);

failure:
   close(s);
   unlink(addrLocal.sun_path);
   return NULL;
}

/**
 * Pipe destructor
 */
NamedPipeListener::~NamedPipeListener()
{
   close(m_handle);
   stop();
   char path[MAX_PATH];
#ifdef UNICODE
   sprintf(path, "/tmp/.%S", m_name);
#else
   sprintf(path, "/tmp/.%s", m_name);
#endif
   unlink(path);
}

/**
 * Get UNIX socket peer user ID
 */
static bool GetPeerUID(SOCKET s, unsigned int *uid)
{
#if defined(__sun)
   ucred_t *peer = NULL;
   if (getpeerucred(s, &peer) == 0)
   {
      *uid = (unsigned int)ucred_geteuid(peer);
      ucred_free(peer);
      return true;
   }
#elif HAVE_GETPEEREID
   uid_t euid;
   gid_t egid;
   if (getpeereid(s, &euid, &egid) == 0)
   {
      *uid = (unsigned int)euid;
      return true;
   }
#elif defined(SO_PEERID)
   struct peercred_struct peer;
   unsigned int len = sizeof(peer);
   if (getsockopt(s, SOL_SOCKET, SO_PEERID, &peer, &len) == 0)
   {
      *uid = (unsigned int)peer.euid;
      return true;
   }
#elif defined(SO_PEERCRED)
   struct ucred peer;
   unsigned int len = sizeof(peer);
   if (getsockopt(s, SOL_SOCKET, SO_PEERCRED, &peer, &len) == 0)
   {
      *uid = (unsigned int)peer.uid;
      return true;
   }
#elif defined(_HPUX)
   // implementation for HP-UX is not possible
#else
#error no valid method to get socket peer UID
#endif
   return false;
}

/**
 * Named pipe server thread
 */
void NamedPipeListener::serverThread()
{
   nxlog_debug(2, _T("NamedPipeListener(%s): waiting for connection"), m_name);
   while(!m_stop)
   {
      struct sockaddr_un addrRemote;
      socklen_t size = sizeof(struct sockaddr_un);
      SOCKET cs = accept(m_handle, (struct sockaddr *)&addrRemote, &size);
      if (cs > 0)
      {
         TCHAR user[64];
         unsigned int uid;
         if (GetPeerUID(cs, &uid))
         {
#if HAVE_GETPWUID_R
            struct passwd pwbuf, *pw;
            char sbuf[4096];
            getpwuid_r(uid, &pwbuf, sbuf, 4096, &pw);
#else
            struct passwd *pw = getpwuid(uid);
#endif
            if (pw != NULL)
            {
#ifdef UNICODE
               MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pw->pw_name, -1, user, 64);
#else
               strlcpy(user, pw->pw_name, 64);
#endif
            }
            else
            {
               _sntprintf(user, 64, _T("[%u]"), uid);
            }
         }
         else
         {
            _tcscpy(user, _T("[unknown]"));
         }

         if ((m_user[0] == 0) || !_tcscmp(m_user, user))
         {
            nxlog_debug(5, _T("NamedPipeListener(%s): accepted connection by user %s"), m_name, user);
            NamedPipe *cp = new NamedPipe(m_name, cs, user);
            m_reqHandler(cp, m_userArg);
            delete cp;
         }
         else
         {
            nxlog_debug(5, _T("NamedPipeListener(%s): rejected connection by user %s"), m_name, user);
         }
      }
      else
      {
         nxlog_debug(2, _T("NamedPipeListener(%s): accept failed (%s)"), m_name, _tcserror(errno));
      }
   }
}

/**
 * Pipe destructor
 */
NamedPipe::~NamedPipe()
{
   close(m_handle);
   MutexDestroy(m_writeLock);
}

/**
 * Create client end for named pipe
 */
NamedPipe *NamedPipe::connect(const TCHAR *name, UINT32 timeout)
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
   if (::connect(s, (struct sockaddr *)&remote, SUN_LEN(&remote)) == -1)
   {
      nxlog_debug(2, _T("NamedPipe(%s): connect() call failed (%s)"), name, _tcserror(errno));
      close(s);
      return NULL;
   }

   return new NamedPipe(name, s, NULL);
}

/**
 * Write to pipe
 */
bool NamedPipe::write(const void *data, size_t size)
{
   return SendEx(m_handle, data, size, 0, m_writeLock) == (int)size;
}

/**
 * Get user name
 */
const TCHAR *NamedPipe::user()
{
   return m_user;
}
