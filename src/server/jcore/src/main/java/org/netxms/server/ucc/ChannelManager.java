/**
 * 
 */
package org.netxms.server.ucc;

import java.util.HashMap;
import java.util.Map;
import org.netxms.bridge.Platform;
import org.netxms.server.ServerConfiguration;
import org.netxms.server.ucc.drivers.FileChannel;

/**
 * User communication channel manager
 */
public final class ChannelManager
{
   private static final String DEBUG_TAG = "ucc.manager";
   
   private static Map<String, Class<? extends UserCommunicationChannel>> builtinDrivers;
   private static Map<String, UserCommunicationChannel> channels = new HashMap<String, UserCommunicationChannel>();
   
   static
   {
      builtinDrivers = new HashMap<String, Class<? extends UserCommunicationChannel>>();
      builtinDrivers.put("builtin.file", FileChannel.class);
   }
   
   /**
    * Configure communication channel
    * 
    * @param id channel ID
    * @param driver channel driver
    */
   private static void configureChannel(String id, String driver)
   {
      Platform.writeDebugLog(DEBUG_TAG, 5, "Configuring channel " + id + " (driver " + driver + ")");
      Class<? extends UserCommunicationChannel> driverClass = builtinDrivers.get(driver);
      if (driverClass != null)
      {
         Platform.writeDebugLog(DEBUG_TAG, 5, "Found built-in driver class " + driverClass.getName());
      }
      else
      {
         // Attempt to find given class
         try
         {
            driverClass = Class.forName(driver).asSubclass(UserCommunicationChannel.class);
         }
         catch(ClassNotFoundException e)
         {
            Platform.writeDebugLog(DEBUG_TAG, 2, "Driver class " + driver + " not found");
            return;
         }
         catch(ClassCastException e)
         {
            Platform.writeDebugLog(DEBUG_TAG, 2, "Driver class " + driver + " is invalid");
            return;
         }
      }
      
      try
      {
         UserCommunicationChannel channel = driverClass.getDeclaredConstructor(String.class).newInstance(id);
         channel.initialize();
         channels.put(id, channel);
         Platform.writeDebugLog(DEBUG_TAG, 5, "Channel " + id + " initialized successfully");
      }
      catch(Exception e)
      {
         Platform.writeDebugLog(DEBUG_TAG, 1, "Exception during channel " + id + " initialization (" + e.getClass().getName() + "):");
         Platform.writeDebugLog(DEBUG_TAG, 1, "   ", e);
      }
   }
   
   /**
    * Initialize channel manager
    * 
    * @return
    */
   public static boolean initialize()
   {
      Platform.writeDebugLog(DEBUG_TAG, 5, "Starting channel manager initialization");
      try
      {
         String[] channels = ServerConfiguration.readAsString("UCC.Channels", "EMAIL=builtin.smtp").split(";"); 
         for(String c : channels)
         {
            String[] parts = c.split("=");
            if (parts.length == 2)
            {
               String id = parts[0].trim();
               String driver = parts[1].trim();
               configureChannel(id, driver);
            }
            else
            {
               Platform.writeDebugLog(DEBUG_TAG, 2, "Invalid channel configuration element \"" + c + "\"");
            }
         }
         return true;
      }
      catch(Throwable e)
      {
         Platform.writeDebugLog(DEBUG_TAG, 1, "Exception during channel manager initialization (" + e.getClass().getName() + "):");
         Platform.writeDebugLog(DEBUG_TAG, 1, "   ", e);
         return false;
      }
   }
}
