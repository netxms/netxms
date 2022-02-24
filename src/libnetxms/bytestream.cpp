/*
 ** NetXMS - Network Management System
 ** NetXMS Foundation Library
 ** Copyright (C) 2003-2022 Raden Solutions
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
 ** File: bytestream.cpp
 **
 **/

#include "libnetxms.h"

/**
 * Create empty byte stream
 */
ByteStream::ByteStream(size_t initial)
{
   m_allocated = initial;
   m_size = 0;
   m_pos = 0;
   m_allocationStep = 4096;
   m_data = (m_allocated > 0) ? MemAllocArrayNoInit<BYTE>(m_allocated) : nullptr;
}

/**
 * Create byte stream from existing data
 */
ByteStream::ByteStream(const void *data, size_t size)
{
   m_allocated = size;
   m_size = size;
   m_pos = 0;
   m_allocationStep = 4096;
   m_data = (m_allocated > 0) ? static_cast<BYTE*>(MemCopyBlock(data, size)) : nullptr;
}

/**
 * Destructor
 */
ByteStream::~ByteStream()
{
   MemFree(m_data);
}

off_t ByteStream::seek(off_t pos, int32_t whence)
{
   switch (whence)
   {
      case SEEK_SET:
         if (pos > m_size || pos < 0)
            return -1;
         m_pos = pos;
         break;
      case SEEK_CUR:
         if (pos + m_pos > m_size || pos + m_pos < 0)
            return -1;
         m_pos += pos;
         break;
      case SEEK_END:
         if (pos + m_size > m_size || pos + m_size < 0)
            return -1;
         m_pos += pos;
         break;
      default:
         return -1;
   }
   return m_pos;
}

/**
 * Take byte stream buffer (byte stream will become empty)
 */
BYTE *ByteStream::takeBuffer()
{
   BYTE *data = m_data;
   m_data = nullptr;
   m_allocated = 0;
   m_size = 0;
   m_pos = 0;
   return data;
}

/**
 * Write data
 */
void ByteStream::write(const void *data, size_t size)
{
   if (m_pos + size > m_allocated)
   {
      m_allocated += std::max(size, m_allocationStep);
      m_data = MemRealloc(m_data, m_allocated);
   }
   memcpy(&m_data[m_pos], data, size);
   m_pos += size;
   if (m_pos > m_size)
      m_size = m_pos;
}

/**
 * Write string
 * @param str the input string
 * @param codepage encoding of the in-stream string
 * @param length if length > -1 exactly length symbols will be written (not paying attention to null-termination)
 * @param prependLength prepend string with is't length
 * @param nullTerminate null-terminate input string in buffer
 * @return Count of bytes written.
 */
size_t ByteStream::writeString(const WCHAR* str, const char* codepage, ssize_t length, bool prependLength, bool nullTerminate)
{
   if (length < 0)
      length = wcslen(str);
   
   size_t byteCount = length * 4;
   size_t writeStartPos = m_pos;

   if (prependLength)
   {
      if (byteCount < 0x8000) // if len < 2^15 prepend str with len as 2 bytes (0xxx xxxx)
         m_pos += 2;
      else // if len > 2^15 prepend str with len as 4 bytes with higher bit set (1xxx xxxx xxxx xxxx)
         m_pos += 4;
   }

   if (m_pos + byteCount > m_allocated)
   {
      m_allocated += std::max(byteCount, m_allocationStep);
      m_data = MemRealloc(m_data, m_allocated);
   }

   size_t bytesWritten = wchar_to_mbcp(str, length, reinterpret_cast<char*>(&m_data[m_pos]), byteCount, codepage);
   m_pos += bytesWritten;

   if (prependLength)
   {
      if (byteCount < 0x8000)
      {
         uint16_t n = HostToBigEndian16(bytesWritten);
         memcpy(&m_data[writeStartPos], &n, 2);
      }
      else
      {
         uint32_t n = HostToBigEndian32(static_cast<uint32_t>(bytesWritten) | 0x80000000);
         memcpy(&m_data[writeStartPos], &n, 4);
      }
   }

   if (nullTerminate)
      write((BYTE)0);

   if (m_pos > m_size)
      m_size = m_pos;

   return m_pos - writeStartPos;
}

/**
 * Write string
 * @param str the input string
 * @param length if length > -1 exactly length symbols will be written (not paying attention to null-termination)
 * @param prependLength prepend string with is't length
 * @param nullTerminate null-terminate input string in buffer
 * @return Count of bytes written.
 */
size_t ByteStream::writeString(const char* str, ssize_t length, bool prependLength, bool nullTerminate)
{
   if (length < 0)
      length = strlen(str);

   size_t writeStartPos = m_pos;

   if (prependLength)
   {
      if (length < 0x8000) // if len < 2^15 prepend s with len as 2 bytes (0xxx xxxx)
      {
         uint16_t tmp = static_cast<uint16_t>(length);
         write(&tmp, 2);
      }
      else // if len > 2^15 prepend s with len as 4 bytes with higher bit set (1xxx xxxx xxxx xxxx)
      {
         uint32_t tmp = static_cast<uint32_t>(length) | 0x80000000;
         write(&tmp, 4);
      }
   }

   write(str, length);

   if (nullTerminate)
      write(static_cast<BYTE>(0));

   return m_pos - writeStartPos;
}

/**
 * Read data
 */
size_t ByteStream::read(void *buffer, size_t count)
{
   size_t c = std::min(count, m_size - m_pos);
   if (c > 0)
   {
      memcpy(buffer, &m_data[m_pos], c);
      m_pos += c;
   }
   return c;
}

/**
 * Determine string length.
 */
ssize_t ByteStream::getLengthToRead(ssize_t length, bool isLenPrepended, bool isNullTerminated)
{
   if (eos())
      return  -1;

   if (isLenPrepended)
   {
      BYTE b = m_data[m_pos];
      if (b & 0x80) // length in 4 bytes
      {
         if (m_size - m_pos < 4)
            return -1;
         length = readUInt32B() & ~0x80000000;
      }
      else // length in 2 bytes
      {
         if (m_size - m_pos < 2)
            return -1;
         length = readUInt16B();
      }
   }
   else if (isNullTerminated)
   {
      ssize_t eosPos = find(0);
      if (eosPos < 0)
         return -1;
      length = eosPos - m_pos;
   }

   if (m_size - m_pos < length)
      return -1;

   return length;
}

/**
 * Read string. Parameters (except codepage) are mutually exclusive.
 * @param codepage encoding of the in-stream string
 * @param length if length > -1 - read length bytes
 * @param isLenPrepended assume that the in-stream string is prepended with it's length
 * @param isNullTerminated assume that the in-stream string is null-terminated
 * @return Dynamically allocated wide char string in UCS-4 (UNICODE)
 */
WCHAR* ByteStream::readStringWCore(const char* codepage, ssize_t length, bool isLenPrepended, bool isNullTerminated)
{
   length = getLengthToRead(length, isLenPrepended, isNullTerminated);
   if (length < 0)
      return nullptr;

   WCHAR* buffer = MemAllocStringW(length + 1);
   size_t count = mbcp_to_wchar(reinterpret_cast<char*>(&m_data[m_pos]), length, buffer, length, codepage);

   m_pos += isNullTerminated ? length + 1: length;
   buffer[count] = 0;
   return buffer;
}

/**
 * Read string. Parameters (except codepage) are mutually exclusive.
 * @param codepage encoding of the in-stream string
 * @param length if length > -1 - read length bytes
 * @param isLenPrepended assume that the in-stream string is prepended with it's length
 * @param isNullTerminated assume that the in-stream string is null-terminated
 * @return Dynamically allocated wide char string in UCS-4 (UNICODE)
 */
char* ByteStream::readStringAsUTF8Core(const char* codepage, ssize_t length, bool isLenPrepended, bool isNullTerminated)
{
   length = getLengthToRead(length, isLenPrepended, isNullTerminated);
   if (length < 0)
      return nullptr;

   char* buffer = MemAllocStringA(length * 4 + 1);
   size_t count = mbcp_to_utf8(reinterpret_cast<char*>(&m_data[m_pos]), length, buffer, length, codepage);
   
   m_pos += isNullTerminated ? length + 1: length;
   buffer[count] = 0;
   return buffer;
}

/**
 * Read string. Parameters are mutually exclusive.
 * @param length if length > -1 - read length bytes
 * @param isLenPrepended assume that the in-stream string is prepended with it's length
 * @param isNullTerminated assume that the in-stream string is null-terminated
 * @return Dynamically allocated multibyte string in same encoding as it was in the stream
 */
char* ByteStream::readStringCore(ssize_t length, bool isLenPrepended, bool isNullTerminated)
{
   length = getLengthToRead(length, isLenPrepended, isNullTerminated);
   if (length < 0)
      return nullptr;

   char* buffer = MemAllocStringA(length + 1);
   memcpy(buffer, &m_data[m_pos], length);
   buffer[length] = 0;
   m_pos += isNullTerminated ? length + 1: length;
   return buffer;
}

/**
 * Save byte stream to file
 */
bool ByteStream::save(int f)
{
#ifdef _WIN32
   return _write(f, m_data, static_cast<unsigned int>(m_size)) == static_cast<int>(m_size);
#else
   return ::write(f, m_data, m_size) == m_size;
#endif
}

/**
 * Load from file
 */
ByteStream *ByteStream::load(const TCHAR *file)
{
   size_t size;
   BYTE *data = LoadFile(file, &size);
   if (data == nullptr)
      return nullptr;
   ByteStream *s = new ByteStream(0);
   s->m_allocated = size;
   s->m_size = size;
   s->m_data = data;
   return s;
}
