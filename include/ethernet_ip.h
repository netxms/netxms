/*
** NetXMS - Network Management System
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
** File: ethernet_ip.h
**
**/

#ifndef _ethernet_ip_h_
#define _ethernet_ip_h_

#include <nms_common.h>
#include <nms_util.h>

#ifdef LIBETHERNETIP_EXPORTS
#define LIBETHERNETIP_EXPORTABLE __EXPORT
#else
#define LIBETHERNETIP_EXPORTABLE __IMPORT
#endif

/**
 * Convert UInt16 to CIP network byte order
 */
inline uint16_t CIP_UInt16Swap(uint16_t value)
{
#if WORDS_BIGENDIAN
   return ((value & 0xFF) << 8) | (value >> 8);
#else
   return value;
#endif
}

/**
 * Convert UInt32 to CIP network byte order
 */
inline uint32_t CIP_UInt32Swap(uint32_t value)
{
#if WORDS_BIGENDIAN
   return ((value & 0xFF) << 24) | ((value & 0xFF00) << 8) | ((value & 0xFF0000) >> 8) | (value >> 24);
#else
   return value;
#endif
}

#ifdef __HP_aCC
#pragma pack 1
#else
#pragma pack(1)
#endif

/**
 * CIP encapsulation header
 */
struct CIP_EncapsulationHeader
{
   uint16_t command;
   uint16_t length;
   uint32_t sessionHandle;
   uint32_t status;
   uint8_t context[8];
   uint32_t options;
};

#if defined(__HP_aCC)
#pragma pack
#elif defined(_AIX) && !defined(__GNUC__)
#pragma pack(pop)
#else
#pragma pack()
#endif

/**
 * CIP commands
 */
enum CIP_Command
{
   CIP_NOOP = 0x0000,
   CIP_LIST_SERVICES = 0x0004,
   CIP_LIST_IDENTITY= 0x0063,
   CIP_LIST_INTERFACES = 0x0064,
   CIP_REGISTER_SESSION = 0x0065,
   CIP_UNREGISTER_SESSION = 0x0066,
   CIP_SEND_RR_DATA = 0x006F,
   CIP_SEND_UNIT_DATA = 0x0070
};

/**
 * CIP message
 */
class LIBETHERNETIP_EXPORTABLE CIP_Message
{
private:
   CIP_EncapsulationHeader m_header;
   size_t m_dataSize;
   uint8_t *m_data;

public:
   CIP_Message(const uint8_t *networkData, size_t size);
   CIP_Message(CIP_Command command, size_t dataSize);
   ~CIP_Message();

   CIP_Command getCommand() const { return static_cast<CIP_Command>(CIP_UInt16Swap(m_header.command)); }
   uint32_t getSessionHandle() const { return CIP_UInt32Swap(m_header.sessionHandle); }
   uint32_t getStatus() const { return CIP_UInt32Swap(m_header.status); }
   size_t getDataSize() const { return m_dataSize; }
   const uint8_t *getData() const { return m_data; }

   uint8_t readDataAsUInt8(size_t offset) const { return (offset < m_dataSize) ? m_data[offset] : 0; }
   uint16_t readDataAsUInt16(size_t offset) const { return (offset < m_dataSize - 1) ? CIP_UInt16Swap(*reinterpret_cast<uint16_t*>(&m_data[offset])) : 0; }
   uint32_t readDataAsUInt32(size_t offset) const { return (offset < m_dataSize - 3) ? CIP_UInt32Swap(*reinterpret_cast<uint32_t*>(&m_data[offset])) : 0; }
};

/**
 * EthernetIP message receiver
 */
class LIBETHERNETIP_EXPORTABLE EthernetIP_MessageReceiver
{
private:
   SOCKET m_socket;
   uint8_t *m_buffer;
   size_t m_allocated;
   size_t m_writePos;

public:
   EthernetIP_MessageReceiver(SOCKET s);
   ~EthernetIP_MessageReceiver();

   CIP_Message *readMessage(uint32_t timeout);
};

#endif   /* _ethernet_ip_h_ */
