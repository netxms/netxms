/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2017 Raden Solutions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
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
package org.netxms.websvc.handlers;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.netxms.client.NXCException;
import org.netxms.client.datacollection.DciData;
import org.netxms.client.datacollection.DciDataRow;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.DataCollectionTarget;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;

public class GrafanaDataCollection extends AbstractHandler
{
   private List<AbstractObject> objects;
   private Logger log = LoggerFactory.getLogger(GrafanaDataCollection.class);
   
   /* (non-Javadoc)
    * @see org.netxms.websvc.handlers.AbstractHandler#getCollection(java.util.Map)
    */
   @Override
   protected Object getCollection(Map<String, String> query) throws Exception
   {
      if (!getSession().isObjectsSynchronized())
         getSession().syncObjects();

      objects = getSession().getAllObjects();
      
      if (query.containsKey("targets"))
      {
         return getGraphData(query);
      }
      else if (query.containsKey("target"))
      {
         return getDciList(query.get("target"));
      }
      
      return getNodeList();     
   }
   
   /**
    * Get query data
    * 
    * @param query
    * @return data
    * @throws Exception
    */
   private JsonArray getGraphData(Map<String, String> query) throws Exception
   {
      JsonParser parser = new JsonParser();
      JsonElement element = parser.parse(query.get("targets"));
      if (!element.isJsonArray())
         return new JsonArray();
      
      JsonArray targets = element.getAsJsonArray();
      
      DateFormat format = new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss.SSSX");
      Date from = format.parse(query.get("from").substring(1, query.get("from").length()-1));
      Date to = format.parse(query.get("to").substring(1, query.get("to").length()-1));
      
      JsonObject root, dciTarget, dci;
      JsonArray result = new JsonArray();
      for(JsonElement e : targets)
      {
         if (!e.getAsJsonObject().has("dciTarget") || !e.getAsJsonObject().has("dci"))
            continue;
         
         dciTarget = e.getAsJsonObject().getAsJsonObject("dciTarget");
         dci = e.getAsJsonObject().getAsJsonObject("dci");
         
         if (dciTarget.size() == 0 || dci.size() == 0)
            continue;
         
         DciData data = getSession().getCollectedData(Long.parseLong(dciTarget.get("id").getAsString()),
                                                      Long.parseLong(dci.get("id").getAsString()), from, to, 0);
         root = new JsonObject();
         JsonArray datapoints = new JsonArray();
         JsonArray datapoint;
         DciDataRow[] values = data.getValues(); 
         for(int i = values.length - 1; i >= 0; i--)
         {
            DciDataRow r = values[i];
            datapoint = new JsonArray();
            datapoint.add(r.getValueAsDouble());
            datapoint.add(r.getTimestamp().getTime());
            datapoints.add(datapoint);
         }
         if (e.getAsJsonObject().has("legend") && !e.getAsJsonObject().get("legend").getAsString().equals(""))
            root.addProperty("target", e.getAsJsonObject().get("legend").getAsString());
         else
            root.addProperty("target", dci.get("name").getAsString());
         
         root.add("datapoints", datapoints);
         result.add(root);
      }
      return result;
   }
   
   /**
    * Get list of nodes
    * 
    * @return
    */
   private Map<Long, String> getNodeList()
   {
      Map<Long, String> result = new HashMap<Long, String>();
      for(AbstractObject o : objects)
      {
         if (o instanceof DataCollectionTarget)
            result.put(o.getObjectId(), o.getObjectName());
      }
      return result;
   }
   
   /**
    * Get list of dci`s for a node
    * 
    * @param id
    * @return dci list
    */
   private Map<Long, String> getDciList(String id) throws Exception
   {
      Map<Long, String> result = new HashMap<Long, String>();
      try
      {
         DciValue[] values = getSession().getLastValues(Long.parseLong(id));
         for(DciValue v : values)
         {
            result.put(v.getId(), v.getDescription());
         }
      }
      catch (NXCException e)
      {
         log.debug("DCI not found");
      }

      return result;
   }
}
