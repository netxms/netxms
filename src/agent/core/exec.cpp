/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2021 Victor Kirhenshtein
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
 * Exec function for external (user-defined) parameters and lists. Handler argument contains command line before substitution.
 */
static LONG RunExternal(const TCHAR *param, const TCHAR *arg, StringList *value)
{
   nxlog_debug_tag(EXEC_DEBUG_TAG, 4, _T("RunExternal called for \"%s\" \"%s\""), param, arg);

   // Substitute $1 .. $9 with actual arguments
   StringBuffer cmdLine;
   cmdLine.setAllocationStep(1024);
   for(const TCHAR *sptr = &arg[1]; *sptr != 0; sptr++)
   {
      if (*sptr == _T('$'))
      {
         sptr++;
         if (*sptr == 0)
            break;   // Single $ character at the end of line
         if ((*sptr >= _T('1')) && (*sptr <= _T('9')))
         {
            TCHAR buffer[1024];
            if (AgentGetParameterArg(param, *sptr - '0', buffer, 1024))
            {
               cmdLine.append(buffer);
            }
         }
         else
         {
            cmdLine.append(*sptr);
         }
      }
      else
      {
         cmdLine.append(*sptr);
      }
   }
   nxlog_debug_tag(EXEC_DEBUG_TAG, 4, _T("RunExternal: command line is \"%s\""), cmdLine.cstr());

   LineOutputProcessExecutor executor(cmdLine, *arg == _T('S'));
   if (!executor.execute())
   {
      nxlog_debug_tag(EXEC_DEBUG_TAG, 4, _T("RunExternal: cannot start process (command line \"%s\")"), cmdLine.cstr());
      return SYSINFO_RC_ERROR;
   }

   if (!executor.waitForCompletion(g_execTimeout))
   {
      nxlog_debug_tag(EXEC_DEBUG_TAG, 4, _T("RunExternal: external process execution timeout (command line \"%s\")"), cmdLine.cstr());
      return SYSINFO_RC_ERROR;
   }

   value->addAll(executor.getData());
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler function for external (user-defined) parameters
 */
LONG H_ExternalParameter(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   session->debugPrintf(4, _T("H_ExternalParameter called for \"%s\" \"%s\""), cmd, arg);
   StringList values;
   LONG status = RunExternal(cmd, arg, &values);
   if (status == SYSINFO_RC_SUCCESS)
   {
      ret_string(value, values.size() > 0 ? values.get(0) : _T(""));
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
   TCHAR **columns = SplitString(data.get(0), td.separator, &numColumns);
   for(int n = 0; n < numColumns; n++)
   {
      bool instanceColumn = false;
      for(int i = 0; i < td.instanceColumnCount; i++)
         if (!_tcsicmp(td.instanceColumns[i], columns[n]))
         {
            instanceColumn = true;
            break;
         }
      table->addColumn(columns[n], DCI_DT_INT, columns[n], instanceColumn);
      MemFree(columns[n]);
   }
   MemFree(columns);

   for(int i = 1; i < data.size(); i++)
   {
      table->addRow();
      int count = 0;
      TCHAR **values = SplitString(data.get(i), td.separator, &count);
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
   session->debugPrintf(4, _T("H_ExternalTable called for \"%s\" (separator=0x%04X mode=%c cmd=\"%s\""), cmd, td->separator, td->cmdLine[0], &td->cmdLine[1]);
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
