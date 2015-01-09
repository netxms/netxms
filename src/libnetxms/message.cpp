/* 
** NetXMS - Network Management System
** NetXMS Foundation Library
** Copyright (C) 2003-2014 Victor Kirhenshtein
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
#include <uthash.h>

/**
 * Calculate field size
 */
static size_t CalculateFieldSize(NXCP_MESSAGE_FIELD *field, bool networkByteOrder)
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
 * Create new hash entry wth given field size
 */
inline MessageField *CreateMessageField(size_t fieldSize)
{
   size_t entrySize = sizeof(MessageField) - sizeof(NXCP_MESSAGE_FIELD) + fieldSize;
   MessageField *entry = (MessageField *)calloc(1, entrySize);
   entry->size = entrySize;
   return entry;
}

/**
 * Default constructor for NXCPMessage class
 */
NXCPMessage::NXCPMessage(int version)
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
 * Create a copy of prepared CSCP message
 */
NXCPMessage::NXCPMessage(NXCPMessage *msg)
{
   m_code = msg->m_code;
   m_id = msg->m_id;
   m_flags = msg->m_flags;
   m_version = msg->m_version;
   m_fields = NULL;

   if (m_flags & MF_BINARY)
   {
      m_dataSize = msg->m_dataSize;
      m_data = (BYTE *)nx_memdup(msg->m_data, m_dataSize);
   }
   else
   {
      m_data = NULL;
      m_dataSize = 0;

      MessageField *entry, *tmp;
      HASH_ITER(hh, msg->m_fields, entry, tmp)
      {
         MessageField *f = (MessageField *)nx_memdup(entry, entry->size);
         HASH_ADD_INT(m_fields, id, f);
      }
   }
}

/**
 * Create NXCPMessage object from received message
 */
NXCPMessage::NXCPMessage(NXCP_MESSAGE *msg, int version)
{
   UINT32 i;

   m_flags = ntohs(msg->flags);
   m_code = ntohs(msg->code);
   m_id = ntohl(msg->id);
   m_version = version;
   m_fields = NULL;

   // Parse data fields
   if (m_flags & MF_BINARY)
   {
      m_dataSize = (size_t)ntohl(msg->numFields);
      m_data = (BYTE *)nx_memdup(msg->fields, m_dataSize);
   }
   else
   {
      m_data = NULL;
      m_dataSize = 0;

      int fieldCount = (int)ntohl(msg->numFields);
      size_t size = (size_t)ntohl(msg->size);
      size_t pos = NXCP_HEADER_SIZE;
      for(int f = 0; f < fieldCount; f++)
      {
         NXCP_MESSAGE_FIELD *field = (NXCP_MESSAGE_FIELD *)(((BYTE *)msg) + pos);

         // Validate position inside message
         if (pos > size - 8)
            break;
         if ((pos > size - 12) && 
             ((field->type == NXCP_DT_STRING) || (field->type == NXCP_DT_BINARY)))
            break;

         // Calculate and validate variable size
         size_t fieldSize = CalculateFieldSize(field, true);
         if (pos + fieldSize > size)
            break;

         // Create new entry
         MessageField *entry = CreateMessageField(fieldSize);
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
               for(i = 0; i < entry->data.df_string.length / 2; i++)
                  entry->data.df_string.value[i] = ntohs(entry->data.df_string.value[i]);
#endif
               break;
            case NXCP_DT_BINARY:
               entry->data.df_string.length = ntohl(entry->data.df_string.length);
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
   deleteAllFields();
   safe_free(m_data);
}

/**
 * Find field by ID
 */
NXCP_MESSAGE_FIELD *NXCPMessage::find(UINT32 fieldId)
{
   MessageField *entry;
   HASH_FIND_INT(m_fields, &fieldId, entry);
   return (entry != NULL) ? &entry->data : NULL;
}

/**
 * set variable
 * Argument size (data size) contains data length in bytes for DT_BINARY type
 * and maximum number of characters for DT_STRING type (0 means no limit)
 */
void *NXCPMessage::set(UINT32 fieldId, BYTE type, const void *value, bool isSigned, size_t size)
{
   if (m_flags & MF_BINARY)
      return NULL;

   size_t length;
#if defined(UNICODE_UCS2) && defined(UNICODE)
#define __buffer value
#else
   UCS2CHAR *__buffer;
#endif

   // Create entry
   MessageField *entry;
   switch(type)
   {
      case NXCP_DT_INT32:
         entry = CreateMessageField(12);
         entry->data.df_int32 = *((const UINT32 *)value);
         break;
      case NXCP_DT_INT16:
         entry = CreateMessageField(8);
         entry->data.df_int16 = *((const WORD *)value);
         break;
      case NXCP_DT_INT64:
         entry = CreateMessageField(16);
         entry->data.df_int64 = *((const UINT64 *)value);
         break;
      case NXCP_DT_FLOAT:
         entry = CreateMessageField(16);
         entry->data.df_real = *((const double *)value);
         break;
      case NXCP_DT_STRING:
#ifdef UNICODE         
         length = _tcslen((const TCHAR *)value);
			if ((size > 0) && (length > size))
				length = size;
#ifndef UNICODE_UCS2 /* assume UNICODE_UCS4 */
         __buffer = (UCS2CHAR *)malloc(length * 2 + 2);
         ucs4_to_ucs2((WCHAR *)value, length, __buffer, length + 1);
#endif         
#else		/* not UNICODE */
			__buffer = UCS2StringFromMBString((const char *)value);
			length = (UINT32)ucs2_strlen(__buffer);
			if ((size > 0) && (length > size))
				length = size;
#endif
         entry = CreateMessageField(12 + length * 2);
         entry->data.df_string.length = (UINT32)(length * 2);
         memcpy(entry->data.df_string.value, __buffer, entry->data.df_string.length);
#if !defined(UNICODE_UCS2) || !defined(UNICODE)
         free(__buffer);
#endif
         break;
      case NXCP_DT_BINARY:
         entry = CreateMessageField(12 + size);
         entry->data.df_string.length = (UINT32)size;
         if ((entry->data.df_string.length > 0) && (value != NULL))
            memcpy(entry->data.df_string.value, value, entry->data.df_string.length);
         break;
      case NXCP_DT_INETADDR:
         entry = CreateMessageField(32);
         entry->data.df_inetaddr.family = (((InetAddress *)value)->getFamily() == AF_INET) ? NXCP_AF_INET : NXCP_AF_INET6;
         entry->data.df_inetaddr.maskBits = (BYTE)((InetAddress *)value)->getMaskBits();
         if (((InetAddress *)value)->getFamily() == AF_INET)
         {
            entry->data.df_inetaddr.addr.v4 = ((InetAddress *)value)->getAddressV4();
         }
         else
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
      free(curr);
   }
   HASH_ADD_INT(m_fields, id, entry);

   return (type == NXCP_DT_INT16) ? ((void *)((BYTE *)&entry->data + 6)) : ((void *)((BYTE *)&entry->data + 8));
#undef __buffer
}

/**
 * Get field value
 */
void *NXCPMessage::get(UINT32 fieldId, BYTE requiredType, BYTE *fieldType)
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
bool NXCPMessage::getFieldAsBoolean(UINT32 fieldId)
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
int NXCPMessage::getFieldType(UINT32 fieldId)
{
   NXCP_MESSAGE_FIELD *field = find(fieldId);
   return (field != NULL) ? (int)field->type : -1;
}

/**
 * get signed integer field
 */
INT32 NXCPMessage::getFieldAsInt32(UINT32 fieldId)
{
   char *value = (char *)get(fieldId, NXCP_DT_INT32);
   return (value != NULL) ? *((INT32 *)value) : 0;
}

/**
 * get unsigned integer field
 */
UINT32 NXCPMessage::getFieldAsUInt32(UINT32 fieldId)
{
   void *value = get(fieldId, NXCP_DT_INT32);
   return (value != NULL) ? *((UINT32 *)value) : 0;
}

/**
 * get signed 16-bit integer field
 */
INT16 NXCPMessage::getFieldAsInt16(UINT32 fieldId)
{
   void *value = get(fieldId, NXCP_DT_INT16);
   return (value != NULL) ? *((INT16 *)value) : 0;
}

/**
 * get unsigned 16-bit integer variable
 */
UINT16 NXCPMessage::getFieldAsUInt16(UINT32 fieldId)
{
   void *value = get(fieldId, NXCP_DT_INT16);
   return value ? *((WORD *)value) : 0;
}

/**
 * get signed 64-bit integer field
 */
INT64 NXCPMessage::getFieldAsInt64(UINT32 fieldId)
{
   void *value = get(fieldId, NXCP_DT_INT64);
   return (value != NULL) ? *((INT64 *)value) : 0;
}

/**
 * get unsigned 64-bit integer field
 */
UINT64 NXCPMessage::getFieldAsUInt64(UINT32 fieldId)
{
   void *value = get(fieldId, NXCP_DT_INT64);
   return value ? *((UINT64 *)value) : 0;
}

/**
 * get 64-bit floating point variable
 */
double NXCPMessage::getFieldAsDouble(UINT32 fieldId)
{
   void *value = get(fieldId, NXCP_DT_FLOAT);
   return (value != NULL) ? *((double *)value) : 0;
}

/**
 * get time_t field
 */
time_t NXCPMessage::getFieldAsTime(UINT32 fieldId)
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
InetAddress NXCPMessage::getFieldAsInetAddress(UINT32 fieldId)
{
   NXCP_MESSAGE_FIELD *f = find(fieldId);
   if (f == NULL)
      return InetAddress();

   if (f->type == NXCP_DT_INETADDR)
   {
      InetAddress a = 
         (f->df_inetaddr.family == NXCP_AF_INET) ?
            InetAddress(f->df_inetaddr.addr.v4) :
            InetAddress(f->df_inetaddr.addr.v6);
      a.setMaskBits(f->df_inetaddr.maskBits);
      return a;
   }
   else if (f->type == NXCP_DT_INT32)
   {
      return InetAddress(f->df_uint32);
   }
   return InetAddress();
}

/**
 * Get string field
 * If buffer is NULL, memory block of required size will be allocated
 * for result; if buffer is not NULL, entire result or part of it will
 * be placed to buffer and pointer to buffer will be returned.
 * Note: bufferSize is buffer size in characters, not bytes!
 */
TCHAR *NXCPMessage::getFieldAsString(UINT32 fieldId, TCHAR *buffer, size_t bufferSize)
{
   if ((buffer != NULL) && (bufferSize == 0))
      return NULL;   // non-sense combination

   TCHAR *str = NULL;
   void *value = get(fieldId, NXCP_DT_STRING);
   if (value != NULL)
   {
      if (buffer == NULL)
      {
#if defined(UNICODE) && defined(UNICODE_UCS4)
         str = (TCHAR *)malloc(*((UINT32 *)value) * 2 + 4);
#elif defined(UNICODE) && defined(UNICODE_UCS2)
         str = (TCHAR *)malloc(*((UINT32 *)value) + 2);
#else
         str = (TCHAR *)malloc(*((UINT32 *)value) / 2 + 1);
#endif
      }
      else
      {
         str = buffer;
      }

      size_t length = (buffer == NULL) ? (*((UINT32 *)value) / 2) : min(*((UINT32 *)value) / 2, bufferSize - 1);
#if defined(UNICODE) && defined(UNICODE_UCS4)
		ucs2_to_ucs4((UCS2CHAR *)((BYTE *)value + 4), length, str, length + 1);
#elif defined(UNICODE) && defined(UNICODE_UCS2)
      memcpy(str, (BYTE *)value + 4, length * 2);
#else
		ucs2_to_mb((UCS2CHAR *)((BYTE *)value + 4), length, str, length + 1);
#endif
      str[length] = 0;
   }
   else
   {
      if (buffer != NULL)
      {
         str = buffer;
         str[0] = 0;
      }
   }
   return str;
}

#ifdef UNICODE

/**
 * get variable as multibyte string
 */
char *NXCPMessage::getFieldAsMBString(UINT32 fieldId, char *buffer, size_t bufferSize)
{
   if ((buffer != NULL) && (bufferSize == 0))
      return NULL;   // non-sense combination

   char *str = NULL;
   void *value = get(fieldId, NXCP_DT_STRING);
   if (value != NULL)
   {
      if (buffer == NULL)
      {
         str = (char *)malloc(*((UINT32 *)value) / 2 + 1);
      }
      else
      {
         str = buffer;
      }

      size_t length = (buffer == NULL) ? (*((UINT32 *)value) / 2) : min(*((UINT32 *)value) / 2, bufferSize - 1);
		ucs2_to_mb((UCS2CHAR *)((BYTE *)value + 4), (int)length, str, (int)length + 1);
      str[length] = 0;
   }
   else
   {
      if (buffer != NULL)
      {
         str = buffer;
         str[0] = 0;
      }
   }
   return str;
}

#else

/**
 * get field as multibyte string
 */
char *NXCPMessage::getFieldAsMBString(UINT32 fieldId, char *buffer, size_t bufferSize)
{
	return getFieldAsString(fieldId, buffer, bufferSize);
}

#endif

/**
 * get field as UTF-8 string
 */
char *NXCPMessage::getFieldAsUtf8String(UINT32 fieldId, char *buffer, size_t bufferSize)
{
   if ((buffer != NULL) && (bufferSize == 0))
      return NULL;   // non-sense combination

   char *str = NULL;
   void *value = get(fieldId, NXCP_DT_STRING);
   if (value != NULL)
   {
      int outSize;
      if (buffer == NULL)
      {
			// Assume worst case scenario - 3 bytes per character
			outSize = (int)(*((UINT32 *)value) + *((UINT32 *)value) / 2 + 1);
         str = (char *)malloc(outSize);
      }
      else
      {
			outSize = (int)bufferSize;
         str = buffer;
      }

      size_t length = *((UINT32 *)value) / 2;
#ifdef UNICODE_UCS2
		int cc = WideCharToMultiByte(CP_UTF8, 0, (WCHAR *)((BYTE *)value + 4), (int)length, str, outSize - 1, NULL, NULL);
#else
		int cc = ucs2_to_utf8((UCS2CHAR *)((BYTE *)value + 4), (int)length, str, outSize - 1);
#endif
      str[cc] = 0;
   }
   else
   {
      if (buffer != NULL)
      {
         str = buffer;
         str[0] = 0;
      }
   }
   return str;
}

/**
 * get binary (byte array) field
 * Result will be placed to the buffer provided (no more than bufferSize bytes,
 * and actual size of data will be returned
 * If pBuffer is NULL, just actual data length is returned
 */
UINT32 NXCPMessage::getFieldAsBinary(UINT32 fieldId, BYTE *pBuffer, size_t bufferSize)
{
   UINT32 size;
   void *value = get(fieldId, NXCP_DT_BINARY);
   if (value != NULL)
   {
      size = *((UINT32 *)value);
      if (pBuffer != NULL)
         memcpy(pBuffer, (BYTE *)value + 4, min(bufferSize, size));
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
BYTE *NXCPMessage::getBinaryFieldPtr(UINT32 fieldId, size_t *size)
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
 * Build protocol message ready to be send over the wire
 */
NXCP_MESSAGE *NXCPMessage::createMessage()
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
   NXCP_MESSAGE *msg = (NXCP_MESSAGE *)malloc(size);
   memset(msg, 0, size);
   msg->code = htons(m_code);
   msg->flags = htons(m_flags);
   msg->size = htonl((UINT32)size);
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
                  for(UINT32 i = 0; i < field->df_string.length / 2; i++)
                     field->df_string.value[i] = htons(field->df_string.value[i]);
                  field->df_string.length = htonl(field->df_string.length);
               }
#endif
               break;
            case NXCP_DT_BINARY:
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
   return msg;
}

/**
 * Delete all variables
 */
void NXCPMessage::deleteAllFields()
{
   MessageField *entry, *tmp;
   HASH_ITER(hh, m_fields, entry, tmp)
   {
      HASH_DEL(m_fields, entry);
      free(entry);
   }
}

#ifdef UNICODE

/**
 * set variable from multibyte string
 */
void NXCPMessage::setFieldFromMBString(UINT32 fieldId, const char *value)
{
	WCHAR *wcValue = WideStringFromMBString(value);
	set(fieldId, NXCP_DT_STRING, wcValue);
	free(wcValue);
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
void NXCPMessage::setFieldFromInt32Array(UINT32 fieldId, IntegerArray<UINT32> *data)
{
   UINT32 *pdwBuffer = (UINT32 *)set(fieldId, NXCP_DT_BINARY, data->getBuffer(), false, data->size() * sizeof(UINT32));
   if (pdwBuffer != NULL)
   {
      pdwBuffer++;   // First UINT32 is a length field
      for(int i = 0; i < data->size(); i++)  // Convert UINT32s to network byte order
         pdwBuffer[i] = htonl(pdwBuffer[i]);
   }
}

/**
 * get binary field as an array of 32 bit unsigned integers
 */
UINT32 NXCPMessage::getFieldAsInt32Array(UINT32 fieldId, UINT32 numElements, UINT32 *buffer)
{
   UINT32 size = getFieldAsBinary(fieldId, (BYTE *)buffer, numElements * sizeof(UINT32));
   size /= sizeof(UINT32);   // Convert bytes to elements
   for(UINT32 i = 0; i < size; i++)
      buffer[i] = ntohl(buffer[i]);
   return size;
}

/**
 * get binary field as an array of 32 bit unsigned integers
 */
UINT32 NXCPMessage::getFieldAsInt32Array(UINT32 fieldId, IntegerArray<UINT32> *data)
{
   data->clear();

   UINT32 *value = (UINT32 *)get(fieldId, NXCP_DT_BINARY);
   if (value != NULL)
   {
      UINT32 size = *value;
      value++;
      for(UINT32 i = 0; i < size; i++)
      {
         data->add(ntohl(*value));
         value++;
      }
   }
   return (UINT32)data->size();
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
   TCHAR *str = (TCHAR *)malloc(*((UINT32 *)df) * 2 + 4);
#elif defined(UNICODE) && defined(UNICODE_UCS2)
   TCHAR *str = (TCHAR *)malloc(*((UINT32 *)df) + 2);
#else
   TCHAR *str = (TCHAR *)malloc(*((UINT32 *)df) / 2 + 1);
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
 * Dump NXCP message
 */
String NXCPMessage::dump(NXCP_MESSAGE *msg, int version)
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
   for(i = 0; i < (int)size; i += 16)
   {
      BinToStr(((BYTE *)msg) + i, min(16, size - i), buffer); 
      out.appendFormattedString(_T("  ** %s\n"), buffer);
   }

   // header
   out.appendFormattedString(_T("  ** code=0x%04X (%s) flags=0x%04X id=%d size=%d numFields=%d\n"), 
      code, NXCPMessageCodeName(code, buffer), flags, id, size, numFields);
   if (flags & MF_BINARY)
   {
      out += _T("  ** binary message\n");
      return out;
   }
   
   // Parse data fields
   size_t pos = NXCP_HEADER_SIZE;
   for(int f = 0; f < numFields; f++)
   {
      NXCP_MESSAGE_FIELD *field = (NXCP_MESSAGE_FIELD *)(((BYTE *)msg) + pos);

      // Validate position inside message
      if (pos > size - 8)
      {
         out += _T("  ** message format error (pos > size - 8)\n");
         break;
      }
      if ((pos > size - 12) && 
          ((field->type == NXCP_DT_STRING) || (field->type == NXCP_DT_BINARY)))
      {
         out.appendFormattedString(_T("  ** message format error (pos > size - 8 and field type %d)\n"), (int)field->type);
         break;
      }

      // Calculate and validate field size
      size_t fieldSize = CalculateFieldSize(field, TRUE);
      if (pos + fieldSize > size)
      {
         out += _T("  ** message format error (invalid field size)\n");
         break;
      }

      // Create new entry
      NXCP_MESSAGE_FIELD *convertedField = (NXCP_MESSAGE_FIELD *)malloc(fieldSize);
      memcpy(convertedField, field, fieldSize);

      // Convert numeric values to host format
      convertedField->fieldId = ntohl(convertedField->fieldId);
      switch(field->type)
      {
         case NXCP_DT_INT32:
            convertedField->df_int32 = ntohl(convertedField->df_int32);
            out.appendFormattedString(_T("  ** [%6d] INT32  %d\n"), (int)convertedField->fieldId, convertedField->df_int32);
            break;
         case NXCP_DT_INT64:
            convertedField->df_int64 = ntohq(convertedField->df_int64);
            out.appendFormattedString(_T("  ** [%6d] INT64  ") INT64_FMT _T("\n"), (int)convertedField->fieldId, convertedField->df_int64);
            break;
         case NXCP_DT_INT16:
            convertedField->df_int16 = ntohs(convertedField->df_int16);
            out.appendFormattedString(_T("  ** [%6d] INT16  %d\n"), (int)convertedField->fieldId, (int)convertedField->df_int16);
            break;
         case NXCP_DT_FLOAT:
            convertedField->df_real = ntohd(convertedField->df_real);
            out.appendFormattedString(_T("  ** [%6d] FLOAT  %f\n"), (int)convertedField->fieldId, convertedField->df_real);
            break;
         case NXCP_DT_STRING:
#if !(WORDS_BIGENDIAN)
            convertedField->df_string.length = ntohl(convertedField->df_string.length);
            for(i = 0; i < (int)convertedField->df_string.length / 2; i++)
               convertedField->df_string.value[i] = ntohs(convertedField->df_string.value[i]);
#endif
            str = GetStringFromField((BYTE *)convertedField + 8);
            out.appendFormattedString(_T("  ** [%6d] STRING \"%s\"\n"), (int)convertedField->fieldId, str);
            free(str);
            break;
         case NXCP_DT_BINARY:
            convertedField->df_string.length = ntohl(convertedField->df_string.length);
            out.appendFormattedString(_T("  ** [%6d] BINARY len=%d\n"), (int)convertedField->fieldId, (int)convertedField->df_string.length);
            break;
         default:
            out.appendFormattedString(_T("  ** [%6d] unknown type %d\n"), (int)convertedField->fieldId, (int)field->type);
            break;
      }
      free(convertedField);

      // Starting from version 2, all fields should be 8-byte aligned
      if (version >= 2)
         pos += fieldSize + ((8 - (fieldSize % 8)) & 7);
      else
         pos += fieldSize;
   }

   return out;
}
