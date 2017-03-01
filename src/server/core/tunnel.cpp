/*
** NetXMS - Network Management System
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
** File: tunnel.cpp
**/

#include "nxcore.h"

/**
 * Tunnel listener
 */
THREAD_RESULT THREAD_CALL TunnelListener(void *arg)
{
   UINT16 port = (UINT16)ConfigReadULong(_T("AgentTunnelListenPort"), 4703);

   // Create socket(s)
   SOCKET hSocket = socket(AF_INET, SOCK_STREAM, 0);
#ifdef WITH_IPV6
   SOCKET hSocket6 = socket(AF_INET6, SOCK_STREAM, 0);
#endif
   if ((hSocket == INVALID_SOCKET)
#ifdef WITH_IPV6
       && (hSocket6 == INVALID_SOCKET)
#endif
      )
   {
      nxlog_write(MSG_SOCKET_FAILED, EVENTLOG_ERROR_TYPE, "s", _T("TunnelListener"));
      return THREAD_OK;
   }

   SetSocketExclusiveAddrUse(hSocket);
   SetSocketReuseFlag(hSocket);
#ifndef _WIN32
   fcntl(hSocket, F_SETFD, fcntl(hSocket, F_GETFD) | FD_CLOEXEC);
#endif

#ifdef WITH_IPV6
   SetSocketExclusiveAddrUse(hSocket6);
   SetSocketReuseFlag(hSocket6);
#ifndef _WIN32
   fcntl(hSocket6, F_SETFD, fcntl(hSocket6, F_GETFD) | FD_CLOEXEC);
#endif
#ifdef IPV6_V6ONLY
   int on = 1;
   setsockopt(hSocket6, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&on, sizeof(int));
#endif
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
   servAddr.sin_port = htons(port);
#ifdef WITH_IPV6
   servAddr6.sin6_port = htons(port);
#endif

   // Bind socket
   TCHAR buffer[64];
   int bindFailures = 0;
   nxlog_debug(1, _T("Trying to bind on %s:%d"), SockaddrToStr((struct sockaddr *)&servAddr, buffer), ntohs(servAddr.sin_port));
   if (bind(hSocket, (struct sockaddr *)&servAddr, sizeof(struct sockaddr_in)) != 0)
   {
      nxlog_write(MSG_BIND_ERROR, EVENTLOG_ERROR_TYPE, "e", WSAGetLastError());
      bindFailures++;
   }

#ifdef WITH_IPV6
   nxlog_debug(1, _T("Trying to bind on [%s]:%d"), SockaddrToStr((struct sockaddr *)&servAddr6, buffer), ntohs(servAddr6.sin6_port));
   if (bind(hSocket6, (struct sockaddr *)&servAddr6, sizeof(struct sockaddr_in6)) != 0)
   {
      nxlog_write(MSG_BIND_ERROR, EVENTLOG_ERROR_TYPE, "dse", port, _T("TunnelListener"), WSAGetLastError());
      bindFailures++;
   }
#else
   bindFailures++;
#endif

   // Abort if cannot bind to socket
   if (bindFailures == 2)
   {
      return THREAD_OK;
   }

   // Set up queue
   if (listen(hSocket, SOMAXCONN) == 0)
   {
      nxlog_write(MSG_LISTENING_FOR_AGENTS, NXLOG_INFO, "ad", ntohl(servAddr.sin_addr.s_addr), port);
   }
   else
   {
      closesocket(hSocket);
      hSocket = INVALID_SOCKET;
   }
#ifdef WITH_IPV6
   if (listen(hSocket6, SOMAXCONN) == 0)
   {
      nxlog_write(MSG_LISTENING_FOR_AGENTS, NXLOG_INFO, "Hd", servAddr6.sin6_addr.s6_addr, port);
   }
   else
   {
      closesocket(hSocket6);
      hSocket6 = INVALID_SOCKET;
   }
#endif

   // Wait for connection requests
   SocketPoller sp;
   int errorCount = 0;
   while(!(g_flags & AF_SHUTDOWN))
   {
      sp.reset();
      if (hSocket != INVALID_SOCKET)
         sp.add(hSocket);
#ifdef WITH_IPV6
      if (hSocket6 != INVALID_SOCKET)
         sp.add(hSocket6);
#endif

      int nRet = sp.poll(1000);
      if ((nRet > 0) && (!(g_flags & AF_SHUTDOWN)))
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
               nxlog_write(MSG_ACCEPT_ERROR, NXLOG_ERROR, "e", error);
            errorCount++;
            if (errorCount > 1000)
            {
               nxlog_write(MSG_TOO_MANY_ACCEPT_ERRORS, NXLOG_WARNING, NULL);
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
         nxlog_debug(5, _T("TunnelListener: incoming connection from %s"), addr.toString(buffer));

         shutdown(hClientSocket, SHUT_RDWR);
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
            ThreadSleepMs(100);
         }
      }
   }

   closesocket(hSocket);
#ifdef WITH_IPV6
   closesocket(hSocket6);
#endif
   nxlog_debug(1, _T("Tunnel listener thread terminated"));
   return THREAD_OK;
}

/**
 * Close all active agent tunnels
 */
void CloseAgentTunnels()
{
}
