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
** File: patch.cpp
**
**/

#include "aifileops.h"
#include <nxstat.h>
#include <netxms-regex.h>

/**
 * Helper: Create backup of a file (returns backup path on success, nullptr on failure)
 */
static TCHAR *CreateBackup(const TCHAR *filePath)
{
   // Check if original file exists
   NX_STAT_STRUCT st;
   if (CALL_STAT(filePath, &st) != 0)
      return nullptr;

   // Generate backup path
   TCHAR *backupPath = MemAllocString(_tcslen(filePath) + 8);
   _tcscpy(backupPath, filePath);
   _tcscat(backupPath, _T(".bak"));

   // Copy file to backup
   FILE *src = _tfopen(filePath, _T("rb"));
   if (src == nullptr)
   {
      MemFree(backupPath);
      return nullptr;
   }

   FILE *dst = _tfopen(backupPath, _T("wb"));
   if (dst == nullptr)
   {
      fclose(src);
      MemFree(backupPath);
      return nullptr;
   }

   char buffer[8192];
   size_t bytesRead;
   while ((bytesRead = fread(buffer, 1, sizeof(buffer), src)) > 0)
   {
      if (fwrite(buffer, 1, bytesRead, dst) != bytesRead)
      {
         fclose(src);
         fclose(dst);
         _tremove(backupPath);
         MemFree(backupPath);
         return nullptr;
      }
   }

   fclose(src);
   fclose(dst);
   return backupPath;
}

/**
 * Helper: Create parent directories if needed
 */
static bool CreateParentDirectories(const TCHAR *filePath)
{
   TCHAR *path = MemCopyString(filePath);
   TCHAR *sep = _tcsrchr(path, FS_PATH_SEPARATOR_CHAR);
   if (sep == nullptr)
   {
      MemFree(path);
      return true;  // No directory component
   }

   *sep = 0;
   bool result = CreateDirectoryTree(path);
   MemFree(path);
   return result;
}

/**
 * Helper: Read file into StringList (one entry per line)
 */
static bool ReadFileToLines(const TCHAR *filePath, StringList *lines)
{
   FILE *f = _tfopen(filePath, _T("r"));
   if (f == nullptr)
      return false;

   char buffer[65536];
   while (fgets(buffer, sizeof(buffer), f) != nullptr)
   {
      // Remove trailing newline
      size_t len = strlen(buffer);
      if (len > 0 && buffer[len - 1] == '\n')
      {
         buffer[len - 1] = 0;
         len--;
      }
      if (len > 0 && buffer[len - 1] == '\r')
      {
         buffer[len - 1] = 0;
      }
#ifdef UNICODE
      lines->addPreallocated(WideStringFromUTF8String(buffer));
#else
      lines->add(buffer);
#endif
   }

   fclose(f);
   return true;
}

/**
 * Helper: Write StringList to file (each entry becomes a line)
 */
static bool WriteLinesToFile(const TCHAR *filePath, const StringList *lines)
{
   FILE *f = _tfopen(filePath, _T("w"));
   if (f == nullptr)
      return false;

   for (int i = 0; i < lines->size(); i++)
   {
#ifdef UNICODE
      char *utf8 = UTF8StringFromWideString(lines->get(i));
      fprintf(f, "%s\n", utf8);
      MemFree(utf8);
#else
      fprintf(f, "%s\n", lines->get(i));
#endif
   }

   fclose(f);
   return true;
}

/**
 * Helper: Atomic write - write to temp file then rename
 */
static bool AtomicWriteFile(const TCHAR *filePath, const char *content, size_t contentLen)
{
   // Generate temp file path
   TCHAR tempPath[MAX_PATH];
   _sntprintf(tempPath, MAX_PATH, _T("%s.tmp.%u"), filePath, (unsigned int)GetCurrentProcessId());

   // Write to temp file
   FILE *f = _tfopen(tempPath, _T("wb"));
   if (f == nullptr)
      return false;

   if (fwrite(content, 1, contentLen, f) != contentLen)
   {
      fclose(f);
      _tremove(tempPath);
      return false;
   }
   fclose(f);

   // Rename temp file to target
#ifdef _WIN32
   // On Windows, need to delete target first if it exists
   _tremove(filePath);
#endif
   if (_trename(tempPath, filePath) != 0)
   {
      _tremove(tempPath);
      return false;
   }

   return true;
}

/**
 * Tool: file_write
 */
uint32_t H_FileWrite(json_t *params, json_t **result, AbstractCommSession *session)
{
   String file = json_object_get_string(params, "file", _T(""));
   if (file.isEmpty())
   {
      SetError(result, "MISSING_PARAM", "Required parameter 'file' must be provided");
      return ERR_BAD_ARGUMENTS;
   }

   const char *content = json_object_get_string_utf8(params, "content", nullptr);
   if (content == nullptr)
   {
      SetError(result, "MISSING_PARAM", "Required parameter 'content' must be provided");
      return ERR_BAD_ARGUMENTS;
   }

   size_t contentLen = strlen(content);
   if (contentLen > MAX_WRITE_SIZE)
   {
      SetError(result, "SIZE_EXCEEDED", "Content size exceeds maximum allowed");
      return ERR_BAD_ARGUMENTS;
   }

   bool createBackup = json_object_get_boolean(params, "create_backup", true);
   bool createDirs = json_object_get_boolean(params, "create_dirs", false);

   // Check if file exists (for backup)
   NX_STAT_STRUCT st;
   bool fileExists = (CALL_STAT(file, &st) == 0);

   // Create parent directories if requested
   if (createDirs && !fileExists)
   {
      if (!CreateParentDirectories(file))
      {
         SetError(result, "DIR_CREATE_FAILED", "Failed to create parent directories");
         return ERR_FILE_OPEN_ERROR;
      }
   }

   // Create backup if file exists and backup is requested
   TCHAR *backupPath = nullptr;
   if (fileExists && createBackup)
   {
      backupPath = CreateBackup(file);
      if (backupPath == nullptr)
      {
         SetError(result, "BACKUP_FAILED", "Failed to create backup file");
         return ERR_FILE_OPEN_ERROR;
      }
   }

   // Write file
   if (!AtomicWriteFile(file, content, contentLen))
   {
      MemFree(backupPath);
      SetError(result, "WRITE_FAILED", "Failed to write file");
      return ERR_FILE_OPEN_ERROR;
   }

   *result = json_object();
   json_object_set_new(*result, "success", json_boolean(true));
   json_object_set_new(*result, "path", json_string_t(file));
   json_object_set_new(*result, "bytes_written", json_integer(contentLen));
   if (backupPath != nullptr)
   {
      json_object_set_new(*result, "backup_path", json_string_t(backupPath));
      MemFree(backupPath);
   }

   return ERR_SUCCESS;
}

/**
 * Tool: file_append
 */
uint32_t H_FileAppend(json_t *params, json_t **result, AbstractCommSession *session)
{
   String file = json_object_get_string(params, "file", _T(""));
   if (file.isEmpty())
   {
      SetError(result, "MISSING_PARAM", "Required parameter 'file' must be provided");
      return ERR_BAD_ARGUMENTS;
   }

   const char *content = json_object_get_string_utf8(params, "content", nullptr);
   if (content == nullptr)
   {
      SetError(result, "MISSING_PARAM", "Required parameter 'content' must be provided");
      return ERR_BAD_ARGUMENTS;
   }

   size_t contentLen = strlen(content);
   if (contentLen > MAX_WRITE_SIZE)
   {
      SetError(result, "SIZE_EXCEEDED", "Content size exceeds maximum allowed");
      return ERR_BAD_ARGUMENTS;
   }

   bool createIfMissing = json_object_get_boolean(params, "create_if_missing", false);
   bool addNewline = json_object_get_boolean(params, "add_newline", true);

   // Check if file exists
   NX_STAT_STRUCT st;
   bool fileExists = (CALL_STAT(file, &st) == 0);

   if (!fileExists && !createIfMissing)
   {
      SetError(result, "FILE_NOT_FOUND", "File does not exist and create_if_missing is false");
      return ERR_FILE_OPEN_ERROR;
   }

   if (fileExists && st.st_size > MAX_FILE_SIZE_MODIFY)
   {
      SetError(result, "SIZE_EXCEEDED", "File size exceeds maximum allowed for modification");
      return ERR_BAD_ARGUMENTS;
   }

   // Open file for appending
   FILE *f = _tfopen(file, _T("ab+"));
   if (f == nullptr)
   {
      SetError(result, "FILE_ERROR", "Failed to open file for appending");
      return ERR_FILE_OPEN_ERROR;
   }

   // Check if we need to add a newline before content
   bool needsNewline = false;
   if (addNewline && fileExists && st.st_size > 0)
   {
      fseek(f, -1, SEEK_END);
      int lastChar = fgetc(f);
      if (lastChar != '\n')
         needsNewline = true;
      fseek(f, 0, SEEK_END);
   }

   size_t bytesAppended = 0;
   if (needsNewline)
   {
      fputc('\n', f);
      bytesAppended++;
   }

   size_t written = fwrite(content, 1, contentLen, f);
   bytesAppended += written;

   // Get new file size
   fseek(f, 0, SEEK_END);
   long newSize = ftell(f);
   fclose(f);

   if (written != contentLen)
   {
      SetError(result, "WRITE_FAILED", "Failed to write all content");
      return ERR_FILE_OPEN_ERROR;
   }

   *result = json_object();
   json_object_set_new(*result, "success", json_boolean(true));
   json_object_set_new(*result, "bytes_appended", json_integer(bytesAppended));
   json_object_set_new(*result, "new_size", json_integer(newSize));

   return ERR_SUCCESS;
}

/**
 * Tool: file_insert
 */
uint32_t H_FileInsert(json_t *params, json_t **result, AbstractCommSession *session)
{
   String file = json_object_get_string(params, "file", _T(""));
   if (file.isEmpty())
   {
      SetError(result, "MISSING_PARAM", "Required parameter 'file' must be provided");
      return ERR_BAD_ARGUMENTS;
   }

   int lineNum = json_object_get_int32(params, "line", 0);
   if (lineNum < 1)
   {
      SetError(result, "INVALID_PARAM", "Parameter 'line' must be >= 1");
      return ERR_BAD_ARGUMENTS;
   }

   const char *content = json_object_get_string_utf8(params, "content", nullptr);
   if (content == nullptr)
   {
      SetError(result, "MISSING_PARAM", "Required parameter 'content' must be provided");
      return ERR_BAD_ARGUMENTS;
   }

   bool createBackup = json_object_get_boolean(params, "create_backup", true);

   // Check file size
   NX_STAT_STRUCT st;
   if (CALL_STAT(file, &st) != 0)
   {
      SetError(result, "FILE_NOT_FOUND", "File does not exist");
      return ERR_FILE_OPEN_ERROR;
   }

   if (st.st_size > MAX_FILE_SIZE_MODIFY)
   {
      SetError(result, "SIZE_EXCEEDED", "File size exceeds maximum allowed for modification");
      return ERR_BAD_ARGUMENTS;
   }

   // Read file into lines
   StringList lines;
   if (!ReadFileToLines(file, &lines))
   {
      SetError(result, "FILE_ERROR", "Failed to read file");
      return ERR_FILE_OPEN_ERROR;
   }

   int linesBefore = lines.size();

   // Create backup
   TCHAR *backupPath = nullptr;
   if (createBackup)
   {
      backupPath = CreateBackup(file);
      if (backupPath == nullptr)
      {
         SetError(result, "BACKUP_FAILED", "Failed to create backup file");
         return ERR_FILE_OPEN_ERROR;
      }
   }

   // Split content into lines and insert
   StringList contentLines;
#ifdef UNICODE
   WCHAR *wContent = WideStringFromUTF8String(content);
   contentLines.splitAndAdd(wContent, _T("\n"));
   MemFree(wContent);
#else
   contentLines.splitAndAdd(content, _T("\n"));
#endif

   // Insert at specified position (convert to 0-based index)
   int insertPos = lineNum - 1;
   if (insertPos > lines.size())
      insertPos = lines.size();

   for (int i = contentLines.size() - 1; i >= 0; i--)
   {
      lines.insert(insertPos, contentLines.get(i));
   }

   // Write back to file
   if (!WriteLinesToFile(file, &lines))
   {
      MemFree(backupPath);
      SetError(result, "WRITE_FAILED", "Failed to write file");
      return ERR_FILE_OPEN_ERROR;
   }

   *result = json_object();
   json_object_set_new(*result, "success", json_boolean(true));
   json_object_set_new(*result, "lines_before", json_integer(linesBefore));
   json_object_set_new(*result, "lines_after", json_integer(lines.size()));
   if (backupPath != nullptr)
   {
      json_object_set_new(*result, "backup_path", json_string_t(backupPath));
      MemFree(backupPath);
   }

   return ERR_SUCCESS;
}

/**
 * Tool: file_delete_lines
 */
uint32_t H_FileDeleteLines(json_t *params, json_t **result, AbstractCommSession *session)
{
   String file = json_object_get_string(params, "file", _T(""));
   if (file.isEmpty())
   {
      SetError(result, "MISSING_PARAM", "Required parameter 'file' must be provided");
      return ERR_BAD_ARGUMENTS;
   }

   int startLine = json_object_get_int32(params, "start_line", 0);
   int endLine = json_object_get_int32(params, "end_line", 0);

   if (startLine < 1 || endLine < 1)
   {
      SetError(result, "INVALID_PARAM", "Parameters 'start_line' and 'end_line' must be >= 1");
      return ERR_BAD_ARGUMENTS;
   }

   if (endLine < startLine)
   {
      SetError(result, "INVALID_PARAM", "Parameter 'end_line' must be >= 'start_line'");
      return ERR_BAD_ARGUMENTS;
   }

   bool createBackup = json_object_get_boolean(params, "create_backup", true);

   // Check file size
   NX_STAT_STRUCT st;
   if (CALL_STAT(file, &st) != 0)
   {
      SetError(result, "FILE_NOT_FOUND", "File does not exist");
      return ERR_FILE_OPEN_ERROR;
   }

   if (st.st_size > MAX_FILE_SIZE_MODIFY)
   {
      SetError(result, "SIZE_EXCEEDED", "File size exceeds maximum allowed for modification");
      return ERR_BAD_ARGUMENTS;
   }

   // Read file into lines
   StringList lines;
   if (!ReadFileToLines(file, &lines))
   {
      SetError(result, "FILE_ERROR", "Failed to read file");
      return ERR_FILE_OPEN_ERROR;
   }

   int linesBefore = lines.size();

   // Validate line range
   if (startLine > linesBefore)
   {
      SetError(result, "INVALID_RANGE", "Start line is beyond end of file");
      return ERR_BAD_ARGUMENTS;
   }

   // Clamp end line to file size
   if (endLine > linesBefore)
      endLine = linesBefore;

   // Create backup
   TCHAR *backupPath = nullptr;
   if (createBackup)
   {
      backupPath = CreateBackup(file);
      if (backupPath == nullptr)
      {
         SetError(result, "BACKUP_FAILED", "Failed to create backup file");
         return ERR_FILE_OPEN_ERROR;
      }
   }

   // Delete lines (convert to 0-based index)
   int deleteStart = startLine - 1;
   int deleteCount = endLine - startLine + 1;

   for (int i = 0; i < deleteCount && deleteStart < lines.size(); i++)
   {
      lines.remove(deleteStart);
   }

   // Write back to file
   if (!WriteLinesToFile(file, &lines))
   {
      MemFree(backupPath);
      SetError(result, "WRITE_FAILED", "Failed to write file");
      return ERR_FILE_OPEN_ERROR;
   }

   *result = json_object();
   json_object_set_new(*result, "success", json_boolean(true));
   json_object_set_new(*result, "lines_deleted", json_integer(deleteCount));
   json_object_set_new(*result, "lines_before", json_integer(linesBefore));
   json_object_set_new(*result, "lines_after", json_integer(lines.size()));
   if (backupPath != nullptr)
   {
      json_object_set_new(*result, "backup_path", json_string_t(backupPath));
      MemFree(backupPath);
   }

   return ERR_SUCCESS;
}

/**
 * Helper: Apply regex replacement to a line
 */
static char *ApplyReplacement(const char *line, pcre *re, pcre_extra *extra, const char *replacement, int *matchCount)
{
   int ovector[30];
   int rc = pcre_exec(re, extra, line, (int)strlen(line), 0, 0, ovector, 30);
   if (rc < 0)
      return nullptr;  // No match

   (*matchCount)++;

   // Build replacement string with backreferences
   StringBuffer result;
   const char *p = replacement;
   while (*p)
   {
      if (*p == '$' && (p[1] >= '0' && p[1] <= '9'))
      {
         int groupNum = p[1] - '0';
         if (groupNum < rc && ovector[groupNum * 2] >= 0)
         {
            int start = ovector[groupNum * 2];
            int len = ovector[groupNum * 2 + 1] - start;
            for (int i = 0; i < len; i++)
               result.append((TCHAR)line[start + i]);
         }
         p += 2;
      }
      else
      {
         result.append((TCHAR)*p);
         p++;
      }
   }

   // Build final string: prefix + replacement + suffix
   StringBuffer finalResult;
   for (int i = 0; i < ovector[0]; i++)
      finalResult.append((TCHAR)line[i]);
   finalResult.append(result);
   for (size_t i = ovector[1]; i < strlen(line); i++)
      finalResult.append((TCHAR)line[i]);

#ifdef UNICODE
   return UTF8StringFromWideString(finalResult);
#else
   return MemCopyStringA(finalResult.cstr());
#endif
}

/**
 * Tool: file_replace
 */
uint32_t H_FileReplace(json_t *params, json_t **result, AbstractCommSession *session)
{
   String file = json_object_get_string(params, "file", _T(""));
   if (file.isEmpty())
   {
      SetError(result, "MISSING_PARAM", "Required parameter 'file' must be provided");
      return ERR_BAD_ARGUMENTS;
   }

   const char *pattern = json_object_get_string_utf8(params, "pattern", nullptr);
   if (pattern == nullptr)
   {
      SetError(result, "MISSING_PARAM", "Required parameter 'pattern' must be provided");
      return ERR_BAD_ARGUMENTS;
   }

   const char *replacement = json_object_get_string_utf8(params, "replacement", nullptr);
   if (replacement == nullptr)
   {
      SetError(result, "MISSING_PARAM", "Required parameter 'replacement' must be provided");
      return ERR_BAD_ARGUMENTS;
   }

   int maxReplacements = json_object_get_int32(params, "max_replacements", 0);
   bool caseInsensitive = json_object_get_boolean(params, "case_insensitive", false);
   bool createBackup = json_object_get_boolean(params, "create_backup", true);
   bool dryRun = json_object_get_boolean(params, "dry_run", false);

   // Compile regex
   const char *errorMsg;
   int errorOffset;
   int options = PCRE_UTF8;
   if (caseInsensitive)
      options |= PCRE_CASELESS;

   pcre *re = pcre_compile(pattern, options, &errorMsg, &errorOffset, nullptr);
   if (re == nullptr)
   {
      SetError(result, "INVALID_PATTERN", "Failed to compile regular expression");
      return ERR_BAD_ARGUMENTS;
   }

   pcre_extra *extra = pcre_study(re, 0, &errorMsg);

   // Check file size
   NX_STAT_STRUCT st;
   if (CALL_STAT(file, &st) != 0)
   {
      pcre_free(re);
      if (extra != nullptr)
         pcre_free_study(extra);
      SetError(result, "FILE_NOT_FOUND", "File does not exist");
      return ERR_FILE_OPEN_ERROR;
   }

   if (st.st_size > MAX_FILE_SIZE_MODIFY)
   {
      pcre_free(re);
      if (extra != nullptr)
         pcre_free_study(extra);
      SetError(result, "SIZE_EXCEEDED", "File size exceeds maximum allowed for modification");
      return ERR_BAD_ARGUMENTS;
   }

   // Read file into lines
   StringList lines;
   if (!ReadFileToLines(file, &lines))
   {
      pcre_free(re);
      if (extra != nullptr)
         pcre_free_study(extra);
      SetError(result, "FILE_ERROR", "Failed to read file");
      return ERR_FILE_OPEN_ERROR;
   }

   // Process replacements
   int totalReplacements = 0;
   int linesModified = 0;
   json_t *preview = dryRun ? json_array() : nullptr;

   for (int i = 0; i < lines.size() && (maxReplacements == 0 || totalReplacements < maxReplacements); i++)
   {
#ifdef UNICODE
      char *lineUtf8 = UTF8StringFromWideString(lines.get(i));
#else
      char *lineUtf8 = MemCopyStringA(lines.get(i));
#endif
      int lineMatchCount = 0;
      char *newLine = ApplyReplacement(lineUtf8, re, extra, replacement, &lineMatchCount);

      if (newLine != nullptr)
      {
         totalReplacements += lineMatchCount;
         linesModified++;

         if (dryRun)
         {
            json_t *change = json_object();
            json_object_set_new(change, "line", json_integer(i + 1));
            json_object_set_new(change, "original", json_string(lineUtf8));
            json_object_set_new(change, "modified", json_string(newLine));
            json_array_append_new(preview, change);
         }
         else
         {
#ifdef UNICODE
            WCHAR *wNewLine = WideStringFromUTF8String(newLine);
            lines.replace(i, wNewLine);
            MemFree(wNewLine);
#else
            lines.replace(i, newLine);
#endif
         }
         MemFree(newLine);
      }
      MemFree(lineUtf8);
   }

   pcre_free(re);
   if (extra != nullptr)
      pcre_free_study(extra);

   // Write back to file (unless dry run)
   TCHAR *backupPath = nullptr;
   if (!dryRun && totalReplacements > 0)
   {
      if (createBackup)
      {
         backupPath = CreateBackup(file);
         if (backupPath == nullptr)
         {
            json_decref(preview);
            SetError(result, "BACKUP_FAILED", "Failed to create backup file");
            return ERR_FILE_OPEN_ERROR;
         }
      }

      if (!WriteLinesToFile(file, &lines))
      {
         MemFree(backupPath);
         json_decref(preview);
         SetError(result, "WRITE_FAILED", "Failed to write file");
         return ERR_FILE_OPEN_ERROR;
      }
   }

   *result = json_object();
   json_object_set_new(*result, "success", json_boolean(true));
   json_object_set_new(*result, "replacements_made", json_integer(totalReplacements));
   json_object_set_new(*result, "lines_modified", json_integer(linesModified));
   if (preview != nullptr)
      json_object_set_new(*result, "preview", preview);
   if (backupPath != nullptr)
   {
      json_object_set_new(*result, "backup_path", json_string_t(backupPath));
      MemFree(backupPath);
   }

   return ERR_SUCCESS;
}

/**
 * Helper: Parse a hunk header line "@@ -start,count +start,count @@"
 */
static bool ParseHunkHeader(const char *line, int *oldStart, int *oldCount, int *newStart, int *newCount)
{
   // Format: @@ -l,s +l,s @@
   if (strncmp(line, "@@ -", 4) != 0)
      return false;

   const char *p = line + 4;

   // Parse old start
   *oldStart = atoi(p);
   while (*p && *p != ',' && *p != ' ')
      p++;
   if (*p == ',')
   {
      p++;
      *oldCount = atoi(p);
      while (*p && *p != ' ')
         p++;
   }
   else
   {
      *oldCount = 1;
   }

   // Skip to new section
   while (*p && *p != '+')
      p++;
   if (*p != '+')
      return false;
   p++;

   // Parse new start
   *newStart = atoi(p);
   while (*p && *p != ',' && *p != ' ')
      p++;
   if (*p == ',')
   {
      p++;
      *newCount = atoi(p);
   }
   else
   {
      *newCount = 1;
   }

   return true;
}

/**
 * Tool: file_patch
 */
uint32_t H_FilePatch(json_t *params, json_t **result, AbstractCommSession *session)
{
   String file = json_object_get_string(params, "file", _T(""));
   if (file.isEmpty())
   {
      SetError(result, "MISSING_PARAM", "Required parameter 'file' must be provided");
      return ERR_BAD_ARGUMENTS;
   }

   const char *diff = json_object_get_string_utf8(params, "diff", nullptr);
   if (diff == nullptr)
   {
      SetError(result, "MISSING_PARAM", "Required parameter 'diff' must be provided");
      return ERR_BAD_ARGUMENTS;
   }

   bool createBackup = json_object_get_boolean(params, "create_backup", true);
   int fuzz = json_object_get_int32(params, "fuzz", 2);

   // Check file size
   NX_STAT_STRUCT st;
   if (CALL_STAT(file, &st) != 0)
   {
      SetError(result, "FILE_NOT_FOUND", "File does not exist");
      return ERR_FILE_OPEN_ERROR;
   }

   if (st.st_size > MAX_FILE_SIZE_MODIFY)
   {
      SetError(result, "SIZE_EXCEEDED", "File size exceeds maximum allowed for modification");
      return ERR_BAD_ARGUMENTS;
   }

   // Read file into lines
   StringList lines;
   if (!ReadFileToLines(file, &lines))
   {
      SetError(result, "FILE_ERROR", "Failed to read file");
      return ERR_FILE_OPEN_ERROR;
   }

   // Parse diff into lines (work with UTF-8 directly)
   StringList diffLinesUtf8;
   char *diffCopy = MemCopyStringA(diff);
   char *saveptr;
   char *token = strtok_r(diffCopy, "\n", &saveptr);
   while (token != nullptr)
   {
      diffLinesUtf8.addMBString(token);
      token = strtok_r(nullptr, "\n", &saveptr);
   }
   MemFree(diffCopy);

   // Process hunks
   int hunksApplied = 0;
   int hunksFailed = 0;
   int lineOffset = 0;  // Tracks cumulative line number changes

   for (int i = 0; i < diffLinesUtf8.size(); i++)
   {
      // Convert to UTF-8 for parsing
#ifdef UNICODE
      char *diffLine = UTF8StringFromWideString(diffLinesUtf8.get(i));
#else
      const char *diffLine = diffLinesUtf8.get(i);
#endif

      // Skip non-hunk lines
      if (strncmp(diffLine, "@@", 2) != 0)
      {
#ifdef UNICODE
         MemFree(diffLine);
#endif
         continue;
      }

      // Parse hunk header
      int oldStart, oldCount, newStart, newCount;
      if (!ParseHunkHeader(diffLine, &oldStart, &oldCount, &newStart, &newCount))
      {
#ifdef UNICODE
         MemFree(diffLine);
#endif
         hunksFailed++;
         continue;
      }
#ifdef UNICODE
      MemFree(diffLine);
#endif

      // Collect hunk content
      StringList contextLines;   // Lines that should match (starting with ' ')
      StringList removeLines;    // Lines to remove (starting with '-')
      StringList addLines;       // Lines to add (starting with '+')

      i++;
      while (i < diffLinesUtf8.size())
      {
         const TCHAR *hunkLineT = diffLinesUtf8.get(i);
         if (_tcslen(hunkLineT) == 0)
         {
            contextLines.add(_T(""));
            i++;
            continue;
         }

         TCHAR prefix = hunkLineT[0];
         if (prefix == _T('@') || (prefix != _T(' ') && prefix != _T('+') && prefix != _T('-')))
         {
            i--;  // Back up to process as next hunk
            break;
         }

         const TCHAR *content = hunkLineT + 1;

         if (prefix == _T(' '))
         {
            contextLines.add(content);
         }
         else if (prefix == _T('-'))
         {
            removeLines.add(content);
            contextLines.add(content);
         }
         else if (prefix == _T('+'))
         {
            addLines.add(content);
         }
         i++;
      }

      // Find the best match position (with fuzz factor)
      int targetLine = oldStart - 1 + lineOffset;
      int bestMatch = -1;

      for (int offset = 0; offset <= fuzz; offset++)
      {
         for (int dir = 0; dir <= 1; dir++)
         {
            int tryPos = targetLine + (dir == 0 ? offset : -offset);
            if (tryPos < 0 || tryPos + oldCount > lines.size())
               continue;

            // Check if context matches
            bool matches = true;
            int contextIdx = 0;
            for (int j = 0; j < oldCount && matches && contextIdx < contextLines.size(); j++)
            {
               if (_tcscmp(lines.get(tryPos + j), contextLines.get(contextIdx)) != 0)
                  matches = false;
               contextIdx++;
            }

            if (matches)
            {
               bestMatch = tryPos;
               break;
            }
         }
         if (bestMatch >= 0)
            break;
      }

      if (bestMatch < 0)
      {
         hunksFailed++;
         continue;
      }

      // Apply the hunk: remove old lines, insert new lines
      for (int j = 0; j < oldCount && bestMatch < lines.size(); j++)
      {
         lines.remove(bestMatch);
      }

      for (int j = addLines.size() - 1; j >= 0; j--)
      {
         lines.insert(bestMatch, addLines.get(j));
      }

      lineOffset += newCount - oldCount;
      hunksApplied++;
   }

   // Write back to file if any hunks were applied
   TCHAR *backupPath = nullptr;
   if (hunksApplied > 0)
   {
      if (createBackup)
      {
         backupPath = CreateBackup(file);
         if (backupPath == nullptr)
         {
            SetError(result, "BACKUP_FAILED", "Failed to create backup file");
            return ERR_FILE_OPEN_ERROR;
         }
      }

      if (!WriteLinesToFile(file, &lines))
      {
         MemFree(backupPath);
         SetError(result, "WRITE_FAILED", "Failed to write file");
         return ERR_FILE_OPEN_ERROR;
      }
   }

   *result = json_object();
   json_object_set_new(*result, "success", json_boolean(hunksFailed == 0));
   json_object_set_new(*result, "hunks_applied", json_integer(hunksApplied));
   json_object_set_new(*result, "hunks_failed", json_integer(hunksFailed));
   if (backupPath != nullptr)
   {
      json_object_set_new(*result, "backup_path", json_string_t(backupPath));
      MemFree(backupPath);
   }

   return ERR_SUCCESS;
}
