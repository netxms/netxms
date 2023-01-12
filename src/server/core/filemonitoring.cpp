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
 * File monitor
 */
struct FileMonitor
{
   uuid clientId;   // Monitor ID on client side
   TCHAR *agentId;  // Monitor ID on agent side
   ClientSession *session;
   uint32_t nodeId;

   FileMonitor(const uuid& _clientId, const TCHAR *_agentId, ClientSession *_session, uint32_t _nodeId) : clientId(_clientId)
   {
      agentId = MemCopyString(_agentId);
      session = _session;
      session->incRefCount();
      nodeId = _nodeId;
   }

   ~FileMonitor()
   {
      MemFree(agentId);
      session->decRefCount();
   }
};

/**
 * List of file monitors
 */
ObjectArray<FileMonitor> s_monitors(64, 64, Ownership::True);

/**
 * Monitor list lock
 */
Mutex s_monitorLock;

/**
 * Create new file monitor. Returns assigned client side ID.
 */
void AddFileMonitor(Node *node, const shared_ptr<AgentConnection>& conn, ClientSession *session, const TCHAR *agentId, const uuid& clientId)
{
   auto monitor = new FileMonitor(clientId, agentId, session, node->getId());

   s_monitorLock.lock();
   s_monitors.add(monitor);
   node->setFileUpdateConnection(conn);
   s_monitorLock.unlock();

   TCHAR buffer[64];
   nxlog_debug_tag(DEBUG_TAG, 5, _T("File monitor for node %s [%u] with agent ID %s and client ID %s registered"), node->getName(), node->getId(), monitor->agentId, clientId.toString(buffer));
}

/**
 * Check if file monitor with given node and agent ID already active
 */
bool IsFileMonitorActive(uint32_t nodeId, const TCHAR *agentId)
{
   bool found = false;
   s_monitorLock.lock();
   for(int i = 0; i < s_monitors.size(); i++)
   {
      FileMonitor *m = s_monitors.get(i);
      if ((m->nodeId == nodeId) && !_tcscmp(m->agentId, agentId))
      {
         found = true;
         break;
      }
   }
   s_monitorLock.unlock();
   return found;
}

/**
 * Find client sessions
 */
unique_ptr<StructArray<std::pair<ClientSession*, uuid>>> FindFileMonitoringSessions(const TCHAR *agentId)
{
   auto result = new StructArray<std::pair<ClientSession*, uuid>>(0, 16);
   s_monitorLock.lock();
   for(int i = 0; i < s_monitors.size(); i++)
   {
      FileMonitor *curr = s_monitors.get(i);
      nxlog_debug_tag(DEBUG_TAG, 9, _T("FileMonitoringList::findClientsByAgentId: current=%s requested=%s"), curr->agentId, agentId);
      if (!_tcscmp(curr->agentId, agentId))
      {
         result->add(std::pair<ClientSession*, uuid>(curr->session, curr->clientId));
         nxlog_debug_tag(DEBUG_TAG, 9, _T("FileMonitoringList::findClientsByAgentId: added session %d"), curr->session->getId());
      }
   }
   s_monitorLock.unlock();
   return unique_ptr<StructArray<std::pair<ClientSession*, uuid>>>(result);
}

/**
 * Remove all file moniotrs with given agent side ID. If session ID is valid (not set to -1) only monitors with matching session ID will be removed.
 */
bool RemoveFileMonitorsByAgentId(const TCHAR *agentId, session_id_t sessionId)
{
   s_monitorLock.lock();

   bool deleted = false;
   shared_ptr<Node> node;
   for(int i = 0; i < s_monitors.size(); i++)
   {
      FileMonitor *curr = s_monitors.get(i);
      if (!_tcscmp(curr->agentId, agentId) && ((sessionId == -1) || (sessionId == curr->session->getId())))
      {
         if (node == nullptr)
            node = static_pointer_cast<Node>(FindObjectById(curr->nodeId, OBJECT_NODE));

         TCHAR buffer[64];
         if (node != nullptr)
            nxlog_debug_tag(DEBUG_TAG, 5, _T("File monitor for node %s [%u] with agent ID %s and client ID %s unregistered"), node->getName(), node->getId(), agentId, curr->clientId.toString(buffer));
         else
            nxlog_debug_tag(DEBUG_TAG, 5, _T("File monitor for unknown node [%u] with agent ID %s and client ID %s unregistered"), curr->nodeId, agentId, curr->clientId.toString(buffer));

         s_monitors.remove(i);
         i--;
         deleted = true;
      }
   }

   // Check if file update connection with agent should be cleared
   bool clearConnection = true;
   if (deleted)
   {
      for(int i = 0; i < s_monitors.size(); i++)
      {
         if (s_monitors.get(i)->nodeId == node->getId())
         {
            clearConnection = false;
            break;
         }
      }
   }

   s_monitorLock.unlock();

   if (node != nullptr)
   {
      shared_ptr<AgentConnectionEx> conn = node->getAgentConnection();
      if (conn != nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("Canceling file monitor on node %s [%u] with ID %s"), node->getName(), node->getId(), agentId);
         conn->cancelFileMonitoring(agentId);
      }
      if (clearConnection)
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("Closing file monitor connection for node %s [%u]"), node->getName(), node->getId());
         node->clearFileUpdateConnection();
      }
   }

   return deleted;
}

/**
 * Remove all file monitors for disconnected client session.
 */
void RemoveFileMonitorsBySessionId(session_id_t sessionId)
{
   StructArray<uuid> monitors(0, 16);
   s_monitorLock.lock();
   for(int i = 0; i < s_monitors.size(); i++)
   {
      FileMonitor *curr = s_monitors.get(i);
      if (sessionId == curr->session->getId())
         monitors.add(curr->clientId);
   }
   s_monitorLock.unlock();

   for(int i = 0; i < monitors.size(); i++)
      RemoveFileMonitorByClientId(*monitors.get(i), sessionId);
}

/**
 * Remove file monitor with given client side ID. If session ID is valid (not set to -1) only monitor with matching session ID will be removed.
 */
bool RemoveFileMonitorByClientId(const uuid& clientId, session_id_t sessionId)
{
   TCHAR agentId[64];
   shared_ptr<Node> node;
   bool deleted = false;

   s_monitorLock.lock();

   for(int i = 0; i < s_monitors.size(); i++)
   {
      FileMonitor *curr = s_monitors.get(i);
      if (curr->clientId.equals(clientId) && ((sessionId == -1) || (sessionId == curr->session->getId())))
      {
         node = static_pointer_cast<Node>(FindObjectById(curr->nodeId, OBJECT_NODE));
         _tcscpy(agentId, curr->agentId);

         TCHAR buffer[64];
         if (node != nullptr)
            nxlog_debug_tag(DEBUG_TAG, 5, _T("File monitor for node %s [%u] with agent ID %s and client ID %s unregistered"), node->getName(), node->getId(), agentId, clientId.toString(buffer));
         else
            nxlog_debug_tag(DEBUG_TAG, 5, _T("File monitor for unknown node [%u] with agent ID %s and client ID %s unregistered"), curr->nodeId, agentId, clientId.toString(buffer));

         s_monitors.remove(i);
         i--;
         deleted = true;
         break;   // Client side IDs are unique
      }
   }

   // Check if file monitor should be cancelled and if file update connection with agent should be cleared
   bool clearConnection = deleted;
   bool cancelMonitor = deleted;
   if (deleted)
   {
      for(int i = 0; i < s_monitors.size(); i++)
      {
         if (s_monitors.get(i)->nodeId == node->getId())
         {
            clearConnection = false;
            if (!_tcscmp(agentId, s_monitors.get(i)->agentId))
            {
               cancelMonitor = false;
               break;
            }
         }
      }
   }

   s_monitorLock.unlock();

   if (cancelMonitor && (node != nullptr))
   {
      shared_ptr<AgentConnectionEx> conn = node->getAgentConnection();
      if (conn != nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("Cancelling file monitor on node %s [%u] with ID %s because no valid client sessions left"), node->getName(), node->getId(), agentId);
         conn->cancelFileMonitoring(agentId);
      }
      if (clearConnection)
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("Closing file monitor connection for node %s [%u]"), node->getName(), node->getId());
         node->clearFileUpdateConnection();
      }
   }

   return deleted;
}

/**
 * Remove all files for disconnected node
 */
void RemoveFileMonitorsByNodeId(uint32_t nodeId)
{
   s_monitorLock.lock();
   for(int i = 0; i < s_monitors.size(); i++)
   {
      FileMonitor *curr = s_monitors.get(i);
      if (curr->nodeId == nodeId)
      {
         s_monitors.remove(i);
         i--;
      }
   }
   s_monitorLock.unlock();

   NotifyClientSessions(NX_NOTIFY_FILE_MONITORING_FAILED, nodeId);

   shared_ptr<Node> node = static_pointer_cast<Node>(FindObjectById(nodeId, OBJECT_NODE));
   if (node != nullptr)
   {
      node->clearFileUpdateConnection();
      nxlog_debug_tag(DEBUG_TAG, 5, _T("All file monitors for node %s [%u] unregistered"), node->getName(), node->getId());
   }
}
