/*
 ** NetXMS - Network Management System
 ** Copyright (C) 2003-2020 Raden Solutions
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
 * Convert UCS-4 to UCS-2
 */
size_t LIBNETXMS_EXPORTABLE ucs4_to_ucs2(const UCS4CHAR *src, ssize_t srcLen, UCS2CHAR *dst, size_t dstLen)
{
   size_t len = (srcLen == -1) ? ucs4_strlen(src) + 1 : srcLen;
   size_t scount = 0, dcount = 0;
   const UCS4CHAR *s = src;
   UCS2CHAR *d = dst;
   while((scount < len) && (dcount < dstLen))
   {
      UCS4CHAR ch = *s++;
      scount++;
      if (ch <= 0xFFFF)
      {
         // Ignore UTF-16 surrogate halves
         if ((ch < 0xD800) || (ch > 0xDFFF))
         {
            *d++ = static_cast<UCS2CHAR>(ch);
            dcount++;
         }
      }
      else if (ch <= 0x10FFFF)
      {
         if (dcount > dstLen - 2)
            break;   // no enough space in destination buffer
         ch -= 0x10000;
         *d++ = static_cast<UCS2CHAR>((ch >> 10) | 0xD800);
         *d++ = static_cast<UCS2CHAR>((ch & 0x3FF) | 0xDC00);
         dcount += 2;
      }
   }

   if ((srcLen == -1) && (dcount == dstLen) && (dstLen > 0))
      dst[dcount - 1] = 0;
   return dcount;
}

/**
 * Calculate length in characters of given UCS-4 string in UCS-2 encoding
 */
size_t LIBNETXMS_EXPORTABLE ucs4_ucs2len(const UCS4CHAR *src, ssize_t srcLen)
{
   size_t len = (srcLen == -1) ? ucs4_strlen(src) + 1 : srcLen;
   size_t dcount = len;
   const UCS4CHAR *s = src;
   while(len-- > 0)
   {
      UCS4CHAR ch = *s++;
      if (ch > 0xFFFF)
         dcount++;
   }
   return dcount;
}

/**
 * Convert UCS-4 to UTF-8
 */
size_t LIBNETXMS_EXPORTABLE ucs4_to_utf8(const UCS4CHAR *src, ssize_t srcLen, char *dst, size_t dstLen)
{
   size_t len = (srcLen == -1) ? ucs4_strlen(src) + 1 : srcLen;
   size_t scount = 0, dcount = 0;
   const UCS4CHAR *s = src;
   char *d = dst;
   while((scount < len) && (dcount < dstLen))
   {
      UCS4CHAR ch = *s++;
      scount++;
      if (ch <= 0x7F)
      {
         *d++ = static_cast<char>(ch);
         dcount++;
      }
      else if (ch <= 0x7FF)
      {
         if (dcount > dstLen - 2)
            break;   // no enough space in destination buffer
         *d++ = static_cast<char>((ch >> 6) | 0xC0);
         *d++ = static_cast<char>((ch & 0x3F) | 0x80);
         dcount += 2;
      }
      else if (ch <= 0xFFFF)
      {
         if (dcount > dstLen - 3)
            break;   // no enough space in destination buffer

         // Ignore UTF-16 surrogate halves
         if ((ch < 0xD800) || (ch > 0xDFFF))
         {
            *d++ = static_cast<char>((ch >> 12) | 0xE0);
            *d++ = static_cast<char>(((ch >> 6) & 0x3F) | 0x80);
            *d++ = static_cast<char>((ch & 0x3F) | 0x80);
            dcount += 3;
         }
      }
      else if (ch <= 0x10FFFF)
      {
         if (dcount > dstLen - 4)
            break;   // no enough space in destination buffer
         *d++ = static_cast<char>((ch >> 18) | 0xF0);
         *d++ = static_cast<char>(((ch >> 12) & 0x3F) | 0x80);
         *d++ = static_cast<char>(((ch >> 6) & 0x3F) | 0x80);
         *d++ = static_cast<char>((ch & 0x3F) | 0x80);
         dcount += 4;
      }
   }

   if ((srcLen == -1) && (dcount == dstLen) && (dstLen > 0))
      dst[dcount - 1] = 0;
   return dcount;
}

/**
 * Calculate length in bytes of given UCS-4 string in UTF-8 encoding
 */
size_t LIBNETXMS_EXPORTABLE ucs4_utf8len(const UCS4CHAR *src, ssize_t srcLen)
{
   size_t len = (srcLen == -1) ? ucs4_strlen(src) + 1 : srcLen;
   int dcount = 0;
   const UCS4CHAR *s = src;
   while(len-- > 0)
   {
      UCS4CHAR ch = *s++;
      if (ch <= 0x7F)
      {
         dcount++;
      }
      else if (ch <= 0x7FF)
      {
         dcount += 2;
      }
      else if (ch <= 0xFFFF)
      {
         if ((ch < 0xD800) || (ch > 0xDFFF)) // Ignore UTF-16 surrogate halves
            dcount += 3;
      }
      else if (ch <= 0x10FFFF)
      {
         dcount += 4;
      }
   }
   return dcount;
}

#if defined(_WIN32)

/**
 * Convert UCS-4 to multibyte
 */
size_t LIBNETXMS_EXPORTABLE ucs4_to_mb(const UCS4CHAR *src, ssize_t srcLen, char *dst, size_t dstLen)
{
   size_t len = (srcLen == -1) ? ucs4_strlen(src) + 1 : srcLen;
   WCHAR *buffer = (len <= 32768) ? (WCHAR *)alloca(len * sizeof(WCHAR)) : (WCHAR *)MemAlloc(len * sizeof(WCHAR));
   size_t ucs2Len = ucs4_to_ucs2(src, srcLen, buffer, len);
   size_t ret = ucs2_to_mb(buffer, ucs2Len, dst, dstLen);
   if (len > 32768)
      MemFree(buffer);
   return ret;
}

#elif !defined(UNICODE_UCS4)

/**
 * Convert UCS-4 to multibyte
 */
size_t LIBNETXMS_EXPORTABLE ucs4_to_mb(const UCS4CHAR *src, ssize_t srcLen, char *dst, size_t dstLen)
{
   if (g_defaultCodePageType == CodePageType::ASCII)
      return ucs4_to_ASCII(src, srcLen, dst, dstLen);
   if (g_defaultCodePageType == CodePageType::ISO8859_1)
      return ucs4_to_ISO8859_1(src, srcLen, dst, dstLen);
   if (g_defaultCodePageType == CodePageType::UTF8)
      return ucs4_to_utf8(src, srcLen, dst, dstLen);

#if HAVE_ICONV && !defined(__DISABLE_ICONV)
   iconv_t cd = IconvOpen(g_cpDefault, UCS4_CODEPAGE_NAME);
   if (cd == (iconv_t) (-1))
   {
      return ucs4_to_ASCII(src, srcLen, dst, dstLen);
   }

   const char *inbuf = reinterpret_cast<const char*>(src);
   size_t inbytes = ((srcLen == -1) ? ucs4_strlen(src) + 1 : srcLen) * sizeof(UCS4CHAR);
   char *outbuf = reinterpret_cast<char*>(dst);
   size_t outbytes = dstLen;
   size_t count = iconv(cd, (ICONV_CONST char **)&inbuf, &inbytes, &outbuf, &outbytes);
   IconvClose(cd);

   if ((count != (size_t)(-1)) || (errno == EILSEQ))
   {
      count = dstLen - outbytes;
   }
   else
   {
      count = 0;
   }
   if ((srcLen == -1) && (outbytes >= sizeof(char)))
   {
      *((char *) outbuf) = 0;
   }

   return count;
#else
   return ucs4_to_ASCII(src, srcLen, dst, dstLen);
#endif
}

#endif

/**
 * Convert UCS-4 to ASCII (also used as fallback if iconv_open fails)
 */
size_t LIBNETXMS_EXPORTABLE ucs4_to_ASCII(const UCS4CHAR *src, ssize_t srcLen, char *dst, size_t dstLen)
{
   size_t size = (srcLen == -1) ? ucs4_strlen(src) + 1 : srcLen;
   if (size > dstLen)
      size = dstLen;

   const UCS4CHAR *psrc = src;
   char *pdst = dst;
   for(size_t pos = 0; pos < size; pos++, psrc++, pdst++)
      *pdst = (*psrc < 128) ? static_cast<char>(*psrc) : '?';

   return size;
}

/**
 * Convert UCS-4 to ISO8859-1
 */
size_t LIBNETXMS_EXPORTABLE ucs4_to_ISO8859_1(const UCS4CHAR *src, ssize_t srcLen, char *dst, size_t dstLen)
{
   size_t size = (srcLen == -1) ? ucs4_strlen(src) + 1 : srcLen;
   if (size > dstLen)
      size = dstLen;

   const UCS4CHAR *psrc = src;
   BYTE *pdst = reinterpret_cast<BYTE*>(dst);
   for(size_t pos = 0; pos < size; pos++, psrc++, pdst++)
      *pdst = ((*psrc < 128) || ((*psrc >= 160) && (*psrc <= 255))) ? static_cast<BYTE>(*psrc) : '?';

   return size;
}
