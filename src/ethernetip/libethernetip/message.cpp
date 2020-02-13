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
** File: message.cpp
**
**/

#include "libethernetip.h"

/**
 * Create message from network data
 */
CIP_Message::CIP_Message(const uint8_t *networkData, size_t size)
{
   m_bytes = MemCopyBlock(networkData, size);
   m_header = reinterpret_cast<CIP_EncapsulationHeader*>(m_bytes);
   m_data = m_bytes + sizeof(CIP_EncapsulationHeader);
   m_dataSize = CIP_UInt16Swap(m_header->length);
   m_itemCount = (m_dataSize > 1) ? readDataAsUInt16(0) : 0;
   m_readOffset = 0;
}

/**
 * Create new message and allocate requested space for data
 */
CIP_Message::CIP_Message(CIP_Command command, size_t dataSize)
{
   m_bytes = MemAllocArray<uint8_t>(dataSize + sizeof(CIP_EncapsulationHeader));
   m_header = reinterpret_cast<CIP_EncapsulationHeader*>(m_bytes);
   m_header->command = CIP_UInt16Swap(command);
   m_data = m_bytes + sizeof(CIP_EncapsulationHeader);
   m_dataSize = dataSize;
   m_itemCount = 0;
   m_readOffset = 0;
}

/**
 * Message destructor
 */
CIP_Message::~CIP_Message()
{
   MemFree(m_bytes);
}

/**
 * Read next CPF item
 */
bool CIP_Message::nextItem(CPF_Item *item)
{
   if (m_readOffset + 4 >= m_dataSize)
      return false;

   if (m_readOffset == 0)
   {
      m_readOffset = 2;
   }
   else
   {
      m_readOffset += readDataAsUInt16(m_readOffset + 2);
   }
   if (m_readOffset + 4 >= m_dataSize)
      return false;

   item->type = readDataAsUInt16(m_readOffset);
   item->length = readDataAsUInt16(m_readOffset + 2);
   item->offset = static_cast<uint32_t>(m_readOffset + 4);
   item->data = &m_data[m_readOffset + 4];
   return m_readOffset + item->length + 4 <= m_dataSize;
}

/**
 * Read data at given offset as length prefixed string
 */
bool CIP_Message::readDataAsLengthPrefixString(size_t offset, TCHAR *buffer, size_t bufferSize) const
{
   size_t len = readDataAsUInt8(offset);
   if (offset + len + 1 > m_dataSize)
      return false;  // invalid string length
   if (len >= bufferSize)
      len = bufferSize - 1;
#ifdef UNICODE
   MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, reinterpret_cast<char*>(&m_data[offset + 1]), len, buffer, bufferSize);
#else
   memcpy(buffer, &m_data[offset + 1], len);
#endif
   buffer[len] = 0;
   return true;
}

/**
 * Create message receiver for Ethernet/IP
 */
EthernetIP_MessageReceiver::EthernetIP_MessageReceiver(SOCKET s)
{
   m_socket = s;
   m_allocated = 8192;
   m_dataSize = 0;
   m_readPos = 0;
   m_buffer = MemAllocArrayNoInit<uint8_t>(m_allocated);
}

/**
 * Message receiver destructor
 */
EthernetIP_MessageReceiver::~EthernetIP_MessageReceiver()
{
   MemFree(m_buffer);
}

/**
 * Read message from already received data in buffer
 */
CIP_Message *EthernetIP_MessageReceiver::readMessageFromBuffer()
{
   if (m_dataSize < sizeof(CIP_EncapsulationHeader))
      return nullptr;

   size_t msgSize = CIP_UInt16Swap(reinterpret_cast<CIP_EncapsulationHeader*>(&m_buffer[m_readPos])->length) + sizeof(CIP_EncapsulationHeader);
   if (m_dataSize < msgSize)
      return nullptr;

   CIP_Message *msg = new CIP_Message(&m_buffer[m_readPos], msgSize);
   m_dataSize -= msgSize;
   if (m_dataSize > 0)
      m_readPos += msgSize;
   else
      m_readPos = 0;
   return msg;
}

/**
 * Read message from network
 */
CIP_Message *EthernetIP_MessageReceiver::readMessage(uint32_t timeout)
{
   while(true)
   {
      CIP_Message *msg = readMessageFromBuffer();
      if (msg != nullptr)
         return msg;

      size_t writePos = m_readPos + m_dataSize;
      if ((m_readPos > 0) && (writePos > m_allocated - 2048))
      {
         memmove(m_buffer, &m_buffer[m_readPos], m_dataSize);
         m_readPos = 0;
         writePos = m_dataSize;
      }
      int bytes = RecvEx(m_socket, &m_buffer[writePos], m_allocated - writePos, 0, timeout);
      if (bytes <= 0)
         break;
      m_dataSize += bytes;
   }
   return nullptr;
}
