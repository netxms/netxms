/**
 * 
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
            Map<String, Object> env;
            if ((login != null) && (password != null))
            {
               env = new HashMap<String, Object>();
               env.put(JMXConnector.CREDENTIALS, new String[] { login, password });
            }
            else
            {
               env = null;
            }
            jmxc = JMXConnectorFactory.connect(new JMXServiceURL(url), env);
            mbsc = null;
         }
         catch(Exception e)
         {
            Platform.writeDebugLog(5, "JMX: cannot setup connection to " + url);
            Platform.writeDebugLog(5, "JMX:   ", e);
            throw e;
         }
      }
      
      if (mbsc == null)
      {
         try
         {
            mbsc = jmxc.getMBeanServerConnection();
            Platform.writeDebugLog(5, "JMX: connected to " + url);
         }
         catch(Exception e)
         {
            Platform.writeDebugLog(5, "JMX: cannot get MBean server connection for " + url);
            Platform.writeDebugLog(5, "JMX:   ", e);
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
            Platform.writeDebugLog(6, "JMX: exception in disconnect() call for " + url);
            Platform.writeDebugLog(6, "JMX:   ", e);
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
         Platform.writeDebugLog(6, String.format("JMX: reading object %s attribute %s", object, attribute));
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
