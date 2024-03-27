/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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
import org.netxms.client.constants.RCC;
import org.netxms.client.datacollection.DataCollectionObject;

public class DCObjectStatusChangeHandler extends AbstractObjectHandler
{
   private static final int UNDEFINED = -1;

   /**
    * @see org.netxms.websvc.handlers.AbstractHandler#executeCommand(java.lang.String, org.json.JSONObject)
    */
   @Override
   protected Object executeCommand(String command, JSONObject data) throws Exception
   {
      int status = UNDEFINED;
      if (command.equals("activate"))
      {
         status = DataCollectionObject.ACTIVE;
      }
      else if (command.equals("disable"))
      {
         status = DataCollectionObject.DISABLED;
      }

      if (!data.has("dcis") || (status == UNDEFINED))
         return createErrorResponse(RCC.INVALID_ARGUMENT);

      JSONArray dciList = data.getJSONArray("dcis");
      long[] items = new long[dciList.length()];      
      for(int i = 0; i < dciList.length(); i++)
      {
         items[i] = dciList.getLong(i);
      }
      

      long[] result = getSession().changeDCIStatus(getObjectId(), items, status);
      if (result != null)
      {
         JSONObject json = new JSONObject();
         json.put("result", result);
         return json;
      }
      else
      {
         return null;
      }
   }
}
