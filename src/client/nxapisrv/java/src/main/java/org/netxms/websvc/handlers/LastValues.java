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
import java.util.List;
import java.util.Map;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.DataCollectionTarget;
import org.netxms.websvc.json.ResponseContainer;

/**
 * Handler for /objects/{object-id}/lastvalues
 */
public class LastValues extends AbstractObjectHandler
{
   /* (non-Javadoc)
    * @see org.netxms.websvc.handlers.AbstractHandler#getCollection(java.util.Map)
    */
   @Override
   protected Object getCollection(Map<String, String> query) throws Exception
   {
      NXCSession session = getSession();
      AbstractObject object = getObject();
      List<DciValue[]> values = new ArrayList<DciValue[]>();
      if (object instanceof DataCollectionTarget)
      {
         values.add(session.getLastValues(object.getObjectId()));
      }
      else
      {
         AbstractObject[] children = object.getChildrenAsArray();
         for(AbstractObject child : children)
         {
            if (child instanceof DataCollectionTarget)
               values.add(session.getLastValues(child.getObjectId()));
         }
      }
      return new ResponseContainer("lastValues", values.toArray());
   }
}
