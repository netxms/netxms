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
EIP_Message::EIP_Message(const uint8_t *networkData, size_t size)
{
   m_bytes = MemCopyBlock(networkData, size);
   m_header = reinterpret_cast<EIP_EncapsulationHeader*>(m_bytes);
   m_data = m_bytes + sizeof(EIP_EncapsulationHeader);
   m_dataSize = CIP_UInt16Swap(m_header->length);
   m_itemCount = 0;
   m_cpfStartOffset = 2;
   m_readOffset = 0;
   m_writeOffset = 0;
}

/**
 * Create new message and allocate requested space for data
 */
EIP_Message::EIP_Message(EIP_Command command, size_t dataSize, EIP_SessionHandle handle)
{
   m_bytes = MemAllocArray<uint8_t>(dataSize + sizeof(EIP_EncapsulationHeader));
   m_header = reinterpret_cast<EIP_EncapsulationHeader*>(m_bytes);
   m_header->command = CIP_UInt16Swap(command);
   uint64_t context = GetCurrentTimeMs();
   memcpy(m_header->context, &context, 8);
   m_header->sessionHandle = handle;
   m_data = m_bytes + sizeof(EIP_EncapsulationHeader);
   m_dataSize = dataSize;
   m_itemCount = 0;
   m_cpfStartOffset = 2;
   m_readOffset = 0;
   m_writeOffset = 0;
}

/**
 * Message destructor
 */
EIP_Message::~EIP_Message()
{
   MemFree(m_bytes);
}

/**
 * Prepare for reading CPF items starting with given offset
 */
void EIP_Message::prepareCPFRead(size_t startOffset)
{
   m_cpfStartOffset = startOffset;
   m_itemCount = readDataAsUInt16(startOffset);
}

/**
 * Read next CPF item
 */
bool EIP_Message::nextItem(CPF_Item *item)
{
   if (m_readOffset + 4 >= m_dataSize)
      return false;

   if (m_readOffset == 0)
   {
      m_readOffset = m_cpfStartOffset + 2;
   }
   else
   {
      m_readOffset += readDataAsUInt16(m_readOffset + 2) + 4;
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
 * Find CPF item with given code
 */
bool EIP_Message::findItem(uint16_t type, CPF_Item *item)
{
   m_readOffset = 0;
   while(nextItem(item))
   {
      if (item->type == type)
         return true;
   }
   return false;
}

/**
 * Read data at given offset as length prefixed string
 */
bool EIP_Message::readDataAsLengthPrefixString(size_t offset, int prefixSize, TCHAR *buffer, size_t bufferSize) const
{
   size_t len = (prefixSize == 2) ? readDataAsUInt16(offset) : readDataAsUInt8(offset);
   if (offset + len + prefixSize > m_dataSize)
      return false;  // invalid string length
   if (len >= bufferSize)
      len = bufferSize - 1;
#ifdef UNICODE
   mb_to_wchar(reinterpret_cast<char*>(&m_data[offset + prefixSize]), len, buffer, bufferSize);
#else
   memcpy(buffer, &m_data[offset + prefixSize], len);
#endif
   buffer[len] = 0;
   return true;
}

/**
 * Create message receiver for Ethernet/IP
 */
EIP_MessageReceiver::EIP_MessageReceiver(SOCKET s)
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
EIP_MessageReceiver::~EIP_MessageReceiver()
{
   MemFree(m_buffer);
}

/**
 * Read message from already received data in buffer
 */
EIP_Message *EIP_MessageReceiver::readMessageFromBuffer()
{
   if (m_dataSize < sizeof(EIP_EncapsulationHeader))
      return nullptr;

   size_t msgSize = CIP_UInt16Swap(reinterpret_cast<EIP_EncapsulationHeader*>(&m_buffer[m_readPos])->length) + sizeof(EIP_EncapsulationHeader);
   if (m_dataSize < msgSize)
      return nullptr;

   EIP_Message *msg = new EIP_Message(&m_buffer[m_readPos], msgSize);
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
EIP_Message *EIP_MessageReceiver::readMessage(uint32_t timeout)
{
   while(true)
   {
      EIP_Message *msg = readMessageFromBuffer();
      if (msg != nullptr)
         return msg;

      size_t writePos = m_readPos + m_dataSize;
      if ((m_readPos > 0) && (writePos > m_allocated - 2048))
      {
         memmove(m_buffer, &m_buffer[m_readPos], m_dataSize);
         m_readPos = 0;
         writePos = m_dataSize;
      }
      ssize_t bytes = RecvEx(m_socket, &m_buffer[writePos], m_allocated - writePos, 0, timeout);
      if (bytes <= 0)
         break;
      m_dataSize += bytes;
   }
   return nullptr;
}
