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

import java.util.Collection;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.UUID;
import org.json.JSONArray;
import org.json.JSONObject;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.RCC;
import org.netxms.client.events.Alarm;
import org.netxms.client.objects.AbstractObject;
import org.netxms.websvc.json.ResponseContainer;
import org.restlet.data.MediaType;
import org.restlet.ext.json.JsonRepresentation;
import org.restlet.representation.Representation;
import org.restlet.representation.StringRepresentation;
import org.restlet.resource.Post;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Handler for alarm management
 */
public class Alarms extends AbstractHandler
{
   private static final int TERMINATE = 0;
   private static final int ACKNOWLEDGE = 1;
   private static final int STICKY_ACKNOWLEDGE = 2;
   private static final int RESOLVE = 3;
   
   private Logger log = LoggerFactory.getLogger(Alarms.class);
   
   /* (non-Javadoc)
    * @see org.netxms.websvc.handlers.AbstractHandler#getCollection(org.json.JSONObject)
    */
   @Override
   protected Object getCollection(Map<String, String> query) throws Exception
   {
      NXCSession session = getSession();
      Collection<Alarm> alarms = session.getAlarms().values();
      String queryGuid = query.get("objectGuid");
      if(queryGuid != null)
      {         
         UUID objectGuid = UUID.fromString(queryGuid);
         if (!session.isObjectsSynchronized())
            session.syncObjects();
         
         AbstractObject object = session.findObjectByGUID(objectGuid);
         if (object == null)
            throw new NXCException(RCC.INVALID_OBJECT_ID);
         
         Iterator<Alarm> iterator =  alarms.iterator();
         while(iterator.hasNext())
         {
            Alarm alarm = iterator.next();
            if(alarm.getSourceObjectId() != object.getObjectId())
            {
               iterator.remove();
            }
         }
      }

      return new ResponseContainer("alarms", alarms);
   }
   
   @Post
   public Representation onPost(Representation entity) throws Exception
   {
      if (entity == null || !attachToSession())
      {
         return new StringRepresentation(createErrorResponse(RCC.ACCESS_DENIED).toString(), MediaType.APPLICATION_JSON);
      }
      
      NXCSession session = getSession();
      HashMap<Long, Alarm> alarms = session.getAlarms();
      
      JSONObject data = new JsonRepresentation(entity).getJsonObject();
      log.debug(data.toString());
      
      if (data.has("alarmList"))
      {
         JSONArray alarmList = data.getJSONArray("alarmList");
         
         for(int i = 0; i < alarmList.length(); i++)
         {
            JSONObject alarmData = alarmList.getJSONObject(i);
            Alarm a = alarms.get(alarmData.getLong("alarmId"));
            if (a != null)
            {
               switch (alarmData.getInt("action"))
               {
                  case TERMINATE:
                     session.terminateAlarm(a.getId());
                     break;
                  case ACKNOWLEDGE:
                     session.acknowledgeAlarm(a.getId());
                     break;
                  case STICKY_ACKNOWLEDGE:
                     if (alarmData.has("timeout"))
                        session.acknowledgeAlarm(a.getId(), true, alarmData.getInt("timeout"));
                     break;
                  case RESOLVE:
                     session.resolveAlarm(a.getId());
                     break;
               }
            }
         }
      }
      else
         return new StringRepresentation(createErrorResponse(RCC.INTERNAL_ERROR).toString(), MediaType.APPLICATION_JSON);
      
      return new StringRepresentation(createErrorResponse(RCC.SUCCESS).toString(), MediaType.APPLICATION_JSON);
   }
}
