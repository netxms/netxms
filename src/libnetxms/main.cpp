/* 
** NetXMS - Network Management System
** Utility Library
** Copyright (C) 2003-2012 Victor Kirhenshtein
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
** File: main.cpp
**
**/

#include "libnetxms.h"

/**
 * Swap byte order in 64-bit integer
 */
#if defined(_WIN32) || !(HAVE_DECL___BSWAP_64)

QWORD LIBNETXMS_EXPORTABLE __bswap_64(QWORD qwVal)
{
   QWORD qwResult;
   BYTE *sptr = (BYTE *)&qwVal;
   BYTE *dptr = (BYTE *)&qwResult + 7;
   int i;

   for(i = 0; i < 8; i++, sptr++, dptr--)
      *dptr = *sptr;

   return qwResult;
}

#endif

/**
 * Swap bytes in double
 */
double LIBNETXMS_EXPORTABLE __bswap_double(double dVal)
{
   double dResult;
   BYTE *sptr = (BYTE *)&dVal;
   BYTE *dptr = (BYTE *)&dResult + 7;
   int i;

   for(i = 0; i < 8; i++, sptr++, dptr--)
      *dptr = *sptr;

   return dResult;
}

/**
 * Swap bytes in wide string (UCS-2)
 */
void LIBNETXMS_EXPORTABLE __bswap_wstr(UCS2CHAR *pStr)
{
   UCS2CHAR *pch;

   for(pch = pStr; *pch != 0; pch++)
      *pch = htons(*pch);
}

#if !defined(_WIN32) && !defined(_NETWARE)

/**
 * strupr() implementation for non-windows platforms
 */
void LIBNETXMS_EXPORTABLE __strupr(char *in)
{
	char *p = in;

	if (in == NULL) 
   { 
		return;
	}
	
	for (; *p != 0; p++) 
   {
		// TODO: check/set locale
		*p = toupper(*p);
	}
}

#if defined(UNICODE_UCS2) || defined(UNICODE_UCS4)

/**
 * wcsupr() implementation for non-windows platforms
 */
void LIBNETXMS_EXPORTABLE __wcsupr(WCHAR *in)
{
	WCHAR *p = in;

	if (in == NULL) 
   { 
		return;
	}
	
	for (; *p != 0; p++) 
   {
		// TODO: check/set locale
#if HAVE_TOWUPPER
		*p = towupper(*p);
#else
		if (*p < 256)
		{
			*p = (WCHAR)toupper(*p);
		}
#endif
	}
}

#endif

#endif


//
// DLL entry point
//

#ifdef _WIN32

#ifndef UNDER_CE // FIXME
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
	{
      DisableThreadLibraryCalls(hInstance);
		SEHInit();
	}
   return TRUE;
}
#endif // UNDER_CE

#endif   /* _WIN32 */


//
// NetWare library entry point
//

#ifdef _NETWARE

int _init(void)
{
   return 0;
}

int _fini(void)
{
   return 0;
}

#endif
