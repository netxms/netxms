/* $Id$ */
/* 
** NetXMS - Network Management System
** Log Parsing Library
** Copyright (C) 2008 Victor Kirhenshtein
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
** File: nxlpapi.h
**
**/

#ifndef _nxlpapi_h_
#define _nxlpapi_h_

#ifdef _WIN32
#ifdef LIBNXLP_EXPORTS
#define LIBNXLP_EXPORTABLE __declspec(dllexport)
#else
#define LIBNXLP_EXPORTABLE __declspec(dllimport)
#endif
#else    /* _WIN32 */
#define LIBNXLP_EXPORTABLE
#endif


//
// Log parser class
//

class LIBNXLP_EXPORTABLE LogParser
{
public:
	LogParser();
	LogParser(const char *xml);
	~LogParser();
	
	BOOL AddRule(const char *regexp);
};

#endif

