/*
** NetXMS - Network Management System
** Copyright (C) 2017-2025 Raden Solutions
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: dfile_info.cpp
**
**/

#include "libnxagent.h"
#include "nxstat.h"

/**
 * Constructor for DownloadFileInfo class only stores given data
 */
DownloadFileInfo::DownloadFileInfo(const TCHAR *name, time_t fileModificationTime)
{
   m_fileName = MemCopyString(name);
   m_fileModificationTime = fileModificationTime;
   m_fileHandle = -1;
   m_compressor = nullptr;
   m_lastUpdateTime = time(nullptr);
}

/**
 * Destructor
 */
DownloadFileInfo::~DownloadFileInfo()
{
   if (m_fileHandle != -1)
      close(false); // calling DownloadFileInfo::close, not system function
   MemFree(m_fileName);
   delete m_compressor;
}

/**
 * Opens file and returns if it was successfully
 */
bool DownloadFileInfo::open(bool append)
{
   TCHAR tempFileName[MAX_PATH];
   _tcslcpy(tempFileName, m_fileName, MAX_PATH);
   _tcslcat(tempFileName, _T(".part"), MAX_PATH);

   if (append)
   {
      NX_STAT_STRUCT fs;
      if (CALL_STAT(tempFileName, &fs) != 0)
      {
         CopyFileOrDirectory(m_fileName, tempFileName);
      }
   }

   m_fileHandle = _topen(tempFileName, O_CREAT | O_WRONLY | O_BINARY |
         (append ? O_APPEND : O_TRUNC), S_IRUSR | S_IWUSR);
   return m_fileHandle != -1;
}

/**
 * Function that writes incoming data to file
 */
bool DownloadFileInfo::write(const BYTE *data, size_t dataSize, bool compressedStream)
{
   static const TCHAR *compressionMethods[] = { _T("NONE"), _T("LZ4"), _T("DEFLATE") };

   m_lastUpdateTime = time(nullptr);
   if (dataSize == 0)
      return true;

   if (!compressedStream)
      return _write(m_fileHandle, data, (int)dataSize) == dataSize;

   if (m_compressor == nullptr)
   {
      NXCPStreamCompressionMethod method = (NXCPStreamCompressionMethod)(*data);
      m_compressor = StreamCompressor::create(method, false, FILE_BUFFER_SIZE);
      const TCHAR *methodName = (((int)method >= 0) && ((int)method <= 2)) ? compressionMethods[method] : _T("UNKNOWN");
      if (m_compressor != nullptr)
      {
         nxlog_debug(5, _T("DownloadFileInfo(%s): created stream compressor for method %s"), m_fileName, methodName);
      }
      else
      {
         nxlog_debug(5, _T("DownloadFileInfo(%s): unable to create stream compressor for method %s"), m_fileName, methodName);
         return false;
      }
   }

   const BYTE *uncompressedData;
   size_t uncompressedDataSize = m_compressor->decompress(data + 4, dataSize - 4, &uncompressedData);
   if (uncompressedDataSize != static_cast<int>(ntohs(*reinterpret_cast<const uint16_t*>(data + 2))))
   {
      // decompressed block size validation failed
      nxlog_debug(5, _T("DownloadFileInfo(%s): decompression failure (size %d should be %d)"),
               m_fileName, (int)uncompressedDataSize, (int)ntohs(*((uint16_t *)(data + 2))));
      return false;
   }

   return _write(m_fileHandle, uncompressedData, (int)uncompressedDataSize) == uncompressedDataSize;
}

/**
 * Closes file and changes it's date if required
 */
uint32_t DownloadFileInfo::getFileInfo(NXCPMessage *response, const TCHAR *fileName)
{
   TCHAR tempFileName[MAX_PATH];
   _tcslcpy(tempFileName, fileName, MAX_PATH);
   _tcslcat(tempFileName, _T(".part"), MAX_PATH);

   uint64_t size = 0;
   BYTE hash[MD5_DIGEST_SIZE];
   memset(hash, 0, MD5_DIGEST_SIZE);
   NX_STAT_STRUCT fs;
   uint32_t rcc = ERR_SUCCESS;

   if (CALL_STAT(tempFileName, &fs) == 0)
   {
      CalculateFileMD5Hash(tempFileName, hash);
      size = fs.st_size;
      rcc = ERR_FILE_APPEND_POSSIBLE;
   }
   else
   {
      if (CALL_STAT(fileName, &fs) == 0)
      {
         CalculateFileMD5Hash(fileName, hash);
         size = fs.st_size;
         rcc = ERR_FILE_APPEND_POSSIBLE;
      }
   }

   if (rcc == ERR_FILE_APPEND_POSSIBLE)
   {
      response->setField(VID_HASH_MD5, hash, MD5_DIGEST_SIZE);
      response->setField(VID_FILE_SIZE, size);
   }
   response->setField(VID_RCC, rcc);
   return rcc;
}

/**
 * Closes file and changes it's date if required
 */
void DownloadFileInfo::close(bool success, bool deleteOnFailure)
{
   _close(m_fileHandle);
   m_fileHandle = -1;

   TCHAR tempFileName[MAX_PATH];
   _tcslcpy(tempFileName, m_fileName, MAX_PATH);
   _tcslcat(tempFileName, _T(".part"), MAX_PATH);

   if (success)
   {
      _tremove(m_fileName);
      if (_trename(tempFileName, m_fileName) == 0)
      {
         if (m_fileModificationTime != 0)
            SetLastModificationTime(m_fileName, m_fileModificationTime);
      }
   }
   else if (deleteOnFailure)
   {
      // Remove received file part in case of failure
      _tremove(tempFileName);
   }
}
