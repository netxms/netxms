/*
** NetXMS - Network Management System
** NetXMS Foundation Library
** Copyright (C) 2003-2021 Victor Kirhenshtein
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
** File: nxproc_win32.cpp
**
**/

#include "libnetxms.h"
#include <nxproc.h>
#include <aclapi.h>

/**
 * Create listener end for named pipe
 */
NamedPipeListener *NamedPipeListener::create(const TCHAR *name, NamedPipeRequestHandler reqHandler, void *userArg, const TCHAR *user)
{
   NamedPipeListener *listener = NULL;

	PSID sidEveryone = NULL;
	ACL *acl = NULL;
	PSECURITY_DESCRIPTOR sd = NULL;

	TCHAR errorText[1024];

	// Create a well-known SID for the Everyone group.
	SID_IDENTIFIER_AUTHORITY sidAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
	if (!AllocateAndInitializeSid(&sidAuthWorld, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &sidEveryone))
	{
		nxlog_debug(2, _T("NamedPipeListener(%s): AllocateAndInitializeSid failed (%s)"), name, GetSystemErrorText(GetLastError(), errorText, 1024));
		goto cleanup;
	}

	// Initialize an EXPLICIT_ACCESS structure for an ACE.
	// The ACE will allow either Everyone or given user to access pipe
	EXPLICIT_ACCESS ea;
	ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));
	ea.grfAccessPermissions = (FILE_GENERIC_READ | FILE_GENERIC_WRITE) & ~FILE_CREATE_PIPE_INSTANCE;
	ea.grfAccessMode = SET_ACCESS;
	ea.grfInheritance = NO_INHERITANCE;
	if ((user == NULL) || (user[0] == 0) || !_tcscmp(user, _T("*")))
	{
		ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
		ea.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
		ea.Trustee.ptstrName  = (LPTSTR)sidEveryone;
	}
	else
	{
		ea.Trustee.TrusteeForm = TRUSTEE_IS_NAME;
		ea.Trustee.TrusteeType = TRUSTEE_IS_USER;
		ea.Trustee.ptstrName  = (LPTSTR)user;
		nxlog_debug(2, _T("NamedPipeListener(%s): will allow connections only for user %s"), name, user);
	}

	// Create a new ACL that contains the new ACEs.
	if (SetEntriesInAcl(1, &ea, NULL, &acl) != ERROR_SUCCESS)
	{
		nxlog_debug(2, _T("NamedPipeListener(%s): SetEntriesInAcl failed (%s)"), name, GetSystemErrorText(GetLastError(), errorText, 1024));
		goto cleanup;
	}

	sd = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
	if (sd == NULL)
	{
		nxlog_debug(2, _T("NamedPipeListener(%s): LocalAlloc failed (%s)"), name, GetSystemErrorText(GetLastError(), errorText, 1024));
		goto cleanup;
	}

	if (!InitializeSecurityDescriptor(sd, SECURITY_DESCRIPTOR_REVISION))
	{
		nxlog_debug(2, _T("NamedPipeListener(%s): InitializeSecurityDescriptor failed (%s)"), name, GetSystemErrorText(GetLastError(), errorText, 1024));
		goto cleanup;
	}

	// Add the ACL to the security descriptor. 
   if (!SetSecurityDescriptorDacl(sd, TRUE, acl, FALSE))
	{
		nxlog_debug(2, _T("NamedPipeListener(%s): SetSecurityDescriptorDacl failed (%s)"), name, GetSystemErrorText(GetLastError(), errorText, 1024));
		goto cleanup;
	}

   TCHAR path[MAX_PATH];
   _sntprintf(path, MAX_PATH, _T("\\\\.\\pipe\\%s"), name);

	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = FALSE;
	sa.lpSecurityDescriptor = sd;
	HANDLE hPipe = CreateNamedPipe(path, PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE, 1, 8192, 8192, 0, &sa);
	if (hPipe == INVALID_HANDLE_VALUE)
	{
		nxlog_debug(2, _T("NamedPipeListener(%s): CreateNamedPipe failed (%s)"), name, GetSystemErrorText(GetLastError(), errorText, 1024));
		goto cleanup;
	}

   listener = new NamedPipeListener(name, hPipe, reqHandler, userArg, user);

cleanup:
	if (sd != NULL)
		LocalFree(sd);

	if (acl != NULL)
		LocalFree(acl);

	if (sidEveryone != NULL)
		FreeSid(sidEveryone);

   return listener;
}

/**
 * Pipe destructor
 */
NamedPipeListener::~NamedPipeListener()
{
   CloseHandle(m_handle);
   stop();
   CloseHandle(m_stopEvent);
}

/**
 * Named pipe server thread
 */
void NamedPipeListener::serverThread()
{
   HANDLE connectEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
   nxlog_debug(2, _T("NamedPipeListener(%s): waiting for connection"), m_name);
   while(!m_stop)
   {
      OVERLAPPED ov;
      memset(&ov, 0, sizeof(OVERLAPPED));
      ov.hEvent = connectEvent;
      BOOL connected = ConnectNamedPipe(m_handle, &ov);
		if (connected || (GetLastError() == ERROR_PIPE_CONNECTED))
		{
         nxlog_debug(5, _T("NamedPipeListener(%s): accepted connection"), m_name);
         NamedPipe *pipe = new NamedPipe(m_name, m_handle, NULL);
         m_reqHandler(pipe, m_userArg);
         delete pipe;
		}
      else if (GetLastError() == ERROR_IO_PENDING)
      {
         HANDLE handles[2];
         handles[0] = connectEvent;
         handles[1] = m_stopEvent;
         DWORD rc = WaitForMultipleObjects(2, handles, FALSE, INFINITE);
         if (rc == WAIT_OBJECT_0 + 1)
         {
            CancelIo(m_handle);
            break;
         }
         if (rc == WAIT_OBJECT_0)
         {
            DWORD bytes;
            if (GetOverlappedResult(m_handle, &ov, &bytes, TRUE))
            {
               nxlog_debug(5, _T("NamedPipeListener(%s): accepted connection"), m_name);
               NamedPipe *pipe = new NamedPipe(m_name, m_handle, NULL);
               m_reqHandler(pipe, m_userArg);
               delete pipe;
            }
         }
      }
		else
		{
         TCHAR errorText[1024];
			nxlog_debug(2, _T("NamedPipeListener(%s): ConnectNamedPipe failed (%s)"), m_name, GetSystemErrorText(GetLastError(), errorText, 1024));
         ThreadSleep(5);
		}
   }
   CloseHandle(connectEvent);
   nxlog_debug(2, _T("NamedPipeListener(%s): stopped"), m_name);
}

/**
 * Named pipe constructor
 */
NamedPipe::NamedPipe(const TCHAR *name, HPIPE handle, const TCHAR *user) : m_writeLock(MutexType::FAST)
{
   _tcslcpy(m_name, name, MAX_PIPE_NAME_LEN);
   m_handle = handle;
   _tcslcpy(m_user, CHECK_NULL_EX(user), 64);
   m_writeEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

/**
 * Pipe destructor
 */
NamedPipe::~NamedPipe()
{
   CancelIo(m_handle);  // Cancel any pending I/O operations
   DWORD flags = 0;
   GetNamedPipeInfo(m_handle, &flags, NULL, NULL, NULL);
   if (flags & PIPE_SERVER_END)
		DisconnectNamedPipe(m_handle);
   else
      CloseHandle(m_handle);
   CloseHandle(m_writeEvent);
}

/**
 * Create client end for named pipe
 */
NamedPipe *NamedPipe::connect(const TCHAR *name, UINT32 timeout)
{
   TCHAR path[MAX_PATH];
   _sntprintf(path, MAX_PATH, _T("\\\\.\\pipe\\%s"), name);

reconnect:
   HANDLE h = CreateFile(path, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
	if (h == INVALID_HANDLE_VALUE)
	{
		if (GetLastError() == ERROR_PIPE_BUSY)
		{
			if (WaitNamedPipe(path, timeout))
				goto reconnect;
		}
		return nullptr;
	}

	DWORD pipeMode = PIPE_READMODE_MESSAGE;
	SetNamedPipeHandleState(h, &pipeMode, nullptr, nullptr);
   return new NamedPipe(name, h, false);
}

/**
 * Write to pipe
 */
bool NamedPipe::write(const void *data, size_t size)
{
   m_writeLock.lock();
   OVERLAPPED ov;
   memset(&ov, 0, sizeof(OVERLAPPED));
   ov.hEvent = m_writeEvent;
   ResetEvent(m_writeEvent);
   BOOL success = WriteFile(m_handle, data, (DWORD)size, nullptr, &ov);
   if (!success && (GetLastError() == ERROR_IO_PENDING))
      success = (WaitForSingleObject(m_writeEvent, 10000) == WAIT_OBJECT_0);
   if (success)
   {
      DWORD bytes;
      success = GetOverlappedResult(m_handle, &ov, &bytes, FALSE);
      if (bytes != static_cast<DWORD>(size))
         success = FALSE;
   }
   m_writeLock.unlock();
   return success ? true : false;
}

/**
 * Get user name
 */
const TCHAR *NamedPipe::user()
{
   if (m_user[0] == 0)
   {
      if (!GetNamedPipeHandleState(m_handle, NULL, NULL, NULL, NULL, m_user, 64))
      {
         if (GetLastError() != ERROR_CANNOT_IMPERSONATE)
         {
            TCHAR errorText[1024];
			   nxlog_debug(5, _T("NamedPipeListener(%s): GetNamedPipeHandleState failed (%s)"), m_name, GetSystemErrorText(GetLastError(), errorText, 1024));
            _tcscpy(m_user, _T("[unknown]"));
         }
      }
   }
   return m_user;
}
