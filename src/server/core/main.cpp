/* 
** Project X - Network Management System
** Copyright (C) 2003 Victor Kirhenshtein
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
#ifdef _WIN32
#include <conio.h>
#endif   /* _WIN32 */

void DumpUsers(void);
void DumpSessions(void);


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

#ifdef _WIN32
static HANDLE m_hEventShutdown = INVALID_HANDLE_VALUE;
#else    /* _WIN32 */
static pthread_cond_t m_hCondShutdown;
#endif


//
// Sleep for specified number of seconds or until system shutdown arrives
// Function will return TRUE if shutdown event occurs
//

BOOL SleepAndCheckForShutdown(int iSeconds)
{
#ifdef _WIN32
   return WaitForSingleObject(m_hEventShutdown, iSeconds * 1000) == WAIT_OBJECT_0;
#else    /* _WIN32 */
   /* TODO: Implement UNIX code */
#endif
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
#ifdef _WIN32
   m_hEventShutdown = CreateEvent(NULL, TRUE, FALSE, NULL);
#else
   pthread_cond_init(&m_hCondShutdown, NULL);
#endif

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
#ifdef _WIN32
   SetEvent(m_hEventShutdown);
#else    /* _WIN32 */
   pthread_cond_broadcast(&m_hCondShutdown);
#endif

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

   CloseLog();
}


//
// Common main()
//

void Main(void)
{
   WriteLog(MSG_SERVER_STARTED, EVENTLOG_INFORMATION_TYPE, NULL);

#ifdef _WIN32
   if (IsStandalone())
   {
      int ch;

      printf("\n*** NMS Server operational. Press ESC to terminate. ***\n");
      while(1)
      {
         ch = getch();
         if (ch == 0)
            ch = -getch();

         if (ch == 27)
            break;

#ifdef _DEBUG
         switch(ch)
         {
            case 't':   // Thread status
            case 'T':
               WatchdogPrintStatus();
               printf("*** Done ***\n");
               break;
            case 'd':   // Dump objects
            case 'D':
               {
                  DWORD i;
                  char *pBuffer;
                  static char *objTypes[]={ "Generic", "Subnet", "Node", "Interface", "Network" };

                  pBuffer = (char *)malloc(128000);
                  MutexLock(g_hMutexIdIndex, INFINITE);
                  for(i = 0; i < g_dwIdIndexSize; i++)
                  {
                     printf("Object ID %d\n"
                            "   Name='%s' Type=%s Addr=%s Status=%d IsModified=%d\n",
                            g_pIndexById[i].pObject->Id(),g_pIndexById[i].pObject->Name(),
                            objTypes[g_pIndexById[i].pObject->Type()],
                            IpToStr(g_pIndexById[i].pObject->IpAddr(), pBuffer),
                            g_pIndexById[i].pObject->Status(), g_pIndexById[i].pObject->IsModified());
                     printf("   Parents: <%s> Childs: <%s>\n", 
                            g_pIndexById[i].pObject->ParentList(pBuffer),
                            g_pIndexById[i].pObject->ChildList(&pBuffer[4096]));
                     if (g_pIndexById[i].pObject->Type() == OBJECT_NODE)
                        printf("   IsSNMP:%d IsAgent:%d IsLocal:%d OID='%s'\n",
                               ((Node *)(g_pIndexById[i].pObject))->IsSNMPSupported(),
                               ((Node *)(g_pIndexById[i].pObject))->IsNativeAgent(),
                               ((Node *)(g_pIndexById[i].pObject))->IsLocalManagenet(),
                               ((Node *)(g_pIndexById[i].pObject))->ObjectId());
                  }
                  MutexUnlock(g_hMutexIdIndex);
                  free(pBuffer);
                  printf("*** Object dump complete ***\n");
               }
               break;
            case 'i':   // Dump interface index by IP
            case 'I':
               {
                  DWORD i;
                  char szBuffer[32];

                  for(i = 0; i < g_dwInterfaceAddrIndexSize; i++)
                  {
                     printf("%10u %-15s -> %d [0x%08x]\n",
                            g_pInterfaceIndexByAddr[i].dwKey,
                            IpToStr(g_pInterfaceIndexByAddr[i].dwKey, szBuffer),
                            g_pInterfaceIndexByAddr[i].pObject->Id(),
                            g_pInterfaceIndexByAddr[i].pObject);
                  }
                  printf("*** Interface IP index dump complete ***\n");
               }
               break;
            case 'm':   // Print mutex status
            case 'M':
               printf("Mutex status:\n");
               DbgTestMutex(g_hMutexIdIndex, "g_hMutexIdIndex");
               DbgTestMutex(g_hMutexNodeIndex, "g_hMutexNodeIndex");
               DbgTestMutex(g_hMutexSubnetIndex, "g_hMutexSubnetIndex");
               DbgTestMutex(g_hMutexInterfaceIndex, "g_hMutexInterfaceIndex");
               DbgTestMutex(g_hMutexObjectAccess, "g_hMutexObjectAccess");
               printf("*** Done ***\n");
               break;
            case 'g':   // Generate test objects
            case 'G':
               {
                  int i;
                  char szQuery[1024];

                  for(i = 0; i < 10000; i++)
                  {
                     sprintf(szQuery, "INSERT INTO newnodes (id,ip_addr,ip_netmask,discovery_flags) VALUES (%d,%d,65535,%d)",
                        i + 1000, htonl(0x0A800001 + i), DF_DEFAULT);
                     DBQuery(g_hCoreDB, szQuery);
                  }
               }
               break;
            case 'c':   // Create test items
            case 'C':
               {
                  DWORD i, j;
                  DC_ITEM item;

                  item.dwId = 20000;
                  item.iDataType = DT_INTEGER;
                  item.iPollingInterval = 60;
                  item.iRetentionTime = 30;
                  item.iSource = DS_INTERNAL;
                  for(i = 0; i < g_dwNodeAddrIndexSize; i++)
                  {
                     for(j = 1; j <= 100; j++)
                     {
                        sprintf(item.szName, "Debug.%d", j);
                        ((Node *)g_pNodeIndexByAddr[i].pObject)->AddItem(&item);
                        item.dwId++;
                     }
                  }
                  printf("*** Done ***\n");
               }
               break;
            case 'u':      // Dump users
            case 'U':
               DumpUsers();
               break;
            case 's':      // Dump sessions
            case 'S':
               DumpSessions();
               break;
            default:
               break;
         }
#endif
      }

      Shutdown();
   }
   else
   {
      WaitForSingleObject(m_hEventShutdown, INFINITE);
   }
#else    /* _WIN32 */
   /* TODO: insert UNIX main thread code here */
#endif
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

#ifndef _WIN32
   /* TODO: insert fork() here */
#endif   /* ! _WIN32 */

   if (!IsStandalone())
   {
      InitService();
   }
   else
   {
      if (!Initialize())
      {
         printf("NMS Core initialization failed\n");
         return 1;
      }
      Main();
   }
   return 0;
}
