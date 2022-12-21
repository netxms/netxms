/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2022 Raden Solutions
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

#define DEBUG_TAG _T("file.monitor")

/**
 * Global instance of file monitoring list
 */
FileMonitoringList g_monitoringList;

/**
 * Constructor
 */
FileMonitoringList::FileMonitoringList() : m_mutex(MutexType::FAST)
{
   m_monitoredFiles.setOwner(Ownership::True);
}

/**
 * Add file to list
 */
void FileMonitoringList::addFile(MONITORED_FILE *file, Node *node, const shared_ptr<AgentConnection>& conn)
{
   lock();
   file->session->incRefCount();
   m_monitoredFiles.add(file);
   node->setFileUpdateConnection(conn);
   nxlog_debug_tag(DEBUG_TAG, 5, _T("File monitor registered: node=%s [%u], file=\"%s\""), node->getName(), node->getId(), file->fileName);
   unlock();
}

/**
 * Check if given file is a duplicate
 */
bool FileMonitoringList::isDuplicate(MONITORED_FILE *file)
{
   bool result = false;
   lock();
   for(int i = 0; i < m_monitoredFiles.size(); i++)
   {
      MONITORED_FILE *curr = m_monitoredFiles.get(i);
      if (!_tcscmp(curr->fileName, file->fileName) && (curr->nodeID == file->nodeID) && (curr->session->getUserId() == file->session->getUserId()))
      {
         result = true;
         break;
      }
   }
   unlock();
   return result;
}

/**
 * Find client sessions
 */
unique_ptr<ObjectArray<ClientSession>> FileMonitoringList::findClientByFNameAndNodeID(const TCHAR *fileName, uint32_t nodeID)
{
   auto result = new ObjectArray<ClientSession>(0, 16);
   lock();
   for(int i = 0; i < m_monitoredFiles.size(); i++)
   {
      MONITORED_FILE *curr = m_monitoredFiles.get(i);
      nxlog_debug_tag(DEBUG_TAG, 9, _T("FileMonitoringList::findClientByFNameAndNodeID: current=%s requested=%s"), curr->fileName, fileName);
      if (!_tcscmp(curr->fileName, fileName) && (curr->nodeID == nodeID))
      {
         result->add(curr->session);
         nxlog_debug_tag(DEBUG_TAG, 9, _T("FileMonitoringList::findClientByFNameAndNodeID: added session %d"), curr->session->getId());
      }
   }
   unlock();
   return unique_ptr<ObjectArray<ClientSession>>(result);
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
   unlock();

   if (deleted)
   {
      shared_ptr<Node> node = static_pointer_cast<Node>(FindObjectById(file->nodeID, OBJECT_NODE));
      if (node != nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("File tracker unregistered: node=%s [%u], file=\"%s\""), node->getName(), node->getId(), file->fileName);
         if (nodeConnectionCount == 1)
            node->clearFileUpdateConnection();
      }
   }

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
      MONITORED_FILE *file = m_monitoredFiles.get(i);
      if (file->nodeID == nodeId)
      {
         NotifyClientSessions(NX_NOTIFY_FILE_MONITORING_FAILED, nodeId);
         file->session->decRefCount();
         m_monitoredFiles.remove(i);
         i--;
      }
   }
   unlock();

   shared_ptr<Node> node = static_pointer_cast<Node>(FindObjectById(nodeId, OBJECT_NODE));
   if (node != nullptr)
   {
      node->clearFileUpdateConnection();
      nxlog_debug_tag(DEBUG_TAG, 5, _T("All file trackers for node %s [%u] unregistered"), node->getName(), node->getId());
   }
}
