/*
** NetXMS - Network Management System
** NetXMS Foundation Library
** Copyright (C) 2003-2022 Victor Kirhenshtein
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
String::String(const String& src)
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
 * Move constructor
 */
String::String(String&& src)
{
   m_length = src.m_length;
   if (m_length < STRING_INTERNAL_BUFFER_SIZE)
   {
      m_buffer = m_internalBuffer;
      memcpy(m_buffer, src.m_buffer, (m_length + 1) * sizeof(TCHAR));
   }
   else
   {
      m_buffer = src.m_buffer;
      src.m_buffer = src.m_internalBuffer;
   }
   src.m_length = 0;
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
 * Create string with given initial content
 */
String::String(const char *init, const char *codepage)
{
   if ((init == nullptr) || (init[0] == 0))
   {
      m_internalBuffer[0] = 0;
      m_buffer = m_internalBuffer;
      m_length = 0;
      return;
   }

   size_t len = strlen(init);
   m_buffer = (len < STRING_INTERNAL_BUFFER_SIZE) ? m_internalBuffer : MemAllocString(len + 1);
#ifdef UNICODE
   m_length = mbcp_to_wchar(init, len, m_buffer, len + 1, codepage);
#else
   if ((codepage != nullptr) && (!stricmp(codepage, "UTF8") || !stricmp(codepage, "UTF-8")))
   {
      m_length = utf8_to_mb(init, len, m_buffer, len + 1);
   }
   else
   {
      memcpy(m_buffer, init, len + 1);
      m_length = len;
   }
#endif
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
		s = (buffer != nullptr) ? buffer : MemAllocString(count + 1);
		memcpy(s, &m_buffer[start], count * sizeof(TCHAR));
		s[count] = 0;
	}
	else
	{
		s = (buffer != nullptr) ? buffer : MemAllocString(1);
		*s = 0;
	}
	return s;
}

/**
 * Check if string is blank
 */
bool String::isBlank() const
{
   for(size_t i = 0; i < m_length; i++)
      if (!_istspace(m_buffer[i]))
         return false;
   return true;
}

/**
 * Find substring in a string
 */
ssize_t String::find(const TCHAR *str, size_t start) const
{
	if (start >= m_length)
		return npos;

	TCHAR *p = _tcsstr(&m_buffer[start], str);
	return (p != nullptr) ? (ssize_t)(((char *)p - (char *)m_buffer) / sizeof(TCHAR)) : npos;
}

/**
 * Find substring in a string ignoring the case of both
 */
ssize_t String::findIgnoreCase(const TCHAR *str, size_t start) const
{
   if (start >= m_length)
      return npos;

   TCHAR *p = _tcsistr(&m_buffer[start], str);
   return (p != nullptr) ? (ssize_t)(((char *)p - (char *)m_buffer) / sizeof(TCHAR)) : npos;
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
   if (s == nullptr)
      return false;
   return !_tcscmp(CHECK_NULL_EX(m_buffer), s);
}

/**
 * Check that two strings are equal ignoring the case of both
 */
bool String::equalsIgnoreCase(const String& s) const
{
   if (m_length != s.m_length)
      return false;
   return !_tcsicmp(m_buffer, s.m_buffer);
}

/**
 * Check that two strings are equal ignoring the case of both
 */
bool String::equalsIgnoreCase(const TCHAR *s) const
{
   if (s == nullptr)
      return false;
   return !_tcsicmp(CHECK_NULL_EX(m_buffer), s);
}

/**
 * Check that two strings are equal using fuzzy matching (Levenshtein distance)
 * @param s String to compare with
 * @param threshold Maximum allowed edit distance (0.0 = exact match, 1.0 = completely different)
 * @return true if strings are similar within the threshold
 */
bool String::fuzzyEqualsImpl(const String& s, double threshold, bool ignoreCase) const
{
   if (threshold >= 1.0)
      return equals(s);

   if (threshold <= 0.0)
      return true;

   size_t maxLen = std::max(m_length, s.m_length);
   if (maxLen == 0)
      return true;  // Both strings are empty

   size_t editDistance = CalculateLevenshteinDistance(m_buffer, m_length, s.m_buffer, s.m_length, ignoreCase);
   double similarity = 1.0 - (static_cast<double>(editDistance) / maxLen);
   return similarity >= threshold;
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
   if (s == nullptr)
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
   if (s == nullptr)
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
StringList String::split(TCHAR *str, size_t len, const TCHAR *separator, bool trim)
{
   StringList result;

   size_t slen = _tcslen(separator);
   if (slen == 0)
   {
      if (trim)
         result.addPreallocated(Trim(MemCopyString(CHECK_NULL_EX(str))));
      else
         result.add(CHECK_NULL_EX(str));
      return result;
   }
   if (len < slen)
   {
      result.add(_T(""));
      return result;
   }

   TCHAR *curr = str;
   while(true)
   {
      TCHAR *next = _tcsstr(curr, separator);
      if (next == nullptr)
      {
         if (trim)
            result.addPreallocated(Trim(MemCopyString(curr)));
         else
            result.add(curr);
         break;
      }

      *next = 0;
      if (trim)
         result.addPreallocated(Trim(MemCopyString(curr)));
      else
         result.add(curr);
      *next = *separator;
      curr = next + slen;
   }

   return result;
}

/**
 * Split string and call callback for each part
 */
void String::split(const TCHAR *str, size_t len, const TCHAR *separator, bool trim, std::function<void (const String&)> callback)
{
   size_t slen = _tcslen(separator);
   if (slen == 0)
   {
      StringBuffer s(str, len);
      if (trim)
         s.trim();
      callback(s);
      return;
   }

   if (len < slen)
      return;

   const TCHAR *curr = str;
   while(true)
   {
      const TCHAR *next = _tcsstr(curr, separator);
      if (next == nullptr)
      {
         StringBuffer s(curr);
         if (trim)
            s.trim();
         callback(s);
         break;
      }

      StringBuffer s(curr, next - curr);
      if (trim)
         s.trim();
      callback(s);

      curr = next + slen;
   }
}

/**
 * Convert 32 bit signed integer to string representation
 */
String String::toString(int32_t v, const TCHAR *format)
{
   String s;
   s.m_length = _sntprintf(s.m_buffer, STRING_INTERNAL_BUFFER_SIZE, (format != nullptr) ? format : _T("%d"), v);
   return s;
}

/**
 * Convert 32 bit unsigned integer to string representation
 */
String String::toString(uint32_t v, const TCHAR *format)
{
   String s;
   s.m_length = _sntprintf(s.m_buffer, STRING_INTERNAL_BUFFER_SIZE, (format != nullptr) ? format : _T("%u"), v);
   return s;
}

/**
 * Convert 64 bit signed integer to string representation
 */
String String::toString(int64_t v, const TCHAR *format)
{
   String s;
   s.m_length = _sntprintf(s.m_buffer, STRING_INTERNAL_BUFFER_SIZE, (format != nullptr) ? format : INT64_FMT, v);
   return s;
}

/**
 * Convert 64 bit unsigned integer to string representation
 */
String String::toString(uint64_t v, const TCHAR *format)
{
   String s;
   s.m_length = _sntprintf(s.m_buffer, STRING_INTERNAL_BUFFER_SIZE, (format != nullptr) ? format : UINT64_FMT, v);
   return s;
}

/**
 * Convert double to string representation
 */
String String::toString(double v, const TCHAR *format)
{
   String s;
   s.m_length = _sntprintf(s.m_buffer, STRING_INTERNAL_BUFFER_SIZE, (format != nullptr) ? format : _T("%f"), v);
   return s;
}

/**
 * Convert byte array to string representation
 */
String String::toString(const BYTE *v, size_t len)
{
   String s;
   s.m_length = len * 2;
   if (s.m_length >= STRING_INTERNAL_BUFFER_SIZE)
   {
      s.m_buffer = MemAllocString(s.m_length + 1);
   }
   BinToStr(v, len, s.m_buffer);
   return s;
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
StringBuffer::StringBuffer(const StringBuffer& src) : String()
{
   m_length = src.m_length;
   m_allocationStep = src.m_allocationStep;
   m_allocated = src.m_allocated;
   if (m_length < STRING_INTERNAL_BUFFER_SIZE)
   {
      memcpy(m_buffer, src.m_buffer, (m_length + 1) * sizeof(TCHAR));
   }
   else
   {
      m_buffer = MemCopyBlock(src.m_buffer, m_allocated * sizeof(TCHAR));
   }
}

/**
 * Move constructor
 */
StringBuffer::StringBuffer(StringBuffer&& src) : String()
{
   m_length = src.m_length;
   m_allocationStep = src.m_allocationStep;
   m_allocated = src.m_allocated;
   if (m_length < STRING_INTERNAL_BUFFER_SIZE)
   {
      memcpy(m_buffer, src.m_buffer, (m_length + 1) * sizeof(TCHAR));
      if (src.m_buffer != src.m_internalBuffer)
      {
         MemFree(src.m_buffer);
         src.m_buffer = src.m_internalBuffer;
      }
   }
   else
   {
      m_buffer = src.m_buffer;
      src.m_buffer = src.m_internalBuffer;
   }
   src.m_allocated = 0;
   src.m_length = 0;
}

/**
 * Copy constructor
 */
StringBuffer::StringBuffer(const String& src) : String(src)
{
   m_allocated = isInternalBuffer() ? 0 : m_length + 1;
   m_allocationStep = 256;
}

/**
 * Copy constructor
 */
StringBuffer::StringBuffer(const SharedString& src) : String(src.str())
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
 * Create string buffer with given initial content
 */
StringBuffer::StringBuffer(const TCHAR *init, size_t length) : String(init, length)
{
   m_allocated = isInternalBuffer() ? 0 : m_length + 1;
   m_allocationStep = 256;
}

/**
 * Create string buffer with given initial content
 */
StringBuffer::StringBuffer(const char *init, const char *codepage) : String(init, codepage)
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
   m_length = (str != nullptr) ? _tcslen(str) : 0;
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
StringBuffer& StringBuffer::appendFormattedString(const TCHAR *format, ...)
{
   va_list args;
   va_start(args, format);
   appendFormattedStringV(format, args);
   va_end(args);
   return *this;
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
   TCHAR *buffer;

#ifdef UNICODE

#if HAVE_VASWPRINTF
   vaswprintf(&buffer, format, args);
#elif HAVE_VSCWPRINTF && HAVE_DECL_VA_COPY
   va_list argsCopy;
   va_copy(argsCopy, args);

   auto len = vscwprintf(format, args) + 1;
   buffer = MemAllocStringW(len);

   vsnwprintf(buffer, len, format, argsCopy);
   va_end(argsCopy);
#else
   // No way to determine required buffer size, guess
   size_t len = wcslen(format) + NumCharsW(format, L'%') * 1000 + 1;
   buffer = MemAllocStringW(len);

   nx_vswprintf(buffer, len, format, args);
#endif

#else    /* UNICODE */

#if HAVE_VASPRINTF && !defined(_IPSO)
   if (vasprintf(&buffer, format, args) == -1)
   {
      buffer = MemAllocStringA(1);
      *buffer = 0;
   }
#elif HAVE_VSCPRINTF && HAVE_DECL_VA_COPY
   va_list argsCopy;
   va_copy(argsCopy, args);

   auto len = vscprintf(format, args) + 1;
   buffer = MemAllocStringA(len);

   vsnprintf(buffer, len, format, argsCopy);
   va_end(argsCopy);
#elif SNPRINTF_RETURNS_REQUIRED_SIZE && HAVE_DECL_VA_COPY
   va_list argsCopy;
   va_copy(argsCopy, args);

   auto len = vsnprintf(nullptr, 0, format, args) + 1;
   buffer = MemAllocStringA(len);

   vsnprintf(buffer, len, format, argsCopy);
   va_end(argsCopy);
#else
   // No way to determine required buffer size, guess
   size_t len = strlen(format) + NumChars(format, '%') * 1000 + 1;
   buffer = MemAllocStringA(len);

   vsnprintf(buffer, len, format, args);
#endif

#endif   /* UNICODE */

   insert(index, buffer, _tcslen(buffer));
   MemFree(buffer);
}

/**
 * Insert placeholder into buffer (expand buffer as needed but do not copy anything into newly allocated space and do not update string length)
 */
void StringBuffer::insertPlaceholder(size_t index, size_t len)
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
      memmove(&m_buffer[index + len], &m_buffer[index], (m_length - index) * sizeof(TCHAR));
   }
}

/**
 * Insert string into buffer
 */
void StringBuffer::insert(size_t index, const TCHAR *str, size_t len)
{
   insertPlaceholder(index, len);
   memcpy(&m_buffer[index], str, len * sizeof(TCHAR));
   m_length += len;
   m_buffer[m_length] = 0;
}

/**
 * Insert multibyte string at given position
 */
void StringBuffer::insertMBString(size_t index, const char *str, ssize_t len)
{
   if (len == -1)
      len = strlen(str);
#ifdef UNICODE
   insertPlaceholder(index, len);
   if (index < m_length)
   {
      size_t wchars = mb_to_wchar(str, len, &m_buffer[index], len + 1);
      if (static_cast<ssize_t>(wchars) < len)
      {
         memmove(&m_buffer[index + len], &m_buffer[index + wchars], (len - wchars) * sizeof(WCHAR));
      }
      m_length += wchars;
   }
   else
   {
      m_length += mb_to_wchar(str, len, &m_buffer[m_length], len + 1);
   }
   m_buffer[m_length] = 0;
#else
   insert(index, str, len);
#endif
}

/**
 * Insert UTF-8 string at given position
 */
void StringBuffer::insertUtf8String(size_t index, const char *str, ssize_t len)
{
   if (len == -1)
      len = strlen(str);
   insertPlaceholder(index, len);
   if (index < m_length)
   {
#ifdef UNICODE
      size_t wchars = utf8_to_wchar(str, len, &m_buffer[index], len + 1);
#else
      size_t wchars = utf8_to_mb(str, len, &m_buffer[index], len + 1);
#endif
      if (static_cast<ssize_t>(wchars) < len)
      {
         memmove(&m_buffer[index + len], &m_buffer[index + wchars], (len - wchars) * sizeof(TCHAR));
      }
      m_length += wchars;
   }
   else
   {
#ifdef UNICODE
      m_length += utf8_to_wchar(str, len, &m_buffer[m_length], len + 1);
#else
      m_length += utf8_to_mb(str, len, &m_buffer[m_length], len + 1);
#endif
   }
   m_buffer[m_length] = 0;
}

/**
 * Insert wide character string at given position
 */
void StringBuffer::insertWideString(size_t index, const WCHAR *str, ssize_t len)
{
   if (len == -1)
      len = wcslen(str);
#ifdef UNICODE
   insert(index, str, len);
#else
   size_t clen = wchar_to_mb(str, len, nullptr, 0);
   insertPlaceholder(index, clen);
   if (index < m_length)
   {
      int chars = wchar_to_mb(str, len, &m_buffer[index], clen);
      if (chars < len)
      {
         memmove(&m_buffer[index + len], &m_buffer[index + chars], clen - chars);
      }
      m_length += chars;
   }
   else
   {
      m_length += wchar_to_mb(str, len, &m_buffer[m_length], len);
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
      insert(index, IntegerToString(n, buffer));
   }
}

/**
 * Insert integer
 */
void StringBuffer::insert(size_t index, uint32_t n, const TCHAR *format)
{
   TCHAR buffer[64];
   if (format != nullptr)
   {
      _sntprintf(buffer, 64, format, n);
      insert(index, buffer);
   }
   else
   {
      insert(index, IntegerToString(n, buffer));
   }
}

/**
 * Insert integer
 */
void StringBuffer::insert(size_t index, int64_t n, const TCHAR *format)
{
   TCHAR buffer[64];
   if (format != nullptr)
   {
      _sntprintf(buffer, 64, format, n);
      insert(index, buffer);
   }
   else
   {
      insert(index, IntegerToString(n, buffer));
   }
}

/**
 * Insert integer
 */
void StringBuffer::insert(size_t index, uint64_t n, const TCHAR *format)
{
   TCHAR buffer[64];
   if (format != nullptr)
   {
      _sntprintf(buffer, 64, format, n);
      insert(index, buffer);
   }
   else
   {
      insert(index, IntegerToString(n, buffer));
   }
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
 * Insert binary data as hex string
 */
void StringBuffer::insertAsHexString(size_t index, const void *data, size_t len, TCHAR separator)
{
   if (len == 0)
      return;

   size_t chars = (separator != 0) ? (len * 3 - 1) : len * 2;
   insertPlaceholder(index, chars);

   const BYTE *in = static_cast<const BYTE*>(data);
   TCHAR *out = &m_buffer[index];
   for(size_t i = 0; i < len - 1; i++, in++)
   {
      *out++ = bin2hex(*in >> 4);
      *out++ = bin2hex(*in & 15);
      if (separator != 0)
         *out++ = separator;
   }
   *out++ = bin2hex(*in >> 4);
   *out++ = bin2hex(*in & 15);

   m_length += chars;
   m_buffer[m_length] = 0;
}

/**
 * Escape given character
 */
StringBuffer& StringBuffer::escapeCharacter(int ch, int esc)
{
   int nCount = NumChars(m_buffer, ch);
   if (nCount == 0)
      return *this;

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
      if (m_buffer[i] == static_cast<TCHAR>(ch))
      {
         memmove(&m_buffer[i + 1], &m_buffer[i], (m_length - i) * sizeof(TCHAR));
         m_buffer[i] = esc;
         i++;
      }
   }
   m_buffer[m_length] = 0;
   return *this;
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
StringBuffer& StringBuffer::replace(const TCHAR *src, const TCHAR *dst)
{
   size_t lenSrc = _tcslen(src);
   if ((lenSrc > m_length) || (lenSrc == 0))
      return *this;

   size_t lenDst = _tcslen(dst);

   for(size_t i = 0; (m_length >= lenSrc) && (i <= m_length - lenSrc); i++)
   {
      if (!memcmp(src, &m_buffer[i], lenSrc * sizeof(TCHAR)))
      {
         if (lenSrc == lenDst)
         {
            memcpy(&m_buffer[i], dst, lenDst * sizeof(TCHAR));
            i += lenDst - 1;
         }
         else if (lenSrc > lenDst)
         {
            memcpy(&m_buffer[i], dst, lenDst * sizeof(TCHAR));
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
            memcpy(&m_buffer[i], dst, lenDst * sizeof(TCHAR));
            i += lenDst - 1;
         }
      }
   }
   return *this;
}

/**
 * Remove given range from string buffer
 */
StringBuffer& StringBuffer::removeRange(size_t start, ssize_t len)
{
   if (start >= m_length)
      return *this;

   len = (len == -1) ? m_length - start : std::min(m_length - start, static_cast<size_t>(len));
   memmove(&m_buffer[start], &m_buffer[start + len], (m_length - start - len + 1) * sizeof(TCHAR));
   m_length -= len;
   return *this;
}

/**
 * Convert string to uppercase
 */
StringBuffer& StringBuffer::toUppercase()
{
   for (size_t i = 0; i < m_length; i++)
      m_buffer[i] = _totupper(m_buffer[i]);
   return *this;
}

/**
* Convert string to lowercase
*/
StringBuffer& StringBuffer::toLowercase()
{
   for (size_t i = 0; i < m_length; i++)
      m_buffer[i] = _totlower(m_buffer[i]);
   return *this;
}

/**
 * Strip leading and trailing spaces, tabs, newlines
 */
StringBuffer& StringBuffer::trim()
{
   Trim(m_buffer);
   m_length = _tcslen(m_buffer);
   return *this;
}

/**
 * Shrink string by removing trailing characters
 */
StringBuffer& StringBuffer::shrink(size_t chars)
{
   if (m_length > 0)
   {
      m_length -= std::min(m_length, chars);
      m_buffer[m_length] = 0;
   }
   return *this;
}

/**
 * Clear string
 */
void StringBuffer::clear(bool releaseBuffer)
{
   if (releaseBuffer && !isInternalBuffer())
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
MutableString& MutableString::operator =(const MutableString &src)
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
   if (src != nullptr)
   {
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
   }
   else
   {
      m_buffer = m_internalBuffer;
      m_length = 0;
      m_buffer[0] = 0;
   }
   return *this;
}
