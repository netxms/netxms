/* 
** NetXMS - Network Management System
** Utility Library
** Copyright (C) 2003-2017 Raden Solutions
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
**/

#include "libnetxms.h"
#include <sys/stat.h>

#ifdef UNICODE

/**
 * opendir() wrapper
 */
DIRW *wopendir(const WCHAR *name)
{
	char *utf8name = UTF8StringFromWideString(name);
	DIR *dir = opendir(utf8name);
	free(utf8name);
	if (dir == NULL)
		return NULL;
	DIRW *d = (DIRW *)malloc(sizeof(DIRW));
	d->dir = dir;
	return d;
}

/**
 * readdir() wrapper
 */
struct dirent_w *wreaddir(DIRW *dirp)
{
	struct dirent *d = readdir(dirp->dir);
	if (d == NULL)
		return NULL;
	MultiByteToWideChar(CP_UTF8, 0, d->d_name, -1, dirp->dirstr.d_name, 257);
	dirp->dirstr.d_name[256] = 0;
	dirp->dirstr.d_ino = d->d_ino;
#if HAVE_DIRENT_D_TYPE
	dirp->dirstr.d_type = d->d_type;
#endif
	return &dirp->dirstr;
}

/*
 * closedir() wrapper
 */
int wclosedir(DIRW *dirp)
{
	closedir(dirp->dir);
	free(dirp);
	return 0;
}

#endif
