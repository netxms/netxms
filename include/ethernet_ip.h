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
 * Default TCP port
 */
#define ETHERNET_IP_DEFAULT_PORT    44818

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
 * EIP encapsulation header
 */
struct EIP_EncapsulationHeader
{
   uint16_t command;
   uint16_t length;
   uint32_t sessionHandle;
   uint32_t status;
   uint64_t context;
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
 * EtherNet/IP encapsulation commands
 */
enum EIP_Command : uint16_t
{
   EIP_NOOP = 0x0000,
   EIP_LIST_SERVICES = 0x0004,
   EIP_LIST_IDENTITY = 0x0063,
   EIP_LIST_INTERFACES = 0x0064,
   EIP_REGISTER_SESSION = 0x0065,
   EIP_UNREGISTER_SESSION = 0x0066,
   EIP_SEND_RR_DATA = 0x006F,
   EIP_SEND_UNIT_DATA = 0x0070,
   EIP_INDICATE_STATUS = 0x0072,
   EIP_CANCEL = 0x0073
};

/**
 * CIP device state
 */
enum CIP_DeviceState
{
   CIP_DEVICE_STATE_NON_EXISTENT = 0,
   CIP_DEVICE_STATE_SELF_TESTING = 1,
   CIP_DEVICE_STATE_STANDBY = 2,
   CIP_DEVICE_STATE_OPERATIONAL = 3,
   CIP_DEVICE_STATE_MAJOR_RECOVERABLE_FAULT = 4,
   CIP_DEVICE_STATE_MAJOR_UNRECOVERABLE_FAULT = 5
};

/**
 * CIP device status
 */
enum CIP_DeviceStatus : uint16_t
{
   CIP_DEVICE_STATUS_OWNED = 0x0001,
   CIP_DEVICE_STATUS_CONFIGURED = 0x0004,
   CIP_DEVICE_STATUS_MINOR_RECOVERABLE_FAULT = 0x0100,
   CIP_DEVICE_STATUS_MINOR_UNRECOVERABLE_FAULT = 0x0200,
   CIP_DEVICE_STATUS_MAJOR_RECOVERABLE_FAULT = 0x0400,
   CIP_DEVICE_STATUS_MAJOR_UNRECOVERABLE_FAULT = 0x0800,
   CIP_DEVICE_STATUS_EXTENDED_STATUS_MASK = 0x00F0
};

/**
 * CIP extended device status
 */
enum CIP_ExtendedDeviceStatus
{
   CIP_EXT_DEVICE_STATUS_UNKNOWN = 0,
   CIP_EXT_DEVICE_STATUS_FIRMWARE_UPDATE = 1,
   CIP_EXT_DEVICE_STATUS_IO_FAULT = 2,
   CIP_EXT_DEVICE_STATUS_NO_IO_CONNECTION = 3,
   CIP_EXT_DEVICE_STATUS_BAD_NVRAM = 4,
   CIP_EXT_DEVICE_STATUS_MAJOR_FAULT = 5,
   CIP_EXT_DEVICE_STATUS_IO_RUN = 6,
   CIP_EXT_DEVICE_STATUS_IO_IDLE = 7
};

/**
 * EtherNet/IP status codes
 */
enum EIP_Status : uint32_t
{
   EIP_STATUS_SUCCESS = 0x00,
   EIP_STATUS_INVALID_UNSUPPORTED = 0x01,
   EIP_STATUS_INSUFFICIENT_MEMORY = 0x02,
   EIP_STATUS_MALFORMED_DATA = 0x03,
   EIP_STATUS_INVALID_SESSION_HANDLE =0x64,
   EIP_STATUS_INVALID_LENGTH = 0x65,
   EIP_STATUS_UNSUPPORTED_PROTOCOL_VERSION = 0x69
};

/**
 * EtherNet/IP helper call status
 */
enum EIP_CallStatus
{
   EIP_CALL_SUCCESS = 0,
   EIP_CALL_CONNECT_FAILED = 1,
   EIP_CALL_COMM_ERROR = 2,
   EIP_CALL_TIMEOUT = 3,
   EIP_CALL_BAD_RESPONSE = 4,
   EIP_CALL_DEVICE_ERROR = 5
};

/**
 * EPATH
 */
struct CIP_EPATH
{
   size_t size;         // Actual path size in bytes
   uint8_t value[24];   // Max 4 32 bit segments
};

/**
 * Common Packet Format item
 */
struct CPF_Item
{
   uint16_t type;
   uint16_t length;
   uint32_t offset;  // Item's data offset within message data
   const uint8_t *data;
};

/**
 * EtherNet/IP message
 */
class LIBETHERNETIP_EXPORTABLE EIP_Message
{
private:
   uint8_t *m_bytes;
   EIP_EncapsulationHeader *m_header;
   size_t m_dataSize;
   uint8_t *m_data;
   int m_itemCount;
   size_t m_readOffset;

public:
   EIP_Message(const uint8_t *networkData, size_t size);
   EIP_Message(EIP_Command command, size_t dataSize);
   ~EIP_Message();

   EIP_Command getCommand() const { return static_cast<EIP_Command>(CIP_UInt16Swap(m_header->command)); }
   uint32_t getSessionHandle() const { return CIP_UInt32Swap(m_header->sessionHandle); }
   EIP_Status getStatus() const { return static_cast<EIP_Status>(CIP_UInt32Swap(m_header->status)); }
   size_t getSize() const { return m_dataSize + sizeof(EIP_EncapsulationHeader); }
   const uint8_t *getBytes() const { return m_bytes; }

   size_t getRawDataSize() const { return m_dataSize; }
   const uint8_t *getRawData() const { return m_data; }
   int getItemCount() const { return m_itemCount; }

   bool nextItem(CPF_Item *item);
   void resetItemReader() { m_readOffset = 0; }

   uint8_t readDataAsUInt8(size_t offset) const { return (offset < m_dataSize) ? m_data[offset] : 0; }
   uint16_t readDataAsUInt16(size_t offset) const
   {
      if (offset >= m_dataSize - 1)
         return 0;
      uint16_t v;
      memcpy(&v, &m_data[offset], 2);
      return CIP_UInt16Swap(v);
   }
   uint32_t readDataAsUInt32(size_t offset) const
   {
      if (offset >= m_dataSize - 3)
         return 0;
      uint32_t v;
      memcpy(&v, &m_data[offset], 4);
      return CIP_UInt32Swap(v);
   }
   InetAddress readDataAsInetAddress(size_t offset) const { return (offset < m_dataSize - 3) ? InetAddress(ntohl(*reinterpret_cast<uint32_t*>(&m_data[offset]))) : InetAddress(); }
   bool readDataAsLengthPrefixString(size_t offset, TCHAR *buffer, size_t bufferSize) const;
};

/**
 * EtherNet/IP message receiver
 */
class LIBETHERNETIP_EXPORTABLE EIP_MessageReceiver
{
private:
   SOCKET m_socket;
   uint8_t *m_buffer;
   size_t m_allocated;
   size_t m_dataSize;
   size_t m_readPos;

   EIP_Message *readMessageFromBuffer();

public:
   EIP_MessageReceiver(SOCKET s);
   ~EIP_MessageReceiver();

   EIP_Message *readMessage(uint32_t timeout);
};

/**
 * CIP identity data
 */
struct CIP_Identity
{
   uint16_t vendor;
   uint16_t deviceType;
   uint16_t productCode;
   uint8_t productRevisionMajor;
   uint8_t productRevisionMinor;
   uint32_t serialNumber;
   TCHAR *productName;
   InetAddress ipAddress;
   uint16_t tcpPort;
   uint16_t protocolVersion;
   uint16_t status;
   uint8_t state;
};

/**
 * Helper function for reading device identity via Ethernet/IP
 */
CIP_Identity LIBETHERNETIP_EXPORTABLE *EIP_ListIdentity(const InetAddress& addr, uint16_t port, uint32_t timeout, EIP_CallStatus *callStatus, EIP_Status *eipStatus);

/**** Path functions ****/
void LIBETHERNETIP_EXPORTABLE CIP_EncodeAttributePath(uint32_t classId, uint32_t instance, uint32_t attributeId, CIP_EPATH *path);
bool LIBETHERNETIP_EXPORTABLE CIP_EncodeAttributePathA(const char *symbolicPath, CIP_EPATH *path);
bool LIBETHERNETIP_EXPORTABLE CIP_EncodeAttributePathW(const WCHAR *symbolicPath, CIP_EPATH *path);
#ifdef UNICODE
#define CIP_EncodeAttributePath CIP_EncodeAttributePathW
#else
#define CIP_EncodeAttributePath CIP_EncodeAttributePathA
#endif

/**** Utility functions ****/
const TCHAR LIBETHERNETIP_EXPORTABLE *CIP_VendorNameFromCode(int32_t code);
const TCHAR LIBETHERNETIP_EXPORTABLE *CIP_DeviceTypeNameFromCode(int32_t code);
const TCHAR LIBETHERNETIP_EXPORTABLE *CIP_DeviceStateTextFromCode(uint8_t state);
const TCHAR LIBETHERNETIP_EXPORTABLE *EIP_StatusTextFromCode(EIP_Status status);
String LIBETHERNETIP_EXPORTABLE CIP_DecodeDeviceStatus(uint16_t status);
const TCHAR LIBETHERNETIP_EXPORTABLE *CIP_DecodeExtendedDeviceStatus(uint16_t status);

#endif   /* _ethernet_ip_h_ */
