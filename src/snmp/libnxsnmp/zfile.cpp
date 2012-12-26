/* 
** NetXMS - Network Management System
** SNMP support library
** Copyright (C) 2003-2010 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
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
** File: zfile.cpp
**
**/

#include "libnxsnmp.h"

/**
 * Constants
 */
#define DATA_BUFFER_SIZE      65536

/**
 * Constructor for ZFile
 */
ZFile::ZFile(FILE *pFile, BOOL bCompress, BOOL bWrite)
{
   m_bCompress = bCompress;
   m_bWrite = bWrite;
   m_pFile = pFile;
   if (bCompress)
   {
      // Initialize compression stream
      m_stream.zalloc = Z_NULL;
      m_stream.zfree = Z_NULL;
      m_stream.opaque = Z_NULL;
      m_stream.avail_in = 0;
      m_stream.next_in = Z_NULL;
      if (bWrite)
         m_nLastZLibError = deflateInit(&m_stream, 9);
      else
         m_nLastZLibError = inflateInit(&m_stream);
      m_nBufferSize = 0;
      m_pDataBuffer = (BYTE *)malloc(DATA_BUFFER_SIZE);
      m_pCompBuffer = (BYTE *)malloc(DATA_BUFFER_SIZE);
   }
   else
   {
      m_pDataBuffer = NULL;
      m_pCompBuffer = NULL;
   }
}

/**
 * Destructor for ZFile
 */
ZFile::~ZFile()
{
   safe_free(m_pDataBuffer);
   safe_free(m_pCompBuffer);
}

/**
 * Write block to compressed file
 */
int ZFile::zwrite(void *pBuf, int nLen)
{
   int nBytes, nSrcPos, nRet;

   for(nSrcPos = 0, nRet = 0; nSrcPos < nLen; nSrcPos += nBytes)
   {
      nBytes = min(nLen - nSrcPos, DATA_BUFFER_SIZE - m_nBufferSize);
      memcpy(&m_pDataBuffer[m_nBufferSize], (BYTE *)pBuf + nSrcPos, nBytes);
      m_nBufferSize += nBytes;
      if (m_nBufferSize == DATA_BUFFER_SIZE)
      {
         // Buffer is full, compress and write it to file
         m_stream.next_in = m_pDataBuffer;
         m_stream.avail_in = DATA_BUFFER_SIZE;
         do
         {
            m_stream.next_out = m_pCompBuffer;
            m_stream.avail_out = DATA_BUFFER_SIZE;
            deflate(&m_stream, Z_NO_FLUSH);
            if (fwrite(m_pCompBuffer, 1, DATA_BUFFER_SIZE - m_stream.avail_out, m_pFile) != DATA_BUFFER_SIZE - m_stream.avail_out)
               nRet = -1;
         } while(m_stream.avail_in > 0);
         m_nBufferSize = 0;
      }
      if (nRet != -1)
         nRet += nBytes;
   }
   return nRet;
}

/**
 * Write single character to compressed file
 */
int ZFile::zputc(int ch)
{
   BYTE bt;

   bt = (BYTE)ch;
   return (zwrite(&bt, 1) == 1) ? ch : -1;
}

/**
 * Fill data buffer with new data from file if buffer is empty
 */
BOOL ZFile::fillDataBuffer()
{
   int nBytes, nRet;

   if (m_nBufferSize > 0)
      return TRUE;

   if (m_stream.avail_in == 0)
   {
      // Read more data from disk
      nBytes = (int)fread(m_pCompBuffer, 1, DATA_BUFFER_SIZE, m_pFile);
      if (nBytes <= 0)
         return FALSE;  // EOF or error

      m_stream.next_in = m_pCompBuffer;
      m_stream.avail_in = nBytes;
   }

   m_stream.next_out = m_pDataBuffer;
   m_stream.avail_out = DATA_BUFFER_SIZE;
   nRet = inflate(&m_stream, Z_NO_FLUSH);
   if ((nRet == Z_OK) || (nRet == Z_STREAM_END))
   {
      m_nBufferSize = DATA_BUFFER_SIZE - m_stream.avail_out;
      m_pBufferPos = m_pDataBuffer;
      return TRUE;
   }

   return FALSE;
}

/**
 * Read block from compressed file
 */
int ZFile::zread(void *pBuf, int nLen)
{
   int nBytes, nDstPos;

   for(nDstPos = 0; nDstPos < nLen; nDstPos += nBytes)
   {
      if (!fillDataBuffer())
         return 0;   // EOF or error
      nBytes = min(nLen - nDstPos, m_nBufferSize);
      memcpy((BYTE *)pBuf + nDstPos, m_pBufferPos, nBytes);
      m_pBufferPos += nBytes;
      m_nBufferSize -= nBytes;
   }
   return nLen;
}

/**
 * Read one character from compressed file
 */
int ZFile::zgetc()
{
   BYTE ch;

   return (zread(&ch, 1) == 1) ? ch : -1;
}

/**
 * Close compressed file
 */
int ZFile::zclose()
{
   int nRet;

   if (m_bWrite)
   {
      // Flush buffer
      if (m_nBufferSize > 0)
      {
         m_stream.next_in = m_pDataBuffer;
         m_stream.avail_in = m_nBufferSize;
         do
         {
            m_stream.next_out = m_pCompBuffer;
            m_stream.avail_out = DATA_BUFFER_SIZE;
            nRet = deflate(&m_stream, Z_FINISH);
            fwrite(m_pCompBuffer, 1, DATA_BUFFER_SIZE - m_stream.avail_out, m_pFile);
         } while(nRet != Z_STREAM_END);
      }

      deflateEnd(&m_stream);
   }
   else
   {
      inflateEnd(&m_stream);
   }
   return fclose(m_pFile);
}
