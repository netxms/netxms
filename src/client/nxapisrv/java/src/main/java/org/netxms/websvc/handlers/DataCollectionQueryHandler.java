/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
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
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.DciValue;

/**
 * Handler for alarm management
 */
public class DataCollectionQueryHandler extends AbstractObjectHandler
{      
   /**
    * @see org.netxms.websvc.handlers.AbstractHandler#getCollection(java.util.Map)
    */
   @Override
   protected Object getCollection(Map<String, String> query) throws Exception
   {
      StringBuilder queryText = new StringBuilder();
      for (String key : query.keySet())
      {
         if (key.equals("query"))
         {
            queryText.append(query.get(key));
            queryText.append(" ");
         }
         else
         {
            queryText.append(key);
            queryText.append(":");
            queryText.append(query.get(key));
            queryText.append(" ");            
         }
      }      

      NXCSession session = getSession();
      DciValue[] result = session.findDCI(getObjectId(), queryText.toString());
      return result;
   }
}
