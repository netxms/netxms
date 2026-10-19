/*
** NetXMS - Network Management System
** NetXMS Foundation Library
** Copyright (C) 2003-2026 Raden Solutions
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
** File: zipwriter.cpp
**
**/

#include "libnetxms.h"
#include <nxzip.h>
#include <zlib.h>

#define OUTPUT_BUFFER_SIZE 65536

#define ZIP_VERSION_NEEDED       45    // 4.5 - ZIP64
#define ZIP_FLAG_UTF8_NAMES      0x0800
#define ZIP_METHOD_DEFLATE       8
#define ZIP64_LOCAL_EXTRA_SIZE   20    // header ID (2) + size (2) + uncompressed (8) + compressed (8)

/**
 * Little-endian field writer for fixed-size headers
 */
class HeaderBuilder
{
private:
   BYTE *m_buffer;
   size_t m_size;
   size_t m_position;

public:
   HeaderBuilder(BYTE *buffer, size_t size) : m_buffer(buffer), m_size(size), m_position(0) {}

   void u16(uint16_t v)
   {
      m_buffer[m_position++] = static_cast<BYTE>(v & 0xFF);
      m_buffer[m_position++] = static_cast<BYTE>((v >> 8) & 0xFF);
   }

   void u32(uint32_t v)
   {
      u16(static_cast<uint16_t>(v & 0xFFFF));
      u16(static_cast<uint16_t>((v >> 16) & 0xFFFF));
   }

   void u64(uint64_t v)
   {
      u32(static_cast<uint32_t>(v & 0xFFFFFFFF));
      u32(static_cast<uint32_t>((v >> 32) & 0xFFFFFFFF));
   }

   size_t size() const { return m_position; }
};

/**
 * Seek to absolute position in file (64 bit)
 */
static inline bool SeekTo(int fd, uint64_t position)
{
#ifdef _WIN32
   return _lseeki64(fd, static_cast<int64_t>(position), SEEK_SET) == static_cast<int64_t>(position);
#else
   return lseek(fd, static_cast<off_t>(position), SEEK_SET) == static_cast<off_t>(position);
#endif
}

/**
 * Constructor
 */
ZipArchiveWriter::ZipArchiveWriter()
{
   m_fd = -1;
   m_position = 0;
   m_entryOpen = false;
   m_error = false;
   m_zstream = nullptr;
   m_outputBuffer = nullptr;
}

/**
 * Destructor. Closes archive if still open; archive left open at destruction is finalized on a best-effort basis.
 */
ZipArchiveWriter::~ZipArchiveWriter()
{
   if (m_fd != -1)
      close();
   MemFree(m_outputBuffer);
   if (m_zstream != nullptr)
   {
      deflateEnd(static_cast<z_stream*>(m_zstream));
      delete static_cast<z_stream*>(m_zstream);
   }
}

/**
 * Create archive file. Existing file is truncated.
 */
bool ZipArchiveWriter::open(const TCHAR *fileName)
{
   if (m_fd != -1)
      return false;

   m_fd = _topen(fileName, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, S_IRUSR | S_IWUSR);
   if (m_fd == -1)
      return false;

   m_position = 0;
   m_entries.clear();
   m_entryOpen = false;
   m_error = false;
   if (m_outputBuffer == nullptr)
      m_outputBuffer = MemAllocArrayNoInit<BYTE>(OUTPUT_BUFFER_SIZE);
   return true;
}

/**
 * Write raw bytes at current position
 */
bool ZipArchiveWriter::writeRaw(const void *data, size_t size)
{
   const BYTE *p = static_cast<const BYTE*>(data);
   while(size > 0)
   {
      int bytes = _write(m_fd, p, static_cast<unsigned int>(std::min(size, static_cast<size_t>(0x40000000))));
      if (bytes <= 0)
      {
         m_error = true;
         return false;
      }
      p += bytes;
      size -= bytes;
      m_position += bytes;
   }
   return true;
}

/**
 * Write local file header for entry at current position. Sizes and CRC are set to ZIP64 markers and
 * filled in by patchEntryHeader() once the entry is complete.
 */
bool ZipArchiveWriter::writeEntryHeader(const Entry& entry)
{
   BYTE buffer[30 + ZIP64_LOCAL_EXTRA_SIZE];
   HeaderBuilder h(buffer, sizeof(buffer));
   h.u32(0x04034B50);                           // local file header signature
   h.u16(ZIP_VERSION_NEEDED);
   h.u16(ZIP_FLAG_UTF8_NAMES);
   h.u16(ZIP_METHOD_DEFLATE);
   h.u16(entry.dosTime);
   h.u16(entry.dosDate);
   h.u32(entry.crc);
   h.u32(0xFFFFFFFF);                           // compressed size: in ZIP64 extra
   h.u32(0xFFFFFFFF);                           // uncompressed size: in ZIP64 extra
   h.u16(static_cast<uint16_t>(entry.name.size()));
   h.u16(ZIP64_LOCAL_EXTRA_SIZE);
   // ZIP64 extended information extra field
   h.u16(0x0001);
   h.u16(16);
   h.u64(entry.uncompressedSize);
   h.u64(entry.compressedSize);
   return writeRaw(buffer, 30) && writeRaw(entry.name.data(), entry.name.size()) && writeRaw(buffer + 30, ZIP64_LOCAL_EXTRA_SIZE);
}

/**
 * Rewrite CRC and sizes of completed entry in its local file header, then return to the end of the archive
 */
bool ZipArchiveWriter::patchEntryHeader(const Entry& entry)
{
   uint64_t endPosition = m_position;

   BYTE crc[4];
   HeaderBuilder c(crc, sizeof(crc));
   c.u32(entry.crc);

   BYTE sizes[16];
   HeaderBuilder s(sizes, sizeof(sizes));
   s.u64(entry.uncompressedSize);
   s.u64(entry.compressedSize);

   bool success = SeekTo(m_fd, entry.localHeaderOffset + 14) && (_write(m_fd, crc, 4) == 4) &&
      SeekTo(m_fd, entry.localHeaderOffset + 30 + entry.name.size() + 4) && (_write(m_fd, sizes, 16) == 16) &&
      SeekTo(m_fd, endPosition);
   if (!success)
      m_error = true;
   return success;
}

/**
 * Start new entry
 */
bool ZipArchiveWriter::beginEntry(const char *name)
{
   if ((m_fd == -1) || m_entryOpen || m_error || (name == nullptr) || (*name == 0) || (strlen(name) > 0xFFFF))
      return false;

   if (m_zstream == nullptr)
   {
      z_stream *zs = new z_stream;
      memset(zs, 0, sizeof(z_stream));
      if (deflateInit2(zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -MAX_WBITS, 8, Z_DEFAULT_STRATEGY) != Z_OK)
      {
         delete zs;
         m_error = true;
         return false;
      }
      m_zstream = zs;
   }
   else if (deflateReset(static_cast<z_stream*>(m_zstream)) != Z_OK)
   {
      m_error = true;
      return false;
   }

   Entry entry;
   entry.name = name;
   entry.crc = static_cast<uint32_t>(crc32(0, nullptr, 0));
   entry.compressedSize = 0;
   entry.uncompressedSize = 0;
   entry.localHeaderOffset = m_position;

   time_t now = time(nullptr);
   struct tm ltm;
#if HAVE_LOCALTIME_R
   localtime_r(&now, &ltm);
#else
   memcpy(&ltm, localtime(&now), sizeof(struct tm));
#endif
   entry.dosTime = static_cast<uint16_t>((ltm.tm_hour << 11) | (ltm.tm_min << 5) | (ltm.tm_sec / 2));
   entry.dosDate = static_cast<uint16_t>(((std::max(ltm.tm_year - 80, 0)) << 9) | ((ltm.tm_mon + 1) << 5) | ltm.tm_mday);

   if (!writeEntryHeader(entry))
      return false;

   m_entries.push_back(entry);
   m_entryOpen = true;
   return true;
}

/**
 * Run deflate with given flush mode until all pending output is written
 */
bool ZipArchiveWriter::flushDeflate(int flush)
{
   z_stream *zs = static_cast<z_stream*>(m_zstream);
   Entry& entry = m_entries.back();
   do
   {
      zs->next_out = m_outputBuffer;
      zs->avail_out = OUTPUT_BUFFER_SIZE;
      int rc = deflate(zs, flush);
      if ((rc != Z_OK) && (rc != Z_STREAM_END) && (rc != Z_BUF_ERROR))
      {
         m_error = true;
         return false;
      }
      size_t produced = OUTPUT_BUFFER_SIZE - zs->avail_out;
      if (produced > 0)
      {
         if (!writeRaw(m_outputBuffer, produced))
            return false;
         entry.compressedSize += produced;
      }
      if (rc == Z_STREAM_END)
         break;
   } while((zs->avail_in > 0) || (zs->avail_out == 0));
   return true;
}

/**
 * Write data to current entry
 */
bool ZipArchiveWriter::write(const void *data, size_t size)
{
   if (!m_entryOpen || m_error)
      return false;
   if (size == 0)
      return true;

   Entry& entry = m_entries.back();
   entry.crc = static_cast<uint32_t>(crc32(entry.crc, static_cast<const Bytef*>(data), static_cast<uInt>(size)));
   entry.uncompressedSize += size;

   z_stream *zs = static_cast<z_stream*>(m_zstream);
   const BYTE *p = static_cast<const BYTE*>(data);
   while(size > 0)
   {
      uInt chunk = static_cast<uInt>(std::min(size, static_cast<size_t>(0x40000000)));
      zs->next_in = const_cast<Bytef*>(p);
      zs->avail_in = chunk;
      if (!flushDeflate(Z_NO_FLUSH))
         return false;
      p += chunk;
      size -= chunk;
   }
   return true;
}

/**
 * Finish current entry
 */
bool ZipArchiveWriter::endEntry()
{
   if (!m_entryOpen || m_error)
      return false;

   z_stream *zs = static_cast<z_stream*>(m_zstream);
   zs->next_in = nullptr;
   zs->avail_in = 0;
   if (!flushDeflate(Z_FINISH))
      return false;

   m_entryOpen = false;
   return patchEntryHeader(m_entries.back());
}

/**
 * Write central directory and end of central directory records
 */
bool ZipArchiveWriter::writeCentralDirectory()
{
   uint64_t directoryOffset = m_position;
   bool needZip64 = (m_entries.size() > 0xFFFF) || (directoryOffset >= 0xFFFFFFFF);

   for(const Entry& entry : m_entries)
   {
      bool largeSizes = (entry.compressedSize >= 0xFFFFFFFF) || (entry.uncompressedSize >= 0xFFFFFFFF);
      bool largeOffset = entry.localHeaderOffset >= 0xFFFFFFFF;
      needZip64 = needZip64 || largeSizes || largeOffset;

      BYTE extra[4 + 24];
      HeaderBuilder e(extra, sizeof(extra));
      if (largeSizes || largeOffset)
      {
         e.u16(0x0001);
         e.u16(static_cast<uint16_t>((largeSizes ? 16 : 0) + (largeOffset ? 8 : 0)));
         if (largeSizes)
         {
            e.u64(entry.uncompressedSize);
            e.u64(entry.compressedSize);
         }
         if (largeOffset)
            e.u64(entry.localHeaderOffset);
      }

      BYTE buffer[46];
      HeaderBuilder h(buffer, sizeof(buffer));
      h.u32(0x02014B50);                        // central directory file header signature
      h.u16(ZIP_VERSION_NEEDED);                // version made by
      h.u16(ZIP_VERSION_NEEDED);                // version needed to extract
      h.u16(ZIP_FLAG_UTF8_NAMES);
      h.u16(ZIP_METHOD_DEFLATE);
      h.u16(entry.dosTime);
      h.u16(entry.dosDate);
      h.u32(entry.crc);
      h.u32(largeSizes ? 0xFFFFFFFF : static_cast<uint32_t>(entry.compressedSize));
      h.u32(largeSizes ? 0xFFFFFFFF : static_cast<uint32_t>(entry.uncompressedSize));
      h.u16(static_cast<uint16_t>(entry.name.size()));
      h.u16(static_cast<uint16_t>(e.size()));   // extra field length
      h.u16(0);                                 // comment length
      h.u16(0);                                 // disk number start
      h.u16(0);                                 // internal attributes
      h.u32(0);                                 // external attributes
      h.u32(largeOffset ? 0xFFFFFFFF : static_cast<uint32_t>(entry.localHeaderOffset));

      if (!writeRaw(buffer, 46) || !writeRaw(entry.name.data(), entry.name.size()) || ((e.size() > 0) && !writeRaw(extra, e.size())))
         return false;
   }

   uint64_t directorySize = m_position - directoryOffset;
   needZip64 = needZip64 || (directorySize >= 0xFFFFFFFF);

   if (needZip64)
   {
      uint64_t zip64EndOffset = m_position;

      BYTE buffer[56 + 20];
      HeaderBuilder h(buffer, sizeof(buffer));
      h.u32(0x06064B50);                        // ZIP64 end of central directory record
      h.u64(44);                                // size of remaining record
      h.u16(ZIP_VERSION_NEEDED);
      h.u16(ZIP_VERSION_NEEDED);
      h.u32(0);                                 // this disk
      h.u32(0);                                 // disk with central directory
      h.u64(m_entries.size());
      h.u64(m_entries.size());
      h.u64(directorySize);
      h.u64(directoryOffset);
      h.u32(0x07064B50);                        // ZIP64 end of central directory locator
      h.u32(0);
      h.u64(zip64EndOffset);
      h.u32(1);                                 // total number of disks
      if (!writeRaw(buffer, h.size()))
         return false;
   }

   BYTE buffer[22];
   HeaderBuilder h(buffer, sizeof(buffer));
   h.u32(0x06054B50);                           // end of central directory record
   h.u16(0);
   h.u16(0);
   h.u16(needZip64 ? 0xFFFF : static_cast<uint16_t>(m_entries.size()));
   h.u16(needZip64 ? 0xFFFF : static_cast<uint16_t>(m_entries.size()));
   h.u32(needZip64 ? 0xFFFFFFFF : static_cast<uint32_t>(directorySize));
   h.u32(needZip64 ? 0xFFFFFFFF : static_cast<uint32_t>(directoryOffset));
   h.u16(0);                                    // comment length
   return writeRaw(buffer, 22);
}

/**
 * Finalize and close archive. Returns false if any write failed; the file is closed in either case.
 */
bool ZipArchiveWriter::close()
{
   if (m_fd == -1)
      return false;

   if (m_entryOpen)
      endEntry();

   bool success = !m_error && writeCentralDirectory();
   _close(m_fd);
   m_fd = -1;
   return success;
}
