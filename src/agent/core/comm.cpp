/* 
** NetXMS multiplatform core agent
** Copyright (C) 2003, 2004 Victor Kirhenshtein
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
** $module: comm.cpp
**
**/

#include "nxagentd.h"


//
// Global variables
//

DWORD g_dwAcceptErrors = 0;
DWORD g_dwAcceptedConnections = 0;
DWORD g_dwRejectedConnections = 0;


//
// Constants
//

#define MAX_CLIENT_SESSIONS   32


//
// Static data
//

static CommSession *m_pSessionList[MAX_CLIENT_SESSIONS];
static MUTEX m_hSessionListAccess;


//
// Validates server's address
//

static BOOL IsValidServerAddr(DWORD dwAddr)
{
   DWORD i;

   for(i=0; i < g_dwServerCount; i++)
      if (dwAddr == g_dwServerAddr[i])
         return TRUE;
   return FALSE;
}


//
// Register new session in list
//

static BOOL RegisterSession(CommSession *pSession)
{
   DWORD i;

   MutexLock(m_hSessionListAccess, INFINITE);
   for(i = 0; i < MAX_CLIENT_SESSIONS; i++)
      if (m_pSessionList[i] == NULL)
      {
         m_pSessionList[i] = pSession;
         pSession->SetIndex(i);
         MutexUnlock(m_hSessionListAccess);
         return TRUE;
      }

   MutexUnlock(m_hSessionListAccess);
   WriteLog(MSG_TOO_MANY_SESSIONS, EVENTLOG_WARNING_TYPE, NULL);
   return FALSE;
}


//
// Unregister session
//

static void UnregisterSession(DWORD dwIndex)
{
   MutexLock(m_hSessionListAccess, INFINITE);
   m_pSessionList[dwIndex] = NULL;
   MutexUnlock(m_hSessionListAccess);
}


//
// Client communication read thread
//

static void ReadThread(void *pArg)
{
   ((CommSession *)pArg)->ReadThread();

   // When CommSession::ReadThread exits, all other session
   // threads are already stopped, so we can safely destroy
   // session object
   UnregisterSession(((CommSession *)pArg)->GetIndex());
   delete (CommSession *)pArg;
}


//
// Client communication write thread
//

static void WriteThread(void *pArg)
{
   ((CommSession *)pArg)->WriteThread();
}


//
// Received message processing thread
//

static void ProcessingThread(void *pArg)
{
   ((CommSession *)pArg)->ProcessingThread();
}


//
// TCP/IP Listener
//

void ListenerThread(void *)
{
   SOCKET hSocket, hClientSocket;
   struct sockaddr_in servAddr;
   int iSize, iNumErrors = 0;
   CommSession *pSession;
   char szBuffer[256];

   // Create socket
   if ((hSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
   {
      WriteLog(MSG_SOCKET_ERROR, EVENTLOG_ERROR_TYPE, "e", WSAGetLastError());
      exit(1);
   }

   // Fill in local address structure
   memset(&servAddr, 0, sizeof(struct sockaddr_in));
   servAddr.sin_family = AF_INET;
   servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
   servAddr.sin_port = htons(g_wListenPort);

   // Bind socket
   if (bind(hSocket, (struct sockaddr *)&servAddr, sizeof(struct sockaddr_in)) != 0)
   {
      WriteLog(MSG_BIND_ERROR, EVENTLOG_ERROR_TYPE, "e", WSAGetLastError());
      exit(1);
   }

   // Set up queue
   listen(hSocket, SOMAXCONN);

   // Create session list access mutex
   m_hSessionListAccess = MutexCreate();

   // Wait for connection requests
   while(!(g_dwFlags & AF_SHUTDOWN))
   {
      iSize = sizeof(struct sockaddr_in);
      if ((hClientSocket = accept(hSocket, (struct sockaddr *)&servAddr, &iSize)) == -1)
      {
         int error = WSAGetLastError();

         if (error != WSAEINTR)
            WriteLog(MSG_ACCEPT_ERROR, EVENTLOG_ERROR_TYPE, "e", error);
         iNumErrors++;
         g_dwAcceptErrors++;
         if (iNumErrors > 1000)
         {
            WriteLog(MSG_TOO_MANY_ERRORS, EVENTLOG_WARNING_TYPE, NULL);
            iNumErrors = 0;
         }
         ThreadSleepMs(500);
      }

      iNumErrors = 0;     // Reset consecutive errors counter
      DebugPrintf("Incoming connection from %s", IpToStr(servAddr.sin_addr.S_addr, szBuffer));

      if (IsValidServerAddr(servAddr.sin_addr.S_addr))
      {
         g_dwAcceptedConnections++;

         // Create new session structure and threads
         pSession = new CommSession(hClientSocket, servAddr.sin_addr.s_addr);
         if (!RegisterSession(pSession))
         {
            delete pSession;
         }
         else
         {
            ThreadCreate(ReadThread, 0, (void *)pSession);
            ThreadCreate(WriteThread, 0, (void *)pSession);
            ThreadCreate(ProcessingThread, 0, (void *)pSession);
         }
      }
      else     // Unauthorized connection
      {
         g_dwRejectedConnections++;
         shutdown(hClientSocket, 2);
         closesocket(hClientSocket);
      }
   }

   MutexDestroy(m_hSessionListAccess);
}
