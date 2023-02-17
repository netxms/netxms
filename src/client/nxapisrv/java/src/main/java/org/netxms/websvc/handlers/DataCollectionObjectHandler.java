/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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

import java.util.Map;
import org.json.JSONObject;
import org.netxms.client.NXCException;
import org.netxms.client.constants.RCC;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.websvc.json.JsonTools;

/**
 * Handler for managing single data collection object
 */
public class DataCollectionObjectHandler extends AbstractObjectHandler
{
   /**
    * @see org.netxms.websvc.handlers.AbstractHandler#get(java.lang.String, java.util.Map)
    */
   @Override
   protected Object get(String id, Map<String, String> query) throws Exception
   {
      return super.get(id, query);
   }

   /**
    * @see org.netxms.websvc.handlers.AbstractHandler#update(java.lang.String, org.json.JSONObject)
    */
   @Override
   protected Object update(String id, JSONObject data) throws Exception
   {
      DataCollectionObject dcObject = JsonTools.createGsonInstance().fromJson(data.toString(), DataCollectionObject.class);
      if ((dcObject.getName() == null) || dcObject.getName().isEmpty() || (dcObject.getDescription() == null)
            || dcObject.getDescription().isEmpty())
         throw new NXCException(RCC.INVALID_ARGUMENT, "Name and description for new DCI cannot be empty");

      dcObject.setId(Long.parseLong(id));
      dcObject.setNodeId(getObjectId());
      getSession().modifyDataCollectionObject(dcObject);
      return null;
   }
}
