/* 
** NetXMS multiplatform core agent
** Copyright (C) 2003-2013 Victor Kirhenshtein
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
** File: comm.cpp
**
**/

#include "nxagentd.h"


//
// Global variables
//

UINT32 g_dwAcceptErrors = 0;
UINT32 g_dwAcceptedConnections = 0;
UINT32 g_dwRejectedConnections = 0;
CommSession **g_pSessionList = NULL;
MUTEX g_hSessionListAccess;


//
// Static data
//

static MUTEX m_mutexWatchdogActive = INVALID_MUTEX_HANDLE;


//
// Initialize session list
//

void InitSessionList()
{
	// Create session list and it's access mutex
	g_dwMaxSessions = min(max(g_dwMaxSessions, 2), 1024);
	g_pSessionList = (CommSession **)malloc(sizeof(CommSession *) * g_dwMaxSessions);
	memset(g_pSessionList, 0, sizeof(CommSession *) * g_dwMaxSessions);
	g_hSessionListAccess = MutexCreate();
}


//
// Validates server's address
//

static BOOL IsValidServerAddr(UINT32 dwAddr, BOOL *pbMasterServer, BOOL *pbControlServer)
{
   UINT32 i;

   for(i=0; i < g_dwServerCount; i++)
	{
      if ((dwAddr & g_pServerList[i].dwNetMask) == g_pServerList[i].dwIpAddr)
      {
         *pbMasterServer = g_pServerList[i].bMasterServer;
         *pbControlServer = g_pServerList[i].bControlServer;
         return TRUE;
      }
	}
   return FALSE;
}


//
// Register new session in list
//

static BOOL RegisterSession(CommSession *pSession)
{
   UINT32 i;

   MutexLock(g_hSessionListAccess);
   for(i = 0; i < g_dwMaxSessions; i++)
      if (g_pSessionList[i] == NULL)
      {
         g_pSessionList[i] = pSession;
         pSession->setIndex(i);
         MutexUnlock(g_hSessionListAccess);
         return TRUE;
      }

   MutexUnlock(g_hSessionListAccess);
   nxlog_write(MSG_TOO_MANY_SESSIONS, EVENTLOG_WARNING_TYPE, NULL);
   return FALSE;
}


//
// Unregister session
//

void UnregisterSession(UINT32 dwIndex)
{
   MutexLock(g_hSessionListAccess);
   g_pSessionList[dwIndex] = NULL;
   MutexUnlock(g_hSessionListAccess);
}


//
// TCP/IP Listener
// 

THREAD_RESULT THREAD_CALL ListenerThread(void *)
{
   SOCKET hSocket, hClientSocket;
   struct sockaddr_in servAddr;
   int iNumErrors = 0, nRet;
   socklen_t iSize;
   CommSession *pSession;
   TCHAR szBuffer[256];
   BOOL bMasterServer, bControlServer;
   struct timeval tv;
   fd_set rdfs;

   // Create socket
   if ((hSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
   {
      nxlog_write(MSG_SOCKET_ERROR, EVENTLOG_ERROR_TYPE, "e", WSAGetLastError());
      exit(1);
   }

	SetSocketExclusiveAddrUse(hSocket);
	SetSocketReuseFlag(hSocket);
#ifndef _WIN32
   fcntl(hSocket, F_SETFD, fcntl(hSocket, F_GETFD) | FD_CLOEXEC);
#endif

   // Fill in local address structure
   memset(&servAddr, 0, sizeof(struct sockaddr_in));
   servAddr.sin_family = AF_INET;
	if (!_tcscmp(g_szListenAddress, _T("*")))
	{
		servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	}
	else
	{
		servAddr.sin_addr.s_addr = ResolveHostName(g_szListenAddress);
		if (servAddr.sin_addr.s_addr == htonl(INADDR_NONE))
			servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	}
   servAddr.sin_port = htons(g_wListenPort);

   // Bind socket
	DebugPrintf(INVALID_INDEX, 1, _T("Trying to bind on %s:%d"), IpToStr(ntohl(servAddr.sin_addr.s_addr), szBuffer), ntohs(servAddr.sin_port));
   if (bind(hSocket, (struct sockaddr *)&servAddr, sizeof(struct sockaddr_in)) != 0)
   {
      nxlog_write(MSG_BIND_ERROR, EVENTLOG_ERROR_TYPE, "e", WSAGetLastError());
      exit(1);
   }

   // Set up queue
   listen(hSocket, SOMAXCONN);
	nxlog_write(MSG_LISTENING, EVENTLOG_INFORMATION_TYPE, "ad", ntohl(servAddr.sin_addr.s_addr), g_wListenPort);

   // Wait for connection requests
   while(!(g_dwFlags & AF_SHUTDOWN))
   {
      tv.tv_sec = 1;
      tv.tv_usec = 0;
      FD_ZERO(&rdfs);
      FD_SET(hSocket, &rdfs);
      nRet = select(SELECT_NFDS(hSocket + 1), &rdfs, NULL, NULL, &tv);
      if ((nRet > 0) && (!(g_dwFlags & AF_SHUTDOWN)))
      {
         iSize = sizeof(struct sockaddr_in);
         if ((hClientSocket = accept(hSocket, (struct sockaddr *)&servAddr, &iSize)) == -1)
         {
            int error = WSAGetLastError();

            if (error != WSAEINTR)
               nxlog_write(MSG_ACCEPT_ERROR, EVENTLOG_ERROR_TYPE, "e", error);
            iNumErrors++;
            g_dwAcceptErrors++;
            if (iNumErrors > 1000)
            {
               nxlog_write(MSG_TOO_MANY_ERRORS, EVENTLOG_WARNING_TYPE, NULL);
               iNumErrors = 0;
            }
            ThreadSleepMs(500);
            continue;
         }

         // Socket should be closed on successful exec
#ifndef _WIN32
         fcntl(hClientSocket, F_SETFD, fcntl(hClientSocket, F_GETFD) | FD_CLOEXEC);
#endif

         iNumErrors = 0;     // Reset consecutive errors counter
         DebugPrintf(INVALID_INDEX, 5, _T("Incoming connection from %s"), IpToStr(ntohl(servAddr.sin_addr.s_addr), szBuffer));

         if (IsValidServerAddr(servAddr.sin_addr.s_addr, &bMasterServer, &bControlServer))
         {
            g_dwAcceptedConnections++;
            DebugPrintf(INVALID_INDEX, 5, _T("Connection from %s accepted"), szBuffer);

            // Create new session structure and threads
            pSession = new CommSession(hClientSocket, ntohl(servAddr.sin_addr.s_addr), 
                                       bMasterServer, bControlServer);
			
            if (!RegisterSession(pSession))
            {
               delete pSession;
            }
            else
            {
               pSession->run();
            }
         }
         else     // Unauthorized connection
         {
            g_dwRejectedConnections++;
            shutdown(hClientSocket, SHUT_RDWR);
            closesocket(hClientSocket);
            DebugPrintf(INVALID_INDEX, 5, _T("Connection from %s rejected"), szBuffer);
         }
      }
      else if (nRet == -1)
      {
         int error = WSAGetLastError();

         // On AIX, select() returns ENOENT after SIGINT for unknown reason
#ifdef _WIN32
         if (error != WSAEINTR)
#else
         if ((error != EINTR) && (error != ENOENT))
#endif
         {
            nxlog_write(MSG_SELECT_ERROR, EVENTLOG_ERROR_TYPE, "e", error);
            ThreadSleepMs(100);
         }
      }
   }

   // Wait for watchdog thread
   MutexLock(m_mutexWatchdogActive);
   MutexUnlock(m_mutexWatchdogActive);
   MutexDestroy(m_mutexWatchdogActive);

   MutexDestroy(g_hSessionListAccess);
   free(g_pSessionList);
   closesocket(hSocket);
   DebugPrintf(INVALID_INDEX, 1, _T("Listener thread terminated"));
   return THREAD_OK;
}


//
// Session watchdog thread
//

THREAD_RESULT THREAD_CALL SessionWatchdog(void *)
{
   UINT32 i;
   time_t now;

   m_mutexWatchdogActive = MutexCreate();
   MutexLock(m_mutexWatchdogActive);

   ThreadSleep(5);
   while(!(g_dwFlags & AF_SHUTDOWN))
   {
      ThreadSleep(1);

      MutexLock(g_hSessionListAccess);
      now = time(NULL);
      for(i = 0; i < g_dwMaxSessions; i++)
         if (g_pSessionList[i] != NULL)
         {
            if (g_pSessionList[i]->getTimeStamp() < (now - (time_t)g_dwIdleTimeout))
				{
					DebugPrintf(i, 5, _T("Session disconnected by watchdog (last activity timestamp is ") TIME_T_FMT _T(")"), g_pSessionList[i]->getTimeStamp());
               g_pSessionList[i]->disconnect();
				}
         }
      MutexUnlock(g_hSessionListAccess);
   }

   // Disconnect all sessions
   MutexLock(g_hSessionListAccess);
   for(i = 0; i < g_dwMaxSessions; i++)
      if (g_pSessionList[i] != NULL)
         g_pSessionList[i]->disconnect();
   MutexUnlock(g_hSessionListAccess);

   ThreadSleep(1);
   MutexUnlock(m_mutexWatchdogActive);
   DebugPrintf(INVALID_INDEX, 1, _T("Session Watchdog thread terminated"));

   return THREAD_OK;
}


//
// Handler for Agent.ActiveConnections parameter
//

LONG H_ActiveConnections(const TCHAR *pszCmd, const TCHAR *pArg, TCHAR *pValue)
{
   int nCounter;
   UINT32 i;

   MutexLock(g_hSessionListAccess);
   for(i = 0, nCounter = 0; i < g_dwMaxSessions; i++)
      if (g_pSessionList[i] != NULL)
         nCounter++;
   MutexUnlock(g_hSessionListAccess);
   ret_int(pValue, nCounter);
   return SYSINFO_RC_SUCCESS;
}
