/*
** NetXMS - Network Management System
** Notification channel driver that writes messages to text file
** Copyright (C) 2019-2021 Raden Solutions
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
** File: shell.cpp
**
**/

#include <ncdrv.h>
#include <nxproc.h>

#define DEBUG_TAG _T("ncd.shell")

static const NCConfigurationTemplate s_config(true, true); 

/**
 * Process executor that logs output
 */
class OutputLoggingExecutor : public ProcessExecutor
{
protected:
   virtual void onOutput(const char *text) override;

public:
   OutputLoggingExecutor(const TCHAR *command, bool shellExec = true, bool selfDestruct = false) : ProcessExecutor(command, shellExec, selfDestruct)
   {
      m_sendOutput = true;
   };
};

/**
 * Log output executor output handler
 */
void OutputLoggingExecutor::onOutput(const char *text)
{
   TCHAR *buffer;
#ifdef UNICODE
   buffer = WideStringFromMBStringSysLocale(text);
#else
   buffer = MemCopyString(text);
#endif
   StringList *outputLines = String::split(buffer, _tcslen(buffer), _T("\n"));
   MemFree(buffer);
   for (int i = 0; i < outputLines->size(); i++)
   {
      nxlog_write_tag(1, DEBUG_TAG, _T("Output: %s"), outputLines->get(i));
   }
   delete outputLines;
}

/**
 * Shell command notification driver class
 */
class ShellDriver : public NCDriver
{
private:
   String m_command;
   ShellDriver(const TCHAR * command) : m_command(command){};
public:
   static ShellDriver *createInstance(Config *config);
   virtual bool send(const TCHAR *recipient, const TCHAR *subject, const TCHAR *body) override;
};

/**
 * Create driver instance
 */
ShellDriver *ShellDriver::createInstance(Config *config)
{
   const TCHAR * command = config->getValue(_T("/Shell/Command"));
   if (command == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 1, _T("Driver configuration not found"));
      return nullptr;
   }
   return new ShellDriver(command);
}

/**
 * Driver send method
 */
bool ShellDriver::send(const TCHAR *recipient, const TCHAR *subject, const TCHAR *body)
{
   StringBuffer command(m_command);
   command.replace(_T("${recipient}"), CHECK_NULL_EX(recipient));
   command.replace(_T("${subject}"), CHECK_NULL_EX(subject));
   command.replace(_T("${text}"), CHECK_NULL_EX(body));
   OutputLoggingExecutor* procexec = new OutputLoggingExecutor(command.getBuffer(), false, true);
   return procexec->execute();
}

/**
 * Driver entry point
 */
DECLARE_NCD_ENTRY_POINT(Shell, &s_config)
{
   return ShellDriver::createInstance(config);
}

#ifdef _WIN32

/**
 * DLL Entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
		DisableThreadLibraryCalls(hInstance);
	return TRUE;
}

#endif
