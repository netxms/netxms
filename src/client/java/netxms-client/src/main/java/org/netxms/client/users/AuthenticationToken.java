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
package org.netxms.client.users;

import java.util.Date;
import org.netxms.base.NXCPMessage;

/**
 * Authentication token information.
 */
public class AuthenticationToken
{
   private int id;
   private int userId;
   private boolean persistent;
   private String description;
   private String value;
   private Date creationTime;
   private Date expirationTime;
   private boolean singleUse;

   /**
    * Create token information object from NXCP message.
    *
    * @param msg NXCP message
    * @param baseId base field ID
    */
   public AuthenticationToken(NXCPMessage msg, long baseId)
   {
      id = msg.getFieldAsInt32(baseId);
      userId = msg.getFieldAsInt32(baseId + 1);
      persistent = msg.getFieldAsBoolean(baseId + 2);
      description = msg.getFieldAsString(baseId + 3);
      value = msg.getFieldAsString(baseId + 4);
      creationTime = msg.getFieldAsDate(baseId + 5);
      expirationTime = msg.getFieldAsDate(baseId + 6);
      singleUse = msg.getFieldAsBoolean(baseId + 8);
   }

   /**
    * @return the id
    */
   public int getId()
   {
      return id;
   }

   /**
    * @return the userId
    */
   public int getUserId()
   {
      return userId;
   }

   /**
    * @return the persistent
    */
   public boolean isPersistent()
   {
      return persistent;
   }

   /**
    * @return the description
    */
   public String getDescription()
   {
      return description;
   }

   /**
    * Get token value. Available for as long as the server keeps the token in memory, which covers newly issued tokens and any
    * live ephemeral, single-use or persistent token issued since the last server restart. Persistent tokens reloaded from the
    * database no longer carry it.
    *
    * @return token value (can be null or empty string if unavailable)
    */
   public String getValue()
   {
      return value;
   }

   /**
    * @return the creationTime
    */
   public Date getCreationTime()
   {
      return creationTime;
   }

   /**
    * @return the expirationTime
    */
   public Date getExpirationTime()
   {
      return expirationTime;
   }

   /**
    * Check if this token is single-use. Such token is destroyed by the login it authenticates and cannot be presented as REST
    * bearer credential.
    *
    * @return true if this token is single-use
    */
   public boolean isSingleUse()
   {
      return singleUse;
   }
}
