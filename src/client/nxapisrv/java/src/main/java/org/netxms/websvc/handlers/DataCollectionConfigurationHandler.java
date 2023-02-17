/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Raden Solutions
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
import java.util.Arrays;
import java.util.List;
import java.util.Map;
import java.util.regex.Pattern;
import org.json.JSONObject;
import org.netxms.client.NXCException;
import org.netxms.client.constants.RCC;
import org.netxms.client.datacollection.DataCollectionConfiguration;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.websvc.json.JsonTools;
import org.netxms.websvc.json.ResponseContainer;

/**
 * Handler for data collection configuration on object level
 */
public class DataCollectionConfigurationHandler extends AbstractObjectHandler
{
   /**
    * @see org.netxms.websvc.handlers.AbstractHandler#getCollection(java.util.Map)
    */
   @Override
   protected Object getCollection(Map<String, String> query) throws Exception
   {
      DataCollectionConfiguration dc = getSession().openDataCollectionConfiguration(getObjectId());
      List<DataCollectionObject> dcObjects = Arrays.asList(dc.getItems());

      String dciName = query.containsKey("dciName") ? query.get("dciName").toUpperCase() : null;
      String dciNameRegexp = query.get("dciNameRegexp");
      String dciDescription = query.containsKey("dciDescription") ? query.get("dciDescription").toUpperCase() : null;
      String dciDescriptionRegexp = query.get("dciDescriptionRegexp");
      
      Pattern nameRegex = null;
      if ((dciNameRegexp != null) && !dciNameRegexp.isEmpty())
         nameRegex = Pattern.compile(dciNameRegexp, Pattern.CASE_INSENSITIVE);
      
      Pattern descriptionRegex = null;
      if ((dciDescriptionRegexp != null) && !dciDescriptionRegexp.isEmpty())
         descriptionRegex = Pattern.compile(dciDescriptionRegexp, Pattern.CASE_INSENSITIVE);
      
      if ((dciName != null) || (dciNameRegexp != null) || (dciDescription != null) || 
            (dciDescriptionRegexp != null))
      {
         List<DataCollectionObject> newValues = new ArrayList<DataCollectionObject>();
         for (DataCollectionObject obj : dcObjects)
         {
            if ((dciName != null && obj.getName().toUpperCase().contains(dciName)) ||
                  (nameRegex != null && nameRegex.matcher(obj.getName()).matches()) ||
                  (dciDescription != null && obj.getDescription().toUpperCase().contains(dciDescription)) ||
                  (descriptionRegex != null && descriptionRegex.matcher(obj.getDescription()).matches()))
               newValues.add(obj);
         }
         dcObjects = newValues;
      }
      
      dc.close();
      return new ResponseContainer("dataCollectionObjects", dcObjects);
   }

   /**
    * @see org.netxms.websvc.handlers.AbstractHandler#create(org.json.JSONObject)
    */
   @Override
   protected Object create(JSONObject data) throws Exception
   {
      DataCollectionObject dcObject = JsonTools.createGsonInstance().fromJson(data.toString(), DataCollectionObject.class);
      if ((dcObject.getName() == null) || dcObject.getName().isEmpty() || (dcObject.getDescription() == null)
            || dcObject.getDescription().isEmpty())
         throw new NXCException(RCC.INVALID_ARGUMENT, "Name and description for new DCI cannot be empty");

      dcObject.setId(0);
      dcObject.setNodeId(getObjectId());
      long dciId = getSession().modifyDataCollectionObject(dcObject);

      JSONObject result = new JSONObject();
      result.append("id", dciId);
      return result;
   }
}
