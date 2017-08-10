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
      strncpy(g_cpDefault, cp, MAX_CODEPAGE_LEN);
      g_cpDefault[MAX_CODEPAGE_LEN - 1] = 0;
      rc = true;
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
   return (UCS2CHAR *)nx_memdup(src, (ucs2_strlen(src) + 1) * sizeof(UCS2CHAR));
}

#endif

#ifndef UNICODE_UCS4

/**
 * Duplicate UCS-4 character string
 */
UCS4CHAR LIBNETXMS_EXPORTABLE *ucs4_strdup(const UCS4CHAR *src)
{
   return (UCS4CHAR *)nx_memdup(src, (ucs4_strlen(src) + 1) * sizeof(UCS4CHAR));
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

/**
 * Convert UNICODE string to single-byte string using simple byte copy (works for ASCII only)
 */
static int WideCharToMultiByteSimpleCopy(int iCodePage, DWORD dwFlags, const WCHAR *pWideCharStr,
                                         int cchWideChar, char *pByteStr, int cchByteChar, char *pDefaultChar, BOOL *pbUsedDefChar)
{
   const WCHAR *pSrc;
   char *pDest;
   int iPos, iSize;

   iSize = (cchWideChar == -1) ? wcslen(pWideCharStr) : cchWideChar;
   if (iSize >= cchByteChar)
      iSize = cchByteChar - 1;
   for(pSrc = pWideCharStr, iPos = 0, pDest = pByteStr; iPos < iSize; iPos++, pSrc++, pDest++)
      *pDest = (*pSrc < 128) ? (char)(*pSrc) : '?';
   *pDest = 0;

   return iSize;
}

#ifndef __DISABLE_ICONV

/**
 * Convert UNICODE string to single-byte string using iconv
 */
static int WideCharToMultiByteIconv(int iCodePage, DWORD dwFlags, const WCHAR *pWideCharStr, int cchWideChar,
                                    char *pByteStr, int cchByteChar, char *pDefaultChar, BOOL *pbUsedDefChar)
{
   iconv_t cd;
   int nRet;
   const char *inbuf;
   char *outbuf;
   size_t inbytes, outbytes;
   char cp[MAX_CODEPAGE_LEN + 16];

   strcpy(cp, g_cpDefault);
#if HAVE_ICONV_IGNORE
   strcat(cp, "//IGNORE");
#endif /* HAVE_ICONV_IGNORE */

   cd = IconvOpen(iCodePage == CP_UTF8 ? "UTF-8" : cp, UNICODE_CODEPAGE_NAME);
   if (cd == (iconv_t)(-1))
   {
      return WideCharToMultiByteSimpleCopy(iCodePage, dwFlags, pWideCharStr, cchWideChar, pByteStr, cchByteChar, pDefaultChar, pbUsedDefChar);
   }

   inbuf = (const char *)pWideCharStr;
   inbytes = ((cchWideChar == -1) ? wcslen(pWideCharStr) + 1 : cchWideChar) * sizeof(WCHAR);
   outbuf = pByteStr;
   outbytes = cchByteChar;
   nRet = iconv(cd, (ICONV_CONST char **)&inbuf, &inbytes, &outbuf, &outbytes);
   IconvClose(cd);
   if (nRet == -1)
   {
      if (errno == EILSEQ)
      {
         nRet = cchByteChar - outbytes;
      }
      else
      {
         nRet = 0;
      }
   }
   else
   {
      nRet = cchByteChar - outbytes;
   }
   if (outbytes > 0)
   {
      *outbuf = 0;
   }

   return nRet;
}

#endif

/**
 * Convert UNICODE string to single-byte string
 */
int LIBNETXMS_EXPORTABLE WideCharToMultiByte(int iCodePage, DWORD dwFlags, const WCHAR *pWideCharStr, int cchWideChar,
                                             char *pByteStr, int cchByteChar, char *pDefaultChar, BOOL *pbUsedDefChar)
{
#if HAVE_ICONV && !defined(__DISABLE_ICONV)
   // Calculate required length. Because iconv cannot calculate
   // resulting multibyte string length, assume the worst case - 3 bytes
   // per character for UTF-8 and 2 bytes per character for other encodings
   if (cchByteChar == 0)
   {
      return wcslen(pWideCharStr) * (iCodePage == CP_UTF8 ? 3 : 2) + 1;
   }

   return WideCharToMultiByteIconv(iCodePage, dwFlags, pWideCharStr, cchWideChar, pByteStr, cchByteChar, pDefaultChar, pbUsedDefChar);
#else
   if (cchByteChar == 0)
   {
      return wcslen(pWideCharStr) + 1;
   }

   return WideCharToMultiByteSimpleCopy(iCodePage, dwFlags, pWideCharStr, cchWideChar, pByteStr, cchByteChar, pDefaultChar, pbUsedDefChar);
#endif
}

/**
 * Convert single-byte to UNICODE string using simple byte copy (works correctly for ASCII only)
 */
static int MultiByteToWideCharSimpleCopy(int iCodePage, DWORD dwFlags, const char *pByteStr, int cchByteChar, WCHAR *pWideCharStr, int cchWideChar)
{
   const char *pSrc;
   WCHAR *pDest;
   int iPos, iSize;

   iSize = (cchByteChar == -1) ? strlen(pByteStr) : cchByteChar;
   if (iSize >= cchWideChar)
      iSize = cchWideChar - 1;
   for(pSrc = pByteStr, iPos = 0, pDest = pWideCharStr; iPos < iSize; iPos++, pSrc++, pDest++)
      *pDest = (((BYTE)*pSrc & 0x80) == 0) ? (WCHAR)(*pSrc) : L'?';
   *pDest = 0;

   return iSize;
}

#ifndef __DISABLE_ICONV

/**
 * Convert single-byte to UNICODE string using iconv
 */
static int MultiByteToWideCharIconv(int iCodePage, DWORD dwFlags, const char *pByteStr, int cchByteChar, WCHAR *pWideCharStr, int cchWideChar)
{
   iconv_t cd;
   int nRet;
   const char *inbuf;
   char *outbuf;
   size_t inbytes, outbytes;

   cd = IconvOpen(UNICODE_CODEPAGE_NAME, iCodePage == CP_UTF8 ? "UTF-8" : g_cpDefault);
   if (cd == (iconv_t)(-1))
   {
      return MultiByteToWideCharSimpleCopy(iCodePage, dwFlags, pByteStr, cchByteChar, pWideCharStr, cchWideChar);
   }

   inbuf = pByteStr;
   inbytes = (cchByteChar == -1) ? strlen(pByteStr) + 1 : cchByteChar;
   outbuf = (char *)pWideCharStr;
   outbytes = cchWideChar * sizeof(WCHAR);
   nRet = iconv(cd, (ICONV_CONST char **)&inbuf, &inbytes, &outbuf, &outbytes);
   IconvClose(cd);

   if (nRet == -1)
   {
      if (errno == EILSEQ)
      {
         nRet = (cchWideChar * sizeof(WCHAR) - outbytes) / sizeof(WCHAR);
      }
      else
      {
         nRet = 0;
      }
   }
   else
   {
      nRet = (cchWideChar * sizeof(WCHAR) - outbytes) / sizeof(WCHAR);
   }
   if (((char *) outbuf - (char *) pWideCharStr > sizeof(WCHAR)) && (*pWideCharStr == 0xFEFF))
   {
      // Remove UNICODE byte order indicator if presented
      memmove(pWideCharStr, &pWideCharStr[1], (char *) outbuf - (char *) pWideCharStr - sizeof(WCHAR));
      outbuf -= sizeof(WCHAR);
      nRet--;
   }
   if (outbytes >= sizeof(WCHAR))
   {
      *((WCHAR *)outbuf) = 0;
   }

   return nRet;
}

#endif   /* __DISABLE_ICONV */

/**
 * Convert single-byte to UNICODE string
 */
int LIBNETXMS_EXPORTABLE MultiByteToWideChar(int iCodePage, DWORD dwFlags, const char *pByteStr, int cchByteChar, WCHAR *pWideCharStr, int cchWideChar)
{
   if (cchWideChar == 0)
   {
      return strlen(pByteStr) + 1;
   }

#if HAVE_ICONV && !defined(__DISABLE_ICONV)
   return MultiByteToWideCharIconv(iCodePage, dwFlags, pByteStr, cchByteChar, pWideCharStr, cchWideChar);
#else
   return MultiByteToWideCharSimpleCopy(iCodePage, dwFlags, pByteStr, cchByteChar, pWideCharStr, cchWideChar);
#endif
}

#endif   /* _WIN32 */

/**
 * Convert multibyte string to wide string using current LC_CTYPE setting and
 * allocating wide string dynamically
 */
WCHAR LIBNETXMS_EXPORTABLE *WideStringFromMBStringSysLocale(const char *pszString)
{
#ifdef _WIN32
   return WideStringFromMBString(pszString);
#else
   if (pszString == NULL)
      return NULL;
   int nLen = (int)strlen(pszString) + 1;
   WCHAR *pwszOut = (WCHAR *) malloc(nLen * sizeof(WCHAR));
#if HAVE_MBSTOWCS
   mbstowcs(pwszOut, pszString, nLen);
#else
   MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pszString, -1, pwszOut, nLen);
#endif
   return pwszOut;
#endif
}

/**
 * Convert multibyte string to wide string using current codepage and
 * allocating wide string dynamically
 */
WCHAR LIBNETXMS_EXPORTABLE *WideStringFromMBString(const char *src)
{
   if (src == NULL)
      return NULL;
   int len = (int)strlen(src) + 1;
   WCHAR *out = (WCHAR *)malloc(len * sizeof(WCHAR));
   MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, src, -1, out, len);
   return out;
}

/**
 * Convert wide string to UTF8 string allocating wide string dynamically
 */
WCHAR LIBNETXMS_EXPORTABLE *WideStringFromUTF8String(const char *pszString)
{
   if (pszString == NULL)
      return NULL;
   int nLen = (int)strlen(pszString) + 1;
   WCHAR *pwszOut = (WCHAR *) malloc(nLen * sizeof(WCHAR));
   MultiByteToWideChar(CP_UTF8, 0, pszString, -1, pwszOut, nLen);
   return pwszOut;
}

/**
 * Convert wide string to multibyte string using current codepage and
 * allocating multibyte string dynamically
 */
char LIBNETXMS_EXPORTABLE *MBStringFromWideString(const WCHAR *pwszString)
{
   if (pwszString == NULL)
      return NULL;
   int nLen = (int)wcslen(pwszString) + 1;
   char *pszOut = (char *)malloc(nLen);
   WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, pwszString, -1, pszOut, nLen, NULL, NULL);
   return pszOut;
}

/**
 * Convert wide string to multibyte string using current LC_CTYPE setting and
 * allocating multibyte string dynamically
 */
char LIBNETXMS_EXPORTABLE *MBStringFromWideStringSysLocale(const WCHAR *pwszString)
{
#ifdef _WIN32
   return MBStringFromWideString(pwszString);
#else
   if (pwszString == NULL)
      return NULL;
   size_t len = wcslen(pwszString) * 3 + 1;  // add extra bytes in case of UTF-8 as target encoding
   char *out = (char *)malloc(len);
#if HAVE_WCSTOMBS
   wcstombs(out, pwszString, len);
#else
   WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, pwszString, -1, out, (int)len, NULL, NULL);
#endif
   return out;
#endif
}

/**
 * Convert wide string to UTF8 string allocating UTF8 string dynamically
 */
char LIBNETXMS_EXPORTABLE *UTF8StringFromWideString(const WCHAR *src)
{
   int len = WideCharToMultiByte(CP_UTF8, 0, src, -1, NULL, 0, NULL, NULL);
   char *out = (char *)malloc(len);
   WideCharToMultiByte(CP_UTF8, 0, src, -1, out, len, NULL, NULL);
   return out;
}

/**
 * Convert UTF8 string to multibyte string using current codepage and
 * allocating multibyte string dynamically
 */
char LIBNETXMS_EXPORTABLE *MBStringFromUTF8String(const char *s)
{
   int len = (int)strlen(s) + 1;
   char *out = (char *)malloc(len);
   utf8_to_mb(s, -1, out, len);
   return out;
}

/**
 * Convert multibyte string to UTF8 string allocating UTF8 string dynamically
 */
char LIBNETXMS_EXPORTABLE *UTF8StringFromMBString(const char *s)
{
   int len = (int)strlen(s) * 3 + 1;   // assume worst case - 3 bytes per character
   char *out = (char *)malloc(len);
   mb_to_utf8(s, -1, out, len);
   return out;
}

/**
 * Convert UCS-4 string to UCS-2 string allocating UCS-2 string dynamically
 */
UCS2CHAR LIBNETXMS_EXPORTABLE *UCS2StringFromUCS4String(const UCS4CHAR *src)
{
   UCS2CHAR *pszOut;
   int nLen;

   nLen = (int)ucs4_strlen(src) + 1;
   pszOut = (UCS2CHAR *)malloc(nLen * sizeof(UCS2CHAR));
   ucs4_to_ucs2(src, -1, pszOut, nLen);
   return pszOut;
}

/**
 * Convert UCS-2 string to UCS-4 string allocating UCS-4 string dynamically
 */
UCS4CHAR LIBNETXMS_EXPORTABLE *UCS4StringFromUCS2String(const UCS2CHAR *src)
{
   int len = (int)ucs2_strlen(src) + 1;
   UCS4CHAR *out = (UCS4CHAR *)malloc(len * sizeof(UCS4CHAR));
   ucs2_to_ucs4(src, -1, out, len);
   return out;
}

#ifndef UNICODE_UCS2

/**
 * Convert UTF-8 to UCS-2 (dynamically allocated output string)
 */
UCS2CHAR LIBNETXMS_EXPORTABLE *UCS2StringFromUTF8String(const char *utf8String)
{
   if (utf8String == NULL)
      return NULL;
   int nLen = (int)strlen(utf8String) + 1;
   UCS2CHAR *out = (UCS2CHAR *)malloc(nLen * sizeof(UCS2CHAR));
   utf8_to_ucs2(utf8String, -1, out, nLen);
   return out;
}

/**
 * Convert UCS-2 string to UTF-8 string allocating UTF-8 string dynamically
 */
char LIBNETXMS_EXPORTABLE *UTF8StringFromUCS2String(const UCS2CHAR *src)
{
   int len = (int)ucs2_strlen(src) + 1;
   char *out = (char *)malloc(len * 2);
   ucs2_to_utf8(src, -1, out, len);
   return out;
}

/**
 * Convert multibyte string to UCS-2 string allocating UCS-2 string dynamically
 */
UCS2CHAR LIBNETXMS_EXPORTABLE *UCS2StringFromMBString(const char *src)
{
   int len = (int)strlen(src) + 1;
   UCS2CHAR *out = (UCS2CHAR *)malloc(len * sizeof(UCS2CHAR));
   mb_to_ucs2(src, -1, out, len);
   return out;
}

/**
 * Convert UCS-2 string to multibyte string allocating multibyte string dynamically
 */
char LIBNETXMS_EXPORTABLE *MBStringFromUCS2String(const UCS2CHAR *src)
{
   int len = (int)ucs2_strlen(src) + 1;
   char *out = (char *)malloc(len);
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
   int len = (int)strlen(src) + 1;
   UCS4CHAR *out = (UCS4CHAR *)malloc(len * sizeof(UCS4CHAR));
   mb_to_ucs4(src, -1, out, len);
   return out;
}

/**
 * Convert UCS-4 string to multibyte string allocating multibyte string dynamically
 */
char LIBNETXMS_EXPORTABLE *MBStringFromUCS4String(const UCS4CHAR *src)
{
   int len = (int)ucs4_strlen(src) + 1;
   char *out = (char *)malloc(len);
   ucs4_to_mb(src, -1, out, len);
   return out;
}

#endif /* UNICODE_UCS4 */

/**
 * UNIX UNICODE specific wrapper functions
 */
#if !defined(_WIN32)

#if !HAVE_WSTAT

int wstat(const WCHAR *_path, struct stat *_sbuf)
{
   char path[MAX_PATH];

   WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR,
      _path, -1, path, MAX_PATH, NULL, NULL);
   return stat(path, _sbuf);
}

#endif /* !HAVE_WSTAT */

#if defined(UNICODE)

/**
 * Wide character version of some libc functions
 */

#define DEFINE_PATH_FUNC(func) \
int w##func(const WCHAR *_path) \
{ \
	char path[MAX_PATH]; \
	WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, _path, -1, path, MAX_PATH, NULL, NULL); \
	return func(path); \
}

#if !HAVE_WCHDIR
DEFINE_PATH_FUNC(chdir)
#endif

#if !HAVE_WRMDIR
DEFINE_PATH_FUNC(rmdir)
#endif

#if !HAVE_WUNLINK
DEFINE_PATH_FUNC(unlink)
#endif

#if !HAVE_WREMOVE
DEFINE_PATH_FUNC(remove)
#endif

#if !HAVE_WMKSTEMP

int LIBNETXMS_EXPORTABLE wmkstemp(WCHAR *_path)
{
   char path[MAX_PATH];
   WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, _path, -1, path, MAX_PATH, NULL, NULL);
   int rc = mkstemp(path);
   if (rc != -1)
   {
      MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, path, -1, _path, wcslen(_path) + 1);
   }
   return rc;
}

#endif

#if !HAVE_WPOPEN

FILE LIBNETXMS_EXPORTABLE *wpopen(const WCHAR *_command, const WCHAR *_type)
{
   char *command, *type;
   FILE *f;

   command = MBStringFromWideString(_command);
   type = MBStringFromWideString(_type);
   f = popen(command, type);
   free(command);
   free(type);
   return f;
}

#endif

#if !HAVE_WFOPEN

FILE LIBNETXMS_EXPORTABLE *wfopen(const WCHAR *_name, const WCHAR *_type)
{
   char *name, *type;
   FILE *f;

   name = MBStringFromWideString(_name);
   type = MBStringFromWideString(_type);
   f = fopen(name, type);
   free(name);
   free(type);
   return f;
}

#endif

#if HAVE_FOPEN64 && !HAVE_WFOPEN64

FILE LIBNETXMS_EXPORTABLE *wfopen64(const WCHAR *_name, const WCHAR *_type)
{
   char *name, *type;
   FILE *f;

   name = MBStringFromWideString(_name);
   type = MBStringFromWideString(_type);
   f = fopen64(name, type);
   free(name);
   free(type);
   return f;
}

#endif

#if !HAVE_WOPEN

int LIBNETXMS_EXPORTABLE wopen(const WCHAR *_name, int flags, ...)
{
   char *name;
   int rc;

   name = MBStringFromWideString(_name);
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
   free(name);
   return rc;
}

#endif

#if !HAVE_WCHMOD

int LIBNETXMS_EXPORTABLE wchmod(const WCHAR *_name, int mode)
{
   char *name;
   int rc;

   name = MBStringFromWideString(_name);
   rc = chmod(name, mode);
   free(name);
   return rc;
}

#endif

#if !HAVE_WRENAME

int wrename(const WCHAR *_oldpath, const WCHAR *_newpath)
{
   char oldpath[MAX_PATH], newpath[MAX_PATH];

   WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR,
      _oldpath, -1, oldpath, MAX_PATH, NULL, NULL);
   WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR,
      _newpath, -1, newpath, MAX_PATH, NULL, NULL);
   return rename(oldpath, newpath);
}

#endif

#if !HAVE_WSYSTEM

int wsystem(const WCHAR *_cmd)
{
   char *cmd = MBStringFromWideStringSysLocale(_cmd);
   int rc = system(cmd);
   free(cmd);
   return rc;
}

#endif

#if !HAVE_WACCESS

int waccess(const WCHAR *_path, int mode)
{
   char path[MAX_PATH];

   WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR,
      _path, -1, path, MAX_PATH, NULL, NULL);
   return access(path, mode);
}

#endif

#if !HAVE_WMKDIR

int wmkdir(const WCHAR *_path, int mode)
{
   char path[MAX_PATH];

   WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR,
      _path, -1, path, MAX_PATH, NULL, NULL);
   return mkdir(path, mode);
}

#endif

#if !HAVE_WUTIME

int wutime(const WCHAR *_path, utimbuf *buf)
{
   char path[MAX_PATH];

   WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR,
      _path, -1, path, MAX_PATH, NULL, NULL);
   return utime(path, buf);
}

#endif

#if !HAVE_WGETENV

WCHAR *wgetenv(const WCHAR *_string)
{
   char name[256], *p;
   static WCHAR value[8192];

   WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, _string, -1, name, 256, NULL, NULL);
   p = getenv(name);
   if (p == NULL)
      return NULL;

   MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, p, -1, value, 8192);
   return value;
}

#endif

#if !HAVE_WCTIME

WCHAR *wctime(const time_t *timep)
{
   static WCHAR value[256];

   MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, ctime(timep), -1, value, 256);
   return value;
}

#endif /* HAVE_WCTIME */

#if !HAVE_PUTWS

int putws(const WCHAR *s)
{
#if HAVE_FPUTWS
   fputws(s, stdout);
   putwc(L'\n', stdout);
#else
   printf("%S\n", s);
#endif /* HAVE_FPUTWS */
   return 1;
}

#endif /* !HAVE_PUTWS */

#if !HAVE_WCSERROR && (HAVE_STRERROR || HAVE_DECL_STRERROR)

WCHAR *wcserror(int errnum)
{
   static WCHAR value[8192];

   MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, strerror(errnum), -1, value, 8192);
   return value;
}

#endif /* !HAVE_WCSERROR && HAVE_STRERROR */

#if !HAVE_WCSERROR_R && HAVE_STRERROR_R

#if HAVE_POSIX_STRERROR_R
int wcserror_r(int errnum, WCHAR *strerrbuf, size_t buflen)
#else
WCHAR *wcserror_r(int errnum, WCHAR *strerrbuf, size_t buflen)
#endif /* HAVE_POSIX_STRERROR_R */
{
   char *mbbuf;
#if HAVE_POSIX_STRERROR_R
   int err = 0;
#endif /* HAVE_POSIX_STRERROR_R */

   mbbuf = (char *)malloc(buflen);
   if (mbbuf != NULL)
   {
#if HAVE_POSIX_STRERROR_R
      err = strerror_r(errnum, mbbuf, buflen);
      MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, mbbuf, -1, strerrbuf, buflen);
#else
      MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, strerror_r(errnum, mbbuf, buflen), -1, strerrbuf, buflen);
#endif
      free(mbbuf);
   }
   else
   {
      *strerrbuf = 0;
   }

#if HAVE_POSIX_STRERROR_R
   return err;
#else
   return strerrbuf;
#endif
}

#endif /* !HAVE_WCSERROR_R && HAVE_STRERROR_R */

/**
 * Wrappers for wprintf/wscanf family
 *
 * All these wrappers replaces %s with %S and %c with %C
 * because in POSIX version of wprintf functions %s and %c
 * means "multibyte string/char" instead of expected "UNICODE string/char"
 */

int LIBNETXMS_EXPORTABLE nx_wprintf(const WCHAR *format, ...)
{
   va_list args;
   int rc;

   va_start(args, format);
   rc = nx_vwprintf(format, args);
   va_end(args);
   return rc;
}

int LIBNETXMS_EXPORTABLE nx_fwprintf(FILE *fp, const WCHAR *format, ...)
{
   va_list args;
   int rc;

   va_start(args, format);
   rc = nx_vfwprintf(fp, format, args);
   va_end(args);
   return rc;
}

int LIBNETXMS_EXPORTABLE nx_swprintf(WCHAR *buffer, size_t size, const WCHAR *format, ...)
{
   va_list args;
   int rc;

   va_start(args, format);
   rc = nx_vswprintf(buffer, size, format, args);
   va_end(args);
   return rc;
}

int LIBNETXMS_EXPORTABLE nx_wscanf(const WCHAR *format, ...)
{
   va_list args;
   int rc;

   va_start(args, format);
   rc = nx_vwscanf(format, args);
   va_end(args);
   return rc;
}

int LIBNETXMS_EXPORTABLE nx_fwscanf(FILE *fp, const WCHAR *format, ...)
{
   va_list args;
   int rc;

   va_start(args, format);
   rc = nx_vfwscanf(fp, format, args);
   va_end(args);
   return rc;
}

int LIBNETXMS_EXPORTABLE nx_swscanf(const WCHAR *str, const WCHAR *format, ...)
{
   va_list args;
   int rc;

   va_start(args, format);
   rc = nx_vswscanf(str, format, args);
   va_end(args);
   return rc;
}

static WCHAR *ReplaceFormatSpecs(const WCHAR *oldFormat)
{
   WCHAR *fmt, *p;
   int state = 0;
   bool hmod;

   fmt = wcsdup(oldFormat);
   for(p = fmt; *p != 0; p++)
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

int LIBNETXMS_EXPORTABLE nx_vwprintf(const WCHAR *format, va_list args)
{
   WCHAR *fmt;
   int rc;

   fmt = ReplaceFormatSpecs(format);
   rc = vwprintf(fmt, args);
   free(fmt);
   return rc;
}

int LIBNETXMS_EXPORTABLE nx_vfwprintf(FILE *fp, const WCHAR *format, va_list args)
{
   WCHAR *fmt;
   int rc;

   fmt = ReplaceFormatSpecs(format);
   rc = vfwprintf(fp, fmt, args);
   free(fmt);
   return rc;
}

int LIBNETXMS_EXPORTABLE nx_vswprintf(WCHAR *buffer, size_t size, const WCHAR *format, va_list args)
{
   WCHAR *fmt;
   int rc;

   fmt = ReplaceFormatSpecs(format);
   rc = vswprintf(buffer, size, fmt, args);
   free(fmt);
   return rc;
}

int LIBNETXMS_EXPORTABLE nx_vwscanf(const WCHAR *format, va_list args)
{
   WCHAR *fmt;
   int rc;

   fmt = ReplaceFormatSpecs(format);
#if HAVE_VWSCANF
   rc = vwscanf(fmt, args);
#else
   rc = -1; // FIXME: add workaround implementation
#endif
   free(fmt);
   return rc;
}

int LIBNETXMS_EXPORTABLE nx_vfwscanf(FILE *fp, const WCHAR *format, va_list args)
{
   WCHAR *fmt;
   int rc;

   fmt = ReplaceFormatSpecs(format);
#if HAVE_VFWSCANF
   rc = vfwscanf(fp, fmt, args);
#else
   rc = -1; // FIXME: add workaround implementation
#endif
   free(fmt);
   return rc;
}

int LIBNETXMS_EXPORTABLE nx_vswscanf(const WCHAR *str, const WCHAR *format, va_list args)
{
   WCHAR *fmt;
   int rc;

   fmt = ReplaceFormatSpecs(format);
#if HAVE_VSWSCANF
   rc = vswscanf(str, fmt, args);
#else
   rc = -1; // FIXME: add workaround implementation
#endif
   free(fmt);
   return rc;
}

#endif /* defined(UNICODE) */

#endif /* !defined(_WIN32) */

/**
 * UNICODE version of inet_addr()
 */
UINT32 LIBNETXMS_EXPORTABLE inet_addr_w(const WCHAR *pszAddr)
{
   char szBuffer[256];
   WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, pszAddr, -1, szBuffer, 256, NULL, NULL);
   return inet_addr(szBuffer);
}

#if defined(_WITH_ENCRYPTION) && WITH_OPENSSL

/**
 * Get OpenSSL error string as UNICODE string
 * Buffer must be at least 256 character long
 */
WCHAR LIBNETXMS_EXPORTABLE *ERR_error_string_W(int nError, WCHAR *pwszBuffer)
{
   char text[256];

   memset(text, 0, sizeof(text));
   ERR_error_string(nError, text);
   MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, text, -1, pwszBuffer, 256);
   return pwszBuffer;
}

#endif
