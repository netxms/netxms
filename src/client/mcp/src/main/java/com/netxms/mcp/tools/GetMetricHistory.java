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

import java.util.Date;
import java.util.Map;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.HistoricalDataType;
import org.netxms.client.datacollection.DataSeries;
import org.netxms.client.datacollection.DciDataRow;
import org.netxms.client.objects.AbstractObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.node.ArrayNode;
import com.fasterxml.jackson.databind.node.ObjectNode;
import com.netxms.mcp.Startup;

/**
 * 
 */
public class GetMetricHistory extends ObjectServerTool
{
   private static final Logger logger = LoggerFactory.getLogger(GetMetricHistory.class);

   /**
    * @see com.netxms.mcp.tools.ServerTool#getName()
    */
   @Override
   public String getName()
   {
      return "get-metric-history";
   }

   /**
    * @see com.netxms.mcp.tools.ServerTool#getDescription()
    */
   @Override
   public String getDescription()
   {
      return "Get historical data for given metric on given node.\nThis tool requires a node object ID and metric ID as parameters.\nTime range is specified using 'from' and 'to' parameters (UNIX time, in seconds). 'from' can be relative to current time (negative value). If 'to' is omitted, current time is used. Timestamps in result are also in UNIX time in seconds.";
   }

   /**
    * @see com.netxms.mcp.tools.ServerTool#getSchema()
    */
   @Override
   public String getSchema()
   {
      return new SchemaBuilder()
         .addArgument("object_id", "string", "ID or name of the node object to rename", true)
         .addIntegerArgument("metric_id", "Metric ID", true)
         .addIntegerArgument("from", "Start time (absolute UNIX time or relative to current moment in seconds; relative value always negative)", true)
         .addIntegerArgument("to", "End time (absolute UNIX time; if omited current time will be used)", false)
         .build();
   }

   /**
    * @see com.netxms.mcp.tools.ObjectServerTool#execute(org.netxms.client.objects.AbstractObject, java.util.Map)
    */
   @Override
   protected String execute(AbstractObject object, Map<String, Object> args) throws Exception
   {
      Integer metricId = (Integer)args.get("metric_id");
      if (metricId == null)
         throw new Exception("Metric ID not provided");

      long from = (Integer)args.get("from");
      long to = (args.get("to") != null) ? (Integer)args.get("to") : System.currentTimeMillis() / 1000;
      if (from < 0)
         from += to;

      NXCSession session = Startup.getSession();
      DataSeries data = session.getCollectedData(object.getObjectId(), metricId, new Date(from * 1000), new Date(to * 1000), 0, HistoricalDataType.PROCESSED);

      logger.debug("GetMetricHistory.execute(): objectId={}, metricId={}, from={}, to={}, dataset-size={}", object.getObjectId(), metricId, from, to, data.getValues().length);

      ObjectMapper mapper = new ObjectMapper();
      ArrayNode values = mapper.createArrayNode();
      for(DciDataRow row : data.getValues())
      {
         ObjectNode v = mapper.createObjectNode();
         v.put("timestamp", row.getTimestamp().getTime() / 1000);
         v.put("value", row.getValueAsString());
         values.add(v);
      }
      return mapper.writeValueAsString(values);
   }
}
