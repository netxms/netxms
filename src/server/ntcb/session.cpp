/*
** NetXMS - Network Management System
** Copyright (C) 2003-2020 Raden Solutions
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: session.cpp
**
**/

#include "ntcb.h"

#define DEBUG_TAG_NTCB_SESSION   DEBUG_TAG_NTCB _T(".session")

/**
 * Unregister session
 */
void UnregisterNTCBDeviceSession(session_id_t id);

/**
 * Socket timeout for NTCB
 */
uint32_t g_ntcbSocketTimeout = 60000;

/**
 * Table for CRC8 calculation
 */
static const uint8_t crc8Table[256] =
{
   0x00, 0x31, 0x62, 0x53, 0xC4, 0xF5, 0xA6, 0x97,
   0xB9, 0x88, 0xDB, 0xEA, 0x7D, 0x4C, 0x1F, 0x2E,
   0x43, 0x72, 0x21, 0x10, 0x87, 0xB6, 0xE5, 0xD4,
   0xFA, 0xCB, 0x98, 0xA9, 0x3E, 0x0F, 0x5C, 0x6D,
   0x86, 0xB7, 0xE4, 0xD5, 0x42, 0x73, 0x20, 0x11,
   0x3F, 0x0E, 0x5D, 0x6C, 0xFB, 0xCA, 0x99, 0xA8,
   0xC5, 0xF4, 0xA7, 0x96, 0x01, 0x30, 0x63, 0x52,
   0x7C, 0x4D, 0x1E, 0x2F, 0xB8, 0x89, 0xDA, 0xEB,
   0x3D, 0x0C, 0x5F, 0x6E, 0xF9, 0xC8, 0x9B, 0xAA,
   0x84, 0xB5, 0xE6, 0xD7, 0x40, 0x71, 0x22, 0x13,
   0x7E, 0x4F, 0x1C, 0x2D, 0xBA, 0x8B, 0xD8, 0xE9,
   0xC7, 0xF6, 0xA5, 0x94, 0x03, 0x32, 0x61, 0x50,
   0xBB, 0x8A, 0xD9, 0xE8, 0x7F, 0x4E, 0x1D, 0x2C,
   0x02, 0x33, 0x60, 0x51, 0xC6, 0xF7, 0xA4, 0x95,
   0xF8, 0xC9, 0x9A, 0xAB, 0x3C, 0x0D, 0x5E, 0x6F,
   0x41, 0x70, 0x23, 0x12, 0x85, 0xB4, 0xE7, 0xD6,
   0x7A, 0x4B, 0x18, 0x29, 0xBE, 0x8F, 0xDC, 0xED,
   0xC3, 0xF2, 0xA1, 0x90, 0x07, 0x36, 0x65, 0x54,
   0x39, 0x08, 0x5B, 0x6A, 0xFD, 0xCC, 0x9F, 0xAE,
   0x80, 0xB1, 0xE2, 0xD3, 0x44, 0x75, 0x26, 0x17,
   0xFC, 0xCD, 0x9E, 0xAF, 0x38, 0x09, 0x5A, 0x6B,
   0x45, 0x74, 0x27, 0x16, 0x81, 0xB0, 0xE3, 0xD2,
   0xBF, 0x8E, 0xDD, 0xEC, 0x7B, 0x4A, 0x19, 0x28,
   0x06, 0x37, 0x64, 0x55, 0xC2, 0xF3, 0xA0, 0x91,
   0x47, 0x76, 0x25, 0x14, 0x83, 0xB2, 0xE1, 0xD0,
   0xFE, 0xCF, 0x9C, 0xAD, 0x3A, 0x0B, 0x58, 0x69,
   0x04, 0x35, 0x66, 0x57, 0xC0, 0xF1, 0xA2, 0x93,
   0xBD, 0x8C, 0xDF, 0xEE, 0x79, 0x48, 0x1B, 0x2A,
   0xC1, 0xF0, 0xA3, 0x92, 0x05, 0x34, 0x67, 0x56,
   0x78, 0x49, 0x1A, 0x2B, 0xBC, 0x8D, 0xDE, 0xEF,
   0x82, 0xB3, 0xE0, 0xD1, 0x46, 0x77, 0x24, 0x15,
   0x3B, 0x0A, 0x59, 0x68, 0xFF, 0xCE, 0x9D, 0xAC
};

/**
 * Calculate CRC8 for given data block
 */
uint8_t crc8(const uint8_t *data, size_t size)
{
   uint8_t crc = 0xFF;
   while (size-- > 0)
   {
      crc = crc8Table[crc ^ *data++];
   }
   return crc;
}

/**
 * NTCB checksum
 */
static inline uint8_t NTCBChecksum(const void *data, size_t size)
{
   uint8_t sum = 0;
   const uint8_t *p = static_cast<const uint8_t*>(data);
   while(size-- > 0)
      sum ^= *p++;
   return sum;
}

/**
 * NTCB header
 */
struct NTCB_HEADER
{
   char preamble[4];
   uint32_t receiverId;
   uint32_t senderId;
   uint16_t dataBytes;
   uint8_t dataChecksum;
   uint8_t headerChecksum;
};

/**
 * Device session constructor
 */
NTCBDeviceSession::NTCBDeviceSession(SOCKET s, const InetAddress& addr) : m_socket(s), m_address(addr)
{
   m_id = -1;
   m_ntcbReceiverId = 0;
   m_ntcbSenderId = 0;
   m_flexProtoVersion = 0;
   m_flexStructVersion = 0;
   m_flexFieldCount = 0;
   memset(m_flexFieldMask, 0, sizeof(m_flexFieldMask));
}

/**
 * Device session destructor
 */
NTCBDeviceSession::~NTCBDeviceSession()
{
}

/**
 * Print debug information
 */
void NTCBDeviceSession::debugPrintf(int level, const TCHAR *format, ...)
{
   va_list args;
   va_start(args, format);
   nxlog_debug_tag_object2(DEBUG_TAG_NTCB_SESSION, m_id, level, format, args);
   va_end(args);
}

/**
 * Start session
 */
bool NTCBDeviceSession::start()
{
   return ThreadCreate(this, &NTCBDeviceSession::readThread);
}

/**
 * Terminate session
 */
void NTCBDeviceSession::terminate()
{
   m_socket.disconnect();
}

/**
 * Send NTCB message
 */
void NTCBDeviceSession::sendNTCBMessage(const void *data, size_t size)
{
   NTCB_HEADER header;
   memcpy(header.preamble, "@NTC", 4);
   header.senderId = m_ntcbReceiverId;
   header.receiverId = m_ntcbSenderId;
   header.dataBytes = static_cast<uint16_t>(size);
   header.dataChecksum = NTCBChecksum(data, size);
   header.headerChecksum = NTCBChecksum(&header, 15);
   m_socket.write(&header, 16);
   m_socket.write(data, size);
}

/**
 * Read thread
 */
void NTCBDeviceSession::readThread()
{
   debugPrintf(3, _T("Read thread started"));

   while(true)
   {
      char preamble;
      if (m_socket.read(&preamble, 1, g_ntcbSocketTimeout) <= 0)
      {
         debugPrintf(3, _T("Socket read error"));
         break;
      }

      if (preamble == '@') // NTCB message with header
      {
         NTCB_HEADER header;
         header.preamble[0] = preamble;
         if (!m_socket.readFully(reinterpret_cast<char*>(&header) + 1, 15, g_ntcbSocketTimeout))
         {
            debugPrintf(6, _T("Cannot read NTCB header"));
            continue;
         }

         if (memcmp(header.preamble, "@NTC", 4))
         {
            debugPrintf(6, _T("Invalid NTCB header (invalid preamble)"));
            continue;
         }

         // Check header checksum
         uint8_t cs = NTCBChecksum(&header, 15);
         if (cs != header.headerChecksum)
         {
            debugPrintf(6, _T("Invalid NTCB header (invalid header checksum)"));
            continue;
         }

         m_ntcbSenderId = header.senderId;
         m_ntcbReceiverId = header.receiverId;

#if WORDS_BIGENDIAN
         // Fixme: swap header fields?
#endif

         void *data;
         if (header.dataBytes > 0)
         {
            data = MemAlloc(header.dataBytes);
            if (!m_socket.readFully(data, header.dataBytes, g_ntcbSocketTimeout))
            {
               debugPrintf(6, _T("Cannot read NTCB packet data"));
               MemFree(data);
               continue;
            }

            // Validate data checksum
            cs = NTCBChecksum(data, header.dataBytes);
            if (cs != header.dataChecksum)
            {
               debugPrintf(6, _T("Invalid NTCB packet (invalid data checksum)"));
               continue;
            }
         }
         else
         {
            data = nullptr;
         }

         debugPrintf(5, _T("Received NTCB packet %d -> %d, %d data bytes"), header.senderId, header.receiverId, header.dataBytes);
         if (data != nullptr)
         {
            processNTCBMessage(data, header.dataBytes);
            MemFree(data);
         }
      }
      else if (preamble == '~') // FLEX message
      {
         if (m_device == nullptr)
         {
            debugPrintf(4, _T("FLEX message from unregistered device"));
            break;
         }
         processFLEXMessage();
      }
   }

   m_socket.disconnect();
   debugPrintf(3, _T("Read thread stopped"));

   // The following call can potentially destroy session object
   // so no access to the object should be performed after this point
   UnregisterNTCBDeviceSession(m_id);
}

/**
 * Process NTCB message
 */
void NTCBDeviceSession::processNTCBMessage(const void *data, size_t size)
{
   if (nxlog_get_debug_level_tag_object(DEBUG_TAG_NTCB_SESSION, m_id) >= 8)
   {
      TCHAR *text = MemAllocString(size * 2 + 1);
      BinToStr(data, size, text);
      debugPrintf(8, _T("NTCB message data: %s"), text);
      MemFree(text);
   }

   if ((size > 4) && !memcmp(data, "*>S:", 4))
   {
      // Handshake - device ID
      TCHAR deviceId[32];
      size_t deviceIdLen = std::min(size - 4, static_cast<size_t>(31));
      for(size_t i = 0; i < deviceIdLen; i++)
         deviceId[i] = *(static_cast<const char*>(data) + 4 + i);
      deviceId[deviceIdLen] = 0;
      debugPrintf(4, _T("Handshake from device %s"), deviceId);

      m_device = FindMobileDeviceByDeviceID(deviceId);
      if (m_device != nullptr)
      {
         debugPrintf(4, _T("Found mobile device object %s [%u]"), m_device->getName(), m_device->getId());
         sendNTCBMessage("*<S", 3);
      }
      else
      {
         debugPrintf(4, _T("Cannot find mobile device object with ID %s"), deviceId);
         m_socket.disconnect();
      }
   }
   else if ((size > 9) && !memcmp(data, "*>FLEX", 6))
   {
      // Handshake - FLEX version negotiation
      auto curr = static_cast<const uint8_t*>(data) + 6;
      if (*curr++ == 0xB0) // Should be 0xB0
      {
         m_flexProtoVersion = *curr++;
         m_flexStructVersion = *curr++;
         m_flexFieldCount = *curr++;
         size_t maskBytes = (m_flexFieldCount + 7) / 8;
         if (maskBytes > sizeof(m_flexFieldMask))
            maskBytes = sizeof(m_flexFieldMask);
         memcpy(m_flexFieldMask, curr, maskBytes);
         memset(&m_flexFieldMask[maskBytes], 0, sizeof(m_flexFieldMask) - maskBytes);

         debugPrintf(4, _T("Device requested FLEX version: protocol = %d.%d, structure = %d.%d"),
                  m_flexProtoVersion / 10, m_flexProtoVersion % 10, m_flexStructVersion / 10, m_flexStructVersion % 10);
         if (m_flexProtoVersion > 30)
            m_flexProtoVersion = 30;
         if (m_flexStructVersion > 30)
            m_flexStructVersion = 30;
         debugPrintf(4, _T("Server offer for FLEX version: protocol = %d.%d, structure = %d.%d"),
                  m_flexProtoVersion / 10, m_flexProtoVersion % 10, m_flexStructVersion / 10, m_flexStructVersion % 10);

         uint8_t response[9];
         memcpy(response, "*<FLEX", 6);
         response[6] = 0xB0;
         response[7] = m_flexProtoVersion;
         response[8] = m_flexStructVersion;
         sendNTCBMessage(response, 9);

         StringBuffer sb;
         for(size_t i = 0; i < maskBytes; i++)
         {
            if (i > 0)
               sb.append(_T(" "));
            TCHAR text[9];
            int pos = 0;
            for(uint8_t m = 0x80; m != 0; m >>= 1)
               text[pos++] = (m_flexFieldMask[i] & m) ? _T('1') : _T('0');
            text[pos] = 0;
            sb.append(text);
         }
         debugPrintf(4, _T("Telemetry field map: %s"), sb.cstr());
      }
      else
      {
         debugPrintf(4, _T("Invalid FLEX protocol code received from device"));
      }
   }
}

/**
 * Process FLEX message
 */
void NTCBDeviceSession::processFLEXMessage()
{
   char command;
   if (m_socket.read(&command, 1, g_ntcbSocketTimeout) != 1)
   {
      debugPrintf(3, _T("Socket read error"));
      return;
   }
   debugPrintf(5, _T("Processing FLEX command \"%c\""), static_cast<TCHAR>(command));

   switch(command)
   {
      case 'A':
         processArchiveTelemetry();
         break;
      case 'T':
         processExtraordinaryTelemetry();
         break;
      case 'C':
         processCurrentTelemetry();
         break;
   }
}

/**
 * Read archive telemetry records
 */
void NTCBDeviceSession::processArchiveTelemetry()
{
   uint8_t count;
   if (m_socket.read(&count, 1, g_ntcbSocketTimeout) != 1)
   {
      debugPrintf(3, _T("Socket read error"));
      return;
   }

   for(int i = 0; i < static_cast<int>(count); i++)
   {
      if (!readTelemetryRecord(true))
      {
         debugPrintf(5, _T("processArchiveTelemetry: failed to read record #%d"), i + 1);
         return;
      }
   }
   m_socket.skip(1); // CRC8

   uint8_t response[4];
   response[0] = '~';
   response[1] = 'A';
   response[2] = count;
   response[3] = crc8(response, 3);
   m_socket.write(response, 4);
}

/**
 * Read current telemetry record
 */
void NTCBDeviceSession::processCurrentTelemetry()
{
   if (!readTelemetryRecord(false))
   {
      debugPrintf(5, _T("processCurrentTelemetry: failed to read record"));
      return;
   }
   m_socket.skip(1); // CRC8

   uint8_t response[3];
   response[0] = '~';
   response[1] = 'C';
   response[2] = crc8(response, 2);
   m_socket.write(response, 3);
}

/**
 * Read extraordinary telemetry record
 */
void NTCBDeviceSession::processExtraordinaryTelemetry()
{
   uint32_t eventIndex;
   if (!m_socket.readFully(&eventIndex, 4, g_ntcbSocketTimeout))
   {
      debugPrintf(5, _T("processExtraordinaryTelemetry: failed to read event index"));
      return;
   }

   if (!readTelemetryRecord(false))
   {
      debugPrintf(5, _T("processExtraordinaryTelemetry: failed to read record"));
      return;
   }
   m_socket.skip(1); // CRC8

   uint8_t response[7];
   response[0] = '~';
   response[1] = 'T';
   memcpy(&response[2], &eventIndex, 4);
   response[6] = crc8(response, 6);
   m_socket.write(response, 7);
}
