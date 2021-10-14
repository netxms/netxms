#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include <stdio.h>
#include <wchar.h>
#include <zlib.h>

/**
 * Deflate given file stream
 */
static int DeflateFileStream(FILE *source, FILE *dest)
{
   /* allocate deflate state */
   z_stream strm;
   strm.zalloc = Z_NULL;
   strm.zfree = Z_NULL;
   strm.opaque = Z_NULL;

   int ret = deflateInit2(&strm, Z_BEST_COMPRESSION, Z_DEFLATED, 31, 8, Z_DEFAULT_STRATEGY);
   if (ret != Z_OK)
      return ret;

   /* compress until end of file */
   BYTE in[16384];
   BYTE out[16384];
   int flush;
   do
   {
      strm.avail_in = static_cast<uInt>(fread(in, 1, 16384, source));
      if (ferror(source))
      {
         deflateEnd(&strm);
         return Z_ERRNO;
      }
      flush = feof(source) ? Z_FINISH : Z_NO_FLUSH;
      strm.next_in = in;

      do
      {
         strm.avail_out = 16384;
         strm.next_out = out;
         ret = deflate(&strm, flush);
         if (ret == Z_STREAM_ERROR)
         {
            deflateEnd(&strm);
            return ret;
         }
         int bytesOut = 16384 - strm.avail_out;
         if ((fwrite(out, 1, bytesOut, dest) != bytesOut) || ferror(dest))
         {
            deflateEnd(&strm);
            return Z_ERRNO;
         }
      } while (strm.avail_out == 0);

   } while (flush != Z_FINISH);

   deflateEnd(&strm);
   return Z_OK;
}

/**
 * Deflate given file
 */
bool DeflateFile(const wchar_t *inputFile)
{
   wchar_t outputFile[MAX_PATH];
   _snwprintf(outputFile, MAX_PATH, L"%s.gz", inputFile);

   FILE *in = _wfopen(inputFile, L"rb");
   if (in == nullptr)
      return false;

   FILE *out = _wfopen(outputFile, L"wb");
   if (out == nullptr)
   {
      fclose(in);
      return false;
   }

   int rc = DeflateFileStream(in, out);

   fclose(in);
   fclose(out);
   return rc == Z_OK;
}
