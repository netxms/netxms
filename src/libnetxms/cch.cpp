/*
** NetXMS - Network Management System
** NetXMS Foundation Library
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
** File: cch.cpp
**
**/

#include "libnetxms.h"

/**
 * Abstract communication channel constructor
 */
AbstractCommChannel::AbstractCommChannel() : RefCountObject()
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
SocketCommChannel::SocketCommChannel(SOCKET socket, bool owner) : AbstractCommChannel()
{
   m_socket = socket;
   m_owner = owner;
}

/**
 * Socket communication channel destructor
 */
SocketCommChannel::~SocketCommChannel()
{
   if (m_owner && (m_socket != INVALID_SOCKET))
      closesocket(m_socket);
}

/**
 * Send data
 */
int SocketCommChannel::send(const void *data, size_t size, MUTEX mutex)
{
   return SendEx(m_socket, data, size, 0, mutex);
}

/**
 * Receive data
 */
int SocketCommChannel::recv(void *buffer, size_t size, UINT32 timeout)
{
   return RecvEx(m_socket, buffer, size, 0, timeout);
}

/**
 * Poll channel
 */
int SocketCommChannel::poll(UINT32 timeout, bool write)
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
   return (m_socket != INVALID_SOCKET) ? ::shutdown(m_socket, SHUT_RDWR) : -1;
}

/**
 * Close channel
 */
void SocketCommChannel::close()
{
   closesocket(m_socket);
   m_socket = INVALID_SOCKET;
}
