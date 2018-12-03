package org.netxms.websvc;

import java.io.IOException;
import java.io.InputStream;
import java.util.Properties;

public class ApiProperties
{
   private Properties properties;
   private String host = "127.0.0.1";
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
         e.printStackTrace();
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
