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

import org.json.JSONObject;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.RCC;
import org.netxms.client.constants.UserAccessRights;
import org.restlet.data.MediaType;
import org.restlet.ext.json.JsonRepresentation;
import org.restlet.representation.Representation;
import org.restlet.representation.StringRepresentation;
import org.restlet.resource.Post;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * External integration tool authentication handler
 */
public class AccessIntegrationTools extends AbstractHandler
{
   private Logger log = LoggerFactory.getLogger(AccessIntegrationTools.class);
   
   /* (non-Javadoc)
    * @see org.netxms.websvc.handlers.AbstractHandler#onPost(org.restlet.representation.Representation)
    */
   @Override
   @Post
   public Representation onPost(Representation entity) throws Exception
   {
      int rc = RCC.ACCESS_DENIED;
      if (entity != null)
      {
         JSONObject request = new JsonRepresentation(entity).getJsonObject();
         String login = request.getString("login");
         String password = request.getString("password");
         if ((login != null) || (password != null))
         {
            NXCSession session = new NXCSession(serverAddress);
            session.connect();
            session.login(login, (password == null) ? "" : password);
            
            if ((session.getUserSystemRights() & UserAccessRights.SYSTEM_ACCESS_EXTERNAL_INTEGRATION) != 0)
               rc = RCC.SUCCESS;
            
            session.disconnect();
         }
         else
         {
            log.debug("Login or password not specified in login call");
            rc = RCC.INVALID_REQUEST;
         }
      }
      else
      {
         log.debug("No POST data in login call");
         rc = RCC.INVALID_REQUEST;
      }

      return new StringRepresentation(createErrorResponse(rc).toString(), MediaType.APPLICATION_JSON);
   }
}
