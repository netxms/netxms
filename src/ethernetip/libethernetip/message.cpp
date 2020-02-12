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
   memcpy(&m_header, networkData, sizeof(CIP_EncapsulationHeader));
   m_dataSize = CIP_UInt16Swap(m_header.length);
   m_data = MemAllocArrayNoInit<uint8_t>(m_dataSize);
   memcpy(m_data, networkData + sizeof(CIP_EncapsulationHeader), std::min(size - sizeof(CIP_EncapsulationHeader), m_dataSize));
}

/**
 * Create new message and allocate requested space for data
 */
CIP_Message::CIP_Message(CIP_Command command, size_t dataSize)
{
   memset(&m_header, 0, sizeof(CIP_EncapsulationHeader));
   m_header.command = CIP_UInt16Swap(command);
   m_dataSize = dataSize;
   m_data = MemAllocArray<uint8_t>(m_dataSize);
}

/**
 * Message destructor
 */
CIP_Message::~CIP_Message()
{
   MemFree(m_data);
}

/**
 * Create message receiver for Ethernet/IP
 */
EthernetIP_MessageReceiver::EthernetIP_MessageReceiver(SOCKET s)
{
   m_socket = s;
   m_allocated = 8192;
   m_writePos = 0;
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
 * Read message from network
 */
CIP_Message *EthernetIP_MessageReceiver::readMessage(uint32_t timeout)
{
   return NULL;
}
