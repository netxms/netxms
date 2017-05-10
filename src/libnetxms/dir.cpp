/* 
** NetXMS - Network Management System
** Utility Library
** Copyright (C) 2003-2015 Victor Kirhenshtein
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
** File: dir.cpp
**
**/

#include "libnetxms.h"

#ifdef _WIN32

/**
 * Open directory
 */
DIR LIBNETXMS_EXPORTABLE *opendir(const char *path)
{
   if (*path == 0)
      return NULL;  // empty file name

   // check to see if filename is a directory
   char pattern[MAX_PATH];
   strncpy_s(pattern, MAX_PATH, path, _TRUNCATE);
   size_t len = strlen(pattern);
   char tail = pattern[len - 1];
   if ((tail == '/') || (tail == '\\'))
   {
      pattern[len - 1] = 0;
      len--;
   }
   if (pattern[0] == 0)
      return NULL;
   if (pattern[len - 1] == ':')
   {
      pattern[len] = '\\';
      pattern[len + 1] = 0;
   }
   struct _stati64 sbuf;
   if ((_stati64(pattern, &sbuf) < 0) || ((sbuf.st_mode & S_IFDIR) == 0))
      return NULL;

   // create search pattern
   if (pattern[len] != '\\')
      pattern[len] = '\\';
   pattern[len + 1] = '*';
   pattern[len + 2] = 0;

   WIN32_FIND_DATAA fd;
   HANDLE handle = FindFirstFileA(pattern, &fd);
   if (handle == INVALID_HANDLE_VALUE)
      return NULL;

   DIR *p = (DIR *)malloc(sizeof(DIR));
   if (p == NULL)
      return NULL;

   p->handle = handle;
   p->dirstr.d_ino = 0;
   p->dirstr.d_type = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? DT_DIR : DT_REG;
   strncpy_s(p->dirstr.d_name, MAX_PATH, fd.cFileName, _TRUNCATE);
   p->dirstr.d_namlen = (int)strlen(p->dirstr.d_name);
   return p;
}

/**
 * Read next entry in directory
 */
struct dirent LIBNETXMS_EXPORTABLE *readdir(DIR *p)
{
   if (p == NULL)
      return NULL;

   // First call to readdir(), return entry found by FindFirstFile
   if (p->dirstr.d_ino == 0)
   {
      p->dirstr.d_ino++;
      return &p->dirstr;
   }

   WIN32_FIND_DATAA fd;
   if (!FindNextFileA(p->handle, &fd))
      return NULL;

   p->dirstr.d_ino++;
   p->dirstr.d_type = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? DT_DIR : DT_REG;
   strncpy_s(p->dirstr.d_name, MAX_PATH, fd.cFileName, _TRUNCATE);
   p->dirstr.d_namlen = (int)strlen(p->dirstr.d_name);
   return &p->dirstr;
}

/**
 * Close directory handle
 */
int LIBNETXMS_EXPORTABLE closedir(DIR *p)
{
   if (p == NULL)
      return -1;
   FindClose(p->handle);
   free(p);
   return 0;
}

#endif   /* _WIN32 */
