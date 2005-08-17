/* 
** NetXMS multiplatform core agent
** Copyright (C) 2003, 2004, 2005 Victor Kirhenshtein
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
** $module: nxagentd.cpp
**
**/

#include "nxagentd.h"

#if defined(_WIN32)
#include <conio.h>
#elif defined(_NETWARE)
#include <screen.h>
#else
# include <signal.h>
# include <sys/wait.h>
#endif

//
// Externals
//

THREAD_RESULT THREAD_CALL ListenerThread(void *);
THREAD_RESULT THREAD_CALL SessionWatchdog(void *);


//
// Valid options for getopt()
//

#if defined(_WIN32)
#define VALID_OPTIONS   "c:CdDEhHIRsSUvX:"
#elif defined(_NETWARE)
#define VALID_OPTIONS   "c:CDhvX:"
#else
#define VALID_OPTIONS   "c:CdDhp:vX:"
#endif


//
// Actions
//

#define ACTION_NONE                    0
#define ACTION_RUN_AGENT               1
#define ACTION_INSTALL_SERVICE         2
#define ACTION_REMOVE_SERVICE          3
#define ACTION_START_SERVICE           4
#define ACTION_STOP_SERVICE            5
#define ACTION_CHECK_CONFIG            6
#define ACTION_INSTALL_EVENT_SOURCE    7
#define ACTION_REMOVE_EVENT_SOURCE     8


//
// Global variables
//

DWORD g_dwFlags = 0;
char g_szLogFile[MAX_PATH] = AGENT_DEFAULT_LOG;
char g_szSharedSecret[MAX_SECRET_LENGTH] = "admin";
char g_szConfigFile[MAX_PATH] = AGENT_DEFAULT_CONFIG;
char g_szFileStore[MAX_PATH] = AGENT_DEFAULT_FILE_STORE;
char g_szPlatformSuffix[MAX_PSUFFIX_LENGTH] = "";
WORD g_wListenPort = AGENT_LISTEN_PORT;
SERVER_INFO g_pServerList[MAX_SERVERS];
DWORD g_dwServerCount = 0;
DWORD g_dwTimeOut = 5000;     // Request timeout in milliseconds
time_t g_dwAgentStartTime;
DWORD g_dwStartupDelay = 0;
DWORD g_dwIdleTimeout = 60;   // Session idle timeout
DWORD g_dwMaxSessions = 32;

#if !defined(_WIN32) && !defined(_NETWARE)
char g_szPidFile[MAX_PATH] = "/var/run/nxagentd.pid";
#endif

#ifdef _WIN32
BOOL (__stdcall *imp_GlobalMemoryStatusEx)(LPMEMORYSTATUSEX);
DWORD (__stdcall *imp_HrLanConnectionNameFromGuidOrPath)(LPWSTR, LPWSTR, LPWSTR, LPDWORD);
#endif   /* _WIN32 */


//
// Static variables
//

static char *m_pszActionList = NULL;
static char *m_pszServerList = NULL;
static char *m_pszControlServerList = NULL;
static char *m_pszInstallServerList = NULL;
static char *m_pszSubagentList = NULL;
static char *m_pszExtParamList = NULL;
static CONDITION m_hCondShutdown = INVALID_CONDITION_HANDLE;
static DWORD m_dwEnabledCiphers = 0xFFFF;

#if !defined(_WIN32) && !defined(_NETWARE)
static pid_t m_pid;
#endif


//
// Configuration file template
//

static NX_CFG_TEMPLATE m_cfgTemplate[] =
{
   { "Action", CT_STRING_LIST, '\n', 0, 0, 0, &m_pszActionList },
   { "ControlServers", CT_STRING_LIST, ',', 0, 0, 0, &m_pszControlServerList },
   { "EnableActions", CT_BOOLEAN, 0, 0, AF_ENABLE_ACTIONS, 0, &g_dwFlags },
   { "EnabledCiphers", CT_LONG, 0, 0, 0, 0, &m_dwEnabledCiphers },
   { "ExternalParameter", CT_STRING_LIST, '\n', 0, 0, 0, &m_pszExtParamList },
   { "FileStore", CT_STRING, 0, 0, MAX_PATH, 0, g_szFileStore },
   { "InstallationServers", CT_STRING_LIST, ',', 0, 0, 0, &m_pszInstallServerList },
   { "ListenPort", CT_WORD, 0, 0, 0, 0, &g_wListenPort },
   { "LogFile", CT_STRING, 0, 0, MAX_PATH, 0, g_szLogFile },
   { "LogUnresolvedSymbols", CT_BOOLEAN, 0, 0, AF_LOG_UNRESOLVED_SYMBOLS, 0, &g_dwFlags },
   { "MaxSessions", CT_LONG, 0, 0, 0, 0, &g_dwMaxSessions },
   { "PlatformSuffix", CT_STRING, 0, 0, MAX_PSUFFIX_LENGTH, 0, g_szPlatformSuffix },
   { "RequireAuthentication", CT_BOOLEAN, 0, 0, AF_REQUIRE_AUTH, 0, &g_dwFlags },
   { "RequireEncryption", CT_BOOLEAN, 0, 0, AF_REQUIRE_ENCRYPTION, 0, &g_dwFlags },
   { "Servers", CT_STRING_LIST, ',', 0, 0, 0, &m_pszServerList },
   { "SessionIdleTimeout", CT_LONG, 0, 0, 0, 0, &g_dwIdleTimeout },
   { "SharedSecret", CT_STRING, 0, 0, MAX_SECRET_LENGTH, 0, g_szSharedSecret },
   { "StartupDelay", CT_LONG, 0, 0, 0, 0, &g_dwStartupDelay },
   { "SubAgent", CT_STRING_LIST, '\n', 0, 0, 0, &m_pszSubagentList },
   { "Timeout", CT_LONG, 0, 0, 0, 0, &g_dwTimeOut },
   { "", CT_END_OF_LIST, 0, 0, 0, 0, NULL }
};


//
// Help text
//

static char m_szHelpText[] =
   "Usage: nxagentd [options]\n"
   "Where valid options are:\n"
   "   -c <file>  : Use configuration file <file> (default " AGENT_DEFAULT_CONFIG ")\n"
   "   -C         : Check configuration file and exit\n"
#ifndef _NETWARE
   "   -d         : Run as daemon/service\n"
#endif
   "   -D         : Turn on debug output\n"
   "   -h         : Display help and exit\n"
#ifdef _WIN32
   "   -H         : Hide agent's window when in standalone mode\n"
   "   -I         : Install Windows service\n"
   "   -R         : Remove Windows service\n"
   "   -s         : Start Windows servive\n"
   "   -S         : Stop Windows service\n"
#endif
   "   -v         : Display version and exit\n"
   "\n";


#ifdef _WIN32

//
// Get our own console window handle (an alternative to Microsoft's GetConsoleWindow)
//

static HWND GetConsoleHWND(void)
{
	HWND hWnd;
	DWORD wpid, cpid;

   cpid = GetCurrentProcessId();
   while(1)
   {
	   hWnd = FindWindowEx(NULL, NULL, _T("ConsoleWindowClass"), NULL);
      if (hWnd == NULL)
         break;
	   
      GetWindowThreadProcessId(hWnd, &wpid);
	   if (cpid == wpid)
         break;
   }

	return hWnd;
}


//
// Get proc address and write log file
//

static FARPROC GetProcAddressAndLog(HMODULE hModule, LPCSTR procName)
{
   FARPROC ptr;

   ptr = GetProcAddress(hModule, procName);
   if ((ptr == NULL) && (g_dwFlags & AF_LOG_UNRESOLVED_SYMBOLS))
      WriteLog(MSG_NO_FUNCTION, EVENTLOG_WARNING_TYPE, "s", procName);
   return ptr;
}


//
// Import symbols
//

static void ImportSymbols(void)
{
   HMODULE hModule;

   // KERNEL32.DLL
   hModule = GetModuleHandle("KERNEL32.DLL");
   if (hModule != NULL)
   {
      imp_GlobalMemoryStatusEx = (BOOL (__stdcall *)(LPMEMORYSTATUSEX))GetProcAddressAndLog(hModule,"GlobalMemoryStatusEx");
   }
   else
   {
      WriteLog(MSG_NO_DLL, EVENTLOG_WARNING_TYPE, "s", "KERNEL32.DLL");
   }

   // NETMAN.DLL
   hModule = LoadLibrary("NETMAN.DLL");
   if (hModule != NULL)
   {
      imp_HrLanConnectionNameFromGuidOrPath = 
         (DWORD (__stdcall *)(LPWSTR, LPWSTR, LPWSTR, LPDWORD))GetProcAddressAndLog(hModule,
            "HrLanConnectionNameFromGuidOrPath");
   }
   else
   {
      WriteLog(MSG_NO_DLL, EVENTLOG_WARNING_TYPE, "s", "NETMAN.DLL");
   }
}


//
// Shutdown thread (created by H_RestartAgent)
//

static THREAD_RESULT THREAD_CALL ShutdownThread(void *pArg)
{
   Shutdown();
   ExitProcess(0);
   return THREAD_OK; // Never reached
}

#endif   /* _WIN32 */


//
// Restart agent
//

static LONG H_RestartAgent(TCHAR *pszAction, NETXMS_VALUES_LIST *pArgs, TCHAR *pData)
{
   TCHAR szCmdLine[4096];
#ifdef _WIN32
   TCHAR szExecName[MAX_PATH];
   DWORD dwResult;
   STARTUPINFO si;
   PROCESS_INFORMATION pi;

   GetModuleFileName(GetModuleHandle(NULL), szExecName, MAX_PATH);
#else
   TCHAR szExecName[MAX_PATH] = PREFIX _T("/bin/nxagentd");
#endif

#ifdef _WIN32
   _sntprintf(szCmdLine, 4096, _T("\"%s\" %s%s-X %lu"), szExecName, 
              (g_dwFlags & AF_DAEMON) ? _T("-d ") : _T(""),
              (g_dwFlags & AF_HIDE_WINDOW) ? _T("-H ") : _T(""),
              (g_dwFlags & AF_DAEMON) ? 0 : GetCurrentProcessId());

   // Fill in process startup info structure
   memset(&si, 0, sizeof(STARTUPINFO));
   si.cb = sizeof(STARTUPINFO);

   // Create new process
   if (!CreateProcess(NULL, szCmdLine, NULL, NULL, FALSE, 
                      (g_dwFlags & AF_DAEMON) ? (CREATE_NO_WINDOW | DETACHED_PROCESS) : (CREATE_NEW_CONSOLE),
                      NULL, NULL, &si, &pi))
   {
      WriteLog(MSG_CREATE_PROCESS_FAILED, EVENTLOG_ERROR_TYPE, "se", szCmdLine, GetLastError());
      dwResult = ERR_EXEC_FAILED;
   }
   else
   {
      // Close all handles
      CloseHandle(pi.hThread);
      CloseHandle(pi.hProcess);
      dwResult = ERR_SUCCESS;
   }
   if ((dwResult == ERR_SUCCESS) && (!(g_dwFlags & AF_DAEMON)))
   {
      if (g_dwFlags & AF_HIDE_WINDOW)
      {
         ConditionSet(m_hCondShutdown);
      }
      else
      {
         ThreadCreate(ShutdownThread, 0, NULL);
      }
   }
   return dwResult;
#else
   _sntprintf(szCmdLine, 4096, _T("\"%s\" %s-X %lu"), szExecName,
              (g_dwFlags & AF_DAEMON) ? _T("-d ") : _T(""), m_pid);
   return ExecuteCommand(szCmdLine, NULL);
#endif
}


//
// This function writes message from subagent to agent's log
//

static void WriteSubAgentMsg(int iLevel, TCHAR *pszMsg)
{
   WriteLog(MSG_SUBAGENT_MSG, iLevel, "s", pszMsg);
}


//
// Signal handler for UNIX platforms
//

#ifndef _WIN32

static int m_XXX = 0;

void OnSignal(int iSignal)
{
	//WriteLog(MSG_SIGNAL_RECEIVED, EVENTLOG_WARNING_TYPE, "d", iSignal);
	switch(iSignal)
	{
		case SIGTERM:
		case SIGINT:
			if (m_XXX == 0)
			{
				ConditionSet(m_hCondShutdown);
				m_XXX = 1;
			}
			break;
		case SIGSEGV:
			abort();
			break;
#ifndef _NETWARE
		case SIGCHLD:
			while (waitpid(-1, NULL, WNOHANG) > 0)
				;
			break;
#endif
		default:
			break;
	}
}

#endif


//
// Load subagent for Windows NT or Windows 9x
//

#ifdef _WIN32

void LoadWindowsSubagent(void)
{
   OSVERSIONINFO ver;

   ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
   if (GetVersionEx(&ver))
   {
      switch(ver.dwPlatformId)
      {
         case VER_PLATFORM_WIN32_WINDOWS:    // Windows 9x
            LoadSubAgent("WIN9X.NSM");
            break;
         case VER_PLATFORM_WIN32_NT:   // Windows NT or higher
            LoadSubAgent("WINNT.NSM");
            break;
         default:
            break;
      }
   }
   else
   {
      WriteLog(MSG_GETVERSION_FAILED, EVENTLOG_WARNING_TYPE, "e", GetLastError());
   }
}

#endif


//
// Initialization routine
//

BOOL Initialize(void)
{
   char *pItem, *pEnd;

   // Open log file
   InitLog();

#ifdef _WIN32
   WSADATA wsaData;
   if (WSAStartup(2, &wsaData) != 0)
   {
      WriteLog(MSG_WSASTARTUP_FAILED, EVENTLOG_ERROR_TYPE, "e", WSAGetLastError());
      return FALSE;
   }
#endif

   // Initialize logger for subagents
   InitSubAgentsLogger(WriteSubAgentMsg);

   // Initialize cryptografy
   if (!InitCryptoLib(m_dwEnabledCiphers))
   {
      WriteLog(MSG_INIT_CRYPTO_FAILED, EVENTLOG_ERROR_TYPE, "e", WSAGetLastError());
      return FALSE;
   }

   // Initialize built-in parameters
   if (!InitParameterList())
      return FALSE;

#ifdef _WIN32
   // Dynamically import functions that may not be presented in all Windows versions
   ImportSymbols();
#endif

   // Parse server list
   if (m_pszServerList != NULL)
   {
      for(pItem = m_pszServerList; *pItem != 0; pItem = pEnd + 1)
      {
         pEnd = strchr(pItem, ',');
         if (pEnd != NULL)
            *pEnd = 0;
         StrStrip(pItem);
         g_pServerList[g_dwServerCount].dwIpAddr = ResolveHostName(pItem);
         if ((g_pServerList[g_dwServerCount].dwIpAddr == INADDR_NONE) ||
             (g_pServerList[g_dwServerCount].dwIpAddr == INADDR_ANY))
         {
            if (!(g_dwFlags & AF_DAEMON))
               printf("Invalid server address '%s'\n", pItem);
         }
         else
         {
            g_pServerList[g_dwServerCount].bInstallationServer = FALSE;
            g_pServerList[g_dwServerCount].bControlServer = FALSE;
            g_dwServerCount++;
         }
      }
      free(m_pszServerList);
   }

   // Parse installation server list
   if (m_pszInstallServerList != NULL)
   {
      DWORD i, dwAddr;

      for(pItem = m_pszInstallServerList; *pItem != 0; pItem = pEnd + 1)
      {
         pEnd = strchr(pItem, ',');
         if (pEnd != NULL)
            *pEnd = 0;
         StrStrip(pItem);

         dwAddr = ResolveHostName(pItem);
         if ((dwAddr == INADDR_NONE) ||
             (dwAddr == INADDR_ANY))
         {
            if (!(g_dwFlags & AF_DAEMON))
               printf("Invalid server address '%s'\n", pItem);
         }
         else
         {
            for(i = 0; i < g_dwServerCount; i++)
               if (g_pServerList[i].dwIpAddr == dwAddr)
                  break;

            if (i == g_dwServerCount)
            {
               g_pServerList[g_dwServerCount].dwIpAddr = dwAddr;
               g_pServerList[g_dwServerCount].bInstallationServer = TRUE;
               g_pServerList[g_dwServerCount].bControlServer = FALSE;
               g_dwServerCount++;
            }
            else
            {
               g_pServerList[i].bInstallationServer = TRUE;
            }
         }
      }
      free(m_pszInstallServerList);
   }

   // Parse control server list
   if (m_pszControlServerList != NULL)
   {
      DWORD i, dwAddr;

      for(pItem = m_pszControlServerList; *pItem != 0; pItem = pEnd + 1)
      {
         pEnd = strchr(pItem, ',');
         if (pEnd != NULL)
            *pEnd = 0;
         StrStrip(pItem);

         dwAddr = ResolveHostName(pItem);
         if ((dwAddr == INADDR_NONE) ||
             (dwAddr == INADDR_ANY))
         {
            if (!(g_dwFlags & AF_DAEMON))
               printf("Invalid server address '%s'\n", pItem);
         }
         else
         {
            for(i = 0; i < g_dwServerCount; i++)
               if (g_pServerList[i].dwIpAddr == dwAddr)
                  break;

            if (i == g_dwServerCount)
            {
               g_pServerList[g_dwServerCount].dwIpAddr = dwAddr;
               g_pServerList[g_dwServerCount].bInstallationServer = FALSE;
               g_pServerList[g_dwServerCount].bControlServer = TRUE;
               g_dwServerCount++;
            }
            else
            {
               g_pServerList[i].bControlServer = TRUE;
            }
         }
      }
      free(m_pszControlServerList);
   }

   // Add built-in actions
   AddAction("Agent.Restart", AGENT_ACTION_SUBAGENT, NULL, H_RestartAgent, "CORE", "Restart agent");

   // Load subagents
#ifdef _WIN32
   LoadWindowsSubagent();
#endif
   if (m_pszSubagentList != NULL)
   {
      for(pItem = m_pszSubagentList; *pItem != 0; pItem = pEnd + 1)
      {
         pEnd = strchr(pItem, '\n');
         if (pEnd != NULL)
            *pEnd = 0;
         StrStrip(pItem);
         LoadSubAgent(pItem);
      }
      free(m_pszSubagentList);
   }

   // Parse action list
   if (m_pszActionList != NULL)
   {
      for(pItem = m_pszActionList; *pItem != 0; pItem = pEnd + 1)
      {
         pEnd = strchr(pItem, '\n');
         if (pEnd != NULL)
            *pEnd = 0;
         StrStrip(pItem);
         if (!AddActionFromConfig(pItem))
            WriteLog(MSG_ADD_ACTION_FAILED, EVENTLOG_WARNING_TYPE, "s", pItem);
      }
      free(m_pszActionList);
   }

   // Parse external parameters list
   if (m_pszExtParamList != NULL)
   {
      for(pItem = m_pszExtParamList; *pItem != 0; pItem = pEnd + 1)
      {
         pEnd = strchr(pItem, '\n');
         if (pEnd != NULL)
            *pEnd = 0;
         StrStrip(pItem);
         if (!AddExternalParameter(pItem))
            WriteLog(MSG_ADD_EXT_PARAM_FAILED, EVENTLOG_WARNING_TYPE, "s", pItem);
      }
      free(m_pszExtParamList);
   }

   ThreadSleep(1);

   // If StartupDelay is greater than zero, then wait
   if (g_dwStartupDelay > 0)
   {
      if (g_dwFlags & AF_DAEMON)
      {
         ThreadSleep(g_dwStartupDelay);
      }
      else
      {
         DWORD i;

         printf("XXXXXX%*s]\rWAIT [", g_dwStartupDelay, " ");
         fflush(stdout);
         for(i = 0; i < g_dwStartupDelay; i++)
         {
            ThreadSleep(1);
            putc('.', stdout);
            fflush(stdout);
         }
         printf("\n");
      }
   }

   // Agent start time
   g_dwAgentStartTime = time(NULL);

   // Start network listener and session watchdog
   ThreadCreate(ListenerThread, 0, NULL);
   ThreadCreate(SessionWatchdog, 0, NULL);

   m_hCondShutdown = ConditionCreate(TRUE);
   ThreadSleep(1);

   return TRUE;
}


//
// Shutdown routine
//

void Shutdown(void)
{
   // Set shutdowm flag and sleep for some time
   // to allow other threads to finish
   g_dwFlags |= AF_SHUTDOWN;
   ThreadSleep(5);

   UnloadAllSubAgents();
   CloseLog();

   // Notify main thread about shutdown
   ConditionSet(m_hCondShutdown);
   
   // Remove PID file
#if !defined(_WIN32) && !defined(_NETWARE)
   remove(g_szPidFile);
#endif
}


//
// Common Main()
//

void Main(void)
{
   WriteLog(MSG_AGENT_STARTED, EVENTLOG_INFORMATION_TYPE, NULL);

   if (g_dwFlags & AF_DAEMON)
   {
      ConditionWait(m_hCondShutdown, INFINITE);
   }
   else
   {
#if defined(_WIN32)
      if (g_dwFlags & AF_HIDE_WINDOW)
      {
         HWND hWnd;

         hWnd = GetConsoleHWND();
         if (hWnd != NULL)
            ShowWindow(hWnd, SW_HIDE);
         ConditionWait(m_hCondShutdown, INFINITE);
         ThreadSleep(1);
      }
      else
      {
         printf("Agent running. Press ESC to shutdown.\n");
         while(1)
         {
            if (getch() == 27)
               break;
         }
         printf("Agent shutting down...\n");
         Shutdown();
      }
#elif defined(_NETWARE)
      printf("Agent running. Type UNLOAD NXAGENTD on the system console for shutdown.\n");
      ConditionWait(m_hCondShutdown, INFINITE);
#else
      printf("Agent running. Press Ctrl+C to shutdown.\n");
      ConditionWait(m_hCondShutdown, INFINITE);
      printf("\nStopping agent...\n");
#endif
   }
}


//
// Do necessary actions on agent restart
//

static void DoRestartActions(DWORD dwOldPID)
{
#ifdef _WIN32
   if (dwOldPID == 0)
   {
      // Service
      StopAgentService();
      WaitForService(SERVICE_STOPPED);
      StartAgentService();
      ExitProcess(0);
   }
   else
   {
      HANDLE hProcess;

      hProcess = OpenProcess(SYNCHRONIZE | PROCESS_TERMINATE, FALSE, dwOldPID);
      if (hProcess != NULL)
      {
         if (WaitForSingleObject(hProcess, 60000) == WAIT_TIMEOUT)
         {
            TerminateProcess(hProcess, 0);
         }
         CloseHandle(hProcess);
      }
   }
#else
   kill(dwOldPID, SIGTERM);
   sleep(10);
#endif
}


//
// Startup
//

int main(int argc, char *argv[])
{
   int ch, iExitCode = 0, iAction = ACTION_RUN_AGENT;
   BOOL bRestart = FALSE;
   DWORD dwOldPID;
#ifdef _WIN32
   char szModuleName[MAX_PATH];
#endif
   
#ifdef _NETWARE
   setscreenmode(SCR_AUTOCLOSE_ON_EXIT | SCR_COLOR_ATTRS);
#endif

   // Parse command line
   opterr = 1;
   while((ch = getopt(argc, argv, VALID_OPTIONS)) != -1)
   {
      switch(ch)
      {
         case 'h':   // Display help and exit
            printf(m_szHelpText);
            iAction = ACTION_NONE;
            break;
         case 'd':   // Run as daemon
            g_dwFlags |= AF_DAEMON;
            break;
         case 'D':   // Turn on debug output
            g_dwFlags |= AF_DEBUG;
            break;
         case 'c':   // Configuration file
            strncpy(g_szConfigFile, optarg, MAX_PATH - 1);
            break;
#if !defined(_WIN32) && !defined(_NETWARE)
         case 'p':   // PID file
            strncpy(g_szPidFile, optarg, MAX_PATH - 1);
            break;
#endif
         case 'C':   // Configuration check only
            iAction = ACTION_CHECK_CONFIG;
            break;
         case 'v':   // Print version and exit
            printf("NetXMS Core Agent Version " AGENT_VERSION_STRING "\n");
            iAction = ACTION_NONE;
            break;
         case 'X':   // Agent is being restarted
            bRestart = TRUE;
            dwOldPID = strtoul(optarg, NULL, 10);
            break;
#ifdef _WIN32
         case 'H':   // Hide window
            g_dwFlags |= AF_HIDE_WINDOW;
            break;
         case 'I':   // Install Windows service
            iAction = ACTION_INSTALL_SERVICE;
            break;
         case 'R':   // Remove Windows service
            iAction = ACTION_REMOVE_SERVICE;
            break;
         case 's':   // Start Windows service
            iAction = ACTION_START_SERVICE;
            break;
         case 'S':   // Stop Windows service
            iAction = ACTION_STOP_SERVICE;
            break;
         case 'E':   // Install Windows event source
            iAction = ACTION_INSTALL_EVENT_SOURCE;
            break;
         case 'U':   // Remove Windows event source
            iAction = ACTION_REMOVE_EVENT_SOURCE;
            break;
#endif
         case '?':
            iAction = ACTION_NONE;
            iExitCode = 1;
            break;
         default:
            break;
      }
   }

   if (bRestart)
      DoRestartActions(dwOldPID);

   // Do requested action
   switch(iAction)
   {
      case ACTION_RUN_AGENT:
         if (NxLoadConfig(g_szConfigFile, "", m_cfgTemplate, !(g_dwFlags & AF_DAEMON)) == NXCFG_ERR_OK)
         {
            if ((!stricmp(g_szLogFile, "{syslog}")) || 
                (!stricmp(g_szLogFile, "{eventlog}")))
               g_dwFlags |= AF_USE_SYSLOG;

#ifdef _WIN32
            if (g_dwFlags & AF_DAEMON)
            {
               InitService();
            }
            else
            {
               if (Initialize())
               {
                  Main();
               }
               else
               {
                  ConsolePrintf("Agent initialization failed\n");
                  CloseLog();
                  iExitCode = 3;
               }
            }
#else    /* _WIN32 */
#ifndef _NETWARE
            if (g_dwFlags & AF_DAEMON)
               if (daemon(0, 0) == -1)
               {
                  perror("Unable to setup itself as a daemon");
                  iExitCode = 4;
               }
				// Setup signal handlers
				//for(int i = 0; i < 32; i++)
				//	signal(i, SIG_IGN);
				signal(SIGPIPE, SIG_IGN);
				signal(SIGINT, OnSignal);
				signal(SIGTERM, OnSignal);
				signal(SIGSEGV, OnSignal);
				signal(SIGCHLD, OnSignal);
#endif
            if (iExitCode == 0)
            {
#ifndef _NETWARE
               m_pid = getpid();
#endif
               if (Initialize())
               {
#ifndef _NETWARE
                  FILE *fp;

                  // Write PID file
                  fp = fopen(g_szPidFile, "w");
                  if (fp != NULL)
                  {
                     fprintf(fp, "%d", m_pid);
                     fclose(fp);
                  }   
#endif
                  Main();
						Shutdown();
               }
               else
               {
                  ConsolePrintf("Agent initialization failed\n");
                  CloseLog();
                  iExitCode = 3;
               }
            }
#endif   /* _WIN32 */

            if (m_hCondShutdown != INVALID_CONDITION_HANDLE)
               ConditionDestroy(m_hCondShutdown);
         }
         else
         {
            ConsolePrintf("Error loading configuration file\n");
            iExitCode = 2;
         }
         break;
      case ACTION_CHECK_CONFIG:
         if (NxLoadConfig(g_szConfigFile, "", m_cfgTemplate, !(g_dwFlags & AF_DAEMON)) != NXCFG_ERR_OK)
         {
            ConsolePrintf("Configuration file check failed\n");
            iExitCode = 2;
         }
         break;
#ifdef _WIN32
      case ACTION_INSTALL_SERVICE:
         GetModuleFileName(GetModuleHandle(NULL), szModuleName, MAX_PATH);
         InstallService(szModuleName, g_szConfigFile);
         break;
      case ACTION_REMOVE_SERVICE:
         RemoveService();
         break;
      case ACTION_INSTALL_EVENT_SOURCE:
         GetModuleFileName(GetModuleHandle(NULL), szModuleName, MAX_PATH);
         InstallEventSource(szModuleName);
         break;
      case ACTION_REMOVE_EVENT_SOURCE:
         RemoveEventSource();
         break;
      case ACTION_START_SERVICE:
         StartAgentService();
         break;
      case ACTION_STOP_SERVICE:
         StopAgentService();
         break;
#endif
      default:
         break;
   }

#ifdef _NETWARE
   if ((iExitCode != 0) || (iAction == ACTION_NONE) || 
       (iAction == ACTION_CHECK_CONFIG))
      setscreenmode(SCR_NO_MODE);
#endif

   return iExitCode;
}
