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
 ** File: cc_ucs4.cpp
 **
 **/

#include "libnetxms.h"
#include "unicode_cc.h"

/**
 * Convert UCS-4 to UCS-2 - internal dumb method
 */
static int __internal_ucs4_to_ucs2(const UCS4CHAR *src, int srcLen, UCS2CHAR *dst, int dstLen)
{
   int i, len;

   len = (int)((srcLen == -1) ? ucs4_strlen(src) : srcLen);
   if (len > (int)dstLen - 1)
      len = (int)dstLen - 1;
   for(i = 0; i < len; i++)
      dst[i] = (UCS2CHAR)src[i];
   dst[i] = 0;
   return len;
}

/**
 * Convert UCS-4 to UCS-2
 */
int LIBNETXMS_EXPORTABLE ucs4_to_ucs2(const UCS4CHAR *src, int srcLen, UCS2CHAR *dst, int dstLen)
{
#if !defined(__DISABLE_ICONV) && !defined(_WIN32)
   iconv_t cd;
   const char *inbuf;
   char *outbuf;
   size_t count, inbytes, outbytes;

   cd = IconvOpen(UCS2_CODEPAGE_NAME, UCS4_CODEPAGE_NAME);
   if (cd == (iconv_t)(-1))
   {
      return __internal_ucs4_to_ucs2(src, srcLen, dst, dstLen);
   }

   inbuf = (const char *) src;
   inbytes = ((srcLen == -1) ? ucs4_strlen(src) + 1 : (size_t)srcLen) * sizeof(UCS4CHAR);
   outbuf = (char *)dst;
   outbytes = (size_t)dstLen * sizeof(UCS2CHAR);
   count = iconv(cd, (ICONV_CONST char **) &inbuf, &inbytes, &outbuf, &outbytes);
   IconvClose(cd);

   if (count == (size_t) - 1)
   {
      if (errno == EILSEQ)
      {
         count = (dstLen * sizeof(UCS2CHAR) - outbytes) / sizeof(UCS2CHAR);
      }
      else
      {
         count = 0;
      }
   }
   else
   {
      count = (dstLen * sizeof(UCS2CHAR) - outbytes) / sizeof(UCS2CHAR);
   }
   if (((char *) outbuf - (char *) dst > sizeof(UCS2CHAR)) && (*dst == 0xFEFF))
   {
      // Remove UNICODE byte order indicator if presented
      memmove(dst, &dst[1], (char *) outbuf - (char *) dst - sizeof(UCS2CHAR));
      outbuf -= sizeof(UCS2CHAR);
   }
   if ((srcLen == -1) && (outbytes >= sizeof(UCS2CHAR)))
   {
      *((UCS2CHAR *)outbuf) = 0;
   }

   return (int)count;
#else
   return __internal_ucs4_to_ucs2(src, srcLen, dst, dstLen);
#endif
}

#if defined(_WIN32)

/**
 * Convert UCS-4 to UTF-8
 */
int LIBNETXMS_EXPORTABLE ucs4_to_utf8(const UCS4CHAR *src, int srcLen, char *dst, int dstLen)
{
   int len = (srcLen < 0) ? (int)ucs4_strlen(src) + 1 : srcLen;
   WCHAR *buffer = (len <= 32768) ? (WCHAR *)alloca(len * sizeof(WCHAR)) : (WCHAR *)malloc(len * sizeof(WCHAR));
   ucs4_to_ucs2(src, srcLen, buffer, len);
   int ret = WideCharToMultiByte(CP_UTF8, 0, buffer, srcLen, dst, dstLen, NULL, NULL);
   if (len > 32768)
      free(buffer);
   return ret;
}

/**
 * Convert UCS-4 to multibyte
 */
int LIBNETXMS_EXPORTABLE ucs4_to_mb(const UCS4CHAR *src, int srcLen, char *dst, int dstLen)
{
   int len = (srcLen < 0) ? (int)ucs4_strlen(src) + 1 : srcLen;
   WCHAR *buffer = (len <= 32768) ? (WCHAR *)alloca(len * sizeof(WCHAR)) : (WCHAR *)malloc(len * sizeof(WCHAR));
   ucs4_to_ucs2(src, srcLen, buffer, len);
   int ret = WideCharToMultiByte(CP_ACP, WC_DEFAULTCHAR | WC_COMPOSITECHECK, buffer, srcLen, dst, dstLen, NULL, NULL);
   if (len > 32768)
      free(buffer);
   return ret;
}

#elif !defined(UNICODE_UCS4)

/**
 * Convert UCS-4 to UTF-8 using stub (no actual conversion for character codes above 0x007F)
 */
static int __internal_ucs4_to_utf8(const UCS4CHAR *src, int srcLen, char *dst, int dstLen)
{
   const UCS4CHAR *psrc;
   char *pdst;
   int pos, size;

   size = (srcLen == -1) ? ucs4_strlen(src) : srcLen;
   if (size >= dstLen)
      size = dstLen - 1;
   for(psrc = src, pos = 0, pdst = dst; pos < size; pos++, psrc++, pdst++)
      *pdst = (*psrc < 128) ? (char) (*psrc) : '?';
   *pdst = 0;
   return size;
}

/**
 * Convert UCS-4 to UTF-8
 */
int LIBNETXMS_EXPORTABLE ucs4_to_utf8(const UCS4CHAR *src, int srcLen, char *dst, int dstLen)
{
#if HAVE_ICONV && !defined(__DISABLE_ICONV)
   iconv_t cd;
   const char *inbuf;
   char *outbuf;
   size_t count, inbytes, outbytes;

   cd = IconvOpen("UTF-8", UCS4_CODEPAGE_NAME);
   if (cd == (iconv_t) (-1))
   {
      return __internal_ucs4_to_utf8(src, srcLen, dst, dstLen);
   }

   inbuf = (const char *) src;
   inbytes = ((srcLen == -1) ? ucs2_strlen(src) + 1 : (size_t) srcLen) * sizeof(UCS4CHAR);
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
   return __internal_ucs4_to_utf8(src, srcLen, dst, dstLen);
#endif
}

/**
 * Convert UCS-4 to multibyte using stub (no actual conversion for character codes above 0x007F)
 */
static int __internal_ucs4_to_mb(const UCS4CHAR *src, int srcLen, char *dst, int dstLen)
{
   const UCS4CHAR *psrc;
   char *pdst;
   int pos, size;

   size = (srcLen == -1) ? (int) ucs4_strlen(src) : srcLen;
   if (size >= dstLen)
      size = dstLen - 1;

   for(psrc = src, pos = 0, pdst = dst; pos < size; pos++, psrc++, pdst++)
      *pdst = (*psrc < 256) ? (char) (*psrc) : '?';
   *pdst = 0;

   return size;
}

/**
 * Convert UCS-4 to multibyte
 */
int LIBNETXMS_EXPORTABLE ucs4_to_mb(const UCS4CHAR *src, int srcLen, char *dst, int dstLen)
{
#if HAVE_ICONV && !defined(__DISABLE_ICONV)
   iconv_t cd;
   const char *inbuf;
   char *outbuf;
   size_t count, inbytes, outbytes;

   cd = IconvOpen(g_cpDefault, UCS4_CODEPAGE_NAME);
   if (cd == (iconv_t) (-1))
   {
      return __internal_ucs4_to_mb(src, srcLen, dst, dstLen);
   }

   inbuf = (const char *) src;
   inbytes = ((srcLen == -1) ? ucs4_strlen(src) + 1 : (size_t) srcLen) * sizeof(UCS4CHAR);
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
   return __internal_ucs4_to_mb(src, srcLen, dst, dstLen);
#endif
}

#endif
