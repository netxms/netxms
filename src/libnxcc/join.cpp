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
** File: join.cpp
**
**/

#include "libnxcc.h"

/**
 * Process joining of single cluster node
 */
void ClusterNodeJoin(void *arg)
{
   static const TCHAR *rspNames[] = { _T("ACCEPTED AS SECONDARY"), _T("ACCEPTED AS MASTER"), _T("WAIT FOR MASTER"), _T("SPLIT BRAIN") };

   ClusterNodeInfo *node = (ClusterNodeInfo *)arg;
   ClusterDebug(4, _T("ClusterNodeJoin: requesting join from from node %d [%s]"), node->m_id, (const TCHAR *)node->m_addr->toString());

   NXCPMessage msg;
   msg.setCode(CMD_JOIN_CLUSTER);
   msg.setField(VID_NODE_ID, g_nxccNodeId);
   msg.setField(VID_IS_MASTER, (INT16)(g_nxccMasterNode ? 1 : 0));

   NXCPMessage *response = ClusterSendDirectCommandEx(node->m_id, &msg);
   if (response != NULL)
   {
      ClusterJoinResponse r = (ClusterJoinResponse)response->getFieldAsInt16(VID_RCC);
      ClusterDebug(4, _T("ClusterNodeJoin: join response from node %d [%s]: %s"), node->m_id, (const TCHAR *)node->m_addr->toString(), rspNames[r]);
      switch(r)
      {
         case CJR_ACCEPTED_AS_SECONDARY:
            ChangeClusterNodeState(node, CLUSTER_NODE_UP);
            node->m_master = response->getFieldAsBoolean(VID_IS_MASTER);
            g_nxccNeedSync = true;
            SetJoinCondition();
            break;
         case CJR_ACCEPTED_AS_MASTER:
            ChangeClusterNodeState(node, CLUSTER_NODE_SYNC);
            SetJoinCondition();
            break;
      }
      delete response;
   }
}

/**
 * Process join request received from other node
 */
void ProcessClusterJoinRequest(ClusterNodeInfo *node, NXCPMessage *request)
{
   ClusterDebug(4, _T("ProcessClusterJoinRequest: request from node %d [%s]"), node->m_id, (const TCHAR *)node->m_addr->toString());

   NXCPMessage response;
   response.setCode(CMD_REQUEST_COMPLETED);
   response.setId(request->getId());

   response.setField(VID_IS_MASTER, (INT16)(g_nxccMasterNode ? 1 : 0));

   bool remoteMaster = request->getFieldAsBoolean(VID_IS_MASTER);
   if (g_nxccMasterNode && !remoteMaster)
   {
      response.setField(VID_RCC, (INT16)CJR_ACCEPTED_AS_SECONDARY);
      ClusterDebug(4, _T("ProcessClusterJoinRequest: node %d [%s] accepted into running cluster"), node->m_id, (const TCHAR *)node->m_addr->toString());
      ChangeClusterNodeState(node, CLUSTER_NODE_SYNC);
   }
   else if (!g_nxccMasterNode && !remoteMaster)
   {
      response.setField(VID_RCC, (INT16)CJR_WAIT_FOR_MASTER);
      ClusterDebug(4, _T("ProcessClusterJoinRequest: waiting for master"));
   }
   else if (!g_nxccMasterNode && remoteMaster)
   {
      response.setField(VID_RCC, (INT16)CJR_ACCEPTED_AS_MASTER);
      ClusterDebug(4, _T("ProcessClusterJoinRequest: joined running cluster as secondary with node %d [%s] as master"), node->m_id, (const TCHAR *)node->m_addr->toString());
      node->m_master = true;
      ChangeClusterNodeState(node, CLUSTER_NODE_UP);
      g_nxccNeedSync = true;
      SetJoinCondition();
   }
   else
   {
      response.setField(VID_RCC, (INT16)CJR_SPLIT_BRAIN);
      ClusterDebug(4, _T("ProcessClusterJoinRequest: split-brain condition detected"));
   }

   ClusterSendMessage(node, &response);

   // Promote this node to master if all cluster nodes already connected
   // and there are no master yet (new cluster)
   if (!g_nxccMasterNode && !remoteMaster && ClusterAllNodesConnected())
   {
      PromoteClusterNode();
   }
}

/**
 * Promote cluster node - callback for running on separate thread
 */
static void PromoteClusterNodeCB(void *arg)
{
   for(int i = 0; i < CLUSTER_MAX_NODE_ID; i++)
      if ((g_nxccNodes[i].m_id > 0) && (g_nxccNodes[i].m_id < g_nxccNodeId) && (g_nxccNodes[i].m_state >= CLUSTER_NODE_CONNECTED))
      {
         ClusterDebug(4, _T("PromoteClusterNode: found connected node with higher priority"));
         return;
      }

   ClusterDebug(4, _T("PromoteClusterNode: promote this node to master"));
   g_nxccMasterNode = true;

   NXCPMessage msg;
   msg.setCode(CMD_CLUSTER_NOTIFY);
   msg.setField(VID_NODE_ID, g_nxccNodeId);
   msg.setField(VID_NOTIFICATION_CODE, (INT16)CN_NEW_MASTER);
   ClusterNotify(&msg);

   SetJoinCondition();
}

/**
 * Promote this node to master if possible (always executed on separate thread)
 */
void PromoteClusterNode()
{
   ThreadPoolExecute(g_nxccThreadPool, PromoteClusterNodeCB, NULL);
}
