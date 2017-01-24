/*
** NetXMS - Network Management System
** Copyright (C) 2003-2017 Raden Solutions
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
** File: command_exec.cpp
**
**/

#include "libnxagent.h"
#include <signal.h>

/**
 * Next free stream ID
 */
static VolatileCounter s_nextStreamId = 0;

#ifdef _WIN32

/**
 * Next free pipe ID
 */
static VolatileCounter s_pipeId = 0;

/**
 * Create pipe
 */
static bool CreatePipeEx(LPHANDLE lpReadPipe, LPHANDLE lpWritePipe, bool asyncRead)
{
   TCHAR name[MAX_PATH];
   _sntprintf(name, MAX_PATH, _T("\\\\.\\Pipe\\nxexec.%08x.%08x"), GetCurrentProcessId(), InterlockedIncrement(&s_pipeId));

	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

   HANDLE readHandle = CreateNamedPipe(
      name,
      PIPE_ACCESS_INBOUND | (asyncRead ? FILE_FLAG_OVERLAPPED : 0),
      PIPE_TYPE_BYTE | PIPE_WAIT,
      1,           // Number of pipes
      8192,        // Out buffer size
      8192,        // In buffer size
      60000,       // Timeout in ms
      &sa
   );
   if (readHandle == NULL)
      return false;

   HANDLE writeHandle = CreateFile(
      name,
      GENERIC_WRITE,
      0,                         // No sharing
      &sa,
      OPEN_EXISTING,
      FILE_ATTRIBUTE_NORMAL,
      NULL                       // Template file
   );
   if (writeHandle == INVALID_HANDLE_VALUE)
   {
      DWORD error = GetLastError();
      CloseHandle(readHandle);
      SetLastError(error);
      return false;
   }

   *lpReadPipe = readHandle;
   *lpWritePipe = writeHandle;
   return true;
}

#endif /* _WIN32 */

/**
 * Create new server command execution object from command
 */
CommandExec::CommandExec(const TCHAR *cmd)
{
#ifdef _WIN32
   m_phandle = INVALID_HANDLE_VALUE;
   m_pipe = INVALID_HANDLE_VALUE;
#else
   m_pid = 0;
   m_pipe[0] = -1;
   m_pipe[1] = -1;
#endif
   m_cmd = _tcsdup_ex(cmd);
   m_streamId = InterlockedIncrement(&s_nextStreamId);
   m_sendOutput = false;
   m_outputThread = INVALID_THREAD_HANDLE;
}

/**
 * Create new server execution object
 */
CommandExec::CommandExec()
{
#ifdef _WIN32
   m_phandle = INVALID_HANDLE_VALUE;
   m_pipe = INVALID_HANDLE_VALUE;
#else
   m_pid = 0;
   m_pipe[0] = -1;
   m_pipe[1] = -1;
#endif
   m_cmd = NULL;
   m_streamId = InterlockedIncrement(&s_nextStreamId);
   m_sendOutput = false;
   m_outputThread = INVALID_THREAD_HANDLE;
}

/**
 * Destructor
 */
CommandExec::~CommandExec()
{
   stop();
   ThreadJoin(m_outputThread);
   free(m_cmd);
#ifdef _WIN32
   CloseHandle(m_phandle);
#endif
}

/**
 * Execute command
 */
bool CommandExec::execute()
{
   bool success = false;

#ifdef _WIN32  /* Windows implementation */

	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

   HANDLE stdoutRead, stdoutWrite;
   if (!CreatePipeEx(&stdoutRead, &stdoutWrite, true))
   {
      TCHAR buffer[1024];
      nxlog_debug(4, _T("CommandExec::execute(): cannot create pipe (%s)"), GetSystemErrorText(GetLastError(), buffer, 1024));
      return false;
   }

   // Ensure the read handle to the pipe for STDOUT is not inherited.
   SetHandleInformation(stdoutRead, HANDLE_FLAG_INHERIT, 0);

   HANDLE stdinRead, stdinWrite;
   if (!CreatePipe(&stdinRead, &stdinWrite, &sa, 0))
   {
      TCHAR buffer[1024];
      nxlog_debug(4, _T("CommandExec::execute(): cannot create pipe (%s)"), GetSystemErrorText(GetLastError(), buffer, 1024));
      CloseHandle(stdoutRead);
      CloseHandle(stdoutWrite);
      return false;
   }

   // Ensure the write handle to the pipe for STDIN is not inherited. 
   SetHandleInformation(stdinWrite, HANDLE_FLAG_INHERIT, 0);

   STARTUPINFO si;
   PROCESS_INFORMATION pi;

   memset(&si, 0, sizeof(STARTUPINFO));
   si.cb = sizeof(STARTUPINFO);
   si.dwFlags = STARTF_USESTDHANDLES;
   si.hStdInput = stdinRead;
   si.hStdOutput = stdoutWrite;
   si.hStdError = stdoutWrite;

   String cmdLine = _T("CMD.EXE /C ");
   cmdLine.append(m_cmd);
   if (CreateProcess(NULL, cmdLine.getBuffer(), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
   {
      nxlog_debug(5, _T("CommandExec::execute(): process \"%s\" started"), cmdLine.getBuffer());

      m_phandle = pi.hProcess;
      CloseHandle(pi.hThread);
      CloseHandle(stdoutWrite);
      CloseHandle(stdinRead);
      CloseHandle(stdinWrite);
      if (m_sendOutput)
      {
         m_pipe = stdoutRead;
         m_outputThread = ThreadCreateEx(readOutput, 0, this);
      }
      else
      {
         CloseHandle(stdoutRead);
      }
      success = true;
   }
   else
   {
      TCHAR buffer[1024];
      nxlog_debug(4, _T("CommandExec::execute(): cannot create process \"%s\" (%s)"), 
         cmdLine.getBuffer(), GetSystemErrorText(GetLastError(), buffer, 1024));

      CloseHandle(stdoutRead);
      CloseHandle(stdoutWrite);
      CloseHandle(stdinRead);
      CloseHandle(stdinWrite);
   }

#else /* UNIX implementation */

   if (pipe(m_pipe) == -1)
   {
      nxlog_debug(4, _T("CommandExec::execute(): pipe() call failed (%s)"), _tcserror(errno));
      return false;
   }

   m_pid = fork();
   switch(m_pid)
   {
      case -1: // error
         nxlog_debug(4, _T("CommandExec::execute(): fork() call failed (%s)"), _tcserror(errno));
         close(m_pipe[0]);
         close(m_pipe[1]);
         break;
      case 0: // child
         setpgid(0, 0); // new process group
         close(m_pipe[0]);
         close(1);
         close(2);
         dup2(m_pipe[1], 1);
         dup2(m_pipe[1], 2);
         close(m_pipe[1]);
#ifdef UNICODE
         execl("/bin/sh", "/bin/sh", "-c", UTF8StringFromWideString(m_cmd), NULL);
#else
         execl("/bin/sh", "/bin/sh", "-c", m_cmd, NULL);
#endif
         exit(127);
         break;
      default: // parent
         close(m_pipe[1]);
         if (m_sendOutput)
         {
            m_outputThread = ThreadCreateEx(readOutput, 0, this);
         }
         else
         {
            close(m_pipe[0]);
         }
         success = true;
         break;
   }

#endif

   return success;
}

/**
 * Start read output thread
 */
THREAD_RESULT THREAD_CALL CommandExec::readOutput(void *pArg)
{
   char buffer[4096];

#ifdef _WIN32  /* Windows implementation */

   OVERLAPPED ov;
	memset(&ov, 0, sizeof(OVERLAPPED));
   ov.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

   HANDLE pipe = ((CommandExec *)pArg)->getOutputPipe();
   while(true)
   {
      if (!ReadFile(pipe, buffer, sizeof(buffer) - 1, NULL, &ov))
      {
         if (GetLastError() != ERROR_IO_PENDING)
         {
            TCHAR emsg[1024];
            nxlog_debug(6, _T("CommandExec::readOutput(): stopped on ReadFile (%s)"), GetSystemErrorText(GetLastError(), emsg, 1024));
            break;
         }
      }

      HANDLE handles[2];
      handles[0] = ov.hEvent;
      handles[1] = ((CommandExec *)pArg)->m_phandle;
      DWORD rc = WaitForMultipleObjects(2, handles, FALSE, 5000);
      if (rc == WAIT_OBJECT_0 + 1)
      {
         nxlog_debug(6, _T("CommandExec::readOutput(): process termination detected"));
         break;   // Process terminated
      }
      if (rc == WAIT_OBJECT_0)
      {
         DWORD bytes;
         if (GetOverlappedResult(pipe, &ov, &bytes, TRUE))
         {
            buffer[bytes] = 0;
            ((CommandExec *)pArg)->onOutput(buffer);
         }
         else
         {
            TCHAR emsg[1024];
            nxlog_debug(6, _T("CommandExec::readOutput(): stopped on GetOverlappedResult (%s)"), GetSystemErrorText(GetLastError(), emsg, 1024));
            break;
         }
      }
      else
      {
         // Send empty output on timeout
         ((CommandExec *)pArg)->onOutput("");
      }
   }

   CloseHandle(ov.hEvent);
   CloseHandle(pipe);

#else /* UNIX implementation */

   int pipe = ((CommandExec *)pArg)->getOutputPipe();
   fcntl(pipe, F_SETFD, fcntl(pipe, F_GETFD) | O_NONBLOCK);

   SocketPoller sp;
   while(true)
   {
      sp.reset();
      sp.add(pipe);
      int rc = sp.poll(10000);
      if (rc > 0)
      {
         rc = read(pipe, buffer, sizeof(buffer) - 1);
         if (rc > 0)
         {
            buffer[rc] = 0;
            ((CommandExec *)pArg)->onOutput(buffer);
         }
         else
         {
            if ((rc == -1) && ((errno == EAGAIN) || (errno == EINTR)))
            {
               ((CommandExec *)pArg)->onOutput("");
               continue;
            }
            nxlog_debug(6, _T("CommandExec::readOutput(): stopped on read (rc=%d err=%s)"), rc, _tcserror(errno));
            break;
         }
      }
      else if (rc == 0)
      {
         // Send empty output on timeout
         ((CommandExec *)pArg)->onOutput("");
      }
      else
      {
         nxlog_debug(6, _T("CommandExec::readOutput(): stopped on poll (%s)"), _tcserror(errno));
         break;
      }
   }
   close(pipe);

#endif

   ((CommandExec *)pArg)->endOfOutput();
   return THREAD_OK;
}

/**
 * kill command
 */
void CommandExec::stop()
{
#ifdef _WIN32
   TerminateProcess(m_phandle, 127);
#else
   kill(-m_pid, SIGKILL);  // kill all processes in group
#endif
}

/**
 * Perform action when output is generated
 */
void CommandExec::onOutput(const char *text)
{
}

/**
 * Perform action after output is generated
 */
void CommandExec::endOfOutput()
{
}
