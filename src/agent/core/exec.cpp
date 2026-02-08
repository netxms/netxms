/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2022 Victor Kirhenshtein
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
** File: exec.cpp
**
**/

#include "nxagentd.h"

#ifdef _WIN32
#include <winternl.h>
#include <WtsApi32.h>
#define WTS_DEBUG_TAG   _T("wts")
#endif

#define EXEC_DEBUG_TAG  _T("exec")

/**
 * Execution function for external (user-defined) metrics and lists. Handler argument contains command line before substitution.
 */
static LONG RunExternal(const TCHAR *param, const TCHAR *arg, StringList *value, bool returnProcessExitCode = false)
{
   nxlog_debug_tag(EXEC_DEBUG_TAG, 4, _T("RunExternal called for \"%s\" \"%s\""), param, arg);

   // Substitute $1 .. $9 with actual arguments
   StringBuffer cmdLine = SubstituteCommandArguments(arg, param);

   nxlog_debug_tag(EXEC_DEBUG_TAG, 4, _T("RunExternal: command line is \"%s\""), cmdLine.cstr());

   LineOutputProcessExecutor executor(cmdLine);
   if (!executor.execute())
   {
      nxlog_debug_tag(EXEC_DEBUG_TAG, 4, _T("RunExternal: cannot start process (command line \"%s\")"), cmdLine.cstr());
      return SYSINFO_RC_ERROR;
   }

   if (!executor.waitForCompletion(g_externalMetricTimeout))
   {
      nxlog_debug_tag(EXEC_DEBUG_TAG, 4, _T("RunExternal: external process execution timeout (command line \"%s\")"), cmdLine.cstr());
      return SYSINFO_RC_ERROR;
   }

   if (returnProcessExitCode)
      value->add(executor.getExitCode());
   else
      value->addAll(executor.getData());
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler function for external (user-defined) metrics
 */
LONG H_ExternalMetric(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   session->debugPrintf(4, _T("H_ExternalMetric called for \"%s\" \"%s\""), cmd, arg);
   StringList values;
   LONG status = RunExternal(cmd, arg, &values);
   if (status == SYSINFO_RC_SUCCESS)
   {
      ret_string(value, values.size() > 0 ? values.get(0) : _T(""));
   }
   return status;
}

/**
 * Handler function for external metrics return exit code version
 */
LONG H_ExternalMetricExitCode(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   session->debugPrintf(4, _T("H_ExternalMetricExitCode called for \"%s\" \"%s\""), cmd, arg);
   StringList values;
   LONG status = RunExternal(cmd, arg, &values, true);
   if (status == SYSINFO_RC_SUCCESS)
   {
      ret_string(value, values.get(0));
   }
   return status;
}

/**
 * Handler function for external (user-defined) lists
 */
LONG H_ExternalList(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   session->debugPrintf(4, _T("H_ExternalList called for \"%s\" \"%s\""), cmd, arg);
   StringList values;
   LONG status = RunExternal(cmd, arg, &values);
   if (status == SYSINFO_RC_SUCCESS)
   {
      value->addAll(&values);
   }
   return status;
}

/**
 * Parse data for external table
 */
void ParseExternalTableData(const ExternalTableDefinition& td, const StringList& data, Table *table)
{
   int numColumns = 0;
   TCHAR **columns = SplitString(data.get(0), td.separator, &numColumns, td.mergeSeparators);
   for(int n = 0; n < numColumns; n++)
   {
      bool instanceColumn = false;
      for(int i = 0; i < td.instanceColumnCount; i++)
         if (!_tcsicmp(td.instanceColumns[i], columns[n]))
         {
            instanceColumn = true;
            break;
         }
      int dataType = td.columnDataTypes.getInt32(columns[n], td.defaultColumnDataType);
      table->addColumn(columns[n], dataType, columns[n], instanceColumn);
      MemFree(columns[n]);
   }
   MemFree(columns);

   for(int i = 1; i < data.size(); i++)
   {
      table->addRow();
      int count = 0;
      TCHAR **values = SplitString(data.get(i), td.separator, &count, td.mergeSeparators);
      for(int n = 0; n < count; n++)
      {
         if (n < numColumns)
            table->setPreallocated(n, values[n]);
         else
            MemFree(values[n]);
      }
      MemFree(values);
   }
}

/**
 * Handler function for external (user-defined) tables
 */
LONG H_ExternalTable(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   const ExternalTableDefinition *td = reinterpret_cast<const ExternalTableDefinition*>(arg);
   session->debugPrintf(4, _T("H_ExternalTable called for \"%s\" (separator=0x%04X mergeSeparators=%s mode=%c cmd=\"%s\""),
      cmd, td->separator, BooleanToString(td->mergeSeparators), td->cmdLine[0], &td->cmdLine[1]);
   StringList output;
   LONG status = RunExternal(cmd, td->cmdLine, &output);
   if (status == SYSINFO_RC_SUCCESS)
   {
      if (output.size() > 0)
      {
         ParseExternalTableData(*td, output, value);
      }
      else
      {
         session->debugPrintf(4, _T("H_ExternalTable(\"%s\"): empty output from command"), cmd);
         status = SYSINFO_RC_ERROR;
      }
   }
   return status;
}

/**
 * Process executor that sends command output to server using action execution context
 */
class SystemActionProcessExecutor : public ProcessExecutor
{
private:
   ActionExecutionContext *m_context;

protected:
   virtual void onOutput(const char *text, size_t length) override;
   virtual void endOfOutput() override;

public:
   SystemActionProcessExecutor(const TCHAR *command, ActionExecutionContext *context) : ProcessExecutor(command)
   {
      m_context = context;
      m_sendOutput = true;
      m_replaceNullCharacters = true;
   }
   virtual ~SystemActionProcessExecutor() = default;
};

/**
 * Handle process output
 */
void SystemActionProcessExecutor::onOutput(const char *text, size_t length)
{
#ifdef UNICODE
   TCHAR *buffer = WideStringFromMBStringSysLocale(text);
   m_context->sendOutput(buffer);
   MemFree(buffer);
#else
   m_context->sendOutput(text);
#endif
}

/**
 * Handle end of output
 */
void SystemActionProcessExecutor::endOfOutput()
{
   m_context->sendEndOfOutputMarker();
}

/**
 * Handler for System.Execute action
 */
uint32_t H_SystemExecute(const shared_ptr<ActionExecutionContext>& context)
{
   if (!context->hasArgs())
      return ERR_BAD_ARGUMENTS;

   const TCHAR *command = context->getArg(0);
   if (context->isOutputRequested())
   {
      SystemActionProcessExecutor executor(command, context.get());
      if (executor.execute())
      {
         context->markAsCompleted(ERR_SUCCESS);
         nxlog_debug_tag(_T("actions"), 4, _T("H_SystemExecute: started execution of command %s"), command);
         executor.waitForCompletion(g_externalCommandTimeout);
         return ERR_SUCCESS;
      }
      else
      {
         nxlog_debug_tag(_T("actions"), 4, _T("H_SystemExecute: execution failed for command %s"), command);
         return ERR_EXEC_FAILED;
      }
   }
   else
   {
      nxlog_debug_tag(_T("actions"), 4, _T("H_SystemExecute: starting command %s"), command);
      return ProcessExecutor::execute(command) ? ERR_SUCCESS : ERR_EXEC_FAILED;
   }
}

#ifdef _WIN32

/**
 * Log error for ExecuteInAllSessions
 */
static inline void ExecuteInAllSessionsLogError(const TCHAR *function, const TCHAR *message)
{
   TCHAR buffer[1024];
   nxlog_debug_tag(WTS_DEBUG_TAG, 4, _T("%s: %s (%s)"),
         function, message, GetSystemErrorText(GetLastError(), buffer, 1024));
}

/**
 * Execute given command in specific session
 */
bool ExecuteInSession(WTS_SESSION_INFO *session, TCHAR *command, bool allSessions)
{
   const TCHAR *function = allSessions ? _T("ExecuteInAllSessions") : _T("ExecuteInSession");
   nxlog_debug_tag(WTS_DEBUG_TAG, 7, _T("%s: attempting to execute command in session #%u (%s)"),
         function, session->SessionId, session->pWinStationName);
 
   bool success = false;
   HANDLE sessionToken;
   if (WTSQueryUserToken(session->SessionId, &sessionToken))
   {
      HANDLE primaryToken;
      if (DuplicateTokenEx(sessionToken, TOKEN_ALL_ACCESS, NULL, SecurityDelegation, TokenPrimary, &primaryToken))
      {
         STARTUPINFO si;
         memset(&si, 0, sizeof(si));
         si.cb = sizeof(si);
         si.lpDesktop = _T("winsta0\\default");
         PROCESS_INFORMATION pi;
         if (CreateProcessAsUser(primaryToken, NULL, command, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, _T("C:\\"), &si, &pi))
         {
            nxlog_debug_tag(WTS_DEBUG_TAG, 7, _T("%s: process created in session #%u (%s)"),
                  function, session->SessionId, session->pWinStationName);
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
            success = true;
         }
         else
         {
            ExecuteInAllSessionsLogError(function, _T("call to CreateProcessAsUser failed"));
         }
         CloseHandle(primaryToken);
      }
      else
      {
         ExecuteInAllSessionsLogError(function, _T("call to DuplicateTokenEx failed"));
      }
      CloseHandle(sessionToken);
   }
   else
   {
      ExecuteInAllSessionsLogError(function, _T("call to WTSQueryUserToken failed"));
   }
   return success;
}

/**
 * Execute given command for all logged in users
 */
bool ExecuteInAllSessions(const TCHAR *command)
{
   WTS_SESSION_INFO *sessions;
   DWORD sessionCount;
   if (!WTSEnumerateSessions(WTS_CURRENT_SERVER_HANDLE, 0, 1, &sessions, &sessionCount))
   {
      ExecuteInAllSessionsLogError(_T("ExecuteInAllSessions"), _T("call to WTSEnumerateSessions failed"));
      return false;
   }

   TCHAR cmdLine[4096];
   _tcslcpy(cmdLine, command, 4096);

   bool success = true;
   for (DWORD i = 0; i < sessionCount; i++)
   {
      if (sessions[i].SessionId == 0)
         continue;
      ExecuteInSession(&sessions[i], cmdLine, true);
   }

   WTSFreeMemory(sessions);
   return success;
}

#endif
