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

import java.util.Map;
import java.util.UUID;
import javax.servlet.ServletContext;
import org.json.JSONException;
import org.json.JSONObject;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.RCC;
import org.netxms.websvc.SessionStore;
import org.netxms.websvc.SessionToken;
import org.netxms.websvc.WebSvcStatusService;
import org.netxms.websvc.json.JsonTools;
import org.restlet.Application;
import org.restlet.data.Header;
import org.restlet.data.MediaType;
import org.restlet.ext.json.JsonRepresentation;
import org.restlet.representation.Representation;
import org.restlet.representation.StringRepresentation;
import org.restlet.resource.Delete;
import org.restlet.resource.Get;
import org.restlet.resource.Post;
import org.restlet.resource.Put;
import org.restlet.resource.ServerResource;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import com.google.gson.JsonObject;

/**
 * Abstract handler implementation
 */
public abstract class AbstractHandler extends ServerResource
{
   public static String serverAddress = "127.0.0.1";
   private Logger log = LoggerFactory.getLogger(AbstractHandler.class);
   private SessionToken sessionToken = null;
   private NXCSession session = null;
   
   /**
    * Process GET requests
    * 
    * @return
    */
   @Get
   public Representation onGet() throws Exception
   {
      String id = getEntityId();
      log.debug("GET: entityId = " + id);
      if (attachToSession())
      {
         Object response = (id == null) ? getCollection(getRequest().getResourceRef().getQueryAsForm().getValuesMap()) : get(id);
         return new StringRepresentation(JsonTools.jsonFromObject(response), MediaType.APPLICATION_JSON);
      }
      else
      {
         return new StringRepresentation(createErrorResponse(RCC.ACCESS_DENIED).toString(), MediaType.APPLICATION_JSON);
      }
   }

   /**
    * Process POST requests
    * 
    * @param entity
    * @return
    */
   @Post
   public Representation onPost(Representation entity) throws Exception
   {
      if (entity == null)
      {
         log.warn("No POST data in call");
         return new StringRepresentation(createErrorResponse(RCC.ACCESS_DENIED).toString(), MediaType.APPLICATION_JSON);
      }
      
      JSONObject data = new JsonRepresentation(entity).getJsonObject();
      log.debug("POST: data = " + data);
      if (attachToSession())
      {
         return new StringRepresentation(JsonTools.jsonFromObject(create(data)), MediaType.APPLICATION_JSON);
      }
      else
      {
         return new StringRepresentation(createErrorResponse(RCC.ACCESS_DENIED).toString(), MediaType.APPLICATION_JSON);
      }
   }
   
   /**
    * Process PUT requests
    * 
    * @param entity
    * @return
    */
   @Put
   public Representation onPut(Representation entity) throws Exception
   {
      if (entity == null)
      {
         log.warn("No PUT data in call");
         return new StringRepresentation(createErrorResponse(RCC.ACCESS_DENIED).toString(), MediaType.APPLICATION_JSON);
      }
      
      JSONObject data = new JsonRepresentation(entity).getJsonObject();
      String id = getEntityId();
      log.debug("PUT: entityId = " + id);
      if (attachToSession())
      {
         Object response = (id != null) ? update(id, data) : createErrorResponse(RCC.INCOMPATIBLE_OPERATION);
         return new StringRepresentation(JsonTools.jsonFromObject(response), MediaType.APPLICATION_JSON);
      }
      else
      {
         return new StringRepresentation(createErrorResponse(RCC.ACCESS_DENIED).toString(), MediaType.APPLICATION_JSON);
      }
   }
   
   /**
    * Process DELETE requests
    * 
    * @param entity
    * @return
    */
   @Delete
   public Representation onDelete(Representation entity) throws Exception
   {
      String id = getEntityId();
      
      log.debug("DELETE: entityId = " + id);
      if (attachToSession())
      {    	 
         Object response = (id != null) ? delete(id) : createErrorResponse(RCC.INCOMPATIBLE_OPERATION);
         return new StringRepresentation(JsonTools.jsonFromObject(response), MediaType.APPLICATION_JSON);
      }
      else
      {
         return new StringRepresentation(createErrorResponse(RCC.ACCESS_DENIED).toString(), MediaType.APPLICATION_JSON);
      }
   }
   
   /**
    * Get entity ID
    * 
    * @return
    */
   protected String getEntityId()
   {
      return (String)getRequest().getAttributes().get("id");
   }
   
   /**
    * Attach this handler to server session
    * 
    * @return true if attached successfully
    */
   protected boolean attachToSession() throws Exception
   {
      SessionToken token = findSessionToken();
      if ((token == null) && (getRequest().getHeaders().getFirstValue("X-Login") != null))
      {
         String login = getRequest().getHeaders().getFirstValue("X-Login");
         String password = getRequest().getHeaders().getFirstValue("X-Password");
         log.debug("Cannot find session token - re-authenticating (login=" + login + ")");
         token = login(login, password);         
         getResponse().getHeaders().add(new Header("X-SessionId", token.getGuid().toString()));         
      }
      
      if (token != null)
      {
         log.debug("Handler attached to session " + token.getGuid());
         sessionToken = token;
         session = token.getSession();
      }
      else
      {
         log.debug("Session token not found and new session cannot be created");
      }
      return session != null;
   }
   
   /**
    * Find session token for this session
    * 
    * @return session token or null
    */
   protected SessionToken findSessionToken()
   {
      String cookie = getCookies().getValues("session_handle");
      if (cookie != null)
      {
         UUID guid = UUID.fromString(cookie);
         return SessionStore.getInstance(getServletContext()).getSessionToken(guid);
      }
      
      String sid = getRequest().getHeaders().getFirstValue("X-SessionId");
      log.debug("sid: " + sid);
      if (sid != null && !sid.equals("null") && !sid.equals("0"))
      {
         UUID guid = UUID.fromString(sid);
         return SessionStore.getInstance(getServletContext()).getSessionToken(guid);
      }
      
      return null;
   }
   
   /**
    * Get attached session
    * 
    * @return
    */
   protected NXCSession getSession()
   {
      return session;
   }
   
   /**
    * Get token of attached session
    * 
    * @return
    */
   protected SessionToken getSessionToken()
   {
      return sessionToken;
   }
   
   /**
    * Get servlet context
    * 
    * @return
    */
   protected ServletContext getServletContext()
   {
      Application app = Application.getCurrent();
      return (ServletContext)app.getContext().getAttributes().get("org.restlet.ext.servlet.ServletContext");
   }
   
   /**
    * Create error response.
    * 
    * @param httpError
    * @param rcc
    * @param message
    * @return
    * @throws JSONException
    */
   protected JsonObject createErrorResponse(int rcc) throws JSONException
   {
      setStatus(WebSvcStatusService.getStatusFromRCC(rcc));
      JsonObject response = new JsonObject();
      response.addProperty("error", rcc);
      response.addProperty("description", WebSvcStatusService.getMessageFromRCC(rcc));
      return response;
   }
   
   /**
    * Get collection. Default implementation will return INCOMPATIBLE OPERATION error.
    * 
    * @param query optional query
    * @return collection
    * @throws Exception
    */
   protected Object getCollection(Map<String, String> query) throws Exception
   {
      return createErrorResponse(RCC.INCOMPATIBLE_OPERATION);
   }

   /**
    * Get entity. Default implementation will return INCOMPATIBLE OPERATION error.
    * 
    * @param id entity id
    * @return entity as JSON
    * @throws Exception
    */
   protected Object get(String id) throws Exception
   {
      return createErrorResponse(RCC.INCOMPATIBLE_OPERATION);
   }

   /**
    * Create new entity. Default implementation will return INCOMPATIBLE OPERATION error.
    * 
    * @param data entity creation data
    * @return entity
    * @throws Exception
    */
   protected Object create(JSONObject data) throws Exception
   {
      return createErrorResponse(RCC.INCOMPATIBLE_OPERATION);
   }

   /**
    * Update entity. Default implementation will return INCOMPATIBLE OPERATION error.
    * 
    * @param id entity ID
    * @param data entity update data
    * @return entity
    * @throws Exception
    */
   protected Object update(String id, JSONObject data) throws Exception
   {
      return createErrorResponse(RCC.INCOMPATIBLE_OPERATION);
   }

   /**
    * Delete entity. Default implementation will return INCOMPATIBLE OPERATION error.
    * 
    * @param id entity id
    * @throws Exception
    */
   protected Object delete(String id) throws Exception
   {
      return createErrorResponse(RCC.INCOMPATIBLE_OPERATION);
   }
   
   /**
    * Login to NetXMS server
    * @param login
    * @param password
    * @return SessionToken
    */
   protected SessionToken login(String login, String password) throws Exception
   {
      session = new NXCSession(serverAddress);
      session.connect();
      session.login(login, (password == null) ? "" : password);
      return SessionStore.getInstance(getServletContext()).registerSession(session);
   }
}
