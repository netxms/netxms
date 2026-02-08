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
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.node.ArrayNode;
import com.fasterxml.jackson.databind.node.ObjectNode;
import com.netxms.mcp.Startup;

/**
 * Tool for reading list of objects from NetXMS server
 */
public class ObjectList extends ServerTool
{
   /**
    * @see com.netxms.mcp.tools.ServerTool#getName()
    */
   @Override
   public String getName()
   {
      return "object-list";
   }

   /**
    * @see com.netxms.mcp.tools.ServerTool#getDescription()
    */
   @Override
   public String getDescription()
   {
      return "Returns list of objects from NetXMS server.\nAccepts optional name filter as argument.";
   }

   /**
    * @see com.netxms.mcp.tools.ServerTool#execute(java.util.Map)
    */
   @Override
   public String execute(Map<String, Object> args) throws Exception
   {
      NXCSession session = Startup.getSession();

      List<AbstractObject> objects;
      String filter = (String)args.get("filter");
      if ((filter == null) || filter.isBlank())
      {
         objects = session.getAllObjects();
      }
      else
      {
         final String f = filter.toLowerCase();
         objects = session.filterObjects((o) -> o.getObjectName().toLowerCase().contains(f));
      }

      ObjectMapper mapper = new ObjectMapper();
      ArrayNode list = mapper.createArrayNode();
      for(AbstractObject object : objects)
         list.add(serializeObject(object, mapper));

      return mapper.writeValueAsString(list);
   }

   /**
    * Serialize object to JSON
    *
    * @param object object to serialize
    * @param mapper object mapper
    * @return JSON representation of the object
    */
   private static JsonNode serializeObject(AbstractObject object, ObjectMapper mapper)
   {
      ObjectNode result = mapper.createObjectNode();
      result.put("id", object.getObjectId());
      result.put("name", object.getObjectName());
      result.put("alias", object.getAlias() != null ? object.getAlias() : "");
      result.put("comments", object.getComments() != null ? object.getComments() : "");
      result.put("class", object.getObjectClassName());
      return result;
   }
}
