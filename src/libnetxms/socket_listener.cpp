/*
** NetXMS - Network Management System
** Utility Library
** Copyright (C) 2003-2026 Victor Kirhenshtein
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

#define DEBUG_TAG _T("comm.listener")

/**
 * Constructor
 */
GenericSocketListener::GenericSocketListener(int type, uint16_t port, bool allowV4, bool allowV6)
{
   m_port = port;
   m_allowV4 = allowV4;
   m_allowV6 = allowV6;
   m_stop = false;
   m_name[0] = 0;
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
   for(int i = 0; i < m_sockets.size(); i++)
      closesocket(m_sockets.get(i));
}

/**
 * Set listen address(es). Accepts comma-separated list of IP addresses and host names.
 * Empty string or * means all available addresses.
 */
void GenericSocketListener::setListenAddress(const TCHAR *addrList)
{
   m_listenAddresses.clear();
   if (addrList == nullptr)
      return;

   String::split(addrList, _T(","), true,
      [this] (const String& e) -> void
      {
         if (!e.isEmpty())
            m_listenAddresses.add(e.cstr());
      });
}

/**
 * Create socket bound to given local address and add it to socket list
 */
void GenericSocketListener::createSocket(const InetAddress& addr)
{
   TCHAR addrText[64], errorText[256];
   if (addr.getFamily() == AF_INET6)
   {
      addrText[0] = _T('[');
      addr.toString(&addrText[1]);
      _tcscat(addrText, _T("]"));
   }
   else
   {
      addr.toString(addrText);
   }
   const TCHAR *protocol = (m_type == SOCK_STREAM) ? _T("tcp") : _T("udp");

   SOCKET s = CreateSocket(addr.getFamily(), m_type, 0);
   if (s == INVALID_SOCKET)
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("SocketListener/%s: cannot create socket for address %s (%s)"), m_name, addrText, GetLastSocketErrorText(errorText, 256));
      return;
   }

   SetSocketExclusiveAddrUse(s);
   SetSocketReuseFlag(s);
#ifndef _WIN32
   fcntl(s, F_SETFD, fcntl(s, F_GETFD) | FD_CLOEXEC);
#endif
#if defined(WITH_IPV6) && defined(IPV6_V6ONLY)
   if (addr.getFamily() == AF_INET6)
   {
      int on = 1;
      setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&on, sizeof(int));
   }
#endif

   SockAddrBuffer sa;
   addr.fillSockAddr(&sa, m_port);
   nxlog_debug_tag(DEBUG_TAG, 1, _T("SocketListener/%s: trying to bind on %s:%u/%s"), m_name, addrText, m_port, protocol);
   if (bind(s, reinterpret_cast<struct sockaddr*>(&sa), SA_LEN(reinterpret_cast<struct sockaddr*>(&sa))) != 0)
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("SocketListener/%s: cannot bind socket to %s:%u/%s (%s)"), m_name, addrText, m_port, protocol, GetLastSocketErrorText(errorText, 256));
      closesocket(s);
      return;
   }

   if ((m_type == SOCK_STREAM) && (listen(s, SOMAXCONN) != 0))
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("SocketListener/%s: listen() call failed for %s:%u/%s (%s)"), m_name, addrText, m_port, protocol, GetLastSocketErrorText(errorText, 256));
      closesocket(s);
      return;
   }

   m_sockets.add(s);
   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("SocketListener/%s: listening on %s:%u/%s"), m_name, addrText, m_port, protocol);
}

/**
 * Initialize listener. Addresses that cannot be resolved are logged and ignored. Returns false only
 * if binding was attempted and no socket could be bound; a listener left without any address to bind
 * initializes successfully and stays idle in mainLoop().
 */
bool GenericSocketListener::initialize()
{
#ifndef WITH_IPV6
   m_allowV6 = false;
#endif

   StructArray<InetAddress> addrList;
   bool wildcardV4 = false, wildcardV6 = false;
   auto addAddress = [this, &addrList, &wildcardV4, &wildcardV6] (const InetAddress& a) -> void
   {
      TCHAR buffer[64];
      if (((a.getFamily() == AF_INET) && !m_allowV4) || ((a.getFamily() == AF_INET6) && !m_allowV6))
      {
         nxlog_debug_tag(DEBUG_TAG, 3, _T("SocketListener/%s: address %s ignored because its address family is disabled"), m_name, a.toString(buffer));
         return;
      }

      // Wildcard address supersedes all specific addresses of the same family
      bool& wildcard = (a.getFamily() == AF_INET) ? wildcardV4 : wildcardV6;
      if (wildcard)
      {
         if (!a.isAnyLocal())
            nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("SocketListener/%s: address %s ignored because wildcard address of the same family is also configured"), m_name, a.toString(buffer));
         return;
      }
      if (a.isAnyLocal())
      {
         wildcard = true;
         for(int i = addrList.size() - 1; i >= 0; i--)
         {
            if (addrList.get(i)->getFamily() == a.getFamily())
            {
               nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("SocketListener/%s: address %s ignored because wildcard address of the same family is also configured"), m_name, addrList.get(i)->toString(buffer));
               addrList.remove(i);
            }
         }
         addrList.add(a);
         return;
      }

      for(int i = 0; i < addrList.size(); i++)
         if (addrList.get(i)->equals(a))
            return;
      addrList.add(a);
   };

   static const BYTE anyV6[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
   if (m_listenAddresses.isEmpty())
   {
      addAddress(InetAddress(static_cast<uint32_t>(INADDR_ANY)));
      addAddress(InetAddress(anyV6));
   }
   for(int i = 0; i < m_listenAddresses.size(); i++)
   {
      const TCHAR *name = m_listenAddresses.get(i);
      if (!_tcscmp(name, _T("*")))
      {
         addAddress(InetAddress(static_cast<uint32_t>(INADDR_ANY)));
         addAddress(InetAddress(anyV6));
         continue;
      }

      InetAddress a = InetAddress::parse(name);
      if (a.isValid())
      {
         addAddress(a);
         continue;
      }

      // Host name - use resolved address for each enabled address family
      bool resolved = false;
      if (m_allowV4)
      {
         a = InetAddress::resolveHostName(name, AF_INET);
         if (a.isValid() && (a.getFamily() == AF_INET))
         {
            addAddress(a);
            resolved = true;
         }
      }
      if (m_allowV6)
      {
         a = InetAddress::resolveHostName(name, AF_INET6);
         if (a.isValid() && (a.getFamily() == AF_INET6))
         {
            addAddress(a);
            resolved = true;
         }
      }
      if (!resolved)
         nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("SocketListener/%s: cannot resolve listen address \"%s\", ignored"), m_name, name);
   }

   if (addrList.isEmpty())
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("SocketListener/%s: no usable listen addresses configured, listener will be inactive"), m_name);
      return true;
   }

   for(int i = 0; i < addrList.size(); i++)
      createSocket(*addrList.get(i));

   if (m_sockets.isEmpty())
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("SocketListener/%s: cannot listen on any address"), m_name);
      return false;
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
 * Accept incoming connection on given listening socket. Returns false on accept() error.
 */
bool StreamSocketListener::acceptConnection(SOCKET s)
{
   SockAddrBuffer clientAddr;
   socklen_t size = sizeof(clientAddr);
   SOCKET hClientSocket = accept(s, reinterpret_cast<struct sockaddr*>(&clientAddr), &size);
   if (hClientSocket == INVALID_SOCKET)
   {
      m_acceptErrors++;
      return WSAGetLastError() == WSAEINTR;
   }

   // Socket should be closed on successful exec
#ifndef _WIN32
   fcntl(hClientSocket, F_SETFD, fcntl(hClientSocket, F_GETFD) | FD_CLOEXEC);
#endif

   InetAddress addr = InetAddress::createFromSockaddr(reinterpret_cast<struct sockaddr*>(&clientAddr));
   TCHAR buffer[64];
   nxlog_debug_tag(DEBUG_TAG, 5, _T("SocketListener/%s: Incoming connection from %s"), m_name, addr.toString(buffer));

   if (isConnectionAllowed(addr))
   {
      m_acceptedConnections++;
      nxlog_debug_tag(DEBUG_TAG, 5, _T("SocketListener/%s: Connection from %s accepted"), m_name, buffer);
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
      nxlog_debug_tag(DEBUG_TAG, 5, _T("SocketListener/%s: Connection from %s rejected"), m_name, buffer);
   }
   return true;
}

/**
 * Report select() error if needed
 */
static void ReportPollError(const TCHAR *name)
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
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("SocketListener/%s: select() call failed (%s)"), name, GetLastSocketErrorText(buffer, 256));
      ThreadSleepMs(100);
   }
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
      if (m_sockets.isEmpty())
      {
         ThreadSleepMs(1000);
         continue;
      }

      sp.reset();
      for(int i = 0; i < m_sockets.size(); i++)
         sp.add(m_sockets.get(i));

      int nRet = sp.poll(1000);
      if ((nRet > 0) && !m_stop && !isStopConditionReached())
      {
         for(int i = 0; i < m_sockets.size(); i++)
         {
            SOCKET s = m_sockets.get(i);
            if (!sp.isSet(s))
               continue;

            if (acceptConnection(s))
            {
               errorCount = 0;     // Reset consecutive errors counter
               continue;
            }

            if (errorCount == 0)
            {
               TCHAR buffer[256];
               nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("SocketListener/%s: accept() call failed (%s)"), m_name, GetLastSocketErrorText(buffer, 256));
            }
            errorCount++;
            if (errorCount > 1000)
            {
               nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("SocketListener/%s: multiple consecutive accept() errors"), m_name);
               errorCount = 0;
            }
            ThreadSleepMs(500);
         }
      }
      else if (nRet == -1)
      {
         ReportPollError(m_name);
      }
   }
}

/**
 * Main datagram listener loop
 */
void DatagramSocketListener::mainLoop()
{
   SocketPoller sp;
   while(!m_stop && !isStopConditionReached())
   {
      if (m_sockets.isEmpty())
      {
         ThreadSleepMs(1000);
         continue;
      }

      sp.reset();
      for(int i = 0; i < m_sockets.size(); i++)
         sp.add(m_sockets.get(i));

      int nRet = sp.poll(1000);
      if ((nRet > 0) && !m_stop && !isStopConditionReached())
      {
         for(int i = 0; i < m_sockets.size(); i++)
         {
            SOCKET s = m_sockets.get(i);
            if (sp.isSet(s))
               processDatagram(s);
         }
      }
      else if (nRet == -1)
      {
         ReportPollError(m_name);
      }
   }
}
