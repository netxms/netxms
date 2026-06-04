/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Victor Kirhenshtein
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
package org.netxms.client;

import java.util.Date;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * Represents NetXMS server's event forwarder
 */
public class EventForwarder
{
   public static final int SEND_STATUS_UNKNOWN = 0;
   public static final int SEND_STATUS_SUCCESS = 1;
   public static final int SEND_STATUS_FAILURE = 2;

   private String name;
   private String description;
   private String driverName;
   private String configuration;
   private boolean driverInitialized;
   private String errorMessage;
   private int sendStatus;
   private boolean healthCheckStatus;
   private Date lastMessageTimestamp;
   private int messageCount;
   private int failureCount;
   private int queueSize;
   private int droppedCount;

   /**
    * Create event forwarder object from NXCP message
    *
    * @param msg NXCP message
    * @param baseId base field ID
    */
   protected EventForwarder(final NXCPMessage msg, long baseId)
   {
      name = msg.getFieldAsString(baseId);
      description = msg.getFieldAsString(baseId + 1);
      driverName = msg.getFieldAsString(baseId + 2);
      configuration = msg.getFieldAsString(baseId + 3);
      driverInitialized = msg.getFieldAsBoolean(baseId + 4);
      errorMessage = msg.getFieldAsString(baseId + 5);
      sendStatus = msg.getFieldAsInt32(baseId + 6);
      healthCheckStatus = msg.getFieldAsBoolean(baseId + 7);
      lastMessageTimestamp = msg.getFieldAsDate(baseId + 8);
      messageCount = msg.getFieldAsInt32(baseId + 9);
      failureCount = msg.getFieldAsInt32(baseId + 10);
      queueSize = msg.getFieldAsInt32(baseId + 11);
      droppedCount = msg.getFieldAsInt32(baseId + 12);
   }

   /**
    * Create empty event forwarder object.
    */
   public EventForwarder()
   {
      name = "";
      description = "";
      driverName = null;
      configuration = "";
      driverInitialized = false;
   }

   /**
    * Fill NXCP message with event forwarder's data
    *
    * @param msg NXCP message
    */
   public void fillMessage(final NXCPMessage msg)
   {
      msg.setField(NXCPCodes.VID_NAME, name);
      msg.setField(NXCPCodes.VID_DESCRIPTION, description);
      msg.setField(NXCPCodes.VID_DRIVER_NAME, driverName);
      msg.setField(NXCPCodes.VID_XML_CONFIG, configuration);
   }

   /**
    * Get event forwarder name.
    *
    * @return event forwarder name
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
    * @return true if driver instance is initialized
    */
   public boolean isDriverInitialized()
   {
      return driverInitialized;
   }

   /**
    * @return the errorMessage
    */
   public String getErrorMessage()
   {
      return errorMessage;
   }

   /**
    * @return the sendStatus
    */
   public int getSendStatus()
   {
      return sendStatus;
   }

   /**
    * @return the healthCheckStatus
    */
   public boolean getHealthCheckStatus()
   {
      return healthCheckStatus;
   }

   /**
    * @return the lastMessageTimestamp
    */
   public Date getLastMessageTimestamp()
   {
      return lastMessageTimestamp;
   }

   /**
    * @return the messageCount
    */
   public int getMessageCount()
   {
      return messageCount;
   }

   /**
    * @return the failureCount
    */
   public int getFailureCount()
   {
      return failureCount;
   }

   /**
    * @return the droppedCount
    */
   public int getDroppedCount()
   {
      return droppedCount;
   }

   /**
    * Get current queue size.
    *
    * @return current queue size
    */
   public int getQueueSize()
   {
      return queueSize;
   }
}
