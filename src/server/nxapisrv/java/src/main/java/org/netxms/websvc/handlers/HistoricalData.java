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

import java.util.Date;
import java.util.Map;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.RCC;
import org.netxms.client.datacollection.DciData;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.DataCollectionTarget;
import org.netxms.websvc.json.ResponseContainer;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Objects request handler
 */
public class HistoricalData extends AbstractObjectHandler
{
   private Logger log = LoggerFactory.getLogger(AbstractHandler.class);

   /* (non-Javadoc)
    * @see org.netxms.websvc.handlers.AbstractHandler#get(java.lang.String)
    */
   @Override
   protected Object get(String id, Map<String, String> query) throws Exception
   {
      NXCSession session = getSession();      
      AbstractObject obj = getObject();
      long dciId = 0;
      try 
      {
         dciId = Long.parseLong(id);
      }
      catch(NumberFormatException e)
      {
         dciId = session.dciNameToId(obj.getObjectId(), id);
      }
      
      if(obj == null || dciId == 0 || !(obj instanceof DataCollectionTarget))
         throw new NXCException(RCC.INVALID_OBJECT_ID);
      
      String timeFrom = query.get("from");
      String timeTo = query.get("to");
      String timeInteval = query.get("timeInterval");
      String itemCount = query.get("itemCount");
      
      DciData data = null;
      
      if (timeFrom != null || timeTo != null)
      {
         data = session.getCollectedData(obj.getObjectId(), dciId, new Date(parseLong(timeFrom, 0) * 1000), new Date(parseLong(timeTo, System.currentTimeMillis() / 1000) * 1000), parseInt(itemCount, 0));
      }
      else if (timeInteval != null)
      {
         Date now = new Date();
         long from = now.getTime() - parseLong(timeInteval, 0) * 1000;
         data  = session.getCollectedData(obj.getObjectId(), dciId, new Date(from), new Date(), parseInt(itemCount, 0));         
      }
      else if (itemCount != null)
      {         
         data  = session.getCollectedData(obj.getObjectId(), dciId, null, null, parseInt(itemCount, 0));
      }  
      else
      {
         Date now = new Date();
         long from = now.getTime() - 3600000; // one hour
         data  = session.getCollectedData(obj.getObjectId(), dciId, new Date(from), now, parseInt(itemCount, 0));           
      }
      
      return new ResponseContainer("values", data);
   }
}
