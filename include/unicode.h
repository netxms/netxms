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

#ifdef UNICODE

#define _tcstoll  wcstoll
#define _tcstoull wcstoull

#else

#define _tcstoll  strtoll
#define _tcstoull strtoull

#endif

#else    /* not _WIN32 */

#if HAVE_WCHAR_H
#include <wchar.h>
#endif

#if HAVE_STRING_H
#include <string.h>
#endif

#ifdef _NETWARE
#define WCHAR     wchar_t
#else
#define WCHAR     unsigned short
#endif

// Redefine wide character functions if system's wchar_t is not 2 bytes long
#if !HAVE_USEABLE_WCHAR

#define wcslen		nx_wcslen

#endif

#ifdef UNICODE

#error UNICODE is not supported on non-Windows platforms

#else

// On some old systems, ctype.h defines _T macro, so we include it
// before our definition and do an undef
#include <ctype.h>

#undef _T
#define _T(x)     x
#define TCHAR     char
#define _TINT     int

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
#define _ftprintf fprintf
#define _sntprintf snprintf
#define _vtprintf vprintf
#define _vstprintf vsprintf
#define _vsntprintf vsnprintf
#define _tfopen   fopen
#define _fgetts   fgets
#define _fputts   fputs
#define _tcstol   strtol
#define _tcstoul  strtoul
#define _tcstoll  strtoll
#define _tcstoull strtoull
#define _tcstod   strtod
#define _tcsdup   strdup
#define _tcsupr   strupr
#define _tcsspn   strspn
#define _tcsstr   strstr
#define _tcscat   strcat
#define _topen    open
#define _taccess  access
#define _tunlink  unlink
#define _tcsftime strftime
#define _tctime   ctime
#define _istspace isspace
#define _istdigit isdigit
#define _istalpha isalpha
#define _istupper isupper

#endif

#define CP_ACP             0
#define CP_UTF8				65001
#define MB_PRECOMPOSED     0x00000001
#define WC_COMPOSITECHECK  0x00000002
#define WC_DEFAULTCHAR     0x00000004

#endif	/* _WIN32 */


#ifdef UNICODE
#define _t_inet_addr    inet_addr_w
#else
#define _t_inet_addr    inet_addr
#endif


#endif   /* _unicode_h_ */
