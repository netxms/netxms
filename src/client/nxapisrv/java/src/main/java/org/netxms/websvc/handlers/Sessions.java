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
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.constants.RCC;
import org.netxms.websvc.SessionStore;
import org.netxms.websvc.SessionToken;
import org.netxms.websvc.json.JsonTools;
import org.restlet.data.CookieSetting;
import org.restlet.data.Header;
import org.restlet.data.MediaType;
import org.restlet.ext.json.JsonRepresentation;
import org.restlet.representation.Representation;
import org.restlet.representation.StringRepresentation;
import org.restlet.resource.Get;
import org.restlet.resource.Post;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import java.io.IOException;

/**
 * Session requests handler
 */
public class Sessions extends AbstractHandler
{
   private Logger log = LoggerFactory.getLogger(Sessions.class);
   
   /**
    * @see org.netxms.websvc.handlers.AbstractHandler#onGet()
    */
   @Override
   @Get
   public Representation onGet() throws Exception
   {
      String id = getEntityId();
      if (attachToSession())
      {
         if ("self".equals(id))
         {
            return new StringRepresentation(JsonTools.jsonFromObject(getSession(), null), MediaType.APPLICATION_JSON);
         }
         else
         {
            return new StringRepresentation(createErrorResponse(RCC.NOT_IMPLEMENTED).toString(), MediaType.APPLICATION_JSON);
         }
      }
      else
      {
         return new StringRepresentation(createErrorResponse("self".equals(id) ? RCC.INVALID_SESSION_HANDLE : RCC.ACCESS_DENIED).toString(), MediaType.APPLICATION_JSON);
      }
   }

   /**
    * @see org.netxms.websvc.handlers.AbstractHandler#onPost(org.restlet.representation.Representation)
    */
   @Override
   @Post
   public Representation onPost(Representation entity) throws Exception
   {
      String login = null;
      String password = null;
      boolean attachNotificationHandler = false;
      if (entity != null)
      {
         JSONObject request = new JsonRepresentation(entity).getJsonObject();
         if (request.has("login"))
            login = request.getString("login");
         if (request.has("password"))
            password = request.getString("password");
         if (request.has("attachNotificationHandler"))
            attachNotificationHandler = request.getBoolean("attachNotificationHandler");
      }

      if (entity == null || (login == null && password == null))
      {
         log.warn("No POST data in login call, looking for authentication data instead...");
         String authHeader = getHeader("Authorization");
         if ((authHeader != null) && !authHeader.isEmpty())
         {
            String[] values = decodeBase64(authHeader).split(":", 2);
            if (values.length == 2)
            {
               login = values[0];
               password = values[1];
            }
         }
      }
      
      if ((login == null) || (password == null))
      {
         log.warn("Login or password not specified in login call");
         return new StringRepresentation(createErrorResponse(RCC.INVALID_REQUEST).toString(), MediaType.APPLICATION_JSON);
      }

      SessionToken token = login(login, password);
      if (attachNotificationHandler)
         attachNotificationHandler(token);

      log.info("Logged in to NetXMS server, assigned session id " + token.getSessionHandle());
      getCookieSettings().add(new CookieSetting(0, "session_handle", token.getSessionHandle().toString(), "/", null));
      getResponse().getHeaders().add(new Header("Session-Id", token.getSessionHandle().toString()));
      
      return new StringRepresentation(JsonTools.jsonFromObject(token, null), MediaType.APPLICATION_JSON);
   }

   /**
    * @see org.netxms.websvc.handlers.AbstractHandler#delete(java.lang.String)
    */
   @Override
   protected Object delete(String id) throws Exception
   {
      if (getSessionToken().getSessionHandle().toString().equals(id) || "self".equals(id))
      {
         log.info("Logout request for session " + id);
         getSession().disconnect();
         SessionStore.getInstance(getServletContext()).unregisterSession(getSessionToken().getSessionHandle());
         return new JSONObject();
      }
      else
      {
         log.warn("Logout request for different session (" + getSessionToken().getSessionHandle() + " -- " + id);
         return createErrorResponse(RCC.ACCESS_DENIED);
      }
   }
   
   /**
    * Atatch notification handler to session
    */
   private void attachNotificationHandler(final SessionToken token) throws NXCException, IOException
   {
      SessionListener listener = new SessionListener() {
         @Override
         public void notificationHandler(SessionNotification n)
         {
            token.addNotificationToQueue(n);
         }
      };
      token.getSession().addListener(listener);
      token.getSession().subscribe(NXCSession.CHANNEL_ALARMS);
   }
}
