#include <nms_common.h>
#include <nms_util.h>
#include <nxzip.h>
#include <testtools.h>
#include <zlib.h>
#include <string>
#include <vector>

/**
 * Read little-endian integer from buffer
 */
static uint64_t ReadLE(const BYTE *p, int bytes)
{
   uint64_t v = 0;
   for(int i = bytes - 1; i >= 0; i--)
      v = (v << 8) | p[i];
   return v;
}

/**
 * Entry as read back from archive
 */
struct ReadEntry
{
   std::string name;
   uint32_t crc;
   uint64_t compressedSize;
   uint64_t uncompressedSize;
   uint64_t localHeaderOffset;
};

/**
 * Parse central directory of archive (non-ZIP64 variant, sufficient for test archives)
 */
static std::vector<ReadEntry> ReadCentralDirectory(const BYTE *archive, size_t size)
{
   std::vector<ReadEntry> entries;
   AssertTrue(size >= 22);
   const BYTE *eocd = archive + size - 22;
   AssertEquals(static_cast<uint32_t>(ReadLE(eocd, 4)), 0x06054B50u);
   uint64_t count = ReadLE(eocd + 10, 2);
   uint64_t directoryOffset = ReadLE(eocd + 16, 4);
   AssertTrue(directoryOffset < size);

   const BYTE *p = archive + directoryOffset;
   for(uint64_t i = 0; i < count; i++)
   {
      AssertEquals(static_cast<uint32_t>(ReadLE(p, 4)), 0x02014B50u);
      ReadEntry e;
      e.crc = static_cast<uint32_t>(ReadLE(p + 16, 4));
      e.compressedSize = ReadLE(p + 20, 4);
      e.uncompressedSize = ReadLE(p + 24, 4);
      uint64_t nameLength = ReadLE(p + 28, 2);
      uint64_t extraLength = ReadLE(p + 30, 2);
      uint64_t commentLength = ReadLE(p + 32, 2);
      e.localHeaderOffset = ReadLE(p + 42, 4);
      e.name.assign(reinterpret_cast<const char*>(p + 46), nameLength);
      entries.push_back(e);
      p += 46 + nameLength + extraLength + commentLength;
   }
   AssertEquals(static_cast<uint64_t>(p - archive), directoryOffset + ReadLE(eocd + 12, 4));
   return entries;
}

/**
 * Inflate entry data and check CRC and size against central directory
 */
static std::string ExtractEntry(const BYTE *archive, const ReadEntry& entry)
{
   const BYTE *header = archive + entry.localHeaderOffset;
   AssertEquals(static_cast<uint32_t>(ReadLE(header, 4)), 0x04034B50u);
   AssertEquals(static_cast<uint32_t>(ReadLE(header + 8, 2)), 8u);   // deflate
   AssertEquals(static_cast<uint32_t>(ReadLE(header + 14, 4)), entry.crc);
   uint64_t nameLength = ReadLE(header + 26, 2);
   uint64_t extraLength = ReadLE(header + 28, 2);
   AssertEquals(nameLength, static_cast<uint64_t>(entry.name.size()));
   AssertEquals(static_cast<int32_t>(memcmp(header + 30, entry.name.data(), nameLength)), 0);

   // ZIP64 extra in local header must carry real sizes
   const BYTE *extra = header + 30 + nameLength;
   AssertEquals(static_cast<uint32_t>(ReadLE(extra, 2)), 1u);
   AssertEquals(ReadLE(extra + 4, 8), entry.uncompressedSize);
   AssertEquals(ReadLE(extra + 12, 8), entry.compressedSize);

   const BYTE *data = extra + extraLength;
   std::string output;
   output.resize(entry.uncompressedSize);

   z_stream zs;
   memset(&zs, 0, sizeof(zs));
   AssertEquals(inflateInit2(&zs, -MAX_WBITS), static_cast<int32_t>(Z_OK));
   zs.next_in = const_cast<Bytef*>(data);
   zs.avail_in = static_cast<uInt>(entry.compressedSize);
   zs.next_out = reinterpret_cast<Bytef*>(&output[0]);
   zs.avail_out = static_cast<uInt>(entry.uncompressedSize);
   int rc = inflate(&zs, Z_FINISH);
   AssertTrue((rc == Z_STREAM_END) || ((rc == Z_BUF_ERROR) && (entry.uncompressedSize == 0)));
   AssertEquals(static_cast<uint64_t>(zs.total_out), entry.uncompressedSize);
   inflateEnd(&zs);

   AssertEquals(static_cast<uint32_t>(crc32(0, reinterpret_cast<const Bytef*>(output.data()), static_cast<uInt>(output.size()))), entry.crc);
   return output;
}

/**
 * Test ZIP archive writer
 */
void TestZipArchiveWriter()
{
   StartTest(_T("ZipArchiveWriter"));

   TCHAR fileName[MAX_PATH];
#ifdef _WIN32
   TCHAR tempDir[MAX_PATH];
   GetTempPath(MAX_PATH, tempDir);
   _sntprintf(fileName, MAX_PATH, _T("%snxtest-%u.zip"), tempDir, static_cast<unsigned int>(GetCurrentProcessId()));
#else
   _sntprintf(fileName, MAX_PATH, _T("/tmp/nxtest-%u.zip"), static_cast<unsigned int>(getpid()));
#endif

   std::string largeText;
   for(int i = 0; i < 200000; i++)
   {
      char line[64];
      snprintf(line, sizeof(line), "%d,2026-01-01T00:%02d:00Z,%.3f\n", i, i % 60, i * 0.125);
      largeText.append(line);
   }
   AssertTrue(largeText.size() > 5000000);   // several deflate output buffers

   ZipArchiveWriter writer;
   AssertFalse(writer.isOpen());
   AssertTrue(writer.open(fileName));
   AssertTrue(writer.isOpen());

   AssertTrue(writer.beginEntry("manifest.json"));
   AssertTrue(writer.write("{\"format\":1}"));
   AssertTrue(writer.endEntry());

   AssertTrue(writer.beginEntry("data/values.csv"));
   size_t offset = 0;
   while(offset < largeText.size())
   {
      size_t chunk = std::min(static_cast<size_t>(7777), largeText.size() - offset);
      AssertTrue(writer.write(largeText.data() + offset, chunk));
      offset += chunk;
   }
   AssertTrue(writer.endEntry());

   AssertTrue(writer.beginEntry("empty.txt"));
   AssertTrue(writer.endEntry());

   AssertFalse(writer.write("x", 1));   // no open entry
   AssertFalse(writer.endEntry());

   AssertEquals(writer.getEntryCount(), 3);
   AssertTrue(writer.close());
   AssertFalse(writer.isOpen());
   AssertFalse(writer.isError());

   size_t archiveSize;
   BYTE *archive = LoadFile(fileName, &archiveSize);
   AssertNotNull(archive);
   AssertEquals(static_cast<uint64_t>(archiveSize), writer.getArchiveSize());

   std::vector<ReadEntry> entries = ReadCentralDirectory(archive, archiveSize);
   AssertEquals(entries.size(), 3);
   AssertEquals(entries[0].name.c_str(), "manifest.json");
   AssertEquals(entries[1].name.c_str(), "data/values.csv");
   AssertEquals(entries[2].name.c_str(), "empty.txt");

   AssertEquals(ExtractEntry(archive, entries[0]).c_str(), "{\"format\":1}");
   AssertTrue(ExtractEntry(archive, entries[1]) == largeText);
   AssertTrue(entries[1].compressedSize < largeText.size() / 2);
   AssertTrue(ExtractEntry(archive, entries[2]).empty());

   MemFree(archive);
   _tremove(fileName);

   // Archive without entries is still valid
   AssertTrue(writer.open(fileName));
   AssertTrue(writer.close());
   archive = LoadFile(fileName, &archiveSize);
   AssertNotNull(archive);
   AssertEquals(archiveSize, 22);
   AssertEquals(ReadCentralDirectory(archive, archiveSize).size(), 0);
   MemFree(archive);
   _tremove(fileName);

   EndTest();
}
