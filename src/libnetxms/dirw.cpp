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
** File: dirw.cpp
**
**/

#include "libnetxms.h"

#ifdef _WIN32

/**
 * Open directory
 */
DIRW LIBNETXMS_EXPORTABLE *wopendir(const WCHAR *path)
{
   if (*path == 0)
      return NULL;  // empty file name

   // check to see if filename is a directory
   WCHAR pattern[MAX_PATH];
   wcsncpy_s(pattern, MAX_PATH, path, _TRUNCATE);
   size_t len = wcslen(pattern);
   WCHAR tail = pattern[len - 1];
   if ((tail == L'/') || (tail == L'\\'))
   {
      pattern[len - 1] = 0;
      len--;
   }
   if (pattern[0] == 0)
      return NULL;
   if (pattern[len - 1] == L':')
   {
      pattern[len] = L'\\';
      pattern[len + 1] = 0;
   }
   struct _stati64 sbuf;
   if ((_wstati64(pattern, &sbuf) < 0) || ((sbuf.st_mode & S_IFDIR) == 0))
      return NULL;

   // create search pattern
   if (pattern[len] != L'\\')
      pattern[len] = L'\\';
   pattern[len + 1] = L'*';
   pattern[len + 2] = 0;

   WIN32_FIND_DATAW fd;
   HANDLE handle = FindFirstFileW(pattern, &fd);
   if (handle == INVALID_HANDLE_VALUE)
      return NULL;

   DIRW *p = (DIRW *)malloc(sizeof(DIRW));
   if (p == NULL)
      return NULL;

   p->handle = handle;
   p->dirstr.d_ino = 0;
   p->dirstr.d_type = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? DT_DIR : DT_REG;
   wcsncpy_s(p->dirstr.d_name, MAX_PATH, fd.cFileName, _TRUNCATE);
   p->dirstr.d_namlen = (int)wcslen(p->dirstr.d_name);
   return p;
}

/**
 * Read next entry in directory
 */
struct dirent_w LIBNETXMS_EXPORTABLE *wreaddir(DIRW *p)
{
   if (p == NULL)
      return NULL;

   // First call to readdir(), return entry found by FindFirstFile
   if (p->dirstr.d_ino == 0)
   {
      p->dirstr.d_ino++;
      return &p->dirstr;
   }

   WIN32_FIND_DATAW fd;
   if (!FindNextFileW(p->handle, &fd))
      return NULL;

   p->dirstr.d_ino++;
   p->dirstr.d_type = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? DT_DIR : DT_REG;
   wcsncpy_s(p->dirstr.d_name, MAX_PATH, fd.cFileName, _TRUNCATE);
   p->dirstr.d_namlen = (int)wcslen(p->dirstr.d_name);
   return &p->dirstr;
}

/**
 * Close directory handle
 */
int LIBNETXMS_EXPORTABLE wclosedir(DIRW *p)
{
   if (p == NULL)
      return -1;
   FindClose(p->handle);
   free(p);
   return 0;
}

#endif   /* _WIN32 */
