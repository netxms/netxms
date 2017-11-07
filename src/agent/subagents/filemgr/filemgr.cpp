/*
 ** File management subagent
 ** Copyright (C) 2014-2017 Raden Solutions
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

#ifndef _WIN32
#include <pwd.h>
#include <grp.h>
#endif

/*
 * Create new RootFolder
 */
RootFolder::RootFolder(const TCHAR *folder)
{
   m_folder = _tcsdup(folder);
   m_readOnly = false;

   TCHAR *ptr = _tcschr(m_folder, _T(';'));
   if (ptr == NULL)
      return;

   *ptr = 0;
   if (_tcscmp(ptr+1, _T("ro")) == 0)
      m_readOnly = true;
}

/**
 * Root folders
 */
static ObjectArray<RootFolder> *g_rootFileManagerFolders;
static HashMap<UINT32, VolatileCounter> *g_downloadFileStopMarkers;

/**
 * Monitored file list
 */
MonitoredFileList g_monitorFileList;


#ifdef _WIN32

/**
 * Convert path from UNIX to local format
 */
static void ConvertPathToHost(TCHAR *path)
{
   for(int i = 0; path[i] != 0; i++)
      if (path[i] == _T('/'))
         path[i] = _T('\\');
}

#else

#define ConvertPathToHost(x)

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

#define ConvertPathToNetwork(x)

#endif

/**
 * Subagent initialization
 */
static BOOL SubagentInit(Config *config)
{
   g_rootFileManagerFolders = new ObjectArray<RootFolder>(16, 16, true);
   g_downloadFileStopMarkers = new HashMap<UINT32, VolatileCounter>();
   ConfigEntry *root = config->getEntry(_T("/filemgr/RootFolder"));
   if (root != NULL)
   {
      for(int i = 0; i < root->getValueCount(); i++)
      {
         RootFolder *folder = new RootFolder(root->getValue(i));
         g_rootFileManagerFolders->add(folder);
         AgentWriteDebugLog(5, _T("FILEMGR: added root folder \"%s\""), folder->getFolder());
      }
   }
   AgentWriteDebugLog(2, _T("FILEMGR: subagent initialized"));
   return TRUE;
}

/**
 * Called by master agent at unload
 */
static void SubagentShutdown()
{
   delete g_rootFileManagerFolders;
   delete g_downloadFileStopMarkers;
}

#ifndef _WIN32

/**
 * Converts path to absolute removing "//", "../", "./" ...
 */
static TCHAR *GetRealPath(TCHAR *path)
{
   if(path == NULL || path[0] == 0)
      return NULL;
   TCHAR *result = (TCHAR*)malloc(sizeof(TCHAR)*MAX_PATH);
   _tcscpy(result,path);
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
 * Takes folder/file path - make it absolute (result will be written back to the folder variable)
 * and check that this folder/file is under allowed root path.
 * If second parameter is set to true - then request is for getting content and "/" path should be acepted
 * and afterwards treatet as: "give list of all allowd folders".
 */
static bool CheckFullPath(TCHAR *folder, bool withHomeDir, bool isModify = false)
{
   AgentWriteDebugLog(3, _T("FILEMGR: CheckFullPath: input is %s"), folder);
   if (withHomeDir && !_tcscmp(folder, FS_PATH_SEPARATOR))
   {
      return true;
   }

#ifdef _WIN32
   TCHAR *fullPathT = _tfullpath(NULL, folder, MAX_PATH);
#else
   TCHAR *fullPathT = GetRealPath(folder);
#endif
   AgentWriteDebugLog(3, _T("FILEMGR: CheckFullPath: Full path %s"), fullPathT);
   if (fullPathT != NULL)
   {
      _tcscpy(folder, fullPathT);
      free(fullPathT);
   }
   else
   {
      return false;
   }
   for(int i = 0; i < g_rootFileManagerFolders->size(); i++)
   {
      if (!_tcsncmp(g_rootFileManagerFolders->get(i)->getFolder(), folder, _tcslen(g_rootFileManagerFolders->get(i)->getFolder())))
      {
         if (isModify && g_rootFileManagerFolders->get(i)->isReadOnly())
            return false;
         else
            return true;
      }
   }

   return false;
}

#define REGULAR_FILE    1
#define DIRECTORY       2
#define SYMLINC         4

/**
 * Returns if file already exist
 */
int CheckFileType(const TCHAR *fileName)
{
   NX_STAT_STRUCT st;
   if (CALL_STAT(fileName, &st) == 0)
   {
      if(S_ISDIR(st.st_mode))
      {
         return DIRECTORY;
      }
      else
         return REGULAR_FILE;
   }
   return -1;
}

bool VerifyFileOperation(const TCHAR *fileName, bool allowOverwirite, NXCPMessage *response)
{
   int fileType = CheckFileType(fileName);
   if(fileType > 0 && !allowOverwirite)
   {
      response->setField(VID_RCC, fileType == DIRECTORY ? ERR_FOLDER_ALREADY_EXISTS : ERR_FILE_ALREADY_EXISTS);
      return false;
   }
   else
      return true;
}

#ifdef _WIN32

TCHAR *GetFileOwnerWin(const TCHAR *file)
{
   return _tcsdup(_T(""));
}

#endif // _WIN32

static bool FillMessageFolderContent(const TCHAR *filePath, const TCHAR *fileName, NXCPMessage *msg, UINT32 varId)
{
   if (_taccess(filePath, 4) != 0)
      return false;

   NX_STAT_STRUCT st;
   if (CALL_STAT(filePath, &st) == 0)
   {
      msg->setField(varId++, fileName);
      msg->setField(varId++, (QWORD)st.st_size);
      msg->setField(varId++, (QWORD)st.st_mtime);
      UINT32 type = 0;
      TCHAR accessRights[11];
#ifndef _WIN32
      if(S_ISLNK(st.st_mode))
      {
         accessRights[0] = _T('l');
         type |= SYMLINC;
         NX_STAT_STRUCT symlincSt;
         if (CALL_STAT_FOLLOW_SYMLINK(filePath, &symlincSt) == 0)
         {
            type |= S_ISDIR(symlincSt.st_mode) ? DIRECTORY : 0;
         }
      }

      if(S_ISCHR(st.st_mode)) accessRights[0] = _T('c');
      if(S_ISBLK(st.st_mode)) accessRights[0] = _T('b');
      if(S_ISFIFO(st.st_mode)) accessRights[0] = _T('p');
      if(S_ISSOCK(st.st_mode)) accessRights[0] = _T('s');
#endif
      if(S_ISREG(st.st_mode))
      {
         type |= REGULAR_FILE;
         accessRights[0] = _T('-');
      }
      if(S_ISDIR(st.st_mode))
      {
         type |= DIRECTORY;
         accessRights[0] = _T('d');
      }

      msg->setField(varId++, type);
      TCHAR fullName[MAX_PATH];
      _tcscpy(fullName, filePath);
      msg->setField(varId++, fullName);

#ifndef _WIN32
      struct passwd *pw = getpwuid(st.st_uid);
      struct group  *gr = getgrgid(st.st_gid);
#ifdef UNICODE
      msg->setFieldFromMBString(varId++, pw->pw_name);
      msg->setFieldFromMBString(varId++, gr->gr_name);
#else
      msg->setField(varId++, pw->pw_name);
      msg->setField(varId++, gr->gr_name);
#endif
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
#else
      TCHAR *owner = GetFileOwnerWin(filePath);
      msg->setField(varId++, owner);
      safe_free(owner);
      msg->setField(varId++, _T(""));
      msg->setField(varId++, _T(""));
#endif // _WIN32
      return true;
   }
   else
   {
      AgentWriteDebugLog(3, _T("FILEMGR: GetFolderContent: cannot get folder %s"), filePath);
      return false;
   }
}

/**
 * Puts in response list of containing files
 */
static void GetFolderContent(TCHAR *folder, NXCPMessage *response, bool rootFolder, bool allowMultipart, AbstractCommSession *session)
{
   nxlog_debug(5, _T("FILEMGR: GetFolderContent: reading \"%s\" (root=%s, multipart=%s)"),
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

   UINT32 count = 0;
   UINT32 fieldId = VID_INSTANCE_LIST_BASE;

   if (!_tcscmp(folder, FS_PATH_SEPARATOR) && rootFolder)
   {
      response->setField(VID_RCC, ERR_SUCCESS);

      for(int i = 0; i < g_rootFileManagerFolders->size(); i++)
      {
         if (FillMessageFolderContent(g_rootFileManagerFolders->get(i)->getFolder(), g_rootFileManagerFolders->get(i)->getFolder(), msg, fieldId))
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
      nxlog_debug(5, _T("FILEMGR: GetFolderContent: reading \"%s\" completed"), folder);
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

   nxlog_debug(5, _T("FILEMGR: GetFolderContent: reading \"%s\" completed"), folder);
}

/**
 * Delete file/folder
 */
static BOOL Delete(const TCHAR *name)
{
   NX_STAT_STRUCT st;

   if (CALL_STAT(name, &st) != 0)
   {
      return FALSE;
   }

   bool result = true;

   if (S_ISDIR(st.st_mode))
   {
      // get element list and for each element run Delete
      _TDIR *dir = _topendir(name);
      if (dir != NULL)
      {
         struct _tdirent *d;
         while((d = _treaddir(dir)) != NULL)
         {
            if (!_tcscmp(d->d_name, _T(".")) || !_tcscmp(d->d_name, _T("..")))
            {
               continue;
            }
            TCHAR newName[MAX_PATH];
            _tcscpy(newName, name);
            _tcscat(newName, FS_PATH_SEPARATOR);
            _tcscat(newName, d->d_name);
            result = result && Delete(newName);
         }
         _tclosedir(dir);
      }
      //remove directory
#ifdef _WIN32
      return RemoveDirectory(name);
#else
      return _trmdir(name) == 0;
#endif
   }
#ifdef _WIN32
   return DeleteFile(name);
#else
   return _tremove(name) == 0;
#endif
}

/**
 * Rename file/folder
 */
static BOOL Rename(TCHAR* oldName, TCHAR * newName)
{
   if (_trename(oldName, newName) == 0)
   {
      return TRUE;
   }
   else
   {
      return FALSE;
   }
}

#ifndef _WIN32

/**
 * Copy file/folder
 */
static BOOL CopyFile(NX_STAT_STRUCT *st, const TCHAR *oldName, const TCHAR *newName)
{
   int oldFile = _topen(oldName, O_RDONLY | O_BINARY);
   if (oldFile == -1)
      return FALSE;

   int newFile = _topen(newName, O_CREAT | O_BINARY | O_WRONLY, st->st_mode); // should be copied with the same acess rights
   if (newFile == -1)
   {
      close(oldFile);
      return FALSE;
   }

   int size = 16384, in, out;
   BYTE *bytes = (BYTE *)malloc(size);

   while((in = read(oldFile, bytes, size)) > 0)
   {
      out = write(newFile, bytes, (ssize_t)in);
      if (out != in)
      {
         close(oldFile);
         close(newFile);
         free(bytes);
         return FALSE;
      }
   }

   close(oldFile);
   close(newFile);
   free(bytes);
   return TRUE;
}

#endif

/**
 * Move file/folder
 */
static BOOL MoveFile(TCHAR* oldName, TCHAR* newName)
{
#ifdef _WIN32
   return MoveFileEx(oldName, newName, MOVEFILE_COPY_ALLOWED);
#else
   if (Rename(oldName, newName))
   {
      return TRUE;
   }

   NX_STAT_STRUCT st;

   if (CALL_STAT(oldName, &st) != 0)
   {
      return FALSE;
   }

   if (S_ISDIR(st.st_mode))
   {
      _tmkdir(newName, st.st_mode);
      _TDIR *dir = _topendir(oldName);
      if (dir != NULL)
      {
         struct _tdirent *d;
         while((d = _treaddir(dir)) != NULL)
         {
            if (!_tcscmp(d->d_name, _T(".")) || !_tcscmp(d->d_name, _T("..")))
            {
               continue;
            }
            TCHAR nextNewName[MAX_PATH];
            _tcscpy(nextNewName, newName);
            _tcscat(nextNewName, _T("/"));
            _tcscat(nextNewName, d->d_name);

            TCHAR nextOldaName[MAX_PATH];
            _tcscpy(nextOldaName, oldName);
            _tcscat(nextOldaName, _T("/"));
            _tcscat(nextOldaName, d->d_name);

            MoveFile(nextOldaName, nextNewName);
         }
         _tclosedir(dir);
      }
      _trmdir(oldName);
   }
   else
   {
      if (!CopyFile(&st, oldName, newName))
         return FALSE;
   }
   return TRUE;
#endif /* _WIN32 */
}

/**
 * Send file
 */
 THREAD_RESULT THREAD_CALL SendFile(void *dataStruct)
{
   MessageData *data = (MessageData *)dataStruct;

   AgentWriteDebugLog(5, _T("CommSession::getLocalFile(): request for file \"%s\", follow = %s, compress = %s"),
               data->fileName, data->follow ? _T("true") : _T("false"), data->allowCompression ? _T("true") : _T("false"));
   bool success = AgentSendFileToServer(data->session, data->id, data->fileName, (int)data->offset, data->allowCompression, g_downloadFileStopMarkers->get(data->id));
   if (data->follow && success)
   {
      g_monitorFileList.add(data->fileNameCode);
      FollowData *flData = new FollowData(data->fileName, data->fileNameCode, 0, data->session->getServerAddress());
      ThreadCreateEx(SendFileUpdatesOverNXCP, 0, flData);
   }
   free(data->fileName);
   free(data->fileNameCode);
   g_downloadFileStopMarkers->remove(data->id);
   delete data;
   return THREAD_OK;
}

/**
 * Get folder information
 */
static void GetFolderInfo(const TCHAR *folder, UINT64 *fileCount, UINT64 *folderSize)
{
   _TDIR *dir = _topendir(folder);
   if (dir != NULL)
   {
      NX_STAT_STRUCT st;
      struct _tdirent *d;
      while((d = _treaddir(dir)) != NULL)
      {
         if (sizeof(folder) >= MAX_PATH)
            return;

         if (!_tcscmp(d->d_name, _T(".")) || !_tcscmp(d->d_name, _T("..")))
         {
            continue;
         }

         TCHAR fullName[MAX_PATH];
         _tcscpy(fullName, folder);
         _tcscat(fullName, FS_PATH_SEPARATOR);
         _tcscat(fullName, d->d_name);

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
}

/**
 * Process commands like get files in folder, delete file/folder, copy file/folder, move file/folder
 */
static BOOL ProcessCommands(UINT32 command, NXCPMessage *request, NXCPMessage *response, AbstractCommSession *session)
{
   switch(command)
   {
   	case CMD_GET_FOLDER_SIZE:
   	{
   	   TCHAR directory[MAX_PATH];
   	   request->getFieldAsString(VID_FILE_NAME, directory, MAX_PATH);
   	   response->setId(request->getId());
   	   if (directory[0] == 0)
         {
            response->setField(VID_RCC, ERR_IO_FAILURE);
            AgentWriteDebugLog(6, _T("FILEMGR: ProcessCommands(CMD_GET_FOLDER_SIZE): File name should be set."));
            return TRUE;
         }
         ConvertPathToHost(directory);

         if (CheckFullPath(directory, false) && session->isMasterServer())
         {
            UINT64 fileCount = 0, fileSize = 0;
            GetFolderInfo(directory, &fileCount, &fileSize);
            response->setField(VID_RCC, ERR_SUCCESS);
            response->setField(VID_FOLDER_SIZE, fileSize);
            response->setField(VID_FILE_COUNT, fileCount);
         }
         else
         {
            AgentWriteDebugLog(6, _T("FILEMGR: ProcessCommands(CMD_GET_FOLDER_SIZE): Access denied"));
            response->setField(VID_RCC, ERR_ACCESS_DENIED);
         }

         return TRUE;
   	}
      case CMD_GET_FOLDER_CONTENT:
      {
         TCHAR directory[MAX_PATH];
         request->getFieldAsString(VID_FILE_NAME, directory, MAX_PATH);
         response->setId(request->getId());
         if (directory[0] == 0)
         {
            response->setField(VID_RCC, ERR_IO_FAILURE);
            AgentWriteDebugLog(6, _T("FILEMGR: ProcessCommands(CMD_GET_FOLDER_CONTENT): File name should be set."));
            return TRUE;
         }
         ConvertPathToHost(directory);

         bool rootFolder = request->getFieldAsUInt16(VID_ROOT) ? 1 : 0;
         if (CheckFullPath(directory, rootFolder) && session->isMasterServer())
         {
            GetFolderContent(directory, response, rootFolder, request->getFieldAsBoolean(VID_ALLOW_MULTIPART), session);
         }
         else
         {
            AgentWriteDebugLog(6, _T("FILEMGR: ProcessCommands(CMD_GET_FOLDER_CONTENT): Access denied"));
            response->setField(VID_RCC, ERR_ACCESS_DENIED);
         }
         return TRUE;
      }
      case CMD_FILEMGR_DELETE_FILE:
      {
         TCHAR file[MAX_PATH];
         request->getFieldAsString(VID_FILE_NAME, file, MAX_PATH);
         response->setId(request->getId());
         if(file[0] == 0)
         {
            response->setField(VID_RCC, ERR_IO_FAILURE);
            AgentWriteDebugLog(6, _T("FILEMGR: ProcessCommands(CMD_FILEMGR_DELETE_FILE): File name should be set."));
            return TRUE;
         }
         ConvertPathToHost(file);

         if (CheckFullPath(file, false, true) && session->isMasterServer())
         {
            if (Delete(file))
            {
               response->setField(VID_RCC, ERR_SUCCESS);
            }
            else
            {
               response->setField(VID_RCC, ERR_IO_FAILURE);
            }
         }
         else
         {
            AgentWriteDebugLog(6, _T("FILEMGR: ProcessCommands(CMD_FILEMGR_DELETE_FILE): Access denied"));
            response->setField(VID_RCC, ERR_ACCESS_DENIED);
         }
         return TRUE;
      }
      case CMD_FILEMGR_RENAME_FILE:
      {
         TCHAR oldName[MAX_PATH];
         request->getFieldAsString(VID_FILE_NAME, oldName, MAX_PATH);
         TCHAR newName[MAX_PATH];
         request->getFieldAsString(VID_NEW_FILE_NAME, newName, MAX_PATH);
         bool allowOverwirite = request->getFieldAsBoolean(VID_OVERVRITE);
         response->setId(request->getId());
         if (oldName[0] == 0 && newName[0] == 0)
         {
            response->setField(VID_RCC, ERR_IO_FAILURE);
            AgentWriteDebugLog(6, _T("FILEMGR: ProcessCommands(CMD_FILEMGR_RENAME_FILE): File names should be set."));
            return TRUE;
         }
         ConvertPathToHost(oldName);
         ConvertPathToHost(newName);

         if (CheckFullPath(oldName, false, true) && CheckFullPath(newName, false) && session->isMasterServer())
         {
            if(VerifyFileOperation(newName, allowOverwirite, response))
            {
               if (Rename(oldName, newName))
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
            AgentWriteDebugLog(6, _T("FILEMGR: ProcessCommands(CMD_FILEMGR_RENAME_FILE): Access denied"));
            response->setField(VID_RCC, ERR_ACCESS_DENIED);
         }
         return TRUE;
      }
      case CMD_FILEMGR_MOVE_FILE:
      {
         TCHAR oldName[MAX_PATH];
         request->getFieldAsString(VID_FILE_NAME, oldName, MAX_PATH);
         TCHAR newName[MAX_PATH];
         request->getFieldAsString(VID_NEW_FILE_NAME, newName, MAX_PATH);
         bool allowOverwirite = request->getFieldAsBoolean(VID_OVERVRITE);
         response->setId(request->getId());
         if ((oldName[0] == 0) && (newName[0] == 0))
         {
            response->setField(VID_RCC, ERR_IO_FAILURE);
            AgentWriteDebugLog(6, _T("FILEMGR: ProcessCommands(CMD_FILEMGR_MOVE_FILE): File names should be set."));
            return TRUE;
         }
         ConvertPathToHost(oldName);
         ConvertPathToHost(newName);

         if (CheckFullPath(oldName, false, true) && CheckFullPath(newName, false) && session->isMasterServer())
         {
            if(VerifyFileOperation(newName, allowOverwirite, response))
            {
               if(MoveFile(oldName, newName))
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
            AgentWriteDebugLog(6, _T("FILEMGR: ProcessCommands(CMD_FILEMGR_MOVE_FILE): Access denied"));
            response->setField(VID_RCC, ERR_ACCESS_DENIED);
         }
         return TRUE;
      }
      case CMD_FILEMGR_UPLOAD:
      {
         TCHAR name[MAX_PATH];
         request->getFieldAsString(VID_FILE_NAME, name, MAX_PATH);
         bool allowOverwirite = request->getFieldAsBoolean(VID_OVERVRITE);
         response->setId(request->getId());
         if (name[0] == 0)
         {
            response->setField(VID_RCC, ERR_IO_FAILURE);
            AgentWriteDebugLog(6, _T("FILEMGR: ProcessCommands(CMD_FILEMGR_UPLOAD): File name should be set."));
            return TRUE;
         }
         ConvertPathToHost(name);

         if (CheckFullPath(name, false, true) && session->isMasterServer())
         {
            if(VerifyFileOperation(name, allowOverwirite, response))
               response->setField(VID_RCC, session->openFile(name, request->getId(), request->getFieldAsTime(VID_MODIFICATION_TIME)));
         }
         else
         {
            AgentWriteDebugLog(6, _T("FILEMGR: ProcessCommands(CMD_FILEMGR_UPLOAD): Access denied"));
            response->setField(VID_RCC, ERR_ACCESS_DENIED);
         }
         return TRUE;
      }
      case CMD_GET_FILE_DETAILS:
      {
         TCHAR fileName[MAX_PATH];
         request->getFieldAsString(VID_FILE_NAME, fileName, MAX_PATH);
         ExpandFileName(fileName, fileName, MAX_PATH, session->isMasterServer());
         response->setId(request->getId());

      	if (session->isMasterServer() && CheckFullPath(fileName, false))
         {
            NX_STAT_STRUCT fs;

            //prepare file name
            if (CALL_STAT(fileName, &fs) == 0)
            {
               response->setField(VID_FILE_SIZE, (UINT64)fs.st_size);
               response->setField(VID_MODIFICATION_TIME, (UINT64)fs.st_mtime);
               response->setField(VID_RCC, ERR_SUCCESS);
            }
            else
            {
               response->setField(VID_RCC, ERR_FILE_STAT_FAILED);
            }
         }
         else
         {
            AgentWriteDebugLog(6, _T("FILEMGR: ProcessCommands(CMD_GET_FILE_DETAILS): Access denied"));
            response->setField(VID_RCC, ERR_ACCESS_DENIED);
         }
         return TRUE;
      }
      case CMD_GET_AGENT_FILE:
      {
         response->setId(request->getId());
         TCHAR fileName[MAX_PATH];
         request->getFieldAsString(VID_FILE_NAME, fileName, MAX_PATH);
         ExpandFileName(fileName, fileName, MAX_PATH, session->isMasterServer());

         if (CheckFullPath(fileName, false))
         {
            TCHAR *fileNameCode = (TCHAR*)malloc(MAX_PATH * sizeof(TCHAR));
            request->getFieldAsString(VID_NAME, fileNameCode, MAX_PATH);

            MessageData *data = new MessageData();
            data->fileName = _tcsdup(fileName);
            data->fileNameCode = fileNameCode;
            data->follow = request->getFieldAsBoolean(VID_FILE_FOLLOW);
            data->allowCompression = request->getFieldAsBoolean(VID_ENABLE_COMPRESSION);
            data->id = request->getId();
            data->offset = request->getFieldAsUInt32(VID_FILE_OFFSET);
            data->session = session;
            g_downloadFileStopMarkers->set(request->getId(), new VolatileCounter(0));

            ThreadCreateEx(SendFile, 0, data);

            response->setField(VID_RCC, ERR_SUCCESS);
         }
         else
         {
            response->setField(VID_RCC, ERR_ACCESS_DENIED);
         }
         return TRUE;
      }
      case CMD_CANCEL_FILE_DOWNLOAD:
      {
         VolatileCounter *counter = g_downloadFileStopMarkers->get(request->getFieldAsUInt32(VID_REQUEST_ID));
         if(counter != NULL)
         {
            InterlockedIncrement(counter);
            response->setField(VID_RCC, ERR_SUCCESS);
         }
         else
            response->setField(VID_RCC, ERR_INTERNAL_ERROR);
         return TRUE;
      }
      case CMD_CANCEL_FILE_MONITORING:
      {
         response->setId(request->getId());
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
         return TRUE;
      }
      case CMD_FILEMGR_CREATE_FOLDER:
      {
         TCHAR directory[MAX_PATH];
         request->getFieldAsString(VID_FILE_NAME, directory, MAX_PATH);
         response->setId(request->getId());
         if (directory[0] == 0)
         {
            response->setField(VID_RCC, ERR_IO_FAILURE);
            AgentWriteDebugLog(6, _T("FILEMGR: ProcessCommands(CMD_FILEMGR_CREATE_FOLDER): File name should be set."));
            return TRUE;
         }
         ConvertPathToHost(directory);

         if (CheckFullPath(directory, false, true) && session->isMasterServer())
         {
            if(VerifyFileOperation(directory, false, response))
            {
               if (CreateFolder(directory))
               {
                  response->setField(VID_RCC, ERR_SUCCESS);
               }
               else
               {
                  AgentWriteDebugLog(6, _T("FILEMGR: ProcessCommands(CMD_FILEMGR_CREATE_FOLDER): Could not create directory"));
                  response->setField(VID_RCC, ERR_IO_FAILURE);
               }
            }
         }
         else
         {
            AgentWriteDebugLog(6, _T("FILEMGR: ProcessCommands(CMD_FILEMGR_CREATE_FOLDER): Access denied"));
            response->setField(VID_RCC, ERR_ACCESS_DENIED);
         }
         return TRUE;
      }
      default:
         return FALSE;
   }
}

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO m_info =
{
   NETXMS_SUBAGENT_INFO_MAGIC,
   _T("FILEMGR"), NETXMS_BUILD_TAG,
   SubagentInit, SubagentShutdown, ProcessCommands,
   0, NULL, // parameters
   0, NULL, // lists
   0, NULL, // tables
   0, NULL, // actions
   0, NULL  // push parameters
};

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(FILEMGR)
{
   *ppInfo = &m_info;
   return TRUE;
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
