/*
 ** NetXMS - Network Management System
 ** Copyright (C) 2003-2021 Raden Solutions
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
   int wcLen = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, src, (int)srcLen, buffer, (int)len);
   size_t ret = ucs2_to_ucs4(buffer, wcLen, dst, dstLen);
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
   if (g_defaultCodePageType == CodePageType::UTF8)
      return utf8_to_ucs4(src, srcLen, dst, dstLen);

#if HAVE_ICONV && !defined(__DISABLE_ICONV)
   iconv_t cd = IconvOpen(UCS4_CODEPAGE_NAME, g_cpDefault);
   if (cd == (iconv_t)(-1))
   {
      return ASCII_to_ucs4(src, srcLen, dst, dstLen);
   }

   const char *inbuf = (const char *)src;
   size_t inbytes = (srcLen == -1) ? strlen(src) + 1 : (size_t)srcLen;
   char *outbuf = (char *)dst;
   size_t outbytes = (size_t)dstLen * sizeof(UCS4CHAR);
   size_t count = iconv(cd, (ICONV_CONST char **) &inbuf, &inbytes, &outbuf, &outbytes);
   IconvClose(cd);

   if ((count != (size_t)(-1)) || (errno == EILSEQ))
   {
      count = (dstLen * sizeof(UCS4CHAR) - outbytes) / sizeof(UCS4CHAR);
   }
   else
   {
      count = 0;
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
   if (g_defaultCodePageType == CodePageType::UTF8)
      return utf8_to_ucs2(src, srcLen, dst, dstLen);

#if HAVE_ICONV && !defined(__DISABLE_ICONV)
   iconv_t cd = IconvOpen(UCS2_CODEPAGE_NAME, g_cpDefault);
   if (cd == (iconv_t)(-1))
   {
      return ASCII_to_ucs2(src, srcLen, dst, dstLen);
   }

   const char *inbuf = src;
   size_t inbytes = (srcLen == -1) ? strlen(src) + 1 : static_cast<size_t>(srcLen);
   char *outbuf = reinterpret_cast<char*>(dst);
   size_t outbytes = static_cast<size_t>(dstLen) * sizeof(UCS2CHAR);
   size_t count = iconv(cd, (ICONV_CONST char **)&inbuf, &inbytes, &outbuf, &outbytes);
   IconvClose(cd);

   if ((count != (size_t)(-1)) || (errno == EILSEQ))
   {
      count = (dstLen * sizeof(UCS2CHAR) - outbytes) / sizeof(UCS2CHAR);
   }
   else
   {
      count = 0;
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
   int wcLen = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, src, (int)srcLen, buffer, (int)len);
   size_t ret = WideCharToMultiByte(CP_UTF8, 0, buffer, wcLen, dst, (int)dstLen, NULL, NULL);
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
   if (g_defaultCodePageType == CodePageType::UTF8)
   {
      if (srcLen == -1)
         return strlcpy(dst, src, dstLen);
      size_t l = std::min(dstLen, static_cast<size_t>(srcLen));
      strncpy(dst, src, l);
      return l;
   }

   if (g_defaultCodePageType == CodePageType::ASCII)
      return ASCII_to_utf8(src, srcLen, dst, dstLen);
   if (g_defaultCodePageType == CodePageType::ISO8859_1)
      return ISO8859_1_to_utf8(src, srcLen, dst, dstLen);

#if HAVE_ICONV && !defined(__DISABLE_ICONV)
   iconv_t cd = IconvOpen("UTF-8", g_cpDefault);
   if (cd == (iconv_t)(-1))
   {
      return ASCII_to_utf8(src, srcLen, dst, dstLen);
   }

   const char *inbuf = (const char *)src;
   size_t inbytes = (srcLen == -1) ? strlen(src) + 1 : (size_t)srcLen;
   char *outbuf = (char *)dst;
   size_t outbytes = (size_t)dstLen;
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
   size_t size = (srcLen == -1) ? strlen(src) + 1 : srcLen;
   if (size > dstLen)
      size = dstLen;

   const BYTE *psrc = reinterpret_cast<const BYTE*>(src);
   char *pdst = dst;
   for(size_t pos = 0; pos < size; pos++, psrc++, pdst++)
      *pdst = (*psrc < 128) ? static_cast<char>(*psrc) : '?';

   return size;
}

/**
 * Convert ASCII to UCS-2
 */
size_t LIBNETXMS_EXPORTABLE ASCII_to_ucs2(const char *src, ssize_t srcLen, UCS2CHAR *dst, size_t dstLen)
{
   size_t size = (srcLen == -1) ? strlen(src) + 1 : srcLen;
   if (size > dstLen)
      size = dstLen;

   const BYTE *psrc = reinterpret_cast<const BYTE*>(src);
   UCS2CHAR *pdst = dst;
   for(size_t pos = 0; pos < size; pos++, psrc++, pdst++)
      *pdst = (*psrc < 128) ? *psrc : '?';

   return size;
}

/**
 * Convert ASCII to UCS-4
 */
size_t LIBNETXMS_EXPORTABLE ASCII_to_ucs4(const char *src, ssize_t srcLen, UCS4CHAR *dst, size_t dstLen)
{
   size_t size = (srcLen == -1) ? strlen(src) + 1 : srcLen;
   if (size > dstLen)
      size = dstLen;

   const BYTE *psrc = reinterpret_cast<const BYTE*>(src);
   UCS4CHAR *pdst = dst;
   for(size_t pos = 0; pos < size; pos++, psrc++, pdst++)
      *pdst = (*psrc < 128) ? *psrc : '?';

   return size;
}

/**
 * Convert ISO8859-1 to UTF-8
 */
size_t LIBNETXMS_EXPORTABLE ISO8859_1_to_utf8(const char *src, ssize_t srcLen, char *dst, size_t dstLen)
{
   size_t realSrcLen = (srcLen == -1) ? strlen(src) + 1 : srcLen;

   const BYTE *psrc = reinterpret_cast<const BYTE*>(src);
   BYTE *pdst = reinterpret_cast<BYTE*>(dst);
   size_t outBytes = 0;
   for(size_t pos = 0; (pos < realSrcLen) && (outBytes < dstLen); pos++, psrc++, pdst++, outBytes++)
   {
      if (*psrc < 128)
      {
         *pdst = *psrc;
      }
      else if (*psrc >= 160)
      {
         if (dstLen - outBytes < 2)
            break;   // no enough space in destination buffer
         *pdst++ = static_cast<BYTE>((static_cast<unsigned int>(*psrc) >> 6) | 0xC0);
         *pdst = static_cast<BYTE>((static_cast<unsigned int>(*psrc) & 0x3F) | 0x80);
         outBytes++;
      }
      else
      {
         *pdst = '?';
      }
   }

   return outBytes;
}

/**
 * Convert ISO8859-1 to UCS-2
 */
size_t LIBNETXMS_EXPORTABLE ISO8859_1_to_ucs2(const char *src, ssize_t srcLen, UCS2CHAR *dst, size_t dstLen)
{
   size_t size = (srcLen == -1) ? strlen(src) + 1 : srcLen;
   if (size > dstLen)
      size = dstLen;

   const BYTE *psrc = reinterpret_cast<const BYTE*>(src);
   UCS2CHAR *pdst = dst;
   for(size_t pos = 0; pos < size; pos++, psrc++, pdst++)
      *pdst = ((*psrc < 128) || (*psrc >= 160)) ? *psrc : '?';

   return size;
}

/**
 * Convert ISO8859-1 to UCS-4
 */
size_t LIBNETXMS_EXPORTABLE ISO8859_1_to_ucs4(const char *src, ssize_t srcLen, UCS4CHAR *dst, size_t dstLen)
{
   size_t size = (srcLen == -1) ? strlen(src) + 1 : srcLen;
   if (size > dstLen)
      size = dstLen;

   const BYTE *psrc = reinterpret_cast<const BYTE*>(src);
   UCS4CHAR *pdst = dst;
   for(size_t pos = 0; pos < size; pos++, psrc++, pdst++)
      *pdst = ((*psrc < 128) || (*psrc >= 160)) ? *psrc : '?';

   return size;
}
