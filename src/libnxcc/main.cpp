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
** File: main.cpp
**
**/

#include "libnxcc.h"

/**
 * Global cluster node settings
 */
UINT32 g_nxccNodeId = 0;
ClusterEventHandler *g_nxccEventHandler = NULL;
ClusterNodeState g_nxccState = CLUSTER_NODE_DOWN;
bool g_nxccInitialized = false;
bool g_nxccMasterNode = false;
bool g_nxccShutdown = false;
bool g_nxccNeedSync = false;
UINT16 g_nxccListenPort = 47000;
UINT32 g_nxccCommandTimeout = 500;

/**
 * Other cluster nodes
 */
ClusterNodeInfo g_nxccNodes[CLUSTER_MAX_NODE_ID];

/**
 * Cluster thread pool
 */
ThreadPool *g_nxccThreadPool;

/**
 * Debug callback
 */
static void (*s_debugCallback)(int, const TCHAR *, va_list) = NULL;

/**
 * Set debug callback
 */
void LIBNXCC_EXPORTABLE ClusterSetDebugCallback(void (*cb)(int, const TCHAR *, va_list))
{
   s_debugCallback = cb;
}

/**
 * Debug output
 */
void ClusterDebug(int level, const TCHAR *format, ...)
{
   if (s_debugCallback == NULL)
      return;

   va_list args;
   va_start(args, format);
   s_debugCallback(level, format, args);
   va_end(args);
}

/**
 * Add cluster peer node from config
 */
static bool AddPeerNode(TCHAR *cfg)
{
   TCHAR *s = _tcschr(cfg, _T(':'));
   if (s == NULL)
   {
      ClusterDebug(1, _T("ClusterInit: invalid peer node configuration record \"%s\""), cfg);
      return false;
   }

   *s = 0;
   s++;
   UINT32 id = _tcstol(cfg, NULL, 0);
   if ((id < 1) || (id > CLUSTER_MAX_NODE_ID) || (id == g_nxccNodeId))
   {
      ClusterDebug(1, _T("ClusterInit: invalid peer node ID %d"), id);
      return false;
   }

   g_nxccNodes[id].m_id = id;
   g_nxccNodes[id].m_addr = new InetAddress(InetAddress::resolveHostName(s));
   g_nxccNodes[id].m_port = (UINT16)(47000 + id);
   g_nxccNodes[id].m_socket = INVALID_SOCKET;
   g_nxccNodes[id].m_msgWaitQueue = new MsgWaitQueue();
   ClusterDebug(1, _T("ClusterInit: added peer node %d"), id);
   return true;
}

/**
 * Cluster configuration template
 */
static TCHAR *s_peerNodeList = NULL;
static NX_CFG_TEMPLATE s_clusterConfigTemplate[] =
{
   { _T("CommandTimeout"), CT_LONG, 0, 0, 0, 0, &g_nxccCommandTimeout, NULL },
   { _T("NodeId"), CT_LONG, 0, 0, 0, 0, &g_nxccNodeId, NULL },
   { _T("PeerNode"), CT_STRING_LIST, '\n', 0, 0, 0, &s_peerNodeList, NULL },
   { _T(""), CT_END_OF_LIST, 0, 0, 0, 0, NULL, NULL }
};

/**
 * Init cluster node
 */
bool LIBNXCC_EXPORTABLE ClusterInit(Config *config, const TCHAR *section, ClusterEventHandler *eventHandler)
{
   if (!config->parseTemplate(section, s_clusterConfigTemplate))
      return false;

   if ((g_nxccNodeId < 1) || (g_nxccNodeId > CLUSTER_MAX_NODE_ID))
      return false;

   g_nxccThreadPool = ThreadPoolCreate(1, 16, _T("CLUSTER"));

   memset(g_nxccNodes, 0, sizeof(g_nxccNodes));
   for(int i = 0; i < CLUSTER_MAX_NODE_ID; i++)
   {
      g_nxccNodes[i].m_mutex = MutexCreate();
      g_nxccNodes[i].m_socket = INVALID_SOCKET;
      g_nxccNodes[i].m_receiverThread = INVALID_THREAD_HANDLE;
   }

   if (s_peerNodeList != NULL)
   {
      TCHAR *curr, *next;
      for(curr = next = s_peerNodeList; next != NULL && (*curr != 0); curr = next + 1)
      {
         next = _tcschr(curr, _T('\n'));
         if (next != NULL)
            *next = 0;
         StrStrip(curr);
         if (!AddPeerNode(curr))
         {
            free(s_peerNodeList);
            s_peerNodeList = NULL;
            return false;
         }
      }
      free(s_peerNodeList);
      s_peerNodeList = NULL;
   }

   g_nxccEventHandler = eventHandler;
   g_nxccInitialized = true;
   return true;
}

/**
 * Shutdown cluster
 */
void LIBNXCC_EXPORTABLE ClusterShutdown()
{
   if (!g_nxccInitialized || g_nxccShutdown)
      return;

   g_nxccShutdown = true;
   ClusterDisconnect();

   for(int i = 0; i < CLUSTER_MAX_NODE_ID; i++)
   {
      MutexDestroy(g_nxccNodes[i].m_mutex);
      delete g_nxccNodes[i].m_msgWaitQueue;
   }
}

/**
 * Check if this node is master node
 */
bool LIBNXCC_EXPORTABLE ClusterIsMasterNode()
{
   return g_nxccMasterNode;
}

/**
 * Get ID of current master. Will return 0 if there is no master node at the moment.
 */
UINT32 LIBNXCC_EXPORTABLE ClusterGetMasterNodeId()
{
   if (g_nxccMasterNode)
      return g_nxccNodeId;
   for(int i = 0; i < CLUSTER_MAX_NODE_ID; i++)
   {
      if (g_nxccNodes[i].m_master)
         return g_nxccNodes[i].m_id;
   }
   return 0;
}

/**
 * Check if synchronization needed
 */
bool LIBNXCC_EXPORTABLE ClusterIsSyncNeeded()
{
   return g_nxccNeedSync;
}

/**
 * Get local node ID
 */
UINT32 LIBNXCC_EXPORTABLE ClusterGetLocalNodeId()
{
   return g_nxccNodeId;
}

#ifdef _WIN32

/**
 * DLL entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
      DisableThreadLibraryCalls(hInstance);
   return TRUE;
}

#endif   /* _WIN32 */
