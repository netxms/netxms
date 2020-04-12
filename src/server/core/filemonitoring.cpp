/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2020 Raden Solutions
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
** File: filemonitoring.cpp
**
**/

#include "nms_core.h"

/**
 * Global instance of file monitoring list
 */
FileMonitoringList g_monitoringList;

/**
 * Constructor
 */
FileMonitoringList::FileMonitoringList()
{
   m_mutex = MutexCreate();
   m_monitoredFiles.setOwner(Ownership::True);
}

/**
 * Destructor
 */
FileMonitoringList::~FileMonitoringList()
{
   MutexDestroy(m_mutex);
}

/**
 * Add file to list
 */
void FileMonitoringList::addFile(MONITORED_FILE *file, Node *node, const shared_ptr<AgentConnection>& conn)
{
   lock();
   file->session->incRefCount();
   m_monitoredFiles.add(file);
   if (!node->hasFileUpdateConnection())
   {
      conn->enableFileUpdates();
      node->setFileUpdateConnection(conn);
   }
   nxlog_debug(5, _T("File tracker registered: node=%s [%d], file=\"%s\""), node->getName(), node->getId(), file->fileName);
   unlock();
}

/**
 * Check if given fiel is a duplicate
 */
bool FileMonitoringList::isDuplicate(MONITORED_FILE *file)
{
   bool result = false;
   lock();
   for(int i = 0; i < m_monitoredFiles.size(); i++)
   {
      MONITORED_FILE *curr = m_monitoredFiles.get(i);
      if(_tcscmp(curr->fileName, file->fileName) == 0
         && curr->nodeID == file->nodeID
         && curr->session->getUserId() == file->session->getUserId())
      {
         result=true;
      }
   }
   unlock();
   return result;
}

/**
 * Find client sessions
 */
ObjectArray<ClientSession> *FileMonitoringList::findClientByFNameAndNodeID(const TCHAR *fileName, UINT32 nodeID)
{
   lock();
   ObjectArray<ClientSession> *result = new ObjectArray<ClientSession>;
   for(int i = 0; i < m_monitoredFiles.size(); i++)
   {
      MONITORED_FILE *curr = m_monitoredFiles.get(i);
      DbgPrintf(9, _T("FileMonitoringList::findClientByFNameAndNodeID: %s is %s should"), curr->fileName, fileName);
      if(_tcscmp(curr->fileName, fileName) == 0 && curr->nodeID == nodeID)
      {
         result->add(curr->session);
      }
   }
   unlock();
   return result;
}

/**
 * Remove file from list
 */
bool FileMonitoringList::removeFile(MONITORED_FILE *file)
{
   lock();
   bool deleted = false;
   int nodeConnectionCount = 0;
   for(int i = 0; i < m_monitoredFiles.size(); i++)
   {
      MONITORED_FILE *curr = m_monitoredFiles.get(i);
      if (curr->nodeID == file->nodeID)
         nodeConnectionCount++;
      if (_tcscmp(curr->fileName, file->fileName) == 0 &&
         curr->nodeID == file->nodeID &&
         curr->session == file->session)
      {
         file->session->decRefCount();
         m_monitoredFiles.remove(i);
         deleted = true;
         i--;
      }
   }

   if (deleted && nodeConnectionCount == 1)
   {
      shared_ptr<Node> node = static_pointer_cast<Node>(FindObjectById(file->nodeID, OBJECT_NODE));
      if (node != nullptr)
         node->clearFileUpdateConnection();
   }
   unlock();
   return deleted;
}

/**
 * Remove all files for disconnected node
 */
void FileMonitoringList::removeDisconnectedNode(uint32_t nodeId)
{
   lock();
   for(int i = 0; i < m_monitoredFiles.size(); i++)
   {
      MONITORED_FILE *m_monitoredFile = m_monitoredFiles.get(i);
      if (m_monitoredFile->nodeID == nodeId)
      {
         NotifyClientSessions(NX_NOTIFY_FILE_MONITORING_FAILED, nodeId);
         m_monitoredFile->session->decRefCount();
         m_monitoredFiles.remove(i);
         i--;
      }
   }
   unlock();
}
