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

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import org.netxms.base.Glob;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.RCC;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.client.users.User;
import org.netxms.client.users.UserGroup;
import org.netxms.websvc.json.ResponseContainer;

/**
 * User database requests handler
 */
public class Users extends AbstractHandler
{
   /* (non-Javadoc)
    * @see org.netxms.websvc.handlers.AbstractHandler#getCollection(java.util.Map)
    */
   @Override
   protected Object getCollection(Map<String, String> query) throws Exception
   {
      NXCSession session = getSession();
      if (!session.isUserDatabaseSynchronized())
         session.syncUserDatabase();
      
      List<AbstractUserObject> objects = Arrays.asList(session.getUserDatabaseObjects());

      String typeFilter = query.get("type");
      String nameFilter = query.get("name");
      
      if ((typeFilter != null) || (nameFilter != null))
      {
         List<AbstractUserObject> filteredObjects = new ArrayList<AbstractUserObject>(objects.size());

         for(AbstractUserObject o : objects)
         {
            // Filter by name
            if ((nameFilter != null) && !nameFilter.isEmpty() && !Glob.matchIgnoreCase(nameFilter, o.getName()))
               continue;
            
            if ((typeFilter != null) && !typeFilter.isEmpty() && !typeFilter.equals("any"))
            {
               if (((o instanceof User) && !typeFilter.equals("user")) ||
                   ((o instanceof UserGroup) && !typeFilter.equals("group")))
                  continue;
            }
            
            filteredObjects.add(o);
         }
         
         objects = filteredObjects;
      }
      
      return new ResponseContainer("users", objects);
   }

   /**
    * @see org.netxms.websvc.handlers.AbstractHandler#get(java.lang.String, java.util.Map)
    */
   @Override
   protected Object get(String id, Map<String, String> query) throws Exception
   {
      NXCSession session = getSession();
      if (!session.isUserDatabaseSynchronized())
         session.syncUserDatabase();
      
      AbstractUserObject object;
      if (id.equals("self"))
      {
         object = session.findUserDBObjectById(session.getUserId(), null);
      }
      else
      {
         try
         {
            int objectId = Integer.parseInt(id);
            object = session.findUserDBObjectById(objectId, null);
         }
         catch(NumberFormatException e)
         {
            UUID objectGuid = UUID.fromString(id);
            object = session.findUserDBObjectByGUID(objectGuid);
         }
      }
      if (object == null)
         throw new NXCException(RCC.INVALID_USER_ID);
      return object;
   }
}
