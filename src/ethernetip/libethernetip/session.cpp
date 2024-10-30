/*
** NetXMS - Network Management System
** Ethernet/IP support library
** Copyright (C) 2003-2020 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
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
** File: session.cpp
**
**/

#include "libethernetip.h"

/**
 * Establish EtherNet/IP session
 */
EIP_Session *EIP_Session::connect(const InetAddress& addr, uint16_t port, uint32_t timeout, EIP_Status *status)
{
   SOCKET s = ConnectToHost(addr, port, timeout);
   if (s == INVALID_SOCKET)
   {
      *status = EIP_Status::callFailure(EIP_CALL_CONNECT_FAILED);
      return nullptr;
   }
   return connect(s, timeout, status);
}

/**
 * Establish EtherNet/IP session over already connected socket
 */
EIP_Session *EIP_Session::connect(SOCKET socket, uint32_t timeout, EIP_Status *status)
{
   EIP_Message request(EIP_REGISTER_SESSION, 4);
   request.writeDataAsUInt16(1); // Protocol version 1
   request.writeDataAsUInt16(0); // Options
   request.completeDataWrite();
   EIP_Message *response = EIP_DoRequest(socket, request, timeout, status);
   if (response == nullptr)
   {
      shutdown(socket, SHUT_RDWR);
      closesocket(socket);
      return nullptr;
   }

   auto session = new EIP_Session(response->getSessionHandle(), socket, timeout);
   delete response;
   return session;
}

/**
 * Session constructor
 */
EIP_Session::EIP_Session(EIP_SessionHandle handle, SOCKET socket, uint32_t timeout)
{
   m_handle = handle;
   m_socket = socket;
   m_timeout = timeout;
   m_connected = true;
}

/**
 * Session destructor
 */
EIP_Session::~EIP_Session()
{
   disconnect();
}

/**
 * Disconnect session
 */
void EIP_Session::disconnect()
{
   if (!m_connected)
      return;

   // There is no reply to UnregisterSession command
   EIP_Message request(EIP_UNREGISTER_SESSION, 0, m_handle);
   SendEx(m_socket, request.getBytes(), request.getSize(), 0, nullptr);

   shutdown(m_socket, SHUT_RDWR);
   closesocket(m_socket);
   m_connected = false;
}

/**
 * Get attribute from device
 */
EIP_Status EIP_Session::getAttribute(const TCHAR *path, void *buffer, size_t *size)
{
   CIP_EPATH epath;
   if (!CIP_EncodeAttributePath(path, &epath))
      return EIP_Status::callFailure(EIP_INVALID_EPATH);
   return getAttribute(epath, buffer, size);
}

/**
 * Get attribute from device
 */
EIP_Status EIP_Session::getAttribute(uint32_t classId, uint32_t instance, uint32_t attributeId, void *buffer, size_t *size)
{
   CIP_EPATH epath;
   CIP_EncodeAttributePath(classId, instance, attributeId, &epath);
   return getAttribute(epath, buffer, size);
}

/**
 * Get attribute from device
 */
EIP_Status EIP_Session::getAttribute(const CIP_EPATH& path, void *buffer, size_t *size)
{
   EIP_Message request(EIP_SEND_RR_DATA, 1024, m_handle);
   request.advanceWritePosition(6); // Interface ID and timeout left as 0
   request.writeDataAsUInt16(2);    // Item count
   request.advanceWritePosition(4); // Item type 0 followed by length 0 - NULL address
   request.writeDataAsUInt16(0xB2); // Type B2 - UCMM message
   request.writeDataAsUInt16(static_cast<uint16_t>(path.size + 2)); // Second item size: service + path length + path
   request.writeDataAsUInt8(CIP_Get_Attribute_Single);
   request.writeDataAsUInt8(static_cast<uint8_t>(path.size / 2)); // Path length in 2 byte words
   request.writeData(path.value, path.size);
   request.completeDataWrite();

   EIP_Status status;
   EIP_Message *response = EIP_DoRequest(m_socket, request, m_timeout, &status);
   if (response == nullptr)
      return status;

   response->prepareCPFRead(6);

   CPF_Item item;
   if (response->findItem(0xB2, &item))
   {
      CIP_GeneralStatus generalStatus = response->readDataAsUInt8(item.offset + 2);
      if (generalStatus == 0)
      {
         uint16_t additionalStatusSize = response->readDataAsUInt8(item.offset + 3) * 2;
         size_t recvSize = item.length - additionalStatusSize - 4;
         memcpy(buffer, response->getRawData() + item.offset + additionalStatusSize + 4, std::min(*size, recvSize));
         *size = recvSize;
         status = EIP_Status::success();
      }
      else
      {
         status = EIP_Status::deviceFailure(generalStatus);
      }
   }
   else
   {
      status = EIP_Status::callFailure(EIP_CALL_BAD_RESPONSE);
   }

   delete response;
   return status;
}
