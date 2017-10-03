/*
** NetXMS - Network Management System
** NetXMS Foundation Library
** Copyright (C) 2003-2016 Victor Kirhenshtein
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
** File: spoll.cpp
**
**/

#include "libnetxms.h"

/**
 * Poller constructor
 */
SocketPoller::SocketPoller(bool write)
{
   m_write = write;
   m_count = 0;
#if HAVE_POLL
   memset(m_sockets, 0, sizeof(m_sockets));
#else
   FD_ZERO(&m_sockets);
#ifndef _WIN32
   m_maxfd = 0;
#endif
#endif
}

/**
 * Poller destructor
 */
SocketPoller::~SocketPoller()
{
}

/**
 * Add socket
 */
bool SocketPoller::add(SOCKET s)
{
   if ((s == INVALID_SOCKET) || (m_count == SOCKET_POLLER_MAX_SOCKETS))
      return false;

#if HAVE_POLL
   m_sockets[m_count].fd = s;
   m_sockets[m_count].events = m_write ? POLLOUT : POLLIN;
#else
#ifndef _WIN32
   if (s >= FD_SETSIZE)
      return false;
#endif
   FD_SET(s, &m_sockets);
#ifndef _WIN32
   if (s > m_maxfd)
      m_maxfd = s;
#endif
#endif

   m_count++;
   return true;
}

/**
 * Poll sockets
 */
int SocketPoller::poll(UINT32 timeout)
{
   if (m_count == 0)
      return -1;

#if HAVE_POLL
   if (timeout == INFINITE)
   {
      return ::poll(m_sockets, m_count, -1);
   }
   else
   {
      int rc;
      do
      {
         INT64 startTime = GetCurrentTimeMs();
         rc = ::poll(m_sockets, m_count, timeout);
         if ((rc != -1) || (errno != EINTR))
            break;
         UINT32 elapsed = (UINT32)(GetCurrentTimeMs() - startTime);
         timeout -= std::min(timeout, elapsed);
      } while(timeout > 0);
      return rc;
   }
#else
   if (timeout == INFINITE)
   {
#ifdef _WIN32
      return select(0, m_write ? NULL : &m_sockets, m_write ? &m_sockets : NULL, NULL, NULL);
#else
      return select(SELECT_NFDS(m_maxfd + 1), m_write ? NULL : &m_sockets, m_write ? &m_sockets : NULL, NULL, NULL);
#endif
   }
   else
   {
      struct timeval tv;
#ifdef _WIN32
      tv.tv_sec = timeout / 1000;
      tv.tv_usec = (timeout % 1000) * 1000;
      return select(0, m_write ? NULL : &m_sockets, m_write ? &m_sockets : NULL, NULL, (timeout != INFINITE) ? &tv : NULL);
#else
      int rc;
      do
      {
         tv.tv_sec = timeout / 1000;
         tv.tv_usec = (timeout % 1000) * 1000;
         INT64 startTime = GetCurrentTimeMs();
         int rc = select(m_maxfd + 1, m_write ? NULL : &m_sockets, m_write ? &m_sockets : NULL, NULL, &tv);
         if ((rc != -1) || (errno != EINTR))
            break;
         UINT32 elapsed = (UINT32)(GetCurrentTimeMs() - startTime);
         timeout -= std::min(timeout, elapsed);
      } while(timeout > 0);
      return rc;
#endif
   }
#endif
}

/**
 * Check if socket is set
 */
bool SocketPoller::isSet(SOCKET s)
{
#if HAVE_POLL
   for(int i = 0; i < SOCKET_POLLER_MAX_SOCKETS; i++)
   {
      if (s == m_sockets[i].fd)
         return m_sockets[i].revents & (m_write ? POLLOUT : POLLIN);
   }
   return false;
#else
   return FD_ISSET(s, &m_sockets) ? true : false;
#endif
}

/**
 * Reset poller (remove all added sockets)
 */
void SocketPoller::reset()
{
   m_count = 0;
#if HAVE_POLL
   memset(m_sockets, 0, sizeof(m_sockets));
#else
   FD_ZERO(&m_sockets);
#ifndef _WIN32
   m_maxfd = 0;
#endif
#endif
}
