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


//
// Thread functions
//

void HouseKeeper(void *arg);
void DiscoveryThread(void *arg);
void Syncer(void *arg);
void NodePoller(void *arg);
void StatusPoller(void *arg);
void ConfigurationPoller(void *arg);
void EventProcessor(void *arg);
void WatchdogThread(void *arg);
void ClientListener(void *);


//
// Global variables
//

DWORD g_dwFlags = 0;
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
}


//
// Server initialization
//

BOOL Initialize(void)
{
   int i, iNumThreads;
   InitLog();

#ifdef _WIN32
   WSADATA wsaData;
   WSAStartup(0x0002, &wsaData);
#endif

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
      return FALSE;

   // Initialize event handling subsystem
   if (!InitEventSubsystem())
      return FALSE;

   // Initialize objects infrastructure and load objects from database
   ObjectsInit();
   if (!LoadObjects())
      return FALSE;

   // Initialize data collection subsystem
   if (!InitDataCollector())
      return FALSE;

   // Initialize watchdog
   WatchdogInit();

   // Start threads
   ThreadCreate(WatchdogThread, 0, NULL);
   ThreadCreate(HouseKeeper, 0, NULL);
   ThreadCreate(DiscoveryThread, 0, NULL);
   ThreadCreate(Syncer, 0, NULL);
   ThreadCreate(NodePoller, 0, NULL);
   ThreadCreate(StatusPoller, 0, NULL);
   ThreadCreate(ConfigurationPoller, 0, NULL);
   ThreadCreate(ClientListener, 0, NULL);
   
   // Start event processors
   iNumThreads = ConfigReadInt("NumberOfEventProcessors", 1);
   for(i = 0; i < iNumThreads; i++)
      ThreadCreate(EventProcessor, 0, NULL);

   return TRUE;
}


//
// Server shutdown
//

void Shutdown(void)
{
   WriteLog(MSG_SERVER_STOPPED, EVENTLOG_INFORMATION_TYPE, NULL);
#ifdef _WIN32
   SetEvent(m_hEventShutdown);
#else    /* _WIN32 */
   pthread_cond_broadcast(&m_hCondShutdown);
#endif
   g_dwFlags |= AF_SHUTDOWN;     // Set shutdown flag
   ThreadSleep(5);     // Give other threads a chance to terminate in a safe way
   SaveObjects();
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
                  char szBuffer[1024];
                  static char *objTypes[]={ "Generic", "Subnet", "Node", "Interface", "Network" };

                  MutexLock(g_hMutexIdIndex, INFINITE);
                  for(i = 0; i < g_dwIdIndexSize; i++)
                  {
                     printf("Object ID %d\n"
                            "   Name='%s' Type=%s Addr=%s Status=%d IsModified=%d\n",
                            g_pIndexById[i].pObject->Id(),g_pIndexById[i].pObject->Name(),
                            objTypes[g_pIndexById[i].pObject->Type()],
                            IpToStr(g_pIndexById[i].pObject->IpAddr(), szBuffer),
                            g_pIndexById[i].pObject->Status(), g_pIndexById[i].pObject->IsModified());
                     printf("   Parents: <%s> Childs: <%s>\n", 
                            g_pIndexById[i].pObject->ParentList(szBuffer),
                            g_pIndexById[i].pObject->ChildList(&szBuffer[512]));
                     if (g_pIndexById[i].pObject->Type() == OBJECT_NODE)
                        printf("   IsSNMP:%d IsAgent:%d IsLocal:%d OID='%s'\n",
                               ((Node *)(g_pIndexById[i].pObject))->IsSNMPSupported(),
                               ((Node *)(g_pIndexById[i].pObject))->IsNativeAgent(),
                               ((Node *)(g_pIndexById[i].pObject))->IsLocalManagenet(),
                               ((Node *)(g_pIndexById[i].pObject))->ObjectId());
                  }
                  MutexUnlock(g_hMutexIdIndex);
                  printf("*** Object dump complete ***\n");
               }
               break;
            case 'i':   // Dump interface index by IP
            case 'I':
               {
                  DWORD i;
                  char szBuffer[32];
                  static char *objTypes[]={ "Generic", "Subnet", "Node", "Interface", "Network" };

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
