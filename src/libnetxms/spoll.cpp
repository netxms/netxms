/*
** NetXMS - Network Management System
** NetXMS Foundation Library
** Copyright (C) 2003-2020 Victor Kirhenshtein
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
#if !HAVE_POLL
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
int SocketPoller::poll(uint32_t timeout)
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
         int64_t startTime = GetCurrentTimeMs();
         rc = ::poll(m_sockets, m_count, timeout);
         if ((rc != -1) || (errno != EINTR))
            break;
         uint32_t elapsed = static_cast<uint32_t>(GetCurrentTimeMs() - startTime);
         timeout -= std::min(timeout, elapsed);
      } while(timeout > 0);
      return rc;
   }
#else
   if (timeout == INFINITE)
   {
#ifdef _WIN32
      return select(0, m_write ? nullptr : &m_sockets, m_write ? &m_sockets : nullptr, nullptr, nullptr);
#else
      return select(SELECT_NFDS(m_maxfd + 1), m_write ? nullptr : &m_sockets, m_write ? &m_sockets : nullptr, nullptr, nullptr);
#endif
   }
   else
   {
      struct timeval tv;
#ifdef _WIN32
      tv.tv_sec = timeout / 1000;
      tv.tv_usec = (timeout % 1000) * 1000;
      return select(0, m_write ? nullptr : &m_sockets, m_write ? &m_sockets : nullptr, nullptr, (timeout != INFINITE) ? &tv : nullptr);
#else
      int rc;
      do
      {
         tv.tv_sec = timeout / 1000;
         tv.tv_usec = (timeout % 1000) * 1000;
         int64_t startTime = GetCurrentTimeMs();
         rc = select(m_maxfd + 1, m_write ? nullptr : &m_sockets, m_write ? &m_sockets : nullptr, nullptr, &tv);
         if ((rc != -1) || (errno != EINTR))
            break;
         uint32_t elapsed = static_cast<uint32_t>(GetCurrentTimeMs() - startTime);
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
   for(int i = 0; i < m_count; i++)
   {
      if (s == m_sockets[i].fd)
         return (m_sockets[i].revents & ((m_write ? POLLOUT : POLLIN) | POLLERR | POLLHUP)) != 0;
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
#if !HAVE_POLL
   FD_ZERO(&m_sockets);
#ifndef _WIN32
   m_maxfd = 0;
#endif
#endif
}

/**
 * Create background socket poller
 */
BackgroundSocketPoller::BackgroundSocketPoller()
{
   m_mutex = MutexCreateFast();
   m_head = m_memoryPool.allocate();   // dummy element at list head
   m_head->next = nullptr;

#ifdef _WIN32
   m_controlSockets[0] = CreateSocket(AF_INET, SOCK_DGRAM, 0);
   m_controlSockets[1] = CreateSocket(AF_INET, SOCK_DGRAM, 0);
   if ((m_controlSockets[0] != INVALID_SOCKET) && (m_controlSockets[1] != INVALID_SOCKET))
   {
      struct sockaddr_in servAddr;
      memset(&servAddr, 0, sizeof(struct sockaddr_in));
      servAddr.sin_family = AF_INET;
      servAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
      servAddr.sin_port = 0;  // Dynamic port assignment

      if (bind(m_controlSockets[0], (struct sockaddr *)&servAddr, sizeof(struct sockaddr_in)) == 0)
      {
         int len = sizeof(struct sockaddr_in);
         if (getsockname(m_controlSockets[0], (struct sockaddr *)&servAddr, &len) == 0)
         {
            connect(m_controlSockets[1], (struct sockaddr *)&servAddr, sizeof(struct sockaddr_in));
         }
      }
   }
#else
   if (pipe(m_controlSockets) != 0)
   {
      m_controlSockets[0] = INVALID_SOCKET;
      m_controlSockets[1] = INVALID_SOCKET;
   }
#endif

   m_workerThreadId = 0;
   m_workerThread = ThreadCreateEx(this, &BackgroundSocketPoller::workerThread);
}

/**
 * Destroy background poller
 */
BackgroundSocketPoller::~BackgroundSocketPoller()
{
   notifyWorkerThread('S');
   ThreadJoin(m_workerThread);
   closesocket(m_controlSockets[1]);
   closesocket(m_controlSockets[0]);
   MutexDestroy(m_mutex);
}

/**
 * Add socket for background polling
 */
void BackgroundSocketPoller::poll(SOCKET socket, uint32_t timeout, void (*callback)(BackgroundSocketPollResult, SOCKET, void*), void *context)
{
   BackgroundSocketPollRequest *request = m_memoryPool.allocate();
   request->socket = socket;
   request->timeout = timeout;
   request->callback = callback;
   request->context = context;
   request->queueTime = GetCurrentTimeMs();
   request->cancelled = false;

   MutexLock(m_mutex);
   request->next = m_head->next;
   m_head->next = request;
   MutexUnlock(m_mutex);

   // No need for notification if poll() called from worker thread itself
   // (likely that means re-insert from poll completion callback)
   if (GetCurrentThreadId() != m_workerThreadId)
      notifyWorkerThread();
}

/**
 * Cancel socket poll. Registered callback will be called with CANCELLED status.
 */
void BackgroundSocketPoller::cancel(SOCKET socket)
{
   MutexLock(m_mutex);
   auto r = m_head->next;
   for(; r != nullptr; r = r->next)
   {
      if (r->socket == socket)
      {
         r->cancelled = true;
         break;
      }
   }
   MutexUnlock(m_mutex);

   // No need for notification if poll() called from worker thread itself
   // (likely that means cancellation from poll completion callback)
   if ((r != nullptr) && (GetCurrentThreadId() != m_workerThreadId))
      notifyWorkerThread();
}

/**
 * Notify worker thread
 */
void BackgroundSocketPoller::notifyWorkerThread(char command)
{
   if (m_controlSockets[1] != INVALID_SOCKET)
   {
#ifdef _WIN32
      send(m_controlSockets[1], &command, 1, 0);
#else
      write(m_controlSockets[1], &command, 1);
#endif
   }
}

/**
 * Background poller's worker thread
 */
void BackgroundSocketPoller::workerThread()
{
   m_workerThreadId = GetCurrentThreadId();
   SocketPoller sp;
   while(true)
   {
      sp.reset();
      sp.add(m_controlSockets[0]);

      uint32_t timeout = 30000;
      int64_t now = GetCurrentTimeMs();
      BackgroundSocketPollRequest *processedRequests = nullptr;

      MutexLock(m_mutex);
      for(auto r = m_head->next, p = m_head; r != nullptr; p = r, r = r->next)
      {
         uint32_t waitTime = static_cast<uint32_t>(now - r->queueTime);
         if ((waitTime < r->timeout) && !r->cancelled)
         {
            uint32_t t = r->timeout - waitTime;
            if (t < timeout)
               timeout = t;
            sp.add(r->socket);
         }
         else
         {
            p->next = r->next;
            r->next = processedRequests;
            processedRequests = r;
            r = p;
         }
      }
      MutexUnlock(m_mutex);

      for(auto r = processedRequests; r != nullptr;)
      {
         auto n = r->next;
         r->callback(r->cancelled ? BackgroundSocketPollResult::CANCELLED : BackgroundSocketPollResult::TIMEOUT, r->socket, r->context);
         m_memoryPool.free(r);
         r = n;
      }

      if (sp.poll(timeout) > 0)
      {
         if (sp.isSet(m_controlSockets[0]))
         {
            char command = 0;
#ifdef _WIN32
            recv(m_controlSockets[0], &command, 1, 0);
#else
            read(m_controlSockets[0], &command, 1);
#endif
            if (command == 'S')
               break;
         }

         processedRequests = nullptr;
         MutexLock(m_mutex);
         for(auto r = m_head->next, p = m_head; r != nullptr; p = r, r = r->next)
         {
            if (r->cancelled || sp.isSet(r->socket))
            {
               p->next = r->next;
               r->next = processedRequests;
               processedRequests = r;
               r = p;
            }
         }
         MutexUnlock(m_mutex);

         for(auto r = processedRequests; r != nullptr;)
         {
            auto n = r->next;
            r->callback(r->cancelled ? BackgroundSocketPollResult::CANCELLED : BackgroundSocketPollResult::SUCCESS, r->socket, r->context);
            m_memoryPool.free(r);
            r = n;
         }
      }
   }

   for(auto r = m_head->next; r != nullptr; r = r->next)
      r->callback(BackgroundSocketPollResult::SHUTDOWN, r->socket, r->context);
}
