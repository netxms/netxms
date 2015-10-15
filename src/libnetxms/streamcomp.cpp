/* 
** NetXMS - Network Management System
** NetXMS Foundation Library
** Copyright (C) 2003-2015 Victor Kirhenshtein
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
** File: streamcomp.cpp
**
**/

#include "libnetxms.h"
#include "lz4.h"

/**
 * Stream compressor destructor
 */
StreamCompressor::~StreamCompressor()
{
}

/**
 * Create compressor object for given method
 */
StreamCompressor *StreamCompressor::create(NXCPCompressionMethod method, bool compress, size_t maxBlockSize)
{
   switch(method)
   {
      case NXCP_COMPRESSION_LZ4:
         return new LZ4StreamCompressor(compress, maxBlockSize);
      case NXCP_COMPRESSION_NONE:
         return new DummyStreamCompressor();
   }
   return NULL;
}

/**
 * Dummy stream compressor destructor
 */
DummyStreamCompressor::~DummyStreamCompressor()
{
}

/**
 * Dummy compressor: compress
 */
size_t DummyStreamCompressor::compress(const BYTE *in, size_t inSize, BYTE *out, size_t maxOutSize)
{
   memcpy(out, in, maxOutSize);
   return inSize;
}

/**
 * Dummy compressor: decompress
 */
size_t DummyStreamCompressor::decompress(const BYTE *in, size_t inSize, const BYTE **out)
{
   *out = in;
   return inSize;
}

/**
 * Get required compress buffer size
 */
size_t DummyStreamCompressor::compressBufferSize(size_t dataSize)
{
   return dataSize;
}

/**
 * LZ4 compressor: constructor
 */
LZ4StreamCompressor::LZ4StreamCompressor(bool compress, size_t maxBlockSize)
{
   m_maxBlockSize = maxBlockSize;
   if (compress)
   {
      m_stream.encoder = LZ4_createStream();
      m_buffer = (char *)malloc(65536);
   }
   else
   {
      m_stream.decoder = LZ4_createStreamDecode();
      m_bufferSize = maxBlockSize * 2 + 65536 + 8;
      m_buffer = (char *)malloc(m_bufferSize);
      m_bufferPos = 0;
   }
   m_compress = compress;
}

/**
 * LZ4 compressor: destructor
 */
LZ4StreamCompressor::~LZ4StreamCompressor()
{
   if (m_compress)
   {
      LZ4_freeStream(m_stream.encoder);
   }
   else
   {
      LZ4_freeStreamDecode(m_stream.decoder);
   }
   free(m_buffer);
}

/**
 * LZ4 compressor: compress
 */
size_t LZ4StreamCompressor::compress(const BYTE *in, size_t inSize, BYTE *out, size_t maxOutSize)
{
   if (!m_compress)
      return 0;   // wrong mode

   int bytes = LZ4_compress_fast_continue(m_stream.encoder, (const char *)in, (char *)out, (int)inSize, (int)maxOutSize, 1);
   if (bytes <= 0)
      return 0;

   if (LZ4_saveDict(m_stream.encoder, m_buffer, 65536) == 0)
      return 0;

   return bytes;
}

/**
 * LZ4 compressor: decompress
 */
size_t LZ4StreamCompressor::decompress(const BYTE *in, size_t inSize, const BYTE **out)
{
   if (m_compress)
      return 0;   // wrong mode

   int bytes = LZ4_decompress_safe_continue(m_stream.decoder, (const char *)in, &m_buffer[m_bufferPos], (int)inSize, (int)(m_bufferSize - m_bufferPos));
   if (bytes <= 0)
      return 0;

   *out = (BYTE *)&m_buffer[m_bufferPos];
   m_bufferPos += bytes;
   if (m_bufferPos > m_bufferSize - m_maxBlockSize)
      m_bufferPos = 0;
   return bytes;
}

/**
 * Get required compress buffer size
 */
size_t LZ4StreamCompressor::compressBufferSize(size_t dataSize)
{
   return LZ4_compressBound((int)dataSize);
}
