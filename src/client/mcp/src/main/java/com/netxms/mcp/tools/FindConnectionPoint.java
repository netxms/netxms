/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Raden Solutions
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
import org.netxms.client.topology.ConnectionPoint;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.node.ObjectNode;
import com.netxms.mcp.Startup;

/**
 * Implementation of MCP tool to find connection point for a given object using NetXMS topology discovery.
 * This tool wraps the NXCSession.findConnectionPoint method to locate where an object is connected in the network.
 */
public class FindConnectionPoint extends ObjectServerTool
{
   /**
    * @see com.netxms.mcp.tools.ServerTool#getName()
    */
   @Override
   public String getName()
   {
      return "find-connection-point";
   }

   /**
    * @see com.netxms.mcp.tools.ServerTool#getDescription()
    */
   @Override
   public String getDescription()
   {
      return "Finds the connection point (switch port) where a specific network object is connected.\n" +
            "This tool uses NetXMS topology discovery to locate the network device and port where the object is connected.\n";
   }

   /**
    * @see com.netxms.mcp.tools.ServerTool#getSchema()
    */
   @Override
   public String getSchema()
   {
      return new SchemaBuilder()
            .addArgument("object_id", "string", "ID or name of the network object to find connection point for", true)
            .build();
   }

   /**
    * @see com.netxms.mcp.tools.ObjectServerTool#execute(org.netxms.client.objects.AbstractObject, java.util.Map)
    */
   @Override
   protected String execute(AbstractObject object, Map<String, Object> args) throws Exception
   {
      NXCSession session = Startup.getSession();
      ConnectionPoint connectionPoint = session.findConnectionPoint(object.getObjectId());

      // Format response as JSON
      ObjectMapper mapper = new ObjectMapper();
      ObjectNode response = mapper.createObjectNode();

      response.put("object_id", object.getObjectId());
      response.put("object_name", object.getObjectName());

      if (connectionPoint != null)
      {
         response.put("found", true);

         ObjectNode connectionData = mapper.createObjectNode();
         connectionData.put("connection_type", connectionPoint.getType().toString());
         connectionData.put("node_id", connectionPoint.getNodeId());
         connectionData.put("interface_id", connectionPoint.getInterfaceId());
         connectionData.put("interface_index", connectionPoint.getInterfaceIndex());
         connectionData.put("local_node_id", connectionPoint.getLocalNodeId());
         connectionData.put("local_interface_id", connectionPoint.getLocalInterfaceId());
         connectionData.put("is_historical", connectionPoint.isHistorical());

         // Add node and interface names if available
         if (connectionPoint.getNodeId() > 0)
         {
            String nodeName = session.getObjectName(connectionPoint.getNodeId());
            connectionData.put("node_name", nodeName != null ? nodeName : "Unknown");
         }

         if (connectionPoint.getLocalNodeId() > 0)
         {
            String localNodeName = session.getObjectName(connectionPoint.getLocalNodeId());
            connectionData.put("local_node_name", localNodeName != null ? localNodeName : "Unknown");
         }

         if (connectionPoint.getInterfaceId() > 0)
         {
            String interfaceName = session.getObjectName(connectionPoint.getInterfaceId());
            connectionData.put("interface_name", interfaceName != null ? interfaceName : "Unknown");
         }

         if (connectionPoint.getLocalInterfaceId() > 0)
         {
            String localInterfaceName = session.getObjectName(connectionPoint.getLocalInterfaceId());
            connectionData.put("local_interface_name", localInterfaceName != null ? localInterfaceName : "Unknown");
         }

         // Add MAC and IP address information if available
         if (connectionPoint.getLocalMacAddress() != null)
         {
            connectionData.put("local_mac_address", connectionPoint.getLocalMacAddress().toString());
         }

         if (connectionPoint.getLocalIpAddress() != null)
         {
            connectionData.put("local_ip_address", connectionPoint.getLocalIpAddress().getHostAddress());
         }

         response.set("connection_point", connectionData);
      }
      else
      {
         response.put("found", false);
         response.put("message", "No connection point found for the specified object");
      }

      return mapper.writeValueAsString(response);
   }
}
