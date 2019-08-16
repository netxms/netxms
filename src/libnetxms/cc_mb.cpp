/*
 ** NetXMS - Network Management System
 ** Copyright (C) 2003-2019 Raden Solutions
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

size_t LIBNETXMS_EXPORTABLE mb_to_ucs4(const char *src, ssize_t srcLen, UCS4CHAR *dst, size_t dstLen)
{
   size_t len = (srcLen < 0) ? strlen(src) + 1 : srcLen;
   WCHAR *buffer = (len <= 32768) ? (WCHAR *)alloca(len * sizeof(WCHAR)) : (WCHAR *)MemAlloc(len * sizeof(WCHAR));
   MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, src, (int)srcLen, buffer, (int)len);
   size_t ret = ucs2_to_ucs4(buffer, srcLen, dst, dstLen);
   if (len > 32768)
      MemFree(buffer);
   return ret;
}

#elif !defined UNICODE_UCS4

/**
 * Convert multibyte to UCS-4
 */
size_t LIBNETXMS_EXPORTABLE mb_to_ucs4(const char *src, ssize_t srcLen, UCS4CHAR *dst, size_t dstLen)
{
   if (g_defaultCodePageType == CodePageType::ASCII)
      return ASCII_to_ucs4(src, srcLen, dst, dstLen);
   if (g_defaultCodePageType == CodePageType::ISO8859_1)
      return ISO8859_1_to_ucs4(src, srcLen, dst, dstLen);

#if HAVE_ICONV && !defined(__DISABLE_ICONV)
   iconv_t cd;
   const char *inbuf;
   char *outbuf;
   size_t count, inbytes, outbytes;

   cd = IconvOpen(UCS4_CODEPAGE_NAME, g_cpDefault);
   if (cd == (iconv_t)(-1))
   {
      return ASCII_to_ucs4(src, srcLen, dst, dstLen);
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

   return count;
#else
   return ASCII_to_ucs4(src, srcLen, dst, dstLen);
#endif
}

#endif

#if !defined(_WIN32) && !defined(UNICODE_UCS2)

/**
 * Convert multibyte to UCS-2
 */
size_t LIBNETXMS_EXPORTABLE mb_to_ucs2(const char *src, ssize_t srcLen, UCS2CHAR *dst, size_t dstLen)
{
   if (g_defaultCodePageType == CodePageType::ASCII)
      return ASCII_to_ucs2(src, srcLen, dst, dstLen);
   if (g_defaultCodePageType == CodePageType::ISO8859_1)
      return ISO8859_1_to_ucs2(src, srcLen, dst, dstLen);

#if HAVE_ICONV && !defined(__DISABLE_ICONV)
   iconv_t cd;
   const char *inbuf;
   char *outbuf;
   size_t count, inbytes, outbytes;

   cd = IconvOpen(UCS2_CODEPAGE_NAME, g_cpDefault);
   if (cd == (iconv_t)(-1))
   {
      return ASCII_to_ucs2(src, srcLen, dst, dstLen);
   }

   inbuf = src;
   inbytes = (srcLen == -1) ? strlen(src) + 1 : static_cast<size_t>(srcLen);
   outbuf = reinterpret_cast<char*>(dst);
   outbytes = static_cast<size_t>(dstLen) * sizeof(UCS2CHAR);
   count = iconv(cd, (ICONV_CONST char **)&inbuf, &inbytes, &outbuf, &outbytes);
   IconvClose(cd);

   if (count == (size_t)-1)
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
   if ((outbuf - reinterpret_cast<char*>(dst) > static_cast<ptrdiff_t>(sizeof(UCS2CHAR))) && (*dst == 0xFEFF))
   {
      // Remove UNICODE byte order indicator if presented
      memmove(dst, &dst[1], (char *) outbuf - (char *) dst - sizeof(UCS2CHAR));
      outbuf -= sizeof(UCS2CHAR);
   }
   if ((srcLen == -1) && (outbytes >= sizeof(UCS2CHAR)))
   {
      *reinterpret_cast<UCS2CHAR*>(outbuf) = 0;
   }

   return count;
#else
   return ASCII_to_ucs2(src, srcLen, dst, dstLen);
#endif
}

#endif /* !defined(_WIN32) && !defined(UNICODE_UCS2) */

#ifdef _WIN32

/**
 * Convert multibyte to UTF-8
 */
size_t LIBNETXMS_EXPORTABLE mb_to_utf8(const char *src, ssize_t srcLen, char *dst, size_t dstLen)
{
   size_t len = (srcLen < 0) ? strlen(src) + 1 : srcLen;
   WCHAR *buffer = (len <= 32768) ? (WCHAR *)alloca(len * sizeof(WCHAR)) : (WCHAR *)MemAlloc(len * sizeof(WCHAR));
   MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, src, (int)srcLen, buffer, (int)len);
   size_t ret = WideCharToMultiByte(CP_UTF8, 0, buffer, (int)srcLen, dst, (int)dstLen, NULL, NULL);
   if (len > 32768)
      MemFree(buffer);
   return ret;
}

#else /* not _WIN32 */

/**
 * Convert multibyte to UTF-8
 */
size_t LIBNETXMS_EXPORTABLE mb_to_utf8(const char *src, ssize_t srcLen, char *dst, size_t dstLen)
{
   if (g_defaultCodePageType == CodePageType::ASCII)
      return ASCII_to_utf8(src, srcLen, dst, dstLen);
   if (g_defaultCodePageType == CodePageType::ISO8859_1)
      return ISO8859_1_to_utf8(src, srcLen, dst, dstLen);

#if HAVE_ICONV && !defined(__DISABLE_ICONV)
   iconv_t cd;
   const char *inbuf;
   char *outbuf;
   size_t count, inbytes, outbytes;

   cd = IconvOpen("UTF-8", g_cpDefault);
   if (cd == (iconv_t)(-1))
   {
      return ASCII_to_utf8(src, srcLen, dst, dstLen);
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

   return count;
#else
   return ASCII_to_utf8(src, srcLen, dst, dstLen);
#endif
}

#endif /* _WIN32 */

/**
 * Convert ASCII to UTF-8
 */
size_t LIBNETXMS_EXPORTABLE ASCII_to_utf8(const char *src, ssize_t srcLen, char *dst, size_t dstLen)
{
   size_t size = (srcLen == -1) ? strlen(src) : srcLen;
   if (size >= dstLen)
      size = dstLen - 1;

   const BYTE *psrc = reinterpret_cast<const BYTE*>(src);
   char *pdst = dst;
   for(size_t pos = 0; pos < size; pos++, psrc++, pdst++)
      *pdst = (*psrc < 128) ? static_cast<char>(*psrc) : '?';
   *pdst = 0;

   return size;
}

/**
 * Convert ASCII to UCS-2
 */
size_t LIBNETXMS_EXPORTABLE ASCII_to_ucs2(const char *src, ssize_t srcLen, UCS2CHAR *dst, size_t dstLen)
{
   size_t size = (srcLen == -1) ? strlen(src) : srcLen;
   if (size >= dstLen)
      size = dstLen - 1;

   const BYTE *psrc = reinterpret_cast<const BYTE*>(src);
   UCS2CHAR *pdst = dst;
   for(size_t pos = 0; pos < size; pos++, psrc++, pdst++)
      *pdst = (*psrc < 128) ? *psrc : '?';
   *pdst = 0;

   return size;
}

/**
 * Convert ASCII to UCS-4
 */
size_t LIBNETXMS_EXPORTABLE ASCII_to_ucs4(const char *src, ssize_t srcLen, UCS4CHAR *dst, size_t dstLen)
{
   size_t size = (srcLen == -1) ? strlen(src) : srcLen;
   if (size >= dstLen)
      size = dstLen - 1;

   const BYTE *psrc = reinterpret_cast<const BYTE*>(src);
   UCS4CHAR *pdst = dst;
   for(size_t pos = 0; pos < size; pos++, psrc++, pdst++)
      *pdst = (*psrc < 128) ? *psrc : '?';
   *pdst = 0;

   return size;
}

/**
 * Convert ISO8859-1 to UTF-8
 */
size_t LIBNETXMS_EXPORTABLE ISO8859_1_to_utf8(const char *src, ssize_t srcLen, char *dst, size_t dstLen)
{
   size_t size = (srcLen == -1) ? strlen(src) : srcLen;
   if (size >= dstLen)
      size = dstLen - 1;

   const BYTE *psrc = reinterpret_cast<const BYTE*>(src);
   BYTE *pdst = reinterpret_cast<BYTE*>(dst);
   for(size_t pos = 0; pos < size; pos++, psrc++, pdst++)
   {
      if (*psrc < 128)
      {
         *pdst = *psrc;
      }
      else if (*psrc >= 160)
      {
         if (size - pos < 2)
            break;   // no enough space in destination buffer
         *pdst++ = static_cast<BYTE>((static_cast<unsigned int>(*psrc) >> 6) | 0xC0);
         *pdst = static_cast<BYTE>((static_cast<unsigned int>(*psrc) & 0x3F) | 0x80);
      }
      else
      {
         *pdst = '?';
      }
   }
   *pdst = 0;

   return size;
}

/**
 * Convert ISO8859-1 to UCS-2
 */
size_t LIBNETXMS_EXPORTABLE ISO8859_1_to_ucs2(const char *src, ssize_t srcLen, UCS2CHAR *dst, size_t dstLen)
{
   size_t size = (srcLen == -1) ? strlen(src) : srcLen;
   if (size >= dstLen)
      size = dstLen - 1;

   const BYTE *psrc = reinterpret_cast<const BYTE*>(src);
   UCS2CHAR *pdst = dst;
   for(size_t pos = 0; pos < size; pos++, psrc++, pdst++)
      *pdst = ((*psrc < 128) || (*psrc >= 160)) ? *psrc : '?';
   *pdst = 0;

   return size;
}

/**
 * Convert ISO8859-1 to UCS-4
 */
size_t LIBNETXMS_EXPORTABLE ISO8859_1_to_ucs4(const char *src, ssize_t srcLen, UCS4CHAR *dst, size_t dstLen)
{
   size_t size = (srcLen == -1) ? strlen(src) : srcLen;
   if (size >= dstLen)
      size = dstLen - 1;

   const BYTE *psrc = reinterpret_cast<const BYTE*>(src);
   UCS4CHAR *pdst = dst;
   for(size_t pos = 0; pos < size; pos++, psrc++, pdst++)
      *pdst = ((*psrc < 128) || (*psrc >= 160)) ? *psrc : '?';
   *pdst = 0;

   return size;
}
