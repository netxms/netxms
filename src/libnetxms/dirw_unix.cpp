/* 
** NetXMS - Network Management System
** Utility Library
** Copyright (C) 2003-2026 Raden Solutions
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

/**
 * opendir() wrapper
 */
DIRHANDLEW LIBNETXMS_EXPORTABLE *OpenDirW(const wchar_t *name)
{
   char mbname[MAX_PATH];
#if HAVE_WCSTOMBS
   wcstombs(mbname, name, MAX_PATH);
#else
   wchar_to_mb(name, -1, mbname, MAX_PATH);
#endif
   mbname[MAX_PATH - 1] = 0;
   DIR *dir = opendir(mbname);
   if (dir == nullptr)
      return nullptr;
   DIRHANDLEW *d = MemAllocStruct<DIRHANDLEW>();
   d->dir = dir;
   return d;
}

/**
 * readdir() wrapper
 */
DIRENTRYW LIBNETXMS_EXPORTABLE *ReadDirW(DIRHANDLEW *dirp)
{
   struct dirent *d = readdir(dirp->dir);
   if (d == nullptr)
      return nullptr;
#if HAVE_MBSTOWCS
   mbstowcs(dirp->dirstr.d_name, d->d_name, 257);
#else
   mb_to_wchar(d->d_name, -1, dirp->dirstr.d_name, 257);
#endif
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
int LIBNETXMS_EXPORTABLE CloseDirW(DIRHANDLEW *dirp)
{
   closedir(dirp->dir);
   MemFree(dirp);
   return 0;
}
