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
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.objects.AbstractObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.node.ArrayNode;
import com.fasterxml.jackson.databind.node.ObjectNode;
import com.netxms.mcp.Startup;

/**
 * Tool for retrieving last collected values for all metrics on given object
 */
public class MetricList extends ObjectServerTool
{
   private static final Logger logger = LoggerFactory.getLogger(MetricList.class);

   /**
    * @see com.netxms.mcp.tools.ServerTool#getName()
    */
   @Override
   public String getName()
   {
      return "metric-list";
   }

   /**
    * @see com.netxms.mcp.tools.ServerTool#getDescription()
    */
   @Override
   public String getDescription()
   {
      return "Returns list of metrics on given object along with last collected (current) values.\nThis function requires an object ID or name as a parameter.";
   }

   /**
    * @see com.netxms.mcp.tools.ServerTool#getSchema()
    */
   @Override
   public String getSchema()
   {
      return new SchemaBuilder()
         .addArgument("object_id", "string", "ID or name of the object to retrieve metric values for", true)
         .addArgument("filter", "string", "Optional filter to retrieve only specific metrics (with name or description containing filter string)", false)
         .build();
   }

   /**
    * @see com.netxms.mcp.tools.ObjectServerTool#execute(org.netxms.client.objects.AbstractObject, java.util.Map)
    */
   @Override
   protected String execute(AbstractObject object, Map<String, Object> args) throws Exception
   {
      NXCSession session = Startup.getSession();
      DciValue[] values = session.getLastValues(object.getObjectId());

      String filter = (String)args.get("filter");
      if (filter != null)
         filter = filter.trim();

      ObjectMapper mapper = new ObjectMapper();
      ArrayNode list = mapper.createArrayNode();
      for(DciValue v : values)
      {
         if (filter != null && !filter.isEmpty())
         {
            String name = v.getName();
            String description = v.getDescription();
            if (!fuzzyMatch(name, filter) && !fuzzyMatch(description, filter))
               continue;
         }
         ObjectNode jv = mapper.createObjectNode();
         jv.put("metric_id", v.getId());
         jv.put("name", v.getName());
         jv.put("description", v.getDescription());
         jv.put("comments", v.getComments());
         jv.put("value", v.getValue());
         jv.put("data_type", v.getDataType().toString());
         jv.put("status", v.getStatus().toString());
         jv.put("timestamp", v.getTimestamp().getTime() / 1000);
         jv.put("data_source", v.getSource().toString());
         list.add(jv);
      }

      logger.debug("LastValues.execute(): object={}, filter={}, result-size={}", object.getObjectName(), filter, list.size());
      return mapper.writeValueAsString(list);
   }
}
