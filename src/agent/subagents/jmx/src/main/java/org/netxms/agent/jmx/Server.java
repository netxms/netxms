/**
 * JMX subagent
 * Copyright (C) 2013-2023 Raden Solutions
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
package org.netxms.agent.jmx;

import java.io.IOException;
import java.net.MalformedURLException;
import java.util.HashMap;
import java.util.Map;
import java.util.Set;
import javax.management.AttributeNotFoundException;
import javax.management.MBeanAttributeInfo;
import javax.management.MBeanInfo;
import javax.management.MBeanServerConnection;
import javax.management.ObjectName;
import javax.management.remote.JMXConnector;
import javax.management.remote.JMXConnectorFactory;
import javax.management.remote.JMXServiceURL;
import org.netxms.bridge.Platform;

/**
 * JMX server
 */
public class Server
{
   private static final String DEBUG_TAG = "jmx";

   private String name;
   private String url;
   private String login;
   private String password;
   JMXConnector jmxc = null;
   MBeanServerConnection mbsc = null;

   /**
    * @param url
    * @param login
    * @param password
    */
   public Server(String name, String url, String login, String password)
   {
      this.name = name;
      this.url = url;
      this.login = login;
      this.password = password;
   }
   
   /**
    * @return the name
    */
   public String getName()
   {
      return name;
   }

   /**
    * @return the url
    */
   public String getUrl()
   {
      return url;
   }

   /**
    * Connect to server
    * 
    * @throws MalformedURLException
    * @throws IOException
    */
   private void connect() throws MalformedURLException, IOException
   {
      if (jmxc == null)
      {
         try
         {
            Map<String, Object> env = new HashMap<String, Object>();
            if ((login != null) && (password != null))
            {
               env.put(JMXConnector.CREDENTIALS, new String[] { login, password });
            }
            env.put(JMXConnectorFactory.PROTOCOL_PROVIDER_CLASS_LOADER, getClass().getClassLoader());
            jmxc = JMXConnectorFactory.connect(new JMXServiceURL(url), env);
            mbsc = null;
         }
         catch(Exception e)
         {
            Platform.writeDebugLog(DEBUG_TAG, 5, "Cannot setup JMX connection to " + url + ": " + e.getClass().getCanonicalName() + ": " + e.getMessage());
            Platform.writeDebugLog(DEBUG_TAG, 5, "   ", e);
            throw e;
         }
      }

      if (mbsc == null)
      {
         try
         {
            mbsc = jmxc.getMBeanServerConnection();
            Platform.writeDebugLog(DEBUG_TAG, 5, "JMX connection established to " + url);
         }
         catch(Exception e)
         {
            Platform.writeDebugLog(DEBUG_TAG, 5, "Cannot get MBean server connection for " + url + ": " + e.getClass().getCanonicalName() + ": " + e.getMessage());
            Platform.writeDebugLog(DEBUG_TAG, 5, "   ", e);
            jmxc.close();
            jmxc = null;
            throw e;
         }
      }
   }

   /**
    * Disconnect from server 
    */
   private void disconnect()
   {
      if (jmxc != null)
      {
         try
         {
            jmxc.close();
         }
         catch(Exception e)
         {
            Platform.writeDebugLog(DEBUG_TAG, 6, "Exception in disconnect() call for " + url + ": " + e.getClass().getCanonicalName() + ": " + e.getMessage());
            Platform.writeDebugLog(DEBUG_TAG, 6, "   ", e);
         }
         jmxc = null;
      }
      mbsc = null;
   }

   /**
    * Get available domains
    * 
    * @return
    * @throws Exception
    */
   public synchronized String[] getDomains() throws Exception
   {
      connect();
      try
      {
         return mbsc.getDomains();
      }
      catch(IOException e)
      {
         disconnect();
         throw e;
      }
   }

   /**
    * Get all objects in given domain
    * 
    * @param domain
    * @return
    * @throws Exception
    */
   public synchronized String[] getObjects(String domain) throws Exception
   {
      connect();
      try
      {
         if ((domain == null) || domain.isEmpty())
            domain = "*";
         Set<ObjectName> names = mbsc.queryNames(new ObjectName(domain + ":*"), null);
         String objects[] = new String[names.size()];
         int i = 0;
         for (ObjectName n : names)
         {
            objects[i++] = n.getCanonicalName();
         }
         return objects;
      }
      catch(IOException e)
      {
         disconnect();
         throw e;
      }
   }

   /**
    * Get attributes of given object
    * 
    * @param object
    * @return
    * @throws Exception
    */
   public synchronized String[] getObjectAttributes(String object) throws Exception
   {
      connect();
      try
      {
         MBeanInfo info = mbsc.getMBeanInfo(new ObjectName(object));
         MBeanAttributeInfo[] mbAttributes = info.getAttributes();
         int count = mbAttributes.length;
         String[] attributes = new String[count];
         for(int i = 0; i < count; i++)
         {
            attributes[i] = mbAttributes[i].getName();
         }
         return attributes;
      }
      catch(IOException e)
      {
         disconnect();
         throw e;
      }
   }
   
   /**
    * Get value of given attribute
    * 
    * @param object
    * @param attribute
    * @return
    * @throws Exception
    */
   public synchronized Object getAttributeValue(String object, String attribute) throws Exception
   {
      connect();
      try
      {
         return mbsc.getAttribute(new ObjectName(object), attribute);
      }
      catch(AttributeNotFoundException e)
      {
         return null;
      }
      catch(IOException e)
      {
         disconnect();
         throw e;
      }
   }

   /**
    * Get value of given attribute as string
    * 
    * @param object
    * @param attribute
    * @return
    * @throws Exception
    */
   public synchronized String getAttributeValueAsString(String object, String attribute) throws Exception
   {
      connect();
      try
      {
         Platform.writeDebugLog(DEBUG_TAG, 6, String.format("Reading JMX object %s attribute %s", object, attribute));
         return mbsc.getAttribute(new ObjectName(object), attribute).toString();
      }
      catch(AttributeNotFoundException e)
      {
         return null;
      }
      catch(IOException e)
      {
         disconnect();
         throw e;
      }
   }
}
