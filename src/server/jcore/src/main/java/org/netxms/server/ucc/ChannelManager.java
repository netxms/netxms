/**
 * 
 */
package org.netxms.server.ucc;

import java.util.HashMap;
import java.util.Map;
import org.netxms.bridge.Platform;
import org.netxms.server.ServerConfiguration;
import org.netxms.server.ucc.drivers.FileChannel;
import org.netxms.server.ucc.drivers.SmtpChannel;

/**
 * User communication channel manager
 */
public final class ChannelManager
{
   private static final String DEBUG_TAG = "ucc.manager";
   
   private static Map<String, Class<? extends UserCommunicationChannel>> builtinDrivers;
   private static Map<String, UserCommunicationChannel> channels = new HashMap<String, UserCommunicationChannel>();
   private static ChannelListener channelListener;
   
   static
   {
      builtinDrivers = new HashMap<String, Class<? extends UserCommunicationChannel>>();
      builtinDrivers.put("builtin.file", FileChannel.class);
      builtinDrivers.put("builtin.smtp", SmtpChannel.class);
      channelListener = new ChannelListener() {
         @Override
         public void messageReceived(String sender, String text)
         {
         }
      };
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
         channel.addListener(channelListener);
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
   
   /**
    * Send message via given channel
    * 
    * @param channelId channel ID
    * @param recipient recipient address
    * @param subject message subject
    * @param text message text
    * @return true on success
    */
   public static boolean send(String channelId, String recipient, String subject, String text)
   {
      UserCommunicationChannel channel = channels.get(channelId);
      if (channel == null)
      {
         Platform.writeDebugLog(DEBUG_TAG, 2, "Cannot send message to " + recipient + " on channel " + channelId + ": channel does not exist");
         return false;
      }
      
      try
      {
         channel.send(recipient, subject, text);
      }
      catch(Exception e)
      {
         Platform.writeDebugLog(DEBUG_TAG, 1, "Exception in channel " + channelId + " (" + e.getClass().getName() + "):");
         Platform.writeDebugLog(DEBUG_TAG, 1, "   ", e);
         return false;
      }
      Platform.writeDebugLog(DEBUG_TAG, 5, "Sent message to " + recipient + " on channel " + channelId);
      return true;
   }
}
