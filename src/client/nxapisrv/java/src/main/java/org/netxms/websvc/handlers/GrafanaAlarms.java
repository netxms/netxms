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
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import org.json.JSONException;
import org.netxms.client.events.Alarm;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.users.AbstractUserObject;
import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;

public class GrafanaAlarms extends AbstractHandler
{
   private static final String[] STATES = { "Outstanding", "Acknowledged", "Resolved", "Terminated" };
   
   /* (non-Javadoc)
    * @see org.netxms.websvc.handlers.AbstractHandler#getCollection(java.util.Map)
    */
   @Override
   public Object getCollection(Map<String, String> query) throws Exception
   {
      if (!getSession().isObjectsSynchronized())
         getSession().syncObjects();
      
      if (query.isEmpty())
      {
         Set<Integer> classFilter = new HashSet<Integer>(5);
         classFilter.add(AbstractObject.OBJECT_NODE);
         classFilter.add(AbstractObject.OBJECT_MOBILEDEVICE);
         classFilter.add(AbstractObject.OBJECT_RACK);
         classFilter.add(AbstractObject.OBJECT_CLUSTER);
         AbstractObject[] objects = getSession().getTopLevelObjects(classFilter);
         Map<Long, String> result = new HashMap<Long, String>();
         for(AbstractObject o : objects)
               result.put(o.getObjectId(), o.getObjectName());
         
         return result;
      }
      
      JsonObject root = new JsonObject();

      JsonArray columns = new JsonArray();
      columns.add(createColumn("Severity", true, true));
      columns.add(createColumn("State", true, false));
      columns.add(createColumn("Source", true, false));
      columns.add(createColumn("Message", true, false));
      columns.add(createColumn("Count", true, false));
      columns.add(createColumn("Helpdesk ID", true, false));
      columns.add(createColumn("Ack/Resolved By", true, false));
      columns.add(createColumn("Created", true, false));
      columns.add(createColumn("Last Change", true, false));
      root.add("columns", columns);
      
      JsonArray rows = new JsonArray();
      
      JsonArray r = new JsonArray();
      DateFormat df = new SimpleDateFormat("dd.MM.yyyy HH:mm:ss");
      AbstractObject object = null;
      AbstractUserObject user = null;
      
      JsonParser parser = new JsonParser();
      JsonElement element = parser.parse(query.get("targets"));
      if (!element.isJsonArray())
         return new JsonArray();
      
      JsonArray targets = element.getAsJsonArray();
      JsonObject alarmSource;
      long sourceId = 0;
      Map<Long, Alarm> alarms = getSession().getAlarms();

      for(JsonElement e : targets)
      {
         for( Alarm a : alarms.values())
         {
            if (e.getAsJsonObject().has("alarmSource"))
            {
               alarmSource = e.getAsJsonObject().getAsJsonObject("alarmSource");
               if (alarmSource.size() > 0)
                  sourceId = Long.parseLong(alarmSource.get("id").getAsString());
            }
            
            if (sourceId == 0 || a.getSourceObjectId() == sourceId)
            {            
               r.add(a.getCurrentSeverity().name());
               r.add(STATES[a.getState()]);
               
               object = getSession().findObjectById(a.getSourceObjectId());
               if (object == null)
                  r.add(a.getSourceObjectId());
               else
                  r.add(object.getObjectName());
               
               r.add(a.getMessage());
               r.add(a.getRepeatCount());
               r.add(a.getHelpdeskReference());
               
               user = getSession().findUserDBObjectById(a.getAckByUser());
               if (user == null)
                  r.add("");
               else
                  r.add(user.getName());
               
               r.add(df.format(a.getCreationTime()));
               r.add(df.format(a.getLastChangeTime()));
               rows.add(r);
               r = new JsonArray();
            }               
         }
      }      
      root.add("rows", rows);      
      root.addProperty("type", "table");

      JsonArray wrapper = new JsonArray();
      wrapper.add(root);
      return wrapper;
   }
   
   /**
    * @param name
    * @param sort
    * @param desc
    * @return
    * @throws JSONException
    */
   private static JsonObject createColumn(String name, boolean sort, boolean desc) throws JSONException
   {
      JsonObject column = new JsonObject();
      column.addProperty("text", name);
      column.addProperty("sort", sort);
      column.addProperty("sort", desc);
      return column;
   }
}
