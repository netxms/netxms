/* 
** libnetxms - Common NetXMS utility library
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
** $module: inline.cpp
**
**/

#define LIBNETXMS_INLINE
#include "libnetxms.h"


//
// Functions defined as inline for C++ programs
//

extern "C" void LIBNETXMS_EXPORTABLE ret_string(char *rbuf, char *value)
{
   memset(rbuf, 0, MAX_RESULT_LENGTH);
   strncpy(rbuf, value, MAX_RESULT_LENGTH - 3);
   strcat(rbuf, "\r\n");
}

extern "C" void LIBNETXMS_EXPORTABLE ret_int(char *rbuf, long value)
{
   sprintf(rbuf, "%ld\r\n", value);
}

extern "C" void LIBNETXMS_EXPORTABLE ret_uint(char *rbuf, unsigned long value)
{
   sprintf(rbuf, "%lu\r\n", value);
}

extern "C" void LIBNETXMS_EXPORTABLE ret_double(char *rbuf, double value)
{
   sprintf(rbuf, "%f\r\n", value);
}

extern "C" void LIBNETXMS_EXPORTABLE ret_int64(char *rbuf, INT64 value)
{
#ifdef _WIN32
   sprintf(rbuf, "%I64d\r\n", value);
#else    /* _WIN32 */
   sprintf(rbuf, "%lld\r\n", value);
#endif   /* _WIN32 */
}

extern "C" void LIBNETXMS_EXPORTABLE ret_uint64(char *rbuf, QWORD value)
{
#ifdef _WIN32
   sprintf(rbuf, "%I64u\r\n", value);
#else    /* _WIN32 */
   sprintf(rbuf, "%llu\r\n", value);
#endif   /* _WIN32 */
}
