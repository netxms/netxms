/*
** NetXMS - Network Management System
** Unit tests for EtherNet/IP support library
** Copyright (C) 2003-2026 Raden Solutions
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
** File: test-libethernetip.cpp
**
**/

#include <nms_common.h>
#include <nms_util.h>
#include <nms_threads.h>
#include <nxsocket.h>
#include <ethernet_ip.h>
#include <testtools.h>
#include <netxms-version.h>

NETXMS_EXECUTABLE_HEADER(test-libethernetip)

/**
 * Create message from encapsulation header fields and data. Returned message must be deleted by caller.
 */
static EIP_Message *CreateMessage(EIP_Command command, const uint8_t *data, size_t dataSize)
{
   size_t size = dataSize + sizeof(EIP_EncapsulationHeader);
   uint8_t *bytes = MemAllocArray<uint8_t>(size);
   bytes[0] = static_cast<uint8_t>(command & 0xFF);
   bytes[1] = static_cast<uint8_t>(command >> 8);
   bytes[2] = static_cast<uint8_t>(dataSize & 0xFF);
   bytes[3] = static_cast<uint8_t>((dataSize >> 8) & 0xFF);
   memcpy(&bytes[sizeof(EIP_EncapsulationHeader)], data, dataSize);
   EIP_Message *msg = new EIP_Message(bytes, size);
   MemFree(bytes);
   return msg;
}

/**
 * Decode attribute placed at deliberately unaligned address and compare result with expected text
 */
static void CheckAttribute(const uint8_t *data, size_t dataSize, uint32_t classId, uint32_t attributeId, const TCHAR *expected)
{
   uint8_t buffer[64];
   memset(buffer, 0xAA, sizeof(buffer));
   memcpy(&buffer[1], data, dataSize);
   TCHAR text[256];
   CIP_DecodeAttribute(&buffer[1], dataSize, classId, attributeId, text, 256);
   AssertEquals(text, expected);
}

/**
 * Encode path, decode it back, and compare with expected symbolic form
 */
static void CheckPathRoundTrip(uint32_t classId, uint32_t instance, uint32_t attributeId, size_t expectedSize, const TCHAR *expected)
{
   CIP_EPATH path;
   CIP_EncodeAttributePath(classId, instance, attributeId, &path);
   AssertEquals(path.size, static_cast<int>(expectedSize));
   AssertEquals(CIP_DecodePath(&path).cstr(), expected);
}

/**
 * Test encapsulation header layout. Header is mapped directly onto received bytes, so its size
 * must not change with compiler or platform.
 */
static void TestHeaderLayout()
{
   StartTest(_T("EIP_EncapsulationHeader layout"));
   AssertEquals(sizeof(EIP_EncapsulationHeader), 24);
   EndTest();
}

/**
 * Test encapsulation header encoding for outgoing messages. All multi-byte header fields are
 * little-endian on the wire regardless of host byte order, so raw bytes are checked directly.
 */
static void TestHeaderEncoding()
{
   StartTest(_T("EIP_Message - outgoing header byte order"));
   EIP_Message request(EIP_SEND_RR_DATA, 4, 0x12345678);
   request.completeDataWrite();
   const uint8_t *bytes = request.getBytes();

   // Command at offset 0 (EIP_SEND_RR_DATA = 0x006F)
   AssertEquals(static_cast<uint32_t>(bytes[0]), 0x6Fu);
   AssertEquals(static_cast<uint32_t>(bytes[1]), 0x00u);

   // Session handle at offset 4 must be little-endian on the wire
   AssertEquals(static_cast<uint32_t>(bytes[4]), 0x78u);
   AssertEquals(static_cast<uint32_t>(bytes[5]), 0x56u);
   AssertEquals(static_cast<uint32_t>(bytes[6]), 0x34u);
   AssertEquals(static_cast<uint32_t>(bytes[7]), 0x12u);

   // Header accessors must decode the same message back to host values
   EIP_Message *parsed = new EIP_Message(bytes, request.getSize());
   AssertEquals(static_cast<uint32_t>(parsed->getCommand()), static_cast<uint32_t>(EIP_SEND_RR_DATA));
   AssertEquals(parsed->getSessionHandle(), 0x12345678u);
   delete parsed;
   EndTest();
}

/**
 * Test symbolic path parsing
 */
static void TestSymbolicPathParsing()
{
   uint32_t classId, instance, attributeId;

   StartTest(_T("CIP_ParseSymbolicPath - decimal"));
   AssertTrue(CIP_ParseSymbolicPath(_T("1.2.3"), &classId, &instance, &attributeId));
   AssertEquals(classId, 1u);
   AssertEquals(instance, 2u);
   AssertEquals(attributeId, 3u);
   EndTest();

   StartTest(_T("CIP_ParseSymbolicPath - hexadecimal"));
   AssertTrue(CIP_ParseSymbolicPath(_T("0xF6.0x01.0x02"), &classId, &instance, &attributeId));
   AssertEquals(classId, 0xF6u);
   AssertEquals(instance, 1u);
   AssertEquals(attributeId, 2u);
   EndTest();

   StartTest(_T("CIP_ParseSymbolicPath - large values"));
   AssertTrue(CIP_ParseSymbolicPath(_T("70000.300.5"), &classId, &instance, &attributeId));
   AssertEquals(classId, 70000u);
   AssertEquals(instance, 300u);
   AssertEquals(attributeId, 5u);
   EndTest();

   StartTest(_T("CIP_ParseSymbolicPath - invalid input"));
   AssertFalse(CIP_ParseSymbolicPath(_T(""), &classId, &instance, &attributeId));
   AssertFalse(CIP_ParseSymbolicPath(_T("1.2"), &classId, &instance, &attributeId));
   AssertFalse(CIP_ParseSymbolicPath(_T("1.2.x"), &classId, &instance, &attributeId));
   AssertFalse(CIP_ParseSymbolicPath(_T("not a path"), &classId, &instance, &attributeId));
   EndTest();

   StartTest(_T("CIP_EncodeAttributePath - symbolic form"));
   CIP_EPATH path;
   AssertTrue(CIP_EncodeAttributePath(_T("1.1.7"), &path));
   AssertEquals(path.size, 6);
   AssertEquals(CIP_DecodePath(&path).cstr(), _T("1.1.7"));
   AssertFalse(CIP_EncodeAttributePath(_T("bad path"), &path));
   EndTest();
}

/**
 * Test path encoding and decoding. Segment width is selected by value magnitude, so all three
 * widths must be covered in every position.
 */
static void TestPathEncoding()
{
   StartTest(_T("CIP_EncodeAttributePath - 8 bit segments"));
   CheckPathRoundTrip(1, 1, 1, 6, _T("1.1.1"));
   CheckPathRoundTrip(0x01, 1, 6, 6, _T("1.1.6"));      // Identity serial number
   CheckPathRoundTrip(0xF6, 1, 2, 6, _T("246.1.2"));    // Ethernet Link interface flags
   CheckPathRoundTrip(255, 255, 255, 6, _T("255.255.255"));
   EndTest();

   StartTest(_T("CIP_EncodeAttributePath - 16 bit segments"));
   CheckPathRoundTrip(1, 256, 6, 8, _T("1.256.6"));
   CheckPathRoundTrip(1, 300, 6, 8, _T("1.300.6"));
   CheckPathRoundTrip(300, 1, 6, 8, _T("300.1.6"));
   CheckPathRoundTrip(1, 1, 300, 8, _T("1.1.300"));
   CheckPathRoundTrip(300, 400, 500, 12, _T("300.400.500"));
   EndTest();

   StartTest(_T("CIP_EncodeAttributePath - 32 bit segments"));
   CheckPathRoundTrip(1, 1, 65536, 10, _T("1.1.65536"));
   CheckPathRoundTrip(1, 1, 100000, 10, _T("1.1.100000"));
   CheckPathRoundTrip(70000, 300, 5, 12, _T("70000.300.5"));
   CheckPathRoundTrip(70000, 80000, 90000, 18, _T("70000.80000.90000"));
   EndTest();

   StartTest(_T("CIP_EncodeAttributePath - class and instance only"));
   CIP_EPATH path;
   CIP_EncodeAttributePath(0x01, 1, &path);
   AssertEquals(path.size, 4);
   AssertEquals(CIP_DecodePath(&path).cstr(), _T("1.1"));
   CIP_EncodeAttributePath(0x01, 300, &path);
   AssertEquals(path.size, 6);
   AssertEquals(CIP_DecodePath(&path).cstr(), _T("1.300"));
   EndTest();

   StartTest(_T("CIP_EncodeAttributePath - segment encoding"));
   CIP_EncodeAttributePath(0x01, 1, 7, &path);
   static const uint8_t expectedBytes[] = { 0x20, 0x01, 0x24, 0x01, 0x30, 0x07 };
   AssertEquals(path.size, 6);
   AssertTrue(!memcmp(path.value, expectedBytes, sizeof(expectedBytes)));
   EndTest();
}

/**
 * Test path decoding for inputs that cannot be produced by the encoder
 */
static void TestPathDecoding()
{
   CIP_EPATH path;

   StartTest(_T("CIP_DecodePath - empty path"));
   memset(&path, 0, sizeof(path));
   AssertEquals(CIP_DecodePath(&path).cstr(), _T(""));
   EndTest();

   StartTest(_T("CIP_DecodePath - truncated 16 bit segment"));
   memset(&path, 0, sizeof(path));
   path.value[0] = 0x20;   // 8 bit logical segment
   path.value[1] = 0x01;
   path.value[2] = 0x25;   // 16 bit logical segment, value bytes missing
   path.value[3] = 0x00;
   path.size = 4;
   AssertEquals(CIP_DecodePath(&path).cstr(), _T("1"));
   EndTest();

   StartTest(_T("CIP_DecodePath - truncated 32 bit segment"));
   memset(&path, 0, sizeof(path));
   path.value[0] = 0x20;
   path.value[1] = 0x01;
   path.value[2] = 0x26;   // 32 bit logical segment, value bytes missing
   path.value[3] = 0x00;
   path.value[4] = 0x01;
   path.size = 5;
   AssertEquals(CIP_DecodePath(&path).cstr(), _T("1"));
   EndTest();

   StartTest(_T("CIP_DecodePath - unsupported segment format"));
   memset(&path, 0, sizeof(path));
   path.value[0] = 0x20;
   path.value[1] = 0x01;
   path.value[2] = 0x23;   // reserved format, parsing must stop
   path.value[3] = 0x00;
   path.size = 4;
   AssertEquals(CIP_DecodePath(&path).cstr(), _T("1"));
   EndTest();
}

/**
 * Test attribute decoding. All values are placed at an unaligned address, because attribute data
 * is decoded in place from the body of received message.
 */
static void TestAttributeDecoding()
{
   StartTest(_T("CIP_DecodeAttribute - Identity object"));
   static const uint8_t vendor[] = { 0x4D, 0x00 };
   CheckAttribute(vendor, sizeof(vendor), 0x01, 1, _T("77"));                    // UINT
   static const uint8_t deviceType[] = { 0x0C, 0x00 };
   CheckAttribute(deviceType, sizeof(deviceType), 0x01, 2, _T("12"));            // UINT
   static const uint8_t status[] = { 0x34, 0x12 };
   CheckAttribute(status, sizeof(status), 0x01, 5, _T("1234"));                  // WORD
   static const uint8_t serialNumber[] = { 0x78, 0x56, 0x34, 0x12 };
   CheckAttribute(serialNumber, sizeof(serialNumber), 0x01, 6, _T("305419896")); // UDINT
   static const uint8_t productName[] = { 5, 'M', '3', '4', '0', 'S' };
   CheckAttribute(productName, sizeof(productName), 0x01, 7, _T("M340S"));       // SHORT_STRING
   static const uint8_t state[] = { 0x03 };
   CheckAttribute(state, sizeof(state), 0x01, 8, _T("3"));                       // USINT
   EndTest();

   StartTest(_T("CIP_DecodeAttribute - Ethernet Link object"));
   static const uint8_t speed[] = { 0xE8, 0x03, 0x00, 0x00 };
   CheckAttribute(speed, sizeof(speed), 0xF6, 1, _T("1000"));                    // UDINT
   static const uint8_t flags[] = { 0x0F, 0x00, 0x00, 0x80 };
   CheckAttribute(flags, sizeof(flags), 0xF6, 2, _T("8000000F"));                // DWORD
   static const uint8_t macAddr[] = { 0x00, 0x1D, 0x9C, 0x01, 0x02, 0x03 };
   CheckAttribute(macAddr, sizeof(macAddr), 0xF6, 3, _T("00:1D:9C:01:02:03"));   // MAC_ADDR
   EndTest();

   // Attributes of classes that are not described in the library are typed by data size
   StartTest(_T("CIP_DecodeAttribute - type guessed from size"));
   static const uint8_t sint[] = { 0xFE };
   CheckAttribute(sint, sizeof(sint), 0x99, 1, _T("254"));
   static const uint8_t intNegative[] = { 0xFE, 0xFF };
   CheckAttribute(intNegative, sizeof(intNegative), 0x99, 1, _T("-2"));
   static const uint8_t intPositive[] = { 0x34, 0x12 };
   CheckAttribute(intPositive, sizeof(intPositive), 0x99, 1, _T("4660"));
   static const uint8_t dintNegative[] = { 0xFF, 0xFF, 0xFF, 0xFF };
   CheckAttribute(dintNegative, sizeof(dintNegative), 0x99, 1, _T("-1"));
   static const uint8_t dintPositive[] = { 0x78, 0x56, 0x34, 0x12 };
   CheckAttribute(dintPositive, sizeof(dintPositive), 0x99, 1, _T("305419896"));
   static const uint8_t mac[] = { 0x00, 0x1D, 0x9C, 0x0A, 0x0B, 0x0C };
   CheckAttribute(mac, sizeof(mac), 0x99, 1, _T("00:1D:9C:0A:0B:0C"));
   static const uint8_t lintPositive[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00 };
   CheckAttribute(lintPositive, sizeof(lintPositive), 0x99, 1, _T("4294967295"));
   static const uint8_t lintNegative[] = { 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
   CheckAttribute(lintNegative, sizeof(lintNegative), 0x99, 1, _T("-2"));
   static const uint8_t byteArray[] = { 0x01, 0x02, 0x03 };
   CheckAttribute(byteArray, sizeof(byteArray), 0x99, 1, _T("010203"));
   EndTest();

   StartTest(_T("CIP_DecodeAttribute - insufficient buffer"));
   TCHAR text[256];
   static const uint8_t longArray[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 };
   CIP_DecodeAttribute(longArray, sizeof(longArray), 0x99, 1, text, 4);
   AssertEquals(text, _T(""));
   CIP_DecodeAttribute(mac, sizeof(mac), 0xF6, 3, text, 8);
   AssertEquals(text, _T(""));
   EndTest();
}

/**
 * Test message data readers
 */
static void TestMessageReaders()
{
   static const uint8_t data[] =
   {
      0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
      0x7F, 0x00, 0x00, 0x01,                              // 127.0.0.1 in network byte order
      0x03, 'a', 'b', 'c'                                  // length prefixed string
   };
   EIP_Message *msg = CreateMessage(EIP_LIST_IDENTITY, data, sizeof(data));

   StartTest(_T("EIP_Message - header accessors"));
   AssertEquals(static_cast<uint32_t>(msg->getCommand()), static_cast<uint32_t>(EIP_LIST_IDENTITY));
   AssertEquals(static_cast<uint32_t>(msg->getStatus()), static_cast<uint32_t>(EIP_STATUS_SUCCESS));
   AssertEquals(static_cast<uint32_t>(msg->getSessionHandle()), 0u);
   AssertEquals(msg->getRawDataSize(), static_cast<int>(sizeof(data)));
   AssertEquals(msg->getSize(), static_cast<int>(sizeof(data) + sizeof(EIP_EncapsulationHeader)));
   EndTest();

   StartTest(_T("EIP_Message::readDataAsUInt8"));
   AssertEquals(static_cast<uint32_t>(msg->readDataAsUInt8(0)), 0x01u);
   AssertEquals(static_cast<uint32_t>(msg->readDataAsUInt8(7)), 0x08u);
   AssertEquals(static_cast<uint32_t>(msg->readDataAsUInt8(sizeof(data))), 0u);   // out of bounds
   EndTest();

   StartTest(_T("EIP_Message::readDataAsUInt16"));
   AssertEquals(static_cast<uint32_t>(msg->readDataAsUInt16(0)), 0x0201u);
   AssertEquals(static_cast<uint32_t>(msg->readDataAsUInt16(1)), 0x0302u);        // unaligned
   AssertEquals(static_cast<uint32_t>(msg->readDataAsUInt16(sizeof(data) - 1)), 0u);
   AssertEquals(static_cast<uint32_t>(msg->readDataAsUInt16(sizeof(data))), 0u);
   EndTest();

   StartTest(_T("EIP_Message::readDataAsUInt32"));
   AssertEquals(msg->readDataAsUInt32(0), 0x04030201u);
   AssertEquals(msg->readDataAsUInt32(1), 0x05040302u);                          // unaligned
   AssertEquals(msg->readDataAsUInt32(sizeof(data) - 3), 0u);
   AssertEquals(msg->readDataAsUInt32(sizeof(data)), 0u);
   EndTest();

   StartTest(_T("EIP_Message::readDataAsInetAddress"));
   TCHAR addrText[64];
   AssertEquals(msg->readDataAsInetAddress(8).toString(addrText), _T("127.0.0.1"));
   AssertFalse(msg->readDataAsInetAddress(sizeof(data) - 3).isValid());
   AssertFalse(msg->readDataAsInetAddress(sizeof(data)).isValid());
   AssertFalse(msg->readDataAsInetAddress(sizeof(data) + 100).isValid());
   EndTest();

   StartTest(_T("EIP_Message::readDataAsNetworkOrderUInt16"));
   AssertEquals(static_cast<uint32_t>(msg->readDataAsNetworkOrderUInt16(0)), 0x0102u);
   AssertEquals(static_cast<uint32_t>(msg->readDataAsNetworkOrderUInt16(1)), 0x0203u);   // unaligned
   AssertEquals(static_cast<uint32_t>(msg->readDataAsNetworkOrderUInt16(sizeof(data) - 1)), 0u);
   AssertEquals(static_cast<uint32_t>(msg->readDataAsNetworkOrderUInt16(sizeof(data))), 0u);
   EndTest();

   StartTest(_T("EIP_Message::readDataAsLengthPrefixString"));
   TCHAR text[64];
   AssertTrue(msg->readDataAsLengthPrefixString(12, 1, text, 64));
   AssertEquals(text, _T("abc"));
   AssertFalse(msg->readDataAsLengthPrefixString(sizeof(data) - 1, 1, text, 64));
   EndTest();

   delete msg;

   // Message with data shorter than a single address. Bound check must reject the read instead of
   // wrapping around and reading past the end of allocated block.
   StartTest(_T("EIP_Message::readDataAsInetAddress - undersized message"));
   static const uint8_t shortData[] = { 0x7F, 0x00, 0x00, 0x01 };
   for(size_t dataSize = 0; dataSize < sizeof(shortData); dataSize++)
   {
      EIP_Message *shortMessage = CreateMessage(EIP_LIST_IDENTITY, shortData, dataSize);
      AssertEquals(shortMessage->getRawDataSize(), static_cast<int>(dataSize));
      AssertFalse(shortMessage->readDataAsInetAddress(0).isValid());
      delete shortMessage;
   }
   EndTest();
}

/**
 * Test CPF item reader
 */
static void TestCPFItemReader()
{
   // Two items: first one is not a socket address item and has odd length, so the second item
   // starts at an odd offset within message data
   static const uint8_t data[] =
   {
      0x02, 0x00,                                          // item count
      0x01, 0x00, 0x03, 0x00, 0xAA, 0xBB, 0xCC,            // item 0: type 0x0001, length 3
      0x0C, 0x00, 0x08, 0x00,                              // item 1: type 0x000C, length 8
      0x01, 0x00,                                          // encapsulation protocol version
      0x00, 0x02,                                          // sin_family
      0x00, 0x00,                                          // sin_port
      0x7F, 0x00, 0x00, 0x01                               // sin_addr - 127.0.0.1
   };
   EIP_Message *msg = CreateMessage(EIP_LIST_IDENTITY, data, sizeof(data));
   msg->prepareCPFRead(0);

   StartTest(_T("EIP_Message::getItemCount"));
   AssertEquals(msg->getItemCount(), 2);
   EndTest();

   StartTest(_T("EIP_Message::nextItem"));
   CPF_Item item;
   AssertTrue(msg->nextItem(&item));
   AssertEquals(static_cast<uint32_t>(item.type), 0x0001u);
   AssertEquals(static_cast<uint32_t>(item.length), 3u);
   AssertEquals(item.offset, 6u);
   AssertTrue(msg->nextItem(&item));
   AssertEquals(static_cast<uint32_t>(item.type), 0x000Cu);
   AssertEquals(static_cast<uint32_t>(item.length), 8u);
   AssertEquals(item.offset, 13u);
   AssertFalse(msg->nextItem(&item));
   EndTest();

   StartTest(_T("EIP_Message::findItem"));
   AssertTrue(msg->findItem(0x000C, &item));
   AssertEquals(item.offset, 13u);
   AssertFalse(msg->findItem(0x00B2, &item));
   EndTest();

   // Socket address within second item starts at odd offset 19
   StartTest(_T("EIP_Message::readDataAsInetAddress - odd offset"));
   AssertTrue(msg->findItem(0x000C, &item));
   TCHAR addrText[64];
   AssertEquals(msg->readDataAsInetAddress(item.offset + 6).toString(addrText), _T("127.0.0.1"));
   EndTest();

   delete msg;

   StartTest(_T("EIP_Message::nextItem - truncated item"));
   static const uint8_t truncated[] =
   {
      0x01, 0x00,                                          // item count
      0x0C, 0x00, 0x20, 0x00,                              // type 0x000C, length 32 - not present
      0x01, 0x00
   };
   EIP_Message *truncatedMessage = CreateMessage(EIP_LIST_IDENTITY, truncated, sizeof(truncated));
   truncatedMessage->prepareCPFRead(0);
   CPF_Item truncatedItem;
   AssertFalse(truncatedMessage->nextItem(&truncatedItem));
   delete truncatedMessage;
   EndTest();
}

/**
 * Test code to text conversions
 */
static void TestCodeToText()
{
   StartTest(_T("CIP_VendorNameFromCode"));
   AssertEquals(CIP_VendorNameFromCode(1), _T("Rockwell Automation/Allen-Bradley"));
   AssertEquals(CIP_VendorNameFromCode(3), _T("Honeywell Inc."));
   AssertEquals(CIP_VendorNameFromCode(-1), _T("Unknown"));
   AssertEquals(CIP_VendorNameFromCode(0x7FFFFFFF), _T("Unknown"));
   EndTest();

   StartTest(_T("CIP_DeviceTypeNameFromCode"));
   AssertEquals(CIP_DeviceTypeNameFromCode(0), _T("Generic Device"));
   AssertEquals(CIP_DeviceTypeNameFromCode(2), _T("AC Drive Device"));
   AssertEquals(CIP_DeviceTypeNameFromCode(0x7FFFFFFF), _T("Unknown"));
   EndTest();

   StartTest(_T("CIP_DeviceStateTextFromCode"));
   AssertEquals(CIP_DeviceStateTextFromCode(0), _T("Non Existent"));
   AssertEquals(CIP_DeviceStateTextFromCode(3), _T("Operational"));
   AssertEquals(CIP_DeviceStateTextFromCode(255), _T("Unknown"));
   EndTest();

   StartTest(_T("CIP_GeneralStatusTextFromCode"));
   AssertEquals(CIP_GeneralStatusTextFromCode(0x00), _T("Success"));
   AssertEquals(CIP_GeneralStatusTextFromCode(0x04), _T("Path segment error"));
   AssertEquals(CIP_GeneralStatusTextFromCode(0xFE), _T("Unknown"));
   EndTest();

   StartTest(_T("CIP_DecodeDeviceStatus"));
   AssertEquals(CIP_DecodeDeviceStatus(0).cstr(), _T(""));
   AssertEquals(CIP_DecodeDeviceStatus(CIP_DEVICE_STATUS_OWNED).cstr(), _T("Owned"));
   AssertEquals(CIP_DecodeDeviceStatus(CIP_DEVICE_STATUS_OWNED | CIP_DEVICE_STATUS_CONFIGURED).cstr(), _T("Owned, Configured"));
   AssertEquals(CIP_DecodeDeviceStatus(CIP_DEVICE_STATUS_CONFIGURED | CIP_DEVICE_STATUS_MAJOR_UNRECOVERABLE_FAULT).cstr(),
            _T("Configured, Major Unrecoverable Fault"));
   EndTest();

   StartTest(_T("EIP_ProtocolStatusTextFromCode"));
   AssertEquals(EIP_ProtocolStatusTextFromCode(EIP_STATUS_SUCCESS), _T("SUCCESS"));
   AssertEquals(EIP_ProtocolStatusTextFromCode(static_cast<EIP_ProtocolStatus>(0xFFFF)), _T("UNKNOWN"));
   EndTest();

   StartTest(_T("EIP_Status"));
   AssertTrue(EIP_Status::success().isSuccess());
   AssertFalse(EIP_Status::callFailure(EIP_CALL_TIMEOUT).isSuccess());
   AssertEquals(EIP_Status::callFailure(EIP_CALL_TIMEOUT).failureReason().cstr(), EIP_CallStatusTextFromCode(EIP_CALL_TIMEOUT));
   EndTest();
}

/**
 * Identity data returned by test server
 */
static const char *s_testProductName = "NetXMS Test Device";
#define TEST_SERIAL_NUMBER    0x12345678u
#define TEST_PRODUCT_CODE     5000u

/**
 * Send canned ListIdentity response to single client
 */
static void ListIdentityServer(SOCKET listenSocket)
{
   SOCKET s = accept(listenSocket, nullptr, nullptr);
   if (s == INVALID_SOCKET)
      return;

   // Read request - encapsulation header without data
   uint8_t request[64];
   RecvEx(s, request, sizeof(request), 0, 5000);

   size_t nameLength = strlen(s_testProductName);
   size_t itemLength = 34 + nameLength;      // identity fields, name length prefix, name, state
   size_t dataSize = itemLength + 6;         // item count and item header
   uint8_t response[256];
   memset(response, 0, sizeof(response));

   response[0] = EIP_LIST_IDENTITY & 0xFF;
   response[1] = EIP_LIST_IDENTITY >> 8;
   response[2] = static_cast<uint8_t>(dataSize & 0xFF);
   response[3] = static_cast<uint8_t>((dataSize >> 8) & 0xFF);

   uint8_t *data = &response[sizeof(EIP_EncapsulationHeader)];
   data[0] = 1;                              // item count
   data[2] = 0x0C;                           // item type 0x000C
   data[4] = static_cast<uint8_t>(itemLength & 0xFF);
   data[5] = static_cast<uint8_t>((itemLength >> 8) & 0xFF);

   // Socket address fields are in network byte order, all other fields are little endian
   uint8_t *item = &data[6];
   item[0] = 1;                              // encapsulation protocol version
   item[3] = 2;                              // sin_family
   item[4] = 0xAF;                           // sin_port - 44818
   item[5] = 0x12;
   item[6] = 127;                            // sin_addr - 127.0.0.1
   item[9] = 1;
   item[18] = 1;                             // vendor
   item[20] = 2;                             // device type
   item[22] = static_cast<uint8_t>(TEST_PRODUCT_CODE & 0xFF);
   item[23] = static_cast<uint8_t>(TEST_PRODUCT_CODE >> 8);
   item[24] = 3;                             // product revision major
   item[25] = 17;                            // product revision minor
   item[26] = CIP_DEVICE_STATUS_OWNED | CIP_DEVICE_STATUS_CONFIGURED;
   item[28] = static_cast<uint8_t>(TEST_SERIAL_NUMBER & 0xFF);   // serial number is 32 bit value
   item[29] = static_cast<uint8_t>((TEST_SERIAL_NUMBER >> 8) & 0xFF);
   item[30] = static_cast<uint8_t>((TEST_SERIAL_NUMBER >> 16) & 0xFF);
   item[31] = static_cast<uint8_t>(TEST_SERIAL_NUMBER >> 24);
   item[32] = static_cast<uint8_t>(nameLength);
   memcpy(&item[33], s_testProductName, nameLength);
   item[33 + nameLength] = 3;                // device state - operational

   SendEx(s, response, dataSize + sizeof(EIP_EncapsulationHeader), 0, nullptr);
   shutdown(s, SHUT_RDWR);
   closesocket(s);
}

/**
 * Test identity reading against local test server
 */
static void TestListIdentity()
{
   SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, 0);
   AssertTrue(listenSocket != INVALID_SOCKET);
   SetSocketReuseFlag(listenSocket);

   struct sockaddr_in localAddr;
   memset(&localAddr, 0, sizeof(localAddr));
   localAddr.sin_family = AF_INET;
   localAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
   localAddr.sin_port = 0;   // any available port
   AssertTrue(bind(listenSocket, reinterpret_cast<struct sockaddr*>(&localAddr), static_cast<socklen_t>(sizeof(localAddr))) == 0);
   AssertTrue(listen(listenSocket, 1) == 0);

   socklen_t addrLen = static_cast<socklen_t>(sizeof(localAddr));
   AssertTrue(getsockname(listenSocket, reinterpret_cast<struct sockaddr*>(&localAddr), &addrLen) == 0);
   uint16_t port = ntohs(localAddr.sin_port);

   THREAD thread = ThreadCreateEx(ListIdentityServer, listenSocket);

   StartTest(_T("EIP_ListIdentity"));
   EIP_Status status;
   CIP_Identity *identity = EIP_ListIdentity(InetAddress::LOOPBACK, port, 5000, &status);
   AssertNotNullEx(identity, status.failureReason().cstr());
   AssertTrue(status.isSuccess());

   AssertEquals(static_cast<uint32_t>(identity->vendor), 1u);
   AssertEquals(static_cast<uint32_t>(identity->deviceType), 2u);
   AssertEquals(static_cast<uint32_t>(identity->productCode), TEST_PRODUCT_CODE);
   AssertEquals(static_cast<uint32_t>(identity->productRevisionMajor), 3u);
   AssertEquals(static_cast<uint32_t>(identity->productRevisionMinor), 17u);
   AssertEquals(static_cast<uint32_t>(identity->protocolVersion), 1u);

   // TCP port is part of socket address structure, so it is in network byte order on the wire
   AssertEquals(static_cast<uint32_t>(identity->tcpPort), 44818u);

   AssertEquals(static_cast<uint32_t>(identity->status),
            static_cast<uint32_t>(CIP_DEVICE_STATUS_OWNED | CIP_DEVICE_STATUS_CONFIGURED));
   AssertEquals(static_cast<uint32_t>(identity->state), 3u);
   AssertEquals(identity->productName, _T("NetXMS Test Device"));

   // Serial number is a 32 bit value, high 16 bits must be preserved
   AssertEquals(identity->serialNumber, TEST_SERIAL_NUMBER);

   TCHAR addrText[64];
   AssertEquals(identity->ipAddress.toString(addrText), _T("127.0.0.1"));

   MemFree(identity);
   EndTest();

   ThreadJoin(thread);
   closesocket(listenSocket);
}

/**
 * main()
 */
int main(int argc, char *argv[])
{
   InitNetXMSProcess(true);

#ifdef _WIN32
   WSADATA wsaData;
   if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
   {
      _tprintf(_T("WSAStartup failed\n"));
      return 1;
   }
#endif

   TestHeaderLayout();
   TestHeaderEncoding();
   TestSymbolicPathParsing();
   TestPathEncoding();
   TestPathDecoding();
   TestAttributeDecoding();
   TestMessageReaders();
   TestCPFItemReader();
   TestCodeToText();
   TestListIdentity();
   return 0;
}
