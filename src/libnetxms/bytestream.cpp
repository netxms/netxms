/*
 ** NetXMS - Network Management System
 ** NetXMS Foundation Library
 ** Copyright (C) 2003-2015 Raden Solutions
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
 ** File: config.cpp
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
   m_data = (m_allocated > 0) ? (BYTE *)malloc(m_allocated) : NULL;
}

/**
 * Create byte stream from existing data
 */
ByteStream::ByteStream(const void *data, size_t size)
{
   m_allocated = size;
   m_size = size;
   m_pos = 0;
   m_data = (m_allocated > 0) ? (BYTE *)nx_memdup(data, size) : NULL;
}

/**
 * Destructor
 */
ByteStream::~ByteStream()
{
   safe_free(m_data);
}

/**
 * Write data
 */
void ByteStream::write(void *data, size_t size)
{
   if (m_pos + size > m_allocated)
   {
      m_allocated += max(size, 4096);
      m_data = (BYTE *)realloc(m_data, m_allocated);
   }
   memcpy(&m_data[m_pos], data, size);
   m_pos += size;
   if (m_pos > m_size)
      m_size = m_pos;
}

/**
 * Read data
 */
size_t ByteStream::read(BYTE *buffer, size_t count)
{
   size_t c = min(count, m_size - m_pos);
   if (c > 0)
   {
      memcpy(buffer, &m_data[m_pos], c);
      m_pos += c;
   }
   return c;
}

/**
 * Read 16 bit integer
 */
INT16 ByteStream::readInt16()
{
   if (m_size - m_pos < 2)
   {
      m_pos = m_size;
      return 0;
   }

   UINT16 n;
   memcpy(&n, &m_data[m_pos], 2);
   m_pos += 2;
   return (INT16)ntohs(n);
}

/**
 * Read unsigned 16 bit integer
 */
UINT16 ByteStream::readUInt16()
{
   if (m_size - m_pos < 2)
   {
      m_pos = m_size;
      return 0;
   }

   UINT16 n;
   memcpy(&n, &m_data[m_pos], 2);
   m_pos += 2;
   return ntohs(n);
}
/**
 * Read 16 bit integer
 */
INT32 ByteStream::readInt32()
{
   if (m_size - m_pos < 4)
   {
      m_pos = m_size;
      return 0;
   }

   UINT32 n;
   memcpy(&n, &m_data[m_pos], 4);
   m_pos += 4;
   return (INT32)ntohl(n);
}

/**
 * Read unsigned 16 bit integer
 */
UINT32 ByteStream::readUInt32()
{
   if (m_size - m_pos < 4)
   {
      m_pos = m_size;
      return 0;
   }

   UINT32 n;
   memcpy(&n, &m_data[m_pos], 4);
   m_pos += 4;
   return ntohl(n);
}

/**
 * Read string
 */
size_t ByteStream::readString(char *buffer, size_t count)
{
   size_t c = read((BYTE *)buffer, count);
   buffer[c] = 0;
   return c;
}
