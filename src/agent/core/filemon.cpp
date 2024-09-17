/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2021 Raden Solutions
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
** File: filemon.cpp
**
**/

#include "nxagentd.h"
#include <netxms-version.h>
#include <nxevent.h>
#include <nxstat.h>

#define DEBUG_TAG _T("filemon")

/**
 * Contains information about a file that shoud be monitored + a tecnical flag checkPassed
 */
struct FileInfo
{
   BYTE hash[SHA256_DIGEST_SIZE];
   time_t modTime; // Last modification time
   uint32_t permissions;
   bool checkPassed; // Flag to identity files that have passed checks for monitoring mulpiple path occurences and deleted files
};

static StringSet s_rootPaths;
static StringObjectMap<FileInfo> s_files(Ownership::True);
static uint32_t s_interval = 21600; // File integrity check interval in seconds (6 hours by default)
static THREAD s_monitorThread = INVALID_THREAD_HANDLE;

/**
 * Saves file info to file_integrity table in agent's local DB
 */
static void SaveToDB(const TCHAR *path, const FileInfo *fi)
{
   DB_HANDLE hdb = GetLocalDatabaseHandle();
   if (hdb != nullptr)
   {
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT OR REPLACE INTO file_integrity (path,hash,mod_time,permissions) VALUES (?,?,?,?)"));
      if (hStmt != nullptr)
      {
         TCHAR hashAsString[SHA256_DIGEST_SIZE * 2 + 1];
         DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, path, DB_BIND_STATIC);
         DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, BinToStr(fi->hash, SHA256_DIGEST_SIZE, hashAsString), DB_BIND_STATIC);
         DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, static_cast<int64_t>(fi->modTime));
         DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, fi->permissions);
         DBExecute(hStmt);
         DBFreeStatement(hStmt);
      }
   }
}

/**
 * Deletes file info from file_integrity table in agent's local DB
 */
static void DeleteFromDB(const TCHAR *path)
{
   DB_HANDLE hdb = GetLocalDatabaseHandle();
   if (hdb != nullptr)
   {
      TCHAR query[MAX_PATH + 40];
      _sntprintf(query, MAX_PATH + 40, _T("DELETE FROM file_integrity WHERE path = %s"), DBPrepareString(hdb, path).cstr());
      DBQuery(hdb, query);
   }
}

/**
 * Loads monitored files info from agent's local DB to s_files
 */
static bool LoadFromDB()
{
   bool success = false;
   DB_HANDLE hdb = GetLocalDatabaseHandle();
   if (hdb != nullptr)
   {
      DB_RESULT result = DBSelect(hdb, _T("SELECT path,hash,mod_time,permissions FROM file_integrity"));
      if (result != nullptr)
      {
         int totalRows = DBGetNumRows(result);
         if (totalRows > 0)
         {
            TCHAR path[MAX_PATH];
            FileInfo fi;
            for (int row = 0; row < totalRows; row++)
            {
               DBGetField(result, row, 0, path, MAX_PATH);

               TCHAR hashAsString[SHA256_DIGEST_SIZE * 2 + 1];
               DBGetField(result, row, 1, hashAsString, SHA256_DIGEST_SIZE * 2 + 1);
               StrToBin(hashAsString, fi.hash, SHA256_DIGEST_SIZE);

               fi.modTime = static_cast<time_t>(DBGetFieldInt64(result, row, 2));
               fi.permissions = DBGetFieldLong(result, row, 3);
               fi.checkPassed = false;
               s_files.set(path, new FileInfo(fi));
            }
         }
         success = true;
         DBFreeResult(result);
      }
   }
   return success;
}

/**
 * Process regular file
 */
static void ProcessFile(const TCHAR *path, const NX_STAT_STRUCT *stat)
{
   FileInfo *oldFi = s_files.get(path);

   if (oldFi != nullptr && oldFi->checkPassed) // File was allready checked during this iteration
      return;

   BYTE newHash[SHA256_DIGEST_SIZE];
   CalculateFileSHA256Hash(path, newHash);

   if (oldFi == nullptr) // New file
   {
      FileInfo fi;
      memcpy(fi.hash, newHash, SHA256_DIGEST_SIZE);
      fi.modTime = stat->st_mtime;
      fi.permissions = stat->st_mode;
      fi.checkPassed = true;
      s_files.set(path, new FileInfo(fi));
      SaveToDB(path, &fi);
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Detected new file %s"), path);
      PostEvent(EVENT_AGENT_FILE_ADDED, nullptr, 0, StringMap().set(_T("file"), path));
   }
   else
   {
      if ((stat->st_mode != oldFi->permissions) ||
          (stat->st_mtime != oldFi->modTime) ||
          (memcmp(newHash, oldFi->hash, SHA256_DIGEST_SIZE) != 0))
      {
         memcpy(oldFi->hash, newHash, SHA256_DIGEST_SIZE);
         oldFi->modTime = stat->st_mtime;
         oldFi->permissions = stat->st_mode;

         SaveToDB(path, oldFi);
         nxlog_debug_tag(DEBUG_TAG, 4, _T("Detected change in file %s"), path);
         PostEvent(EVENT_AGENT_FILE_CHANGED, nullptr, 0, StringMap().set(_T("file"), path));
      }
      oldFi->checkPassed = true;
   }
}

static void CheckFilesFromDirTree(TCHAR *path);

/**
 * Process directory
 */
static void ProcessDirectory(TCHAR *path)
{
   size_t rootPathLen = _tcslen(path);
   if (rootPathLen >= MAX_PATH - 2)
      return; // Path is too long

   _TDIR *dir = _topendir(path);
   if (dir == nullptr)
      return;

   path[rootPathLen++] = FS_PATH_SEPARATOR_CHAR;

   while (true)
   {
      struct _tdirent *e = _treaddir(dir);
      if (e == nullptr)
         break;

      if (!_tcscmp(e->d_name, _T(".")) || !_tcscmp(e->d_name, _T("..")))
         continue;

      _tcslcpy(&path[rootPathLen], e->d_name, MAX_PATH - rootPathLen);
      CheckFilesFromDirTree(path);
   }

   _tclosedir(dir);
}

/**
 * Searches given path for files, detects changes and updates file info.
 */
static void CheckFilesFromDirTree(TCHAR *path)
{
   NX_STAT_STRUCT stat;
   if (CALL_STAT_FOLLOW_SYMLINK(path, &stat) != 0) // Ingore entity if can't read it's stat
      return;

   switch (stat.st_mode & S_IFMT)
   {
      case S_IFREG: // File
         ProcessFile(path, &stat);
         break;
      case S_IFDIR: // Directory
         ProcessDirectory(path);
         break;
   }
}

/**
 * File monitor worker thread
 */
static void FileMonitoringThread()
{
   nxlog_debug_tag(DEBUG_TAG, 3, _T("File monitor thread started"));
   do
   {
      for (const TCHAR *e : s_rootPaths)
      {
         nxlog_debug_tag(DEBUG_TAG, 7, _T("Checking files in %s"), e);
         TCHAR path[MAX_PATH];
         _tcscpy(path, e);
         CheckFilesFromDirTree(path);
      }

      Iterator<KeyValuePair<FileInfo>> i = s_files.begin();
      while (i.hasNext())
      {
         KeyValuePair<FileInfo> *e = i.next();
         if (e->value->checkPassed)
         {
            e->value->checkPassed = false;
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 4, _T("Detected deletion of file %s"), e->key);
            PostEvent(EVENT_AGENT_FILE_DELETED, nullptr, 0, StringMap().set(_T("file"), e->key));
            DeleteFromDB(e->key);
            i.remove();
         }
      }
   }
   while (!AgentSleepAndCheckForShutdown(s_interval * 1000));
   nxlog_debug_tag(DEBUG_TAG, 3, _T("File monitor thread stopped"));
}

/**
 * Start file monitor (detects changes in files under given directories)
 */
void StartFileMonitor(const shared_ptr<Config>& config)
{
   ConfigEntry *paths = config->getEntry(_T("/FileMonitor/Path"));
   if (paths == nullptr)
   {
      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Path list for file monitor is empty"));
      return;
   }

   s_interval = config->getValueAsUInt(_T("/FileMonitor/Interval"), s_interval);
   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("File check interval set to %d"), s_interval);

   for (int i = 0; i < paths->getValueCount(); i++)
   {
      const TCHAR *path = paths->getValue(i);
      s_rootPaths.add(path);
      nxlog_debug_tag(DEBUG_TAG, 2, _T("Path %s added to file monitor"), path);
   }

   if (!LoadFromDB())
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Cannot load file monitor persistent data from agent database"));

   s_monitorThread = ThreadCreateEx(FileMonitoringThread);
}

/**
 * Stops file monitor
 */
void StopFileMonitor()
{
   ThreadJoin(s_monitorThread);
}
