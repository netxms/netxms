/**
 * NetXMS - open source network management system
 * Copyright (C) 2017-2021 Raden Solutions
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

import java.util.Date;
import java.util.Map;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.RCC;
import org.netxms.websvc.json.ResponseContainer;

/**
 * Handler for user support application notifications
 */
public class UserAgentNotifications extends AbstractHandler
{
   /**
    * @see org.netxms.websvc.handlers.AbstractHandler#getCollection(java.util.Map)
    */
   @Override
   protected Object getCollection(Map<String, String> query) throws Exception
   {
      NXCSession session = getSession();
      if (!session.areObjectsSynchronized())
         session.syncObjects();
      return new ResponseContainer("notifications", session.getUserAgentNotifications());
   }

   /**
    * @see org.netxms.websvc.handlers.AbstractHandler#create(org.json.JSONObject)
    */
   @Override
   protected Object create(JSONObject data) throws Exception
   {
      try
      {
         String message = data.getString("message");
         JSONArray objects = data.getJSONArray("objects");
         long startTime = data.has("startTime") ? data.getLong("startTime") * 1000 : 0;
         long endTime = data.has("endTime") ? data.getLong("endTime") * 1000 : 0;
         boolean onStartup = data.has("onStartup") ? data.getBoolean("onStartup") : false;
         long[] objectIds = new long[objects.length()];
         for(int i = 0; i < objects.length(); i++)
            objectIds[i] = objects.getLong(i);
         getSession().createUserAgentNotification(message, objectIds, new Date(startTime), new Date(endTime), onStartup);
         return new JSONObject();
      }
      catch(JSONException e)
      {
         throw new NXCException(RCC.INVALID_ARGUMENT);
      }
   }
}
