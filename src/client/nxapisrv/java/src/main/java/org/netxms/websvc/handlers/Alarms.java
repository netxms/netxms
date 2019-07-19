/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2019 Raden Solutions
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

import java.util.ArrayList;
import java.util.Collection;
import java.util.Date;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import org.json.JSONArray;
import org.json.JSONObject;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.RCC;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.events.Alarm;
import org.netxms.client.events.AlarmCategory;
import org.netxms.client.events.EventTemplate;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Node;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.websvc.json.JsonTools;
import org.netxms.websvc.json.ResponseContainer;
import com.google.gson.Gson;
import com.google.gson.JsonArray;
import com.google.gson.JsonObject;

/**
 * Handler for alarm management
 */
public class Alarms extends AbstractHandler
{
   private static final int UNDEFINED = -1;
   private static final int TERMINATE = 0;
   private static final int ACKNOWLEDGE = 1;
   private static final int STICKY_ACKNOWLEDGE = 2;
   private static final int RESOLVE = 3;
   
   /* (non-Javadoc)
    * @see org.netxms.websvc.handlers.AbstractHandler#getCollection(org.json.JSONObject)
    */
   @Override
   protected Object getCollection(Map<String, String> query) throws Exception
   {
      NXCSession session = getSession();
      Collection<Alarm> alarms = session.getAlarms().values();
      
      AbstractObject rootObject = getObjectFromQuery(query);

      final boolean[] stateMask;
      String stateFilter = query.get("state");
      if (stateFilter != null)
      {
         stateMask = new boolean[] { false, false, false, false };
         for(String s : stateFilter.split(","))
         {
            try
            {
               int n = Integer.parseInt(s);
               if ((n >= Alarm.STATE_OUTSTANDING) && (n <= Alarm.STATE_TERMINATED))
                  stateMask[n] = true;
            }
            catch(NumberFormatException e)
            {
               if (s.equalsIgnoreCase("outstanding"))
                  stateMask[Alarm.STATE_OUTSTANDING] = true;
               else if (s.equalsIgnoreCase("acknowledged"))
                  stateMask[Alarm.STATE_ACKNOWLEDGED] = true;
               else if (s.equalsIgnoreCase("resolved"))
                  stateMask[Alarm.STATE_RESOLVED] = true;
               else if (s.equalsIgnoreCase("terminated"))
                  stateMask[Alarm.STATE_TERMINATED] = true;
            }
         }
      }
      else
      {
         stateMask = new boolean[] { true, true, true, true };
      }
      
      Date createdBefore = parseTimestamp(query.get("createdBefore"));
      Date createdAfter = parseTimestamp(query.get("createdAfter"));
      Date updatedBefore = parseTimestamp(query.get("updatedBefore"));
      Date updatedAfter = parseTimestamp(query.get("updatedAfter"));
      
      if ((rootObject != null) || (stateFilter != null) || (createdBefore != null) ||
          (createdAfter != null) || (updatedBefore != null) || (updatedAfter != null))
      {
         boolean includeChildren = Boolean.parseBoolean(query.getOrDefault("includeChildObjects", "false"));
         
         Iterator<Alarm> iterator =  alarms.iterator();
         while(iterator.hasNext())
         {
            Alarm alarm = iterator.next();
            
            if (!stateMask[alarm.getState() & Alarm.STATE_MASK])
            {
               iterator.remove();
               continue;
            }
            
            if (((createdBefore != null) && alarm.getCreationTime().after(createdBefore)) ||
                ((createdAfter != null) && alarm.getCreationTime().before(createdAfter)) ||
                ((updatedBefore != null) && alarm.getLastChangeTime().after(updatedBefore)) ||
                ((updatedAfter != null) && alarm.getLastChangeTime().before(updatedAfter)))
            {
               iterator.remove();
               continue;
            }
            
            if ((rootObject != null) &&
                (alarm.getSourceObjectId() != rootObject.getObjectId()) &&
                (!includeChildren || !rootObject.isParentOf(alarm.getSourceObjectId())))
            {
               iterator.remove();
            }
         }
      }
      
      if (!Boolean.parseBoolean(query.getOrDefault("resolveReferences", "false")) || alarms.isEmpty())
         return new ResponseContainer("alarms", alarms);

      if (!session.isObjectsSynchronized())
         session.syncObjects();
      if (!session.isUserDatabaseSynchronized())
         session.syncUserDatabase();
      if (!session.isAlarmCategoriesSynchronized())
         session.syncAlarmCategories();
      if (!session.isEventObjectsSynchronized())
         session.syncEventTemplates();
      
      List<JsonObject> serializedAlarms = new ArrayList<JsonObject>();
      Map<Long, DciValue[]> cachedValues = null;
      Gson gson = JsonTools.createGsonInstance();
      for(Alarm a : alarms)
      {
         JsonObject json = (JsonObject)gson.toJsonTree(a);
         AbstractObject object = session.findObjectById(a.getSourceObjectId());
         if (object != null)
         {
            json.add("sourceObject", gson.toJsonTree(object));
            if (a.getDciId() != 0)
            {
               DciValue[] values = null;
               if (cachedValues != null)
                  values = cachedValues.get(object.getObjectId());
               if (values == null)
                  values = session.getLastValues(object.getObjectId());
               
               for(DciValue v : values)
               {
                  if (v.getId() == a.getDciId())
                  {
                     json.add("dci", gson.toJsonTree(v));
                     break;
                  }
               }
               
               if (cachedValues == null)
                  cachedValues = new HashMap<Long, DciValue[]>();
               cachedValues.put(object.getObjectId(), values);
            }
            if (session.isZoningEnabled() && (object instanceof Node))
            {
               json.addProperty("zoneUIN", ((Node)object).getZoneId());
               json.addProperty("zoneName", ((Node)object).getZoneName());
            }
         }

         addUserName(json, session, a.getAcknowledgedByUser(), "acknowledgedByUserName");
         addUserName(json, session, a.getResolvedByUser(), "resolvedByUserName");
         addUserName(json, session, a.getTerminatedByUser(), "terminatedByUserName");

         EventTemplate e = session.findEventTemplateByCode(a.getSourceEventCode());
         if (e != null)
         {
            json.addProperty("sourceEventName", e.getName());
         }
         
         json.remove("categories");
         JsonArray categories = new JsonArray();
         for(long cid : a.getCategories())
         {
            JsonObject c = new JsonObject();
            c.addProperty("id", cid);
            AlarmCategory category = session.findAlarmCategoryById(cid);
            c.addProperty("name", (category != null) ? category.getName() : Long.toString(cid));
            categories.add(c);
         }
         json.add("categories", categories);

         serializedAlarms.add(json);
      }
      return new ResponseContainer("alarms", serializedAlarms);
   }
   
   /**
    * Add user name for given user to JSON object as property
    *
    * @param json JSON object
    * @param session client session
    * @param userId user ID
    * @param property property name
    */
   private static void addUserName(JsonObject json, NXCSession session, int userId, String property)
   {
      if (userId == 0)
         return;

      AbstractUserObject user = session.findUserDBObjectById(userId);
      if (user != null)
      {
         json.addProperty(property, user.getName());
      }
   }

   /* (non-Javadoc)
    * @see org.netxms.websvc.handlers.AbstractHandler#executeCommand(java.lang.String, org.json.JSONObject)
    */
   @Override
   protected Object executeCommand(String command, JSONObject data) throws Exception
   {
      int action = UNDEFINED;
      int timeout = 0;
      if (command.equals("terminate"))
      {
         action = TERMINATE;
      }
      else if (command.equals("acknowledge"))
      {
         action = ACKNOWLEDGE;
      }
      else if (command.equals("sticky_acknowledge"))
      {
         action = STICKY_ACKNOWLEDGE;
         if (data.has("timeout"))
            timeout = data.getInt("timeout");
      }
      else if (command.equals("resolve"))
      {
         action = RESOLVE;
      }
      
      if (!data.has("alarms") || (action == UNDEFINED))
         return createErrorResponse(RCC.INVALID_ARGUMENT);
 
      NXCSession session = getSession();
      
      JSONArray alarmList = data.getJSONArray("alarms");
      for(int i = 0; i < alarmList.length(); i++)
      {
         long alarmId = alarmList.getLong(i);
         switch(action)
         {
            case TERMINATE:
               session.terminateAlarm(alarmId);
               break;
            case ACKNOWLEDGE:
               session.acknowledgeAlarm(alarmId);
               break;
            case STICKY_ACKNOWLEDGE:
               session.acknowledgeAlarm(alarmId, true, timeout);
               break;
            case RESOLVE:
               session.resolveAlarm(alarmId);
               break;
         }
      }
      
      return null;
   }
}
