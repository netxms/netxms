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

import java.util.List;
import java.util.Map;
import org.netxms.client.HardwareComponent;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.node.ArrayNode;
import com.fasterxml.jackson.databind.node.ObjectNode;
import com.netxms.mcp.Startup;

/**
 * Tool returning hardware inventory for given node
 */
public class HardwareInventory extends ObjectServerTool
{
   /**
    * @see com.netxms.mcp.tools.ServerTool#getName()
    */
   @Override
   public String getName()
   {
      return "hardware-inventory";
   }

   /**
    * @see com.netxms.mcp.tools.ServerTool#getDescription()
    */
   @Override
   public String getDescription()
   {
      return "Returns list of hardware components for given node (server, workstation, or device).\nThis tool requires a node object ID or name as a parameter.";
   }

   /**
    * @see com.netxms.mcp.tools.ServerTool#getSchema()
    */
   @Override
   public String getSchema()
   {
      return new SchemaBuilder()
            .addArgument("object_id", "string", "ID or name of the node object to retrieve hardware component list from", true)
            .build();
   }

   /**
    * @see com.netxms.mcp.tools.ObjectServerTool#execute(org.netxms.client.objects.AbstractObject, java.util.Map)
    */
   @Override
   protected String execute(AbstractObject object, Map<String, Object> args) throws Exception
   {
      NXCSession session = Startup.getSession();
      List<HardwareComponent> components = session.getNodeHardwareComponents(object.getObjectId());

      ObjectMapper mapper = new ObjectMapper();
      ArrayNode list = mapper.createArrayNode();
      for(HardwareComponent c : components)
      {
         ObjectNode entry = mapper.createObjectNode();
         entry.put("category", (c.getCategory() != null) ? c.getCategory().name() : null);
         entry.put("index", c.getIndex());
         entry.put("type", c.getType());
         entry.put("vendor", c.getVendor());
         entry.put("model", c.getModel());
         entry.put("partNumber", c.getPartNumber());
         entry.put("serialNumber", c.getSerialNumber());
         entry.put("location", c.getLocation());
         entry.put("capacity", c.getCapacity());
         entry.put("description", c.getDescription());
         list.add(entry);
      }
      return mapper.writeValueAsString(list);
   }
}
