/*
 ** NetXMS - Network Management System
 ** NetXMS Foundation Library
 ** Copyright (C) 2003-2025 Raden Solutions
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
 * Create byte stream from existing data
 */
ConstByteStream::ConstByteStream(const BYTE *data, size_t size)
{
   m_allocated = size;
   m_size = size;
   m_pos = 0;
   m_allocationStep = 4096;
   m_data = const_cast<BYTE *>(data);
}

/**
 * Sets the current position of this stream to the given value.
 */
off_t ConstByteStream::seek(off_t offset, int origin)
{
   off_t pos;
   switch (origin)
   {
      case SEEK_SET:
         pos = offset;
         break;
      case SEEK_CUR:
         pos = static_cast<off_t>(m_pos) + offset;
         break;
      case SEEK_END:
         pos = static_cast<off_t>(m_size) + offset;
         break;
      default:
         return -1;
   }
   if ((pos < 0) || (pos > static_cast<off_t>(m_size)))
      return -1;
   m_pos = pos;
   return pos;
}

/**
 * Determine length of encoded string.
 */
ssize_t ConstByteStream::getEncodedStringLength(ssize_t byteCount, bool isLenPrepended, bool isNullTerminated, size_t charSize)
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
         byteCount = readUInt32B() & ~0x80000000;
      }
      else // length in 2 bytes
      {
         if (m_size - m_pos < 2)
            return -1;
         byteCount = readUInt16B();
      }
   }
   else if (isNullTerminated)
   {
      uint32_t n = 0;
      BYTE* p = nullptr;
      for (size_t i = m_pos; i < m_size; i += charSize)
      {
         if (memcmp(&m_data[i], &n, charSize) == 0)
         {
            p = &m_data[i];
            break;
         }
      }
      if (p == nullptr)
         return -1;
      byteCount = p - m_data - m_pos;
   }

   return (static_cast<ssize_t>(m_size - m_pos) < byteCount) ? -1 : byteCount;
}

/**
 * Read data
 */
size_t ConstByteStream::read(void* buffer, size_t count)
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
 * Read MB/UNICODE string as UNICODE string. Parameters (except codepage) are mutually exclusive.
 * @param codepage encoding of the in-stream string
 * @param byteCount if byteCount > -1 - read byteCount bytes
 * @param isLenPrepended assume that the in-stream string is prepended with it's length
 * @param isNullTerminated assume that the in-stream string is null-terminated
 * @return Dynamically allocated wide char string in UNICODE
 */
WCHAR *ConstByteStream::readStringWCore(const char* codepage, ssize_t byteCount, bool isLenPrepended, bool isNullTerminated)
{
   size_t charSize;
   if (!strnicmp(CHECK_NULL_A(codepage), "UCS2", 4) || !strnicmp(CHECK_NULL_A(codepage), "UCS-2", 5))
      charSize = sizeof(UCS2CHAR);
   else if (!strnicmp(CHECK_NULL_A(codepage), "UCS4", 4) || !strnicmp(CHECK_NULL_A(codepage), "UCS-4", 5))
      charSize = sizeof(UCS4CHAR);
   else
      charSize = sizeof(char);

   byteCount = getEncodedStringLength(byteCount, isLenPrepended, isNullTerminated, charSize);
   if (byteCount < 0)
      return nullptr;

   WCHAR* buffer = MemAllocStringW(byteCount + 1);
   ssize_t count;
   if (strnicmp(CHECK_NULL_A(codepage), "UCS", 3) == 0)
      count = readStringU(buffer, codepage, byteCount);
   else
      count = mbcp_to_wchar(reinterpret_cast<char*>(&m_data[m_pos]), byteCount, buffer, byteCount, codepage);
   if (count == -1)
   {
      MemFree(buffer);
      return nullptr;
   }

   m_pos += byteCount;
   if (isNullTerminated)
      m_pos += charSize;

   buffer[count] = 0;
   return buffer;
}

/**
 * Swap bytes in UCS2 character
 */
static inline UCS2CHAR SwapUCS2(UCS2CHAR c)
{
   return bswap_16(c);
}

/**
 * Swap bytes in UCS4 character
 */
static inline UCS4CHAR SwapUCS4(UCS4CHAR c)
{
   return bswap_32(c);
}

/**
 * memcpy wrapper for use in ReadUnicodeString
 */
template<typename T> static inline size_t DirectCopyReader(const T* source, ssize_t length, WCHAR* destination, size_t available)
{
   memcpy(destination, source, length * sizeof(T));
   return length;
}

/**
 * Read UNICODE string into provided buffer wth byte swap
 */
template<typename T, size_t (*Reader)(const T*, ssize_t, WCHAR*, size_t), T (*Swapper)(T)> static inline size_t ReadUnicodeString(const BYTE *source, ssize_t byteCount, WCHAR *destination)
{
   size_t length = byteCount / sizeof(T);

   T localBuffer[1024];
   T* conversionBuffer = (length < 1024) ? localBuffer : MemAllocArrayNoInit<T>(length);
   const T* s = reinterpret_cast<const T*>(source);
   for (size_t i = 0; i < length; i++)
      conversionBuffer[i] = Swapper(*s++);

   size_t count = Reader(conversionBuffer, length, destination, length);

   if (conversionBuffer != localBuffer)
      MemFree(conversionBuffer);
   return count;
}

/**
 * Read UNICODE string into provided buffer without byte swap
 */
template<typename T, size_t (*Reader)(const T*, ssize_t, WCHAR*, size_t)> static inline size_t ReadUnicodeString(const BYTE *source, ssize_t byteCount, WCHAR *destination)
{
   size_t length = byteCount / sizeof(T);
   return Reader(reinterpret_cast<const T*>(source), length, destination, length);
}

/**
 * Read UNICODE string as UNICODE string
 * @param buffer read string will be stored here
 * @param codepage encoding of the in-stream string
 * @param byteCount if length > -1 - read length bytes
 * @return Count of bytes read. -1 on codepage recognition error.
 */
ssize_t ConstByteStream::readStringU(WCHAR* buffer, const char* codepage, ssize_t byteCount)
{
#if defined(UNICODE_UCS2)
   if (!stricmp(codepage, "UCS2") || !stricmp(codepage, "UCS-2"))
   {
      return ReadUnicodeString<UCS2CHAR, DirectCopyReader<UCS2CHAR>>(&m_data[m_pos], byteCount, buffer);
   }
   else if (!stricmp(codepage, "UCS2BE") || !stricmp(codepage, "UCS-2BE"))
   {
#ifdef WORDS_BIGENDIAN
      return ReadUnicodeString<UCS2CHAR, DirectCopyReader<UCS2CHAR>>(&m_data[m_pos], byteCount, buffer);
#else
      return ReadUnicodeString<UCS2CHAR, DirectCopyReader<UCS2CHAR>, SwapUCS2>(&m_data[m_pos], byteCount, buffer);
#endif
   }
   else if (!stricmp(codepage, "UCS2LE") || !stricmp(codepage, "UCS-2LE"))
   {
#ifdef WORDS_BIGENDIAN
      return ReadUnicodeString<UCS2CHAR, DirectCopyReader<UCS2CHAR>, SwapUCS2>(&m_data[m_pos], byteCount, buffer);
#else
      return ReadUnicodeString<UCS2CHAR, DirectCopyReader<UCS2CHAR>>(&m_data[m_pos], byteCount, buffer);
#endif
   }
   else if (!stricmp(codepage, "UCS4") || !stricmp(codepage, "UCS-4"))
   {
      return ReadUnicodeString<UCS4CHAR, ucs4_to_ucs2>(&m_data[m_pos], byteCount, buffer);
   }
   else if (!stricmp(codepage, "UCS4BE") || !stricmp(codepage, "UCS-4BE"))
   {
#ifdef WORDS_BIGENDIAN
      return ReadUnicodeString<UCS4CHAR, ucs4_to_ucs2>(&m_data[m_pos], byteCount, buffer);
#else
      return ReadUnicodeString<UCS4CHAR, ucs4_to_ucs2, SwapUCS4>(&m_data[m_pos], byteCount, buffer);
#endif
   }
   else if (!stricmp(codepage, "UCS4LE") || !stricmp(codepage, "UCS-4LE"))
   {
#ifdef WORDS_BIGENDIAN
      return ReadUnicodeString<UCS4CHAR, ucs4_to_ucs2, SwapUCS4>(&m_data[m_pos], byteCount, buffer);
#else
      return ReadUnicodeString<UCS4CHAR, ucs4_to_ucs2>(&m_data[m_pos], byteCount, buffer);
#endif
   }
#else
   if (!stricmp(codepage, "UCS2") || !stricmp(codepage, "UCS-2"))
   {
      return ReadUnicodeString<UCS2CHAR, ucs2_to_ucs4>(&m_data[m_pos], byteCount, buffer);
   }
   else if (!stricmp(codepage, "UCS2BE") || !stricmp(codepage, "UCS-2BE"))
   {
#ifdef WORDS_BIGENDIAN
      return ReadUnicodeString<UCS2CHAR, ucs2_to_ucs4>(&m_data[m_pos], byteCount, buffer);
#else
      return ReadUnicodeString<UCS2CHAR, ucs2_to_ucs4, SwapUCS2>(&m_data[m_pos], byteCount, buffer);
#endif
   }
   else if (!stricmp(codepage, "UCS2LE") || !stricmp(codepage, "UCS-2LE"))
   {
#ifdef WORDS_BIGENDIAN
      return ReadUnicodeString<UCS2CHAR, ucs2_to_ucs4, SwapUCS2>(&m_data[m_pos], byteCount, buffer);
#else
      return ReadUnicodeString<UCS2CHAR, ucs2_to_ucs4>(&m_data[m_pos], byteCount, buffer);
#endif
   }
   else if (!stricmp(codepage, "UCS4") || !stricmp(codepage, "UCS-4"))
   {
      return ReadUnicodeString<UCS4CHAR, DirectCopyReader<UCS4CHAR>>(&m_data[m_pos], byteCount, buffer);
   }
   else if (!stricmp(codepage, "UCS4BE") || !stricmp(codepage, "UCS-4BE"))
   {
#ifdef WORDS_BIGENDIAN
      return ReadUnicodeString<UCS4CHAR, DirectCopyReader<UCS4CHAR>>(&m_data[m_pos], byteCount, buffer);
#else
      return ReadUnicodeString<UCS4CHAR, DirectCopyReader<UCS4CHAR>, SwapUCS4>(&m_data[m_pos], byteCount, buffer);
#endif
   }
   else if (!stricmp(codepage, "UCS4LE") || !stricmp(codepage, "UCS-4LE"))
   {
#ifdef WORDS_BIGENDIAN
      return ReadUnicodeString<UCS4CHAR, DirectCopyReader<UCS4CHAR>, SwapUCS4>(&m_data[m_pos], byteCount, buffer);
#else
      return ReadUnicodeString<UCS4CHAR, DirectCopyReader<UCS4CHAR>>(&m_data[m_pos], byteCount, buffer);
#endif
   }
#endif
   return -1;
}

/**
 * Read MB string as UTF-8 string. Parameters (except codepage) are mutually exclusive.
 * @param codepage encoding of the in-stream string
 * @param byteCount if byteCount > -1 - read byteCount bytes
 * @param isLenPrepended assume that the in-stream string is prepended with it's length
 * @param isNullTerminated assume that the in-stream string is null-terminated
 * @return Dynamically allocated wide char string in UCS-4 (UNICODE)
 */
char* ConstByteStream::readStringAsUTF8Core(const char* codepage, ssize_t byteCount, bool isLenPrepended, bool isNullTerminated)
{
   byteCount = getEncodedStringLength(byteCount, isLenPrepended, isNullTerminated, sizeof(char));
   if (byteCount < 0)
      return nullptr;

   char* buffer = MemAllocStringA(byteCount * sizeof(WCHAR) + 1);
   size_t count = mbcp_to_utf8(reinterpret_cast<char*>(&m_data[m_pos]), byteCount, buffer, byteCount, codepage);

   m_pos += isNullTerminated ? byteCount + 1 : byteCount;
   buffer[count] = 0;
   return buffer;
}

/**
 * Read MB string as MB string. Parameters are mutually exclusive.
 * @param byteCount if byteCount > -1 - read byteCount bytes
 * @param isLenPrepended assume that the in-stream string is prepended with it's length
 * @param isNullTerminated assume that the in-stream string is null-terminated
 * @return Dynamically allocated multibyte string in same encoding as it was in the stream
 */
char* ConstByteStream::readStringCore(ssize_t byteCount, bool isLenPrepended, bool isNullTerminated)
{
   byteCount = getEncodedStringLength(byteCount, isLenPrepended, isNullTerminated, sizeof(char));
   if (byteCount < 0)
      return nullptr;

   char* buffer = MemAllocStringA(byteCount + 1);
   memcpy(buffer, &m_data[m_pos], byteCount);
   buffer[byteCount] = 0;
   m_pos += isNullTerminated ? byteCount + 1 : byteCount;
   return buffer;
}

/**
 * Read string from NXCP message
 */
TCHAR *ConstByteStream::readNXCPString(MemoryPool *pool)
{
   if (eos())
      return nullptr;

   if (m_size - m_pos < 2)
      return nullptr;

   ssize_t byteCount = readUInt16B();

   if (static_cast<ssize_t>(m_size - m_pos) < byteCount)
      return nullptr;

   TCHAR* buffer = pool != nullptr ? pool->allocateString(byteCount + 1) : MemAllocString(byteCount + 1);

#ifdef UNICODE
   size_t chars = utf8_to_wchar(reinterpret_cast<char*>(&m_data[m_pos]), byteCount, buffer, byteCount + 1);
#else
   size_t chars = utf8_to_mb(reinterpret_cast<char*>(&m_data[m_pos]), byteCount, buffer, byteCount + 1);
#endif

   buffer[chars] = 0;
   m_pos += byteCount;
   return buffer;
}

/**
 * Save byte stream to file
 */
bool ConstByteStream::save(int f)
{
#ifdef _WIN32
   return _write(f, m_data, static_cast<unsigned int>(m_size)) == static_cast<int>(m_size);
#else
   return ::write(f, m_data, m_size) == static_cast<ssize_t>(m_size);
#endif
}


/*************************************************************************
 * ByteStream
 *************************************************************************/

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
 * Write signed value in LEB128 format
 */
void ByteStream::writeSignedLEB128(int64_t n)
{
   uint8_t encoded[10];
   size_t len = 0;
   bool more;
   do
   {
      uint8_t byte = static_cast<uint8_t>(n & 0x7F);
      n >>= 7;
      more = (byte & 0x40) ? (n != -1) : (n != 0);
      if (more)
         byte |= 0x80;
      encoded[len++] = byte;
   }
   while (more);
   write(encoded, len);
}

/**
 * Write unsigned value in LEB128 format
 */
void ByteStream::writeUnsignedLEB128(uint64_t n)
{
   uint8_t encoded[10];
   size_t len = 0;
   do
   {
      uint8_t byte = static_cast<uint8_t>(n & 0x7F);
      n >>= 7;
      if (n != 0)
         byte |= 0x80;
      encoded[len++] = byte;
   }
   while (n != 0);
   write(encoded, len);
}

/**
 * Write UNICODE string as MB/UNICODE string
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

   ssize_t bytesWritten;
   if ((codepage != nullptr) && !strnicmp(codepage, "UCS", 3))
   {
      bytesWritten = writeStringU(str, length, codepage);
   }
   else
   {
      bytesWritten = wchar_to_mbcp(str, length, reinterpret_cast<char*>(&m_data[m_pos]), byteCount, codepage);
   }
   m_pos += bytesWritten;

   if (prependLength)
   {
      if (byteCount < 0x8000)
      {
         uint16_t n = HostToBigEndian16(static_cast<uint16_t>(bytesWritten));
         memcpy(&m_data[writeStartPos], &n, 2);
      }
      else
      {
         uint32_t n = HostToBigEndian32(static_cast<uint32_t>(bytesWritten) | 0x80000000);
         memcpy(&m_data[writeStartPos], &n, 4);
      }
   }

   if (nullTerminate)
   {
      if ((codepage != nullptr) && (!strnicmp(codepage, "UCS2", 4) || !strnicmp(codepage, "UCS-2", 5)))
         writeL((int16_t)0);
      else if ((codepage != nullptr) && (!strnicmp(codepage, "UCS4", 4) || !strnicmp(codepage, "UCS-4", 5)))
         writeL((int32_t)0);
      else
         write((BYTE)0);
   }

   if (m_pos > m_size)
      m_size = m_pos;

   return m_pos - writeStartPos;
}

/**
 * Write MB string as MB string
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
         uint16_t n = HostToBigEndian16(static_cast<uint16_t>(length));
         write(&n, 2);
      }
      else // if len > 2^15 prepend s with len as 4 bytes with higher bit set (1xxx xxxx xxxx xxxx)
      {
         uint32_t n = HostToBigEndian32(static_cast<uint32_t>(length) | 0x80000000);
         write(&n, 4);
      }
   }

   write(str, length);

   if (nullTerminate)
      write(static_cast<BYTE>(0));

   return m_pos - writeStartPos;
}

/**
 * Write string to NXCP message in UTF8 and with prepend string length
 * @return Count of bytes written.
 */
size_t ByteStream::writeNXCPString(const TCHAR *string)
{
   size_t length = _tcslen(string);
   size_t byteCount = length * 4;
   size_t writeStartPos = m_pos;

   m_pos += 2;

   if (m_pos + byteCount > m_allocated)
   {
      m_allocated += std::max(byteCount, m_allocationStep);
      m_data = MemRealloc(m_data, m_allocated);
   }

#ifdef UNICODE
#if UNICODE_UCS4
   ssize_t bytesWritten = ucs4_to_utf8(string, length, reinterpret_cast<char*>(&m_data[m_pos]), byteCount);
#else
   ssize_t bytesWritten = ucs2_to_utf8(string, length, reinterpret_cast<char*>(&m_data[m_pos]), byteCount);
#endif
#else    /* not UNICODE */
   ssize_t bytesWritten = mb_to_utf8(string, length, reinterpret_cast<char*>(&m_data[m_pos]), byteCount);
#endif
   m_pos += bytesWritten;

   uint16_t n = HostToBigEndian16(static_cast<uint16_t>(bytesWritten));
   memcpy(&m_data[writeStartPos], &n, 2);

   if (m_pos > m_size)
      m_size = m_pos;

   return m_pos - writeStartPos;
}

/**
 * memcpy wrapper for use in WriteUnicodeString
 */
template<typename T> static inline size_t DirectCopyWriter(const WCHAR* source, ssize_t length, T* destination, size_t available)
{
   memcpy(destination, source, length * sizeof(WCHAR));
   return length;
}

/**
 * Write UNICODE string into provided buffer
 */
template<typename T, size_t (*Writer)(const WCHAR*, ssize_t, T*, size_t), T (*Swapper)(T)> static inline size_t WriteUnicodeString(const WCHAR* source, size_t length, BYTE *destination)
{
   size_t l = Writer(source, length, reinterpret_cast<T*>(destination), length * 2);
#if __cpp_if_constexpr
   if constexpr (Swapper != nullptr)
#else
   if (Swapper != nullptr)
#endif
   {
      for (size_t i = 0; i < l; i++)
      {
         *reinterpret_cast<T*>(destination[i]) = Swapper(*reinterpret_cast<T*>(destination[i]));
         i += sizeof(T);
      }
   }
   return l * sizeof(T);
}

/**
 * Write UNICODE string as UNICODE string
 * @param str the input string
 * @param length if length > -1 exactly length symbols will be written (not paying attention to null-termination)
 * @param codepage encoding of the in-stream string
 * @return Count of bytes written. -1 on codepage recognition error.
 */
ssize_t ByteStream::writeStringU(const WCHAR* str, size_t length, const char* codepage)
{
#if defined(UNICODE_UCS2)
   if (!stricmp(codepage, "UCS2") || !stricmp(codepage, "UCS-2"))
   {
      return WriteUnicodeString<UCS2CHAR, DirectCopyWriter<UCS2CHAR>, nullptr>(str, length, &m_data[m_pos]);
   }
   else if (!stricmp(codepage, "UCS2BE") || !stricmp(codepage, "UCS-2BE"))
   {
#ifdef WORDS_BIGENDIAN
      return WriteUnicodeString<UCS2CHAR, DirectCopyWriter<UCS2CHAR>, nullptr>(str, length, &m_data[m_pos]);
#else
      return WriteUnicodeString<UCS2CHAR, DirectCopyWriter<UCS2CHAR>, SwapUCS2>(str, length, &m_data[m_pos]);
#endif
   }
   else if (!stricmp(codepage, "UCS2LE") || !stricmp(codepage, "UCS-2LE"))
   {
#ifdef WORDS_BIGENDIAN
      return WriteUnicodeString<UCS2CHAR, DirectCopyWriter<UCS2CHAR>, SwapUCS2>(str, length, &m_data[m_pos]);
#else
      return WriteUnicodeString<UCS2CHAR, DirectCopyWriter<UCS2CHAR>, nullptr>(str, length, &m_data[m_pos]);
#endif
   }
   else if (!stricmp(codepage, "UCS4") || !stricmp(codepage, "UCS-4"))
   {
      return WriteUnicodeString<UCS4CHAR, ucs2_to_ucs4, nullptr>(str, length, &m_data[m_pos]);
   }
   else if (!stricmp(codepage, "UCS4BE") || !stricmp(codepage, "UCS-4BE"))
   {
#ifdef WORDS_BIGENDIAN
      return WriteUnicodeString<UCS4CHAR, ucs2_to_ucs4, nullptr>(str, length, &m_data[m_pos]);
#else
      return WriteUnicodeString<UCS4CHAR, ucs2_to_ucs4, SwapUCS4>(str, length, &m_data[m_pos]);
#endif
   }
   else if (!stricmp(codepage, "UCS4LE") || !stricmp(codepage, "UCS-4LE"))
   {
#ifdef WORDS_BIGENDIAN
      return WriteUnicodeString<UCS4CHAR, ucs2_to_ucs4, SwapUCS4>(str, length, &m_data[m_pos]);
#else
      return WriteUnicodeString<UCS4CHAR, ucs2_to_ucs4, nullptr>(str, length, &m_data[m_pos]);
#endif
   }
#else
   if (!stricmp(codepage, "UCS2") || !stricmp(codepage, "UCS-2"))
   {
      return WriteUnicodeString<UCS2CHAR, ucs4_to_ucs2, nullptr>(str, length, &m_data[m_pos]);
   }
   else if (!stricmp(codepage, "UCS2BE") || !stricmp(codepage, "UCS-2BE"))
   {
#ifdef WORDS_BIGENDIAN
      return WriteUnicodeString<UCS2CHAR, ucs4_to_ucs2, nullptr>(str, length, &m_data[m_pos]);
#else
      return WriteUnicodeString<UCS2CHAR, ucs4_to_ucs2, SwapUCS2>(str, length, &m_data[m_pos]);
#endif
   }
   else if (!stricmp(codepage, "UCS2LE") || !stricmp(codepage, "UCS-2LE"))
   {
#ifdef WORDS_BIGENDIAN
      return WriteUnicodeString<UCS2CHAR, ucs4_to_ucs2, SwapUCS2>(str, length, &m_data[m_pos]);
#else
      return WriteUnicodeString<UCS2CHAR, ucs4_to_ucs2, nullptr>(str, length, &m_data[m_pos]);
#endif
   }
   else if (!stricmp(codepage, "UCS4") || !stricmp(codepage, "UCS-4"))
   {
      return WriteUnicodeString<UCS4CHAR, DirectCopyWriter<UCS4CHAR>, nullptr>(str, length, &m_data[m_pos]);
   }
   else if (!stricmp(codepage, "UCS4BE") || !stricmp(codepage, "UCS-4BE"))
   {
#ifdef WORDS_BIGENDIAN
      return WriteUnicodeString<UCS4CHAR, DirectCopyWriter<UCS4CHAR>, nullptr>(str, length, &m_data[m_pos]);
#else
      return WriteUnicodeString<UCS4CHAR, DirectCopyWriter<UCS4CHAR>, SwapUCS4>(str, length, &m_data[m_pos]);
#endif
   }
   else if (!stricmp(codepage, "UCS4LE") || !stricmp(codepage, "UCS-4LE"))
   {
#ifdef WORDS_BIGENDIAN
      return WriteUnicodeString<UCS4CHAR, DirectCopyWriter<UCS4CHAR>, SwapUCS4>(str, length, &m_data[m_pos]);
#else
      return WriteUnicodeString<UCS4CHAR, DirectCopyWriter<UCS4CHAR>, nullptr>(str, length, &m_data[m_pos]);
#endif
   }
#endif
   return -1;
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

/**
 * Callback for writing data received from cURL to provided byte stream
 */
size_t ByteStream::curlWriteFunction(char *ptr, size_t size, size_t nmemb, ByteStream *data)
{
   size_t bytes = size * nmemb;
   data->write(ptr, bytes);
   return bytes;
}
