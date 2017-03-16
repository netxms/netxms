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
import org.json.JSONArray;
import org.json.JSONTokener;
import org.netxms.client.datacollection.DciData;
import org.netxms.client.datacollection.DciDataRow;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.DataCollectionTarget;
import org.netxms.client.objects.Node;
import com.google.gson.JsonArray;
import com.google.gson.JsonObject;

public class GrafanaDataCollection extends AbstractHandler
{
   private List<AbstractObject> objects;
   
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
      else if (query.containsKey("node"))
      {
         return getDciList(query.get("node"));
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
      JSONTokener tokener = new JSONTokener(query.get("targets"));
      JSONArray targets = new JSONArray(tokener);
      
      DateFormat format = new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss.SSSX");
      Date from = format.parse(query.get("from").substring(1, query.get("from").length()-1));
      Date to = format.parse(query.get("to").substring(1, query.get("to").length()-1));
      
      JsonObject root;
      JsonArray result = new JsonArray();
      for(int i = 0; i < targets.length(); i++)
      {
         long nodeId = findNodeByName(targets.getJSONObject(i).getString("node"));
         DciData data = getSession().getCollectedData(nodeId, findDciByDescription(targets.getJSONObject(i).getString("dci"), nodeId), from, to, 0);
         root = new JsonObject();
         JsonArray datapoints = new JsonArray();
         JsonArray datapoint;         
         for(DciDataRow r : data.getValues())
         {
            datapoint = new JsonArray();
            datapoint.add(r.getValueAsLong());
            datapoint.add(r.getTimestamp().getTime());
            datapoints.add(datapoint);
         }
         root.addProperty("target", targets.getJSONObject(i).getString("dci"));
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
    * @param node
    * @return dci list
    */
   private Map<Long, String> getDciList(String node) throws Exception
   {
      Map<Long, String> result = new HashMap<Long, String>();
      for(AbstractObject o : objects)
      {
         if (o.getObjectName().equals(node))
         {
            DciValue[] values = getSession().getLastValues(o.getObjectId());
            for(DciValue v : values)
            {
               result.put(v.getId(), v.getDescription());
            };
         }
      }
      return result;
   }
   
   /**
    * Find node ID by object name
    * 
    * @param name
    * @return node ID
    */
   private long findNodeByName(String name)
   {
      for(AbstractObject o : objects)
      {
         if (o instanceof Node && o.getObjectName().equals(name))
            return o.getObjectId();
      }      
      return 0;
   }
   
   /**
    * Find DCI ID by name
    * 
    * @param name
    * @return DCI ID
    */
   private long findDciByDescription(String name, long nodeId) throws Exception
   {
      DciValue[] values = getSession().getLastValues(nodeId);
      for(DciValue v : values)
      {
         if (v.getDescription().equals(name))
            return v.getId();
      }
      return 0;
   }
}
