/*
** NetXMS - Network Management System
** Copyright (C) 2003-2018 Raden Solutions
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
** File: procexec.cpp
**
**/

#include "libnetxms.h"
#include <nxproc.h>
#include <signal.h>

#if HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

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
 * Create new process executor object for given command line
 */
ProcessExecutor::ProcessExecutor(const TCHAR *cmd, bool shellExec)
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
   m_shellExec = shellExec;
   m_sendOutput = false;
   m_outputThread = INVALID_THREAD_HANDLE;
   m_started = false;
   m_running = false;
}

/**
 * Destructor
 */
ProcessExecutor::~ProcessExecutor()
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
bool ProcessExecutor::execute()
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
      nxlog_debug(4, _T("ProcessExecutor::execute(): cannot create pipe (%s)"), GetSystemErrorText(GetLastError(), buffer, 1024));
      return false;
   }

   // Ensure the read handle to the pipe for STDOUT is not inherited.
   SetHandleInformation(stdoutRead, HANDLE_FLAG_INHERIT, 0);

   HANDLE stdinRead, stdinWrite;
   if (!CreatePipe(&stdinRead, &stdinWrite, &sa, 0))
   {
      TCHAR buffer[1024];
      nxlog_debug(4, _T("ProcessExecutor::execute(): cannot create pipe (%s)"), GetSystemErrorText(GetLastError(), buffer, 1024));
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

   String cmdLine = m_shellExec ? _T("CMD.EXE /C ") : _T("");
   cmdLine.append(m_cmd);
   if (CreateProcess(NULL, cmdLine.getBuffer(), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
   {
      nxlog_debug(5, _T("ProcessExecutor::execute(): process \"%s\" started"), cmdLine.getBuffer());

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
      nxlog_debug(4, _T("ProcessExecutor::execute(): cannot create process \"%s\" (%s)"),
         cmdLine.getBuffer(), GetSystemErrorText(GetLastError(), buffer, 1024));

      CloseHandle(stdoutRead);
      CloseHandle(stdoutWrite);
      CloseHandle(stdinRead);
      CloseHandle(stdinWrite);
   }

#else /* UNIX implementation */

   if (pipe(m_pipe) == -1)
   {
      nxlog_debug(4, _T("ProcessExecutor::execute(): pipe() call failed (%s)"), _tcserror(errno));
      return false;
   }

   m_pid = fork();
   switch(m_pid)
   {
      case -1: // error
         nxlog_debug(4, _T("ProcessExecutor::execute(): fork() call failed (%s)"), _tcserror(errno));
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
         if (m_shellExec)
         {
#ifdef UNICODE
            execl("/bin/sh", "/bin/sh", "-c", UTF8StringFromWideString(m_cmd), NULL);
#else
            execl("/bin/sh", "/bin/sh", "-c", m_cmd, NULL);
#endif
         }
         else
         {
#ifdef UNICODE
            char *tmp = UTF8StringFromWideString(m_cmd);
#else
            char *tmp = m_cmd;
#endif

            char *argv[256];
            argv[0] = tmp;

            int index = 1;
            bool squotes = false, dquotes = false;
            for(char *p = tmp; *p != 0; p++)
            {
               if ((*p == ' ') && !squotes && !dquotes)
               {
                  *p = 0;
                  p++;
                  while(*p == ' ')
                     p++;
                  argv[index++] = p;
               }
               else if ((*p == '\'') && !dquotes)
               {
                  squotes = !squotes;
                  memmove(p, p + 1, strlen(p));
                  p--;
               }
               else if ((*p == '"') && !squotes)
               {
                  dquotes = !dquotes;
                  memmove(p, p + 1, strlen(p));
                  p--;
               }
            }
            argv[index] = NULL;

            execv(argv[0], argv);
         }
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
            m_outputThread = ThreadCreateEx(waitForProcess, 0, this);
         }
         success = true;
         break;
   }

#endif

   m_started = true;
   m_running = success;
   return success;
}

#ifndef _WIN32

/**
 * Process waiting thread
 */
THREAD_RESULT THREAD_CALL ProcessExecutor::waitForProcess(void *arg)
{
   waitpid(static_cast<ProcessExecutor*>(arg)->m_pid, NULL, 0);
   static_cast<ProcessExecutor*>(arg)->m_running = false;
   return THREAD_OK;
}

#endif

/**
 * Output reading thread
 */
THREAD_RESULT THREAD_CALL ProcessExecutor::readOutput(void *arg)
{
   char buffer[4096];

#ifdef _WIN32  /* Windows implementation */

   OVERLAPPED ov;
	memset(&ov, 0, sizeof(OVERLAPPED));
   ov.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

   HANDLE pipe = static_cast<ProcessExecutor*>(arg)->getOutputPipe();
   while(true)
   {
      if (!ReadFile(pipe, buffer, sizeof(buffer) - 1, NULL, &ov))
      {
         if (GetLastError() != ERROR_IO_PENDING)
         {
            TCHAR emsg[1024];
            nxlog_debug(6, _T("ProcessExecutor::readOutput(): stopped on ReadFile (%s)"), GetSystemErrorText(GetLastError(), emsg, 1024));
            break;
         }
      }

      HANDLE handles[2];
      handles[0] = ov.hEvent;
      handles[1] = static_cast<ProcessExecutor*>(arg)->m_phandle;
      DWORD rc = WaitForMultipleObjects(2, handles, FALSE, 5000);
      if (rc == WAIT_OBJECT_0 + 1)
      {
         nxlog_debug(6, _T("ProcessExecutor::readOutput(): process termination detected"));
         break;   // Process terminated
      }
      if (rc == WAIT_OBJECT_0)
      {
         DWORD bytes;
         if (GetOverlappedResult(pipe, &ov, &bytes, TRUE))
         {
            buffer[bytes] = 0;
            static_cast<ProcessExecutor*>(arg)->onOutput(buffer);
         }
         else
         {
            TCHAR emsg[1024];
            nxlog_debug(6, _T("ProcessExecutor::readOutput(): stopped on GetOverlappedResult (%s)"), GetSystemErrorText(GetLastError(), emsg, 1024));
            break;
         }
      }
      else
      {
         // Send empty output on timeout
         static_cast<ProcessExecutor*>(arg)->onOutput("");
      }
   }

   CloseHandle(ov.hEvent);
   CloseHandle(pipe);

#else /* UNIX implementation */

   int pipe = static_cast<ProcessExecutor*>(arg)->getOutputPipe();
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
            static_cast<ProcessExecutor*>(arg)->onOutput(buffer);
         }
         else
         {
            if ((rc == -1) && ((errno == EAGAIN) || (errno == EINTR)))
            {
               static_cast<ProcessExecutor*>(arg)->onOutput("");
               continue;
            }
            nxlog_debug(6, _T("ProcessExecutor::readOutput(): stopped on read (rc=%d err=%s)"), rc, _tcserror(errno));
            break;
         }
      }
      else if (rc == 0)
      {
         // Send empty output on timeout
         static_cast<ProcessExecutor*>(arg)->onOutput("");
      }
      else
      {
         nxlog_debug(6, _T("ProcessExecutor::readOutput(): stopped on poll (%s)"), _tcserror(errno));
         break;
      }
   }
   close(pipe);

#endif

   static_cast<ProcessExecutor*>(arg)->endOfOutput();

#ifndef _WIN32
   waitpid(static_cast<ProcessExecutor*>(arg)->m_pid, NULL, 0);
   static_cast<ProcessExecutor*>(arg)->m_running = false;
#endif

   return THREAD_OK;
}

/**
 * kill command
 */
void ProcessExecutor::stop()
{
#ifdef _WIN32
   if (m_phandle != INVALID_HANDLE_VALUE)
      TerminateProcess(m_phandle, 127);
#else
   if (m_pid != 0)
      kill(-m_pid, SIGKILL);  // kill all processes in group
#endif
   m_running = false;
   m_started = false;
}

/**
 * Perform action when output is generated
 */
void ProcessExecutor::onOutput(const char *text)
{
}

/**
 * Perform action after output is generated
 */
void ProcessExecutor::endOfOutput()
{
}

/**
 * Check that process is still running
 */
bool ProcessExecutor::isRunning()
{
   if (!m_running)
      return false;

#ifdef _WIN32
   DWORD exitCode;
   if (GetExitCodeProcess(m_phandle, &exitCode))
   {
      if (exitCode != STILL_ACTIVE)
         m_running = false;
   }
   else
   {
      m_running = false;
   }
#else
   if (kill(m_pid, 0) != 0)
      m_running = false;
#endif
   return m_running;
}

/**
 * Wait for process completion
 */
bool ProcessExecutor::waitForCompletion(UINT32 timeout)
{
   if (!m_running)
      return true;

#ifdef _WIN32
   return WaitForSingleObject(m_phandle, timeout) == WAIT_OBJECT_0;
#else
   while(isRunning() && (timeout > 0))
   {
      ThreadSleepMs(50);
      timeout -= std::min(timeout, (UINT32)50);
   }
   return !m_running;
#endif
}

/**
 * Get process ID
 */
pid_t ProcessExecutor::getProcessId()
{
#ifdef _WIN32
   return GetProcessId(m_phandle);
#else
   return m_pid;
#endif
}
