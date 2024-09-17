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

import org.json.JSONArray;
import org.json.JSONObject;
import org.netxms.websvc.json.JsonTools;
import java.util.Date;

/**
 * Handler for event management
 */
public class Events extends AbstractHandler
{
   /**
    * @see org.netxms.websvc.handlers.AbstractHandler#create(org.json.JSONObject)
    */
   @Override
   protected Object create(JSONObject data) throws Exception
   {
      long eventCode = JsonTools.getLongFromJson(data, "eventCode", 0);
      String eventName = JsonTools.getStringFromJson(data, "eventName", null);
      long objectId = JsonTools.getLongFromJson(data, "objectId", 0);

      JSONArray parametersJsonArray = JsonTools.getJsonArrayFromJson(data, "parameters", null);
      String[] parameters = new String[0];
      if (parametersJsonArray != null)
      {
         parameters = new String[parametersJsonArray.length()];
         for(int i = 0; i < parametersJsonArray.length(); i++)
         {
            parameters[i] = parametersJsonArray.getString(i);
         }
      }

      String userTag = JsonTools.getStringFromJson(data, "userTag", null);

      Date originTimestamp;
      long date = JsonTools.getLongFromJson(data, "originTimestamp", -1);
      originTimestamp = date > 0 ? new Date(date * 1000) : null;

      getSession().sendEvent(eventCode, eventName, objectId, parameters, null, userTag, originTimestamp);

      return null;
   }
}
