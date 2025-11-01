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
** File: variable.cpp
**
**/

#include "libnxsnmp.h"

/**
 * SNMP_Variable default constructor
 */
SNMP_Variable::SNMP_Variable()
{
   m_value = nullptr;
   m_valueLength = 0;
   m_type = ASN_NULL;
}

/**
 * Create variable of given type with empty value
 */
SNMP_Variable::SNMP_Variable(const TCHAR *name, uint32_t type)
{
   m_name = SNMP_ObjectId::parse(name);
   m_value = nullptr;
   m_valueLength = 0;
   m_type = type;
}

/**
 * Create variable of given type with empty value
 */
SNMP_Variable::SNMP_Variable(const uint32_t *name, size_t nameLen, uint32_t type) : m_name(name, nameLen)
{
   m_value = nullptr;
   m_valueLength = 0;
   m_type = type;
}

/**
 * Create variable of given type with empty value
 */
SNMP_Variable::SNMP_Variable(const SNMP_ObjectId &name, uint32_t type) : m_name(name)
{
   m_value = nullptr;
   m_valueLength = 0;
   m_type = type;
}

/**
 * Create variable of given type with empty value
 */
SNMP_Variable::SNMP_Variable(std::initializer_list<uint32_t> name, uint32_t type) : m_name(name)
{
   m_value = nullptr;
   m_valueLength = 0;
   m_type = type;
}

/**
 * Copy constructor
 */
SNMP_Variable::SNMP_Variable(const SNMP_Variable& src) : m_name(src.m_name), m_codepage(src.m_codepage)
{
   m_valueLength = src.m_valueLength;
   if ((m_valueLength <= SNMP_VARBIND_INTERNAL_BUFFER_SIZE) && (src.m_value != nullptr))
   {
      m_value = m_valueBuffer;
      memcpy(m_value, src.m_value, m_valueLength);
   }
   else
   {
      m_value = (src.m_value != nullptr) ? MemCopyBlock(src.m_value, src.m_valueLength) : nullptr;
   }
   m_type = src.m_type;
}

/**
 * Move constructor
 */
SNMP_Variable::SNMP_Variable(SNMP_Variable&& src) : m_name(std::move(src.m_name)), m_codepage(src.m_codepage)
{
   m_valueLength = src.m_valueLength;
   if ((m_valueLength <= SNMP_VARBIND_INTERNAL_BUFFER_SIZE) && (src.m_value != nullptr))
   {
      m_value = m_valueBuffer;
      memcpy(m_value, src.m_value, m_valueLength);
      if (src.m_value != src.m_valueBuffer)
         MemFreeAndNull(src.m_value);
   }
   else
   {
      m_value = src.m_value;
      src.m_value = nullptr;
   }
   src.m_valueLength = 0;
   m_type = src.m_type;
}

/**
 * SNMP_Variable destructor
 */
SNMP_Variable::~SNMP_Variable()
{
   if (m_value != m_valueBuffer)
      MemFree(m_value);
}

/**
 * Assignment operator
 */
SNMP_Variable& SNMP_Variable::operator=(const SNMP_Variable& src)
{
   if (this != &src)
   {
      m_name = src.m_name;
      m_codepage = src.m_codepage;

      m_valueLength = src.m_valueLength;
      if ((m_valueLength <= SNMP_VARBIND_INTERNAL_BUFFER_SIZE) && (src.m_value != nullptr))
      {
         if (m_value != m_valueBuffer)
            MemFree(m_value);
         m_value = m_valueBuffer;
         memcpy(m_value, src.m_value, m_valueLength);
      }
      else
      {
         if (m_value != m_valueBuffer)
            MemFree(m_value);
         m_value = (src.m_value != nullptr) ? MemCopyBlock(src.m_value, src.m_valueLength) : nullptr;
      }
      m_type = src.m_type;
   }
   return *this;
}

/**
 * Move assignment operator
 */
SNMP_Variable& SNMP_Variable::operator=(SNMP_Variable&& src)
{
   if (this != &src)
   {
      m_name = std::move(src.m_name);
      m_codepage = src.m_codepage;

      m_valueLength = src.m_valueLength;
      if ((m_valueLength <= SNMP_VARBIND_INTERNAL_BUFFER_SIZE) && (src.m_value != nullptr))
      {
         if (m_value != m_valueBuffer)
            MemFree(m_value);
         m_value = m_valueBuffer;
         memcpy(m_value, src.m_value, m_valueLength);
         if (src.m_value != src.m_valueBuffer)
            MemFreeAndNull(src.m_value);
      }
      else
      {
         if (m_value != m_valueBuffer)
            MemFree(m_value);
         m_value = src.m_value;
         src.m_value = nullptr;
      }
      src.m_valueLength = 0;
      m_type = src.m_type;
   }
   return *this;
}

/**
 * Decode variable record in PDU
 */
bool SNMP_Variable::decode(const BYTE *data, size_t varLength)
{
   // Object ID
   uint32_t type;
   size_t length, dwIdLength;
   const BYTE *pbCurrPos;
   if (!BER_DecodeIdentifier(data, varLength, &type, &length, &pbCurrPos, &dwIdLength))
      return false;
   if (type != ASN_OBJECT_ID)
      return false;

   bool success = false;
   SNMP_OID oid;
   if (BER_DecodeContent(type, pbCurrPos, length, (BYTE *)&oid))
   {
      m_name.setValue(oid.value, oid.length);
      varLength -= length + dwIdLength;
      pbCurrPos += length;
      success = true;
      if (oid.value != oid.internalBuffer)
      {
         MemFree(oid.value);
      }
   }

   if (success)
      success = decodeContent(pbCurrPos, varLength - length - dwIdLength, false);

   return success;
}

/**
 * Decode varbind content
 */
bool SNMP_Variable::decodeContent(const BYTE *data, size_t dataLength, bool enclosedInOpaque)
{
   const BYTE *pbCurrPos;
   size_t length, dwIdLength;
   if (!BER_DecodeIdentifier(data, dataLength, &m_type, &length, &pbCurrPos, &dwIdLength))
      return false;

   if (enclosedInOpaque && (m_type >= ASN_OPAQUE_TAG2))
      m_type -= ASN_OPAQUE_TAG2;

   bool success = false;
   switch(m_type)
   {
      case ASN_OBJECT_ID:
         SNMP_OID oid;
         if (BER_DecodeContent(m_type, pbCurrPos, length, reinterpret_cast<BYTE*>(&oid)))
         {
            m_valueLength = oid.length * sizeof(uint32_t);
            if (oid.value != oid.internalBuffer)
            {
               m_value = reinterpret_cast<BYTE*>(oid.value);
            }
            else
            {
               m_value = MemCopyBlock(reinterpret_cast<BYTE*>(oid.value), m_valueLength);
            }
            success = true;
         }
         break;
      case ASN_INTEGER:
      case ASN_COUNTER32:
      case ASN_GAUGE32:
      case ASN_TIMETICKS:
      case ASN_UINTEGER32:
         m_valueLength = sizeof(uint32_t);
         m_value = m_valueBuffer;
         success = BER_DecodeContent(m_type, pbCurrPos, length, m_value);
         break;
      case ASN_COUNTER64:
      case ASN_INTEGER64:
      case ASN_UINTEGER64:
         m_valueLength = sizeof(uint64_t);
         m_value = m_valueBuffer;
         success = BER_DecodeContent(m_type, pbCurrPos, length, m_value);
         break;
      case ASN_FLOAT:
         m_valueLength = sizeof(float);
         m_value = m_valueBuffer;
         success = BER_DecodeContent(m_type, pbCurrPos, length, m_value);
         break;
      default:
         m_valueLength = length;
         if (length <= SNMP_VARBIND_INTERNAL_BUFFER_SIZE)
         {
            m_value = m_valueBuffer;
            memcpy(m_value, pbCurrPos, length);
         }
         else
         {
            m_value = MemCopyBlock(pbCurrPos, length);
         }
         success = true;
         break;
   }
   return success;
}

/**
 * Check if value can be represented as integer
 */
bool SNMP_Variable::isInteger() const
{
   return (m_type == ASN_INTEGER) || (m_type == ASN_COUNTER32) ||
      (m_type == ASN_GAUGE32) || (m_type == ASN_TIMETICKS) ||
      (m_type == ASN_UINTEGER32) || (m_type == ASN_IP_ADDR) ||
      (m_type == ASN_COUNTER64) || (m_type == ASN_INTEGER64) ||
      (m_type == ASN_UINTEGER64);
}

/**
* Check if value can be represented as floating point number
*/
bool SNMP_Variable::isFloat() const
{
   return isInteger() || (m_type == ASN_DOUBLE) || (m_type == ASN_FLOAT);
}

/**
* Check if value can be represented as string
*/
bool SNMP_Variable::isString() const
{
   return isFloat() || (m_type == ASN_OCTET_STRING) || (m_type == ASN_OBJECT_ID);
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
uint32_t SNMP_Variable::getValueAsUInt() const
{
   switch(m_type)
   {
      case ASN_INTEGER:
      case ASN_COUNTER32:
      case ASN_GAUGE32:
      case ASN_TIMETICKS:
      case ASN_UINTEGER32:
      case ASN_IP_ADDR:
         return *reinterpret_cast<uint32_t*>(m_value);
      case ASN_COUNTER64:
      case ASN_INTEGER64:
      case ASN_UINTEGER64:
         return static_cast<uint32_t>(*reinterpret_cast<uint64_t*>(m_value));
      case ASN_FLOAT:
         return static_cast<uint32_t>(*reinterpret_cast<float*>(m_value));
      case ASN_DOUBLE:
         return static_cast<uint32_t>(*reinterpret_cast<double*>(m_value));
      default:
         return 0;
   }
}

/**
 * Get value as signed integer
 */
int32_t SNMP_Variable::getValueAsInt() const
{
   switch(m_type)
   {
      case ASN_INTEGER:
      case ASN_COUNTER32:
      case ASN_GAUGE32:
      case ASN_TIMETICKS:
      case ASN_UINTEGER32:
      case ASN_IP_ADDR:
         return *reinterpret_cast<int32_t*>(m_value);
      case ASN_COUNTER64:
      case ASN_INTEGER64:
      case ASN_UINTEGER64:
         return static_cast<int32_t>(*reinterpret_cast<int64_t*>(m_value));
      case ASN_FLOAT:
         return static_cast<int32_t>(*reinterpret_cast<float*>(m_value));
      case ASN_DOUBLE:
         return static_cast<int32_t>(*reinterpret_cast<double*>(m_value));
      default:
         return 0;
   }
}

/**
 * Get value as 64 bit unsigned integer
 */
uint64_t SNMP_Variable::getValueAsUInt64() const
{
   switch(m_type)
   {
      case ASN_INTEGER:
      case ASN_COUNTER32:
      case ASN_GAUGE32:
      case ASN_TIMETICKS:
      case ASN_UINTEGER32:
      case ASN_IP_ADDR:
         return *reinterpret_cast<uint32_t*>(m_value);
      case ASN_COUNTER64:
      case ASN_INTEGER64:
      case ASN_UINTEGER64:
         return *reinterpret_cast<uint64_t*>(m_value);
      case ASN_FLOAT:
         return static_cast<uint64_t>(*reinterpret_cast<float*>(m_value));
      case ASN_DOUBLE:
         return static_cast<uint64_t>(*reinterpret_cast<double*>(m_value));
      default:
         return 0;
   }
}

/**
 * Get value as 64 bit signed integer
 */
int64_t SNMP_Variable::getValueAsInt64() const
{
   switch(m_type)
   {
      case ASN_INTEGER:
      case ASN_COUNTER32:
      case ASN_GAUGE32:
      case ASN_TIMETICKS:
         return *reinterpret_cast<int32_t*>(m_value);
      case ASN_IP_ADDR:
      case ASN_UINTEGER32:
         return *reinterpret_cast<uint32_t*>(m_value);
      case ASN_COUNTER64:
      case ASN_INTEGER64:
      case ASN_UINTEGER64:
         return *reinterpret_cast<int64_t*>(m_value);
      case ASN_FLOAT:
         return static_cast<int64_t>(*reinterpret_cast<float*>(m_value));
      case ASN_DOUBLE:
         return static_cast<int64_t>(*reinterpret_cast<double*>(m_value));
      default:
         return 0;
   }
}

/**
 * Get value as double precision floating point number
 */
double SNMP_Variable::getValueAsDouble() const
{
   switch(m_type)
   {
      case ASN_INTEGER:
      case ASN_COUNTER32:
      case ASN_GAUGE32:
      case ASN_TIMETICKS:
         return *reinterpret_cast<int32_t*>(m_value);
      case ASN_IP_ADDR:
      case ASN_UINTEGER32:
         return *reinterpret_cast<uint32_t*>(m_value);
      case ASN_COUNTER64:
      case ASN_INTEGER64:
      case ASN_UINTEGER64:
         return static_cast<double>(*reinterpret_cast<int64_t*>(m_value));
      case ASN_FLOAT:
         return *reinterpret_cast<float*>(m_value);
      case ASN_DOUBLE:
         return *reinterpret_cast<double*>(m_value);
      default:
         return 0;
   }
}

/**
 * Get value as string
 * Note: buffer size is in characters
 */
TCHAR *SNMP_Variable::getValueAsString(TCHAR *buffer, size_t bufferSize, const char *codepage) const
{
   if ((buffer == nullptr) || (bufferSize == 0))
      return nullptr;

   size_t length;
   SNMP_Variable *v;

   switch(m_type)
   {
      case ASN_INTEGER:
         _sntprintf(buffer, bufferSize, _T("%d"), *reinterpret_cast<int32_t*>(m_value));
         break;
      case ASN_COUNTER32:
      case ASN_GAUGE32:
      case ASN_TIMETICKS:
      case ASN_UINTEGER32:
         _sntprintf(buffer, bufferSize, _T("%u"), *reinterpret_cast<uint32_t*>(m_value));
         break;
      case ASN_INTEGER64:
         _sntprintf(buffer, bufferSize, INT64_FMT, *reinterpret_cast<int64_t*>(m_value));
         break;
      case ASN_COUNTER64:
      case ASN_UINTEGER64:
         _sntprintf(buffer, bufferSize, UINT64_FMT, *reinterpret_cast<uint64_t*>(m_value));
         break;
      case ASN_FLOAT:
         _sntprintf(buffer, bufferSize, _T("%f"), *reinterpret_cast<float*>(m_value));
         break;
      case ASN_DOUBLE:
         _sntprintf(buffer, bufferSize, _T("%f"), *reinterpret_cast<double*>(m_value));
         break;
      case ASN_IP_ADDR:
         if (bufferSize >= 16)
            IpToStr(ntohl(*reinterpret_cast<uint32_t*>(m_value)), buffer);
         else
            buffer[0] = 0;
         break;
      case ASN_OBJECT_ID:
         SnmpConvertOIDToText(m_valueLength / sizeof(uint32_t), reinterpret_cast<uint32_t*>(m_value), buffer, bufferSize);
         break;
      case ASN_OCTET_STRING:
         length = std::min(bufferSize - 1, m_valueLength);
         if (length > 0)
         {
#ifdef UNICODE
            size_t cch = mbcp_to_wchar(reinterpret_cast<const char *>(m_value), length, buffer, bufferSize, m_codepage.effectiveValue(codepage));
            if (cch > 0)
            {
               length = cch;  // length can be different for multibyte character set
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
#else
            memcpy(buffer, m_value, length);
#endif
         }
         buffer[length] = 0;
         break;
      case ASN_OPAQUE:
         v = decodeOpaque();
         if (v != nullptr)
         {
            v->getValueAsString(buffer, bufferSize);
            delete v;
         }
         else
         {
            buffer[0] = 0;
         }
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
TCHAR *SNMP_Variable::getValueAsPrintableString(TCHAR *buffer, size_t bufferSize, bool *convertToHex, const char *codepage) const
{
   if ((buffer == nullptr) || (bufferSize == 0))
      return nullptr;

   size_t length;
	bool convertToHexAllowed = *convertToHex;
	*convertToHex = false;

   if (m_type == ASN_OCTET_STRING)
	{
      length = std::min(bufferSize - 1, m_valueLength);
      if (length > 0)
      {
         bool conversionNeeded = false;
         if (convertToHexAllowed)
         {
            for(size_t i = 0; i < length; i++)
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
            size_t cch = mbcp_to_wchar((char *)m_value, length, buffer, bufferSize, m_codepage.effectiveValue(codepage));
            if (cch > 0)
            {
               length = cch;  // length can be different for multibyte character set
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
#else
            memcpy(buffer, m_value, length);
#endif
            buffer[length] = 0;
         }

         if (conversionNeeded)
         {
            size_t bufferLen = (length * 3 + 1) * sizeof(TCHAR);
            TCHAR *hexString = static_cast<TCHAR*>(SNMP_MemAlloc(bufferLen));
            size_t i, j;
            for(i = 0, j = 0; i < length; i++)
            {
               hexString[j++] = bin2hex(m_value[i] >> 4);
               hexString[j++] = bin2hex(m_value[i] & 15);
               hexString[j++] = _T(' ');
            }
            hexString[j] = 0;
            _tcslcpy(buffer, hexString, bufferSize);
            SNMP_MemFree(hexString, bufferLen);
            *convertToHex = true;
         }
         else
         {
            // Replace non-printable characters with question marks
            for(size_t i = 0; i < length; i++)
            {
               if ((buffer[i] == 0) && (i == length - 1))
                  break;   // 0 at the end is OK
               if ((buffer[i] < 0x1F) && (buffer[i] != 0x0D) && (buffer[i] != 0x0A))
                  buffer[i] = _T('?');
            }
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
MacAddress SNMP_Variable::getValueAsMACAddr() const
{
   // MAC address usually encoded as octet string
   if ((m_type == ASN_OCTET_STRING) && (m_valueLength >= 6))
   {
      return MacAddress(m_value, m_valueLength);
   }
   else
   {
      return MacAddress(6);   // return 00:00:00:00:00:00 if value cannot be interpreted as MAC address
   }
}

/**
 * Get value as IP address
 */
TCHAR *SNMP_Variable::getValueAsIPAddr(TCHAR *buffer) const
{
   // Ignore type and check only length
   if (m_valueLength >= 4)
   {
      IpToStr(ntohl(*reinterpret_cast<uint32_t*>(m_value)), buffer);
   }
   else
   {
      _tcscpy(buffer, _T("0.0.0.0"));
   }
   return buffer;
}

/**
 * Decode opaque value
 */
SNMP_Variable *SNMP_Variable::decodeOpaque() const
{
   // Length should be at least 4 to accommodate 2 byte tag and inner length
   if ((m_type != ASN_OPAQUE) || (m_valueLength < 3) || (*m_value != ASN_OPAQUE_TAG1))
      return nullptr;

   SNMP_Variable *v = new SNMP_Variable(m_name);
   if (!v->decodeContent(m_value + 1, m_valueLength - 1, true))
      delete_and_null(v);
   return v;
}

/**
 * Encode variable using BER
 * Normally buffer provided should be at least m_valueLength + (name_length * 4) + 12 bytes
 * Return value is number of bytes actually used in buffer
 */
size_t SNMP_Variable::encode(BYTE *pBuffer, size_t bufferSize) const
{
   size_t bytes, dwWorkBufSize;
   BYTE *pWorkBuf;

   dwWorkBufSize = (UINT32)(m_valueLength + m_name.length() * 4 + 16);
   pWorkBuf = static_cast<BYTE*>(SNMP_MemAlloc(dwWorkBufSize));
   bytes = BER_Encode(ASN_OBJECT_ID, (BYTE *)m_name.value(), m_name.length() * sizeof(uint32_t), pWorkBuf, dwWorkBufSize);
   bytes += BER_Encode(m_type, m_value, m_valueLength, pWorkBuf + bytes, dwWorkBufSize - bytes);
   bytes = BER_Encode(ASN_SEQUENCE, pWorkBuf, bytes, pBuffer, bufferSize);
   SNMP_MemFree(pWorkBuf, dwWorkBufSize);
   return bytes;
}

/**
 * Set variable from string
 */
void SNMP_Variable::setValueFromString(uint32_t type, const TCHAR *value, const char *codepage)
{
   uint32_t *pdwBuffer;
   size_t length;

   m_type = type;
   switch(m_type)
   {
      case ASN_INTEGER:
         reallocValueBuffer(sizeof(int32_t));
         *reinterpret_cast<int32_t*>(m_value) = _tcstol(value, nullptr, 0);
         break;
      case ASN_COUNTER32:
      case ASN_GAUGE32:
      case ASN_TIMETICKS:
      case ASN_UINTEGER32:
         reallocValueBuffer(sizeof(uint32_t));
         *reinterpret_cast<uint32_t*>(m_value) = _tcstoul(value, nullptr, 0);
         break;
      case ASN_COUNTER64:
         reallocValueBuffer(sizeof(uint64_t));
         *reinterpret_cast<uint64_t*>(m_value) = _tcstoull(value, nullptr, 0);
         break;
      case ASN_IP_ADDR:
         reallocValueBuffer(sizeof(uint32_t));
         *reinterpret_cast<uint32_t*>(m_value) = htonl(InetAddress::parse(value).getAddressV4());
         break;
      case ASN_OBJECT_ID:
         pdwBuffer = MemAllocArrayNoInit<uint32_t>(256);
         length = SnmpParseOID(value, pdwBuffer, 256);
         if (length > 0)
         {
            reallocValueBuffer(length * sizeof(uint32_t));
            memcpy(m_value, pdwBuffer, m_valueLength);
         }
         else
         {
            // OID parse error, set to .ccitt.zeroDotZero (.0.0)
            reallocValueBuffer(sizeof(uint32_t) * 2);
            memset(m_value, 0, m_valueLength);
         }
         break;
      case ASN_OCTET_STRING:
#ifdef UNICODE
         length = wcslen(value) * 3;
         reallocValueBuffer(length);
         // wchar_to_mbcp returns number of characters in converted string including terminating \0 which should be excluded from value being sent
         m_valueLength = wchar_to_mbcp(value, -1, reinterpret_cast<char*>(m_value), length, m_codepage.effectiveValue(codepage)) - 1;
#else
         length = strlen(value);
         reallocValueBuffer(length);
         memcpy(m_value, value, length);
#endif
         break;
      default:
         m_type = ASN_NULL;
         if (m_value != m_valueBuffer)
            MemFree(m_value);
         m_value = nullptr;
         m_valueLength = 0;
         break;
   }
}

/**
 * Set variable from unsigned integer
 */
void SNMP_Variable::setValueFromUInt32(uint32_t type, uint32_t value)
{
   m_type = type;
   switch(m_type)
   {
      case ASN_INTEGER:
         reallocValueBuffer(sizeof(int32_t));
         *reinterpret_cast<int32_t*>(m_value) = value;
         break;
      case ASN_COUNTER32:
      case ASN_GAUGE32:
      case ASN_TIMETICKS:
      case ASN_UINTEGER32:
         reallocValueBuffer(sizeof(uint32_t));
         *reinterpret_cast<int32_t*>(m_value) = value;
         break;
      case ASN_COUNTER64:
         reallocValueBuffer(sizeof(uint64_t));
         *reinterpret_cast<uint64_t*>(m_value) = value;
         break;
      case ASN_IP_ADDR:
         reallocValueBuffer(sizeof(uint32_t));
         *reinterpret_cast<uint32_t*>(m_value) = htonl(value);
         break;
      case ASN_OBJECT_ID:
         reallocValueBuffer(sizeof(uint32_t));
         *reinterpret_cast<uint32_t*>(m_value) = htonl(value);
         break;
      case ASN_OCTET_STRING:
         reallocValueBuffer(16);
         snprintf(reinterpret_cast<char*>(m_value), 16, "%u", value);
         m_valueLength = strlen(reinterpret_cast<char*>(m_value));
         break;
      default:
         m_type = ASN_NULL;
         if (m_value != m_valueBuffer)
            MemFree(m_value);
         m_value = nullptr;
         m_valueLength = 0;
         break;
   }
}

/**
 * Set variable from unsigned integer
 */
void SNMP_Variable::setValueFromObjectId(uint32_t type, const SNMP_ObjectId& value)
{
   m_type = type;
   switch(m_type)
   {
      case ASN_OBJECT_ID:
         reallocValueBuffer(value.length() * sizeof(uint32_t));
         memcpy(m_value, value.value(), m_valueLength);
         break;
      case ASN_OCTET_STRING:
         if (m_value != m_valueBuffer)
            MemFree(m_value);
         m_value = reinterpret_cast<BYTE*>(value.toString().getUTF8String());
         m_valueLength = strlen(reinterpret_cast<char*>(m_value));
         break;
      default:
         m_type = ASN_NULL;
         if (m_value != m_valueBuffer)
            MemFree(m_value);
         m_value = nullptr;
         m_valueLength = 0;
         break;
   }
}

/**
 * Set variable from byte array
 */
void SNMP_Variable::setValueFromByteArray(uint32_t type, const BYTE* data, size_t size)
{
   m_type = type;
   reallocValueBuffer(size);
   memcpy(m_value, data, size);
}
