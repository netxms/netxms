/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
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
package org.netxms.nxmc;

import java.io.IOException;
import java.io.InputStream;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.Properties;
import java.util.function.BiConsumer;
import javax.naming.InitialContext;
import org.eclipse.rap.rwt.RWT;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import jakarta.servlet.ServletContext;

/**
 * Loader class for application properties. Will try to find properties in JNDI, nxmc.properties file, JVM options, and process
 * environment. Properties passed as JVM options should have names prefixed with "nxmc.". Properties passed as environment variables
 * should have names prefixed with "NXMC_" and converted to uppercase.
 */
public class AppPropertiesLoader
{
   private static final Logger logger = LoggerFactory.getLogger(AppPropertiesLoader.class);

   private Properties properties;
   private String resolvedServerAddress = null;
   private boolean resolveServerNameCompleted = false;

   /**
    * Create new instance of property loader
    */
   public AppPropertiesLoader()
   {
      properties = loadPropertyFile();
   }

   /**
    * Get property
    *
    * @param name property name
    * @return property value or default value
    */
   public String getProperty(String name)
   {
      return getProperty(name, null);
   }

   /**
    * Get property
    *
    * @param name property name
    * @param defaultValue default value if property is not set
    * @return property value or default value
    */
   public String getProperty(String name, String defaultValue)
   {
      try
      {
         InitialContext context = new InitialContext();
         return context.lookup("java:comp/env/nxmc/" + name).toString();
      }
      catch(Exception e)
      {
      }

      String value = properties.getProperty(name);
      if (value != null)
         return value;

      // Check JVM options
      value = System.getProperties().getProperty("nxmc." + name);
      if (value != null)
         return value;

      // Check environment
      value = System.getenv("NXMC_" + name.toUpperCase());
      if (value != null)
         return value;

      // Special case for server name - attempt to resolve DNS name NETXMS_SERVER
      if (name.equals("server"))
      {
         if (!resolveServerNameCompleted)
         {
            try
            {
               InetAddress addr = InetAddress.getByName("NETXMS_SERVER");
               resolvedServerAddress = addr.getHostAddress();
               logger.info("Server address resolved from NETXMS_SERVER: " + resolvedServerAddress);
            }
            catch(UnknownHostException e)
            {
            }
            resolveServerNameCompleted = true;
         }

         if (resolvedServerAddress != null)
            return resolvedServerAddress;
      }

      return defaultValue;
   }

   /**
    * Get property as integer
    *
    * @param name property name
    * @param defaultValue default value if property is not set
    * @return property value or default value
    */
   public int getPropertyAsInteger(String name, int defaultValue)
   {
      String value = getProperty(name, null);
      if (value == null)
         return defaultValue;

      try
      {
         return Integer.parseInt(value);
      }
      catch(NumberFormatException e)
      {
         return defaultValue;
      }
   }

   /**
    * Get property as boolean
    *
    * @param name property name
    * @param defaultValue default value if property is not set
    * @return property value or default value
    */
   public boolean getPropertyAsBoolean(String name, boolean defaultValue)
   {
      String value = getProperty(name, null);
      return (value != null) ? Boolean.parseBoolean(value) : defaultValue;
   }

   /**
    * Set property to given value.
    *
    * @param name Property name
    * @param value New property value
    */
   public void setProperty(String name, String value)
   {
      properties.setProperty(name, value);
   }

	/**
	 * Read application properties from nxmc.properties, Java VM properties, and environment
	 */
	private Properties loadPropertyFile()
	{
		Properties properties = new Properties();
		InputStream in = null;
		try
		{
			in = getClass().getResourceAsStream("/nxmc.properties"); //$NON-NLS-1$
			if (in == null)
			{
				ServletContext context = RWT.getRequest().getSession().getServletContext();
				if (context != null)
				{
					in = context.getResourceAsStream("/nxmc.properties"); //$NON-NLS-1$
				}
			}
			if (in != null)
			{
				properties.load(in);
            logger.info("Properties loaded from nxmc.properties");
				properties.forEach(new BiConsumer<Object, Object>() {
               @Override
               public void accept(Object t, Object u)
               {
                  logger.info("   " + t + " = " + u);
               }
            });
			}
         else
         {
            logger.info("Property file nxmc.properties not found in classpath");
         }
		}
		catch(Exception e)
		{
         logger.warn("Cannot load property file nxmc.properties", e);
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
		return properties;
	}
}
