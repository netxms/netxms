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
** $module: unicode.h
**
**/

#ifndef _unicode_h_
#define _unicode_h_

#ifdef _WIN32

// Ensure that both UNICODE and _UNICODE are defined
#ifdef _UNICODE
#ifndef UNICODE
#define UNICODE
#endif
#endif

#ifdef UNICODE
#ifndef _UNICODE
#define _UNICODE
#endif
#endif

#include <tchar.h>

#else    /* not _WIN32 */

#define WCHAR     unsigned short

#ifdef UNICODE

#error UNICODE is not supported on non-Windows platforms

#else

#define _T(x)     x
#define TCHAR     char

#define _tcscpy   strcpy
#define _tcsncpy  strncpy
#define _tcslen   strlen
#define _tcschr   strchr
#define _tcsrchr  strrchr
#define _tcscmp   strcmp
#define _tcsicmp  stricmp
#define _tcsncmp  strncmp
#define _tprintf  printf
#define _stprintf sprintf
#define _sntprintf snprintf
#define _vtprintf vprintf
#define _vstprintf vsprintf
#define _vsntprintf vsnprintf
#define _tfopen   fopen
#define _fgetts   fgets
#define _tcstol   strtol
#define _tcstoul  strtoul
#define _tcstod   strtod
#define _tcsdup   strdup
#define _tcsupr   strupr
#define _tcsspn   strspn
#define _topen    open

#endif

#define CP_ACP             1
#define MB_PRECOMPOSED     0x00000001
#define WC_COMPOSITECHECK  0x00000002
#define WC_DEFAULTCHAR     0x00000004

#endif


#ifdef UNICODE
#define _t_inet_addr    inet_addr_w
#else
#define _t_inet_addr    inet_addr
#endif


#endif   /* _unicode_h_ */
