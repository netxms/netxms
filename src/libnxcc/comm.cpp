/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2015 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: comm.cpp
**
**/

#include "libnxcc.h"

/**
 * Externals
 */
void ClusterNodeJoin(void *arg);
void ProcessClusterJoinRequest(ClusterNodeInfo *node, NXCPMessage *msg);

/**
 * Keepalive interval
 */
#define KEEPALIVE_INTERVAL    200

/**
 * Thread handles
 */
static THREAD s_listenerThread = INVALID_THREAD_HANDLE;
static THREAD s_connectorThread = INVALID_THREAD_HANDLE;
static THREAD s_keepaliveThread = INVALID_THREAD_HANDLE;

/**
 * Command ID
 */
static VolatileCounter s_commandId = 1;

/**
 * Join condition
 */
static CONDITION s_joinCondition = ConditionCreate(TRUE);

/**
 * Process cluster notification
 */
static void ProcessClusterNotification(ClusterNodeInfo *node, ClusterNotificationCode code)
{
   switch(code)
   {
      case CN_NEW_MASTER:
         ClusterDebug(3, _T("Node %d [%s] is new master"), node->m_id, (const TCHAR *)node->m_addr->toString());
         node->m_master = true;
         ChangeClusterNodeState(node, CLUSTER_NODE_UP);
         ConditionSet(s_joinCondition);
         break;
   }
}

/**
 * Node receiver thread
 */
static THREAD_RESULT THREAD_CALL ClusterReceiverThread(void *arg)
{
   ClusterNodeInfo *node = (ClusterNodeInfo *)arg;
   SOCKET s = node->m_socket;
   ClusterDebug(5, _T("Receiver thread started for cluster node %d [%s] on socket %d"), node->m_id, (const TCHAR *)node->m_addr->toString(), (int)s);

   SocketMessageReceiver receiver(s, 8192, 4194304);

   while(!g_nxccShutdown)
   {
      MutexLock(node->m_mutex);
      SOCKET cs = node->m_socket;
      MutexUnlock(node->m_mutex);

      if (cs != s)
         break;   // socket was changed

      MessageReceiverResult result;
      NXCPMessage *msg = receiver.readMessage(KEEPALIVE_INTERVAL * 3, &result);
      if (msg != NULL)
      {
         if (msg->getCode() != CMD_KEEPALIVE)
         {
            TCHAR buffer[128];
            ClusterDebug(7, _T("ClusterReceiverThread: message %s from node %d [%s]"),
               NXCPMessageCodeName(msg->getCode(), buffer), node->m_id, (const TCHAR *)node->m_addr->toString());
         }

         switch(msg->getCode())
         {
            case CMD_CLUSTER_NOTIFY:
               ProcessClusterNotification(node, (ClusterNotificationCode)msg->getFieldAsInt16(VID_NOTIFICATION_CODE));
               break;
            case CMD_JOIN_CLUSTER:
               ProcessClusterJoinRequest(node, msg);
               delete msg;
               if (g_nxccMasterNode)
                  ConditionSet(s_joinCondition);
               break;
            case CMD_KEEPALIVE:
               delete msg;
               break;
            default:
               if (g_nxccEventHandler->onMessage(msg, node->m_id))
                  delete msg;
               else
                  node->m_msgWaitQueue->put(msg);
               break;
         }
      }
      else
      {
         ClusterDebug(5, _T("Receiver error for cluster node %d [%s] on socket %d: %s"),
            node->m_id, (const TCHAR *)node->m_addr->toString(), (int)s, AbstractMessageReceiver::resultToText(result));
         MutexLock(node->m_mutex);
         if (node->m_socket == s)
         {
            shutdown(s, SHUT_RDWR);
            node->m_socket = INVALID_SOCKET;
            ChangeClusterNodeState(node, CLUSTER_NODE_DOWN);
         }
         MutexUnlock(node->m_mutex);
      }
   }

   closesocket(s);
   ClusterDebug(5, _T("Receiver thread stopped for cluster node %d [%s] on socket %d"), node->m_id, (const TCHAR *)node->m_addr->toString(), (int)s);
   return THREAD_OK;
}

/**
 * Find cluster node by IP
 */
static int FindClusterNode(const InetAddress& addr)
{
   for(int i = 0; i < CLUSTER_MAX_NODE_ID; i++)
      if ((g_nxccNodes[i].m_id != 0) && g_nxccNodes[i].m_addr->equals(addr))
         return i;
   return -1;
}

/**
 * Find cluster node by ID
 */
static int FindClusterNode(UINT32 id)
{
   for(int i = 0; i < CLUSTER_MAX_NODE_ID; i++)
      if (g_nxccNodes[i].m_id == id)
         return i;
   return -1;
}

/**
 * Change cluster node state
 */
void ChangeClusterNodeState(ClusterNodeInfo *node, ClusterNodeState state)
{
   static const TCHAR *stateNames[] = { _T("DOWN"), _T("CONNECTED"), _T("SYNC"), _T("UP") };

   if (node->m_state == state)
      return;

   node->m_state = state;
   ClusterDebug(1, _T("Cluster node %d [%s] changed state to %s"), node->m_id, (const TCHAR *)node->m_addr->toString(), stateNames[state]);
   switch(state)
   {
      case CLUSTER_NODE_CONNECTED:
         node->m_receiverThread = ThreadCreateEx(ClusterReceiverThread, 0, node);
         if (node->m_id < g_nxccNodeId)
            ThreadPoolExecute(g_nxccThreadPool, ClusterNodeJoin, node);
         break;
      case CLUSTER_NODE_DOWN:
         ThreadJoin(node->m_receiverThread);
         node->m_receiverThread = INVALID_THREAD_HANDLE;
         g_nxccEventHandler->onNodeDisconnect(node->m_id);
         break;
      case CLUSTER_NODE_SYNC:
         g_nxccEventHandler->onNodeJoin(node->m_id);
         break;
      case CLUSTER_NODE_UP:
         g_nxccEventHandler->onNodeUp(node->m_id);
         break;
   }
}

/**
 * Listener thread
 */
static THREAD_RESULT THREAD_CALL ClusterListenerThread(void *arg)
{
   SOCKET s = CAST_FROM_POINTER(arg, SOCKET);
   while(!g_nxccShutdown)
   {
      struct timeval tv;
      tv.tv_sec = 1;
      tv.tv_usec = 0;

      fd_set rdfs;
      FD_ZERO(&rdfs);
      FD_SET(s, &rdfs);
      int rc = select(SELECT_NFDS(s + 1), &rdfs, NULL, NULL, &tv);
      if ((rc > 0) && !g_nxccShutdown)
      {
         char clientAddr[128];
         socklen_t size = 128;
         SOCKET in = accept(s, (struct sockaddr *)clientAddr, &size);
         if (in == INVALID_SOCKET)
         {
            ClusterDebug(5, _T("ClusterListenerThread: accept() failure"));
            continue;
         }

#ifndef _WIN32
         fcntl(in, F_SETFD, fcntl(in, F_GETFD) | FD_CLOEXEC);
#endif

         InetAddress addr = InetAddress::createFromSockaddr((struct sockaddr *)clientAddr);
         ClusterDebug(5, _T("Incoming connection from %s"), (const TCHAR *)addr.toString());

         int idx = FindClusterNode(addr);
         if (idx == -1)
         {
            ClusterDebug(5, _T("ClusterListenerThread: incoming connection rejected (unknown IP address)"));
            closesocket(in);
            continue;
         }

         MutexLock(g_nxccNodes[idx].m_mutex);
         if (g_nxccNodes[idx].m_socket == INVALID_SOCKET)
         {
            g_nxccNodes[idx].m_socket = in;
            ClusterDebug(5, _T("Cluster peer node %d [%s] connected"),
               g_nxccNodes[idx].m_id, (const TCHAR *)g_nxccNodes[idx].m_addr->toString());
            ChangeClusterNodeState(&g_nxccNodes[idx], CLUSTER_NODE_CONNECTED);
         }
         else
         {
            ClusterDebug(5, _T("Cluster connection from peer %d [%s] discarded because connection already present"),
               g_nxccNodes[idx].m_id, (const TCHAR *)g_nxccNodes[idx].m_addr->toString());
            closesocket(s);
         }
         MutexUnlock(g_nxccNodes[idx].m_mutex);
      }
   }

   closesocket(s);
   ClusterDebug(1, _T("Cluster listener thread stopped"));
   return THREAD_OK;
}

/**
 * Connector thread
 */
static THREAD_RESULT THREAD_CALL ClusterConnectorThread(void *arg)
{
   ClusterDebug(1, _T("Cluster connector thread started"));

   while(!g_nxccShutdown)
   {
      ThreadSleepMs(500);
      if (g_nxccShutdown)
         break;

      for(int i = 0; i < CLUSTER_MAX_NODE_ID; i++)
      {
         MutexLock(g_nxccNodes[i].m_mutex);
         if ((g_nxccNodes[i].m_id != 0) && (g_nxccNodes[i].m_socket == INVALID_SOCKET))
         {
            MutexUnlock(g_nxccNodes[i].m_mutex);
            SOCKET s = ConnectToHost(*g_nxccNodes[i].m_addr, g_nxccNodes[i].m_port, 500);
            MutexLock(g_nxccNodes[i].m_mutex);
            if (s != INVALID_SOCKET)
            {
               if (g_nxccNodes[i].m_socket == INVALID_SOCKET)
               {
                  g_nxccNodes[i].m_socket = s;
                  ClusterDebug(5, _T("Cluster peer node %d [%s] connected"),
                     g_nxccNodes[i].m_id, (const TCHAR *)g_nxccNodes[i].m_addr->toString());
                  ChangeClusterNodeState(&g_nxccNodes[i], CLUSTER_NODE_CONNECTED);
               }
               else
               {
                  ClusterDebug(5, _T("Cluster connection established with peer %d [%s] but discarded because connection already present"),
                     g_nxccNodes[i].m_id, (const TCHAR *)g_nxccNodes[i].m_addr->toString());
                  closesocket(s);
               }
            }
         }
         MutexUnlock(g_nxccNodes[i].m_mutex);
      }
   }

   ClusterDebug(1, _T("Cluster connector thread stopped"));
   return THREAD_OK;
}

/**
 * Cluster keepalive thread
 */
static THREAD_RESULT THREAD_CALL ClusterKeepaliveThread(void *arg)
{
   ClusterDebug(1, _T("Cluster keepalive thread started"));

   NXCPMessage msg;
   msg.setCode(CMD_KEEPALIVE);
   msg.setField(VID_NODE_ID, g_nxccNodeId);
   NXCP_MESSAGE *rawMsg = msg.createMessage();

   while(!g_nxccShutdown)
   {
      ThreadSleepMs(KEEPALIVE_INTERVAL);
      if (g_nxccShutdown)
         break;

      for(int i = 0; i < CLUSTER_MAX_NODE_ID; i++)
      {
         if (g_nxccNodes[i].m_id == 0)
            continue;   // empty slot

         MutexLock(g_nxccNodes[i].m_mutex);
         if (g_nxccNodes[i].m_socket != INVALID_SOCKET)
         {
            if (SendEx(g_nxccNodes[i].m_socket, rawMsg, ntohl(rawMsg->size), 0, NULL) <= 0)
            {
               ClusterDebug(5, _T("ClusterKeepaliveThread: send failed for peer %d [%s]"),
                  g_nxccNodes[i].m_id, (const TCHAR *)g_nxccNodes[i].m_addr->toString());
               shutdown(g_nxccNodes[i].m_socket, SHUT_RDWR);
               g_nxccNodes[i].m_socket = INVALID_SOCKET; // current socket will be closed by receiver
               ChangeClusterNodeState(&g_nxccNodes[i], CLUSTER_NODE_DOWN);
            }
         }
         MutexUnlock(g_nxccNodes[i].m_mutex);
      }
   }

   ClusterDebug(1, _T("Cluster keepalive thread stopped"));
   return THREAD_OK;
}

/**
 * Send message to cluster node
 */
void ClusterSendMessage(ClusterNodeInfo *node, NXCPMessage *msg)
{
   NXCP_MESSAGE *rawMsg = msg->createMessage();
   MutexLock(node->m_mutex);
   if (node->m_socket != INVALID_SOCKET)
   {
      if (SendEx(node->m_socket, rawMsg, ntohl(rawMsg->size), 0, NULL) <= 0)
      {
         ClusterDebug(5, _T("ClusterSendResponse: send failed for peer %d [%s]"), node->m_id, (const TCHAR *)node->m_addr->toString());
         shutdown(node->m_socket, SHUT_RDWR);
         node->m_socket = INVALID_SOCKET; // current socket will be closed by receiver
         ChangeClusterNodeState(node, CLUSTER_NODE_DOWN);
      }
   }
   MutexUnlock(node->m_mutex);
   free(rawMsg);
}

/**
 * Send notification to all connected nodes
 */
void LIBNXCC_EXPORTABLE ClusterNotify(NXCPMessage *msg)
{
   NXCP_MESSAGE *rawMsg = msg->createMessage();

   for(int i = 0; i < CLUSTER_MAX_NODE_ID; i++)
   {
      if (g_nxccNodes[i].m_id == 0)
         continue;   // empty slot

      MutexLock(g_nxccNodes[i].m_mutex);
      if (g_nxccNodes[i].m_socket != INVALID_SOCKET)
      {
         if (SendEx(g_nxccNodes[i].m_socket, rawMsg, ntohl(rawMsg->size), 0, NULL) <= 0)
         {
            ClusterDebug(5, _T("ClusterNotify: send failed for peer %d [%s]"),
               g_nxccNodes[i].m_id, (const TCHAR *)g_nxccNodes[i].m_addr->toString());
            shutdown(g_nxccNodes[i].m_socket, SHUT_RDWR);
            g_nxccNodes[i].m_socket = INVALID_SOCKET; // current socket will be closed by receiver
            ChangeClusterNodeState(&g_nxccNodes[i], CLUSTER_NODE_DOWN);
         }
      }
      MutexUnlock(g_nxccNodes[i].m_mutex);
   }

   free(rawMsg);
}

/**
 * Send command to all connected nodes and wait for response
 *
 * @return number of execution errors
 */
int LIBNXCC_EXPORTABLE ClusterSendCommand(NXCPMessage *msg)
{
   UINT32 requestId = (UINT32)InterlockedIncrement(&s_commandId);
   msg->setId(requestId);
   NXCP_MESSAGE *rawMsg = msg->createMessage();

   bool waitFlags[CLUSTER_MAX_NODE_ID];
   memset(waitFlags, 0, sizeof(waitFlags));

   int errors = 0;
   for(int i = 0; i < CLUSTER_MAX_NODE_ID; i++)
   {
      if (g_nxccNodes[i].m_id == 0)
         continue;   // empty slot

      MutexLock(g_nxccNodes[i].m_mutex);
      if (g_nxccNodes[i].m_socket != INVALID_SOCKET)
      {
         if (SendEx(g_nxccNodes[i].m_socket, rawMsg, ntohl(rawMsg->size), 0, NULL) > 0)
         {
            waitFlags[i] = true;
         }
         else
         {
            ClusterDebug(5, _T("ClusterCommand: send failed for peer %d [%s]"),
               g_nxccNodes[i].m_id, (const TCHAR *)g_nxccNodes[i].m_addr->toString());
            shutdown(g_nxccNodes[i].m_socket, SHUT_RDWR);
            g_nxccNodes[i].m_socket = INVALID_SOCKET; // current socket will be closed by receiver
            ChangeClusterNodeState(&g_nxccNodes[i], CLUSTER_NODE_DOWN);
            errors++;
         }
      }
      MutexUnlock(g_nxccNodes[i].m_mutex);
   }

   free(rawMsg);

   // Collect responses
   for(int i = 0; i < CLUSTER_MAX_NODE_ID; i++)
   {
      if (!waitFlags[i])
         continue;
      NXCPMessage *response = g_nxccNodes[i].m_msgWaitQueue->waitForMessage(CMD_REQUEST_COMPLETED, requestId, g_nxccCommandTimeout);
      if (response != NULL)
      {
         UINT32 rcc = response->getFieldAsInt32(VID_RCC);
         if (rcc != 0)
         {
            ClusterDebug(5, _T("ClusterCommand: failed request to peer %d [%s] RCC=%d"),
               g_nxccNodes[i].m_id, (const TCHAR *)g_nxccNodes[i].m_addr->toString(), rcc);
            errors++;
         }
         delete response;
      }
      else
      {
         ClusterDebug(5, _T("ClusterCommand: timed out request to peer %d [%s]"),
            g_nxccNodes[i].m_id, (const TCHAR *)g_nxccNodes[i].m_addr->toString());
         errors++;
      }
   }

   return errors;
}

/**
 * Send command to specific node and wait for response
 *
 * @return request completion code
 */
UINT32 LIBNXCC_EXPORTABLE ClusterSendDirectCommand(UINT32 nodeId, NXCPMessage *msg)
{
   NXCPMessage *response = ClusterSendDirectCommandEx(nodeId, msg);
   if (response == NULL)
      return NXCC_RCC_TIMEOUT;

   UINT32 rcc = response->getFieldAsUInt32(VID_RCC);
   if (rcc != 0)
   {
      ClusterDebug(5, _T("ClusterDirectCommand: failed request to peer %d: rcc=%d"), nodeId, rcc);
   }
   delete response;
   return rcc;
}

/**
 * Send command to specific node and wait for response
 *
 * @return request completion code
 */
NXCPMessage LIBNXCC_EXPORTABLE *ClusterSendDirectCommandEx(UINT32 nodeId, NXCPMessage *msg)
{
   int index = FindClusterNode(nodeId);
   if (index == -1)
   {
      NXCPMessage *response = new NXCPMessage();
      response->setField(VID_RCC, NXCC_RCC_INVALID_NODE);
      return response;
   }

   ClusterNodeInfo *node = &g_nxccNodes[index];

   UINT32 requestId = (UINT32)InterlockedIncrement(&s_commandId);
   msg->setId(requestId);
   NXCP_MESSAGE *rawMsg = msg->createMessage();

   bool sent = false;
   MutexLock(node->m_mutex);
   if (node->m_socket != INVALID_SOCKET)
   {
      if (SendEx(node->m_socket, rawMsg, ntohl(rawMsg->size), 0, NULL) > 0)
      {
         sent = true;
      }
      else
      {
         ClusterDebug(5, _T("ClusterDirectCommand: send failed for peer %d [%s]"), nodeId, (const TCHAR *)node->m_addr->toString());
         shutdown(node->m_socket, SHUT_RDWR);
         node->m_socket = INVALID_SOCKET; // current socket will be closed by receiver
         ChangeClusterNodeState(node, CLUSTER_NODE_DOWN);
      }
   }
   MutexUnlock(node->m_mutex);

   free(rawMsg);

   // Wait for responses
   NXCPMessage *response;
   if (sent)
   {
      response = node->m_msgWaitQueue->waitForMessage(CMD_REQUEST_COMPLETED, requestId, g_nxccCommandTimeout);
   }
   else
   {
      response = new NXCPMessage();
      response->setField(VID_RCC, NXCC_RCC_COMM_FAILURE);
   }

   return response;
}

/**
 * Send response to cluster peer
 */
void LIBNXCC_EXPORTABLE ClusterSendResponse(UINT32 nodeId, UINT32 requestId, UINT32 rcc)
{
   int index = FindClusterNode(nodeId);
   if (index == -1)
      return;

   ClusterNodeInfo *node = &g_nxccNodes[index];

   NXCPMessage msg;
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(requestId);

   ClusterSendMessage(node, &msg);
}

/**
 * Check if all cluster nodes connected
 */
bool LIBNXCC_EXPORTABLE ClusterAllNodesConnected()
{
   int total = 0, connected = 0;
   for(int i = 0; i < CLUSTER_MAX_NODE_ID; i++)
      if (g_nxccNodes[i].m_id > 0)
      {
         total++;
         if (g_nxccNodes[i].m_state >= CLUSTER_NODE_CONNECTED)
            connected++;
      }
   return total == connected;
}

/**
 * Join cluster
 *
 * @return true on successful join
 */
bool LIBNXCC_EXPORTABLE ClusterJoin()
{
   if (!g_nxccInitialized)
      return false;

   SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
   if (s == INVALID_SOCKET)
   {
      ClusterDebug(1, _T("ClusterJoin: cannot create socket"));
      return false;
   }

   SetSocketExclusiveAddrUse(s);
   SetSocketReuseFlag(s);

   struct sockaddr_in servAddr;
   memset(&servAddr, 0, sizeof(struct sockaddr_in));
   servAddr.sin_family = AF_INET;
   servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
   servAddr.sin_port = htons((UINT16)(47000 + g_nxccNodeId));
   if (bind(s, (struct sockaddr *)&servAddr, sizeof(struct sockaddr_in)) != 0)
   {
      ClusterDebug(1, _T("ClusterJoin: cannot bind listening socket (%s)"), _tcserror(WSAGetLastError()));
      closesocket(s);
      return false;
   }

   if (listen(s, SOMAXCONN) == 0)
   {
      ClusterDebug(1, _T("ClusterJoin: listening on port %d"), (int)ntohs(servAddr.sin_port));
   }
   else
   {
      ClusterDebug(1, _T("ClusterJoin: listen() failed (%s)"), _tcserror(WSAGetLastError()));
      closesocket(s);
      return false;
   }

   s_listenerThread = ThreadCreateEx(ClusterListenerThread, 0, CAST_TO_POINTER(s, void *));
   s_connectorThread = ThreadCreateEx(ClusterConnectorThread, 0, NULL);
   s_keepaliveThread = ThreadCreateEx(ClusterKeepaliveThread, 0, NULL);

   ClusterDebug(1, _T("ClusterJoin: waiting for other nodes"));
   if (ConditionWait(s_joinCondition, 60000))  // wait 1 minute for other nodes to join
   {
      ClusterDebug(1, _T("ClusterJoin: successfully joined"));
   }
   else
   {
      // no other nodes, declare self as master
      ClusterDebug(1, _T("ClusterJoin: cannot contact other nodes, declaring self as master"));
      PromoteClusterNode();
   }

   return true;
}

/**
 * Disconnect all sockets
 */
void ClusterDisconnect()
{
   ThreadJoin(s_listenerThread);
   ThreadJoin(s_connectorThread);
   ThreadJoin(s_keepaliveThread);
}
