/*
 ** File management subagent
 ** Copyright (C) 2014-2025 Raden Solutions
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
 **/

#include "filemgr.h"
#include <netxms-version.h>

#ifdef _WIN32
#include <accctrl.h>
#include <aclapi.h>
#include <Lmcons.h>

bool SetPrivilege(HANDLE hToken, const TCHAR* privilege, bool enabled)
{
   LUID luid;
   if (!LookupPrivilegeValue(NULL, privilege, &luid))
   {
      TCHAR errorText[1024];
      nxlog_debug_tag(DEBUG_TAG, 4, _T("LookupPrivilegeValue error: %s"), GetSystemErrorText(GetLastError(), errorText, 1024));
      return false;
   }

   TOKEN_PRIVILEGES tp;
   tp.PrivilegeCount = 1;
   tp.Privileges[0].Luid = luid;
   if (enabled)
   {
      tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
   }
   else
   {
      tp.Privileges[0].Attributes = 0;
   }

   if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), (PTOKEN_PRIVILEGES)NULL, (PDWORD)NULL))
   {
      TCHAR errorText[1024];
      nxlog_debug_tag(DEBUG_TAG, 4, _T("AdjustTokenPrivileges error:  %s"), GetSystemErrorText(GetLastError(), errorText, 1024));
      return false;
   }

   if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("The token does not have the specified privilege."));
      return false;
   }

   return true;
}

#else
#include <pwd.h>
#include <grp.h>
#endif

/**
 * Root folders
 */
static ObjectArray<RootFolder> s_rootDirectories(16, 16, Ownership::True);
static SynchronizedHashMap<uint64_t, VolatileCounter> s_downloadFileStopMarkers(Ownership::False);

/**
 * Monitored file list
 */
MonitoredFileList g_monitorFileList;


#ifdef _WIN32

/**
 * Convert path from UNIX to local format and do macro expansion
 */
static inline void ConvertPathToHost(TCHAR *path, bool allowPathExpansion, bool allowShellCommands)
{
   for(int i = 0; path[i] != 0; i++)
      if (path[i] == _T('/'))
         path[i] = _T('\\');

   if (allowPathExpansion)
      ExpandFileName(path, path, MAX_PATH, allowShellCommands);
}

#else

/**
 * Convert path from UNIX to local format and do macro expansion
 */
static inline void ConvertPathToHost(TCHAR *path, bool allowPathExpansion, bool allowShellCommands)
{
   if (allowPathExpansion)
      ExpandFileName(path, path, MAX_PATH, allowShellCommands);
}

#endif

#ifdef _WIN32

/**
 * Convert path from local to UNIX format
 */
static void ConvertPathToNetwork(TCHAR *path)
{
   for(int i = 0; path[i] != 0; i++)
      if (path[i] == _T('\\'))
         path[i] = _T('/');
}

#else

/**
 * Convert path from local to UNIX format
 */
#define ConvertPathToNetwork(x)

#endif

/*
 * Create new RootFolder
 */
RootFolder::RootFolder(const TCHAR *folder)
{
   m_folder = MemCopyString(folder);
   m_readOnly = false;

   TCHAR *ptr = _tcschr(m_folder, _T(';'));
   if (ptr != nullptr)
   {
      *ptr = 0;
      if (_tcscmp(ptr + 1, _T("ro")) == 0)
         m_readOnly = true;
   }

   ConvertPathToHost(m_folder, false, false);
}

/**
 * Subagent initialization
 */
static bool SubagentInit(Config *config)
{
   ConfigEntry *root = config->getEntry(_T("/filemgr/RootFolder"));
   if (root != nullptr)
   {
      for(int i = 0; i < root->getValueCount(); i++)
      {
         auto folder = new RootFolder(root->getValue(i));

         bool alreadyRegistered = false;
         for(int j = 0; j < s_rootDirectories.size(); j++)
         {
            RootFolder *curr = s_rootDirectories.get(j);
#ifdef _WIN32
            if (!_tcsicmp(curr->getFolder(), folder->getFolder()))
#else
            if (!_tcscmp(curr->getFolder(), folder->getFolder()))
#endif
            {
               if (curr->isReadOnly() && !folder->isReadOnly())
                  s_rootDirectories.remove(j); // Replace read-only element with read-write
               else
                  alreadyRegistered = true;
               break;
            }
         }

         if (!alreadyRegistered)
         {
            s_rootDirectories.add(folder);
            nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Added file manager root directory \"%s\" (%s)"), folder->getFolder(), folder->isReadOnly() ? _T("R/O") : _T("R/W"));
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 5, _T("File manager root directory \"%s\" already registered"), folder->getFolder());
            delete folder;
         }
      }
   }

   if (s_rootDirectories.isEmpty())
   {
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("No root directories in file manager configuration"));
      return false;
   }

   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("File manager subagent initialized"));
   return true;
}

/**
 * Called by master agent at unload
 */
static void SubagentShutdown()
{
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

#endif

/**
 * Takes folder/file path - make it absolute (result will be written to "fullPath" parameter)
 * and check that this folder/file is under allowed root path.
 * If third parameter is set to true - then request is for getting content and "/" path should be accepted
 * and afterwards interpreted as "give list of all allowed folders".
 */
static bool CheckFullPath(const TCHAR *path, TCHAR **fullPath, bool withHomeDir, bool isModify = false)
{
   nxlog_debug_tag(DEBUG_TAG, 5, _T("CheckFullPath: input is %s"), path);
   if (withHomeDir && !_tcscmp(path, FS_PATH_SEPARATOR))
   {
      *fullPath = MemCopyString(path);
      return true;
   }

   *fullPath = nullptr;

#ifdef _WIN32
   TCHAR *fullPathBuffer = MemAllocString(MAX_PATH);
   TCHAR *fullPathT = _tfullpath(fullPathBuffer, path, MAX_PATH);
#else
   TCHAR *fullPathT = GetRealPath(path);
#endif
   nxlog_debug_tag(DEBUG_TAG, 5, _T("CheckFullPath: Full path %s"), fullPathT);
   if (fullPathT == nullptr)
   {
#ifdef _WIN32
      MemFree(fullPathBuffer);
#endif
      return false;
   }

   bool found = false;
   bool readOnly;
   size_t maxPathLen = 0;
   for(int i = 0; i < s_rootDirectories.size(); i++)
   {
      RootFolder *root = s_rootDirectories.get(i);
      size_t folderPathLen = _tcslen(root->getFolder());
#if defined(_WIN32) || defined(__APPLE__)
      if (!_tcsnicmp(root->getFolder(), fullPathT, folderPathLen))
#else
      if (!_tcsncmp(root->getFolder(), fullPathT, folderPathLen))
#endif
      {
         if (maxPathLen < folderPathLen)
         {
            maxPathLen = folderPathLen;
            readOnly = root->isReadOnly();
            found = true;
         }
      }
   }

   if (found)
   {
      if (!isModify || !readOnly)
      {
         *fullPath = fullPathT;
         return true;
      }
   }

   nxlog_debug_tag(DEBUG_TAG, 5, _T("CheckFullPath: Access denied to %s"), fullPathT);
   MemFree(fullPathT);
   return false;
}

#define REGULAR_FILE    1
#define DIRECTORY       2
#define SYMLINK         4

/**
 * Validate file change operation (upload, delete, etc.). Return strue if operation is allowed.
 */
static bool ValidateFileChangeOperation(const TCHAR *fileName, bool allowOverwrite, NXCPMessage *response)
{
   NX_STAT_STRUCT st;
   if (CALL_STAT(fileName, &st) == 0)
   {
      if (allowOverwrite)
         return true;
      response->setField(VID_RCC, S_ISDIR(st.st_mode) ? ERR_FOLDER_ALREADY_EXISTS : ERR_FILE_ALREADY_EXISTS);
      return false;
   }
   if (errno != ENOENT)
   {
      response->setField(VID_RCC, ERR_IO_FAILURE);
      return false;
   }
   return true;
}

#ifndef _WIN32

/**
 * File file owner fields in given NXCP message with data from given stat struct
 */
static void FillFileOwnerFields(NXCPMessage *msg, uint32_t fieldId, const NX_STAT_STRUCT& st)
{
#if HAVE_GETPWUID_R
   struct passwd *pw, pwbuf;
   char pwtxt[4096];
   getpwuid_r(st.st_uid, &pwbuf, pwtxt, 4096, &pw);
#else
   struct passwd *pw = getpwuid(st.st_uid);
#endif
#if HAVE_GETGRGID_R
   struct group *gr, grbuf;
   char grtxt[4096];
   getgrgid_r(st.st_gid, &grbuf, grtxt, 4096, &gr);
#else
   struct group *gr = getgrgid(st.st_gid);
#endif

   if (pw != nullptr)
   {
      msg->setFieldFromMBString(fieldId, pw->pw_name);
   }
   else
   {
      TCHAR id[32];
      _sntprintf(id, 32, _T("[%lu]"), (unsigned long)st.st_uid);
      msg->setField(fieldId, id);
   }
   if (gr != nullptr)
   {
      msg->setFieldFromMBString(fieldId + 1, gr->gr_name);
   }
   else
   {
      TCHAR id[32];
      _sntprintf(id, 32, _T("[%lu]"), (unsigned long)st.st_gid);
      msg->setField(fieldId + 1, id);
   }
}

#endif   /* _WIN32 */

static bool FillMessageFolderContent(const TCHAR *filePath, const TCHAR *fileName, NXCPMessage *msg, uint32_t varId)
{
   if (_taccess(filePath, R_OK) != 0)
      return false;

   NX_STAT_STRUCT st;
   if (CALL_STAT(filePath, &st) == 0)
   {
      msg->setField(varId++, fileName);
      msg->setField(varId++, static_cast<uint64_t>(st.st_size));
      msg->setField(varId++, static_cast<uint64_t>(st.st_mtime));
      uint32_t type = 0;
      TCHAR accessRights[11];
#ifndef _WIN32
      if (S_ISLNK(st.st_mode))
      {
         accessRights[0] = _T('l');
         type |= SYMLINK;
         NX_STAT_STRUCT symlinkStat;
         if (CALL_STAT_FOLLOW_SYMLINK(filePath, &symlinkStat) == 0)
         {
            type |= S_ISDIR(symlinkStat.st_mode) ? DIRECTORY : 0;
         }
      }

      if (S_ISCHR(st.st_mode))
         accessRights[0] = _T('c');
      if (S_ISBLK(st.st_mode))
         accessRights[0] = _T('b');
      if (S_ISFIFO(st.st_mode))
         accessRights[0] = _T('p');
      if (S_ISSOCK(st.st_mode))
         accessRights[0] = _T('s');
#endif
      if (S_ISREG(st.st_mode))
      {
         type |= REGULAR_FILE;
         accessRights[0] = _T('-');
      }
      if (S_ISDIR(st.st_mode))
      {
         type |= DIRECTORY;
         accessRights[0] = _T('d');
      }

      msg->setField(varId++, type);
      TCHAR fullName[MAX_PATH];
      _tcscpy(fullName, filePath);
      msg->setField(varId++, fullName);

#ifndef _WIN32
      FillFileOwnerFields(msg, varId, st);
      varId += 2;
      accessRights[1] = (S_IRUSR & st.st_mode) > 0 ? _T('r') : _T('-');
      accessRights[2] = (S_IWUSR & st.st_mode) > 0 ? _T('w') : _T('-');
      accessRights[3] = (S_IXUSR & st.st_mode) > 0 ? _T('x') : _T('-');
      accessRights[4] = (S_IRGRP & st.st_mode) > 0 ? _T('r') : _T('-');
      accessRights[5] = (S_IWGRP & st.st_mode) > 0 ? _T('w') : _T('-');
      accessRights[6] = (S_IXGRP & st.st_mode) > 0 ? _T('x') : _T('-');
      accessRights[7] = (S_IROTH & st.st_mode) > 0 ? _T('r') : _T('-');
      accessRights[8] = (S_IWOTH & st.st_mode) > 0 ? _T('w') : _T('-');
      accessRights[9] = (S_IXOTH & st.st_mode) > 0 ? _T('x') : _T('-');
      accessRights[10] = 0;
      msg->setField(varId++, accessRights);
#else	/* _WIN32 */
      TCHAR owner[1024];
      msg->setField(varId++, GetFileOwner(filePath, owner, 1024));
      msg->setField(varId++, _T(""));
      msg->setField(varId++, _T(""));
#endif // _WIN32
      return true;
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("GetFolderContent: cannot get folder %s"), filePath);
      return false;
   }
}

/**
 * Puts in response list of containing files
 */
static void GetFolderContent(TCHAR *folder, NXCPMessage *response, bool rootFolder, bool allowMultipart, AbstractCommSession *session)
{
   nxlog_debug_tag(DEBUG_TAG, 6, _T("GetFolderContent: reading \"%s\" (root=%s, multipart=%s)"),
      folder, rootFolder ? _T("true") : _T("false"), allowMultipart ? _T("true") : _T("false"));

   NXCPMessage *msg;
   if (allowMultipart)
   {
      msg = new NXCPMessage();
      msg->setCode(CMD_REQUEST_COMPLETED);
      msg->setId(response->getId());
      msg->setField(VID_ALLOW_MULTIPART, (INT16)1);
   }
   else
   {
      msg = response;
   }

   uint32_t count = 0;
   uint32_t fieldId = VID_INSTANCE_LIST_BASE;

   if (!_tcscmp(folder, FS_PATH_SEPARATOR) && rootFolder)
   {
      response->setField(VID_RCC, ERR_SUCCESS);

      for(int i = 0; i < s_rootDirectories.size(); i++)
      {
         const TCHAR *path = s_rootDirectories.get(i)->getFolder();
         if (FillMessageFolderContent(path, path, msg, fieldId))
         {
            count++;
            fieldId += 10;
         }
      }
      msg->setField(VID_INSTANCE_COUNT, count);
      if (allowMultipart)
      {
         msg->setEndOfSequence();
         msg->setField(VID_INSTANCE_COUNT, count);
         session->sendMessage(msg);
         delete msg;
      }
      nxlog_debug_tag(DEBUG_TAG, 6, _T("GetFolderContent: reading \"%s\" completed"), folder);
      return;
   }

   _TDIR *dir = _topendir(folder);
   if (dir != NULL)
   {
      response->setField(VID_RCC, ERR_SUCCESS);

      struct _tdirent *d;
      while((d = _treaddir(dir)) != NULL)
      {
         if (!_tcscmp(d->d_name, _T(".")) || !_tcscmp(d->d_name, _T("..")))
            continue;

         TCHAR fullName[MAX_PATH];
         _tcscpy(fullName, folder);
         _tcscat(fullName, FS_PATH_SEPARATOR);
         _tcscat(fullName, d->d_name);

         if (FillMessageFolderContent(fullName, d->d_name, msg, fieldId))
         {
            count++;
            fieldId += 10;
         }
         if (allowMultipart && (count == 64))
         {
            msg->setField(VID_INSTANCE_COUNT, count);
            session->sendMessage(msg);
            msg->deleteAllFields();
            msg->setField(VID_ALLOW_MULTIPART, (INT16)1);
            count = 0;
            fieldId = VID_INSTANCE_LIST_BASE;
         }
      }
      msg->setField(VID_INSTANCE_COUNT, count);
      _tclosedir(dir);

      if (allowMultipart)
      {
         msg->setEndOfSequence();
         msg->setField(VID_INSTANCE_COUNT, count);
         session->sendMessage(msg);
      }
   }
   else
   {
      response->setField(VID_RCC, ERR_IO_FAILURE);
   }

   if (allowMultipart)
      delete msg;

   nxlog_debug_tag(DEBUG_TAG, 6, _T("GetFolderContent: reading \"%s\" completed"), folder);
}

/**
 * Delete file/folder
 */
static bool Delete(const TCHAR *name)
{
   NX_STAT_STRUCT st;
   if (CALL_STAT(name, &st) != 0)
      return false;

   if (!S_ISDIR(st.st_mode))
      return _tremove(name) == 0;

   // get element list and for each element run Delete
   bool result = true;
   _TDIR *dir = _topendir(name);
   if (dir != nullptr)
   {
      struct _tdirent *d;
      while((d = _treaddir(dir)) != nullptr)
      {
         if (!_tcscmp(d->d_name, _T(".")) || !_tcscmp(d->d_name, _T("..")))
            continue;

         TCHAR newName[MAX_PATH];
         _tcscpy(newName, name);
         _tcslcat(newName, FS_PATH_SEPARATOR, MAX_PATH);
         _tcslcat(newName, d->d_name, MAX_PATH);
         result = result && Delete(newName);
      }
      _tclosedir(dir);
   }
   return _trmdir(name) == 0;
}

/**
 * Get folder information
 */
static void GetFolderInfo(const TCHAR *folder, uint64_t *fileCount, uint64_t *folderSize)
{
   _TDIR *dir = _topendir(folder);
   if (dir == nullptr)
      return;

   struct _tdirent *d;
   while((d = _treaddir(dir)) != nullptr)
   {
      if (!_tcscmp(d->d_name, _T(".")) || !_tcscmp(d->d_name, _T("..")))
         continue;

      TCHAR fullName[MAX_PATH];
      _tcslcpy(fullName, folder, MAX_PATH);
      _tcslcat(fullName, FS_PATH_SEPARATOR, MAX_PATH);
      _tcslcat(fullName, d->d_name, MAX_PATH);

      NX_STAT_STRUCT st;
      if (CALL_STAT(fullName, &st) == 0)
      {
         if (S_ISDIR(st.st_mode))
         {
            GetFolderInfo(fullName, fileCount, folderSize);
         }
         else
         {
            *folderSize += st.st_size;
            (*fileCount)++;
         }
      }
   }
   _tclosedir(dir);
}

/**
 * Handler for "get folder size" command
 */
static void CH_GetFolderSize(NXCPMessage *request, NXCPMessage *response, AbstractCommSession *session)
{
   TCHAR directory[MAX_PATH];
   request->getFieldAsString(VID_FILE_NAME, directory, MAX_PATH);
   if (directory[0] == 0)
   {
      response->setField(VID_RCC, ERR_IO_FAILURE);
      nxlog_debug_tag(DEBUG_TAG, 5, _T("CH_GetFolderSize: File name is not set"));
      return;
   }

   ConvertPathToHost(directory, request->getFieldAsBoolean(VID_ALLOW_PATH_EXPANSION), session->isMasterServer());

   TCHAR *fullPath;
   if (CheckFullPath(directory, &fullPath, false))
   {
      uint64_t fileCount = 0, fileSize = 0;
      GetFolderInfo(fullPath, &fileCount, &fileSize);
      response->setField(VID_RCC, ERR_SUCCESS);
      response->setField(VID_FOLDER_SIZE, fileSize);
      response->setField(VID_FILE_COUNT, fileCount);
      MemFree(fullPath);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("CH_GetFolderSize: CheckFullPath failed"));
      response->setField(VID_RCC, ERR_ACCESS_DENIED);
   }
}

/**
 * Handler for "get folder content" command
 */
static void CH_GetFolderContent(NXCPMessage *request, NXCPMessage *response, AbstractCommSession *session)
{
   TCHAR directory[MAX_PATH] = _T("");
   request->getFieldAsString(VID_FILE_NAME, directory, MAX_PATH);
   if (directory[0] == 0)
   {
      response->setField(VID_RCC, ERR_IO_FAILURE);
      nxlog_debug_tag(DEBUG_TAG, 5, _T("CH_GetFolderContent: File name is not set"));
      return;
   }

   ConvertPathToHost(directory, request->getFieldAsBoolean(VID_ALLOW_PATH_EXPANSION), session->isMasterServer());

   bool rootFolder = request->getFieldAsUInt16(VID_ROOT) ? 1 : 0;
   TCHAR *fullPath;
   if (CheckFullPath(directory, &fullPath, rootFolder))
   {
      GetFolderContent(fullPath, response, rootFolder, request->getFieldAsBoolean(VID_ALLOW_MULTIPART), session);
      MemFree(fullPath);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("CH_GetFolderContent: CheckFullPath failed"));
      response->setField(VID_RCC, ERR_ACCESS_DENIED);
   }
}

/**
 * Handler for "create folder" command
 */
static void CH_CreateFolder(NXCPMessage *request, NXCPMessage *response, AbstractCommSession *session)
{
   TCHAR directory[MAX_PATH];
   request->getFieldAsString(VID_FILE_NAME, directory, MAX_PATH);
   if (directory[0] == 0)
   {
      response->setField(VID_RCC, ERR_IO_FAILURE);
      nxlog_debug_tag(DEBUG_TAG, 5, _T("CH_CreateFolder: File name is not set"));
      return;
   }

   ConvertPathToHost(directory, request->getFieldAsBoolean(VID_ALLOW_PATH_EXPANSION), session->isMasterServer());

   TCHAR *fullPath = NULL;
   if (CheckFullPath(directory, &fullPath, false, true) && session->isMasterServer())
   {
      if (ValidateFileChangeOperation(fullPath, false, response))
      {
         if (CreateDirectoryTree(fullPath))
         {
            response->setField(VID_RCC, ERR_SUCCESS);
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 5, _T("CH_CreateFolder: Could not create directory \"%s\""), fullPath);
            response->setField(VID_RCC, ERR_IO_FAILURE);
         }
      }
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("CH_CreateFolder: CheckFullPath failed"));
      response->setField(VID_RCC, ERR_ACCESS_DENIED);
   }
   MemFree(fullPath);
}

/**
 * Handler for "delete file" command
 */
static void CH_DeleteFile(NXCPMessage *request, NXCPMessage *response, AbstractCommSession *session)
{
   TCHAR file[MAX_PATH];
   request->getFieldAsString(VID_FILE_NAME, file, MAX_PATH);
   if (file[0] == 0)
   {
      response->setField(VID_RCC, ERR_IO_FAILURE);
      nxlog_debug_tag(DEBUG_TAG, 5, _T("CH_DeleteFile: File name is not set"));
      return;
   }

   ConvertPathToHost(file, request->getFieldAsBoolean(VID_ALLOW_PATH_EXPANSION), session->isMasterServer());

   TCHAR *fullPath = NULL;
   if (CheckFullPath(file, &fullPath, false, true) && session->isMasterServer())
   {
      if (Delete(fullPath))
      {
         response->setField(VID_RCC, ERR_SUCCESS);
      }
      else
      {
         response->setField(VID_RCC, ERR_IO_FAILURE);
         nxlog_debug_tag(DEBUG_TAG, 5, _T("CH_DeleteFile: Delete failed on \"%s\""), fullPath);
      }
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("CH_DeleteFile: CheckFullPath failed on \"%s\""), file);
      response->setField(VID_RCC, ERR_ACCESS_DENIED);
   }
   MemFree(fullPath);
}

/**
 * Handler for "rename file" command
 */
static void CH_RenameFile(NXCPMessage *request, NXCPMessage *response, AbstractCommSession *session)
{
   TCHAR oldName[MAX_PATH];
   request->getFieldAsString(VID_FILE_NAME, oldName, MAX_PATH);
   TCHAR newName[MAX_PATH];
   request->getFieldAsString(VID_NEW_FILE_NAME, newName, MAX_PATH);
   bool allowOverwirite = request->getFieldAsBoolean(VID_OVERWRITE);

   if (oldName[0] == 0 && newName[0] == 0)
   {
      response->setField(VID_RCC, ERR_IO_FAILURE);
      nxlog_debug_tag(DEBUG_TAG, 5, _T("CH_RenameFile: File names is not set"));
      return;
   }

   bool allowPathExpansion = request->getFieldAsBoolean(VID_ALLOW_PATH_EXPANSION);
   ConvertPathToHost(oldName, allowPathExpansion, session->isMasterServer());
   ConvertPathToHost(newName, allowPathExpansion, session->isMasterServer());

   TCHAR *fullPathOld = NULL, *fullPathNew = NULL;
   if (CheckFullPath(oldName, &fullPathOld, false, true) && CheckFullPath(newName, &fullPathNew, false) && session->isMasterServer())
   {
      if (ValidateFileChangeOperation(fullPathNew, allowOverwirite, response))
      {
         if (_trename(fullPathOld, fullPathNew) == 0)
         {
            response->setField(VID_RCC, ERR_SUCCESS);
         }
         else
         {
            response->setField(VID_RCC, ERR_IO_FAILURE);
         }
      }
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("CH_RenameFile: CheckFullPath failed"));
      response->setField(VID_RCC, ERR_ACCESS_DENIED);
   }
   MemFree(fullPathOld);
   MemFree(fullPathNew);
}

/**
 * Handler for "move file" command
 */
static void CH_MoveFile(NXCPMessage *request, NXCPMessage *response, AbstractCommSession *session)
{
   TCHAR oldName[MAX_PATH];
   request->getFieldAsString(VID_FILE_NAME, oldName, MAX_PATH);
   TCHAR newName[MAX_PATH];
   request->getFieldAsString(VID_NEW_FILE_NAME, newName, MAX_PATH);
   bool allowOverwirite = request->getFieldAsBoolean(VID_OVERWRITE);
   if ((oldName[0] == 0) && (newName[0] == 0))
   {
      response->setField(VID_RCC, ERR_IO_FAILURE);
      nxlog_debug_tag(DEBUG_TAG, 5, _T("CH_MoveFile: File names are not set"));
      return;
   }

   bool allowPathExpansion = request->getFieldAsBoolean(VID_ALLOW_PATH_EXPANSION);
   ConvertPathToHost(oldName, allowPathExpansion, session->isMasterServer());
   ConvertPathToHost(newName, allowPathExpansion, session->isMasterServer());

   TCHAR *fullPathOld = NULL, *fullPathNew = NULL;
   if (CheckFullPath(oldName, &fullPathOld, false, true) && CheckFullPath(newName, &fullPathNew, false) && session->isMasterServer())
   {
      if (ValidateFileChangeOperation(fullPathNew, allowOverwirite, response))
      {
         if (MoveFileOrDirectory(fullPathOld, fullPathNew))
         {
            response->setField(VID_RCC, ERR_SUCCESS);
         }
         else
         {
            response->setField(VID_RCC, ERR_IO_FAILURE);
         }
      }
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("CH_MoveFile: CheckFullPath failed"));
      response->setField(VID_RCC, ERR_ACCESS_DENIED);
   }
   MemFree(fullPathOld);
   MemFree(fullPathNew);
}

/**
 * Handler for "copy file" command
 */
static void CH_CopyFile(NXCPMessage *request, NXCPMessage *response, AbstractCommSession *session)
{
   TCHAR oldName[MAX_PATH];
   request->getFieldAsString(VID_FILE_NAME, oldName, MAX_PATH);
   TCHAR newName[MAX_PATH];
   request->getFieldAsString(VID_NEW_FILE_NAME, newName, MAX_PATH);
   bool allowOverwrite = request->getFieldAsBoolean(VID_OVERWRITE);
   response->setField(VID_RCC, ERR_SUCCESS);

   if ((oldName[0] == 0) && (newName[0] == 0))
   {
      response->setField(VID_RCC, ERR_IO_FAILURE);
      nxlog_debug_tag(DEBUG_TAG, 5, _T("CH_CopyFile: File names are not set"));
      return;
   }

   bool allowPathExpansion = request->getFieldAsBoolean(VID_ALLOW_PATH_EXPANSION);
   ConvertPathToHost(oldName, allowPathExpansion, session->isMasterServer());
   ConvertPathToHost(newName, allowPathExpansion, session->isMasterServer());

   TCHAR *fullPathOld = NULL, *fullPathNew = NULL;
   if (CheckFullPath(oldName, &fullPathOld, false, true) && CheckFullPath(newName, &fullPathNew, false) && session->isMasterServer())
   {
      if (ValidateFileChangeOperation(fullPathNew, allowOverwrite, response))
      {
         if (!CopyFileOrDirectory(fullPathOld, fullPathNew))
            response->setField(VID_RCC, ERR_IO_FAILURE);
      }
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("CH_CopyFile: CheckFullPath failed"));
      response->setField(VID_RCC, ERR_ACCESS_DENIED);
   }
   MemFree(fullPathOld);
   MemFree(fullPathNew);
}

/**
 * Get path to file without file name
 * Will return null if there is only file name
 */
static TCHAR *GetPathToFile(const TCHAR *fullPath)
{
   TCHAR *pathToFile = MemCopyString(fullPath);
   TCHAR *ptr = _tcsrchr(pathToFile, FS_PATH_SEPARATOR_CHAR);
   if (ptr != nullptr)
   {
      *ptr = 0;
   }
   else
   {
      MemFree(pathToFile);
      pathToFile = nullptr;
   }
   return pathToFile;
}

/**
 * Handler for "upload" command
 */
static void CH_Upload(NXCPMessage *request, NXCPMessage *response, AbstractCommSession *session)
{
   TCHAR name[MAX_PATH];
   request->getFieldAsString(VID_FILE_NAME, name, MAX_PATH);
   if (name[0] == 0)
   {
      response->setField(VID_RCC, ERR_IO_FAILURE);
      nxlog_debug_tag(DEBUG_TAG, 5, _T("CH_Upload: File name is not set"));
      return;
   }

   ConvertPathToHost(name, request->getFieldAsBoolean(VID_ALLOW_PATH_EXPANSION), session->isMasterServer());

   TCHAR *fullPath = nullptr;
   if (CheckFullPath(name, &fullPath, false, true) && session->isMasterServer())
   {
      TCHAR *pathToFile = GetPathToFile(fullPath);
      if (pathToFile != nullptr)
      {
         CreateDirectoryTree(pathToFile);
         MemFree(pathToFile);
      }

      bool allowOverwirite = request->getFieldAsBoolean(VID_OVERWRITE);
      if (ValidateFileChangeOperation(fullPath, allowOverwirite, response))
         session->openFile(response, fullPath, request->getId(), request->getFieldAsTime(VID_MODIFICATION_TIME), static_cast<FileTransferResumeMode>(request->getFieldAsUInt16(VID_RESUME_MODE)));
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("CH_Upload: CheckFullPath failed"));
      response->setField(VID_RCC, ERR_ACCESS_DENIED);
   }
   MemFree(fullPath);
}

/**
 * Handler for "get file details" command
 */
static void CH_GetFileDetails(NXCPMessage *request, NXCPMessage *response, AbstractCommSession *session)
{
   TCHAR fileName[MAX_PATH];
   request->getFieldAsString(VID_FILE_NAME, fileName, MAX_PATH);
   ConvertPathToHost(fileName, request->getFieldAsBoolean(VID_ALLOW_PATH_EXPANSION), session->isMasterServer());

   TCHAR *fullPath;
   if (CheckFullPath(fileName, &fullPath, false))
   {
      NX_STAT_STRUCT fs;
      if (CALL_STAT(fullPath, &fs) == 0)
      {
         response->setField(VID_FILE_SIZE, static_cast<uint64_t>(fs.st_size));
         response->setField(VID_MODIFICATION_TIME, static_cast<uint64_t>(fs.st_mtime));
         response->setField(VID_RCC, ERR_SUCCESS);
      }
      else
      {
         response->setField(VID_RCC, ERR_FILE_STAT_FAILED);
      }
      MemFree(fullPath);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("CH_GetFileDetails: CheckFullPath failed"));
      response->setField(VID_RCC, ERR_ACCESS_DENIED);
   }
}

#ifdef WIN32
#define mode_t unsigned short
#endif

/**
 * Convert file permissions into internal format
 */
static uint16_t ConvertFilePermissions(mode_t mode)
{
#ifdef _WIN32
   return 0;   // FIXME: how to return correct permissions?
#else
   uint16_t accessRights = 0;
   if (S_IRUSR & mode)
      accessRights |= (1 << 0);
   if (S_IWUSR & mode)
      accessRights |= (1 << 1);
   if (S_IXUSR & mode)
      accessRights |= (1 << 2);
   if (S_IRGRP & mode)
      accessRights |= (1 << 3);
   if (S_IWGRP & mode)
      accessRights |= (1 << 4);
   if (S_IXGRP & mode)
      accessRights |= (1 << 5);
   if (S_IROTH & mode)
      accessRights |= (1 << 6);
   if (S_IWOTH & mode)
      accessRights |= (1 << 7);
   if (S_IXOTH & mode)
      accessRights |= (1 << 8);
   return accessRights;
#endif
}

/**
 * Handler for "get file set details" command
 */
static void CH_GetFileSetDetails(const NXCPMessage& request, NXCPMessage *response, AbstractCommSession *session)
{
   bool allowPathExpansion = request.getFieldAsBoolean(VID_ALLOW_PATH_EXPANSION);
   StringList files(request, VID_ELEMENT_LIST_BASE, VID_NUM_ELEMENTS);
   uint32_t fieldId = VID_ELEMENT_LIST_BASE;

   for(int i = 0; i < files.size(); i++)
   {
      TCHAR fileName[MAX_PATH];
      _tcslcpy(fileName, files.get(i), MAX_PATH);
      ConvertPathToHost(fileName, allowPathExpansion, session->isMasterServer());

      TCHAR *fullPath;
      if (CheckFullPath(fileName, &fullPath, false))
      {
         NX_STAT_STRUCT fs;
         if (CALL_STAT_FOLLOW_SYMLINK(fullPath, &fs) == 0)
         {
            response->setField(fieldId++, ERR_SUCCESS);
            response->setField(fieldId++, static_cast<uint64_t>(fs.st_size));
            response->setField(fieldId++, static_cast<uint64_t>(fs.st_mtime));
            BYTE hash[MD5_DIGEST_SIZE];
            if (!CalculateFileMD5Hash(fullPath, hash))
               memset(hash, 0, MD5_DIGEST_SIZE);
            response->setField(fieldId++, hash, MD5_DIGEST_SIZE);
            response->setField(fieldId++, ConvertFilePermissions(fs.st_mode));
#ifdef _WIN32
            TCHAR owner[1024];
            response->setField(fieldId++, GetFileOwner(fileName, owner, 1024));
            response->setField(fieldId++, _T("")); // Do not set group on Windows
#else
            FillFileOwnerFields(response, fieldId, fs);
            fieldId += 2;
#endif
            fieldId += 3;
         }
         else
         {
            response->setField(fieldId++, ERR_FILE_STAT_FAILED);
            fieldId += 9;
         }
         MemFree(fullPath);
      }
      else
      {
         response->setField(fieldId++, ERR_ACCESS_DENIED);
         fieldId += 9;
      }
   }
   response->setField(VID_NUM_ELEMENTS, files.size());
   response->setField(VID_RCC, ERR_SUCCESS);
}

/**
 * Data for file sending thread
 */
struct FileSendData
{
   TCHAR *fileName;
   TCHAR *fileId;
   bool follow;
   NXCPStreamCompressionMethod compressionMethod;
   uint64_t id;
   off64_t offset;
   shared_ptr<AbstractCommSession> session;
   VolatileCounter stopMarker;

   FileSendData(const shared_ptr<AbstractCommSession>& _session, TCHAR *_fileName, const NXCPMessage& request) : session(_session)
   {
      fileName = _fileName;
      fileId = request.getFieldAsString(VID_NAME);
      follow = request.getFieldAsBoolean(VID_FILE_FOLLOW);
      if (request.isFieldExist(VID_COMPRESSION_METHOD))
      {
         compressionMethod = static_cast<NXCPStreamCompressionMethod>(request.getFieldAsInt16(VID_COMPRESSION_METHOD));
      }
      else // Servers before 4.2 will not send VID_COMPRESSION_METHOD
      {
         bool allowCompression = request.getFieldAsBoolean(VID_ENABLE_COMPRESSION);
         compressionMethod = allowCompression ? NXCP_STREAM_COMPRESSION_DEFLATE : NXCP_STREAM_COMPRESSION_NONE;
      }
      id = (static_cast<uint64_t>(session->getId()) << 32) | static_cast<uint64_t>(request.getId());
      offset = request.getFieldAsInt32(VID_FILE_OFFSET);
      stopMarker = 0;
   }

   ~FileSendData()
   {
      MemFree(fileName);
      MemFree(fileId);
   }
};

/**
 * Send file
 */
static void SendFile(FileSendData *data)
{
   const TCHAR *compressionMethodName;
   switch(data->compressionMethod)
   {
      case NXCP_STREAM_COMPRESSION_DEFLATE:
         compressionMethodName = _T("DEFLATE");
         break;
      case NXCP_STREAM_COMPRESSION_LZ4:
         compressionMethodName = _T("LZ4");
         break;
      case NXCP_STREAM_COMPRESSION_NONE:
         compressionMethodName = _T("NONE");
         break;
      default:
         compressionMethodName = _T("UNKNOWN");
         break;
   }
   nxlog_debug_tag(DEBUG_TAG, 5, _T("SendFile: request for file \"%s\", follow = %s, compression = %s"),
               data->fileName, data->follow ? _T("true") : _T("false"), compressionMethodName);
   bool success = data->session->sendFile(static_cast<uint32_t>(data->id & _LL(0xFFFFFFFF)), data->fileName, data->offset, data->compressionMethod, &data->stopMarker);
   if (data->follow && success)
   {
      g_monitorFileList.add(data->fileId);
      auto flData = new FollowData(data->fileName, data->fileId, 0, data->session->getServerAddress());
      ThreadCreateEx(SendFileUpdatesOverNXCP, 0, flData);
   }
   s_downloadFileStopMarkers.remove(data->id);
   delete data;
}

/**
 * Handler for "get file" command
 */
static void CH_GetFile(const NXCPMessage& request, NXCPMessage *response, AbstractCommSession *session)
{
   if (request.getFieldAsBoolean(VID_FILE_FOLLOW) && !session->isMasterServer())
   {
      response->setField(VID_RCC, ERR_ACCESS_DENIED);
      return;
   }

   TCHAR fileName[MAX_PATH];
   request.getFieldAsString(VID_FILE_NAME, fileName, MAX_PATH);
   ConvertPathToHost(fileName, request.getFieldAsBoolean(VID_ALLOW_PATH_EXPANSION), session->isMasterServer());

   TCHAR *fullPath;
   if (CheckFullPath(fileName, &fullPath, false))
   {
      auto fd = new FileSendData(session->self(), fullPath, request);
      s_downloadFileStopMarkers.set(fd->id, &fd->stopMarker);
      ThreadCreateEx(SendFile, fd);
      response->setField(VID_RCC, ERR_SUCCESS);
   }
   else
   {
      response->setField(VID_RCC, ERR_ACCESS_DENIED);
   }
}

/**
 * Handler for "change file accessRights" command.
 */
static void CH_ChangeFilePermissions(NXCPMessage *request, NXCPMessage *response, AbstractCommSession *session)
{
   if (!session->isMasterServer())
   {
      response->setField(VID_RCC, ERR_ACCESS_DENIED);
      return;
   }

   TCHAR fileName[MAX_PATH];
   request->getFieldAsString(VID_FILE_NAME, fileName, MAX_PATH);
   ConvertPathToHost(fileName, request->getFieldAsBoolean(VID_ALLOW_PATH_EXPANSION), session->isMasterServer());

   TCHAR *fullPath;
   if (CheckFullPath(fileName, &fullPath, false))
   {
      uint16_t accessRights = request->getFieldAsUInt16(VID_FILE_PERMISSIONS);
      if (accessRights != 0)
      {
#if defined(_WIN32)
         PACL pOldDACL = nullptr;
         PSECURITY_DESCRIPTOR pSD = nullptr;
         GetNamedSecurityInfo(fullPath, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, nullptr, nullptr, &pOldDACL, nullptr, &pSD);

         uint32_t accessCount = 1;
         TCHAR username[UNLEN + 1] = {};
         request->getFieldAsString(VID_USER_NAME, username, UNLEN+1);
         if (username[0] != 0)
         {
            accessCount += 1;
         }

         TCHAR group[GNLEN + 1] = {};
         request->getFieldAsString(VID_GROUP_NAME, group, GNLEN + 1);
         if (group[0] != 0)
         {
            accessCount += 1;
         }

         EXPLICIT_ACCESS* ea = (EXPLICIT_ACCESS*)MemAlloc(accessCount * sizeof(EXPLICIT_ACCESS));
         memset(ea, 0, accessCount * sizeof(EXPLICIT_ACCESS));

         //Add User
         int counter = 0;
         DWORD UserAccessPermissions = 0;
         if (accessRights & 0x0001)
            UserAccessPermissions |= FILE_GENERIC_READ;
         if (accessRights & 0x0002)
            UserAccessPermissions |= FILE_GENERIC_WRITE;
         if (accessRights & 0x0004)
            UserAccessPermissions |= FILE_GENERIC_EXECUTE;
         if (username[0] && UserAccessPermissions)
         {
            ea[counter].grfAccessPermissions = UserAccessPermissions;
            ea[counter].grfAccessMode = SET_ACCESS;
            ea[counter].grfInheritance = NO_INHERITANCE;
            ea[counter].Trustee.TrusteeForm = TRUSTEE_IS_NAME;
            ea[counter].Trustee.TrusteeType = TRUSTEE_IS_USER;
            ea[counter].Trustee.ptstrName = username;
            counter++;
         }

         // Add Group
         DWORD GroupAccessPermissions = 0;
         if (accessRights & 0x0008)
            GroupAccessPermissions |= FILE_GENERIC_READ;
         if (accessRights & 0x0010)
            GroupAccessPermissions |= FILE_GENERIC_WRITE;
         if (accessRights & 0x0020)
            GroupAccessPermissions |= FILE_GENERIC_EXECUTE;
         if (group[0] && GroupAccessPermissions)
         {
            ea[counter].grfAccessPermissions = GroupAccessPermissions;
            ea[counter].grfAccessMode = SET_ACCESS;
            ea[counter].grfInheritance = NO_INHERITANCE;
            ea[counter].Trustee.TrusteeForm = TRUSTEE_IS_NAME;
            ea[counter].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
            ea[counter].Trustee.ptstrName = group;
            counter++;
         }

         //Add Everyone
         DWORD EveryoneAccessPermissions = 0;
         if (accessRights & 0x0040)
            EveryoneAccessPermissions |= FILE_GENERIC_READ;
         if (accessRights & 0x0080)
            EveryoneAccessPermissions |= FILE_GENERIC_WRITE;
         if (accessRights & 0x0100)
            EveryoneAccessPermissions |= FILE_GENERIC_EXECUTE;
         PSID pEveryoneSID = nullptr;
         if (EveryoneAccessPermissions)
         {
            SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
            AllocateAndInitializeSid(&SIDAuthWorld, 1,
               SECURITY_WORLD_RID,
               0, 0, 0, 0, 0, 0, 0,
               &pEveryoneSID);
            ea[counter].grfAccessPermissions = EveryoneAccessPermissions;
            ea[counter].grfAccessMode = SET_ACCESS;
            ea[counter].grfInheritance = NO_INHERITANCE;
            ea[counter].Trustee.TrusteeForm = TRUSTEE_IS_SID;
            ea[counter].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
            ea[counter].Trustee.ptstrName = (LPTSTR)pEveryoneSID;
            counter++;
         }

         bool success = false;
         PACL pACL = nullptr;
         if (counter)
         {
            success = SetEntriesInAcl(counter, ea, pOldDACL, &pACL) == ERROR_SUCCESS;
         }

         if (success && counter)
         {
            success = SetNamedSecurityInfo(fullPath, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, nullptr, nullptr, pACL, nullptr);
         }

         if (pEveryoneSID != nullptr)
         {
            FreeSid(pEveryoneSID);
         }
         LocalFree(pACL);
         MemFree(ea);

         if (success)
         {
            response->setField(VID_RCC, ERR_SUCCESS);
         }
         else
         {
            response->setField(VID_RCC, ERR_IO_FAILURE);
         }
#else
         mode_t mode = 0;
         if (accessRights & (1 << 0))
            mode |= S_IRUSR;
         if (accessRights & (1 << 1))
            mode |= S_IWUSR;
         if (accessRights & (1 << 2))
            mode |= S_IXUSR;
         if (accessRights & (1 << 3))
            mode |= S_IRGRP;
         if (accessRights & (1 << 4))
            mode |= S_IWGRP;
         if (accessRights & (1 << 5))
            mode |= S_IXGRP;
         if (accessRights & (1 << 6))
            mode |= S_IROTH;
         if (accessRights & (1 << 7))
            mode |= S_IWOTH;
         if (accessRights & (1 << 8))
            mode |= S_IXOTH;

         bool success = (_tchmod(fullPath, mode) == 0);
         nxlog_debug_tag(DEBUG_TAG, 6, _T("chmod(\"%s\", %o): %s"), fullPath, static_cast<int>(mode), _tcserror(errno));
         if (success)
         {
            response->setField(VID_RCC, ERR_SUCCESS);
         }
         else
         {
            response->setField(VID_RCC, ((errno == EPERM) || (errno == EACCES)) ? ERR_ACCESS_DENIED : ERR_IO_FAILURE);
         }
#endif
      }
      else
      {
         response->setField(VID_RCC, ERR_BAD_ARGUMENTS);
      }
      MemFree(fullPath);
   }
   else
   {
      response->setField(VID_RCC, ERR_ACCESS_DENIED);
   }
}

/**
 * Handler for "change file owner" command.
 */
static void CH_ChangeFileOwner(NXCPMessage *request, NXCPMessage *response, AbstractCommSession *session)
{
   if (!session->isMasterServer())
   {
      response->setField(VID_RCC, ERR_ACCESS_DENIED);
      return;
   }

   TCHAR fileName[MAX_PATH];
   request->getFieldAsString(VID_FILE_NAME, fileName, MAX_PATH);
   ConvertPathToHost(fileName, request->getFieldAsBoolean(VID_ALLOW_PATH_EXPANSION), session->isMasterServer());

   TCHAR *fullPath;
   if (CheckFullPath(fileName, &fullPath, false))
   {
#if defined(_WIN32)
      response->setField(VID_RCC, ERR_SUCCESS);
#else // POSIX
      char* userName = request->getFieldAsMBString(VID_USER_NAME);
      char* groupName = request->getFieldAsMBString(VID_GROUP_NAME);
      uid_t newOwner = -1;
      gid_t newGroup = -1;
      if (userName != nullptr)
      {
#if HAVE_GETPWUID_R
         struct passwd *pw, pwbuf;
         char pwtxt[4096];
         getpwnam_r(userName, &pwbuf, pwtxt, 4096, &pw);
#else
         struct passwd *pw = getpwnam(userName);
#endif
         MemFree(userName);
         newOwner = (pw != nullptr) ? pw->pw_uid : -1;
      }

      if (groupName != nullptr)
      {
#if HAVE_GETGRGID_R
         struct group *gr, grbuf;
         char grtxt[4096];
         getgrnam_r(groupName, &grbuf, grtxt, 4096, &gr);
#else
         struct group *gr = getgrnam(groupName);
#endif
         MemFree(groupName);
         newGroup = (gr != nullptr) ? gr->gr_gid : -1;
      }

      if ((newOwner != -1) || (newGroup != -1))
      {
         bool success = false;
#ifdef UNICODE
         char *fullPathStr = MBStringFromWideString(fullPath);
         success = (chown(fullPathStr, newOwner, newGroup) == 0);
         MemFree(fullPathStr);
#else
         success = (chown(fullPath, newOwner, newGroup) == 0);
#endif
         nxlog_debug_tag(DEBUG_TAG, 6, _T("chown(\"%s\", %d, %d): %s"), fullPath, static_cast<int>(newOwner), static_cast<int>(newGroup), _tcserror(errno));
         if (success)
         {
            response->setField(VID_RCC, ERR_SUCCESS);
         }
         else
         {
            response->setField(VID_RCC, ((errno == EPERM) || (errno == EACCES)) ? ERR_ACCESS_DENIED : ERR_IO_FAILURE);
         }
      }
      else
      {
         response->setField(VID_RCC, ERR_BAD_ARGUMENTS);
      }
#endif // POSIX
      MemFree(fullPath);
   }
   else
   {
      response->setField(VID_RCC, ERR_ACCESS_DENIED);
   }
}

/**
 * Handler for "cancel file download" command
 */
static void CH_CancelFileDownload(NXCPMessage *request, NXCPMessage *response, AbstractCommSession *session)
{
   uint64_t requestId = request->getFieldAsUInt32(VID_REQUEST_ID);
   VolatileCounter *counter = s_downloadFileStopMarkers.get((static_cast<uint64_t>(session->getId()) << 32) | requestId);
   if (counter != nullptr)
   {
      InterlockedIncrement(counter);
      response->setField(VID_RCC, ERR_SUCCESS);
   }
   else
   {
      response->setField(VID_RCC, ERR_BAD_ARGUMENTS);
   }
}

/**
 * Handler for "cancel file monitoring" command
 */
static void CH_CancelFileMonitoring(NXCPMessage *request, NXCPMessage *response)
{
   TCHAR fileName[MAX_PATH];
   request->getFieldAsString(VID_FILE_NAME, fileName, MAX_PATH);
   if (g_monitorFileList.remove(fileName))
   {
      response->setField(VID_RCC, ERR_SUCCESS);
   }
   else
   {
      response->setField(VID_RCC, ERR_BAD_ARGUMENTS);
   }
}

/**
 * Handler for "get file fingerprint" command
 */
static void CH_GetFileFingerprint(NXCPMessage *request, NXCPMessage *response, AbstractCommSession *session)
{
   TCHAR fileName[MAX_PATH];
   request->getFieldAsString(VID_FILE_NAME, fileName, MAX_PATH);
   ConvertPathToHost(fileName, request->getFieldAsBoolean(VID_ALLOW_PATH_EXPANSION), session->isMasterServer());

   TCHAR *fullPath;
   if (CheckFullPath(fileName, &fullPath, false))
   {
      NX_STAT_STRUCT fs;
      if (CALL_STAT_FOLLOW_SYMLINK(fullPath, &fs) == 0)
      {
         response->setField(VID_FILE_SIZE, static_cast<uint64_t>(fs.st_size));

         uint32_t crc32;
         CalculateFileCRC32(fullPath, &crc32);
         response->setField(VID_HASH_CRC32, static_cast<uint64_t>(crc32)); // Pass as 64 bit to avoid signed/unsigned issues on Java side

         BYTE md5[MD5_DIGEST_SIZE];
         CalculateFileMD5Hash(fullPath, md5);
         response->setField(VID_HASH_MD5, md5, MD5_DIGEST_SIZE);

         BYTE sha256[SHA256_DIGEST_SIZE];
         CalculateFileSHA256Hash(fullPath, sha256);
         response->setField(VID_HASH_SHA256, sha256, SHA256_DIGEST_SIZE);

         int fh = _topen(fullPath, O_RDONLY | O_BINARY);
         if (fh != -1)
         {
            BYTE buffer[64];
            int bytes = _read(fh, buffer, 64);
            _close(fh);
            if (bytes > 0)
            {
               response->setField(VID_FILE_DATA, buffer, bytes);
            }
         }
         response->setField(VID_RCC, ERR_SUCCESS);
      }
      else
      {
         response->setField(VID_RCC, ERR_IO_FAILURE);
      }
      MemFree(fullPath);
   }
   else
   {
      response->setField(VID_RCC, ERR_ACCESS_DENIED);
   }
}

/**
 * Handler for "merge files" command
 */
static void CH_MergeFiles(const NXCPMessage& request, NXCPMessage *response, AbstractCommSession *session)
{
   if (!session->isMasterServer())
   {
      response->setField(VID_RCC, ERR_ACCESS_DENIED);
      return;
   }

   TCHAR destinationFileName[MAX_PATH];
   request.getFieldAsString(VID_DESTINATION_FILE_NAME, destinationFileName, MAX_PATH);
   ConvertPathToHost(destinationFileName, request.getFieldAsBoolean(VID_ALLOW_PATH_EXPANSION), session->isMasterServer());

   TCHAR *destinationFullPath;
   if (CheckFullPath(destinationFileName, &destinationFullPath, false))
   {
      size_t size;
      const BYTE *md5 = request.getBinaryFieldPtr(VID_HASH_MD5, &size);
      if ((md5 != nullptr) && (size ==  MD5_DIGEST_SIZE))
      {
         StringList partFiles(request, VID_FILE_LIST_BASE, VID_FILE_COUNT);
         if (!partFiles.isEmpty())
         {
            bool success = true;
            Delete(destinationFullPath);
            for(int i = 0; (i < partFiles.size()) && success; i++)
            {
               TCHAR sourceFileName[MAX_PATH];
               _tcslcpy(sourceFileName, partFiles.get(i), MAX_PATH);
               ConvertPathToHost(sourceFileName, request.getFieldAsBoolean(VID_ALLOW_PATH_EXPANSION), session->isMasterServer());
               TCHAR *sourceFullPath;
               if (CheckFullPath(sourceFileName, &sourceFullPath, false))
               {
                  if (!MergeFiles(sourceFullPath, destinationFullPath))
                  {
                     response->setField(VID_RCC, ERR_IO_FAILURE);
                     success = false;
                  }
                  MemFree(sourceFullPath);
               }
               else
               {
                  response->setField(VID_RCC, ERR_ACCESS_DENIED);
                  success = false;
               }
            }

            if (success)
            {
               for(int i = 0; i < partFiles.size(); i++)
               {
                  TCHAR sourceFileName[MAX_PATH];
                  _tcslcpy(sourceFileName, partFiles.get(i), MAX_PATH);
                  ConvertPathToHost(sourceFileName, request.getFieldAsBoolean(VID_ALLOW_PATH_EXPANSION), session->isMasterServer());
                  TCHAR *sourceFullPath;
                  if (CheckFullPath(sourceFileName, &sourceFullPath, false))
                  {
                     Delete(sourceFullPath);
                     MemFree(sourceFullPath);
                  }
               }

               BYTE hash[MD5_DIGEST_SIZE];
               CalculateFileMD5Hash(destinationFullPath, hash);
               if (!memcmp(md5, hash, MD5_DIGEST_SIZE))
               {
                  response->setField(VID_RCC, ERR_SUCCESS);
               }
               else
               {
                  response->setField(VID_RCC, ERR_FILE_HASH_MISMATCH);
               }
            }
         }
         else
         {
            response->setField(VID_RCC, ERR_BAD_ARGUMENTS);
         }
      }
      else
      {
         response->setField(VID_RCC, ERR_BAD_ARGUMENTS);
      }
      MemFree(destinationFullPath);
   }
   else
   {
      response->setField(VID_RCC, ERR_ACCESS_DENIED);
   }
}

/**
 * Process commands like get files in folder, delete file/folder, copy file/folder, move file/folder
 */
static bool ProcessCommands(UINT32 command, NXCPMessage *request, NXCPMessage *response, AbstractCommSession *session)
{
   switch(command)
   {
   	case CMD_GET_FOLDER_SIZE:
   	   CH_GetFolderSize(request, response, session);
         break;
      case CMD_GET_FOLDER_CONTENT:
         CH_GetFolderContent(request, response, session);
         break;
      case CMD_FILEMGR_CREATE_FOLDER:
         CH_CreateFolder(request, response, session);
         break;
      case CMD_GET_FILE_DETAILS:
         CH_GetFileDetails(request, response, session);
         break;
      case CMD_GET_FILE_SET_DETAILS:
         CH_GetFileSetDetails(*request, response, session);
         break;
      case CMD_FILEMGR_DELETE_FILE:
         CH_DeleteFile(request, response, session);
         break;
      case CMD_FILEMGR_RENAME_FILE:
         CH_RenameFile(request, response, session);
         break;
      case CMD_FILEMGR_MOVE_FILE:
         CH_MoveFile(request, response, session);
         break;
      case CMD_FILEMGR_COPY_FILE:
         CH_CopyFile(request, response, session);
         break;
      case CMD_FILEMGR_UPLOAD:
         CH_Upload(request, response, session);
         break;
      case CMD_GET_AGENT_FILE:
         CH_GetFile(*request, response, session);
         break;
      case CMD_CANCEL_FILE_DOWNLOAD:
         CH_CancelFileDownload(request, response, session);
         break;
      case CMD_CANCEL_FILE_MONITORING:
         CH_CancelFileMonitoring(request, response);
         break;
      case CMD_FILEMGR_CHMOD:
         CH_ChangeFilePermissions(request, response, session);
         break;
      case CMD_FILEMGR_CHOWN:
         CH_ChangeFileOwner(request, response, session);
         break;
      case CMD_FILEMGR_GET_FILE_FINGERPRINT:
         CH_GetFileFingerprint(request, response, session);
         break;
      case CMD_FILEMGR_MERGE_FILES:
         CH_MergeFiles(*request, response, session);
         break;
      default:
         return false;
   }

   return true;
}

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO s_info =
{
   NETXMS_SUBAGENT_INFO_MAGIC,
   _T("FILEMGR"), NETXMS_VERSION_STRING,
   SubagentInit, SubagentShutdown, ProcessCommands, nullptr, nullptr,
   0, nullptr, // parameters
   0, nullptr, // lists
   0, nullptr, // tables
   0, nullptr, // actions
   0, nullptr  // push parameters
};

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(FILEMGR)
{
   *ppInfo = &s_info;
   return true;
}

#ifdef _WIN32

/**
 * DLL entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
      DisableThreadLibraryCalls(hInstance);
   return TRUE;
}

#endif
