/*
** NetXMS - Network Management System
** Copyright (C) 2003-2023 Victor Kirhenshtein
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
** File: jetty.cpp
**
**/

#include "nxweblauncher.h"
#include <nxproc.h>

/**
 * Jetty process handle
 */
static HANDLE s_jettyProcess;

/**
 * Start jetty
 */
bool RunJetty(WCHAR *commandLine)
{
   STARTUPINFO si;
   memset(&si, 0, sizeof(STARTUPINFO));
   si.cb = sizeof(STARTUPINFO);

   PROCESS_INFORMATION pi;
   if (!CreateProcess(g_javaExecutable, commandLine, nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi))
   {
      TCHAR buffer[1024];
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_STARTUP, _T("Cannot create process (Error 0x%08x: %s)"), GetLastError(), GetSystemErrorText(GetLastError(), buffer, 1024));
      return false;
   }

   nxlog_debug_tag(DEBUG_TAG_STARTUP, 5, _T("Server process started"));

   CloseHandle(pi.hThread);
   s_jettyProcess = pi.hProcess;
   WaitForSingleObject(pi.hProcess, INFINITE);
   CloseHandle(pi.hProcess);

   nxlog_debug_tag(DEBUG_TAG_SHUTDOWN, 5, _T("Server process terminated"));
   return true;
}

/**
 * Process executor that logs it's output
 */
class LoggingProcessExecutor : public ProcessExecutor
{
protected:
   virtual void onOutput(const char *text, size_t length) override
   {
      nxlog_debug_tag(L"jetty", 5, L"%hs", text);
   }

public:
   LoggingProcessExecutor(const WCHAR *cmdLine) : ProcessExecutor(cmdLine, false) { }
};

/**
 * Send stop signal to jetty
 */
void StopJetty()
{
   StringBuffer commandLine;
   commandLine.append(L"\"");
   commandLine.append(g_javaExecutable);
   commandLine.append(L"\" -jar \"");
   commandLine.append(g_installDir);
   commandLine.append(L"\\jetty-home\\start.jar\" --stop stop.port=17003 stop.key=nxmc$jetty$key stop.wait=20");

   LoggingProcessExecutor executor(commandLine);
   executor.execute();
   executor.waitForCompletion(30000);

   TerminateProcess(s_jettyProcess, 0);
}
