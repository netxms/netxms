/*
 ** NetXMS - Network Management System
 ** Copyright (C) 2003-2018 Raden Solutions
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
int LIBNETXMS_EXPORTABLE ucs2_to_ucs4(const UCS2CHAR *src, int srcLen, UCS4CHAR *dst, int dstLen)
{
   int len = static_cast<int>((srcLen == -1) ? ucs2_strlen(src) : srcLen);
   int scount = 0, dcount = 0;
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

   if (srcLen != -1)
      return dcount;
   if (dcount == dstLen)
      dcount--;
   dst[dcount] = 0;
   return dcount;
}

/**
 * Convert UCS-2 to UTF-8
 */
int LIBNETXMS_EXPORTABLE ucs2_to_utf8(const UCS2CHAR *src, int srcLen, char *dst, int dstLen)
{
   int len = static_cast<int>((srcLen == -1) ? ucs2_strlen(src) : srcLen);
   int scount = 0, dcount = 0;
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

   if (srcLen != -1)
      return dcount;
   if (dcount == dstLen)
      dcount--;
   dst[dcount] = 0;
   return dcount;
}

/**
 * Calculate length in bytes of given UCS-2 string in UTF-8 encoding (including terminating 0 byte)
 */
int LIBNETXMS_EXPORTABLE ucs2_utf8len(const UCS2CHAR *src, int srcLen)
{
   int len = static_cast<int>((srcLen == -1) ? ucs2_strlen(src) : srcLen);
   int scount = 0, dcount = 1;
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
