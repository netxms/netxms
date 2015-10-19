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
 * Join condition
 */
static CONDITION s_joinCondition = ConditionCreate(TRUE);

/**
 * Change cluster node state
 */
static void ChangeClusterNodeState(ClusterNodeInfo *node, ClusterNodeState state);

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
         g_nxccEventHandler->onMessage(msg, node->m_id);
         delete msg;
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
 * Change cluster node state
 */
static void ChangeClusterNodeState(ClusterNodeInfo *node, ClusterNodeState state)
{
   static const TCHAR *stateNames[] = { _T("DOWN"), _T("CONNECTED"), _T("UP") };

   if (node->m_state == state)
      return;

   node->m_state = state;
   ClusterDebug(1, _T("Cluster node %d [%s] changed state to %s"), node->m_id, (const TCHAR *)node->m_addr->toString(), stateNames[state]);
   switch(state)
   {
      case CLUSTER_NODE_CONNECTED:
         node->m_receiverThread = ThreadCreateEx(ClusterReceiverThread, 0, node);
         break;
      case CLUSTER_NODE_DOWN:
         ThreadJoin(node->m_receiverThread);
         node->m_receiverThread = INVALID_THREAD_HANDLE;
         g_nxccEventHandler->onNodeDisconnect(node->m_id);
         break;
      case CLUSTER_NODE_UP:
         g_nxccEventHandler->onNodeJoin(node->m_id);
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
      ClusterDebug(1, _T("ClusterJoin: listening on port %d"), (int)g_nxccListenPort);
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

   if (ConditionWait(s_joinCondition, 60000))  // wait 1 minute for other nodes to join
   {

   }
   else
   {
      // no other nodes, declare self as master
      g_nxccMasterNode = true;
      ClusterDebug(1, _T("ClusterJoin: cannot contact other nodes, declaring self as master"));
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
            ClusterDebug(5, _T("ClusterKeepaliveThread: send failed for peer %d [%s]"),
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
