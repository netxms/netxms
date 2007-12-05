/* $Id: unicode.cpp,v 1.26 2007-12-05 15:29:58 victor Exp $ */
/*
** NetXMS - Network Management System
** Copyright (C) 2003, 2004, 2005, 2006, 2007 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: unicode.cpp
**
**/

#include "libnetxms.h"


//
// Static data
//

static char m_cpDefault[MAX_CODEPAGE_LEN] = ICONV_DEFAULT_CODEPAGE;


#ifndef _WIN32

#if HAVE_ICONV_H
#include <iconv.h>
#endif


//
// UNICODE character set
//

#ifndef __DISABLE_ICONV

// configure first test for libiconv, then for iconv
// if libiconv was found, HAVE_ICONV will not be set correctly
#if HAVE_LIBICONV
#undef HAVE_ICONV
#define HAVE_ICONV 1
#endif

#if HAVE_ICONV_UCS_2_INTERNAL
#define UCS2_CODEPAGE_NAME	"UCS-2-INTERNAL"
#elif HAVE_ICONV_UCS_2
#define UCS2_CODEPAGE_NAME	"UCS-2"
#elif HAVE_ICONV_UCS2
#define UCS2_CODEPAGE_NAME	"UCS2"
#elif HAVE_ICONV_UCS_2BE && WORDS_BIGENDIAN
#define UCS2_CODEPAGE_NAME	"UCS-2BE"
#else
#warning Cannot determine valid UCS-2 codepage name
#undef HAVE_ICONV
#endif

#endif   /* __DISABLE_ICONV */


//
// Set application's default codepage
//

BOOL LIBNETXMS_EXPORTABLE SetDefaultCodepage(const char *cp)
{
	BOOL rc;
	iconv_t cd;

#if HAVE_ICONV && !defined(__DISABLE_ICONV)
	cd = iconv_open(cp, "UTF-8");
	if (cd != (iconv_t)(-1))
	{
		iconv_close(cd);
#endif		
		strncpy(m_cpDefault, cp, MAX_CODEPAGE_LEN);
		m_cpDefault[MAX_CODEPAGE_LEN - 1] = 0;
		rc = TRUE;
#if HAVE_ICONV && !defined(__DISABLE_ICONV)
	}
	else
	{
		rc = FALSE;
	}
#endif
	return rc;
}


//
// Calculate length of wide character string
//

#if !HAVE_USEABLE_WCHAR

int LIBNETXMS_EXPORTABLE nx_wcslen(const WCHAR *pStr)
{
   int iLen = 0;
   const WCHAR *pCurr = pStr;

   while(*pCurr++)
      iLen++;
   return iLen;
}

#endif


//
// Duplicate wide character string
//

#if !HAVE_USEABLE_WCHAR

WCHAR LIBNETXMS_EXPORTABLE *nx_wcsdup(const WCHAR *pStr)
{
	return (WCHAR *)nx_memdup(pStr, (wcslen(pStr) + 1) * sizeof(WCHAR));
}

#endif


//
// Copy wide character string with length limitation
//

#if !HAVE_USEABLE_WCHAR

WCHAR LIBNETXMS_EXPORTABLE *nx_wcsncpy(WCHAR *pDst, const WCHAR *pSrc, int nDstLen)
{
	int nLen;

	nLen = wcslen(pSrc) + 1;
	if (nLen > nDstLen)
		nLen = nDstLen;
	memcpy(pDst, pSrc, nLen * sizeof(WCHAR));
	return pDst;
}

#endif


//
// Convert UNICODE string to single-byte string
//

int LIBNETXMS_EXPORTABLE WideCharToMultiByte(int iCodePage, DWORD dwFlags,
                                             const WCHAR *pWideCharStr, int cchWideChar,
															char *pByteStr, int cchByteChar, 
                                             char *pDefaultChar, BOOL *pbUsedDefChar)
{
#if HAVE_ICONV && !defined(__DISABLE_ICONV)
	iconv_t cd;
	int nRet;
	const char *inbuf;
	char *outbuf;
	size_t inbytes, outbytes;
	char cp[MAX_CODEPAGE_LEN + 16];

	// Calculate required length. Because iconv cannot calculate
	// resulting multibyte string length, assume the worst case - 3 bytes
	// per character for UTF-8 and 2 bytes per character for other encodings
	if (cchByteChar == 0)
	{
		return wcslen(pWideCharStr) * (iCodePage == CP_UTF8 ? 3 : 2) + 1;
	}

	strcpy(cp, m_cpDefault);
#if HAVE_ICONV_IGNORE
	strcat(cp, "//IGNORE");
#endif
	cd = iconv_open(iCodePage == CP_UTF8 ? "UTF-8" : cp, UCS2_CODEPAGE_NAME);
	if (cd != (iconv_t)(-1))
	{
		inbuf = (const char *)pWideCharStr;
		inbytes = ((cchWideChar == -1) ? wcslen(pWideCharStr) + 1 : cchWideChar) * sizeof(WCHAR);
		outbuf = pByteStr;
		outbytes = cchByteChar;
		nRet = iconv(cd, (ICONV_CONST char **)&inbuf, &inbytes, &outbuf, &outbytes);
		iconv_close(cd);
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
		if ((cchWideChar == -1) && (outbytes > 0))
		{
			*outbuf = 0;
		}
	}
	else
	{
		*pByteStr = 0;
		nRet = 0;
	}
	return nRet;

#else

   const WCHAR *pSrc;
   char *pDest;
   int iPos, iSize;

   if (cchByteChar == 0)
   {
      return wcslen(pWideCharStr) + 1;
   }

   iSize = (cchWideChar == -1) ? wcslen(pWideCharStr) : cchWideChar;
   if (iSize >= cchByteChar)
      iSize = cchByteChar - 1;
   for(pSrc = pWideCharStr, iPos = 0, pDest = pByteStr; iPos < iSize; iPos++, pSrc++, pDest++)
      *pDest = (*pSrc < 256) ? (char)(*pSrc) : '?';
   *pDest = 0;
   return iSize;

#endif	/* HAVE_ICONV */
}


//
// Convert single-byte to UNICODE string
//

int LIBNETXMS_EXPORTABLE MultiByteToWideChar(int iCodePage, DWORD dwFlags, const char *pByteStr,
                                             int cchByteChar, WCHAR *pWideCharStr, int cchWideChar)
{
#if HAVE_ICONV && !defined(__DISABLE_ICONV)

	iconv_t cd;
	int nRet;
	const char *inbuf;
	char *outbuf;
	size_t inbytes, outbytes;

	if (cchWideChar == 0)
	{
		return strlen(pByteStr) + 1;
	}

	cd = iconv_open(UCS2_CODEPAGE_NAME, iCodePage == CP_UTF8 ? "UTF-8" : m_cpDefault);
	if (cd != (iconv_t)(-1))
	{
		inbuf = pByteStr;
		inbytes = (cchByteChar == -1) ? strlen(pByteStr) + 1 : cchByteChar;
		outbuf = (char *)pWideCharStr;
		outbytes = cchWideChar * sizeof(WCHAR);
		nRet = iconv(cd, (ICONV_CONST char **)&inbuf, &inbytes, &outbuf, &outbytes);
		iconv_close(cd);
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
		if (((char *)outbuf - (char *)pWideCharStr > sizeof(WCHAR)) && (*pWideCharStr == 0xFEFF))
		{
			// Remove UNICODE byte order indicator if presented
			memmove(pWideCharStr, &pWideCharStr[1], (char *)outbuf - (char *)pWideCharStr - sizeof(WCHAR));
			outbuf -= sizeof(WCHAR);
		}
		if ((cchByteChar == -1) && (outbytes >= sizeof(WCHAR)))
		{
			*((WCHAR *)outbuf) = 0;
		}
	}
	else
	{
		*pWideCharStr = 0;
		nRet = 0;
	}
	return nRet;

#else

	const char *pSrc;
	WCHAR *pDest;
	int iPos, iSize;

	if (cchWideChar == 0)
	{
		return strlen(pByteStr) + 1;
	}

	iSize = (cchByteChar == -1) ? strlen(pByteStr) : cchByteChar;
	if (iSize >= cchWideChar)
		iSize = cchWideChar - 1;
	for(pSrc = pByteStr, iPos = 0, pDest = pWideCharStr; iPos < iSize; iPos++, pSrc++, pDest++)
		*pDest = (WCHAR)(*pSrc);
	*pDest = 0;

	return iSize;
#endif
}

#endif   /* not _WIN32 */


//
// UNICODE version of inet_addr()
//

DWORD LIBNETXMS_EXPORTABLE inet_addr_w(const WCHAR *pszAddr)
{
   char szBuffer[256];

   WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR,
                       pszAddr, -1, szBuffer, 256, NULL, NULL);
   return inet_addr(szBuffer);
}


//
// Convert multibyte string to wide string using current codepage and
// allocating wide string dynamically
//

WCHAR LIBNETXMS_EXPORTABLE *WideStringFromMBString(const char *pszString)
{
   WCHAR *pwszOut;
   int nLen;

   nLen = (int)strlen(pszString) + 1;
   pwszOut = (WCHAR *)malloc(nLen * sizeof(WCHAR));
   MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pszString, -1, pwszOut, nLen);
   return pwszOut;
}


//
// Convert wide string to multibyte string using current codepage and
// allocating multibyte string dynamically
//

char LIBNETXMS_EXPORTABLE *MBStringFromWideString(const WCHAR *pwszString)
{
   char *pszOut;
   int nLen;

   nLen = (int)wcslen(pwszString) + 1;
   pszOut = (char *)malloc(nLen);
   WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR,
                       pwszString, -1, pszOut, nLen, NULL, NULL);
   return pszOut;
}


//
// Convert wide string to UTF8 string allocating UTF8 string dynamically
//

char LIBNETXMS_EXPORTABLE *UTF8StringFromWideString(const WCHAR *pwszString)
{
   char *pszOut;
   int nLen;

   nLen = WideCharToMultiByte(CP_UTF8, 0, pwszString, -1, NULL, 0, NULL, NULL);
   pszOut = (char *)malloc(nLen);
   WideCharToMultiByte(CP_UTF8, 0, pwszString, -1, pszOut, nLen, NULL, NULL);
   return pszOut;
}


//
// Get OpenSSL error string as UNICODE string
// Buffer must be at least 256 character long
//

#ifdef _WITH_ENCRYPTION

WCHAR LIBNETXMS_EXPORTABLE *ERR_error_string_W(int nError, WCHAR *pwszBuffer)
{
	char text[256];

	memset(text, 0, sizeof(text));
	ERR_error_string(nError, text);
   MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, text, -1, pwszBuffer, 256);
	return pwszBuffer;
}

#endif
