/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004 Victor Kirhenshtein
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
** $module: unicode.cpp
**
**/

#include "libnetxms.h"


#ifndef _WIN32


//
// Calculate length of wide character string
//

int LIBNETXMS_EXPORTABLE wcslen(WCHAR *pStr)
{
   int iLen = 0;
   WCHAR *pCurr = pStr;

   while(*pCurr++)
      iLen++;
   return iLen;
}


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

   iSize = (cchWideChar == -1) ? wcslen(pWideCharStr) : cchWideChar;
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

#endif
