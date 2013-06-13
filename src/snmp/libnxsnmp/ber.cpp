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


//
// Decode BER-encoded variable
//

BOOL BER_DecodeIdentifier(BYTE *pRawData, UINT32 dwRawSize, UINT32 *pdwType, 
                          UINT32 *pdwLength, BYTE **pData, UINT32 *pdwIdLength)
{
   BOOL bResult = FALSE;
   BYTE *pbCurrPos = pRawData;
   UINT32 dwIdLength = 0;

   *pdwType = (UINT32)(*pbCurrPos);
   pbCurrPos++;
   dwIdLength++;

   // Get length
   if ((*pbCurrPos & 0x80) == 0)
   {
      *pdwLength = (UINT32)(*pbCurrPos);
      pbCurrPos++;
      dwIdLength++;
      bResult = TRUE;
   }
   else
   {
      UINT32 dwLength = 0;
      BYTE *pbTemp;
      int iNumBytes;

      iNumBytes = *pbCurrPos & 0x7F;
      pbCurrPos++;
      dwIdLength++;
      pbTemp = ((BYTE *)&dwLength) + (4 - iNumBytes);
      if ((iNumBytes >= 1) && (iNumBytes <= 4))
      {
         while(iNumBytes > 0)
         {
            *pbTemp++ = *pbCurrPos++;
            dwIdLength++;
            iNumBytes--;
         }
         *pdwLength = ntohl(dwLength);
         bResult = TRUE;
      }
   }

   // Set pointer to variable's data
   *pData = pbCurrPos;
   *pdwIdLength = dwIdLength;
   return bResult;
}


//
// Decode content of specified types
//

BOOL BER_DecodeContent(UINT32 dwType, BYTE *pData, UINT32 dwLength, BYTE *pBuffer)
{
   BOOL bResult = TRUE;

   switch(dwType)
   {
      case ASN_INTEGER:
      case ASN_COUNTER32:
      case ASN_GAUGE32:
      case ASN_TIMETICKS:
      case ASN_UINTEGER32:
         if ((dwLength >= 1) && (dwLength <= 5))
         {
            UINT32 dwValue;
            BYTE *pbTemp;

            // Pre-fill buffer with 1's for negative values and 0's for positive
            dwValue = (*pData & 0x80) ? 0xFFFFFFFF : 0;

            // For large unsigned integers, we can have length of 5, and first byte
            // is usually 0. In this case, we skip first byte.
            if (dwLength == 5)
            {
               pData++;
               dwLength--;
            }

            pbTemp = ((BYTE *)&dwValue) + (4 - dwLength);
            while(dwLength > 0)
            {
               *pbTemp++ = *pData++;
               dwLength--;
            }
            dwValue = ntohl(dwValue);
            memcpy(pBuffer, &dwValue, sizeof(UINT32));
         }
         else
         {
            bResult = FALSE;  // We didn't expect more than 32 bit integers
         }
         break;
      case ASN_COUNTER64:
         if ((dwLength >= 1) && (dwLength <= 9))
         {
            QWORD qwValue;
            BYTE *pbTemp;

            // Pre-fill buffer with 1's for negative values and 0's for positive
            qwValue = (*pData & 0x80) ? _ULL(0xFFFFFFFFFFFFFFFF) : 0;

            // For large unsigned integers, we can have length of 9, and first byte
            // is usually 0. In this case, we skip first byte.
            if (dwLength == 9)
            {
               pData++;
               dwLength--;
            }

            pbTemp = ((BYTE *)&qwValue) + (8 - dwLength);
            while(dwLength > 0)
            {
               *pbTemp++ = *pData++;
               dwLength--;
            }
            qwValue = ntohq(qwValue);
            memcpy(pBuffer, &qwValue, sizeof(QWORD));
         }
         else
         {
            bResult = FALSE;  // We didn't expect more than 64 bit integers
         }
         break;
      case ASN_OBJECT_ID:
         if (dwLength > 0)
         {
            SNMP_OID *oid;
            UINT32 dwValue;

            oid = (SNMP_OID *)pBuffer;
            oid->pdwValue = (UINT32 *)malloc(sizeof(UINT32) * (dwLength + 1));

            // First octet need special handling
            oid->pdwValue[0] = *pData / 40;
            oid->pdwValue[1] = *pData % 40;
            pData++;
            oid->dwLength = 2;
            dwLength--;

            // Parse remaining octets
            while(dwLength > 0)
            {
               dwValue = 0;

               // Loop through octets with 8th bit set to 1
               while((*pData & 0x80) && (dwLength > 0))
               {
                  dwValue = (dwValue << 7) | (*pData & 0x7F);
                  pData++;
                  dwLength--;
               }

               // Last octet in element
               if (dwLength > 0)
               {
                  oid->pdwValue[oid->dwLength++] = (dwValue << 7) | *pData;
                  pData++;
                  dwLength--;
               }
            }
         }
         break;
      default:    // For unknown types, simply move content to buffer
         memcpy(pBuffer, pData, dwLength);
         break;
   }
   return bResult;
}


//
// Encode content
//

static LONG EncodeContent(UINT32 dwType, BYTE *pData, UINT32 dwDataLength, BYTE *pResult)
{
   LONG nBytes = 0;
   UINT32 dwTemp;
   QWORD qwTemp;
   BYTE *pTemp, sign;
   int i, iOidLength;

   switch(dwType)
   {
      case ASN_NULL:
         break;
      case ASN_INTEGER:
         dwTemp = htonl(*((UINT32 *)pData));
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
         dwTemp = htonl(*((UINT32 *)pData));
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
         qwTemp = htonq(*((QWORD *)pData));
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
         iOidLength = dwDataLength / sizeof(UINT32);
         if (iOidLength > 1)
         {
            BYTE *pbCurrPos = pResult;
            UINT32 j, dwValue, dwSize, *pdwCurrId = (UINT32 *)pData;
            static UINT32 dwLengthMask[5] = { 0x0000007F, 0x00003FFF, 0x001FFFFF, 0x0FFFFFFF, 0xFFFFFFFF };

            // First two ids encoded in one byte
            *pbCurrPos = (BYTE)pdwCurrId[0] * 40 + (BYTE)pdwCurrId[1];
            pbCurrPos++;
            pdwCurrId += 2;
            nBytes++;

            // Encode other ids
            for(i = 2; i < iOidLength; i++, pdwCurrId++)
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
			else if (iOidLength == 1)
			{
				*pResult = (BYTE)(*((UINT32 *)pData)) * 40;
				nBytes++;
			}
         break;
      default:
         memcpy(pResult, pData, dwDataLength);
         nBytes = dwDataLength;
         break;
   }
   return nBytes;
}


//
// Encode identifier and content
// Return value is size of encoded identifier and content in buffer
// or 0 if there are not enough place in buffer or type is unknown
//

UINT32 BER_Encode(UINT32 dwType, BYTE *pData, UINT32 dwDataLength,
                 BYTE *pBuffer, UINT32 dwBufferSize)
{
   UINT32 dwBytes = 0;
   BYTE *pbCurrPos = pBuffer, *pEncodedData;
   LONG nDataBytes;

   if (dwBufferSize < 2)
      return 0;

   *pbCurrPos++ = (BYTE)dwType;
   dwBytes++;

   // Encode content
   pEncodedData = (BYTE *)malloc(dwDataLength);
   nDataBytes = EncodeContent(dwType, pData, dwDataLength, pEncodedData);

   // Encode length
   if (nDataBytes < 128)
   {
      *pbCurrPos++ = (BYTE)nDataBytes;
      dwBytes++;
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
      if (dwBufferSize < (UINT32)nHdrBytes + dwBytes + 1)
      {
         free(pEncodedData);
         return 0;
      }

      // Write length field
      *pbCurrPos++ = (BYTE)(0x80 | nHdrBytes);
      memcpy(pbCurrPos, bLength, nHdrBytes);
      pbCurrPos += nHdrBytes;
      dwBytes += nHdrBytes + 1;
   }

   // Copy encoded data to buffer
   if (dwBufferSize >= dwBytes + nDataBytes)
   {
      memcpy(pbCurrPos, pEncodedData, nDataBytes);
      dwBytes += nDataBytes;
   }
   else
   {
      dwBytes = 0;   // Buffer is too small
   }

   free(pEncodedData);
   return dwBytes;
}
