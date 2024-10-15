/*
** NetXMS - Network Management System
** NetXMS Foundation Library
** Copyright (C) 2003-2024 Victor Kirhenshtein
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
** File: msgrecv.cpp
**
**/

#include "libnetxms.h"
#include <nxcpapi.h>

#ifdef _WITH_ENCRYPTION
#include <openssl/ssl.h>
#endif

/**
 * Abstract message receiver constructor
 */
AbstractMessageReceiver::AbstractMessageReceiver(size_t initialSize, size_t maxSize)
{
   m_initialSize = initialSize;
   m_size = initialSize;
   m_maxSize = maxSize;
   m_dataSize = 0;
   m_bytesToSkip = 0;
   m_buffer = MemAllocArrayNoInit<BYTE>(initialSize);
   m_decryptionBuffer = nullptr;
}

/**
 * Abstract message receiver destructor
 */
AbstractMessageReceiver::~AbstractMessageReceiver()
{
   MemFree(m_buffer);
   MemFree(m_decryptionBuffer);
}

/**
 * Get message from buffer
 */
NXCPMessage *AbstractMessageReceiver::getMessageFromBuffer(bool *protocolError, bool *decryptionError)
{
   NXCPMessage *msg = nullptr;

   if (m_dataSize >= NXCP_HEADER_SIZE)
   {
      size_t msgSize = (size_t)ntohl(((NXCP_MESSAGE *)m_buffer)->size);
      if ((msgSize < NXCP_HEADER_SIZE) || (msgSize % 8 != 0))
      {
         // impossible value in message size field, assuming garbage on input
         *protocolError = true;
         m_dataSize -= 8;  // advance to next 8 byte block
         if (m_dataSize > 0)
         {
            memmove(m_buffer, &m_buffer[8], m_dataSize);
         }
      }
      else if (msgSize <= m_dataSize)
      {
         if (ntohs(reinterpret_cast<NXCP_MESSAGE*>(m_buffer)->code) == CMD_ENCRYPTED_MESSAGE)
         {
            if (m_encryptionContext != nullptr)
            {
               if (m_decryptionBuffer == nullptr)
                  m_decryptionBuffer = MemAllocArrayNoInit<BYTE>(m_size);
               if (m_encryptionContext->decryptMessage(reinterpret_cast<NXCP_ENCRYPTED_MESSAGE*>(m_buffer), m_decryptionBuffer))
               {
                  msg = NXCPMessage::deserialize(reinterpret_cast<NXCP_MESSAGE*>(m_buffer));
                  if (msg == nullptr)
                     *protocolError = true;  // message deserialization error
               }
               else
               {
                  *protocolError = true;
                  *decryptionError = true;
               }
            }
         }
         else
         {
            msg = NXCPMessage::deserialize(reinterpret_cast<NXCP_MESSAGE*>(m_buffer));
            if (msg == nullptr)
               *protocolError = true;  // message deserialization error
         }
         m_dataSize -= msgSize;
         if (m_dataSize > 0)
         {
            memmove(m_buffer, &m_buffer[msgSize], m_dataSize);
         }
      }
      else if (msgSize > m_size)
      {
         if (msgSize <= m_maxSize)
         {
            m_size = msgSize;
            m_buffer = MemRealloc(m_buffer, m_size);
            MemFreeAndNull(m_decryptionBuffer);
         }
         else if (msgSize > (size_t)0x3FFFFFFF)
         {
            // too large value in message size field, assuming garbage on input
            *protocolError = true;
        }
         else
         {
            m_bytesToSkip = msgSize - m_dataSize;
            m_dataSize = 0;
         }
      }
   }

   return msg;
}

/**
 * Read message from communication channel
 */
NXCPMessage *AbstractMessageReceiver::readMessage(uint32_t timeout, MessageReceiverResult *result, bool allowReadBytes)
{
   NXCPMessage *msg;
   bool protocolError = false, decryptionError = false;
   while(true)
   {
      msg = getMessageFromBuffer(&protocolError, &decryptionError);
      if (msg != nullptr)
      {
         *result = MSGRECV_SUCCESS;
         break;
      }
      if (protocolError)
      {
         *result = decryptionError ? MSGRECV_DECRYPTION_FAILURE : MSGRECV_PROTOCOL_ERROR;
         break;
      }
      if (!allowReadBytes)
      {
         *result = MSGRECV_WANT_READ;
         break;
      }
      ssize_t bytes = readBytes(&m_buffer[m_dataSize], m_size - m_dataSize, timeout);
      if (bytes <= 0)
      {
         if (bytes == 0)
            *result = MSGRECV_CLOSED;
         else if (bytes == -4)
            *result = MSGRECV_WANT_READ;
         else if (bytes == -3)
            *result = MSGRECV_WANT_WRITE;
         else if (bytes == -2)
            *result = MSGRECV_TIMEOUT;
         else
            *result = MSGRECV_COMM_FAILURE;
         break;
      }
      if (m_bytesToSkip > 0)
      {
         if (bytes <= m_bytesToSkip)
         {
            m_bytesToSkip -= bytes;
         }
         else
         {
            m_dataSize = bytes - m_bytesToSkip;
            memmove(m_buffer, &m_buffer[m_bytesToSkip], m_dataSize);
            m_bytesToSkip = 0;
         }
      }
      else
      {
         m_dataSize += bytes;
      }
   }
   return msg;
}

/**
 * Convert result to text
 */
const TCHAR *AbstractMessageReceiver::resultToText(MessageReceiverResult result)
{
   static const TCHAR *text[] = { 
      _T("MSGRECV_SUCCESS"), 
      _T("MSGRECV_CLOSED"), 
      _T("MSGRECV_TIMEOUT"), 
      _T("MSGRECV_COMM_FAILURE"), 
      _T("MSGRECV_DECRYPTION_FAILURE"),
      _T("MSGRECV_PROTOCOL_ERROR")
   };
   return ((result >= MSGRECV_SUCCESS) && (result <= MSGRECV_PROTOCOL_ERROR)) ? text[result] : _T("UNKNOWN");
}

/**
 * Socket message receiver constructor
 */
SocketMessageReceiver::SocketMessageReceiver(SOCKET socket, size_t initialSize, size_t maxSize) : AbstractMessageReceiver(initialSize, maxSize)
{
   m_socket = socket;
#ifndef _WIN32
   if (pipe(m_controlPipe) != 0)
   {
      m_controlPipe[0] = -1;
      m_controlPipe[1] = -1;
   }
#endif
}

/**
 * Socket message receiver destructor
 */
SocketMessageReceiver::~SocketMessageReceiver()
{
#ifndef _WIN32
   if (m_controlPipe[0] != -1)
      _close(m_controlPipe[0]);
   if (m_controlPipe[1] != -1)
      _close(m_controlPipe[1]);
#endif
}

/**
 * Read bytes from socket
 */
ssize_t SocketMessageReceiver::readBytes(BYTE *buffer, size_t size, uint32_t timeout)
{
   if (timeout == 0)
   {
#ifdef _WIN32
      int rc = recv(m_socket, reinterpret_cast<char*>(buffer), static_cast<int>(size), 0);
#else
      int rc = recv(m_socket, reinterpret_cast<char*>(buffer), size, 0);
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
 * Stop receiver
 */
void SocketMessageReceiver::cancel()
{
#ifdef _WIN32
   shutdown(m_socket, SHUT_RDWR);
#else
   if (m_controlPipe[1] != -1)
      _write(m_controlPipe[1], "X", 1);
   else
      shutdown(m_socket, SHUT_RDWR);
#endif
}

/**
 * Communication channel message receiver constructor
 */
CommChannelMessageReceiver::CommChannelMessageReceiver(const shared_ptr<AbstractCommChannel>& channel, size_t initialSize, size_t maxSize) :
         AbstractMessageReceiver(initialSize, maxSize), m_channel(channel)
{
}

/**
 * Read bytes from communication channel
 */
ssize_t CommChannelMessageReceiver::readBytes(BYTE *buffer, size_t size, uint32_t timeout)
{
   return m_channel->recv(buffer, size, timeout);
}

/**
 * Stop receiver
 */
void CommChannelMessageReceiver::cancel()
{
   m_channel->shutdown();
}

#ifdef _WITH_ENCRYPTION

/**
 * TLS message receiver constructor
 */
TlsMessageReceiver::TlsMessageReceiver(SOCKET socket, SSL *ssl, Mutex *mutex, size_t initialSize, size_t maxSize) : AbstractMessageReceiver(initialSize, maxSize)
{
   m_socket = socket;
   m_ssl = ssl;
   m_mutex = mutex;
#ifndef _WIN32
   if (pipe(m_controlPipe) != 0)
   {
      m_controlPipe[0] = -1;
      m_controlPipe[1] = -1;
   }
#endif
}

/**
 * TLS message receiver destructor
 */
TlsMessageReceiver::~TlsMessageReceiver()
{
#ifndef _WIN32
   if (m_controlPipe[0] != -1)
      _close(m_controlPipe[0]);
   if (m_controlPipe[1] != -1)
      _close(m_controlPipe[1]);
#endif
}

/**
 * Read bytes from TLS connection
 */
ssize_t TlsMessageReceiver::readBytes(BYTE *buffer, size_t size, uint32_t timeout)
{
   bool doRead = true;
   bool needWrite = false;
   int bytes = 0;
   m_mutex->lock();
   while(doRead)
   {
      if (SSL_pending(m_ssl) == 0)
      {
         m_mutex->unlock();
         SocketPoller sp(needWrite);
         sp.add(m_socket);
#ifndef _WIN32
         if (!needWrite && (m_controlPipe[0] != -1))
            sp.add(m_controlPipe[0]);
#endif
         int rc = sp.poll(timeout);
         if (rc <= 0)
            return (rc == 0) ? ((timeout == 0) ? (needWrite ? -3 : -4) : -2) : -1;   // -2 for timeout
#ifndef _WIN32
         if (!needWrite && (m_controlPipe[0] != -1) && sp.isSet(m_controlPipe[0]))
         {
            char data;
            _read(m_controlPipe[0], &data, 1);
            return 0;
         }
#endif
         needWrite = false;
         m_mutex->lock();
      }
      doRead = false;
      bytes = SSL_read(m_ssl, buffer, (int)size);
      if (bytes <= 0)
      {
         int err = SSL_get_error(m_ssl, bytes);
         if ((err == SSL_ERROR_WANT_READ) || (err == SSL_ERROR_WANT_WRITE))
         {
            doRead = true;
            needWrite = (err == SSL_ERROR_WANT_WRITE);
         }
         else
         {
            nxlog_debug(7, _T("TlsMessageReceiver: SSL_read error (ssl_err=%d socket_err=%d)"), err, WSAGetLastError());
            if (err == SSL_ERROR_SSL)
               LogOpenSSLErrorStack(7);
         }
      }
   }
   m_mutex->unlock();
   return bytes;
}

/**
 * Stop receiver
 */
void TlsMessageReceiver::cancel()
{
#ifdef _WIN32
   shutdown(m_socket, SHUT_RDWR);
#else
   if (m_controlPipe[1] != -1)
      _write(m_controlPipe[1], "X", 1);
   else
      shutdown(m_socket, SHUT_RDWR);
#endif
}

#endif /* _WITH_ENCRYPTION */

/**
 * Pipe message receiver constructor
 */
PipeMessageReceiver::PipeMessageReceiver(HPIPE hpipe, size_t initialSize, size_t maxSize) : AbstractMessageReceiver(initialSize, maxSize)
{
   m_pipe = hpipe;
#ifdef _WIN32
   m_readEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
   m_cancelEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
#else
   if (pipe(m_controlPipe) != 0)
   {
      m_controlPipe[0] = -1;
      m_controlPipe[1] = -1;
   }
#endif
}

/**
 * Socket message receiver destructor
 */
PipeMessageReceiver::~PipeMessageReceiver()
{
#ifdef _WIN32
	CloseHandle(m_readEvent);
	CloseHandle(m_cancelEvent);
#else
   if (m_controlPipe[0] != -1)
      _close(m_controlPipe[0]);
   if (m_controlPipe[1] != -1)
      _close(m_controlPipe[1]);
#endif
}

/**
 * Read bytes from socket
 */
ssize_t PipeMessageReceiver::readBytes(BYTE *buffer, size_t size, uint32_t timeout)
{
#ifdef _WIN32
	OVERLAPPED ov;
	memset(&ov, 0, sizeof(OVERLAPPED));
	ov.hEvent = m_readEvent;
   DWORD bytes;
	if (ReadFile(m_pipe, buffer, static_cast<DWORD>(size), &bytes, &ov))
   {
      return bytes;   // completed immediately
   }

	if (GetLastError() != ERROR_IO_PENDING)
		return -1;

	HANDLE handles[2] = { m_readEvent, m_cancelEvent };
	DWORD rc = WaitForMultipleObjects(2, handles, FALSE, timeout);
   if (rc != WAIT_OBJECT_0)
   {
      CancelIo(m_pipe);
      return (rc == WAIT_OBJECT_0 + 1) ? 0 : -2;
   }

	if (!GetOverlappedResult(m_pipe, &ov, &bytes, TRUE))
	{
      if (GetLastError() != ERROR_MORE_DATA)
		   return -1;
	}
   return bytes;
#else
   return RecvEx(m_pipe, buffer, size, 0, timeout, m_controlPipe[0]);
#endif
}

/**
 * Stop receiver
 */
void PipeMessageReceiver::cancel()
{
#ifdef _WIN32
   SetEvent(m_cancelEvent);
#else
   if (m_controlPipe[1] != -1)
      _write(m_controlPipe[1], "X", 1);
#endif
}
