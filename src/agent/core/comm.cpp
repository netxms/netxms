/* 
** NetXMS multiplatform core agent
** Copyright (C) 2003-2017 Victor Kirhenshtein
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

/**
 * Statistics
 */
UINT32 g_acceptErrors = 0;
UINT32 g_acceptedConnections = 0;
UINT32 g_rejectedConnections = 0;

/**
 * Session list
 */
CommSession **g_pSessionList = NULL;
MUTEX g_hSessionListAccess;

/**
 * Static data
 */
static MUTEX m_mutexWatchdogActive = INVALID_MUTEX_HANDLE;
static VolatileCounter s_messageId = (INT32)time(NULL);

/**
 * Generate new message ID
 */
UINT32 GenerateMessageId()
{
   return (UINT32)InterlockedIncrement(&s_messageId);
}

/**
 * Initialize session list
 */
void InitSessionList()
{
   if (g_dwMaxSessions == 0)  // default value
   {
      g_dwMaxSessions = (g_dwFlags & (AF_ENABLE_PROXY | AF_ENABLE_SNMP_PROXY)) ? 1024 : 32;
   }
   else
   {
      g_dwMaxSessions = MIN(MAX(g_dwMaxSessions, 2), 4096);
   }
   nxlog_debug(2, _T("Maximum number of sessions set to %d"), g_dwMaxSessions);
	g_pSessionList = (CommSession **)malloc(sizeof(CommSession *) * g_dwMaxSessions);
	memset(g_pSessionList, 0, sizeof(CommSession *) * g_dwMaxSessions);
	g_hSessionListAccess = MutexCreate();
}

/**
 * Destroy session list
 */
void DestroySessionList()
{
   MutexDestroy(g_hSessionListAccess);
   free(g_pSessionList);
}

/**
 * Validates server's address
 */
bool IsValidServerAddress(const InetAddress &addr, bool *pbMasterServer, bool *pbControlServer)
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
bool RegisterSession(CommSession *session)
{
   MutexLock(g_hSessionListAccess);
   for(UINT32 i = 0; i < g_dwMaxSessions; i++)
      if (g_pSessionList[i] == NULL)
      {
         g_pSessionList[i] = session;
         session->setIndex(i);
         MutexUnlock(g_hSessionListAccess);
         return true;
      }

   MutexUnlock(g_hSessionListAccess);
   nxlog_write(MSG_TOO_MANY_SESSIONS, EVENTLOG_WARNING_TYPE, NULL);
   return false;
}

/**
 * Unregister session
 */
void UnregisterSession(UINT32 index, UINT32 id)
{
   MutexLock(g_hSessionListAccess);
   if ((g_pSessionList[index] != NULL) && (g_pSessionList[index]->getId() == id))
   {
      g_pSessionList[index]->debugPrintf(4, _T("Session unregistered"));
      g_pSessionList[index] = NULL;
   }
   MutexUnlock(g_hSessionListAccess);
}

/**
 * Enumerates active agent sessions. Callback will be called for each valid session.
 * Callback must return _STOP to stop enumeration or _CONTINUE to continue.
 *
 * @return true if enumeration was stopped by callback
 */
bool EnumerateSessions(EnumerationCallbackResult (* callback)(AbstractCommSession *, void *), void *data)
{
   bool result = false;
   MutexLock(g_hSessionListAccess);
   for(UINT32 i = 0; i < g_dwMaxSessions; i++)
   {
      if (g_pSessionList[i] == NULL)
         continue;

      if (callback(g_pSessionList[i], data) == _STOP)
      {
         result = true;
         break;
      }
   }
   MutexUnlock(g_hSessionListAccess);
   return result;
}

/**
 * Find server session. Caller must call decRefCount() for session object when finished.
 */
AbstractCommSession *FindServerSession(UINT64 serverId)
{
   AbstractCommSession *session = NULL;
   MutexLock(g_hSessionListAccess);
   for(UINT32 i = 0; i < g_dwMaxSessions; i++)
   {
      if ((g_pSessionList[i] != NULL) && (g_pSessionList[i]->getServerId() == serverId))
      {
         session = g_pSessionList[i];
         session->incRefCount();
         break;
      }
   }
   MutexUnlock(g_hSessionListAccess);
   return session;
}

/**
 * Find server session using comparator callback. Caller must call decRefCount() for session object when finished.
 */
AbstractCommSession *FindServerSession(bool (*comparator)(AbstractCommSession *, void *), void *userData)
{
   AbstractCommSession *session = NULL;
   MutexLock(g_hSessionListAccess);
   for(UINT32 i = 0; i < g_dwMaxSessions; i++)
   {
      if ((g_pSessionList[i] != NULL) && (comparator(g_pSessionList[i], userData)))
      {
         session = g_pSessionList[i];
         session->incRefCount();
         break;
      }
   }
   MutexUnlock(g_hSessionListAccess);
   return session;
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
#ifdef IPV6_V6ONLY
      int on = 1;
      setsockopt(hSocket6, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&on, sizeof(int));
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
		   servAddr.sin_addr.s_addr = htonl(bindAddress.getAddressV4());
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
	   DebugPrintf(1, _T("Trying to bind on %s:%d"), SockaddrToStr((struct sockaddr *)&servAddr, buffer), ntohs(servAddr.sin_port));
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
      DebugPrintf(1, _T("Trying to bind on [%s]:%d"), SockaddrToStr((struct sockaddr *)&servAddr6, buffer), ntohs(servAddr6.sin6_port));
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
      if (listen(hSocket, SOMAXCONN) == 0)
		{
	   	nxlog_write(MSG_LISTENING, EVENTLOG_INFORMATION_TYPE, "ad", ntohl(servAddr.sin_addr.s_addr), g_wListenPort);
		}
		else
		{
			closesocket(hSocket);
			hSocket = INVALID_SOCKET;
		}
   }
#ifdef WITH_IPV6
   if (!(g_dwFlags & AF_DISABLE_IPV6))
   {
      if (listen(hSocket6, SOMAXCONN) == 0)
		{
	   	nxlog_write(MSG_LISTENING, EVENTLOG_INFORMATION_TYPE, "Hd", servAddr6.sin6_addr.s6_addr, g_wListenPort);
		}
		else
		{
			closesocket(hSocket6);
			hSocket6 = INVALID_SOCKET;
		}
   }
#endif

   // Wait for connection requests
   SocketPoller sp;
   int errorCount = 0;
   while(!(g_dwFlags & AF_SHUTDOWN))
   {
      sp.reset();
      if (hSocket != INVALID_SOCKET)
         sp.add(hSocket);
#ifdef WITH_IPV6
      if (hSocket6 != INVALID_SOCKET)
         sp.add(hSocket6);
#endif

      int nRet = sp.poll(1000);
      if ((nRet > 0) && (!(g_dwFlags & AF_SHUTDOWN)))
      {
         char clientAddr[128];
         socklen_t size = 128;
#ifdef WITH_IPV6
         SOCKET hClientSocket = accept(sp.isSet(hSocket) ? hSocket : hSocket6, (struct sockaddr *)clientAddr, &size);
#else
         SOCKET hClientSocket = accept(hSocket, (struct sockaddr *)clientAddr, &size);
#endif
         if (hClientSocket == INVALID_SOCKET)
         {
            int error = WSAGetLastError();

            if (error != WSAEINTR)
               nxlog_write(MSG_ACCEPT_ERROR, EVENTLOG_ERROR_TYPE, "e", error);
            errorCount++;
            g_acceptErrors++;
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
         DebugPrintf(5, _T("Incoming connection from %s"), addr.toString(buffer));

         bool masterServer, controlServer;
         if (IsValidServerAddress(addr, &masterServer, &controlServer))
         {
            g_acceptedConnections++;
            DebugPrintf(5, _T("Connection from %s accepted"), buffer);

            // Create new session structure and threads
            SocketCommChannel *channel = new SocketCommChannel(hClientSocket);
            CommSession *session = new CommSession(channel, addr, masterServer, controlServer);
            channel->decRefCount();
			
            if (!RegisterSession(session))
            {
               delete session;
            }
            else
            {
               DebugPrintf(9, _T("Session registered for %s"), buffer);
               session->run();
            }
         }
         else     // Unauthorized connection
         {
            g_rejectedConnections++;
            shutdown(hClientSocket, SHUT_RDWR);
            closesocket(hClientSocket);
            DebugPrintf(5, _T("Connection from %s rejected"), buffer);
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

   closesocket(hSocket);
#ifdef WITH_IPV6
   closesocket(hSocket6);
#endif
   DebugPrintf(1, _T("Listener thread terminated"));
   return THREAD_OK;
}

/**
 * Session watchdog thread
 */
THREAD_RESULT THREAD_CALL SessionWatchdog(void *)
{
   m_mutexWatchdogActive = MutexCreate();
   MutexLock(m_mutexWatchdogActive);

   while(!AgentSleepAndCheckForShutdown(5000))
   {
      MutexLock(g_hSessionListAccess);
      time_t now = time(NULL);
      for(UINT32 i = 0; i < g_dwMaxSessions; i++)
         if (g_pSessionList[i] != NULL)
         {
            if (g_pSessionList[i]->getTimeStamp() < (now - (time_t)g_dwIdleTimeout))
				{
               g_pSessionList[i]->debugPrintf(4, _T("Session disconnected by watchdog (last activity timestamp is ") UINT64_FMT _T(")"), (UINT64)g_pSessionList[i]->getTimeStamp());
               g_pSessionList[i]->disconnect();
               g_pSessionList[i] = NULL;
				}
         }
      MutexUnlock(g_hSessionListAccess);
   }

   // Disconnect all sessions
   MutexLock(g_hSessionListAccess);
   for(UINT32 i = 0; i < g_dwMaxSessions; i++)
      if (g_pSessionList[i] != NULL)
         g_pSessionList[i]->disconnect();
   MutexUnlock(g_hSessionListAccess);

   ThreadSleep(1);
   MutexUnlock(m_mutexWatchdogActive);
   DebugPrintf(1, _T("Session Watchdog thread terminated"));

   return THREAD_OK;
}

/**
 * Handler for Agent.ActiveConnections parameter
 */
LONG H_ActiveConnections(const TCHAR *pszCmd, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
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
