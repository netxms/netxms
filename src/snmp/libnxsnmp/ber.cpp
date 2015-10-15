/* 
** NetXMS - Network Management System
** SNMP support library
** Copyright (C) 2003-2010 Victor Kirhenshtein
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
bool BER_DecodeIdentifier(BYTE *rawData, size_t rawSize, UINT32 *type, size_t *dataLength, BYTE **data, size_t *idLength)
{
   bool bResult = false;
   BYTE *pbCurrPos = rawData;
   UINT32 dwIdLength = 0;

   *type = (UINT32)(*pbCurrPos);
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
      UINT32 length = 0;
      BYTE *pbTemp;
      int iNumBytes;

      iNumBytes = *pbCurrPos & 0x7F;
      pbCurrPos++;
      dwIdLength++;
      pbTemp = ((BYTE *)&length) + (4 - iNumBytes);
      if ((iNumBytes >= 1) && (iNumBytes <= 4))
      {
         while(iNumBytes > 0)
         {
            *pbTemp++ = *pbCurrPos++;
            dwIdLength++;
            iNumBytes--;
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
bool BER_DecodeContent(UINT32 type, BYTE *data, size_t length, BYTE *buffer)
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
            UINT32 dwValue;
            BYTE *pbTemp;

            // Pre-fill buffer with 1's for negative values and 0's for positive
            dwValue = (*data & 0x80) ? 0xFFFFFFFF : 0;

            // For large unsigned integers, we can have length of 5, and first byte
            // is usually 0. In this case, we skip first byte.
            if (length == 5)
            {
               data++;
               length--;
            }

            pbTemp = ((BYTE *)&dwValue) + (4 - length);
            while(length > 0)
            {
               *pbTemp++ = *data++;
               length--;
            }
            dwValue = ntohl(dwValue);
            memcpy(buffer, &dwValue, sizeof(UINT32));
         }
         else
         {
            bResult = false;  // We didn't expect more than 32 bit integers
         }
         break;
      case ASN_COUNTER64:
         if ((length >= 1) && (length <= 9))
         {
            QWORD qwValue;
            BYTE *pbTemp;

            // Pre-fill buffer with 1's for negative values and 0's for positive
            qwValue = (*data & 0x80) ? _ULL(0xFFFFFFFFFFFFFFFF) : 0;

            // For large unsigned integers, we can have length of 9, and first byte
            // is usually 0. In this case, we skip first byte.
            if (length == 9)
            {
               data++;
               length--;
            }

            pbTemp = ((BYTE *)&qwValue) + (8 - length);
            while(length > 0)
            {
               *pbTemp++ = *data++;
               length--;
            }
            qwValue = ntohq(qwValue);
            memcpy(buffer, &qwValue, sizeof(QWORD));
         }
         else
         {
            bResult = FALSE;  // We didn't expect more than 64 bit integers
         }
         break;
      case ASN_OBJECT_ID:
         if (length > 0)
         {
            SNMP_OID *oid;
            UINT32 dwValue;

            oid = (SNMP_OID *)buffer;
            oid->value = (UINT32 *)malloc(sizeof(UINT32) * (length + 1));

            // First octet need special handling
            oid->value[0] = *data / 40;
            oid->value[1] = *data % 40;
            data++;
            oid->length = 2;
            length--;

            // Parse remaining octets
            while(length > 0)
            {
               dwValue = 0;

               // Loop through octets with 8th bit set to 1
               while((*data & 0x80) && (length > 0))
               {
                  dwValue = (dwValue << 7) | (*data & 0x7F);
                  data++;
                  length--;
               }

               // Last octet in element
               if (length > 0)
               {
                  oid->value[oid->length++] = (dwValue << 7) | *data;
                  data++;
                  length--;
               }
            }
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
static size_t EncodeContent(UINT32 type, const BYTE *data, size_t dataLength, BYTE *pResult)
{
   size_t nBytes = 0;
   UINT32 dwTemp;
   QWORD qwTemp;
   BYTE *pTemp, sign;
   size_t oidLength;

   switch(type)
   {
      case ASN_NULL:
         break;
      case ASN_INTEGER:
         dwTemp = htonl(*((UINT32 *)data));
         pTemp = (BYTE *)&dwTemp;
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
         dwTemp = htonl(*((UINT32 *)data));
         pTemp = (BYTE *)&dwTemp;
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
      case ASN_COUNTER64:
         qwTemp = htonq(*((QWORD *)data));
         pTemp = (BYTE *)&qwTemp;
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
      case ASN_OBJECT_ID:
         oidLength = dataLength / sizeof(UINT32);
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
				*pResult = (BYTE)(*((UINT32 *)data)) * 40;
				nBytes++;
			}
         break;
      default:
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
size_t BER_Encode(UINT32 type, const BYTE *data, size_t dataLength, BYTE *buffer, size_t bufferSize)
{
   size_t bytes = 0;
   BYTE *pbCurrPos = buffer, *pEncodedData;
   size_t nDataBytes;

   if (bufferSize < 2)
      return 0;

   *pbCurrPos++ = (BYTE)type;
   bytes++;

   // Encode content
   pEncodedData = (BYTE *)malloc(dataLength);
   nDataBytes = EncodeContent(type, data, dataLength, pEncodedData);

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

      *((UINT32 *)bLength) = htonl((UINT32)nDataBytes);
      for(i = 0, nHdrBytes = 4; (bLength[i] == 0) && (nHdrBytes > 1); i++, nHdrBytes--);
      if (i > 0)
         memmove(bLength, &bLength[i], nHdrBytes);

      // Check for available buffer size
      if (bufferSize < (UINT32)nHdrBytes + bytes + 1)
      {
         free(pEncodedData);
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

   free(pEncodedData);
   return bytes;
}
