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
import org.netxms.base.MacAddress;
import org.netxms.client.NXCSession;
import org.netxms.client.topology.ConnectionPoint;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.node.ObjectNode;
import com.netxms.mcp.Startup;

/**
 * Implementation of MCP tool to find MAC address in the network using NetXMS topology discovery.
 */
public class FindMACAddress extends ServerTool
{
   /**
    * @see com.netxms.mcp.tools.ServerTool#getName()
    */
   @Override
   public String getName()
   {
      return "find-mac-address";
   }

   /**
    * @see com.netxms.mcp.tools.ServerTool#getDescription()
    */
   @Override
   public String getDescription()
   {
      return "Finds the connection point (switch port) where a device with given MAC address is connected.\n" + "This tool uses NetXMS topology discovery to locate the network device and port.\n";
   }

   /**
    * @see com.netxms.mcp.tools.ServerTool#getSchema()
    */
   @Override
   public String getSchema()
   {
      return new SchemaBuilder()
            .addArgument("mac_address", "string", "MAC address to find (e.g., 00:11:22:33:44:55)", true)
            .build();
   }

   /**
    * @see com.netxms.mcp.tools.ServerTool#execute(java.util.Map)
    */
   @Override
   public String execute(Map<String, Object> args) throws Exception
   {
      NXCSession session = Startup.getSession();

      String macAddressStr = (String)args.get("mac_address");
      if ((macAddressStr == null) || macAddressStr.trim().isEmpty())
         throw new IllegalArgumentException("MAC address not specified");

      // Parse will throw exception if format is invalid
      MacAddress macAddress = MacAddress.parseMacAddress(macAddressStr.trim());
      ConnectionPoint connectionPoint = session.findConnectionPoint(macAddress);

      // Format response as JSON
      ObjectMapper mapper = new ObjectMapper();
      ObjectNode response = mapper.createObjectNode();

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
         response.put("message", "MAC address not found in topology database");
      }

      return mapper.writeValueAsString(response);
   }
}
