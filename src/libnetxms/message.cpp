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
#include <expat.h>
#include <uthash.h>

/**
 * XML parser state codes for creating CSCPMessage object from XML
 */
enum ParserStates
{
   XML_STATE_INIT = -1,
   XML_STATE_END = -2,
   XML_STATE_ERROR = -255,
   XML_STATE_NXCP = 0,
   XML_STATE_MESSAGE = 1,
   XML_STATE_VARIABLE = 2,
   XML_STATE_VALUE = 3
};

/**
 * XML parser state data
 */
typedef struct
{
	CSCPMessage *msg;
	int state;
	int valueLen;
	char *value;
	int varType;
	UINT32 varId;
} XML_PARSER_STATE;

/**
 * Calculate field size
 */
static int CalculateFieldSize(CSCP_DF *field, bool networkByteOrder)
{
   int nSize;

   switch(field->bType)
   {
      case CSCP_DT_INT32:
         nSize = 12;
         break;
      case CSCP_DT_INT64:
      case CSCP_DT_FLOAT:
         nSize = 16;
         break;
      case CSCP_DT_INT16:
         nSize = 8;
         break;
      case CSCP_DT_STRING:
      case CSCP_DT_BINARY:
         if (networkByteOrder)
            nSize = ntohl(field->df_string.dwLen) + 12;
         else
            nSize = field->df_string.dwLen + 12;
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
   UINT32 size;
   CSCP_DF data;
};

/**
 * Create new hash entry wth given field size
 */
inline MessageField *CreateMessageField(int fieldSize)
{
   int entrySize = sizeof(MessageField) - sizeof(CSCP_DF) + fieldSize;
   MessageField *entry = (MessageField *)malloc(entrySize);
   entry->size = entrySize;
   return entry;
}

/**
 * Default constructor for CSCPMessage class
 */
CSCPMessage::CSCPMessage(int version)
{
   m_code = 0;
   m_id = 0;
   m_fields = NULL;
   m_flags = 0;
   m_version = version;
}

/**
 * Create a copy of prepared CSCP message
 */
CSCPMessage::CSCPMessage(CSCPMessage *pMsg)
{
   m_code = pMsg->m_code;
   m_id = pMsg->m_id;
   m_flags = pMsg->m_flags;
   m_version = pMsg->m_version;
   m_fields = NULL;

   MessageField *entry, *tmp;
   HASH_ITER(hh, pMsg->m_fields, entry, tmp)
   {
      MessageField *f = (MessageField *)nx_memdup(entry, entry->size);
      HASH_ADD_INT(m_fields, id, f);
   }
}

/**
 * Create CSCPMessage object from received message
 */
CSCPMessage::CSCPMessage(CSCP_MESSAGE *pMsg, int version)
{
   UINT32 i;

   m_flags = ntohs(pMsg->wFlags);
   m_code = ntohs(pMsg->wCode);
   m_id = ntohl(pMsg->dwId);
   m_version = version;
   m_fields = NULL;

   // Parse data fields
   int fieldCount = (int)ntohl(pMsg->dwNumVars);
   UINT32 dwSize = ntohl(pMsg->dwSize);
   UINT32 dwPos = CSCP_HEADER_SIZE;
   for(int f = 0; f < fieldCount; f++)
   {
      CSCP_DF *field = (CSCP_DF *)(((BYTE *)pMsg) + dwPos);

      // Validate position inside message
      if (dwPos > dwSize - 8)
         break;
      if ((dwPos > dwSize - 12) && 
          ((field->bType == CSCP_DT_STRING) || (field->bType == CSCP_DT_BINARY)))
         break;

      // Calculate and validate variable size
      int fieldSize = CalculateFieldSize(field, true);
      if (dwPos + fieldSize > dwSize)
         break;

      // Create new entry
      MessageField *entry = CreateMessageField(fieldSize);
      entry->id = ntohl(field->fieldId);
      memcpy(&entry->data, field, fieldSize);

      // Convert values to host format
      entry->data.fieldId = ntohl(entry->data.fieldId);
      switch(field->bType)
      {
         case CSCP_DT_INT32:
            entry->data.df_int32 = ntohl(entry->data.df_int32);
            break;
         case CSCP_DT_INT64:
            entry->data.df_int64 = ntohq(entry->data.df_int64);
            break;
         case CSCP_DT_INT16:
            entry->data.df_int16 = ntohs(entry->data.df_int16);
            break;
         case CSCP_DT_FLOAT:
            entry->data.df_real = ntohd(entry->data.df_real);
            break;
         case CSCP_DT_STRING:
#if !(WORDS_BIGENDIAN)
            entry->data.df_string.dwLen = ntohl(entry->data.df_string.dwLen);
            for(i = 0; i < entry->data.df_string.dwLen / 2; i++)
               entry->data.df_string.szValue[i] = ntohs(entry->data.df_string.szValue[i]);
#endif
            break;
         case CSCP_DT_BINARY:
            entry->data.df_string.dwLen = ntohl(entry->data.df_string.dwLen);
            break;
      }

      HASH_ADD_INT(m_fields, id, entry);

      // Starting from version 2, all variables should be 8-byte aligned
      if (m_version >= 2)
         dwPos += fieldSize + ((8 - (fieldSize % 8)) & 7);
      else
         dwPos += fieldSize;
   }
}

/**
 * Create CSCPMessage object from XML document
 */
static void StartElement(void *userData, const char *name, const char **attrs)
{
	if (!strcmp(name, "nxcp"))
	{
		((XML_PARSER_STATE *)userData)->state = XML_STATE_NXCP;
	}
	else if (!strcmp(name, "message"))
	{
		((XML_PARSER_STATE *)userData)->state = XML_STATE_MESSAGE;
	}
	else if (!strcmp(name, "variable"))
	{
		((XML_PARSER_STATE *)userData)->state = XML_STATE_VARIABLE;
	}
	else if (!strcmp(name, "value"))
	{
		((XML_PARSER_STATE *)userData)->valueLen = 1;
		((XML_PARSER_STATE *)userData)->value = NULL;
		((XML_PARSER_STATE *)userData)->state = XML_STATE_VALUE;
	}
	else
	{
		((XML_PARSER_STATE *)userData)->state = XML_STATE_ERROR;
	}
	if (((XML_PARSER_STATE *)userData)->state != XML_STATE_ERROR)
		((XML_PARSER_STATE *)userData)->msg->processXMLToken(userData, attrs);
}

static void EndElement(void *userData, const char *name)
{
	if (!strcmp(name, "nxcp"))
	{
		((XML_PARSER_STATE *)userData)->state = XML_STATE_END;
	}
	else if (!strcmp(name, "message"))
	{
		((XML_PARSER_STATE *)userData)->state = XML_STATE_NXCP;
	}
	else if (!strcmp(name, "variable"))
	{
		((XML_PARSER_STATE *)userData)->state = XML_STATE_MESSAGE;
	}
	else if (!strcmp(name, "value"))
	{
		((XML_PARSER_STATE *)userData)->msg->processXMLData(userData);
		safe_free(((XML_PARSER_STATE *)userData)->value);
		((XML_PARSER_STATE *)userData)->state = XML_STATE_VARIABLE;
	}
}

static void CharData(void *userData, const XML_Char *s, int len)
{
	XML_PARSER_STATE *ps = (XML_PARSER_STATE *)userData;

	if (ps->state != XML_STATE_VALUE)
		return;

	ps->value = (char *)realloc(ps->value, ps->valueLen + len);
	memcpy(&ps->value[ps->valueLen - 1], s, len);
	ps->valueLen += len;
	ps->value[ps->valueLen - 1] = 0;
}

CSCPMessage::CSCPMessage(const char *xml)
{
	XML_Parser parser = XML_ParserCreate(NULL);
	XML_PARSER_STATE state;

	// Default values
   m_code = 0;
   m_id = 0;
   m_fields = NULL;
   m_flags = 0;
   m_version = NXCP_VERSION;

	// Parse XML
	state.msg = this;
	state.state = -1;
	XML_SetUserData(parser, &state);
	XML_SetElementHandler(parser, StartElement, EndElement);
	XML_SetCharacterDataHandler(parser, CharData);
	if (XML_Parse(parser, xml, (int)strlen(xml), TRUE) == XML_STATUS_ERROR)
	{
/*fprintf(stderr,
        "%s at line %d\n",
        XML_ErrorString(XML_GetErrorCode(parser)),
        XML_GetCurrentLineNumber(parser));*/
	}
	XML_ParserFree(parser);
}

void CSCPMessage::processXMLToken(void *state, const char **attrs)
{
	XML_PARSER_STATE *ps = (XML_PARSER_STATE *)state;
	const char *type;
	static const char *types[] = { "int32", "string", "int64", "int16", "binary", "float", NULL };

	switch(ps->state)
	{
		case XML_STATE_NXCP:
			m_version = XMLGetAttrInt(attrs, "version", m_version);
			break;
		case XML_STATE_MESSAGE:
			m_id = XMLGetAttrUINT32(attrs, "id", m_id);
			m_code = (WORD)XMLGetAttrUINT32(attrs, "code", m_code);
			break;
		case XML_STATE_VARIABLE:
			ps->varId = XMLGetAttrUINT32(attrs, "id", 0);
			type = XMLGetAttr(attrs, "type");
			if (type != NULL)
			{
				int i;

				for(i = 0; types[i] != NULL; i++)
					if (!stricmp(types[i], type))
					{
						ps->varType = i;
						break;
					}
			}
			break;
		default:
			break;
	}
}

void CSCPMessage::processXMLData(void *state)
{
	XML_PARSER_STATE *ps = (XML_PARSER_STATE *)state;
	char *binData;
	size_t binLen;
#ifdef UNICODE
	WCHAR *temp;
#endif

	if (ps->value == NULL)
		return;

	switch(ps->varType)
	{
		case CSCP_DT_INT32:
			SetVariable(ps->varId, (UINT32)strtoul(ps->value, NULL, 0));
			break;
		case CSCP_DT_INT16:
			SetVariable(ps->varId, (WORD)strtoul(ps->value, NULL, 0));
			break;
		case CSCP_DT_INT64:
			SetVariable(ps->varId, (UINT64)strtoull(ps->value, NULL, 0));
			break;
		case CSCP_DT_FLOAT:
			SetVariable(ps->varId, strtod(ps->value, NULL));
			break;
		case CSCP_DT_STRING:
#ifdef UNICODE
			temp = WideStringFromUTF8String(ps->value);
			SetVariable(ps->varId, temp);
			free(temp);
#else
			SetVariable(ps->varId, ps->value);
#endif
			break;
		case CSCP_DT_BINARY:
			if (base64_decode_alloc(ps->value, ps->valueLen, &binData, &binLen))
			{
				if (binData != NULL)
				{
					SetVariable(ps->varId, (BYTE *)binData, (UINT32)binLen);
					free(binData);
				}
			}
			break;
	}
}

/**
 * Destructor for CSCPMessage
 */
CSCPMessage::~CSCPMessage()
{
   deleteAllVariables();
}

/**
 * Find field by ID
 */
CSCP_DF *CSCPMessage::find(UINT32 fieldId)
{
   MessageField *entry;
   HASH_FIND_INT(m_fields, &fieldId, entry);
   return (entry != NULL) ? &entry->data : NULL;
}

/**
 * set variable
 * Argument dwSize (data size) contains data length in bytes for DT_BINARY type
 * and maximum number of characters for DT_STRING type (0 means no limit)
 */
void *CSCPMessage::set(UINT32 fieldId, BYTE bType, const void *pValue, UINT32 dwSize)
{
   UINT32 dwLength;
#if defined(UNICODE_UCS2) && defined(UNICODE)
#define __buffer pValue
#else
   UCS2CHAR *__buffer;
#endif

   // Create entry
   MessageField *entry;
   switch(bType)
   {
      case CSCP_DT_INT32:
         entry = CreateMessageField(12);
         entry->data.df_int32 = *((const UINT32 *)pValue);
         break;
      case CSCP_DT_INT16:
         entry = CreateMessageField(8);
         entry->data.df_int16 = *((const WORD *)pValue);
         break;
      case CSCP_DT_INT64:
         entry = CreateMessageField(16);
         entry->data.df_int64 = *((const UINT64 *)pValue);
         break;
      case CSCP_DT_FLOAT:
         entry = CreateMessageField(16);
         entry->data.df_real = *((const double *)pValue);
         break;
      case CSCP_DT_STRING:
#ifdef UNICODE         
         dwLength = (UINT32)_tcslen((const TCHAR *)pValue);
			if ((dwSize > 0) && (dwLength > dwSize))
				dwLength = dwSize;
#ifndef UNICODE_UCS2 /* assume UNICODE_UCS4 */
         __buffer = (UCS2CHAR *)malloc(dwLength * 2 + 2);
         ucs4_to_ucs2((WCHAR *)pValue, dwLength, __buffer, dwLength + 1);
#endif         
#else		/* not UNICODE */
			__buffer = UCS2StringFromMBString((const char *)pValue);
			dwLength = (UINT32)ucs2_strlen(__buffer);
			if ((dwSize > 0) && (dwLength > dwSize))
				dwLength = dwSize;
#endif
         entry = CreateMessageField(12 + dwLength * 2);
         entry->data.df_string.dwLen = dwLength * 2;
         memcpy(entry->data.df_string.szValue, __buffer, entry->data.df_string.dwLen);
#if !defined(UNICODE_UCS2) || !defined(UNICODE)
         free(__buffer);
#endif
         break;
      case CSCP_DT_BINARY:
         entry = CreateMessageField(12 + dwSize);
         entry->data.df_string.dwLen = dwSize;
         if ((entry->data.df_string.dwLen > 0) && (pValue != NULL))
            memcpy(entry->data.df_string.szValue, pValue, entry->data.df_string.dwLen);
         break;
      default:
         return NULL;  // Invalid data type, unable to handle
   }
   entry->id = fieldId;
   entry->data.fieldId = fieldId;
   entry->data.bType = bType;

   // add or replace field
   MessageField *curr;
   HASH_FIND_INT(m_fields, &fieldId, curr);
   if (curr != NULL)
   {
      HASH_DEL(m_fields, curr);
      free(curr);
   }
   HASH_ADD_INT(m_fields, id, entry);

   return (bType == CSCP_DT_INT16) ? ((void *)((BYTE *)&entry->data + 6)) : ((void *)((BYTE *)&entry->data + 8));
#undef __buffer
}

/**
 * get field value
 */
void *CSCPMessage::get(UINT32 fieldId, BYTE requiredType, BYTE *fieldType)
{
   CSCP_DF *field = find(fieldId);
   if (field == NULL)
      return NULL;      // No such field

   // Check data type
   if ((requiredType != 0xFF) && (field->bType != requiredType))
      return NULL;

   if (fieldType != NULL)
      *fieldType = field->bType;
   return (field->bType == CSCP_DT_INT16) ?
           ((void *)((BYTE *)field + 6)) : 
           ((void *)((BYTE *)field + 8));
}

/**
 * get 16 bit field as boolean
 */
bool CSCPMessage::getFieldAsBoolean(UINT32 fieldId)
{
   BYTE type;
   void *value = (void *)get(fieldId, 0xFF, &type);
   if (value == NULL)
      return false;

   switch(type)
   {
      case CSCP_DT_INT16:
         return *((UINT16 *)value) ? true : false;
      case CSCP_DT_INT32:
         return *((UINT32 *)value) ? true : false;
      case CSCP_DT_INT64:
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
int CSCPMessage::getFieldType(UINT32 fieldId)
{
   CSCP_DF *field = find(fieldId);
   return (field != NULL) ? (int)field->bType : -1;
}

/**
 * get signed integer field
 */
INT32 CSCPMessage::getFieldAsInt32(UINT32 fieldId)
{
   char *value = (char *)get(fieldId, CSCP_DT_INT32);
   return (value != NULL) ? *((INT32 *)value) : 0;
}

/**
 * get unsigned integer field
 */
UINT32 CSCPMessage::GetVariableLong(UINT32 fieldId)
{
   void *value = get(fieldId, CSCP_DT_INT32);
   return (value != NULL) ? *((UINT32 *)value) : 0;
}

/**
 * get signed 16-bit integer field
 */
INT16 CSCPMessage::getFieldAsInt16(UINT32 fieldId)
{
   void *value = get(fieldId, CSCP_DT_INT16);
   return (value != NULL) ? *((INT16 *)value) : 0;
}

/**
 * get unsigned 16-bit integer variable
 */
UINT16 CSCPMessage::GetVariableShort(UINT32 fieldId)
{
   void *pValue = get(fieldId, CSCP_DT_INT16);
   return pValue ? *((WORD *)pValue) : 0;
}

/**
 * get signed 64-bit integer field
 */
INT64 CSCPMessage::getFieldAsInt64(UINT32 fieldId)
{
   void *value = get(fieldId, CSCP_DT_INT64);
   return (value != NULL) ? *((INT64 *)value) : 0;
}

/**
 * get unsigned 64-bit integer field
 */
UINT64 CSCPMessage::GetVariableInt64(UINT32 fieldId)
{
   void *pValue = get(fieldId, CSCP_DT_INT64);
   return pValue ? *((UINT64 *)pValue) : 0;
}

/**
 * get 64-bit floating point variable
 */
double CSCPMessage::getFieldAsDouble(UINT32 fieldId)
{
   void *value = get(fieldId, CSCP_DT_FLOAT);
   return (value != NULL) ? *((double *)value) : 0;
}

/**
 * get time_t field
 */
time_t CSCPMessage::getFieldAsTime(UINT32 fieldId)
{
   BYTE type;
   void *value = (void *)get(fieldId, 0xFF, &type);
   if (value == NULL)
      return 0;

   switch(type)
   {
      case CSCP_DT_INT32:
         return (time_t)(*((UINT32 *)value));
      case CSCP_DT_INT64:
         return (time_t)(*((UINT64 *)value));
      default:
         return false;
   }
}

/**
 * get string variable
 * If szBuffer is NULL, memory block of required size will be allocated
 * for result; if szBuffer is not NULL, entire result or part of it will
 * be placed to szBuffer and pointer to szBuffer will be returned.
 * Note: dwBufSize is buffer size in characters, not bytes!
 */
TCHAR *CSCPMessage::GetVariableStr(UINT32 fieldId, TCHAR *pszBuffer, UINT32 dwBufSize)
{
   void *pValue;
   TCHAR *str = NULL;
   UINT32 dwLen;

   if ((pszBuffer != NULL) && (dwBufSize == 0))
      return NULL;   // non-sense combination

   pValue = get(fieldId, CSCP_DT_STRING);
   if (pValue != NULL)
   {
      if (pszBuffer == NULL)
      {
#if defined(UNICODE) && defined(UNICODE_UCS4)
         str = (TCHAR *)malloc(*((UINT32 *)pValue) * 2 + 4);
#elif defined(UNICODE) && defined(UNICODE_UCS2)
         str = (TCHAR *)malloc(*((UINT32 *)pValue) + 2);
#else
         str = (TCHAR *)malloc(*((UINT32 *)pValue) / 2 + 1);
#endif
      }
      else
      {
         str = pszBuffer;
      }

      dwLen = (pszBuffer == NULL) ? (*((UINT32 *)pValue) / 2) : min(*((UINT32 *)pValue) / 2, dwBufSize - 1);
#if defined(UNICODE) && defined(UNICODE_UCS4)
		ucs2_to_ucs4((UCS2CHAR *)((BYTE *)pValue + 4), dwLen, str, dwLen + 1);
#elif defined(UNICODE) && defined(UNICODE_UCS2)
      memcpy(str, (BYTE *)pValue + 4, dwLen * 2);
#else
		ucs2_to_mb((UCS2CHAR *)((BYTE *)pValue + 4), dwLen, str, dwLen + 1);
#endif
      str[dwLen] = 0;
   }
   else
   {
      if (pszBuffer != NULL)
      {
         str = pszBuffer;
         str[0] = 0;
      }
   }
   return str;
}

#ifdef UNICODE

/**
 * get variable as multibyte string
 */
char *CSCPMessage::GetVariableStrA(UINT32 fieldId, char *pszBuffer, UINT32 dwBufSize)
{
   void *pValue;
   char *str = NULL;
   UINT32 dwLen;

   if ((pszBuffer != NULL) && (dwBufSize == 0))
      return NULL;   // non-sense combination

   pValue = get(fieldId, CSCP_DT_STRING);
   if (pValue != NULL)
   {
      if (pszBuffer == NULL)
      {
         str = (char *)malloc(*((UINT32 *)pValue) / 2 + 1);
      }
      else
      {
         str = pszBuffer;
      }

      dwLen = (pszBuffer == NULL) ? (*((UINT32 *)pValue) / 2) : min(*((UINT32 *)pValue) / 2, dwBufSize - 1);
		ucs2_to_mb((UCS2CHAR *)((BYTE *)pValue + 4), dwLen, str, dwLen + 1);
      str[dwLen] = 0;
   }
   else
   {
      if (pszBuffer != NULL)
      {
         str = pszBuffer;
         str[0] = 0;
      }
   }
   return str;
}

#else

/**
 * get field as multibyte string
 */
char *CSCPMessage::GetVariableStrA(UINT32 fieldId, char *pszBuffer, UINT32 dwBufSize)
{
	return GetVariableStr(fieldId, pszBuffer, dwBufSize);
}

#endif

/**
 * get field as UTF-8 string
 */
char *CSCPMessage::GetVariableStrUTF8(UINT32 fieldId, char *pszBuffer, UINT32 dwBufSize)
{
   void *pValue;
   char *str = NULL;
   UINT32 dwLen, dwOutSize;
	int cc;

   if ((pszBuffer != NULL) && (dwBufSize == 0))
      return NULL;   // non-sense combination

   pValue = get(fieldId, CSCP_DT_STRING);
   if (pValue != NULL)
   {
      if (pszBuffer == NULL)
      {
			// Assume worst case scenario - 3 bytes per character
			dwOutSize = *((UINT32 *)pValue) + *((UINT32 *)pValue) / 2 + 1;
         str = (char *)malloc(dwOutSize);
      }
      else
      {
			dwOutSize = dwBufSize;
         str = pszBuffer;
      }

      dwLen = *((UINT32 *)pValue) / 2;
#ifdef UNICODE_UCS2
		cc = WideCharToMultiByte(CP_UTF8, 0, (WCHAR *)((BYTE *)pValue + 4), dwLen, str, dwOutSize - 1, NULL, NULL);
#else
		cc = ucs2_to_utf8((UCS2CHAR *)((BYTE *)pValue + 4), dwLen, str, dwOutSize - 1);
#endif
      str[cc] = 0;
   }
   else
   {
      if (pszBuffer != NULL)
      {
         str = pszBuffer;
         str[0] = 0;
      }
   }
   return str;
}

/**
 * get binary (byte array) field
 * Result will be placed to the buffer provided (no more than dwBufSize bytes,
 * and actual size of data will be returned
 * If pBuffer is NULL, just actual data length is returned
 */
UINT32 CSCPMessage::GetVariableBinary(UINT32 fieldId, BYTE *pBuffer, UINT32 dwBufSize)
{
   UINT32 size;
   void *value = get(fieldId, CSCP_DT_BINARY);
   if (value != NULL)
   {
      size = *((UINT32 *)value);
      if (pBuffer != NULL)
         memcpy(pBuffer, (BYTE *)value + 4, min(dwBufSize, size));
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
BYTE *CSCPMessage::getBinaryFieldPtr(UINT32 fieldId, size_t *size)
{
   BYTE *data;
   void *value = get(fieldId, CSCP_DT_BINARY);
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
CSCP_MESSAGE *CSCPMessage::createMessage()
{
   // Calculate message size
   UINT32 dwSize = CSCP_HEADER_SIZE;
   UINT32 fieldCount = 0;
   MessageField *entry, *tmp;
   HASH_ITER(hh, m_fields, entry, tmp)
   {
      int fieldSize = CalculateFieldSize(&entry->data, false);
      if (m_version >= 2)
         dwSize += fieldSize + ((8 - (fieldSize % 8)) & 7);
      else
         dwSize += fieldSize;
      fieldCount++;
   }

   // Message should be aligned to 8 bytes boundary
   // This is always the case starting from version 2 because
   // all variables are padded to 8 bytes boundary
   if (m_version < 2)
      dwSize += (8 - (dwSize % 8)) & 7;

   // Create message
   CSCP_MESSAGE *pMsg = (CSCP_MESSAGE *)malloc(dwSize);
   memset(pMsg, 0, dwSize);
   pMsg->wCode = htons(m_code);
   pMsg->wFlags = htons(m_flags);
   pMsg->dwSize = htonl(dwSize);
   pMsg->dwId = htonl(m_id);
   pMsg->dwNumVars = htonl(fieldCount);

   // Fill data fields
   CSCP_DF *field = (CSCP_DF *)((char *)pMsg + CSCP_HEADER_SIZE);
   HASH_ITER(hh, m_fields, entry, tmp)
   {
      int fieldSize = CalculateFieldSize(&entry->data, false);
      memcpy(field, &entry->data, fieldSize);

      // Convert numeric values to network format
      field->fieldId = htonl(field->fieldId);
      switch(field->bType)
      {
         case CSCP_DT_INT32:
            field->df_int32 = htonl(field->df_int32);
            break;
         case CSCP_DT_INT64:
            field->df_int64 = htonq(field->df_int64);
            break;
         case CSCP_DT_INT16:
            field->df_int16 = htons(field->df_int16);
            break;
         case CSCP_DT_FLOAT:
            field->df_real = htond(field->df_real);
            break;
         case CSCP_DT_STRING:
#if !(WORDS_BIGENDIAN)
            {
               for(UINT32 i = 0; i < field->df_string.dwLen / 2; i++)
                  field->df_string.szValue[i] = htons(field->df_string.szValue[i]);
               field->df_string.dwLen = htonl(field->df_string.dwLen);
            }
#endif
            break;
         case CSCP_DT_BINARY:
            field->df_string.dwLen = htonl(field->df_string.dwLen);
            break;
      }

      if (m_version >= 2)
         field = (CSCP_DF *)((char *)field + fieldSize + ((8 - (fieldSize % 8)) & 7));
      else
         field = (CSCP_DF *)((char *)field + fieldSize);
   }

   return pMsg;
}

/**
 * Delete all variables
 */
void CSCPMessage::deleteAllVariables()
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
void CSCPMessage::SetVariableFromMBString(UINT32 fieldId, const char *pszValue)
{
	WCHAR *wcValue = WideStringFromMBString(pszValue);
	set(fieldId, CSCP_DT_STRING, wcValue);
	free(wcValue);
}

#endif

/**
 * set binary field to an array of UINT32s
 */
void CSCPMessage::setFieldInt32Array(UINT32 fieldId, UINT32 dwNumElements, const UINT32 *pdwData)
{
   UINT32 *pdwBuffer = (UINT32 *)set(fieldId, CSCP_DT_BINARY, pdwData, dwNumElements * sizeof(UINT32));
   if (pdwBuffer != NULL)
   {
      pdwBuffer++;   // First UINT32 is a length field
      for(UINT32 i = 0; i < dwNumElements; i++)  // Convert UINT32s to network byte order
         pdwBuffer[i] = htonl(pdwBuffer[i]);
   }
}

/**
 * set binary field to an array of UINT32s
 */
void CSCPMessage::setFieldInt32Array(UINT32 fieldId, IntegerArray<UINT32> *data)
{
   UINT32 *pdwBuffer = (UINT32 *)set(fieldId, CSCP_DT_BINARY, data->getBuffer(), data->size() * sizeof(UINT32));
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
UINT32 CSCPMessage::getFieldAsInt32Array(UINT32 fieldId, UINT32 numElements, UINT32 *buffer)
{
   UINT32 size = GetVariableBinary(fieldId, (BYTE *)buffer, numElements * sizeof(UINT32));
   size /= sizeof(UINT32);   // Convert bytes to elements
   for(UINT32 i = 0; i < size; i++)
      buffer[i] = ntohl(buffer[i]);
   return size;
}

/**
 * get binary field as an array of 32 bit unsigned integers
 */
UINT32 CSCPMessage::getFieldAsInt32Array(UINT32 fieldId, IntegerArray<UINT32> *data)
{
   data->clear();

   UINT32 *value = (UINT32 *)get(fieldId, CSCP_DT_BINARY);
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
bool CSCPMessage::setFieldFromFile(UINT32 fieldId, const TCHAR *pszFileName)
{
   FILE *pFile;
   BYTE *pBuffer;
   UINT32 dwSize;
   bool bResult = false;

   dwSize = (UINT32)FileSize(pszFileName);
   pFile = _tfopen(pszFileName, _T("rb"));
   if (pFile != NULL)
   {
      pBuffer = (BYTE *)set(fieldId, CSCP_DT_BINARY, NULL, dwSize);
      if (pBuffer != NULL)
      {
         if (fread(pBuffer + sizeof(UINT32), 1, dwSize, pFile) == dwSize)
            bResult = true;
      }
      fclose(pFile);
   }
   return bResult;
}

/**
 * Create XML document
 */
char *CSCPMessage::createXML()
{
	String xml;
	char *out, *bdata;
	size_t blen;
	TCHAR *tempStr;
#if !defined(UNICODE) || defined(UNICODE_UCS4)
	int bytes;
#endif
	static const TCHAR *dtString[] = { _T("int32"), _T("string"), _T("int64"), _T("int16"), _T("binary"), _T("float") };

	xml.addFormattedString(_T("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n<nxcp version=\"%d\">\r\n   <message code=\"%d\" id=\"%d\">\r\n"), m_version, m_code, m_id);
   MessageField *entry, *tmp;
   HASH_ITER(hh, m_fields, entry, tmp)
	{
		xml.addFormattedString(_T("      <variable id=\"%d\" type=\"%s\">\r\n         <value>"),
		                       entry->data.fieldId, dtString[entry->data.bType]);
		switch(entry->data.bType)
		{
			case CSCP_DT_INT32:
				xml.addFormattedString(_T("%d"), entry->data.data.dwInteger);
				break;
			case CSCP_DT_INT16:
				xml.addFormattedString(_T("%d"), entry->data.wInt16);
				break;
			case CSCP_DT_INT64:
				xml.addFormattedString(INT64_FMT, entry->data.data.qwInt64);
				break;
			case CSCP_DT_STRING:
#ifdef UNICODE
#ifdef UNICODE_UCS2
				xml.addDynamicString(EscapeStringForXML((TCHAR *)entry->data.data.string.szValue, entry->data.data.string.dwLen / 2));
#else
				tempStr = (WCHAR *)malloc(entry->data.data.string.dwLen * 2);
				bytes = ucs2_to_ucs4(entry->data.data.string.szValue, entry->data.data.string.dwLen / 2, tempStr, entry->data.data.string.dwLen / 2);
				xml.addDynamicString(EscapeStringForXML(tempStr, bytes));
				free(tempStr);
#endif
#else		/* not UNICODE */
#ifdef UNICODE_UCS2
				bytes = WideCharToMultiByte(CP_UTF8, 0, (UCS2CHAR *)entry->data.data.string.szValue,
				                            entry->data.data.string.dwLen / 2, NULL, 0, NULL, NULL);
				tempStr = (char *)malloc(bytes + 1);
				bytes = WideCharToMultiByte(CP_UTF8, 0, (UCS2CHAR *)entry->data.data.string.szValue,
				                            entry->data.data.string.dwLen / 2, tempStr, bytes + 1, NULL, NULL);
				xml.addDynamicString(EscapeStringForXML(tempStr, bytes));
				free(tempStr);
#else
				tempStr = (char *)malloc(entry->data.data.string.dwLen);
				bytes = ucs2_to_utf8(entry->data.data.string.szValue, entry->data.data.string.dwLen / 2, tempStr, entry->data.data.string.dwLen);
				xml.addDynamicString(EscapeStringForXML(tempStr, bytes));
				free(tempStr);
#endif
#endif	/* UNICODE */
				break;
			case CSCP_DT_BINARY:
				blen = base64_encode_alloc((char *)entry->data.data.string.szValue,
				                           entry->data.data.string.dwLen, &bdata);
				if ((blen != 0) && (bdata != NULL))
				{
#ifdef UNICODE
					tempStr = (WCHAR *)malloc((blen + 1) * sizeof(WCHAR));
					MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, bdata, (int)blen, tempStr, (int)blen);
					tempStr[blen] = 0;
					xml.addDynamicString(tempStr);
#else
					xml.addString(bdata, (UINT32)blen);
#endif
				}
				safe_free(bdata);
				break;
			default:
				break;
		}
		xml += _T("</value>\r\n      </variable>\r\n");
	}
	xml += _T("   </message>\r\n</nxcp>\r\n");

#ifdef UNICODE
	out = UTF8StringFromWideString(xml);
#else
	out = strdup(xml);
#endif
	return out;
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
String CSCPMessage::dump(CSCP_MESSAGE *pMsg, int version)
{
   String out;
   int i;
   TCHAR *str, buffer[128];

   WORD flags = ntohs(pMsg->wFlags);
   WORD code = ntohs(pMsg->wCode);
   UINT32 id = ntohl(pMsg->dwId);
   UINT32 size = ntohl(pMsg->dwSize);
   int numFields = (int)ntohl(pMsg->dwNumVars);

   // Dump raw message
   for(i = 0; i < (int)size; i += 16)
   {
      BinToStr(((BYTE *)pMsg) + i, min(16, size - i), buffer); 
      out.addFormattedString(_T("  ** %s\n"), buffer);
   }

   // header
   out.addFormattedString(_T("  ** code=0x%04X (%s) flags=0x%04X id=%d size=%d numFields=%d\n"), 
      code, NXCPMessageCodeName(code, buffer), flags, id, size, numFields);
   if (flags & MF_BINARY)
   {
      out += _T("  ** binary message\n");
      return out;
   }
   
   // Parse data fields
   UINT32 pos = CSCP_HEADER_SIZE;
   for(int f = 0; f < numFields; f++)
   {
      CSCP_DF *field = (CSCP_DF *)(((BYTE *)pMsg) + pos);

      // Validate position inside message
      if (pos > size - 8)
      {
         out += _T("  ** message format error (pos > size - 8)\n");
         break;
      }
      if ((pos > size - 12) && 
          ((field->bType == CSCP_DT_STRING) || (field->bType == CSCP_DT_BINARY)))
      {
         out.addFormattedString(_T("  ** message format error (pos > size - 8 and field type %d)\n"), (int)field->bType);
         break;
      }

      // Calculate and validate field size
      int fieldSize = CalculateFieldSize(field, TRUE);
      if (pos + fieldSize > size)
      {
         out += _T("  ** message format error (invalid field size)\n");
         break;
      }

      // Create new entry
      CSCP_DF *convertedField = (CSCP_DF *)malloc(fieldSize);
      memcpy(convertedField, field, fieldSize);

      // Convert numeric values to host format
      convertedField->fieldId = ntohl(convertedField->fieldId);
      switch(field->bType)
      {
         case CSCP_DT_INT32:
            convertedField->df_int32 = ntohl(convertedField->df_int32);
            out.addFormattedString(_T("  ** [%6d] INT32  %d\n"), (int)convertedField->fieldId, convertedField->df_int32);
            break;
         case CSCP_DT_INT64:
            convertedField->df_int64 = ntohq(convertedField->df_int64);
            out.addFormattedString(_T("  ** [%6d] INT64  ") INT64_FMT _T("\n"), (int)convertedField->fieldId, convertedField->df_int64);
            break;
         case CSCP_DT_INT16:
            convertedField->df_int16 = ntohs(convertedField->df_int16);
            out.addFormattedString(_T("  ** [%6d] INT16  %d\n"), (int)convertedField->fieldId, (int)convertedField->df_int16);
            break;
         case CSCP_DT_FLOAT:
            convertedField->df_real = ntohd(convertedField->df_real);
            out.addFormattedString(_T("  ** [%6d] FLOAT  %f\n"), (int)convertedField->fieldId, convertedField->df_real);
            break;
         case CSCP_DT_STRING:
#if !(WORDS_BIGENDIAN)
            convertedField->df_string.dwLen = ntohl(convertedField->df_string.dwLen);
            for(i = 0; i < (int)convertedField->df_string.dwLen / 2; i++)
               convertedField->df_string.szValue[i] = ntohs(convertedField->df_string.szValue[i]);
#endif
            str = GetStringFromField((BYTE *)convertedField + 8);
            out.addFormattedString(_T("  ** [%6d] STRING \"%s\"\n"), (int)convertedField->fieldId, str);
            free(str);
            break;
         case CSCP_DT_BINARY:
            convertedField->df_string.dwLen = ntohl(convertedField->df_string.dwLen);
            out.addFormattedString(_T("  ** [%6d] BINARY len=%d\n"), (int)convertedField->fieldId, (int)convertedField->df_string.dwLen);
            break;
         default:
            out.addFormattedString(_T("  ** [%6d] unknown type %d\n"), (int)convertedField->fieldId, (int)field->bType);
            break;
      }
      free(convertedField);

      // Starting from version 2, all variables should be 8-byte aligned
      if (version >= 2)
         pos += fieldSize + ((8 - (fieldSize % 8)) & 7);
      else
         pos += fieldSize;
   }

   return out;
}
