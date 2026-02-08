/*
** NetXMS AI File Operations subagent
** Copyright (C) 2003-2026 Victor Kirhenshtein
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
** File: file.cpp
**
**/

#include "aifileops.h"
#include <nxstat.h>

/**
 * Tool: file_info
 */
uint32_t H_FileInfo(json_t *params, json_t **result, AbstractCommSession *session)
{
   String file = json_object_get_string(params, "file", _T(""));
   if (file.isEmpty())
   {
      SetError(result, "MISSING_PARAM", "Required parameter 'file' must be provided");
      return ERR_BAD_ARGUMENTS;
   }

   bool countLines = json_object_get_boolean(params, "count_lines", false);

   NX_STAT_STRUCT st;
   if (CALL_STAT(file, &st) != 0)
   {
      SetError(result, "FILE_ERROR", "Failed to stat file");
      return ERR_FILE_OPEN_ERROR;
   }

   *result = json_object();
   json_object_set_new(*result, "path", json_string_t(file));
   json_object_set_new(*result, "size", json_integer(st.st_size));
   json_object_set_new(*result, "mtime", json_integer(st.st_mtime));
   json_object_set_new(*result, "atime", json_integer(st.st_atime));
   json_object_set_new(*result, "ctime", json_integer(st.st_ctime));

#ifndef _WIN32
   json_object_set_new(*result, "mode", json_integer(st.st_mode & 0777));
   json_object_set_new(*result, "uid", json_integer(st.st_uid));
   json_object_set_new(*result, "gid", json_integer(st.st_gid));
#endif

   json_object_set_new(*result, "is_directory", json_boolean(S_ISDIR(st.st_mode)));
   json_object_set_new(*result, "is_regular_file", json_boolean(S_ISREG(st.st_mode)));
#ifndef _WIN32
   json_object_set_new(*result, "is_symlink", json_boolean(S_ISLNK(st.st_mode)));
#endif

   // Count lines if requested
   if (countLines && S_ISREG(st.st_mode))
   {
      FILE *f = _tfopen(file, _T("r"));
      if (f != nullptr)
      {
         int64_t lineCount = 0;
         int ch;
         while ((ch = fgetc(f)) != EOF)
         {
            if (ch == '\n')
               lineCount++;
         }
         fclose(f);
         json_object_set_new(*result, "line_count", json_integer(lineCount));
      }
   }

   return ERR_SUCCESS;
}

/**
 * Helper: Recursively list directory
 */
static void ListDirectory(const TCHAR *basePath, const TCHAR *pattern, bool recursive, int maxEntries, json_t *entries, int *count)
{
   _TDIR *dir = _topendir(basePath);
   if (dir == nullptr)
      return;

   struct _tdirent *entry;
   while ((entry = _treaddir(dir)) != nullptr && *count < maxEntries)
   {
      if (!_tcscmp(entry->d_name, _T(".")) || !_tcscmp(entry->d_name, _T("..")))
         continue;

      TCHAR fullPath[MAX_PATH];
      _sntprintf(fullPath, MAX_PATH, _T("%s") FS_PATH_SEPARATOR _T("%s"), basePath, entry->d_name);

      NX_STAT_STRUCT st;
      if (CALL_STAT_FOLLOW_SYMLINK(fullPath, &st) != 0)
         continue;

      bool isDir = S_ISDIR(st.st_mode);

      // Check if matches pattern
      if (MatchString(pattern, entry->d_name, false))
      {
         json_t *fileEntry = json_object();
         json_object_set_new(fileEntry, "name", json_string_t(entry->d_name));
         json_object_set_new(fileEntry, "path", json_string_t(fullPath));
         json_object_set_new(fileEntry, "size", json_integer(st.st_size));
         json_object_set_new(fileEntry, "mtime", json_integer(st.st_mtime));
         json_object_set_new(fileEntry, "is_directory", json_boolean(isDir));
         json_array_append_new(entries, fileEntry);
         (*count)++;
      }

      // Recurse into directories
      if (isDir && recursive && *count < maxEntries)
      {
         ListDirectory(fullPath, pattern, recursive, maxEntries, entries, count);
      }
   }

   _tclosedir(dir);
}

/**
 * Tool: file_list
 */
uint32_t H_FileList(json_t *params, json_t **result, AbstractCommSession *session)
{
   String directory = json_object_get_string(params, "directory", _T(""));
   if (directory.isEmpty())
   {
      SetError(result, "MISSING_PARAM", "Required parameter 'directory' must be provided");
      return ERR_BAD_ARGUMENTS;
   }

   String pattern = json_object_get_string(params, "pattern", _T("*"));
   bool recursive = json_object_get_boolean(params, "recursive", false);
   int maxEntries = json_object_get_int32(params, "max_entries", 1000);

   // Check if directory exists
   NX_STAT_STRUCT st;
   if (CALL_STAT(directory, &st) != 0 || !S_ISDIR(st.st_mode))
   {
      SetError(result, "NOT_DIRECTORY", "Path is not a directory or does not exist");
      return ERR_BAD_ARGUMENTS;
   }

   *result = json_object();
   json_t *entries = json_array();
   int count = 0;

   ListDirectory(directory, pattern, recursive, maxEntries, entries, &count);

   json_object_set_new(*result, "entries", entries);
   json_object_set_new(*result, "count", json_integer(count));
   json_object_set_new(*result, "truncated", json_boolean(count >= maxEntries));

   return ERR_SUCCESS;
}

/**
 * Tool: file_read
 */
uint32_t H_FileRead(json_t *params, json_t **result, AbstractCommSession *session)
{
   String file = json_object_get_string(params, "file", _T(""));
   if (file.isEmpty())
   {
      SetError(result, "MISSING_PARAM", "Required parameter 'file' must be provided");
      return ERR_BAD_ARGUMENTS;
   }

   int64_t offset = json_object_get_int64(params, "offset", 0);
   int64_t limit = json_object_get_int64(params, "limit", 65536);

   if (limit > 1048576)
      limit = 1048576;  // Cap at 1MB

   FILE *f = _tfopen(file, _T("rb"));
   if (f == nullptr)
   {
      SetError(result, "FILE_ERROR", "Failed to open file");
      return ERR_FILE_OPEN_ERROR;
   }

   // Get file size
   fseek(f, 0, SEEK_END);
   int64_t fileSize = ftell(f);

   if (offset >= fileSize)
   {
      fclose(f);
      *result = json_object();
      json_object_set_new(*result, "content", json_string(""));
      json_object_set_new(*result, "bytes_read", json_integer(0));
      json_object_set_new(*result, "offset", json_integer(offset));
      json_object_set_new(*result, "file_size", json_integer(fileSize));
      return ERR_SUCCESS;
   }

   fseek(f, (long)offset, SEEK_SET);

   int64_t toRead = limit;
   if (offset + toRead > fileSize)
      toRead = fileSize - offset;

   char *buffer = MemAllocArray<char>(toRead + 1);
   size_t bytesRead = fread(buffer, 1, (size_t)toRead, f);
   buffer[bytesRead] = 0;
   fclose(f);

   *result = json_object();
   json_object_set_new(*result, "content", json_string(buffer));
   json_object_set_new(*result, "bytes_read", json_integer(bytesRead));
   json_object_set_new(*result, "offset", json_integer(offset));
   json_object_set_new(*result, "file_size", json_integer(fileSize));
   json_object_set_new(*result, "has_more", json_boolean(offset + (int64_t)bytesRead < fileSize));

   MemFree(buffer);
   return ERR_SUCCESS;
}
