/*
** NetXMS - Network Management System
** Ethernet/IP support library
** Copyright (C) 2003-2021 Victor Kirhenshtein
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
** File: path.cpp
**
**/

#include "libethernetip.h"

/**
 * Encode logical segment of given type
 */
static inline size_t EncodeLogicalSegment(uint8_t logicalType, uint32_t value, uint8_t *buffer)
{
   if (value < 256)
   {
      *buffer = 0x20 | (logicalType << 2);   // Logical segment, 8 bit address
      *(buffer + 1) = static_cast<uint8_t>(value);
      return 2;
   }
   if (value < 65536)
   {
      *buffer = 0x21 | (logicalType << 2);   // Logical segment, 16 bit address
      *(buffer + 1) = 0;   // Padding
      *(buffer + 2) = static_cast<uint8_t>(value & 0xFF); // Lower 8 bits
      *(buffer + 3) = static_cast<uint8_t>(value >> 8);   // Higher 8 bits
      return 4;
   }
   *buffer = 0x22 | (logicalType << 2);   // Logical segment, 32 bit address
   *(buffer + 1) = 0;   // Padding
#if WORDS_BIGENDIAN
   *(buffer + 2) = static_cast<uint8_t>(value & 0xFF);         // Bits 0..7
   *(buffer + 3) = static_cast<uint8_t>((value >> 8) & 0xFF);  // Bits 8..15
   *(buffer + 4) = static_cast<uint8_t>((value >> 16) & 0xFF); // Bits 16..23
   *(buffer + 5) = static_cast<uint8_t>(value >> 24);          // Bits 24..31
#else
   memcpy(buffer + 2, &value, 4);
#endif
   return 6;
}

/**
 * Encode object path (without attribute) into byte array ready to be sent over the network
 */
void LIBETHERNETIP_EXPORTABLE CIP_EncodeAttributePath(uint32_t classId, uint32_t instance, CIP_EPATH *path)
{
   memset(path, 0, sizeof(CIP_EPATH));
   path->size += EncodeLogicalSegment(0, classId, path->value);
   path->size += EncodeLogicalSegment(1, instance, &path->value[path->size]);
}

/**
 * Encode attribute path into byte array ready to be sent over the network
 */
void LIBETHERNETIP_EXPORTABLE CIP_EncodeAttributePath(uint32_t classId, uint32_t instance, uint32_t attributeId, CIP_EPATH *path)
{
   memset(path, 0, sizeof(CIP_EPATH));
   path->size += EncodeLogicalSegment(0, classId, path->value);
   path->size += EncodeLogicalSegment(1, instance, &path->value[path->size]);
   path->size += EncodeLogicalSegment(4, attributeId, &path->value[path->size]);
}

/**
 * Parse path element from string
 */
static bool ParsePathElement(char **curr, uint32_t *value)
{
   if (*curr == 0)
      return false;
   char *p = strchr(*curr, '.');
   if (p != nullptr)
      *p = 0;
   char *eptr;
   *value = strtoul(*curr, &eptr, 0);
   if (*eptr != 0)
      return false;
   *curr = (p != nullptr) ? p + 1 : nullptr;
   return true;
}

/**
 * Internal implementation for CIP_ParseSymbolicPathA and CIP_ParseSymbolicPathW
 */
static bool ParseSymbolicAttributePathInternal(char *symbolicPath, uint32_t *classId, uint32_t *instance, uint32_t *attributeId)
{
   char *curr = symbolicPath;
   bool success = ParsePathElement(&curr, classId);
   if (success)
      success = ParsePathElement(&curr, instance);
   if (success)
      success = ParsePathElement(&curr, attributeId);
   return success;
}

/**
 * Parse symbolic attribute path
 */
bool LIBETHERNETIP_EXPORTABLE CIP_ParseSymbolicPathA(const char *symbolicPath, uint32_t *classId, uint32_t *instance, uint32_t *attributeId)
{
   char buffer[256];
   strlcpy(buffer, symbolicPath, 256);
   return ParseSymbolicAttributePathInternal(buffer, classId, instance, attributeId);
}

/**
 * Parse symbolic attribute path
 */
bool LIBETHERNETIP_EXPORTABLE CIP_ParseSymbolicPathW(const WCHAR *symbolicPath, uint32_t *classId, uint32_t *instance, uint32_t *attributeId)
{
   char buffer[256];
   wchar_to_ASCII(symbolicPath, -1, buffer, 256);
   return ParseSymbolicAttributePathInternal(buffer, classId, instance, attributeId);
}

/**
 * Internal implementation for CIP_EncodeAttributePathA and CIP_EncodeAttributePathW
 */
static bool EncodeSymbolicAttributePathInternal(char *symbolicPath, CIP_EPATH *path)
{
   uint32_t classId, instance, attributeId;
   bool success = ParseSymbolicAttributePathInternal(symbolicPath, &classId, &instance, &attributeId);
   if (success)
      CIP_EncodeAttributePath(classId, instance, attributeId, path);
   return success;
}

/**
 * Encode attribute path into byte array ready to be sent over the network.
 * Returns false if symbolic path representation is invalid.
 * Expected format is three numbers (decimal or hexadecimal) separated by dots.
 */
bool LIBETHERNETIP_EXPORTABLE CIP_EncodeAttributePathA(const char *symbolicPath, CIP_EPATH *path)
{
   char buffer[256];
   strlcpy(buffer, symbolicPath, 256);
   return EncodeSymbolicAttributePathInternal(buffer, path);
}

/**
 * Encode attribute path into byte array ready to be sent over the network.
 * Returns false if symbolic path representation is invalid.
 * Expected format is three numbers (decimal or hexadecimal) separated by dots.
 */
bool LIBETHERNETIP_EXPORTABLE CIP_EncodeAttributePathW(const WCHAR *symbolicPath, CIP_EPATH *path)
{
   char buffer[256];
   wchar_to_ASCII(symbolicPath, -1, buffer, 256);
   return EncodeSymbolicAttributePathInternal(buffer, path);
}

/**
 * Decode EPATH
 */
String LIBETHERNETIP_EXPORTABLE CIP_DecodePath(const CIP_EPATH *path)
{
   StringBuffer sb;
   const uint8_t *curr = path->value;
   size_t size = path->size;
   while(size > 1)
   {
      switch(*curr & 0x03)
      {
         case 0:  // 8 bit
            curr++;
            if (!sb.isEmpty())
               sb.append(_T('.'));
            sb.append(static_cast<uint32_t>(*curr++));
            size -= 2;
            break;
         case 1:  // 16 bit
            if (size >= 4)
            {
               curr += 2;
               sb.append(CIP_UInt16Swap(*reinterpret_cast<const uint16_t*>(curr)));
               curr += 2;
               size -= 4;
            }
            else
            {
               size = 0;
            }
            break;
         case 2:  // 32 bit
            if (size >= 6)
            {
               curr += 2;
               uint32_t v;
               memcpy(&v, curr, 4);
               sb.append(CIP_UInt32Swap(v));
               curr += 4;
               size -= 6;
            }
            else
            {
               size = 0;
            }
            break;
         default:
            size = 0;   // stop parsing
            break;
      }
   }
   return sb;
}
