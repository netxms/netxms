/* 
** NetXMS multiplatform core agent
** Copyright (C) 2003-2024 Victor Kirhenshtein
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
uint32_t g_acceptErrors = 0;
uint32_t g_acceptedConnections = 0;
uint32_t g_rejectedConnections = 0;

/**
 * Session list
 */
SharedObjectArray<CommSession> g_sessions;
Mutex g_sessionLock(MutexType::FAST);

/**
 * Static data
 */
static Mutex s_mutexWatchdogActive;
static VolatileCounter s_messageId = static_cast<int32_t>(time(NULL));

/**
 * Generate new message ID
 */
uint32_t GenerateMessageId()
{
   return static_cast<uint32_t>(InterlockedIncrement(&s_messageId));
}

/**
 * Initialize session list
 */
void InitSessionList()
{
   if (g_maxCommSessions == 0)  // default value
   {
      g_maxCommSessions = (g_dwFlags & (AF_ENABLE_PROXY | AF_ENABLE_SNMP_PROXY)) ? 1024 : 32;
   }
   else
   {
      g_maxCommSessions = MIN(MAX(g_maxCommSessions, 2), 4096);
   }
   nxlog_debug_tag(DEBUG_TAG_COMM, 2, _T("Maximum number of sessions set to %d"), g_maxCommSessions);
}

/**
 * Validates server's address
 */
bool IsValidServerAddress(const InetAddress &addr, bool *pbMasterServer, bool *pbControlServer, bool forceResolve)
{
   for(int i = 0; i < g_serverList.size(); i++)
	{
      ServerInfo *s = g_serverList.get(i);
      if (s->match(addr, forceResolve))
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
bool RegisterSession(const shared_ptr<CommSession>& session)
{
   g_sessionLock.lock();
   if (g_sessions.size() < static_cast<int>(g_maxCommSessions))
   {
      g_sessions.add(session);
      g_sessionLock.unlock();
      session->debugPrintf(4, _T("Session registered (control=%s, master=%s)"), BooleanToString(session->isControlServer()), BooleanToString(session->isMasterServer()));
      return true;
   }

   g_sessionLock.unlock();
   nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG_COMM, _T("Too many communication sessions open - unable to accept new connection"));
   return false;
}

/**
 * Unregister session
 */
void UnregisterSession(uint32_t id)
{
   g_sessionLock.lock();
   for(int i = 0; i < g_sessions.size(); i++)
      if (g_sessions.get(i)->getId() == id)
      {
         g_sessions.get(i)->debugPrintf(4, _T("Session unregistered"));
         g_sessions.remove(i);
      }
   g_sessionLock.unlock();
}

/**
 * Enumerates active agent sessions. Callback will be called for each valid session.
 * Callback must return _STOP to stop enumeration or _CONTINUE to continue.
 *
 * @return true if enumeration was stopped by callback
 */
bool EnumerateSessions(EnumerationCallbackResult (*callback)(AbstractCommSession *, void *), void *data)
{
   bool result = false;
   g_sessionLock.lock();
   for(int i = 0; i < g_sessions.size(); i++)
   {
      if (callback(g_sessions.get(i), data) == _STOP)
      {
         result = true;
         break;
      }
   }
   g_sessionLock.unlock();
   return result;
}

/**
 * Find server session by server ID. Caller must call decRefCount() for session object when finished.
 */
shared_ptr<AbstractCommSession> FindServerSessionByServerId(uint64_t serverId)
{
   shared_ptr<AbstractCommSession> session;
   g_sessionLock.lock();
   for(int i = 0; i < g_sessions.size(); i++)
   {
      if (g_sessions.get(i)->getServerId() == serverId)
      {
         session = g_sessions.getShared(i);
         break;
      }
   }
   g_sessionLock.unlock();
   return session;
}

/**
 * Find server session by session ID. Caller must call decRefCount() for session object when finished.
 */
shared_ptr<AbstractCommSession> FindServerSessionById(uint32_t id)
{
   shared_ptr<AbstractCommSession> session;
   g_sessionLock.lock();
   for(int i = 0; i < g_sessions.size(); i++)
   {
      if (g_sessions.get(i)->getId() == id)
      {
         session = g_sessions.getShared(i);
         break;
      }
   }
   g_sessionLock.unlock();
   return session;
}

/**
 * Find server session using comparator callback. Caller must call decRefCount() for session object when finished.
 */
shared_ptr<AbstractCommSession> FindServerSession(bool (*comparator)(AbstractCommSession *, void *), void *userData)
{
   shared_ptr<AbstractCommSession> session;
   g_sessionLock.lock();
   for(int i = 0; i < g_sessions.size(); i++)
   {
      if (comparator(g_sessions.get(i), userData))
      {
         session = g_sessions.getShared(i);
         break;
      }
   }
   g_sessionLock.unlock();
   return session;
}

/**
 * Send notification message to all connected servers
 */
void NotifyConnectedServers(const TCHAR *notificationCode)
{
   NXCPMessage msg(CMD_NOTIFY, 0);
   msg.setField(VID_NOTIFICATION_CODE, notificationCode);

   g_sessionLock.lock();
   for(int j = 0; j < g_sessions.size(); j++)
   {
      CommSession *session = g_sessions.get(j);
      if (session->canAcceptTraps())
         session->sendMessage(msg);
   }
   g_sessionLock.unlock();
}

/**
 * TCP/IP Listener
 */ 
void ListenerThread()
{
   // Create socket(s)
   SOCKET hSocket = (g_dwFlags & AF_DISABLE_IPV4) ? INVALID_SOCKET : CreateSocket(AF_INET, SOCK_STREAM, 0);
#ifdef WITH_IPV6
   SOCKET hSocket6 = (g_dwFlags & AF_DISABLE_IPV6) ? INVALID_SOCKET : CreateSocket(AF_INET6, SOCK_STREAM, 0);
#endif
   if (((hSocket == INVALID_SOCKET) && !(g_dwFlags & AF_DISABLE_IPV4))
#ifdef WITH_IPV6
       && ((hSocket6 == INVALID_SOCKET) && !(g_dwFlags & AF_DISABLE_IPV6))
#endif
      )
   {
      TCHAR buffer[1024];
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_COMM, _T("Unable to create socket (%s)"), GetLastSocketErrorText(buffer, 1024));
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
	   nxlog_debug_tag(DEBUG_TAG_COMM, 1, _T("Trying to bind on %s:%d"), SockaddrToStr((struct sockaddr *)&servAddr, buffer), ntohs(servAddr.sin_port));
      if (bind(hSocket, (struct sockaddr *)&servAddr, sizeof(struct sockaddr_in)) != 0)
      {
         TCHAR buffer[1024];
         nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_COMM, _T("Unable to bind IPv4 socket (%s)"), GetLastSocketErrorText(buffer, 1024));
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
         TCHAR buffer[1024];
         nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_COMM, _T("Unable to bind IPv6 socket (%s)"), GetLastSocketErrorText(buffer, 1024));
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
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_COMM, _T("Cannot bind at least one socket, terminating agent process"));
#ifdef _WIN32
      ExitProcess(99);
#else
      _exit(99);
#endif
   }

   // Set up queue
   if (!(g_dwFlags & AF_DISABLE_IPV4))
   {
      if (listen(hSocket, SOMAXCONN) == 0)
		{
         TCHAR ipAddrText[64];
         nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_COMM, _T("Listening on socket %s:%u"), InetAddress(ntohl(servAddr.sin_addr.s_addr)).toString(ipAddrText), g_wListenPort);
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
         TCHAR ipAddrText[64];
         nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_COMM, _T("Listening on socket %s:%u"), InetAddress(servAddr6.sin6_addr.s6_addr).toString(ipAddrText), g_wListenPort);
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
            {
               TCHAR buffer[1024];
               nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_COMM, _T("Unable to accept incoming connection (%s)"), GetLastSocketErrorText(buffer, 1024));
            }
            errorCount++;
            g_acceptErrors++;
            if (errorCount > 1000)
            {
               nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG_COMM, _T("Too many consecutive errors on accept() call"));
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
         nxlog_debug_tag(DEBUG_TAG_COMM, 5, _T("Incoming connection from %s"), addr.toString(buffer));

         bool masterServer, controlServer;
         if (IsValidServerAddress(addr, &masterServer, &controlServer, false))
         {
            g_acceptedConnections++;
            nxlog_debug_tag(DEBUG_TAG_COMM, 5, _T("Connection from %s accepted"), buffer);

            // Create new session structure and threads
            shared_ptr<SocketCommChannel> channel = make_shared<SocketCommChannel>(hClientSocket, nullptr, Ownership::True);
            shared_ptr<CommSession> session = MakeSharedCommSession<CommSession>(channel, addr, masterServer, controlServer);

            if (RegisterSession(session))
            {
               nxlog_debug_tag(DEBUG_TAG_COMM, 9, _T("Session registered for %s"), buffer);
               session->run();
            }
         }
         else     // Unauthorized connection
         {
            g_rejectedConnections++;
            shutdown(hClientSocket, SHUT_RDWR);
            closesocket(hClientSocket);
            nxlog_debug_tag(DEBUG_TAG_COMM, 5, _T("Connection from %s rejected"), buffer);
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
            TCHAR buffer[1024];
            nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_COMM, _T("Call to select() failed (%s)"), GetLastSocketErrorText(buffer, 1024));
            ThreadSleepMs(100);
         }
      }
   }

   // Wait for watchdog thread
   s_mutexWatchdogActive.lock();
   s_mutexWatchdogActive.unlock();

   closesocket(hSocket);
#ifdef WITH_IPV6
   closesocket(hSocket6);
#endif
   nxlog_debug_tag(DEBUG_TAG_COMM, 1, _T("Listener thread terminated"));
}

/**
 * Session watchdog thread
 */
void SessionWatchdog()
{
   s_mutexWatchdogActive.lock();

   while(!AgentSleepAndCheckForShutdown(5000))
   {
      g_sessionLock.lock();
      int64_t now = GetMonotonicClockTime();
      int64_t timeout = static_cast<int64_t>(g_dwIdleTimeout) * _LL(1000);
      for(int i = 0; i < g_sessions.size(); i++)
      {
         CommSession *session = g_sessions.get(i);
         if (session->getTimeStamp() + timeout < now)
         {
            session->debugPrintf(4, _T("Session disconnected by watchdog (timestamp = ") INT64_FMT _T(", now = ") INT64_FMT _T(")"), session->getTimeStamp(), now);
            session->disconnect();
            g_sessions.remove(i);
            i--;
         }
      }
      g_sessionLock.unlock();
   }

   // Disconnect all sessions
   g_sessionLock.lock();
   for(int i = 0; i < g_sessions.size(); i++)
      g_sessions.get(i)->disconnect();
   g_sessionLock.unlock();

   ThreadSleep(1);
   s_mutexWatchdogActive.unlock();
   nxlog_debug_tag(DEBUG_TAG_COMM, 1, _T("Session Watchdog thread terminated"));
}

/**
 * Handler for Agent.ActiveConnections parameter
 */
LONG H_ActiveConnections(const TCHAR *pszCmd, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
   g_sessionLock.lock();
   ret_int(pValue, g_sessions.size());
   g_sessionLock.unlock();
   return SYSINFO_RC_SUCCESS;
}
