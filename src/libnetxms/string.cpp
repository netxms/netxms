/* 
** NetXMS - Network Management System
** NetXMS Foundation Library
** Copyright (C) 2003-2010 Victor Kirhenshtein
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


//
// Static members
//

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
   m_dwBufSize = (DWORD)_tcslen(init) + 1;
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
   m_dwBufSize = (DWORD)_tcslen(CHECK_NULL_EX(pszStr)) + 1;
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


//
// Operator +=
//

const String& String::operator +=(const TCHAR *pszStr)
{
   DWORD dwLen;

	if (pszStr != NULL)
	{
   	dwLen = (DWORD)_tcslen(pszStr);
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

   vswprintf(buffer, len, format, args);
#endif

#else		/* UNICODE */

#if HAVE_VASPRINTF && !defined(_IPSO)
	vasprintf(&buffer, format, args);
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


//
// Add string to the end of buffer
//

void String::addString(const TCHAR *pStr, DWORD dwSize)
{
   m_pszBuffer = (TCHAR *)realloc(m_pszBuffer, (m_dwBufSize + dwSize) * sizeof(TCHAR));
   memcpy(&m_pszBuffer[m_dwBufSize - 1], pStr, dwSize * sizeof(TCHAR));
   m_dwBufSize += dwSize;
	m_pszBuffer[m_dwBufSize - 1] = 0;
}


//
// Add multibyte string to the end of buffer
//

void String::addMultiByteString(const char *pStr, DWORD dwSize, int nCodePage)
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

void String::addWideCharString(const WCHAR *pStr, DWORD dwSize)
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


//
// Escape given character
//

void String::escapeCharacter(int ch, int esc)
{
   int nCount;
   DWORD i;

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


//
// Set dynamically allocated string as a new buffer
//

void String::setBuffer(TCHAR *pszBuffer)
{
   safe_free(m_pszBuffer);
   m_pszBuffer = pszBuffer;
   m_dwBufSize = (m_pszBuffer != NULL) ? (DWORD)_tcslen(m_pszBuffer) + 1 : 1;
}


//
// Translate given substring
//

void String::translate(const TCHAR *pszSrc, const TCHAR *pszDst)
{
   DWORD i, dwLenSrc, dwLenDst, dwDelta;

   if (m_pszBuffer == NULL)
      return;

   dwLenSrc = (DWORD)_tcslen(pszSrc);
   dwLenDst = (DWORD)_tcslen(pszDst);

   if (m_dwBufSize <= dwLenSrc)
      return;
   
   for(i = 0; i < m_dwBufSize - dwLenSrc; i++)
   {
      if (!memcmp(pszSrc, &m_pszBuffer[i], dwLenSrc * sizeof(TCHAR)))
      {
         if (dwLenSrc == dwLenDst)
         {
            memcpy(&m_pszBuffer[i], pszDst, dwLenDst * sizeof(TCHAR));
            i += dwLenDst - 1;
         }
         else if (dwLenSrc > dwLenDst)
         {
            memcpy(&m_pszBuffer[i], pszDst, dwLenDst * sizeof(TCHAR));
            i += dwLenDst;
            dwDelta = dwLenSrc - dwLenDst;
            m_dwBufSize -= dwDelta;
            memmove(&m_pszBuffer[i], &m_pszBuffer[i + dwDelta], (m_dwBufSize - i) * sizeof(TCHAR));
            i--;
         }
         else
         {
            dwDelta = dwLenDst - dwLenSrc;
            m_pszBuffer = (TCHAR *)realloc(m_pszBuffer, (m_dwBufSize + dwDelta) * sizeof(TCHAR));
            memmove(&m_pszBuffer[i + dwLenDst], &m_pszBuffer[i + dwLenSrc], (m_dwBufSize - i - dwLenSrc) * sizeof(TCHAR));
            m_dwBufSize += dwDelta;
            memcpy(&m_pszBuffer[i], pszDst, dwLenDst * sizeof(TCHAR));
            i += dwLenDst - 1;
         }
      }
   }
}


//
// Extract substring into buffer
//

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
		m_dwBufSize = (DWORD)_tcslen(m_pszBuffer) + 1;
	}
}


//
// Shring string by removing trailing characters
//

void String::shrink(int chars)
{
	if (m_dwBufSize > 1)
	{
		m_dwBufSize -= min(m_dwBufSize - 1, (DWORD)chars);
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
