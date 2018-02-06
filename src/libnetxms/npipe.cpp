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
** File: nxproc.cpp
**
**/

#include "libnetxms.h"
#include <nxproc.h>

/**
 * Named pipe listener constructor
 */
NamedPipeListener::NamedPipeListener(const TCHAR *name, HPIPE handle, NamedPipeRequestHandler reqHandler, void *userArg, const TCHAR *user)
{
   _tcslcpy(m_name, name, MAX_PIPE_NAME_LEN);
   m_handle = handle;
   m_reqHandler = reqHandler;
   m_userArg = userArg;
   m_serverThread = INVALID_THREAD_HANDLE;
   m_stop = false;
   _tcslcpy(m_user, CHECK_NULL_EX(user), 64);
}

/**
 * Start named pipe server
 */
void NamedPipeListener::start()
{
   if (m_serverThread != INVALID_THREAD_HANDLE)
      return;  // already started

   m_stop = false;
   m_serverThread = ThreadCreateEx(NamedPipeListener::serverThreadStarter, 0, this);
}

/**
 * Stop named pipe server
 */
void NamedPipeListener::stop()
{
   m_stop = true;
   ThreadJoin(m_serverThread);
   m_serverThread = INVALID_THREAD_HANDLE;
}

/**
 * Named pipe server thread starter
 */
THREAD_RESULT THREAD_CALL NamedPipeListener::serverThreadStarter(void *arg)
{
   static_cast<NamedPipeListener*>(arg)->serverThread();
   return THREAD_OK;
}

/**
 * Named pipe constructor
 */
NamedPipe::NamedPipe(const TCHAR *name, HPIPE handle, const TCHAR *user)
{
   _tcslcpy(m_name, name, MAX_PIPE_NAME_LEN);
   m_handle = handle;
   m_writeLock = MutexCreate();
   _tcslcpy(m_user, CHECK_NULL_EX(user), 64);
}
