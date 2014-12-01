/*
** NetXMS - Network Management System
** NetXMS Foundation Library
** Copyright (C) 2003-2014 Victor Kirhenshtein
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
   m_buffer = (BYTE *)malloc(initialSize);
   m_decryptionBuffer = NULL;
   m_encryptionContext = NULL;
}

/**
 * Abstract message receiver destructor
 */
AbstractMessageReceiver::~AbstractMessageReceiver()
{
   free(m_buffer);
   safe_free(m_decryptionBuffer);
}

/**
 * Get message from buffer
 */
NXCPMessage *AbstractMessageReceiver::getMessageFromBuffer()
{
   NXCPMessage *msg = NULL;

   if (m_dataSize >= NXCP_HEADER_SIZE)
   {
      size_t msgSize = (size_t)ntohl(((NXCP_MESSAGE *)m_buffer)->size);
      if (msgSize <= m_dataSize)
      {
         if (ntohs(((NXCP_MESSAGE *)m_buffer)->code) == CMD_ENCRYPTED_MESSAGE)
         {
            if ((m_encryptionContext != NULL) && (m_encryptionContext != PROXY_ENCRYPTION_CTX))
            {
               if (m_decryptionBuffer == NULL)
                  m_decryptionBuffer = (BYTE *)malloc(m_size);
               if (m_encryptionContext->decryptMessage((NXCP_ENCRYPTED_MESSAGE *)m_buffer, m_decryptionBuffer))
               {
                  msg = new NXCPMessage((NXCP_MESSAGE *)m_buffer);
               }
            }
         }
         else
         {
            msg = new NXCPMessage((NXCP_MESSAGE *)m_buffer);
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
            m_buffer = (BYTE *)realloc(m_buffer, m_size);
            safe_free_and_null(m_decryptionBuffer);
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
NXCPMessage *AbstractMessageReceiver::readMessage(UINT32 timeout, MessageReceiverResult *result)
{
   NXCPMessage *msg;
   while(true)
   {
      msg = getMessageFromBuffer();
      if (msg != NULL)
      {
         *result = MSGRECV_SUCCESS;
         break;
      }
      int bytes = readBytes(&m_buffer[m_dataSize], m_size - m_dataSize, timeout);
      if (bytes <= 0)
      {
         *result = (bytes == 0) ? MSGRECV_CLOSED : ((bytes == -2) ? MSGRECV_TIMEOUT : MSGRECV_COMM_FAILURE);
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
            m_dataSize = (size_t)bytes - m_bytesToSkip;
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
      _T("MSGRECV_DECRYPTION_FAILURE") 
   };
   return ((result >= MSGRECV_SUCCESS) && (result <= MSGRECV_DECRYPTION_FAILURE)) ? text[result] : _T("UNKNOWN");
}

/**
 * Socket message receiver constructor
 */
SocketMessageReceiver::SocketMessageReceiver(SOCKET socket, size_t initialSize, size_t maxSize) : AbstractMessageReceiver(initialSize, maxSize)
{
   m_socket = socket;
}

/**
 * Socket message receiver destructor
 */
SocketMessageReceiver::~SocketMessageReceiver()
{
}

/**
 * Read bytes from socket
 */
int SocketMessageReceiver::readBytes(BYTE *buffer, size_t size, UINT32 timeout)
{
   return RecvEx(m_socket, buffer, size, 0, timeout);
}

/**
 * Pipe message receiver constructor
 */
PipeMessageReceiver::PipeMessageReceiver(HPIPE pipe, size_t initialSize, size_t maxSize) : AbstractMessageReceiver(initialSize, maxSize)
{
   m_pipe = pipe;
#ifdef _WIN32
   m_readEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
#endif
}

/**
 * Socket message receiver destructor
 */
PipeMessageReceiver::~PipeMessageReceiver()
{
#ifdef _WIN32
	CloseHandle(m_readEvent);
#endif
}

/**
 * Read bytes from socket
 */
int PipeMessageReceiver::readBytes(BYTE *buffer, size_t size, UINT32 timeout)
{
#ifdef _WIN32
	OVERLAPPED ov;
	memset(&ov, 0, sizeof(OVERLAPPED));
	ov.hEvent = m_readEvent;
   DWORD bytes;
	if (ReadFile(m_pipe, buffer, (DWORD)size, &bytes, &ov))
   {
      return (int)bytes;   // completed immediately
   }

	if (GetLastError() != ERROR_IO_PENDING)
		return -1;

   if (WaitForSingleObject(m_readEvent, timeout) != WAIT_OBJECT_0)
   {
      CancelIo(m_pipe);
      return -2;
   }

	if (!GetOverlappedResult(m_pipe, &ov, &bytes, TRUE))
	{
      if (GetLastError() != ERROR_MORE_DATA)
		   return -1;
	}
   return (int)bytes;
#else
   return RecvEx(m_pipe, buffer, size, 0, timeout);
#endif
}
