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
void EventProcessor(void *arg);


//
// Global variables
//

DWORD g_dwFlags = 0;
char g_szConfigFile[MAX_PATH] = DEFAULT_CONFIG_FILE;
char g_szLogFile[MAX_PATH] = DEFAULT_LOG_FILE;
DB_HANDLE g_hCoreDB = 0;
DWORD g_dwDiscoveryPollingInterval;


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
   g_dwDiscoveryPollingInterval = ConfigReadInt("DiscoveryPollingInterval", 3600);
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
   init_mib();

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

   // Initialize event handling subsystem
   if (!InitEventSubsystem())
      return FALSE;

   // Initialize objects infrastructure and load objects from database
   ObjectsInit();
   if (!LoadObjects())
      return FALSE;

   // Start threads
   ThreadCreate(HouseKeeper, 0, NULL);
   ThreadCreate(DiscoveryThread, 0, NULL);
   ThreadCreate(Syncer, 0, NULL);
   ThreadCreate(NodePoller, 0, NULL);
   
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
            case 'd':   // Dump objects
            case 'D':
               {
                  DWORD i;
                  char szBuffer[32];
                  static char *objTypes[]={ "Generic", "Subnet", "Node", "Interface", "Network" };

                  for(i = 0; i < g_dwIdIndexSize; i++)
                  {
                     printf("Object ID %d\n"
                            "   Name='%s' Type=%s Addr=%s\n",
                            g_pIndexById[i].pObject->Id(),g_pIndexById[i].pObject->Name(),
                            objTypes[g_pIndexById[i].pObject->Type()],
                            IpToStr(g_pIndexById[i].pObject->IpAddr(), szBuffer));
                     if (g_pIndexById[i].pObject->Type() == OBJECT_NODE)
                        printf("   IsSNMP:%d IsAgent:%d IsLocal:%d OID='%s'\n",
                               ((Node *)(g_pIndexById[i].pObject))->IsSNMPSupported(),
                               ((Node *)(g_pIndexById[i].pObject))->IsNativeAgent(),
                               ((Node *)(g_pIndexById[i].pObject))->IsLocalManagenet(),
                               ((Node *)(g_pIndexById[i].pObject))->ObjectId());
                  }
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
            case 'a':
               {
                  char szBuffer[32];
                  ARP_CACHE *arp;
                  arp=SnmpGetArpCache(inet_addr("10.0.0.44"),"public");
                  for(DWORD i=0;i<arp->dwNumEntries;i++)
                     printf("%d %s %02X:%02X:%02X:%02X:%02X:%02X\n",
                        arp->pEntries[i].dwIndex,
                        IpToStr(arp->pEntries[i].dwIpAddr,szBuffer),
                        arp->pEntries[i].bMacAddr[0],
                        arp->pEntries[i].bMacAddr[1],
                        arp->pEntries[i].bMacAddr[2],
                        arp->pEntries[i].bMacAddr[3],
                        arp->pEntries[i].bMacAddr[4],
                        arp->pEntries[i].bMacAddr[5]);
                  printf("Total %d entries\n",arp->dwNumEntries);
                  DestroyArpCache(arp);
               }
               break;
            case 's':
               {
                  INTERFACE_LIST *iflist;
                  printf("*** 10.0.0.1 *** \n");
                  iflist=SnmpGetInterfaceList(inet_addr("10.0.0.1"),"public");
                  if (iflist != NULL)
                  {
                     char addr[32],mask[32];
                     for(int i=0;i<iflist->iNumEntries;i++)
                        printf("IF: %d %s %s %s\n",iflist->pInterfaces[i].dwIndex,
                        IpToStr(iflist->pInterfaces[i].dwIpAddr,addr),
                        IpToStr(iflist->pInterfaces[i].dwIpNetMask,mask),
                        iflist->pInterfaces[i].szName);
                     DestroyInterfaceList(iflist);
                  }
                  printf("*** 159.148.208.1 *** \n");
                  iflist=SnmpGetInterfaceList(inet_addr("159.148.208.1"),"public");
                  if (iflist != NULL)
                  {
                     char addr[32],mask[32];
                     for(int i=0;i<iflist->iNumEntries;i++)
                        printf("IF: %d %s %s %s\n",iflist->pInterfaces[i].dwIndex,
                        IpToStr(iflist->pInterfaces[i].dwIpAddr,addr),
                        IpToStr(iflist->pInterfaces[i].dwIpNetMask,mask),
                        iflist->pInterfaces[i].szName);
                     DestroyInterfaceList(iflist);
                  }
                  printf("*** 10.0.0.77 *** \n");
                  iflist=SnmpGetInterfaceList(inet_addr("10.0.0.77"),"public");
                  if (iflist != NULL)
                  {
                     char addr[32],mask[32];
                     for(int i=0;i<iflist->iNumEntries;i++)
                        printf("IF: %d %s %s %s\n",iflist->pInterfaces[i].dwIndex,
                        IpToStr(iflist->pInterfaces[i].dwIpAddr,addr),
                        IpToStr(iflist->pInterfaces[i].dwIpNetMask,mask),
                        iflist->pInterfaces[i].szName);
                     DestroyInterfaceList(iflist);
                  }
                  printf("*** 159.148.208.222 *** \n");
                  iflist=SnmpGetInterfaceList(inet_addr("159.148.208.222"),"public");
                  if (iflist != NULL)
                  {
                     char addr[32],mask[32];
                     for(int i=0;i<iflist->iNumEntries;i++)
                        printf("IF: %d %s %s %s\n",iflist->pInterfaces[i].dwIndex,
                        IpToStr(iflist->pInterfaces[i].dwIpAddr,addr),
                        IpToStr(iflist->pInterfaces[i].dwIpNetMask,mask),
                        iflist->pInterfaces[i].szName);
                     DestroyInterfaceList(iflist);
                  }
               }
               break;
            case 'l':
               {
                  char buffer[256],addr[32],mask[32];
                  INTERFACE_LIST *iflist;
                  AgentConnection *ac = new AgentConnection(inet_addr("127.0.0.1"));
                  ac->Connect();
                  ac->GetParameter("Agent.Version",255,buffer);
                  printf("Agent version: %s\n",buffer);
                  iflist = ac->GetInterfaceList();
                  if (iflist != NULL)
                  {
                     for(int i=0;i<iflist->iNumEntries;i++)
                        printf("IF: %d %s %s %s\n",iflist->pInterfaces[i].dwIndex,
                        IpToStr(iflist->pInterfaces[i].dwIpAddr,addr),
                        IpToStr(iflist->pInterfaces[i].dwIpNetMask,mask),
                        iflist->pInterfaces[i].szName);
                     DestroyInterfaceList(iflist);
                  }
                  ARP_CACHE *arp;
                  arp=ac->GetArpCache();
                  if (arp!=NULL)
                  {
                     for(DWORD i=0;i<arp->dwNumEntries;i++)
                        printf("%d %s %02X:%02X:%02X:%02X:%02X:%02X\n",
                           arp->pEntries[i].dwIndex,
                           IpToStr(arp->pEntries[i].dwIpAddr,buffer),
                           arp->pEntries[i].bMacAddr[0],
                           arp->pEntries[i].bMacAddr[1],
                           arp->pEntries[i].bMacAddr[2],
                           arp->pEntries[i].bMacAddr[3],
                           arp->pEntries[i].bMacAddr[4],
                           arp->pEntries[i].bMacAddr[5]);
                     printf("Total %d entries\n",arp->dwNumEntries);
                     DestroyArpCache(arp);
                  }
                  else
                  {
                     printf("Unable to retrieve ARP cache from agent\n");
                  }
                  ac->Disconnect();
                  delete ac;
               }
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
