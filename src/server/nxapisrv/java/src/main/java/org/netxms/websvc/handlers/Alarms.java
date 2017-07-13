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
import java.util.Iterator;
import java.util.Map;
import java.util.UUID;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.RCC;
import org.netxms.client.events.Alarm;
import org.netxms.client.objects.AbstractObject;
import org.netxms.websvc.json.ResponseContainer;

/**
 * Handler for alarm management
 */
public class Alarms extends AbstractHandler
{
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
}
