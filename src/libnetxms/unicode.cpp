/*
 ** NetXMS - Network Management System
 ** Copyright (C) 2003-2025 Raden Solutions
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
 ** File: unicode.cpp
 **
 **/

#include "libnetxms.h"
#include "unicode_cc.h"
#include <nxcrypto.h>

/**
 * Default codepage
 */
char g_cpDefault[MAX_CODEPAGE_LEN] = ICONV_DEFAULT_CODEPAGE;
CodePageType g_defaultCodePageType = CodePageType::ISO8859_1;

/**
 * Set application's default codepage
 */
bool LIBNETXMS_EXPORTABLE SetDefaultCodepage(const char *cp)
{
   bool rc;
#if HAVE_ICONV && !defined(__DISABLE_ICONV)
   iconv_t cd;

   cd = iconv_open(cp, "UTF-8");
   if (cd != (iconv_t)(-1))
   {
      iconv_close(cd);
#endif
      strlcpy(g_cpDefault, cp, MAX_CODEPAGE_LEN);
      rc = true;

      if (!stricmp(cp, "ASCII"))
         g_defaultCodePageType = CodePageType::ASCII;
      else if (!stricmp(cp, "UTF8") || !stricmp(cp, "UTF-8"))
         g_defaultCodePageType = CodePageType::UTF8;
      else if (!stricmp(cp, "ISO-8859-1") || !stricmp(cp, "ISO_8859_1") ||
               !stricmp(cp, "ISO8859-1") || !stricmp(cp, "ISO8859_1") ||
               !stricmp(cp, "LATIN-1") || !stricmp(cp, "LATIN1"))
         g_defaultCodePageType = CodePageType::ISO8859_1;
      else
         g_defaultCodePageType = CodePageType::OTHER;

#if HAVE_ICONV && !defined(__DISABLE_ICONV)
   }
   else
   {
      rc = false;
   }
#endif
   return rc;
}

#ifndef UNICODE_UCS2

/**
 * Calculate length of UCS-2 character string
 */
size_t LIBNETXMS_EXPORTABLE ucs2_strlen(const UCS2CHAR *s)
{
   size_t len = 0;
   const UCS2CHAR *curr = s;
   while(*curr++)
      len++;
   return len;
}

#endif

#ifndef UNICODE_UCS4

/**
 * Calculate length of UCS-4 character string
 */
size_t LIBNETXMS_EXPORTABLE ucs4_strlen(const UCS4CHAR *s)
{
   size_t len = 0;
   const UCS4CHAR *curr = s;
   while(*curr++)
      len++;
   return len;
}

#endif

#if !HAVE_WCSLEN

/**
 * Calculate length of wide character string
 */
size_t LIBNETXMS_EXPORTABLE wcslen(const WCHAR *s)
{
   size_t len = 0;
   const WCHAR *curr = s;
   while(*curr++)
      len++;
   return len;
}

#endif

#ifndef UNICODE_UCS2

/**
 * Duplicate UCS-2 character string
 */
UCS2CHAR LIBNETXMS_EXPORTABLE *ucs2_strdup(const UCS2CHAR *src)
{
   return (UCS2CHAR *)MemCopyBlock(src, (ucs2_strlen(src) + 1) * sizeof(UCS2CHAR));
}

#endif

#ifndef UNICODE_UCS4

/**
 * Duplicate UCS-4 character string
 */
UCS4CHAR LIBNETXMS_EXPORTABLE *ucs4_strdup(const UCS4CHAR *src)
{
   return (UCS4CHAR *)MemCopyBlock(src, (ucs4_strlen(src) + 1) * sizeof(UCS4CHAR));
}

#endif

#if !UNICODE_UCS2

/**
 * Copy UCS-2 character string with length limitation
 */
UCS2CHAR LIBNETXMS_EXPORTABLE *ucs2_strncpy(UCS2CHAR *dest, const UCS2CHAR *src, size_t n)
{
   size_t len = ucs2_strlen(src) + 1;
   if (len > n)
      len = n;
   memcpy(dest, src, len * sizeof(UCS2CHAR));
   return dest;
}

#endif

#if !UNICODE_UCS4

/**
 * Copy UCS-2 character string with length limitation
 */
UCS4CHAR LIBNETXMS_EXPORTABLE *ucs4_strncpy(UCS4CHAR *dest, const UCS4CHAR *src, size_t n)
{
   size_t len = ucs4_strlen(src) + 1;
   if (len > n)
      len = n;
   memcpy(dest, src, len * sizeof(UCS4CHAR));
   return dest;
}

#endif

#if !HAVE_WCSNCPY

/**
 * Copy UCS-2 character string with length limitation
 */
WCHAR LIBNETXMS_EXPORTABLE *wcsncpy(WCHAR *dest, const WCHAR *src, size_t n)
{
   size_t len = wcslen(src) + 1;
   if (len > n)
      len = n;
   memcpy(dest, src, len * sizeof(WCHAR));
   return dest;
}

#endif

#ifndef _WIN32

#ifndef __DISABLE_ICONV

/**
 * Convert UNICODE string to single-byte string using iconv
 */
static size_t WideCharToMultiByteIconv(const char *codepage, const wchar_t *src, ssize_t srcLen, char *dst, size_t dstLen)
{
#if HAVE_ICONV_IGNORE
   char cp[MAX_CODEPAGE_LEN + 16];
   strcpy(cp, (codepage != nullptr) ? codepage : g_cpDefault);
   strcat(cp, "//IGNORE");
#else
   const char *cp = codepage;
#endif /* HAVE_ICONV_IGNORE */

   iconv_t cd = IconvOpen(cp, UNICODE_CODEPAGE_NAME);
   if (cd == (iconv_t)(-1))
   {
#if UNICODE_UCS4
      return ucs4_to_ASCII(src, srcLen, dst, dstLen);
#else
      return ucs2_to_ASCII(src, srcLen, dst, dstLen);
#endif
   }

   const char *inbuf = (const char *)src;
   size_t inbytes = ((srcLen == -1) ? wcslen(src) + 1 : srcLen) * sizeof(wchar_t);
   char *outbuf = dst;
   size_t outbytes = dstLen;
   size_t count = iconv(cd, (ICONV_CONST char **)&inbuf, &inbytes, &outbuf, &outbytes);
   IconvClose(cd);
   if (count == (size_t)(-1))
   {
      if (errno == EILSEQ)
      {
         count = dstLen - outbytes;
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
   if (outbytes > 0)
   {
      *outbuf = 0;
   }

   return count;
}

#endif

/**
 * Convert UNICODE string to single-byte string (Windows compatibility layer)
 */
size_t LIBNETXMS_EXPORTABLE wchar_to_mb(const wchar_t *src, ssize_t srcLen, char *dst, size_t dstLen)
{
#if HAVE_ICONV && !defined(__DISABLE_ICONV)
   if (dstLen == 0)
   {
      // Calculate required length. Because iconv cannot calculate
      // resulting multibyte string length, assume the worst case - 2 bytes per character
      return ((srcLen == -1) ? wcslen(src) : srcLen) * 2 + 1;
   }

#if UNICODE_UCS4
   if (g_defaultCodePageType == CodePageType::ISO8859_1)
      return ucs4_to_ISO8859_1(src, srcLen, dst, dstLen);
   if (g_defaultCodePageType == CodePageType::ASCII)
      return ucs4_to_ASCII(src, srcLen, dst, dstLen);
   if (g_defaultCodePageType == CodePageType::UTF8)
      return ucs4_to_utf8(src, srcLen, dst, dstLen);
#else
   if (g_defaultCodePageType == CodePageType::ISO8859_1)
      return ucs2_to_ISO8859_1(src, srcLen, dst, dstLen);
   if (g_defaultCodePageType == CodePageType::ASCII)
      return ucs2_to_ASCII(src, srcLen, dst, dstLen);
   if (g_defaultCodePageType == CodePageType::UTF8)
      return ucs2_to_utf8(src, srcLen, dst, dstLen);
#endif

   return WideCharToMultiByteIconv(nullptr, src, srcLen, dst, dstLen);
#else
   if (dstLen == 0)
   {
      return wcslen(src) + 1;
   }

#if UNICODE_UCS4
   if (g_defaultCodePageType == CodePageType::ISO8859_1)
      return ucs4_to_ISO8859_1(src, srcLen, dst, dstLen);
   if (g_defaultCodePageType == CodePageType::UTF8)
      return ucs4_to_utf8(src, srcLen, dst, dstLen);
   return ucs4_to_ASCII(src, srcLen, dst, dstLen);
#else
   if (g_defaultCodePageType == CodePageType::ISO8859_1)
      return ucs2_to_ISO8859_1(src, srcLen, dst, dstLen);
   if (g_defaultCodePageType == CodePageType::UTF8)
      return ucs2_to_utf8(src, srcLen, dst, dstLen);
   return ucs2_to_ASCII(src, srcLen, dst, dstLen);
#endif
#endif
}

#ifndef __DISABLE_ICONV

/**
 * Convert single-byte to UNICODE string using iconv
 */
static size_t MultiByteToWideCharIconv(const char *codepage, const char *src, ssize_t srcLen, wchar_t *dst, size_t dstLen)
{
   iconv_t cd = IconvOpen(UNICODE_CODEPAGE_NAME, (codepage != nullptr) ? codepage : g_cpDefault);
   if (cd == (iconv_t)(-1))
   {
#if UNICODE_UCS4
      return ASCII_to_ucs4(src, srcLen, dst, dstLen);
#else
      return ASCII_to_ucs2(src, srcLen, dst, dstLen);
#endif
   }

   const char *inbuf = src;
   size_t inbytes = (srcLen == -1) ? strlen(src) + 1 : srcLen;
   char *outbuf = (char *)dst;
   size_t outbytes = dstLen * sizeof(WCHAR);
   size_t count = iconv(cd, (ICONV_CONST char **)&inbuf, &inbytes, &outbuf, &outbytes);
   IconvClose(cd);

   if (count == (size_t)(-1))
   {
      if (errno == EILSEQ)
      {
         count = (dstLen * sizeof(WCHAR) - outbytes) / sizeof(WCHAR);
      }
      else
      {
         count = 0;
      }
   }
   else
   {
      count = (dstLen * sizeof(WCHAR) - outbytes) / sizeof(WCHAR);
   }
   if ((static_cast<size_t>(outbuf - (char *)dst) > sizeof(WCHAR)) && (*dst == 0xFEFF))
   {
      // Remove UNICODE byte order indicator if presented
      memmove(dst, &dst[1], outbuf - (char *)dst - sizeof(WCHAR));
      outbuf -= sizeof(WCHAR);
      count--;
   }
   if (outbytes >= sizeof(WCHAR))
   {
      *((WCHAR *)outbuf) = 0;
   }

   return count;
}

#endif   /* __DISABLE_ICONV */

/**
 * Convert multibyte string to wide character string
 */
size_t LIBNETXMS_EXPORTABLE mb_to_wchar(const char *src, ssize_t srcLen, wchar_t *dst, size_t dstLen)
{
   if (dstLen == 0)
      return strlen(src) + 1;

#if UNICODE_UCS4
   if (g_defaultCodePageType == CodePageType::ISO8859_1)
      return ISO8859_1_to_ucs4(src, srcLen, dst, dstLen);
   if (g_defaultCodePageType == CodePageType::ASCII)
      return ASCII_to_ucs4(src, srcLen, dst, dstLen);
   if (g_defaultCodePageType == CodePageType::UTF8)
      return utf8_to_ucs4(src, srcLen, dst, dstLen);
#else
   if (g_defaultCodePageType == CodePageType::ISO8859_1)
      return ISO8859_1_to_ucs2(src, srcLen, dst, dstLen);
   if (g_defaultCodePageType == CodePageType::ASCII)
      return ASCII_to_ucs2(src, srcLen, dst, dstLen);
   if (g_defaultCodePageType == CodePageType::UTF8)
      return UTF8_to_ucs2(src, srcLen, dst, dstLen);
#endif

#if HAVE_ICONV && !defined(__DISABLE_ICONV)
   return MultiByteToWideCharIconv(nullptr, src, srcLen, dst, dstLen);
#else
#if UNICODE_UCS4
   return ASCII_to_ucs4(src, srcLen, dst, dstLen);
#else
   return ASCII_to_ucs2(src, srcLen, dst, dstLen);
#endif
#endif
}

#endif   /* _WIN32 */

/**
 * Convert multibyte string to wide string using current LC_CTYPE setting and
 * allocating wide string dynamically
 */
WCHAR LIBNETXMS_EXPORTABLE *WideStringFromMBStringSysLocale(const char *src)
{
#ifdef _WIN32
   return WideStringFromMBString(src);
#else
   if (src == nullptr)
      return nullptr;
   size_t len = strlen(src) + 1;
   WCHAR *out = MemAllocStringW(len);
#if HAVE_MBSTOWCS
   mbstowcs(out, src, len);
#else
   mb_to_wchar(src, -1, out, len);
#endif
   return out;
#endif
}

/**
 * Convert multibyte string to wide string using current codepage and
 * allocating wide string dynamically
 */
WCHAR LIBNETXMS_EXPORTABLE *WideStringFromMBString(const char *src)
{
   if (src == nullptr)
      return nullptr;
   size_t len = strlen(src) + 1;
   WCHAR *out = MemAllocStringW(len);
   mb_to_wchar(src, -1, out, len);
   return out;
}

/**
 * Convert wide string to UTF8 string allocating wide string dynamically
 */
WCHAR LIBNETXMS_EXPORTABLE *WideStringFromUTF8String(const char *src)
{
   if (src == nullptr)
      return nullptr;
   size_t len = strlen(src) + 1;
   WCHAR *out = MemAllocStringW(len);
   utf8_to_wchar(src, -1, out, len);
   return out;
}

/**
 * Convert wide string to multibyte string using current codepage and
 * allocating multibyte string dynamically
 */
char LIBNETXMS_EXPORTABLE *MBStringFromWideString(const WCHAR *src)
{
   if (src == nullptr)
      return nullptr;
   size_t len = wcslen(src) + 1;
   char *out = MemAllocStringA(len);
   wchar_to_mb(src, -1, out, len);
   return out;
}

/**
 * Convert wide string to multibyte string using current LC_CTYPE setting and
 * allocating multibyte string dynamically
 */
char LIBNETXMS_EXPORTABLE *MBStringFromWideStringSysLocale(const WCHAR *src)
{
#ifdef _WIN32
   return MBStringFromWideString(src);
#else
   if (src == nullptr)
      return nullptr;
   size_t len = wcslen(src) * 3 + 1;  // add extra bytes in case of UTF-8 as target encoding
   char *out = MemAllocStringA(len);
#if HAVE_WCSTOMBS
   wcstombs(out, src, len);
#else
   wchar_to_mb(src, -1, out, len);
#endif
   return out;
#endif
}

/**
 * Convert wide string to UTF8 string allocating UTF8 string dynamically
 */
char LIBNETXMS_EXPORTABLE *UTF8StringFromWideString(const WCHAR *src)
{
   if (src == nullptr)
      return nullptr;
   size_t len = wchar_utf8len(src, -1);
   char *out = MemAllocStringA(len);
   wchar_to_utf8(src, -1, out, len);
   return out;
}

/**
 * Convert UTF8 string to multibyte string using current codepage and
 * allocating multibyte string dynamically
 */
char LIBNETXMS_EXPORTABLE *MBStringFromUTF8String(const char *s)
{
   if (s == nullptr)
      return nullptr;
   size_t len = strlen(s) + 1;
   char *out = MemAllocStringA(len);
   utf8_to_mb(s, -1, out, len);
   return out;
}

/**
 * Convert multibyte string to UTF8 string allocating UTF8 string dynamically
 */
char LIBNETXMS_EXPORTABLE *UTF8StringFromMBString(const char *s)
{
   if (s == nullptr)
      return nullptr;
   size_t len = strlen(s) * 3 + 1;   // assume worst case - 3 bytes per character
   char *out = MemAllocStringA(len);
   mb_to_utf8(s, -1, out, len);
   return out;
}

/**
 * Convert UCS-4 string to UCS-2 string allocating UCS-2 string dynamically
 */
UCS2CHAR LIBNETXMS_EXPORTABLE *UCS2StringFromUCS4String(const UCS4CHAR *src)
{
   if (src == nullptr)
      return nullptr;
   size_t len = ucs4_ucs2len(src, -1);
   UCS2CHAR *out = MemAllocArrayNoInit<UCS2CHAR>(len);
   ucs4_to_ucs2(src, -1, out, len);
   return out;
}

/**
 * Convert UCS-2 string to UCS-4 string allocating UCS-4 string dynamically
 */
UCS4CHAR LIBNETXMS_EXPORTABLE *UCS4StringFromUCS2String(const UCS2CHAR *src)
{
   if (src == nullptr)
      return nullptr;
   size_t len = ucs2_strlen(src) + 1;
   UCS4CHAR *out = MemAllocArrayNoInit<UCS4CHAR>(len);
   ucs2_to_ucs4(src, -1, out, static_cast<int>(len));
   return out;
}

#ifndef UNICODE_UCS2

/**
 * Convert UTF-8 to UCS-2 (dynamically allocated output string)
 */
UCS2CHAR LIBNETXMS_EXPORTABLE *UCS2StringFromUTF8String(const char *utf8String)
{
   if (utf8String == nullptr)
      return nullptr;
   size_t len = strlen(utf8String) + 1;
   UCS2CHAR *out = MemAllocArrayNoInit<UCS2CHAR>(len);
   utf8_to_ucs2(utf8String, -1, out, len);
   return out;
}

/**
 * Convert UCS-2 string to UTF-8 string allocating UTF-8 string dynamically
 */
char LIBNETXMS_EXPORTABLE *UTF8StringFromUCS2String(const UCS2CHAR *src)
{
   if (src == nullptr)
      return nullptr;
   size_t len = ucs2_utf8len(src, -1);
   char *out = MemAllocStringA(len);
   ucs2_to_utf8(src, -1, out, len);
   return out;
}

/**
 * Convert multibyte string to UCS-2 string allocating UCS-2 string dynamically
 */
UCS2CHAR LIBNETXMS_EXPORTABLE *UCS2StringFromMBString(const char *src)
{
   if (src == nullptr)
      return nullptr;
   size_t len = strlen(src) + 1;
   UCS2CHAR *out = MemAllocArrayNoInit<UCS2CHAR>(len);
   mb_to_ucs2(src, -1, out, len);
   return out;
}

/**
 * Convert UCS-2 string to multibyte string allocating multibyte string dynamically
 */
char LIBNETXMS_EXPORTABLE *MBStringFromUCS2String(const UCS2CHAR *src)
{
   if (src == nullptr)
      return nullptr;
   size_t len = ucs2_strlen(src) + 1;
   char *out = MemAllocStringA(len);
   ucs2_to_mb(src, -1, out, len);
   return out;
}

#endif /* UNICODE_UCS2 */

#ifndef UNICODE_UCS4

/**
 * Convert multibyte string to UCS-4 string allocating UCS-4 string dynamically
 */
UCS4CHAR LIBNETXMS_EXPORTABLE *UCS4StringFromMBString(const char *src)
{
   if (src == nullptr)
      return nullptr;
   size_t len = strlen(src) + 1;
   UCS4CHAR *out = MemAllocArrayNoInit<UCS4CHAR>(len);
   mb_to_ucs4(src, -1, out, len);
   return out;
}

/**
 * Convert UCS-4 string to multibyte string allocating multibyte string dynamically
 */
char LIBNETXMS_EXPORTABLE *MBStringFromUCS4String(const UCS4CHAR *src)
{
   if (src == nullptr)
      return nullptr;
   size_t len = ucs4_strlen(src) + 1;
   char *out = MemAllocStringA(len);
   ucs4_to_mb(src, -1, out, len);
   return out;
}

#endif /* UNICODE_UCS4 */

#ifdef _WIN32

/**
 * Code page
 */
struct CodePage
{
   const char *name;
   size_t length;
   int value;

   CodePage(const char *n, int value)
   {
      name = n;
      length = (n != nullptr) ? strlen(n) : 0;
   }
};

/**
 * Code page mappings
 */
static CodePage s_codePages[] =
{
   CodePage("ARABIC", 708),
   CodePage("ASCII", 20127),
   CodePage("ASMO-449+", 709),
   CodePage("ASMO-708", 708),
   CodePage("BIG-5", 950),
   CodePage("BIG-FIVE", 950),
   CodePage("BIG5", 950),
   CodePage("BIGFIVE", 950),
   CodePage("CN-BIG5", 950),
   CodePage("CN-GB", 936),
   CodePage("CP-IS", 861),
   CodePage("CP-GR", 869),
   CodePage("CP1250", 1250),
   CodePage("CP1251", 1251),
   CodePage("CP1252", 1252),
   CodePage("CP1253", 1253),
   CodePage("CP1254", 1254),
   CodePage("CP1255", 1255),
   CodePage("CP1256", 1256),
   CodePage("CP1257", 1257),
   CodePage("CP1258", 1258),
   CodePage("CP1361", 1361),
   CodePage("CP367", 20127),
   CodePage("CP437", 437),
   CodePage("CP737", 737),
   CodePage("CP775", 775),
   CodePage("CP819", 28591),
   CodePage("CP852", 852),
   CodePage("CP855", 855),
   CodePage("CP857", 857),
   CodePage("CP858", 858),
   CodePage("CP860", 860),
   CodePage("CP861", 861),
   CodePage("CP863", 863),
   CodePage("CP864", 864),
   CodePage("CP865", 865),
   CodePage("CP866", 866),
   CodePage("CP869", 869),
   CodePage("CP875", 875),
   CodePage("CS", 437),
   CodePage("CSASCII", 20127),
   CodePage("CSBIG5", 950),
   CodePage("CSGB2312", 936),
   CodePage("CSIBM855", 855),
   CodePage("CSIBM857", 857),
   CodePage("CSIBM860", 860),
   CodePage("CSIBM861", 861),
   CodePage("CSIBM863", 863),
   CodePage("CSIBM864", 864),
   CodePage("CSIBM865", 865),
   CodePage("CSIBM869", 869),
   CodePage("CSKOI8R", 20866),
   CodePage("CSKSC56011987", 949),
   CodePage("CSMACINTOSH", 10000),
   CodePage("CSPC775BALTIC", 775),
   CodePage("CSPCP852", 852),
   CodePage("CSSHIFTJIS", 932),
   CodePage("CYRILLIC", 28595),
   CodePage("DOS-720", 720),
   CodePage("DOS-862", 862),
   CodePage("ECMA-114", 708),
   CodePage("EUC-CN", 936),
   CodePage("EUCCN", 936),
   CodePage("GB2312", 936),
   CodePage("IBM037", 37),
   CodePage("IBM1026", 1026),
   CodePage("IBM1047", 1047),
   CodePage("IBM1140", 1140),
   CodePage("IBM1141", 1141),
   CodePage("IBM1142", 1142),
   CodePage("IBM1143", 1143),
   CodePage("IBM1144", 1144),
   CodePage("IBM1145", 1145),
   CodePage("IBM1146", 1146),
   CodePage("IBM1147", 1147),
   CodePage("IBM1148", 1148),
   CodePage("IBM1149", 1149),
   CodePage("IBM367", 20127),
   CodePage("IBM437", 437),
   CodePage("IBM500", 500),
   CodePage("IBM737", 737),
   CodePage("IBM775", 775),
   CodePage("IBM819", 28591),
   CodePage("IBM850", 850),
   CodePage("IBM852", 852),
   CodePage("IBM855", 855),
   CodePage("IBM857", 857),
   CodePage("IBM858", 858),
   CodePage("IBM860", 860),
   CodePage("IBM861", 861),
   CodePage("IBM863", 863),
   CodePage("IBM864", 864),
   CodePage("IBM865", 865),
   CodePage("IBM869", 869),
   CodePage("IBM870", 870),
   CodePage("ISO-8859-1", 28591),
   CodePage("ISO-8859-2", 28592),
   CodePage("ISO-8859-3", 28593),
   CodePage("ISO-8859-4", 28594),
   CodePage("ISO-8859-5", 28595),
   CodePage("ISO-8859-6", 28596),
   CodePage("ISO-8859-7", 28597),
   CodePage("ISO-8859-8", 28598),
   CodePage("ISO-8859-9", 28599),
   CodePage("ISO-8859-13", 28603),
   CodePage("ISO-8859-15", 28605),
   CodePage("ISO-IR-149", 949),
   CodePage("JOHAB", 1361),
   CodePage("KOI8-R", 20866),
   CodePage("KOI8-U", 21866),
   CodePage("KOREAN KSC_5601", 949),
   CodePage("KS_C_5601-1987", 949),
   CodePage("KS_C_5601-1989", 949),
   CodePage("LATIN1", 28591),
   CodePage("LATIN2", 28592),
   CodePage("LATIN3", 28593),
   CodePage("LATIN4", 28594),
   CodePage("MAC", 10000),
   CodePage("MACINTOSH", 10000),
   CodePage("MACROMAN", 10000),
   CodePage("MS-ANSI", 1252),
   CodePage("MS-ARAB", 1256),
   CodePage("MS-CYRL", 1251),
   CodePage("MS-EE", 1250),
   CodePage("MS-GREEK", 1253),
   CodePage("MS-HEBR", 1255),
   CodePage("MS-TURK", 1254),
   CodePage("MS_KANJI", 932),
   CodePage("SHIFT-JIS", 932),
   CodePage("SHIFT_JIS", 932),
   CodePage("SJIS", 932),
   CodePage("US", 20127),
   CodePage("US-ASCII", 20127),
   CodePage("UTF-7", 65000),
   CodePage("WINBALTRIM", 1257),
   CodePage("WINDOWS-1250", 1250),
   CodePage("WINDOWS-1251", 1251),
   CodePage("WINDOWS-1252", 1252),
   CodePage("WINDOWS-1253", 1253),
   CodePage("WINDOWS-1254", 1254),
   CodePage("WINDOWS-1255", 1255),
   CodePage("WINDOWS-1256", 1256),
   CodePage("WINDOWS-1257", 1257),
   CodePage("WINDOWS-1258", 1258),
   CodePage("WINDOWS-874", 874),
   CodePage(nullptr, 0)
};

/**
 * Get Windows code page number from code page name
 */
static int GetCodePageNumber(const char *codepage)
{
   char cp[32];
   size_t clen = strlen(codepage);
   if (clen > 31)
      return CP_ACP;

   memcpy(cp, codepage, clen + 1);
   strupr(cp);

   for (int i = 0; s_codePages[i].value != 0; i++)
   {
      if (s_codePages[i].length != clen)
         continue;
      if (!memcmp(s_codePages[i].name, cp, clen))
         return s_codePages[i].value;
   }

   return CP_ACP;
}

#endif

/**
 * Convert wide character string to multibyte character string using given codepage
 */
size_t LIBNETXMS_EXPORTABLE wchar_to_mbcp(const wchar_t *src, ssize_t srcLen, char *dst, size_t dstLen, const char *codepage)
{
   if (codepage == nullptr)
      return wchar_to_mb(src, srcLen, dst, dstLen);

   if (!stricmp(codepage, "ASCII"))
   {
#if UNICODE_UCS4
      return ucs4_to_ASCII(src, srcLen, dst, dstLen);
#else
      return ucs2_to_ASCII(src, srcLen, dst, dstLen);
#endif
   }
   if (!stricmp(codepage, "UTF8") || !stricmp(codepage, "UTF-8"))
   {
#if UNICODE_UCS4
      return ucs4_to_utf8(src, srcLen, dst, dstLen);
#else
      return ucs2_to_utf8(src, srcLen, dst, dstLen);
#endif
   }
   if (!stricmp(codepage, "ISO-8859-1") || !stricmp(codepage, "ISO_8859_1") ||
       !stricmp(codepage, "ISO8859-1") || !stricmp(codepage, "ISO8859_1") ||
       !stricmp(codepage, "LATIN-1") || !stricmp(codepage, "LATIN1"))
   {
#if UNICODE_UCS4
      return ucs4_to_ISO8859_1(src, srcLen, dst, dstLen);
#else
      return ucs2_to_ISO8859_1(src, srcLen, dst, dstLen);
#endif
   }

#if defined(_WIN32)
   return (size_t)WideCharToMultiByte(GetCodePageNumber(codepage), WC_COMPOSITECHECK | WC_DEFAULTCHAR, src, (int)srcLen, dst, (int)dstLen, nullptr, nullptr);
#elif !defined(__DISABLE_ICONV)
   return WideCharToMultiByteIconv(codepage, src, srcLen, dst, dstLen);
#elif UNICODE_UCS4
   return ucs4_to_ISO8859_1(src, srcLen, dst, dstLen);
#else
   return ucs2_to_ISO8859_1(src, srcLen, dst, dstLen);
#endif
}

/**
 * Convert multibyte character string to wide character string using given codepage
 */
size_t LIBNETXMS_EXPORTABLE mbcp_to_wchar(const char *src, ssize_t srcLen, wchar_t *dst, size_t dstLen, const char *codepage)
{
   if (codepage == nullptr)
      return mb_to_wchar(src, srcLen, dst, dstLen);

   if (!stricmp(codepage, "ASCII"))
   {
#if UNICODE_UCS4
      return ASCII_to_ucs4(src, srcLen, dst, dstLen);
#else
      return ASCII_to_ucs2(src, srcLen, dst, dstLen);
#endif
   }
   if (!stricmp(codepage, "UTF8") || !stricmp(codepage, "UTF-8"))
   {
#if UNICODE_UCS4
      return utf8_to_ucs4(src, srcLen, dst, dstLen);
#else
      return utf8_to_ucs2(src, srcLen, dst, dstLen);
#endif
   }
   if (!stricmp(codepage, "ISO-8859-1") || !stricmp(codepage, "ISO_8859_1") ||
       !stricmp(codepage, "ISO8859-1") || !stricmp(codepage, "ISO8859_1") ||
       !stricmp(codepage, "LATIN-1") || !stricmp(codepage, "LATIN1"))
   {
#if UNICODE_UCS4
      return ISO8859_1_to_ucs4(src, srcLen, dst, dstLen);
#else
      return ISO8859_1_to_ucs2(src, srcLen, dst, dstLen);
#endif
   }

#if defined(_WIN32)
   return (size_t)MultiByteToWideChar(GetCodePageNumber(codepage), MB_PRECOMPOSED, src, (int)srcLen, dst, (int)dstLen);
#elif !defined(__DISABLE_ICONV)
   return MultiByteToWideCharIconv(codepage, src, srcLen, dst, dstLen);
#elif UNICODE_UCS4
   return ISO8859_1_to_ucs4(src, srcLen, dst, dstLen);
#else
   return ISO8859_1_to_ucs2(src, srcLen, dst, dstLen);
#endif
}

/**
 * Convert UTF8 string to multibyte character string using given codepage
 */
size_t LIBNETXMS_EXPORTABLE utf8_to_mbcp(const char *src, ssize_t srcLen, char *dst, size_t dstLen, const char *codepage)
{
   if (codepage == nullptr)
      return utf8_to_mb(src, srcLen, dst, dstLen);

   if (!stricmp(codepage, "ASCII"))
   {
      return utf8_to_ASCII(src, srcLen, dst, dstLen);
   }
   if (!stricmp(codepage, "UTF8") || !stricmp(codepage, "UTF-8"))
   {
      if (srcLen == -1)
         srcLen = strlen(src) + 1;
      memcpy(dst, src, std::min((size_t)srcLen, dstLen));
   }
   if (!stricmp(codepage, "ISO-8859-1") || !stricmp(codepage, "ISO_8859_1") ||
       !stricmp(codepage, "ISO8859-1") || !stricmp(codepage, "ISO8859_1") ||
       !stricmp(codepage, "LATIN-1") || !stricmp(codepage, "LATIN1"))
   {
      return utf8_to_ISO8859_1(src, srcLen, dst, dstLen);
   }

   if(srcLen == -1)
      srcLen = strlen(src) + 1;

#if defined(_WIN32)
   WCHAR *buffer = (srcLen <= 32768) ? (WCHAR *)alloca(srcLen * sizeof(WCHAR)) : (WCHAR *)MemAlloc(srcLen * sizeof(WCHAR));
   int ret = MultiByteToWideChar(CP_UTF8, 0, src, (int)srcLen, buffer, (int)srcLen);
   ret = WideCharToMultiByte(GetCodePageNumber(codepage), WC_DEFAULTCHAR | WC_COMPOSITECHECK, buffer, ret, dst, (int)dstLen, NULL, NULL);
   if (srcLen > 32768)
      MemFree(buffer);
   return ret;
#elif !defined(__DISABLE_ICONV)
   iconv_t cd = IconvOpen(codepage, "UTF-8");
   if(cd == (iconv_t)-1)
      return utf8_to_ISO8859_1(src, srcLen, dst, dstLen);
   size_t count = iconv(cd, (ICONV_CONST char**)&src, (size_t*)&srcLen, &dst, &dstLen);
   IconvClose(cd);
   return count;
#else
   return utf8_to_ISO8859_1(src, srcLen, dst, dstLen);
#endif
}

/**
 * Convert multibyte character string to UTF8 string using given codepage
 */
size_t LIBNETXMS_EXPORTABLE mbcp_to_utf8(const char *src, ssize_t srcLen, char *dst, size_t dstLen, const char *codepage)
{
   if (codepage == nullptr)
      return mb_to_utf8(src, srcLen, dst, dstLen);

   if (!stricmp(codepage, "ASCII"))
   {
      return ASCII_to_utf8(src, srcLen, dst, dstLen);
   }
   if (!stricmp(codepage, "UTF8") || !stricmp(codepage, "UTF-8"))
   {
      if (srcLen == -1)
         srcLen = strlen(src) + 1;
      memcpy(dst, src, std::min((size_t)srcLen, dstLen));
   }
   if (!stricmp(codepage, "ISO-8859-1") || !stricmp(codepage, "ISO_8859_1") ||
       !stricmp(codepage, "ISO8859-1") || !stricmp(codepage, "ISO8859_1") ||
       !stricmp(codepage, "LATIN-1") || !stricmp(codepage, "LATIN1"))
   {
      return ISO8859_1_to_utf8(src, srcLen, dst, dstLen);
   }

   if(srcLen == -1)
      srcLen = strlen(src) + 1;

#if defined(_WIN32)
   WCHAR *buffer = (srcLen <= 32768) ? (WCHAR *)alloca(srcLen * sizeof(WCHAR)) : (WCHAR *)MemAlloc(srcLen * sizeof(WCHAR));
   int ret = MultiByteToWideChar(GetCodePageNumber(codepage), 0, src, (int)srcLen, buffer, (int)srcLen);
   ret = WideCharToMultiByte(CP_UTF8, WC_DEFAULTCHAR | WC_COMPOSITECHECK, buffer, ret, dst, (int)dstLen, NULL, NULL);
   if (srcLen > 32768)
      MemFree(buffer);
   return ret;
#elif !defined(__DISABLE_ICONV)
   iconv_t cd = IconvOpen("UTF-8", codepage);
   if(cd == (iconv_t)-1)
      return utf8_to_ISO8859_1(src, srcLen, dst, dstLen);
   size_t count = iconv(cd, (ICONV_CONST char**)&src, (size_t*)&srcLen, &dst, &dstLen);
   IconvClose(cd);
   return count;
#else
   return ISO8859_1_to_utf8(src, srcLen, dst, dstLen);
#endif
}

/*** UNIX UNICODE specific wrapper functions ***/

#if !defined(_WIN32)

/**
 * Wide char version of fopen (Windows compatibility layer)
 */
FILE LIBNETXMS_EXPORTABLE *_wfopen(const wchar_t *_name, const wchar_t *_type)
{
   char name[MAX_PATH], type[128];
   WideCharToMultiByteSysLocale(_name, name, sizeof(name));
   WideCharToMultiByteSysLocale(_type, type, sizeof(type));
   return fopen(name, type);
}

#if defined(UNICODE)

/**
 * Wide character version of some libc functions
 */
#define DEFINE_PATH_FUNC(func) \
int LIBNETXMS_EXPORTABLE _w##func(const WCHAR *_path) \
{ \
	char path[MAX_PATH]; \
	WideCharToMultiByteSysLocale(_path, path, sizeof(path)); \
	return func(path); \
}

DEFINE_PATH_FUNC(rmdir)
DEFINE_PATH_FUNC(remove)

/**
 * Wide char version of open (Windows compatibility layer)
 */
int LIBNETXMS_EXPORTABLE _wopen(const wchar_t *_name, int flags, ...)
{
   char name[MAX_PATH];
   WideCharToMultiByteSysLocale(_name, name, sizeof(name));
   int rc;
   if (flags & O_CREAT)
   {
      va_list args;
      va_start(args, flags);
      rc = open(name, flags, (mode_t)va_arg(args, int));
      va_end(args);
   }
   else
   {
      rc = open(name, flags);
   }
   return rc;
}

/**
 * Wide char version of chmod (Windows compatibility layer)
 */
int LIBNETXMS_EXPORTABLE _wchmod(const wchar_t *_path, int mode)
{
   char path[MAX_PATH];
   WideCharToMultiByteSysLocale(_path, path, sizeof(path));
   return chmod(path, mode);
}

/**
 * Wide char version of chmod (Windows compatibility layer)
 */
int LIBNETXMS_EXPORTABLE _wrename(const wchar_t *_oldpath, const wchar_t *_newpath)
{
   char oldpath[MAX_PATH], newpath[MAX_PATH];
   WideCharToMultiByteSysLocale(_oldpath, oldpath, sizeof(oldpath));
   WideCharToMultiByteSysLocale(_newpath, newpath, sizeof(newpath));
   return rename(oldpath, newpath);
}

/**
 * Wide char version of access (Windows compatibility layer)
 */
int LIBNETXMS_EXPORTABLE _waccess(const wchar_t *_path, int mode)
{
   char path[MAX_PATH];
   WideCharToMultiByteSysLocale(_path, path, sizeof(path));
   return access(path, mode);
}

/**
 * Wide char version of mkdir (Windows compatibility layer)
 */
int LIBNETXMS_EXPORTABLE _wmkdir(const wchar_t *_path, int mode)
{
   char path[MAX_PATH];
   WideCharToMultiByteSysLocale(_path, path, sizeof(path));
   return mkdir(path, mode);
}

/**
 * Wide char version of strerror (Windows compatibility layer)
 */
wchar_t LIBNETXMS_EXPORTABLE *_wcserror(int errnum)
{
   static thread_local wchar_t value[256];
#if HAVE_STRERROR_R
   char buffer[256];
#if HAVE_POSIX_STRERROR_R
   strerror_r(errnum, buffer, 256);
   MultiByteToWideCharSysLocale(buffer, value, 256);
#else
   MultiByteToWideCharSysLocale(strerror_r(errnum, buffer, 256), value, 256);
#endif
#else
   MultiByteToWideCharSysLocale(strerror(errnum), value, 256);
#endif
   return value;
}

#if HAVE_STRERROR_R

#if HAVE_POSIX_STRERROR_R
int LIBNETXMS_EXPORTABLE wcserror_r(int errnum, wchar_t *strerrbuf, size_t buflen)
#else
wchar_t LIBNETXMS_EXPORTABLE *wcserror_r(int errnum, wchar_t *strerrbuf, size_t buflen)
#endif /* HAVE_POSIX_STRERROR_R */
{
#if HAVE_POSIX_STRERROR_R
   int err = 0;
#endif /* HAVE_POSIX_STRERROR_R */

   Buffer<char, 1024> mbbuf(buflen);
#if HAVE_POSIX_STRERROR_R
   err = strerror_r(errnum, mbbuf, buflen);
   MultiByteToWideCharSysLocale(mbbuf, strerrbuf, buflen);
#else
   MultiByteToWideCharSysLocale(strerror_r(errnum, mbbuf, buflen), strerrbuf, buflen);
#endif

#if HAVE_POSIX_STRERROR_R
   return err;
#else
   return strerrbuf;
#endif
}

#endif /* !HAVE_WCSERROR_R && HAVE_STRERROR_R */

/*************************************************************************
 * Wrappers for wprintf/wscanf family
 *
 * All these wrappers replaces %s with %S and %c with %C
 * because in POSIX version of wprintf functions %s and %c
 * means "multibyte string/char" instead of expected "UNICODE string/char"
 *************************************************************************/

/**
 * Internal version of wprintf() with Windows compatible format specifiers
 */
int LIBNETXMS_EXPORTABLE nx_wprintf(const wchar_t *format, ...)
{
   va_list args;
   va_start(args, format);
   int rc = nx_vwprintf(format, args);
   va_end(args);
   return rc;
}

/**
 * Internal version of fwprintf() with Windows compatible format specifiers
 */
int LIBNETXMS_EXPORTABLE nx_fwprintf(FILE *fp, const wchar_t *format, ...)
{
   va_list args;
   va_start(args, format);
   int rc = nx_vfwprintf(fp, format, args);
   va_end(args);
   return rc;
}

/**
 * Internal version of swprintf() with Windows compatible format specifiers
 */
int LIBNETXMS_EXPORTABLE nx_swprintf(wchar_t *buffer, size_t size, const wchar_t *format, ...)
{
   va_list args;
   va_start(args, format);
   int rc = nx_vswprintf(buffer, size, format, args);
   va_end(args);
   return rc;
}

/**
 * Internal version of wscanf() with Windows compatible format specifiers
 */
int LIBNETXMS_EXPORTABLE nx_wscanf(const wchar_t *format, ...)
{
   va_list args;
   va_start(args, format);
   int rc = nx_vwscanf(format, args);
   va_end(args);
   return rc;
}

/**
 * Internal version of fwscanf() with Windows compatible format specifiers
 */
int LIBNETXMS_EXPORTABLE nx_fwscanf(FILE *fp, const wchar_t *format, ...)
{
   va_list args;
   va_start(args, format);
   int rc = nx_vfwscanf(fp, format, args);
   va_end(args);
   return rc;
}

/**
 * Internal version of swscanf() with Windows compatible format specifiers
 */
int LIBNETXMS_EXPORTABLE nx_swscanf(const wchar_t *str, const wchar_t *format, ...)
{
   va_list args;
   va_start(args, format);
   int rc = nx_vswscanf(str, format, args);
   va_end(args);
   return rc;
}

/**
 * Length of local format buffer
 */
#define LOCAL_FORMAT_BUFFER_LENGTH  256

/**
 * Replace string/char format specifiers from Windows to POSIX version
 */
static wchar_t *ReplaceFormatSpecs(const wchar_t *oldFormat, wchar_t *localBuffer)
{
   size_t len = wcslen(oldFormat) + 1;
   wchar_t *fmt = (len <= LOCAL_FORMAT_BUFFER_LENGTH) ? localBuffer : MemAllocStringW(len);
   memcpy(fmt, oldFormat, len * sizeof(wchar_t));

   int state = 0;
   bool hmod;
   for(wchar_t *p = fmt; *p != 0; p++)
   {
      switch (state)
      {
         case 0:	// Normal text
            if (*p == L'%')
            {
               state = 1;
               hmod = false;
            }
            break;
         case 1:	// Format start
            switch (*p)
            {
               case L's':
                  if (hmod)
                  {
                     memmove(p - 1, p, wcslen(p - 1) * sizeof(TCHAR));
                  }
                  else
                  {
                     *p = L'S';
                  }
                  state = 0;
                  break;
               case L'S':
                  *p = L's';
                  state = 0;
                  break;
               case L'c':
                  if (hmod)
                  {
                     memmove(p - 1, p, wcslen(p - 1) * sizeof(TCHAR));
                  }
                  else
                  {
                     *p = L'C';
                  }
                  state = 0;
                  break;
               case L'C':
                  *p = L'c';
                  state = 0;
                  break;
               case L'.':	// All this characters could be part of format specifier
               case L'*':	// and has no interest for us
               case L'+':
               case L'-':
               case L' ':
               case L'#':
               case L'0':
               case L'1':
               case L'2':
               case L'3':
               case L'4':
               case L'5':
               case L'6':
               case L'7':
               case L'8':
               case L'9':
               case L'l':
               case L'L':
               case L'F':
               case L'N':
               case L'w':
                  break;
               case L'h':	// check for %hs
                  hmod = true;
                  break;
               default:		// All other characters means end of format
                  state = 0;
                  break;

            }
            break;
      }
   }
   return fmt;
}

/**
 * Free format buffer of it is not local
 */
#define FreeFormatBuffer(b) do { if (b != localBuffer) MemFree(b); } while(0)

/**
 * Internal version of vwprintf() with Windows compatible format specifiers
 */
int LIBNETXMS_EXPORTABLE nx_vwprintf(const wchar_t *format, va_list args)
{
   wchar_t localBuffer[LOCAL_FORMAT_BUFFER_LENGTH];
   wchar_t *fmt = ReplaceFormatSpecs(format, localBuffer);
   int rc = vwprintf(fmt, args);
   FreeFormatBuffer(fmt);
   return rc;
}

/**
 * Internal version of vfwprintf() with Windows compatible format specifiers
 */
int LIBNETXMS_EXPORTABLE nx_vfwprintf(FILE *fp, const wchar_t *format, va_list args)
{
   wchar_t localBuffer[LOCAL_FORMAT_BUFFER_LENGTH];
   wchar_t *fmt = ReplaceFormatSpecs(format, localBuffer);
   int rc = vfwprintf(fp, fmt, args);
   FreeFormatBuffer(fmt);
   return rc;
}

/**
 * Internal version of vswprintf() with Windows compatible format specifiers
 */
int LIBNETXMS_EXPORTABLE nx_vswprintf(WCHAR *buffer, size_t size, const wchar_t *format, va_list args)
{
   wchar_t localBuffer[LOCAL_FORMAT_BUFFER_LENGTH];
   wchar_t *fmt = ReplaceFormatSpecs(format, localBuffer);
   int rc = vswprintf(buffer, size, fmt, args);
   FreeFormatBuffer(fmt);
   return rc;
}

/**
 * Internal version of vwscanf() with Windows compatible format specifiers
 */
int LIBNETXMS_EXPORTABLE nx_vwscanf(const wchar_t *format, va_list args)
{
   wchar_t localBuffer[LOCAL_FORMAT_BUFFER_LENGTH];
   wchar_t *fmt = ReplaceFormatSpecs(format, localBuffer);
#if HAVE_VWSCANF
   int rc = vwscanf(fmt, args);
#else
   int rc = -1; // FIXME: add workaround implementation
#endif
   FreeFormatBuffer(fmt);
   return rc;
}

/**
 * Internal version of vfwscanf() with Windows compatible format specifiers
 */
int LIBNETXMS_EXPORTABLE nx_vfwscanf(FILE *fp, const wchar_t *format, va_list args)
{
   wchar_t localBuffer[LOCAL_FORMAT_BUFFER_LENGTH];
   wchar_t *fmt = ReplaceFormatSpecs(format, localBuffer);
#if HAVE_VFWSCANF
   int rc = vfwscanf(fp, fmt, args);
#else
   int rc = -1; // FIXME: add workaround implementation
#endif
   FreeFormatBuffer(fmt);
   return rc;
}

/**
 * Internal version of vswscanf() with Windows compatible format specifiers
 */
int LIBNETXMS_EXPORTABLE nx_vswscanf(const wchar_t *str, const wchar_t *format, va_list args)
{
   wchar_t localBuffer[LOCAL_FORMAT_BUFFER_LENGTH];
   wchar_t *fmt = ReplaceFormatSpecs(format, localBuffer);
#if HAVE_VSWSCANF
   int rc = vswscanf(str, fmt, args);
#else
   int rc = -1; // FIXME: add workaround implementation
#endif
   FreeFormatBuffer(fmt);
   return rc;
}

#endif /* defined(UNICODE) */

#endif /* !defined(_WIN32) */

#if defined(_WITH_ENCRYPTION) && WITH_OPENSSL

/**
 * Get OpenSSL error string as UNICODE string
 * Buffer must be at least 256 character long
 */
wchar_t LIBNETXMS_EXPORTABLE *ERR_error_string_W(int errorCode, wchar_t *buffer)
{
   char text[256];
   memset(text, 0, sizeof(text));
   ERR_error_string(errorCode, text);
   MultiByteToWideCharSysLocale(text, buffer, 256);
   return buffer;
}

#endif
