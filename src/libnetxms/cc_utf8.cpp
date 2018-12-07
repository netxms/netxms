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
 ** File: cc_utf8.cpp
 **
 **/

#include "libnetxms.h"
#include "unicode_cc.h"

inline UCS4CHAR CodePointFromUTF8(const BYTE*& s, size_t& len)
{
   BYTE b = *s++;
   if ((b & 0x80) == 0)
   {
      len--;
      return static_cast<UCS4CHAR>(b);
   }
   if (((b & 0xE0) == 0xC0) && (len >= 2))
   {
      len -= 2;
      return (static_cast<UCS4CHAR>(b & 0x1F) << 6) | static_cast<UCS4CHAR>(*s++ & 0x3F);
   }
   if (((b & 0xF0) == 0xE0) && (len >= 3))
   {
      len -= 3;
      BYTE b2 = *s++;
      return (static_cast<UCS4CHAR>(b & 0x0F) << 12) | (static_cast<UCS4CHAR>(b2 & 0x3F) << 6) | static_cast<UCS4CHAR>(*s++ & 0x3F);
   }
   if (((b & 0xF8) == 0xF0) && (len >= 4))
   {
      len -= 4;
      BYTE b2 = *s++;
      BYTE b3 = *s++;
      return (static_cast<UCS4CHAR>(b & 0x0F) << 18) |
               (static_cast<UCS4CHAR>(b2 & 0x3F) << 12) |
               (static_cast<UCS4CHAR>(b3 & 0x3F) << 6) |
               static_cast<UCS4CHAR>(*s++ & 0x3F);
   }
   len--;
   return '?';   // invalid sequence
}

/**
 * Convert UTF-8 to UCS-4
 */
int LIBNETXMS_EXPORTABLE utf8_to_ucs4(const char *src, int srcLen, UCS4CHAR *dst, int dstLen)
{
   size_t len = (srcLen == -1) ? strlen(src) : srcLen;
   const BYTE *s = reinterpret_cast<const BYTE*>(src);
   UCS4CHAR *d = dst;
   int dcount = 0;
   while((len > 0) && (dcount < dstLen))
   {
      *d++ = CodePointFromUTF8(s, len);
      dcount++;
   }

   if (srcLen != -1)
      return dcount;
   if (dcount == dstLen)
      dcount--;
   dst[dcount] = 0;
   return dcount;
}

/**
 * Calculate length of given UTF-8 string in UCS-4 characters (including terminator)
 */
int LIBNETXMS_EXPORTABLE utf8_ucs4len(const char *src, int srcLen)
{
   size_t len = (srcLen == -1) ? strlen(src) : srcLen;
   const BYTE *s = reinterpret_cast<const BYTE*>(src);
   int dcount = 1;
   while(len > 0)
   {
      CodePointFromUTF8(s, len);
      dcount++;
   }
   return dcount;
}

/**
 * Convert UTF-8 to UCS-2
 */
int LIBNETXMS_EXPORTABLE utf8_to_ucs2(const char *src, int srcLen, UCS2CHAR *dst, int dstLen)
{
   size_t len = (srcLen == -1) ? strlen(src) : srcLen;
   const BYTE *s = reinterpret_cast<const BYTE*>(src);
   UCS2CHAR *d = dst;
   int dcount = 0;
   while((len > 0) && (dcount < dstLen))
   {
      UCS4CHAR ch = CodePointFromUTF8(s, len);
      if (ch <= 0xFFFF)
      {
         *d++ = static_cast<UCS2CHAR>(ch);
         dcount++;
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

   if (srcLen != -1)
      return dcount;
   if (dcount == dstLen)
      dcount--;
   dst[dcount] = 0;
   return dcount;
}

/**
 * Calculate length of given UTF-8 string in UCS-2 characters (including terminator)
 */
int LIBNETXMS_EXPORTABLE utf8_ucs2len(const char *src, int srcLen)
{
   size_t len = (srcLen == -1) ? strlen(src) : srcLen;
   const BYTE *s = reinterpret_cast<const BYTE*>(src);
   int dcount = 1;
   while(len > 0)
   {
      UCS4CHAR ch = CodePointFromUTF8(s, len);
      dcount++;
      if (ch > 0xFFFF)
         dcount++;
   }
   return dcount;
}

#ifdef _WIN32

/**
 * Convert UTF-8 to multibyte
 */
int LIBNETXMS_EXPORTABLE utf8_to_mb(const char *src, int srcLen, char *dst, int dstLen)
{
   int len = (int)strlen(src) + 1;
   WCHAR *buffer = (len <= 32768) ? (WCHAR *)alloca(len * sizeof(WCHAR)) : (WCHAR *)malloc(len * sizeof(WCHAR));
   MultiByteToWideChar(CP_UTF8, 0, src, -1, buffer, len);
   int ret = WideCharToMultiByte(CP_ACP, WC_DEFAULTCHAR | WC_COMPOSITECHECK, buffer, -1, dst, dstLen, NULL, NULL);
   if (len > 32768)
      free(buffer);
   return ret;
}

#else /* _WIN32 */

/**
 * Convert UTF-8 to multibyte using stub (no actual conversion for character codes above 0x7F)
 */
static int __internal_utf8_to_mb(const char *src, int srcLen, char *dst, int dstLen)
{
   const char *psrc = src;
   char *pdst = dst;
   int dsize = 0;
   int ssize = (srcLen == -1) ? strlen(src) : srcLen;
   for(int pos = 0; (pos < ssize) && (dsize < dstLen - 1); pos++, psrc++)
   {
      BYTE b = (BYTE)*psrc;
      if ((b & 0x80) == 0)  // ASCII-7 character
      {
         *pdst = (char)b;
         pdst++;
         dsize++;
      }
      else if ((b & 0xC0) == 0xC0)  // UTF-8 start byte
      {
         *pdst = '?';
         pdst++;
         dsize++;
      }
   }
   *pdst = 0;
   return dsize;
}

/**
 * Convert UTF-8 to multibyte
 */
int LIBNETXMS_EXPORTABLE utf8_to_mb(const char *src, int srcLen, char *dst, int dstLen)
{
#if HAVE_ICONV && !defined(__DISABLE_ICONV)
   iconv_t cd;
   const char *inbuf;
   char *outbuf;
   size_t count, inbytes, outbytes;

   cd = IconvOpen(g_cpDefault, "UTF-8");
   if (cd == (iconv_t)(-1))
   {
      return __internal_utf8_to_mb(src, srcLen, dst, dstLen);
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
   return __internal_utf8_to_mb(src, srcLen, dst, dstLen);
#endif
}

#endif /* _WIN32 */
