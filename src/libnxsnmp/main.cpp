/* 
** NetXMS - Network Management System
** SNMP support library
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
** $module: main.cpp
**
**/

#include "libnxsnmp.h"


//
// Convert OID to text
//

void LIBNXSNMP_EXPORTABLE SNMPConvertOIDToText(DWORD dwLength, DWORD *pdwValue, TCHAR *pszBuffer, DWORD dwBufferSize)
{
   DWORD i, dwBufPos, dwNumChars;

   pszBuffer[0] = 0;
   for(i = 0, dwBufPos = 0; (i < dwLength) && (dwBufPos < dwBufferSize); i++)
   {
      dwNumChars = _sntprintf(&pszBuffer[dwBufPos], dwBufferSize - dwBufPos, _T(".%d"), pdwValue[i]);
      dwBufPos += dwNumChars;
   }
}


//
// DLL entry point
//

#ifdef _WIN32

#ifndef UNDER_CE // FIXME
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   return TRUE;
}
#endif // UNDER_CE

#endif   /* _WIN32 */
