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

#include "filemgr.h"

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
      AgentWriteDebugLog(6, _T("MonitoredFileList::removeMonitoringFile: attempt to delete non-existing file %s"), fileName);
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

/**
 * Data for message sending callback
 */
struct SendMessageData
{
   InetAddress ip;
   NXCPMessage *pMsg;
};

/**
 * Callback to send message to agent communication session's peer
 */
static bool SendMessage(AbstractCommSession *session, void *data)
{
   if (session != NULL && ((SendMessageData *)data)->ip.equals(session->getServerAddress()))
   {
      session->sendMessage(((SendMessageData *)data)->pMsg);
      return false;
   }
   return true;
};

/**
 * Thread sending file updates over NXCP
 */
THREAD_RESULT THREAD_CALL SendFileUpdatesOverNXCP(void *args)
{
   FollowData *flData = (reinterpret_cast<FollowData*>(args));
   int hFile, threadSleepTime = 1;
   BYTE* readBytes = NULL;
   BOOL bResult = FALSE;
   NXCPMessage *pMsg;
   UINT32 readSize;

   bool follow = true;
   hFile = _topen(flData->getFile(), O_RDONLY | O_BINARY);
   NX_STAT_STRUCT st;
   NX_FSTAT(hFile, &st);
   flData->setOffset((long)st.st_size);
   ThreadSleep(threadSleepTime);
   int headerSize = NXCP_HEADER_SIZE + MAX_PATH*2;

   if (hFile == -1)
   {
      AgentWriteDebugLog(6, _T("SendFileUpdatesOverNXCP: File does not exists or couldn't be opened. File: %s."), flData->getFile());
      g_monitorFileList.removeMonitoringFile(flData->getFile());
      return THREAD_OK;
   }
   while (follow)
   {
      NX_FSTAT(hFile, &st);
      long newOffset = (long)st.st_size;
      if (flData->getOffset() < newOffset)
      {
         readSize = newOffset - flData->getOffset();
         for(int i = readSize; i > 0; i = i - readSize)
         {
            if(readSize+1+headerSize > MAX_MSG_SIZE)
            {
               readSize = MAX_MSG_SIZE - headerSize;
               newOffset = flData->getOffset() + readSize;
            }
            pMsg = new NXCPMessage();
            pMsg->setCode(CMD_FILE_MONITORING);
            pMsg->setId(0);
            pMsg->setField(VID_FILE_NAME, flData->getFileId(), MAX_PATH);

            lseek(hFile, flData->getOffset(), SEEK_SET);
            readBytes = (BYTE*)malloc(readSize);
            readSize = read(hFile, readBytes, readSize);
            AgentWriteDebugLog(6, _T("SendFileUpdatesOverNXCP: %d bytes will be sent."), readSize);
#ifdef UNICODE
            TCHAR *text = WideStringFromMBString((char *)readBytes);
            pMsg->setField(VID_FILE_DATA, text, readSize);
            free(text);
#else
            pMsg->setField(VID_FILE_DATA, (char *)readBytes, readSize);
#endif
            flData->setOffset(newOffset);

            SendMessageData data;
            data.ip = flData->getServerAddress();
            data.pMsg = pMsg;

            bool sent = EnumerateSessions(SendMessage, &data);

            if (!sent)
            {
               g_monitorFileList.removeMonitoringFile(flData->getFileId());
            }

            safe_free(readBytes);
            delete pMsg;
         }
      }

      ThreadSleep(threadSleepTime);
      if (!g_monitorFileList.checkFileMonitored(flData->getFileId()))
      {
         follow = false;
      }
   }
   delete flData;
   close(hFile);
   return THREAD_OK;
};
