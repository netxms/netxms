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

static BOOL IsValidServerAddr(DWORD dwAddr, BOOL *pbInstallationServer)
{
   DWORD i;

   for(i=0; i < g_dwServerCount; i++)
      if (dwAddr == g_pServerList[i].dwIpAddr)
      {
         *pbInstallationServer = g_pServerList[i].bInstallationServer;
         return TRUE;
      }
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

void UnregisterSession(DWORD dwIndex)
{
   MutexLock(m_hSessionListAccess, INFINITE);
   m_pSessionList[dwIndex] = NULL;
   MutexUnlock(m_hSessionListAccess);
}


//
// TCP/IP Listener
//

THREAD_RESULT THREAD_CALL ListenerThread(void *)
{
   SOCKET hSocket, hClientSocket;
   struct sockaddr_in servAddr;
   int iNumErrors = 0;
   socklen_t iSize;
   CommSession *pSession;
   char szBuffer[256];
   BOOL bInstallationServer;

   // Create socket
   if ((hSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
   {
      WriteLog(MSG_SOCKET_ERROR, EVENTLOG_ERROR_TYPE, "e", WSAGetLastError());
      exit(1);
   }

	SetSocketReuseFlag(hSocket);
#ifndef _WIN32
   fcntl(hSocket, F_SETFD, fcntl(hSocket, F_GETFD) | FD_CLOEXEC);
#endif

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

      // Socket should be closed on successful exec
#ifndef _WIN32
      fcntl(hSocket, F_SETFD, fcntl(hSocket, F_GETFD) | FD_CLOEXEC);
#endif

      iNumErrors = 0;     // Reset consecutive errors counter
      DebugPrintf("Incoming connection from %s", IpToStr(ntohl(servAddr.sin_addr.s_addr), szBuffer));

      if (IsValidServerAddr(servAddr.sin_addr.s_addr, &bInstallationServer))
      {
         g_dwAcceptedConnections++;
         DebugPrintf("Connection from %s accepted", szBuffer);

         // Create new session structure and threads
         pSession = new CommSession(hClientSocket, ntohl(servAddr.sin_addr.s_addr), 
                                    bInstallationServer);
         if (!RegisterSession(pSession))
         {
            delete pSession;
         }
         else
         {
            pSession->Run();
         }
      }
      else     // Unauthorized connection
      {
         g_dwRejectedConnections++;
         shutdown(hClientSocket, 2);
         closesocket(hClientSocket);
         DebugPrintf("Connection from %s rejected", szBuffer);
      }
   }

   MutexDestroy(m_hSessionListAccess);
   return THREAD_OK;
}
