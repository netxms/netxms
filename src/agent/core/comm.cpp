/* 
** NetXMS multiplatform core agent
** Copyright (C) 2003-2014 Victor Kirhenshtein
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

/**
 * Static data
 */
static MUTEX m_mutexWatchdogActive = INVALID_MUTEX_HANDLE;

/**
 * Initialize session list
 */
void InitSessionList()
{
	// Create session list and it's access mutex
	g_dwMaxSessions = min(max(g_dwMaxSessions, 2), 1024);
	g_pSessionList = (CommSession **)malloc(sizeof(CommSession *) * g_dwMaxSessions);
	memset(g_pSessionList, 0, sizeof(CommSession *) * g_dwMaxSessions);
	g_hSessionListAccess = MutexCreate();
}

/**
 * Validates server's address
 */
static bool IsValidServerAddress(const InetAddress &addr, bool *pbMasterServer, bool *pbControlServer)
{
   for(int i = 0; i < g_serverList.size(); i++)
	{
      ServerInfo *s = g_serverList.get(i);
      if (s->match(addr))
      {
         *pbMasterServer = s->isMaster();
         *pbControlServer = s->isControl();
         return true;
      }
	}
   return false;
}

/**
 * Register new session in list
 */
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

/**
 * Unregister session
 */
void UnregisterSession(UINT32 dwIndex)
{
   MutexLock(g_hSessionListAccess);
   g_pSessionList[dwIndex] = NULL;
   MutexUnlock(g_hSessionListAccess);
}

/**
 * TCP/IP Listener
 */ 
THREAD_RESULT THREAD_CALL ListenerThread(void *)
{
   // Create socket(s)
   SOCKET hSocket = (g_dwFlags & AF_DISABLE_IPV4) ? INVALID_SOCKET : socket(AF_INET, SOCK_STREAM, 0);
#ifdef WITH_IPV6
   SOCKET hSocket6 = (g_dwFlags & AF_DISABLE_IPV6) ? INVALID_SOCKET : socket(AF_INET6, SOCK_STREAM, 0);
#endif
   if (((hSocket == INVALID_SOCKET) && !(g_dwFlags & AF_DISABLE_IPV4))
#ifdef WITH_IPV6
       && ((hSocket6 == INVALID_SOCKET) && !(g_dwFlags & AF_DISABLE_IPV6))
#endif
      )
   {
      nxlog_write(MSG_SOCKET_ERROR, EVENTLOG_ERROR_TYPE, "e", WSAGetLastError());
      exit(1);
   }

   if (!(g_dwFlags & AF_DISABLE_IPV4))
   {
	   SetSocketExclusiveAddrUse(hSocket);
	   SetSocketReuseFlag(hSocket);
#ifndef _WIN32
      fcntl(hSocket, F_SETFD, fcntl(hSocket, F_GETFD) | FD_CLOEXEC);
#endif
   }

#ifdef WITH_IPV6
   if (!(g_dwFlags & AF_DISABLE_IPV6))
   {
	   SetSocketExclusiveAddrUse(hSocket6);
	   SetSocketReuseFlag(hSocket6);
#ifndef _WIN32
      fcntl(hSocket6, F_SETFD, fcntl(hSocket6, F_GETFD) | FD_CLOEXEC);
#endif
   }
#endif

   // Fill in local address structure
   struct sockaddr_in servAddr;
   memset(&servAddr, 0, sizeof(struct sockaddr_in));
   servAddr.sin_family = AF_INET;

#ifdef WITH_IPV6
   struct sockaddr_in6 servAddr6;
   memset(&servAddr6, 0, sizeof(struct sockaddr_in6));
   servAddr6.sin6_family = AF_INET6;
#endif

   if (!_tcscmp(g_szListenAddress, _T("*")))
	{
		servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
#ifdef WITH_IPV6
		memset(servAddr6.sin6_addr.s6_addr, 0, 16);
#endif
	}
	else
	{
      InetAddress bindAddress = InetAddress::resolveHostName(g_szListenAddress, AF_INET);
      if (bindAddress.isValid() && (bindAddress.getFamily() == AF_INET))
      {
		   servAddr.sin_addr.s_addr = bindAddress.getAddressV4();
      }
      else
      {
   		servAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
      }
#ifdef WITH_IPV6
      bindAddress = InetAddress::resolveHostName(g_szListenAddress, AF_INET6);
      if (bindAddress.isValid() && (bindAddress.getFamily() == AF_INET6))
      {
		   memcpy(servAddr6.sin6_addr.s6_addr, bindAddress.getAddressV6(), 16);
      }
      else
      {
   		memset(servAddr6.sin6_addr.s6_addr, 0, 15);
         servAddr6.sin6_addr.s6_addr[15] = 1;
      }
#endif
	}
   servAddr.sin_port = htons(g_wListenPort);
#ifdef WITH_IPV6
   servAddr6.sin6_port = htons(g_wListenPort);
#endif

   // Bind socket
   TCHAR buffer[64];
   int bindFailures = 0;
   if (!(g_dwFlags & AF_DISABLE_IPV4))
   {
	   DebugPrintf(INVALID_INDEX, 1, _T("Trying to bind on %s:%d"), SockaddrToStr((struct sockaddr *)&servAddr, buffer), ntohs(servAddr.sin_port));
      if (bind(hSocket, (struct sockaddr *)&servAddr, sizeof(struct sockaddr_in)) != 0)
      {
         nxlog_write(MSG_BIND_ERROR, EVENTLOG_ERROR_TYPE, "e", WSAGetLastError());
         bindFailures++;
      }
   }
   else
   {
      bindFailures++;
   }

#ifdef WITH_IPV6
   if (!(g_dwFlags & AF_DISABLE_IPV6))
   {
      DebugPrintf(INVALID_INDEX, 1, _T("Trying to bind on [%s]:%d"), SockaddrToStr((struct sockaddr *)&servAddr6, buffer), ntohs(servAddr6.sin6_port));
      if (bind(hSocket6, (struct sockaddr *)&servAddr6, sizeof(struct sockaddr_in6)) != 0)
      {
         nxlog_write(MSG_BIND_ERROR, EVENTLOG_ERROR_TYPE, "e", WSAGetLastError());
         bindFailures++;
      }
   }
   else
   {
      bindFailures++;
   }
#else
   bindFailures++;
#endif

   // Abort if cannot bind to socket
   if (bindFailures == 2)
   {
      exit(1);
   }

   // Set up queue
   if (!(g_dwFlags & AF_DISABLE_IPV4))
   {
      listen(hSocket, SOMAXCONN);
	   nxlog_write(MSG_LISTENING, EVENTLOG_INFORMATION_TYPE, "ad", ntohl(servAddr.sin_addr.s_addr), g_wListenPort);
   }
#ifdef WITH_IPV6
   if (!(g_dwFlags & AF_DISABLE_IPV6))
   {
      listen(hSocket6, SOMAXCONN);
	   nxlog_write(MSG_LISTENING, EVENTLOG_INFORMATION_TYPE, "Hd", servAddr6.sin6_addr.s6_addr, g_wListenPort);
   }
#endif

   // Wait for connection requests
   int errorCount = 0;
   while(!(g_dwFlags & AF_SHUTDOWN))
   {
      struct timeval tv;
      tv.tv_sec = 1;
      tv.tv_usec = 0;

      fd_set rdfs;
      FD_ZERO(&rdfs);
      if (hSocket != INVALID_SOCKET)
         FD_SET(hSocket, &rdfs);
#ifdef WITH_IPV6
      if (hSocket6 != INVALID_SOCKET)
         FD_SET(hSocket6, &rdfs);
#endif

#if defined(WITH_IPV6) && !defined(_WIN32)
      SOCKET nfds = 0;
      if (hSocket != INVALID_SOCKET)
         nfds = hSocket;
      if ((hSocket6 != INVALID_SOCKET) && (hSocket6 > nfds))
         nfds = hSocket6;
      int nRet = select(SELECT_NFDS(nfds + 1), &rdfs, NULL, NULL, &tv);
#else
      int nRet = select(SELECT_NFDS(hSocket + 1), &rdfs, NULL, NULL, &tv);
#endif
      if ((nRet > 0) && (!(g_dwFlags & AF_SHUTDOWN)))
      {
         char clientAddr[128];
         socklen_t size = 128;
#ifdef WITH_IPV6
         SOCKET hClientSocket = accept(FD_ISSET(hSocket, &rdfs) ? hSocket : hSocket6, (struct sockaddr *)clientAddr, &size);
#else
         SOCKET hClientSocket = accept(hSocket, (struct sockaddr *)clientAddr, &size);
#endif
         if (hClientSocket == INVALID_SOCKET)
         {
            int error = WSAGetLastError();

            if (error != WSAEINTR)
               nxlog_write(MSG_ACCEPT_ERROR, EVENTLOG_ERROR_TYPE, "e", error);
            errorCount++;
            g_dwAcceptErrors++;
            if (errorCount > 1000)
            {
               nxlog_write(MSG_TOO_MANY_ERRORS, EVENTLOG_WARNING_TYPE, NULL);
               errorCount = 0;
            }
            ThreadSleepMs(500);
            continue;
         }

         // Socket should be closed on successful exec
#ifndef _WIN32
         fcntl(hClientSocket, F_SETFD, fcntl(hClientSocket, F_GETFD) | FD_CLOEXEC);
#endif

         errorCount = 0;     // Reset consecutive errors counter
         InetAddress addr = InetAddress::createFromSockaddr((struct sockaddr *)clientAddr);
         DebugPrintf(INVALID_INDEX, 5, _T("Incoming connection from %s"), addr.toString(buffer));

         bool masterServer, controlServer;
         if (IsValidServerAddress(addr, &masterServer, &controlServer))
         {
            g_dwAcceptedConnections++;
            DebugPrintf(INVALID_INDEX, 5, _T("Connection from %s accepted"), buffer);

            // Create new session structure and threads
            CommSession *session = new CommSession(hClientSocket, addr, masterServer, controlServer);
			
            if (!RegisterSession(session))
            {
               delete session;
            }
            else
            {
               session->run();
            }
         }
         else     // Unauthorized connection
         {
            g_dwRejectedConnections++;
            shutdown(hClientSocket, SHUT_RDWR);
            closesocket(hClientSocket);
            DebugPrintf(INVALID_INDEX, 5, _T("Connection from %s rejected"), buffer);
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

/**
 * Session watchdog thread
 */
THREAD_RESULT THREAD_CALL SessionWatchdog(void *)
{
   m_mutexWatchdogActive = MutexCreate();
   MutexLock(m_mutexWatchdogActive);

   ThreadSleep(5);
   while(!(g_dwFlags & AF_SHUTDOWN))
   {
      MutexLock(g_hSessionListAccess);
      time_t now = time(NULL);
      for(UINT32 i = 0; i < g_dwMaxSessions; i++)
         if (g_pSessionList[i] != NULL)
         {
            if (g_pSessionList[i]->getTimeStamp() < (now - (time_t)g_dwIdleTimeout))
				{
					DebugPrintf(i, 5, _T("Session disconnected by watchdog (last activity timestamp is ") UINT64_FMT _T(")"), (UINT64)g_pSessionList[i]->getTimeStamp());
               g_pSessionList[i]->disconnect();
				}
         }
      MutexUnlock(g_hSessionListAccess);
      ThreadSleep(5);
   }

   // Disconnect all sessions
   MutexLock(g_hSessionListAccess);
   for(UINT32 i = 0; i < g_dwMaxSessions; i++)
      if (g_pSessionList[i] != NULL)
         g_pSessionList[i]->disconnect();
   MutexUnlock(g_hSessionListAccess);

   ThreadSleep(1);
   MutexUnlock(m_mutexWatchdogActive);
   DebugPrintf(INVALID_INDEX, 1, _T("Session Watchdog thread terminated"));

   return THREAD_OK;
}

/**
 * Handler for Agent.ActiveConnections parameter
 */
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
