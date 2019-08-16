/* 
** NetXMS - Network Management System
** NetXMS Foundation Library
** Copyright (C) 2003-2019 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation; either version 3 of the License, or
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
** File: message.cpp
**
**/
#include "libnetxms.h"
#include <nxcpapi.h>
#include <zlib.h>

#undef uthash_malloc
#define uthash_malloc(sz) m_pool.allocate(sz)
#undef uthash_free
#define uthash_free(ptr,sz) do { } while(0)

#include <uthash.h>

/**
 * Calculate field size
 */
static size_t CalculateFieldSize(const NXCP_MESSAGE_FIELD *field, bool networkByteOrder)
{
   size_t nSize;

   switch(field->type)
   {
      case NXCP_DT_INT32:
         nSize = 12;
         break;
      case NXCP_DT_INT64:
      case NXCP_DT_FLOAT:
         nSize = 16;
         break;
      case NXCP_DT_INT16:
         nSize = 8;
         break;
      case NXCP_DT_INETADDR:
         nSize = 32;
         break;
      case NXCP_DT_STRING:
      case NXCP_DT_UTF8_STRING:
      case NXCP_DT_BINARY:
         if (networkByteOrder)
            nSize = ntohl(field->df_string.length) + 12;
         else
            nSize = field->df_string.length + 12;
         break;
      default:
         nSize = 8;
         break;
   }
   return nSize;
}

/**
 * Field hash map entry
 */
struct MessageField
{
   UT_hash_handle hh;
   UINT32 id;
   size_t size;
   NXCP_MESSAGE_FIELD data;
};

/**
 * Create new hash entry with given field size
 */
inline MessageField *CreateMessageField(MemoryPool& pool, size_t fieldSize)
{
   size_t entrySize = sizeof(MessageField) - sizeof(NXCP_MESSAGE_FIELD) + fieldSize;
   MessageField *entry = static_cast<MessageField*>(pool.allocate(entrySize));
   memset(entry, 0, entrySize);
   entry->size = entrySize;
   return entry;
}

/**
 * Default constructor for NXCPMessage class
 */
NXCPMessage::NXCPMessage(int version) : m_pool(NXCP_DEFAULT_SIZE_HINT)
{
   m_code = 0;
   m_id = 0;
   m_fields = NULL;
   m_flags = 0;
   m_version = version;
   m_data = NULL;
   m_dataSize = 0;
}

/**
 * Create message with given code and ID
 */
NXCPMessage::NXCPMessage(UINT16 code, UINT32 id, int version) : m_pool(NXCP_DEFAULT_SIZE_HINT)
{
   m_code = code;
   m_id = id;
   m_fields = NULL;
   m_flags = 0;
   m_version = version;
   m_data = NULL;
   m_dataSize = 0;
}

/**
 * Create a copy of prepared CSCP message
 */
NXCPMessage::NXCPMessage(NXCPMessage *msg) : m_pool(msg->m_pool.regionSize())
{
   m_code = msg->m_code;
   m_id = msg->m_id;
   m_flags = msg->m_flags;
   m_version = msg->m_version;
   m_fields = NULL;

   if (m_flags & MF_BINARY)
   {
      m_dataSize = msg->m_dataSize;
      m_data = m_pool.copyMemoryBlock(msg->m_data, m_dataSize);
   }
   else
   {
      m_data = NULL;
      m_dataSize = 0;

      MessageField *entry, *tmp;
      HASH_ITER(hh, msg->m_fields, entry, tmp)
      {
         MessageField *f = m_pool.copyMemoryBlock(entry, entry->size);
         HASH_ADD_INT(m_fields, id, f);
      }
   }
}

/**
 * Create NXCPMessage object from serialized message
 *
 * @return message object or NULL on failure
 */
NXCPMessage *NXCPMessage::deserialize(const NXCP_MESSAGE *rawMsg, int version)
{
   NXCPMessage *msg = new NXCPMessage(rawMsg, version);
   if (msg->isValid())
      return msg;
   delete msg;
   return NULL;
}

/**
 * Calculate size hint for message memory pool
 */
inline size_t SizeHint(const NXCP_MESSAGE *msg)
{
   size_t size = ntohl(msg->size);
   return size + 4096 - (size % 4096);
}

/**
 * Create NXCPMessage object from serialized message
 */
NXCPMessage::NXCPMessage(const NXCP_MESSAGE *msg, int version) : m_pool(SizeHint(msg))
{
   m_flags = ntohs(msg->flags);
   m_code = ntohs(msg->code);
   m_id = ntohl(msg->id);
   m_fields = NULL;

   int v = getEncodedProtocolVersion();
   m_version = (v != 0) ? v : version; // Use encoded version if present

   // Parse data fields
   if (m_flags & MF_BINARY)
   {
      m_dataSize = (size_t)ntohl(msg->numFields);
      if ((m_flags & MF_COMPRESSED) && !(m_flags & MF_STREAM) && (m_version >= 4))
      {
         m_flags &= ~MF_COMPRESSED; // clear "compressed" flag so it will not be mistakenly re-sent

         z_stream stream;
         stream.zalloc = Z_NULL;
         stream.zfree = Z_NULL;
         stream.opaque = Z_NULL;
         stream.avail_in = (UINT32)ntohl(msg->size) - NXCP_HEADER_SIZE - 4;
         stream.next_in = (BYTE *)msg + NXCP_HEADER_SIZE + 4;
         if (inflateInit(&stream) != Z_OK)
         {
            nxlog_debug(6, _T("NXCPMessage: inflateInit() failed"));
            m_version = -1;   // error indicator
            return;
         }

         m_data = m_pool.allocateArray<BYTE>(m_dataSize);
         stream.next_out = m_data;
         stream.avail_out = (UINT32)m_dataSize;

         if (inflate(&stream, Z_FINISH) != Z_STREAM_END)
         {
            inflateEnd(&stream);
            TCHAR buffer[256];
            nxlog_debug(6, _T("NXCPMessage: failed to decompress binary message %s with ID %d"), NXCPMessageCodeName(m_code, buffer), m_id);
            m_version = -1;   // error indicator
            return;
         }
         inflateEnd(&stream);
      }
      else
      {
         m_data = m_pool.copyMemoryBlock(reinterpret_cast<const BYTE*>(msg->fields), m_dataSize);
      }
   }
   else
   {
      m_data = NULL;
      m_dataSize = 0;

      BYTE *msgData;
      size_t msgDataSize;
      if ((m_flags & MF_COMPRESSED) && (m_version >= 4))
      {
         m_flags &= ~MF_COMPRESSED; // clear "compressed" flag so it will not be mistakenly re-sent
         msgDataSize = (size_t)ntohl(*((UINT32 *)((BYTE *)msg + NXCP_HEADER_SIZE))) - NXCP_HEADER_SIZE;

         z_stream stream;
         stream.zalloc = Z_NULL;
         stream.zfree = Z_NULL;
         stream.opaque = Z_NULL;
         stream.avail_in = (UINT32)ntohl(msg->size) - NXCP_HEADER_SIZE - 4;
         stream.next_in = (BYTE *)msg + NXCP_HEADER_SIZE + 4;
         if (inflateInit(&stream) != Z_OK)
         {
            nxlog_debug(6, _T("NXCPMessage: inflateInit() failed"));
            m_version = -1;   // error indicator
            return;
         }

         msgData = m_pool.allocateArray<BYTE>(msgDataSize);
         stream.next_out = msgData;
         stream.avail_out = (UINT32)msgDataSize;

         if (inflate(&stream, Z_FINISH) != Z_STREAM_END)
         {
            inflateEnd(&stream);
            TCHAR buffer[256];
            nxlog_debug(6, _T("NXCPMessage: failed to decompress message %s with ID %d"), NXCPMessageCodeName(m_code, buffer), m_id);
            m_version = -1;   // error indicator
            return;
         }
         inflateEnd(&stream);
      }
      else
      {
         msgData = (BYTE *)msg + NXCP_HEADER_SIZE;
         msgDataSize = (size_t)ntohl(msg->size) - NXCP_HEADER_SIZE;
      }

      int fieldCount = (int)ntohl(msg->numFields);
      size_t pos = 0;
      for(int f = 0; f < fieldCount; f++)
      {
         NXCP_MESSAGE_FIELD *field = (NXCP_MESSAGE_FIELD *)(msgData + pos);

         // Validate position inside message
         if (pos > msgDataSize - 8)
         {
            m_version = -1;   // error indicator
            break;
         }
         if ((pos > msgDataSize - 12) &&
             ((field->type == NXCP_DT_STRING) || (field->type == NXCP_DT_UTF8_STRING) || (field->type == NXCP_DT_BINARY)))
         {
            m_version = -1;   // error indicator
            break;
         }

         // Calculate and validate field size
         size_t fieldSize = CalculateFieldSize(field, true);
         if (pos + fieldSize > msgDataSize)
         {
            m_version = -1;   // error indicator
            break;
         }

         // Create new entry
         MessageField *entry = CreateMessageField(m_pool, fieldSize);
         entry->id = ntohl(field->fieldId);
         memcpy(&entry->data, field, fieldSize);

         // Convert values to host format
         entry->data.fieldId = ntohl(entry->data.fieldId);
         switch(field->type)
         {
            case NXCP_DT_INT32:
               entry->data.df_int32 = ntohl(entry->data.df_int32);
               break;
            case NXCP_DT_INT64:
               entry->data.df_int64 = ntohq(entry->data.df_int64);
               break;
            case NXCP_DT_INT16:
               entry->data.df_int16 = ntohs(entry->data.df_int16);
               break;
            case NXCP_DT_FLOAT:
               entry->data.df_real = ntohd(entry->data.df_real);
               break;
            case NXCP_DT_STRING:
#if !(WORDS_BIGENDIAN)
               entry->data.df_string.length = ntohl(entry->data.df_string.length);
               bswap_array_16(entry->data.df_string.value, entry->data.df_string.length / 2);
#endif
               break;
            case NXCP_DT_BINARY:
               entry->data.df_binary.length = ntohl(entry->data.df_binary.length);
               break;
            case NXCP_DT_UTF8_STRING:
               entry->data.df_utf8string.length = ntohl(entry->data.df_utf8string.length);
               break;
            case NXCP_DT_INETADDR:
               if (entry->data.df_inetaddr.family == NXCP_AF_INET)
               {
                  entry->data.df_inetaddr.addr.v4 = ntohl(entry->data.df_inetaddr.addr.v4);
               }
               break;
         }

         HASH_ADD_INT(m_fields, id, entry);

         // Starting from version 2, all variables should be 8-byte aligned
         if (m_version >= 2)
            pos += fieldSize + ((8 - (fieldSize % 8)) & 7);
         else
            pos += fieldSize;
      }
   }
}

/**
 * Destructor for NXCPMessage
 */
NXCPMessage::~NXCPMessage()
{
}

/**
 * Find field by ID
 */
NXCP_MESSAGE_FIELD *NXCPMessage::find(UINT32 fieldId) const
{
   MessageField *entry;
   HASH_FIND_INT(m_fields, &fieldId, entry);
   return (entry != NULL) ? &entry->data : NULL;
}

/**
 * Set field
 * Argument size (data size) contains data length in bytes for DT_BINARY type
 * and maximum number of characters for DT_STRING and DT_UTF8_STRING types (0 means no limit)
 */
void *NXCPMessage::set(UINT32 fieldId, BYTE type, const void *value, bool isSigned, size_t size, bool isUtf8)
{
   if (m_flags & MF_BINARY)
      return NULL;

   size_t length, bufferLength;
#if defined(UNICODE_UCS2) && defined(UNICODE)
#define __buffer value
#else
   UCS2CHAR *__buffer, localBuffer[256];
#endif

   // Create entry
   MessageField *entry;
   switch(type)
   {
      case NXCP_DT_INT32:
         entry = CreateMessageField(m_pool, 12);
         entry->data.df_int32 = *((const UINT32 *)value);
         break;
      case NXCP_DT_INT16:
         entry = CreateMessageField(m_pool, 8);
         entry->data.df_int16 = *((const WORD *)value);
         break;
      case NXCP_DT_INT64:
         entry = CreateMessageField(m_pool, 16);
         entry->data.df_int64 = *((const UINT64 *)value);
         break;
      case NXCP_DT_FLOAT:
         entry = CreateMessageField(m_pool, 16);
         entry->data.df_real = *((const double *)value);
         break;
      case NXCP_DT_STRING:
         if (isUtf8)
         {
            length = utf8_ucs2len(static_cast<const char*>(value), -1) - 1;
            if ((size > 0) && (length > size))
               length = size;
#if defined(UNICODE_UCS2) && defined(UNICODE)
            UCS2CHAR localBuffer[256];
            UCS2CHAR *___buffer = (length < 256) ? localBuffer : m_pool.allocateArray<UCS2CHAR>(length + 1);
            utf8_to_ucs2(static_cast<const char*>(value), -1, ___buffer, length + 1);
            entry = CreateMessageField(m_pool, 12 + length * 2);
            entry->data.df_string.length = (UINT32)(length * 2);
            memcpy(entry->data.df_string.value, ___buffer, entry->data.df_string.length);
#else
            __buffer = (length < 256) ? localBuffer : m_pool.allocateArray<UCS2CHAR>(length + 1);
            utf8_to_ucs2(static_cast<const char*>(value), -1, __buffer, length + 1);
            entry = CreateMessageField(m_pool, 12 + length * 2);
            entry->data.df_string.length = (UINT32)(length * 2);
            memcpy(entry->data.df_string.value, __buffer, entry->data.df_string.length);
#endif
         }
         else
         {
            length = _tcslen(static_cast<const TCHAR*>(value));
            if ((size > 0) && (length > size))
               length = size;
#ifdef UNICODE         
#ifndef UNICODE_UCS2 /* assume UNICODE_UCS4 */
            __buffer = (length < 256) ? localBuffer : m_pool.allocateArray<UCS2CHAR>(length + 1);
            ucs4_to_ucs2(static_cast<const WCHAR*>(value), length, __buffer, length + 1);
#endif         
#else		/* not UNICODE */
            __buffer = (length < 256) ? localBuffer : m_pool.allocateArray<UCS2CHAR>(length + 1);
            mb_to_ucs2(static_cast<const char*>(value), length, __buffer, length + 1);
#endif
            entry = CreateMessageField(m_pool, 12 + length * 2);
            entry->data.df_string.length = (UINT32)(length * 2);
            memcpy(entry->data.df_string.value, __buffer, entry->data.df_string.length);
         }
         break;
      case NXCP_DT_UTF8_STRING:
         if (isUtf8)
         {
            length = strlen(static_cast<const char*>(value));
            if ((size > 0) && (length > size))
               length = size;
            entry = CreateMessageField(m_pool, 12 + length);
            entry->data.df_utf8string.length = (UINT32)length;
            memcpy(entry->data.df_utf8string.value, value, length);
         }
         else
         {
            length = _tcslen(static_cast<const TCHAR*>(value));
            if ((size > 0) && (length > size))
               length = size;
#ifdef UNICODE
#ifdef UNICODE_UCS4
            bufferLength = ucs4_utf8len(static_cast<const TCHAR*>(value), length);
#else
            bufferLength = ucs2_utf8len(static_cast<const TCHAR*>(value), length);
#endif
#else    /* not UNICODE */
            bufferLength = length * 3;
#endif
            entry = CreateMessageField(m_pool, 12 + bufferLength);
#ifdef UNICODE
#ifdef UNICODE_UCS4
            entry->data.df_utf8string.length = (UINT32)ucs4_to_utf8(static_cast<const TCHAR*>(value), length, entry->data.df_utf8string.value, bufferLength);
#else
            entry->data.df_utf8string.length = (UINT32)ucs2_to_utf8(static_cast<const TCHAR*>(value), length, entry->data.df_utf8string.value, bufferLength);
#endif
#else    /* not UNICODE */
            entry->data.df_utf8string.length = (UINT32)mb_to_utf8(static_cast<const TCHAR*>(value), length, entry->data.df_utf8string.value, (int)bufferLength);
#endif
         }
         break;
      case NXCP_DT_BINARY:
         entry = CreateMessageField(m_pool, 12 + size);
         entry->data.df_binary.length = (UINT32)size;
         if ((entry->data.df_binary.length > 0) && (value != NULL))
            memcpy(entry->data.df_binary.value, value, entry->data.df_binary.length);
         break;
      case NXCP_DT_INETADDR:
         entry = CreateMessageField(m_pool, 32);
         entry->data.df_inetaddr.family =
                  (((InetAddress *)value)->getFamily() == AF_INET) ? NXCP_AF_INET :
                           ((((InetAddress *)value)->getFamily() == AF_INET6) ? NXCP_AF_INET6 : NXCP_AF_UNSPEC);
         entry->data.df_inetaddr.maskBits = (BYTE)((InetAddress *)value)->getMaskBits();
         if (((InetAddress *)value)->getFamily() == AF_INET)
         {
            entry->data.df_inetaddr.addr.v4 = ((InetAddress *)value)->getAddressV4();
         }
         else if (((InetAddress *)value)->getFamily() == AF_INET6)
         {
            memcpy(entry->data.df_inetaddr.addr.v6, ((InetAddress *)value)->getAddressV6(), 16);
         }
         break;
      default:
         return NULL;  // Invalid data type, unable to handle
   }
   entry->id = fieldId;
   entry->data.fieldId = fieldId;
   entry->data.type = type;
   if (isSigned)
      entry->data.flags |= NXCP_MFF_SIGNED;

   // add or replace field
   MessageField *curr;
   HASH_FIND_INT(m_fields, &fieldId, curr);
   if (curr != NULL)
   {
      HASH_DEL(m_fields, curr);
   }
   HASH_ADD_INT(m_fields, id, entry);

   return (type == NXCP_DT_INT16) ? ((void *)((BYTE *)&entry->data + 6)) : ((void *)((BYTE *)&entry->data + 8));
#undef __buffer
}

/**
 * Get field value
 */
void *NXCPMessage::get(UINT32 fieldId, BYTE requiredType, BYTE *fieldType) const
{
   NXCP_MESSAGE_FIELD *field = find(fieldId);
   if (field == NULL)
      return NULL;      // No such field

   // Data type check exception - return IPv4 address as INT32 if requested
   if ((requiredType == NXCP_DT_INT32) && (field->type == NXCP_DT_INETADDR) && (field->df_inetaddr.family == NXCP_AF_INET))
      return &field->df_inetaddr.addr.v4;

   // Check data type
   if ((requiredType != 0xFF) && (field->type != requiredType))
      return NULL;

   if (fieldType != NULL)
      *fieldType = field->type;
   return (field->type == NXCP_DT_INT16) ?
           ((void *)((BYTE *)field + 6)) : 
           ((void *)((BYTE *)field + 8));
}

/**
 * Get 16 bit field as boolean
 */
bool NXCPMessage::getFieldAsBoolean(UINT32 fieldId) const
{
   BYTE type;
   void *value = (void *)get(fieldId, 0xFF, &type);
   if (value == NULL)
      return false;

   switch(type)
   {
      case NXCP_DT_INT16:
         return *((UINT16 *)value) ? true : false;
      case NXCP_DT_INT32:
         return *((UINT32 *)value) ? true : false;
      case NXCP_DT_INT64:
         return *((UINT64 *)value) ? true : false;
      default:
         return false;
   }
}

/**
 * Get data type of message field.
 *
 * @return field type or -1 if field with given ID does not exist
 */
int NXCPMessage::getFieldType(UINT32 fieldId) const
{
   NXCP_MESSAGE_FIELD *field = find(fieldId);
   return (field != NULL) ? (int)field->type : -1;
}

/**
 * get signed integer field
 */
INT32 NXCPMessage::getFieldAsInt32(UINT32 fieldId) const
{
   char *value = (char *)get(fieldId, NXCP_DT_INT32);
   return (value != NULL) ? *((INT32 *)value) : 0;
}

/**
 * get unsigned integer field
 */
UINT32 NXCPMessage::getFieldAsUInt32(UINT32 fieldId) const
{
   void *value = get(fieldId, NXCP_DT_INT32);
   return (value != NULL) ? *((UINT32 *)value) : 0;
}

/**
 * get signed 16-bit integer field
 */
INT16 NXCPMessage::getFieldAsInt16(UINT32 fieldId) const
{
   void *value = get(fieldId, NXCP_DT_INT16);
   return (value != NULL) ? *((INT16 *)value) : 0;
}

/**
 * get unsigned 16-bit integer variable
 */
UINT16 NXCPMessage::getFieldAsUInt16(UINT32 fieldId) const
{
   void *value = get(fieldId, NXCP_DT_INT16);
   return value ? *((WORD *)value) : 0;
}

/**
 * get signed 64-bit integer field
 */
INT64 NXCPMessage::getFieldAsInt64(UINT32 fieldId) const
{
   void *value = get(fieldId, NXCP_DT_INT64);
   return (value != NULL) ? *((INT64 *)value) : 0;
}

/**
 * get unsigned 64-bit integer field
 */
UINT64 NXCPMessage::getFieldAsUInt64(UINT32 fieldId) const
{
   void *value = get(fieldId, NXCP_DT_INT64);
   return value ? *((UINT64 *)value) : 0;
}

/**
 * get 64-bit floating point variable
 */
double NXCPMessage::getFieldAsDouble(UINT32 fieldId) const
{
   void *value = get(fieldId, NXCP_DT_FLOAT);
   return (value != NULL) ? *((double *)value) : 0;
}

/**
 * get time_t field
 */
time_t NXCPMessage::getFieldAsTime(UINT32 fieldId) const
{
   BYTE type;
   void *value = (void *)get(fieldId, 0xFF, &type);
   if (value == NULL)
      return 0;

   switch(type)
   {
      case NXCP_DT_INT32:
         return (time_t)(*((UINT32 *)value));
      case NXCP_DT_INT64:
         return (time_t)(*((UINT64 *)value));
      default:
         return false;
   }
}

/**
 * Get field as inet address
 */
InetAddress NXCPMessage::getFieldAsInetAddress(UINT32 fieldId) const
{
   NXCP_MESSAGE_FIELD *f = find(fieldId);
   if (f == NULL)
      return InetAddress();

   if (f->type == NXCP_DT_INETADDR)
   {
      InetAddress a = 
         (f->df_inetaddr.family == NXCP_AF_INET) ?
            InetAddress(f->df_inetaddr.addr.v4) :
            ((f->df_inetaddr.family == NXCP_AF_INET6) ? InetAddress(f->df_inetaddr.addr.v6) : InetAddress());
      a.setMaskBits(f->df_inetaddr.maskBits);
      return a;
   }
   else if (f->type == NXCP_DT_INT32)
   {
      return InetAddress(f->df_uint32);
   }
   return InetAddress();
}

/*
 * Get field as MAC address
 */
MacAddress NXCPMessage::getFieldAsMacAddress(UINT32 fieldId) const
{
   NXCP_MESSAGE_FIELD *f = find(fieldId);

   if (f == NULL || !((f->type == NXCP_DT_BINARY) && (f->df_binary.length <= 8)))
      return MacAddress();

   return MacAddress(f->df_binary.value, f->df_binary.length);
}

/**
 * Get string field
 * If buffer is NULL, memory block of required size will be allocated
 * for result; if buffer is not NULL, entire result or part of it will
 * be placed to buffer and pointer to buffer will be returned.
 * Note: bufferSize is buffer size in characters, not bytes!
 */
TCHAR *NXCPMessage::getFieldAsString(UINT32 fieldId, MemoryPool *pool, TCHAR *buffer, size_t bufferSize) const
{
   if ((buffer != NULL) && (bufferSize == 0))
      return NULL;   // non-sense combination

   if (buffer != NULL)
      *buffer = 0;

   BYTE type;
   void *value = get(fieldId, 0xFF, &type);
   if (value == NULL)
      return NULL;

   TCHAR *str = NULL;
   if (type == NXCP_DT_STRING)
   {
      if (buffer == NULL)
      {
#if defined(UNICODE) && defined(UNICODE_UCS4)
         size_t bytes = *((UINT32 *)value) * 2 + 4;
#elif defined(UNICODE) && defined(UNICODE_UCS2)
         size_t bytes = *((UINT32 *)value) + 2;
#else
         size_t bytes = *((UINT32 *)value) / 2 + 1;
#endif
         str = static_cast<TCHAR*>((pool != NULL) ? pool->allocate(bytes) : MemAlloc(bytes));
      }
      else
      {
         str = buffer;
      }

      size_t length = (buffer == NULL) ? 
            static_cast<size_t>(*static_cast<UINT32*>(value) / 2) :
            std::min(static_cast<size_t>(*static_cast<UINT32*>(value) / 2), bufferSize - 1);
#if defined(UNICODE) && defined(UNICODE_UCS4)
		ucs2_to_ucs4((UCS2CHAR *)((BYTE *)value + 4), length, str, length + 1);
#elif defined(UNICODE) && defined(UNICODE_UCS2)
      memcpy(str, (BYTE *)value + 4, length * 2);
#else
		ucs2_to_mb((UCS2CHAR *)((BYTE *)value + 4), length, str, length + 1);
#endif
      str[length] = 0;
   }
   else if (type == NXCP_DT_UTF8_STRING)
   {
      size_t length = static_cast<size_t>(*static_cast<UINT32*>(value));
#ifdef UNICODE
      if (buffer != NULL)
      {
         size_t outlen = utf8_to_wchar(static_cast<char*>(value) + 4, length, buffer, bufferSize - 1);
         buffer[outlen] = 0;
      }
      else
      {
         size_t outlen = utf8_wcharlen(static_cast<char*>(value) + 4, length);
         str = MemAllocStringW(outlen + 1);
         outlen = utf8_to_wchar(static_cast<char*>(value) + 4, length, str, outlen);
         str[outlen] = 0;
      }
#else
      if (buffer != NULL)
      {
         size_t outlen = utf8_to_mb(static_cast<char*>(value) + 4, length, buffer, bufferSize - 1);
         buffer[outlen] = 0;
      }
      else
      {
         str = MemAllocStringA(length + 1);
         size_t outlen = utf8_to_mb(static_cast<char*>(value) + 4, length, str, length);
         str[outlen] = 0;
      }
#endif
   }
   return (str != NULL) ? str : buffer;
}

#ifdef UNICODE

/**
 * get variable as multibyte string
 */
char *NXCPMessage::getFieldAsMBString(UINT32 fieldId, char *buffer, size_t bufferSize) const
{
   if ((buffer != NULL) && (bufferSize == 0))
      return NULL;   // non-sense combination

   char *str = NULL;
   BYTE ftype;
   void *value = get(fieldId, 0xFF, &ftype);
   if (value != NULL)
   {
      if (ftype == NXCP_DT_STRING)
      {
         if (buffer == NULL)
         {
            str = static_cast<char*>(MemAlloc(*static_cast<UINT32*>(value) / 2 + 1));
         }
         else
         {
            str = buffer;
         }

         size_t length = (buffer == NULL) ?
               static_cast<size_t>(*static_cast<UINT32*>(value) / 2) :
               std::min(static_cast<size_t>(*static_cast<UINT32*>(value) / 2), bufferSize - 1);
         ucs2_to_mb((UCS2CHAR *)((BYTE *)value + 4), (int)length, str, (int)length + 1);
         str[length] = 0;
      }
      else if (ftype == NXCP_DT_UTF8_STRING)
      {
         if (buffer == NULL)
         {
            str = static_cast<char*>(MemAlloc(*static_cast<UINT32*>(value) + 1));
         }
         else
         {
            str = buffer;
         }

         size_t length = (buffer == NULL) ?
               static_cast<size_t>(*static_cast<UINT32*>(value)) :
               std::min(static_cast<size_t>(*static_cast<UINT32*>(value)), bufferSize - 1);
         length = utf8_to_mb(static_cast<char*>(value) + 4, (int)length, str, (int)length + 1);
         str[length] = 0;
      }
      else if (buffer != NULL)
      {
         str = buffer;
         str[0] = 0;
      }
   }
   else if (buffer != NULL)
   {
      str = buffer;
      str[0] = 0;
   }
   return str;
}

#else

/**
 * get field as multibyte string
 */
char *NXCPMessage::getFieldAsMBString(UINT32 fieldId, char *buffer, size_t bufferSize) const
{
	return getFieldAsString(fieldId, buffer, bufferSize);
}

#endif

/**
 * get field as UTF-8 string
 */
char *NXCPMessage::getFieldAsUtf8String(UINT32 fieldId, char *buffer, size_t bufferSize) const
{
   if ((buffer != NULL) && (bufferSize == 0))
      return NULL;   // non-sense combination

   char *str = NULL;
   BYTE type;
   void *value = get(fieldId, 0xFF, &type);
   if (value != NULL)
   {
      if (type == NXCP_DT_STRING)
      {
         UCS2CHAR *in = reinterpret_cast<UCS2CHAR*>(static_cast<BYTE*>(value) + 4);
         int inSize = *static_cast<UINT32*>(value) / 2;

         size_t outSize;
         if (buffer == NULL)
         {
            outSize = ucs2_utf8len(in, inSize);
            str = MemAllocArray<char>(outSize);
         }
         else
         {
            outSize = bufferSize;
            str = buffer;
         }

         size_t cc = ucs2_to_utf8(in, inSize, str, outSize - 1);
         str[cc] = 0;
      }
      else if (type == NXCP_DT_UTF8_STRING)
      {
         size_t srcLen = *static_cast<UINT32*>(value);
         if (buffer == NULL)
         {
            str = MemAllocArrayNoInit<char>(srcLen + 1);
            memcpy(str, static_cast<BYTE*>(value) + 4, srcLen);
            str[srcLen] = 0;
         }
         else
         {
            strlcpy(buffer, static_cast<char*>(value) + 4, std::min(srcLen + 1, bufferSize));
            str = buffer;
         }
      }
      else if (buffer != NULL)
      {
         str = buffer;
         str[0] = 0;
      }
   }
   else if (buffer != NULL)
   {
      str = buffer;
      str[0] = 0;
   }
   return str;
}

/**
 * get binary (byte array) field
 * Result will be placed to the buffer provided (no more than bufferSize bytes,
 * and actual size of data will be returned
 * If pBuffer is NULL, just actual data length is returned
 */
size_t NXCPMessage::getFieldAsBinary(UINT32 fieldId, BYTE *pBuffer, size_t bufferSize) const
{
   size_t size;
   void *value = get(fieldId, NXCP_DT_BINARY);
   if (value != NULL)
   {
      size = *((UINT32 *)value);
      if (pBuffer != NULL)
         memcpy(pBuffer, (BYTE *)value + 4, std::min(bufferSize, size));
   }
   else
   {
      size = 0;
   }
   return size;
}

/**
 * get binary (byte array) field
 * Returns pointer to internal buffer or NULL if field not found
 * Data length set in size parameter.
 */
const BYTE *NXCPMessage::getBinaryFieldPtr(UINT32 fieldId, size_t *size) const
{
   BYTE *data;
   void *value = get(fieldId, NXCP_DT_BINARY);
   if (value != NULL)
   {
      *size = (size_t)(*((UINT32 *)value));
      data = (BYTE *)value + 4;
   }
   else
   {
      *size = 0;
      data = NULL;
   }
   return data;
}

/**
 * Get field as GUID
 * Returns NULL GUID on error
 */
uuid NXCPMessage::getFieldAsGUID(UINT32 fieldId) const
{
   NXCP_MESSAGE_FIELD *f = find(fieldId);
   if (f == NULL)
      return uuid::NULL_UUID;

   if ((f->type == NXCP_DT_BINARY) && (f->df_binary.length == UUID_LENGTH))
   {
      return uuid(f->df_binary.value);
   }
   else if ((f->type == NXCP_DT_STRING) || (f->type == NXCP_DT_UTF8_STRING))
   {
      TCHAR buffer[64] = _T("");
      getFieldAsString(fieldId, buffer, 64);
      return uuid::parse(buffer);
   }
   return uuid::NULL_UUID;
}

/**
 * Build protocol message ready to be send over the wire
 */
NXCP_MESSAGE *NXCPMessage::serialize(bool allowCompression) const
{
   // Calculate message size
   size_t size = NXCP_HEADER_SIZE;
   UINT32 fieldCount = 0;
   if (m_flags & MF_BINARY)
   {
      size += m_dataSize;
      fieldCount = (UINT32)m_dataSize;
      size += (8 - (size % 8)) & 7;
   }
   else
   {
      MessageField *entry, *tmp;
      HASH_ITER(hh, m_fields, entry, tmp)
      {
         size_t fieldSize = CalculateFieldSize(&entry->data, false);
         if (m_version >= 2)
            size += fieldSize + ((8 - (fieldSize % 8)) & 7);
         else
            size += fieldSize;
         fieldCount++;
      }

      // Message should be aligned to 8 bytes boundary
      // This is always the case starting from version 2 because
      // all fields are padded to 8 bytes boundary
      if (m_version < 2)
         size += (8 - (size % 8)) & 7;
   }

   // Create message
   NXCP_MESSAGE *msg = static_cast<NXCP_MESSAGE*>(MemAlloc(size));
   memset(msg, 0, size);
   msg->code = htons(m_code);
   msg->flags = htons(m_flags | MF_NXCP_VERSION(m_version));
   msg->size = htonl(static_cast<UINT32>(size));
   msg->id = htonl(m_id);
   msg->numFields = htonl(fieldCount);

   // Fill data fields
   if (m_flags & MF_BINARY)
   {
      memcpy(msg->fields, m_data, m_dataSize);
   }
   else
   {
      NXCP_MESSAGE_FIELD *field = (NXCP_MESSAGE_FIELD *)((char *)msg + NXCP_HEADER_SIZE);
      MessageField *entry, *tmp;
      HASH_ITER(hh, m_fields, entry, tmp)
      {
         size_t fieldSize = CalculateFieldSize(&entry->data, false);
         memcpy(field, &entry->data, fieldSize);

         // Convert numeric values to network format
         field->fieldId = htonl(field->fieldId);
         switch(field->type)
         {
            case NXCP_DT_INT32:
               field->df_int32 = htonl(field->df_int32);
               break;
            case NXCP_DT_INT64:
               field->df_int64 = htonq(field->df_int64);
               break;
            case NXCP_DT_INT16:
               field->df_int16 = htons(field->df_int16);
               break;
            case NXCP_DT_FLOAT:
               field->df_real = htond(field->df_real);
               break;
            case NXCP_DT_STRING:
#if !(WORDS_BIGENDIAN)
               {
                  bswap_array_16(field->df_string.value, field->df_string.length / 2);
                  field->df_string.length = htonl(field->df_string.length);
               }
#endif
               break;
            case NXCP_DT_BINARY:
            case NXCP_DT_UTF8_STRING:
               field->df_string.length = htonl(field->df_string.length);
               break;
            case NXCP_DT_INETADDR:
               if (field->df_inetaddr.family == NXCP_AF_INET)
               {
                  field->df_inetaddr.addr.v4 = htonl(field->df_inetaddr.addr.v4);
               }
               break;
         }

         if (m_version >= 2)
            field = (NXCP_MESSAGE_FIELD *)((char *)field + fieldSize + ((8 - (fieldSize % 8)) & 7));
         else
            field = (NXCP_MESSAGE_FIELD *)((char *)field + fieldSize);
      }
   }

   // Compress message payload if requested. Compression supported starting with NXCP version 4.
   if ((m_version >= 4) && allowCompression && (size > 128) && !(m_flags & MF_STREAM))
   {
      z_stream stream;
      stream.zalloc = Z_NULL;
      stream.zfree = Z_NULL;
      stream.opaque = Z_NULL;
      stream.avail_in = 0;
      stream.next_in = Z_NULL;
      if (deflateInit(&stream, 9) == Z_OK)
      {
         size_t compBufferSize = deflateBound(&stream, (unsigned long)(size - NXCP_HEADER_SIZE));
         BYTE *compressedMsg = (BYTE *)MemAlloc(compBufferSize + NXCP_HEADER_SIZE + 4);
         stream.next_in = (BYTE *)msg->fields;
         stream.avail_in = (UINT32)(size - NXCP_HEADER_SIZE);
         stream.next_out = compressedMsg + NXCP_HEADER_SIZE + 4;
         stream.avail_out = (UINT32)compBufferSize;
         if (deflate(&stream, Z_FINISH) == Z_STREAM_END)
         {
            size_t compMsgSize = compBufferSize - stream.avail_out + NXCP_HEADER_SIZE + 4;
            // Message should be aligned to 8 bytes boundary
            compMsgSize += (8 - (compMsgSize % 8)) & 7;
            if (compMsgSize < size - 4)
            {
               memcpy(compressedMsg, msg, NXCP_HEADER_SIZE);
               MemFree(msg);
               msg = (NXCP_MESSAGE *)compressedMsg;
               msg->flags |= htons(MF_COMPRESSED);
               memcpy((BYTE *)msg + NXCP_HEADER_SIZE, &msg->size, 4); // Save size of uncompressed message
               msg->size = htonl((UINT32)compMsgSize);
            }
            else
            {
               MemFree(compressedMsg);
            }
         }
         else
         {
            MemFree(compressedMsg);
         }
         deflateEnd(&stream);
      }
   }
   return msg;
}

/**
 * Delete all variables
 */
void NXCPMessage::deleteAllFields()
{
   m_fields = NULL;
   m_data = NULL;
   m_dataSize = 0;
   m_pool.clear();
}

#ifdef UNICODE

/**
 * Set field from multibyte string
 */
void NXCPMessage::setFieldFromMBString(UINT32 fieldId, const char *value)
{
   size_t l = strlen(value) + 1;
#if HAVE_ALLOCA
   WCHAR *wcValue = reinterpret_cast<WCHAR*>(alloca(l * sizeof(WCHAR)));
#else
   WCHAR localBuffer[256];
   WCHAR *wcValue = (l <= 256) ? localBuffer : static_cast<WCHAR*>(MemAlloc(l * sizeof(WCHAR)));
#endif
   MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, value, -1, wcValue, static_cast<int>(l));
   set(fieldId, (m_version >= 5) ? NXCP_DT_UTF8_STRING : NXCP_DT_STRING, wcValue);
#if !HAVE_ALLOCA
   if (wcValue != localBuffer)
      MemFree(wcValue);
#endif
}

#endif

/**
 * set binary field to an array of UINT32s
 */
void NXCPMessage::setFieldFromInt32Array(UINT32 fieldId, size_t numElements, const UINT32 *elements)
{
   UINT32 *pdwBuffer = (UINT32 *)set(fieldId, NXCP_DT_BINARY, elements, false, numElements * sizeof(UINT32));
   if (pdwBuffer != NULL)
   {
      pdwBuffer++;   // First UINT32 is a length field
      for(size_t i = 0; i < numElements; i++)  // Convert UINT32s to network byte order
         pdwBuffer[i] = htonl(pdwBuffer[i]);
   }
}

/**
 * set binary field to an array of UINT32s
 */
void NXCPMessage::setFieldFromInt32Array(UINT32 fieldId, const IntegerArray<UINT32> *data)
{
   if (data != NULL)
   {
      UINT32 *pdwBuffer = (UINT32 *)set(fieldId, NXCP_DT_BINARY, data->getBuffer(), false, data->size() * sizeof(UINT32));
      if (pdwBuffer != NULL)
      {
         pdwBuffer++;   // First UINT32 is a length field
         for (int i = 0; i < data->size(); i++)  // Convert UINT32s to network byte order
            pdwBuffer[i] = htonl(pdwBuffer[i]);
      }
   }
   else
   {
      // Encode empty array
      set(fieldId, NXCP_DT_BINARY, NULL, false, 0);
   }
}

/**
 * get binary field as an array of 32 bit unsigned integers
 */
size_t NXCPMessage::getFieldAsInt32Array(UINT32 fieldId, UINT32 numElements, UINT32 *buffer) const
{
   size_t size = getFieldAsBinary(fieldId, (BYTE *)buffer, numElements * sizeof(UINT32));
   size /= sizeof(UINT32);   // Convert bytes to elements
   for(size_t i = 0; i < size; i++)
      buffer[i] = ntohl(buffer[i]);
   return size;
}

/**
 * get binary field as an array of 32 bit unsigned integers
 */
size_t NXCPMessage::getFieldAsInt32Array(UINT32 fieldId, IntegerArray<UINT32> *data) const
{
   data->clear();

   UINT32 *value = (UINT32 *)get(fieldId, NXCP_DT_BINARY);
   if (value != NULL)
   {
      size_t size = *value / sizeof(UINT32);
      value++;
      for(size_t i = 0; i < size; i++)
      {
         data->add(ntohl(*value));
         value++;
      }
   }
   return data->size();
}

/**
 * set binary field from file
 */
bool NXCPMessage::setFieldFromFile(UINT32 fieldId, const TCHAR *pszFileName)
{
   FILE *pFile;
   BYTE *pBuffer;
   UINT32 size;
   bool bResult = false;

   size = (UINT32)FileSize(pszFileName);
   pFile = _tfopen(pszFileName, _T("rb"));
   if (pFile != NULL)
   {
      pBuffer = (BYTE *)set(fieldId, NXCP_DT_BINARY, NULL, false, size);
      if (pBuffer != NULL)
      {
         if (fread(pBuffer + sizeof(UINT32), 1, size, pFile) == size)
            bResult = true;
      }
      fclose(pFile);
   }
   return bResult;
}

/**
 * Get string from field
 */
static TCHAR *GetStringFromField(void *df)
{
#if defined(UNICODE) && defined(UNICODE_UCS4)
   TCHAR *str = (TCHAR *)MemAlloc(*((UINT32 *)df) * 2 + 4);
#elif defined(UNICODE) && defined(UNICODE_UCS2)
   TCHAR *str = (TCHAR *)MemAlloc(*((UINT32 *)df) + 2);
#else
   TCHAR *str = (TCHAR *)MemAlloc(*((UINT32 *)df) / 2 + 1);
#endif
   int len = (int)(*((UINT32 *)df) / 2);
#if defined(UNICODE) && defined(UNICODE_UCS4)
	ucs2_to_ucs4((UCS2CHAR *)((BYTE *)df + 4), len, str, len + 1);
#elif defined(UNICODE) && defined(UNICODE_UCS2)
   memcpy(str, (BYTE *)df + 4, len * 2);
#else
	ucs2_to_mb((UCS2CHAR *)((BYTE *)df + 4), len, str, len + 1);
#endif
   str[len] = 0;
   return str;
}

/**
 * Get string from UTF8 encoded field
 */
static TCHAR *GetStringFromFieldUTF8(void *df)
{
   size_t utf8len = *static_cast<UINT32*>(df);
   const char *utf8str = static_cast<char*>(df) + 4;
#if defined(UNICODE) && defined(UNICODE_UCS4)
   size_t dlen = utf8_ucs4len(utf8str, utf8len) + 1;
#elif defined(UNICODE) && defined(UNICODE_UCS2)
   size_t dlen = utf8_ucs2len(utf8str, utf8len) + 1;
#else
   size_t dlen = utf8len + 1;
#endif
   TCHAR *str = MemAllocString(dlen);
#ifdef UNICODE
   dlen = utf8_to_wchar(utf8str, utf8len, str, dlen);
#else
   dlen = utf8_to_mb(utf8str, utf8len, str, dlen);
#endif
   str[dlen] = 0;
   return str;
}

/**
 * Dump NXCP message
 */
String NXCPMessage::dump(const NXCP_MESSAGE *msg, int version)
{
   String out;
   int i;
   TCHAR *str, buffer[128];

   WORD flags = ntohs(msg->flags);
   WORD code = ntohs(msg->code);
   UINT32 id = ntohl(msg->id);
   UINT32 size = ntohl(msg->size);
   int numFields = (int)ntohl(msg->numFields);

   // Dump raw message
   TCHAR textForm[32];
   for(i = 0; i < (int)size; i += 16)
   {
      const BYTE *block = reinterpret_cast<const BYTE*>(msg) + i;
      size_t blockSize = MIN(16, size - i);
      BinToStr(block, blockSize, buffer);
      size_t j;
      for(j = 0; j < blockSize; j++)
      {
         BYTE b = block[j];
         textForm[j] = ((b >= ' ') && (b < 127)) ? (TCHAR)b : _T('.');
      }
      textForm[j] = 0;
      out.appendFormattedString(_T("  ** %06X: %s %s\n"), i, buffer, textForm);
   }

   // header
   out.appendFormattedString(_T("  ** code=0x%04X (%s) version=%d flags=0x%04X id=%d size=%d numFields=%d\n"),
      code, NXCPMessageCodeName(code, buffer), flags >> 12, flags & 0x0FFF, id, size, numFields);
   if (flags & MF_BINARY)
   {
      out += _T("  ** binary message\n");
      return out;
   }
   if (flags & MF_CONTROL)
   {
      out += _T("  ** control message\n");
      return out;
   }

   size_t msgDataSize;
   const BYTE *msgData;
   BYTE *allocatedMsgData;
   if ((flags & MF_COMPRESSED) && (version >= 4))
   {
      msgDataSize = (size_t)ntohl(*((UINT32 *)((BYTE *)msg + NXCP_HEADER_SIZE))) - NXCP_HEADER_SIZE;

      z_stream stream;
      stream.zalloc = Z_NULL;
      stream.zfree = Z_NULL;
      stream.opaque = Z_NULL;
      stream.avail_in = size - NXCP_HEADER_SIZE - 4;
      stream.next_in = (BYTE *)msg + NXCP_HEADER_SIZE + 4;
      if (inflateInit(&stream) != Z_OK)
      {
         out.append(_T("Cannot decompress message"));
         return out;
      }

      msgData = allocatedMsgData = static_cast<BYTE*>(MemAlloc(msgDataSize));
      stream.next_out = allocatedMsgData;
      stream.avail_out = (UINT32)msgDataSize;

      if (inflate(&stream, Z_FINISH) != Z_STREAM_END)
      {
         inflateEnd(&stream);
         MemFree(allocatedMsgData);
         out.append(_T("Cannot decompress message"));
         return out;
      }
      inflateEnd(&stream);
   }
   else
   {
      msgDataSize = size - NXCP_HEADER_SIZE;
      msgData = reinterpret_cast<const BYTE*>(msg) + NXCP_HEADER_SIZE;
      allocatedMsgData = NULL;
   }
   
   // Parse data fields
   size_t pos = 0;
   for(int f = 0; f < numFields; f++)
   {
      const NXCP_MESSAGE_FIELD *field = reinterpret_cast<const NXCP_MESSAGE_FIELD*>(msgData + pos);

      // Validate position inside message
      if (pos > msgDataSize - 8)
      {
         out += _T("  ** message format error (pos > msgDataSize - 8)\n");
         break;
      }
      if ((pos > msgDataSize - 12) &&
          ((field->type == NXCP_DT_STRING) || (field->type == NXCP_DT_UTF8_STRING) || (field->type == NXCP_DT_BINARY)))
      {
         out.appendFormattedString(_T("  ** message format error (pos > msgDataSize - 12 and field type %d)\n"), (int)field->type);
         break;
      }

      // Calculate and validate field size
      size_t fieldSize = CalculateFieldSize(field, true);
      if (pos + fieldSize > msgDataSize)
      {
         out.appendFormattedString(_T("  ** message format error (invalid field size %d at offset 0x%06X)\n"), (int)fieldSize, (int)pos);
         break;
      }

      // Create new entry
      NXCP_MESSAGE_FIELD *convertedField = static_cast<NXCP_MESSAGE_FIELD*>(MemCopyBlock(field, fieldSize));

      // Convert numeric values to host format
      convertedField->fieldId = ntohl(convertedField->fieldId);
      switch(field->type)
      {
         case NXCP_DT_INT32:
            convertedField->df_int32 = ntohl(convertedField->df_int32);
            out.appendFormattedString(_T("  ** %06X: [%6d] INT32       %d\n"), (int)pos, (int)convertedField->fieldId, convertedField->df_int32);
            break;
         case NXCP_DT_INT64:
            convertedField->df_int64 = ntohq(convertedField->df_int64);
            out.appendFormattedString(_T("  ** %06X: [%6d] INT64       ") INT64_FMT _T("\n"), (int)pos, (int)convertedField->fieldId, convertedField->df_int64);
            break;
         case NXCP_DT_INT16:
            convertedField->df_int16 = ntohs(convertedField->df_int16);
            out.appendFormattedString(_T("  ** %06X: [%6d] INT16       %d\n"), (int)pos, (int)convertedField->fieldId, (int)convertedField->df_int16);
            break;
         case NXCP_DT_FLOAT:
            convertedField->df_real = ntohd(convertedField->df_real);
            out.appendFormattedString(_T("  ** %06X: [%6d] FLOAT       %f\n"), (int)pos, (int)convertedField->fieldId, convertedField->df_real);
            break;
         case NXCP_DT_STRING:
#if !(WORDS_BIGENDIAN)
            convertedField->df_string.length = ntohl(convertedField->df_string.length);
            bswap_array_16(convertedField->df_string.value, (int)convertedField->df_string.length / 2);
#endif
            str = GetStringFromField((BYTE *)convertedField + 8);
            out.appendFormattedString(_T("  ** %06X: [%6d] STRING      \"%s\"\n"), (int)pos, (int)convertedField->fieldId, str);
            MemFree(str);
            break;
         case NXCP_DT_UTF8_STRING:
#if !(WORDS_BIGENDIAN)
            convertedField->df_utf8string.length = ntohl(convertedField->df_utf8string.length);
#endif
            str = GetStringFromFieldUTF8((BYTE *)convertedField + 8);
            out.appendFormattedString(_T("  ** %06X: [%6d] UTF8-STRING \"%s\"\n"), (int)pos, (int)convertedField->fieldId, str);
            MemFree(str);
            break;
         case NXCP_DT_BINARY:
#if !(WORDS_BIGENDIAN)
            convertedField->df_binary.length = ntohl(convertedField->df_binary.length);
#endif
            out.appendFormattedString(_T("  ** %06X: [%6d] BINARY      len=%d\n"), (int)pos,
                     (int)convertedField->fieldId, (int)convertedField->df_binary.length);
            break;
         case NXCP_DT_INETADDR:
            {
               InetAddress a = 
                  (convertedField->df_inetaddr.family == NXCP_AF_INET) ?
                     InetAddress(ntohl(convertedField->df_inetaddr.addr.v4)) :
                     InetAddress(convertedField->df_inetaddr.addr.v6);
               a.setMaskBits(convertedField->df_inetaddr.maskBits);
               out.appendFormattedString(_T("  ** %06X: [%6d] INETADDR    %s\n"), (int)pos, (int)convertedField->fieldId, (const TCHAR *)a.toString());
            }
            break;
         default:
            out.appendFormattedString(_T("  ** %06X: [%6d] unknown type %d\n"), (int)pos, (int)convertedField->fieldId, (int)field->type);
            break;
      }
      MemFree(convertedField);

      // Starting from version 2, all fields should be 8-byte aligned
      if (version >= 2)
         pos += fieldSize + ((8 - (fieldSize % 8)) & 7);
      else
         pos += fieldSize;
   }

   MemFree(allocatedMsgData);
   return out;
}

/**
 * Set protocol version
 */
void NXCPMessage::setProtocolVersion(int version)
{
   if ((m_version >= 5) && (version < 5))
   {
      // Convert all UTF8-STRING fields to STRING
      IntegerArray<UINT32> stringFields(256, 256);
      MessageField *entry, *tmp;
      HASH_ITER(hh, m_fields, entry, tmp)
      {
         if (entry->data.type == NXCP_DT_UTF8_STRING)
            stringFields.add(entry->id);
      }

      char localBuffer[4096];
      for(int i = 0; i < stringFields.size(); i++)
      {
         UINT32 fieldId = stringFields.get(i);
         void *value = get(fieldId, NXCP_DT_UTF8_STRING);
         size_t len = *static_cast<UINT32*>(value);
         char *buffer = (len < 4096) ? localBuffer : static_cast<char*>(m_pool.allocate(len + 1));
         memcpy(buffer, static_cast<char*>(value) + 4, len);
         buffer[len] = 0;
         set(fieldId, NXCP_DT_STRING, buffer, false, 0, true);
      }
   }

   m_version = version;

   // Update encoded version in flags
   m_flags &= 0x0FFF;
   m_flags |= MF_NXCP_VERSION(m_version);
}
