/*
** NetXMS - Network Management System
** NetXMS Foundation Library
** Copyright (C) 2003-2020 Victor Kirhenshtein
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
const ssize_t __EXPORT String::npos = -1;
const String __EXPORT String::empty;

/**
 * Create empty string
 */
String::String()
{
   m_internalBuffer[0] = 0;
   m_buffer = m_internalBuffer;
   m_length = 0;
}

/**
 * Copy constructor
 */
String::String(const String &src)
{
   m_length = src.m_length;
   if (m_length < STRING_INTERNAL_BUFFER_SIZE)
   {
      m_buffer = m_internalBuffer;
      memcpy(m_buffer, src.m_buffer, (m_length + 1) * sizeof(TCHAR));
   }
   else
   {
      m_buffer = MemCopyBlock(src.m_buffer, (m_length + 1) * sizeof(TCHAR));
   }
}

/**
 * Create string with given initial content
 */
String::String(const TCHAR *init)
{
   m_length = (init != nullptr) ? _tcslen(init) : 0;
   if (m_length < STRING_INTERNAL_BUFFER_SIZE)
   {
      m_buffer = m_internalBuffer;
      memcpy(m_buffer, CHECK_NULL_EX(init), (m_length + 1) * sizeof(TCHAR));
   }
   else
   {
      m_buffer = MemCopyString(init);
   }
}

/**
 * Create string with given initial content
 */
String::String(const TCHAR *init, ssize_t len, Ownership takeOwnership)
{
   m_length = (init == nullptr) ? 0 : ((len >= 0) ? len : _tcslen(init));
   if (m_length < STRING_INTERNAL_BUFFER_SIZE)
   {
      m_buffer = m_internalBuffer;
      memcpy(m_buffer, init, m_length * sizeof(TCHAR));
      if (takeOwnership == Ownership::True)
         MemFree(const_cast<TCHAR*>(init));
   }
   else if (takeOwnership == Ownership::True)
   {
      m_buffer = const_cast<TCHAR*>(init);
   }
   else
   {
      m_buffer = MemAllocString(m_length + 1);
      memcpy(m_buffer, init, m_length * sizeof(TCHAR));
   }
   m_buffer[m_length] = 0;
}

/**
 * Destructor
 */
String::~String()
{
   if (!isInternalBuffer())
      MemFree(m_buffer);
}

/**
 * Concatenate two strings
 */
String String::operator +(const String &right) const
{
   String result;
   result.m_length = m_length + right.m_length;
   if (result.m_length >= STRING_INTERNAL_BUFFER_SIZE)
   {
      result.m_buffer = MemAllocString(result.m_length + 1);
   }
   memcpy(result.m_buffer, m_buffer, m_length * sizeof(TCHAR));
   memcpy(&result.m_buffer[m_length], right.m_buffer, right.m_length * sizeof(TCHAR));
   result.m_buffer[result.m_length] = 0;
   return result;
}

/**
 * Concatenate two strings
 */
String String::operator +(const TCHAR *right) const
{
   if (right == NULL)
      return String(*this);

   size_t rlength = _tcslen(right);
   String result;
   result.m_length = m_length + rlength;
   if (result.m_length >= STRING_INTERNAL_BUFFER_SIZE)
   {
      result.m_buffer = MemAllocString(result.m_length + 1);
   }
   memcpy(result.m_buffer, m_buffer, m_length * sizeof(TCHAR));
   memcpy(&result.m_buffer[m_length], right, rlength * sizeof(TCHAR));
   result.m_buffer[result.m_length] = 0;
   return result;
}

/**
 * Extract substring into buffer
 */
String String::substring(size_t start, ssize_t len) const
{
   if (start >= m_length)
      return String();

   size_t count;
   if (len == -1)
   {
      count = m_length - start;
   }
   else
   {
      count = std::min(static_cast<size_t>(len), m_length - start);
   }
   return String(&m_buffer[start], count);
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
		s = (buffer != NULL) ? buffer : (TCHAR *)MemAlloc((count + 1) * sizeof(TCHAR));
		memcpy(s, &m_buffer[start], count * sizeof(TCHAR));
		s[count] = 0;
	}
	else
	{
		s = (buffer != NULL) ? buffer : (TCHAR *)MemAlloc(sizeof(TCHAR));
		*s = 0;
	}
	return s;
}

/**
 * Find substring in a string
 */
ssize_t String::find(const TCHAR *str, size_t start) const
{
	if (start >= m_length)
		return npos;

	TCHAR *p = _tcsstr(&m_buffer[start], str);
	return (p != NULL) ? (ssize_t)(((char *)p - (char *)m_buffer) / sizeof(TCHAR)) : npos;
}

/**
 * Get content as dynamically allocated UTF-8 string
 */
char *String::getUTF8String() const
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
   return !_tcscmp(CHECK_NULL_EX(m_buffer), s);
}

/**
 * Check that this string starts with given sub-string
 */
bool String::startsWith(const String& s) const
{
   if (s.m_length > m_length)
      return false;
   if (s.m_length == 0)
      return true;
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
   if (l == 0)
      return true;
   return !memcmp(m_buffer, s, l * sizeof(TCHAR));
}

/**
 * Check that this string ends with given sub-string
 */
bool String::endsWith(const String& s) const
{
   if (s.m_length > m_length)
      return false;
   if (s.m_length == 0)
      return true;
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
   if (l == 0)
      return true;
   return !memcmp(&m_buffer[m_length - l], s, l * sizeof(TCHAR));
}

/**
 * Split string
 */
StringList *String::split(TCHAR *str, size_t len, const TCHAR *separator)
{
   StringList *result = new StringList();

   size_t slen = _tcslen(separator);
   if (slen == 0)
   {
      result->add(CHECK_NULL(str));
      return result;
   }
   if (len < slen)
   {
      result->add(_T(""));
      return result;
   }

   TCHAR *curr = str;
   while(true)
   {
      TCHAR *next = _tcsstr(curr, separator);
      if (next == nullptr)
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

/**
 * Create empty string buffer
 */
StringBuffer::StringBuffer() : String()
{
   m_allocated = 0;
   m_allocationStep = 256;
}

/**
 * Copy constructor
 */
StringBuffer::StringBuffer(const StringBuffer &src) : String()
{
   m_length = src.m_length;
   m_allocationStep = src.m_allocationStep;
   if (m_length < STRING_INTERNAL_BUFFER_SIZE)
   {
      m_allocated = src.m_allocated;
      memcpy(m_buffer, src.m_buffer, (m_length + 1) * sizeof(TCHAR));
   }
   else
   {
      m_allocated = src.m_allocated;
      m_buffer = MemCopyBlock(src.m_buffer, m_allocated * sizeof(TCHAR));
   }
}

/**
 * Copy constructor
 */
StringBuffer::StringBuffer(const String &src) : String(src)
{
   m_allocated = isInternalBuffer() ? 0 : m_length + 1;
   m_allocationStep = 256;
}

/**
 * Copy constructor
 */
StringBuffer::StringBuffer(const SharedString &src) : String(src.str())
{
   m_allocated = isInternalBuffer() ? 0 : m_length + 1;
   m_allocationStep = 256;
}

/**
 * Create string buffer with given initial content
 */
StringBuffer::StringBuffer(const TCHAR *init) : String(init)
{
   m_allocated = isInternalBuffer() ? 0 : m_length + 1;
   m_allocationStep = 256;
}

/**
 * Destructor
 */
StringBuffer::~StringBuffer()
{
}

/**
 * Operator =
 */
StringBuffer& StringBuffer::operator =(const TCHAR *str)
{
   if (!isInternalBuffer())
      MemFree(m_buffer);
   m_length = (str != NULL) ? _tcslen(str) : 0;
   if (m_length < STRING_INTERNAL_BUFFER_SIZE)
   {
      m_allocated = 0;
      m_buffer = m_internalBuffer;
      memcpy(m_buffer, CHECK_NULL_EX(str), (m_length + 1) * sizeof(TCHAR));
   }
   else
   {
      m_buffer = MemCopyString(CHECK_NULL_EX(str));
      m_allocated = m_length + 1;
   }
   return *this;
}

/**
 * Operator =
 */
StringBuffer& StringBuffer::operator =(const StringBuffer &src)
{
   if (&src == this)
      return *this;
   if (!isInternalBuffer())
      MemFree(m_buffer);
   m_length = src.m_length;
   if (m_length < STRING_INTERNAL_BUFFER_SIZE)
   {
      m_allocated = 0;
      m_buffer = m_internalBuffer;
      memcpy(m_buffer, src.m_buffer, (m_length + 1) * sizeof(TCHAR));
   }
   else
   {
      m_allocated = src.m_allocated;
      m_buffer = MemCopyBlock(src.m_buffer, m_allocated * sizeof(TCHAR));
   }
   m_allocationStep = src.m_allocationStep;
   return *this;
}

/**
 * Operator =
 */
StringBuffer& StringBuffer::operator =(const String &src)
{
   if (&src == this)
      return *this;
   if (!isInternalBuffer())
      MemFree(m_buffer);
   m_length = src.length();
   if (m_length < STRING_INTERNAL_BUFFER_SIZE)
   {
      m_allocated = 0;
      m_buffer = m_internalBuffer;
      memcpy(m_buffer, src.cstr(), (m_length + 1) * sizeof(TCHAR));
   }
   else
   {
      m_allocated = m_length + 1;
      m_buffer = MemCopyBlock(src.cstr(), m_allocated * sizeof(TCHAR));
   }
   m_allocationStep = 256;
   return *this;
}

/**
 * Operator =
 */
StringBuffer& StringBuffer::operator =(const SharedString &src)
{
   return operator=(src.str());
}

/**
 * Append formatted string to the end of buffer
 */
void StringBuffer::appendFormattedString(const TCHAR *format, ...)
{
   va_list args;
   va_start(args, format);
   appendFormattedStringV(format, args);
   va_end(args);
}

/**
 * Insert formatted string at given position
 */
void StringBuffer::insertFormattedString(size_t index, const TCHAR *format, ...)
{
   va_list args;
   va_start(args, format);
   insertFormattedStringV(index, format, args);
   va_end(args);
}

/**
 * Insert formatted string at given position
 */
void StringBuffer::insertFormattedStringV(size_t index, const TCHAR *format, va_list args)
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
   buffer = MemAllocStringW(len);

   vsnwprintf(buffer, len, format, argsCopy);
   va_end(argsCopy);
#else
   // No way to determine required buffer size, guess
   len = wcslen(format) + NumCharsW(format, L'%') * 1000 + 1;
   buffer = (WCHAR *)malloc(len * sizeof(WCHAR));

   nx_vswprintf(buffer, len, format, args);
#endif

#else    /* UNICODE */

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

#endif   /* UNICODE */

   insert(index, buffer, _tcslen(buffer));
   free(buffer);
}

/**
 * Append string to the end of buffer
 */
void StringBuffer::insert(size_t index, const TCHAR *str, size_t len)
{
   if (len <= 0)
      return;

   if (isInternalBuffer())
   {
      if (m_length + len >= STRING_INTERNAL_BUFFER_SIZE)
      {
         m_allocated = std::max(m_allocationStep, m_length + len + 1);
         m_buffer = MemAllocString(m_allocated);
         memcpy(m_buffer, m_internalBuffer, m_length * sizeof(TCHAR));
      }
   }
   else
   {
      if (m_length + len >= m_allocated)
      {
         m_allocated += std::max(m_allocationStep, len + 1);
         m_buffer = MemRealloc(m_buffer, m_allocated * sizeof(TCHAR));
      }
   }
   if (index < m_length)
   {
      memmove(&m_buffer[index], &m_buffer[index + len], m_length - len);
      memcpy(&m_buffer[index], str, len * sizeof(TCHAR));
   }
   else
   {
      memcpy(&m_buffer[m_length], str, len * sizeof(TCHAR));
   }
   m_length += len;
   m_buffer[m_length] = 0;
}

/**
 * Insert multibyte string at given position
 */
void StringBuffer::insertMBString(size_t index, const char *str, size_t len, int codePage)
{
#ifdef UNICODE
   if (isInternalBuffer())
   {
      if (m_length + len >= STRING_INTERNAL_BUFFER_SIZE)
      {
         m_allocated = m_length + len + 1;
         m_buffer = MemAllocString(m_allocated);
         memcpy(m_buffer, m_internalBuffer, m_length * sizeof(TCHAR));
      }
   }
   else
   {
      if (m_length + len >= m_allocated)
      {
         m_allocated += std::max(m_allocationStep, len + 1);
         m_buffer = MemRealloc(m_buffer, m_allocated * sizeof(TCHAR));
      }
   }
   if (index < m_length)
   {
      memmove(&m_buffer[index], &m_buffer[index + len], m_length - len);
      int wchars = MultiByteToWideChar(codePage, (codePage == CP_UTF8) ? 0 : MB_PRECOMPOSED, str, (int)len, &m_buffer[index], (int)len + 1);
      if (static_cast<size_t>(wchars) < len)
      {
         memmove(&m_buffer[index + len], &m_buffer[index + wchars], len - wchars);
      }
      m_length += wchars;
   }
   else
   {
      m_length += MultiByteToWideChar(codePage, (codePage == CP_UTF8) ? 0 : MB_PRECOMPOSED, str, (int)len, &m_buffer[m_length], (int)len + 1);
   }
   m_buffer[m_length] = 0;
#else
   insert(index, str, len);
#endif
}

/**
 * Insert wide character string at given position
 */
void StringBuffer::insertWideString(size_t index, const WCHAR *str, size_t len)
{
#ifdef UNICODE
   insert(index, str, len);
#else
   size_t clen = WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, str, len, nullptr, 0, nullptr, nullptr);
   if (isInternalBuffer())
   {
      if (m_length + clen >= STRING_INTERNAL_BUFFER_SIZE)
      {
         m_allocated = m_length + clen + 1;
         m_buffer = MemAllocString(m_allocated);
         memcpy(m_buffer, m_internalBuffer, m_length * sizeof(TCHAR));
      }
   }
   else
   {
      if (m_length + clen >= m_allocated)
      {
         m_allocated += std::max(m_allocationStep, clen + 1);
         m_buffer = MemRealloc(m_buffer, m_allocated * sizeof(TCHAR));
      }
   }
   if (index < m_length)
   {
      memmove(&m_buffer[index], &m_buffer[index + clen], m_length - clen);
      int chars = WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, str, len, &m_buffer[index], clen, nullptr, nullptr);
      if (chars < len)
      {
         memmove(&m_buffer[index + len], &m_buffer[index + chars], clen - chars);
      }
      m_length += chars;
   }
   else
   {
      m_length += WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, str, len, &m_buffer[m_length], len, nullptr, nullptr);
   }
   m_buffer[m_length] = 0;
#endif
}

/**
 * Insert integer
 */
void StringBuffer::insert(size_t index, int32_t n, const TCHAR *format)
{
   TCHAR buffer[64];
   if (format != nullptr)
   {
      _sntprintf(buffer, 64, format, n);
      insert(index, buffer);
   }
   else
   {
      insert(index, _itot(n, buffer, 10));
   }
}

/**
 * Insert integer
 */
void StringBuffer::insert(size_t index, uint32_t n, const TCHAR *format)
{
   TCHAR buffer[64];
   _sntprintf(buffer, 64, (format != nullptr) ? format : _T("%u"), n);
   insert(index, buffer);
}

/**
 * Insert integer
 */
void StringBuffer::insert(size_t index, int64_t n, const TCHAR *format)
{
   TCHAR buffer[64];
   _sntprintf(buffer, 64, (format != nullptr) ? format : INT64_FMT, n);
   insert(index, buffer);
}

/**
 * Insert integer
 */
void StringBuffer::insert(size_t index, uint64_t n, const TCHAR *format)
{
   TCHAR buffer[64];
   _sntprintf(buffer, 64, (format != nullptr) ? format : UINT64_FMT, n);
   insert(index, buffer);
}

/**
 * Insert double
 */
void StringBuffer::insert(size_t index, double d, const TCHAR *format)
{
   TCHAR buffer[64];
   _sntprintf(buffer, 64, (format != nullptr) ? format : _T("%f"), d);
   insert(index, buffer);
}

/**
 * Insert GUID
 */
void StringBuffer::insert(size_t index, const uuid& guid)
{
   TCHAR buffer[64];
   guid.toString(buffer);
   insert(index, buffer, _tcslen(buffer));
}

/**
 * Escape given character
 */
void StringBuffer::escapeCharacter(int ch, int esc)
{
   int nCount = NumChars(m_buffer, ch);
   if (nCount == 0)
      return;

   if (isInternalBuffer())
   {
      if (m_length + nCount >= STRING_INTERNAL_BUFFER_SIZE)
      {
         m_allocated = std::max(m_allocationStep, m_length + nCount + 1);
         m_buffer = MemAllocString(m_allocated);
         memcpy(m_buffer, m_internalBuffer, (m_length + 1) * sizeof(TCHAR));
      }
   }
   else
   {
      if (m_length + nCount >= m_allocated)
      {
         m_allocated += std::max(m_allocationStep, (size_t)nCount);
         m_buffer = MemRealloc(m_buffer, m_allocated * sizeof(TCHAR));
      }
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
void StringBuffer::setBuffer(TCHAR *buffer)
{
   if (!isInternalBuffer())
      MemFree(m_buffer);
   if (buffer != NULL)
   {
      m_buffer = buffer;
      m_length = _tcslen(m_buffer);
      m_allocated = m_length + 1;
   }
   else
   {
      m_buffer = m_internalBuffer;
      m_buffer[0] = 0;
      m_length = 0;
      m_allocated = 0;
   }
}

/**
 * Replace all occurrences of source substring with destination substring
 */
void StringBuffer::replace(const TCHAR *pszSrc, const TCHAR *pszDst)
{
   size_t lenSrc = _tcslen(pszSrc);
   if (lenSrc > m_length)
      return;

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
            if (isInternalBuffer())
            {
               if (m_length + delta >= STRING_INTERNAL_BUFFER_SIZE)
               {
                  m_allocated = std::max(m_allocationStep, m_length + delta + 1);
                  m_buffer = MemAllocString(m_allocated);
                  memcpy(m_buffer, m_internalBuffer, (m_length + 1) * sizeof(TCHAR));
               }
            }
            else
            {
               if (m_length + delta >= m_allocated)
               {
                  m_allocated += std::max(m_allocationStep, delta);
                  m_buffer = (TCHAR *)MemRealloc(m_buffer, m_allocated * sizeof(TCHAR));
               }
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
 * Convert string to uppercase
 */
void StringBuffer::toUppercase()
{
   for (size_t i = 0; i < m_length; i++)
      m_buffer[i] = _totupper(m_buffer[i]);
}

/**
* Convert string to lowercase
*/
void StringBuffer::toLowercase()
{
   for (size_t i = 0; i < m_length; i++)
      m_buffer[i] = _totlower(m_buffer[i]);
}

/**
 * Strip leading and trailing spaces, tabs, newlines
 */
void StringBuffer::trim()
{
   Trim(m_buffer);
   m_length = _tcslen(m_buffer);
}

/**
 * Shrink string by removing trailing characters
 */
void StringBuffer::shrink(size_t chars)
{
   if (m_length > 0)
   {
      m_length -= std::min(m_length, chars);
      m_buffer[m_length] = 0;
   }
}

/**
 * Clear string
 */
void StringBuffer::clear()
{
   if (!isInternalBuffer())
   {
      MemFree(m_buffer);
      m_buffer = m_internalBuffer;
   }
   m_length = 0;
   m_buffer[m_length] = 0;
}

/**
 * Operator = for mutable string
 */
MutableString& MutableString::operator =(const String &src)
{
   if (&src == this)
      return *this;
   if (!isInternalBuffer())
      MemFree(m_buffer);
   m_length = src.length();
   if (m_length < STRING_INTERNAL_BUFFER_SIZE)
   {
      m_buffer = m_internalBuffer;
      memcpy(m_buffer, src.cstr(), (m_length + 1) * sizeof(TCHAR));
   }
   else
   {
      m_buffer = MemCopyString(src.cstr());
   }
   return *this;
}

/**
 * Operator = for mutable string
 */
MutableString& MutableString::operator =(const TCHAR *src)
{
   if (!isInternalBuffer())
      MemFree(m_buffer);
   m_length = _tcslen(src);
   if (m_length < STRING_INTERNAL_BUFFER_SIZE)
   {
      m_buffer = m_internalBuffer;
      memcpy(m_buffer, src, (m_length + 1) * sizeof(TCHAR));
   }
   else
   {
      m_buffer = MemCopyString(src);
   }
   return *this;
}
