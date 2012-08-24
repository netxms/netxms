/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2010 Victor Kirhenshtein
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

#ifdef USE_BUNDLED_LIBTRE
#include "../src/libtre/tre.h"

#ifdef UNICODE
#define _tregcomp  tre_regwcomp
#define _tregexec  tre_regwexec
#define _tregncomp tre_regwncomp
#define _tregnexec tre_regwnexec
#else
#define _tregcomp  tre_regcomp
#define _tregexec  tre_regexec
#define _tregncomp tre_regncomp
#define _tregnexec tre_regnexec
#endif

#define regfree tre_regfree

#else
#include <tre/regex.h>

#ifdef UNICODE
#define _tregcomp  tre_regwcomp
#define _tregexec  tre_regwexec
#define _tregncomp tre_regwncomp
#define _tregnexec tre_regwnexec
#else
#define _tregcomp  tre_regcomp
#define _tregexec  tre_regexec
#define _tregncomp tre_regncomp
#define _tregnexec tre_regnexec
#endif

#endif	/* USE_BUNDLED_LIBTRE */

#endif	/* _netxms_regex_h */
