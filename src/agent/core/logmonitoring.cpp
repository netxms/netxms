/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2014 Raden Solutions
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
** File: logmonitoring.cpp
**
**/

#include "nxagentd.h"
#include <nxstat.h>

#ifdef _WIN32
#include <share.h>

#define read _read
#define close _close

#endif

/**
 * Max NXCP message size
 */
#define MAX_MSG_SIZE    262144

MonitoredFileList::MonitoredFileList()
{
   m_mutex = MutexCreate();
   m_monitoredFiles.setOwner(true);
};

MonitoredFileList::~MonitoredFileList()
{
   MutexDestroy(m_mutex);
};

void MonitoredFileList::addMonitoringFile(const TCHAR *fileName)
{
   Lock();
   bool alreadyMonitored = false;
   for(int i = 0; i < m_monitoredFiles.size(); i++)
   {
      m_newFile = m_monitoredFiles.get(i);
      if (_tcscmp(m_newFile->fileName, fileName) == 0)
      {
         alreadyMonitored = true;
         m_newFile->monitoringCount++;
      }
   }

   if(!alreadyMonitored)
   {
      m_newFile = new MONITORED_FILE();
      _tcscpy(m_newFile->fileName, fileName);
      m_newFile->monitoringCount = 1;
      m_monitoredFiles.add(m_newFile);
   }
   Unlock();
};

bool MonitoredFileList::checkFileMonitored(const TCHAR *fileName)
{
   Lock();
   bool result = false;
   for(int i = 0; i < m_monitoredFiles.size() ; i++)
   {
      m_newFile = m_monitoredFiles.get(i);
      if (_tcscmp(m_newFile->fileName, fileName) == 0)
      {
         result = true;
      }
   }
   Unlock();
   return result;
};

bool MonitoredFileList::removeMonitoringFile(const TCHAR *fileName)
{
   Lock();
   bool alreadyMonitored = false;
   for(int i = 0; i < m_monitoredFiles.size() ; i++)
   {
      m_newFile = m_monitoredFiles.get(i);
      if (_tcscmp(m_newFile->fileName, fileName) == 0)
      {
         alreadyMonitored = true;
         m_newFile->monitoringCount--;
         if(0 == m_newFile->monitoringCount)
         {
            m_monitoredFiles.remove(i);
         }
      }
   }

   if(!alreadyMonitored)
   {
      DebugPrintf(INVALID_INDEX, 6, _T("MonitoredFileList::removeMonitoringFile: Tryed to delete not existing file %s."), fileName);
   }
   Unlock();
   return alreadyMonitored;
};

void MonitoredFileList::Lock()
{
   MutexLock(m_mutex);
};

void MonitoredFileList::Unlock()
{
   MutexUnlock(m_mutex);
};

THREAD_RESULT THREAD_CALL SendFileUpdatesOverNXCP(void *args)
{
   FollowData *flData = (reinterpret_cast<FollowData*>(args));
   int hFile, threadSleepTime = 1;
   BYTE* readBytes = NULL;
   BOOL bResult = FALSE;
   CSCPMessage *pMsg;
   UINT32 readSize;

   bool follow = true;
   hFile = _topen(flData->pszFile, O_RDONLY | O_BINARY);
   NX_STAT_STRUCT st;
   NX_FSTAT(hFile, &st);
   flData->offset = (long)st.st_size;
   ThreadSleep(threadSleepTime);
   int headerSize = CSCP_HEADER_SIZE + MAX_PATH*2;

   if (hFile == -1)
   {
      DebugPrintf(INVALID_INDEX, 6, _T("SendFileUpdatesOverNXCP: File does not exists or couldn't be opened. File: %s."), flData->pszFile);
      g_monitorFileList.removeMonitoringFile(flData->pszFile);
      return THREAD_OK;
   }
   while (follow)
   {
      NX_FSTAT(hFile, &st);
      long newOffset = (long)st.st_size;
      if(flData->offset < newOffset)
      {
         readSize = newOffset - flData->offset;
         for(int i = readSize; i > 0; i = i - readSize)
         {
            if(readSize+1+headerSize > MAX_MSG_SIZE)
            {
               readSize = MAX_MSG_SIZE - headerSize;
               newOffset = flData->offset + readSize;
            }
            pMsg = new CSCPMessage();
            pMsg->SetCode(CMD_FILE_MONITORING);
            pMsg->SetId(0);
            pMsg->SetVariable(VID_FILE_NAME, flData->pszFile, MAX_PATH);

            lseek(hFile, flData->offset, SEEK_SET);
            readBytes = (BYTE*)malloc(readSize);
            readSize = read(hFile, readBytes, readSize);
            DebugPrintf(INVALID_INDEX, 6, _T("SendFileUpdatesOverNXCP: %d bytes will be sent."), readSize);
#ifdef UNICODE
            TCHAR* text;
            text = WideStringFromMBString((char*)readBytes);
            pMsg->SetVariable(VID_FILE_DATA, text, readSize);
#else
            pMsg->SetVariable(VID_FILE_DATA, (TCHAR*)readBytes, readSize);
#endif


            flData->offset = newOffset;

            MutexLock(g_hSessionListAccess);
            for(UINT32 i = 0; i < g_dwMaxSessions; i++)
            {
               if (g_pSessionList[i] != NULL)
               {
                  g_pSessionList[i]->sendMessage(pMsg);
                  break;
               }
            }
            MutexUnlock(g_hSessionListAccess);

            safe_free(readBytes);
            delete pMsg;
         }
      }

      ThreadSleep(threadSleepTime);
      if(!g_monitorFileList.checkFileMonitored(flData->pszFile))
      {
         follow = false;
      }
   }
   delete flData->pszFile;
   delete flData;
   close(hFile);
   return THREAD_OK;
};
