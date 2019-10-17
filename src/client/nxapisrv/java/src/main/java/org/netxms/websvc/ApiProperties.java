/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2019 Raden Solutions
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
   private String host = "10.5.5.43"; //"127.0.0.1";
   private int port = 4701;
   
   /**
    * Class for reading API properties file
    */
   public ApiProperties()
   {
      properties = new Properties();
      InputStream in = null;
      try
      {
         in = getClass().getResourceAsStream("/nxapisrv.properties"); //$NON-NLS-1$
         if (in != null)
         {
            properties.load(in);
            host = properties.getProperty("netxms.server.address", "127.0.0.1");
            port = getIntProperty("netxms.server.port", 4701);
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
}
