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

import java.util.UUID;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.RCC;
import org.netxms.client.objects.AbstractObject;

/**
 * Abstract handler for everything below /objects
 */
public class AbstractObjectHandler extends AbstractHandler
{
   /**
    * Get object from URL entity ID part.
    * 
    * @return object
    * @throws Exception if object cannot be found or request is invalid
    */
   protected AbstractObject getObject() throws Exception
   {
      NXCSession session = getSession();
      if (!session.areObjectsSynchronized())
         session.syncObjects();

      String entityId = (String)getRequest().getAttributes().get("object-id");
      AbstractObject object;
      try
      {
         long objectId = Long.parseLong(entityId);
         object = session.findObjectById(objectId);
      }
      catch(NumberFormatException e)
      {
         UUID objectGuid = UUID.fromString(entityId);
         object = session.findObjectByGUID(objectGuid);
      }
      if (object == null)
         throw new NXCException(RCC.INVALID_OBJECT_ID);
      return object;
   }

   /**
    * Get object ID (validated) from URL entity ID part.
    * 
    * @return object ID
    * @throws Exception if object cannot be found or request is invalid
    */
   protected long getObjectId() throws Exception
   {
      return getObject().getObjectId();
   }

   /**
    * Get object GUID (validated) from URL entity ID part.
    * 
    * @return object GUID
    * @throws Exception if object cannot be found or request is invalid
    */
   protected UUID getObjectGuid() throws Exception
   {
      return getObject().getGuid();
   }
}
