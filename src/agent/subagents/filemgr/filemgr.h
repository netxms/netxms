/*
 ** File management subagent
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

#ifndef _filemgr_h_
#define _filemgr_h_

#include <nms_common.h>
#include <nms_agent.h>
#include <nxcpapi.h>
#include <nxstat.h>

#define DEBUG_TAG _T("sa.filemgr")

/**
 * File monitoring
 */

struct MONITORED_FILE
{
   TCHAR fileName[MAX_PATH];
   int monitoringCount;
};

/**
 * Data for file change follow thread
 */
class FollowData
{
private:
   TCHAR *m_file;
   TCHAR *m_fileId;
   ssize_t m_offset;
   InetAddress m_serverAddress;

public:
   FollowData(const TCHAR *file, const TCHAR *fileId, off_t offset, const InetAddress &address)
   {
      m_file = MemCopyString(file);
      m_fileId = MemCopyString(fileId);
      m_offset = offset;
      m_serverAddress = address;
   }

   ~FollowData()
   {
      MemFree(m_file);
      MemFree(m_fileId);
   }

   const TCHAR *getFile() { return m_file; }
   const TCHAR *getFileId() { return m_fileId; }
   ssize_t getOffset() { return m_offset; }
   const InetAddress& getServerAddress() { return m_serverAddress; }

   void setOffset(ssize_t offset) { m_offset = offset; }
};

struct MessageData
{
   TCHAR *fileName;
   TCHAR *fileNameCode;
   bool follow;
   bool allowCompression;
   uint32_t id;
   long offset;
	shared_ptr<AbstractCommSession> session;

	MessageData(const shared_ptr<AbstractCommSession>& _session) : session(_session)
	{
	   fileName = nullptr;
	   fileNameCode = nullptr;
	   follow = false;
	   allowCompression = false;
	   id = 0;
	   offset = 0;
	}

	~MessageData()
	{
	   MemFree(fileName);
	   MemFree(fileNameCode);
	}
};

/**
 * List of monitored files
 */
class MonitoredFileList
{
private:
   MUTEX m_mutex;
   ObjectArray<MONITORED_FILE> m_files;

   void lock() { MutexLock(m_mutex); }
   void unlock() { MutexUnlock(m_mutex); }

public:
   MonitoredFileList();
   ~MonitoredFileList();

   void add(const TCHAR *fileName);
   bool contains(const TCHAR *fileName);
   bool remove(const TCHAR *fileName);
};

/*
 * Root folders
 */
class RootFolder
{
private:
   TCHAR *m_folder;
   bool m_readOnly;

public:
   RootFolder(const TCHAR *folder);
   ~RootFolder()
   {
      free(m_folder);
   }

   const TCHAR *getFolder() { return m_folder; }
   bool isReadOnly() { return m_readOnly; }
};

/**
 * Global instance of monitored files list
 */
extern MonitoredFileList g_monitorFileList;

THREAD_RESULT THREAD_CALL SendFileUpdatesOverNXCP(void *arg);

#endif
