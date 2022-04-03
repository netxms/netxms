/*
** NetXMS - Network Management System
** NetXMS Foundation Library
** Copyright (C) 2003-2022 Victor Kirhenshtein
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
** File: cch.cpp
**
**/

#include "libnetxms.h"

/**
 * Abstract communication channel constructor
 */
AbstractCommChannel::AbstractCommChannel()
{
}

/**
 * Abstract communication channel destructor
 */
AbstractCommChannel::~AbstractCommChannel()
{
}

/**
 * Socket communication channel constructor
 */
SocketCommChannel::SocketCommChannel(SOCKET socket, BackgroundSocketPollerHandle *socketPoller, Ownership owner) : AbstractCommChannel()
{
   m_socket = socket;
   m_owner = (owner == Ownership::True);
#ifndef _WIN32
   if (pipe(m_controlPipe) != 0)
   {
      m_controlPipe[0] = -1;
      m_controlPipe[1] = -1;
   }
#endif
   m_socketPoller = socketPoller;
}

/**
 * Socket communication channel destructor
 */
SocketCommChannel::~SocketCommChannel()
{
   if (m_owner && (m_socket != INVALID_SOCKET))
      closesocket(m_socket);
#ifndef _WIN32
   if (m_controlPipe[0] != -1)
      _close(m_controlPipe[0]);
   if (m_controlPipe[1] != -1)
      _close(m_controlPipe[1]);
#endif
   if (m_socketPoller != nullptr)
      InterlockedDecrement(&m_socketPoller->usageCount);
}

/**
 * Send data
 */
ssize_t SocketCommChannel::send(const void *data, size_t size, Mutex* mutex)
{
   return SendEx(m_socket, data, size, 0, mutex);
}

/**
 * Receive data
 */
ssize_t SocketCommChannel::recv(void *buffer, size_t size, uint32_t timeout)
{
   if (timeout == 0)
   {
#ifdef _WIN32
      int rc = ::recv(m_socket, reinterpret_cast<char*>(buffer), static_cast<int>(size), 0);
#else
      int rc = ::recv(m_socket, reinterpret_cast<char*>(buffer), size, 0);
#endif
      if (rc >= 0)
         return rc;
      return ((WSAGetLastError() == WSAEWOULDBLOCK) || (WSAGetLastError() == WSAEINPROGRESS)) ? -4 : -1;
   }
#ifdef _WIN32
   return RecvEx(m_socket, buffer, size, 0, timeout);
#else
   return RecvEx(m_socket, buffer, size, 0, timeout, m_controlPipe[0]);
#endif
}

/**
 * Poll channel
 */
int SocketCommChannel::poll(uint32_t timeout, bool write)
{
   if (m_socket == INVALID_SOCKET)
      return -1;

   SocketPoller sp(write);
   sp.add(m_socket);
   return sp.poll(timeout);
}

/**
 * Shutdown channel
 */
int SocketCommChannel::shutdown()
{
#ifndef _WIN32
   // Cause select/poll to wake up
   if (m_controlPipe[1] != -1)
      _write(m_controlPipe[1], "X", 1);
#endif
   return (m_socket != INVALID_SOCKET) ? ::shutdown(m_socket, SHUT_RDWR) : -1;
}

/**
 * Close channel
 */
void SocketCommChannel::close()
{
   if (m_socket == INVALID_SOCKET)
      return;

   if (m_socketPoller != nullptr)
      m_socketPoller->poller.cancel(m_socket);
   closesocket(m_socket);
   m_socket = INVALID_SOCKET;
}

/**
 * Wrapper context for background socket poll
 */
struct BackgroundPollContext
{
   SocketCommChannel *channel;
   void (*callback)(BackgroundSocketPollResult, AbstractCommChannel*, void*);
   void *context;
};

/**
 * Wrapper callback for background socket poll
 */
static void BackgroundPollWrapper(BackgroundSocketPollResult pollResult, SOCKET hSocket, BackgroundPollContext *context)
{
   context->callback(pollResult, context->channel, context->context);
   MemFree(context);
}

/**
 * Start background poll
 */
void SocketCommChannel::backgroundPoll(uint32_t timeout, void (*callback)(BackgroundSocketPollResult, AbstractCommChannel*, void*), void *context)
{
   if ((m_socketPoller != nullptr) && (m_socket != INVALID_SOCKET))
   {
      auto wrapperContext = MemAllocStruct<BackgroundPollContext>();
      wrapperContext->channel = this;
      wrapperContext->callback = callback;
      wrapperContext->context = context;
      m_socketPoller->poller.poll(m_socket, timeout, BackgroundPollWrapper, wrapperContext);
   }
   else
   {
      callback(BackgroundSocketPollResult::FAILURE, this, context);
   }
}
