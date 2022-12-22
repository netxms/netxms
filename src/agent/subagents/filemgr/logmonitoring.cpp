/*
 ** File management subagent
 ** Copyright (C) 2014-2022 Raden Solutions
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
#endif

/**
 * Max NXCP message size
 */
#define MAX_MSG_SIZE    262144

/**
 * Add file to list
 */
void MonitoredFileList::add(const TCHAR *fileName)
{
   lock();

   bool alreadyMonitored = false;
   for(int i = 0; i < m_files.size(); i++)
   {
      MONITORED_FILE *file = m_files.get(i);
      if (!_tcscmp(file->fileName, fileName))
      {
         alreadyMonitored = true;
         file->monitorCount++;
         nxlog_debug_tag(DEBUG_TAG, 6, _T("MonitoredFileList::add: new reference count for file monitor %s is %d"), fileName, file->monitorCount);
         break;
      }
   }

   if (!alreadyMonitored)
   {
      MONITORED_FILE *file = new MONITORED_FILE();
      _tcscpy(file->fileName, fileName);
      file->monitorCount = 1;
      m_files.add(file);
      nxlog_debug_tag(DEBUG_TAG, 6, _T("MonitoredFileList::add: added new file monitor %s"), fileName);
   }

   unlock();
}

/**
 * Check if file list contains given file
 */
bool MonitoredFileList::contains(const TCHAR *fileName)
{
   lock();
   bool result = false;
   for(int i = 0; i < m_files.size() ; i++)
   {
      if (!_tcscmp(m_files.get(i)->fileName, fileName))
      {
         result = true;
         break;
      }
   }
   unlock();
   return result;
}

/**
 * Remove file from list
 */
bool MonitoredFileList::remove(const TCHAR *fileName)
{
   lock();

   bool found = false;
   for(int i = 0; i < m_files.size() ; i++)
   {
      MONITORED_FILE *file = m_files.get(i);
      if (!_tcscmp(file->fileName, fileName))
      {
         found = true;
         file->monitorCount--;
         if (file->monitorCount == 0)
         {
            m_files.remove(i);
            nxlog_debug_tag(DEBUG_TAG, 6, _T("MonitoredFileList::remove: file monitor %s removed"), fileName);
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 6, _T("MonitoredFileList::remove: new reference count for file monitor %s is %d"), fileName, file->monitorCount);
         }
         break;
      }
   }

   if (!found)
      nxlog_debug_tag(DEBUG_TAG, 6, _T("MonitoredFileList::removeMonitoringFile: attempt to remove non-existing file monitor %s"), fileName);

   unlock();
   return found;
}

/**
 * Data for message sending callback
 */
struct SendFileUpdateCallbackData
{
   InetAddress ip;
   NXCPMessage *pMsg;
};

/**
 * Callback to send message to agent communication session's peer
 */
static EnumerationCallbackResult SendFileUpdateCallback(AbstractCommSession *session, void *data)
{
   if (((SendFileUpdateCallbackData *)data)->ip.equals(session->getServerAddress()) && session->canAcceptFileUpdates())
   {
      session->postMessage(((SendFileUpdateCallbackData *)data)->pMsg);
      return _STOP;
   }
   return _CONTINUE;
}

/**
 * Thread sending file updates over NXCP
 */
THREAD_RESULT THREAD_CALL SendFileUpdatesOverNXCP(void *args)
{
   FollowData *flData = (reinterpret_cast<FollowData*>(args));
   int hFile = _topen(flData->getFile(), O_RDONLY | O_BINARY);
   if (hFile == -1)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("SendFileUpdatesOverNXCP: File \"%s\" does not exists or cannot be opened (monitor-id=%s)"), flData->getFile(), flData->getFileId());
      g_monitorFileList.remove(flData->getFileId());
      return THREAD_OK;
   }

   int threadSleepTime = 1;

   NX_STAT_STRUCT st;
   NX_FSTAT(hFile, &st);
   flData->setOffset((long)st.st_size);
   ThreadSleep(threadSleepTime);
   char *content = static_cast<char*>(MemAlloc(65536));

   while (true)
   {
      NX_FSTAT(hFile, &st);
      ssize_t newOffset = static_cast<ssize_t>(st.st_size);
      if (flData->getOffset() < newOffset)
      {
         size_t readSize = newOffset - flData->getOffset();
         for(size_t i = readSize; i > 0; i -= readSize)
         {
            if (readSize > 65535)
            {
               readSize = 65535;
               newOffset = flData->getOffset() + readSize;
            }
            NXCPMessage msg;
            msg.setCode(CMD_FILE_MONITORING);
            msg.setId(0);
            msg.setField(VID_FILE_NAME, flData->getFileId());

#ifdef _WIN32
            _lseeki64(hFile, flData->getOffset(), SEEK_SET);
#else
            _lseek(hFile, flData->getOffset(), SEEK_SET);
#endif
            readSize = _read(hFile, content, static_cast<int>(readSize));
            for(size_t j = 0; j < readSize; j++)
               if (content[j] == 0)
                  content[j] = ' ';
            content[readSize] = 0;
            nxlog_debug_tag(DEBUG_TAG, 6, _T("SendFileUpdatesOverNXCP: %u bytes will be sent"), static_cast<unsigned int>(readSize));
            msg.setFieldFromMBString(VID_FILE_DATA, content);
            flData->setOffset(newOffset);

            SendFileUpdateCallbackData data;
            data.ip = flData->getServerAddress();
            data.pMsg = &msg;

            bool sent = AgentEnumerateSessions(SendFileUpdateCallback, &data);
            if (!sent)
            {
               nxlog_debug_tag(DEBUG_TAG, 4, _T("SendFileUpdatesOverNXCP: Removing file monitor %s for file \"%s\" because of update sending failure"), flData->getFileId(), flData->getFile());
               g_monitorFileList.remove(flData->getFileId());
               break;
            }
         }
      }

      if (!g_monitorFileList.contains(flData->getFileId()))
      {
         break;
      }
      ThreadSleep(threadSleepTime);
   }
   MemFree(content);
   delete flData;
   _close(hFile);
   return THREAD_OK;
}
