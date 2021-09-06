/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2016 Victor Kirhenshtein
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
package org.netxms.client.market;

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * NetXMS configuration repository information
 */
public class Repository
{
   private int id;
   private String url;
   private String authToken;
   private String description;
   
   /**
    * Create new repository object
    * 
    * @param url repository URL
    * @param authToken authentication token
    * @param description description
    */
   public Repository(String url, String authToken, String description)
   {
      this.id = 0;
      this.url = url;
      this.authToken = authToken;
      this.description = description;
   }
   
   /**
    * Create repository object from NXCP message
    * 
    * @param msg NXCP message
    * @param baseId base field ID
    */
   public Repository(NXCPMessage msg, long baseId)
   {
      id = msg.getFieldAsInt32(baseId);
      url = msg.getFieldAsString(baseId + 1);
      authToken = msg.getFieldAsString(baseId + 2);
      description = msg.getFieldAsString(baseId + 3);
   }
   
   /**
    * Fill NXCP message with repository data
    * 
    * @param msg NXCP message
    */
   public void fillMessage(NXCPMessage msg)
   {
      msg.setFieldInt32(NXCPCodes.VID_REPOSITORY_ID, id);
      msg.setField(NXCPCodes.VID_URL, url);
      msg.setField(NXCPCodes.VID_AUTH_TOKEN, authToken);
      msg.setField(NXCPCodes.VID_DESCRIPTION, description);
   }

   /**
    * @return the id
    */
   public int getId()
   {
      return id;
   }

   /**
    * @param id the id to set
    */
   public void setId(int id)
   {
      this.id = id;
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
    * @return the authToken
    */
   public String getAuthToken()
   {
      return authToken;
   }

   /**
    * @param authToken the authToken to set
    */
   public void setAuthToken(String authToken)
   {
      this.authToken = authToken;
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
}
