/* 
** NetXMS - Network Management System
** Command line client
** Copyright (C) 2004 Victor Kirhenshtein
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

#include "nxcmd.h"

#include <string.h>


//
// Global variables
//

DWORD g_dwState = STATE_DISCONNECTED;


//
// Callback function for debug printing
//

static void DebugCallback(char *pMsg)
{
   printf("*debug* %s\n", pMsg);
}


//
// Event handler
//

static void EventHandler(DWORD dwEvent, DWORD dwCode, void *pArg)
{
   switch(dwEvent)
   {
      case NXC_EVENT_STATE_CHANGED:
         g_dwState = dwCode;
         break;
      case NXC_EVENT_NEW_ELOG_RECORD:
         printf("EVENT: %s\n", ((NXC_EVENT *)pArg)->szMessage);
         free(pArg);
         break;
      default:
         break;
   }
}


//
// Print object
//

static BOOL PrintObject(NXC_OBJECT *pObject)
{
   static char *pszObjectClass[] = { "generic", "subnet", "node", "interface", "network" };
   printf("%4d %s \"%s\"\n", pObject->dwId, pszObjectClass[pObject->iClass],
          pObject->szName);
   return TRUE;
}


//
// main()
//

int main(int argc, char *argv[])
{
   DWORD dwVersion;
   char szServer[256] = "127.0.0.1", szLogin[256] = "", szPassword[256] = "";
   DWORD dwResult;
   int ch;
   BOOL bStart = TRUE, bDebug = FALSE, bPasswordProvided = FALSE;

#ifdef _WIN32
   WSADATA wsaData;
   WSAStartup(0x0002, &wsaData);
#endif

   // Parse command line
   opterr = 1;
   while((ch = getopt(argc, argv, "dhp:s:u:")) != -1)
   {
      switch(ch)
      {
         case 'd':   // Debug on
            bDebug = TRUE;
            break;
         case 'h':   // Display help and exit
            printf("Usage: nxcmd [<options>]\n"
                   "Valid options are:\n"
                   "   -d            : Turn on debug output\n"
                   "   -h            : Display help and exit.\n"
                   "   -p <password> : Login using specified password\n"
                   "   -s <server>   : Connect to specified server (default is 127.0.0.1)\n"
                   "   -u <login>    : Use specified login name\n"
                   "\n");
            bStart = FALSE;
            break;
         case 'p':   // Password
            strncpy(szPassword, optarg, 255);
            bPasswordProvided = TRUE;
            break;
         case 's':   // Server
            strncpy(szServer, optarg, 255);
            break;
         case 'u':   // Login
            strncpy(szLogin, optarg, 255);
            break;
         case '?':
            bStart = FALSE;
            break;
         default:
            break;
      }
   }

   if (!bStart)
      return 1;

   dwVersion = NXCGetVersion();
   printf("Using NetXMS Client Library version %d.%d.%d\n", dwVersion >> 24,
          (dwVersion >> 16) & 255, dwVersion & 0xFFFF);
   if (!NXCInitialize())
   {
      printf("Failed to initialize NetXMS client library\n");
      return 1;
   }

   // Ask login name if not specified on command line
   if (szLogin[0] == 0)
   {
      printf("Login [admin]: ");
      gets(szLogin);
      if (szLogin[0] == 0)
         strcpy(szLogin, "admin");
   }

   // Ask for password if needed
   if ((szPassword[0] == 0) && (!bPasswordProvided))
   {
      printf("Password: ");
      gets(szPassword);
   }

   NXCSetEventHandler(EventHandler);
   if (bDebug)
      NXCSetDebugCallback(DebugCallback);

   printf("Connecting to server %s as user %s ...\n", szServer, szLogin);
   dwResult = NXCConnect(szServer, szLogin, szPassword);
   if (dwResult != RCC_SUCCESS)
   {
      printf("Unable to connect to server: %s\n", NXCGetErrorText(dwResult));
   }
   else
   {
      printf("Connection established.\n");

      CommandLoop();
      NXCDisconnect();
   }

   return 0;
}
