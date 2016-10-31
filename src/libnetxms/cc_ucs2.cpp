/*
 ** NetXMS - Network Management System
 ** Copyright (C) 2003-2016 Raden Solutions
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
 ** File: cc_ucs2.cpp
 **
 **/

#include "libnetxms.h"
#include "unicode_cc.h"

/**
 * Convert UCS-2 to UCS-4 - internal dumb method
 */
static int __internal_ucs2_to_ucs4(const UCS2CHAR *src, int srcLen, UCS4CHAR *dst, int dstLen)
{
   int i, len;

   len = (int) ((srcLen == -1) ? ucs2_strlen(src) : srcLen);
   if (len > (int) dstLen - 1)
      len = (int) dstLen - 1;
   for(i = 0; i < len; i++)
      dst[i] = (UCS4CHAR)src[i];
   dst[i] = 0;
   return len;
}

/**
 * Convert UCS-2 to UCS-4
 */
int LIBNETXMS_EXPORTABLE ucs2_to_ucs4(const UCS2CHAR *src, int srcLen, UCS4CHAR *dst, int dstLen)
{
#if !defined(__DISABLE_ICONV) && !defined(_WIN32)
   iconv_t cd;
   const char *inbuf;
   char *outbuf;
   size_t count, inbytes, outbytes;

   cd = IconvOpen(UCS4_CODEPAGE_NAME, UCS2_CODEPAGE_NAME);
   if (cd == (iconv_t) (-1))
   {
      return __internal_ucs2_to_ucs4(src, srcLen, dst, dstLen);
   }

   inbuf = (const char *) src;
   inbytes = ((srcLen == -1) ? ucs2_strlen(src) + 1 : (size_t) srcLen) * sizeof(UCS2CHAR);
   outbuf = (char *) dst;
   outbytes = (size_t) dstLen * sizeof(UCS4CHAR);
   count = iconv(cd, (ICONV_CONST char **) &inbuf, &inbytes, &outbuf, &outbytes);
   IconvClose(cd);

   if (count == (size_t) - 1)
   {
      if (errno == EILSEQ)
      {
         count = (dstLen * sizeof(UCS4CHAR) - outbytes) / sizeof(UCS4CHAR);
      }
      else
      {
         count = 0;
      }
   }
   else
   {
      count = (dstLen * sizeof(UCS4CHAR) - outbytes) / sizeof(UCS4CHAR);
   }
   if ((srcLen == -1) && (outbytes >= sizeof(UCS4CHAR)))
   {
      *((UCS4CHAR *) outbuf) = 0;
   }

   return (int)count;
#else
   return __internal_ucs2_to_ucs4(src, srcLen, dst, dstLen);
#endif
}

#if !defined(_WIN32) && !defined(UNICODE_UCS2)

/**
 * Convert UCS-2 to UTF-8 using stub (no actual conversion for character codes above 0x007F)
 */
static int __internal_ucs2_to_utf8(const UCS2CHAR *src, int srcLen, char *dst, int dstLen)
{
   const UCS2CHAR *psrc;
   char *pdst;
   int pos, size;

   size = (srcLen == -1) ? ucs2_strlen(src) : srcLen;
   if (size >= dstLen)
      size = dstLen - 1;
   for(psrc = src, pos = 0, pdst = dst; pos < size; pos++, psrc++, pdst++)
      *pdst = (*psrc < 128) ? (char) (*psrc) : '?';
   *pdst = 0;
   return size;
}

/**
 * Convert UCS-2 to UTF-8
 */
int LIBNETXMS_EXPORTABLE ucs2_to_utf8(const UCS2CHAR *src, int srcLen, char *dst, int dstLen)
{
#if HAVE_ICONV && !defined(__DISABLE_ICONV)
   iconv_t cd;
   const char *inbuf;
   char *outbuf;
   size_t count, inbytes, outbytes;

   cd = IconvOpen("UTF-8", UCS2_CODEPAGE_NAME);
   if (cd == (iconv_t) (-1))
   {
      return __internal_ucs2_to_utf8(src, srcLen, dst, dstLen);
   }

   inbuf = (const char *) src;
   inbytes = ((srcLen == -1) ? ucs2_strlen(src) + 1 : (size_t) srcLen) * sizeof(UCS2CHAR);
   outbuf = (char *) dst;
   outbytes = (size_t) dstLen;
   count = iconv(cd, (ICONV_CONST char **) &inbuf, &inbytes, &outbuf, &outbytes);
   IconvClose(cd);

   if (count == (size_t) - 1)
   {
      if (errno == EILSEQ)
      {
         count = (dstLen * sizeof(char) - outbytes) / sizeof(char);
      }
      else
      {
         count = 0;
      }
   }
   else
   {
      count = dstLen - outbytes;
   }
   if ((srcLen == -1) && (outbytes >= sizeof(char)))
   {
      *((char *) outbuf) = 0;
   }

   return (int)count;
#else
   return __internal_ucs2_to_utf8(src, srcLen, dst, dstLen);
#endif
}

/**
 * Convert UCS-2 to multibyte using stub (no actual conversion for character codes above 0x007F)
 */
static int __internal_ucs2_to_mb(const UCS2CHAR *src, int srcLen, char *dst, int dstLen)
{
   const UCS2CHAR *psrc;
   char *pdst;
   int pos, size;

   size = (srcLen == -1) ? (int) ucs2_strlen(src) : srcLen;
   if (size >= dstLen)
      size = dstLen - 1;

   for(psrc = src, pos = 0, pdst = dst; pos < size; pos++, psrc++, pdst++)
      *pdst = (*psrc < 256) ? (char) (*psrc) : '?';
   *pdst = 0;

   return size;
}

/**
 * Convert UCS-2 to multibyte
 */
int LIBNETXMS_EXPORTABLE ucs2_to_mb(const UCS2CHAR *src, int srcLen, char *dst, int dstLen)
{
#if HAVE_ICONV && !defined(__DISABLE_ICONV)
   iconv_t cd;
   const char *inbuf;
   char *outbuf;
   size_t count, inbytes, outbytes;

   cd = IconvOpen(g_cpDefault, UCS2_CODEPAGE_NAME);
   if (cd == (iconv_t) (-1))
   {
      return __internal_ucs2_to_mb(src, srcLen, dst, dstLen);
   }

   inbuf = (const char *) src;
   inbytes = ((srcLen == -1) ? ucs2_strlen(src) + 1 : (size_t) srcLen) * sizeof(UCS2CHAR);
   outbuf = (char *) dst;
   outbytes = (size_t) dstLen;
   count = iconv(cd, (ICONV_CONST char **) &inbuf, &inbytes, &outbuf, &outbytes);
   IconvClose(cd);

   if (count == (size_t) - 1)
   {
      if (errno == EILSEQ)
      {
         count = (dstLen * sizeof(char) - outbytes) / sizeof(char);
      }
      else
      {
         count = 0;
      }
   }
   if ((srcLen == -1) && (outbytes >= sizeof(char)))
   {
      *((char *) outbuf) = 0;
   }

   return (int)count;
#else
   return __internal_ucs2_to_mb(src, srcLen, dst, dstLen);
#endif
}

#endif
