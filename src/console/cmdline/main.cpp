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
#include <time.h>


//
// Global variables
//

DWORD g_dwState = STATE_DISCONNECTED;
CONDITION g_hCondOperationComplete;
HREQUEST g_hOpenRq;


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
         switch(dwCode)
         {
            case STATE_IDLE:
            case STATE_DISCONNECTED:
               ConditionSet(g_hCondOperationComplete);
               break;
            default:
               break;
         }
         break;
      case NXC_EVENT_NEW_ELOG_RECORD:
         printf("LOG_RECORD: %s\n", ((NXC_EVENT *)pArg)->szMessage);
         MemFree(pArg);
         break;
      case NXC_EVENT_REQUEST_COMPLETED:
         printf("Request %d completed with code %d\n", dwCode, (DWORD)pArg);
         break;
      default:
         printf("Event %d [Code = %d, Arg = 0x%08X]\n", dwEvent, dwCode, pArg);
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
   char szServer[256], szLogin[256], szPassword[256];
   NXC_EVENT_TEMPLATE **ppEventList;
   DWORD dwNumEvents;

#ifdef _WIN32
   WSADATA wsaData;

   WSAStartup(0x0002, &wsaData);
#endif

   g_hCondOperationComplete = ConditionCreate();

   dwVersion = NXCGetVersion();
   printf("Using NetXMS Client Library version %d.%d.%d\n", dwVersion >> 24,
          (dwVersion >> 16) & 255, dwVersion & 0xFFFF);
   NXCInitialize();
   NXCSetEventHandler(EventHandler);
   //NXCSetDebugCallback(DebugCallback);

strcpy(szServer,"eagle");
strcpy(szLogin,"admin");
strcpy(szPassword,"netxms");

   printf("Connecting to server %s as user %s ...\n", szServer, szLogin);
   NXCConnect(szServer, szLogin, szPassword);
   ConditionWait(g_hCondOperationComplete, INFINITE);
   if (g_dwState == STATE_DISCONNECTED)
   {
      printf("Unable to establish connection with the server.\n");
      return 1;
   }
   printf("Connection established.\n");

   // Load oblects from server
   printf("Loading objects...\n");
   NXCSyncObjects();
   ConditionWait(g_hCondOperationComplete, INFINITE);
   printf("Objects synchronized.\n");

   //NXCEnumerateObjects(PrintObject);

   time_t t = time(0);
   printf("%s Loading events...\n", ctime(&t));
   NXCSyncEvents();
   ConditionWait(g_hCondOperationComplete, INFINITE);
   t = time(0);
   printf("%s All events loaded.\n", ctime(&t));

   NXCSetDebugCallback(DebugCallback);
   printf("Debug mode activated\n");

   g_hOpenRq = NXCOpenEventDB();
   ConditionWait(g_hCondOperationComplete, INFINITE);
   printf("Event templates loaded.\n");

   if (NXCGetEventDB(&ppEventList, &dwNumEvents))
   {
      DWORD i;

      for(i = 0; i < dwNumEvents; i++)
         printf("%5d %-24s %s\n", ppEventList[i]->dwCode, ppEventList[i]->szName,
                ppEventList[i]->pszMessage);
      printf("%d events total\n", dwNumEvents);
   }
   else
   {
      printf("NXCGetEventDB() failed\n");
   }
   NXCCloseEventDB();

   while(1)
      Sleep(1000);

   return 0;
}
