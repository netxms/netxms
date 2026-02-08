/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Reden Solutions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package com.netxms.mcp.tools;

import java.util.Map;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Interface;
import org.netxms.client.objects.Node;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.node.ArrayNode;
import com.fasterxml.jackson.databind.node.ObjectNode;
import com.netxms.mcp.Startup;

/**
 * Tool returning list of connected nodes based on peer node information from underlying interfaces
 */
public class ConnectedNodes extends ObjectServerTool
{
   /**
    * @see com.netxms.mcp.tools.ServerTool#getName()
    */
   @Override
   public String getName()
   {
      return "connected-nodes";
   }

   /**
    * @see com.netxms.mcp.tools.ServerTool#getDescription()
    */
   @Override
   public String getDescription()
   {
      return "Returns list of nodes connected to the given node based on peer node information from underlying interfaces.\nThis tool requires a node object ID or name as a parameter.";
   }

   /**
    * @see com.netxms.mcp.tools.ServerTool#getSchema()
    */
   @Override
   public String getSchema()
   {
      return new SchemaBuilder()
            .addArgument("object_id", "string", "ID or name of the node object to retrieve connected nodes from", true)
            .build();
   }

   /**
    * @see com.netxms.mcp.tools.ObjectServerTool#execute(org.netxms.client.objects.AbstractObject, java.util.Map)
    */
   @Override
   protected String execute(AbstractObject object, Map<String, Object> args) throws Exception
   {
      if (!(object instanceof Node))
      {
         throw new IllegalArgumentException("Object must be a node");
      }

      NXCSession session = Startup.getSession();
      ObjectMapper mapper = new ObjectMapper();
      ArrayNode list = mapper.createArrayNode();

      // Get all interfaces for this node
      for(AbstractObject child : object.getAllChildren(AbstractObject.OBJECT_INTERFACE))
      {
         Interface localInterface = (Interface)child;

         // Check if interface has a peer node
         long peerNodeId = localInterface.getPeerNodeId();
         if (peerNodeId != 0)
         {
            AbstractObject peerNode = session.findObjectById(peerNodeId);
            if ((peerNode != null) && (peerNode instanceof Node))
            {
               Node remoteNode = (Node)peerNode;

               // Create entry for this connection
               ObjectNode connectionEntry = mapper.createObjectNode();

               // Add local interface information
               ObjectNode localIfaceEntry = mapper.createObjectNode();
               localIfaceEntry.put("id", localInterface.getObjectId());
               localIfaceEntry.put("name", localInterface.getObjectName());
               localIfaceEntry.put("type", localInterface.getIfTypeName());
               localIfaceEntry.put("description", localInterface.getDescription());
               localIfaceEntry.put("macAddress", (localInterface.getMacAddress() != null) ? localInterface.getMacAddress().toString() : null);
               localIfaceEntry.put("adminState", localInterface.getAdminStateAsText());
               localIfaceEntry.put("operState", localInterface.getOperStateAsText());
               localIfaceEntry.put("speed", localInterface.getSpeed());
               connectionEntry.set("localInterface", localIfaceEntry);

               // Add remote node information
               ObjectNode remoteNodeEntry = mapper.createObjectNode();
               remoteNodeEntry.put("id", remoteNode.getObjectId());
               remoteNodeEntry.put("name", remoteNode.getObjectName());
               remoteNodeEntry.put("type", remoteNode.getObjectClassName());
               remoteNodeEntry.put("status", remoteNode.getStatus().toString());
               remoteNodeEntry.put("primaryIP", (remoteNode.getPrimaryIP() != null) ? remoteNode.getPrimaryIP().getHostAddress() : null);
               connectionEntry.set("remoteNode", remoteNodeEntry);

               // Add remote interface information if available
               long peerInterfaceId = localInterface.getPeerInterfaceId();
               if (peerInterfaceId != 0)
               {
                  AbstractObject peerInterface = session.findObjectById(peerInterfaceId);
                  if (peerInterface instanceof Interface)
                  {
                     Interface remoteInterface = (Interface)peerInterface;
                     ObjectNode remoteIfaceEntry = mapper.createObjectNode();
                     remoteIfaceEntry.put("id", remoteInterface.getObjectId());
                     remoteIfaceEntry.put("name", remoteInterface.getObjectName());
                     remoteIfaceEntry.put("type", remoteInterface.getIfTypeName());
                     remoteIfaceEntry.put("description", remoteInterface.getDescription());
                     remoteIfaceEntry.put("macAddress", (remoteInterface.getMacAddress() != null) ? remoteInterface.getMacAddress().toString() : null);
                     remoteIfaceEntry.put("adminState", remoteInterface.getAdminStateAsText());
                     remoteIfaceEntry.put("operState", remoteInterface.getOperStateAsText());
                     remoteIfaceEntry.put("speed", remoteInterface.getSpeed());
                     connectionEntry.set("remoteInterface", remoteIfaceEntry);
                  }
               }

               list.add(connectionEntry);
            }
         }
      }

      return mapper.writeValueAsString(list);
   }
}