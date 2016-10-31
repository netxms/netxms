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
 ** File: cc_mb.cpp
 **
 **/

#include "libnetxms.h"
#include "unicode_cc.h"

#if defined(_WIN32)

int LIBNETXMS_EXPORTABLE mb_to_ucs4(const char *src, int srcLen, UCS4CHAR *dst, int dstLen)
{
   int len = (srcLen < 0) ? (int)strlen(src) + 1 : srcLen;
   WCHAR *buffer = (len <= 32768) ? (WCHAR *)alloca(len * sizeof(WCHAR)) : (WCHAR *)malloc(len * sizeof(WCHAR));
   MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, src, srcLen, buffer, len);
   int ret = ucs2_to_ucs4(buffer, srcLen, dst, dstLen);
   if (len > 32768)
      free(buffer);
   return ret;
}

#elif !defined UNICODE_UCS4

/**
 * Convert multibyte to UCS-4 using stub
 */
static int __internal_mb_to_ucs4(const char *src, int srcLen, UCS4CHAR *dst, int dstLen)
{
   const char *psrc;
   UCS2CHAR *pdst;
   int pos, size;

   size = (srcLen == -1) ? (int) strlen(src) : srcLen;
   if (size >= dstLen)
      size = dstLen - 1;

   for(psrc = src, pos = 0, pdst = dst; pos < size; pos++, psrc++, pdst++)
      *pdst = (UCS4CHAR)(*psrc);
   *pdst = 0;

   return size;
}

/**
 * Convert multibyte to UCS-4
 */
int LIBNETXMS_EXPORTABLE mb_to_ucs4(const char *src, int srcLen, UCS4CHAR *dst, int dstLen)
{
#if HAVE_ICONV && !defined(__DISABLE_ICONV)
   iconv_t cd;
   const char *inbuf;
   char *outbuf;
   size_t count, inbytes, outbytes;

   cd = IconvOpen(UCS4_CODEPAGE_NAME, g_cpDefault);
   if (cd == (iconv_t)(-1))
   {
      return __internal_mb_to_ucs4(src, srcLen, dst, dstLen);
   }

   inbuf = (const char *) src;
   inbytes = (srcLen == -1) ? strlen(src) + 1 : (size_t) srcLen;
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
   if (((char *) outbuf - (char *) dst > sizeof(UCS4CHAR)) && (*dst == 0xFEFF))
   {
      // Remove UNICODE byte order indicator if presented
      memmove(dst, &dst[1], (char *) outbuf - (char *)dst - sizeof(UCS4CHAR));
      outbuf -= sizeof(UCS4CHAR);
   }
   if ((srcLen == -1) && (outbytes >= sizeof(UCS4CHAR)))
   {
      *((UCS4CHAR *)outbuf) = 0;
   }

   return (int)count;
#else
   return __internal_mb_to_ucs4(src, srcLen, dst, dstLen);
#endif
}

#endif

#if !defined(_WIN32) && !defined(UNICODE_UCS2)

/**
 * Convert multibyte to UCS-2 using stub
 */
static int __internal_mb_to_ucs2(const char *src, int srcLen, UCS2CHAR *dst, int dstLen)
{
   const char *psrc;
   UCS2CHAR *pdst;
   int pos, size;

   size = (srcLen == -1) ? (int) strlen(src) : srcLen;
   if (size >= dstLen)
      size = dstLen - 1;

   for(psrc = src, pos = 0, pdst = dst; pos < size; pos++, psrc++, pdst++)
      *pdst = (UCS2CHAR)(*psrc);
   *pdst = 0;

   return size;
}

/**
 * Convert multibyte to UCS-2
 */
int LIBNETXMS_EXPORTABLE mb_to_ucs2(const char *src, int srcLen, UCS2CHAR *dst, int dstLen)
{
#if HAVE_ICONV && !defined(__DISABLE_ICONV)
   iconv_t cd;
   const char *inbuf;
   char *outbuf;
   size_t count, inbytes, outbytes;

   cd = IconvOpen(UCS2_CODEPAGE_NAME, g_cpDefault);
   if (cd == (iconv_t)(-1))
   {
      return __internal_mb_to_ucs2(src, srcLen, dst, dstLen);
   }

   inbuf = (const char *) src;
   inbytes = (srcLen == -1) ? strlen(src) + 1 : (size_t) srcLen;
   outbuf = (char *) dst;
   outbytes = (size_t) dstLen * sizeof(UCS2CHAR);
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
   if (((char *) outbuf - (char *) dst > sizeof(UCS2CHAR)) && (*dst == 0xFEFF))
   {
      // Remove UNICODE byte order indicator if presented
      memmove(dst, &dst[1], (char *) outbuf - (char *) dst - sizeof(UCS2CHAR));
      outbuf -= sizeof(UCS2CHAR);
   }
   if ((srcLen == -1) && (outbytes >= sizeof(UCS2CHAR)))
   {
      *((UCS2CHAR *) outbuf) = 0;
   }

   return (int)count;
#else
   return __internal_mb_to_ucs2(src, srcLen, dst, dstLen);
#endif
}

#endif /* !defined(_WIN32) && !defined(UNICODE_UCS2) */

#ifdef _WIN32

/**
 * Convert multibyte to UTF-8
 */
int LIBNETXMS_EXPORTABLE mb_to_utf8(const char *src, int srcLen, char *dst, int dstLen)
{
   int len = (srcLen < 0) ? (int)strlen(src) + 1 : srcLen;
   WCHAR *buffer = (len <= 32768) ? (WCHAR *)alloca(len * sizeof(WCHAR)) : (WCHAR *)malloc(len * sizeof(WCHAR));
   MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, src, srcLen, buffer, len);
   int ret = WideCharToMultiByte(CP_UTF8, 0, buffer, srcLen, dst, dstLen, NULL, NULL);
   if (len > 32768)
      free(buffer);
   return ret;
}

#else /* not _WIN32 */

/**
 * Convert multibyte to UTF-8 using stub (no actual conversion for character codes above 0x7F)
 */
static int __internal_mb_to_utf8(const char *src, int srcLen, char *dst, int dstLen)
{
   const char *psrc;
   char *pdst;
   int pos, size;

   size = (srcLen == -1) ? strlen(src) : srcLen;
   if (size >= dstLen)
      size = dstLen - 1;
   for(psrc = src, pos = 0, pdst = dst; pos < size; pos++, psrc++, pdst++)
      *pdst = ((unsigned char)(*psrc) < 128) ? *psrc : '?';
   *pdst = 0;
   return size;
}

/**
 * Convert multibyte to UTF-8
 */
int LIBNETXMS_EXPORTABLE mb_to_utf8(const char *src, int srcLen, char *dst, int dstLen)
{
#if HAVE_ICONV && !defined(__DISABLE_ICONV)
   iconv_t cd;
   const char *inbuf;
   char *outbuf;
   size_t count, inbytes, outbytes;

   cd = IconvOpen("UTF-8", g_cpDefault);
   if (cd == (iconv_t)(-1))
   {
      return __internal_mb_to_utf8(src, srcLen, dst, dstLen);
   }

   inbuf = (const char *)src;
   inbytes = (srcLen == -1) ? strlen(src) + 1 : (size_t)srcLen;
   outbuf = (char *)dst;
   outbytes = (size_t)dstLen;
   count = iconv(cd, (ICONV_CONST char **)&inbuf, &inbytes, &outbuf, &outbytes);
   IconvClose(cd);

   if (count == (size_t) - 1)
   {
      if (errno == EILSEQ)
      {
         count = dstLen * sizeof(char) - outbytes;
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
   if ((srcLen == -1) && (outbytes >= 1))
   {
      *((char *)outbuf) = 0;
   }

   return (int)count;
#else
   return __internal_mb_to_utf8(src, srcLen, dst, dstLen);
#endif
}

#endif /* _WIN32 */
