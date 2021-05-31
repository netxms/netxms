/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * Represents two-factor authentication method
 */
public class TwoFactorAuthenticationMethod
{
   private String name;
   private String description;
   private String driver;
   private String configuration;
   private boolean loaded;

   /**
    * Create new method.
    *
    * @param name method name
    * @param description method description
    * @param driver two-factor authentication driver to use
    * @param configuration method configuration
    */
   public TwoFactorAuthenticationMethod(String name, String description, String driver, String configuration)
   {
      this.name = name;
      this.description = description;
      this.driver = driver;
      this.configuration = configuration;
      this.loaded = false;
   }

   /**
    * Get method definition from NXCP message.
    *
    * @param msg NXCP message
    */
   public TwoFactorAuthenticationMethod(NXCPMessage msg)
   {
      name = msg.getFieldAsString(NXCPCodes.VID_NAME);
      description = msg.getFieldAsString(NXCPCodes.VID_DESCRIPTION);
      driver = msg.getFieldAsString(NXCPCodes.VID_DRIVER_NAME);
      configuration = msg.getFieldAsString(NXCPCodes.VID_CONFIG_FILE_DATA);
      loaded = msg.getFieldAsBoolean(NXCPCodes.VID_IS_ACTIVE);
   }

   /**
    * Get method definition from NXCP message.
    *
    * @param msg NXCP message
    * @param baseId base field ID
    */
   public TwoFactorAuthenticationMethod(NXCPMessage msg, long baseId)
   {
      name = msg.getFieldAsString(baseId);
      description = msg.getFieldAsString(baseId + 1);
      driver = msg.getFieldAsString(baseId + 2);
      configuration = msg.getFieldAsString(baseId + 3);
      loaded = msg.getFieldAsBoolean(baseId + 4);
   }

   /**
    * Fill NXCP message
    *
    * @param msg NXCP message
    */
   public void fillMessage(NXCPMessage msg)
   {
      msg.setField(NXCPCodes.VID_NAME, name);
      msg.setField(NXCPCodes.VID_DESCRIPTION, description);
      msg.setField(NXCPCodes.VID_DRIVER_NAME, driver);
      msg.setField(NXCPCodes.VID_CONFIG_FILE_DATA, configuration);
   }

   /**
    * Get method's description.
    *
    * @return method's description
    */
   public String getDescription()
   {
      return description;
   }

   /**
    * Set description.
    *
    * @param description new description
    */
   public void setDescription(String description)
   {
      this.description = description;
   }

   /**
    * Get driver name.
    *
    * @return driver name
    */
   public String getDriver()
   {
      return driver;
   }

   /**
    * Set driver name.
    *
    * @param driver new driver name
    */
   public void setDriver(String driver)
   {
      this.driver = driver;
   }

   /**
    * Get method configuration (can be null if not received from server).
    *
    * @return method configuration or null
    */
   public String getConfiguration()
   {
      return configuration;
   }

   /**
    * Set method configuration
    *
    * @param configuration new configuration
    */
   public void setConfiguration(String configuration)
   {
      this.configuration = configuration;
   }

   /**
    * Get method name.
    *
    * @return method name
    */
   public String getName()
   {
      return name;
   }

   /**
    * Check if method is loaded and can be used for authentication.
    *
    * @return true if method is loaded
    */
   public boolean isLoaded()
   {
      return loaded;
   }
}
