/*
** NetXMS - Network Management System
** Copyright (C) 2003-2023 Victor Kirhenshtein
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
static inline uint16_t CIP_UInt16Swap(uint16_t value)
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
static inline uint32_t CIP_UInt32Swap(uint32_t value)
{
#if WORDS_BIGENDIAN
   return ((value & 0xFF) << 24) | ((value & 0xFF00) << 8) | ((value & 0xFF0000) >> 8) | (value >> 24);
#else
   return value;
#endif
}

#pragma pack(1)

/**
 * EIP encapsulation header
 */
struct EIP_EncapsulationHeader
{
   uint16_t command;
   uint16_t length;
   uint32_t sessionHandle;
   uint32_t status;
   BYTE context[8];
   uint32_t options;
};

#pragma pack()

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
 * CIP data types
 */
enum CIP_DataType
{
   CIP_DT_NULL = 0,
   CIP_DT_BOOL = 1,
   CIP_DT_SINT = 2,
   CIP_DT_INT = 3,
   CIP_DT_DINT = 4,
   CIP_DT_LINT = 5,
   CIP_DT_USINT = 6,
   CIP_DT_UINT = 7,
   CIP_DT_UDINT = 8,
   CIP_DT_ULINT = 9,
   CIP_DT_REAL = 10,
   CIP_DT_LREAL = 11,
   CIP_DT_ITIME = 12,
   CIP_DT_TIME = 13,
   CIP_DT_FTIME = 14,
   CIP_DT_LTIME = 15,
   CIP_DT_DATE = 16,
   CIP_DT_TIME_OF_DAY = 17,
   CIP_DT_DATE_AND_TIME = 18,
   CIP_DT_STRING = 19,
   CIP_DT_STRING2 = 20,
   CIP_DT_SHORT_STRING = 21,
   CIP_DT_BYTE = 22,
   CIP_DT_WORD = 23,
   CIP_DT_DWORD = 24,
   CIP_DT_LWORD = 25,
   CIP_DT_EPATH = 26,
   CIP_DT_ENGUNITS = 27,
   CIP_DT_MAC_ADDR = 28,
   CIP_DT_BYTE_ARRAY = 29
};

/**
 * CIP service codes
 */
enum CIP_ServiceCode : uint8_t
{
   CIP_Get_Attributes_All = 0x01,
   CIP_Set_Attributes_All = 0x02,
   CIP_Get_Attribute_List = 0x03,
   CIP_Set_Attribute_List = 0x04,
   CIP_Reset = 0x05,
   CIP_Start = 0x06,
   CIP_Stop = 0x07,
   CIP_Create = 0x08,
   CIP_Delete = 0x09,
   CIP_Multiple_Service_Packet = 0x0A,
   CIP_Apply_Attributes = 0x0D,
   CIP_Get_Attribute_Single = 0x0E,
   CIP_Set_Attribute_Single = 0x10,
   CIP_Find_Next_Object_Instance = 0x11,
   CIP_Error_Response = 0x14,
   CIP_Restore = 0x15,
   CIP_Save = 0x16,
   CIP_No_Operation = 0x17,
   CIP_Get_Member = 0x18,
   CIP_Set_Member = 0x19,
   CIP_Insert_Member = 0x1A,
   CIP_Remove_Member = 0x1B
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
 * EtherNet/IP protocol status codes
 */
enum EIP_ProtocolStatus : uint32_t
{
   EIP_STATUS_SUCCESS = 0x00,
   EIP_STATUS_INVALID_UNSUPPORTED = 0x01,
   EIP_STATUS_INSUFFICIENT_MEMORY = 0x02,
   EIP_STATUS_MALFORMED_DATA = 0x03,
   EIP_STATUS_INVALID_SESSION_HANDLE =0x64,
   EIP_STATUS_INVALID_LENGTH = 0x65,
   EIP_STATUS_UNSUPPORTED_PROTOCOL_VERSION = 0x69,
   EIP_STATUS_UNKNOWN = 0xFFFFFFFF
};

/**
 * EtherNet/IP helper call status
 */
enum EIP_CallStatus : uint8_t
{
   EIP_CALL_SUCCESS = 0,
   EIP_CALL_CONNECT_FAILED = 1,
   EIP_CALL_COMM_ERROR = 2,
   EIP_CALL_TIMEOUT = 3,
   EIP_CALL_BAD_RESPONSE = 4,
   EIP_INVALID_EPATH = 5,
   EIP_CALL_EIP_ERROR = 6,
   EIP_CALL_CIP_ERROR = 7
};

/**
 * CIP general status
 */
typedef uint8_t CIP_GeneralStatus;

/**
 * Status object
 */
struct LIBETHERNETIP_EXPORTABLE EIP_Status
{
   EIP_CallStatus callStatus;
   CIP_GeneralStatus cipGeneralStatus;
   EIP_ProtocolStatus protocolStatus;

   static EIP_Status success()
   {
      EIP_Status status;
      memset(&status, 0, sizeof(EIP_Status));
      return status;
   }

   static EIP_Status callFailure(EIP_CallStatus statusCode)
   {
      EIP_Status status;
      status.callStatus = statusCode;
      status.protocolStatus = EIP_STATUS_UNKNOWN;
      status.cipGeneralStatus = 0xFF;
      return status;
   }

   static EIP_Status protocolFailure(EIP_ProtocolStatus statusCode)
   {
      EIP_Status status;
      status.callStatus = EIP_CALL_SUCCESS;
      status.protocolStatus = statusCode;
      status.cipGeneralStatus = 0xFF;
      return status;
   }

   static EIP_Status deviceFailure(CIP_GeneralStatus statusCode)
   {
      EIP_Status status;
      status.callStatus = EIP_CALL_SUCCESS;
      status.protocolStatus = EIP_STATUS_SUCCESS;
      status.cipGeneralStatus = statusCode;
      return status;
   }

   bool isSuccess() const { return callStatus == EIP_CALL_SUCCESS; }
   String failureReason() const;
};

/**
 * EtherNet/IP session handle
 */
typedef uint32_t EIP_SessionHandle;

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
   size_t m_cpfStartOffset;
   size_t m_readOffset;
   size_t m_writeOffset;

public:
   EIP_Message(const uint8_t *networkData, size_t size);
   EIP_Message(EIP_Command command, size_t dataSize, EIP_SessionHandle handle = 0);
   ~EIP_Message();

   EIP_Command getCommand() const { return static_cast<EIP_Command>(CIP_UInt16Swap(m_header->command)); }
   EIP_SessionHandle getSessionHandle() const { return CIP_UInt32Swap(m_header->sessionHandle); }
   EIP_ProtocolStatus getStatus() const { return static_cast<EIP_ProtocolStatus>(CIP_UInt32Swap(m_header->status)); }
   size_t getSize() const { return m_dataSize + sizeof(EIP_EncapsulationHeader); }
   const uint8_t *getBytes() const { return m_bytes; }

   size_t getRawDataSize() const { return m_dataSize; }
   const uint8_t *getRawData() const { return m_data; }

   void prepareCPFRead(size_t startOffset);
   int getItemCount() const { return m_itemCount; }
   bool nextItem(CPF_Item *item);
   void resetItemReader() { m_readOffset = 0; }
   bool findItem(uint16_t type, CPF_Item *item);

   uint8_t readDataAsUInt8(size_t offset) const { return (offset < m_dataSize) ? m_data[offset] : 0; }
   uint16_t readDataAsUInt16(size_t offset) const
   {
      if (offset + 2 > m_dataSize)
         return 0;
      uint16_t v;
      memcpy(&v, &m_data[offset], 2);
      return CIP_UInt16Swap(v);
   }
   uint32_t readDataAsUInt32(size_t offset) const
   {
      if (offset + 4 > m_dataSize)
         return 0;
      uint32_t v;
      memcpy(&v, &m_data[offset], 4);
      return CIP_UInt32Swap(v);
   }
   InetAddress readDataAsInetAddress(size_t offset) const { return (offset < m_dataSize - 3) ? InetAddress(ntohl(*reinterpret_cast<uint32_t*>(&m_data[offset]))) : InetAddress(); }
   bool readDataAsLengthPrefixString(size_t offset, int prefixSize, TCHAR *buffer, size_t bufferSize) const;

   void resetWritePosition() { m_writeOffset = 0; }
   void writeData(const uint8_t *value, size_t size)
   {
      if (m_writeOffset + size <= m_dataSize)
      {
         memcpy(&m_data[m_writeOffset], value, size);
         m_writeOffset += size;
      }
   }
   void writeDataAsUInt8(uint8_t value)
   {
      if (m_writeOffset < m_dataSize)
         m_data[m_writeOffset++] = value;
   }
   void writeDataAsUInt16(uint16_t value)
   {
      if (m_writeOffset + 2 <= m_dataSize)
      {
         m_data[m_writeOffset++] = static_cast<uint8_t>(value & 0xFF);
         m_data[m_writeOffset++] = static_cast<uint8_t>(value >> 8);
      }
   }
   void writeDataAsUInt32(uint32_t value)
   {
      if (m_writeOffset + 4 <= m_dataSize)
      {
#if WORDS_BIGENDIAN
         m_data[m_writeOffset++] = static_cast<uint8_t>(value & 0xFF);         // Bits 0..7
         m_data[m_writeOffset++] = static_cast<uint8_t>((value >> 8) & 0xFF);  // Bits 8..15
         m_data[m_writeOffset++] = static_cast<uint8_t>((value >> 16) & 0xFF); // Bits 16..23
         m_data[m_writeOffset++] = static_cast<uint8_t>(value >> 24);          // Bits 24..31
#else
         memcpy(&m_data[m_writeOffset], &value, 4);
         m_writeOffset += 4;
#endif
      }
   }
   size_t advanceWritePosition(size_t bytes)
   {
      size_t curr = m_writeOffset;
      if (m_writeOffset + bytes <= m_dataSize)
         m_writeOffset += bytes;
      return curr;
   }
   void completeDataWrite()
   {
      m_dataSize = m_writeOffset;
      m_header->length = CIP_UInt16Swap(static_cast<uint16_t>(m_dataSize));
   }
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
 * EtherNet/IP session
 */
class LIBETHERNETIP_EXPORTABLE EIP_Session
{
private:
   EIP_SessionHandle m_handle;
   SOCKET m_socket;
   uint32_t m_timeout;
   bool m_connected;

   EIP_Session(EIP_SessionHandle handle, SOCKET socket, uint32_t timeout);

public:
   static EIP_Session *connect(const InetAddress& addr, uint16_t port, uint32_t timeout, EIP_Status *status);
   static EIP_Session *connect(SOCKET socket, uint32_t timeout, EIP_Status *status);

   ~EIP_Session();

   void disconnect();

   EIP_Status getAttribute(const TCHAR *path, void *buffer, size_t *size);
   EIP_Status getAttribute(uint32_t classId, uint32_t instance, uint32_t attributeId, void *buffer, size_t *size);
   EIP_Status getAttribute(const CIP_EPATH& path, void *buffer, size_t *size);

   EIP_SessionHandle getHandle() const { return m_handle; }
   SOCKET getSocket() const { return m_socket; }
   bool isConnected() const { return m_connected; }
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
CIP_Identity LIBETHERNETIP_EXPORTABLE *EIP_ListIdentity(const InetAddress& addr, uint16_t port, uint32_t timeout, EIP_Status *status);

/**** Path functions ****/
void LIBETHERNETIP_EXPORTABLE CIP_EncodeAttributePath(uint32_t classId, uint32_t instance, CIP_EPATH *path);
void LIBETHERNETIP_EXPORTABLE CIP_EncodeAttributePath(uint32_t classId, uint32_t instance, uint32_t attributeId, CIP_EPATH *path);
bool LIBETHERNETIP_EXPORTABLE CIP_EncodeAttributePathA(const char *symbolicPath, CIP_EPATH *path);
bool LIBETHERNETIP_EXPORTABLE CIP_EncodeAttributePathW(const WCHAR *symbolicPath, CIP_EPATH *path);
static inline bool CIP_EncodeAttributePath(const TCHAR *symbolicPath, CIP_EPATH *path)
{
#ifdef UNICODE
   return CIP_EncodeAttributePathW(symbolicPath, path);
#else
   return CIP_EncodeAttributePathA(symbolicPath, path);
#endif
}

bool LIBETHERNETIP_EXPORTABLE CIP_ParseSymbolicPathA(const char *symbolicPath, uint32_t *classId, uint32_t *instance, uint32_t *attributeId);
bool LIBETHERNETIP_EXPORTABLE CIP_ParseSymbolicPathW(const WCHAR *symbolicPath, uint32_t *classId, uint32_t *instance, uint32_t *attributeId);
static inline bool CIP_ParseSymbolicPath(const TCHAR *symbolicPath, uint32_t *classId, uint32_t *instance, uint32_t *attributeId)
{
#ifdef UNICODE
   return CIP_ParseSymbolicPathW(symbolicPath, classId, instance, attributeId);
#else
   return CIP_ParseSymbolicPathA(symbolicPath, classId, instance, attributeId);
#endif
}

String LIBETHERNETIP_EXPORTABLE CIP_DecodePath(const CIP_EPATH *path);

/**** Attribute function ****/
TCHAR LIBETHERNETIP_EXPORTABLE *CIP_DecodeAttribute(const uint8_t *data, size_t dataSize, uint32_t classId, uint32_t attributeId, TCHAR *buffer, size_t bufferSize);

/**** Utility functions ****/
const TCHAR LIBETHERNETIP_EXPORTABLE *CIP_VendorNameFromCode(int32_t code);
const TCHAR LIBETHERNETIP_EXPORTABLE *CIP_DeviceTypeNameFromCode(int32_t code);
const TCHAR LIBETHERNETIP_EXPORTABLE *CIP_DeviceStateTextFromCode(uint8_t state);
String LIBETHERNETIP_EXPORTABLE CIP_DecodeDeviceStatus(uint16_t status);
const TCHAR LIBETHERNETIP_EXPORTABLE *CIP_DecodeExtendedDeviceStatus(uint16_t status);
const TCHAR LIBETHERNETIP_EXPORTABLE *CIP_GeneralStatusTextFromCode(CIP_GeneralStatus status);
const TCHAR LIBETHERNETIP_EXPORTABLE *EIP_ProtocolStatusTextFromCode(EIP_ProtocolStatus status);
const TCHAR LIBETHERNETIP_EXPORTABLE *EIP_CallStatusTextFromCode(EIP_CallStatus status);

#endif   /* _ethernet_ip_h_ */
