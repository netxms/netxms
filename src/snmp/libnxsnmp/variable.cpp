/* 
** NetXMS - Network Management System
** SNMP support library
** Copyright (C) 2003-2016 Victor Kirhenshtein
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
** File: variable.cpp
**
**/

#include "libnxsnmp.h"

/**
 * SNMP_Variable default constructor
 */
SNMP_Variable::SNMP_Variable()
{
   m_value = NULL;
   m_type = ASN_NULL;
   m_valueLength = 0;
}

/**
 * Create variable of ASN_NULL type
 */
SNMP_Variable::SNMP_Variable(const TCHAR *name)
{
   m_name = SNMP_ObjectId::parse(name);
   m_value = NULL;
   m_type = ASN_NULL;
   m_valueLength = 0;
}

/**
 * Create variable of ASN_NULL type
 */
SNMP_Variable::SNMP_Variable(const UINT32 *name, size_t nameLen) : m_name(name, nameLen)
{
   m_value = NULL;
   m_type = ASN_NULL;
   m_valueLength = 0;
}

/**
 * Create variable of ASN_NULL type
 */
SNMP_Variable::SNMP_Variable(const SNMP_ObjectId &name) : m_name(name)
{
   m_value = NULL;
   m_type = ASN_NULL;
   m_valueLength = 0;
}

/**
 * Copy constructor
 */
SNMP_Variable::SNMP_Variable(const SNMP_Variable *src)
{
   m_valueLength = src->m_valueLength;
   m_value = (src->m_value != NULL) ? (BYTE *)nx_memdup(src->m_value, src->m_valueLength) : NULL;
   m_type = src->m_type;
   m_name = src->m_name;
}

/**
 * SNMP_Variable destructor
 */
SNMP_Variable::~SNMP_Variable()
{
   free(m_value);
}

/**
 * Parse variable record in PDU
 */
bool SNMP_Variable::parse(const BYTE *data, size_t varLength)
{
   const BYTE *pbCurrPos;
   UINT32 type;
   size_t length, dwIdLength;
   SNMP_OID *oid;
   bool bResult = false;

   // Object ID
   if (!BER_DecodeIdentifier(data, varLength, &type, &length, &pbCurrPos, &dwIdLength))
      return false;
   if (type != ASN_OBJECT_ID)
      return false;

   oid = (SNMP_OID *)malloc(sizeof(SNMP_OID));
   memset(oid, 0, sizeof(SNMP_OID));
   if (BER_DecodeContent(type, pbCurrPos, length, (BYTE *)oid))
   {
      m_name.setValue(oid->value, (size_t)oid->length);
      varLength -= length + dwIdLength;
      pbCurrPos += length;
      bResult = TRUE;
   }
   safe_free(oid->value);
   free(oid);

   if (bResult)
   {
      bResult = FALSE;
      if (BER_DecodeIdentifier(pbCurrPos, varLength, &m_type, &length, &pbCurrPos, &dwIdLength))
      {
         switch(m_type)
         {
            case ASN_OBJECT_ID:
               oid = (SNMP_OID *)malloc(sizeof(SNMP_OID));
               memset(oid, 0, sizeof(SNMP_OID));
               if (BER_DecodeContent(m_type, pbCurrPos, length, (BYTE *)oid))
               {
                  m_valueLength = oid->length * sizeof(UINT32);
                  m_value = (BYTE *)oid->value;
                  bResult = true;
               }
               else
               {
                  safe_free(oid->value);
               }
               free(oid);
               break;
            case ASN_INTEGER:
            case ASN_COUNTER32:
            case ASN_GAUGE32:
            case ASN_TIMETICKS:
            case ASN_UINTEGER32:
               m_valueLength = sizeof(UINT32);
               m_value = (BYTE *)malloc(8);
               bResult = BER_DecodeContent(m_type, pbCurrPos, length, m_value);
               break;
		      case ASN_COUNTER64:
               m_valueLength = sizeof(QWORD);
               m_value = (BYTE *)malloc(16);
               bResult = BER_DecodeContent(m_type, pbCurrPos, length, m_value);
               break;
            default:
               m_valueLength = length;
               m_value = (BYTE *)nx_memdup(pbCurrPos, length);
               bResult = TRUE;
               break;
         }
      }
   }

   return bResult;
}

/**
 * Get raw value
 * Returns actual data length
 */
size_t SNMP_Variable::getRawValue(BYTE *buffer, size_t bufSize) const
{
	size_t len = std::min(bufSize, m_valueLength);
   memcpy(buffer, m_value, len);
	return len;
}

/**
 * Get value as unsigned integer
 */
UINT32 SNMP_Variable::getValueAsUInt() const
{
   UINT32 dwValue;

   switch(m_type)
   {
      case ASN_INTEGER:
      case ASN_COUNTER32:
      case ASN_GAUGE32:
      case ASN_TIMETICKS:
      case ASN_UINTEGER32:
      case ASN_IP_ADDR:
         dwValue = *((UINT32 *)m_value);
         break;
      case ASN_COUNTER64:
         dwValue = (UINT32)(*((QWORD *)m_value));
         break;
      default:
         dwValue = 0;
         break;
   }

   return dwValue;
}

/**
 * Get value as signed integer
 */
LONG SNMP_Variable::getValueAsInt() const
{
   LONG iValue;

   switch(m_type)
   {
      case ASN_INTEGER:
      case ASN_COUNTER32:
      case ASN_GAUGE32:
      case ASN_TIMETICKS:
      case ASN_UINTEGER32:
      case ASN_IP_ADDR:
         iValue = *((LONG *)m_value);
         break;
      case ASN_COUNTER64:
         iValue = (LONG)(*((QWORD *)m_value));
         break;
      default:
         iValue = 0;
         break;
   }

   return iValue;
}

/**
 * Get value as string
 * Note: buffer size is in characters
 */
TCHAR *SNMP_Variable::getValueAsString(TCHAR *buffer, size_t bufferSize) const
{
   size_t length;

   if ((buffer == NULL) || (bufferSize == 0))
      return NULL;

   switch(m_type)
   {
      case ASN_INTEGER:
         _sntprintf(buffer, bufferSize, _T("%d"), *((LONG *)m_value));
         break;
      case ASN_COUNTER32:
      case ASN_GAUGE32:
      case ASN_TIMETICKS:
      case ASN_UINTEGER32:
         _sntprintf(buffer, bufferSize, _T("%u"), *((UINT32 *)m_value));
         break;
      case ASN_COUNTER64:
         _sntprintf(buffer, bufferSize, UINT64_FMT, *((QWORD *)m_value));
         break;
      case ASN_IP_ADDR:
         if (bufferSize >= 16)
            IpToStr(ntohl(*((UINT32 *)m_value)), buffer);
         else
            buffer[0] = 0;
         break;
      case ASN_OBJECT_ID:
         SNMPConvertOIDToText(m_valueLength / sizeof(UINT32), (UINT32 *)m_value, buffer, bufferSize);
         break;
      case ASN_OCTET_STRING:
         length = std::min(bufferSize - 1, m_valueLength);
         if (length > 0)
         {
#ifdef UNICODE
            if (MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (char *)m_value, (int)length, buffer, (int)bufferSize) == 0)
            {
               // fallback if conversion fails
		         for(size_t i = 0; i < length; i++)
		         {
		            char c = ((char *)m_value)[i];
			         buffer[i] = (((BYTE)c & 0x80) == 0) ? c : '?';
		         }
            }
#else
            memcpy(buffer, m_value, length);
#endif
         }
         buffer[length] = 0;
         break;
      default:
         buffer[0] = 0;
         break;
   }
   return buffer;
}

/**
 * Get value as printable string, doing bin to hex conversion if necessary
 * Note: buffer size is in characters
 */
TCHAR *SNMP_Variable::getValueAsPrintableString(TCHAR *buffer, size_t bufferSize, bool *convertToHex) const
{
   size_t length;
	bool convertToHexAllowed = *convertToHex;
	*convertToHex = false;

   if ((buffer == NULL) || (bufferSize == 0))
      return NULL;

   if (m_type == ASN_OCTET_STRING)
	{
      length = std::min(bufferSize - 1, m_valueLength);
      if (length > 0)
      {
         bool conversionNeeded = false;
         if (convertToHexAllowed)
         {
            for(UINT32 i = 0; i < length; i++)
               if ((m_value[i] < 0x1F) && (m_value[i] != 0x0D) && (m_value[i] != 0x0A))
               {
                  if ((i == length - 1) && (m_value[i] == 0))
                     break;   // 0 at the end is OK
                  conversionNeeded = true;
                  break;
               }
         }

         if (!conversionNeeded)
         {
#ifdef UNICODE
            if (MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (char *)m_value, (int)length, buffer, (int)bufferSize) < (int)length)
            {
               if (convertToHexAllowed)
               {
                  conversionNeeded = true;
               }
               else
               {
                  // fallback if conversion fails
                  for(size_t i = 0; i < length; i++)
                  {
                     char c = ((char *)m_value)[i];
                     buffer[i] = (((BYTE)c & 0x80) == 0) ? c : '?';
                  }
               }
            }
#else
            memcpy(buffer, m_value, length);
#endif
            buffer[length] = 0;
         }

         if (conversionNeeded)
         {
            TCHAR *hexString = (TCHAR *)malloc((length * 3 + 1) * sizeof(TCHAR));
            UINT32 i, j;
            for(i = 0, j = 0; i < length; i++)
            {
               hexString[j++] = bin2hex(m_value[i] >> 4);
               hexString[j++] = bin2hex(m_value[i] & 15);
               hexString[j++] = _T(' ');
            }
            hexString[j] = 0;
            nx_strncpy(buffer, hexString, bufferSize);
            free(hexString);
            *convertToHex = true;
         }
         else
         {
            // Replace non-printable characters with question marks
            for(UINT32 i = 0; i < length; i++)
               if ((buffer[i] < 0x1F) && (buffer[i] != 0x0D) && (buffer[i] != 0x0A))
                  buffer[i] = _T('?');
         }
      }
      else
      {
         buffer[0] = 0;
      }
	}
	else
	{
		return getValueAsString(buffer, bufferSize);
	}

	return buffer;
}

/**
 * Get value as object id. Returned object must be destroyed by caller
 */
SNMP_ObjectId SNMP_Variable::getValueAsObjectId() const
{
   if (m_type != ASN_OBJECT_ID)
      return SNMP_ObjectId();
   return SNMP_ObjectId((UINT32 *)m_value, m_valueLength / sizeof(UINT32));
}

/**
 * Get value as MAC address
 */
TCHAR *SNMP_Variable::getValueAsMACAddr(TCHAR *buffer) const
{
   int i;
   TCHAR *pszPos;

   // MAC address usually encoded as octet string
   if ((m_type == ASN_OCTET_STRING) && (m_valueLength >= 6))
   {
      for(i = 0, pszPos = buffer; i < 6; i++, pszPos += 3)
         _sntprintf(pszPos, 4, _T("%02X:"), m_value[i]);
      *(pszPos - 1) = 0;
   }
   else
   {
      _tcscpy(buffer, _T("00:00:00:00:00:00"));
   }
   return buffer;
}

/**
 * Get value as IP address
 */
TCHAR *SNMP_Variable::getValueAsIPAddr(TCHAR *buffer) const
{
   // Ignore type and check only length
   if (m_valueLength >= 4)
   {
      IpToStr(ntohl(*((UINT32 *)m_value)), buffer);
   }
   else
   {
      _tcscpy(buffer, _T("0.0.0.0"));
   }
   return buffer;
}

/**
 * Encode variable using BER
 * Normally buffer provided should be at least m_valueLength + (name_length * 4) + 12 bytes
 * Return value is number of bytes actually used in buffer
 */
size_t SNMP_Variable::encode(BYTE *pBuffer, size_t bufferSize)
{
   size_t bytes, dwWorkBufSize;
   BYTE *pWorkBuf;

   dwWorkBufSize = (UINT32)(m_valueLength + m_name.length() * 4 + 16);
   pWorkBuf = (BYTE *)malloc(dwWorkBufSize);
   bytes = BER_Encode(ASN_OBJECT_ID, (BYTE *)m_name.value(),
                        m_name.length() * sizeof(UINT32),
                        pWorkBuf, dwWorkBufSize);
   bytes += BER_Encode(m_type, m_value, m_valueLength, 
                         pWorkBuf + bytes, dwWorkBufSize - bytes);
   bytes = BER_Encode(ASN_SEQUENCE, pWorkBuf, bytes, pBuffer, bufferSize);
   free(pWorkBuf);
   return bytes;
}

/**
 * Set variable from string
 */
void SNMP_Variable::setValueFromString(UINT32 type, const TCHAR *value)
{
   UINT32 *pdwBuffer;
   size_t length;

   m_type = type;
   switch(m_type)
   {
      case ASN_INTEGER:
         m_valueLength = sizeof(LONG);
         m_value = (BYTE *)realloc(m_value, m_valueLength);
         *((LONG *)m_value) = _tcstol(value, NULL, 0);
         break;
      case ASN_COUNTER32:
      case ASN_GAUGE32:
      case ASN_TIMETICKS:
      case ASN_UINTEGER32:
         m_valueLength = sizeof(UINT32);
         m_value = (BYTE *)realloc(m_value, m_valueLength);
         *((UINT32 *)m_value) = _tcstoul(value, NULL, 0);
         break;
      case ASN_COUNTER64:
         m_valueLength = sizeof(QWORD);
         m_value = (BYTE *)realloc(m_value, m_valueLength);
         *((QWORD *)m_value) = _tcstoull(value, NULL, 0);
         break;
      case ASN_IP_ADDR:
         m_valueLength = sizeof(UINT32);
         m_value = (BYTE *)realloc(m_value, m_valueLength);
         *((UINT32 *)m_value) = _t_inet_addr(value);
         break;
      case ASN_OBJECT_ID:
         pdwBuffer = (UINT32 *)malloc(sizeof(UINT32) * 256);
         length = SNMPParseOID(value, pdwBuffer, 256);
         if (length > 0)
         {
            m_valueLength = length * sizeof(UINT32);
            safe_free(m_value);
            m_value = (BYTE *)nx_memdup(pdwBuffer, m_valueLength);
         }
         else
         {
            // OID parse error, set to .ccitt.zeroDotZero (.0.0)
            m_valueLength = sizeof(UINT32) * 2;
            m_value = (BYTE *)realloc(m_value, m_valueLength);
            memset(m_value, 0, m_valueLength);
         }
         break;
      case ASN_OCTET_STRING:
         m_valueLength = (UINT32)_tcslen(value);
#ifdef UNICODE
         m_value = (BYTE *)realloc(m_value, m_valueLength);
         WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR,
                             value, (int)m_valueLength, (char *)m_value,
                             (int)m_valueLength, NULL, NULL);
#else
         safe_free(m_value);
         m_value = (BYTE *)nx_memdup(value, m_valueLength);
#endif
         break;
      default:
         break;
   }
}
