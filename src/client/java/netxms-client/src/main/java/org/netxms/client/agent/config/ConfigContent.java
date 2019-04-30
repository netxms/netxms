/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Raden Solutions
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
 * Agent configuration file content
 */
public class ConfigContent
{
   private long id;
   private String config;
   private String filter;
   private String name;
   private long sequenceNumber;

   /**
    * Default constructior
    */
   public ConfigContent()
   {
      id = 0;
      config = "";
      filter = "";
      name = "New config";
      sequenceNumber = -1;
   }

   /**
    * Constructs object from message
    *
    * @param id  Configuration policy ID
    * @param msg Incoming message to be parsed
    */
   public ConfigContent(long id, NXCPMessage msg)
   {
      this.id = id;
      name = msg.getFieldAsString(NXCPCodes.VID_NAME) == null ? "" : msg.getFieldAsString(NXCPCodes.VID_NAME);
      config = msg.getFieldAsString(NXCPCodes.VID_CONFIG_FILE) == null ? "" : msg.getFieldAsString(NXCPCodes.VID_CONFIG_FILE);
      filter = msg.getFieldAsString(NXCPCodes.VID_FILTER) == null ? "" : msg.getFieldAsString(NXCPCodes.VID_FILTER);
      sequenceNumber = msg.getFieldAsInt64(NXCPCodes.VID_SEQUENCE_NUMBER);
   }

   /**
    * TODO
    *
    * @param request TODO
    */
   public void fillMessage(NXCPMessage request)
   {
      request.setFieldInt32(NXCPCodes.VID_CONFIG_ID, (int)id);
      request.setField(NXCPCodes.VID_NAME, name);
      request.setField(NXCPCodes.VID_CONFIG_FILE, config);
      request.setField(NXCPCodes.VID_FILTER, filter);
      request.setFieldInt32(NXCPCodes.VID_SEQUENCE_NUMBER, (int)sequenceNumber);
   }

   /**
    * @return the id
    */
   public long getId()
   {
      return id;
   }

   /**
    * @param id the id to set
    */
   public void setId(long id)
   {
      this.id = id;
   }

   /**
    * @return the config
    */
   public String getConfig()
   {
      return config;
   }

   /**
    * @param config the config to set
    */
   public void setConfig(String config)
   {
      this.config = config;
   }

   /**
    * @return the filter
    */
   public String getFilter()
   {
      return filter;
   }

   /**
    * @param filter the filter to set
    */
   public void setFilter(String filter)
   {
      this.filter = filter;
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
    * @return the sequenceNumber
    */
   public long getSequenceNumber()
   {
      return sequenceNumber;
   }

   /**
    * @param sequenceNumber the sequenceNumber to set
    */
   public void setSequenceNumber(long sequenceNumber)
   {
      this.sequenceNumber = sequenceNumber;
   }

}
