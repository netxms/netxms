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

import java.nio.charset.Charset;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import java.util.UUID;
import javax.servlet.ServletContext;
import javax.xml.bind.DatatypeConverter;
import org.json.JSONException;
import org.json.JSONObject;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.RCC;
import org.netxms.websvc.ApiProperties;
import org.netxms.websvc.SessionStore;
import org.netxms.websvc.SessionToken;
import org.netxms.websvc.WebSvcStatusService;
import org.netxms.websvc.json.JsonTools;
import org.restlet.Application;
import org.restlet.data.CookieSetting;
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
   static ApiProperties properties = new ApiProperties();
   protected static String serverAddress = properties.getNXServerAddress();
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
         Map<String, String> query = getRequest().getResourceRef().getQueryAsForm().getValuesMap();
         Object response = (id == null) ? getCollection(query) : get(id, query);
         return new StringRepresentation(JsonTools.jsonFromObject(response, getRequestedFields()), MediaType.APPLICATION_JSON);
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
         return new StringRepresentation(JsonTools.jsonFromObject(create(data), null), MediaType.APPLICATION_JSON);
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
         return new StringRepresentation(JsonTools.jsonFromObject(response, null), MediaType.APPLICATION_JSON);
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
         return new StringRepresentation(JsonTools.jsonFromObject(response, null), MediaType.APPLICATION_JSON);
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
      return (String)getRequest().getAttributes().get(getEntityIdFieldName());
   }
   
   /**
    * Get name of entity ID field
    * 
    * @return entity ID field
    */
   protected String getEntityIdFieldName()
   {
      return "id";
   }
   
   /**
    * Decode strings from Base64
    * 
    * @param data coded in base64
    * @return decoded String value
    */
   protected String decodeBase64(String data)
   {
      if (data != null && !data.equals("0"))
      {
         String value = data.substring(data.indexOf(' ') + 1, data.length());
         return new String(DatatypeConverter.parseBase64Binary(value), Charset.forName("UTF-8"));
      }
      return null;
   }
   
   /**
    * Attach this handler to server session
    * 
    * @return true if attached successfully
    */
   protected boolean attachToSession() throws Exception
   {
      SessionToken token = findSessionToken();
      if ((token == null) && (getHeader("Authorization") != null))
      {
         String value = decodeBase64(getHeader("Authorization"));
         String login = value.substring(0, value.indexOf(':'));
         String password = value.substring(value.indexOf(':') + 1, value.length());
         log.debug("Cannot find session token - re-authenticating (login=" + login + ")");
         token = login(login, password);
         getCookieSettings().add(new CookieSetting(0, "session_handle", token.getGuid().toString(), "/", null));
         getResponse().getHeaders().add(new Header("Session-Id", token.getGuid().toString()));
      }
      else if (token != null)
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
         log.debug("session_handle: " + cookie);
         UUID guid = UUID.fromString(cookie);
         return SessionStore.getInstance(getServletContext()).getSessionToken(guid);
      }
      
      String sid = getHeader("Session-Id");
      if (sid != null && !sid.equals("0"))
      {
         log.debug("Session-Id: " + sid);
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
   protected Object get(String id, Map<String, String> query) throws Exception
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
   
   /**
    * Get set of field names specified in "fields" parameter
    * 
    * @return set of field names or null
    */
   private Set<String> getRequestedFields()
   {
      String fieldList = getRequest().getResourceRef().getQueryAsForm().getValuesMap().get("fields");
      if ((fieldList == null) || fieldList.isEmpty())
         return null;
      
      String[] parts = fieldList.split(",");
      if (parts.length == 0)
         return null;
      
      Set<String> fields = new HashSet<String>();
      for(String f : parts)
         fields.add(f);
      return fields;
   }
   
   /**
    * Get header by name
    * 
    * @param name name of header 
    * @return value of requested header
    */   
   protected String getHeader(String name)
   {
      return getRequest().getHeaders().getFirstValue(name, true);
   }
   
   /**
    * Parse string as Integer value
    * 
    * @param value input string
    * @param defaultValue default value to return if parse fails
    * @return value parsed as integer or default value
    */
   static protected int parseInt(String value, int defaultValue)
   {
      if (value == null)
         return defaultValue;
      
      try 
      {
         return Integer.parseInt(value);
      }
      catch(NumberFormatException e)
      {
         return defaultValue;
      }      
   }
   
   /**
    * Parse string as Long value
    * 
    * @param value input string
    * @param defaultValue default value to return if parse fails
    * @return value parsed as long or default value
    */
   static protected long parseLong(String value, long defaultValue)
   {
      if (value == null)
         return defaultValue;
      
      try 
      {
         return Long.parseLong(value);
      }
      catch(NumberFormatException e)
      {
         return defaultValue;
      }      
   }
}
