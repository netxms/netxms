/* $Id: unicode.cpp,v 1.9 2006-10-01 15:26:29 victor Exp $ */
/*
** NetXMS - Network Management System
** Copyright (C) 2003, 2004, 2005, 2006 Victor Kirhenshtein
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


#ifdef _WIN32

#define __nx__wcslen wcslen

#else


//
// Calculate length of wide character string
//

#if HAVE_USEABLE_WCHAR

#define __nx__wcslen wcslen

#else

int LIBNETXMS_EXPORTABLE __nx__wcslen(WCHAR *pStr)
{
   int iLen = 0;
   WCHAR *pCurr = pStr;

   while(*pCurr++)
      iLen++;
   return iLen;
}

#endif


//
// Convert UNICODE string to single-byte string
//

int LIBNETXMS_EXPORTABLE WideCharToMultiByte(int iCodePage, DWORD dwFlags, WCHAR *pWideCharStr, 
                                             int cchWideChar, char *pByteStr, int cchByteChar, 
                                             char *pDefaultChar, BOOL *pbUsedDefChar)
{
   WCHAR *pSrc;
   char *pDest;
   int iPos, iSize;

   if (cchByteChar == 0)
   {
      return __nx__wcslen(pWideCharStr) + 1;
   }

   iSize = (cchWideChar == -1) ? __nx__wcslen(pWideCharStr) : cchWideChar;
   if (iSize >= cchByteChar)
      iSize = cchByteChar - 1;
   for(pSrc = pWideCharStr, iPos = 0, pDest = pByteStr; iPos < iSize; iPos++, pSrc++, pDest++)
      *pDest = (*pSrc < 256) ? (char)(*pSrc) : '?';
   *pDest = 0;
   return iSize;
}


//
// Convert single-byte to UNICODE string
//

int LIBNETXMS_EXPORTABLE MultiByteToWideChar(int iCodePage, DWORD dwFlags, char *pByteStr, 
                                             int cchByteChar, WCHAR *pWideCharStr, 
                                             int cchWideChar)
{
   char *pSrc;
   WCHAR *pDest;
   int iPos, iSize;

   iSize = (cchByteChar == -1) ? strlen(pByteStr) : cchByteChar;
   if (iSize >= cchWideChar)
      iSize = cchWideChar - 1;
   for(pSrc = pByteStr, iPos = 0, pDest = pWideCharStr; iPos < iSize; iPos++, pSrc++, pDest++)
      *pDest = (WCHAR)(*pSrc);
   *pDest = 0;
   return iSize;
}

#endif   /* not _WIN32 */


//
// UNICODE version of inet_addr()
//

DWORD LIBNETXMS_EXPORTABLE inet_addr_w(WCHAR *pszAddr)
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

WCHAR LIBNETXMS_EXPORTABLE *WideStringFromMBString(char *pszString)
{
   WCHAR *pwszOut;
   int nLen;

   nLen = strlen(pszString) + 1;
   pwszOut = (WCHAR *)malloc(nLen * sizeof(WCHAR));
   MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pszString, -1, pwszOut, nLen);
   return pwszOut;
}


//
// Convert wide string to multibyte string using current codepage and
// allocating multibyte string dynamically
//

char LIBNETXMS_EXPORTABLE *MBStringFromWideString(WCHAR *pwszString)
{
   char *pszOut;
   int nLen;

   nLen = __nx__wcslen(pwszString) + 1;
   pszOut = (char *)malloc(nLen);
   WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR,
                       pwszString, -1, pszOut, nLen, NULL, NULL);
   return pszOut;
}


//
// Convert wide string to UTF8 string allocating UTF8 string dynamically
//

char LIBNETXMS_EXPORTABLE *UTF8StringFromWideString(WCHAR *pwszString)
{
   char *pszOut;
   int nLen;

   nLen = WideCharToMultiByte(CP_UTF8, 0, pwszString, -1, NULL, 0, NULL, NULL);
   pszOut = (char *)malloc(nLen);
   WideCharToMultiByte(CP_UTF8, 0, pwszString, -1, pszOut, nLen, NULL, NULL);
   return pszOut;
}
