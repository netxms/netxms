/*
** NetXMS - Network Management System
** NetXMS Foundation Library
** Copyright (C) 2003-2024 Victor Kirhenshtein
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
** File: tfw.cpp
**
**/

#include "libnetxms.h"

/**
 * Append integer
 */
TextFileWriter& TextFileWriter::append(int32_t n, const TCHAR *format)
{
   if (format != nullptr)
   {
      TCHAR buffer[64];
      _sntprintf(buffer, 64, format, n);
      append(buffer);
   }
   else
   {
      char buffer[64];
      appendUtf8String(IntegerToString(n, buffer), -1);
   }
   return *this;
}

/**
 * Append integer
 */
TextFileWriter& TextFileWriter::append(uint32_t n, const TCHAR *format)
{
   if (format != nullptr)
   {
      TCHAR buffer[64];
      _sntprintf(buffer, 64, format, n);
      append(buffer);
   }
   else
   {
      char buffer[64];
      appendUtf8String(IntegerToString(n, buffer), -1);
   }
   return *this;
}

/**
 * Append integer
 */
TextFileWriter& TextFileWriter::append(int64_t n, const TCHAR *format)
{
   if (format != nullptr)
   {
      TCHAR buffer[64];
      _sntprintf(buffer, 64, format, n);
      append(buffer);
   }
   else
   {
      char buffer[64];
      appendUtf8String(IntegerToString(n, buffer), -1);
   }
   return *this;
}

/**
 * Append integer
 */
TextFileWriter& TextFileWriter::append(uint64_t n, const TCHAR *format)
{
   if (format != nullptr)
   {
      TCHAR buffer[64];
      _sntprintf(buffer, 64, format, n);
      append(buffer);
   }
   else
   {
      char buffer[64];
      appendUtf8String(IntegerToString(n, buffer), -1);
   }
   return *this;
}

/**
 * Append floating point number
 */
TextFileWriter& TextFileWriter::append(double d, const TCHAR *format)
{
   if (format != nullptr)
   {
      TCHAR buffer[64];
      _sntprintf(buffer, 64, format, d);
      append(buffer);
   }
   else
   {
      char buffer[64];
      snprintf(buffer, 64, "%f", d);
      appendUtf8String(buffer, -1);
   }
   return *this;
}

/**
 * Append GUID
 */
TextFileWriter& TextFileWriter::append(const uuid& guid)
{
   char buffer[64];
   guid.toStringA(buffer);
   return appendUtf8String(buffer, -1);
}

/**
 * Append multibyte string
 */
TextFileWriter& TextFileWriter::appendMBString(const char *str, ssize_t len)
{
   if (len < 0)
      len = strlen(str);
   Buffer<char, 4096> buffer(len * 3);
   size_t nchars = mb_to_utf8(str, len, buffer, len * 3);
   fwrite(buffer, 1, nchars, m_handle);
   return *this;
}

/**
 * Append UTF-8 string
 */
TextFileWriter& TextFileWriter::appendUtf8String(const char *str, ssize_t len)
{
   if (len < 0)
      len = strlen(str);
   fwrite(str, 1, len, m_handle);
   return *this;
}

/**
 * Append wide character string
 */
TextFileWriter& TextFileWriter::appendWideString(const WCHAR *str, ssize_t len)
{
   if (len < 0)
      len = wcslen(str);
   Buffer<char, 4096> buffer(len * 3);
   size_t nchars = wchar_to_utf8(str, len, buffer, len * 3);
   fwrite(buffer, 1, nchars, m_handle);
   return *this;
}

/**
 * Append formatted string
 */
TextFileWriter& TextFileWriter::appendFormattedString(const TCHAR *format, ...)
{
   va_list args;
   va_start(args, format);
   appendFormattedStringV(format, args);
   va_end(args);
   return *this;
}

/**
 * Append formatted string
 */
TextFileWriter& TextFileWriter::appendFormattedStringV(const TCHAR *format, va_list args)
{
   return *this;
}

/**
 * Append binary data as hex string
 */
TextFileWriter& TextFileWriter::appendAsHexString(const void *data, size_t len, char separator)
{
   size_t bsize = (separator != 0) ? len * 3 : len * 2 + 1;
   Buffer<char, 4096> buffer(bsize);
   BinToStrExA(data, len, buffer, separator, 0);
   fwrite(buffer, 1, bsize - 1, m_handle);
   return *this;
}

/**
 * Append binary data as base64 string
 */
TextFileWriter& TextFileWriter::appendAsBase64String(const void *data, size_t len)
{
   char base64Buffer[BASE64_LENGTH(9000)];
   const char *in = static_cast<const char*>(data);
   while(len > 0)
   {
      size_t blockLen = std::min(len, static_cast<size_t>(9000));
      base64_encode(in, blockLen, base64Buffer, sizeof(base64Buffer));
      appendUtf8String(base64Buffer, BASE64_LENGTH(blockLen));
      len -= blockLen;
      in += blockLen;
   }
   return *this;
}
