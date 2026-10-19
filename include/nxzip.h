/*
** NetXMS - Network Management System
** Utility Library
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
** File: nxzip.h
**
**/

#ifndef _nxzip_h_
#define _nxzip_h_

#include <nms_common.h>
#include <nms_util.h>
#include <vector>
#include <string>

/**
 * Streaming ZIP archive writer. Entries are written sequentially: beginEntry(), any number of write() calls,
 * endEntry(). Entry data is deflated on the fly, so entry size does not need to be known in advance.
 * Archive is ZIP64 capable (entries and archives larger than 4 GB, more than 65535 entries).
 * Entry names are UTF-8 with forward slashes as path separators.
 */
class LIBNETXMS_EXPORTABLE ZipArchiveWriter
{
private:
   struct Entry
   {
      std::string name;
      uint32_t crc;
      uint64_t compressedSize;
      uint64_t uncompressedSize;
      uint64_t localHeaderOffset;
      uint16_t dosTime;
      uint16_t dosDate;
   };

   int m_fd;
   uint64_t m_position;
   std::vector<Entry> m_entries;
   bool m_entryOpen;
   bool m_error;
   void *m_zstream;        // z_stream*, kept opaque to avoid zlib.h in the header
   BYTE *m_outputBuffer;

   bool writeRaw(const void *data, size_t size);
   bool flushDeflate(int flush);
   bool writeEntryHeader(const Entry& entry);
   bool patchEntryHeader(const Entry& entry);
   bool writeCentralDirectory();

public:
   ZipArchiveWriter();
   ~ZipArchiveWriter();

   bool open(const TCHAR *fileName);
   bool beginEntry(const char *name);
   bool write(const void *data, size_t size);
   bool write(const char *text) { return write(text, strlen(text)); }
   bool write(const std::string& text) { return write(text.data(), text.size()); }
   bool endEntry();
   bool close();

   bool isOpen() const { return m_fd != -1; }
   bool isError() const { return m_error; }
   int getEntryCount() const { return static_cast<int>(m_entries.size()); }
   uint64_t getArchiveSize() const { return m_position; }
};

#endif   /* _nxzip_h_ */
