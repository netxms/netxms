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


//
// Global variables
//

DWORD g_dwFlags = 0;
char g_szConfigFile[MAX_PATH] = DEFAULT_CONFIG_FILE;
char g_szLogFile[MAX_PATH] = DEFAULT_LOG_FILE;
DB_HANDLE g_hCoreDB = 0;


//
// Static data
//

#ifdef _WIN32
HANDLE m_hEventShutdown = INVALID_HANDLE_VALUE;
#endif


//
// Server initialization
//

BOOL Initialize(void)
{
   InitLog();
   if (!DBInit())
      return FALSE;

   g_hCoreDB = DBConnect();
   if (g_hCoreDB == 0)
   {
      WriteLog(MSG_DB_CONNFAIL, EVENTLOG_ERROR_TYPE, NULL);
      return FALSE;
   }

   // Create synchronization stuff
#ifdef _WIN32
   m_hEventShutdown = CreateEvent(NULL, TRUE, FALSE, NULL);
#endif

   // Initialize objects
   ObjectsInit();

   // Start threads
   ThreadCreate(HouseKeeper, 0, NULL);
   ThreadCreate(DiscoveryThread, 0, NULL);
   ThreadCreate(Syncer, 0, NULL);
   ThreadCreate(NodePoller, 0, NULL);

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
#endif
   g_dwFlags |= AF_SHUTDOWN;     // Set shutdown flag
   ThreadSleep(15);     // Give other threads a chance to terminate in a safe way
   if (g_hCoreDB != 0)
      DBDisconnect(g_hCoreDB);
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
