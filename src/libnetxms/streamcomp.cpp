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
#include <nxcpapi.h>
#include <zlib.h>
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
StreamCompressor *StreamCompressor::create(NXCPStreamCompressionMethod method, bool compress, size_t maxBlockSize)
{
   switch(method)
   {
      case NXCP_STREAM_COMPRESSION_DEFLATE:
         return new DeflateStreamCompressor(compress, maxBlockSize);
      case NXCP_STREAM_COMPRESSION_LZ4:
         return new LZ4StreamCompressor(compress, maxBlockSize);
      case NXCP_STREAM_COMPRESSION_NONE:
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

/**
 * Create new DEFLATE stream compressor
 */
DeflateStreamCompressor::DeflateStreamCompressor(bool compress, size_t maxBlockSize)
{
   m_compress = compress;
   m_stream = (z_stream_s *)malloc(sizeof(z_stream_s));
   m_stream->zalloc = Z_NULL;
   m_stream->zfree = Z_NULL;
   m_stream->opaque = Z_NULL;
   m_stream->avail_in = 0;
   m_stream->next_in = Z_NULL;
   if (compress)
   {
      m_buffer = NULL;
      if (deflateInit(m_stream, 9) != Z_OK)
      {
         nxlog_debug(5, _T("DeflateStreamCompressor: deflateInit() failed"));
         free(m_stream);
         m_stream = NULL;
      }
   }
   else
   {
      m_bufferSize = maxBlockSize * 2;
      m_buffer = (BYTE *)malloc(m_bufferSize);
      if (inflateInit(m_stream) != Z_OK)
      {
         nxlog_debug(5, _T("DeflateStreamCompressor: inflateInit() failed"));
         free(m_stream);
         m_stream = NULL;
      }
   }
}

/**
 * DEFLATE stream compressor destructor
 */
DeflateStreamCompressor::~DeflateStreamCompressor()
{
   if (m_stream != NULL)
   {
      if (m_compress)
         deflateEnd(m_stream);
      else
         inflateEnd(m_stream);
      free(m_stream);
   }
   free(m_buffer);
}

/**
 * DEFLATE compressor: compress
 */
size_t DeflateStreamCompressor::compress(const BYTE *in, size_t inSize, BYTE *out, size_t maxOutSize)
{
   if (m_stream == NULL)
      return 0;

   m_stream->avail_in = (uInt)inSize;
   m_stream->next_in = (BYTE *)in;
   m_stream->avail_out = (uInt)maxOutSize;
   m_stream->next_out = out;
   if (deflate(m_stream, Z_SYNC_FLUSH) != Z_OK)
   {
      nxlog_debug(5, _T("DeflateStreamCompressor: deflate() failed"));
      return 0;
   }
   return maxOutSize - m_stream->avail_out;
}

/**
 * DEFLATE compressor: decompress
 */
size_t DeflateStreamCompressor::decompress(const BYTE *in, size_t inSize, const BYTE **out)
{
   if (m_stream == NULL)
      return 0;

   m_stream->avail_in = (uInt)inSize;
   m_stream->next_in = (BYTE *)in;
   m_stream->avail_out = (uInt)m_bufferSize;
   m_stream->next_out = m_buffer;
   int rc = inflate(m_stream, Z_SYNC_FLUSH);
   if ((rc != Z_OK) && (rc != Z_STREAM_END))
   {
      nxlog_debug(5, _T("DeflateStreamCompressor: inflate() failed"));
      return 0;
   }
   *out = m_buffer;
   return m_bufferSize - m_stream->avail_out;
}

/**
 * Get required compress buffer size
 */
size_t DeflateStreamCompressor::compressBufferSize(size_t dataSize)
{
   return (m_stream != NULL) ? deflateBound(m_stream, (uLong)dataSize) : 0;
}
