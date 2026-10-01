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
#include <userenv.h>
#include <tlhelp32.h>
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
   uint32_t m_completionCode;

protected:
   virtual void onOutput(const char *text, size_t length) override;
   virtual void endOfOutput() override;

public:
   SystemActionProcessExecutor(const TCHAR *command, ActionExecutionContext *context) : ProcessExecutor(command)
   {
      m_context = context;
      m_sendOutput = true;
      m_replaceNullCharacters = true;
      m_completionCode = ERR_SUCCESS;
   }
   virtual ~SystemActionProcessExecutor() = default;

   /**
    * Set code to be reported to server as command execution result. Must be called before process is
    * terminated, so that output reader thread will see it when sending end of output marker.
    */
   void setCompletionCode(uint32_t rcc) { m_completionCode = rcc; }
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
   m_context->sendEndOfOutputMarker(m_completionCode);
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
         if (!executor.waitForCompletion(g_externalCommandTimeout))
         {
            nxlog_write_tag(NXLOG_WARNING, _T("actions"), _T("Command \"%s\" started by System.Execute terminated after exceeding execution time limit of %u seconds"),
                  command, g_externalCommandTimeout / 1000);
            executor.setCompletionCode(ERR_EXEC_TIMEOUT);
            executor.stop();
         }
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
 * Enumerate user sessions. When WTS enumeration is not available (for example, Terminal Services
 * service is stopped), active console session is returned as the only session.
 */
StructArray<UserSession> EnumerateUserSessions()
{
   StructArray<UserSession> result;
   DWORD consoleSessionId = WTSGetActiveConsoleSessionId();

   WTS_SESSION_INFO *sessions;
   DWORD sessionCount;
   if (WTSEnumerateSessions(WTS_CURRENT_SERVER_HANDLE, 0, 1, &sessions, &sessionCount))
   {
      for (DWORD i = 0; i < sessionCount; i++)
      {
         UserSession *s = result.addPlaceholder();
         s->id = sessions[i].SessionId;
         s->state = sessions[i].State;
         _tcslcpy(s->name, sessions[i].pWinStationName, 64);
         s->console = (sessions[i].SessionId == consoleSessionId);
      }
      WTSFreeMemory(sessions);
      return result;
   }

   TCHAR buffer[1024];
   nxlog_debug_tag(WTS_DEBUG_TAG, 6, _T("EnumerateUserSessions: call to WTSEnumerateSessions failed (%s)"),
         GetSystemErrorText(GetLastError(), buffer, 1024));
   if (consoleSessionId == 0xFFFFFFFF)
   {
      nxlog_debug_tag(WTS_DEBUG_TAG, 6, _T("EnumerateUserSessions: no active console session"));
      return result;
   }

   nxlog_debug_tag(WTS_DEBUG_TAG, 6, _T("EnumerateUserSessions: using console session #%u"), consoleSessionId);
   UserSession *s = result.addPlaceholder();
   s->id = consoleSessionId;
   s->state = WTSActive;
   _tcslcpy(s->name, _T("Console"), 64);
   s->console = true;
   return result;
}

/**
 * Check if given process token belongs to interactively logged on user (not to system or service account)
 */
static bool IsInteractiveUserToken(HANDLE token)
{
   union
   {
      TOKEN_USER tokenUser;
      BYTE buffer[sizeof(TOKEN_USER) + SECURITY_MAX_SID_SIZE];
   } user;
   DWORD size;
   if (!GetTokenInformation(token, TokenUser, &user, sizeof(user), &size))
      return false;

   PSID userSid = user.tokenUser.User.Sid;
   if (IsWellKnownSid(userSid, WinLocalSystemSid) || IsWellKnownSid(userSid, WinLocalServiceSid) || IsWellKnownSid(userSid, WinNetworkServiceSid))
      return false;

   // Desktop Window Manager (S-1-5-90-*) and font driver host (S-1-5-96-*) virtual accounts are interactive but do not represent a user
   SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
   if (!memcmp(GetSidIdentifierAuthority(userSid), &ntAuthority, sizeof(SID_IDENTIFIER_AUTHORITY)) && (*GetSidSubAuthorityCount(userSid) > 0))
   {
      DWORD rid = *GetSidSubAuthority(userSid, 0);
      if ((rid == 90) || (rid == 96))
         return false;
   }

   GetTokenInformation(token, TokenGroups, nullptr, 0, &size);
   if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
      return false;

   TOKEN_GROUPS *groups = static_cast<TOKEN_GROUPS*>(MemAlloc(size));
   bool interactive = false;
   if (GetTokenInformation(token, TokenGroups, groups, size, &size))
   {
      for (DWORD i = 0; i < groups->GroupCount; i++)
      {
         if (IsWellKnownSid(groups->Groups[i].Sid, WinInteractiveSid))
         {
            interactive = true;
            break;
         }
      }
   }
   MemFree(groups);
   return interactive;
}

/**
 * Find token of interactively logged on user in given session by scanning processes running in that session.
 * Returned token handle should be closed by caller. Returns nullptr if there are no interactive user processes in the session.
 */
HANDLE FindInteractiveUserToken(DWORD sessionId)
{
   HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
   if (snapshot == INVALID_HANDLE_VALUE)
   {
      TCHAR buffer[1024];
      nxlog_debug_tag(WTS_DEBUG_TAG, 6, _T("FindInteractiveUserToken: call to CreateToolhelp32Snapshot failed (%s)"),
            GetSystemErrorText(GetLastError(), buffer, 1024));
      return nullptr;
   }

   // Earliest created process is the one started by logon sequence for console user; processes started later
   // via runas or elevation may have logon SID without access to winsta0\default, so their token is unusable
   HANDLE userToken = nullptr;
   FILETIME userTokenProcessCreationTime;
   DWORD userTokenProcessId = 0;
   TCHAR userTokenProcessName[MAX_PATH];

   PROCESSENTRY32 pe;
   pe.dwSize = sizeof(PROCESSENTRY32);
   if (Process32First(snapshot, &pe))
   {
      do
      {
         DWORD processSessionId;
         if (!ProcessIdToSessionId(pe.th32ProcessID, &processSessionId) || (processSessionId != sessionId))
            continue;

         HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pe.th32ProcessID);
         if (hProcess == nullptr)
            continue;

         HANDLE token;
         if (OpenProcessToken(hProcess, TOKEN_QUERY | TOKEN_DUPLICATE, &token))
         {
            FILETIME creationTime, exitTime, kernelTime, userTime;
            if (IsInteractiveUserToken(token) && GetProcessTimes(hProcess, &creationTime, &exitTime, &kernelTime, &userTime) &&
                ((userToken == nullptr) || (CompareFileTime(&creationTime, &userTokenProcessCreationTime) < 0)))
            {
               if (userToken != nullptr)
                  CloseHandle(userToken);
               userToken = token;
               userTokenProcessCreationTime = creationTime;
               userTokenProcessId = pe.th32ProcessID;
               _tcslcpy(userTokenProcessName, pe.szExeFile, MAX_PATH);
            }
            else
            {
               CloseHandle(token);
            }
         }
         CloseHandle(hProcess);
      } while (Process32Next(snapshot, &pe));
   }
   CloseHandle(snapshot);

   if (userToken != nullptr)
      nxlog_debug_tag(WTS_DEBUG_TAG, 6, _T("FindInteractiveUserToken: using token of process %s (PID %u) in session #%u"),
            userTokenProcessName, userTokenProcessId, sessionId);
   return userToken;
}

/**
 * Execute given command in specific session
 */
bool ExecuteInSession(const UserSession& session, TCHAR *command, bool allSessions, HANDLE *processHandle, DWORD *pid)
{
   const TCHAR *function = allSessions ? _T("ExecuteInAllSessions") : _T("ExecuteInSession");
   nxlog_debug_tag(WTS_DEBUG_TAG, 7, _T("%s: attempting to execute command in session #%u (%s)"),
         function, session.id, session.name);

   HANDLE sessionToken;
   if (!WTSQueryUserToken(session.id, &sessionToken))
   {
      TCHAR buffer[1024];
      nxlog_debug_tag(WTS_DEBUG_TAG, 6, _T("%s: call to WTSQueryUserToken for session #%u (%s) failed (%s)"),
            function, session.id, session.name, GetSystemErrorText(GetLastError(), buffer, 1024));
      sessionToken = FindInteractiveUserToken(session.id);
      if (sessionToken == nullptr)
      {
         nxlog_debug_tag(WTS_DEBUG_TAG, 6, _T("%s: no interactive user in session #%u (%s)"), function, session.id, session.name);
         return false;
      }
   }

   bool success = false;
   HANDLE primaryToken;
   if (DuplicateTokenEx(sessionToken, TOKEN_ALL_ACCESS, NULL, SecurityDelegation, TokenPrimary, &primaryToken))
   {
      // Without explicit environment block new process inherits environment of the agent service (USERPROFILE,
      // APPDATA, etc. of the service account), and per-user folder lookup in it then resolves to service profile
      // or fails, depending on token elevation
      void *environment = nullptr;
      if (!CreateEnvironmentBlock(&environment, primaryToken, FALSE))
      {
         ExecuteInAllSessionsLogError(function, _T("call to CreateEnvironmentBlock failed"));
         environment = nullptr;
      }

      STARTUPINFO si;
      memset(&si, 0, sizeof(si));
      si.cb = sizeof(si);
      si.lpDesktop = _T("winsta0\\default");
      PROCESS_INFORMATION pi;
      if (CreateProcessAsUser(primaryToken, NULL, command, NULL, NULL, FALSE, CREATE_NEW_CONSOLE | CREATE_UNICODE_ENVIRONMENT, environment, _T("C:\\"), &si, &pi))
      {
         nxlog_debug_tag(WTS_DEBUG_TAG, 7, _T("%s: process created in session #%u (%s), PID %u"),
               function, session.id, session.name, pi.dwProcessId);
         if (pid != nullptr)
            *pid = pi.dwProcessId;
         CloseHandle(pi.hThread);
         if (processHandle != nullptr)
            *processHandle = pi.hProcess;   // caller takes ownership of the handle
         else
            CloseHandle(pi.hProcess);
         success = true;
      }
      else
      {
         ExecuteInAllSessionsLogError(function, _T("call to CreateProcessAsUser failed"));
      }
      if (environment != nullptr)
         DestroyEnvironmentBlock(environment);
      CloseHandle(primaryToken);
   }
   else
   {
      ExecuteInAllSessionsLogError(function, _T("call to DuplicateTokenEx failed"));
   }
   CloseHandle(sessionToken);
   return success;
}

/**
 * Execute given command for all logged in users
 */
bool ExecuteInAllSessions(const TCHAR *command)
{
   StructArray<UserSession> sessions = EnumerateUserSessions();
   if (sessions.isEmpty())
      return false;

   TCHAR cmdLine[4096];
   _tcslcpy(cmdLine, command, 4096);

   bool success = true;
   for (int i = 0; i < sessions.size(); i++)
   {
      const UserSession *s = sessions.get(i);
      if ((s->id == 0) && !s->console)
         continue;
      ExecuteInSession(*s, cmdLine, true, nullptr, nullptr);
   }

   return success;
}

#endif
