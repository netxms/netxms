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
** File: attr.cpp
**
**/

#include "libethernetip.h"

/**
 * Attribute definition
 */
struct CIP_Attribute
{
   uint32_t id;
   CIP_DataType dataType;
};

/**
 * Class definition
 */
struct CIP_Class
{
   uint32_t id;
   const CIP_Attribute *attributes;
};

/**
 * Identity Object attributes
 */
static CIP_Attribute s_identityObject[] =
{
   { 1, CIP_DT_UINT },
   { 2, CIP_DT_UINT },
   { 3, CIP_DT_UINT },
   { 4, CIP_DT_WORD },
   { 5, CIP_DT_WORD },
   { 6, CIP_DT_UDINT },
   { 7, CIP_DT_SHORT_STRING },
   { 8, CIP_DT_USINT },
   { 9, CIP_DT_UINT },
   { 10, CIP_DT_USINT },
   { 0, CIP_DT_NULL }
};

/**
 * Ethernet Link Object attributes
 */
static CIP_Attribute s_ethernetLinkObject[] =
{
   { 1, CIP_DT_UDINT },
   { 2, CIP_DT_DWORD },
   { 3, CIP_DT_MAC_ADDR },
   { 0, CIP_DT_NULL }
};

/**
 * Standard CIP classes
 */
static CIP_Class s_standardClasses[] =
{
   { 0x01, s_identityObject },
   { 0xF6, s_ethernetLinkObject },
   { 0, nullptr }
};

/**
 * Generic attributes
 */
static CIP_Attribute s_attrSInt = { 0, CIP_DT_SINT };
static CIP_Attribute s_attrInt = { 0, CIP_DT_INT };
static CIP_Attribute s_attrDInt = { 0, CIP_DT_DINT };
static CIP_Attribute s_attrLInt = { 0, CIP_DT_LINT };
static CIP_Attribute s_attrMAC = { 0, CIP_DT_MAC_ADDR };
static CIP_Attribute s_attrByteArray = { 0, CIP_DT_BYTE_ARRAY };

/**
 * Extract string into buffer
 */
static void ExtractString(const uint8_t *bytes, size_t slen, TCHAR *buffer, size_t blen)
{
   size_t len = std::min(blen - 1, slen);
#ifdef UNICODE
#ifdef UNICODE_UCS2
   ISO8859_1_to_ucs2(reinterpret_cast<const char*>(bytes), slen, buffer, blen);
#else
   ISO8859_1_to_ucs4(reinterpret_cast<const char*>(bytes), slen, buffer, blen);
#endif
#else
   memcpy(buffer, bytes, len);
#endif
   buffer[len] = 0;
}

/**
 * Decode attribute and write extual representation into given buffer
 */
TCHAR LIBETHERNETIP_EXPORTABLE *CIP_DecodeAttribute(const uint8_t *data, size_t dataSize, uint32_t classId, uint32_t attributeId, TCHAR *buffer, size_t bufferSize)
{
   // Try to lookup type in standard classes
   const CIP_Attribute *attr = nullptr;
   for(const CIP_Class *c = s_standardClasses; c->id != 0; c++)
   {
      if (c->id != classId)
         continue;
      for(const CIP_Attribute *a = c->attributes; a->id != 0; a++)
      {
         if (a->id == attributeId)
         {
            attr = a;
            goto stop_search;
         }
      }
   }

stop_search:
   if (attr == nullptr)
   {
      // try to guess type from size
      switch(dataSize)
      {
         case 1:
            attr = &s_attrSInt;
            break;
         case 2:
            attr = &s_attrInt;
            break;
         case 4:
            attr = &s_attrDInt;
            break;
         case 6:
            attr = &s_attrMAC;
            break;
         case 8:
            attr = &s_attrLInt;
            break;
         default:
            attr = &s_attrByteArray;
            break;
      }
   }

   switch(attr->dataType)
   {
      case CIP_DT_SINT:
         _sntprintf(buffer, bufferSize, _T("%d"), static_cast<int>(*data));
         break;
      case CIP_DT_USINT:
         _sntprintf(buffer, bufferSize, _T("%u"), static_cast<unsigned int>(*data));
         break;
      case CIP_DT_INT:
         _sntprintf(buffer, bufferSize, _T("%d"), static_cast<int>(*reinterpret_cast<const int16_t*>(data)));
         break;
      case CIP_DT_UINT:
         _sntprintf(buffer, bufferSize, _T("%u"), static_cast<unsigned int>(*reinterpret_cast<const uint16_t*>(data)));
         break;
      case CIP_DT_DINT:
         _sntprintf(buffer, bufferSize, _T("%d"), *reinterpret_cast<const int32_t*>(data));
         break;
      case CIP_DT_UDINT:
         _sntprintf(buffer, bufferSize, _T("%u"), *reinterpret_cast<const uint32_t*>(data));
         break;
      case CIP_DT_LINT:
         _sntprintf(buffer, bufferSize, INT64_FMT, *reinterpret_cast<const int64_t*>(data));
         break;
      case CIP_DT_ULINT:
         _sntprintf(buffer, bufferSize, UINT64_FMT, *reinterpret_cast<const uint64_t*>(data));
         break;
      case CIP_DT_BYTE:
         _sntprintf(buffer, bufferSize, _T("%02X"), static_cast<unsigned int>(*data));
         break;
      case CIP_DT_WORD:
         _sntprintf(buffer, bufferSize, _T("%04X"), static_cast<unsigned int>(*reinterpret_cast<const uint16_t*>(data)));
         break;
      case CIP_DT_DWORD:
         _sntprintf(buffer, bufferSize, _T("%08X"), *reinterpret_cast<const uint32_t*>(data));
         break;
      case CIP_DT_LWORD:
         _sntprintf(buffer, bufferSize, UINT64X_FMT(_T("016")), *reinterpret_cast<const uint64_t*>(data));
         break;
      case CIP_DT_MAC_ADDR:
         if (bufferSize >= 18)
            MACToStr(data, buffer);
         else
            *buffer = 0;
         break;
      case CIP_DT_BYTE_ARRAY:
         if (bufferSize > dataSize * 2 + 1)
            BinToStr(data, dataSize, buffer);
         else
            *buffer = 0;
         break;
      case CIP_DT_SHORT_STRING:
         ExtractString(data + 1, *data, buffer, bufferSize);
         break;
      default:
         buffer[0] = 0;
         break;
   }

   return buffer;
}
