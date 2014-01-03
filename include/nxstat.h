/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2013 Victor Kirhenshtein
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
** File: nxstat.h
**
**/

#ifndef _nxstat_h_
#define _nxstat_h_

#ifndef S_ISDIR
#define S_ISDIR(m)      (((m) & S_IFMT) == S_IFDIR)
#endif

#ifndef S_ISREG
#define S_ISREG(m)      (((m) & S_IFMT) == S_IFREG)
#endif

#if defined(_WIN32)

#define NX_STAT _tstati64
#define NX_FSTAT _fstati64
#define NX_STAT_STRUCT struct _stati64

#elif HAVE_LSTAT64 && HAVE_STRUCT_STAT64

#define NX_STAT lstat64
#define NX_FSTAT fstat64
#define NX_STAT_STRUCT struct stat64

#else

#define NX_STAT lstat
#define NX_FSTAT fstat
#define NX_STAT_STRUCT struct stat

#endif

#if defined(UNICODE) && !defined(_WIN32)
inline int __call_stat(const WCHAR *f, NX_STAT_STRUCT *s)
{
	char *mbf = MBStringFromWideString(f);
	int rc = NX_STAT(mbf, s);
	free(mbf);
	return rc;
}
#define CALL_STAT(f, s) __call_stat(f, s)
#else
#define CALL_STAT(f, s) NX_STAT(f, s)
#endif

#endif
