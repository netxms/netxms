/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2018 Raden Solutions
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

import org.json.JSONObject;
import org.netxms.client.SessionNotification;
import org.netxms.client.constants.RCC;
import org.restlet.data.MediaType;
import org.restlet.representation.Representation;
import org.restlet.representation.StringRepresentation;
import org.restlet.resource.Get;

/**
 * Notification handler
 */
public class NotificationHandler extends AbstractHandler
{
   
   /* (non-Javadoc)
    * @see org.netxms.websvc.handlers.AbstractHandler#onGet()
    */
   @Override
   @Get
   public Representation onGet() throws Exception
   {
      if (!attachToSession())
         return new StringRepresentation(createErrorResponse(RCC.ACCESS_DENIED).toString(), MediaType.APPLICATION_JSON);
      
      SessionNotification n = getSessionToken().pollNotificationQueue(30);      
      if (n == null)
         return new StringRepresentation(createErrorResponse(RCC.TIMEOUT).toString(), MediaType.APPLICATION_JSON);

      JSONObject response = new JSONObject();
      response.put("code", n.getCode());
      response.put("subcode", n.getSubCode());
      return new StringRepresentation(response.toString(), MediaType.APPLICATION_JSON);
   }
}
