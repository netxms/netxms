/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004, 2005 NetXMS Team
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

#include "nxcore.h"
#include <netxmsdb.h>

#if !defined(_WIN32) && HAVE_READLINE_READLINE_H
#include <readline/readline.h>
#include <readline/history.h>
#define USE_READLINE 1
#endif

#ifdef _WIN32
# include <direct.h>
# include <errno.h> 
#else
# include <signal.h>
# include <sys/wait.h>
#endif


//
// Thread functions
//

THREAD_RESULT THREAD_CALL HouseKeeper(void *pArg);
THREAD_RESULT THREAD_CALL Syncer(void *pArg);
THREAD_RESULT THREAD_CALL NodePoller(void *pArg);
THREAD_RESULT THREAD_CALL NodePollManager(void *pArg);
THREAD_RESULT THREAD_CALL EventProcessor(void *pArg);
THREAD_RESULT THREAD_CALL WatchdogThread(void *pArg);
THREAD_RESULT THREAD_CALL ClientListener(void *pArg);
THREAD_RESULT THREAD_CALL LocalAdminListener(void *pArg);
THREAD_RESULT THREAD_CALL SNMPTrapReceiver(void *pArg);
THREAD_RESULT THREAD_CALL SyslogDaemon(void *pArg);


//
// Global variables
//

DWORD NXCORE_EXPORTABLE g_dwFlags = AF_USE_EVENT_LOG;
char NXCORE_EXPORTABLE g_szConfigFile[MAX_PATH] = DEFAULT_CONFIG_FILE;
char NXCORE_EXPORTABLE g_szLogFile[MAX_PATH] = DEFAULT_LOG_FILE;
#ifndef _WIN32
char NXCORE_EXPORTABLE g_szPIDFile[MAX_PATH] = "/var/run/netxmsd.pid";
#endif
DB_HANDLE g_hCoreDB = 0;
DWORD g_dwDiscoveryPollingInterval;
DWORD g_dwStatusPollingInterval;
DWORD g_dwConfigurationPollingInterval;
DWORD g_dwRoutingTableUpdateInterval;
DWORD g_dwPingSize;
char g_szDataDir[MAX_PATH];
DWORD g_dwDBSyntax = DB_SYNTAX_GENERIC;
QWORD g_qwServerId;
RSA *g_pServerKey = NULL;
time_t g_tServerStartTime = 0;

//
// Static data
//

static CONDITION m_condShutdown = INVALID_CONDITION_HANDLE;
static THREAD m_thNodePollMgr = INVALID_THREAD_HANDLE;
static THREAD m_thHouseKeeper = INVALID_THREAD_HANDLE;
static THREAD m_thSyncer = INVALID_THREAD_HANDLE;
static THREAD m_thSyslogDaemon = INVALID_THREAD_HANDLE;

#ifndef _WIN32
static pid_t m_pid = -1;     // Server process ID
#endif


//
// Sleep for specified number of seconds or until system shutdown arrives
// Function will return TRUE if shutdown event occurs
//

BOOL NXCORE_EXPORTABLE SleepAndCheckForShutdown(int iSeconds)
{
   return ConditionWait(m_condShutdown, iSeconds * 1000);
}


//
// Disconnect from database (exportable function for startup module)
//

void NXCORE_EXPORTABLE ShutdownDB(void)
{
   if (g_hCoreDB != NULL)
      DBDisconnect(g_hCoreDB);
   DBUnloadDriver();
}


//
// Check data directory for existence
//

static BOOL CheckDataDir(void)
{
   char szBuffer[MAX_PATH];

   if (chdir(g_szDataDir) == -1)
   {
      WriteLog(MSG_INVALID_DATA_DIR, EVENTLOG_ERROR_TYPE, "s", g_szDataDir);
      return FALSE;
   }

#ifdef _WIN32
# define MKDIR(name) mkdir(name)
#else
# define MKDIR(name) mkdir(name, 0700)
#endif

   // Create directory for mib files if it doesn't exist
   strcpy(szBuffer, g_szDataDir);
   strcat(szBuffer, DDIR_MIBS);
   if (MKDIR(szBuffer) == -1)
      if (errno != EEXIST)
      {
         WriteLog(MSG_ERROR_CREATING_DATA_DIR, EVENTLOG_ERROR_TYPE, "s", szBuffer);
         return FALSE;
      }

   // Create directory for image files if it doesn't exist
   strcpy(szBuffer, g_szDataDir);
   strcat(szBuffer, DDIR_IMAGES);
   if (MKDIR(szBuffer) == -1)
      if (errno != EEXIST)
      {
         WriteLog(MSG_ERROR_CREATING_DATA_DIR, EVENTLOG_ERROR_TYPE, "s", szBuffer);
         return FALSE;
      }

   // Create directory for package files if it doesn't exist
   strcpy(szBuffer, g_szDataDir);
   strcat(szBuffer, DDIR_PACKAGES);
   if (MKDIR(szBuffer) == -1)
      if (errno != EEXIST)
      {
         WriteLog(MSG_ERROR_CREATING_DATA_DIR, EVENTLOG_ERROR_TYPE, "s", szBuffer);
         return FALSE;
      }

#undef MKDIR

   return TRUE;
}


//
// Load global configuration parameters
//

static void LoadGlobalConfig()
{
   g_dwDiscoveryPollingInterval = ConfigReadInt("DiscoveryPollingInterval", 900);
   g_dwStatusPollingInterval = ConfigReadInt("StatusPollingInterval", 60);
   g_dwConfigurationPollingInterval = ConfigReadInt("ConfigurationPollingInterval", 3600);
   g_dwRoutingTableUpdateInterval = ConfigReadInt("RoutingTableUpdateInterval", 300);
   if (ConfigReadInt("EnableAccessControl", 1))
      g_dwFlags |= AF_ENABLE_ACCESS_CONTROL;
   if (ConfigReadInt("EnableEventsAccessControl", 1))
      g_dwFlags |= AF_ENABLE_EVENTS_ACCESS_CONTROL;
   if (ConfigReadInt("DeleteEmptySubnets", 1))
      g_dwFlags |= AF_DELETE_EMPTY_SUBNETS;
   if (ConfigReadInt("EnableSNMPTraps", 1))
      g_dwFlags |= AF_ENABLE_SNMP_TRAPD;
   if (ConfigReadInt("EnableZoning", 0))
      g_dwFlags |= AF_ENABLE_ZONING;
   if (ConfigReadInt("EnableMultipleDBConnections", 1))
      g_dwFlags |= AF_ENABLE_MULTIPLE_DB_CONN;
   ConfigReadStr("DataDirectory", g_szDataDir, MAX_PATH, DEFAULT_DATA_DIR);
   g_dwPingSize = ConfigReadInt("IcmpPingSize", 46);
}


//
// Initialize cryptografic functions
//

static BOOL InitCryptografy(void)
{
#ifdef _WITH_ENCRYPTION
   char szKeyFile[MAX_PATH];
   BOOL bResult = FALSE;
   int fd, iPolicy;
   DWORD dwLen;
   BYTE *pBufPos, *pKeyBuffer, hash[SHA1_DIGEST_SIZE];

   if (!InitCryptoLib(ConfigReadULong("AllowedCiphers", 15)))
      return FALSE;

   strcpy(szKeyFile, g_szDataDir);
   strcat(szKeyFile, DFILE_KEYS);
   fd = open(szKeyFile, O_RDONLY | O_BINARY);
   g_pServerKey = LoadRSAKeys(szKeyFile);
   if (g_pServerKey == NULL)
   {
      DbgPrintf(AF_DEBUG_MISC, "Generating RSA key pair...");
      g_pServerKey = RSA_generate_key(NETXMS_RSA_KEYLEN, 17, NULL, 0);
      if (g_pServerKey != NULL)
      {
         fd = open(szKeyFile, O_WRONLY | O_BINARY | O_CREAT | O_TRUNC, 0600);
         if (fd != -1)
         {
            dwLen = i2d_RSAPublicKey(g_pServerKey, NULL);
            dwLen += i2d_RSAPrivateKey(g_pServerKey, NULL);
            pKeyBuffer = (BYTE *)malloc(dwLen);

            pBufPos = pKeyBuffer;
            i2d_RSAPublicKey(g_pServerKey, &pBufPos);
            i2d_RSAPrivateKey(g_pServerKey, &pBufPos);
            write(fd, &dwLen, sizeof(DWORD));
            write(fd, pKeyBuffer, dwLen);

            CalculateSHA1Hash(pKeyBuffer, dwLen, hash);
            write(fd, hash, SHA1_DIGEST_SIZE);
            
            close(fd);
            free(pKeyBuffer);
            bResult = TRUE;
         }
      }
   }
   else
   {
      bResult = TRUE;
   }

   iPolicy = ConfigReadInt("DefaultEncryptionPolicy", 1);
   if ((iPolicy < 0) || (iPolicy > 3))
      iPolicy = 1;
   SetAgentDEP(iPolicy);

   return bResult;
#else
   return TRUE;
#endif
}


//
// Server initialization
//

BOOL NXCORE_EXPORTABLE Initialize(void)
{
   int i, iNumThreads, iDBVersion;
   DWORD dwAddr;
   char szInfo[256];

   g_tServerStartTime = time(NULL);
   srand(g_tServerStartTime);
   InitLog(g_dwFlags & AF_USE_EVENT_LOG, g_szLogFile, g_dwFlags & AF_STANDALONE);

#ifdef _WIN32
   WSADATA wsaData;
   WSAStartup(0x0002, &wsaData);
#endif

   InitLocalNetInfo();

   // Create queue for delayed SQL queries
   g_pLazyRequestQueue = new Queue(64, 16);

   // Initialize database driver and connect to database
   if (!DBInit(TRUE, g_dwFlags & AF_LOG_SQL_ERRORS, g_dwFlags & AF_DEBUG_SQL))
      return FALSE;

   g_hCoreDB = DBConnect();
   if (g_hCoreDB == NULL)
   {
      WriteLog(MSG_DB_CONNFAIL, EVENTLOG_ERROR_TYPE, NULL);
      return FALSE;
   }
   DbgPrintf(AF_DEBUG_MISC, "Successfully connected to database %s@%s", g_szDbName, g_szDbServer);

   // Check database version
   iDBVersion = ConfigReadInt("DBFormatVersion", 0);
   if (iDBVersion != DB_FORMAT_VERSION)
   {
      WriteLog(MSG_WRONG_DB_VERSION, EVENTLOG_ERROR_TYPE, "dd", iDBVersion, DB_FORMAT_VERSION);
      return FALSE;
   }

   // Read database syntax
   ConfigReadStr("DBSyntax", szInfo, 256, "");
   if (!stricmp(szInfo, "MYSQL"))
   {
      g_dwDBSyntax = DB_SYNTAX_MYSQL;
   }
   else if (!stricmp(szInfo, "PGSQL"))
   {
      g_dwDBSyntax = DB_SYNTAX_PGSQL;
   }
   else if (!stricmp(szInfo, "MSSQL"))
   {
      g_dwDBSyntax = DB_SYNTAX_MSSQL;
   }
   else if (!stricmp(szInfo, "ORACLE"))
   {
      g_dwDBSyntax = DB_SYNTAX_ORACLE;
   }
   else if (!stricmp(szInfo, "SQLITE"))
   {
      g_dwDBSyntax = DB_SYNTAX_SQLITE;
   }
   else
   {
      g_dwDBSyntax = DB_SYNTAX_GENERIC;
   }

   // Read server ID
   ConfigReadStr("ServerID", szInfo, 256, "");
   StrStrip(szInfo);
   if (szInfo[0] != 0)
   {
      StrToBin(szInfo, (BYTE *)&g_qwServerId, sizeof(QWORD));
   }
   else
   {
      // Generate new ID
      g_qwServerId = (((QWORD)time(NULL)) << 32) | rand();
      BinToStr((BYTE *)&g_qwServerId, sizeof(QWORD), szInfo);
      ConfigWriteStr("ServerID", szInfo, TRUE);
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
   g_dwFlags |= AF_DB_LOCKED;

   // Load global configuration parameters
   LoadGlobalConfig();
   DbgPrintf(AF_DEBUG_MISC, "Global configuration loaded");

   // Check data directory
   if (!CheckDataDir())
      return FALSE;

   // Initialize cryptografy
   if (!InitCryptografy())
   {
      WriteLog(MSG_INIT_CRYPTO_FAILED, EVENTLOG_ERROR_TYPE, NULL);
      return FALSE;
   }

   // Initialize SNMP stuff
   SnmpInit();

   // Update hashes for image files
   UpdateImageHashes();

   // Create synchronization stuff
   m_condShutdown = ConditionCreate(TRUE);

   // Setup unique identifiers table
   if (!InitIdTable())
      return FALSE;
   DbgPrintf(AF_DEBUG_MISC, "ID table created");

   // Initialize mailer and SMS sender
   InitMailer();
   InitSMSSender();

   // Load users from database
   if (!LoadUsers())
   {
      WriteLog(MSG_ERROR_LOADING_USERS, EVENTLOG_ERROR_TYPE, NULL);
      return FALSE;
   }
   DbgPrintf(AF_DEBUG_MISC, "User accounts loaded");

   // Initialize objects infrastructure and load objects from database
   ObjectsInit();
   if (!LoadObjects())
      return FALSE;
   DbgPrintf(AF_DEBUG_MISC, "Objects loaded and initialized");

   // Initialize and load event actions
   if (!InitActions())
   {
      WriteLog(MSG_ACTION_INIT_ERROR, EVENTLOG_ERROR_TYPE, NULL);
      return FALSE;
   }

   // Initialize event handling subsystem
   if (!InitEventSubsystem())
      return FALSE;

   // Initialize alarms
   if (!g_alarmMgr.Init())
      return FALSE;

   // Initialize data collection subsystem
   if (!InitDataCollector())
      return FALSE;

   // Initialize watchdog
   WatchdogInit();

   // Check if management node object presented in database
   CheckForMgmtNode();

   // Start threads
   ThreadCreate(WatchdogThread, 0, NULL);
   ThreadCreate(NodePoller, 0, NULL);
   ThreadCreate(ClientListener, 0, NULL);
   m_thSyncer = ThreadCreateEx(Syncer, 0, NULL);
   m_thHouseKeeper = ThreadCreateEx(HouseKeeper, 0, NULL);
   m_thNodePollMgr = ThreadCreateEx(NodePollManager, 0, NULL);

   // Start event processors
   iNumThreads = ConfigReadInt("NumberOfEventProcessors", 1);
   for(i = 0; i < iNumThreads; i++)
      ThreadCreate(EventProcessor, 0, (void *)(i + 1));

   // Start SNMP trapper
   InitTraps();
   if (ConfigReadInt("EnableSNMPTraps", 1))
      ThreadCreate(SNMPTrapReceiver, 0, NULL);

   // Start built-in syslog daemon
   if (ConfigReadInt("EnableSyslogDaemon", 0))
      m_thSyslogDaemon = ThreadCreateEx(SyslogDaemon, 0, NULL);

   // Start database "lazy" write thread
   StartDBWriter();

   // Start local administartive interface listener if required
   if (ConfigReadInt("EnableAdminInterface", 1))
      ThreadCreate(LocalAdminListener, 0, NULL);

   // Load modules
   LoadNetXMSModules();

   DbgPrintf(AF_DEBUG_MISC, "Server initialization completed");
   return TRUE;
}


//
// Server shutdown
//

void NXCORE_EXPORTABLE Shutdown(void)
{
   DWORD i, dwNumThreads;

   // Notify clients
   NotifyClientSessions(NX_NOTIFY_SHUTDOWN, 0);

   WriteLog(MSG_SERVER_STOPPED, EVENTLOG_INFORMATION_TYPE, NULL);
   g_dwFlags |= AF_SHUTDOWN;     // Set shutdown flag
   ConditionSet(m_condShutdown);

#ifndef _WIN32
   if (IsStandalone())
      kill(m_pid, SIGUSR1);   // Terminate signal handler
#endif

   // Stop event processor(s)
   g_pEventQueue->Clear();
   dwNumThreads = ConfigReadInt("NumberOfEventProcessors", 1);
   for(i = 0; i < dwNumThreads; i++)
      g_pEventQueue->Put(INVALID_POINTER_VALUE);

   ShutdownMailer();
   ShutdownSMSSender();

   ThreadSleep(2);     // Give other threads a chance to terminate in a safe way
   DbgPrintf(AF_DEBUG_MISC, "All threads was notified, continue with shutdown");

   // Wait for critical threads
   ThreadJoin(m_thHouseKeeper);
   ThreadJoin(m_thNodePollMgr);
   ThreadJoin(m_thSyncer);
   ThreadJoin(m_thSyslogDaemon);

   SaveObjects(g_hCoreDB);
   DbgPrintf(AF_DEBUG_MISC, "All objects saved to database");
   SaveUsers(g_hCoreDB);
   DbgPrintf(AF_DEBUG_MISC, "All users saved to database");
   StopDBWriter();
   DbgPrintf(AF_DEBUG_MISC, "Database writer stopped");

   // Remove database lock
   UnlockDB();

   // Disconnect from database and unload driver
   if (g_hCoreDB != NULL)
      DBDisconnect(g_hCoreDB);
   DBUnloadDriver();
   DbgPrintf(AF_DEBUG_MISC, "Database driver unloaded");

   CleanupActions();
   ShutdownEventSubsystem();
   DbgPrintf(AF_DEBUG_MISC, "Event processing stopped");

   // Delete all objects
   for(i = 0; i < g_dwIdIndexSize; i++)
      delete g_pIndexById[i].pObject;

   CloseLog();

   // Remove PID file
#ifndef _WIN32
   remove(g_szPIDFile);
#endif

   // Terminate process
#ifdef _WIN32
   if (g_dwFlags & AF_STANDALONE)
      ExitProcess(0);
#else
   exit(0);
#endif
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

BOOL ProcessConsoleCommand(char *pszCmdLine, CONSOLE_CTX pCtx)
{
   char *pArg;
   char szBuffer[256];
   BOOL bExitCode = FALSE;

   // Get command
   pArg = ExtractWord(pszCmdLine, szBuffer);

   if (IsCommand("DEBUG", szBuffer, 2))
   {
      // Get argument
      pArg = ExtractWord(pArg, szBuffer);

      if (IsCommand("ON", szBuffer, 2))
      {
         g_dwFlags |= AF_DEBUG_ALL;
         ConsolePrintf(pCtx, "Debug mode turned on\n");
      }
      else if (IsCommand("OFF", szBuffer, 2))
      {
         g_dwFlags &= ~AF_DEBUG_ALL;
         ConsolePrintf(pCtx, "Debug mode turned off\n");
      }
      else
      {
         if (szBuffer[0] == 0)
            ConsolePrintf(pCtx, "ERROR: Missing argument\n\n");
         else
            ConsolePrintf(pCtx, "ERROR: Invalid DEBUG argument\n\n");
      }
   }
   else if (IsCommand("DOWN", szBuffer, 4))
   {
      ConsolePrintf(pCtx, "Proceeding with server shutdown...\n");
      bExitCode = TRUE;
   }
   else if (IsCommand("DUMP", szBuffer, 4))
   {
      ConsolePrintf(pCtx, "Dumping process to disk...\n");
      DumpProcess();
      ConsolePrintf(pCtx, "Done.\n");
   }
   else if (IsCommand("SHOW", szBuffer, 2))
   {
      // Get argument
      pArg = ExtractWord(pArg, szBuffer);

      if (IsCommand("FLAGS", szBuffer, 1))
      {
         ConsolePrintf(pCtx, "Flags: 0x%08X\n", g_dwFlags);
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_STANDALONE));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_USE_EVENT_LOG));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_ENABLE_ACCESS_CONTROL));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_ENABLE_EVENTS_ACCESS_CONTROL));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_LOG_SQL_ERRORS));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_DELETE_EMPTY_SUBNETS));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_ENABLE_SNMP_TRAPD));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_ENABLE_ZONING));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_DEBUG_EVENTS));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_DEBUG_CSCP));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_DEBUG_DISCOVERY));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_DEBUG_DC));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_DEBUG_HOUSEKEEPER));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_DEBUG_LOCKS));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_DEBUG_ACTIONS));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_DEBUG_MISC));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_DEBUG_SQL));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_DEBUG_SNMP));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_DEBUG_OBJECTS));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_DB_LOCKED));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_ENABLE_MULTIPLE_DB_CONN));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_SHUTDOWN));
         ConsolePrintf(pCtx, "\n");
      }
      else if (IsCommand("MUTEX", szBuffer, 1))
      {
         ConsolePrintf(pCtx, "Mutex status:\n");
         DbgTestRWLock(g_rwlockIdIndex, "g_hMutexIdIndex", pCtx);
         DbgTestRWLock(g_rwlockNodeIndex, "g_hMutexNodeIndex", pCtx);
         DbgTestRWLock(g_rwlockSubnetIndex, "g_hMutexSubnetIndex", pCtx);
         DbgTestRWLock(g_rwlockInterfaceIndex, "g_hMutexInterfaceIndex", pCtx);
         ConsolePrintf(pCtx, "\n");
      }
      else if (IsCommand("OBJECTS", szBuffer, 1))
      {
         DumpObjects(pCtx);
      }
      else if (IsCommand("POLLERS", szBuffer, 1))
      {
         ShowPollerState(pCtx);
      }
      else if (IsCommand("ROUTING-TABLE", szBuffer, 1))
      {
         DWORD dwNode;
         NetObj *pObject;

         pArg = ExtractWord(pArg, szBuffer);
         dwNode = strtoul(szBuffer, NULL, 0);
         if (dwNode != 0)
         {
            pObject = FindObjectById(dwNode);
            if (pObject != NULL)
            {
               if (pObject->Type() == OBJECT_NODE)
               {
                  ROUTING_TABLE *pRT;
                  char szIpAddr[16];
                  int i;

                  ConsolePrintf(pCtx, "Routing table for node %s:\n\n", pObject->Name());
                  pRT = ((Node *)pObject)->GetCachedRoutingTable();
                  if (pRT != NULL)
                  {
                     for(i = 0; i < pRT->iNumEntries; i++)
                     {
                        sprintf(szBuffer, "%s/%d", IpToStr(pRT->pRoutes[i].dwDestAddr, szIpAddr),
                                BitsInMask(pRT->pRoutes[i].dwDestMask));
                        ConsolePrintf(pCtx, "%-18s %-15s %-6d %d\n", szBuffer,
                                      IpToStr(pRT->pRoutes[i].dwNextHop, szIpAddr),
                                      pRT->pRoutes[i].dwIfIndex, pRT->pRoutes[i].dwRouteType);
                     }
                     ConsolePrintf(pCtx, "\n");
                  }
                  else
                  {
                     ConsolePrintf(pCtx, "Node doesn't have cached routing table\n\n");
                  }
               }
               else
               {
                  ConsolePrintf(pCtx, "ERROR: Object is not a node\n\n");
               }
            }
            else
            {
               ConsolePrintf(pCtx, "ERROR: Object with ID %d does not exist\n\n", dwNode);
            }
         }
         else
         {
            ConsolePrintf(pCtx, "ERROR: Invalid or missing node ID\n\n");
         }
      }
      else if (IsCommand("SESSIONS", szBuffer, 2))
      {
         DumpSessions(pCtx);
      }
      else if (IsCommand("STATS", szBuffer, 2))
      {
         ShowServerStats(pCtx);
      }
      else if (IsCommand("USERS", szBuffer, 1))
      {
         DumpUsers(pCtx);
      }
      else if (IsCommand("WATCHDOG", szBuffer, 1))
      {
         WatchdogPrintStatus(pCtx);
         ConsolePrintf(pCtx, "\n");
      }
      else
      {
         if (szBuffer[0] == 0)
            ConsolePrintf(pCtx, "ERROR: Missing subcommand\n\n");
         else
            ConsolePrintf(pCtx, "ERROR: Invalid SHOW subcommand\n\n");
      }
   }
   else if (IsCommand("TRACE", szBuffer, 1))
   {
      DWORD dwNode1, dwNode2;
      NetObj *pObject1, *pObject2;
      NETWORK_PATH_TRACE *pTrace;
      char szNextHop[16];
      int i;

      // Get arguments
      pArg = ExtractWord(pArg, szBuffer);
      dwNode1 = strtoul(szBuffer, NULL, 0);

      pArg = ExtractWord(pArg, szBuffer);
      dwNode2 = strtoul(szBuffer, NULL, 0);

      if ((dwNode1 != 0) && (dwNode2 != 0))
      {
         pObject1 = FindObjectById(dwNode1);
         if (pObject1 == NULL)
         {
            ConsolePrintf(pCtx, "ERROR: Object with ID %d does not exist\n\n", dwNode1);
         }
         else
         {
            pObject2 = FindObjectById(dwNode2);
            if (pObject2 == NULL)
            {
               ConsolePrintf(pCtx, "ERROR: Object with ID %d does not exist\n\n", dwNode2);
            }
            else
            {
               if ((pObject1->Type() == OBJECT_NODE) &&
                   (pObject2->Type() == OBJECT_NODE))
               {
                  pTrace = TraceRoute((Node *)pObject1, (Node *)pObject2);
                  if (pTrace != NULL)
                  {
                     ConsolePrintf(pCtx, "Trace from %s to %s (%d hops):\n",
                                   pObject1->Name(), pObject2->Name(), pTrace->iNumHops);
                     for(i = 0; i < pTrace->iNumHops; i++)
                        ConsolePrintf(pCtx, "[%d] %s %s %s %d\n",
                                      pTrace->pHopList[i].pObject->Id(),
                                      pTrace->pHopList[i].pObject->Name(),
                                      IpToStr(pTrace->pHopList[i].dwNextHop, szNextHop),
                                      pTrace->pHopList[i].bIsVPN ? "VPN Connector ID:" : "Interface Index: ",
                                      pTrace->pHopList[i].dwIfIndex);
                     DestroyTraceData(pTrace);
                     ConsolePrintf(pCtx, "\n");
                  }
                  else
                  {
                     ConsolePrintf(pCtx, "ERROR: Call to TraceRoute() failed\n\n");
                  }
               }
               else
               {
                  ConsolePrintf(pCtx, "ERROR: Object is not a node\n\n");
               }
            }
         }
      }
      else
      {
         ConsolePrintf(pCtx, "ERROR: Invalid or missing node id(s)\n\n");
      }
   }
   else if (IsCommand("HELP", szBuffer, 2) || IsCommand("?", szBuffer, 1))
   {
      ConsolePrintf(pCtx, "Valid commands are:\n"
                          "   debug [on|off]            - Turn debug mode on or off\n"
                          "   down                      - Down NetXMS server\n"
                          "   exit                      - Exit from remote session\n"
                          "   help                      - Display this help\n"
                          "   show flags                - Show internal server flags\n"
                          "   show mutex                - Display mutex status\n"
                          "   show objects              - Dump network objects to screen\n"
                          "   show pollers              - Show poller threads state information\n"
                          "   show routing-table <node> - Show cached routing table for node\n"
                          "   show sessions             - Show active client sessions\n"
                          "   show stats                - Show server statistics\n"
                          "   show users                - Show users\n"
                          "   show watchdog             - Display watchdog information\n"
                          "   trace <node1> <node2>     - Show network path trace between two nodes\n"
                          "\nAlmost all commands can be abbreviated to 2 or 3 characters\n"
                          "\n");
   }
   else
   {
      ConsolePrintf(pCtx, "UNKNOWN COMMAND\n\n");
   }
   
   return bExitCode;
}


//
// Signal handler for UNIX platforms
//

#ifndef _WIN32

THREAD_RESULT THREAD_CALL NXCORE_EXPORTABLE SignalHandler(void *pArg)
{
   sigset_t signals;
   int nSignal;
   BOOL bCallShutdown = FALSE;

   m_pid = getpid();

   sigemptyset(&signals);
   sigaddset(&signals, SIGTERM);
   sigaddset(&signals, SIGINT);
   sigaddset(&signals, SIGPIPE);
   sigaddset(&signals, SIGSEGV);
   sigaddset(&signals, SIGCHLD);
   sigaddset(&signals, SIGHUP);
   sigaddset(&signals, SIGUSR1);
   sigaddset(&signals, SIGUSR2);

   sigprocmask(SIG_BLOCK, &signals, NULL);

   while(1)
   {
      if (sigwait(&signals, &nSignal) == 0)
      {
         switch(nSignal)
         {
            case SIGTERM:
            case SIGINT:
               if (IsStandalone())
                  bCallShutdown = TRUE;
               ConditionSet(m_condShutdown);
               goto stop_handler;
            case SIGSEGV:
               abort();
               break;
            case SIGCHLD:
               while (waitpid(-1, NULL, WNOHANG) > 0)
                  ;
               break;
            case SIGUSR1:
               if (g_dwFlags & AF_SHUTDOWN)
                  goto stop_handler;
               break;
            default:
               break;
         }
      }
      else
      {
         ThreadSleepMs(100);
      }
   }

stop_handler:
   sigprocmask(SIG_UNBLOCK, &signals, NULL);
   if (bCallShutdown)
      Shutdown();
   return THREAD_OK;
}

#endif


//
// Common main()
//

THREAD_RESULT THREAD_CALL NXCORE_EXPORTABLE Main(void *)
{
   WriteLog(MSG_SERVER_STARTED, EVENTLOG_INFORMATION_TYPE, NULL);

   if (IsStandalone())
   {
      char *ptr, szCommand[256];
      struct __console_ctx ctx;

      ctx.hSocket = -1;
      ctx.pMsg = NULL;
      printf("\nNetXMS Server V" NETXMS_VERSION_STRING " Ready\n"
             "Enter \"help\" for command list or \"down\" for server shutdown\n"
             "System Console\n\n");

#if USE_READLINE
      // Initialize readline library if we use it
# if (RL_READLINE_VERSION && ((RL_VERSION_MAJOR == 4 && RL_VERSION_MINOR >= 2) || (RL_VERSION_MAJOR >= 5))) || __FreeBSD__ >= 5
      rl_bind_key('\t', (rl_command_func_t *)rl_insert);
# else 
      rl_bind_key('\t', (Function *)rl_insert);
# endif
#endif
      
      while(1)
      {
#if USE_READLINE
         ptr = readline("netxmsd: ");
#else
         printf("netxmsd: ");
         fflush(stdout);
         if (fgets(szCommand, 255, stdin) == NULL)
            break;   // Error reading stdin
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
               if (ProcessConsoleCommand(ptr, &ctx))
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
		ConditionWait(m_condShutdown, INFINITE);
      // On Win32, Shutdown() will be called by service control handler
#ifndef _WIN32
      Shutdown();
#endif
   }
}


//
// Initiate server shutdown
//

void InitiateShutdown(void)
{
#ifdef _WIN32
   Shutdown();
#else
   if (IsStandalone())
   {
      Shutdown();
   }
   else
   {
      kill(m_pid, SIGTERM);
   }
#endif
}


//
// DLL Entry point
//

#ifdef _WIN32

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   return TRUE;
}

#endif
