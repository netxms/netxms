/*
** NetXMS - Network Management System
** Copyright (C) 2003-2017 Victor Kirhenshtein
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
** File: ztools.cpp
**
**/

#include "libnetxms.h"
#include <zlib.h>

/**
 * Deflate given file stream
 */
int LIBNETXMS_EXPORTABLE DeflateFileStream(FILE *source, FILE *dest, bool gzipFormat)
{
   /* allocate deflate state */
   z_stream strm;
   strm.zalloc = Z_NULL;
   strm.zfree = Z_NULL;
   strm.opaque = Z_NULL;

   int ret = deflateInit2(&strm, Z_BEST_COMPRESSION, Z_DEFLATED, gzipFormat ? (15 + 16) : 15, 8, Z_DEFAULT_STRATEGY);
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
      } while(strm.avail_out == 0);
 
   } while (flush != Z_FINISH);

   deflateEnd(&strm);
   return Z_OK;
}

/**
 * Deflate given file
 */
bool LIBNETXMS_EXPORTABLE DeflateFile(const TCHAR *inputFile, const TCHAR *outputFile)
{
   TCHAR realOutputFile[MAX_PATH];
   if (outputFile != NULL)
      _tcslcpy(realOutputFile, outputFile, MAX_PATH);
   else
      _sntprintf(realOutputFile, MAX_PATH, _T("%s.gz"), inputFile);

#ifdef _WIN32
   FILE *in = _tfopen(inputFile, _T("rb"));
#else
   FILE *in = _tfopen(inputFile, _T("r"));
#endif
   if (in == NULL)
      return false;

#ifdef _WIN32
   FILE *out = _tfopen(realOutputFile, _T("wb"));
#else
   FILE *out = _tfopen(realOutputFile, _T("w"));
#endif
   if (out == NULL)
   {
      fclose(in);
      return false;
   }

   int rc = DeflateFileStream(in, out, true);

   fclose(in);
   fclose(out);
   return rc == Z_OK;
}
