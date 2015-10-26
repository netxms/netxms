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
   ClusterNodeInfo *node = (ClusterNodeInfo *)arg;
   ClusterDebug(4, _T("ClusterNodeJoin: requesting join from from node %d [%s]"), node->m_id, (const TCHAR *)node->m_addr->toString());

   NXCPMessage msg;
   msg.setCode(CMD_JOIN_CLUSTER);
   msg.setField(VID_NODE_ID, g_nxccNodeId);
   msg.setField(VID_IS_MASTER, (INT16)(g_nxccMasterNode ? 1 : 0));

   NXCPMessage *response = ClusterSendDirectCommandEx(node->m_id, &msg);
}

/**
 * Process join request received from other node
 */
void ProcessClusterJoinRequest(ClusterNodeInfo *node, NXCPMessage *msg)
{
   ClusterDebug(4, _T("ProcessClusterJoinRequest: request from node %d [%s]"), node->m_id, (const TCHAR *)node->m_addr->toString());

}
