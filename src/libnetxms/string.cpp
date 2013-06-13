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
** File: string.cpp
**
**/

#include "libnetxms.h"

/**
 * Static members
 */
const int String::npos = -1;


//
// Constructors
//

String::String()
{
   m_dwBufSize = 1;
   m_pszBuffer = NULL;
}

String::String(const String &src)
{
	m_dwBufSize = src.m_dwBufSize;
	m_pszBuffer = (src.m_pszBuffer != NULL) ? (TCHAR *)nx_memdup(src.m_pszBuffer, src.m_dwBufSize * sizeof(TCHAR)) : NULL;
}

String::String(const TCHAR *init)
{
   m_dwBufSize = (UINT32)_tcslen(init) + 1;
   m_pszBuffer = _tcsdup(init);
}


//
// Destructor
//

String::~String()
{
   safe_free(m_pszBuffer);
}


//
// Operator =
//

const String& String::operator =(const TCHAR *pszStr)
{
   safe_free(m_pszBuffer);
   m_pszBuffer = _tcsdup(CHECK_NULL_EX(pszStr));
   m_dwBufSize = (UINT32)_tcslen(CHECK_NULL_EX(pszStr)) + 1;
   return *this;
}

const String& String::operator =(const String &src)
{
	if (&src == this)
		return *this;
   safe_free(m_pszBuffer);
	m_dwBufSize = src.m_dwBufSize;
	m_pszBuffer = (src.m_pszBuffer != NULL) ? (TCHAR *)nx_memdup(src.m_pszBuffer, src.m_dwBufSize * sizeof(TCHAR)) : NULL;
   return *this;
}

/**
 * Append given string to the end
 */
const String& String::operator +=(const TCHAR *pszStr)
{
   UINT32 dwLen;

	if (pszStr != NULL)
	{
   	dwLen = (UINT32)_tcslen(pszStr);
   	m_pszBuffer = (TCHAR *)realloc(m_pszBuffer, (m_dwBufSize + dwLen) * sizeof(TCHAR));
   	_tcscpy(&m_pszBuffer[m_dwBufSize - 1], pszStr);
   	m_dwBufSize += dwLen;
	}
   return *this;
}


//
// Add formatted string to the end of buffer
//

void String::addFormattedString(const TCHAR *format, ...)
{
   va_list args;

	va_start(args, format);
	addFormattedStringV(format, args);
	va_end(args);
}

void String::addFormattedStringV(const TCHAR *format, va_list args)
{
   int len;
   TCHAR *buffer;

#ifdef UNICODE

#if HAVE_VASWPRINTF
	vaswprintf(&buffer, format, args);
#elif HAVE_VSCWPRINTF && HAVE_DECL_VA_COPY
	va_list argsCopy;
	va_copy(argsCopy, args);

	len = (int)vscwprintf(format, args) + 1;
   buffer = (WCHAR *)malloc(len * sizeof(WCHAR));

   vsnwprintf(buffer, len, format, argsCopy);
   va_end(argsCopy);
#else
	// No way to determine required buffer size, guess
	len = wcslen(format) + NumCharsW(format, L'%') * 1000 + 1;
   buffer = (WCHAR *)malloc(len * sizeof(WCHAR));

   nx_vswprintf(buffer, len, format, args);
#endif

#else		/* UNICODE */

#if HAVE_VASPRINTF && !defined(_IPSO)
	if (vasprintf(&buffer, format, args) == -1)
	{
		buffer = (char *)malloc(1);
		*buffer = 0;
	}
#elif HAVE_VSCPRINTF && HAVE_DECL_VA_COPY
	va_list argsCopy;
	va_copy(argsCopy, args);

	len = (int)vscprintf(format, args) + 1;
   buffer = (char *)malloc(len);

   vsnprintf(buffer, len, format, argsCopy);
   va_end(argsCopy);
#elif SNPRINTF_RETURNS_REQUIRED_SIZE && HAVE_DECL_VA_COPY
	va_list argsCopy;
	va_copy(argsCopy, args);

	len = (int)vsnprintf(NULL, 0, format, args) + 1;
	buffer = (char *)malloc(len);

	vsnprintf(buffer, len, format, argsCopy);
	va_end(argsCopy);
#else
	// No way to determine required buffer size, guess
	len = strlen(format) + NumChars(format, '%') * 1000 + 1;
   buffer = (char *)malloc(len);

   vsnprintf(buffer, len, format, args);
#endif

#endif	/* UNICODE */

   *this += buffer;
   free(buffer);
}

/**
 * Add string to the end of buffer
 */
void String::addString(const TCHAR *pStr, UINT32 dwSize)
{
   m_pszBuffer = (TCHAR *)realloc(m_pszBuffer, (m_dwBufSize + dwSize) * sizeof(TCHAR));
   memcpy(&m_pszBuffer[m_dwBufSize - 1], pStr, dwSize * sizeof(TCHAR));
   m_dwBufSize += dwSize;
	m_pszBuffer[m_dwBufSize - 1] = 0;
}


//
// Add multibyte string to the end of buffer
//

void String::addMultiByteString(const char *pStr, UINT32 dwSize, int nCodePage)
{
#ifdef UNICODE
   m_pszBuffer = (TCHAR *)realloc(m_pszBuffer, (m_dwBufSize + dwSize) * sizeof(TCHAR));
	m_dwBufSize += MultiByteToWideChar(nCodePage, (nCodePage == CP_UTF8) ? 0 : MB_PRECOMPOSED, pStr, dwSize, &m_pszBuffer[m_dwBufSize - 1], dwSize);
	m_pszBuffer[m_dwBufSize - 1] = 0;
#else
	addString(pStr, dwSize);
#endif
}


//
// Add widechar string to the end of buffer
//

void String::addWideCharString(const WCHAR *pStr, UINT32 dwSize)
{
#ifdef UNICODE
	addString(pStr, dwSize);
#else
   m_pszBuffer = (TCHAR *)realloc(m_pszBuffer, (m_dwBufSize + dwSize) * sizeof(TCHAR));
	WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, pStr, dwSize, &m_pszBuffer[m_dwBufSize - 1], dwSize, NULL, NULL);
   m_dwBufSize += dwSize;
	m_pszBuffer[m_dwBufSize - 1] = 0;
#endif
}

/**
 * Escape given character
 */
void String::escapeCharacter(int ch, int esc)
{
   int nCount;
   UINT32 i;

   if (m_pszBuffer == NULL)
      return;

   nCount = NumChars(m_pszBuffer, ch);
   if (nCount == 0)
      return;

   m_dwBufSize += nCount;
   m_pszBuffer = (TCHAR *)realloc(m_pszBuffer, m_dwBufSize * sizeof(TCHAR));
   for(i = 0; m_pszBuffer[i] != 0; i++)
   {
      if (m_pszBuffer[i] == ch)
      {
         memmove(&m_pszBuffer[i + 1], &m_pszBuffer[i], (m_dwBufSize - i - 1) * sizeof(TCHAR));
         m_pszBuffer[i] = esc;
         i++;
      }
   }
}

/**
 * Set dynamically allocated string as a new buffer
 */
void String::setBuffer(TCHAR *pszBuffer)
{
   safe_free(m_pszBuffer);
   m_pszBuffer = pszBuffer;
   m_dwBufSize = (m_pszBuffer != NULL) ? (UINT32)_tcslen(m_pszBuffer) + 1 : 1;
}

/**
 * Replace all occurences of source substring with destination substring
 */
void String::replace(const TCHAR *pszSrc, const TCHAR *pszDst)
{
   if (m_pszBuffer == NULL)
      return;

   int lenSrc = (int)_tcslen(pszSrc);
   int lenDst = (int)_tcslen(pszDst);

   for(int i = 0; ((int)m_dwBufSize > lenSrc) && (i < (int)m_dwBufSize - lenSrc); i++)
   {
      if (!memcmp(pszSrc, &m_pszBuffer[i], lenSrc * sizeof(TCHAR)))
      {
         if (lenSrc == lenDst)
         {
            memcpy(&m_pszBuffer[i], pszDst, lenDst * sizeof(TCHAR));
            i += lenDst - 1;
         }
         else if (lenSrc > lenDst)
         {
            memcpy(&m_pszBuffer[i], pszDst, lenDst * sizeof(TCHAR));
            i += lenDst;
            int delta = lenSrc - lenDst;
            m_dwBufSize -= (UINT32)delta;
            memmove(&m_pszBuffer[i], &m_pszBuffer[i + delta], (m_dwBufSize - (UINT32)i) * sizeof(TCHAR));
            i--;
         }
         else
         {
            int delta = lenDst - lenSrc;
            m_pszBuffer = (TCHAR *)realloc(m_pszBuffer, (m_dwBufSize + (UINT32)delta) * sizeof(TCHAR));
            memmove(&m_pszBuffer[i + lenDst], &m_pszBuffer[i + lenSrc], ((int)m_dwBufSize - i - lenSrc) * sizeof(TCHAR));
            m_dwBufSize += (UINT32)delta;
            memcpy(&m_pszBuffer[i], pszDst, lenDst * sizeof(TCHAR));
            i += lenDst - 1;
         }
      }
   }
}

/**
 * Extract substring into buffer
 */
TCHAR *String::subStr(int nStart, int nLen, TCHAR *pszBuffer)
{
	int nCount;
	TCHAR *pszOut;

	if ((nStart < (int)m_dwBufSize - 1) && (nStart >= 0))
	{
		if (nLen == -1)
		{
			nCount = (int)m_dwBufSize - nStart - 1;
		}
		else
		{
			nCount = min(nLen, (int)m_dwBufSize - nStart - 1);
		}
		pszOut = (pszBuffer != NULL) ? pszBuffer : (TCHAR *)malloc((nCount + 1) * sizeof(TCHAR));
		memcpy(pszOut, &m_pszBuffer[nStart], nCount * sizeof(TCHAR));
		pszOut[nCount] = 0;
	}
	else
	{
		pszOut = (pszBuffer != NULL) ? pszBuffer : (TCHAR *)malloc(sizeof(TCHAR));
		*pszOut = 0;
	}
	return pszOut;
}


//
// Find substring in a string
//

int String::find(const TCHAR *pszStr, int nStart)
{
	TCHAR *p;

	if ((nStart >= (int)m_dwBufSize - 1) || (nStart < 0))
		return npos;

	p = _tcsstr(&m_pszBuffer[nStart], pszStr);
	return (p != NULL) ? (int)(((char *)p - (char *)m_pszBuffer) / sizeof(TCHAR)) : npos;
}


//
// Strip leading and trailing spaces, tabs, newlines
//

void String::trim()
{
	if (m_pszBuffer != NULL)
	{
		Trim(m_pszBuffer);
		m_dwBufSize = (UINT32)_tcslen(m_pszBuffer) + 1;
	}
}


//
// Shring string by removing trailing characters
//

void String::shrink(int chars)
{
	if (m_dwBufSize > 1)
	{
		m_dwBufSize -= min(m_dwBufSize - 1, (UINT32)chars);
		if (m_pszBuffer != NULL)
			m_pszBuffer[m_dwBufSize - 1] = 0;
	}
}


//
// Get content as dynamically allocated UTF-8 string
//

char *String::getUTF8String()
{
#ifdef UNICODE
	return UTF8StringFromWideString(m_pszBuffer);
#else
	return strdup(m_pszBuffer);
#endif
}
