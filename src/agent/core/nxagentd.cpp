/* 
** NetXMS multiplatform core agent
** Copyright (C) 2003, 2004 Victor Kirhenshtein
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

#ifdef _WIN32
#include <conio.h>
#endif

#ifdef _NETWARE
#include <screen.h>
#endif


//
// Externals
//

THREAD_RESULT THREAD_CALL ListenerThread(void *);


//
// Valid options for getopt()
//

#if defined(_WIN32)
#define VALID_OPTIONS   "c:CdDhIRsSv"
#elif defined(_NETWARE)
#define VALID_OPTIONS   "c:CDhv"
#else
#define VALID_OPTIONS   "c:CdDhp:v"
#endif


//
// Actions
//

#define ACTION_NONE              0
#define ACTION_RUN_AGENT         1
#define ACTION_INSTALL_SERVICE   2
#define ACTION_REMOVE_SERVICE    3
#define ACTION_START_SERVICE     4
#define ACTION_STOP_SERVICE      5
#define ACTION_CHECK_CONFIG      6


//
// Global variables
//

DWORD g_dwFlags = 0;
char g_szLogFile[MAX_PATH] = AGENT_DEFAULT_LOG;
char g_szSharedSecret[MAX_SECRET_LENGTH] = "admin";
char g_szConfigFile[MAX_PATH] = AGENT_DEFAULT_CONFIG;
WORD g_wListenPort = AGENT_LISTEN_PORT;
DWORD g_dwServerAddr[MAX_SERVERS];
DWORD g_dwServerCount = 0;
DWORD g_dwTimeOut = 5000;     // Request timeout in milliseconds
time_t g_dwAgentStartTime;

#if !defined(_WIN32) && !defined(_NETWARE)
char g_szPidFile[MAX_PATH] = "/var/run/nxagentd.pid";
#endif

#ifdef _WIN32
DWORD (__stdcall *imp_GetGuiResources)(HANDLE, DWORD);
BOOL (__stdcall *imp_GetProcessIoCounters)(HANDLE, PIO_COUNTERS);
BOOL (__stdcall *imp_GetPerformanceInfo)(PPERFORMANCE_INFORMATION, DWORD);
BOOL (__stdcall *imp_GlobalMemoryStatusEx)(LPMEMORYSTATUSEX);
DWORD (__stdcall *imp_HrLanConnectionNameFromGuidOrPath)(LPWSTR, LPWSTR, LPWSTR, LPDWORD);
#endif   /* _WIN32 */


//
// Static variables
//

static char m_szActionList[16384] = "";
static char m_szServerList[16384] = "";
static char m_szSubagentList[16384] = "";
static CONDITION m_hCondShutdown = INVALID_CONDITION_HANDLE;


//
// Configuration file template
//

static NX_CFG_TEMPLATE cfgTemplate[] =
{
   { "Action", CT_STRING_LIST, '\n', 0, 16384, 0, m_szActionList },
   { "EnableActions", CT_BOOLEAN, 0, 0, AF_ENABLE_ACTIONS, 0, &g_dwFlags },
   { "ListenPort", CT_WORD, 0, 0, 0, 0, &g_wListenPort },
   { "LogFile", CT_STRING, 0, 0, MAX_PATH, 0, g_szLogFile },
   { "LogUnresolvedSymbols", CT_BOOLEAN, 0, 0, AF_LOG_UNRESOLVED_SYMBOLS, 0, &g_dwFlags },
   { "RequireAuthentication", CT_BOOLEAN, 0, 0, AF_REQUIRE_AUTH, 0, &g_dwFlags },
   { "Servers", CT_STRING_LIST, ',', 0, 16384, 0, m_szServerList },
   { "SharedSecret", CT_STRING, 0, 0, MAX_SECRET_LENGTH, 0, g_szSharedSecret },
   { "SubAgent", CT_STRING_LIST, '\n', 0, 16384, 0, m_szSubagentList },
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
   "   -I         : Install Windows service\n"
   "   -R         : Remove Windows service\n"
   "   -s         : Start Windows servive\n"
   "   -S         : Stop Windows service\n"
#endif
   "   -v         : Display version and exit\n"
   "\n";


#ifdef _WIN32

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

   // USER32.DLL
   hModule = GetModuleHandle("USER32.DLL");
   if (hModule != NULL)
   {
      imp_GetGuiResources = (DWORD (__stdcall *)(HANDLE, DWORD))GetProcAddressAndLog(hModule,"GetGuiResources");
   }
   else
   {
      WriteLog(MSG_NO_DLL, EVENTLOG_WARNING_TYPE, "s", "USER32.DLL");
   }

   // KERNEL32.DLL
   hModule = GetModuleHandle("KERNEL32.DLL");
   if (hModule != NULL)
   {
      imp_GetProcessIoCounters = (BOOL (__stdcall *)(HANDLE, PIO_COUNTERS))GetProcAddressAndLog(hModule,"GetProcessIoCounters");
      imp_GlobalMemoryStatusEx = (BOOL (__stdcall *)(LPMEMORYSTATUSEX))GetProcAddressAndLog(hModule,"GlobalMemoryStatusEx");
   }
   else
   {
      WriteLog(MSG_NO_DLL, EVENTLOG_WARNING_TYPE, "s", "KERNEL32.DLL");
   }

   // PSAPI.DLL
   hModule = GetModuleHandle("PSAPI.DLL");
   if (hModule != NULL)
   {
      imp_GetPerformanceInfo = (BOOL (__stdcall *)(PPERFORMANCE_INFORMATION, DWORD))GetProcAddressAndLog(hModule,"GetPerformanceInfo");
   }
   else
   {
      WriteLog(MSG_NO_DLL, EVENTLOG_WARNING_TYPE, "s", "PSAPI.DLL");
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

#endif   /* _WIN32 */


//
// This function writes message from subagent to agent's log
//

static void WriteSubAgentMsg(int iLevel, TCHAR *pszMsg)
{
   WriteLog(MSG_SUBAGENT_MSG, iLevel, "s", pszMsg);
}


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

   // Initialize built-in parameters
   if (!InitParameterList())
      return FALSE;

#ifdef _WIN32
   // Dynamically import functions that may not be presented in all Windows versions
   ImportSymbols();
#endif

   // Parse server list
   for(pItem = m_szServerList; *pItem != 0; pItem = pEnd + 1)
   {
      pEnd = strchr(pItem, ',');
      if (pEnd != NULL)
         *pEnd = 0;
      StrStrip(pItem);
      g_dwServerAddr[g_dwServerCount] = inet_addr(pItem);
      if ((g_dwServerAddr[g_dwServerCount] == INADDR_NONE) ||
          (g_dwServerAddr[g_dwServerCount] == INADDR_ANY))
      {
         if (!(g_dwFlags & AF_DAEMON))
            printf("Invalid server address '%s'\n", pItem);
      }
      else
      {
         g_dwServerCount++;
      }
   }

   // Load subagents
   for(pItem = m_szSubagentList; *pItem != 0; pItem = pEnd + 1)
   {
      pEnd = strchr(pItem, '\n');
      if (pEnd != NULL)
         *pEnd = 0;
      StrStrip(pItem);
      LoadSubAgent(pItem);
   }

   // Parse action list
   for(pItem = m_szActionList; *pItem != 0; pItem = pEnd + 1)
   {
      pEnd = strchr(pItem, '\n');
      if (pEnd != NULL)
         *pEnd = 0;
      StrStrip(pItem);
      if (!AddActionFromConfig(pItem))
         WriteLog(MSG_ADD_ACTION_FAILED, EVENTLOG_WARNING_TYPE, "s", pItem);
   }

   // Agent start time
   g_dwAgentStartTime = time(NULL);

   // Start network listener
   ThreadCreate(ListenerThread, 0, NULL);

   m_hCondShutdown = ConditionCreate(FALSE);
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
      printf("Agent running. Press ESC to shutdown.\n");
      while(1)
      {
         if (getch() == 27)
            break;
      }
      printf("Agent shutting down...\n");
      Shutdown();
#elif defined(_NETWARE)
      printf("Agent running. Type UNLOAD NXAGENTD on the system console for shutdown.\n");
      ConditionWait(m_hCondShutdown, INFINITE);
#else
      printf("Agent running. Press Ctrl+C to shutdown.\n");
      ConditionWait(m_hCondShutdown, INFINITE);
#endif
   }
}


//
// Startup
//

int main(int argc, char *argv[])
{
   int ch, iExitCode = 0, iAction = ACTION_RUN_AGENT;
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
         case 'p':   // PID file
            strncpy(g_szPidFile, optarg, MAX_PATH - 1);
            break;
         case 'C':   // Configuration check only
            iAction = ACTION_CHECK_CONFIG;
            break;
         case 'v':   // Print version and exit
            printf("NetXMS Core Agent Version " AGENT_VERSION_STRING "\n");
            iAction = ACTION_NONE;
            break;
#ifdef _WIN32
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
#endif
         case '?':
            iAction = ACTION_NONE;
            iExitCode = 1;
            break;
         default:
            break;
      }
   }

   // Do requested action
   switch(iAction)
   {
      case ACTION_RUN_AGENT:
         if (NxLoadConfig(g_szConfigFile, "", cfgTemplate, !(g_dwFlags & AF_DAEMON)) == NXCFG_ERR_OK)
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
#endif
            if (iExitCode == 0)
            {
               if (Initialize())
               {
#ifndef _NETWARE
                  FILE *fp;

                  // Write PID file
                  fp = fopen(g_szPidFile, "w");
                  if (fp != NULL)
                  {
                     fprintf(fp, "%d", getpid());
                     fclose(fp);
                  }   
#endif
                  Main();
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
         if (NxLoadConfig(g_szConfigFile, "", cfgTemplate, !(g_dwFlags & AF_DAEMON)) != NXCFG_ERR_OK)
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
