/*
** NetXMS - Network Management System
** Utility Library
** Copyright (C) 2003-2017 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: socket_listener.cpp
**/

#include "libnetxms.h"
#include <socket_listener.h>

/**
 * Constructor
 */
GenericSocketListener::GenericSocketListener(int type, UINT16 port, bool allowV4, bool allowV6)
{
   m_listenAddress = NULL;
   m_port = port;
   m_allowV4 = allowV4;
   m_allowV6 = allowV6;
   m_socketV4 = INVALID_SOCKET;
   m_socketV6 = INVALID_SOCKET;
   m_stop = false;
   m_acceptErrors = 0;
   m_acceptedConnections = 0;
   m_rejectedConnections = 0;
   m_type = type;
}

/**
 * Destructor
 */
GenericSocketListener::~GenericSocketListener()
{
   shutdown();
   if (m_socketV4 != INVALID_SOCKET)
   {
      closesocket(m_socketV4);
      m_socketV4 = INVALID_SOCKET;
   }
   if (m_socketV6 != INVALID_SOCKET)
   {
      closesocket(m_socketV6);
      m_socketV6 = INVALID_SOCKET;
   }
   free(m_listenAddress);
}

/**
 * Initialize listener
 */
bool GenericSocketListener::initialize()
{
   // Create socket(s)
   m_socketV4 = m_allowV4 ? socket(AF_INET, m_type, 0) : INVALID_SOCKET;
#ifdef WITH_IPV6
   m_socketV6 = m_allowV6 ? socket(AF_INET6, m_type, 0) : INVALID_SOCKET;
#endif
   if (((m_socketV4 == INVALID_SOCKET) && m_allowV4)
#ifdef WITH_IPV6
       && ((m_socketV6 == INVALID_SOCKET) && m_allowV6)
#endif
      )
   {
      TCHAR buffer[256];
      nxlog_write_generic(NXLOG_ERROR, _T("SocketListener/%s: socket() call failed (%s)"), m_name, GetLastSocketErrorText(buffer, 256));
      exit(1);
   }

   if (m_allowV4)
   {
      SetSocketExclusiveAddrUse(m_socketV4);
      SetSocketReuseFlag(m_socketV4);
#ifndef _WIN32
      fcntl(m_socketV4, F_SETFD, fcntl(m_socketV4, F_GETFD) | FD_CLOEXEC);
#endif
   }

#ifdef WITH_IPV6
   if (m_allowV6)
   {
      SetSocketExclusiveAddrUse(m_socketV6);
      SetSocketReuseFlag(m_socketV6);
#ifndef _WIN32
      fcntl(m_socketV6, F_SETFD, fcntl(m_socketV6, F_GETFD) | FD_CLOEXEC);
#endif
#ifdef IPV6_V6ONLY
      int on = 1;
      setsockopt(m_socketV6, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&on, sizeof(int));
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

   if ((m_listenAddress == NULL) || (*m_listenAddress == 0) || !_tcscmp(m_listenAddress, _T("*")))
   {
      servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
#ifdef WITH_IPV6
      memset(servAddr6.sin6_addr.s6_addr, 0, 16);
#endif
   }
   else
   {
      InetAddress bindAddress = InetAddress::resolveHostName(m_listenAddress, AF_INET);
      if (bindAddress.isValid() && (bindAddress.getFamily() == AF_INET))
      {
         servAddr.sin_addr.s_addr = htonl(bindAddress.getAddressV4());
      }
      else
      {
         servAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
      }
#ifdef WITH_IPV6
      bindAddress = InetAddress::resolveHostName(m_listenAddress, AF_INET6);
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
   servAddr.sin_port = htons(m_port);
#ifdef WITH_IPV6
   servAddr6.sin6_port = htons(m_port);
#endif

   // Bind socket
   TCHAR buffer[64];
   int bindFailures = 0;
   if (m_allowV4)
   {
      nxlog_debug(1, _T("SocketListener/%s: Trying to bind on %s:%d/%s"), m_name,
               SockaddrToStr((struct sockaddr *)&servAddr, buffer), ntohs(servAddr.sin_port),
               (m_type == SOCK_STREAM) ? _T("tcp") : _T("udp"));
      if (bind(m_socketV4, (struct sockaddr *)&servAddr, sizeof(struct sockaddr_in)) != 0)
      {
         nxlog_write_generic(NXLOG_ERROR, _T("SocketListener/%s: cannot bind IPv4 socket (%s)"), m_name, GetLastSocketErrorText(buffer, 256));
         bindFailures++;
      }
   }
   else
   {
      bindFailures++;
   }

#ifdef WITH_IPV6
   if (m_allowV6)
   {
      nxlog_debug(1, _T("SocketListener/%s: Trying to bind on [%s]:%d/%s"), m_name,
               SockaddrToStr((struct sockaddr *)&servAddr6, buffer), ntohs(servAddr6.sin6_port),
               (m_type == SOCK_STREAM) ? _T("tcp") : _T("udp"));
      if (bind(m_socketV6, (struct sockaddr *)&servAddr6, sizeof(struct sockaddr_in6)) != 0)
      {
         nxlog_write_generic(NXLOG_ERROR, _T("SocketListener/%s: cannot bind IPv6 socket (%s)"), m_name, GetLastSocketErrorText(buffer, 256));
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

   if (bindFailures == 2)
      return false;

   // Set up queue
   if (m_type == SOCK_STREAM)
   {
      if (m_allowV4)
      {
         if (listen(m_socketV4, SOMAXCONN) == 0)
         {
            nxlog_write_generic(NXLOG_INFO, _T("SocketListener/%s: listening on %s:%d"), m_name, SockaddrToStr((struct sockaddr *)&servAddr, buffer), (int)m_port);
         }
         else
         {
            closesocket(m_socketV4);
            m_socketV4 = INVALID_SOCKET;
         }
      }
#ifdef WITH_IPV6
      if (m_allowV6)
      {
         if (listen(m_socketV6, SOMAXCONN) == 0)
         {
            nxlog_write_generic(NXLOG_INFO, _T("SocketListener/%s: listening on [%s]:%d"), m_name, SockaddrToStr((struct sockaddr *)&servAddr, buffer), (int)m_port);
         }
         else
         {
            closesocket(m_socketV6);
            m_socketV6 = INVALID_SOCKET;
         }
      }
#endif
   }

   return true;
}

/**
 * Shutdown listener
 */
void GenericSocketListener::shutdown()
{
   m_stop = true;
}

/**
 * Check if external stop condition is reached
 */
bool GenericSocketListener::isStopConditionReached()
{
   return false;
}

/**
 * Check if incoming connection is allowed
 */
bool StreamSocketListener::isConnectionAllowed(const InetAddress& peer)
{
   return true;
}

/**
 * Main socket listener loop
 */
void StreamSocketListener::mainLoop()
{
   SocketPoller sp;
   int errorCount = 0;
   while(!m_stop && !isStopConditionReached())
   {
      sp.reset();
      if (m_socketV4 != INVALID_SOCKET)
         sp.add(m_socketV4);
#ifdef WITH_IPV6
      if (m_socketV6 != INVALID_SOCKET)
         sp.add(m_socketV6);
#endif

      int nRet = sp.poll(1000);
      if ((nRet > 0) && !m_stop && !isStopConditionReached())
      {
         char clientAddr[128];
         socklen_t size = 128;
#ifdef WITH_IPV6
         SOCKET hClientSocket = accept(sp.isSet(m_socketV4) ? m_socketV4 : m_socketV6, (struct sockaddr *)clientAddr, &size);
#else
         SOCKET hClientSocket = accept(m_socketV4, (struct sockaddr *)clientAddr, &size);
#endif
         if (hClientSocket == INVALID_SOCKET)
         {
            int error = WSAGetLastError();
            if (error != WSAEINTR)
            {
               if (errorCount == 0)
               {
                  TCHAR buffer[256];
                  nxlog_write_generic(NXLOG_WARNING, _T("SocketListener/%s: accept() call failed (%s)"), m_name, GetLastSocketErrorText(buffer, 256));
               }
               errorCount++;
            }
            m_acceptErrors++;
            if (errorCount > 1000)
            {
               nxlog_write_generic(NXLOG_WARNING, _T("SocketListener/%s: multiple consecutive accept() errors"), m_name);
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
         TCHAR buffer[256];
         nxlog_debug(5, _T("SocketListener/%s: Incoming connection from %s"), m_name, addr.toString(buffer));

         if (isConnectionAllowed(addr))
         {
            m_acceptedConnections++;
            nxlog_debug(5, _T("SocketListener/%s: Connection from %s accepted"), m_name, buffer);
            if (processConnection(hClientSocket, addr) == CPR_COMPLETED)
            {
               ::shutdown(hClientSocket, SHUT_RDWR);
               closesocket(hClientSocket);
            }
         }
         else     // Unauthorized connection
         {
            m_rejectedConnections++;
            ::shutdown(hClientSocket, SHUT_RDWR);
            closesocket(hClientSocket);
            nxlog_debug(5, _T("SocketListener/%s: Connection from %s rejected"), m_name, buffer);
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
            TCHAR buffer[256];
            nxlog_write_generic(NXLOG_ERROR, _T("SocketListener/%s: select() call failed (%s)"), m_name, GetLastSocketErrorText(buffer, 256));
            ThreadSleepMs(100);
         }
      }
   }
}

/**
 * Main datagram listener loop
 */
void DatagramSocketListener::mainLoop()
{
   SocketPoller sp;
   int errorCount = 0;
   while(!m_stop && !isStopConditionReached())
   {
      sp.reset();
      if (m_socketV4 != INVALID_SOCKET)
         sp.add(m_socketV4);
#ifdef WITH_IPV6
      if (m_socketV6 != INVALID_SOCKET)
         sp.add(m_socketV6);
#endif

      int nRet = sp.poll(1000);
      if ((nRet > 0) && !m_stop && !isStopConditionReached())
      {
         processDatagram(sp.isSet(m_socketV4) ? m_socketV4 : m_socketV6);
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
            TCHAR buffer[256];
            nxlog_write_generic(NXLOG_ERROR, _T("SocketListener/%s: select() call failed (%s)"), m_name, GetLastSocketErrorText(buffer, 256));
            ThreadSleepMs(100);
         }
      }
   }
}
