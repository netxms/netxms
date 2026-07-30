/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.client;

import java.util.Date;
import java.util.HashMap;
import java.util.Map;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * Chat bot configuration and status
 */
public class ChatBot
{
   private String name;
   private String description;
   private String driverName;
   private String configuration;
   private boolean driverActive;
   private boolean healthCheckStatus;
   private int idleTimeout;
   private String providerSlot;
   private int activeSessionCount;
   private Date lastInboundMessageTime;
   private String errorMessage;
   private Map<String, Integer> userMappings;

   /**
    * Create new chat bot object.
    */
   public ChatBot()
   {
      name = "";
      description = "";
      driverName = null;
      configuration = "";
      driverActive = false;
      healthCheckStatus = false;
      idleTimeout = 0;
      providerSlot = "";
      activeSessionCount = 0;
      lastInboundMessageTime = null;
      errorMessage = "";
      userMappings = new HashMap<String, Integer>();
   }

   /**
    * Create chat bot object from NXCP message.
    *
    * @param msg NXCP message
    * @param baseId base field ID
    */
   protected ChatBot(NXCPMessage msg, long baseId)
   {
      name = msg.getFieldAsString(baseId);
      description = msg.getFieldAsString(baseId + 1);
      driverName = msg.getFieldAsString(baseId + 2);
      configuration = msg.getFieldAsString(baseId + 3);
      driverActive = msg.getFieldAsBoolean(baseId + 4);
      healthCheckStatus = msg.getFieldAsBoolean(baseId + 5);
      idleTimeout = msg.getFieldAsInt32(baseId + 6);
      providerSlot = msg.getFieldAsString(baseId + 7);
      activeSessionCount = msg.getFieldAsInt32(baseId + 8);
      lastInboundMessageTime = msg.getFieldAsDate(baseId + 9);
      errorMessage = msg.getFieldAsString(baseId + 10);
      userMappings = new HashMap<String, Integer>();
      int count = msg.getFieldAsInt32(baseId + 11);
      long fieldId = baseId + 12;
      for(int i = 0; i < count; i++)
      {
         String peerId = msg.getFieldAsString(fieldId++);
         int userId = msg.getFieldAsInt32(fieldId++);
         userMappings.put(peerId, userId);
      }
   }

   /**
    * Fill NXCP message with chat bot configuration data.
    *
    * @param msg NXCP message
    */
   public void fillMessage(NXCPMessage msg)
   {
      msg.setField(NXCPCodes.VID_NAME, name);
      msg.setField(NXCPCodes.VID_DESCRIPTION, description);
      msg.setField(NXCPCodes.VID_DRIVER_NAME, driverName);
      msg.setField(NXCPCodes.VID_XML_CONFIG, configuration);
      msg.setFieldInt32(NXCPCodes.VID_TIMEOUT, idleTimeout);
      msg.setField(NXCPCodes.VID_AI_MODEL_SLOT, providerSlot);
      msg.setFieldInt32(NXCPCodes.VID_NUM_RECORDS, userMappings.size());
      long fieldId = NXCPCodes.VID_ELEMENT_LIST_BASE;
      for(Map.Entry<String, Integer> e : userMappings.entrySet())
      {
         msg.setField(fieldId++, e.getKey());
         msg.setFieldInt32(fieldId++, e.getValue());
      }
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
    * @return the driverName
    */
   public String getDriverName()
   {
      return driverName;
   }

   /**
    * @param driverName the driverName to set
    */
   public void setDriverName(String driverName)
   {
      this.driverName = driverName;
   }

   /**
    * @return the configuration
    */
   public String getConfiguration()
   {
      return configuration;
   }

   /**
    * @param configuration the configuration to set
    */
   public void setConfiguration(String configuration)
   {
      this.configuration = configuration;
   }

   /**
    * @return true if driver instance was successfully created and started
    */
   public boolean isDriverActive()
   {
      return driverActive;
   }

   /**
    * @return last health check status
    */
   public boolean isHealthy()
   {
      return healthCheckStatus;
   }

   /**
    * @return session idle timeout in seconds
    */
   public int getIdleTimeout()
   {
      return idleTimeout;
   }

   /**
    * @param idleTimeout session idle timeout in seconds
    */
   public void setIdleTimeout(int idleTimeout)
   {
      this.idleTimeout = idleTimeout;
   }

   /**
    * @return AI provider slot for bot sessions (empty string for default)
    */
   public String getProviderSlot()
   {
      return providerSlot;
   }

   /**
    * @param providerSlot AI provider slot for bot sessions (empty string for default)
    */
   public void setProviderSlot(String providerSlot)
   {
      this.providerSlot = providerSlot;
   }

   /**
    * @return number of currently active chat sessions
    */
   public int getActiveSessionCount()
   {
      return activeSessionCount;
   }

   /**
    * @return time of last inbound message (null if none received)
    */
   public Date getLastInboundMessageTime()
   {
      return lastInboundMessageTime;
   }

   /**
    * @return last error message
    */
   public String getErrorMessage()
   {
      return errorMessage;
   }

   /**
    * Get user mappings (platform peer ID to NetXMS user ID).
    *
    * @return user mappings (live map, can be modified by caller)
    */
   public Map<String, Integer> getUserMappings()
   {
      return userMappings;
   }

   /**
    * Set user mappings (platform peer ID to NetXMS user ID).
    *
    * @param userMappings new user mappings
    */
   public void setUserMappings(Map<String, Integer> userMappings)
   {
      this.userMappings = userMappings;
   }
}
