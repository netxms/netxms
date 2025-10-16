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
import org.netxms.base.GeoLocation;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Interface;
import org.netxms.client.objects.Node;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.node.ObjectNode;

/**
 * 
 */
public class ObjectDetails extends ObjectServerTool
{
   /**
    * @see com.netxms.mcp.tools.ServerTool#getName()
    */
   @Override
   public String getName()
   {
      return "object-details";
   }

   /**
    * @see com.netxms.mcp.tools.ServerTool#getDescription()
    */
   @Override
   public String getDescription()
   {
      return "Returns details of a specific object from the NetXMS server.\nThis tool requires an object ID or name as a parameter.";
   }

   /**
    * @see com.netxms.mcp.tools.ServerTool#getSchema()
    */
   @Override
   public String getSchema()
   {
      return new SchemaBuilder()
         .addArgument("object_id", "string", "ID or name of the object to retrieve details for", true)
         .build();
   }

   /**
    * @see com.netxms.mcp.tools.ObjectServerTool#execute(org.netxms.client.objects.AbstractObject, java.util.Map)
    */
   @Override
   protected String execute(AbstractObject object, Map<String, Object> args) throws Exception
   {
      // Create JSON response with basic object details
      ObjectMapper mapper = new ObjectMapper();
      ObjectNode result = mapper.createObjectNode();

      result.put("id", object.getObjectId());
      result.put("name", object.getObjectName());
      result.put("alias", object.getAlias() != null ? object.getAlias() : "");
      result.put("comments", object.getComments() != null ? object.getComments() : "");
      result.put("class", object.getObjectClassName());
      result.put("status", object.getStatus().toString());

      GeoLocation location = object.getGeolocation();
      if (location != null)
      {
         ObjectNode l = mapper.createObjectNode();
         l.put("latitude", location.getLatitude());
         l.put("longitude", location.getLongitude());
         result.set("location", l);
      }

      // Add Node-specific details if object is a Node
      if (object instanceof Node)
      {
         Node node = (Node) object;
         result.put("ip_address", node.getPrimaryIP().getHostAddress());
         result.put("vendor", node.getHardwareVendor());
         result.put("model", node.getHardwareProductName());
         result.put("serial_number", node.getHardwareSerialNumber());
         result.put("platform_name", node.getPlatformName());
         result.put("system_description", node.getSystemDescription());
         result.put("node_type", node.getNodeType().toString());
         result.put("has_agent", node.hasAgent());
         result.put("has_snmp_agent", node.hasSnmpAgent());
         result.put("has_file_manager", node.hasFileManager());
      }

      if (object instanceof Interface)
      {
         Interface iface = (Interface)object;
         result.put("ip_address_list", iface.getIpAddressListAsString());
         result.put("mac_address", iface.getMacAddress().toString());
         result.put("if_index", iface.getIfIndex());
         result.put("type", iface.getIfTypeName());
         result.put("speed", iface.getSpeed());
         result.put("mtu", iface.getMtu());
         result.put("is_physical", iface.isPhysicalPort());
         result.put("is_loopback", iface.isLoopback());
         result.put("admin_state", iface.getAdminStateAsText());
         result.put("oper_state", iface.getOperStateAsText());
      }

      return mapper.writeValueAsString(result);
   }
}
