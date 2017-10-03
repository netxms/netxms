/*
 ** NetXMS - Network Management System
 ** NetXMS Foundation Library
 ** Copyright (C) 2003-2017 Raden Solutions
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
 ** File: rbuffer.cpp
 **
 **/

#include "libnetxms.h"

/**
 * Constructor
 */
RingBuffer::RingBuffer(size_t initial, size_t allocationStep)
{
   m_data = (BYTE *)malloc(initial);
   m_size = 0;
   m_allocated = initial;
   m_allocationStep = allocationStep;
   m_readPos = 0;
   m_writePos = 0;
}

/**
 * Destructor
 */
RingBuffer::~RingBuffer()
{
   free(m_data);
}

/**
 * Write data
 */
void RingBuffer::write(const BYTE *data, size_t dataSize)
{
   if (dataSize <= m_allocated - m_size)
   {
      size_t chunkSize = m_allocated - m_writePos;
      if (dataSize <= chunkSize)
      {
         memcpy(&m_data[m_writePos], data, dataSize);
         m_writePos += dataSize;
      }
      else
      {
         memcpy(&m_data[m_writePos], data, chunkSize);
         memcpy(m_data, &data[chunkSize], dataSize - chunkSize);
         m_writePos = dataSize - chunkSize;
      }
   }
   else if (m_writePos > m_readPos)
   {
      m_allocated += std::max(dataSize, m_allocationStep);
      m_data = (BYTE *)realloc(m_data, m_allocated);
      memcpy(&m_data[m_writePos], data, dataSize);
      m_writePos += dataSize;
   }
   else if (m_size == 0)   // buffer is empty but not large enough to hold new data
   {
      m_allocated = dataSize + m_allocationStep;
      m_data = (BYTE *)realloc(m_data, m_allocated);
      memcpy(m_data, data, dataSize);
      m_writePos = dataSize;
      m_readPos = 0;
   }
   else
   {
      size_t tailSize = m_allocated - m_readPos;
      m_allocated = m_size + dataSize + m_allocationStep;
      BYTE *temp = (BYTE *)malloc(m_allocated);
      memcpy(temp, &m_data[m_readPos], tailSize);
      memcpy(&temp[tailSize], m_data, m_writePos);
      memcpy(&temp[m_size], data, dataSize);
      free(m_data);
      m_data = temp;
      m_readPos = 0;
      m_writePos = m_size + dataSize;
   }
   m_size += dataSize;
}

/**
 * Read data
 */
size_t RingBuffer::read(BYTE *buffer, size_t bufferSize)
{
   size_t readSize = std::min(bufferSize, m_size);
   if (readSize == 0)
      return 0;

   if (m_readPos + readSize > m_allocated)
   {
      size_t chunkSize = m_allocated - m_readPos;
      memcpy(buffer, &m_data[m_readPos], chunkSize);
      memcpy(&buffer[chunkSize], m_data, readSize - chunkSize);
      m_readPos = readSize - chunkSize;
   }
   else
   {
      memcpy(buffer, &m_data[m_readPos], readSize);
      m_readPos += readSize;
   }

   m_size -= readSize;
   return readSize;
}
