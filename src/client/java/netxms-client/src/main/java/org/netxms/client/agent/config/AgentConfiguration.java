/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Raden Solutions
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
package org.netxms.client.agent.config;

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * Agent configuration defined on server
 */
public class AgentConfiguration
{
   private long id;
   private String name;
   private long sequenceNumber;
   private String filter;
   private String content;

   /**
    * Default constructor
    */
   public AgentConfiguration()
   {
      id = 0;
      name = "New configuration";
      sequenceNumber = -1;
      filter = "";
      content = "";
   }

   /**
    * Constructs object from incoming message
    *
    * @param id Configuration object ID
    * @param msg Incoming message to be parsed
    */
   public AgentConfiguration(long id, NXCPMessage msg)
   {
      this.id = id;
      name = msg.getFieldAsString(NXCPCodes.VID_NAME);
      sequenceNumber = msg.getFieldAsInt64(NXCPCodes.VID_SEQUENCE_NUMBER);
      filter = msg.getFieldAsString(NXCPCodes.VID_FILTER);
      content = msg.getFieldAsString(NXCPCodes.VID_CONFIG_FILE);
   }

   /**
    * Fill NXCP message with object data
    *
    * @param message NXCP message
    */
   public void fillMessage(NXCPMessage message)
   {
      message.setFieldInt32(NXCPCodes.VID_CONFIG_ID, (int)id);
      message.setField(NXCPCodes.VID_NAME, name);
      message.setField(NXCPCodes.VID_CONFIG_FILE, content);
      message.setField(NXCPCodes.VID_FILTER, filter);
      message.setFieldInt32(NXCPCodes.VID_SEQUENCE_NUMBER, (int)sequenceNumber);
   }

   /**
    * Get configuration ID
    * 
    * @return configuration ID
    */
   public long getId()
   {
      return id;
   }

   /**
    * Set configuration ID.
    * 
    * @param id new configuration ID
    */
   public void setId(long id)
   {
      this.id = id;
   }

   /**
    * Get configuration file content
    * 
    * @return configuration file content
    */
   public String getContent()
   {
      return (content != null) ? content : "";
   }

   /**
    * Set configuration file content.
    * 
    * @param content new configuration file content
    */
   public void setContent(String content)
   {
      this.content = content;
   }

   /**
    * Get filter script.
    * 
    * @return filter script
    */
   public String getFilter()
   {
      return (filter != null) ? filter : "";
   }

   /**
    * Set filter script
    * 
    * @param filter new filter script
    */
   public void setFilter(String filter)
   {
      this.filter = filter;
   }

   /**
    * Get configuration name.
    * 
    * @return configuration name
    */
   public String getName()
   {
      return (name != null) ? name : "";
   }

   /**
    * Set configuration name.
    * 
    * @param name new configuration name
    */
   public void setName(String name)
   {
      this.name = name;
   }

   /**
    * Get configuration sequence number (priority).
    * 
    * @return configuration sequence number
    */
   public long getSequenceNumber()
   {
      return sequenceNumber;
   }

   /**
    * Set configuration sequence number (priority).
    * 
    * @param sequenceNumber new configuration sequence number
    */
   public void setSequenceNumber(long sequenceNumber)
   {
      this.sequenceNumber = sequenceNumber;
   }
}
