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
#include <nxclapi.h>
#include <nxstat.h>


/**
 * File monitoring
 */

struct MONITORED_FILE
{
   TCHAR fileName[MAX_PATH];
   int monitoringCount;
};

struct FollowData
{
   TCHAR *pszFile;
   TCHAR *fileId;
   long offset;
	UINT32 serverAddress;
	AbstractCommSession *session;
};

class MonitoredFileList
{
private:
   MUTEX m_mutex;
   ObjectArray<MONITORED_FILE>  m_monitoredFiles;
   MONITORED_FILE* m_newFile;

public:
   MonitoredFileList();
   ~MonitoredFileList();
   void addMonitoringFile(const TCHAR *fileName);
   bool checkFileMonitored(const TCHAR *fileName);
   bool removeMonitoringFile(const TCHAR *fileName);

private:
   void Lock();
   void Unlock();
};

extern MonitoredFileList g_monitorFileList;

THREAD_RESULT THREAD_CALL SendFileUpdatesOverNXCP(void *arg);

#endif
