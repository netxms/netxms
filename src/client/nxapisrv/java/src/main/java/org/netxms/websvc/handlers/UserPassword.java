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

import java.util.UUID;
import org.json.JSONObject;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.RCC;
import org.netxms.client.users.AbstractUserObject;

/**
 * User password management
 */
public class UserPassword extends AbstractHandler
{
   /**
    * @see org.netxms.websvc.handlers.AbstractHandler#create(org.json.JSONObject)
    */
   @Override
   protected Object create(JSONObject data) throws Exception
   {
      NXCSession session = getSession();

      String id = getEntityId();
      if (id == null)
         throw new NXCException(RCC.INVALID_USER_ID);

      int userId;
      if (id.equals("self"))
      {
         userId = session.getUserId();
      }
      else
      {
         try
         {
            userId = Integer.parseInt(id);
         }
         catch(NumberFormatException e)
         {
            UUID objectGuid = UUID.fromString(id);
            if (!session.isUserDatabaseSynchronized())
               session.syncUserDatabase();
            AbstractUserObject object = session.findUserDBObjectByGUID(objectGuid);
            if (object == null)
               throw new NXCException(RCC.INVALID_USER_ID);
            userId = object.getId();
         }
      }
      
      if (!data.has("newPassword"))
         throw new NXCException(RCC.INVALID_ARGUMENT);
      session.setUserPassword(userId, data.getString("newPassword"), data.has("oldPassword") ? data.getString("oldPassword") : null);
      return new Object();
   }
}
