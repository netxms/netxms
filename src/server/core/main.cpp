/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004 NetXMS Team
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
** $module: main.cpp
**
**/

#include "nms_core.h"

#if !defined(_WIN32) && HAVE_READLINE_READLINE_H
#include <readline/readline.h>
#include <readline/history.h>
#define USE_READLINE 1
#endif


//
// Thread functions
//

void HouseKeeper(void *pArg);
void DiscoveryThread(void *pArg);
void Syncer(void *pArg);
void NodePoller(void *pArg);
void StatusPoller(void *pArg);
void ConfigurationPoller(void *pArg);
void EventProcessor(void *pArg);
void WatchdogThread(void *pArg);
void ClientListener(void *pArg);
void LocalAdminListener(void *pArg);
void DBWriteThread(void *pArg);


//
// Global variables
//

DWORD g_dwFlags = AF_USE_EVENT_LOG;
char g_szConfigFile[MAX_PATH] = DEFAULT_CONFIG_FILE;
char g_szLogFile[MAX_PATH] = DEFAULT_LOG_FILE;
DB_HANDLE g_hCoreDB = 0;
DWORD g_dwDiscoveryPollingInterval;
DWORD g_dwStatusPollingInterval;
DWORD g_dwConfigurationPollingInterval;


//
// Static data
//

static CONDITION m_hEventShutdown;


//
// Sleep for specified number of seconds or until system shutdown arrives
// Function will return TRUE if shutdown event occurs
//

BOOL SleepAndCheckForShutdown(int iSeconds)
{
   return ConditionWait(m_hEventShutdown, iSeconds * 1000);
}


//
// Load global configuration parameters
//

static void LoadGlobalConfig()
{
   g_dwDiscoveryPollingInterval = ConfigReadInt("DiscoveryPollingInterval", 900);
   g_dwStatusPollingInterval = ConfigReadInt("StatusPollingInterval", 60);
   g_dwConfigurationPollingInterval = ConfigReadInt("ConfigurationPollingInterval", 3600);
   if (ConfigReadInt("EnableAccessControl", 1))
      g_dwFlags |= AF_ENABLE_ACCESS_CONTROL;
   if (ConfigReadInt("EnableEventsAccessControl", 1))
      g_dwFlags |= AF_ENABLE_EVENTS_ACCESS_CONTROL;
   if (ConfigReadInt("DeleteEmptySubnets", 1))
      g_dwFlags |= AF_DELETE_EMPTY_SUBNETS;
   if (ConfigReadInt("EnableSNMPTraps", 1))
      g_dwFlags |= AF_ENABLE_SNMP_TRAPD;
}


//
// Server initialization
//

BOOL Initialize(void)
{
   int i, iNumThreads;
   DWORD dwAddr;
   char szInfo[256];

   InitLog();

#ifdef _WIN32
   WSADATA wsaData;
   WSAStartup(0x0002, &wsaData);
#endif

   // Initialize SSL library
   SSL_library_init();
   SSL_load_error_strings();

   // Create queue for delayed SQL queries
   g_pLazyRequestQueue = new Queue(64, 16);

   // Initialize SNMP stuff
   SnmpInit();

   // Initialize database driver and connect to database
   if (!DBInit())
      return FALSE;

   g_hCoreDB = DBConnect();
   if (g_hCoreDB == 0)
   {
      WriteLog(MSG_DB_CONNFAIL, EVENTLOG_ERROR_TYPE, NULL);
      return FALSE;
   }

   // Initialize locks
   if (!InitLocks(&dwAddr, szInfo))
   {
      if (dwAddr == UNLOCKED)    // Some SQL problems
         WriteLog(MSG_INIT_LOCKS_FAILED, EVENTLOG_ERROR_TYPE, NULL);
      else     // Database already locked by another server instance
         WriteLog(MSG_DB_LOCKED, EVENTLOG_ERROR_TYPE, "as", dwAddr, szInfo);
      return FALSE;
   }

   // Load global configuration parameters
   LoadGlobalConfig();

   // Create synchronization stuff
   m_hEventShutdown = ConditionCreate(TRUE);

   // Setup unique identifiers table
   if (!InitIdTable())
      return FALSE;

   // Load users from database
   if (!LoadUsers())
   {
      WriteLog(MSG_ERROR_LOADING_USERS, EVENTLOG_ERROR_TYPE, NULL);
      return FALSE;
   }

   // Initialize objects infrastructure and load objects from database
   ObjectsInit();
   if (!LoadObjects())
      return FALSE;

   // Load event actions
   if (!LoadActions())
      return FALSE;

   // Initialize event handling subsystem
   if (!InitEventSubsystem())
      return FALSE;

   // Initialize data collection subsystem
   if (!InitDataCollector())
      return FALSE;

   // Initialize watchdog
   WatchdogInit();

   // Start threads
   ThreadCreate(WatchdogThread, 0, NULL);
   ThreadCreate(HouseKeeper, 0, NULL);
   ThreadCreate(Syncer, 0, NULL);
   ThreadCreate(NodePoller, 0, NULL);
   ThreadCreate(StatusPoller, 0, NULL);
   ThreadCreate(ConfigurationPoller, 0, NULL);
   ThreadCreate(ClientListener, 0, NULL);

   // Start network discovery thread if required
   if (ConfigReadInt("RunNetworkDiscovery", 1))
      ThreadCreate(DiscoveryThread, 0, NULL);
   else
      CheckForMgmtNode();

   // Start event processors
   iNumThreads = ConfigReadInt("NumberOfEventProcessors", 1);
   for(i = 0; i < iNumThreads; i++)
      ThreadCreate(EventProcessor, 0, NULL);

   // Start database "lazy" write thread
   ThreadCreate(DBWriteThread, 0, NULL);

   // Start local administartive interface listener if required
   if (ConfigReadInt("EnableAdminInterface", 1))
      ThreadCreate(LocalAdminListener, 0, NULL);

   return TRUE;
}


//
// Handler for client notification
//

static void NotifyClient(ClientSession *pSession, void *pArg)
{
   pSession->Notify(NX_NOTIFY_SHUTDOWN);
}


//
// Server shutdown
//

void Shutdown(void)
{
   // Notify clients
   EnumerateClientSessions(NotifyClient, NULL);

   WriteLog(MSG_SERVER_STOPPED, EVENTLOG_INFORMATION_TYPE, NULL);
   g_dwFlags |= AF_SHUTDOWN;     // Set shutdown flag
   ConditionSet(m_hEventShutdown);

   g_pEventQueue->Put(INVALID_POINTER_VALUE);   // Stop event processor
   ThreadSleep(5);     // Give other threads a chance to terminate in a safe way
   SaveObjects();
   StopDBWriter();

   // Remove database lock
   DBQuery(g_hCoreDB, "UPDATE locks SET lock_status=-1,owner_info='' WHERE component_id=0");

   // Disconnect from database and unload driver
   if (g_hCoreDB != 0)
      DBDisconnect(g_hCoreDB);
   DBUnloadDriver();

   DestroyActionList();

   CloseLog();
}


//
// Compare given string to command template with abbreviation possibility
//

static BOOL IsCommand(char *pszTemplate, char *pszString, int iMinChars)
{
   int i;

   // Convert given string to uppercase
   strupr(pszString);

   for(i = 0; pszString[i] != 0; i++)
      if (pszString[i] != pszTemplate[i])
         return FALSE;
   if (i < iMinChars)
      return FALSE;
   return TRUE;
}


//
// Process command entered from command line in standalone mode
// Return TRUE if command was "down"
//

static BOOL ProcessCommand(char *pszCmdLine)
{
   char *pArg;
   char szBuffer[256];
   BOOL bExitCode = FALSE;

   // Get command
   pArg = ExtractWord(pszCmdLine, szBuffer);

   if (IsCommand("DOWN", szBuffer, 4))
   {
      bExitCode = TRUE;
   }
   else if (IsCommand("DUMP", szBuffer, 2))
   {
      // Get argument
      pArg = ExtractWord(pArg, szBuffer);

      if (IsCommand("OBJECTS", szBuffer, 1))
      {
      }
      else if (IsCommand("SESSIONS", szBuffer, 1))
      {
         DumpSessions();
      }
      else if (IsCommand("USERS", szBuffer, 1))
      {
         DumpUsers();
      }
      else
      {
         printf("ERROR: Invalid command argument\n\n");
      }
   }
   else if (IsCommand("HELP", szBuffer, 2))
   {
      printf("Valid commands are:\n"
             "   down          - Down NetXMS server\n"
             "   dump objects  - Dump network objects to screen\n"
             "   dump sessions - Dump active client sessions to screen\n"
             "   dump users    - Dump users to screen\n"
             "   mutex         - Display mutex status\n"
             "   wd            - Display watchdog information\n"
             "\nAlmost all commands can be abbreviated to 2 or 3 characters\n"
             "\n");
   }
   else if (IsCommand("MUTEX", szBuffer, 2))
   {
      printf("Mutex status:\n");
      DbgTestMutex(g_hMutexIdIndex, "g_hMutexIdIndex");
      DbgTestMutex(g_hMutexNodeIndex, "g_hMutexNodeIndex");
      DbgTestMutex(g_hMutexSubnetIndex, "g_hMutexSubnetIndex");
      DbgTestMutex(g_hMutexInterfaceIndex, "g_hMutexInterfaceIndex");
      DbgTestMutex(g_hMutexObjectAccess, "g_hMutexObjectAccess");
      printf("\n");
   }
   else if (IsCommand("WD", szBuffer, 2))
   {
      WatchdogPrintStatus();
      printf("\n");
   }
   else
   {
      printf("INVALID COMMAND\n\n");
   }
   
   return bExitCode;
}


//
// Common main()
//

void Main(void)
{
   WriteLog(MSG_SERVER_STARTED, EVENTLOG_INFORMATION_TYPE, NULL);

   if (IsStandalone())
   {
      char *ptr, szCommand[256];

      printf("\nNetXMS Server V" NETXMS_VERSION_STRING " Ready\n"
             "Enter \"help\" for command list or \"down\" for server shutdown\n"
             "System Console\n\n");

#if USE_READLINE
      // Initialize readline library if we use it
      rl_bind_key('\t', rl_insert);
#endif
      
      while(1)
      {
#if USE_READLINE
         ptr = readline("netxmsd: ");
#else
         printf("netxmsd: ");
         fflush(stdout);
         fgets(szCommand, 255, stdin);
         ptr = strchr(szCommand, '\n');
         if (ptr != NULL)
            *ptr = 0;
         ptr = szCommand;
#endif

         if (ptr != NULL)
         {
            StrStrip(ptr);
            if (*ptr != 0)
            {
               if (ProcessCommand(ptr))
                  break;
#if USE_READLINE
               add_history(ptr);
#endif
            }
#if USE_READLINE
            free(ptr);
#endif
         }
         else
         {
            printf("\n");
         }
      }

#if USE_READLINE
      free(ptr);
#endif
      Shutdown();
   }
   else
   {
		ConditionWait(m_hEventShutdown, INFINITE);
   }
}


//
// Startup code
//

int main(int argc, char *argv[])
{
   if (!ParseCommandLine(argc, argv))
      return 1;

   if (!LoadConfig())
      return 1;

#ifdef _WIN32
   if (!IsStandalone())
   {
      InitService();
   }
   else
   {
      if (!Initialize())
      {
         printf("NMS Core initialization failed\n");
         return 3;
      }
      Main();
   }
#else    /* _WIN32 */
   if (!IsStandalone())
   {
      if (daemon(0, 0) == -1)
      {
         perror("Call to daemon() failed");
         return 2;
      }
   }
   if (!Initialize())
      return 3;
   Main();
#endif   /* _WIN32 */
   return 0;
}
