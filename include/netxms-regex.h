/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2019 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
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
** File: netxms-regex.h
**
**/

#ifndef _netxms_regex_h
#define _netxms_regex_h

#if HAVE_PCRE_H || defined(_WIN32)
#include <pcre.h>
#elif HAVE_PCRE_PCRE_H
#include <pcre/pcre.h>
#endif

#ifdef UNICODE_UCS2
#define PCRE_WCHAR              PCRE_UCHAR16
#define PCREW                   pcre16
#define PCRE_UNICODE_FLAGS      PCRE_UTF16
#define _pcre_compile_w         pcre16_compile
#define _pcre_exec_w            pcre16_exec
#define _pcre_free_w            pcre16_free
#else
#define PCRE_WCHAR              PCRE_UCHAR32
#define PCREW                   pcre32
#define PCRE_UNICODE_FLAGS      PCRE_UTF32
#define _pcre_compile_w         pcre32_compile
#define _pcre_exec_w            pcre32_exec
#define _pcre_free_w            pcre32_free
#endif

#ifdef UNICODE
#define PCRE_TCHAR              PCRE_WCHAR
#define PCRE                    PCREW
#define PCRE_COMMON_FLAGS       PCRE_UNICODE_FLAGS
#define _pcre_compile_t         _pcre_compile_w
#define _pcre_exec_t            _pcre_exec_w
#define _pcre_free_t            _pcre_free_w
#else   /* UNICODE */
#define PCRE_TCHAR              char
#define PCRE                    pcre
#define PCRE_COMMON_FLAGS       0
#define _pcre_compile_t         pcre_compile
#define _pcre_exec_t            pcre_exec
#define _pcre_free_t            pcre_free
#endif

#define PCRE_COMMON_FLAGS_W     PCRE_UNICODE_FLAGS
#define PCRE_COMMON_FLAGS_A     0

#endif	/* _netxms_regex_h */
