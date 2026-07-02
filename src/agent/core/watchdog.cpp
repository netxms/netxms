/* 
** NetXMS multiplatform core agent
** Copyright (C) 2003-2025 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be usefu,,
** but ITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: watchdog.cpp
**
**/

#include "nxagentd.h"
#ifdef _WIN32
#include <WtsApi32.h>
#include <tlhelp32.h>
#include <winternl.h>
#else
#include <signal.h>
#include <syslog.h>
#endif

#define DEBUG_TAG _T("watchdog")

StringList GetDisconnectedExtSubagents();
bool IsExternalSubagentConfigured(const TCHAR *name);
uint32_t GetConnectedExtSubagentPid(const TCHAR *name);

/**
 * Static data
 */
static ProcessExecutor *s_watchdog = nullptr;

/**
 * Start watchdog process
 */
void StartWatchdog()
{
   TCHAR szCmdLine[4096], szPlatformSuffixOption[MAX_PSUFFIX_LENGTH + 16];
#ifdef _WIN32
   TCHAR szExecName[MAX_PATH];
   GetModuleFileName(GetModuleHandle(NULL), szExecName, MAX_PATH);
#else
   TCHAR szExecName[MAX_PATH] = PREFIX _T("/bin/nxagentd");
#endif

	if (g_szPlatformSuffix[0] != 0)
	{
		_sntprintf(szPlatformSuffixOption, MAX_PSUFFIX_LENGTH + 16, _T("-P \"%s\" "), g_szPlatformSuffix);
	}
	else
	{
		szPlatformSuffixOption[0] = 0;
	}

   shared_ptr<Config> config = g_config;
   const TCHAR *configSection = config->getAlias(_T("agent"));
   TCHAR configSectionOption[256];
   if ((configSection != nullptr) && (*configSection != 0))
   {
      _sntprintf(configSectionOption, 256, _T("-G %s "), configSection);
   }
   else
   {
      configSectionOption[0] = 0;
   }

#ifdef _WIN32
   _sntprintf(szCmdLine, 4096, _T("\"%s\" -c \"%s\" %s%s%s%s%s%s-D %d %s-W %u"), szExecName,
              g_szConfigFile, configSectionOption,
              (g_dwFlags & AF_DAEMON) ? _T("-d ") : _T(""),
              (g_dwFlags & AF_HIDE_WINDOW) ? _T("-H ") : _T(""),
				  (g_dwFlags & AF_CENTRAL_CONFIG) ? _T("-M ") : _T(""),
				  (g_dwFlags & AF_CENTRAL_CONFIG) ? g_szConfigServer : _T(""),
				  (g_dwFlags & AF_CENTRAL_CONFIG) ? _T(" ") : _T(""),
				  nxlog_get_debug_level(), szPlatformSuffixOption,
              (g_dwFlags & AF_DAEMON) ? 0 : GetCurrentProcessId());
	nxlog_debug_tag(DEBUG_TAG, 1, _T("Starting agent watchdog with command line '%s'"), szCmdLine);
#else
   _sntprintf(szCmdLine, 4096, _T("\"%s\" -c \"%s\" %s%s%s%s%s-D %d %s-W %lu"), szExecName,
              g_szConfigFile, configSectionOption,
              (g_dwFlags & AF_DAEMON) ? _T("-d ") : _T(""),
				  (g_dwFlags & AF_CENTRAL_CONFIG) ? _T("-M ") : _T(""),
				  (g_dwFlags & AF_CENTRAL_CONFIG) ? g_szConfigServer : _T(""),
				  (g_dwFlags & AF_CENTRAL_CONFIG) ? _T(" ") : _T(""),
				  nxlog_get_debug_level(), szPlatformSuffixOption,
              (unsigned long)getpid());
#endif

   s_watchdog = new ProcessExecutor(szCmdLine);
   if (s_watchdog->execute())
   {
      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Watchdog process started"));
   }
   else
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Unable to create process \"%s\""), szCmdLine);
   }
}

/**
 * Stop watchdog process
 */
void StopWatchdog()
{
   s_watchdog->stop();
   delete_and_null(s_watchdog);
   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Watchdog process stopped"));
}

/**
 * Watchdog process main
 */
int WatchdogMain(DWORD pid, const TCHAR *configSection)
{
	TCHAR szPlatformSuffixOption[MAX_PSUFFIX_LENGTH + 16];
	if (g_szPlatformSuffix[0] != 0)
	{
		_sntprintf(szPlatformSuffixOption, MAX_PSUFFIX_LENGTH + 16, _T("-P \"%s\" "), g_szPlatformSuffix);
	}
	else
	{
		szPlatformSuffixOption[0] = 0;
	}

	TCHAR configSectionOption[256];
   if ((configSection != NULL) && (*configSection != 0))
   {
      _sntprintf(configSectionOption, 256, _T("-G %s "), configSection);
   }
   else
   {
      configSectionOption[0] = 0;
   }

#if defined(_WIN32)
   if (pid == 0)
   {
      // Service
		WaitForService(SERVICE_STOPPED);
		StartAgentService();
   }
   else
   {
      HANDLE hProcess;

      hProcess = OpenProcess(SYNCHRONIZE, FALSE, pid);
      if (hProcess != NULL)
      {
         if (WaitForSingleObject(hProcess, INFINITE) != WAIT_TIMEOUT)
         {
				/* TODO: start agent's process */
         }
         CloseHandle(hProcess);
      }
   }
#else
	openlog("nxagentd-watchdog", LOG_PID, LOG_DAEMON);
	while(1)
   {
      sleep(2);
      if (kill(pid, SIGCONT) == -1)
         break;
   }
	syslog(LOG_WARNING, "restarting agent");
	
   TCHAR cmdLine[4096];
   _sntprintf(cmdLine, 4096, _T("\"") PREFIX _T("/bin/nxagentd\" -c \"%s\" %s%s%s%s%s-D %d %s"),
              g_szConfigFile, configSectionOption,
              (g_dwFlags & AF_DAEMON) ? _T("-d ") : _T(""),
	           (g_dwFlags & AF_CENTRAL_CONFIG) ? _T("-M ") : _T(""),
				  (g_dwFlags & AF_CENTRAL_CONFIG) ? g_szConfigServer : _T(""),
				  (g_dwFlags & AF_CENTRAL_CONFIG) ? _T(" ") : _T(""),
				  nxlog_get_debug_level(), szPlatformSuffixOption);
#ifdef UNICODE
	syslog(LOG_INFO, "command line: %ls", cmdLine);
#else
	syslog(LOG_INFO, "command line: %s", cmdLine);
#endif
   if (!ProcessExecutor::execute(cmdLine))
	{
#ifdef UNICODE
		syslog(LOG_ERR, "failed to restart agent (command line: %ls)", cmdLine);
#else
		syslog(LOG_ERR, "failed to restart agent (command line: %s)", cmdLine);
#endif
	}
#endif
	return 1;
}

#ifdef _WIN32

bool ExecuteInSession(WTS_SESSION_INFO *session, TCHAR *command, bool allSessions, HANDLE *processHandle, DWORD *pid);

/**
 * Verify executable
 */
static inline bool VerifyExecutable(const wchar_t *path)
{
   if (VerifyFileSignature(path))
      return true;

   PE_CERT_INFO exeCert, agentCert;
   if (GetPeCertificateInfo(path, &exeCert) && GetPeCertificateInfo(GetAgentExecutableName(), &agentCert))
      return (exeCert.fingerprintSize == agentCert.fingerprintSize) && !memcmp(exeCert.fingerprint, agentCert.fingerprint, exeCert.fingerprintSize);

   return false;
}

/**
 * Watchdog for user agents
 */
void UserAgentWatchdog(TCHAR *executableName)
{
   nxlog_debug_tag(DEBUG_TAG, 1, _T("User agent watchdog started (executable name %s)"), executableName);

   while (!AgentSleepAndCheckForShutdown(60000))
   {
      WTS_SESSION_INFO *sessions;
      DWORD sessionCount;
      if (!WTSEnumerateSessions(WTS_CURRENT_SERVER_HANDLE, 0, 1, &sessions, &sessionCount))
         continue;

      WTS_PROCESS_INFO *processes;
      DWORD processCount;
      if (!WTSEnumerateProcesses(WTS_CURRENT_SERVER_HANDLE, 0, 1, &processes, &processCount))
      {
         WTSFreeMemory(sessions);
         continue;
      }

      for (DWORD i = 0; i < sessionCount; i++)
      {
         if ((sessions[i].State != WTSActive) && (sessions[i].State != WTSConnected))
            continue;

         DWORD sessionId = sessions[i].SessionId;
         bool found = false;
         for (DWORD j = 0; j < processCount; j++)
         {
            if ((processes[j].SessionId == sessionId) && !_tcscmp(processes[j].pProcessName, executableName))
            {
               found = true;
               break;
            }
         }

         if (!found)
         {
            nxlog_debug_tag(DEBUG_TAG, 3, _T("User agent process not found in session #%u (%s)"),
                  sessionId, sessions[i].pWinStationName);
            TCHAR binDir[MAX_PATH];
            GetNetXMSDirectory(nxDirBin, binDir);

            StringBuffer command = _T("\"");
            command.append(binDir);
            command.append(_T("\\"));
            command.append(executableName);
            if (VerifyExecutable(command.cstr() + 1)) // skip leading "
            {
               command.append(_T("\""));
               ExecuteInSession(&sessions[i], command.getBuffer(), false, nullptr, nullptr);
            }
            else
            {
               nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Signature validation failed for user agent executable \"%s\""), command.cstr() + 1);
            }
         }
      }

      WTSFreeMemory(sessions);
      WTSFreeMemory(processes);
   }

   MemFree(executableName);
   nxlog_debug_tag(DEBUG_TAG, 1, _T("User agent watchdog stopped"));
}

/**
 * Tracking state for an external subagent instance started by the watchdog.
 * The process handle is kept open for the lifetime of the instance so that the PID cannot be recycled
 * by Windows while we are still tracking it - checking a raw PID would otherwise race against reuse.
 */
struct ExternalSubagentInstance
{
   HANDLE hProcess;
   DWORD pid;
   time_t startTime;

   ~ExternalSubagentInstance() { CloseHandle(hProcess); }
};

/**
 * Read command line of a process identified by PID. Returns false on failure.
 */
static bool ReadProcessCommandLine(DWORD pid, TCHAR *cmdLine, size_t len)
{
#ifdef __64BIT__
   const DWORD desiredAccess = PROCESS_QUERY_INFORMATION | PROCESS_VM_READ;
#else
   const DWORD desiredAccess = PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_VM_WRITE |
                               PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION;
#endif
   HANDLE hProcess = OpenProcess(desiredAccess, FALSE, pid);
   if (hProcess == nullptr)
      return false;

   bool success = false;

#ifdef __64BIT__
   // NtQueryInformationProcess lives in ntdll.dll, which nxagentd does not link against - resolve it dynamically
   typedef NTSTATUS (NTAPI *NtQueryInformationProcessFn)(HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG);
   NtQueryInformationProcessFn queryInformationProcess = reinterpret_cast<NtQueryInformationProcessFn>(
         GetProcAddress(GetModuleHandle(_T("ntdll.dll")), "NtQueryInformationProcess"));
   ULONG size;
   PROCESS_BASIC_INFORMATION pbi;
   if ((queryInformationProcess != nullptr) &&
       (queryInformationProcess(hProcess, ProcessBasicInformation, &pbi, sizeof(PROCESS_BASIC_INFORMATION), &size) == 0))  // STATUS_SUCCESS
   {
      SIZE_T dummy;
      PEB peb;
      if (ReadProcessMemory(hProcess, pbi.PebBaseAddress, &peb, sizeof(PEB), &dummy))
      {
         RTL_USER_PROCESS_PARAMETERS pp;
         if (ReadProcessMemory(hProcess, peb.ProcessParameters, &pp, sizeof(RTL_USER_PROCESS_PARAMETERS), &dummy))
         {
            size_t bufSize = (pp.CommandLine.Length + 1) * sizeof(WCHAR);
            WCHAR *buffer = static_cast<WCHAR*>(MemAlloc(bufSize));
            if (ReadProcessMemory(hProcess, pp.CommandLine.Buffer, buffer, bufSize, &dummy))
            {
               buffer[pp.CommandLine.Length / sizeof(WCHAR)] = 0;
#ifdef UNICODE
               wcslcpy(cmdLine, buffer, len);
#else
               WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, buffer, -1, cmdLine, static_cast<int>(len), nullptr, nullptr);
               cmdLine[len - 1] = 0;
#endif
               success = true;
            }
            MemFree(buffer);
         }
      }
   }
#else
   FARPROC pfnGetCommandLine = GetProcAddress(GetModuleHandle(_T("KERNEL32.DLL")),
#ifdef UNICODE
      "GetCommandLineW");
#else
      "GetCommandLineA");
#endif
   HANDLE hThread = CreateRemoteThread(hProcess, 0, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(pfnGetCommandLine), 0, 0, 0);
   if (hThread != nullptr)
   {
      WaitForSingleObject(hThread, INFINITE);
      DWORD addressOfCommandLine;
      GetExitCodeThread(hThread, &addressOfCommandLine);
      SIZE_T dummy;
      success = ReadProcessMemory(hProcess, reinterpret_cast<PVOID>(static_cast<uintptr_t>(addressOfCommandLine)), cmdLine, len * sizeof(TCHAR), &dummy);
      cmdLine[len - 1] = 0;
      CloseHandle(hThread);
   }
#endif

   CloseHandle(hProcess);
   return success;
}

/**
 * Extract the external subagent name from an agent command line of the form
 * "<agent executable>" ... -G EXT:<name>. Returns false if the command line does not
 * belong to an external subagent started from this agent's executable.
 */
static bool ParseExternalSubagentName(const TCHAR *cmdLine, TCHAR *name, size_t nameLen)
{
   if (_tcsstr(cmdLine, GetAgentExecutableName()) == nullptr)
      return false;   // not our executable - could be a different agent installation

   const TCHAR *marker = _tcsstr(cmdLine, _T("-G EXT:"));
   if (marker == nullptr)
      return false;
   marker += 7;   // length of "-G EXT:"

   size_t i = 0;
   while ((marker[i] != 0) && (marker[i] != _T(' ')) && (marker[i] != _T('\t')) && (i < nameLen - 1))
   {
      name[i] = marker[i];
      i++;
   }
   name[i] = 0;
   return i > 0;
}

/**
 * Forcibly terminate process identified by PID
 */
static void TerminateProcessByPid(DWORD pid)
{
   HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
   if (hProcess != nullptr)
   {
      TerminateProcess(hProcess, 0);
      CloseHandle(hProcess);
   }
}

/**
 * Terminate orphaned and duplicate external subagent processes.
 *
 * External subagent processes survive a master agent restart and reconnect on their own, but the
 * in-memory tracking of the watchdog does not survive a restart. This sweep reconciles the actual
 * running processes against configuration and the currently connected instances, and terminates:
 *   - orphans: name no longer present in configuration (e.g. its section was removed);
 *   - duplicates: name is configured and connected, but this process is not the connected instance.
 * A configured name with no connected instance yet is left alone - it may still be initializing, and
 * we cannot tell a stale instance from a legitimate one until one of them connects.
 */
static void ReconcileExternalSubagentProcesses()
{
   HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
   if (snapshot == INVALID_HANDLE_VALUE)
   {
      TCHAR errorText[1024];
      nxlog_debug_tag(DEBUG_TAG, 6, _T("ReconcileExternalSubagentProcesses: CreateToolhelp32Snapshot failed (%s)"), GetSystemErrorText(GetLastError(), errorText, 1024));
      return;
   }

   const TCHAR *exePath = GetAgentExecutableName();
   const TCHAR *exeBaseName = _tcsrchr(exePath, _T('\\'));
   exeBaseName = (exeBaseName != nullptr) ? exeBaseName + 1 : exePath;
   DWORD selfPid = GetCurrentProcessId();

   PROCESSENTRY32 pe;
   pe.dwSize = sizeof(PROCESSENTRY32);
   if (Process32First(snapshot, &pe))
   {
      do
      {
         if ((pe.th32ProcessID == selfPid) || _tcsicmp(pe.szExeFile, exeBaseName))
            continue;   // not our executable image

         TCHAR cmdLine[4096];
         if (!ReadProcessCommandLine(pe.th32ProcessID, cmdLine, 4096))
            continue;

         TCHAR name[MAX_SUBAGENT_NAME];
         if (!ParseExternalSubagentName(cmdLine, name, MAX_SUBAGENT_NAME))
            continue;

         if (!IsExternalSubagentConfigured(name))
         {
            nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Terminating orphaned external subagent process (PID %u): %s is no longer configured"), pe.th32ProcessID, name);
            TerminateProcessByPid(pe.th32ProcessID);
            continue;
         }

         uint32_t connectedPid = GetConnectedExtSubagentPid(name);
         if ((connectedPid != 0) && (pe.th32ProcessID != connectedPid))
         {
            nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Terminating duplicate external subagent %s process (PID %u); connected instance is PID %u"), name, pe.th32ProcessID, connectedPid);
            TerminateProcessByPid(pe.th32ProcessID);
         }
      } while (Process32Next(snapshot, &pe));
   }

   CloseHandle(snapshot);
}

/**
 * Watchdog for external subagents
 */
void ExternalSubagentWatchdog()
{
   nxlog_debug_tag(DEBUG_TAG, 1, L"External subagent watchdog started");

   // Time (in seconds) given to a freshly started external subagent to initialize and connect to the
   // master agent before it is considered hung. Device autodetection can take a long time, and until the
   // subagent connects it is indistinguishable from a not-running one, so it must not be relaunched too early.
   uint32_t startupTimeout = g_config->getValueAsUInt(_T("/CORE/ExternalSubagentStartupTimeout"), 120);
   if (startupTimeout < 30)
      startupTimeout = 30;
   nxlog_debug_tag(DEBUG_TAG, 3, _T("External subagent startup timeout set to %u seconds"), startupTimeout);

   // Instances started by this watchdog, keyed by subagent name
   StringObjectMap<ExternalSubagentInstance> instances(Ownership::True);

   while (!AgentSleepAndCheckForShutdown(60000))
   {
      // Reap orphaned and duplicate subagent processes (e.g. left over from a previous master agent
      // instance, or from a removed configuration section) before deciding what to relaunch.
      ReconcileExternalSubagentProcesses();

      StringList disconnectedSubagents = GetDisconnectedExtSubagents();
      if (disconnectedSubagents.isEmpty())
         continue;

      nxlog_debug_tag(DEBUG_TAG, 6, L"%d external subagent%s not connected", disconnectedSubagents.size(), disconnectedSubagents.size() == 1 ? L" is" : L"s are");

      // Decide which subagents actually need to be (re)started. A subagent whose process was started by
      // this watchdog and is still alive is either still initializing (given time until startup timeout)
      // or hung (terminated before relaunch) - either way it must not be launched again blindly, otherwise
      // duplicate instances would compete for the same config directory and local database.
      StringList subagentsToStart;
      time_t now = time(nullptr);
      for(int i = 0; i < disconnectedSubagents.size(); i++)
      {
         const TCHAR *name = disconnectedSubagents.get(i);
         ExternalSubagentInstance *instance = instances.get(name);
         if ((instance != nullptr) && (WaitForSingleObject(instance->hProcess, 0) == WAIT_TIMEOUT))
         {
            if (now - instance->startTime < static_cast<time_t>(startupTimeout))
            {
               nxlog_debug_tag(DEBUG_TAG, 6, _T("External subagent %s (PID %u) is still initializing (%d of %u seconds elapsed)"),
                     name, instance->pid, static_cast<int>(now - instance->startTime), startupTimeout);
               continue;
            }
            nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("External subagent %s (PID %u) did not connect within %u seconds, terminating hung process"),
                  name, instance->pid, startupTimeout);
            TerminateProcess(instance->hProcess, 0);
         }
         subagentsToStart.add(name);
      }

      if (subagentsToStart.isEmpty())
         continue;

      // Find session to run
      WTS_SESSION_INFO *sessions;
      DWORD sessionCount;
      if (!WTSEnumerateSessions(WTS_CURRENT_SERVER_HANDLE, 0, 1, &sessions, &sessionCount))
         continue;

      bool found = false;
      for (DWORD i = 0; i < sessionCount; i++)
      {
         if ((sessions[i].State == WTSActive) || (sessions[i].State == WTSConnected))
         {
            for (int j = 0; j < subagentsToStart.size(); j++)
            {
               const wchar_t *name = subagentsToStart.get(j);
               StringBuffer command = L"\"";
               command.append(GetAgentExecutableName());
               command.append(L"\" -H -G EXT:");
               command.append(name);
               HANDLE hProcess = nullptr;
               DWORD pid = 0;
               if (ExecuteInSession(&sessions[i], command.getBuffer(), false, &hProcess, &pid))
               {
                  nxlog_debug_tag(DEBUG_TAG, 6, L"Started external subagent %s (PID %u) in session %u", name, pid, sessions[i].SessionId);
                  ExternalSubagentInstance *instance = new ExternalSubagentInstance();
                  instance->hProcess = hProcess;
                  instance->pid = pid;
                  instance->startTime = time(nullptr);
                  instances.set(name, instance);   // replacing a tracked instance closes its previous handle
               }
               else
               {
                  nxlog_debug_tag(DEBUG_TAG, 6, L"Cannot start external subagent %s in session %u", name, sessions[i].SessionId);
               }
            }
            found = true;
            break;
         }
      }

      if (!found)
         nxlog_debug_tag(DEBUG_TAG, 6, L"Cannot find suitable session for starting external subagent");

      WTSFreeMemory(sessions);
   }

   nxlog_debug_tag(DEBUG_TAG, 1, L"External subagent watchdog stopped");
}

#endif
