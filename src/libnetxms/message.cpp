/* 
** NetXMS - Network Management System
** NetXMS Foundation Library
** Copyright (C) 2003-2013 Victor Kirhenshtein
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


//
// Parser state for creating CSCPMessage object from XML
//

#define XML_STATE_INIT		-1
#define XML_STATE_END		-2
#define XML_STATE_ERROR    -255
#define XML_STATE_NXCP		0
#define XML_STATE_MESSAGE	1
#define XML_STATE_VARIABLE	2
#define XML_STATE_VALUE		3

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
static int CalculateFieldSize(CSCP_DF *field, BOOL bNetworkByteOrder)
{
   int nSize;

   switch(field->bType)
   {
      case CSCP_DT_INTEGER:
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
         if (bNetworkByteOrder)
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
 * Default constructor for CSCPMessage class
 */
CSCPMessage::CSCPMessage(int version)
{
   m_wCode = 0;
   m_dwId = 0;
   m_dwNumVar = 0;
   m_ppVarList = NULL;
   m_wFlags = 0;
   m_nVersion = version;
}

/**
 * Create a copy of prepared CSCP message
 */
CSCPMessage::CSCPMessage(CSCPMessage *pMsg)
{
   UINT32 i;

   m_wCode = pMsg->m_wCode;
   m_dwId = pMsg->m_dwId;
   m_wFlags = pMsg->m_wFlags;
   m_nVersion = pMsg->m_nVersion;
   m_dwNumVar = pMsg->m_dwNumVar;
   m_ppVarList = (CSCP_DF **)malloc(sizeof(CSCP_DF *) * m_dwNumVar);
   for(i = 0; i < m_dwNumVar; i++)
   {
      m_ppVarList[i] = (CSCP_DF *)nx_memdup(pMsg->m_ppVarList[i],
                                            CalculateFieldSize(pMsg->m_ppVarList[i], FALSE));
   }
}

/**
 * Create CSCPMessage object from received message
 */
CSCPMessage::CSCPMessage(CSCP_MESSAGE *pMsg, int version)
{
   UINT32 i, dwPos, dwSize, dwVar;
   CSCP_DF *field;
   int iVarSize;

   m_wFlags = ntohs(pMsg->wFlags);
   m_wCode = ntohs(pMsg->wCode);
   m_dwId = ntohl(pMsg->dwId);
   dwSize = ntohl(pMsg->dwSize);
   m_dwNumVar = ntohl(pMsg->dwNumVars);
   m_ppVarList = (CSCP_DF **)malloc(sizeof(CSCP_DF *) * m_dwNumVar);
   m_nVersion = version;
   
   // Parse data fields
   for(dwPos = CSCP_HEADER_SIZE, dwVar = 0; dwVar < m_dwNumVar; dwVar++)
   {
      field = (CSCP_DF *)(((BYTE *)pMsg) + dwPos);

      // Validate position inside message
      if (dwPos > dwSize - 8)
         break;
      if ((dwPos > dwSize - 12) && 
          ((field->bType == CSCP_DT_STRING) || (field->bType == CSCP_DT_BINARY)))
         break;

      // Calculate and validate variable size
      iVarSize = CalculateFieldSize(field, TRUE);
      if (dwPos + iVarSize > dwSize)
         break;

      // Create new entry
      m_ppVarList[dwVar] = (CSCP_DF *)malloc(iVarSize);
      memcpy(m_ppVarList[dwVar], field, iVarSize);

      // Convert numeric values to host format
      m_ppVarList[dwVar]->dwVarId = ntohl(m_ppVarList[dwVar]->dwVarId);
      switch(field->bType)
      {
         case CSCP_DT_INTEGER:
            m_ppVarList[dwVar]->df_int32 = ntohl(m_ppVarList[dwVar]->df_int32);
            break;
         case CSCP_DT_INT64:
            m_ppVarList[dwVar]->df_int64 = ntohq(m_ppVarList[dwVar]->df_int64);
            break;
         case CSCP_DT_INT16:
            m_ppVarList[dwVar]->df_int16 = ntohs(m_ppVarList[dwVar]->df_int16);
            break;
         case CSCP_DT_FLOAT:
            m_ppVarList[dwVar]->df_real = ntohd(m_ppVarList[dwVar]->df_real);
            break;
         case CSCP_DT_STRING:
#if !(WORDS_BIGENDIAN)
            m_ppVarList[dwVar]->df_string.dwLen = ntohl(m_ppVarList[dwVar]->df_string.dwLen);
            for(i = 0; i < m_ppVarList[dwVar]->df_string.dwLen / 2; i++)
               m_ppVarList[dwVar]->df_string.szValue[i] = ntohs(m_ppVarList[dwVar]->df_string.szValue[i]);
#endif
            break;
         case CSCP_DT_BINARY:
            m_ppVarList[dwVar]->df_string.dwLen = ntohl(m_ppVarList[dwVar]->df_string.dwLen);
            break;
      }

      // Starting from version 2, all variables should be 8-byte aligned
      if (m_nVersion >= 2)
         dwPos += iVarSize + ((8 - (iVarSize % 8)) & 7);
      else
         dwPos += iVarSize;
   }

   // Cut unfilled variables, if any
   m_dwNumVar = dwVar;
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
   m_wCode = 0;
   m_dwId = 0;
   m_dwNumVar = 0;
   m_ppVarList = NULL;
   m_wFlags = 0;
   m_nVersion = NXCP_VERSION;

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
			m_nVersion = XMLGetAttrInt(attrs, "version", m_nVersion);
			break;
		case XML_STATE_MESSAGE:
			m_dwId = XMLGetAttrUINT32(attrs, "id", m_dwId);
			m_wCode = (WORD)XMLGetAttrUINT32(attrs, "code", m_wCode);
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
		case CSCP_DT_INTEGER:
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
 * Find variable by name
 */
UINT32 CSCPMessage::findVariable(UINT32 dwVarId)
{
   UINT32 i;

   for(i = 0; i < m_dwNumVar; i++)
      if (m_ppVarList[i] != NULL)
         if (m_ppVarList[i]->dwVarId == dwVarId)
            return i;
   return INVALID_INDEX;
}

/**
 * set variable
 * Argument dwSize (data size) contains data length in bytes for DT_BINARY type
 * and maximum number of characters for DT_STRING type (0 means no limit)
 */
void *CSCPMessage::set(UINT32 dwVarId, BYTE bType, const void *pValue, UINT32 dwSize)
{
   UINT32 dwIndex, dwLength;
   CSCP_DF *field;
#if defined(UNICODE_UCS2) && defined(UNICODE)
#define __buffer pValue
#else
   UCS2CHAR *__buffer;
#endif

   // Create CSCP_DF structure
   switch(bType)
   {
      case CSCP_DT_INTEGER:
         field = (CSCP_DF *)malloc(12);
         field->df_int32 = *((const UINT32 *)pValue);
         break;
      case CSCP_DT_INT16:
         field = (CSCP_DF *)malloc(8);
         field->df_int16 = *((const WORD *)pValue);
         break;
      case CSCP_DT_INT64:
         field = (CSCP_DF *)malloc(16);
         field->df_int64 = *((const UINT64 *)pValue);
         break;
      case CSCP_DT_FLOAT:
         field = (CSCP_DF *)malloc(16);
         field->df_real = *((const double *)pValue);
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
         field = (CSCP_DF *)malloc(12 + dwLength * 2);
         field->df_string.dwLen = dwLength * 2;
         memcpy(field->df_string.szValue, __buffer, field->df_string.dwLen);
#if !defined(UNICODE_UCS2) || !defined(UNICODE)
         free(__buffer);
#endif
         break;
      case CSCP_DT_BINARY:
         field = (CSCP_DF *)malloc(12 + dwSize);
         field->df_string.dwLen = dwSize;
         if ((field->df_string.dwLen > 0) && (pValue != NULL))
            memcpy(field->df_string.szValue, pValue, field->df_string.dwLen);
         break;
      default:
         return NULL;  // Invalid data type, unable to handle
   }
   field->dwVarId = dwVarId;
   field->bType = bType;

   // Check if variable exists
   dwIndex = findVariable(field->dwVarId);
   if (dwIndex == INVALID_INDEX) // Add new variable to list
   {
      m_ppVarList = (CSCP_DF **)realloc(m_ppVarList, sizeof(CSCP_DF *) * (m_dwNumVar + 1));
      m_ppVarList[m_dwNumVar] = field;
      m_dwNumVar++;
   }
   else  // Replace existing variable
   {
      free(m_ppVarList[dwIndex]);
      m_ppVarList[dwIndex] = field;
   }

   return (bType == CSCP_DT_INT16) ? ((void *)((BYTE *)field + 6)) : ((void *)((BYTE *)field + 8));
#undef __buffer
}

/**
 * get variable value
 */
void *CSCPMessage::get(UINT32 dwVarId, BYTE bType)
{
   UINT32 dwIndex;

   // Find variable
   dwIndex = findVariable(dwVarId);
   if (dwIndex == INVALID_INDEX)
      return NULL;      // No such variable

   // Check data type
   if (m_ppVarList[dwIndex]->bType != bType)
      return NULL;

   return (bType == CSCP_DT_INT16) ?
             ((void *)((BYTE *)m_ppVarList[dwIndex] + 6)) : 
             ((void *)((BYTE *)m_ppVarList[dwIndex] + 8));
}

/**
 * get integer variable
 */
UINT32 CSCPMessage::GetVariableLong(UINT32 dwVarId)
{
   char *pValue;

   pValue = (char *)get(dwVarId, CSCP_DT_INTEGER);
   return pValue ? *((UINT32 *)pValue) : 0;
}

/**
 * get 16-bit integer variable
 */
UINT16 CSCPMessage::GetVariableShort(UINT32 dwVarId)
{
   void *pValue;

   pValue = get(dwVarId, CSCP_DT_INT16);
   return pValue ? *((WORD *)pValue) : 0;
}

/**
 * get 16-bit integer variable as signel 32-bit integer
 */
INT32 CSCPMessage::GetVariableShortAsInt32(UINT32 dwVarId)
{
   void *pValue;

   pValue = get(dwVarId, CSCP_DT_INT16);
   return pValue ? *((short *)pValue) : 0;
}

/**
 * get 64-bit integer variable
 */
UINT64 CSCPMessage::GetVariableInt64(UINT32 dwVarId)
{
   char *pValue;

   pValue = (char *)get(dwVarId, CSCP_DT_INT64);
   return pValue ? *((UINT64 *)pValue) : 0;
}

/**
 * get 64-bit floating point variable
 */
double CSCPMessage::GetVariableDouble(UINT32 dwVarId)
{
   char *pValue;

   pValue = (char *)get(dwVarId, CSCP_DT_FLOAT);
   return pValue ? *((double *)pValue) : 0;
}

/**
 * get string variable
 * If szBuffer is NULL, memory block of required size will be allocated
 * for result; if szBuffer is not NULL, entire result or part of it will
 * be placed to szBuffer and pointer to szBuffer will be returned.
 * Note: dwBufSize is buffer size in characters, not bytes!
 */
TCHAR *CSCPMessage::GetVariableStr(UINT32 dwVarId, TCHAR *pszBuffer, UINT32 dwBufSize)
{
   void *pValue;
   TCHAR *str = NULL;
   UINT32 dwLen;

   if ((pszBuffer != NULL) && (dwBufSize == 0))
      return NULL;   // non-sense combination

   pValue = get(dwVarId, CSCP_DT_STRING);
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
char *CSCPMessage::GetVariableStrA(UINT32 dwVarId, char *pszBuffer, UINT32 dwBufSize)
{
   void *pValue;
   char *str = NULL;
   UINT32 dwLen;

   if ((pszBuffer != NULL) && (dwBufSize == 0))
      return NULL;   // non-sense combination

   pValue = get(dwVarId, CSCP_DT_STRING);
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
 * get variable as multibyte string
 */
char *CSCPMessage::GetVariableStrA(UINT32 dwVarId, char *pszBuffer, UINT32 dwBufSize)
{
	return GetVariableStr(dwVarId, pszBuffer, dwBufSize);
}

#endif

/**
 * get variable as UTF-8 string
 */
char *CSCPMessage::GetVariableStrUTF8(UINT32 dwVarId, char *pszBuffer, UINT32 dwBufSize)
{
   void *pValue;
   char *str = NULL;
   UINT32 dwLen, dwOutSize;
	int cc;

   if ((pszBuffer != NULL) && (dwBufSize == 0))
      return NULL;   // non-sense combination

   pValue = get(dwVarId, CSCP_DT_STRING);
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


//
// get binary (byte array) variable
// Result will be placed to the buffer provided (no more than dwBufSize bytes,
// and actual size of data will be returned
// If pBuffer is NULL, just actual data length is returned
//

UINT32 CSCPMessage::GetVariableBinary(UINT32 dwVarId, BYTE *pBuffer, UINT32 dwBufSize)
{
   void *pValue;
   UINT32 dwSize;

   pValue = get(dwVarId, CSCP_DT_BINARY);
   if (pValue != NULL)
   {
      dwSize = *((UINT32 *)pValue);
      if (pBuffer != NULL)
         memcpy(pBuffer, (BYTE *)pValue + 4, min(dwBufSize, dwSize));
   }
   else
   {
      dwSize = 0;
   }
   return dwSize;
}


//
// Build protocol message ready to be send over the wire
//

CSCP_MESSAGE *CSCPMessage::CreateMessage()
{
   UINT32 dwSize;
   int iVarSize;
   UINT32 i, j;
   CSCP_MESSAGE *pMsg;
   CSCP_DF *field;

   // Calculate message size
   for(i = 0, dwSize = CSCP_HEADER_SIZE; i < m_dwNumVar; i++)
   {
      iVarSize = CalculateFieldSize(m_ppVarList[i], FALSE);
      if (m_nVersion >= 2)
         dwSize += iVarSize + ((8 - (iVarSize % 8)) & 7);
      else
         dwSize += iVarSize;
   }

   // Message should be aligned to 8 bytes boundary
   // This is always the case starting from version 2 because
   // all variables padded to be _kratnimi_ 8 bytes
   if (m_nVersion < 2)
      dwSize += (8 - (dwSize % 8)) & 7;

   // Create message
   pMsg = (CSCP_MESSAGE *)malloc(dwSize);
   pMsg->wCode = htons(m_wCode);
   pMsg->wFlags = htons(m_wFlags);
   pMsg->dwSize = htonl(dwSize);
   pMsg->dwId = htonl(m_dwId);
   pMsg->dwNumVars = htonl(m_dwNumVar);

   // Fill data fields
   for(i = 0, field = (CSCP_DF *)((char *)pMsg + CSCP_HEADER_SIZE); i < m_dwNumVar; i++)
   {
      iVarSize = CalculateFieldSize(m_ppVarList[i], FALSE);
      memcpy(field, m_ppVarList[i], iVarSize);

      // Convert numeric values to network format
      field->dwVarId = htonl(field->dwVarId);
      switch(field->bType)
      {
         case CSCP_DT_INTEGER:
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
            for(j = 0; j < field->df_string.dwLen / 2; j++)
               field->df_string.szValue[j] = htons(field->df_string.szValue[j]);
            field->df_string.dwLen = htonl(field->df_string.dwLen);
#endif
            break;
         case CSCP_DT_BINARY:
            field->df_string.dwLen = htonl(field->df_string.dwLen);
            break;
      }

      if (m_nVersion >= 2)
         field = (CSCP_DF *)((char *)field + iVarSize + ((8 - (iVarSize % 8)) & 7));
      else
         field = (CSCP_DF *)((char *)field + iVarSize);
   }

   return pMsg;
}

/**
 * Delete all variables
 */
void CSCPMessage::deleteAllVariables()
{
   if (m_ppVarList != NULL)
   {
      UINT32 i;

      for(i = 0; i < m_dwNumVar; i++)
         safe_free(m_ppVarList[i]);
      free(m_ppVarList);

      m_ppVarList = NULL;
      m_dwNumVar = 0;
   }
}


#ifdef UNICODE

/**
 * set variable from multibyte string
 */
void CSCPMessage::SetVariableFromMBString(UINT32 dwVarId, const char *pszValue)
{
	WCHAR *wcValue = WideStringFromMBString(pszValue);
	set(dwVarId, CSCP_DT_STRING, wcValue);
	free(wcValue);
}

#endif

/**
 * set binary variable to an array of UINT32s
 */
void CSCPMessage::SetVariableToInt32Array(UINT32 dwVarId, UINT32 dwNumElements, const UINT32 *pdwData)
{
   UINT32 i, *pdwBuffer;

   pdwBuffer = (UINT32 *)set(dwVarId, CSCP_DT_BINARY, pdwData, dwNumElements * sizeof(UINT32));
   if (pdwBuffer != NULL)
   {
      pdwBuffer++;   // First UINT32 is a length field
      for(i = 0; i < dwNumElements; i++)  // Convert UINT32s to network byte order
         pdwBuffer[i] = htonl(pdwBuffer[i]);
   }
}

/**
 * get binary variable as an array of UINT32s
 */
UINT32 CSCPMessage::GetVariableInt32Array(UINT32 dwVarId, UINT32 dwNumElements, UINT32 *pdwBuffer)
{
   UINT32 i, dwSize;

   dwSize = GetVariableBinary(dwVarId, (BYTE *)pdwBuffer, dwNumElements * sizeof(UINT32));
   dwSize /= sizeof(UINT32);   // Convert bytes to elements
   for(i = 0; i < dwSize; i++)
      pdwBuffer[i] = ntohl(pdwBuffer[i]);
   return dwSize;
}


//
// set binary variable from file
//

BOOL CSCPMessage::SetVariableFromFile(UINT32 dwVarId, const TCHAR *pszFileName)
{
   FILE *pFile;
   BYTE *pBuffer;
   UINT32 dwSize;
   BOOL bResult = FALSE;

   dwSize = (UINT32)FileSize(pszFileName);
   pFile = _tfopen(pszFileName, _T("rb"));
   if (pFile != NULL)
   {
      pBuffer = (BYTE *)set(dwVarId, CSCP_DT_BINARY, NULL, dwSize);
      if (pBuffer != NULL)
      {
         if (fread(pBuffer + sizeof(UINT32), 1, dwSize, pFile) == dwSize)
            bResult = TRUE;
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
	UINT32 i;
	char *out, *bdata;
	size_t blen;
	TCHAR *tempStr;
#if !defined(UNICODE) || defined(UNICODE_UCS4)
	int bytes;
#endif
	static const TCHAR *dtString[] = { _T("int32"), _T("string"), _T("int64"), _T("int16"), _T("binary"), _T("float") };

	xml.addFormattedString(_T("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n<nxcp version=\"%d\">\r\n   <message code=\"%d\" id=\"%d\">\r\n"), m_nVersion, m_wCode, m_dwId);
	for(i = 0; i < m_dwNumVar; i++)
	{
		xml.addFormattedString(_T("      <variable id=\"%d\" type=\"%s\">\r\n         <value>"),
		                       m_ppVarList[i]->dwVarId, dtString[m_ppVarList[i]->bType]);
		switch(m_ppVarList[i]->bType)
		{
			case CSCP_DT_INTEGER:
				xml.addFormattedString(_T("%d"), m_ppVarList[i]->data.dwInteger);
				break;
			case CSCP_DT_INT16:
				xml.addFormattedString(_T("%d"), m_ppVarList[i]->wInt16);
				break;
			case CSCP_DT_INT64:
				xml.addFormattedString(INT64_FMT, m_ppVarList[i]->data.qwInt64);
				break;
			case CSCP_DT_STRING:
#ifdef UNICODE
#ifdef UNICODE_UCS2
				xml.addDynamicString(EscapeStringForXML((TCHAR *)m_ppVarList[i]->data.string.szValue, m_ppVarList[i]->data.string.dwLen / 2));
#else
				tempStr = (WCHAR *)malloc(m_ppVarList[i]->data.string.dwLen * 2);
				bytes = ucs2_to_ucs4(m_ppVarList[i]->data.string.szValue, m_ppVarList[i]->data.string.dwLen / 2, tempStr, m_ppVarList[i]->data.string.dwLen / 2);
				xml.addDynamicString(EscapeStringForXML(tempStr, bytes));
				free(tempStr);
#endif
#else		/* not UNICODE */
#ifdef UNICODE_UCS2
				bytes = WideCharToMultiByte(CP_UTF8, 0, (UCS2CHAR *)m_ppVarList[i]->data.string.szValue,
				                            m_ppVarList[i]->data.string.dwLen / 2, NULL, 0, NULL, NULL);
				tempStr = (char *)malloc(bytes + 1);
				bytes = WideCharToMultiByte(CP_UTF8, 0, (UCS2CHAR *)m_ppVarList[i]->data.string.szValue,
				                            m_ppVarList[i]->data.string.dwLen / 2, tempStr, bytes + 1, NULL, NULL);
				xml.addDynamicString(EscapeStringForXML(tempStr, bytes));
				free(tempStr);
#else
				tempStr = (char *)malloc(m_ppVarList[i]->data.string.dwLen);
				bytes = ucs2_to_utf8(m_ppVarList[i]->data.string.szValue, m_ppVarList[i]->data.string.dwLen / 2, tempStr, m_ppVarList[i]->data.string.dwLen);
				xml.addDynamicString(EscapeStringForXML(tempStr, bytes));
				free(tempStr);
#endif
#endif	/* UNICODE */
				break;
			case CSCP_DT_BINARY:
				blen = base64_encode_alloc((char *)m_ppVarList[i]->data.string.szValue,
				                           m_ppVarList[i]->data.string.dwLen, &bdata);
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
      convertedField->dwVarId = ntohl(convertedField->dwVarId);
      switch(field->bType)
      {
         case CSCP_DT_INTEGER:
            convertedField->df_int32 = ntohl(convertedField->df_int32);
            out.addFormattedString(_T("  ** [%6d] INT32  %d\n"), (int)convertedField->dwVarId, convertedField->df_int32);
            break;
         case CSCP_DT_INT64:
            convertedField->df_int64 = ntohq(convertedField->df_int64);
            out.addFormattedString(_T("  ** [%6d] INT64  ") INT64_FMT _T("\n"), (int)convertedField->dwVarId, convertedField->df_int64);
            break;
         case CSCP_DT_INT16:
            convertedField->df_int16 = ntohs(convertedField->df_int16);
            out.addFormattedString(_T("  ** [%6d] INT16  %d\n"), (int)convertedField->dwVarId, (int)convertedField->df_int16);
            break;
         case CSCP_DT_FLOAT:
            convertedField->df_real = ntohd(convertedField->df_real);
            out.addFormattedString(_T("  ** [%6d] FLOAT  %f\n"), (int)convertedField->dwVarId, convertedField->df_real);
            break;
         case CSCP_DT_STRING:
#if !(WORDS_BIGENDIAN)
            convertedField->df_string.dwLen = ntohl(convertedField->df_string.dwLen);
            for(i = 0; i < (int)convertedField->df_string.dwLen / 2; i++)
               convertedField->df_string.szValue[i] = ntohs(convertedField->df_string.szValue[i]);
#endif
            str = GetStringFromField((BYTE *)convertedField + 8);
            out.addFormattedString(_T("  ** [%6d] STRING \"%s\"\n"), (int)convertedField->dwVarId, str);
            free(str);
            break;
         case CSCP_DT_BINARY:
            convertedField->df_string.dwLen = ntohl(convertedField->df_string.dwLen);
            out.addFormattedString(_T("  ** [%6d] BINARY len=%d\n"), (int)convertedField->dwVarId, (int)convertedField->df_string.dwLen);
            break;
         default:
            out.addFormattedString(_T("  ** [%6d] unknown type %d\n"), (int)convertedField->dwVarId, (int)field->bType);
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
