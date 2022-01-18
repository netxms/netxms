/**
 * NetXMS - open source network management system
 * Copyright (C) 2021-2022 Raden Solutions
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
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.DciPushData;
import org.netxms.websvc.json.JsonTools;

/**
 * Push new DCI data
 */
public class PushDataHandler extends AbstractHandler
{
   /**
    * Push DCI data 
    */
   @Override
   protected Object create(JSONObject data) throws Exception
   {
      NXCSession session = getSession();
      DciPushData[] dciData = null;
      if (data.has("data"))
      {
         JSONArray obj = data.getJSONArray("data");
         dciData = JsonTools.createGsonInstance().fromJson(obj.toString(), DciPushData[].class);
      }
      else
      {
         DciPushData tmp = JsonTools.createGsonInstance().fromJson(data.toString(), DciPushData.class);         
         dciData = new DciPushData[] {tmp};
      }
      session.pushDciData(dciData);
      return null;
   }
}
