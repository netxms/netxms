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
package org.netxms.websvc;

import java.io.IOException;
import java.io.InputStream;
import java.util.Properties;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class ApiProperties
{
   private Logger log = LoggerFactory.getLogger(ApiProperties.class);
   private Properties properties;
   private String host = "127.0.0.1";
   private int port = 4701;
   private boolean enableCompression;
   private long sessionTimeout = 300000;

   /**
    * Class for reading API properties file
    */
   public ApiProperties()
   {
      properties = new Properties();
      InputStream in = null;
      try
      {
         in = getClass().getResourceAsStream("/nxapisrv.properties");
         if (in != null)
         {
            properties.load(in);
            host = properties.getProperty("netxms.server.address", "127.0.0.1");
            port = getIntProperty("netxms.server.port", 4701);
            enableCompression = getBooleanProperty("netxms.server.enableCompression", true);
            sessionTimeout = getIntProperty("session.timeout", 300) * 1000;
         }
      }
      catch(Exception e)
      {
         log.warn("Cannot open property file", e);
      }
      finally
      {
         if (in != null)
         {
            try
            {
               in.close();
            }
            catch(IOException e)
            {
            }
         }
      }
   }
   
   /**
    * Get property value as integer
    * 
    * @param name
    * @param defaultValue
    * @return
    */
   private int getIntProperty(String name, int defaultValue)
   {
      try
      {
         String v = properties.getProperty(name);
         if (v == null)
            return defaultValue;
         return Integer.parseInt(v);
      }
      catch(NumberFormatException e)
      {
         return defaultValue;
      }
   }
   
   /**
    * Get property value as boolean
    * 
    * @param name
    * @param defaultValue
    * @return
    */
   private boolean getBooleanProperty(String name, boolean defaultValue)
   {
      String v = properties.getProperty(name);
      if (v == null)
         return defaultValue;
      return Boolean.parseBoolean(v);
   }

   /**
    * @return the address of the NetXMS server
    */
   public String getServerAddress()
   {
      return host;
   }
   
   /**
    * Get NetXMS server port
    * 
    * @return NetXMS server port
    */
   public int getServerPort()
   {
      return port;
   }

   /**
    * Check if compression for NXCP is enabled.
    *
    * @return true if compression for NXCP is enabled
    */
   public boolean isCompressionEnabled()
   {
      return enableCompression;
   }

   /**
    * Get session inactivity timeout in milliseconds.
    *
    * @return session inactivity timeout in milliseconds
    */
   public long getSessionTimeout()
   {
      return sessionTimeout;
   }
}
