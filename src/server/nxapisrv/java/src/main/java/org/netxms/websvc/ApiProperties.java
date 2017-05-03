package org.netxms.websvc;

import java.io.IOException;
import java.io.InputStream;
import java.util.Properties;

public class ApiProperties
{
   private Properties properties;
   
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
            properties.load(in);
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
    * @return the address of the NetXMS server
    */
   public String getNXServerAddress()
   {
      return properties.getProperty("NXServer", "127.0.0.1");
   }
}