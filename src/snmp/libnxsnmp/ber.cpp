/* 
** NetXMS - Network Management System
** SNMP support library
** Copyright (C) 2003-2025 Victor Kirhenshtein
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
** File: ber.cpp
**
**/

#include "libnxsnmp.h"

/**
 * Decode BER-encoded variable
 */
bool BER_DecodeIdentifier(const BYTE *rawData, size_t rawSize, uint32_t *type, size_t *dataLength, const BYTE **data, size_t *idLength)
{
   bool bResult = false;
   const BYTE *pbCurrPos = rawData;
   uint32_t dwIdLength = 0;

   *type = (uint32_t)(*pbCurrPos);
   pbCurrPos++;
   dwIdLength++;

   // Get length
   if ((*pbCurrPos & 0x80) == 0)
   {
      *dataLength = (size_t)(*pbCurrPos);
      pbCurrPos++;
      dwIdLength++;
      bResult = true;
   }
   else
   {
      uint32_t length = 0;
      int numBytes = *pbCurrPos & 0x7F;
      pbCurrPos++;
      dwIdLength++;
      BYTE *temp = ((BYTE *)&length) + (4 - numBytes);
      if ((numBytes >= 1) && (numBytes <= 4))
      {
         while(numBytes > 0)
         {
            *temp++ = *pbCurrPos++;
            dwIdLength++;
            numBytes--;
         }
         *dataLength = (size_t)ntohl(length);
         bResult = true;
      }
   }

   // Set pointer to variable's data
   *data = pbCurrPos;
   *idLength = dwIdLength;
   return bResult;
}

/**
 * Decode content of specified types
 */
bool BER_DecodeContent(uint32_t type, const BYTE *data, size_t length, BYTE *buffer)
{
   bool bResult = true;

   switch(type)
   {
      case ASN_INTEGER:
      case ASN_COUNTER32:
      case ASN_GAUGE32:
      case ASN_TIMETICKS:
      case ASN_UINTEGER32:
         if ((length >= 1) && (length <= 5))
         {
            // Pre-fill buffer with 1's for negative values and 0's for positive
            uint32_t value = (*data & 0x80) ? 0xFFFFFFFF : 0;

            // For large unsigned integers, we can have length of 5, and first byte
            // is usually 0. In this case, we skip first byte.
            if (length == 5)
            {
               data++;
               length--;
            }

            BYTE *temp = ((BYTE *)&value) + (4 - length);
            while(length > 0)
            {
               *temp++ = *data++;
               length--;
            }
            value = ntohl(value);
            memcpy(buffer, &value, sizeof(UINT32));
         }
         else
         {
            bResult = false;  // We didn't expect more than 32 bit integers
         }
         break;
      case ASN_COUNTER64:
      case ASN_INTEGER64:
      case ASN_UINTEGER64:
         if ((length >= 1) && (length <= 9))
         {
            // Pre-fill buffer with 1's for negative values and 0's for positive
            uint64_t value = (*data & 0x80) ? _ULL(0xFFFFFFFFFFFFFFFF) : 0;

            // For large unsigned integers, we can have length of 9, and first byte
            // is usually 0. In this case, we skip first byte.
            if (length == 9)
            {
               data++;
               length--;
            }

            BYTE *temp = ((BYTE *)&value) + (8 - length);
            while(length > 0)
            {
               *temp++ = *data++;
               length--;
            }
            value = ntohq(value);
            memcpy(buffer, &value, sizeof(uint64_t));
         }
         else
         {
            bResult = false;  // We didn't expect more than 64 bit integers
         }
         break;
      case ASN_FLOAT:
         if (length == 4)
         {
            // use memcpy to temporary buffer to avoid memory alignment problem
            float t;
            memcpy(&t, data, 4);
            *reinterpret_cast<float*>(buffer) = ntohf(t);
         }
         else
         {
            bResult = false;  // Wrong length
         }
         break;
      case ASN_DOUBLE:
         if (length == 8)
         {
            // use memcpy to temporary buffer to avoid memory alignment problem
            double t;
            memcpy(&t, data, 8);
            *reinterpret_cast<double*>(buffer) = ntohd(t);
         }
         else
         {
            bResult = false;  // Wrong length
         }
         break;
      case ASN_OBJECT_ID:
         if (length > 0)
         {
            SNMP_OID *oid = reinterpret_cast<SNMP_OID*>(buffer);
            oid->value = oid->internalBuffer;

            // First octet need special handling
            oid->value[0] = *data / 40;
            oid->value[1] = *data % 40;
            data++;
            oid->length = 2;
            length--;

            // Parse remaining octets
            while(length > 0)
            {
               uint32_t value = 0;

               // Loop through octets with 8th bit set to 1
               while((*data & 0x80) && (length > 0))
               {
                  value = (value << 7) | (*data & 0x7F);
                  data++;
                  length--;
               }

               // Last octet in element
               if (length > 0)
               {
                  if (oid->length == SNMP_OID_INTERNAL_BUFFER_SIZE)
                  {
                     // Need to allocate memory for OID
                     oid->value = MemAllocArrayNoInit<uint32_t>(length + SNMP_OID_INTERNAL_BUFFER_SIZE);
                     memcpy(oid->value, oid->internalBuffer, oid->length * sizeof(uint32_t));
                  }
                  oid->value[oid->length++] = (value << 7) | *data;
                  data++;
                  length--;
               }
            }
         }
         else
         {
            bResult = false;  // Invalid length
         }
         break;
      default:    // For unknown types, simply move content to buffer
         memcpy(buffer, data, length);
         break;
   }
   return bResult;
}

/**
 * Encode content
 */
static size_t EncodeContent(uint32_t type, const BYTE *data, size_t dataLength, BYTE *pResult)
{
   size_t nBytes = 0;
   uint32_t tempInt32;
   uint64_t tempInt64;
   float tempFloat;
   double tempDouble;
   BYTE *pTemp, sign;
   size_t oidLength;

   switch(type)
   {
      case ASN_NULL:
         break;
      case ASN_INTEGER:
         tempInt32 = htonl(*((uint32_t *)data));
         pTemp = (BYTE *)&tempInt32;
         sign = (*pTemp & 0x80) ? 0xFF : 0;
         for(nBytes = 4; (*pTemp == sign) && (nBytes > 1); pTemp++, nBytes--);
         if ((*pTemp & 0x80) != (sign & 0x80))
         {
            memcpy(&pResult[1], pTemp, nBytes);
            pResult[0] = sign;
            nBytes++;
         }
         else
         {
            memcpy(pResult, pTemp, nBytes);
         }
         break;
      case ASN_COUNTER32:
      case ASN_GAUGE32:
      case ASN_TIMETICKS:
      case ASN_UINTEGER32:
         tempInt32 = htonl(*((uint32_t *)data));
         pTemp = (BYTE *)&tempInt32;
         for(nBytes = 4; (*pTemp == 0) && (nBytes > 1); pTemp++, nBytes--);
         if (*pTemp & 0x80)
         {
            memcpy(&pResult[1], pTemp, nBytes);
            pResult[0] = 0;
            nBytes++;
         }
         else
         {
            memcpy(pResult, pTemp, nBytes);
         }
         break;
      case ASN_INTEGER64:
         tempInt64 = htonq(*((uint64_t *)data));
         pTemp = (BYTE *)&tempInt64;
         sign = (*pTemp & 0x80) ? 0xFF : 0;
         for(nBytes = 8; (*pTemp == sign) && (nBytes > 1); pTemp++, nBytes--);
         if ((*pTemp & 0x80) != (sign & 0x80))
         {
            memcpy(&pResult[1], pTemp, nBytes);
            pResult[0] = sign;
            nBytes++;
         }
         else
         {
            memcpy(pResult, pTemp, nBytes);
         }
         break;
      case ASN_COUNTER64:
      case ASN_UINTEGER64:
         tempInt64 = htonq(*((uint64_t *)data));
         pTemp = (BYTE *)&tempInt64;
         for(nBytes = 8; (*pTemp == 0) && (nBytes > 1); pTemp++, nBytes--);
         if (*pTemp & 0x80)
         {
            memcpy(&pResult[1], pTemp, nBytes);
            pResult[0] = 0;
            nBytes++;
         }
         else
         {
            memcpy(pResult, pTemp, nBytes);
         }
         break;
      case ASN_FLOAT:
         nBytes = 4;
         tempFloat = htonf(*reinterpret_cast<const float*>(data));
         memcpy(pResult, &tempFloat, nBytes);
         break;
      case ASN_DOUBLE:
         nBytes = 8;
         tempDouble = htond(*reinterpret_cast<const double*>(data));
         memcpy(pResult, &tempDouble, nBytes);
         break;
      case ASN_OBJECT_ID:
         oidLength = dataLength / sizeof(uint32_t);
         if (oidLength > 1)
         {
            BYTE *pbCurrPos = pResult;
            UINT32 j, dwValue, dwSize, *pdwCurrId = (UINT32 *)data;
            static UINT32 dwLengthMask[5] = { 0x0000007F, 0x00003FFF, 0x001FFFFF, 0x0FFFFFFF, 0xFFFFFFFF };

            // First two ids encoded in one byte
            *pbCurrPos = (BYTE)pdwCurrId[0] * 40 + (BYTE)pdwCurrId[1];
            pbCurrPos++;
            pdwCurrId += 2;
            nBytes++;

            // Encode other ids
            for(size_t i = 2; i < oidLength; i++, pdwCurrId++)
            {
               dwValue = *pdwCurrId;

               // Determine size of oid
               for(dwSize = 0; (dwLengthMask[dwSize] & dwValue) != dwValue; dwSize++);
               dwSize++;   // Size is at least one byte...

               if (dwSize > 1)
               {
                  // Encode by 7 bits
                  pbCurrPos += (dwSize - 1);
                  *pbCurrPos-- = (BYTE)(dwValue & 0x7F);
                  for(j = dwSize - 1; j > 0; j--)
                  {
                     dwValue >>= 7;
                     *pbCurrPos-- = (BYTE)(dwValue & 0x7F) | 0x80;
                  }
                  pbCurrPos += (dwSize + 1);
               }
               else
               {
                  *pbCurrPos++ = (BYTE)(dwValue & 0x7F);
               }
               nBytes += dwSize;
            }
         }
			else if (oidLength == 1)
			{
				*pResult = (BYTE)(*((uint32_t *)data)) * 40;
				nBytes++;
			}
         break;
      default:
         if (dataLength > 0)
            memcpy(pResult, data, dataLength);
         nBytes = dataLength;
         break;
   }
   return nBytes;
}

/**
 * Encode identifier and content
 * Return value is size of encoded identifier and content in buffer
 * or 0 if there are not enough place in buffer or type is unknown
 */
size_t BER_Encode(uint32_t type, const BYTE *data, size_t dataLength, BYTE *buffer, size_t bufferSize)
{
   if (bufferSize < 2)
      return 0;

   size_t bytes = 0;
   BYTE *pbCurrPos = buffer;

   *pbCurrPos++ = (BYTE)type;
   bytes++;

   // Encode content
   BYTE *pEncodedData = static_cast<BYTE*>(SNMP_MemAlloc(dataLength));
   size_t nDataBytes = EncodeContent(type, data, dataLength, pEncodedData);

   // Encode length
   if (nDataBytes < 128)
   {
      *pbCurrPos++ = (BYTE)nDataBytes;
      bytes++;
   }
   else
   {
      BYTE bLength[8];
      LONG nHdrBytes;
      int i;

      *((uint32_t *)bLength) = htonl((uint32_t)nDataBytes);
      for(i = 0, nHdrBytes = 4; (bLength[i] == 0) && (nHdrBytes > 1); i++, nHdrBytes--);
      if (i > 0)
         memmove(bLength, &bLength[i], nHdrBytes);

      // Check for available buffer size
      if (bufferSize < (uint32_t)nHdrBytes + bytes + 1)
      {
         SNMP_MemFree(pEncodedData, dataLength);
         return 0;
      }

      // Write length field
      *pbCurrPos++ = (BYTE)(0x80 | nHdrBytes);
      memcpy(pbCurrPos, bLength, nHdrBytes);
      pbCurrPos += nHdrBytes;
      bytes += nHdrBytes + 1;
   }

   // Copy encoded data to buffer
   if (bufferSize >= bytes + nDataBytes)
   {
      memcpy(pbCurrPos, pEncodedData, nDataBytes);
      bytes += nDataBytes;
   }
   else
   {
      bytes = 0;   // Buffer is too small
   }

   SNMP_MemFree(pEncodedData, dataLength);
   return bytes;
}
