/* 
** libnetxms - Common NetXMS utility library
** Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 Victor Kirhenshtein
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
** File: inline.cpp
**
**/

#define LIBNETXMS_INLINE
#include "libnetxms.h"
#include <nms_agent.h>


//
// Functions defined as inline for C++ programs
//

extern "C" void LIBNETXMS_EXPORTABLE ret_string(TCHAR *rbuf, const TCHAR *value)
{
   _tcsncpy(rbuf, value, MAX_RESULT_LENGTH - 1);
   rbuf[MAX_RESULT_LENGTH - 1] = 0;
}

extern "C" void LIBNETXMS_EXPORTABLE ret_int(TCHAR *rbuf, LONG value)
{
   _sntprintf(rbuf, MAX_RESULT_LENGTH, _T("%d"), value);
}

extern "C" void LIBNETXMS_EXPORTABLE ret_uint(TCHAR *rbuf, DWORD value)
{
   _sntprintf(rbuf, MAX_RESULT_LENGTH, _T("%u"), value);
}

extern "C" void LIBNETXMS_EXPORTABLE ret_double(TCHAR *rbuf, double value)
{
   _sntprintf(rbuf, MAX_RESULT_LENGTH, _T("%f"), value);
}

extern "C" void LIBNETXMS_EXPORTABLE ret_int64(TCHAR *rbuf, INT64 value)
{
#ifdef _WIN32
   _sntprintf(rbuf, MAX_RESULT_LENGTH, _T("%I64d"), value);
#else    /* _WIN32 */
   _sntprintf(rbuf, MAX_RESULT_LENGTH, _T("%lld"), value);
#endif   /* _WIN32 */
}

extern "C" void LIBNETXMS_EXPORTABLE ret_uint64(TCHAR *rbuf, QWORD value)
{
#ifdef _WIN32
   _sntprintf(rbuf, MAX_RESULT_LENGTH, _T("%I64u"), value);
#else    /* _WIN32 */
   _sntprintf(rbuf, MAX_RESULT_LENGTH, _T("%llu"), value);
#endif   /* _WIN32 */
}

extern "C" TCHAR LIBNETXMS_EXPORTABLE *nx_strncpy(TCHAR *pszDest, const TCHAR *pszSrc, size_t nLen)
{
   _tcsncpy(pszDest, pszSrc, nLen - 1);
   pszDest[nLen - 1] = 0;
   return pszDest;
}
