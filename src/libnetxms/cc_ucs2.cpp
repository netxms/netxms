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
 ** File: cc_ucs2.cpp
 **
 **/

#include "libnetxms.h"
#include "unicode_cc.h"

/**
 * Convert UCS-2 to UCS-4
 */
size_t LIBNETXMS_EXPORTABLE ucs2_to_ucs4(const UCS2CHAR *src, ssize_t srcLen, UCS4CHAR *dst, size_t dstLen)
{
   size_t len = (srcLen == -1) ? ucs2_strlen(src) + 1 : srcLen;
   size_t scount = 0, dcount = 0;
   const UCS2CHAR *s = src;
   UCS4CHAR *d = dst;
   while((scount < len) && (dcount < dstLen))
   {
      UCS2CHAR ch2 = *s++;
      scount++;
      if ((ch2 & 0xFC00) == 0xD800)  // high surrogate
      {
         UCS4CHAR ch4 = static_cast<UCS4CHAR>(ch2 & 0x03FF) << 10;
         if (scount < len)
         {
            ch2 = *s;
            if ((ch2 & 0xFC00) == 0xDC00)  // low surrogate
            {
               s++;
               scount++;
               *d++ = (ch4 | static_cast<UCS4CHAR>(ch2 & 0x03FF)) + 0x10000;
               dcount++;
            }
         }
      }
      else if ((ch2 & 0xFC00) != 0xDC00)   // ignore unexpected low surrogate
      {
         *d++ = static_cast<UCS4CHAR>(ch2);
         dcount++;
      }
   }

   if ((srcLen == -1) && (dcount == dstLen) && (dstLen > 0))
      dst[dcount - 1] = 0;
   return dcount;
}

/**
 * Convert UCS-2 to UTF-8
 */
size_t LIBNETXMS_EXPORTABLE ucs2_to_utf8(const UCS2CHAR *src, ssize_t srcLen, char *dst, size_t dstLen)
{
   size_t len = (srcLen == -1) ? ucs2_strlen(src) + 1 : srcLen;
   size_t scount = 0, dcount = 0;
   const UCS2CHAR *s = src;
   char *d = dst;
   while((scount < len) && (dcount < dstLen))
   {
      UCS2CHAR ch = *s++;
      scount++;

      UCS4CHAR codepoint;
      if ((ch & 0xFC00) == 0xD800)  // high surrogate
      {
         codepoint = static_cast<UCS4CHAR>(ch & 0x03FF) << 10;
         if (scount < len)
         {
            ch = *s;
            if ((ch & 0xFC00) == 0xDC00)  // low surrogate
            {
               s++;
               scount++;
               codepoint = (codepoint | static_cast<UCS4CHAR>(ch & 0x03FF)) + 0x10000;
            }
         }
      }
      else if ((ch & 0xFC00) != 0xDC00)   // ignore unexpected low surrogate
      {
         codepoint = static_cast<UCS4CHAR>(ch);
      }
      else
      {
         continue;
      }

      if (codepoint <= 0x7F)
      {
         *d++ = static_cast<char>(codepoint);
         dcount++;
      }
      else if (codepoint <= 0x7FF)
      {
         if (dcount > dstLen - 2)
            break;   // no enough space in destination buffer
         *d++ = static_cast<char>((codepoint >> 6) | 0xC0);
         *d++ = static_cast<char>((codepoint & 0x3F) | 0x80);
         dcount += 2;
      }
      else if (codepoint <= 0xFFFF)
      {
         if (dcount > dstLen - 3)
            break;   // no enough space in destination buffer
         *d++ = static_cast<char>((codepoint >> 12) | 0xE0);
         *d++ = static_cast<char>(((codepoint >> 6) & 0x3F) | 0x80);
         *d++ = static_cast<char>((codepoint & 0x3F) | 0x80);
         dcount += 3;
      }
      else if (codepoint <= 0x10FFFF)
      {
         if (dcount > dstLen - 4)
            break;   // no enough space in destination buffer
         *d++ = static_cast<char>((codepoint >> 18) | 0xF0);
         *d++ = static_cast<char>(((codepoint >> 12) & 0x3F) | 0x80);
         *d++ = static_cast<char>(((codepoint >> 6) & 0x3F) | 0x80);
         *d++ = static_cast<char>((codepoint & 0x3F) | 0x80);
         dcount += 4;
      }
   }

   if ((srcLen == -1) && (dcount == dstLen) && (dstLen > 0))
      dst[dcount - 1] = 0;
   return dcount;
}

/**
 * Calculate length in bytes of given UCS-2 string in UTF-8 encoding (including terminating 0 byte)
 */
size_t LIBNETXMS_EXPORTABLE ucs2_utf8len(const UCS2CHAR *src, ssize_t srcLen)
{
   size_t len = (srcLen == -1) ? ucs2_strlen(src) + 1 : srcLen;
   size_t scount = 0, dcount = 0;
   const UCS2CHAR *s = src;
   while(scount < len)
   {
      UCS2CHAR ch = *s++;
      scount++;

      UCS4CHAR codepoint;
      if ((ch & 0xFC00) == 0xD800)  // high surrogate
      {
         codepoint = static_cast<UCS4CHAR>(ch & 0x03FF) << 10;
         if (scount < len)
         {
            ch = *s;
            if ((ch & 0xFC00) == 0xDC00)  // low surrogate
            {
               s++;
               scount++;
               codepoint = (codepoint | static_cast<UCS4CHAR>(ch & 0x03FF)) + 0x10000;
            }
         }
      }
      else if ((ch & 0xFC00) != 0xDC00)   // ignore unexpected low surrogate
      {
         codepoint = static_cast<UCS4CHAR>(ch);
      }
      else
      {
         continue;
      }

      if (codepoint <= 0x7F)
      {
         dcount++;
      }
      else if (codepoint <= 0x7FF)
      {
         dcount += 2;
      }
      else if (codepoint <= 0xFFFF)
      {
         dcount += 3;
      }
      else if (codepoint <= 0x10FFFF)
      {
         dcount += 4;
      }
   }
   return dcount;
}

#if !defined(_WIN32) && !defined(UNICODE_UCS2)

/**
 * Convert UCS-2 to multibyte
 */
size_t LIBNETXMS_EXPORTABLE ucs2_to_mb(const UCS2CHAR *src, ssize_t srcLen, char *dst, size_t dstLen)
{
   if (g_defaultCodePageType == CodePageType::ASCII)
      return ucs2_to_ASCII(src, srcLen, dst, dstLen);
   if (g_defaultCodePageType == CodePageType::ISO8859_1)
      return ucs2_to_ISO8859_1(src, srcLen, dst, dstLen);
   if (g_defaultCodePageType == CodePageType::UTF8)
      return ucs2_to_utf8(src, srcLen, dst, dstLen);

#if HAVE_ICONV && !defined(__DISABLE_ICONV)
   iconv_t cd = IconvOpen(g_cpDefault, UCS2_CODEPAGE_NAME);
   if (cd == (iconv_t) (-1))
   {
      return ucs2_to_ASCII(src, srcLen, dst, dstLen);
   }

   const char *inbuf = reinterpret_cast<const char*>(src);
   size_t inbytes = ((srcLen == -1) ? ucs2_strlen(src) + 1 : srcLen) * sizeof(UCS2CHAR);
   char *outbuf = reinterpret_cast<char*>(dst);
   size_t outbytes = dstLen;
   size_t count = iconv(cd, (ICONV_CONST char **) &inbuf, &inbytes, &outbuf, &outbytes);
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
   return ucs2_to_ASCII(src, srcLen, dst, dstLen);
#endif
}

#endif

/**
 * Convert UCS-2 to ASCII (also used as fallback if iconv_open fails)
 */
size_t LIBNETXMS_EXPORTABLE ucs2_to_ASCII(const UCS2CHAR *src, ssize_t srcLen, char *dst, size_t dstLen)
{
   size_t size = (srcLen == -1) ? ucs2_strlen(src) + 1 : srcLen;
   if (size > dstLen)
      size = dstLen;

   const UCS2CHAR *psrc = src;
   char *pdst = dst;
   for(size_t pos = 0; pos < size; pos++, psrc++, pdst++)
   {
      if ((*psrc & 0xFC00) == 0xD800)  // high surrogate
         continue;
      *pdst = (*psrc < 128) ? static_cast<char>(*psrc) : '?';
   }

   return size;
}

/**
 * Convert UCS-2 to ISO8859-1
 */
size_t LIBNETXMS_EXPORTABLE ucs2_to_ISO8859_1(const UCS2CHAR *src, ssize_t srcLen, char *dst, size_t dstLen)
{
   size_t size = (srcLen == -1) ? ucs2_strlen(src) + 1 : srcLen;
   if (size > dstLen)
      size = dstLen;

   const UCS2CHAR *psrc = src;
   BYTE *pdst = reinterpret_cast<BYTE*>(dst);
   for(size_t pos = 0; pos < size; pos++, psrc++, pdst++)
   {
      if ((*psrc & 0xFC00) == 0xD800)  // high surrogate
         continue;
      *pdst = ((*psrc < 128) || ((*psrc >= 160) && (*psrc <= 255))) ? static_cast<BYTE>(*psrc) : '?';
   }

   return size;
}
