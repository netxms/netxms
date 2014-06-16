/*
 ** Device Emulation subagent
 ** Copyright (C) 2014 Raden Solutions
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

#include <nms_common.h>
#include <nms_agent.h>
#include <nxclapi.h>

#ifdef _WIN32
#define FILEMNGR_EXPORTABLE __declspec(dllexport) __cdecl
#else
#define FILEMNGR_EXPORTABLE
#endif



/**
 * Variables
 */
static StringList *g_rootFileManagerFolders;

/**
 * Functions
 */
static BOOL SubagentInit(Config *config);
static void SubagentShutdown();
static void SubagentShutdown();
static BOOL ProcessCommands(UINT32 command, CSCPMessage *request, CSCPMessage *response, void *session, int serverType);

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO m_info =
{
   NETXMS_SUBAGENT_INFO_MAGIC,
   _T("FILEMNGR"), NETXMS_VERSION_STRING,
   SubagentInit, SubagentShutdown, ProcessCommands,
   0, NULL, // parameters
   0, NULL, //enums
   0, NULL, // tables
   0, NULL, // actions
   0, NULL  // push parameters
};



/**
 * Subagent initialization
 */
static BOOL SubagentInit(Config *config)
{
   g_rootFileManagerFolders = new StringList();
   ConfigEntry *root = config->getEntry(_T("/filemngr/FileManagerRootFolder"));
   if (root != NULL)
   {
      for(int i = 0; i < root->getValueCount(); i++)
      {
         g_rootFileManagerFolders->add(root->getValue(i));
      }
   }
   return TRUE;
}

/**
 * Called by master agent at unload
 */
static void SubagentShutdown()
{
   delete g_rootFileManagerFolders;
}

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(NXVSCE)
{
   *ppInfo = &m_info;
   return TRUE;
}

TCHAR *getLinuxRealPath(TCHAR *path)
{
   if(path == NULL || path[0] == 0)
      return NULL;
   TCHAR *result = (TCHAR*)malloc(sizeof(TCHAR)*MAX_PATH);
   _tcscpy(result,path);
   TCHAR *current = result;

   //just remove all dots before path
   if(!_tcsncmp(current,_T("../"),3))
      _tcscat(current, current+3);

   if(!_tcsncmp(current,_T("./"),2))
      _tcscat(current, current+2);

   while(current[0] != 0)
   {
      if(current[0] == '/')
      {
         if(current[1] != 0)
         {
            switch(current[1])
            {
               case '/':
                  _tcscat(current, current+1);
                  break;
               case '.':
                  if(current[2] != 0)
                  {
                     if(current[2] == '.' && (current[3] == 0 || current[3] == '/'))
                     {
                        //move to previousX2 '/' symbol
                        if(current == result) //should be adress compare
                        {
                           _tcscat(current, current+3);
                        }
                        else
                        {
                           TCHAR *tmp = current;
                           do
                           {
                              tmp--;
                              if(tmp[0] == '/')
                              {
                                 break;
                              }
                           } while(result != tmp); //should be adress compare
                           _tcscat(tmp, current+3);
                        }
                     }
                  }
                  else
                  {
                     _tcscat(current, current+2);
                  }
               default:
                  current++;
            }
         }
         else
         {
            current++;
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
 * Takes folder/file path - make it absalute(result will be again set to the folder variable)
 * and check that this folder/file is under allowd path.
 * If second parameter is set to true - then request is fror getting content and "/" path should be acepted
 * and afterwards treatet as: "give list of all allowd folders".
 */
BOOL checkFullPath(TCHAR *folder, bool withHomeDir)
{
   char* fullPath;
   TCHAR *fullPathT = NULL;
   AgentWriteDebugLog(3, _T("filemngr getFolderContent: Previous path %s"), folder);

#ifdef _WIN32
   fullPathT = _tfullpath(NULL, folder, MAX_PATH);
#else
   fullPathT = getLinuxRealPath(folder);
#endif
   AgentWriteDebugLog(3, _T("filemngr getFolderContent: Full path %s"), fullPathT);
   if(fullPathT != NULL)
   {
      _tcscpy(folder, fullPathT);
      safe_free(fullPathT);
   }
   else
   {
      return FALSE;
   }
   for(int i = 0; i < g_rootFileManagerFolders->getSize(); i++)
   {
      if(!_tcsncmp(g_rootFileManagerFolders->getValue(i), folder, _tcslen(g_rootFileManagerFolders->getValue(i))) || (!_tcscmp(folder, _T("/")) && withHomeDir))
         return TRUE;
   }

   return FALSE;
}

#define REGULAR_FILE    1
#define DIRECTORY       2
#define SYMLINC         4

/**
 * Puts in response list of containing files
 */
void getFolderContent(TCHAR* folder, CSCPMessage *msg)
{
   msg->SetVariable(VID_RCC, ERR_SUCCESS);
   #ifdef _WIN32
         struct _stat st;
   #else
         struct stat st;
   #endif
   UINT32 count = 0, varId = VID_INSTANCE_LIST_BASE;

   if(!_tcscmp(folder, _T("/")))
   {
      for(int i = 0; i < g_rootFileManagerFolders->getSize(); i++)
      {

         if (_tstat(g_rootFileManagerFolders->getValue(i), &st) == 0)
         {
            msg->SetVariable(varId++, g_rootFileManagerFolders->getValue(i));
            msg->SetVariable(varId++, (QWORD)st.st_size);
            msg->SetVariable(varId++, (QWORD)st.st_mtime);
            UINT32 type = 0;
            type |= (st.st_mode & S_IFLNK) > 0 ? SYMLINC : 0;
            type |= (st.st_mode & S_IFREG) > 0 ? REGULAR_FILE : 0;
            type |= (st.st_mode & S_IFDIR) > 0 ? DIRECTORY : 0;
            msg->SetVariable(varId++, type); //add converation
            TCHAR fullName[MAX_PATH];
            _tcscpy(fullName, g_rootFileManagerFolders->getValue(i));
            msg->SetVariable(varId++, fullName);

            varId += 5;
            count++;
         }
         else
         {
            AgentWriteDebugLog(3, _T("filemngr getFolderContent: Not possible to get folder %s"), g_rootFileManagerFolders->getValue(i));
         }
      }
      msg->SetVariable(VID_INSTANCE_COUNT, count);
      return;
   }

   _TDIR *dir = _topendir(folder);
   if (dir != NULL)
   {
      struct _tdirent *d;
      while((d = _treaddir(dir)) != NULL)
      {
         if (!_tcscmp(d->d_name, _T(".")) || !_tcscmp(d->d_name, _T("..")))
         {
            continue;
         }

         TCHAR fullName[MAX_PATH];
         _tcscpy(fullName, folder);
         _tcscat(fullName, _T("/"));
         _tcscat(fullName, d->d_name);

         if (_tstat(fullName, &st) == 0)
         {
            msg->SetVariable(varId++, d->d_name);
            msg->SetVariable(varId++, (QWORD)st.st_size);
            msg->SetVariable(varId++, (QWORD)st.st_mtime);
            UINT32 type = 0;
            type |= (st.st_mode & S_IFLNK) > 0 ? SYMLINC : 0;
            type |= (st.st_mode & S_IFREG) > 0 ? REGULAR_FILE : 0;
            type |= (st.st_mode & S_IFDIR) > 0 ? DIRECTORY : 0;
            msg->SetVariable(varId++, type); //add converation
            msg->SetVariable(varId++, fullName);

            /*
            S_IFMT     0170000   bit mask for the file type bit fields

            S_IFSOCK   0140000   socket
            S_IFLNK    0120000   symbolic link
            S_IFREG    0100000   regular file
            S_IFBLK    0060000   block device
            S_IFDIR    0040000   directory
            S_IFCHR    0020000   character device
            S_IFIFO    0010000   FIFO
            */

            varId += 5;
            count++;
         }
         else
         {
             AgentWriteDebugLog(6, _T("filemngr getFolderContent: Not possible to get folder %s"), fullName);
         }
      }
      msg->SetVariable(VID_INSTANCE_COUNT, count);
   }
   else
   {
      msg->SetVariable(VID_RCC, RCC_IO_ERROR);
   }
}

static BOOL DeleteFile(TCHAR* name)
{
   #ifdef _WIN32
         struct _stat st;
   #else
         struct stat st;
   #endif

   if(_tstat(name, &st) != 0)
   {
      return FALSE;
   }

   bool result = true;

   if(S_ISDIR(st.st_mode))
   {
      //get element list and for each element run DeleteFile
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
            result &= DeleteFile(newName);
         }
      }
      //remove directory
      #ifdef _WIN32
         #ifdef UNICODE
            if(RemoveDirectoryW(name))
         #else
            if(RemoveDirectory(name))
         #endif
         {
            return result & TRUE;
         }
         else
         {
            return FALSE;
         }
      #else
         #ifdef UNICODE
            char *cName = UTF8StringFromWideString(name);
            result &= rmdir(cName) == 0 ? true : false;
            safe_free(cName);
         #else
            result &= rmdir(name) == 0 ? true : false;
         #endif
         return result;
      #endif
   }
   else
   {
      if (_tremove(name) == 0)
      {
         return TRUE;
      }
      else
      {
         return FALSE;
      }
   }
}

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

static BOOL copyLinuxFile(struct stat *st, TCHAR* oldName, TCHAR* newName)
{
   int oldFile, newFile;
   oldFile = _topen(oldName, O_RDONLY | O_BINARY);
   newFile = _topen(newName, O_CREAT | O_BINARY | O_WRONLY, st->st_mode); // should be copyed with the same acess rights
   if (oldFile == -1 || newFile == -1)
   {
      return FALSE;
   }

   BYTE *readBytes;
   int readSize = 16384, readIn, readOut;
   readBytes = (BYTE*)malloc(readSize);

   while((readIn = read (oldFile, readBytes, readSize)) > 0)
   {
      readOut = write(newFile, readBytes, (ssize_t) readIn);
      if(readOut != readIn)
      {
         close(oldFile);
         close(newFile);
         return FALSE;
      }
    }

    /* Close file descriptors */
    close(oldFile);
    close(newFile);
    return TRUE;
}

static BOOL MoveFile(TCHAR* oldName, TCHAR* newName)
{
   #ifdef _WIN32
   #ifdef UNICODE
   if(MoveFileExW(oldName, newName, MOVEFILE_COPY_ALLOWED) != 0)
   #else
   if(MoveFileEx(oldName, newName, MOVEFILE_COPY_ALLOWED) != 0)
   #endif
   {
      return TRUE;
   }
   else
   {
      return FALSE;
   }
   #else
   if(Rename(oldName, newName))
   {
      return TRUE;
   }

   struct stat st;

   if(_tstat(oldName, &st) != 0)
   {
      return FALSE;
   }

   if(S_ISDIR(st.st_mode))
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
      }
      #ifdef UNICODE
         char *cName = UTF8StringFromWideString(oldName);
         rmdir(cName);
         safe_free(cName);
      #else
         rmdir(oldName);
      #endif
   }
   else
   {
      if (!copyLinuxFile(&st, oldName, newName))
         return false;
   }
   return TRUE;
   #endif
}

/**
 * Process commands like get files in folder, delete file/folder, create folder, copy file/folder, create file
 */
static BOOL ProcessCommands(UINT32 command, CSCPMessage *request, CSCPMessage *response, void *session, int serverType)
{
   switch(command)
   {
      case CMD_GET_FOLDER_CONTENT:
      {
         TCHAR directory[MAX_PATH];
         request->GetVariableStr(VID_FILE_NAME, directory, MAX_PATH);
         response->SetId(request->GetId());
         if(directory == NULL)
         {
            response->SetVariable(VID_RCC, RCC_IO_ERROR);
            AgentWriteDebugLog(6, _T("filemngr ProcessCommands(): File name should be set."));
            return TRUE;
         }

         AgentWriteDebugLog(6, _T("filemngr ProcessCommands(): %d %d ."), checkFullPath(directory, true), serverType == MASTER_SERVER);
         if(checkFullPath(directory, true) && serverType == MASTER_SERVER)
         {
            getFolderContent(directory, response);
         }
         else
         {
            AgentWriteDebugLog(6, _T("filemngr ProcessCommands(): Acess denid."));
            response->SetVariable(VID_RCC, RCC_ACCESS_DENIED);
         }
         return TRUE;
      }
      case CMD_FILEMNGR_DELETE_FILE:
      {
         TCHAR file[MAX_PATH];
         request->GetVariableStr(VID_FILE_NAME, file, MAX_PATH);
         response->SetId(request->GetId());
         if(file == NULL)
         {
            response->SetVariable(VID_RCC, RCC_IO_ERROR);
            AgentWriteDebugLog(6, _T("filemngr ProcessCommands(): File name should be set."));
            return TRUE;
         }

         if(checkFullPath(file, false) && serverType == MASTER_SERVER)
         {
            if(DeleteFile(file))
            {
               response->SetVariable(VID_RCC, ERR_SUCCESS);
            }
            else
            {
               response->SetVariable(VID_RCC, RCC_IO_ERROR);
            }
         }
         else
         {
            AgentWriteDebugLog(6, _T("filemngr ProcessCommands(): Acess denid."));
            response->SetVariable(VID_RCC, RCC_ACCESS_DENIED);
         }
         return TRUE;
      }
      case CMD_FILEMNGR_RENAME_FILE:
      {
         TCHAR oldName[MAX_PATH];
         request->GetVariableStr(VID_FILE_NAME, oldName, MAX_PATH);
         TCHAR newName[MAX_PATH];
         request->GetVariableStr(VID_NEW_FILE_NAME, newName, MAX_PATH);
         response->SetId(request->GetId());
         if(oldName == NULL && newName == NULL)
         {
            response->SetVariable(VID_RCC, RCC_IO_ERROR);
            AgentWriteDebugLog(6, _T("filemngr ProcessCommands(): File names should be set."));
            return TRUE;
         }


         AgentWriteDebugLog(6, _T("filemngr ProcessCommands(): %d, %d, %d."), checkFullPath(oldName, false), checkFullPath(newName, false), serverType == MASTER_SERVER);
         if(checkFullPath(oldName, false) && checkFullPath(newName, false) && serverType == MASTER_SERVER)
         {
            if(Rename(oldName, newName))
            {
               response->SetVariable(VID_RCC, ERR_SUCCESS);
            }
            else
            {
               response->SetVariable(VID_RCC, RCC_IO_ERROR);
            }
         }
         else
         {
            AgentWriteDebugLog(6, _T("filemngr ProcessCommands(): Acess denid."));
            response->SetVariable(VID_RCC, RCC_ACCESS_DENIED);
         }
         return TRUE;
      }
      case CMD_FILEMNGR_MOVE_FILE:
      {
         TCHAR oldName[MAX_PATH];
         request->GetVariableStr(VID_FILE_NAME, oldName, MAX_PATH);
         TCHAR newName[MAX_PATH];
         request->GetVariableStr(VID_NEW_FILE_NAME, newName, MAX_PATH);
         response->SetId(request->GetId());
         if(oldName == NULL && newName == NULL)
         {
            response->SetVariable(VID_RCC, RCC_IO_ERROR);
            AgentWriteDebugLog(6, _T("filemngr ProcessCommands(): File names should be set."));
            return TRUE;
         }

         if(checkFullPath(oldName, false) && checkFullPath(newName, false) && serverType == MASTER_SERVER)
         {
            if(MoveFile(oldName, newName))
            {
               response->SetVariable(VID_RCC, ERR_SUCCESS);
            }
            else
            {
               response->SetVariable(VID_RCC, RCC_IO_ERROR);
            }
         }
         else
         {
            AgentWriteDebugLog(6, _T("filemngr ProcessCommands(): Acess denid."));
            response->SetVariable(VID_RCC, RCC_ACCESS_DENIED);
         }
         return TRUE;

      }
      default:
         return FALSE;
   }
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
