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
NamedPipeListener *NamedPipeListener::create(const TCHAR *name, NamedPipeRequestHandler reqHandler, void *userArg)
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
   prevMask = umask(S_IWGRP | S_IWOTH);
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

   return new NamedPipeListener(name, s, reqHandler, userArg);

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
   CloseHandle(m_handle);
   stop();
}

/**
 * Named pipe server thread
 */
void NamedPipeListener::serverThread()
{
   nxlog_debug(2, _T("NamedPipeListener(%s): waiting for connection"), m_name);
   while(!m_stop)
   {
   }
}

/**
 * Pipe destructor
 */
NamedPipe::~NamedPipe()
{
   CloseHandle(m_handle);
   MutexDestroy(m_writeLock);
}

/**
 * Create client end for named pipe
 */
NamedPipe *NamedPipe::connect(const TCHAR *name, UINT32 timeout)
{
   TCHAR path[MAX_PATH];
   _sntprintf(path, MAX_PATH, _T("\\\\.\\pipe\\%s"), name);

reconnect:
	HANDLE h = CreateFile(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (h == INVALID_HANDLE_VALUE)
	{
		if (GetLastError() == ERROR_PIPE_BUSY)
		{
			if (WaitNamedPipe(path, timeout))
				goto reconnect;
		}
		return NULL;
	}

	DWORD pipeMode = PIPE_READMODE_MESSAGE;
	SetNamedPipeHandleState(h, &pipeMode, NULL, NULL);
   return new NamedPipe(name, h, false);
}

/**
 * Write to pipe
 */
bool NamedPipe::write(const void *data, size_t size)
{
	DWORD bytes;
	if (!WriteFile(s_handle, data, size, &bytes, NULL))
		return false;
   return bytes == (DWORD)size;
}
