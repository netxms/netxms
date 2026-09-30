/*
** NetXMS - Network Management System
** Copyright (C) 2026 Raden Solutions
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
** File: agent_tunnels.cpp
**
**/

#include "webapi.h"
#include <nxcore_agent_tunnel.h>

/**
 * Handler for GET /v1/agent-tunnels
 */
int H_AgentTunnels(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_AGENT_TUNNELS))
      return 403;

   json_t *output = GetAgentTunnelsAsJson();
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for POST /v1/agent-tunnels/:tunnel-id/bind
 */
int H_AgentTunnelBind(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_AGENT_TUNNELS))
   {
      context->writeAuditLog(AUDIT_SYSCFG, false, 0, L"Access denied on binding agent tunnel");
      return 403;
   }

   uint32_t tunnelId = context->getPlaceholderValueAsUInt32(L"tunnel-id");
   if (tunnelId == 0)
      return 400;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
   {
      context->setErrorResponse("Missing or invalid JSON body");
      return 400;
   }

   uint32_t nodeId = json_object_get_uint32(request, "nodeId");
   shared_ptr<NetObj> node = FindObjectById(nodeId, OBJECT_NODE);
   if (node == nullptr)
   {
      context->setErrorResponse("Invalid node ID");
      return 400;
   }

   uint32_t rcc = BindAgentTunnel(tunnelId, nodeId, context->getUserId());
   switch(rcc)
   {
      case RCC_SUCCESS:
         context->writeAuditLog(AUDIT_SYSCFG, true, nodeId, L"Agent tunnel %u bound to node %s", tunnelId, node->getName());
         return 204;
      case RCC_INVALID_TUNNEL_ID:
         context->setErrorResponse("Unbound tunnel with given ID not found");
         return 404;
      case RCC_INVALID_OBJECT_ID:
         context->setErrorResponse("Invalid node ID");
         return 400;
      case RCC_OUT_OF_STATE_REQUEST:
         context->setErrorResponse("Tunnel cannot be bound in its current state (bind already in progress or certificate is externally provisioned)");
         return 409;
      case RCC_TIMEOUT:
         context->setErrorResponse("Timeout waiting for certificate request completion by agent");
         return 504;
      default:
         context->setErrorResponse("Agent failed to complete certificate request");
         return 500;
   }
}

/**
 * Handler for POST /v1/objects/:object-id/unbind-agent-tunnel
 */
int H_ObjectUnbindAgentTunnel(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_AGENT_TUNNELS))
   {
      context->writeAuditLog(AUDIT_SYSCFG, false, 0, L"Access denied on unbinding agent tunnel");
      return 403;
   }

   uint32_t objectId = context->getPlaceholderValueAsUInt32(L"object-id");
   if (objectId == 0)
      return 400;

   shared_ptr<NetObj> object = FindObjectById(objectId);
   if (object == nullptr)
      return 404;

   if (object->getObjectClass() != OBJECT_NODE)
   {
      context->setErrorResponse("Agent tunnels can be unbound only from nodes");
      return 400;
   }

   uint32_t rcc = UnbindAgentTunnel(objectId, context->getUserId());
   if (rcc != RCC_SUCCESS)
      return 404;  // Only possible failure is node being deleted concurrently

   context->writeAuditLog(AUDIT_SYSCFG, true, objectId, L"Agent tunnel unbound from node %s", object->getName());
   return 204;
}
