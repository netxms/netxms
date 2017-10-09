/*
** NetXMS - Network Management System
** NetXMS Foundation Library
** Copyright (C) 2003-2017 Victor Kirhenshtein
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

/**
 * Create empty string
 */
String::String()
{
   m_buffer = NULL;
   m_length = 0;
   m_allocated = 0;
   m_allocationStep = 256;
}

/**
 * Copy constructor
 */
String::String(const String &src)
{
   m_length = src.m_length;
   m_allocated = src.m_length + 1;
   m_allocationStep = src.m_allocationStep;
	m_buffer = ((src.m_buffer != NULL) && (src.m_length > 0)) ? (TCHAR *)nx_memdup(src.m_buffer, m_allocated * sizeof(TCHAR)) : NULL;
}

/**
 * Create string with given initial content
 */
String::String(const TCHAR *init)
{
   m_buffer = _tcsdup(init);
   m_length = _tcslen(init);
   m_allocated = m_length + 1;
   m_allocationStep = 256;
}

/**
 * Destructor
 */
String::~String()
{
   safe_free(m_buffer);
}

/**
 * Operator =
 */
String& String::operator =(const TCHAR *str)
{
   free(m_buffer);
   m_buffer = _tcsdup(CHECK_NULL_EX(str));
   m_length = _tcslen(CHECK_NULL_EX(str));
   m_allocated = m_length + 1;
   return *this;
}

/**
 * Operator =
 */
String& String::operator =(const String &src)
{
	if (&src == this)
		return *this;
   free(m_buffer);
	m_length = src.m_length;
   m_allocated = src.m_length + 1;
   m_allocationStep = src.m_allocationStep;
	m_buffer = ((src.m_buffer != NULL) && (src.m_length > 0)) ? (TCHAR *)nx_memdup(src.m_buffer, m_allocated * sizeof(TCHAR)) : NULL;
   return *this;
}

/**
 * Append given string to the end
 */
String& String::operator +=(const TCHAR *str)
{
	if (str != NULL)
	{
   	size_t len = _tcslen(str);
      if (m_length + len >= m_allocated)
      {
         m_allocated += std::max(m_allocationStep, len + 1);
      	m_buffer = (TCHAR *)realloc(m_buffer, m_allocated * sizeof(TCHAR));
      }
   	_tcscpy(&m_buffer[m_length], str);
   	m_length += len;
	}
   return *this;
}

/**
 * Append given string to the end
 */
String& String::operator +=(const String &str)
{
   if (str.m_length > 0)
   {
      if (m_length + str.m_length >= m_allocated)
      {
         m_allocated += std::max(m_allocationStep, str.m_length + 1);
      	m_buffer = (TCHAR *)realloc(m_buffer, m_allocated * sizeof(TCHAR));
      }
      memcpy(&m_buffer[m_length], str.m_buffer, (str.m_length + 1) * sizeof(TCHAR));
	   m_length += str.m_length;
   }
   return *this;
}

/**
 * Concatenate two strings
 */
String String::operator +(const String &right) const
{
   String result(*this);
   result.append(right);
   return result;
}

/**
 * Concatenate two strings
 */
String String::operator +(const TCHAR *right) const
{
   String result(*this);
   result.append(right);
   return result;
}

/**
 * Add formatted string to the end of buffer
 */
void String::appendFormattedString(const TCHAR *format, ...)
{
   va_list args;

	va_start(args, format);
	appendFormattedStringV(format, args);
	va_end(args);
}

void String::appendFormattedStringV(const TCHAR *format, va_list args)
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

   append(buffer, _tcslen(buffer));
   free(buffer);
}

/**
 * Append string to the end of buffer
 */
void String::append(const TCHAR *str, size_t len)
{
   if (len <= 0)
      return;

   if (m_length + len >= m_allocated)
   {
      m_allocated += std::max(m_allocationStep, len + 1);
   	m_buffer = (TCHAR *)realloc(m_buffer, m_allocated * sizeof(TCHAR));
   }
   memcpy(&m_buffer[m_length], str, len * sizeof(TCHAR));
   m_length += len;
   m_buffer[m_length] = 0;
}

/**
 * Append integer
 */
void String::append(INT32 n)
{
   TCHAR buffer[64];
   append(_itot(n, buffer, 10));
}

/**
 * Append integer
 */
void String::append(UINT32 n)
{
   TCHAR buffer[64];
   _sntprintf(buffer, 64, _T("%u"), n);
   append(buffer);
}

/**
 * Append integer
 */
void String::append(INT64 n)
{
   TCHAR buffer[64];
   _sntprintf(buffer, 64, INT64_FMT, n);
   append(buffer);
}

/**
 * Append integer
 */
void String::append(UINT64 n)
{
   TCHAR buffer[64];
   _sntprintf(buffer, 64, UINT64_FMT, n);
   append(buffer);
}


/**
 * Append multibyte string to the end of buffer
 */
void String::appendMBString(const char *str, size_t len, int nCodePage)
{
#ifdef UNICODE
   if (m_length + len >= m_allocated)
   {
      m_allocated += std::max(m_allocationStep, len + 1);
   	m_buffer = (TCHAR *)realloc(m_buffer, m_allocated * sizeof(TCHAR));
   }
	m_length += MultiByteToWideChar(nCodePage, (nCodePage == CP_UTF8) ? 0 : MB_PRECOMPOSED, str, (int)len, &m_buffer[m_length], (int)len);
	m_buffer[m_length] = 0;
#else
	append(str, len);
#endif
}

/**
 * Append widechar string to the end of buffer
 */
void String::appendWideString(const WCHAR *str, size_t len)
{
#ifdef UNICODE
	append(str, len);
#else
   if (m_length + len >= m_allocated)
   {
      m_allocated += std::max(m_allocationStep, len + 1);
   	m_buffer = (TCHAR *)realloc(m_buffer, m_allocated * sizeof(TCHAR));
   }
	WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, str, len, &m_buffer[m_length], len, NULL, NULL);
   m_length += len;
	m_buffer[m_length] = 0;
#endif
}

/**
 * Escape given character
 */
void String::escapeCharacter(int ch, int esc)
{
   if (m_buffer == NULL)
      return;

   int nCount = NumChars(m_buffer, ch);
   if (nCount == 0)
      return;

   if (m_length + nCount >= m_allocated)
   {
      m_allocated += std::max(m_allocationStep, (size_t)nCount);
   	m_buffer = (TCHAR *)realloc(m_buffer, m_allocated * sizeof(TCHAR));
   }

   m_length += nCount;
   for(int i = 0; m_buffer[i] != 0; i++)
   {
      if (m_buffer[i] == ch)
      {
         memmove(&m_buffer[i + 1], &m_buffer[i], (m_length - i) * sizeof(TCHAR));
         m_buffer[i] = esc;
         i++;
      }
   }
   m_buffer[m_length] = 0;
}

/**
 * Set dynamically allocated string as a new buffer
 */
void String::setBuffer(TCHAR *buffer)
{
   safe_free(m_buffer);
   m_buffer = buffer;
   if (m_buffer != NULL)
   {
      m_length = _tcslen(m_buffer);
      m_allocated = m_length + 1;
   }
   else
   {
      m_length = 0;
      m_allocated = 0;
   }
}

/**
 * Replace all occurences of source substring with destination substring
 */
void String::replace(const TCHAR *pszSrc, const TCHAR *pszDst)
{
   if (m_buffer == NULL)
      return;

   size_t lenSrc = _tcslen(pszSrc);
   size_t lenDst = _tcslen(pszDst);

   for(size_t i = 0; (m_length >= lenSrc) && (i <= m_length - lenSrc); i++)
   {
      if (!memcmp(pszSrc, &m_buffer[i], lenSrc * sizeof(TCHAR)))
      {
         if (lenSrc == lenDst)
         {
            memcpy(&m_buffer[i], pszDst, lenDst * sizeof(TCHAR));
            i += lenDst - 1;
         }
         else if (lenSrc > lenDst)
         {
            memcpy(&m_buffer[i], pszDst, lenDst * sizeof(TCHAR));
            i += lenDst;
            size_t delta = lenSrc - lenDst;
            m_length -= delta;
            memmove(&m_buffer[i], &m_buffer[i + delta], (m_length - i + 1) * sizeof(TCHAR));
            i--;
         }
         else
         {
            size_t delta = lenDst - lenSrc;
            if (m_length + delta >= m_allocated)
            {
               m_allocated += std::max(m_allocationStep, delta);
               m_buffer = (TCHAR *)realloc(m_buffer, m_allocated * sizeof(TCHAR));
            }
            memmove(&m_buffer[i + lenDst], &m_buffer[i + lenSrc], (m_length - i - lenSrc + 1) * sizeof(TCHAR));
            m_length += delta;
            memcpy(&m_buffer[i], pszDst, lenDst * sizeof(TCHAR));
            i += lenDst - 1;
         }
      }
   }
}

/**
 * Extract substring into buffer
 */
String String::substring(size_t start, ssize_t len) const
{
   String s;
   if (start < m_length)
   {
      size_t count;
      if (len == -1)
      {
         count = m_length - start;
      }
      else
      {
         count = std::min(static_cast<size_t>(len), m_length - start);
      }
      s.append(&m_buffer[start], count);
   }
   return s;
}

/**
 * Extract substring into buffer
 */
TCHAR *String::substring(size_t start, ssize_t len, TCHAR *buffer) const
{
	TCHAR *s;
	if (start < m_length)
	{
	   size_t count;
		if (len == -1)
		{
			count = m_length - start;
		}
		else
		{
			count = std::min(static_cast<size_t>(len), m_length - start);
		}
		s = (buffer != NULL) ? buffer : (TCHAR *)malloc((count + 1) * sizeof(TCHAR));
		memcpy(s, &m_buffer[start], count * sizeof(TCHAR));
		s[count] = 0;
	}
	else
	{
		s = (buffer != NULL) ? buffer : (TCHAR *)malloc(sizeof(TCHAR));
		*s = 0;
	}
	return s;
}

/**
 * Find substring in a string
 */
int String::find(const TCHAR *str, size_t start) const
{
	if (start >= m_length)
		return npos;

	TCHAR *p = _tcsstr(&m_buffer[start], str);
	return (p != NULL) ? (int)(((char *)p - (char *)m_buffer) / sizeof(TCHAR)) : npos;
}

/**
 * Strip leading and trailing spaces, tabs, newlines
 */
void String::trim()
{
	if (m_buffer != NULL)
	{
		Trim(m_buffer);
		m_length = _tcslen(m_buffer);
	}
}

/**
 * Shring string by removing trailing characters
 */
void String::shrink(size_t chars)
{
	if (m_length > 0)
	{
		m_length -= std::min(m_length, chars);
		if (m_buffer != NULL)
			m_buffer[m_length] = 0;
	}
}

/**
 * Clear string
 */
void String::clear()
{
   m_length = 0;
   if (m_buffer != NULL)
      m_buffer[m_length] = 0;
}

/**
 * Get content as dynamically allocated UTF-8 string
 */
char *String::getUTF8String()
{
#ifdef UNICODE
	return UTF8StringFromWideString(m_buffer);
#else
	return UTF8StringFromMBString(m_buffer);
#endif
}

/**
 * Check that two strings are equal
 */
bool String::equals(const String& s) const
{
   if (m_length != s.m_length)
      return false;
   return !memcmp(m_buffer, s.m_buffer, m_length * sizeof(TCHAR));
}

/**
 * Check that two strings are equal
 */
bool String::equals(const TCHAR *s) const
{
   if (s == NULL)
      return false;
   return !_tcscmp(m_buffer, s);
}

/**
 * Check that this string starts with given sub-string
 */
bool String::startsWith(const String& s) const
{
   if (s.m_length > m_length)
      return false;
   return !memcmp(m_buffer, s.m_buffer, s.m_length * sizeof(TCHAR));
}

/**
 * Check that this string starts with given sub-string
 */
bool String::startsWith(const TCHAR *s) const
{
   if (s == NULL)
      return false;
   size_t l = _tcslen(s);
   if (l > m_length)
      return false;
   return !memcmp(m_buffer, s, l * sizeof(TCHAR));
}

/**
 * Check that this string ends with given sub-string
 */
bool String::endsWith(const String& s) const
{
   if (s.m_length > m_length)
      return false;
   return !memcmp(&m_buffer[m_length - s.m_length], s.m_buffer, s.m_length * sizeof(TCHAR));
}

/**
 * Check that this string ends with given sub-string
 */
bool String::endsWith(const TCHAR *s) const
{
   if (s == NULL)
      return false;
   size_t l = _tcslen(s);
   if (l > m_length)
      return false;
   return !memcmp(&m_buffer[m_length - l], s, l * sizeof(TCHAR));
}

/**
 * Split string
 */
StringList *String::split(const TCHAR *separator) const
{
   StringList *result = new StringList();

   size_t slen = _tcslen(separator);
   if (slen == 0)
   {
      result->add(CHECK_NULL(m_buffer));
      return result;
   }
   if (m_length < slen)
   {
      result->add(_T(""));
      return result;
   }

   TCHAR *curr = m_buffer;
   while(true)
   {
      TCHAR *next = _tcsstr(curr, separator);
      if (next == NULL)
      {
         result->add(curr);
         break;
      }

      *next = 0;
      result->add(curr);
      *next = *separator;
      curr = next + slen;
   }

   return result;
}
