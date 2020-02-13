/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Victor Kirhenshtein
 * <p>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * <p>
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * <p>
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.client.datacollection;

import java.util.HashMap;
import java.util.Map;
import java.util.UUID;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.constants.WebServiceAuthType;

/**
 * Web service definition
 */
public class WebServiceDefinition
{
   private int id;
   private UUID guid;
   private String name;
   private String description;
   private String url;
   private WebServiceAuthType authenticationType;
   private String login;
   private String password;
   private int cacheRetentionTime;
   private int requestTimeout;
   private Map<String, String> headers;

   /**
    * Create new definition. Definition object will be created with random GUID and ID 0.
    * 
    * @param name name for new definition
    */
   public WebServiceDefinition(String name)
   {
      id = 0;
      guid = UUID.randomUUID();
      this.name = name;
      description = "";
      url = "";
      authenticationType = WebServiceAuthType.NONE;
      login = null;
      password = null;
      cacheRetentionTime = 0;
      requestTimeout = 0;
      headers = new HashMap<String, String>();
   }

   /**
    * Create definition from NXCP message
    * 
    * @param msg NXCP message
    */
   public WebServiceDefinition(NXCPMessage msg)
   {
      id = msg.getFieldAsInt32(NXCPCodes.VID_WEBSVC_ID);
      guid = msg.getFieldAsUUID(NXCPCodes.VID_GUID);
      name = msg.getFieldAsString(NXCPCodes.VID_NAME);
      description = msg.getFieldAsString(NXCPCodes.VID_DESCRIPTION);
      url = msg.getFieldAsString(NXCPCodes.VID_URL);
      authenticationType = WebServiceAuthType.getByValue(msg.getFieldAsInt32(NXCPCodes.VID_AUTH_TYPE));
      login = msg.getFieldAsString(NXCPCodes.VID_LOGIN_NAME);
      password = msg.getFieldAsString(NXCPCodes.VID_PASSWORD);
      cacheRetentionTime = msg.getFieldAsInt32(NXCPCodes.VID_RETENTION_TIME);
      requestTimeout = msg.getFieldAsInt32(NXCPCodes.VID_TIMEOUT);
      headers = msg.getStringMapFromFields(NXCPCodes.VID_HEADERS_BASE, NXCPCodes.VID_NUM_HEADERS);
   }

   /**
    * Fill NXCP message with object data
    * 
    * @param msg NXCP message
    */
   public void fillMessage(NXCPMessage msg)
   {
      msg.setFieldInt32(NXCPCodes.VID_WEBSVC_ID, id);
      msg.setField(NXCPCodes.VID_GUID, guid);
      msg.setField(NXCPCodes.VID_NAME, name);
      msg.setField(NXCPCodes.VID_DESCRIPTION, description);
      msg.setField(NXCPCodes.VID_URL, url);
      msg.setFieldInt16(NXCPCodes.VID_AUTH_TYPE, authenticationType.getValue());
      msg.setField(NXCPCodes.VID_LOGIN_NAME, login);
      msg.setField(NXCPCodes.VID_PASSWORD, password);
      msg.setFieldInt32(NXCPCodes.VID_RETENTION_TIME, cacheRetentionTime);
      msg.setFieldInt32(NXCPCodes.VID_TIMEOUT, requestTimeout);
      msg.setFieldsFromStringMap(headers, NXCPCodes.VID_HEADERS_BASE, NXCPCodes.VID_NUM_HEADERS);
   }

   /**
    * Get headers. Collection returned is actual header set and any changes to it will affect web service definition object.
    * 
    * @return defined headers
    */
   public Map<String, String> getHeaders()
   {
      return headers;
   }

   /**
    * Get all header names. Returned array is a copy of actual header set.
    * 
    * @return array containing all header names
    */
   public String[] getHeaderNames()
   {
      return headers.keySet().toArray(new String[headers.size()]);
   }

   /**
    * Get value of given header.
    * 
    * @param name header name
    * @return header value or null
    */
   public String getHeader(String name)
   {
      return headers.get(name);
   }

   /**
    * Set header value.
    * 
    * @param name header name
    * @param value new header value
    */
   public void setHeader(String name, String value)
   {
      headers.put(name, value);
   }

   /**
    * Remove header.
    * 
    * @param name header name
    */
   public void removeHeader(String name)
   {
      headers.remove(name);
   }

   /**
    * Get web service definition ID.
    * 
    * @return web service definition ID
    */
   public int getId()
   {
      return id;
   }

   /**
    * Set web service definition ID.
    * 
    * @param id new web service definition ID
    */
   public void setId(int id)
   {
      this.id = id;
   }

   /**
    * @return the guid
    */
   public UUID getGuid()
   {
      return guid;
   }

   /**
    * @param guid the guid to set
    */
   public void setGuid(UUID guid)
   {
      this.guid = guid;
   }

   /**
    * @return the name
    */
   public String getName()
   {
      return name;
   }

   /**
    * @param name the name to set
    */
   public void setName(String name)
   {
      this.name = name;
   }

   /**
    * @return the description
    */
   public String getDescription()
   {
      return description;
   }

   /**
    * @param description the description to set
    */
   public void setDescription(String description)
   {
      this.description = description;
   }

   /**
    * @return the url
    */
   public String getUrl()
   {
      return url;
   }

   /**
    * @param url the url to set
    */
   public void setUrl(String url)
   {
      this.url = url;
   }

   /**
    * @return the authenticationType
    */
   public WebServiceAuthType getAuthenticationType()
   {
      return authenticationType;
   }

   /**
    * @param authenticationType the authenticationType to set
    */
   public void setAuthenticationType(WebServiceAuthType authenticationType)
   {
      this.authenticationType = authenticationType;
   }

   /**
    * @return the login
    */
   public String getLogin()
   {
      return login;
   }

   /**
    * @param login the login to set
    */
   public void setLogin(String login)
   {
      this.login = login;
   }

   /**
    * @return the password
    */
   public String getPassword()
   {
      return password;
   }

   /**
    * @param password the password to set
    */
   public void setPassword(String password)
   {
      this.password = password;
   }

   /**
    * @return the cacheRetentionTime
    */
   public int getCacheRetentionTime()
   {
      return cacheRetentionTime;
   }

   /**
    * @param cacheRetentionTime the cacheRetentionTime to set
    */
   public void setCacheRetentionTime(int cacheRetentionTime)
   {
      this.cacheRetentionTime = cacheRetentionTime;
   }

   /**
    * @return the requestTimeout
    */
   public int getRequestTimeout()
   {
      return requestTimeout;
   }

   /**
    * @param requestTimeout the requestTimeout to set
    */
   public void setRequestTimeout(int requestTimeout)
   {
      this.requestTimeout = requestTimeout;
   }
}
