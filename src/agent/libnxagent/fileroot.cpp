/*
** NetXMS - Network Management System
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
** File: fileroot.cpp
**
**/

#include "libnxagent.h"

/**
 * Create root folder from configuration value "path[;ro][;nofollow]"
 */
FileAccessRoot::FileAccessRoot(const TCHAR *definition)
{
   m_folder = MemCopyString(definition);
   m_readOnly = false;
   m_followSymlinks = true;

   // Parse semicolon-separated flags after the path: "ro", "nofollow"
   TCHAR *ptr = _tcschr(m_folder, _T(';'));
   if (ptr != nullptr)
   {
      *ptr = 0;
      TCHAR *flag = ptr + 1;
      while (flag != nullptr)
      {
         TCHAR *next = _tcschr(flag, _T(';'));
         if (next != nullptr)
            *next++ = 0;
         if (_tcscmp(flag, _T("ro")) == 0)
            m_readOnly = true;
         else if (_tcscmp(flag, _T("nofollow")) == 0)
            m_followSymlinks = false;
         flag = next;
      }
   }

#ifdef _WIN32
   for(int i = 0; m_folder[i] != 0; i++)
      if (m_folder[i] == _T('/'))
         m_folder[i] = _T('\\');
#endif

   size_t len = _tcslen(m_folder);

#ifdef _WIN32
   // On Windows a bare drive specification ("C:") is drive-relative — it refers
   // to the current directory on that drive, not the drive root. Normalize it to
   // a proper drive root ("C:\") so it cannot be expanded into an arbitrary path.
   if ((len == 2) && (m_folder[1] == _T(':')))
   {
      m_folder = MemReallocArray(m_folder, 4);
      m_folder[2] = FS_PATH_SEPARATOR_CHAR;
      m_folder[3] = 0;
      len = 3;
   }
#endif

   // Strip trailing path separator so that "/usr" and "/usr/" are equivalent
   // and the prefix check in resolvePath has a consistent boundary. Bare
   // length-1 roots ("/" or "\") are kept as-is, and on Windows the trailing
   // separator of a drive root ("C:\") is kept too — "C:" alone is drive-relative.
   if ((len > 1) && (m_folder[len - 1] == FS_PATH_SEPARATOR_CHAR)
#ifdef _WIN32
       && !((len == 3) && (m_folder[1] == _T(':')))
#endif
      )
      m_folder[len - 1] = 0;
}

/**
 * Register root directories from all values of given configuration entry
 */
void FileAccessRootList::addFromConfig(const ConfigEntry *entry)
{
   for(int i = 0; i < entry->getValueCount(); i++)
   {
      auto folder = new FileAccessRoot(entry->getValue(i));

      bool alreadyRegistered = false;
      for(int j = 0; j < m_roots.size(); j++)
      {
         FileAccessRoot *curr = m_roots.get(j);
#ifdef _WIN32
         if (!_tcsicmp(curr->getFolder(), folder->getFolder()))
#else
         if (!_tcscmp(curr->getFolder(), folder->getFolder()))
#endif
         {
            if (curr->isReadOnly() && !folder->isReadOnly())
               m_roots.remove(j); // Replace read-only element with read-write
            else
               alreadyRegistered = true;
            break;
         }
      }

      if (!alreadyRegistered)
      {
         m_roots.add(folder);
         nxlog_write_tag(NXLOG_INFO, m_debugTag, _T("Added root directory \"%s\" (%s%s)"),
            folder->getFolder(),
            folder->isReadOnly() ? _T("R/O") : _T("R/W"),
            folder->followSymlinks() ? _T("") : _T(", nofollow"));
      }
      else
      {
         nxlog_debug_tag(m_debugTag, 5, _T("Root directory \"%s\" already registered"), folder->getFolder());
         delete folder;
      }
   }
}

#ifndef _WIN32

/**
 * Converts path to absolute removing "//", "../", "./" ...
 */
static TCHAR *GetRealPath(const TCHAR *path)
{
   if ((path == nullptr) || (path[0] == 0))
      return nullptr;
   TCHAR *result = MemAllocString(MAX_PATH);
   _tcscpy(result, path);
   TCHAR *current = result;

   // just remove all dots before path
   if (!_tcsncmp(current, _T("../"), 3))
      memmove(current, current + 3, (_tcslen(current+3) + 1) * sizeof(TCHAR));

   if (!_tcsncmp(current, _T("./"), 2))
      memmove(current, current + 2, (_tcslen(current+2) + 1) * sizeof(TCHAR));

   while(*current != 0)
   {
      if (current[0] == '/')
      {
         switch(current[1])
         {
            case '/':
               memmove(current, current + 1, _tcslen(current) * sizeof(TCHAR));
               break;
            case '.':
               if (current[2] != 0)
               {
                  if (current[2] == '.' && (current[3] == 0 || current[3] == '/'))
                  {
                     if (current == result)
                     {
                        memmove(current, current + 3, (_tcslen(current + 3) + 1) * sizeof(TCHAR));
                     }
                     else
                     {
                        TCHAR *tmp = current;
                        do
                        {
                           tmp--;
                           if (tmp[0] == '/')
                           {
                              break;
                           }
                        } while(result != tmp);
                        memmove(tmp, current + 3, (_tcslen(current+3) + 1) * sizeof(TCHAR));
                     }
                  }
                  else
                  {
                     // dot + something, skip both
                     current += 2;
                  }
               }
               else
               {
                  // "/." at the end
                  *current = 0;
               }
               break;
            default:
               current++;
               break;
         }
      }
      else
      {
         current++;
      }
   }
   return result;
}

/**
 * Resolve symbolic links in an absolute path using realpath(3). If the target
 * does not yet exist, walk up to the deepest existing ancestor, resolve that,
 * and re-append the unresolved trailing components verbatim. This still ensures
 * that any symbolic link in the existing portion of the path is followed, so
 * the caller can verify the resolved path stays inside the configured root.
 * Returns a newly allocated TCHAR string on success, nullptr on error.
 */
static TCHAR *ResolveSymlinks(const TCHAR *path)
{
   if ((path == nullptr) || (path[0] != _T('/')))
      return nullptr;

   char workingPath[MAX_PATH];
#ifdef UNICODE
   if (wchar_to_mb(path, -1, workingPath, MAX_PATH) == 0)
      return nullptr;
   workingPath[MAX_PATH - 1] = 0;
#else
   strlcpy(workingPath, path, MAX_PATH);
#endif

   char resolved[PATH_MAX];
   char tail[MAX_PATH];
   tail[0] = 0;
   size_t tailLen = 0;

   while (realpath(workingPath, resolved) == nullptr)
   {
      if (errno != ENOENT)
         return nullptr;

      char *slash = strrchr(workingPath, '/');
      if (slash == nullptr)
         return nullptr;

      const char *segment = slash + 1;
      size_t segmentLen = strlen(segment);
      if (segmentLen > 0)
      {
         size_t addLen = segmentLen + (tailLen > 0 ? 1 : 0);
         if (tailLen + addLen + 1 > sizeof(tail))
            return nullptr;
         if (tailLen > 0)
         {
            memmove(tail + segmentLen + 1, tail, tailLen + 1);
            tail[segmentLen] = '/';
         }
         else
         {
            tail[segmentLen] = 0;
         }
         memcpy(tail, segment, segmentLen);
         tailLen += addLen;
      }

      if (slash == workingPath)
         workingPath[1] = 0;  // parent of last component is root "/"
      else
         *slash = 0;
   }

   if (tailLen > 0)
   {
      size_t resolvedLen = strlen(resolved);
      bool needsSeparator = (resolvedLen == 0) || (resolved[resolvedLen - 1] != '/');
      if (resolvedLen + (needsSeparator ? 1 : 0) + tailLen + 1 > sizeof(resolved))
         return nullptr;
      if (needsSeparator)
         resolved[resolvedLen++] = '/';
      memcpy(resolved + resolvedLen, tail, tailLen + 1);
   }

   TCHAR *result = MemAllocString(MAX_PATH);
#ifdef UNICODE
   if (mb_to_wchar(resolved, -1, result, MAX_PATH) == 0)
   {
      MemFree(result);
      return nullptr;
   }
   result[MAX_PATH - 1] = 0;
#else
   strlcpy(result, resolved, MAX_PATH);
#endif
   return result;
}

#endif

/**
 * Check whether candidatePath is contained within rootPath. Requires the prefix
 * match to terminate at a path boundary (separator or end of string), so root
 * "/opt/netxms" does not falsely match sibling "/opt/netxms-secret/...".
 */
static bool IsPathUnderRoot(const TCHAR *rootPath, size_t folderPathLen, const TCHAR *candidatePath)
{
#if defined(_WIN32) || defined(__APPLE__)
   if (_tcsnicmp(rootPath, candidatePath, folderPathLen) != 0)
      return false;
#else
   if (_tcsncmp(rootPath, candidatePath, folderPathLen) != 0)
      return false;
#endif
   if (rootPath[folderPathLen - 1] == FS_PATH_SEPARATOR_CHAR)
      return true;  // root already ended in separator — boundary covered by prefix compare
   TCHAR boundary = candidatePath[folderPathLen];
   return (boundary == 0) || (boundary == FS_PATH_SEPARATOR_CHAR);
}

/**
 * Make given path absolute and check that it is under one of the registered root folders.
 * If "modify" is true, the matching root must not be read-only.
 * Returns newly allocated absolute path on success, or nullptr if access is denied.
 */
TCHAR *FileAccessRootList::resolvePath(const TCHAR *path, bool modify) const
{
   nxlog_debug_tag(m_debugTag, 5, _T("FileAccessRootList::resolvePath: input is %s"), path);

#ifdef _WIN32
   TCHAR *fullPathBuffer = MemAllocString(MAX_PATH);
   TCHAR *fullPath = _tfullpath(fullPathBuffer, path, MAX_PATH);
#else
   TCHAR *fullPath = GetRealPath(path);
#endif
   nxlog_debug_tag(m_debugTag, 5, _T("FileAccessRootList::resolvePath: full path %s"), fullPath);
   if (fullPath == nullptr)
   {
#ifdef _WIN32
      MemFree(fullPathBuffer);
#endif
      return nullptr;
   }

   const FileAccessRoot *matchedRoot = nullptr;
   size_t maxPathLen = 0;
   for(int i = 0; i < m_roots.size(); i++)
   {
      const FileAccessRoot *root = m_roots.get(i);
      const TCHAR *rootPath = root->getFolder();
      size_t folderPathLen = _tcslen(rootPath);
      if (folderPathLen == 0)
         continue;
      if (!IsPathUnderRoot(rootPath, folderPathLen, fullPath))
         continue;

      if (maxPathLen < folderPathLen)
      {
         maxPathLen = folderPathLen;
         matchedRoot = root;
      }
   }

   if ((matchedRoot != nullptr) && (!modify || !matchedRoot->isReadOnly()))
   {
#ifndef _WIN32
      // For roots configured with "nofollow", resolve symbolic links and verify
      // the resolved path is still under the same root. This blocks a symlink
      // inside a writable area (placed by another process) from being used to
      // operate on files outside the configured root.
      if (!matchedRoot->followSymlinks())
      {
         TCHAR *resolved = ResolveSymlinks(fullPath);
         if ((resolved == nullptr) || !IsPathUnderRoot(matchedRoot->getFolder(), maxPathLen, resolved))
         {
            nxlog_debug_tag(m_debugTag, 5, _T("FileAccessRootList::resolvePath: symlink target outside root for %s (resolved to %s)"),
               fullPath, (resolved != nullptr) ? resolved : _T("(unresolved)"));
            MemFree(resolved);
            MemFree(fullPath);
            return nullptr;
         }
         MemFree(fullPath);
         fullPath = resolved;
      }
#endif
      return fullPath;
   }

   nxlog_debug_tag(m_debugTag, 5, _T("FileAccessRootList::resolvePath: access denied to %s"), fullPath);
   MemFree(fullPath);
   return nullptr;
}
