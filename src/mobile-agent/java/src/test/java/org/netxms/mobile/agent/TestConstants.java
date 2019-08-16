package org.netxms.mobile.agent;

import java.io.FileInputStream;
import java.io.InputStream;
import java.util.Properties;


public class TestConstants
{   
   //Server connection constants
   public static String serverAddress = "127.0.0.1";
   public static int serverPort = Session.DEFAULT_CONN_PORT;
   public static String loginName = "admin";
   public static String password = "";
   //Other constants
   public static String DEVICE_ID = "0000000000";
   //Reinitialize variables from properties file
   public static TestConstants testConstants = new TestConstants();
   
   public TestConstants()
   {
      try
      {
         String propFile = "/etc/test.properties";
         if(System.getProperty("ConfigFile") != null && !System.getProperty("ConfigFile").isEmpty())
            propFile = System.getProperty("ConfigFile");
         InputStream stream = new FileInputStream(propFile);
         Properties properties = new Properties();
         properties.load(stream);
         serverAddress = properties.getProperty("server.address", "127.0.0.1");
         loginName = properties.getProperty("login.name", "admin");
         password = properties.getProperty("password", "");
         DEVICE_ID = properties.getProperty("device.id", "0000000000");
         serverPort = Integer.parseInt(properties.getProperty("mobile.server.port", Integer.toString(Session.DEFAULT_CONN_PORT)));
      }
      catch (Exception e)
      {
         System.out.println("No properties file found: " + e.getMessage());
      }
   }
}

/******************************************************************
 *Documentation about test server configuration
 ******************************************************************
 *On server side should exist mobile device with ID as DEVICE_ID
 ******************************************************************/
