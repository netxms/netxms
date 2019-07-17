/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * Represents NetXMS server's action
 *
 */
public class NotificationChannel
{
	private String name;
	private String description;
	private String driverName;
	private String configuration;
	private boolean isActive;
	private NCConfigurationTemplate configurationTemplate;
	
	/**
	 * Create server action object from NXCP message
	 * 
	 * @param msg NXCP message
	 */
	protected NotificationChannel(final NXCPMessage msg, long base)
	{
		name = msg.getFieldAsString(base);
		description = msg.getFieldAsString(base+1);
		driverName = msg.getFieldAsString(base+2);
		configuration = msg.getFieldAsString(base+3);
		isActive = msg.getFieldAsBoolean(base+4);
		configurationTemplate = new NCConfigurationTemplate(msg, base+5);
	}
	
	public NotificationChannel()
   {
	   name = "";
	   description = "";
	   driverName = null;
	   configuration = "";
	   isActive = false;
	   configurationTemplate = null;
   }

   /**
	 * Fill NXCP message with action's data
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
    * @return the configurationTemplate
    */
   public NCConfigurationTemplate getConfigurationTemplate()
   {
      return configurationTemplate;
   }

   /**
    * @return the isActive
    */
   public boolean isActive()
   {
      return isActive;
   }
}
