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
import java.util.Set;
import org.netxms.base.InetAddressEx;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Interface;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.node.ArrayNode;
import com.fasterxml.jackson.databind.node.ObjectNode;

/**
 * MCP tool for retrieving network interfaces from a node
 */
public class InterfaceList extends ObjectServerTool
{
   /**
    * @see com.netxms.mcp.tools.ServerTool#getName()
    */
   @Override
   public String getName()
   {
      return "interface-list";
   }

   /**
    * @see com.netxms.mcp.tools.ServerTool#getDescription()
    */
   @Override
   public String getDescription()
   {
      return "Returns list of network interfaces on given node (server, workstation, or device).\nThis tool requires a node object ID or name as a parameter.";
   }

   /**
    * @see com.netxms.mcp.tools.ServerTool#getSchema()
    */
   @Override
   public String getSchema()
   {
      return new SchemaBuilder()
         .addArgument("object_id", "string", "ID or name of the node object to retrieve interface list from", true)
         .build();
   }

   /**
    * @see com.netxms.mcp.tools.ObjectServerTool#execute(org.netxms.client.objects.AbstractObject, java.util.Map)
    */
   @Override
   protected String execute(AbstractObject object, Map<String, Object> args) throws Exception
   {
      // Get all child objects of type Interface
      Set<AbstractObject> interfaces = object.getAllChildren(AbstractObject.OBJECT_INTERFACE);

      ObjectMapper mapper = new ObjectMapper();
      ArrayNode list = mapper.createArrayNode();
      
      for(AbstractObject o : interfaces)
      {
         Interface iface = (Interface)o;
         ObjectNode entry = mapper.createObjectNode();

         // Basic interface information
         entry.put("id", iface.getObjectId());
         entry.put("name", iface.getObjectName());
         entry.put("description", iface.getDescription());
         entry.put("alias", iface.getIfAlias());
         entry.put("index", iface.getIfIndex());
         entry.put("type", iface.getIfType());
         entry.put("type_name", iface.getIfTypeName());
         entry.put("mtu", iface.getMtu());
         entry.put("speed", iface.getSpeed());

         // MAC address
         if (iface.getMacAddress() != null && !iface.getMacAddress().isNull())
         {
            entry.put("mac_address", iface.getMacAddress().toString());
         }
         else
         {
            entry.putNull("mac_address");
         }

         // Administrative and operational states
         entry.put("admin_state", iface.getAdminStateAsText());
         entry.put("oper_state", iface.getOperStateAsText());
         entry.put("expected_state", iface.getExpectedState());

         // Physical location
         entry.put("chassis", iface.getChassis());
         entry.put("module", iface.getModule());
         entry.put("pic", iface.getPIC());
         entry.put("port", iface.getPort());

         // Utilization
         entry.put("inbound_utilization", iface.getInboundUtilization());
         entry.put("outbound_utilization", iface.getOutboundUtilization());

         // IP addresses
         ArrayNode ipAddresses = mapper.createArrayNode();
         for(InetAddressEx addr : iface.getIpAddressList())
         {
            ObjectNode ipEntry = mapper.createObjectNode();
            ipEntry.put("address", addr.getHostAddress());
            ipEntry.put("mask_bits", addr.getMask());
            ipAddresses.add(ipEntry);
         }
         entry.set("ip_addresses", ipAddresses);

         // VLAN information
         if (iface.getVlans() != null && iface.getVlans().length > 0)
         {
            ArrayNode vlans = mapper.createArrayNode();
            for(long vlan : iface.getVlans())
            {
               vlans.add(vlan);
            }
            entry.set("vlans", vlans);
         }
         else
         {
            entry.set("vlans", mapper.createArrayNode());
         }

         // Peer information (for discovered topology)
         if (iface.getPeerNodeId() != 0)
         {
            ObjectNode peerInfo = mapper.createObjectNode();
            peerInfo.put("node_id", iface.getPeerNodeId());
            peerInfo.put("interface_id", iface.getPeerInterfaceId());
            peerInfo.put("discovery_protocol", iface.getPeerDiscoveryProtocol().toString());
            if (iface.getPeerLastUpdateTime() != null)
            {
               peerInfo.put("last_update", iface.getPeerLastUpdateTime().getTime());
            }
            entry.set("peer", peerInfo);
         }
         else
         {
            entry.putNull("peer");
         }

         // Interface flags
         entry.put("is_physical_port", iface.isPhysicalPort());
         entry.put("is_loopback", iface.isLoopback());
         entry.put("exclude_from_topology", iface.isExcludedFromTopology());

         // Spanning Tree Protocol state
         if (iface.getStpPortState() != null)
         {
            entry.put("stp_port_state", iface.getStpPortState().toString());
         }
         else
         {
            entry.putNull("stp_port_state");
         }

         // 802.1X states
         entry.put("dot1x_pae_state", iface.getDot1xPaeState());
         entry.put("dot1x_backend_state", iface.getDot1xBackendState());

         list.add(entry);
      }
      
      return mapper.writeValueAsString(list);
   }
}
