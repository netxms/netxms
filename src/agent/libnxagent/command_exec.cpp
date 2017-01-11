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

/**
 * Create new server command execution object from command
 */
CommandExec::CommandExec(const TCHAR *cmd)
{
   m_pid = 0;
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
   m_pid = 0;
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
   ThreadJoin(m_outputThread);
   free(m_cmd);
   stop();
}

/**
 * Execute command
 */
bool CommandExec::execute()
{
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
         return false;
      case 0: // child
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
         return true;
   }

   return false;
}

/**
 * Start read output thread
 */
THREAD_RESULT THREAD_CALL CommandExec::readOutput(void *pArg)
{
   int pipe = ((CommandExec *)pArg)->getOutputPipe();
   fcntl(pipe, F_SETFD, fcntl(pipe, F_GETFD) | O_NONBLOCK);

   char buffer[4096];
   SocketPoller sp;
   while(true)
   {
      sp.reset();
      sp.add(pipe);
      int rc = sp.poll(10000);
      if (rc > 0)
      {
         rc = read(pipe, buffer, 4096);
         if (rc > 0)
         {
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
   ((CommandExec *)pArg)->endOfOutput();

   close(pipe);
   return THREAD_OK;
}

/**
 * kill command
 */
void CommandExec::stop()
{
   kill(m_pid, SIGKILL);
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
